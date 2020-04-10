return {
	forwardDependencies = {
		imgui = "renderer"
	},
	init = function(self, world)
		glfw.events.KeyRelease:add(self, self.onKeyRelease)
		self.isDebugWindowOpen = false
	end,
	update = function(self, world)
		if self.isDebugWindowOpen then
			ig.showDemoWindow()
		end
	end,
	onKeyRelease = function(self, keyReleaseEvent)
		if keyReleaseEvent.key == keys.GRAVE then
			self.isDebugWindowOpen = not self.isDebugWindowOpen
		end
	end
}
