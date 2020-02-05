#pragma once

#include <cstdint>

namespace vecs {

	// Forward Declarations
	class World;
	
	// A System is part of an ECS design. It holds some logic that will
	// handle a specific feature in the program.
	class System {
	public:

		World* world;

		virtual void init() {};

		virtual void update() = 0;
	};
}
