local cube

renderer = {
	shaders = {
		["resources/shaders/skybox.vert"] = shaderStages.Vertex,
		["resources/shaders/skybox.frag"] = shaderStages.Fragment
	},
	pushConstantsRange = sizes.mat4,
	performDepthTest = false,
	renderPriority = -1,
	vertexLayout = {
		["0"] = vertexComponents.Position
	},
	init = function(loader)
		cube = loader:loadModel("resources/models/skybox.obj")
	end,
	render = function(renderer)
		local viewProj = renderer:getProjectionMatrix() * renderer:getViewMatrix(vec3(0, 0, 0), renderer:getCameraForward())
		renderer:pushMat4(shaderStages.Vertex, 0, viewProj)
		renderer:draw(cube)
	end
}
