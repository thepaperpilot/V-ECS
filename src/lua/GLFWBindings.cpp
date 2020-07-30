#include "GLFWBindings.h"

#include "../jobs/Worker.h"
#include "../engine/Engine.h"

#include <GLFW/glfw3.h>

void vecs::GLFWBindings::setupState(sol::state& lua, Worker* worker, GLFWwindow* window) {
	lua.new_enum<uint16_t>("keys", {
		{ "Space", GLFW_KEY_SPACE },
		{ "Apostrophe", GLFW_KEY_APOSTROPHE },
		{ "Comma", GLFW_KEY_COMMA },
		{ "Minus", GLFW_KEY_MINUS },
		{ "Period", GLFW_KEY_PERIOD },
		{ "Slash", GLFW_KEY_SLASH },
		{ "0", GLFW_KEY_0 },
		{ "1", GLFW_KEY_1 },
		{ "2", GLFW_KEY_2 },
		{ "3", GLFW_KEY_3 },
		{ "4", GLFW_KEY_4 },
		{ "5", GLFW_KEY_5 },
		{ "6", GLFW_KEY_6 },
		{ "7", GLFW_KEY_7 },
		{ "8", GLFW_KEY_8 },
		{ "9", GLFW_KEY_9 },
		{ "W", GLFW_KEY_W },
		{ "Semicolon", GLFW_KEY_SEMICOLON },
		{ "Equal", GLFW_KEY_EQUAL },
		{ "A", GLFW_KEY_A },
		{ "B", GLFW_KEY_B },
		{ "C", GLFW_KEY_C },
		{ "D", GLFW_KEY_D },
		{ "E", GLFW_KEY_E },
		{ "F", GLFW_KEY_F },
		{ "G", GLFW_KEY_G },
		{ "H", GLFW_KEY_H },
		{ "I", GLFW_KEY_I },
		{ "J", GLFW_KEY_J },
		{ "K", GLFW_KEY_K },
		{ "L", GLFW_KEY_L },
		{ "M", GLFW_KEY_M },
		{ "N", GLFW_KEY_N },
		{ "O", GLFW_KEY_O },
		{ "P", GLFW_KEY_P },
		{ "Q", GLFW_KEY_Q },
		{ "R", GLFW_KEY_R },
		{ "S", GLFW_KEY_S },
		{ "T", GLFW_KEY_T },
		{ "U", GLFW_KEY_U },
		{ "V", GLFW_KEY_V },
		{ "W", GLFW_KEY_W },
		{ "X", GLFW_KEY_X },
		{ "Y", GLFW_KEY_Y },
		{ "Z", GLFW_KEY_Z },
		// We start skipping once we get to the function keys,
		// only binding the ones we think we'll need
		{ "LeftBracket", GLFW_KEY_LEFT_BRACKET },
		{ "Backslash", GLFW_KEY_BACKSLASH },
		{ "RightBracket", GLFW_KEY_RIGHT_BRACKET },
		{ "Grave", GLFW_KEY_GRAVE_ACCENT },
		{ "Escape", GLFW_KEY_ESCAPE },
		{ "Enter", GLFW_KEY_ENTER },
		{ "Tab", GLFW_KEY_TAB },
		{ "Backspace", GLFW_KEY_BACKSPACE },
		{ "Insert", GLFW_KEY_INSERT },
		{ "Delete", GLFW_KEY_DELETE },
		{ "Right", GLFW_KEY_RIGHT },
		{ "Left", GLFW_KEY_LEFT },
		{ "Down", GLFW_KEY_DOWN },
		{ "Up", GLFW_KEY_UP },
		{ "PageUp", GLFW_KEY_PAGE_UP },
		{ "PageDown", GLFW_KEY_PAGE_DOWN },
		{ "Home", GLFW_KEY_HOME },
		{ "End", GLFW_KEY_END },
		{ "LeftShift", GLFW_KEY_LEFT_SHIFT },
		{ "LeftControl", GLFW_KEY_LEFT_CONTROL },
		{ "LeftAlt", GLFW_KEY_LEFT_ALT },
		{ "LeftSuper", GLFW_KEY_LEFT_SUPER },
		{ "RightShift", GLFW_KEY_RIGHT_SHIFT },
		{ "RightControl", GLFW_KEY_RIGHT_CONTROL },
		{ "RightAlt", GLFW_KEY_RIGHT_ALT },
		{ "RightSuper", GLFW_KEY_RIGHT_SUPER }
	});

	// glfw namespace
	lua["glfw"] = lua.create_table_with(
		// We set a flag here to actually set the input mode on the main thread
		"hideCursor", [worker]() { worker->engine->nextInputMode = GLFW_CURSOR_DISABLED; },//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); },
		"showCursor", [worker]() { worker->engine->nextInputMode = GLFW_CURSOR_NORMAL; },//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); },
		"isCursorVisible", [window]() -> bool { return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL; },
		"cursorPos", [window]()->std::tuple<double, double> {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			return std::make_tuple(xpos, ypos);
		},
		"isKeyPressed", [window](int key) -> bool { return glfwGetKey(window, key) == GLFW_PRESS; },
		"windowSize", [window]()->std::tuple<int, int> {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			return std::make_tuple(width, height);
		}
	);
}
