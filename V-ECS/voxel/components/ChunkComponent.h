#pragma once

#include "../../ecs/World.h"

namespace vecs {

	struct ChunkComponent : public Component {
		int32_t x;
		int32_t y;
		int32_t z;
		
		// TODO calculate max chunk size by how large a blockPos value can be stored in a uint16_t
		// If it seems too small, consider making blockPos a uint32_t or even larger
		std::unordered_map<uint16_t, uint32_t> blocks;
		std::unordered_map<uint16_t, uint32_t> addedBlocks;
		std::unordered_map<uint16_t, uint32_t> removedBlocks;
	};
}
