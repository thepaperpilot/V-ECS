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
		ChunkSystem(uint16_t chunkSize) {
			this->chunkSize = chunkSize;
		}

		void init() override;
		void update() override;

	private:
		uint16_t chunkSize;

		EntityQuery chunks;

		void checkAddedBlock(uint32_t entity, glm::u16vec3 internalPos, ChunkComponent* chunk, MeshComponent* mesh,
			ChunkComponent* top, MeshComponent* topMesh, ChunkComponent* bottom, MeshComponent* bottomMesh,
			ChunkComponent* back, MeshComponent* backMesh, ChunkComponent* front, MeshComponent* frontMesh,
			ChunkComponent* left, MeshComponent* leftMesh, ChunkComponent* right, MeshComponent* rightMesh);

		// Internal offset is the blockPos difference between this block and a theoretical one next to this one
		// inside this chunk along the correct axis
		// External offset is for the theoretical block next to this one in an adjacent chunk
		void checkFace(glm::vec3 p0, glm::vec3 p1, glm::vec4 uvs, bool clockWise, bool isOnEdge,
			glm::u16vec3 internalPos, glm::u16vec3 externalPos, ChunkComponent* chunk, MeshComponent* mesh,
			ChunkComponent* adjacentChunk, MeshComponent* adjacentMesh, glm::vec3 adjacentP0, glm::vec3 adjacentP1);
	};
}
