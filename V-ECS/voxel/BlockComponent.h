#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct BlockComponent : public Component {
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t z = 0;
	};
}
