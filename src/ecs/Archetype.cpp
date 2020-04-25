#include "Archetype.h"

#include "World.h"
#include "EntityQuery.h"

using namespace vecs;

Archetype::Archetype(World* world, std::unordered_set<std::string> componentTypes, sol::table sharedComponents) {
	this->world = world;
	this->componentTypes = componentTypes;
	this->sharedComponents = sharedComponents;

	for (auto component : componentTypes) {
		components[component] = world->lua.create_table();
	}
}

sol::table Archetype::getSharedComponent(std::string componentType) {
	return sharedComponents[componentType];
}

sol::table Archetype::getComponentList(std::string componentType) {
	return components[componentType];
}

bool Archetype::checkQuery(EntityQuery* query) {
	for (std::string component_t : query->filter.required) {
		if (componentTypes.find(component_t) == componentTypes.end())
			return false;
	}
	for (std::string component_t : query->filter.disallowed)
		if (componentTypes.find(component_t) != componentTypes.end())
			return false;
	for (std::string component_t : query->sharedFilter.required) {
		if (sharedComponents[component_t].get_type() == sol::type::nil)
			return false;
	}
	for (std::string component_t : query->sharedFilter.disallowed)
		if (sharedComponents[component_t].get_type() != sol::type::nil)
			return false;
	return true;
}

std::pair<uint32_t, uint32_t> Archetype::createEntities(uint32_t amount) {
	uint32_t firstEntity = world->createEntities(amount);
	uint32_t lastEntity = firstEntity + amount - 1;

	for (auto& kvp : components) {
		for (uint32_t index = firstEntity; index <= lastEntity; index++) {
			kvp.second[index] = kvp.second.create();
		}
	}

	return std::make_pair(firstEntity, lastEntity);
}

void Archetype::addEntities(std::vector<uint32_t> entities) {
	for (auto kvp : components) {
		for (uint32_t index : entities)
			kvp.second[index] = kvp.second.create();
	}
}

void Archetype::removeEntities(std::vector<uint32_t> entities) {
	for (uint32_t entity : entities) {
		for (auto kvp : components) {
			kvp.second[entity] = sol::nil;
		}
	}
}
