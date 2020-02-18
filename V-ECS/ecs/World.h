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

namespace vecs {

	// Forward Declarations
	class ArchetypeBuilder;

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
			dirtyEntities = new std::unordered_set<uint32_t>;
			addSystem(&startRenderingSystem, START_RENDERING_PRIORITY);
		};

		uint32_t createEntity();
		void deleteEntity(uint32_t entity);

		template <class Component>
		void addComponent(uint32_t entity, Component* component) {
			// Mark entity dirty since it has had a structural change
			dirtyEntities->insert(entity);
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
				dirtyEntities->insert(entity);
			return static_cast<Component*>(components[typeid(Component)][entity]);
		}
		template <class Component>
		void removeComponent(uint32_t entity) {
			// Mark entity dirty since it has had a structural change
			dirtyEntities->insert(entity);
			// Remove component value for this entity
			components[typeid(Component)][entity]->cleanup(device);
			components[typeid(Component)].erase(entity);
		}
		// Reserve the next n entities. Doesn't ensure you get the next
		// consecutive n entities, just prepares dirtyEntities to accept
		// n more entities more efficiently
		void reserveEntities(int n) {
			dirtyEntities->reserve(dirtyEntities->size() + n);
		}
		// Used to cache specific component lists so systems don't need
		// to perform as many lookups when iterating over many entities
		template <class C>
		std::map<uint32_t, Component*>* getComponentList() {
			return &components[typeid(C)];
		};

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
		uint32_t nextEntity = 0;

		StartRenderingSystem startRenderingSystem;

		// We use a map for the systems because it can sort on insert very quickly
		// and allow us to run our systems in order by "priority"
		std::multimap<int, System*> systems;
		
		// This set tracks any entities with structural changes,
		// so that listeners can be triggered in the next update() call
		// This way structural changes can be batched for each frame.
		std::unordered_set<uint32_t>* dirtyEntities;
		
		// The type index of the component struct (which is guaranteed unique) gets mapped
		// to a map of entities to their instances of those components
		// This should have a decent performance improvement due to data locality
		std::unordered_map<std::type_index, std::map<uint32_t, Component*>> components;
		
		// Store a list of filters added by our systems. Each tracks which entities meet a specific
		// criteria of components it needs and/or disallows, and contains pointers for functions
		// to run whenever an entity is added to or removed from the filtered entity list
		std::vector<EntityQuery*> queries;
	};
}
