local up = vec3.new(0, 1, 0)

return {
	speed = 25,
	lookSpeed = 0.1,
	init = function(self, world)
		self.pitch = 0
		self.lastPitch = 0
		self.yaw = 0
		self.lastYaw = 0
		self.dirty = false
		self.firstUpdate = true

		local x, y = glfw.cursorPos()
		self.lastX = x
		self.lastY = y

		glfw.events.MouseMove:add(self, self.onMouseMove)

		if world.systems.debug ~= nil then
			world.systems.debug:addCommand("flySpeed", self, self.setMoveSpeed, "Usage: flySpeed {number}\n\tSets the noclip camera's movement speed to {number}, in m/s")
		end
	end,
	forwardDependencies = {
		camera = "system"
	},
	update = function(self, world)
		if self.firstUpdate then
			glfw.hideCursor()
			self.firstUpdate = false
		end

		local camera = world.systems.camera.main

		if world.systems.debug == nil or (not world.systems.debug.isDebugWindowOpen and not world.systems.debug.appearing) then
			if self.dirty then
				local pitch = radians(self.pitch)
				local yaw = radians(self.yaw)
				local forward = normalize(vec3.new(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)))
				local right = normalize(cross(forward, up))

				camera.forward = forward
				camera.right = right

				self.lastPitch = self.pitch
				self.lastYaw = self.yaw
				self.dirty = false
			end

			local velocity = vec3.new(0, 0, 0)
			if glfw.isKeyPressed(keys.W) then velocity = velocity + camera.forward end
			if glfw.isKeyPressed(keys.S) then velocity = velocity - camera.forward end
			if glfw.isKeyPressed(keys.A) then velocity = velocity - camera.right end
			if glfw.isKeyPressed(keys.D) then velocity = velocity + camera.right end
			if glfw.isKeyPressed(keys.Space) then velocity = velocity + up end
			if glfw.isKeyPressed(keys.LeftShift) then velocity = velocity - up end

			if velocity:length() > 0 then
				velocity = normalize(velocity) * self.speed
			end

			camera.position = camera.position + velocity * time.getDeltaTime()
		elseif world.systems.debug ~= nil and world.systems.debug.appearing then
			self.pitch = self.lastPitch
			self.yaw = self.lastYaw
			local x, y = glfw.cursorPos()
			self.lastX = x
			self.lastY = y
		end
	end,
	onMouseMove = function(self, world, mouseMoveEvent)
		self.yaw = self.yaw + (mouseMoveEvent.xPos - self.lastX) * self.lookSpeed
		self.pitch = self.pitch - (mouseMoveEvent.yPos - self.lastY) * self.lookSpeed
		self.lastX = mouseMoveEvent.xPos
		self.lastY = mouseMoveEvent.yPos

		if self.pitch > 89 then self.pitch = 89 end
		if self.pitch < -89 then self.pitch = -89 end

		self.dirty = true
	end,
	setMoveSpeed = function(self, args)
		if #args == 1 then
			self.speed = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: flySpeed {number}\n\tSets the noclip camera's movement speed to {number}, in m/s")
		end
	end
}
