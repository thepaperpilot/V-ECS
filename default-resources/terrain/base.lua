return {
	priority = 1,
	init = function(self, chunkSize, seed)
		self.terrainNoise = noise.new(noiseType.Simplex, seed, .1)
		self.largeNoise = noise.new(noiseType.Simplex, seed + 1, .001)

		self.biomes = {}
		for key,filename in pairs(getResources("biomes", ".lua")) do
			-- Note we strip the ".lua" from the filename
			local tempValue = require(filename:sub(1,filename:len()-4))
			self.biomes[tempValue.name] = tempValue
		end
	end,
	generateChunk = function(self, archetypes, chunkSize, chunk)
		local chunkBiome = self.biomes["Plain"]

		local canFillChunk = chunkBiome.canFillChunk(chunk.x, chunk.y, chunk.z)
		if canFillChunk == false then
			-- chunk is all air
			return
		elseif canFillChunk == nil then
			local smallNoiseBuffer = noiseBuffer.new(chunkSize)
			local largeNoiseBuffer = noiseBuffer.new(chunkSize)
			-- we can't fill the chunk
			-- generate noise and calculate each block
			local noiseSet = self.terrainNoise:getNoiseSet(smallNoiseBuffer, chunk.x, chunk.y, chunk.z, chunkSize)
			local largeNoiseSet = self.largeNoise:getNoiseSet(largeNoiseBuffer, chunk.x, chunk.y, chunk.z, chunkSize)
			for point, density in pairs(noiseSet) do
				local internalY = math.floor((point - 1) % (chunkSize * chunkSize) / chunkSize)
				local archetype = chunkBiome.getArchetype(density, largeNoiseSet[point], chunk.y * chunkSize + internalY)
				
				if archetype ~= false then
					chunk.blocks[point - 1] = archetypes[archetype]
				end
			end
		else
			-- every block is canFillChunk
			local archetype = archetypes[canFillChunk]
			for point = 1, chunkSize ^ 3 do
				chunk.blocks[point - 1] = archetype
			end
		end
	end
}
