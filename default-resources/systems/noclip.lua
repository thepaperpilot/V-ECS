return {
	speed = 25,
	lookSpeed = 0.1,
	up = vec3.new(0, 1, 0),
	init = function(self)
		self.pitch = 0
		self.lastPitch = 0
		self.yaw = 0
		self.lastYaw = 0
		self.dirty = false
		self.firstUpdate = true

		local x, y = glfw.cursorPos()
		self.lastX = x
		self.lastY = y

		createEntity({
			AddCommandEvent = {
				command = "flySpeed",
				self = self,
				callback = self.setMoveSpeed,
				description = "Usage: flySpeed {number}\n\tSets the noclip camera's movement speed to {number}, in m/s"
			}
		})

		self.camera = archetype.new({ "Camera" })
		self.mouseMoveEvent = archetype.new({ "MouseMoveEvent" })
		self.keyPressEvent = archetype.new({ "KeyPressEvent" })
		self.keyReleaseEvent = archetype.new({ "KeyReleaseEvent" })

		self.WPressed = false
		self.SPressed = false
		self.APressed = false
		self.DPressed = false
		self.SpacePressed = false
		self.LeftShiftPressed = false
	end,
	forwardDependencies = {
		camera = "system"
	},
	update = function(self)
		if self.firstUpdate then
			glfw.hideCursor()
			self.firstUpdate = false
		end

		if not self.mouseMoveEvent:isEmpty() then
			for _,event in self.mouseMoveEvent:getComponents("MouseMoveEvent"):iterate() do
				if event.cancelled then
					self.pitch = self.lastPitch
					self.yaw = self.lastYaw
					local x, y = glfw.cursorPos()
					self.lastX = x
					self.lastY = y
				else
					self.yaw = self.yaw + (event.xPos - self.lastX) * self.lookSpeed
					self.pitch = self.pitch - (event.yPos - self.lastY) * self.lookSpeed
					self.lastX = event.xPos
					self.lastY = event.yPos

					if self.pitch > 89 then self.pitch = 89 end
					if self.pitch < -89 then self.pitch = -89 end

					self.dirty = true
				end
			end
		end
		
		if not self.keyPressEvent:isEmpty() then
			for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
				if not event.cancelled then
					if event.key == keys.W then self.WPressed = true end
					if event.key == keys.S then self.SPressed = true end
					if event.key == keys.A then self.APressed = true end
					if event.key == keys.D then self.DPressed = true end
					if event.key == keys.Space then self.SpacePressed = true end
					if event.key == keys.LeftShift then self.LeftShiftPressed = true end
				end
			end
		end
		if not self.keyReleaseEvent:isEmpty() then
			for _,event in self.keyReleaseEvent:getComponents("KeyReleaseEvent"):iterate() do
				if not event.cancelled then
					if event.key == keys.W then self.WPressed = false end
					if event.key == keys.S then self.SPressed = false end
					if event.key == keys.A then self.APressed = false end
					if event.key == keys.D then self.DPressed = false end
					if event.key == keys.Space then self.SpacePressed = false end
					if event.key == keys.LeftShift then self.LeftShiftPressed = false end
				end
			end
		end

		if not self.camera:isEmpty() then
			for index,camera in self.camera:getComponents("Camera"):iterate() do
				if self.dirty then
					local pitch = radians(self.pitch)
					local yaw = radians(self.yaw)
					local forward = normalize(vec3.new(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)))
					local right = normalize(cross(forward, self.up))

					camera.forward = forward
					camera.right = right

					self.lastPitch = self.pitch
					self.lastYaw = self.yaw
					self.dirty = false
				end

				local velocity = vec3.new(0, 0, 0)
				if self.WPressed then velocity = velocity + camera.forward end
				if self.SPressed then velocity = velocity - camera.forward end
				if self.APressed then velocity = velocity - camera.right end
				if self.DPressed then velocity = velocity + camera.right end
				if self.SpacePressed then velocity = velocity + self.up end
				if self.LeftShiftPressed then velocity = velocity - self.up end

				if velocity:length() > 0 then
					velocity = normalize(velocity) * self.speed
				end

				camera.position = camera.position + velocity * time.getDeltaTime()
			end
		end
	end,
	setMoveSpeed = function(self, args)
		if #args == 1 then
			self.speed = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: flySpeed {number}\n\tSets the noclip camera's movement speed to {number}, in m/s")
		end
	end
}
