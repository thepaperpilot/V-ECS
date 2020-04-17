return {
	currencyName = "currency",
	currency = 0,
	producers = 0,
	producerMulti = 1,
	producersProducers = 0,
	tickSpeed = 1,
	manualMulti = 1,
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
			cost = 1000,
			effect = function(self) self.producersProducers = 1 end,
			threshold = 500,
			icon = "drugs.png"
		}
	},
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
			world.systems.debug:addCommand("addProducersProducers", self, self.addProducersProducers, "Usage: addProducersProducers {amount}\n\tAdds {amount} producer producers")
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

		ig.text("You currently have "..tostring(self.currency).." "..self.currencyName)

		if self.producers > 0 then
			ig.text("You currently have "..tostring(self.producers).." "..self.currencyName.." producers producing "..tostring(math.ceil(self.producers * self.producerMulti)).." "..self.currencyName.." per second")
		end

		if self.producersProducers > 0 then
			ig.text("You currently have "..tostring(self.producersProducers).." "..self.currencyName.." producer producers producing "..tostring(self.producersProducers).." "..self.currencyName.." producers per second")
		end

		ig.separator()

		if ig.button("Produce "..self.manualMulti.." "..self.currencyName) then
			self.currency = self.currency + self.manualMulti
			currencyIncreased = true

			if self.hitTen == false and self.currency >= 10 then
				self.hitTen = true
			end
		end

		if self.hitTen then
			local producerCost = math.floor((self.producers + 7) ^ 1.2)
			if ig.button("Purchase 1 "..self.currencyName.." producer for "..tostring(producerCost).." "..self.currencyName) then
				if self.currency >= producerCost then
					self.currency = self.currency - producerCost
					self.producers = self.producers + 1
					-- technically it didn't increase, but it may have unlocked the upgrades panel
					currencyIncreased = true
				end
			end
		end

		self.deltaTime = self.deltaTime + time.getDeltaTime()
		while self.deltaTime >= self.tickSpeed do
			self.deltaTime = self.deltaTime - self.tickSpeed
			self.currency = self.currency + math.ceil(self.producers * self.producerMulti)
			self.producers = self.producers + self.producersProducers
			currencyIncreased = true
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

				local UVs = self.upgradesMap[upgrade.icon]
				ig.image(self.upgradesTex, vec2.new(100, 100), vec4.new(UVs.p, UVs.q, UVs.s, UVs.t))

				if ig.button(upgrade.cost.." "..self.currencyName) and self.currency >= upgrade.cost then
					self.currency = self.currency - upgrade.cost
					upgrade.effect(self)
					table.remove(self.availableUpgrades, index)
				end

				ig.endChild()

				-- TODO auto-wrap to next line?
				ig.sameLine()
			end
		end

		ig.endWindow()
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
	addProducersProducers = function(self, args)
		if #args == 1 then
			self.producersProducers = self.producersProducers + tonumber(args[1])
		else
			debugger.addLog(debugLevels.Warn, "Wrong number of parameters.\nUsage: addProducersProducers {amount}\n\tAdds {amount} producer producers")
		end
	end
}
