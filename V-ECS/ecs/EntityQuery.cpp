#include "EntityQuery.h"
#include "World.h"

using namespace vecs;

EntityQuery::EntityQuery(std::type_index components ...) {
	with(components);
}

void EntityQuery::with(std::type_index components ...) {
	required.insert(components);
}

void EntityQuery::without(std::type_index components ...) {
	disallowed.insert(components);
}

bool EntityQuery::checkEntity(World* world, uint32_t entity) {
	for (auto const& component_t : required) {
		if (!world->hasComponentType(entity, component_t))
			return false;
	}
	for (auto const& component_t : disallowed)
		if (world->hasComponentType(entity, component_t))
			return false;
	return true;
}
