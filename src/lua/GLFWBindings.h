#pragma once

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

// Forward Declarations
struct GLFWwindow;

namespace vecs {

	// Forward Declarations
	class Worker;

	namespace GLFWBindings {

		void setupState(sol::state& lua, Worker* Worker, GLFWwindow* window);
	}
}
