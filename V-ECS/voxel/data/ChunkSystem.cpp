#include "ChunkSystem.h"
#include "../data/ChunkComponent.h"
#include "../data/BlockComponent.h"
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
}

void ChunkSystem::update() {
	for (auto archetype : chunks.matchingArchetypes) {
		std::vector<Component*>* meshComponents = archetype->getComponentList(typeid(MeshComponent));
		std::vector<Component*>* chunkComponents = archetype->getComponentList(typeid(ChunkComponent));
		for (size_t i = 0; i < chunkComponents->size(); i++) {
			ChunkComponent* chunk = static_cast<ChunkComponent*>(chunkComponents->at(i));

			// Early exit if the chunk is un-changed
			if (chunk->addedBlocks.size() == 0 && chunk->removedBlocks.size() == 0 && !chunk->justCreated)
				return;

			MeshComponent* mesh = static_cast<MeshComponent*>(meshComponents->at(i));

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
			for (auto archetype : chunks.matchingArchetypes) {
				std::vector<Component*>* otherChunkComponents = archetype->getComponentList(typeid(ChunkComponent));
				std::vector<Component*>* otherMeshComponents = archetype->getComponentList(typeid(ChunkComponent));
				for (size_t i = 0; i < otherChunkComponents->size(); i++) {
					ChunkComponent* otherChunk = static_cast<ChunkComponent*>(otherChunkComponents->at(i));
					if (otherChunk->x == chunk->x) {
						if (otherChunk->y == chunk->y) {
							if (otherChunk->z == chunk->z + 1) {
								front = otherChunk;
								frontMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
							}
							else if (otherChunk->z == chunk->z - 1) {
								back = otherChunk;
								backMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
							}
						}
						else if (otherChunk->z == chunk->z) {
							if (otherChunk->y == chunk->y + 1) {
								top = otherChunk;
								topMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
							}
							else if (otherChunk->y == chunk->y - 1) {
								bottom = otherChunk;
								bottomMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
							}
						}
					}
					else if (otherChunk->y == chunk->y && otherChunk->z == chunk->z) {
						if (otherChunk->x == chunk->x + 1) {
							right = otherChunk;
							rightMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
						}
						else if (otherChunk->x == chunk->x - 1) {
							left = otherChunk;
							leftMesh = static_cast<MeshComponent*>(otherMeshComponents->at(i));
						}
					}
				}
			}

			if (chunk->justCreated) {
				for (uint16_t z = 0; z < chunkSize; z++) {
					for (uint16_t y = 0; y < chunkSize; y++) {
						for (uint16_t x = 0; x < chunkSize; x++) {
							uint32_t entity = chunk->blocks.at(x, y, z);
							if (entity)
								checkAddedBlock(entity, { x, y, z }, chunk, mesh,
									top, topMesh, bottom, bottomMesh, back, backMesh, front, frontMesh, left, leftMesh, right, rightMesh);
						}
					}
				}
			} else {
				// For each added block, check if we need to add or remove the various faces
				for (auto internalPos : chunk->addedBlocks) {
					checkAddedBlock(chunk->blocks.at(internalPos.x, internalPos.y, internalPos.z), internalPos,
						chunk, mesh, top, topMesh, bottom, bottomMesh, back, backMesh, front, frontMesh, left, leftMesh, right, rightMesh);
				}
			}

			// TODO remove blocks
		}
	}

	for (auto archetype : chunks.matchingArchetypes) {
		std::vector<Component*>* meshComponents = archetype->getComponentList(typeid(MeshComponent));
		std::vector<Component*>* chunkComponents = archetype->getComponentList(typeid(ChunkComponent));
		for (size_t i = 0; i < chunkComponents->size(); i++) {
			ChunkComponent* chunk = static_cast<ChunkComponent*>(chunkComponents->at(i));
			
			chunk->justCreated = false;
			chunk->addedBlocks.clear();
			chunk->removedBlocks.clear();
		}
	}
}

