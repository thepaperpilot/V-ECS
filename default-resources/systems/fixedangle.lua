return {
	height = 10,
	dragSpeed = 0.05,
	dragging = false,
	angle = vec3.new(.5, -1, 1):normalized(),
	init = function(self)
		self.camera = archetype.new({ "Camera" })
		self.rightMousePressEvent = archetype.new({ "RightMousePressEvent" })
		self.rightMouseReleaseEvent = archetype.new({ "RightMouseReleaseEvent" })
		self.mouseMoveEvent = archetype.new({ "MouseMoveEvent" })

		if not self.camera:isEmpty() then
			for index,camera in self.camera:getComponents("Camera"):iterate() do
				camera.position = vec3.new(camera.position.x, self.height, camera.position.z)
				local right = normalize(cross(self.angle, vec3.new(0, 1, 0)))
				camera.forward = self.angle
				camera.right = right
			end
		end
	end,
	forwardDependencies = {
		camera = "system"
	},
	update = function(self)
		if not self.rightMousePressEvent:isEmpty() then
			for _,event in self.rightMousePressEvent:getComponents("RightMousePressEvent"):iterate() do
				if not event.cancelled then
					self.dragging = true
					local posX, posY = glfw.cursorPos()
					self.lastX = posX
					self.lastY = posY
				end
			end
		end

		if not self.rightMouseReleaseEvent:isEmpty() then
			self.dragging = false
		end

		local dirty = false
		local moveX = 0
		local moveZ = 0
		if not self.mouseMoveEvent:isEmpty() then
			for _,event in self.mouseMoveEvent:getComponents("MouseMoveEvent"):iterate() do
				if not event.cancelled and self.dragging then
					dirty = true
					local x, y = glfw.cursorPos()
					moveX = x - self.lastX
					moveZ = y - self.lastY
					self.lastX = x
					self.lastY = y
				end
			end
		end

		if dirty and not self.camera:isEmpty() then
			for index,camera in self.camera:getComponents("Camera"):iterate() do
				camera.position = camera.position + vec3.new(moveX * self.dragSpeed, 0, moveZ * self.dragSpeed)
			end
		end
	end
}
