#include "Archetype.h"

#include "World.h"

using namespace vecs;

std::vector<Component*>* Archetype::getComponentList(std::type_index componentType) {
	return components[componentType];
}

bool Archetype::hasEntity(uint32_t entity) {
	return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

ptrdiff_t Archetype::getIndex(uint32_t entity) {
	return std::distance(this->entities.begin(), std::find(this->entities.begin(), this->entities.end(), entity));
}

// Returns pair of data representing the first entity ID, and index within the archetype
std::pair<uint32_t, size_t> Archetype::createEntities(uint32_t amount) {
	std::vector<uint32_t> entities(amount);
	uint32_t firstEntity = world->createEntities(amount);
	std::iota(entities.begin(), entities.end(), firstEntity);

	size_t firstIndex = this->entities.size();
	this->entities.insert(this->entities.end(), entities.begin(), entities.end());
	size_t newCapacity = this->entities.size();
	for (auto kvp : components) {
		kvp.second->resize(newCapacity);
	}

	return std::make_pair(firstEntity, firstIndex);
}

size_t Archetype::addEntities(std::vector<uint32_t> entities) {
	size_t firstIndex = this->entities.size();
	this->entities.insert(this->entities.end(), entities.begin(), entities.end());
	size_t newCapacity = this->entities.size();
	for (auto kvp : components) {
		kvp.second->resize(newCapacity);
	}
	return firstIndex;
}

void Archetype::removeEntities(std::vector<uint32_t> entities) {
	for (uint32_t entity : entities) {
		auto index = getIndex(entity);
		this->entities[index] = this->entities.back();
		this->entities.pop_back();
		for (auto kvp : components) {
			kvp.second->at(index) = kvp.second->back();
			kvp.second->pop_back();
		}
	}
}

void Archetype::cleanup(VkDevice* device) {
	for (auto kvp : components) {
		for (auto component : *kvp.second)
			component->cleanup(device);
	}
}