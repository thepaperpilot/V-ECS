local biomes = {}
for key,filename in pairs(getResources("terrain/biomes", ".lua")) do
	-- Note we strip the ".lua" from the filename
	local tempValue = require(filename:sub(1,filename:len()-4))
	biomes[tempValue.name] = tempValue
end

return {
	priority = 1,
	init = function(self, world, seed)
		self.terrainNoise = noise.new(noiseType.Simplex, seed, .1)
		self.largeNoise = noise.new(noiseType.Simplex, seed + 1, .001)
		self.noiseBuffer = noiseBuffer.new(world.renderers.voxel.chunkSize)
		self.largeNoiseBuffer = noiseBuffer.new(world.renderers.voxel.chunkSize)
	end,
	generateChunk = function(self, world, chunk, x, y, z)
		local chunkSize = world.renderers.voxel.chunkSize
		local chunkBiome = biomes["Plain"]

		local canFillChunk = chunkBiome.canFillChunk(x, y, z)
		if canFillChunk == false then
			-- chunk is all air
			return
		elseif canFillChunk == nil then
			-- we can't fill the chunk
			-- generate noise and calculate each block
			local noiseSet = self.terrainNoise:getNoiseSet(self.noiseBuffer, x, y, z, chunkSize)
			local largeNoiseSet = self.largeNoise:getNoiseSet(self.largeNoiseBuffer, x, y, z, chunkSize)
			for point, density in pairs(noiseSet) do
				local internalY = math.floor((point - 1) % (chunkSize * chunkSize) / chunkSize)
				local archetype = chunkBiome.getArchetype(density, largeNoiseSet[point], y * chunkSize + internalY)
				
				if archetype ~= false then
					chunk.blocks[point - 1] = world.renderers.voxel.blockArchetypes[archetype]
				end
			end
		else
			-- every block is canFillChunk
			local archetype = world.renderers.voxel.blockArchetypes[canFillChunk]
			for point = 1, chunkSize ^ 3 do
				chunk.blocks[point - 1] = archetype
			end
		end
	end
}
