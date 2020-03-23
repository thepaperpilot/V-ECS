#pragma once

#include <vulkan/vulkan.h>
#include <typeindex>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>

#include "../rendering/Renderer.h"
#include "../rendering/SubRenderer.h"
#include "EntityQuery.h"
#include "System.h"
#include "Archetype.h"

namespace vecs {

	struct Component {
		// Optional function to cleanup any necessary fields a component may have
		virtual void cleanup(VkDevice* device) {}
		// TODO serialize and deserialize functions
	};

	// Forward declare World because we're just about to define it
	class World;
	struct StartRenderingSystem : public System {
		Renderer* renderer;
		World* world;

		StartRenderingSystem(Renderer* renderer, World* world) {
			this->renderer = renderer;
			this->world = world;
		}

		virtual void update() override;
	};

	// The World contains all of the program's Systems
	// and handles updating each system as appropriate
	// TODO other ECS implementations have a class for each component
	// type that handles storing data for that component (e.g. SOA instead of AOS).
	class World {
	public:
		Renderer* renderer;

		double deltaTime = 0;
		bool cancelUpdate = false;
		bool activeWorld = false;

		std::multiset<SubRenderer*, SubRendererCompare> subrenderers;

		World(Renderer* renderer) : startRenderingSystem(renderer, this) {
			this->renderer = renderer;
			addSystem(&startRenderingSystem, START_RENDERING_PRIORITY);
		};

		uint32_t createEntities(uint32_t amount = 1);
		void deleteEntity(uint32_t entity);

		Archetype* getArchetype(std::unordered_set<std::type_index> componentTypes, std::unordered_map<std::type_index, Component*>* sharedComponents = nullptr);
		Archetype* getArchetype(uint32_t entity);

		Archetype* addComponents(std::vector<uint32_t> entities, std::unordered_set<std::type_index> components) {
			Archetype* oldArchetype = getArchetype(*entities.begin());
			// Make new list of components
			components.insert(oldArchetype->componentTypes.begin(), oldArchetype->componentTypes.end());
			// Find/create archetype for this set of components
			Archetype* newArchetype = getArchetype(components, oldArchetype->sharedComponents);

			size_t numComponents = components.size();
			std::vector<std::vector<Component*>*> oldComponents(numComponents);
			std::vector<std::vector<Component*>*> newComponents(numComponents);
			uint16_t i = 0;
			for (auto type : oldArchetype->componentTypes) {
				oldComponents[i] = oldArchetype->getComponentList(type);
				newComponents[i] = newArchetype->getComponentList(type);
				i++;
			}

			// Add entities to new archetype
			size_t newIndex = newArchetype->addEntities(entities);

			// Copy components over to new archetype
			for (uint32_t entity : entities) {
				ptrdiff_t oldIndex = oldArchetype->getIndex(entity);

				for (size_t i = 0; i < numComponents; i++) {
					newComponents[i][newIndex] = oldComponents[i][oldIndex];
				}

				newIndex++;
			}

			// Remove entities from old archetype
			oldArchetype->removeEntities(entities);

			// Return new archetype so it can be used to set the values of the new components
			return newArchetype;
		};
		bool hasComponentType(uint32_t entity, std::type_index component_t);
		template <class Component>
		Archetype* removeComponents(std::vector<uint32_t> entities, std::unordered_set<std::type_index> components) {
			Archetype* oldArchetype = getArchetype(*entities.begin());
			// Make new list of components
			std::unordered_set<std::type_index> newComponents;
			std::copy_if(oldArchetype->componentTypes.begin(), oldArchetype->componentTypes.end(), std::back_inserter(newComponents),
				[&components](int needle) { return !components.count(needle); });
			// Find/create archetype for this set of components
			Archetype* newArchetype = getArchetype(newComponents, oldArchetype->sharedComponents);
			
			uint16_t numComponents = newComponents.size();
			std::vector<std::vector<Component*>*> oldComponents(numComponents);
			std::vector<std::vector<Component*>*> newComponents(numComponents);
			uint16_t i = 0;
			for (auto type : components) {
				oldComponents[i] = oldArchetype->getComponentList(type);
				newComponents[i] = newArchetype->getComponentList(type);
				i++;
			}

			// Add entities to new archetype
			size_t newIndex = newArchetype->addEntities(entities);

			// Copy components over to new archetype
			for (uint32_t entity : entities) {
				ptrdiff_t oldIndex = oldArchetype->getIndex(entity);

				for (auto component : components) {
					newComponents[newIndex] = oldComponents[oldIndex];
				}

				newIndex++;
			}

			// Remove entities from old archetype
			oldArchetype->removeEntities(entities);

			// Return new archetype
			return newArchetype;
		}

		void addSystem(System* system, int priority);
		void addQuery(EntityQuery* query);

		void init(Device* device, GLFWwindow* window) {
			this->device = device;
			this->window = window;

			preInit();

			// Initialize our sub-renderers
			for (auto subrenderer : subrenderers)
				subrenderer->init(device, renderer);
		}

		// Optional function for child classes to setup anything they need to
		// whenever setting the world up
		virtual void preInit() {};
		void update(double deltaTime);
		void cleanup();

	protected:
		Device* device;
		GLFWwindow* window;

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
