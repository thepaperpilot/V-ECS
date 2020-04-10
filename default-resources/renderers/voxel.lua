return {
	shaders = {
		["resources/shaders/voxel.vert"] = shaderStages.Vertex,
		["resources/shaders/voxel.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.mat4,
	vertexLayout = {
		[0] = vertexComponents.R32G32B32,
		[1] = vertexComponents.R32G32
	},
	init = function(self, world, renderer)
		-- setup block archetypes
		self.textureMap = texture.createStitched(renderer, getResources("textures/blocks", ".png"))
		self.blockArchetypes = {}
		for key,filename in pairs(getResources("blocks", ".lua")) do
			local id, getArchetype = require(filename:sub(1,filename:len()-4))
			self.blockArchetypes[id] = getArchetype(world)
		end

		self.chunksQuery = query({
			"Chunk"
		})
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	render = function(self, world, renderer)
		local viewProj = world.systems.camera.main.viewProjectionMatrix
		renderer:pushConstant(shaderStages.Vertex, 0, sizes.mat4, viewProj)

		local cullFrustum = frustum.new(MVP)

		for key,archetype in pairs(self.chunksQuery.archetypes) do
			for id,chunk in pairs(archetype:getComponents("Chunk")) do
				if chunk.indexCount > 0 and cullFrustum:isBoxVisible(chunk.minBounds, chunk.maxBounds) then
					renderer:drawVertices(chunk.vertexBuffer, chunk.indexBuffer, chunk.indexCount)
				end
			end
		end
	end,
	getBlockFromTexture = function(self, texture)
		local UVs = self.textureMap[texture]
		local top = { UVs.p, UVs.q, UVs.s, UV.t }
		local bottom = { UVs.p, UVs.t, UVs.s, UVs.q }
		return archetype.new({}, {}, {
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
