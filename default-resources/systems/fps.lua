return {
	forwardDependencies = {
		imgui = "renderer"
	},
	update = function(self, world)
		local width, height = glfw.windowSize()
		ig.setNextWindowPos(width - 100, 0)
		ig.setNextWindowSize(100, 20)

		ig.beginWindow("FPS", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav,
			windowFlags.NoBackground
		})

		ig.text(tostring(math.floor(1 / time.getDeltaTime())).." fps")

		ig.endWindow()
	end
}
