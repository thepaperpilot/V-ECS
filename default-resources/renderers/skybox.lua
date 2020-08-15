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
	init = function(self, renderer)
		self.cube = model.new(renderer, "resources/models/skybox.obj")

		self.camera = archetype.new({ "Camera" })
	end,
	dependencies = {
		camera = "system"
	},
	render = function(self, renderer)
		if not self.camera:isEmpty() then
			for index,c in self.camera:getComponents("Camera"):iterate() do
				local commandBuffer = renderer:startRendering()
				local viewProj = c.projectionMatrix * c.getViewMatrix(vec3.new(0, 0, 0), c.forward)
				renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, viewProj)
				renderer:draw(commandBuffer, self.cube)

				renderer:finishRendering(commandBuffer)
				
				-- don't want us pushing too many constants
				return
			end
		end
	end
}
