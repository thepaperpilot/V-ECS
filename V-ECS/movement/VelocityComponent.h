#pragma once

#include "../ecs/World.h"
#include <glm/glm.hpp>

namespace vecs {

	struct VelocityComponent : public Component {
		glm::vec3 velocity{ 0.0, 0.0, 0.0 };
	};
}
