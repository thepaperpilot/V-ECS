return {
	fieldOfView = 45,
	nearPlane = .1,
	farPlane = 10000000000,
	dependencies = {
		debug = "system"
	},
	preInit = function(self)
		self.camera = archetype.new({ "Camera" })
		if self.camera:isEmpty() then
			local id,index = self.camera:createEntity()
			local camera = self.camera:getComponents("Camera")[id]
			camera.position = vec3.new(0, 2, 0)
			camera.forward = vec3.new(0, 0, 1)
			camera.right = vec3.new(1, 0, 0)
			camera.getViewMatrix = self.getViewMatrix
			local width, height = glfw.windowSize()
			local aspectRatio = width / height
			camera.projectionMatrix = perspective(radians(self.fieldOfView), aspectRatio, self.nearPlane, self.farPlane)
			-- flip yy component because glm assumes we're using opengl but vulkan has y=0 on the opposite side of the screen as opengl
			camera.projectionMatrix:set(1, 1, camera.projectionMatrix:get(1, 1) * -1)
			camera.viewProjectionMatrix = camera.projectionMatrix * self.getViewMatrix(camera.position, camera.position + camera.forward)
		end

		createEntity({
			AddCommandEvent = {
				command = "fov",
				self = self,
				callback = self.setFov,
				description = "Usage: fov {number}\n\tSets the camera's field of view to {number}, in degrees"
			}
		})

		self.windowResizeEvent = archetype.new({ "WindowResizeEvent" })
	end,
	getViewMatrix = function(position, forward)
		return lookAt(position, forward, vec3.new(0, 1, 0))
	end,
	update = function(self)
		if not self.camera:isEmpty() then
			for id,camera in self.camera:getComponents("Camera"):iterate() do
				if not self.windowResizeEvent:isEmpty() then
					for idx,event in self.windowResizeEvent:getComponents("WindowResizeEvent"):iterate() do
						local aspectRatio = event.width / event.height
						camera.projectionMatrix = perspective(radians(self.fieldOfView), aspectRatio, self.nearPlane, self.farPlane)
						-- flip yy component because glm assumes we're using opengl but vulkan has y=0 on the opposite side of the screen as opengl
						camera.projectionMatrix:set(1, 1, camera.projectionMatrix:get(1, 1) * -1)
					end
				end

				camera.viewProjectionMatrix = camera.projectionMatrix * self.getViewMatrix(camera.position, camera.position + camera.forward)
			end
		end
	end,
	setFov = function(self, args)
		if #args == 1 then
			self.fieldOfView = tonumber(args[1])
			local width, height = glfw.windowSize()
			createEntity({
				WindowResizeEvent = { width = width, height = height }
			})
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: fov {number}\n\tSets the camera's field of view to {number}, in degrees")
		end
	end
}
