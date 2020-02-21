#pragma once

#include <cstdint>
#include <string>

namespace vecs {

	class Manifest {
	public:
		// These values are used for the initial size of the window,
		// but the window may be resized
		int windowWidth = 1280;
		int windowHeight = 720;

		// This value is used for the intial title of the window
		std::string applicationName = "A V-ECS Application";
		uint32_t applicationVersion = 0;

		Manifest();
	};
}
