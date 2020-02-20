#pragma once

#include <vulkan/vulkan.h>
#include <typeindex>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>

#include "../rendering/Renderer.h"
#include "EntityQuery.h"
#include "System.h"
#include "Archetype.h"

namespace vecs {

	struct Component {
		// Optional function to cleanup any necessary fields a component may have
		virtual void cleanup(VkDevice* device) {}
		// TODO serialize and deserialize functions
	};

	struct StartRenderingSystem : public System {
		Renderer* renderer;

		StartRenderingSystem(Renderer* renderer) { this->renderer = renderer; }

		virtual void update() override { renderer->acquireImage(); }
	};

	// The World contains all of the program's Systems
	// and handles updating each system as appropriate
	// TODO other ECS implementations have a class for each component
	// type that handles storing data for that component (e.g. SOA instead of AOS).
	class World {
		friend bool ComponentFilter::checkEntity(World* world, uint32_t entity);
		friend class ArchetypeBuilder;
	public:
		double deltaTime = 0;
		bool cancelUpdate = false;

		World() : startRenderingSystem(&renderer) {
			addSystem(&startRenderingSystem, START_RENDERING_PRIORITY);
		};

		uint32_t createEntities(uint32_t amount = 1);
		void deleteEntity(uint32_t entity);

		Archetype* getArchetype(std::unordered_set<std::type_index> componentTypes, std::unordered_map<std::type_index, Component*>* sharedComponents = nullptr);
		Archetype* getArchetype(uint32_t entity);

		Archetype* addComponents(uint32_t entity, std::unordered_set<std::type_index> components) {
			Archetype* oldArchetype = getArchetype(entity);
			ptrdiff_t oldIndex = oldArchetype->getIndex(entity);

			// Make new list of components
			components.insert(oldArchetype->componentTypes.begin(), oldArchetype->componentTypes.end());
			// Find/create archetype for this set of components
			Archetype* newArchetype = getArchetype(components, &oldArchetype->sharedComponents);
			// Add entity to new archetype
			size_t newIndex = newArchetype->addEntities({ entity });

			// Copy components over to new archetype
			for (auto component : oldArchetype->componentTypes) {
				// How slow is this?
				// Concerned about performance when needing to add components to many entities at once
				newArchetype->getComponentList(component)[newIndex] = oldArchetype->getComponentList(component)[oldIndex];
			}

			// Remove entity from old archetype
			oldArchetype->removeEntities({ entity });

			// Return new archetype so it can be used to set the values of the new components
			return newArchetype;
		};
		bool hasComponentType(uint32_t entity, std::type_index component_t);
		template <class Component>
		Archetype* removeComponents(uint32_t entity, std::unordered_set<std::type_index> components) {
			Archetype* oldArchetype = getArchetype(entity);
			ptrdiff_t oldIndex = oldArchetype->getIndex(entity);

			// Make new list of components
			std::unordered_set<std::type_index> newComponents;
			std::copy_if(oldArchetype->componentTypes.begin(), oldArchetype->componentTypes.end(), std::back_inserter(newComponents),
				[&components](int needle) { return !components.count(needle); });
			// Find/create archetype for this set of components
			Archetype* newArchetype = getArchetype(newComponents, &oldArchetype->sharedComponents);
			// Add entity to new archetype
			size_t newIndex = newArchetype->addEntities({ entity });

			// Copy components over to new archetype
			for (auto component : oldArchetype->componentTypes) {
				if (!components.count(component))
					newArchetype->getComponentList(component)[newIndex] = oldArchetype->getComponentList(component)[oldIndex];
			}

			// Remove entity from old archetype
			oldArchetype->removeEntities({ entity });

			// Return new archetype
			return newArchetype;
		}

		void addSystem(System* system, int priority);
		void addQuery(EntityQuery* query);

		void init(Device* device, VkSurfaceKHR surface, GLFWwindow* window) {
			this->device = device;
			this->window = window;

			init();

			this->renderer.init(device, surface, window);
		};

		// Optional function for child classes to setup anything they need to
		// whenever setting the world up
		virtual void init() {};
		void update(double deltaTime);
		void cleanup();

	protected:
		Device* device;
		GLFWwindow* window;

		Renderer renderer;

		// Optional function for child classes to cleanup anything they need to
		// This won't get called automatically by the Engine because Worlds may
		// be switched around and re-used 
		virtual void cleanupSystems() {};

	private:
		// Start at 1 so that 0 can be used to represent an invalid entity
		uint32_t nextEntity = 1;

		StartRenderingSystem startRenderingSystem;

		// We use a map for the systems because it can sort on insert very quickly
		// and allow us to run our systems in order by "priority"
		std::multimap<int, System*> systems;
		
		// Store a list of filters added by our systems. Each tracks which entities meet a specific
		// criteria of components it needs and/or disallows, and contains pointers for functions
		// to run whenever an entity is added to or removed from the filtered entity list
		std::vector<EntityQuery*> queries;

		// Each unique set of components is managed by an archetype
		std::vector<Archetype*> archetypes;
	};
}
