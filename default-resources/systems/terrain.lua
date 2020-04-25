-- function used for creating a sorted iterator of k,v pairs in a table
-- we use it here to sort our terrain generators by priority
-- taken from https://stackoverflow.com/questions/15706270/sort-a-table-in-lua
function spairs(t, order)
    -- collect the keys
    local keys = {}
    for k in pairs(t) do keys[#keys+1] = k end

    -- if order function given, sort by it by passing the table and keys a, b,
    -- otherwise just sort the keys 
    if order then
        table.sort(keys, function(a,b) return order(t, a, b) end)
    else
        table.sort(keys)
    end

    -- return the iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end


return {
	init = function(self, world)
		-- create or find our seed
		local seedArchetype = archetype.new({
			"Seed"
		})
		if seedArchetype:isEmpty() then
			local index,_ = seedArchetype:createEntity()
			seedArchetype:getComponents("Seed")[index].seed = 1337
			self.seed = 1337
		else
			self.seed = seedArchetype:getComponents("Seed")[1].seed
		end

		local terrainGens = {}
		for key,filename in pairs(getResources("terrain", ".lua")) do
			-- Note we strip the ".lua"  from the filename
			local generator = require(filename:sub(1,filename:len()-4))
			generator:init(world, self.seed)
			table.insert(terrainGens, generator)
		end
		-- sort our terrain generators by priority
		-- we only need the priority for this sorting step,
		-- so we store the generators in an array so we don't need to sort them for every generated chunk
		self.terrainGens = {}
		for _,generator in spairs(terrainGens, function(t, a, b) return t[a].priority < t[b].priority end) do
			table.insert(self.terrainGens, generator)
		end

		local chunkArchetype = archetype.new({
			"Chunk"
		})
		self.chunkComponents = chunkArchetype:getComponents("Chunk")
		-- TODO is storing 3D array of chunk indices going to be okay?
		self.chunks = {}
		if chunkArchetype:isEmpty() then
			debugger.addLog(debugLevels.Info, "about to start terrain gen")
			local index,_ = chunkArchetype:createEntities((2 * world.renderers.voxel.loadDistance) ^ 3)
			local currIndex = index
			for x = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
				self.chunks[x] = {}
				for y = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
					self.chunks[x][y] = {}
					for z = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
						self:fillChunk(world, x, y, z, currIndex)
						self.chunks[x][y][z] = currIndex
						currIndex = currIndex + 1
					end
				end
			end
			debugger.addLog(debugLevels.Info, "finished fill step")
			currIndex = index
			for x = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
				for y = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
					for z = -world.renderers.voxel.loadDistance, world.renderers.voxel.loadDistance - 1 do
						self:createMesh(world, x, y, z, currIndex)
						currIndex = currIndex + 1
					end
				end
			end
			debugger.addLog(debugLevels.Info, "finished mesh step")
		end
	end,
	fillChunk = function(self, world, x, y, z, index)
		local chunk = self.chunkComponents[index]
		local chunkSize = world.renderers.voxel.chunkSize

		chunk.minBounds = vec3.new(x * chunkSize, y * chunkSize, z * chunkSize)
		chunk.maxBounds = vec3.new((x + 1) * chunkSize, (y + 1) * chunkSize, (z + 1) * chunkSize)
		chunk.blocks = {}

		-- run our terrain generators
		for i,generator in ipairs(self.terrainGens) do
			generator:generateChunk(world, chunk, x, y, z)
		end
	end,
	doesBlockExist = function(self, x, y, z, point)
		if self.chunks[x] ~= nil and self.chunks[x][y] ~= nil and self.chunks[x][y][z] ~= nil then
			return self.chunkComponents[self.chunks[x][y][z]].blocks[point] ~= nil
		end
		return false
	end,
	createMesh = function(self, world, x, y, z, index)
		local chunk = self.chunkComponents[index]

		local chunkSize = world.renderers.voxel.chunkSize
		local chunkSizeSquared = chunkSize ^ 2
		local axisSize = chunkSize - 1

		-- currently chunk.blocks maps points to their archetype
		-- we want a map of archetypes to the points using it
		local archetypes = {}
		for point,archetype in pairs(chunk.blocks) do
			if archetypes[archetype] == nil then
				archetypes[archetype] = {}
			end
			table.insert(archetypes[archetype], point)
		end

		-- create the block entities and add their faces to a mesh
		local vertices = {}
		local indices = {}
		local numVertices = 0
		local numIndices = 0
		for archetype, points in pairs(archetypes) do
			local firstId, firstIndex = archetype:createEntities(#points)
			local block = archetype:getSharedComponent("Block")
			for _, point in pairs(points) do
				-- local[XYZ] are the coordinates of this block inside this chunk
				local localX = math.floor(point / chunkSizeSquared)
				local localY = math.floor((point % chunkSizeSquared) / chunkSize)
				local localZ = point % chunkSize
				-- global[XYZ] are the coordinates of this block in global space
				-- remember x, y, and z are the chunk's position
				local globalX = localX + x * chunkSize
				local globalY = localY + y * chunkSize
				local globalZ = localZ + z * chunkSize

				-- for each face, we only add it to our mesh if 
				-- 1. its an exterior block and the block doesn't exist in the adjacent chunk (or if that chunk isn't loaded)
				-- 2. its an interior block and the adjacent block doesn't exist
				-- We need to check if its interior or exterior twice because lua doesn't support ternaries,
				-- and the standard "{condition} and X or Y" doesn't work if X is falsey (as it would be in our case)
				-- Unfortunately that makes this block of code look pretty messy, hence this explanation
				if (localY == axisSize and not self:doesBlockExist(x, y + 1, z, point - axisSize * chunkSize)) or (localY ~= axisSize and chunk.blocks[point + chunkSize] == nil) then -- top
					self.addFace(globalX, globalY + 1, globalZ, globalX + 1, globalY + 1, globalZ + 1, 0, 1, 0, block.top, true, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
				if (localY == 0 and not self:doesBlockExist(x, y - 1, z, point + axisSize * chunkSize)) or (localY ~= 0 and chunk.blocks[point - chunkSize] == nil) then -- bottom
					self.addFace(globalX, globalY, globalZ, globalX + 1, globalY, globalZ + 1, 0, -1, 0, block.bottom, false, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
				if (localZ == axisSize and not self:doesBlockExist(x, y, z + 1, point - axisSize)) or (localZ ~= axisSize and chunk.blocks[point + 1] == nil) then -- back
					self.addFace(globalX + 1, globalY + 1, globalZ + 1, globalX, globalY, globalZ + 1, 0, 0, 1, block.back, false, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
				if (localZ == 0 and not self:doesBlockExist(x, y, z - 1, point + axisSize)) or (localZ ~= 0 and chunk.blocks[point - 1] == nil) then -- front
					self.addFace(globalX, globalY, globalZ, globalX + 1, globalY + 1, globalZ, 0, 0, -1, block.front, true, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
				if (localX == axisSize and not self:doesBlockExist(x + 1, y, z, point - axisSize * chunkSizeSquared)) or (localX ~= axisSize and chunk.blocks[point + chunkSizeSquared] == nil) then -- left
					self.addFace(globalX + 1, globalY, globalZ, globalX + 1, globalY + 1, globalZ + 1, 1, 0, 0, block.left, true, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
				if (localX == 0 and not self:doesBlockExist(x - 1, y, z, point + axisSize * chunkSizeSquared)) or (localX ~= 0 and chunk.blocks[point - chunkSizeSquared] == nil) then -- right
					self.addFace(globalX, globalY, globalZ + 1, globalX, globalY + 1, globalZ, -1, 0, 0, block.right, true, vertices, indices, numVertices)
					numVertices = numVertices + 4
					numIndices = numIndices + 6
				end
			end
		end

		chunk.indexCount = numIndices
		if numIndices > 0 then
			-- build our vertex and index buffers
			chunk.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, numVertices * 8 * sizes.Float)
			chunk.vertexBuffer:setDataFloats(vertices)
			chunk.indexBuffer = buffer.new(bufferUsage.IndexBuffer, numIndices * sizes.Float)
			chunk.indexBuffer:setDataInts(indices)
		end
	end,
	addFace = function(x1, y1, z1, x2, y2, z2, nX, nY, nZ, UVs, isClockWise, vertices, indices, numVertices)
		-- vertex 0
		table.insert(vertices, x1)
		table.insert(vertices, y1)
		table.insert(vertices, z1)
		table.insert(vertices, nX)
		table.insert(vertices, nY)
		table.insert(vertices, nZ)
		table.insert(vertices, UVs.s)
		table.insert(vertices, UVs.t)
		-- vertex 1
		if y1 == y2 then
			table.insert(vertices, x1)
			table.insert(vertices, y1)
			table.insert(vertices, z2)
		else
			table.insert(vertices, x1)
			table.insert(vertices, y2)
			table.insert(vertices, z1)
		end
		table.insert(vertices, nX)
		table.insert(vertices, nY)
		table.insert(vertices, nZ)
		table.insert(vertices, UVs.s)
		table.insert(vertices, UVs.q)
		-- vertex 2
		table.insert(vertices, x2)
		table.insert(vertices, y2)
		table.insert(vertices, z2)
		table.insert(vertices, nX)
		table.insert(vertices, nY)
		table.insert(vertices, nZ)
		table.insert(vertices, UVs.p)
		table.insert(vertices, UVs.q)
		-- vertex 3
		if x1 == x2 then
			table.insert(vertices, x1)
			table.insert(vertices, y1)
			table.insert(vertices, z2)
		else
			table.insert(vertices, x2)
			table.insert(vertices, y1)
			table.insert(vertices, z1)
		end
		table.insert(vertices, nX)
		table.insert(vertices, nY)
		table.insert(vertices, nZ)
		table.insert(vertices, UVs.p)
		table.insert(vertices, UVs.t)

		-- indices
		if isClockWise then
			-- first triangle
			table.insert(indices, numVertices)
			table.insert(indices, numVertices + 1)
			table.insert(indices, numVertices + 2)
			-- second triangle
			table.insert(indices, numVertices + 2)
			table.insert(indices, numVertices + 3)
			table.insert(indices, numVertices)
		else
			-- first triangle
			table.insert(indices, numVertices)
			table.insert(indices, numVertices + 2)
			table.insert(indices, numVertices + 1)
			-- second triangle
			table.insert(indices, numVertices + 2)
			table.insert(indices, numVertices)
			table.insert(indices, numVertices + 3)
		end
	end
}
