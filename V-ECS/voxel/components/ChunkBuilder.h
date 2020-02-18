#pragma once

#include "../../ecs/ArchetypeBuilder.h"

namespace vecs {

	class ChunkBuilder : public ArchetypeBuilder {
	public:
		ChunkBuilder(World* world, uint16_t chunkSize) : ArchetypeBuilder(world) {
			this->chunkSize = chunkSize;
			this->chunkSizeSquared = chunkSize * chunkSize;
		}

		void fillChunk(uint32_t chunk, ChunkComponent* chunkComponent) {
			auto blockComponentList = world->getComponentList<BlockComponent>();
			auto hint = blockComponentList->end();
			
			// TODO load blocks in and create one BlockComponent for each type
			BlockComponent block = {};

			//auto queries = getQueriesMatchingArchetype({ typeid(BlockComponent) });
			uint32_t firstEntity = createEntities(chunkSize * chunkSize * chunkSize / 8);
			for (uint16_t x = chunkSize / 4; x < 3 * chunkSize / 4; x++) {
				uint32_t firstXEntity = firstEntity + (x - chunkSize / 4) * chunkSize * chunkSize / 4;
				for (uint16_t y = chunkSize / 4; y < 3 * chunkSize / 4; y++) {
					uint32_t firstYEntity = firstXEntity + (y - chunkSize / 4) * chunkSize / 2;
					for (uint16_t z = chunkSize / 4; z < 3 * chunkSize / 4; z++) {
						uint32_t entity = firstYEntity + (z - chunkSize / 4);
						blockComponentList->insert(hint, { entity, &block });
						// Calling onEntityAdded is pretty slow
						// At least for the moment we know exactly what's going to be happening: ChunkSystem is going
						// to find the BlockComponent and the ChunkComponent it uses, and add the block to the chunk.
						// We can just do that here more performantly
						// This is very dangerous! If another system gets added that cares about BlockComponents, they
						// won't get caught by any onEntityAdded queries!
						//for (auto query : queries)
							//if (query->onEntityAdded) query->onEntityAdded(entity);

						uint16_t blockPos = x * chunkSizeSquared + y * chunkSize + z;
						chunkComponent->blocks[blockPos] = entity;
						chunkComponent->addedBlocks[blockPos] = entity;
					}
				}
			}
		}
		
	private:
		uint16_t chunkSize;
		uint16_t chunkSizeSquared;
	};
}
