local createJob = function(name, baseDuration, icon, effect)
	return {
		name = name,
		duration = baseDuration,
		icon = icon,
		level = 1,
		exp = 0,
		progress = 0,
		active = false,
		effect = effect
	}
end

return {
	currencyName = "currency",
	currency = 0,
	producers = 0,
	producerMulti = 1,
	tickSpeed = 1,
	manualMulti = 1,
	timeSlots = 0,
	hitTen = false,
	deltaTime = 0,
	availableUpgrades = {},
	upgrades = {
		{
			name = "Better producers",
			cost = 10,
			effect = function(self) self.producerMulti = self.producerMulti + .5 end,
			threshold = 0,
			icon = "hammer.png"
		},
		{
			name = "How to produce: A guide for dummies",
			cost = 10,
			effect = function(self) self.manualMulti = self.manualMulti + .5 end,
			threshold = 0,
			icon = "book.png"
		},
		{
			name = "Ascension",
			cost = 200,
			effect = function(self)
				self.timeSlots = 1
				self.producerMulti = 2
				self.jobs[1].active = true
				self.activeJobs = 1
			end,
			threshold = 100,
			icon = "drugs.png"
		}
	},
	jobs = {
		createJob("Make Producers", 1, "pick_silver.png", function(self) self.producers = self.producers + 1 end)
	},
	activeJobs = 0,
	forwardDependencies = {
		imgui = "renderer"
	},
	init = function(self, world)
		local upgradesTex, upgradesMap = world.renderers.imgui:addStitchedTexture(getResources("textures/incremental", ".png"))
		self.upgradesTex = upgradesTex
		self.upgradesMap = upgradesMap

		if world.systems.debug ~= nil then
			world.systems.debug:addCommand("currency", self, self.setName, "Usage: currency {name}\n\tSets the name of the currency you're producing")
			world.systems.debug:addCommand("addCurrency", self, self.addCurrency, "Usage: addCurrency {amount}\n\tAdds {amount} currency")
			world.systems.debug:addCommand("addProducers", self, self.addProducers, "Usage: addProducers {amount}\n\tAdds {amount} producers")
			world.systems.debug:addCommand("addTimeSlots", self, self.addTimeSlots, "Usage: addTimeSlots {amount}\n\tAdds {amount} time slots")
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

		local message ="You currently have "..tostring(self.currency).." "..self.currencyName
		local nextTimeSlot = 10 ^ (self.timeSlots + 3)

		if self.timeSlots > 0 then
			message = message.." (Next time slot at "..tostring(nextTimeSlot).." "..self.currencyName..")"
		end

		ig.text(message)

		if self.producers > 0 then
			ig.text("You currently have "..tostring(self.producers).." "..self.currencyName.." producers producing "..tostring(math.ceil(self.producers * self.producerMulti)).." "..self.currencyName.." per second")
		end

		ig.separator()

		self.deltaTime = self.deltaTime + time.getDeltaTime()
		while self.deltaTime >= self.tickSpeed do
			self.deltaTime = self.deltaTime - self.tickSpeed
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

				ig.text("Upgrades")

				if currencyIncreased then
					for index, upgrade in ipairs(self.upgrades) do
						if upgrade.threshold <= self.currency then
							table.insert(self.availableUpgrades, upgrade)
							table.remove(self.upgrades, index)
						end
					end
				end

				for index,upgrade in ipairs(self.availableUpgrades) do
					ig.beginChild("upgrade"..tostring(index), vec2.new(125, 200), true, {})

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

					if ig.button(upgrade.cost.." "..self.currencyName, vec2.new(-1, 0)) and self.currency >= upgrade.cost then
						self.currency = self.currency - upgrade.cost
						upgrade.effect(self)
						table.remove(self.availableUpgrades, index)
					end

					ig.endChild()

					-- TODO auto-wrap to next line?
					ig.sameLine()
				end
			end
		else
			-- Post-ascension
			ig.text("Jobs")

			if currencyIncreased then
				if self.currency >= nextTimeSlot then
					self.timeSlots = self.timeSlots + 1
				end
			end

			ig.text("You currently have "..tostring(self.timeSlots - self.activeJobs).." time slots of "..tostring(self.timeSlots))

			for _,job in ipairs(self.jobs) do
				self:jobDisplay(job)
			end
		end

		ig.endWindow()
	end,
	jobDisplay = function(self, job)
		local nextLevel = math.floor((10 * job.level) ^ 1.8)
		
		if job.active then
			job.progress = job.progress + time.getDeltaTime()

			if job.progress > job.duration then
				job.progress = job.progress - job.duration
				job.exp = job.exp + 1
				if job.exp >= nextLevel then
					job.level = job.level + 1
					job.duration = job.duration / 1.5
					job.exp = 0
				end
				
				job.effect(self)
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

		if ig.isItemHovered() and ig.isMouseClicked() then
			if job.active then
				job.active = false
				self.activeJobs = self.activeJobs - 1
			elseif self.timeSlots > self.activeJobs then
				job.active = true
				self.activeJobs = self.activeJobs + 1
			end
		end
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
	end
}
