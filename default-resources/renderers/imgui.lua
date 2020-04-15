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
	preInit = function(self, world, renderer)
		ig.createFontTexture(renderer)
	end,
	startFrame = function(self, world, renderer)
		ig.newFrame()
	end,
	render = function(self, world, renderer)
		ig.render(renderer)
	end
}
