return {
	init = function(self, world)
		-- create or find our seed
		local seedArchetype = archetype.new({
			"Seed"
		})
		if seedArchetype:isEmpty() then
			local id, index = seedArchetype:createEntity()
			seedArchetype:getComponents("Seed")[index].seed = 1337
			self.seed = 1337
		else
			self.seed = seedArchetype:getComponents("Seed")[1].seed
		end

		self.terrainGens = {}
		for key,filename in pairs(getResources("terrain", ".lua")) do
			-- Note we strip the ".lua"  from the filename
			local generator = require(filename:sub(1,filename:len()-4))
			generator:init(self.seed)
			self.terrainGens[generator] = generator.priority
		end
		-- sort our terrain generators by priority
		table.sort(self.terrainGens)

		local chunkArchetype = archetype.new({
			"Chunk"
		})
		self.chunkComponents = chunkArchetype:getComponents("Chunk")
		if chunkArchetype:isEmpty() then
			local firstId, firstIndex = chunkArchetype:createEntities(world.loadDistance ^ 3)
			local currIndex = firstIndex
			for x = 0, world.loadDistance - 1 do
				for y = 0, world.loadDistance - 1 do
					for z = 0, world.loadDistance - 1 do
						self:fillChunk(world, x, y, z, currIndex)
						currIndex = currIndex + 1
					end
				end
			end
		end
	end,
	fillChunk = function(self, world, x, y, z, index)
		local chunk = chunkComponents[index]
		local chunkSizeSquared = world.chunkSize ^ 2

		chunk.minBounds = vec3(x * world.chunkSize, y * world.chunkSize, z * world.chunkSize)
		chunk.maxBounds = vec3((x + 1) * world.chunkSize, (y + 1) * world.chunkSize, (z + 1) * world.chunkSize)
		chunk.blocks = {}
		-- store neigbors?

		-- run our terrain generators
		for i,generator in ipairs(self.terrainGens) do
			generator:generateChunk(world, chunk, x, y, z)
		end

		-- currently chunk.blocks maps points to their archetype
		-- we want a map of archetypes to the points using it
		local archetypes = {}
		for point,archetype in chunk.blocks do
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
		for archetype, points in archetypes do
			local firstId, firstIndex = archetype:createEntities(#points)
			local block = archetype:getSharedComponent("Block")
			for _, point in points do
				-- local[XYZ] are the coordinates of this block inside this chunk
				local localX = point / chunkSizeSquared
				local localY = (point % chunkSizeSquared) / world.chunkSize
				local localZ = point % world.chunkSize
				-- global[XYZ] are the coordinates of this block in global space
				-- remember x, y, and z are the chunk's position
				local globalX = localX + x * world.chunkSize
				local globalY = localY + y * world.chunkSize
				local globalZ = localZ + z * world.chunkSize

				if localY == world.chunkSize - 1 or chunk.blocks[point + world.chunkSize] == nil then -- top
					addFace(globalX, globalY + 1, globalZ, globalX + 1, globalY + 1, globalZ + 1, block.top, true)
				end
				if localY == 0 or chunk.blocks[point + world.chunkSize] == nil then -- bottom
					addFace(globalX, globalY, globalZ, globalX + 1, globalY, globalZ + 1, block.bottom, false)
				end
				if localZ == world.chunkSize - 1 or chunk.blocks[point + 1] == nil then -- back
					addFace(globalX + 1, globalY + 1, globalZ + 1, globalX, globalY, globalZ + 1, block.back, false)
				end
				if localZ == 0 or chunk.blocks[point - 1] == nil then -- front
					addFace(globalX, globalY, globalZ, globalX + 1, globalY + 1, globalZ, block.front, true)
				end
				if localX == world.chunkSize - 1 or chunk.blocks[point + chunkSizeSquared] == nil then -- left
					addFace(globalX + 1, globalY, globalZ, globalX + 1, globalY + 1, globalZ + 1, block.left, true)
				end
				if localX == 0 or chunk.blocks[point - chunkSizeSquared] == nil then -- right
					addFace(globalX, globalY, globalZ + 1, globalX, globalY + 1, globalZ, block.right, true)
				end
			end
		end

		-- build our vertex and index buffers
		chunk.indexCount = numIndices
		chunk.vertexBuffer = buffer(bufferUsage.VertexBuffer, numVertices * sizes.Float)
		chunk.vertexBuffer:setData(vertices)
		chunk.indexBuffer = buffer(bufferUsage.IndexBuffer, numIndices * sizes.Float)
		chunk.indexBuffer:setData(indices)
	end,
	addFace = function(x1, y1, z1, x2, y2, z2, UVs, isClockWise, vertices, indices, numVertices)
		-- vertex 0
		table.insert(vertices, x1)
		table.insert(vertices, y1)
		table.insert(vertices, z1)
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
		table.insert(vertices, UVs.s)
		table.insert(vertices, UVs.q)
		-- vertex 2
		table.insert(vertices, x2)
		table.insert(vertices, y2)
		table.insert(vertices, z2)
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
