return {
	shaders = {
		["resources/shaders/stone.vert"] = shaderStages.Vertex,
		["resources/shaders/stone.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 * 2 + sizes.Vec3,
	vertexLayout = {
		[0] = vertexComponents.Position,
		[1] = vertexComponents.Normal,
		[2] = vertexComponents.UV
	},
	preInit = function(self, renderer)
		self.stoneModel = model.new(renderer, "resources/gamejam/stone.obj", shaderStages.Vertex, { })

		self.stone = archetype.new({ "Stone" })
		self.camera = archetype.new({ "Camera" })
	end,
	dependencies = {
		camera = "system"
	},
	render = function(self, renderer)
		if not self.camera:isEmpty() then
			for index,c in self.camera:getComponents("Camera"):iterate() do
				for id,stone in self.stone:getComponents("Stone"):iterate() do
					local M = mat4.translate(stone.position) * mat4.rotate(stone.rotation, vec3.new(0, 1, 0)) * mat4.scale(stone.scale)
					local commandBuffer = renderer:startRendering()
					renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, c.viewProjectionMatrix)
					renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, sizes.Mat4, M)
					renderer:pushConstantVec3(commandBuffer, shaderStages.Vertex, sizes.Mat4 * 2, c.position)
					renderer:draw(commandBuffer, self.stoneModel)
					renderer:finishRendering(commandBuffer)
				end
			end
		end
	end
}
