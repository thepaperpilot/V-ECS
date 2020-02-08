#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	// Forward declarations
	class Engine;

	class PostRenderSystem : public System {
	public:
		PostRenderSystem(Engine* engine) {
			this->engine = engine;
		}

		void init() override;
		void update() override;

	private:
		Engine* engine;

		EntityQuery renderState;
	};
}
