local up = vec3.new(0, 1, 0)
local cameraQuery

return {
	fieldOfView = 45,
	nearPlane = .1,
	farPlane = 10000000000,
	init = function(self, world)
		cameraQuery = query({
			"Camera"
		})

		local cameraArchetype = archetype.new({
			"Camera"
		})
		if cameraArchetype:isEmpty() then
			local id, index = cameraArchetype:createEntity()
			self.main = cameraArchetype:getComponents("Camera")[index]
			self.main.getViewMatrix = function(position, forward)
				return lookAt(position, forward, up)
			end
		else
			self.main = cameraArchetype:getComponents("Camera")[1]
		end

		glfw.events.WindowResize:add(self, self.onWindowResize)
	end,
	update = function(self, world)
		for key,archetype in pairs(cameraQuery.archetypes) do
			for id,camera in pairs(archetype:getComponents("Camera")) do
				camera.viewProjectionMatrix = camera.projectionMatrix * camera.getViewMatrix(camera.position, camera.forward)
			end
		end
	end,
	onWindowResize = function(self, windowResizeEvent)
		local aspectRatio = windowResizeEvent.width / windowResizeEvent.height
		self.main.projectionMatrix = perspective(radians(self.fieldOfView), aspectRatio, self.near, self.far)
		-- flip yy component because glm assumes we're using opengl but vulkan has y=0 on the opposite side of the screen as opengl
		self.main.projectionMatrix:set(1, 1, self.main.projectionMatrix:get(1, 1) * -1)
	end
}
