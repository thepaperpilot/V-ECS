return {
	forwardDependencies = {
		imgui = "renderer"
	},
	rows = 5,
	cols = 9,
	activeSlot = 1,
	hotbarWidth = .5,
	inventoryWidth = .8,
	initialInventory = {
		{ id = "sword_diamond", amount = 1 },
		{ id = "axe_diamond", amount = 1 },
		{ id = "hammer_diamond", amount = 1 },
		{ id = "hoe_diamond", amount = 1 },
		{ id = "pick_diamond", amount = 1 },
		{ id = "shovel_diamond", amount = 1 },
		{ id = "apple", amount = 23 },
		nil,
		{ id = "seed", amount = 45 }
	},
	items = { },
	isInventoryOpen = false,
	preInit = function(self)
		local pixels, width, height, itemsMap = texture.createStitched(getResources("textures/items", ".png"))
		self.itemsMap = itemsMap

		ig.lock()
		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 24)
		ig.release()

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
				command = "invSize",
				self = self,
				callback = self.setInvSize,
				description = "Usage: invSize {columns} {rows}\n\tSets the size of the inventory to {rows} rows of {columns} items each"
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

		createEntity({
			AddCommandEvent = {
				command = "inventoryWidth",
				self = self,
				callback = self.setInventoryWidth,
				description = "Usage: inventoryWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the inventory take up"
			}
		})

		createEntity({
			Inventory = self.initialInventory
		})

		self.verticalScrollEvent = archetype.new({ "VerticalScrollEvent" })
		self.keyPressEvent = archetype.new({ "KeyPressEvent" })
		self.mouseMoveEvent = archetype.new({ "MouseMoveEvent" })
		self.inventory = archetype.new({ "Inventory" })
	end,
	init = function(self)
		self:addItem("sword_diamond", "Diamond Sword", "sword_diamond.png", { maxAmount = 1 })
		self:addItem("axe_diamond", "Diamond Axe", "axe_diamond.png", { maxAmount = 1 })
		self:addItem("hammer_diamond", "Diamond Hammer", "hammer_diamond.png", { maxAmount = 1 })
		self:addItem("hoe_diamond", "Diamond Hoe", "hoe_diamond.png", { maxAmount = 1 })
		self:addItem("pick_diamond", "Diamond Pick", "pick_diamond.png", { maxAmount = 1 })
		self:addItem("shovel_diamond", "Diamond Shovel", "shovel_diamond.png", { maxAmount = 1 })
		self:addItem("apple", "Apple", "apple.png")
		self:addItem("seed", "Seed", "seed.png")

		local addItemEventArchetype = archetype.new({ "AddItemEvent" })
		for _,event in addItemEventArchetype:getComponents("AddItemEvent"):iterate() do
			self:addItem(event.id, event.name, event.location, event.extra)
		end
		addItemEventArchetype:clearEntities()
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
	startFrame = function(self)
		-- "Cancel" any mouse movement when the inventory window is open
		if self.isInventoryOpen then
			if not self.keyPressEvent:isEmpty() then
				for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
					if not (event.key > keys["0"] and event.key <= keys["9"]) and event.key ~= keys.E then
						event.cancelled = true
					end
				end
			end
			if not self.mouseMoveEvent:isEmpty() then
				for _,event in self.mouseMoveEvent:getComponents("MouseMoveEvent"):iterate() do
					event.cancelled = true
				end
			end
		end
	end,
	update = function(self)
		if not self.verticalScrollEvent:isEmpty() then
			for _,event in self.verticalScrollEvent:getComponents("VerticalScrollEvent"):iterate() do
				if not event.cancelled then
					self.activeSlot = self.activeSlot - event.yOffset
					if self.activeSlot < 1 then
						self.activeSlot = self.cols
					elseif self.activeSlot > self.cols then
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
						if key <= self.cols then
							self.activeSlot = key
						end
					elseif event.key == keys.E then
						self.isInventoryOpen = not self.isInventoryOpen
						if self.isInventoryOpen then
							glfw.showCursor()
						else
							glfw.hideCursor()
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
		local hotbarSize = (width * self.hotbarWidth - 16) / self.cols - 7
		ig.setNextWindowPos(math.floor(width * (1 - self.hotbarWidth) / 2), height - math.floor(hotbarSize) - 16)
		ig.setNextWindowSize(math.floor(width * self.hotbarWidth), math.floor(hotbarSize) + 16)

		ig.beginWindow("Hotbar", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav
		})

		for _,inventory in self.inventory:getComponents("Inventory"):iterate() do
			for i = 1, self.cols do
				if self.activeSlot == i then
					ig.pushStyleVar(styleVars.FramePadding, 0, 0)
					ig.pushStyleVar(styleVars.ChildBorderSize, 4)
					ig.pushStyleColor(styleColors.Border, vec4.new(1, 1, 1, 1))
				end
				self:renderItem(inventory, i, hotbarSize)
				if self.activeSlot == i then
					ig.popStyleColor(1)
					ig.popStyleVar(2)
				end
				ig.sameLine()
			end
			break
		end

		ig.endWindow()

		if self.isInventoryOpen then
			ig.pushStyleVar(styleVars.WindowRounding, 0)

			ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, .5))
			ig.pushStyleColor(styleColors.ChildBg, vec4.new(0, 0, 0, 1))
			
			local width, height = glfw.windowSize()
			ig.setNextWindowPos(0, 0)
			ig.setNextWindowSize(width, height)

			ig.beginWindow("InventoryContainer", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoResize,
				windowFlags.NoScrollbar
			})

			local inventorySize = (width * self.inventoryWidth - 16) / self.cols - 7
			local invHeight = math.floor(inventorySize * self.rows) + 72

			ig.setCursorPos(math.floor(width * (1 - self.inventoryWidth) / 2), math.floor((height - invHeight) / 2))

			ig.beginChild("Inventory", vec2.new(math.floor(width * self.inventoryWidth), invHeight), true, { })

			ig.pushFont(self.largeFont)
			ig.text("Inventory")
			ig.popFont()

			for _,inventory in self.inventory:getComponents("Inventory"):iterate() do
				for i = self.rows, 1, -1 do
					for j = 1, self.cols do
						self:renderItem(inventory, (i - 1) * self.cols + j, inventorySize)
						if j ~= self.cols then ig.sameLine() end
					end
					if i == 2 then
						ig.dummy(vec2.new(0, 8))
					end
				end

				ig.endChild()
				
				if inventory.hand ~= nil then
					local cursorX, cursorY = glfw.cursorPos()
					ig.setCursorPos(math.floor(cursorX - inventorySize / 2), math.floor(cursorY - inventorySize / 2))
					ig.pushStyleColor(styleColors.ChildBg, vec4.new(0, 0, 0, 0))
					ig.pushStyleVar(styleVars.ChildBorderSize, 0)
					self:renderItem(inventory, "hand", inventorySize, true)
					ig.popStyleVar(1)
					ig.popStyleColor(1)
				end
				break
			end
			if self.inventory:isEmpty() then ig.endChild() end

			ig.endWindow()

			ig.popStyleVar(1)
			ig.popStyleColor(2)
		end

		ig.release()
	end,
	addItem = function(self, id, name, location, extra)
		local item = extra
		if item == nil then item = {} end
		item.name = name
		item.location = location
		self.items[id] = item
	end,
	renderItem = function(self, inventory, index, size, ignoreHover)
		local hovered = false
		if not ignoreHover then
			-- TODO check if we're hovering over the item before creating the child so we can style its background
			-- I think the reason this isn't working is because cursorPos isn't relative to the glfw window, but rather the current ig window
			local cursorX, cursorY = glfw.cursorPos()
			local cursorPos = ig.getCursorPos()
			if cursorX > cursorPos.x and cursorY > cursorPos.y and cursorX < cursorPos.x + size and cursorY < cursorPos.y + size then
				--hovered = true
				--ig.pushStyleColor(styleColors.ChildBg, vec4.new(0.5, 0.5, 0.5, 1))
			end
		end
		ig.beginChild("item"..tostring(index), vec2.new(size, size), true, { windowFlags.NoScrollbar })
		if inventory[index] ~= nil then
			local item = self.items[inventory[index].id]
			local amount = inventory[index].amount
			local UVs = self.itemsMap[item.location]
			ig.image(self.itemsTex, vec2.new(size - 16, size - 16), vec4.new(UVs.p, UVs.q, UVs.s, UVs.t))
			if amount ~= nil and amount > 1 then
				local cursorPos = ig.getCursorPos()
				ig.setCursorPos(cursorPos.x, cursorPos.y - 24)
				ig.pushFont(self.largeFont)
				ig.text(tostring(amount))
				ig.popFont()
				ig.setCursorPos(cursorPos.x, cursorPos.y)
			end
		end		
		ig.endChild()
		if ig.isItemHovered() and ignoreHover ~= true then
			if ig.isMouseClicked() then
				local temp = inventory[index]
				inventory[index] = inventory.hand
				inventory.hand = temp
			elseif inventory[index] ~= nil then
				local item = self.items[inventory[index].id]
				ig.beginTooltip()
				ig.pushFont(self.largeFont)
				ig.pushTextWrapPos(select(1, ig.calcTextSize(item.name) + 16))
				ig.text(item.name)
				ig.popTextWrapPos()
				ig.popFont()
				ig.endTooltip()
			end
		end
		if hovered then ig.popStyleColor(1) end
	end,
	setInvSize = function(self, args)
		if #args == 2 then
			self.cols = math.floor(tonumber(args[1]))
			self.rows = math.floor(tonumber(args[2]))
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: invSize {columns} {rows}\n\tSets the size of the inventory to {rows} rows of {columns} items each")
		end
	end,
	setHotbarWidth = function(self, args)
		if #args == 1 then
			self.hotbarWidth = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: hotbarWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the hotbar take up")
		end
	end,
	setInventoryWidth = function(self, args)
		if #args == 1 then
			self.inventoryWidth = tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: inventoryWidth {width}\n\tSets what percentage of the width of the screen (from 0 to 1) does the inventory take up")
		end
	end
}
