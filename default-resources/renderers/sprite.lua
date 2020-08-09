return {
	shaders = {
		["resources/shaders/sprite.vert"] = shaderStages.Vertex,
		["resources/shaders/sprite.frag"] = shaderStages.Fragment
	},
	vertexLayout = {
		[0] = vertexComponents.R32G32,
		[1] = vertexComponents.R32G32
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
				local commandBuffer = renderer:startRendering()
				for index,s in archetype:getComponents("Sprite"):iterate() do
					renderer:drawVertices(commandBuffer, s.vertexBuffer, s.indexBuffer, s.indexCount)
				end
				renderer:finishRendering(commandBuffer)
			end
		end
	end
}
