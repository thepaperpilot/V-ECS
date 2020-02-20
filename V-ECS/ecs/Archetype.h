#pragma once

#include <unordered_map>
#include <typeindex>
#include <array>
#include <numeric>
#include <unordered_set>

#include <vulkan/vulkan.h>

namespace vecs {

	// Forward Declarations
	class World;
	struct Component;
	
	class Archetype {
	public:
		std::unordered_set<std::type_index> componentTypes;

		std::unordered_map<std::type_index, Component*> sharedComponents;

		Archetype(World* world, std::unordered_set<std::type_index> componentTypes, std::unordered_map<std::type_index, Component*>* sharedComponents = nullptr) {
			this->world = world;
			this->componentTypes = componentTypes;
			if (sharedComponents != nullptr)
				this->sharedComponents = *sharedComponents;

			for (auto component : componentTypes) {
				components[component] = new std::vector<Component*>;
			}
		}

		template <class C>
		C* getComponent(size_t index) {
			return static_cast<C*>(components[typeid(C)]->at(index));
		}

		template <class C>
		C* getSharedComponent() {
			return static_cast<C>(sharedComponents[typeid(C)]);
		}

		std::vector<Component*>* getComponentList(std::type_index componentType);

		bool hasEntity(uint32_t entity);

		ptrdiff_t getIndex(uint32_t entity);

		// Returns pair of data representing the first entity ID, and index within the archetype
		std::pair<uint32_t, size_t> createEntities(uint32_t amount);

		size_t addEntities(std::vector<uint32_t> entities);

		void removeEntities(std::vector<uint32_t> entities);

		void cleanup(VkDevice* device);

	private:
		World* world;

		std::vector<uint32_t> entities;

		std::unordered_map<std::type_index, std::vector<Component*>*> components;
	};
}
