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
		self.gundams = archetype.new({ "Gundam" })
	end,
	dependencies = {
		camera = "system"
	},
	forwardDependencies = {
		imgui = "renderer"
	},
	render = function(self, renderer)
		if not self.camera:isEmpty() then
			for index,c in self.camera:getComponents("Camera"):iterate() do
				-- TODO sort by distance from player and attempt to perform occlusion culling
				local data = luaVal.new({
					gundams = self.gundams,
					gundam = self.gundam,
					viewProj = c.viewProjectionMatrix,
					cameraPos = c.position,
					renderer = renderer
				})
				jobs.createParallel(self.renderGundams, data, self.gundams, 64):submit()
			end
		end
	end,
	renderGundams = function(data, first, last)
		data.gundams:lock_shared()
		for id,gundam in data.gundams:getComponents("Gundam"):iterate_range(luaVal.new(first), luaVal.new(last)) do
			local MVP = data.viewProj * mat4.translate(vec3.new(gundam.x, gundam.y, gundam.z)) * mat4.rotate(gundam.rotation, vec3.new(0, 1, 0))

			local commandBuffer = data.renderer:startRendering()
			data.renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, MVP)
			data.renderer:pushConstantVec3(commandBuffer, shaderStages.Vertex, sizes.Mat4, data.cameraPos)
			
			local cullFrustum = frustum.new(MVP)
			if cullFrustum:isBoxVisible(data.gundam.minBounds, data.gundam.maxBounds) then
				data.renderer:draw(commandBuffer, data.gundam)
			end
			
			data.renderer:finishRendering(commandBuffer)
		end
		data.gundams:unlock_shared()
	end
}
