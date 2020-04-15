local up = vec3.new(0, 1, 0)

return {
	fieldOfView = 45,
	nearPlane = .1,
	farPlane = 10000000000,
	cameraQuery = query.new({
		"Camera"
	}),
	init = function(self, world)
		local cameraArchetype = archetype.new({
			"Camera"
		})
		if cameraArchetype:isEmpty() then
			local id, index = cameraArchetype:createEntity()
			self.main = cameraArchetype:getComponents("Camera")[index]
			self.main.position = vec3.new(0, 2, 0)
			self.main.forward = vec3.new(0, 0, 1)
			self.main.right = vec3.new(1, 0, 0)
			local width, height = glfw.windowSize()
			self:onWindowResize({ width = width, height = height })
		else
			self.main = cameraArchetype:getComponents("Camera")[1]
		end

		glfw.events.WindowResize:add(self, self.onWindowResize)

		if world.systems.debug ~= nil then
			world.systems.debug:addCommand("fov", self, self.setFov, "Usage: fov {number}\n\tSets the camera's field of view to {number}, in degrees")
		end
	end,
	getViewMatrix = function(position, forward)
		return lookAt(position, forward, up)
	end,
	update = function(self, world)
		for key,archetype in pairs(self.cameraQuery:getArchetypes()) do
			for id,camera in pairs(archetype:getComponents("Camera")) do
				camera.viewProjectionMatrix = camera.projectionMatrix * self.getViewMatrix(camera.position, camera.position + camera.forward)
			end
		end
	end,
	onWindowResize = function(self, windowResizeEvent)
		local aspectRatio = windowResizeEvent.width / windowResizeEvent.height
		self.main.projectionMatrix = perspective(radians(self.fieldOfView), aspectRatio, self.nearPlane, self.farPlane)
		-- flip yy component because glm assumes we're using opengl but vulkan has y=0 on the opposite side of the screen as opengl
		self.main.projectionMatrix:set(1, 1, self.main.projectionMatrix:get(1, 1) * -1)
	end,
	setFov = function(self, args)
		if #args == 1 then
			self.fieldOfView = tonumber(args[1])
			local width, height = glfw.windowSize()
			self:onWindowResize({ width = width, height = height })
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: fov {number}\n\tSets the camera's field of view to {number}, in degrees")
		end
	end
}
