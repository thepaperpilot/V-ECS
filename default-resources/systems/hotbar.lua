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
	preInit = function(self)
		local pixels, width, height, itemsMap = texture.createStitched(getResources("textures/items", ".png"))
		self.itemsMap = itemsMap

		-- Make event to register the texture in our imgui renderer
		createEntity({
			RegisterTextureEvent = {
				pixels = pixels,
				width = width,
				height = height,
				id = "hotbar:itemsTex"
			}
		})

		createEntity({
			AddCommandEvent = {
				command = "numSlots",
				self = self,
				callback = self.setNumSlots,
				description = "Usage: numSlots {slots}\n\tSets the number of hotbar slots to {slots}"
			}
		})

		createEntity({
			AddCommandEvent = {
				command = "hotbarWidth",
				self = self,
				callback = self.setHotbarWidth,
				description = "Usage: hotbarWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the hotbar take up"
			}
		})

		self.verticalScrollEvent = archetype.new({
			"VerticalScrollEvent"
		})
		self.keyPressEvent = archetype.new({
			"KeyPressEvent"
		})
	end,
	postInit = function(self)
		local registerTextureEventArchetype = archetype.new({ "RegisterTextureEvent" })
		for index,event in registerTextureEventArchetype:getComponents("RegisterTextureEvent"):iterate() do
			if event.texture ~= nil and event.id == "hotbar:itemsTex" then
				self.itemsTex = event.texture
				registerTextureEventArchetype:deleteEntity(index)
			end
		end
	end,
	update = function(self)
		if not self.verticalScrollEvent:isEmpty() then
			for _,event in self.verticalScrollEvent:getComponents("VerticalScrollEvent"):iterate() do
				if not event.cancelled then
					self.activeSlot = self.activeSlot - event.yOffset
					if self.activeSlot < 1 then
						self.activeSlot = self.numSlots
					elseif self.activeSlot > self.numSlots then
						self.activeSlot = 1
					end
				end
			end
		end

		if not self.keyPressEvent:isEmpty() then
			for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
				if not event.cancelled then
					if event.key > keys["0"] and event.key <= keys["9"] then
						local key = event.key - keys["0"]
						if key <= self.numSlots then
							self.activeSlot = key
						end
					end
				end
			end
		end

		ig.lock()
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
		ig.release()
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
	end
}
