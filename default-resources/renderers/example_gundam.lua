return {
	shaders = {
		["resources/shaders/gundam.vert"] = shaderStages.Vertex,
		["resources/shaders/gundam.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.mat4,
	vertexLayout = {
		[0] = vertexComponents.Position,
		[1] = vertexComponents.MaterialIndex
	},
	init = function(self, world, renderer)
		self.gundam = model.new(renderer, "resources/models/gundam/model.obj", shaderStages.Fragment, {
			materialComponents.Diffuse
		})
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	render = function(self, world, renderer)
		local MVP = world.systems.camera.main.viewProjectionMatrix * mat4.translate(vec3.new(0, 10, 0))
		renderer:pushConstant(shaderStages.Vertex, 0, sizes.mat4, MVP)

		local cullFrustum = frustum.new(MVP)

		if cullFrustum:isBoxVisible(self.gundam.minBounds, self.gundam.maxBounds) then
			renderer:draw(self.gundam)
		end
	end
}
