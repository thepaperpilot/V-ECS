return {
	forwardDependencies = {
		stone = "renderer"
	},
	preInit = function(self)
		createEntity({
			Stone = {
				position = vec3.new(0, 1, 4),
				rotation = 0,
				scale = vec3.new(1, 1, 1)
			}
		})

		self.stone = archetype.new({ "Stone" })
	end,
	update = function(self)
		local delta = time.getDeltaTime()
		if delta < 0.1 then
			for id,stone in self.stone:getComponents("Stone"):iterate() do
				stone.rotation = stone.rotation + delta * 10
			end
		end
	end
}
