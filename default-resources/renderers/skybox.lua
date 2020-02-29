local cube

renderer = {
	shaders = {
		["resources/shaders/skybox.vert"] = shaderStages.Vertex,
		["resources/shaders/skybox.frag"] = shaderStages.Fragment
	},
	pushConstantsRange = sizes.mat4,
	vertexLayout = {
		["0"] = vertexComponents.Position
	},
	init = function(loader)
		cube = loader:loadModel("resources/models/skybox.obj")
	end,
	render = function(renderer)
		local MVP = renderer:getMVP()
		renderer:pushMat4(shaderStages.Vertex, 0, MVP)
		renderer:draw(cube)
	end,
	cleanup = function()
		cube:cleanup()
	end
}
