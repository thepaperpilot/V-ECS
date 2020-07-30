return {
	forwardDependencies = {
		imgui = "renderer"
	},
	preInit = function(self)
		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 72)
	end,
	init = function(self)
		self.worlds = {}
		local worlds = getResources("worlds", ".lua")
		for i=1, #worlds do
			local filename = worlds[i]
			local world = require(filename:sub(1,filename:len()-4))
			if world["name"] ~= nil then
				self.worlds[world["name"]] = filename
			end
		end

		self.loading = archetype.new({
			"LoadStatus"
		})
	end,
	update = function(self)
		if self.loading:isEmpty() then
			ig.lock()
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

			ig.pushFont(self.largeFont)
			ig.text("V-ECS")
			ig.popFont()
			ig.separator()

			for name, w in self.worlds:iterate() do
				if ig.button(name) then
					local id,index = self.loading:createEntity()
					self.loading:getComponents("LoadStatus")[id].status = loadWorld(w)
				end
			end

			ig.endWindow()
			ig.release()
		end
	end
}
