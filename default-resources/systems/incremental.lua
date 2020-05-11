local createJob = function(name, description, baseDuration, icon, effect, onUpgrade)
	return {
		name = name,
		description = description,
		duration = baseDuration,
		icon = icon,
		level = 1,
		exp = 0,
		progress = 0,
		active = false,
		effect = effect,
		onUpgrade = onUpgrade
	}
end

local teach = function() return createJob("Teach", "Improve how much each producer produces by teaching them how to produce", 10, "adventurer_stand.png", function(self, game, amount) game.producerMulti = game.producerMulti + .1 * amount end, nil) end
local research = function(self) return createJob("Research time paradoxes", "Delve deep into long books looking for answers", self.researchSpeed, "tile_bookcaseFull.png", function(self, game, amount) game.nextRp = game.nextRp + amount end, nil) end
local metaResearch = function() return createJob("Read ancient texts", "Find more information that can help speed up research", 100, "displayCaseBooks_S.png",
	function(self, game, amount)
		game.researchSpeed = game.researchSpeed * (.95 ^ amount)
		-- find existing research job if it exists
		for index,job in ipairs(game.jobs) do
			if job.name == "Research time paradoxes" then
				job.duration = job.duration * (.95 ^ amount)
				break
			end
		end
	end, nil) end
local timeFlux = function() return createJob("Gather time flux", "Harvest a substance that can speed up everything", 20, "tile_flowerBlue.png", function(self, game, amount) game.tickSpeed = game.tickSpeed + .05 * amount end, nil) end
local makeProducers = function() return createJob("Make producers", "Find more producers", 2, "pick_silver.png",
	function(self, game, amount) game.producers = game.producers + amount end, -- effect
	function(self, game) -- onUpgrade
		if self.level == 2 then
			table.insert(game.jobs, teach())
			table.insert(game.jobs, research(game))
		end
	end
) end

local upgrades = function() return {
	{
		name = "Better producers",
		description = "Increases how much each producer produces",
		cost = 10,
		effect = function(self) self.producerMulti = self.producerMulti + .5 end,
		threshold = 0,
		icon = "hammer.png"
	},
	{
		name = "How to produce: A guide for dummies",
		description = "Increases how much you can manually produce",
		cost = 10,
		effect = function(self) self.manualMulti = self.manualMulti + .5 end,
		threshold = 0,
		icon = "book.png"
	},
	{
		name = "Ascension",
		description = "Unlock a new kind of experience...",
		cost = 200,
		effect = function(self)
			self.timeSlots = 1
			self.producerMulti = 2
			table.insert(self.jobs, makeProducers())
		end,
		threshold = 100,
		icon = "drugs.png"
	}
} end

local secretUpgrades = function() return {
	{
		name = "Ancestral Influence",
		description = "Gain a tick speed multiplier based on how many research points gained all time",
		cost = 1000,
		effect = function(self) self.ancestralInfluence = 1 + self.rpAllTime * .1 end,
		icon = "platformPack_tile011.png"
	},
	{
		name = "Ancient texts",
		description = "Unlocks a new job that influences research speed",
		cost = 10000,
		effect = function(self) table.insert(self.jobs, metaResearch()) end,
		icon = "displayCaseBooks_S.png"
	}
} end

local rpUpgrades = function() return {
	{
		name = "Secret Shop",
		description = "Unlocks a new ascension shop with new upgrades",
		cost = 1,
		effect = function(self) self.secretShop = true end,
		icon = "market_stallRed.png"
	},
	{
		name = "A tiny loan",
		description = "Start transcensions with more than nothing",
		cost = 2,
		effect = function(self)
			self.initialCurrency = 250
			self.currency = self.currency + 250
			self.hitTen = true
		end,
		icon = "coin_01.png"
	},
	{
		name = "Temporal flux",
		description = "Unlocks a new job that influences everything's speed",
		cost = 5,
		effect = function(self)
			self.timeFlux = true
			table.insert(self.jobs, timeFlux())
		end,
		icon = "tile_flowerBlue.png"
	}
} end

function center(message, centerX)
	local textWidth, textHeight = ig.calcTextSize(message)
	ig.setCursorPos(centerX - textWidth / 2, ig.getCursorPos().y)
	ig.text(message)
end

