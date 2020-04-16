return {
	shaders = {
		["resources/shaders/gundam.vert"] = shaderStages.Vertex,
		["resources/shaders/passthrough.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.Vec3,
	vertexLayout = {
		[0] = vertexComponents.Position,
		[1] = vertexComponents.Normal,
		[2] = vertexComponents.MaterialIndex
	},
	preInit = function(self, world, renderer)
		self.gundam = model.new(renderer, "resources/models/gundam/model.obj", shaderStages.Vertex, {
			materialComponents.Diffuse
		})
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	render = function(self, world, renderer)
		local MVP = world.systems.camera.main.viewProjectionMatrix * mat4.translate(vec3.new(0, 10, 0))
		renderer:pushConstantMat4(shaderStages.Vertex, 0, MVP)
		renderer:pushConstantVec3(shaderStages.Vertex, sizes.Mat4, world.systems.camera.main.position)

		local cullFrustum = frustum.new(MVP)

		if cullFrustum:isBoxVisible(self.gundam.minBounds, self.gundam.maxBounds) then
			renderer:draw(self.gundam)
		end
	end
}
