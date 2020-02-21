#include "World.h"
#include "System.h"

using namespace vecs;

uint32_t World::createEntities(uint32_t amount) {
	uint32_t firstEntity = nextEntity;
	nextEntity += amount;
	return firstEntity;
}

void World::deleteEntity(uint32_t entity) {
	for (auto archetype : archetypes) {
		if (archetype->hasEntity(entity)) {
			archetype->removeEntities({ entity });
			return;
		}			
	}
}

Archetype* World::getArchetype(std::unordered_set<std::type_index> componentTypes, std::unordered_map<std::type_index, Component*>* sharedComponents) {
	auto itr = std::find_if(archetypes.begin(), archetypes.end(), [&componentTypes, &sharedComponents](Archetype* archetype) {
		return archetype->componentTypes == componentTypes && (sharedComponents == nullptr || &archetype->sharedComponents == sharedComponents);
	});

	if (itr == archetypes.end()) {
		// Create a new archetype
		Archetype* archetype = archetypes.emplace_back(new Archetype(this, componentTypes, sharedComponents));

		// Add it to any entity queries it matches
		for (auto query : queries) {
			if (query->filter.checkArchetype(componentTypes))
				query->matchingArchetypes.push_back(archetype);
		}

		return archetype;
	}

	return *itr;
}

Archetype* World::getArchetype(uint32_t entity) {
	for (auto archetype : archetypes) {
		if (archetype->hasEntity(entity)) {
			return archetype;
		}
	}
}

bool World::hasComponentType(uint32_t entity, std::type_index component_t) {
	Archetype* archetype = getArchetype(entity);
	return archetype->componentTypes.count(component_t);
}

void World::addSystem(System* system, int priority) {
	system->world = this;
	system->init();
	// Insert our system with a given priority
	systems.insert(std::pair<int, System*>(priority, system));
}

void World::addQuery(EntityQuery* query) {
	queries.push_back(query);

	// Check what archetypes match this query
	for (auto archetype : archetypes) {
		if (query->filter.checkArchetype(archetype->componentTypes))
			query->matchingArchetypes.push_back(archetype);
	}
}

void World::update(double deltaTime) {
	this->deltaTime = deltaTime;

	// Update each system in priority-order
	for (auto& kvp : systems) {
		System* system = kvp.second;
		system->update();
		if (cancelUpdate) break;
	}
	cancelUpdate = false;

	if (activeWorld)
		renderer.presentImage();
}

void World::cleanup() {
	renderer.cleanup();
	cleanupSystems();

	for (auto archetype : archetypes)
		archetype->cleanup(&device->logical);
}

void StartRenderingSystem::update() { if (world->activeWorld) renderer->acquireImage(); }
