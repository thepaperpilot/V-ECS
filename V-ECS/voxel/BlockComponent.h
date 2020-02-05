#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct BlockComponent : public Component {
		uint32_t x;
		uint32_t y;
		uint32_t z;
	};
}
