#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>

#include "../engine/Engine.h"
#include "../jobs/DependencyGraph.h"
#include "../rendering/Renderer.h"
#include "EntityQuery.h"
#include "Archetype.h"

namespace vecs {

	// The World contains subrenderers and systems
	// and handles updating them as appropriate
	class World {
	public:
		sol::state lua;

		bool isValid = false;

		double deltaTime = 0;

		DependencyGraph dependencyGraph;

		World(Engine* engine, std::string filename) {
			device = engine->device;

			setupState(engine);

			auto result = lua.script_file(filename);
			if (result.valid()) {
				config = result;
			} else {
				sol::error err = result;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
				return;
			}

			dependencyGraph.init(engine->device, &engine->renderer, &lua, config);

			isValid = true;
		};

		World(Engine* engine, sol::table worldConfig) {
			device = engine->device;

			setupState(engine);

			dependencyGraph.init(engine->device, &engine->renderer, &lua, worldConfig);

			isValid = true;
		};

		uint32_t createEntities(uint32_t amount = 1);
		void deleteEntity(uint32_t entity);

		Archetype* getArchetype(std::unordered_set<std::string> componentTypes, sol::table sharedComponents = sol::table());
		Archetype* getArchetype(uint32_t entity);

		Archetype* addComponents(std::vector<uint32_t> entities, std::unordered_set<std::string> components) {
			Archetype* oldArchetype = getArchetype(*entities.begin());
			// Make new list of components
			components.insert(oldArchetype->componentTypes.begin(), oldArchetype->componentTypes.end());
			// Find/create archetype for this set of components
			Archetype* newArchetype = getArchetype(components, oldArchetype->sharedComponents);

			size_t numComponents = components.size();
			std::vector<sol::table> oldComponents(numComponents);
			std::vector<sol::table> newComponents(numComponents);
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
		bool hasComponentType(uint32_t entity, std::string component_t);
		template <class Component>
		Archetype* removeComponents(std::vector<uint32_t> entities, std::unordered_set<std::string> components) {
			Archetype* oldArchetype = getArchetype(*entities.begin());
			// Make new list of components
			std::unordered_set<std::string> newComponents;
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

		void addQuery(EntityQuery* query);

		void update(double deltaTime);
		void windowRefresh(bool numImagesChanged, int imageCount);

		void cleanup();

	private:
		Device* device;

		sol::table config;

		// Start at 1 so that 0 can be used to represent an invalid entity
		uint32_t nextEntity = 1;
		
		// Store a list of filters added by our systems. Each tracks which entities meet a specific
		// criteria of components it needs and/or disallows, and contains pointers for functions
		// to run whenever an entity is added to or removed from the filtered entity list
		std::vector<EntityQuery*> queries;

		// Each unique set of components is managed by an archetype
		std::vector<Archetype*> archetypes;

		std::vector<Buffer> buffers;

		void setupState(Engine* engine);
	};
}
