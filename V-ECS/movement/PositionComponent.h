#pragma once

#include "../ecs/World.h"
#include <glm/glm.hpp>

namespace vecs {

	struct PositionComponent : public Component {
		glm::vec3 position{ 0.0,0.0,0.0 };
	};
}
