#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct PositionComponent : public Component {
		glm::vec3 position{ 0.0,0.0,0.0 };
	};
}
