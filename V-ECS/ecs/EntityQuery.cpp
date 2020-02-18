#include "EntityQuery.h"
#include "World.h"

using namespace vecs;

void ComponentFilter::with(std::type_index component) {
	required.insert(component);
}

void ComponentFilter::with(std::vector<std::type_index> components) {
	required.insert(components.begin(), components.end());
}

void ComponentFilter::without(std::type_index component) {
	disallowed.insert(component);
}

void ComponentFilter::without(std::vector<std::type_index> components) {
	disallowed.insert(components.begin(), components.end());
}

bool ComponentFilter::checkEntity(World* world, uint32_t entity) {
	for (auto const& component_t : required) {
		if (!world->hasComponentType(entity, component_t))
			return false;
	}
	for (auto const& component_t : disallowed)
		if (world->hasComponentType(entity, component_t))
			return false;
	return true;
}

bool ComponentFilter::checkArchetype(std::set<std::type_index> componentTypes) {
	for (auto const& component_t : required) {
		if (componentTypes.find(component_t) == componentTypes.end())
			return false;
	}
	for (auto const& component_t : disallowed)
		if (componentTypes.find(component_t) != componentTypes.end())
			return false;
	return true;
}
