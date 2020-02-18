#pragma once

#include <glm/glm.hpp>

#include "../../ecs/System.h"
#include "../../ecs/EntityQuery.h"
#include "../components/ChunkComponent.h"

namespace vecs {

	// Forward Declarations
	struct BlockComponent;
	struct ChunkComponent;
	struct MeshComponent;

	class ChunkSystem : public System {
	public:
		ChunkSystem(uint16_t chunkSize) {
			this->chunkSize = chunkSize;
			this->chunkSizeSquared = chunkSize * chunkSize;
		}

		void init() override;
		void update() override;

	private:
		uint16_t chunkSize;
		uint16_t chunkSizeSquared;

		EntityQuery chunks;
		EntityQuery blocks;

		std::map<uint32_t, Component*>* meshComponents;
		std::map<uint32_t, Component*>* chunkComponents;

		// Internal offset is the blockPos difference between this block and a theoretical one next to this one
		// inside this chunk along the correct axis
		// External offset is for the theoretical block next to this one in an adjacent chunk
		void checkFace(glm::vec3 p0, glm::vec3 p1, glm::vec2 uv0, glm::vec2 uv1, bool clockWise, bool isOnEdge,
			uint16_t internalOffset, uint16_t externalOffset, ChunkComponent* chunk, MeshComponent* mesh,
			ChunkComponent* adjacentChunk, MeshComponent* adjacentMesh, glm::vec3 adjacentP0, glm::vec3 adjacentP1);

		void onBlockAdded(uint32_t entity);
		void onBlockRemoved(uint32_t entity);
	};
}
