return {
	shaders = {
		["resources/shaders/voxel.vert"] = shaderStages.Vertex,
		["resources/shaders/voxel.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.Vec3,
	vertexLayout = {
		[0] = vertexComponents.R32G32B32,
		[1] = vertexComponents.R32G32B32,
		[2] = vertexComponents.R32G32,
		[3] = vertexComponents.R32G32B32A32
	},
	lightPos = vec3.new(0, 1, 0),
	ambient = 0.8,
	preInit = function(self, renderer)
		-- setup blocks texture map
		local pixels, width, height, map = texture.createStitched(getResources("textures/blocks", ".png"))
		texture.new(renderer, pixels, width, height)

		-- create block archetypes
		local blockArchetypes = {}
		local blockUtils = {
			getBlockFromTexture = function (UVs)
				local top = {
					p = UVs.p,
					q = UVs.q,
					s = UVs.s,
					t = UVs.t
				}
				local bottom = {
					p = UVs.p,
					q = UVs.t,
					s = UVs.s,
					t = UVs.q
				}
				return archetype.new({}, {
					Block = {
						top = top,
						bottom = bottom,
						front = top,
						back = bottom,
						left = top,
						right = top
					}
				})
			end
		}
		for key,filename in pairs(getResources("blocks", ".lua")) do
			local block = require(filename:sub(1,filename:len()-4))
			blockArchetypes[block.id] = block.getArchetype(map, blockUtils)
		end
		
		-- save block archetypes to entity
		createEntity({
			Blocks = {
				archetypes = blockArchetypes
			}
		})

		-- setup uniform buffer
		self.ubo = renderer:createUBO(shaderStages.Vertex, 4 * sizes.Float)
		self.ubo:setDataFloats({ self.lightPos.x, self.lightPos.y, self.lightPos.z, self.ambient })

		self.camera = archetype.new({ "Camera" })
		self.chunksQuery = query.new({ "Chunk" })
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	forwardDependencies = {
		imgui = "renderer"
	},
	render = function(self, renderer)
		if not self.camera:isEmpty() then
			for id,c in self.camera:getComponents("Camera"):iterate() do
				local viewProj = c.viewProjectionMatrix
				local cullFrustum = frustum.new(viewProj)

				-- TODO sort by distance from player and attempt to perform occlusion culling
				for key,archetype in pairs(self.chunksQuery:getArchetypes()) do
					local data = luaVal.new({
						chunks = archetype,
						viewProj = viewProj,
						cameraPos = c.position,
						cullFrustum = cullFrustum,
						renderer = renderer
					})
					jobs.createParallel(self.renderChunks, data, archetype, 64):submit()
				end
			end
		end
	end,
	renderChunks = function(data, first, last)
		local commandBuffer = data.renderer:startRendering()
		data.renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, data.viewProj)
		data.renderer:pushConstantVec3(commandBuffer, shaderStages.Vertex, sizes.Mat4, data.cameraPos)
		data.chunks:lock_shared()
		for id,chunk in data.chunks:getComponents("Chunk"):iterate_range(luaVal.new(first), luaVal.new(last)) do
			if chunk.valid and data.cullFrustum:isBoxVisible(chunk.minBounds, chunk.maxBounds) then
				data.renderer:drawVertices(commandBuffer, chunk.vertexBuffer, chunk.indexBuffer, chunk.indexCount)
			end
		end
		data.chunks:unlock_shared()
		data.renderer:finishRendering(commandBuffer)
	end
}
