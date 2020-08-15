return {
	id = "vecs:grass",
	getArchetype = function(textureMap)
		local top = textureMap["grass_top.png"]
		
		local bottom = textureMap["dirt.png"]
		local temp = bottom.q
		bottom.q = bottom.t
		bottom.t = temp
		
		local sides = textureMap["dirt_grass.png"]
		
		local back = {}
		for k,v in pairs(sides) do
			back[k] = v
		end
		back.q = sides.t
		back.t = sides.q

		return archetype.new({}, {
			Block = {
				top = top,
				bottom = bottom,
				front = sides,
				back = back,
				left = sides,
				right = sides
			}
		})
	end
}
