-- Spherical underground rooms, like the "Worley caves" in https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2
local caveNoise = noise.createCellular(noise.seed, .01)

terrain = {
	priority = 100,
	processChunk = function(chunk, x, y, z)
		if y > -2 then return end
		local noiseSet = caveNoise:getNoiseSet(x, y, z, blocks.chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -3 then
				if density < 0.01 then
					chunk:clearBlock(point)
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (blocks.chunkSize * blocks.chunkSize) / blocks.chunkSize
				if density + internalY * 0.01 / blocks.chunkSize < 0.01 then
					chunk:clearBlock(point)
				end
			end
		end
	end
}
