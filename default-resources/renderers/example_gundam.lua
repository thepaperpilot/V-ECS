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
	preInit = function(self, renderer)
		self.gundam = model.new(renderer, "resources/models/gundam/model.obj", shaderStages.Vertex, {
			materialComponents.Diffuse
		})

		self.camera = archetype.new({ "Camera" })
	end,
	dependencies = {
		camera = "system",
		skybox = "renderer"
	},
	render = function(self, renderer)
		if not self.camera:isEmpty() then
			for index,c in self.camera:getComponents("Camera"):iterate() do
				local commandBuffer = renderer:startRendering()
				local MVP = c.viewProjectionMatrix * mat4.translate(vec3.new(0, 10, 0))
				renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, MVP)
				renderer:pushConstantVec3(commandBuffer, shaderStages.Vertex, sizes.Mat4, c.position)

				local cullFrustum = frustum.new(MVP)

				if cullFrustum:isBoxVisible(self.gundam.minBounds, self.gundam.maxBounds) then
					renderer:draw(commandBuffer, self.gundam)
				end

				renderer:finishRendering(commandBuffer)

				-- don't want us pushing too many constants
				return
			end
		end
	end
}
