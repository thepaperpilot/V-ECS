return {
	loadDistance = 4,
	chunkSize = 16,
	seed = 1337,
	forwardDependencies = {
		voxel = "renderer"
	},
	init = function(self)
		self.blocks = archetype.new({ "Blocks" })

		-- create or find our seed
		local seedArchetype = archetype.new({
			"Seed"
		})
		if seedArchetype:isEmpty() then
			local id,index = seedArchetype:createEntity()
			seedArchetype:getComponents("Seed")[id].seed = self.seed
		else
			self.seed = seedArchetype:getComponents("Seed")[1].seed
		end

		local terrainGens = {}
		for key,filename in pairs(getResources("terrain", ".lua")) do
			-- Note we strip the ".lua"  from the filename
			local generator = require(filename:sub(1,filename:len()-4))
			generator:init(self.chunkSize, self.seed)
			terrainGens[#terrainGens + 1] = generator
		end
		-- sort our terrain generators by priority
		-- we only need the priority for this sorting step,
		-- so we store the generators in an array so we don't need to sort them for every generated chunk
		self.terrainGens = {}
		for _,generator in self.spairs(terrainGens, function(t, a, b) return t[a].priority < t[b].priority end) do
			self.terrainGens[#self.terrainGens + 1] = generator
		end

		self.chunkArchetype = archetype.new({
			"Chunk"
		})
		-- TODO is storing 3D array of chunk indices going to be okay?
		self.chunks = {}
		if self.chunkArchetype:isEmpty() and not self.blocks:isEmpty() then
			local blocks
			for id,b in self.blocks:getComponents("Blocks"):iterate() do
				blocks = b.archetypes
				break
			end

			local id,index = self.chunkArchetype:createEntities((2 * self.loadDistance + 1) ^ 3)
			local chunks = self.chunkArchetype:getComponents("Chunk")
			local currId = id
			for x = -self.loadDistance, self.loadDistance do
				self.chunks[x] = {}
				for y = -self.loadDistance, self.loadDistance  do
					self.chunks[x][y] = {}
					for z = -self.loadDistance, self.loadDistance do
						self.chunks[x][y][z] = currId
						chunks[currId].x = x
						chunks[currId].y = y
						chunks[currId].z = z

						local data = luaVal.new({
							id = currId,
							chunks = self.chunkArchetype,
							chunkSize = self.chunkSize,
							terrainGens = self.terrainGens,
							blocks = blocks,
							addVertex = self.addVertex
						})
						jobs.create(self.generateChunk, data):submit()

						currId = currId + 1
					end
				end
			end
		end

		createEntity({
			AddCommandEvent = {
				command = "loadDistance",
				self = self,
				callback = self.setLoadDistance,
				description = "Usage: self.loadDistance {number}\n\tSets the radius of chunks to load from the player's chunk to {number}"
			}
		})
	end,
	postInit = function(self)
		self.camera = archetype.new({ "Camera" })
		for id,camera in self.camera:getComponents("Camera"):iterate() do	
			self.x = math.floor(camera.position.x / self.chunkSize)
			self.y = math.floor(camera.position.y / self.chunkSize)
			self.z = math.floor(camera.position.z / self.chunkSize)
			break
		end
	end,
	update = function(self)
		for id,camera in self.camera:getComponents("Camera"):iterate() do
			local x = math.floor(camera.position.x / self.chunkSize)
			local y = math.floor(camera.position.y / self.chunkSize)
			local z = math.floor(camera.position.z / self.chunkSize)
			if (self.x ~= x or self.y ~= y or self.z ~= z) and not self.blocks:isEmpty() then
				local blocks
				for id,b in self.blocks:getComponents("Blocks"):iterate() do
					blocks = b.archetypes
					break
				end

				-- unload chunks that are far away
				local toRemove = {}
				local chunks = self.chunkArchetype:getComponents("Chunk")
				self.chunkArchetype:lock_shared()
				for i,chunk in chunks:iterate() do
					if chunk.valid and (math.abs(chunk.x - x) > self.loadDistance or math.abs(chunk.y - y) > self.loadDistance or math.abs(chunk.z - z) > self.loadDistance) then
						table.insert(toRemove, i)
					end
				end
				self.chunkArchetype:unlock_shared()
				self.chunkArchetype:deleteEntities(toRemove)

				-- load chunks that are nearby but don't currently exist
				if self.x < x then
					for cx = self.x + self.loadDistance, x + self.loadDistance do
						if self.chunks[cx] == nil then self.chunks[cx] = {} end
						for cy = y - self.loadDistance, y + self.loadDistance do
							if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
							for cz = z - self.loadDistance, z + self.loadDistance do
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				elseif self.x > x then
					for cx = x - self.loadDistance, self.x - self.loadDistance do
						if self.chunks[cx] == nil then self.chunks[cx] = {} end
						for cy = y - self.loadDistance, y + self.loadDistance do
							if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
							for cz = z - self.loadDistance, z + self.loadDistance do
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				end

				if self.y < y then
					for cy = self.y + self.loadDistance, y + self.loadDistance do
						for cx = x - self.loadDistance, x + self.loadDistance do
							if self.chunks[cx] == nil then self.chunks[cx] = {} end
							if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
							for cz = z - self.loadDistance, z + self.loadDistance do
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				elseif self.y > y then
					for cy = y - self.loadDistance, self.y - self.loadDistance do
						for cx = x - self.loadDistance, x + self.loadDistance do
							if self.chunks[cx] == nil then self.chunks[cx] = {} end
							if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
							for cz = z - self.loadDistance, z + self.loadDistance do
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				end

				if self.z < z then
					for cz = self.z + self.loadDistance, z + self.loadDistance do
						for cy = y - self.loadDistance, y + self.loadDistance do
							for cx = x - self.loadDistance, x + self.loadDistance do
								if self.chunks[cx] == nil then self.chunks[cx] = {} end
								if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				elseif self.z > z then
					for cz = z - self.loadDistance, self.z - self.loadDistance do
						for cy = y - self.loadDistance, y + self.loadDistance do
							for cx = x - self.loadDistance, x + self.loadDistance do
								if self.chunks[cx] == nil then self.chunks[cx] = {} end
								if self.chunks[cx][cy] == nil then self.chunks[cx][cy] = {} end
								self:checkChunk(blocks, chunks, cx, cy, cz)
							end
						end
					end
				end

				self.x = x
				self.y = y
				self.z = z
			end
			break
		end
	end,
	checkChunk = function(self, blocks, chunks, x, y, z)
		if self.chunks[x][y][z] == nil or chunks[self.chunks[x][y][z]] == nil then
			local id, index = self.chunkArchetype:createEntity()
			self.chunks[x][y][z] = id
			chunks[id].x = x
			chunks[id].y = y
			chunks[id].z = z

			-- start job to generate this chunk
			local data = luaVal.new({
				chunks = self.chunkArchetype,
				chunkSize = self.chunkSize,
				terrainGens = self.terrainGens,
				blocks = blocks,
				addVertex = self.addVertex,
				id = id
			})
			local job = jobs.create(self.generateChunk, data)
			job.persistent = true
			job:submit()
		end
	end,
	generateChunk = function(data)
		local size = data.chunkSize
		local sizeSq = size ^ 2
		local invalid = {}

		data.chunks:lock_shared()
		local chunk = data.chunks:getComponents("Chunk")[data.id]
		data.chunks:unlock_shared()

		chunk.minBounds = vec3.new(chunk.x * size, chunk.y * size, chunk.z * size)
		chunk.maxBounds = vec3.new((chunk.x + 1) * size, (chunk.y + 1) * size, (chunk.z + 1) * size)
		chunk.blocks = {}

		-- run our terrain generators
		for i,generator in data.terrainGens:iterate() do
			generator:generateChunk(data.blocks, size, chunk)
		end

		-- TODO make the rest of this function a compute shader job?

		-- create the block entities and add their faces to a mesh
		local vertices = {}
		local indices = {}
		chunk.vertexCount = 0
		chunk.indexCount = 0

		-- pre-calculate these so we don't have to do it in each of the 6 sweeps
		local chunkX = chunk.x * size
		local chunkY = chunk.y * size
		local chunkZ = chunk.z * size

		-- sweep over 3-axes
		for d = 0, 2 do
			-- d, u, v becomes the three axes but shifted over one in each iteration
			local u = (d + 1) % 3
			local v = (d + 2) % 3
			-- we do two axis sweeps with backface and frontface
			for bf = 0, 1 do
				local backface = bf == 1
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
				while x[d] < size do
					local n = 0
					x[v] = 0
					while x[v] < size do
						x[u] = 0
						while x[u] < size do
							-- compare the two blocks sharing this face
							-- we either want to render a, b, or neither (in the case both blocks exist and are opague then we can cull this face)
							local a, b
							-- first check the face between this chunk and the one before it
							if x[d] == -1 then
								-- if we're at the edge of one chunk, we can't just add q to get to the neighboring block,
								-- because its coordinates local to the adjacent chunk will be on the other edge
								-- fortunately that means we can just add size instead of 1 on that side,
								-- so effectively all we need to do is add q[index] * size
								local point = (x[0] + q[0] * size) * sizeSq + (x[1] + q[1] * size) * size + x[2] + q[2] * size
								-- a is in a different chunk, so we use this method to get it:
								-- note it returns nil if the chunk isn't loaded or generated
								--a = self:getBlock(chunk.x - q[0], chunk.y - q[1], chunk.z - q[2], point)
								-- TODO how to handle neighboring chunks?
								a = nil
								b = chunk.blocks[(x[0] + q[0]) * sizeSq + (x[1] + q[1]) * size + x[2] + q[2]]
							-- second, check the face between this chunk and the next
							elseif x[d] == size - 1 then
								-- this is similar to the previous case, just flipped
								local point = (x[0] - q[0] * (size - 1)) * sizeSq + (x[1] - q[1] * (size - 1)) * size + x[2] - q[2] * (size - 1)
								a = chunk.blocks[x[0] * sizeSq + x[1] * size + x[2]]
								--b = self:getBlock(chunk.x + q[0], chunk.y + q[1], chunk.z + q[2], point)
								-- TODO how to handle neighboring chunks?
								b = nil
							-- lastly, handle all the in-between values
							else
								a = chunk.blocks[x[0] * sizeSq + x[1] * size + x[2]]
								b = chunk.blocks[(x[0] + q[0]) * sizeSq + (x[1] + q[1]) * size + x[2] + q[2]]
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
					while j < size do
						j = j + 1
						local i = 0
						while i < size do
							if mask[n] then
								-- get the archetype
								-- TODO each chunk gets the archetype 6 times. Is that okay?
								local archetype = mask[n]
								local block = archetype:getSharedComponent("Block")

								-- compute width
								local w = 1
								while mask[n + w] and i + w < size do w = w + 1 end
								-- compute height
								local h = 1
								while j + h < size do
									local k = 0
									while k < w and mask[n + k + h * size] do k = k + 1 end
									if k < w then break end
									h = h + 1
								end

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
								data.addVertex(vertices,
									baseX,
									baseY,
									baseZ,
									q, backface and -1 or 1, 0, 0, texSize)

								if d == 0 then
									-- left/right faces are handled very slightly differently,
									-- but enough to make a layer of abstraction too annoying to implement,
									-- so now we have this large if statement for the other 3 vertices
									data.addVertex(vertices,
										baseX + du[0],
										baseY + du[1],
										baseZ + du[2],
										q, backface and -1 or 1, 0, -w, texSize)
									data.addVertex(vertices,
										baseX + du[0] + dv[0],
										baseY + du[1] + dv[1],
										baseZ + du[2] + dv[2],
										q, backface and -1 or 1, h, -w, texSize)
									data.addVertex(vertices,
										baseX         + dv[0],
										baseY         + dv[1],
										baseZ         + dv[2],
										q, backface and -1 or 1, h, 0, texSize)
								else
									data.addVertex(vertices,
										baseX + du[0],
										baseY + du[1],
										baseZ + du[2],
										q, backface and -1 or 1, w, 0, texSize)
									data.addVertex(vertices,
										baseX + du[0] + dv[0],
										baseY + du[1] + dv[1],
										baseZ + du[2] + dv[2],
										q, backface and -1 or 1, w, h * (backface and -1 or 1), texSize)
									data.addVertex(vertices,
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
										mask[n + k + l * size] = false
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
			end
		end

		-- save our mesh
		if chunk.indexCount > 0 then
			chunk.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, chunk.vertexCount * 12 * sizes.Float)
			chunk.vertexBuffer:setDataFloats(vertices)
			chunk.indexBuffer = buffer.new(bufferUsage.IndexBuffer, chunk.indexCount * sizes.Float)
			chunk.indexBuffer:setDataInts(indices)
			chunk.valid = true
		else
			data.chunks:deleteEntity(data.id)
		end
	end,
	addVertex = function(vertices, x, y, z, normals, normalsMod, texCoordX, texCoordY, texSize)
		-- TODO way to further optimize information passed per-vertex?
		table.insert(vertices, x)
		table.insert(vertices, y)
		table.insert(vertices, z)
		table.insert(vertices, -normals[0] * normalsMod)
		table.insert(vertices, -normals[1] * normalsMod)
		table.insert(vertices, -normals[2] * normalsMod)
		table.insert(vertices, texCoordX)
		table.insert(vertices, texCoordY)
		table.insert(vertices, texSize.p)
		table.insert(vertices, texSize.q)
		table.insert(vertices, texSize.s)
		table.insert(vertices, texSize.t)
	end,
	spairs = function(t, order)
		-- function used for creating a sorted iterator of k,v pairs in a table
		-- we use it here to sort our terrain generators by priority
		-- taken from https://stackoverflow.com/questions/15706270/sort-a-table-in-lua

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
	end,
	-- TODO utility function for rebuilding only parts of mesh when adding/removing single blocks
	setLoadDistance = function(self, args)
		if #args == 1 then
			self.loadDistance = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: self.loadDistance {number}\n\tSets the radius of chunks to load from the player's chunk to {number}")
		end
	end
}
