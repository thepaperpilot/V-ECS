#pragma once

#include "../ecs/World.h"
#include <glm/glm.hpp>

namespace vecs {

	struct PositionComponent : public Component {
		glm::vec3 position{ -2.0,0.5,0.5 };
	};
}
