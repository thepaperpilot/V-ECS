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
		self.cube = model.new(renderer, "resources/models/skybox.obj")
	end,
	dependencies = {
		camera = "system"
	},
	render = function(self, world, renderer)
		local viewProj = world.systems.camera.main.projectionMatrix * world.systems.camera:getViewMatrix(vec3.new(0, 0, 0), world.systems.camera.main.forward)
		renderer:pushConstant(shaderStages.Vertex, 0, size.mat4, viewProj)
		renderer:draw(self.cube)
	end
}
