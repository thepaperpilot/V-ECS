local biomes = {}
for key,filename in pairs(getResources("terrain/biomes", ".lua")) do
	-- Note we strip the ".lua" from the filename
	local tempValue = require(filename:sub(1,filename:len()-4))
	biomes[tempValue.name] = tempValue
end

return {
	priority = 1,
	init = function(self, seed)
		self.terrainNoise = noise.new(noiseType.Simplex, seed, .01)
	end,
	generateChunk = function(self, world, chunk, x, y, z)
		local noiseSet = self.terrainNoise:getNoiseSet(x, y, z, world.chunkSize)
		for point,density in pairs(noiseSet) do
			local internalY = (point - 1) % (world.chunkSize * world.chunkSize) / world.chunkSize
			local archetype = biomes["Plain"].getArchetype(density, y * world.chunkSize + internalY + z)
			
			if archetype ~= false then
				chunk.blocks[point - 1] = world.renderers.voxel.blockArchetypes[archetype]
			end
		end
	end
}
