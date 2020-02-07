#pragma once

#include "../../ecs/World.h"

namespace vecs {

	struct ChunkComponent : public Component {
		std::set<uint32_t> blocks;
	};
}