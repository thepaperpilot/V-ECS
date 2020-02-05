#include "ChunkSystem.h"
#include "ChunkComponent.h"
#include "BlockComponent.h"
#include "../rendering/MeshComponent.h"
#include "../ecs/World.h"

using namespace vecs;

void ChunkSystem::init() {
	// Create our query for all active chunks,
	// plus their mesh components so we can update their meshes
	// whenever blocks are added or removed
	chunks.filter.with(typeid(ChunkComponent), typeid(MeshComponent));
	world->addQuery(chunks);

	// Create our query for all Blocks,
	// so we can add them to the chunks as necessary
	blocks.filter.with(typeid(BlockComponent));
	blocks.onEntityAdded = EntityQuery::bind(this, &ChunkSystem::onBlockAdded);
	blocks.onEntityRemoved = EntityQuery::bind(this, &ChunkSystem::onBlockRemoved);
	world->addQuery(blocks);
}

void ChunkSystem::update() {
	
}
