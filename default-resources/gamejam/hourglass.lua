return {
	forwardDependencies = {
		sprite = "renderer"
	},
	preInit = function(self)
		local vertexBuffer = buffer.new(bufferUsage.VertexBuffer, 16 * sizes.Float)
		
		local indexBuffer = buffer.new(bufferUsage.IndexBuffer, 6 * sizes.Float)
		indexBuffer:setDataInts({ 0, 1, 2, 2, 1, 3 })

		createEntity({
			Sprite = {
				location = "resources/gamejam/hourglass.png",
				filename = "hourglass.png", -- TODO make this get set in sprite renderer automatically, or make it use this value when creating the texturesMap
				indexBuffer = indexBuffer,
				vertexBuffer = vertexBuffer,
				indexCount = 6
			},
			Hourglass = {
				flipping = false,
				rotation = 0,
				grains = 0,
				remainingGrains = 0
			}
		})

		self.hourglass = archetype.new({ "Sprite", "Hourglass" })
	end,
	postInit = function(self)
		local sprites = self.hourglass:getComponents("Sprite")
		for id,hourglass in self.hourglass:getComponents("Hourglass"):iterate() do
			local sprite = sprites[id]
			-- set sprite position
			local width, height = glfw.windowSize()
			local spriteHeight = height * 0.4
			local widthFromEdge = (spriteHeight / 2) / width
			sprite.vertexBuffer:setDataFloats({
				1 - 4 * widthFromEdge, -0.4, sprite.UVs.p, sprite.UVs.q,
				1 - 4 * widthFromEdge, 0.4, sprite.UVs.p, sprite.UVs.t,
				1 - widthFromEdge, -0.4, sprite.UVs.s, sprite.UVs.q,
				1 - widthFromEdge, 0.4, sprite.UVs.s, sprite.UVs.t
			})
		end
	end
}
