#pragma once

#include <glm/glm.hpp>

#include "../../ecs/System.h"
#include "../../ecs/EntityQuery.h"
#include "ChunkComponent.h"

namespace vecs {

	// Forward Declarations
	struct BlockComponent;
	struct ChunkComponent;
	struct MeshComponent;

	class ChunkSystem : public System {
	public:
		static void addBlockFaces(BlockComponent* blockComponent, glm::u16vec3 internalPos,
			uint16_t chunkSize, glm::i32vec3 chunkPos, Octree<uint32_t>* blocks, MeshComponent* mesh);

		ChunkSystem(uint16_t chunkSize) {
			this->chunkSize = chunkSize;
		}

		void init() override;
		void update() override;

	private:
		uint16_t chunkSize;

		EntityQuery chunks;
	};
}
