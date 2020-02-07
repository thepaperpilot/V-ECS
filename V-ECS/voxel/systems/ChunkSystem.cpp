#include "ChunkSystem.h"
#include "../components/ChunkComponent.h"
#include "../components/BlockComponent.h"
#include "../../rendering/MeshComponent.h"
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
}

void ChunkSystem::update() {}

void ChunkSystem::onBlockAdded(uint32_t entity) {
	// TODO determine which chunk it belongs in
	uint32_t chunk = *chunks.entities.begin();

	// Add the entity to that chunk's blocks list
	world->getComponent<ChunkComponent>(chunk)->blocks.emplace(entity);

	// Update the mesh component with the new indices and any new vertices
	MeshComponent* mesh = world->getComponent<MeshComponent>(chunk);
	BlockComponent* block = world->getComponent<BlockComponent>(entity);
	// 2 triangles means 6 points
	// Some vertices are shared, but addVertex will optimize that for us :)
	Vertex v0{ {block->x, block->y}, {0.0f, 1.0f, 0.0f} };
	Vertex v1{ {block->x + 1, block->y}, {1.0f, 0.0f, 0.0f} };
	Vertex v2{ {block->x + 1, block->y + 1}, {1.0f, 1.0f, 1.0f} };
	Vertex v3{ {block->x, block->y + 1}, {0.0f, 0.0f, 1.0f} };
	mesh->addVertex(v0);
	mesh->addVertex(v1);
	mesh->addVertex(v2);
	mesh->addVertex(v2);
	mesh->addVertex(v3);
	mesh->addVertex(v0);
	// TODO add other half of the vertices once we add 3D rendering
}

void ChunkSystem::onBlockRemoved(uint32_t entity) {
	// TODO determine which chunk it belongs in
	uint32_t chunk = *chunks.entities.begin();

	// Add the entity to that chunk's blocks list
	world->getComponent<ChunkComponent>(chunk)->blocks.erase(entity);

	// Update the mesh component to remove these indices
	MeshComponent* mesh = world->getComponent<MeshComponent>(chunk);
	BlockComponent* block = world->getComponent<BlockComponent>(entity);
	mesh->removeVertices(std::vector<Vertex> {
		{ {block->x, block->y}, { 1.0f, 0.0f, 0.0f } },
		{ {block->x + 1, block->y}, { 1.0f, 0.0f, 0.0f } },
		{ {block->x + 1, block->y + 1}, { 1.0f, 0.0f, 0.0f } },
		{ {block->x, block->y + 1}, { 1.0f, 0.0f, 0.0f } }
	});
}
