#pragma once

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <vulkan\vulkan.h>

namespace vecs {

	// Forward Declarations
	class Device;
	class Worker;

	namespace RenderingBindings {

		void setupState(sol::state& lua, Worker* worker, Device* device);
	}
}
