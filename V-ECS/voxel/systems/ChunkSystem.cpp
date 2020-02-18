#include "ChunkSystem.h"
#include "../components/ChunkComponent.h"
#include "../components/BlockComponent.h"
#include "../rendering/MeshComponent.h"
#include "../../ecs/World.h"

using namespace vecs;

void ChunkSystem::init() {
	// Create our query for all active chunks,
	// plus their mesh components so we can update their meshes
	// whenever blocks are added or removed
	chunks.filter.with(typeid(ChunkComponent));
	chunks.filter.with(typeid(MeshComponent));
	world->addQuery(&chunks);

	// Create our query for all Blocks,
	// so we can add them to the chunks as necessary
	blocks.filter.with(typeid(BlockComponent));
	blocks.onEntityAdded = EntityQuery::bind(this, &ChunkSystem::onBlockAdded);
	blocks.onEntityRemoved = EntityQuery::bind(this, &ChunkSystem::onBlockRemoved);
	world->addQuery(&blocks);

	// Store pointers to specific component lists to speed up lookups later
	meshComponents = world->getComponentList<MeshComponent>();
	chunkComponents = world->getComponentList<ChunkComponent>();
}

void ChunkSystem::update() {
	for (uint32_t entity : chunks.entities) {
		// No need to check if entity has chunk and mesh components, because the entity query already assures that
		ChunkComponent* chunk = static_cast<ChunkComponent*>(chunkComponents->at(entity));

		// Early exit if the chunk is un-changed
		if (chunk->addedBlocks.size() == 0 && chunk->removedBlocks.size() == 0)
			return;

		MeshComponent* mesh = static_cast<MeshComponent*>(meshComponents->at(entity));

		// Find adjacent chunks
		ChunkComponent* front = nullptr;
		MeshComponent* frontMesh = nullptr;
		ChunkComponent* back = nullptr;
		MeshComponent* backMesh = nullptr;
		ChunkComponent* left = nullptr;
		MeshComponent* leftMesh = nullptr;
		ChunkComponent* right = nullptr;
		MeshComponent* rightMesh = nullptr;
		ChunkComponent* top = nullptr;
		MeshComponent* topMesh = nullptr;
		ChunkComponent* bottom = nullptr;
		MeshComponent* bottomMesh = nullptr;
		// TODO find out how to optimize
		for (uint32_t entity : chunks.entities) {
			ChunkComponent* otherChunk = static_cast<ChunkComponent*>(chunkComponents->at(entity));
			if (otherChunk->x == chunk->x) {
				if (otherChunk->y == chunk->y) {
					if (otherChunk->z == chunk->z + 1) {
						front = otherChunk;
						frontMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
					} else if (otherChunk->z == chunk->z - 1) {
						back = otherChunk;
						backMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
					}						
				} else if (otherChunk->z == chunk->z) {
					if (otherChunk->y == chunk->y + 1) {
						top = otherChunk;
						topMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
					} else if (otherChunk->y == chunk->y - 1) {
						bottom = otherChunk;
						bottomMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
					}
				}
			} else if (otherChunk->y == chunk->y && otherChunk->z == chunk->z) {
				if (otherChunk->x == chunk->x + 1) {
					right = otherChunk;
					rightMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
				} else if (otherChunk->x == chunk->x - 1) {
					left = otherChunk;
					leftMesh = static_cast<MeshComponent*>(meshComponents->at(entity));
				}
			}
		}

		// For each added block, check if we need to add or remove the various faces
		for (auto kvp : chunk->addedBlocks) {
			uint16_t blockPos = kvp.first;

			// Integer division discards fractional data, i.e. it floors the result
			// Internal coordinates are relative to the chunk
			uint16_t internalX = blockPos / chunkSizeSquared;
			uint16_t internalY = (blockPos % chunkSizeSquared) / chunkSize;
			uint16_t internalZ = blockPos % chunkSize;
			// These coordinates are global
			int32_t x = chunk->x * chunkSize + internalX;
			int32_t y = chunk->y * chunkSize + internalY;
			int32_t z = chunk->z * chunkSize + internalZ;

			// For each face, we check if the adjacent block exists. If it does, then in theory
			//  we don't need to add our face, just remove there's, because the two blocks are sharing
			//  those faces and therefore don't need to render any geometry.
			// Note: since we're removing the adjacent block's vertices, the clockwise ordering
			//  of the points are reversed compared to the face we could've added
			// However, if the other block was also just added, then there is no face to remove, so we
			//  shouldn't do anything.
			// If the adjacent block doesn't exist, then we should all our face. It doesn't matter if
			//  the block was just removed or not.

			// Top Face
			checkFace({ x, y + 1, z }, { x + 1, y + 1, z + 1 }, { 1.0, 1.0 }, { 0.0, 0.0 }, true,
				internalY == chunkSize - 1, blockPos + chunkSize, blockPos - (chunkSize - 1) * chunkSize, chunk, mesh,
				top, topMesh, { x + 1, 0, z + 1 }, { x, 0, z });

			// Bottom Face
			checkFace({ x, y, z }, { x + 1, y, z + 1 }, { 1.0, 0.0 }, { 0.0, 1.0 }, false,
				internalY == 0, blockPos - chunkSize, blockPos + (chunkSize - 1) * chunkSize, chunk, mesh,
				bottom, bottomMesh, { x + 1, (chunkSize - 1) * chunkSize, z + 1 }, { x, (chunkSize - 1) * chunkSize, z });

			// Back Face
			checkFace({ x + 1, y + 1, z + 1 }, { x, y, z + 1 }, { 1.0, 0.0 }, { 0.0, 1.0 }, false,
				internalZ == chunkSize - 1, blockPos + 1, blockPos - (chunkSize - 1), chunk, mesh,
				back, backMesh, { x, y, 0 }, { x + 1, y + 1, 0 });
			
			// Front Face
			checkFace({ x, y, z }, { x + 1, y + 1, z }, { 1.0, 1.0 }, { 0.0, 0.0 }, true,
				internalZ == 0, blockPos - 1, blockPos + (chunkSize - 1), chunk, mesh,
				front, frontMesh, { x + 1, y + 1, chunkSize - 1 }, { x, y, chunkSize - 1 });

			// Left Face
			checkFace({ x + 1, y, z }, { x + 1, y + 1, z + 1 }, { 1.0, 1.0 }, { 0.0, 0.0 }, true,
				internalX == chunkSize - 1, blockPos + chunkSizeSquared, blockPos - (chunkSize - 1) * chunkSizeSquared, chunk, mesh,
				left, leftMesh, { 0, y + 1, z + 1 }, { 0, y, z });

			// Right Face
			checkFace({ x, y, z + 1 }, { x, y + 1, z }, { 1.0, 1.0 }, { 0.0, 0.0 }, true,
				internalX == 0, blockPos - chunkSizeSquared, blockPos + (chunkSize - 1) * chunkSizeSquared, chunk, mesh,
				right, rightMesh, { (chunkSize - 1) * chunkSizeSquared, y + 1, z }, { (chunkSize - 1) * chunkSizeSquared, y, z + 1 });
		}
		chunk->addedBlocks.clear();

		// TODO remove blocks
	}
}

