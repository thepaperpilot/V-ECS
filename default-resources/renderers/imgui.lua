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
		ig.init(renderer)

		for _,event in archetype.new({ "RegisterTextureEvent" }):getComponents("RegisterTextureEvent"):iterate() do
			if event.texture == nil then
				event.texture = texture.new(renderer, event.pixels, event.width, event.height)
				renderer:addTexture(event.texture)
			end
		end
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