return {
	currencyName = "currency",
	currency = 0,
	producers = 0,
	producerMulti = 1,
	ancestralInfluence = 1,
	tickSpeed = 1,
	manualMulti = 1,
	researchSpeed = 100,
	timeSlots = 0,
	hitTen = false,
	deltaTime = 0,
	rp = 0,
	nextRp = 0,
	rpAllTime = 0,
	secretShop = false,
	timeFlux = false,
	initialCurrency = 0,
	upgrades = upgrades(),
	secretUpgrades = secretUpgrades(),
	rpUpgrades = rpUpgrades(),
	availableUpgrades = {},
	jobs = {},
	jobMultipliers = {},
	activeJobs = 0,
	forwardDependencies = {
		imgui = "renderer"
	},
	preInit = function(self, world)
		self.largeFont = ig.addFont("resources/fonts/Roboto-Medium.ttf", 24)
	end,
	init = function(self, world)
		local upgradesTex, upgradesMap = world.renderers.imgui:addStitchedTexture(getResources("textures/incremental", ".png"))
		self.upgradesTex = upgradesTex
		self.upgradesMap = upgradesMap

		if world.systems.debug ~= nil then
			world.systems.debug:addCommand("currency", self, self.setName, "Usage: currency {name}\n\tSets the name of the currency you're producing")
			world.systems.debug:addCommand("addCurrency", self, self.addCurrency, "Usage: addCurrency {amount}\n\tAdds {amount} currency")
			world.systems.debug:addCommand("addProducers", self, self.addProducers, "Usage: addProducers {amount}\n\tAdds {amount} producers")
			world.systems.debug:addCommand("addTimeSlots", self, self.addTimeSlots, "Usage: addTimeSlots {amount}\n\tAdds {amount} time slots")
			world.systems.debug:addCommand("addRP", self, self.addRP, "Usage: addRP {amount}\n\tAdds {amount} research points")
		end
	end,
	update = function(self, world)
		local width, height = glfw.windowSize()
		ig.setNextWindowPos(0, 0)
		ig.setNextWindowSize(width, height)

		ig.beginWindow("Incremental", nil, {
			windowFlags.NoTitleBar,
			windowFlags.NoMove,
			windowFlags.NoResize,
			windowFlags.NoCollapse,
			windowFlags.NoNav,
			windowFlags.NoBackground,
			windowFlags.NoBringToFrontOnFocus
		})

		local currencyIncreased = false

		ig.pushFont(self.largeFont)
		center("You currently have "..tostring(self.currency).." "..self.currencyName, width / 2)
		ig.popFont()
		local nextTimeSlot = 10 ^ (self.timeSlots + 3)

		if self.timeSlots > 0 then
			center("Next time slot in "..tostring(nextTimeSlot - self.currency).." "..self.currencyName, width / 2)
		end

		if self.producers > 0 then
			ig.text("You currently have "..tostring(self.producers).." "..self.currencyName.." producers producing "..tostring(math.ceil(self.producers * self.producerMulti)).." "..self.currencyName.." per tick")
		end

		ig.separator()

		self.deltaTime = self.deltaTime + time.getDeltaTime() * self.tickSpeed * self.ancestralInfluence
		while self.deltaTime >= 1 do
			self.deltaTime = self.deltaTime - 1
			self.currency = self.currency + math.ceil(self.producers * self.producerMulti)
			currencyIncreased = true
		end

		if self.timeSlots == 0 then
			-- Pre-ascension
			if ig.button("Produce "..self.manualMulti.." "..self.currencyName) then
				self.currency = self.currency + self.manualMulti
				currencyIncreased = true

				if self.hitTen == false and self.currency >= 10 then
					self.hitTen = true
				end
			end

			if self.hitTen then
				local producerCost = math.floor((self.producers + 5) ^ 1.5)
				if ig.button("Purchase 1 "..self.currencyName.." producer for "..tostring(producerCost).." "..self.currencyName) then
					if self.currency >= producerCost then
						self.currency = self.currency - producerCost
						self.producers = self.producers + 1
						-- technically it didn't increase, but it may have unlocked the upgrades panel
						currencyIncreased = true
					end
				end
			end

			if self.producers > 2 then
				ig.separator()

				ig.pushFont(self.largeFont)
				ig.text("Upgrades")
				ig.popFont()

				if currencyIncreased then
					for index, upgrade in ipairs(self.upgrades) do
						if upgrade.threshold <= self.currency then
							table.insert(self.availableUpgrades, upgrade)
							table.remove(self.upgrades, index)
						end
					end
				end

				self:upgradesDisplay("availableUpgrades", "currency")
			end
		else
			-- Post-ascension
			ig.pushFont(self.largeFont)
			ig.text("Jobs")
			ig.popFont()

			if currencyIncreased then
				if self.currency >= nextTimeSlot then
					self.timeSlots = self.timeSlots + 1
				end
			end

			ig.text("You currently have "..tostring(self.timeSlots - self.activeJobs).." time slots of "..tostring(self.timeSlots))

			for _,job in ipairs(self.jobs) do
				self:jobDisplay(job)
			end

			if self.secretShop then
				ig.separator()

				ig.pushFont(self.largeFont)
				ig.text("Secret Shop")
				ig.popFont()

				self:upgradesDisplay("secretUpgrades", "currency")
			end
		end

		if self.nextRp ~= 0 or self.rpAllTime ~= 0 then
			ig.separator()

			ig.pushFont(self.largeFont)
			ig.text("Transcend")
			ig.popFont()
			ig.text("You currently have "..tostring(self.rp).." research points")

			if self.nextRp > 0 then
				-- we don't use the button's callback because it won't work anytime the text on the button updates between mouse down and up
				ig.pushFont(self.largeFont)
				ig.button("Transcend for "..tostring(self.nextRp).." research points")
				ig.popFont()

				if ig.isItemHovered() and ig.isMouseClicked() then
					self.currency = self.initialCurrency
					if self.currency >= 10 then
						self.hitTen = true
					end
					self.producers = 0
					self.producerMulti = 1
					self.ancestralInfluence = 1
					self.tickSpeed = 1
					self.manualMulti = 1
					self.researchSpeed = 1
					self.timeSlots = 0
					self.hitTen = false
					self.deltaTime = 0
					self.rp = self.rp + self.nextRp
					self.rpAllTime = self.rpAllTime + self.nextRp
					self.nextRp = 0
					self.upgrades = upgrades()
					self.secretUpgrades = secretUpgrades()
					self.availableUpgrades = {}
					self.jobs = {}
					self.activeJobs = 0

					if self.timeFlux then
						table.insert(self.jobs, timeFlux())
					end
				end
			end			

			self:upgradesDisplay("rpUpgrades", "rp")
		end

		ig.endWindow()
	end,
	upgradesDisplay = function(self, key, currency)
		for index,upgrade in ipairs(self[key]) do
			ig.beginChild("upgrade"..key..tostring(index), vec2.new(125, 200), true, {})

			ig.textWrapped(upgrade.name)

			local img = self.upgradesMap[upgrade.icon]
			local aspectRatio = img.width / img.height
			local width,height
			local cursorPos = ig.getCursorPos()
			-- padding on each side of the panel is 8
			local desiredSize = 125 - 16
			-- 20 is the height of the button
			local buttonHeight = 200 - 8 - 20
			local verticalSpace = buttonHeight - cursorPos.y - 4

			if aspectRatio > 1 then
				width = desiredSize
				height = desiredSize / aspectRatio
				ig.setCursorPos(cursorPos.x, cursorPos.y + (verticalSpace - height) / 2)
			else
				width = desiredSize * aspectRatio
				height = desiredSize
				ig.setCursorPos(cursorPos.x + (desiredSize - width) / 2, cursorPos.y + (verticalSpace - height) / 2)
			end

			ig.image(self.upgradesTex, vec2.new(width, height), vec4.new(img.p, img.q, img.s, img.t))

			ig.setCursorPos(cursorPos.x, 172)

			if ig.button(upgrade.cost.." "..self.currencyName, vec2.new(-1, 0)) and self[currency] >= upgrade.cost then
				self[currency] = self[currency] - upgrade.cost
				upgrade.effect(self)
				table.remove(self[key], index)
			end

			ig.endChild()

			if ig.isItemHovered() then
				ig.beginTooltip()
				ig.pushTextWrapPos(100)
				ig.text(upgrade.description)
				ig.popTextWrapPos()
				ig.endTooltip()
			end

			if index ~= #self[key] then
				ig.sameLine()
			end
		end
	end,
	jobDisplay = function(self, job)
		local nextLevel = math.floor(10 ^ job.level)

		if job.active then
			job.progress = job.progress + time.getDeltaTime() * self.tickSpeed * self.ancestralInfluence * math.sqrt(self.jobMultipliers[job.name] or 1)

			local numCompletions = math.floor(job.progress / job.duration)
			job.exp = job.exp + numCompletions
			job.progress = job.progress - job.duration * numCompletions

			while numCompletions > 0 do
				local completionsToProcess = math.min(numCompletions, nextLevel - job.exp)
				numCompletions = numCompletions - completionsToProcess
				if completionsToProcess == nextLevel - job.exp then
					job.level = job.level + 1
					job.exp = job.exp - nextLevel
					nextLevel = math.floor((5 * job.level) ^ 1.8)
					job.duration = job.duration / 1.5
					if job.onUpgrade ~= nil then
						job:onUpgrade(self)
					end
				end
				job:effect(self, completionsToProcess)
			end

			ig.pushStyleVar(styleVars.FramePadding, 0, 0)
			ig.pushStyleVar(styleVars.ChildBorderSize, 4)
			ig.pushStyleColor(styleColors.Border, vec4.new(1, 1, 1, 1))
		end

		ig.beginChild("job"..job.name, vec2.new(300,75), true, {})

		if job.active then
			ig.popStyleColor(1)
			ig.popStyleVar(2)
		end

		-- Draw icon on left side of window
		local img = self.upgradesMap[job.icon]
		local aspectRatio = img.width / img.height
		local width,height
		local cursorPos = ig.getCursorPos()
		-- padding on each side of the panel is 8
		local desiredSize = 75 - 16

		if aspectRatio > 1 then
			width = desiredSize
			height = desiredSize / aspectRatio
			ig.setCursorPos(cursorPos.x, (75 - height) / 2)
		else
			width = desiredSize * aspectRatio
			height = desiredSize
			ig.setCursorPos(cursorPos.x + (desiredSize - width) / 2, cursorPos.y)
		end

		ig.image(self.upgradesTex, vec2.new(width, height), vec4.new(img.p, img.q, img.s, img.t))

		-- Set cursor to draw the rest next to the icon
		-- TODO is there a way to not have to set this each time?
		ig.setCursorPos(75, 8)
		ig.text(job.name.." (lv."..tostring(job.level)..")")
		ig.setCursorPos(75, ig.getCursorPos().y)
		ig.text("Next level in "..tostring(nextLevel - job.exp))
		ig.setCursorPos(75, ig.getCursorPos().y)
		ig.progressBar(job.progress / job.duration)

		ig.endChild()

		if ig.isItemHovered() then
			if ig.isMouseClicked() then
				if job.active then
					job.active = false
					self.activeJobs = self.activeJobs - 1
				elseif self.timeSlots > self.activeJobs then
					job.active = true
					self.activeJobs = self.activeJobs + 1
				end
			end

			ig.beginTooltip()
			ig.pushTextWrapPos(100)
			ig.text(job.description)
			ig.popTextWrapPos()
			ig.endTooltip()
		end

		if self.rpAllTime > 0 then
			ig.sameLine()

			local multiplierCost = 2 ^ ((self.jobMultipliers[job.name] or 1) - 1)

			ig.beginChild("jobMultipliers"..job.name, vec2.new(250, 75), true, {})
			ig.pushFont(self.largeFont)
			ig.text("Speed multiplier: x"..tostring(self.jobMultipliers[job.name] or 1))
			ig.popFont()
			if ig.button("Increase for "..tostring(multiplierCost).." research points", vec2.new(-1, 0)) and self.rp >= multiplierCost then
				self.rp = self.rp - multiplierCost
				self.jobMultipliers[job.name] = (self.jobMultipliers[job.name] or 1) + 1
			end
			ig.endChild()
		end

		ig.spacing()
	end,
	setName = function(self, args)
		if #args == 1 then
			self.currencyName = args[1]
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: currency {name}\n\tSets the name of the currency you're producing")
		end
	end,
	addCurrency = function(self, args)
		if #args == 1 then
			self.currency = self.currency + tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: addCurrency {amount}\n\tAdds {amount} currency")
		end
	end,
	addProducers = function(self, args)
		if #args == 1 then
			self.producers = self.producers + tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: addProducers {amount}\n\tAdds {amount} producers")
		end
	end,
	addTimeSlots = function(self, args)
		if #args == 1 then
			self.timeSlots = self.timeSlots + tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: addTimeSlots {amount}\n\tAdds {amount} time slots")
		end
	end,
	addRP = function(self, args)
		if #args == 1 then
			self.rp = self.rp + tonumber(args[1])
			self.rpAllTime = self.rpAllTime + tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: addRP {amount}\n\tAdds {amount} research points")
		end
	end
}
