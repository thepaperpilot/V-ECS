#pragma once

#include <glm/gtx/hash.hpp>

#include "../../ecs/World.h"

#include "../../libs/octree/octree.h"

namespace vecs {

	struct ChunkComponent : public Component {

		ChunkComponent(uint16_t chunkSize, int32_t x, int32_t y, int32_t z) : blocks(chunkSize), x(x), y(y), z(z) {}

		int32_t x;
		int32_t y;
		int32_t z;
		
		Octree<uint32_t> blocks;
		std::unordered_set<glm::u16vec3> addedBlocks;
		std::unordered_set<glm::u16vec3> removedBlocks;
	};
}
