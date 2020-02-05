#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityQuery.h"

namespace vecs {

	class ChunkSystem : public System {
	public:
		void init() override;
		void update() override;

	private:
		EntityQuery chunks;
		EntityQuery blocks;

		void onBlockAdded(uint32_t entity);
		void onBlockRemoved(uint32_t entity);
	};
}
