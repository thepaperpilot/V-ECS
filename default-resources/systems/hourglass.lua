return {
	forwardDependencies = {
		sprite = "renderer",
		stone = "system",
		starnest = "renderer"
	},
	preInit = function(self)
		local vertexBuffer = buffer.new(bufferUsage.VertexBuffer, 16 * sizes.Float)
		
		local indexBuffer = buffer.new(bufferUsage.IndexBuffer, 6 * sizes.Float)
		indexBuffer:setDataInts({ 0, 1, 2, 2, 1, 3 })

		local width, height = glfw.windowSize()
		local spriteHeight = height * 0.6
		local spriteWidth = spriteHeight / 2 / (width / 2)
		local widthFromEdge = 0.2

		-- TODO make stone and hourglass one entity, and make the hourglass sprite separate
		-- Then separate the hourglass logic code from the hourglass rendering code, etc.
		--  so that the logic systems can be implemented in Kronos Chapter 2 eventually
		createEntity({
			Sprite = {
				location = "resources/textures/hourglass.png",
				filename = "hourglass.png", -- TODO make this get set in sprite renderer automatically, or make it use this value when creating the texturesMap
				indexBuffer = indexBuffer,
				vertexBuffer = vertexBuffer,
				position = vec3.new(1 - widthFromEdge - spriteWidth / 2, 0.2, 0),
				rotation = 0,
				indexCount = 6
			},
			Hourglass = {
				stonesChipped = 0,
				flipping = false,
				targetRotation = 0,
				flipDuration = 5,
				fallDuration = 4,
				fallTime = 0,
				grainsRemaining = 0,
				grainsFallen = 0,
				timePoints = 0,
				xp = 0,
				xpReq = 10,
				level = 0,
				autoChip = 0,
				autoChipPerZoom = 0,
				autoFlip = 0,
				autoFlipTime = 0,
				chipEff = {
					level = 0,
					title = "Chipping Efficiency",
					description = "Additively increase how much the stone shrinks with each chip",
					calculateCost = function(level) return 10 * 2 ^ level end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 1 + 0.1 * level).." chipping effectiveness" end
				},
				chipSpeed = {
					level = 0,
					title = "Chipping Speed",
					description = "Additively increase how many times per second the stone gets chipped",
					calculateCost = function(level) return 2 * 5 ^ (level + 1) end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 1 + 0.2 * level).." chipping speed" end
				},
				fallSpeed = {
					level = 0,
					title = "Channel Lubricant",
					description = "Additively increases how quickly grains of sand fall through the hourglass",
					calculateCost = function(level) return 2 ^ (level + 3) end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 1 + 0.5 * level).." falling speed" end
				},
				fallAmount = {
					level = 0,
					title = "Channel Size",
					description = "Additively increases how many grains of sand can fall through the hourglass at once",
					calculateCost = function(level) return (4 ^ (level + 2)) * 1.5 end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 1 + level).." grains" end
				},
				flipSpeed = {
					level = 0,
					title = "Hourglass Flipping Speed",
					description = "Multiplicatively increases how quickly the hourglass flips",
					calculateCost = function(level) return 3 * 2 ^ (level + 1) end,
					calculateEffect = function(level) return "x"..string.format("%.3g", math.floor(100 * (1.25 ^ level)) / 100).." flip speed" end
				},
				timeMult = {
					level = 0,
					title = "Hourglass Timebines",
					description = "Additively increases how many time points are collected from every grain that falls through the hourglass",
					calculateCost = function(level) return 50 * 3 ^ level end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 1 + level).." collected time points" end
				},
				grainsAddMod = {
					level = 0,
					title = "Grain Collection Efficiency",
					description = "Additively and retroactively increases how many grains of sand you gather from each completely chipped stone",
					calculateCost = function(level) return 75 * 10 ^ level end,
					calculateEffect = function(level) return "+"..string.format("%.3g", level).." grains" end,
					onPurchase = function(hourglass, level)
						hourglass.grainsRemaining = hourglass.grainsRemaining + hourglass.stonesChipped
					end
				},
				grainsMulMod = {
					level = 0,
					title = "Larger Stones",
					description = "Multiplicatively and retroactively increases how many grains of sand you gather by starting with even larger stones",
					calculateCost = function(level) return 100 ^ (level + 1) end,
					calculateEffect = function(level) return "x"..string.format("%.3g", 2 ^ level).." grains, x"..string.format("%.3g", 10 ^ level).." initial stone size" end,
					onPurchase = function(hourglass, level)
						hourglass.grainsRemaining = 2 * hourglass.grainsRemaining + hourglass.grainsFallen
					end
				},
				-- one-time purchases
				largerGrains = {
					bought = false,
					title = "Even Larger Stones",
					description = "Massively reduces the cost scaling of the \"Larger Stones\" repeatable upgrade",
					cost = 1e6,
					onPurchase = function(hourglass)
						hourglass.grainsMulMod.calculateCost = function(level) return 25 ^ (level + 1) end
					end
				},
				heavyGrains = {
					bought = false,
					title = "Heavier Grains",
					description = "Raise the falling speed multiplier based on total number of grains to the 1.5 power",
					cost = 1e9
				},
				betterManual = {
					bought = false,
					title = "Why does active play not do anything?",
					description = "Multiply the effectiveness of holding the stone by the current XP level",
					cost = 1e12
				},
				fasterAutomators = {
					bought = false,
					title = "Faster Automators",
					description = "Add +25%% effectiveness to all automators per XP level",
					cost = 1e12,
					onPurchase = function(hourglass)
						hourglass.autoChip = hourglass.autoChip + 0.25 * hourglass.level
						hourglass.autoFlip = hourglass.autoFlip + 0.25 * hourglass.level
					end
				}
			}
		})

		-- Make event to register the hourglass texture in our imgui renderer
		createEntity({
			RegisterTextureEvent = {
				location = "resources/gamejam/hourglass.png",
				id = "sandsoftime:hourglass"
			}
		})

		-- Make background entity
		local snVertexBuffer = buffer.new(bufferUsage.VertexBuffer, 16 * sizes.Float)
		snVertexBuffer:setDataFloats({
			-1, -1, 0, 0,
			-1, 1, 0, 1,
			1, -1, 1, 0,
			1, 1, 1, 1
		})
		local snIndexBuffer = buffer.new(bufferUsage.IndexBuffer, 6 * sizes.Float)
		snIndexBuffer:setDataInts({ 0, 1, 2, 2, 1, 3 })
		createEntity({
			StarNest = {		
				vertexBuffer = snVertexBuffer,
				indexBuffer = snIndexBuffer,
				indexCount = 6,
				time = 0,
				velocity = 0,
				alpha = 0,
				enabled = true
			}
		})

		self.hourglass = archetype.new({ "Sprite", "Hourglass" })
		self.background = archetype.new({ "StarNest" })

		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 30)
		self.mediumFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 24)
	end,
	postInit = function(self)
		for id,sprite in self.hourglass:getComponents("Sprite"):iterate() do
			-- set sprite UVs
			local width, height = glfw.windowSize()
			local spriteHeight = height * 0.6
			local spriteWidth = spriteHeight / 2 / (width / 2)
			local widthFromEdge = width * 0.1

			sprite.vertexBuffer:setDataFloats({
				-spriteWidth / 2, -0.6, sprite.UVs.p, sprite.UVs.q,
				-spriteWidth / 2, 0.6, sprite.UVs.p, sprite.UVs.t,
				spriteWidth / 2, -0.6, sprite.UVs.s, sprite.UVs.q,
				spriteWidth / 2, 0.6, sprite.UVs.s, sprite.UVs.t
			})
		end

		local registerTextureEventArchetype = archetype.new({ "RegisterTextureEvent" })
		for index,event in registerTextureEventArchetype:getComponents("RegisterTextureEvent"):iterate() do
			if event.texture ~= nil and event.id == "sandsoftime:hourglass" then
				self.hourglassTex = event.texture
				registerTextureEventArchetype:deleteEntity(index)
			end
		end
	end,
	update = function(self)
		local width, height = glfw.windowSize()

		-- bottom bar
		ig.lock()

		ig.setNextWindowPos(0, height - 36)
		ig.setNextWindowSize(width, 36)

		ig.pushStyleVar(styleVars.WindowRounding, 0)

		ig.beginWindow("menu", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav
		})

		for snId,starnest in self.background:getComponents("StarNest"):iterate() do
			starnest.enabled = ig.checkbox("Special Effects Enabled", starnest.enabled)
		end

		ig.sameLine()

		local vsync = engine.getVsyncEnabled()
		if ig.checkbox("VSync", vsync) ~= vsync then
			engine.setVsyncEnabled(not vsync)
		end

		ig.setCursorPos(width - 300, 10)

		ig.text("Check me out:")

		ig.setCursorPos(width - 300 + ig.calcTextSize("Check me out:") + 8, 8)

		if ig.button("thepaperpilot.org") then
			openURL("http://www.thepaperpilot.org")
		end

		ig.sameLine()

		if ig.button("Discord") then
			openURL("https://discord.gg/WzejVAx")
		end

		ig.endWindow()

		ig.popStyleVar(1)

		ig.release()

		for id,hourglass in self.hourglass:getComponents("Hourglass"):iterate() do
			-- xp and timepoints display
			if hourglass.xp > 0 then
				-- starnest background
				for snId,starnest in self.background:getComponents("StarNest"):iterate() do
					if starnest.enabled then
						if starnest.alpha < 1 then
							starnest.alpha = math.min(1, starnest.alpha + time.getDeltaTime())
						end
						if starnest.velocity < hourglass.level then
							starnest.velocity = math.min(hourglass.level, starnest.velocity + time.getDeltaTime())
						end
						starnest.time = starnest.time + time.getDeltaTime() * starnest.velocity
					end
				end

				ig.lock()

				ig.pushStyleVar(styleVars.WindowPadding, 0, 0)
				ig.pushStyleVar(styleVars.WindowBorderSize, 4)
				ig.pushStyleColor(styleColors.Border, vec4.new(1, 1, 1, 1))

				ig.setNextWindowPos(math.floor(width * 0.1), 25)
				ig.setNextWindowSize(math.floor(width * 0.8), 75)

				ig.beginWindow("xp", nil, {
					windowFlags.NoTitleBar,
					windowFlags.NoMove,
					windowFlags.NoResize,
					windowFlags.NoCollapse,
					windowFlags.NoNav
				})

				ig.popStyleColor(1)
				ig.popStyleVar(2)

				-- Draw icon on left side of window
				local aspectRatio = 1 / 2
				local imgWidth,imgHeight
				local cursorPos = ig.getCursorPos()
				-- padding on each side of the panel is 8
				local desiredSize = 75 - 16

				-- todo abstract into utility function
				if aspectRatio > 1 then
					imgWidth = desiredSize
					imgHeight = desiredSize / aspectRatio
					ig.setCursorPos(cursorPos.x, (75 - imgHeight) / 2)
				else
					imgWidth = desiredSize * aspectRatio
					imgHeight = desiredSize
					ig.setCursorPos(8 + (desiredSize - imgWidth) / 2, 8)
				end

				ig.image(self.hourglassTex, vec2.new(imgWidth, imgHeight))

				-- Set cursor to draw the rest next to the icon
				-- TODO is there a way to not have to set this each time?
				ig.setCursorPos(75, 8)
				ig.text("Sands of Time (lv."..tostring(hourglass.level)..")")
				ig.setCursorPos(75, ig.getCursorPos().y)
				ig.text(string.format("%.3g", hourglass.xp).." / "..string.format("%.3g", hourglass.xpReq).." towards next level")
				ig.setCursorPos(75, ig.getCursorPos().y)
				ig.progressBar(hourglass.xp / hourglass.xpReq, vec2.new(math.floor(width * 0.8) - 75 - 16, 0))
				ig.setCursorPos(375, 8)
				if hourglass.level == 0 then
					ig.textWrapped("Next level unlocks the Time Store,\nand everything runs 10%% faster")
				elseif hourglass.level == 1 then
					ig.textWrapped("Next level unlocks 50%% efficient auto-chipping even while not holding,\nand everything runs 10%% faster")
				elseif hourglass.level == 2 then
					ig.textWrapped("Next level unlocks .01%% * total grains multiplier towards grain falling speed,\nand everything runs 10%% faster")
				elseif hourglass.level == 3 then
					ig.textWrapped("Next level unlocks multiplicative +10%% auto-chipping efficiency per zoom level,\nand everything runs 10%% faster")
				elseif hourglass.level == 4 then
					ig.textWrapped("Next level unlocks 50%% efficient hourglass auto-flipping,\nand everything runs 10%% faster")
				elseif hourglass.level == 5 then
					ig.textWrapped("Next level unlocks +50%% efficiency for all automation,\nand everything runs 10%% faster")
				elseif hourglass.level == 6 then
					ig.textWrapped("Next level unlocks an expansion to the time shop,\nand everything runs 10%% faster")
				elseif hourglass.level == 7 then
					ig.textWrapped("Next level unlocks auto-buying repeatable upgrades every frame from the bottom up,\nand everything runs 10%% faster")
				else
					ig.textWrapped("Next level everything runs 10%% faster")
				end

				ig.endWindow()

				ig.pushStyleColor(styleColors.Border, vec4.new(0, 0, 0, 0))
				ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, 0))

				ig.setNextWindowPos(math.floor(width * 0.1), 125)
				ig.setNextWindowSize(math.floor(width * 0.8), 75)

				ig.beginWindow("timepoints", nil, {
					windowFlags.NoTitleBar,
					windowFlags.NoMove,
					windowFlags.NoResize,
					windowFlags.NoCollapse,
					windowFlags.NoNav
				})

				ig.popStyleColor(2)

				ig.pushFont(self.largeFont)
				self.center(string.format("%.3g", hourglass.timePoints).." Time Points", width * 0.4)
				ig.popFont()

				ig.endWindow()

				ig.release()
			end

			-- flipping hourglass
			local deltaTime = time.getDeltaTime() * (1.1 ^ hourglass.level)
			if hourglass.flipping then
				local sprite = self.hourglass:getComponents("Sprite")[id]
				sprite.rotation = sprite.rotation + deltaTime * 180 * (1.25 ^ hourglass.flipSpeed.level) / hourglass.flipDuration
				if sprite.rotation >= hourglass.targetRotation then
					sprite.rotation = hourglass.targetRotation
					hourglass.flipping = false
				end
			end

			-- hourglass
			if (not hourglass.flipping) and (hourglass.grainsFallen > 0 or hourglass.grainsRemaining > 0) then
				if hourglass.grainsRemaining > 0 then
					local delta = deltaTime * (1 + 0.5 * hourglass.fallSpeed.level)
					if hourglass.level >= 3 then
						local mod = 0.01 * (hourglass.grainsRemaining + hourglass.grainsFallen)
						if hourglass.heavyGrains.bought then mod = math.pow(mod, 1.5) end
						delta = delta * (1 + mod)
					end
					hourglass.fallTime = hourglass.fallTime + delta
				end
				if not hourglass.flipping then
					local fallenGrains = math.min(math.ceil(hourglass.grainsRemaining / (1 + hourglass.fallAmount.level)), math.floor(hourglass.fallTime / hourglass.fallDuration))
					if fallenGrains > 0 then
						hourglass.fallTime = hourglass.fallTime - hourglass.fallDuration * fallenGrains
						local amount = math.min(hourglass.grainsRemaining, fallenGrains * (1 + hourglass.fallAmount.level))
						hourglass.grainsRemaining = hourglass.grainsRemaining - amount
						hourglass.grainsFallen = hourglass.grainsFallen + amount
						hourglass.timePoints = hourglass.timePoints + (hourglass.timeMult.level + 1) * amount
						hourglass.xp = hourglass.xp + amount
						while hourglass.xp >= hourglass.xpReq do
							hourglass.level = hourglass.level + 1
							hourglass.xpReq = hourglass.xpReq * 10
							
							if hourglass.level == 2 then
								hourglass.autoChip = 0.5
							elseif hourglass.level == 4 then
								hourglass.autoChipPerZoom = 0.1
							elseif hourglass.level == 5 then
								hourglass.autoFlip = 0.5
							elseif hourglass.level == 6 then
								hourglass.autoChip = hourglass.autoChip + 0.5
								hourglass.autoFlip = hourglass.autoFlip + 0.5
							end

							if hourglass.fasterAutomators then
								hourglass.autoChip = hourglass.autoChip + 0.25
								hourglass.autoFlip = hourglass.autoFlip + 0.25
							end
						end
					end

					ig.lock()

					local spriteHeight = height * 0.6
					local spriteWidth = spriteHeight / 2
					local widthFromEdge = width * 0.1

					ig.pushStyleColor(styleColors.Border, vec4.new(0, 0, 0, 0))
					ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, 0))
					ig.pushStyleColor(styleColors.Text, vec4.new(1, 0.5, 0, 1))

					ig.setNextWindowPos(math.floor(width - widthFromEdge - spriteWidth * 0.8), math.floor(height * 0.3 + spriteHeight * 0.41 - spriteWidth * 0.6))
					ig.setNextWindowSize(math.floor(spriteWidth * 0.6), math.floor(spriteWidth * 0.6))

					ig.beginWindow("grainsRemaining", nil, {
						windowFlags.NoTitleBar,
						windowFlags.NoMove,
						windowFlags.NoResize,
						windowFlags.NoCollapse,
						windowFlags.NoNav
					})

					ig.pushFont(self.largeFont)
					ig.setCursorPos(ig.getCursorPos().x, (math.floor(spriteWidth * 0.6) - 30) / 2)
					self.center(string.format("%.3g", hourglass.grainsRemaining), math.floor(spriteWidth * 0.6 / 2))
					ig.popFont()

					ig.endWindow()

					ig.popStyleColor(1)
					ig.pushStyleColor(styleColors.Text, vec4.new(1, 0, 0, 1))

					ig.setNextWindowPos(math.floor(width - widthFromEdge - spriteWidth * 0.8), math.floor(height * 0.9 - spriteHeight * 0.41))
					ig.setNextWindowSize(math.floor(spriteWidth * 0.6), math.floor(spriteWidth * 0.6))

					ig.beginWindow("grainsFallen", nil, {
						windowFlags.NoTitleBar,
						windowFlags.NoMove,
						windowFlags.NoResize,
						windowFlags.NoCollapse,
						windowFlags.NoNav
					})

					ig.pushFont(self.largeFont)
					ig.setCursorPos(ig.getCursorPos().x, (math.floor(spriteWidth * 0.6) - 30) / 2)
					self.center(string.format("%.3g", hourglass.grainsFallen), math.floor(spriteWidth * 0.6 / 2))
					ig.popFont()

					ig.endWindow()

					ig.popStyleColor(3)

					if hourglass.grainsFallen > 0 then
						ig.setNextWindowPos(math.floor(width - widthFromEdge - spriteWidth), math.floor(height * 0.9 - 48))
						ig.setNextWindowSize(math.floor(spriteWidth), 48)

						ig.beginWindow("flipHourglass", nil, {
							windowFlags.NoTitleBar,
							windowFlags.NoMove,
							windowFlags.NoResize,
							windowFlags.NoCollapse,
							windowFlags.NoNav
						})

						if ig.isWindowHovered() and ig.isMouseClicked() then
							self.flip(hourglass)
						elseif hourglass.grainsRemaining == 0 then
							hourglass.autoFlipTime = hourglass.autoFlipTime + deltaTime * hourglass.autoFlip * 1.25 ^ hourglass.flipSpeed.level
							if hourglass.autoFlipTime >= hourglass.flipDuration then
								self.flip(hourglass)
								hourglass.autoFlipTime = 0
							end
						end

						ig.pushFont(self.largeFont)
						self.center("Flip Hourglass", math.floor(spriteWidth / 2))
						ig.popFont()

						ig.endWindow()
					end

					ig.release()
				end
			end

			-- time points shop
			if hourglass.level >= 1 then
				ig.lock()

				ig.setNextWindowPos(math.floor(width * 0.02), math.floor(height * 0.3))
				ig.setNextWindowSize(math.floor(width * 0.31), math.floor(height * 0.6))

				ig.beginWindow("Points Store", nil, {
					windowFlags.NoTitleBar,
					windowFlags.NoMove,
					windowFlags.NoResize,
					windowFlags.NoCollapse,
					windowFlags.NoNav
				})

				ig.pushFont(self.largeFont)
				self.center("Time Store", math.floor(width * 0.155))
				ig.popFont()
				ig.separator()

				if hourglass.level >= 7 then
					ig.beginTabBar("time stores", {})

					if ig.beginTabItem("Basic Store", nil, {}) then
						self:basicShop(hourglass)

						ig.endTabItem()
					else
						-- still auto-buy from the basic shop even when its not rendering
						self.handlePurchase(hourglass, hourglass.chipEff, false)
						self.handlePurchase(hourglass, hourglass.chipSpeed, false)
						self.handlePurchase(hourglass, hourglass.fallSpeed, false)
						self.handlePurchase(hourglass, hourglass.fallAmount, false)
						self.handlePurchase(hourglass, hourglass.flipSpeed, false)
						self.handlePurchase(hourglass, hourglass.timeMult, false)
						self.handlePurchase(hourglass, hourglass.grainsAddMod, false)
						self.handlePurchase(hourglass, hourglass.grainsMulMod, false)
					end

					if ig.beginTabItem("Advanced Store", nil, {}) then
						ig.beginChild("timestorescrollView", vec2.new(0, 0), false, {})
						self:oneTimeUpgradeDisplay(hourglass, hourglass.largerGrains)
						self:oneTimeUpgradeDisplay(hourglass, hourglass.heavyGrains)
						self:oneTimeUpgradeDisplay(hourglass, hourglass.betterManual)
						self:oneTimeUpgradeDisplay(hourglass, hourglass.fasterAutomators)
						ig.endChild()

						ig.endTabItem()
					end

					ig.endTabBar()
				else
					self:basicShop(hourglass)
				end

				ig.endWindow()

				ig.release()
			end
		end
	end,
	center = function(message, centerX)
		local textWidth, textHeight = ig.calcTextSize(message)
		ig.setCursorPos(centerX - textWidth / 2, ig.getCursorPos().y)
		ig.text(message)
	end,
	flip = function(hourglass)
		hourglass.flipping = true
		hourglass.fallTime = 0
		hourglass.grainsRemaining = hourglass.grainsRemaining + hourglass.grainsFallen
		hourglass.grainsFallen = 0
		hourglass.targetRotation = hourglass.targetRotation + 180
		hourglass.autoFlipTime = 0
	end,
	basicShop = function(self, hourglass)
		ig.beginChild("timestorescrollView", vec2.new(0, 0), false, {})
		self:upgradeDisplay(hourglass, hourglass.chipEff)
		self:upgradeDisplay(hourglass, hourglass.chipSpeed)
		self:upgradeDisplay(hourglass, hourglass.fallSpeed)
		self:upgradeDisplay(hourglass, hourglass.fallAmount)
		self:upgradeDisplay(hourglass, hourglass.flipSpeed)
		self:upgradeDisplay(hourglass, hourglass.timeMult)
		self:upgradeDisplay(hourglass, hourglass.grainsAddMod)
		self:upgradeDisplay(hourglass, hourglass.grainsMulMod)
		ig.endChild()
	end,
	upgradeDisplay = function(self, hourglass, upgrade)
		ig.pushFont(self.mediumFont)
		ig.textWrapped(upgrade.title)
		ig.popFont()
		ig.textWrapped(upgrade.description)
		self.handlePurchase(hourglass, upgrade, true)
		ig.sameLine()
		ig.textWrapped(upgrade.calculateEffect(upgrade.level).." -> "..upgrade.calculateEffect(upgrade.level + 1))

		ig.separator()
	end,
	oneTimeUpgradeDisplay = function(self, hourglass, upgrade)
		ig.pushFont(self.mediumFont)
		ig.textWrapped(upgrade.title)
		ig.popFont()
		ig.textWrapped(upgrade.description)

		local disabled = hourglass.timePoints < upgrade.cost or upgrade.bought
		if disabled then
			ig.pushStyleColor(styleColors.Button, vec4.new(1, 1, 1, 0.5))
			ig.pushStyleColor(styleColors.ButtonActive, vec4.new(1, 1, 1, 0.5))
			ig.pushStyleColor(styleColors.ButtonHovered, vec4.new(1, 1, 1, 0.5))
		end
		local buttonText
		if upgrade.bought then
			buttonText = "Bought!"
		else
			buttonText = string.format("%.3g", upgrade.cost).." Time Points"
		end
		ig.button(buttonText)
		if ig.isItemHovered() and ig.isMouseClicked() and not disabled then
			hourglass.timePoints = hourglass.timePoints - upgrade.cost
			upgrade.bought = true
			if upgrade.onPurchase ~= nil then
				upgrade.onPurchase(hourglass)
			end
		end
		if disabled then
			ig.popStyleColor(3)
		end

		ig.separator()
	end,
	handlePurchase = function(hourglass, upgrade, render)
		local cost = upgrade.calculateCost(upgrade.level)
		local disabled = hourglass.timePoints < cost
		if render and disabled then
			ig.pushStyleColor(styleColors.Button, vec4.new(1, 1, 1, 0.5))
			ig.pushStyleColor(styleColors.ButtonActive, vec4.new(1, 1, 1, 0.5))
			ig.pushStyleColor(styleColors.ButtonHovered, vec4.new(1, 1, 1, 0.5))
		end
		if render then
			ig.button(string.format("%.3g", upgrade.calculateCost(upgrade.level)).." Time Points")
		end
		if ((render and ig.isItemHovered() and ig.isMouseClicked()) or hourglass.level >= 8) and not disabled then
			hourglass.timePoints = hourglass.timePoints - cost
			upgrade.level = upgrade.level + 1
			if upgrade.onPurchase ~= nil then
				upgrade.onPurchase(hourglass, upgrade.level)
			end
		end
		if render and disabled then
			ig.popStyleColor(3)
		end
	end
}
