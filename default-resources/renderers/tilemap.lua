return {
	shaders = {
		["resources/shaders/spriteatlas.vert"] = shaderStages.Vertex,
		["resources/shaders/spriteatlas.frag"] = shaderStages.Fragment
	},
	pushConstantsSize = sizes.Mat4 + sizes.Vec4,
	vertexLayout = {
		[0] = vertexComponents.R32G32B32,
		[1] = vertexComponents.R32G32,
		[2] = vertexComponents.R32G32B32A32
	},
	forwardDependencies = {
		imgui = "renderer"
	},
	init = function(self, renderer)
		self.tiles = query.new({ "TileMap" })

		local textures = {} -- list, for creating stitched texture
		local foundTextures = {} -- set, for finding duplicates
   		for key,archetype in pairs(self.tiles:getArchetypes()) do
   			for _,map in archetype:getComponents("TileMap"):iterate() do
   				if map.folder == nil then map.folder = "" end
   				for _,tileset in map.tilesets:iterate() do
	   				if foundTextures[tileset.image] == nil then
						foundTextures[tileset.image] = true
						textures[#textures + 1] = map.folder..tileset.image
					end
				end
				self:traverseLayersForTextures(map.folder, map.layers, textures, foundTextures)
			end
   		end

   		-- TODO use transparentColor to set alpha channel on textures

		local pixels, width, height, texturesMap = texture.createStitched(textures)
		texture.new(renderer, pixels, width, height)

   		for key,archetype in pairs(self.tiles:getArchetypes()) do
   			for _,map in archetype:getComponents("TileMap"):iterate() do
   				map.gids = {}
   				for _,tileset in map.tilesets:iterate() do
   					tileset.UVs = texturesMap[tileset.image]
   				end
				self:setupLayers(map, map.layers, texturesMap, 0, 0, vec4.new(1, 1, 1, 1), map.height * map.tileheight / 2 + 512)
			end
   		end
	end,
	render = function(self, renderer)
		for key,archetype in pairs(self.tiles:getArchetypes()) do
			if not archetype:isEmpty() then
				for index,map in archetype:getComponents("TileMap"):iterate() do
					local scale = map.scale == nil and vec3.new(1, 1, 1) or map.scale
					local width, height = glfw.windowSize()
					scale.x = scale.x * height / width
					scale.z = 1 / (map.height * map.tileheight + 1024)
					local M = mat4.scale(scale) * mat4.translate(map.position == nil and vec3.new(0, 0, 0) or map.position) * mat4.rotate(map.rotation ~= nil and map.rotation or 0, vec3.new(0, 0, 1))
					self:renderLayers(renderer, map, M, map.layers, 0, 0, vec4.new(1, 1, 1, 1), map.height * map.tileheight / 2 + 512)
				end
			end
		end
	end,
	traverseLayersForTextures = function(self, folder, layers, textures, foundTextures)
		for _,layer in layers:iterate() do
			if layer.type == "group" then
				self:traverseLayersForTextures(folder, layer.layers, textures, foundTextures)
			elseif layer.type == "imagelayer" then
   				if foundTextures[layer.image] == nil then
					foundTextures[layer.image] = true
					textures[#textures + 1] = folder..layer.image
				end
			end
		end
	end,
	setupLayers = function(self, map, layers, texturesMap, offsetx, offsety, tint, layerZ)
		for _,layer in layers:iterate() do
			local layertint = tint * (layer.tintcolor ~= nil and vec4.new(layer.tintcolor[1] / 255, layer.tintcolor[2] / 255, layer.tintcolor[3] / 255, layer.opacity) or vec4.new(1, 1, 1, layer.opacity))
			local zOffset = layerZ
			if layer.properties.zOffset ~= nil then zOffset = zOffset + layer.properties.zOffset end
			if layer.type == "group" then
				self:setupLayers(map, layer.layers, texturesMap, offsetx + layer.offsetx, offsety + layer.offsety, layertint, zOffset)
			elseif layer.type == "imagelayer" then
				layer.UVs = texturesMap[layer.image]
			end
			self:updateBuffersForLayer(map, layer, offsetx + layer.offsetx, offsety + layer.offsety, layertint, zOffset)
		end
	end,
	updateBuffersForLayer = function(self, map, layer, offsetx, offsety, tint, layerZ)
		-- TODO draworder properties?
		layer.tint = tint
		local zOffset = layerZ
		if layer.properties.zOffset ~= nil then zOffset = zOffset + layer.properties.zOffset end
		local startX = -map.width * map.tilewidth / 2 + offsetx
		local startY = -map.height * map.tileheight / 2 + offsety
		if layer.type == "imagelayer" then
			local imageZOffset = zOffset --- startY
			if layer.vertexBuffer == nil then layer.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, 36 * sizes.Float) end
			local uvWidth = layer.UVs.s - layer.UVs.p
			local uvHeight = layer.UVs.t - layer.UVs.q
			layer.vertexBuffer:setDataFloats({
				startX, startY, imageZOffset + startY, 0, 0, layer.UVs.p, layer.UVs.q, uvWidth, uvHeight,
				startX, startY + layer.UVs.height, imageZOffset + startY + layer.UVs.height, 0, 1, layer.UVs.p, layer.UVs.q, uvWidth, uvHeight,
				startX + layer.UVs.width, startY, imageZOffset + startY, 1, 0, layer.UVs.p, layer.UVs.q, uvWidth, uvHeight,
				startX + layer.UVs.width, startY + layer.UVs.height, imageZOffset + startY + layer.UVs.height, 1, 1, layer.UVs.p, layer.UVs.q, uvWidth, uvHeight
			})
			if layer.indexBuffer == nil then
				layer.indexBuffer = buffer.new(bufferUsage.IndexBuffer, 6 * sizes.Float)
				layer.indexBuffer:setDataInts({ 0, 1, 2, 2, 1, 3 })
			end
			layer.indexCount = 6
			layer.dirty = false
		elseif layer.type == "tilelayer" then
			-- generate mesh greedily
			local indices = {}
			local vertices = {}
			local i = 1
			local r = 0
			local mask = {}
			-- copy layer data to mask
			for x = 0, layer.width do
				for y = 0, layer.height do
					mask[i] = layer.data[i]
					i = i + 1
				end
			end
			i = 1
			while r < layer.height do
				local c = 0
				while c < layer.width do
					if mask[i] > 0 then
						-- compute width
						local w = 1
						while mask[i + w] == mask[i] and c + w < layer.width do w = w + 1 end
						-- compute height
						local h = 1
						while r + h < layer.height do
							local k = 0
							while k < w and mask[i + k + h * layer.width] == mask[i] do k = k + 1 end
							if k < w then break end
							h = h + 1
						end

						local UVs, flipped = self:getUVsFromGID(map, mask[i])

						local tileset = self.getTilesetFromGID(map, mask[i])
						local tileZOffset = zOffset-- - startY - r * map.tileheight
						if tileset ~= nil then
							for _,tile in tileset.tiles:iterate() do
								if tile.id + tileset.firstgid == mask[i] then
									if tile.properties.zOffset ~= nil then
										tileZOffset = tileZOffset + tile.properties.zOffset
									end
									break
								end
							end
						end

						local p1
						local p2
						if flipped then
							p1 = 1
							p2 = 0
						else
							p1 = 0
							p2 = 1
						end

						local n = #vertices / 9
						self.concatTable(vertices, { startX + c * map.tilewidth, startY + r * map.tileheight, tileZOffset + startY + r * map.tileheight, p1 * w, p1 * h })
						self.concatTable(vertices, UVs)
						self.concatTable(vertices, { startX + c * map.tilewidth, startY + (r + h) * map.tileheight, tileZOffset + startY + (r + h) * map.tileheight, 0, h })
						self.concatTable(vertices, UVs)
						self.concatTable(vertices, { startX + (c + w) * map.tilewidth, startY + r * map.tileheight, tileZOffset + startY + r * map.tileheight, w, 0 })
						self.concatTable(vertices, UVs)
						self.concatTable(vertices, { startX + (c + w) * map.tilewidth, startY + (r + h) * map.tileheight, tileZOffset + startY + (r + h) * map.tileheight, p2 * w, p2 * h })
						self.concatTable(vertices, UVs)
						self.concatTable(indices, { n + 0, n + 1, n + 2, n + 2, n + 1, n + 3 })

						-- zero out the mask
						for l = 0, h - 1 do
							for k = 0, w - 1 do
								mask[i + k + l * layer.width] = 0
							end
						end

						i = i + w
						c = c + w
					else
						i = i + 1
						c = c + 1
					end
				end
				r = r + 1
			end
			-- TODO destroying old buffers?
			layer.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, #vertices * sizes.Float)
			layer.vertexBuffer:setDataFloats(vertices)
			layer.indexCount = #indices
			layer.indexBuffer = buffer.new(bufferUsage.IndexBuffer, layer.indexCount * sizes.Float)
			layer.indexBuffer:setDataInts(indices)
			layer.dirty = false
		elseif layer.type == "objectgroup" then
			local indices = {}
			local vertices = {}
			for _,object in layer.objects:iterate() do
				local objectZOffset = zOffset-- - startY - object.y
				if object.properties.zOffset ~= nil then objectZOffset = objectZOffset + object.properties.zOffset end
				-- TODO objects w/o GIDs?
				-- TODO text
				if object.gid ~= nil and object.visible then
					local UVs, flipped = self:getUVsFromGID(map, object.gid)
					local p1
					local p2
					if flipped then
						p1 = 1
						p2 = 0
					else
						p1 = 0
						p2 = 1
					end

					-- complicated math to make the object pass through the z-order plane at the next whole number y coordinate,
					-- while still rendering above/below other objects as appropriate
					-- thought hard about the solution, it didn't work, and fiddled with it in desmos until it did
					-- https://www.desmos.com/calculator/qykbhuspdx
					-- Note: this still isn't perfect. I couldn't come up with a way to make z-order work *with greedy meshing tile layers*
					--  and also *other objects*. (but I thought I did, haha). This will clip with other objects that aren't grid-aligned,
					--  so should only be used when any moving object will have a collision box of some sort
					-- TODO make a setting to render things such that objects don't clip, but you won't appear above/below greedy mesh tilelayers correctly
					local lowerY = startY + object.y - map.tileheight
					local floorY = math.floor(lowerY / map.tileheight) * map.tileheight
					-- if this ever needs fixing due to z-fighting or wrong z-order, it's probably in this lowerZ calculation:
					local lowerZ = floorY + (lowerY - floorY) / (1 + (floorY + map.tileheight - lowerY) / map.tileheight)
					local s = vec2.new(floorY + map.tileheight, floorY + map.tileheight)
					local direction = normalize(s - vec2.new(lowerY, lowerZ))
					local upperZ = lowerZ + direction.y * object.height / direction.x

					local i = #vertices / 9
					self.concatTable(vertices, { startX + object.x, lowerY, lowerZ + objectZOffset, p1, p1 })
					self.concatTable(vertices, UVs)
					self.concatTable(vertices, { startX + object.x, lowerY + object.height, upperZ + objectZOffset, 0, 1 })
					self.concatTable(vertices, UVs)
					self.concatTable(vertices, { startX + object.x + object.width, lowerY, lowerZ + objectZOffset, 1, 0 })
					self.concatTable(vertices, UVs)
					self.concatTable(vertices, { startX + object.x + object.width, lowerY + object.height, upperZ + objectZOffset, p2, p2 })
					self.concatTable(vertices, UVs)
					self.concatTable(indices, { i + 0, i + 1, i + 2, i + 2, i + 1, i + 3 })
				end
			end
			-- TODO destroying old buffers?
			layer.vertexBuffer = buffer.new(bufferUsage.VertexBuffer, #vertices * sizes.Float)
			layer.vertexBuffer:setDataFloats(vertices)
			layer.indexCount = #indices
			layer.indexBuffer = buffer.new(bufferUsage.IndexBuffer, layer.indexCount * sizes.Float)
			layer.indexBuffer:setDataInts(indices)
			layer.dirty = false
		end

		-- create entities
		if layer.properties ~= {} and layer.entity == nil then
			local id,idx = createEntity({}, {
				TileLayerProperties = layer.properties
			})
			layer.entity = id
		end
		if layer.type == "objectgroup" then
			for _,object in layer.objects:iterate() do
				if object.entity == nil then
					local id,idx = createEntity({
						TileObject = object
					}, {
						TileObjectProperties = object.properties
					})
					object.entity = id
				end
			end
		elseif layer.type == "tilelayer" then
			if layer.entities == nil then layer.entities = {} end
			for idx,gid in layer.data:iterate() do
				local tileset = self.getTilesetFromGID(map, gid)
				if tileset ~= nil then
					for _,tile in tileset.tiles:iterate() do
						if tile.id + tileset.firstgid == gid then
							if layer.entities[idx] == nil then
								local id,_ = createEntity({
									Tile = {
										x = idx % layer.width,
										y = math.floor(idx / layer.width)
									}
								}, {
									TileProperties = tile.properties
								})
								layer.entities[idx] = id
							end
							break
						end
					end
				end
			end
		end
	end,
	renderLayers = function(self, renderer, map, M, layers, offsetx, offsety, tint, layerZ)
		for _,layer in layers:iterate() do
			local zOffset = layerZ
			if layer.properties.zOffset ~= nil then zOffset = zOffset + layer.properties.zOffset end
			local layertint = tint * (layer.tintcolor ~= nil and vec4.new(layer.tintcolor[1] / 255, layer.tintcolor[2] / 255, layer.tintcolor[3] / 255, layer.opacity) or vec4.new(1, 1, 1, layer.opacity))
			if layer.type == "objectgroup" then
				for _,object in layer.objects:iterate() do
					if object.dirty then
						self:updateBuffersForLayer(map, layer, offsetx + layer.offsetx, offsety + layer.offsety, layertint, zOffset)
						break
					end
				end
			end
			if layer.dirty then
				self:updateBuffersForLayer(map, layer, offsetx + layer.offsetx, offsety + layer.offsety, layertint, zOffset)
			end
			if layer.type == "group" then
				self:renderLayers(renderer, map, M, layer.layers, offsetx + layer.offsetx, offsety + layer.offsety, layertint, zOffset)
			elseif layer.indexCount ~= nil and layer.indexCount > 0  then
				local commandBuffer = renderer:startRendering()
				renderer:pushConstantMat4(commandBuffer, shaderStages.Vertex, 0, M)
				renderer:pushConstantVec4(commandBuffer, shaderStages.Vertex, sizes.Mat4, layertint)
				renderer:drawVertices(commandBuffer, layer.vertexBuffer, layer.indexBuffer, layer.indexCount)
				renderer:finishRendering(commandBuffer)
			end
		end
	end,
	concatTable = function(t1, t2)
		for i=1, #t2 do
			t1[#t1 + 1] = t2[i]
		end
	end,
	getTilesetFromGID = function(map, gid)
		-- find tileset gid belongs to
		local tileset = nil
		for _,t in map.tilesets:iterate() do
			if t.firstgid <= gid and t.firstgid + t.tilecount > gid then
				tileset = t
			end
		end
		return tileset
	end,
	getUVsFromGID = function(self, map, gid)
		if map.gids[gid] ~= nil then
			local g = gid
			if g > 2 ^ 32 then g = g - 2 ^ 31 end
			if g > 2 ^ 31 then g = g - 2 ^ 30 end
			return map.gids[gid], g > 2 ^ 29
		end

		-- find actual gid w/o flips
		local originalGid = gid
		local flippedHoriz = gid > 2 ^ 31
		if flippedHoriz then gid = gid - 2 ^ 31 end
		local flippedVert = gid > 2 ^ 30
		if flippedVert then gid = gid - 2 ^ 30 end
		local flippedDiag = gid > 2 ^ 29
		if flippedDiag then gid = gid - 2 ^ 29 end

		local tileset = self.getTilesetFromGID(map, gid)
		if tileset == nil then
			debugger.addLog(debugLevels.Error, "gid "..tostring(gid).." not found in any tilemap tileset!")
			return nil
		end

		-- calculate UVs for un-flipped tile
		local row = math.floor((gid - tileset.firstgid) / tileset.columns)
		local column = (gid - tileset.firstgid) % tileset.columns
		local pixelUVWidth = (tileset.UVs.s - tileset.UVs.p) / tileset.imagewidth
		local pixelUVHeight = (tileset.UVs.t - tileset.UVs.q) / tileset.imageheight
		local p = tileset.UVs.p + tileset.margin * pixelUVWidth + column * (tileset.tilewidth + tileset.spacing) * pixelUVWidth + pixelUVWidth * 0.5
		local q = tileset.UVs.q + tileset.margin * pixelUVHeight + row * (tileset.tileheight + tileset.spacing) * pixelUVHeight + pixelUVHeight * 0.5
		local width = pixelUVWidth * (tileset.tilewidth - 1)
		local height = pixelUVHeight * (tileset.tileheight - 1)
		local s = p + width
		local t = q + height

		-- handle flips
		if flippedVert then
			local temp = q
			q = t
			t = temp
			height = -height
		end
		if flippedHoriz then
			local temp = p
			p = s
			s = temp
			width = -width
		end

		-- cache and return UVs
		local UVs = { p, q, width, height }
		map.gids[originalGid] = UVs
		return UVs, flippedDiag
	end
}
