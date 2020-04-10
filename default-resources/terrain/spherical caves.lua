-- Spherical underground rooms, like the "Worley caves" in https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2

return {
	priority = 100,
	init = function(self, seed)
		self.caveNoise = noise.new(noiseType.Cellular, seed, .01)
	end,
	generateChunk = function(self, world, chunk, x, y, z)
		if y > -2 then return end
		local noiseSet = self.caveNoise:getNoiseSet(x, y, z, world.chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -3 then
				if density < 0.01 then
					chunk.blocks[point] = nil
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (world.chunkSize * world.chunkSize) / world.chunkSize
				if density + internalY * 0.01 / world.chunkSize < 0.01 then
					chunk.blocks[point] = nil
				end
			end
		end
	end
}