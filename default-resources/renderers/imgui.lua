return {
	shaders = {
		["resources/shaders/imgui.vert"] = shaderStages.Vertex,
		["resources/shaders/imgui.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Float * 4,
	vertexLayout = {
		[0] = vertexComponents.R32G32,
		[1] = vertexComponents.R32G32,
		[2] = vertexComponents.R8G8B8A8_UNORM
	},
	performDepthTest = false,
	cullMode = cullModes.None,
	preInit = function(self, renderer)
		ig.preInit(renderer)
	end,
	init = function(self, renderer)
		self.registerTexturesEvent = archetype.new({ "RegisterTextureEvent" })
		for _,event in self.registerTexturesEvent:getComponents("RegisterTextureEvent"):iterate() do
			if event.texture == nil then
				if event.pixels ~= nil then
					event.texture = texture.new(renderer, event.pixels, event.width, event.height, false)
				elseif event.location ~= nil then
					event.texture = texture.new(renderer, event.location, false)
				end
			end
		end
	end,
	postInit = function(self, renderer)
		for _,event in self.registerTexturesEvent:getComponents("RegisterTextureEvent"):iterate() do
			if event.texture ~= nil then
				renderer:addTexture(event.texture)
			end
		end
		ig.postInit(renderer)
	end,
	startFrame = function(self)
		ig.newFrame()
	end,
	render = function(self, renderer)
		local commandBuffer = renderer:startRendering()
		ig.render(commandBuffer, renderer)
		renderer:finishRendering(commandBuffer)
	end
}
