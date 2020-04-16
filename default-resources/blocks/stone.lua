return {
	id = "vecs:stone",
	getArchetype = function(world)
		return world.renderers.voxel:getBlockFromTexture("stone.png")
	end
}
