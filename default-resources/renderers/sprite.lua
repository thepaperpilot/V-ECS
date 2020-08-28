return {
	shaders = {
		["resources/shaders/sprite.vert"] = shaderStages.Vertex,
		["resources/shaders/sprite.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.vec4,
	performDepthTest = false,
	vertexLayout = {
		[0] = vertexComponents.R32G32,
		[1] = vertexComponents.R32G32
	},
	forwardDependencies = {
		imgui = "renderer"
	},
	init = function(self, renderer)
		self.sprites = query.new({ "Sprite" })

		local textures = {} -- list, for creating stitched texture
		local foundTextures = {} -- set, for finding duplicates
   		for key,archetype in pairs(self.sprites:getArchetypes()) do
   			for _,sprite in archetype:getComponents("Sprite"):iterate() do
   				if foundTextures[sprite.location] == nil then
					foundTextures[sprite.location] = true
					textures[#textures + 1] = sprite.location
				end
			end
   		end

		local pixels, width, height, texturesMap = texture.createStitched(textures)
		texture.new(renderer, pixels, width, height)

   		for key,archetype in pairs(self.sprites:getArchetypes()) do
   			for _,sprite in archetype:getComponents("Sprite"):iterate() do
   				sprite.UVs = texturesMap[sprite.filename]
			end
   		end
	end,
	render = function(self, renderer)
		for key,archetype in pairs(self.sprites:getArchetypes()) do
			if not archetype:isEmpty() then
				for index,s in archetype:getComponents("Sprite"):iterate() do
					local commandBuffer = renderer:startRendering()
					local M = mat4.translate(s.position == nil and vec3.new(0, 0, 0) or s.position) * mat4.rotate(s.rotation ~= nil and s.rotation or 0, vec3.new(0, 0, 1)) * mat4.scale(s.scale == nil and vec3.new(1, 1, 1) or s.scale)
					renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, M)
					renderer:pushConstantVec4(commandBuffer, shaderStages.Vertex, sizes.Mat4, s.tint ~= nil and s.tint or vec4.new(1, 1, 1, 1))
					renderer:drawVertices(commandBuffer, s.vertexBuffer, s.indexBuffer, s.indexCount)
					renderer:finishRendering(commandBuffer)
				end
			end
		end
	end
}
