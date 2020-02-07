#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	class MovementSystem : public System {
	public:
		void init() override;
		void update() override;

	private:
		EntityQuery withVelocity;
	};
}
