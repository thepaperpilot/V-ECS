#pragma once

#include "../../ecs/World.h"

#include <glm/gtx/string_cast.hpp>

namespace vecs {

	class ChunkBuilder {
	public:
		ChunkBuilder(World* world, uint16_t chunkSize) {
			this->world = world;
			this->chunkSize = chunkSize;
			
			chunkArchetype = world->getArchetype({ typeid(ChunkComponent), typeid(MeshComponent) });
			chunkComponents = chunkArchetype->getComponentList(typeid(ChunkComponent));
			meshComponents = chunkArchetype->getComponentList(typeid(MeshComponent));
		}

		void fillChunk(int32_t x, int32_t y, int32_t z) {
			size_t chunk = chunkArchetype->createEntities(1).second;

			ChunkComponent* chunkComponent = new ChunkComponent(chunkSize, x, y, z);
			chunkComponents->at(chunk) = chunkComponent;

			MeshComponent* meshComponent = new MeshComponent;
			meshComponent->minBounds = { x * chunkSize, y * chunkSize, z * chunkSize };
			meshComponent->maxBounds = { (x + 1) * chunkSize, (y + 1) * chunkSize, (z + 1) * chunkSize };
			meshComponents->at(chunk) = meshComponent;

			if (y > 0) {
				// Sky doesn't have blocks!
				return;
			}
			
			std::unordered_map<std::type_index, Component*> sharedComponents;
			sharedComponents.insert({ typeid(BlockComponent), new BlockComponent });
			Archetype* blockArchetype = world->getArchetype({}, &sharedComponents);

			std::pair<uint32_t, size_t> block = blockArchetype->createEntities(chunkSize * chunkSize * chunkSize / 8);
			//chunkComponent->addedBlocks.reserve(chunkSize * chunkSize * chunkSize / 8);

			for (uint16_t x = chunkSize / 4; x < 3 * chunkSize / 4; x++) {
				for (uint16_t y = chunkSize / 4; y < 3 * chunkSize / 4; y++) {
					for (uint16_t z = chunkSize / 4; z < 3 * chunkSize / 4; z++) {
						chunkComponent->blocks.set(x, y, z, block.first);
						//chunkComponent->addedBlocks.insert({ x, y, z });

						block.first++;
						block.second++;
					}
				}
			}
		}
		
	private:
		World* world;

		uint16_t chunkSize;

		Archetype* chunkArchetype;
		std::vector<Component*>* chunkComponents;
		std::vector<Component*>* meshComponents;
	};
}
