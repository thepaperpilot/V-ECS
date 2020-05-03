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
	getBlock = function(self, x, y, z, point)
		if self.chunks[x] ~= nil and self.chunks[x][y] ~= nil and self.chunks[x][y][z] ~= nil then
			return self.chunkComponents[self.chunks[x][y][z]].blocks[point]
		end
		return nil
	end,
	createMesh = function(self, world, x, y, z, index)
		local chunk = self.chunkComponents[index]
		chunk.x = x
		chunk.y = y
		chunk.z = z

		-- pre-calculate these so we don't have to do it in each of the 6 sweeps
		local chunkSize = world.renderers.voxel.chunkSize
		local chunkSizeSquared = chunkSize ^ 2

		-- create the block entities and add their faces to a mesh
		-- References:
		-- https://github.com/roboleary/GreedyMesh/blob/master/src/mygame/Main.java
		-- https://github.com/mikolalysenko/mikolalysenko.github.com/blob/gh-pages/MinecraftMeshes/js/greedy.js
		-- and more
		local vertices = {}
		local indices = {}
		chunk.vertexCount = 0
		chunk.indexCount = 0
		-- sweep over 3-axes
		for d = 0, 2 do
			-- we do two axis sweeps with backface and frontface
			self:axisSweep(chunk, chunkSize, chunkSizeSquared, vertices, indices, d, true)
			self:axisSweep(chunk, chunkSize, chunkSizeSquared, vertices, indices, d, false)
		end

		if chunk.indexCount > 0 then
			-- build our vertex and index buffers
			chunk.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, chunk.vertexCount * 12 * sizes.Float)
			chunk.vertexBuffer:setDataFloats(vertices)
			chunk.indexBuffer = buffer.new(bufferUsage.IndexBuffer, chunk.indexCount * sizes.Float)
			chunk.indexBuffer:setDataInts(indices)
		end
	end,
	axisSweep = function(self, chunk, chunkSize, chunkSizeSquared, vertices, indices, d, backface)
		-- calculate chunk's position offsets
		local chunkX = chunk.x * chunkSize
		local chunkY = chunk.y * chunkSize
		local chunkZ = chunk.z * chunkSize

		-- d, u, v becomes the three axes but shifted over one in each iteration
		local u = (d + 1) % 3
		local v = (d + 2) % 3

		local x = { [0] = 0, [1] = 0, [2] = 0 }
		local q = { [0] = 0, [1] = 0, [2] = 0 }
		q[d] = 1

		-- TODO figure out why this offset is necessary to make the voxels line up
		local r = { [0] = 0, [1] = 0, [2] = 0 }
		r[v] = -1

		-- make our mask
		-- each point will either be false or the archetype at that position
		local mask = {}
		x[d] = -1
		while x[d] < chunkSize do
			local n = 0
			x[v] = 0
			while x[v] < chunkSize do
				x[u] = 0
				while x[u] < chunkSize do
					-- compare the two blocks sharing this face
					-- we either want to render a, b, or neither (in the case both blocks exist and are opague then we can cull this face)
					local a, b
					-- first check the face between this chunk and the one before it
					if x[d] == -1 then
						-- if we're at the edge of one chunk, we can't just add q to get to the neighboring block,
						-- because its coordinates local to the adjacent chunk will be on the other edge
						-- fortunately that means we can just add chunkSize instead of 1 on that side,
						-- so effectively all we need to do is add q[index] * chunkSize
						local point = (x[0] + q[0] * chunkSize) * chunkSizeSquared + (x[1] + q[1] * chunkSize) * chunkSize + x[2] + q[2] * chunkSize
						-- a is in a different chunk, so we use this method to get it:
						-- note it returns nil if the chunk isn't loaded or generated
						a = self:getBlock(chunk.x - q[0], chunk.y - q[1], chunk.z - q[2], point)
						b = chunk.blocks[(x[0] + q[0]) * chunkSizeSquared + (x[1] + q[1]) * chunkSize + x[2] + q[2]]
					-- second, check the face between this chunk and the next
					elseif x[d] == chunkSize - 1 then
						-- this is similar to the previous case, just flipped
						local point = (x[0] - q[0] * (chunkSize - 1)) * chunkSizeSquared + (x[1] - q[1] * (chunkSize - 1)) * chunkSize + x[2] - q[2] * (chunkSize - 1)
						a = chunk.blocks[x[0] * chunkSizeSquared + x[1] * chunkSize + x[2]]
						b = self:getBlock(chunk.x + q[0], chunk.y + q[1], chunk.z + q[2], point)
					-- lastly, handle all the in-between values
					else
						a = chunk.blocks[x[0] * chunkSizeSquared + x[1] * chunkSize + x[2]]
						b = chunk.blocks[(x[0] + q[0]) * chunkSizeSquared + (x[1] + q[1]) * chunkSize + x[2] + q[2]]
					end

					-- if both blocks exist draw neither
					if a ~= nil and b ~= nil then
						mask[n] = false
					else
						-- if they're different then we use a if we're doing the backface, and b if we're doing the frontface
						-- this means if one exists then this face will only get rendered in one pass and not the other,
						-- and if neither exist then this face won't get rendered in either pass (since nil is falsey)
						if backface then
							mask[n] = a
						else
							mask[n] = b
						end
					end
					n = n + 1
					x[u] = x[u] + 1
				end
				x[v] = x[v] + 1
			end
			x[d] = x[d] + 1

			n = 0
			-- generate mesh
			local j = 0
			while j < chunkSize do
				j = j + 1
				local i = 0
				while i < chunkSize do
					if mask[n] then
						-- get the archetype
						-- TODO each chunk gets the archetype 6 times. Is that okay?
						local archetype = mask[n]
						local block = archetype:getSharedComponent("Block")

						-- compute width
						local w = 1
						while mask[n + w] and i + w < chunkSize do w = w + 1 end
						-- compute height
						local h = 1
						while j + h < chunkSize do
							local k = 0
							while k < w and mask[n + k + h * chunkSize] do k = k + 1 end
							if k < w then break end
							h = h + 1
						end

						-- create entities for each of the blocks
						-- TODO determine if the archetype has per-entity data, and only create entities if that's the case?
						--local firstId, firstIndex = archetype:createEntities(w * h)

						local UVs
						if d == 0 then
							UVs = backface and block.right or block.left
						elseif d == 1 then
							UVs = backface and block.top or block.bottom
						else
							UVs = backface and block.front or block.back
						end

						local texSize = {
							-- start UVs
							p = UVs.p,
							q = UVs.q,
							-- size of each dimension
							s = UVs.s - UVs.p,
							t = UVs.t - UVs.q
						}

						-- add face
						x[u] = i
						x[v] = j
						local du = { [0] = 0, [1] = 0, [2] = 0 }
						local dv = { [0] = 0, [1] = 0, [2] = 0 }
						du[u] = w
						dv[v] = h

						local baseX = chunkX + r[0] + x[0]
						local baseY = chunkY + r[1] + x[1]
						local baseZ = chunkZ + r[2] + x[2]
						-- first vertex is always the same
						self.addVertex(vertices,
							baseX,
							baseY,
							baseZ,
							q, backface and -1 or 1, 0, 0, texSize)

						if d == 0 then
							-- left/right faces are handled very slightly differently,
							-- but enough to make a layer of abstraction too annoying to implement,
							-- so now we have this large if statement for the other 3 vertices
							self.addVertex(vertices,
								baseX + du[0],
								baseY + du[1],
								baseZ + du[2],
								q, backface and -1 or 1, 0, -w, texSize)
							self.addVertex(vertices,
								baseX + du[0] + dv[0],
								baseY + du[1] + dv[1],
								baseZ + du[2] + dv[2],
								q, backface and -1 or 1, h, -w, texSize)
							self.addVertex(vertices,
								baseX         + dv[0],
								baseY         + dv[1],
								baseZ         + dv[2],
								q, backface and -1 or 1, h, 0, texSize)
						else
							self.addVertex(vertices,
								baseX + du[0],
								baseY + du[1],
								baseZ + du[2],
								q, backface and -1 or 1, w, 0, texSize)
							self.addVertex(vertices,
								baseX + du[0] + dv[0],
								baseY + du[1] + dv[1],
								baseZ + du[2] + dv[2],
								q, backface and -1 or 1, w, h * (backface and -1 or 1), texSize)
							self.addVertex(vertices,
								baseX         + dv[0],
								baseY         + dv[1],
								baseZ         + dv[2],
								q, backface and -1 or 1, 0, h * (backface and -1 or 1), texSize)
						end

						if backface then
							-- Clockwise indices
							-- first triangle
							table.insert(indices, chunk.vertexCount)
							table.insert(indices, chunk.vertexCount + 1)
							table.insert(indices, chunk.vertexCount + 2)
							-- second triangle
							table.insert(indices, chunk.vertexCount + 2)
							table.insert(indices, chunk.vertexCount + 3)
							table.insert(indices, chunk.vertexCount)
						else
							-- Counter clockwise indices
							-- first triangle
							table.insert(indices, chunk.vertexCount)
							table.insert(indices, chunk.vertexCount + 2)
							table.insert(indices, chunk.vertexCount + 1)
							-- second triangle
							table.insert(indices, chunk.vertexCount + 2)
							table.insert(indices, chunk.vertexCount)
							table.insert(indices, chunk.vertexCount + 3)
						end

						chunk.vertexCount = chunk.vertexCount + 4
						chunk.indexCount = chunk.indexCount + 6

						-- zero out the mask
						for l = 0, h - 1 do
							for k = 0, w - 1 do
								mask[n + k + l * chunkSize] = false
							end
						end

						i = i + w
						n = n + w
					else
						i = i + 1
						n = n + 1
					end
				end
			end
		end
	end,
	addVertex = function(vertices, x, y, z, normals, normalsMod, texCoordX, texCoordY, texSize)
		-- TODO way to further optimize information passed per-vertex?
		table.insert(vertices, x)
		table.insert(vertices, y)
		table.insert(vertices, z)
		table.insert(vertices, normals[0] * normalsMod)
		table.insert(vertices, normals[1] * normalsMod)
		table.insert(vertices, normals[2] * normalsMod)
		table.insert(vertices, texCoordX)
		table.insert(vertices, texCoordY)
		table.insert(vertices, texSize.p)
		table.insert(vertices, texSize.q)
		table.insert(vertices, texSize.s)
		table.insert(vertices, texSize.t)
	end
}
