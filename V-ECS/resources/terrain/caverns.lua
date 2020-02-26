-- Based off the cave demo settings, as found in a screen shot here: https://github.com/Auburns/FastNoiseSIMD/issues/37
local caveNoise = noise.createCellular(noise.seed, .003)
caveNoise:setCellularReturnType(noise.cellularReturnType.Distance2Noise)
caveNoise:setCellularJitter(.3)
caveNoise:setPerturbType(noise.perturbType.GradientFractal)
caveNoise:setPerturbAmp(0.3)
caveNoise:setPerturbFrequency(3)
caveNoise:setPerturbFractalOctaves(2)
caveNoise:setPerturbFractalLacunarity(12)
caveNoise:setPerturbFractalGain(.08)

terrain = {
	priority = 100,
	processChunk = function(chunk, x, y, z)
		if y > -5 then return end
		local noiseSet = caveNoise:getNoiseSet(x, y, z, blocks.chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -6 then
				if density > 0.88 then
					chunk:clearBlock(point)
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (blocks.chunkSize * blocks.chunkSize) / blocks.chunkSize
				if density - internalY * 0.88/ blocks.chunkSize > 0.88 then
					chunk:clearBlock(point)
				end
			end
		end
	end
}
