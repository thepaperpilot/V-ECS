return {
	forwardDependencies = {
		example_gundam = "renderer"
	},
	gridSize = 50,
	preInit = function(self)
		self.gundams = archetype.new({
			"Gundam"
		})
		self.gundams:createEntities(self.gridSize * self.gridSize)
		local x = 0
		local z = 0
		for id,gundam in self.gundams:getComponents("Gundam"):iterate() do
			gundam.x = 5 * x
			gundam.y = 0
			gundam.z = 5 * z
			x = x + 1
			if x == self.gridSize - 1 then
				x = 0
				z = z + 1
			end
			gundam.rotation = math.random()
			gundam.rotSpeed = math.random(360)
		end
	end,
	update = function(self)
		jobs.createParallel(self.updateGundams, luaVal.new({ gundams = self.gundams }), self.gundams, 64):submit()
	end,
	updateGundams = function(data, first, last)
		data.gundams:lock_shared()
		for id,gundam in data.gundams:getComponents("Gundam"):iterate_range(luaVal.new(first), luaVal.new(last)) do
			gundam.rotation = gundam.rotation + time.getDeltaTime() * gundam.rotSpeed
		end
		data.gundams:unlock_shared()
	end
}
