#pragma once

#include "../events/EventManager.h"

#include <GLFW/glfw3.h>

namespace vecs {

	struct RefreshWindowEvent : public EventData {};

	struct WindowResizeEvent : public EventData {
		int width;
		int height;
	};

	struct MouseMoveEvent : public EventData {
		double xPos;
		double yPos;
	};
	
	// Some events get a mods field
	// Mods is a set of modifier flags you can get bitwise
	// 1 - Shift
	// 2 - Control
	// 4 - Alt
	// 8 - Super (Windows key or Command key)
	// 16 - Caps Lock (if GLFW_LOCK_KEY_MODS input mode is set)
	// 32 - Num Lock (if GLFW_LOCK_KEY_MODS input mode is set)

	struct LeftMousePressEvent : public EventData {
		int mods;
	};

	struct LeftMouseReleaseEvent : public EventData {
		int mods;
	};

	struct RightMousePressEvent : public EventData {
		int mods;
	};

	struct RightMouseReleaseEvent : public EventData {
		int mods;
	};

	struct VerticalScrollEvent : public EventData {
		double yOffset;
	};

	struct HorizontalScrollEvent : public EventData {
		double xOffset;
	};

	struct KeyPressEvent : public EventData {
		int key;
		int scancode;
		int mods;
	};

	struct KeyReleaseEvent : public EventData {
		int key;
		int scancode;
		int mods;
	};

	// GLFW input callbacks
	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
		MouseMoveEvent event;
		event.xPos = xpos;
		event.yPos = ypos;
		EventManager::fire(event);
	}

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				LeftMousePressEvent event;
				event.mods = mods;
				EventManager::fire(event);
			}
			else {
				LeftMouseReleaseEvent event;
				event.mods = mods;
				EventManager::fire(event);
			}
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				RightMousePressEvent event;
				event.mods = mods;
				EventManager::fire(event);
			}
			else {
				RightMouseReleaseEvent event;
				event.mods = mods;
				EventManager::fire(event);
			}
		}
		// TODO add a middle click event?
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			KeyPressEvent event;
			event.key = key;
			event.scancode = scancode;
			event.mods = mods;
			EventManager::fire(event);
		}
		else if (action == GLFW_RELEASE) {
			KeyReleaseEvent event;
			event.key = key;
			event.scancode = scancode;
			event.mods = mods;
			EventManager::fire(event);
		}
	}

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		if (xoffset != 0) {
			HorizontalScrollEvent event;
			event.xOffset = xoffset;
			EventManager::fire(event);
		}
		if (yoffset != 0) {
			VerticalScrollEvent event;
			event.yOffset = yoffset;
			EventManager::fire(event);
		}
	}

	static void windowResizeCallback(GLFWwindow* window, int width, int height) {
		WindowResizeEvent event;
		event.width = width;
		event.height = height;
		EventManager::fire(event);
	}

	static void windowRefreshCallback(GLFWwindow* window) {
		EventManager::fire(RefreshWindowEvent());
	}
}
