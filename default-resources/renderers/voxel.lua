return {
	shaders = {
		["resources/shaders/voxel.vert"] = shaderStages.Vertex,
		["resources/shaders/voxel.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.Vec3,
	vertexLayout = {
		[0] = vertexComponents.R32G32B32,
		[1] = vertexComponents.R32G32B32,
		[2] = vertexComponents.R32G32
	},
	loadDistance = 8,
	chunkSize = 16,
	preInit = function(self, world, renderer)
		self.renderer = renderer
		-- setup block archetypes
		local texture, map = texture.createStitched(renderer, getResources("textures/blocks", ".png"))
		self.textureMap = map
		self.blockArchetypes = {}
		for key,filename in pairs(getResources("blocks", ".lua")) do
			local block = require(filename:sub(1,filename:len()-4))
			self.blockArchetypes[block.id] = block.getArchetype(world)
		end
	end,
	init = function(self, world, renderer)
		self.chunksQuery = query.new({
			"Chunk"
		})
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	render = function(self, world)
		local viewProj = world.systems.camera.main.viewProjectionMatrix
		self.renderer:pushConstantMat4(shaderStages.Vertex, 0, viewProj)
		self.renderer:pushConstantVec3(shaderStages.Vertex, sizes.Mat4, world.systems.camera.main.position)

		local cullFrustum = frustum.new(viewProj)

		-- TODO sort by distance from player and attempt to perform occlusion culling
		for key,archetype in pairs(self.chunksQuery:getArchetypes()) do
			for id,chunk in pairs(archetype:getComponents("Chunk")) do
				if chunk.indexCount > 0 and cullFrustum:isBoxVisible(chunk.minBounds, chunk.maxBounds) then
					self.renderer:drawVertices(chunk.vertexBuffer, chunk.indexBuffer, chunk.indexCount)
				end
			end
		end
	end,
	getBlockFromTexture = function(self, texture)
		local UVs = self.textureMap[texture]
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
