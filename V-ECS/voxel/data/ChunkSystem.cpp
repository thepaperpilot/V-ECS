#include "ChunkSystem.h"
#include "../data/ChunkComponent.h"
#include "../data/BlockComponent.h"
#include "../rendering/MeshComponent.h"
#include "../../ecs/World.h"

using namespace vecs;

void ChunkSystem::addBlockFaces(BlockComponent* blockComponent, glm::u16vec3 internalPos, uint16_t chunkSize, glm::i32vec3 chunkPos, Octree<uint32_t>* blocks, MeshComponent* mesh) {
	// These coordinates are global
	int32_t x = chunkPos.x * chunkSize + internalPos.x;
	int32_t y = chunkPos.y * chunkSize + internalPos.y;
	int32_t z = chunkPos.z * chunkSize + internalPos.z;

	// Top Face
	if (internalPos.y == chunkSize - 1 || !blocks->at(internalPos.x, internalPos.y + 1, internalPos.z))
		mesh->addFace({ x, y + 1, z }, { x + 1, y + 1, z + 1 }, blockComponent->top, true);

	// Bottom Face
	if (internalPos.y == 0 || !blocks->at(internalPos.x, internalPos.y - 1, internalPos.z))
		mesh->addFace({ x, y, z }, { x + 1, y, z + 1 }, blockComponent->bottom, false);

	// Back Face
	if (internalPos.z == chunkSize - 1 || !blocks->at(internalPos.x, internalPos.y, internalPos.z + 1))
		mesh->addFace({ x + 1, y + 1, z + 1 }, { x, y, z + 1 }, blockComponent->back, false);

	// Front Face
	if (internalPos.z == 0 || !blocks->at(internalPos.x, internalPos.y, internalPos.z - 1))
		mesh->addFace({ x, y, z }, { x + 1, y + 1, z }, blockComponent->front, true);

	// Left Face
	if (internalPos.x == chunkSize - 1 || !blocks->at(internalPos.x + 1, internalPos.y, internalPos.z))
		mesh->addFace({ x + 1, y, z }, { x + 1, y + 1, z + 1 }, blockComponent->left, true);

	// Right Face
	if (internalPos.x == 0 || !blocks->at(internalPos.x - 1, internalPos.y, internalPos.z))
		mesh->addFace({ x, y, z + 1 }, { x, y + 1, z }, blockComponent->right, true);
}

void ChunkSystem::init() {
	// Create our query for all active chunks,
	// plus their mesh components so we can update their meshes
	// whenever blocks are added or removed
	chunks.filter.with(typeid(ChunkComponent));
	chunks.filter.with(typeid(MeshComponent));
	world->addQuery(&chunks);
}

void ChunkSystem::update() {
	for (auto archetype : chunks.matchingArchetypes) {
		std::vector<Component*>* meshComponents = archetype->getComponentList(typeid(MeshComponent));
		std::vector<Component*>* chunkComponents = archetype->getComponentList(typeid(ChunkComponent));
		for (size_t i = 0; i < chunkComponents->size(); i++) {
			ChunkComponent* chunk = static_cast<ChunkComponent*>(chunkComponents->at(i));

			// Early exit if the chunk is un-changed
			if (chunk->addedBlocks.size() == 0 && chunk->removedBlocks.size() == 0)
				return;

			MeshComponent* mesh = static_cast<MeshComponent*>(meshComponents->at(i));

			// For each added block, check if we need to add or remove the various faces
			// TODO store addedBlocks by archetype?
			for (auto internalPos : chunk->addedBlocks) {
				uint32_t entity = chunk->blocks.at(internalPos.x, internalPos.y, internalPos.z);
				BlockComponent* blockComponent = world->getArchetype(entity)->getSharedComponent<BlockComponent>();
				addBlockFaces(blockComponent, internalPos, chunkSize, { chunk->x, chunk->y, chunk->z }, &chunk->blocks, mesh);
			}

			// TODO remove blocks
		}
	}

	for (auto archetype : chunks.matchingArchetypes) {
		std::vector<Component*>* meshComponents = archetype->getComponentList(typeid(MeshComponent));
		std::vector<Component*>* chunkComponents = archetype->getComponentList(typeid(ChunkComponent));
		for (size_t i = 0; i < chunkComponents->size(); i++) {
			ChunkComponent* chunk = static_cast<ChunkComponent*>(chunkComponents->at(i));
			
			chunk->addedBlocks.clear();
			chunk->removedBlocks.clear();
		}
	}
}
