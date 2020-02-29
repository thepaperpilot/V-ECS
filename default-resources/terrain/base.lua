local biomes = {}
for key,filename in pairs(getFiles("terrain/biomes", ".lua")) do
	-- Note we strip the ".lua" from the filename
	local tempValue = require(filename:sub(1,filename:len()-4))
	biomes[tempValue.name] = tempValue
end

local seed = noise.seed
--local biomeNoise = noise.createCellular(seed)
local terrainNoise = noise.createSimplex(seed, .01)

terrain = {
	priority = 1,
	processChunk = function(chunk, x, y, z)
		local noiseSet = terrainNoise:getNoiseSet(x, y, z, blocks.chunkSize)
		for point,density in pairs(noiseSet) do
			local internalY = (point - 1) % (blocks.chunkSize * blocks.chunkSize) / blocks.chunkSize
			local archetype = biomes["Plain"].getArchetype(density, y * blocks.chunkSize + internalY + z)
			
			if archetype ~= false then
				chunk:setBlock(point, archetype)
			end
		end
	end
}
