#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"
#include "../input/InputEvents.h"
#include <GLFW/glfw3.h>

namespace vecs {

	class ControllerSystem : public System {
	public:
		ControllerSystem(GLFWwindow* window) {
			this->window = window;
		}

		void init() override;
		void update() override;

	private:
		EntityQuery controlled;
		GLFWwindow* window;

		double lastX = 0;
		double lastY = 0;

		void onControlledAdded();
		void onControlledRemoved();

		void onMouseMove(MouseMoveEvent* event);
		void onLeftMousePress(LeftMousePressEvent* event) {};
		void onLeftMouseRelease(LeftMouseReleaseEvent* event) {};
		void onRightMousePress(RightMousePressEvent* event) {};
		void onVerticalScroll(VerticalScrollEvent* event) {};
		void onKeyPress(KeyPressEvent* event);
		void onKeyRelease(KeyReleaseEvent* event);
		// TODO joystick and gamepad controls
	};
}
