#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct ChunkComponent : public Component {
		std::vector<uint32_t> blocks;
	};
}
