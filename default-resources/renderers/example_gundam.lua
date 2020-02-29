local gundam

renderer = {
	shaders = {
		["resources/shaders/gundam.vert"] = shaderStages.Vertex,
		["resources/shaders/gundam.frag"] = shaderStages.Fragment
	},
	pushConstantsRange = sizes.mat4,
	vertexLayout = {
		["0"] = vertexComponents.Position
	},
	init = function(loader)
		gundam = loader:loadModel("resources/models/gundam/model.obj")
	end,
	render = function(renderer)
		local MVP = renderer:getMVP()
		local frustum = renderer:getFrustum()
		MVP:translate(0, 10, 0)
		local minBounds = gundam.minBounds + vec3(0, 10, 0)
		local maxBounds = gundam.maxBounds + vec3(0, 10, 0)
		-- TODO draw grid of gundams
		if frustum:isBoxVisible(minBounds, maxBounds) then
			renderer:pushMat4(shaderStages.Vertex, 0, MVP)
			renderer:draw(gundam)
		end
	end,
	cleanup = function()
		gundam:cleanup()
	end
}
