return {
	speed = 2,
	preInit = function(self)
		local map = require("resources/maps/test")
		map.scale = vec3.new(1 / 200, 1 / 200, 1 / 200)
		map.folder = "resources/maps/"
		createEntity({
			TileMap = map
		})

		self.maps = archetype.new({ "TileMap" })
		self.players = archetype.new({ "TileObject" }, {
			TileObjectProperties = {
				player = true
			}
		})
		self.colliders = archetype.new({ "TileObject" }, {
			TileObjectProperties = {
				collision = true
			}
		})

		self.keyPressEvent = archetype.new({ "KeyPressEvent" })
		self.keyReleaseEvent = archetype.new({ "KeyReleaseEvent" })
		self.WPressed = false
		self.SPressed = false
		self.APressed = false
		self.DPressed = false
	end,
	postInit = function(self)
		for _,map in self.maps:getComponents("TileMap"):iterate() do
			for _,obj in self.players:getComponents("TileObject"):iterate() do
				map.position = vec3.new(-obj.x + map.width * map.tilewidth / 2, -obj.y + map.height * map.tileheight / 2, 0)
			end
		end

		self.tileColliders = {}
		for _,map in self.maps:getComponents("TileMap"):iterate() do
			for _,tile in archetype.new({ "Tile" }, { TileProperties = { collision = true } }):getComponents("Tile"):iterate() do
				self.tileColliders[tile.x + tile.y * map.width - 1] = true
				-- TODO make these lists of colliders with sub-cell collision boundaries
			end
		end
	end,
	update = function(self)
		if not self.keyPressEvent:isEmpty() then
			for _,event in self.keyPressEvent:getComponents("KeyPressEvent"):iterate() do
				if not event.cancelled then
					if event.key == keys.W then self.WPressed = true end
					if event.key == keys.S then self.SPressed = true end
					if event.key == keys.A then self.APressed = true end
					if event.key == keys.D then self.DPressed = true end
					if event.key == keys.Space then self.SpacePressed = true end
					if event.key == keys.LeftShift then self.LeftShiftPressed = true end
				end
			end
		end
		if not self.keyReleaseEvent:isEmpty() then
			for _,event in self.keyReleaseEvent:getComponents("KeyReleaseEvent"):iterate() do
				if not event.cancelled then
					if event.key == keys.W then self.WPressed = false end
					if event.key == keys.S then self.SPressed = false end
					if event.key == keys.A then self.APressed = false end
					if event.key == keys.D then self.DPressed = false end
					if event.key == keys.Space then self.SpacePressed = false end
					if event.key == keys.LeftShift then self.LeftShiftPressed = false end
				end
			end
		end

		local velocity = vec2.new(0, 0)
		if self.WPressed then velocity.y = velocity.x - 1 end
		if self.SPressed then velocity.y = velocity.x + 1 end
		if self.APressed then velocity.x = velocity.x - 1 end
		if self.DPressed then velocity.x = velocity.x + 1 end

		if velocity:length() > 0 then
			velocity = normalize(velocity) * self.speed

			for _,map in self.maps:getComponents("TileMap"):iterate() do
				for _,obj in self.players:getComponents("TileObject"):iterate() do
					obj.dirty = true

					-- check collisions with objects
					-- TODO doesn't work
					for _,col in self.colliders:getComponents("TileObject"):iterate() do
						if ((obj.y > col.y) ~= (obj.y + obj.height > col.y)) or ((obj.y > col.y + col.height) ~= (obj.y + obj.height > col.y + col.height)) then
							if obj.x + obj.width < col.x and obj.x + obj.width + velocity.x > col.x then
								velocity.x = col.x - obj.width - obj.x
							elseif obj.x + obj.width > col.x + col.width and obj.x + obj.width + velocity.x < col.x + col.width then
								velocity.x = col.x + col.width - obj.width - obj.x
							end
						elseif ((obj.x > col.x) ~= (obj.x + obj.width > col.x)) or ((obj.x > col.x + col.width) ~= (obj.x + obj.width > col.x + col.width)) then
							if obj.y + obj.height < col.y and obj.y + obj.height + velocity.y > col.y then
								velocity.y = col.y - obj.height - obj.y
							elseif obj.y + obj.height > col.y + col.height and obj.y + obj.height + velocity.y < col.y + col.height then
								velocity.y = col.y + col.height - obj.height - obj.y
							end
						end
					end

					-- check collisions with tiles
					if math.floor(obj.x + obj.width) ~= math.floor(obj.x + obj.width + velocity.x) or math.floor(obj.x) ~= math.floor(obj.x + velocity.x) then
						if velocity.x > 0 then
							for y = math.floor((obj.y + 0.01) / map.tileheight) - 1, math.floor((obj.y + obj.height - 0.01) / map.tileheight) - 1 do
								if self.tileColliders[y * map.width + math.floor((obj.x + obj.width + velocity.x) / map.tilewidth)] then
									velocity.x = math.floor((obj.x + obj.width + velocity.x) / map.tilewidth) * map.tilewidth - obj.width - obj.x
									break
								end
							end
						else
							for y = math.floor((obj.y + 0.01) / map.tileheight) - 1, math.floor((obj.y + obj.height - 0.01) / map.tileheight) - 1 do
								if self.tileColliders[y * map.width + math.floor((obj.x + velocity.x) / map.tilewidth)] then
									velocity.x = math.floor((obj.x + velocity.x) / map.tilewidth + 1) * map.tilewidth - obj.x
									break
								end
							end
						end
					end
					if math.floor(obj.y + obj.height) ~= math.floor(obj.y + obj.height + velocity.y) or math.floor(obj.y) ~= math.floor(obj.y + velocity.y) then
						if velocity.y > 0 then
							for x = math.floor((obj.x + 0.01) / map.tilewidth), math.floor((obj.x + obj.width - 0.01) / map.tilewidth) do
								if self.tileColliders[math.floor((obj.y + obj.height + velocity.y) / map.tileheight - 1) * map.width + x] then
									velocity.y = math.floor((obj.y + obj.height + velocity.y) / map.tileheight) * map.tileheight - obj.height - obj.y
									break
								end
							end
						else
							for x = math.floor((obj.x + 0.01) / map.tilewidth), math.floor((obj.x + obj.width - 0.01) / map.tilewidth) do
								if self.tileColliders[math.floor((obj.y + velocity.y) / map.tileheight - 1) * map.width + x] then
									velocity.y = math.floor((obj.y + velocity.y) / map.tileheight + 1) * map.tileheight - obj.y
									break
								end
							end
						end
					end

					obj.x = obj.x + velocity.x
					obj.y = obj.y + velocity.y

					-- clamp obj to edge of map
					obj.x = math.min(math.max(obj.x, 0), map.width * map.tilewidth - obj.width)
					obj.y = math.min(math.max(obj.y, 0), map.height * map.tileheight - obj.height)

					-- calculate map position, clamping to edge of map
					local vConstraint = map.height * map.tileheight / 2 - 1 / map.scale.y
					local mapY = math.min(math.max(map.height * map.tileheight / 2 - obj.y, -vConstraint), vConstraint)
					-- x scale is modified by the aspect ratio to make pixels 1:1 on screen
					local width, height = glfw.windowSize()
					local xScale = map.scale.x * height / width
					local hConstraint = map.width * map.tilewidth / 2 - 1 / xScale
					local mapX = math.min(math.max(map.width * map.tilewidth / 2 - obj.x, -hConstraint), hConstraint)

					map.position = vec3.new(mapX, mapY, 0)
				end
			end
		end
	end
}
