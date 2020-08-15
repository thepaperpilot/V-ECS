-- Spherical underground rooms, like the "Worley caves" in https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2

return {
	priority = 100,
	init = function(self, chunkSize, seed)
		self.caveNoise = noise.new(noiseType.Cellular, seed, .01)
	end,
	generateChunk = function(self, archetypes, chunkSize, chunk)
		if chunk.y > -2 then return end
		local caveNoiseBuffer = noiseBuffer.new(chunkSize)
		local noiseSet = self.caveNoise:getNoiseSet(caveNoiseBuffer, chunk.x, chunk.y, chunk.z, chunkSize)
		for point,density in pairs(noiseSet) do
			if chunk.y < -3 then
				if density < 0.01 then
					chunk.blocks[point - 1] = nil
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (chunkSize * chunkSize) / chunkSize
				if density + internalY * 0.01 / chunkSize < 0.01 then
					chunk.blocks[point - 1] = nil
				end
			end
		end
	end
}