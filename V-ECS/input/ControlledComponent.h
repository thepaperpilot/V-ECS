#pragma once

#include "../ecs/World.h"

namespace vecs {

	struct ControlledComponent : public Component {
		float speed = 2;
		float lookSpeed = 0.1;

		// Inputs is a set of bitwise flags
		// 1 - Forward
		// 2 - Left
		// 4 - Backwards
		// 8 - Right
		// 16 - Up
		// 32 - Down
		int inputs = 0;

		float pitch = 0;
		float yaw = 0;
		bool dirty = true;
	};
}
