-- Based off the simplex caves described here: https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2

return {
	priority = 100,
	init = function(self, world, seed)
		self.caveNoise = noise.new(noiseType.Simplex, seed, .1)
		self.noiseBuffer = noiseBuffer.new(world.renderers.voxel.chunkSize)
	end,
	generateChunk = function(self, world, chunk, x, y, z)
		if y > -1 then return end
		local chunkSize = world.renderers.voxel.chunkSize
		local noiseSet = self.caveNoise:getNoiseSet(self.noiseBuffer, x, y, z, chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -2 then
				if density > 0.4 then
					chunk.blocks[point - 1] = nil
				end
			else
				-- Taper off over the course of one vertical chunk
				local internalY = (point - 1) % (chunkSize * chunkSize) / chunkSize
				if density - internalY * 0.4 / chunkSize > 0.4 then
					chunk.blocks[point - 1] = nil
				end
			end
		end
	end
}
