return {
	forwardDependencies = {
		imgui = "renderer"
	},
	numSlots = 9,
	activeSlot = 1,
	hotbarWidth = .5,
	hotbar = {
		"sword_diamond.png",
		"axe_diamond.png",
		"hammer_diamond.png",
		"hoe_diamond.png",
		"pick_diamond.png",
		"shovel_diamond.png",
		"sword_diamond.png"
	},
	init = function(self, world)
		glfw.events.VerticalScroll:add(self, self.onVerticalScroll)
		glfw.events.KeyPress:add(self, self.onKeyPress)

		local itemsTex, itemsMap = world.renderers.imgui:addStitchedTexture(getResources("textures/items", ".png"))
		self.itemsTex = itemsTex
		self.itemsMap = itemsMap

		if world.systems.debug ~= nil then
			world.systems.debug:addCommand("numSlots", self, self.setNumSlots, "Usage: numSlots {slots}\n\tSets the number of hotbar slots to {slots}")
			world.systems.debug:addCommand("hotbarWidth", self, self.setHotbarWidth, "Usage: hotbarWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the hotbar take up")
		end
	end,
	update = function(self, world)
		local width, height = glfw.windowSize()
		-- 16 is twice the default window padding in imgui (8)
		-- 7 is used to account for the child frame padding
		-- they're used here to find the actual size the hotbar slots can be
		-- without showing any scrollbars
		local hotbarSize = (width * self.hotbarWidth - 16) / self.numSlots - 7
		ig.setNextWindowPos(math.floor(width * (1 - self.hotbarWidth) / 2), height - math.floor(hotbarSize) - 16)
		ig.setNextWindowSize(math.floor(width * self.hotbarWidth), math.floor(hotbarSize) + 16)

		ig.beginWindow("Hotbar", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav
		})

		for i = 1, self.numSlots do
			if self.activeSlot == i then
				ig.pushStyleVar(styleVars.FramePadding, 0, 0)
				ig.pushStyleVar(styleVars.ChildBorderSize, 4)
				ig.pushStyleColor(styleColors.Border, vec4.new(1, 1, 1, 1))
			end
			ig.beginChild("slot"..tostring(i), vec2.new(hotbarSize, hotbarSize), true, {})
			if self.hotbar[i] ~= nil then
				local UVs = self.itemsMap[self.hotbar[i]]
				ig.image(self.itemsTex, vec2.new(hotbarSize - 16, hotbarSize - 16), vec4.new(UVs.p, UVs.q, UVs.s, UVs.t))
			end			
			ig.endChild()
			if self.activeSlot == i then
				ig.popStyleColor(1)
				ig.popStyleVar(2)
			end
			ig.sameLine()
		end

		ig.endWindow()
	end,
	setNumSlots = function(self, args)
		if #args == 1 then
			self.numSlots = math.floor(tonumber(args[1]))
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: numSlots {slots}\n\tSets the number of hotbar slots to {slots}")
		end
	end,
	setHotbarWidth = function(self, args)
		if #args == 1 then
			self.hotbarWidth = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: hotbarWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the hotbar take up")
		end
	end,
	onVerticalScroll = function(self, world, data)
		if world.systems.debug ~= nil and world.systems.debug.isDebugWindowOpen then return end
		self.activeSlot = self.activeSlot - data.yOffset
		if self.activeSlot < 1 then
			self.activeSlot = self.numSlots
		elseif self.activeSlot > self.numSlots then
			self.activeSlot = 1
		end
	end,
	onKeyPress = function(self, world, data)
		if world.systems.debug ~= nil and world.systems.debug.isDebugWindowOpen then return end
		if data.key > keys["0"] and data.key <= keys["9"] then
			local key = data.key - keys["0"]
			if key <= self.numSlots then
				self.activeSlot = key
			end
		end
	end
}
