#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>
#include <filesystem>

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
		sol::table config;

		bool isValid = false;

		double deltaTime = 0;

		DependencyGraph dependencyGraph;

		World(Engine* engine, std::string filename) {
			device = engine->device;

			setupState(engine);

			if (!std::filesystem::exists(filename)) {
				Debugger::addLog(DEBUG_LEVEL_WARN, "[WORLD] Attempted to load world at \"" + filename + "\" but file does not exist.");
				return;
			}

			auto result = lua.script_file(filename);
			if (result.valid()) {
				config = result;
			} else {
				sol::error err = result;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Attempted to load world at \"" + filename + "\" but lua parsing failed with error:\n[LUA] " + std::string(err.what()));
				return;
			}

			isValid = dependencyGraph.init(engine->device, &engine->renderer, &lua, config);
		};

		World(Engine* engine, sol::table worldConfig) {
			device = engine->device;

			setupState(engine);

			isValid = dependencyGraph.init(engine->device, &engine->renderer, &lua, worldConfig);
		};

		uint32_t createEntities(uint32_t amount = 1);
		void deleteEntity(uint32_t entity);

		Archetype* getArchetype(std::unordered_set<std::string> componentTypes, sol::table sharedComponents = sol::table());

		void addQuery(EntityQuery* query);

		void update(double deltaTime);
		void windowRefresh(bool numImagesChanged, int imageCount);

		void cleanup();

	private:
		Device* device;

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
