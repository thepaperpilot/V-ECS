return {
	shaders = {
		["resources/shaders/stone.vert"] = shaderStages.Vertex,
		["resources/shaders/stone.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.Vec3,
	vertexLayout = {
		[0] = vertexComponents.Position,
		[1] = vertexComponents.Normal,
		[2] = vertexComponents.UV
	},
	preInit = function(self, renderer)
		self.stoneModel = model.new(renderer, "resources/models/stone.obj", shaderStages.Vertex, { })

		self.stone = archetype.new({ "Stone" })
		self.camera = archetype.new({ "Camera" })

		self.ubo = renderer:createUBO(shaderStages.Vertex, sizes.Mat4 + sizes.Vec3)
	end,
	init = function(self, renderer)
		for index,c in self.camera:getComponents("Camera"):iterate() do
			local VP = c.viewProjectionMatrix
			self.ubo:setDataFloats({
				VP:get(0, 0), VP:get(0, 1), VP:get(0, 2), VP:get(0, 3),
				VP:get(1, 0), VP:get(1, 1), VP:get(1, 2), VP:get(1, 3),
				VP:get(2, 0), VP:get(2, 1), VP:get(2, 2), VP:get(2, 3),
				VP:get(3, 0), VP:get(3, 1), VP:get(3, 2), VP:get(3, 3),
				c.position.x, c.position.y, c.position.z
			})
		end
	end,
	dependencies = {
		camera = "system",
		imgui = "renderer"
	},
	render = function(self, renderer)
		-- TODO check if camera has changed
		for id,stone in self.stone:getComponents("Stone"):iterate() do
			local commandBuffer = renderer:startRendering()
			renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, mat4.translate(stone.position) * mat4.rotate(stone.rotation, vec3.new(0, 1, 0)) * mat4.scale(stone.scale))
			renderer:draw(commandBuffer, self.stoneModel)
			renderer:finishRendering(commandBuffer)
		end
	end
}