void ChunkSystem::checkAddedBlock(uint32_t entity, glm::u16vec3 internalPos, ChunkComponent* chunk, MeshComponent* mesh,
	ChunkComponent* top, MeshComponent* topMesh, ChunkComponent* bottom, MeshComponent* bottomMesh,
	ChunkComponent* back, MeshComponent* backMesh, ChunkComponent* front, MeshComponent* frontMesh,
	ChunkComponent* left, MeshComponent* leftMesh, ChunkComponent* right, MeshComponent* rightMesh) {

	// These coordinates are global
	int32_t x = chunk->x * chunkSize + internalPos.x;
	int32_t y = chunk->y * chunkSize + internalPos.y;
	int32_t z = chunk->z * chunkSize + internalPos.z;

	// For each face, we check if the adjacent block exists. If it does, then in theory
	//  we don't need to add our face, just remove there's, because the two blocks are sharing
	//  those faces and therefore don't need to render any geometry.
	// Note: since we're removing the adjacent block's vertices, the clockwise ordering
	//  of the points are reversed compared to the face we could've added
	// However, if the other block was also just added, then there is no face to remove, so we
	//  shouldn't do anything.
	// If the adjacent block doesn't exist, then we should all our face. It doesn't matter if
	//  the block was just removed or not.

	// TODO not sure how I can better organize chunks such that I don't need to query this for every block
	// (in theory I should just need to do it for each archetype present in the chunk)
	BlockComponent* blockComponent = world->getArchetype(entity)->getSharedComponent<BlockComponent>();

	// Top Face
	checkFace({ x, y + 1, z }, { x + 1, y + 1, z + 1 }, blockComponent->top, true, internalPos.y == chunkSize - 1,
		internalPos + glm::u16vec3{ 0, 1, 0 }, internalPos + glm::u16vec3{ 0, -chunkSize + 1, 0 },
		chunk, mesh, top, topMesh, { x + 1, 0, z + 1 }, { x, 0, z });

	// Bottom Face
	checkFace({ x, y, z }, { x + 1, y, z + 1 }, blockComponent->bottom, false, internalPos.y == 0,
		internalPos + glm::u16vec3{ 0, -1, 0 }, internalPos + glm::u16vec3{ 0, chunkSize - 1, 0 },
		chunk, mesh, bottom, bottomMesh, { x + 1, chunkSize - 1, z + 1 }, { x, chunkSize - 1, z });

	// Back Face
	checkFace({ x + 1, y + 1, z + 1 }, { x, y, z + 1 }, blockComponent->back, false, internalPos.z == chunkSize - 1,
		internalPos + glm::u16vec3{ 0, 0, 1 }, internalPos + glm::u16vec3{ 0, 0, -chunkSize + 1 },
		chunk, mesh, back, backMesh, { x, y, 0 }, { x + 1, y + 1, 0 });

	// Front Face
	checkFace({ x, y, z }, { x + 1, y + 1, z }, blockComponent->front, true, internalPos.z == 0,
		internalPos + glm::u16vec3{ 0, 0, -1 }, internalPos + glm::u16vec3{ 0, 0, chunkSize - 1 },
		chunk, mesh, front, frontMesh, { x + 1, y + 1, chunkSize - 1 }, { x, y, chunkSize - 1 });

	// Left Face
	checkFace({ x + 1, y, z }, { x + 1, y + 1, z + 1 }, blockComponent->left, true, internalPos.x == chunkSize - 1,
		internalPos + glm::u16vec3{ 1, 0, 0 }, internalPos + glm::u16vec3{ -chunkSize + 1, 0, 0 },
		chunk, mesh, left, leftMesh, { 0, y + 1, z + 1 }, { 0, y, z });

	// Right Face
	checkFace({ x, y, z + 1 }, { x, y + 1, z }, blockComponent->right, true, internalPos.x == 0,
		internalPos + glm::u16vec3{ -1, 0, 0 }, internalPos + glm::u16vec3{ chunkSize - 1, 0, 0 },
		chunk, mesh, right, rightMesh, { chunkSize - 1, y + 1, z }, { chunkSize - 1, y, z + 1 });
}

void ChunkSystem::checkFace(glm::vec3 p0, glm::vec3 p1, glm::vec4 uvs, bool clockWise,
	bool isOnEdge, glm::u16vec3 internalPos, glm::u16vec3 externalPos, ChunkComponent* chunk, MeshComponent* mesh,
	ChunkComponent* adjacentChunk, MeshComponent* adjacentMesh, glm::vec3 adjacentP0, glm::vec3 adjacentP1) {

	// Remove our face if we're not on the edge and there's a block next to us
	if (!isOnEdge ? chunk->blocks.at(internalPos.x, internalPos.y, internalPos.z) :
		// or if we're at the edge and there either isn't a chunk next to us,
		//  or the chunk next to us has a block next to us
		adjacentChunk == nullptr || adjacentChunk->blocks.at(externalPos.x, externalPos.y, externalPos.z)) {

		// Remove face in chunk next to us
		if (isOnEdge && adjacentChunk != nullptr && !adjacentChunk->justCreated && !adjacentChunk->addedBlocks.count(externalPos))
			adjacentMesh->removeFace(adjacentP0, adjacentP1, !clockWise);
		// Remove face in block next to us (in our chunk)
		else if (!chunk->justCreated && !chunk->addedBlocks.count(internalPos))
			mesh->removeFace(p1, p0, !clockWise);

		// If neither if statement triggers it means we just added this block and the one next to it, so
		//  there's no face to remove, we just won't add it in the first place
		mesh->addFace(p0, p1, uvs, clockWise);
	} else {
		// If there's no block next to us, then add a new face to our geometry
		mesh->addFace(p0, p1, uvs, clockWise);
	}
}
