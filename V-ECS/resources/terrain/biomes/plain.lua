local stone = blocks.getArchetype("vecs:stone")
local grass = blocks.getArchetype("vecs:grass")

return {
	name = "Plain",
	getArchetype = function(density, y)
		local adjustedDensity = density * 2 + y

		if adjustedDensity > 1 then return false end
		if adjustedDensity < -1 then return stone end
		return grass
	end
}
