local up = vec3.new(0, 1, 0)

return {
	speed = 25,
	lookSpeed = 0.1,
	init = function(self, world)
		glfw.hideCursor()
		self.pitch = 0
		self.yaw = 0
		self.dirty = false
		self.velocity = vec3.new(0, 0, 0)

		local x, y = glfw.cursorPos()
		self.lastX = x
		self.lastY = y

		glfw.events.MouseMove:add(self, self.onMouseMove)
	end,
	forwardDependencies = {
		camera = "system"
	},
	update = function(self, world)
		if self.dirty then
			local pitch = radians(self.pitch)
			local yaw = radians(self.yaw)
			local forward = normalize(vec3.new(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)))
			local right = normalize(cross(forward, up))

			self.velocity = vec3.new(0, 0, 0)
			if glfw.isKeyPressed(keys.W) then self.velocity = self.velocity + forward end
			if glfw.isKeyPressed(keys.S) then self.velocity = self.velocity - forward end
			if glfw.isKeyPressed(keys.A) then self.velocity = self.velocity - right end
			if glfw.isKeyPressed(keys.D) then self.velocity = self.velocity + right end
			if glfw.isKeyPressed(keys.Space) then self.velocity = self.velocity + up end
			if glfw.isKeyPressed(keys.LeftShift) then self.velocity = self.velocity - up end

			if self.velocity:length() > 0 then
				self.velocity = normalize(self.velocity) * self.speed
			end

			local camera = world.systems.camera.main
			camera.position = camera.position + self.velocity * time.deltaTime
			camera.forward = forward
			camera.right = right

			self.dirty = false
		else
			local camera = world.systems.camera.main
			camera.position = camera.position + self.velocity * time.deltaTime
			camera.forward = forward
			camera.right = right
		end
	end,
	onMouseMove = function(self, mouseMoveEvent)
		self.yaw = self.yaw + (mouseMoveEvent.xPos - self.lastX) * self.lookSpeed
		self.pitch = self.pitch - (mouseMoveEvent.yPos - self.lastY) * self.lookSpeed
		self.lastX = mouseMoveEvent.xPos
		self.lastY = mouseMoveEvent.yPos

		if self.pitch > 89 then self.pitch = 89 end
		if self.pitch < -89 then self.pitch = -89 end

		self.dirty = true
	end
}
