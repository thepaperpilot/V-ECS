-- Based off the simplex caves described here: https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2
local caveNoise = noise.createSimplex(noise.seed, 0.1)

terrain = {
	priority = 100,
	processChunk = function(chunk, x, y, z)
		if y > -1 then return end
		local noiseSet = caveNoise:getNoiseSet(x, y, z, blocks.chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -2 then
				if density > 0.4 then
					chunk:clearBlock(point)
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (blocks.chunkSize * blocks.chunkSize) / blocks.chunkSize
				if density - internalY * 0.4 / blocks.chunkSize > 0.4 then
					chunk:clearBlock(point)
				end
			end
		end
	end
}
