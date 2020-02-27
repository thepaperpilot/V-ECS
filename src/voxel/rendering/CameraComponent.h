#pragma once

#include "../../ecs/World.h"
#include <glm/glm.hpp>

namespace vecs {

	// A component that stores the MVP matrices to send to the GPU
	// to create a 3D camera
	struct CameraComponent : Component {
		glm::mat4 model = glm::mat4(1.0);
		glm::mat4 view = glm::mat4(1.0);
		glm::mat4 projection = glm::mat4(1.0);

		// If the window resizes, then we need to recalculate the projection
		// matrix. Due to that, systems besides the camera system shouldn't
		// update the projection matrix directly, but rather use the projDirty
		// flag to tell it when to recalculate it. The camera system will set
		// the flag automatically on window resize
		float fieldOfView = glm::radians(45.0f);
		float near = 0.1;
		float far = 1000;

		bool projDirty = true;
		bool isDirty = true;
	};
}
