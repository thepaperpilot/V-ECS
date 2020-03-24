local gundam

renderer = {
	shaders = {
		["resources/shaders/gundam.vert"] = shaderStages.Vertex,
		["resources/shaders/gundam.frag"] = shaderStages.Fragment
	},
	pushConstantsRange = sizes.mat4,
	vertexLayout = {
		["0"] = vertexComponents.Position,
		["1"] = vertexComponents.MaterialIndex
	},
	init = function(loader)
		local gundamMatLayout = loader:createMaterialLayout(shaderStages.Fragment, {
			materialComponents.Diffuse
		})
		gundam = loader:loadModelWithMaterials("resources/models/gundam/model.obj", gundamMatLayout)
	end,
	render = function(renderer)
		local MVP = renderer:getViewProjectionMatrix() * mat4.translate(0, 10, 0)
		local cullFrustum = frustum(MVP)
		renderer:pushMat4(shaderStages.Vertex, 0, MVP)
		if cullFrustum:isBoxVisible(gundam.minBounds, gundam.maxBounds) then
			renderer:draw(gundam)
		end
	end
}
