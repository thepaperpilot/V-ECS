#pragma once

#include "../ecs/World.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm\ext\matrix_transform.hpp>

namespace vecs {

	// A component that stores the MVP matrices to send to the GPU
	// to create a 3D camera
	struct CameraComponent : Component {
		// We store the projection matrix since it won't change often
		glm::mat4 projection = glm::mat4(1.0);

		// Store camera's position in order to calculate view matrix
		glm::vec3 position = glm::vec3(0);
		glm::vec3 forwardDir = glm::vec3(0, 0, 1);
		glm::vec3 upDir = glm::vec3(0, 1, 0);

		// If the window resizes, then we need to recalculate the projection
		// matrix. Due to that, systems besides the camera system shouldn't
		// update the projection matrix directly, but rather use the projDirty
		// flag to tell it when to recalculate it. The camera system will set
		// the flag automatically on window resize
		float fieldOfView = glm::radians(45.0f);
		float near = 0.1;
		float far = 10000000000;

		bool projDirty = true;
		bool isDirty = true;

		// Small utility function to get a view matrix for this camera
		glm::mat4 getViewMatrix() {
			return glm::lookAt(position, position + forwardDir, upDir);
		}
	};
}
