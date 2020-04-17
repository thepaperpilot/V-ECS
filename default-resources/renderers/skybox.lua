return {
	shaders = {
		["resources/shaders/skybox.vert"] = shaderStages.Vertex,
		["resources/shaders/skybox.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4,
	performDepthTest = false,
	vertexLayout = {
		[0] = vertexComponents.Position
	},
	init = function(self, world, renderer)
		self.renderer = renderer
		self.cube = model.new(renderer, "resources/models/skybox.obj")
	end,
	dependencies = {
		camera = "system"
	},
	render = function(self, world)
		local viewProj = world.systems.camera.main.projectionMatrix * world.systems.camera.getViewMatrix(vec3.new(0, 0, 0), world.systems.camera.main.forward)
		self.renderer:pushConstantMat4(shaderStages.Vertex, 0, viewProj)
		self.renderer:draw(self.cube)
	end
}
