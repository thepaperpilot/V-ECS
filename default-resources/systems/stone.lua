return {
	forwardDependencies = {
		stone = "renderer",
		imgui = "renderer"
	},
	preInit = function(self)
		createEntity({
			Stone = {
				position = vec3.new(0, 1, 4),
				rotation = 0,
				scale = vec3.new(1, 1, 1),
				size = 1000,
				initialSize = 1000,
				holdTime = 0,
				hardness = 0.95
			}
		})

		self.stone = archetype.new({ "Stone" })
		self.grains = archetype.new({ "Sprite", "Hourglass" })

		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 48)
		self.mediumFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 36)
	end,
	update = function(self)
		local delta = time.getDeltaTime()
		local grains
		for id,hourglass in self.grains:getComponents("Hourglass"):iterate() do
			delta = delta * (1.1 ^ hourglass.level)
			grains = hourglass
			break
		end
		for id,stone in self.stone:getComponents("Stone"):iterate() do
			ig.lock()

			ig.pushFont(self.mediumFont)

			local width, height = glfw.windowSize()
			local stoneHeight = height * 0.5
			ig.setNextWindowPos(math.floor((width - stoneHeight) / 2), math.floor(height * 0.3))
			ig.setNextWindowSize(math.floor(stoneHeight), math.floor(height * 0.6))

			ig.beginWindow("Chip Stone", nil, {
				windowFlags.NoTitleBar,
				windowFlags.NoMove,
				windowFlags.NoResize,
				windowFlags.NoCollapse,
				windowFlags.NoNav,
				windowFlags.NoBringToFrontOnFocus
			})

			local chipped = self.checkChippingStone(stone, grains, delta)

			self.center("Hold to chip stone", width * 0.5 - math.floor((width - stoneHeight) / 2))

			local size
			if stone.size >= 1000000 then
				size = string.format("%.3g", stone.size / 1000000).." km"
			elseif stone.size >= 1000 then
				size = string.format("%.3g", stone.size / 1000).." m"
			elseif stone.size >= 10 then
				size = string.format("%.3g", stone.size / 10).." cm"
			else
				size = string.format("%.3g", stone.size).." mm"
			end
			ig.pushFont(self.largeFont)
			self.center(size, width * 0.5 - math.floor((width - stoneHeight) / 2) - 18)
			ig.popFont()
			ig.sameLine()
			ig.text("3")

			ig.endWindow()

			if stone.size <= 100 or stone.size > 1000 then
				local zoom = math.floor(math.log10(stone.size)) - 2
				local hue = zoom * 30
				while hue < 0 do hue = hue + 360 end
				while hue >= 360 do hue = hue - 360 end
				local r, g, b = self.HSVToRGB(hue, 1, 1)
				local zoomColor = vec4.new(r, g, b, 0.1)

				ig.pushStyleVar(styleVars.WindowRounding, 50)
				ig.pushStyleColor(styleColors.WindowBg, zoomColor)

				ig.setNextWindowPos(math.floor((width - stoneHeight) / 2 + 5), math.floor(height * 0.45))
				ig.setNextWindowSize(math.floor(stoneHeight - 10), math.floor(height * 0.45) - 5)

				ig.beginWindow("Chip Stone Zoom", nil, {
					windowFlags.NoTitleBar,
					windowFlags.NoMove,
					windowFlags.NoResize,
					windowFlags.NoCollapse,
					windowFlags.NoNav
				})
				
				if not chipped then
					chipped = self.checkChippingStone(stone, grains, delta)
				end

				self.center(string.format("%.3g", 1 / 10 ^ zoom).."x Zoom", width * 0.5 - math.floor((width - stoneHeight) / 2 + 5))

				ig.endWindow()

				ig.popStyleColor(1)
				ig.popStyleVar(1)
			else
				-- Still create the window, so "isWindowActive" keeps working correctly
				ig.pushStyleVar(styleVars.WindowRounding, 50)
				ig.pushStyleColor(styleColors.Border, vec4.new(0, 0, 0, 0))
				ig.pushStyleColor(styleColors.WindowBg, vec4.new(0, 0, 0, 0))

				ig.setNextWindowPos(math.floor((width - stoneHeight) / 2 + 5), math.floor(height * 0.45))
				ig.setNextWindowSize(math.floor(stoneHeight - 10), math.floor(height * 0.45) - 5)

				ig.beginWindow("Chip Stone Zoom", nil, {
					windowFlags.NoTitleBar,
					windowFlags.NoMove,
					windowFlags.NoResize,
					windowFlags.NoCollapse,
					windowFlags.NoNav
				})
				
				if not chipped then
					chipped = self.checkChippingStone(stone, grains, delta)
				end

				ig.endWindow()

				ig.popStyleColor(2)
				ig.popStyleVar(1)
			end

			if grains.autoChip > 0 then
				local zoomLevel = math.floor(math.log10(stone.initialSize * (10 ^ grains.grainsMulMod.level) / stone.size))
				local autoDelta = delta * (grains.autoChip + grains.autoChipPerZoom ^ zoomLevel)
				chipped = self.checkChippingStone(stone, grains, autoDelta, true)
			end

			if not chipped then
				stone.holdTime = 0
			end

			ig.popFont()

			ig.release()

			if delta < 0.1 then
				stone.rotation = stone.rotation + delta * 10
			end
		end
	end,
	center = function(message, centerX)
		local textWidth, textHeight = ig.calcTextSize(message)
		ig.setCursorPos(centerX - textWidth / 2, ig.getCursorPos().y)
		ig.text(message)
	end,
	checkChippingStone = function(stone, grains, delta, override)
		if override == true or ig.isItemActive() then
			if not override and grains.betterManual.bought then
				delta = delta * grains.level
			end
			stone.holdTime = stone.holdTime + delta * 10 * (1 + 0.2 * grains.chipSpeed.level)
			while stone.holdTime > 1 do
				stone.holdTime = stone.holdTime - 1
				stone.size = (stone.hardness / (1 + 0.1 * grains.chipEff.level)) * stone.size
				
				if stone.size <= 1 then
					stone.size = stone.initialSize * (10 ^ grains.grainsMulMod.level)
					stone.hardness = 1 - (1 - stone.hardness) * 0.6

					grains.grainsRemaining = grains.grainsRemaining + (1 + grains.grainsAddMod.level) * (2 ^ grains.grainsMulMod.level)
					grains.stonesChipped = grains.stonesChipped + 1
					grains.autoFlipTime = 0
				end

				local zoom = stone.size / 10 ^ (math.floor(math.log10(stone.size)) + 1)
				stone.scale = vec3.new(zoom, zoom, zoom)
			end
			return true
		end
		return false
	end,
	-- https://gist.github.com/GigsD4X/8513963
	HSVToRGB = function(hue, saturation, value)
		-- Returns the RGB equivalent of the given HSV-defined color
		-- (adapted from some code found around the web)

		-- If it's achromatic, just return the value
		if saturation == 0 then
			return value, value, value
		end

		-- Get the hue sector
		local hue_sector = math.floor(hue / 60)
		local hue_sector_offset = (hue / 60) - hue_sector

		local p = value * (1 - saturation)
		local q = value * (1 - saturation * hue_sector_offset)
		local t = value * (1 - saturation * (1 - hue_sector_offset))

		if hue_sector == 0 then
			return value, t, p
		elseif hue_sector == 1 then
			return q, value, p
		elseif hue_sector == 2 then
			return p, value, t
		elseif hue_sector == 3 then
			return p, q, value
		elseif hue_sector == 4 then
			return t, p, value
		elseif hue_sector == 5 then
			return value, p, q
		end
	end
}
