#pragma once

#include "../../ecs/World.h"

#include <glm/glm.hpp>

namespace vecs {

	struct BlockComponent : public Component {
		// Stores texture UVs for each side of the block
		glm::vec4 top;
		glm::vec4 bottom;
		glm::vec4 front;
		glm::vec4 back;
		glm::vec4 left;
		glm::vec4 right;
	};
}
