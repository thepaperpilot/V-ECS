return {
	name = "Plain",
	getArchetype = function(density, y)
		local adjustedDensity = density * 2 + y

		if adjustedDensity > 1 then return false end
		if adjustedDensity < -1 then return "vecs:stone" end
		return "vecs:grass"
	end
}
