return {
	forwardDependencies = {
		imgui = "renderer"
	},
	init = function(self, world)
		self.worlds = {}
		local worlds = getResources("worlds", ".lua")
		for i=1, #worlds do
			local filename = worlds[i]
			local world = require(filename:sub(1,filename:len()-4))
			if world["name"] ~= nil then
				self.worlds[world["name"]] = world
			end
		end
	end,
	update = function(self, world)
		local width, height = glfw.windowSize()
		ig.setNextWindowPos(0, 0)
		ig.setNextWindowSize(width, height)

		ig.beginWindow("Title", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav,
			windowFlags.NoBackground,
			windowFlags.NoBringToFrontOnFocus
		})

		ig.text("V-ECS")
		ig.separator()

		for name, world in pairs(self.worlds) do
			if ig.button(name) then
				loadWorld(world)
			end
			ig.text(name)
		end

		ig.endWindow()
	end
}
