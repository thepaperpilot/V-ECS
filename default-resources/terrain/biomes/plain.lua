return {
	name = "Plain",
	canFillChunk = function(x, y, z)
		if y < -1 then
			return "vecs:stone"
		elseif y > 1 then
			return false
		else
			return nil
		end
	end,
	getArchetype = function(density, y)
		local adjustedDensity = density * 2 + y

		if adjustedDensity > 1 then return false end
		if adjustedDensity < -1 then return "vecs:stone" end
		return "vecs:grass"
	end
}
