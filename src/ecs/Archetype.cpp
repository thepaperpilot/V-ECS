#include "Archetype.h"

#include "World.h"
#include "EntityQuery.h"
#include "../lua/LuaVal.h"

using namespace vecs;

Archetype::Archetype(World* world, std::unordered_set<std::string> componentTypes, LuaVal* sharedComponents) {
	this->world = world;
	this->componentTypes = componentTypes;

	if (sharedComponents != nullptr) {
		assert(sharedComponents->type == LUA_TYPE_TABLE);
		this->sharedComponents = new LuaVal(std::get<LuaVal::MapType*>(sharedComponents->value));
	} else this->sharedComponents = nullptr;

	for (auto component : componentTypes) {
		components[component] = LuaVal({});
	}
}

LuaVal Archetype::getSharedComponent(std::string componentType) {
	if (sharedComponents == nullptr) return LuaVal();
	return sharedComponents->get(componentType);
}

LuaVal Archetype::getComponentList(std::string componentType) {
	return components[componentType];
}

// TODO do I need to lock_shared mutex when calling find or contains?
bool Archetype::checkQuery(EntityQuery* query) {
	for (std::string component_t : query->filter.required) {
		if (componentTypes.find(component_t) == componentTypes.end())
			return false;
	}
	for (std::string component_t : query->filter.disallowed)
		if (componentTypes.find(component_t) != componentTypes.end())
			return false;
	if (sharedComponents == nullptr && query->sharedFilter.required.size() > 0) return false;
	for (std::string component_t : query->sharedFilter.required) {
		if (!sharedComponents->contains(component_t))
			return false;
	}
	if (sharedComponents == nullptr) return true;
	for (std::string component_t : query->sharedFilter.disallowed)
		if (sharedComponents->contains(component_t))
			return false;
	return true;
}

std::pair<uint32_t, uint32_t> Archetype::createEntities(uint32_t amount) {
	uint32_t firstEntity = world->createEntities(amount);
	uint32_t lastEntity = firstEntity + amount - 1;
	uint32_t index = numEntities.fetch_add(amount);

	mutex.lock();
	for (auto& kvp : components) {
		for (uint32_t index = firstEntity; index <= lastEntity; index++) {
			kvp.second.set(LuaVal((double)index), LuaVal({}));
		}
	}
	for (uint32_t index = firstEntity; index <= lastEntity; index++) {
		entities.insert(index);
	}
	mutex.unlock();
	return std::make_pair(firstEntity, index);
}

void Archetype::addEntities(std::vector<uint32_t> entities) {
	mutex.lock();
	for (auto kvp : components) {
		for (uint32_t index : entities)
			kvp.second.set(LuaVal((double)index), LuaVal({}));
	}
	for (uint32_t index : entities)
		this->entities.insert(index);
	mutex.unlock();
	numEntities += entities.size();
}

void Archetype::removeEntities(std::vector<uint32_t> entities) {
	mutex.lock();
	for (uint32_t entity : entities) {
		for (auto kvp : components) {
			kvp.second.set_nil(LuaVal((double)entity));
		}
	}
	for (uint32_t entity : entities) {
		this->entities.erase(entity);
	}
	mutex.unlock();
	numEntities -= entities.size();
}

void Archetype::clearEntities() {
	mutex.lock();
	for (auto kvp : components) {
		kvp.second.clear();
	}
	numEntities = 0;
	entities.clear();
	mutex.unlock();
}

void Archetype::lock_shared() {
	mutex.lock_shared();
}

void Archetype::unlock_shared() {
	mutex.unlock_shared();
}
