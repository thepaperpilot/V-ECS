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

sol::table Archetype::getComponentList(std::string componentType) {
	return components[componentType];
}

bool Archetype::hasEntity(uint32_t entity) {
	return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

ptrdiff_t Archetype::getIndex(uint32_t entity) {
	return std::distance(this->entities.begin(), std::find(this->entities.begin(), this->entities.end(), entity));
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

// Returns pair of data representing the first entity ID, and index within the archetype
std::pair<uint32_t, size_t> Archetype::createEntities(uint32_t amount) {
	std::vector<uint32_t> entities(amount);
	uint32_t firstEntity = world->createEntities(amount);
	std::iota(entities.begin(), entities.end(), firstEntity);

	size_t firstIndex = this->entities.size();
	this->entities.insert(this->entities.end(), entities.begin(), entities.end());
	size_t newCapacity = this->entities.size();
	for (auto& kvp : components) {
		for (size_t index = firstIndex; index < newCapacity; index++) {
			kvp.second[index + 1] = kvp.second.create();
		}
	}

	return std::make_pair(firstEntity + 1, firstIndex + 1);
}

size_t Archetype::addEntities(std::vector<uint32_t> entities) {
	size_t firstIndex = this->entities.size();
	this->entities.insert(this->entities.end(), entities.begin(), entities.end());
	size_t newCapacity = this->entities.size();
	for (auto kvp : components) {
		for (size_t i = firstIndex; i < newCapacity; i++) {
			kvp.second[i + 1] = kvp.second.create();
		}
	}
	return firstIndex + 1;
}

void Archetype::removeEntities(std::vector<uint32_t> entities) {
	for (uint32_t entity : entities) {
		auto index = getIndex(entity);
		this->entities[index] = this->entities.back();
		this->entities.pop_back();
		for (auto kvp : components) {
			size_t size = kvp.second.size();
			kvp.second[index] = kvp.second[size];
			kvp.second[size] = sol::nil;
		}
	}
}