void ChunkSystem::checkFace(glm::vec3 p0, glm::vec3 p1, glm::vec2 uv0, glm::vec2 uv1, bool clockWise,
	bool isOnEdge, uint16_t internalOffset, uint16_t externalOffset, ChunkComponent* chunk, MeshComponent* mesh,
	ChunkComponent* adjacentChunk, MeshComponent* adjacentMesh, glm::vec3 adjacentP0, glm::vec3 adjacentP1) {

	// Remove our face if we're not on the edge and there's a block next to us
	if (!isOnEdge ? chunk->blocks.count(internalOffset) :
		// or if we're at the edge and there either isn't a chunk next to us,
		//  or the chunk next to us has a block next to us
		adjacentChunk == nullptr || adjacentChunk->blocks.count(externalOffset)) {

		// Remove face in chunk next to us
		if (isOnEdge && adjacentChunk != nullptr && !adjacentChunk->addedBlocks.count(externalOffset))
			adjacentMesh->removeFace(adjacentP0, adjacentP1, !clockWise);
		// Remove face in block next to us (in our chunk)
		else if (!chunk->addedBlocks.count(internalOffset))
			mesh->removeFace(p1, p0, !clockWise);

		// If neither if statement triggers it means we just added this block and the one next to it, so
		//  there's no face to remove, we just won't add it in the first place
	} else {
		// If there's no block next to us, then add a new face to our geometry
		mesh->addFace(p0, uv0, p1, uv1, clockWise);
	}
}

void ChunkSystem::onBlockAdded(uint32_t entity) {
	/*
	BlockComponent* block = world->getComponent<BlockComponent>(entity);
	ChunkComponent* chunk = world->getComponent<ChunkComponent>(block->chunk);

	// Block position will be stored in a single value that can be easily converted into x,y,z
	// but also be used as a key to find adjacent blocks quickly
	uint16_t blockPos = block->x * chunkSizeSquared + block->y * chunkSize + block->z;
	// Add the entity to that chunk's blocks
	chunk->blocks[blockPos] = entity;
	// Mark that this block was added
	chunk->addedBlocks[blockPos] = entity;
	*/
}

void ChunkSystem::onBlockRemoved(uint32_t entity) {
	/*
	BlockComponent* block = world->getComponent<BlockComponent>(entity);
	ChunkComponent* chunk = world->getComponent<ChunkComponent>(block->chunk);

	// Block position will be stored in a single value that can be easily converted into x,y,z
	// but also be used as a key to find adjacent blocks quickly
	uint16_t blockPos = block->x * chunkSizeSquared + block->y * chunkSize + block->z;
	// Add the entity to that chunk's blocks
	chunk->blocks.erase(blockPos);
	// Mark that this block was added
	chunk->removedBlocks[blockPos] = entity;
	*/
}
