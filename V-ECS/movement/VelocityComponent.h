#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct VelocityComponent : public Component {
		glm::vec3 velocity{ 0.0, 0.0, 0.0 };
	};
}
