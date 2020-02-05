#pragma once

#include <typeindex>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>
#include <functional>

#include "EntityQuery.h"
#include "System.h"

namespace vecs {

	struct Component {};

	// The World contains all of the program's Systems
	// and handles updating each system as appropriate
	class World {
		friend bool ComponentFilter::checkEntity(World* world, uint32_t entity);
	public:
		std::set<uint32_t> entities;

		uint32_t createEntity();
		void deleteEntity(uint32_t entity);

		// I don't know enough about c++ to figure out why,
		// but I can't define templated functions in a separate .cpp file
		template <class Component>
		void addComponent(uint32_t entity, Component* component) {
			// Mark entity dirty since it has had a structural change
			dirtyEntities.insert(entity);
			// Add component as this entity's value for this component type
			components[typeid(Component)][entity] = component;
		};
		bool hasComponentType(uint32_t entity, std::type_index component_t);
		// This function will create a component with default values
		// if it didn't exist already
		template <class Component>
		Component* getComponent(uint32_t entity) {
			// If it doesn't already have a value, the map will create one
			// automatically on access. If that happens, our entity will
			// have a structural change. Therefore we need to check if
			// it has that component already, so we can mark it dirty if necessary
			if (!hasComponentType(entity, typeid(Component)))
				dirtyEntities.insert(entity);
			return components[typeif(Component)][entity];
		}
		template <class Component>
		void removeComponent(uint32_t entity) {
			// Mark entity dirty since it has had a structural change
			dirtyEntities.insert(entity);
			// Remove component value for this entity
			components[typeid(Component)].erase(entity);
		}
		// Reserves an additional n components of the specified type
		// that way we can add components without needing to rehash
		// multiple times
		template <class Component>
		void reserveComponents(int n) {
			std::unordered_map<uint32_t, Component*> componentList = components[typeid(Component)];
			componentList.reserve(componentList.size() + n);
		}

		void addSystem(System* system, int priority);
		void addQuery(EntityQuery query);

		// Optional function for child classes to setup anything they need to
		// whenever setting the world up
		virtual void init() {
			std::cout << "init NOT overriden" << std::endl;
		};
		void update();
		// Optional function for child classes to cleanup anything they need to
		// This won't get called automatically by the Engine because Worlds may
		// be switched around and re-used 
		virtual void cleanup() {};

	private:
		uint32_t nextEntity;
		// We use a map for the systems because it can sort on insert very quickly
		// and allow us to run our systems in order by "priority"
		std::multimap<int, System*> systems;
		// This set tracks any entities with structural changes,
		// so that listeners can be triggered in the next update() call
		// This way structural changes can be batched for each frame
		std::set<uint32_t> dirtyEntities;
		// The type index of the component struct (which is guaranteed unique) gets mapped
		// to a map of entities to their instances of those components
		// This should have a decent performance improvement due to data locality
		std::unordered_map<std::type_index, std::unordered_map<uint32_t, Component*>> components;
		// Store a list of filters added by our systems. Each tracks which entities meet a specific
		// criteria of components it needs and/or disallows, and contains pointers for functions
		// to run whenever an entity is added to or removed from the filtered entity list
		std::vector<EntityQuery> queries;
	};
}
