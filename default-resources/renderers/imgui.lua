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
		self.renderer = renderer
		ig.preInit(renderer)
	end,
	init = function(self, world)
		ig.init(self.renderer)
	end,
	startFrame = function(self, world)
		ig.newFrame()
	end,
	render = function(self, world)
		ig.render(self.renderer)
	end,
	addStitchedTexture = function(self, filenames)
		local texture, map = texture.createStitched(self.renderer, filenames)
		self.renderer:addTexture(texture)
		return texture, map
	end
}
