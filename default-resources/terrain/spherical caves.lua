-- Spherical underground rooms, like the "Worley caves" in https://forum.playboundless.com/t/having-the-hardest-time-with-cave-noise/2664/2

return {
	priority = 100,
	init = function(self, seed)
		self.caveNoise = noise.new(noiseType.Cellular, seed, .01)
	end,
	generateChunk = function(self, world, chunk, x, y, z)
		if y > -2 then return end
		local chunkSize = world.renderers.voxel.chunkSize
		local noiseSet = self.caveNoise:getNoiseSet(x, y, z, chunkSize)
		for point,density in pairs(noiseSet) do
			if y < -3 then
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