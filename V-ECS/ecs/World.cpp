#include "World.h"
#include "System.h"

using namespace vecs;

uint32_t World::createEntity() {
	dirtyEntities.insert(nextEntity);
	// TODO handle overflow
	return nextEntity++;
}

void World::deleteEntity(uint32_t entity) {
	dirtyEntities.insert(entity);
	// Iterate over each type of component
	for (auto& kvp : components) {
		// Attempt to erase the entity from the list of component values
		kvp.second.erase(entity);
	}
}

bool World::hasComponentType(uint32_t entity, std::type_index component_t) {
	// count will return 1 iff entity has a value for this component
	return components[component_t].count(entity);
}

void World::addSystem(System* system, int priority) {
	system->world = this;
	// Insert our system with a given priority
	systems.insert(std::pair<int, System*>(priority, system));
}

void World::addFilter(ComponentFilter filter) {
	filters.push_back(filter);
}

void World::update() {
	// Copy our list of dirty entities so that we can clear it before handling the dirty entities
	// That's because, whilst handling our dirty entities, a system may dirty more
	// and we want to make sure we don't to erase the fact it was dirtied
	std::set<uint32_t> entitiesToClean = dirtyEntities;
	dirtyEntities.clear();
	for (auto const& entity : entitiesToClean) {
		for (auto filter : filters) {
			if (filter.entities.count(entity)) {
				// Check if its been removed
				if (!filter.query.checkEntity(this, entity)) {
					filter.onEntityRemoved(entity);
				}
			} else {
				// Check if its been added
				if (!filter.query.checkEntity(this, entity)) {
					filter.onEntityAdded(entity);
				}
			}
		}
	}

	// Update each system in priority-order
	for (auto& kvp : systems) {
		System* system = kvp.second;
		system->update();
	}
}
