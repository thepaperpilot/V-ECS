#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <set>
#include <unordered_map>
#include <iostream>
#include <filesystem>

#include "../engine/Engine.h"
#include "../jobs/DependencyGraph.h"
#include "../jobs/ThreadResources.h"
#include "../rendering/Renderer.h"
#include "EntityQuery.h"
#include "Archetype.h"
#include "WorldLoadStatus.h"

namespace vecs {

	// The World contains subrenderers and systems
	// and handles updating them as appropriate
	class World {
	public:
		sol::state lua;
		sol::table config;

		bool isValid = false;
		bool isDisposed = false;
		WorldLoadStatus* status;

		ImFontAtlas* fonts = nullptr;

		double deltaTime = 0;

		DependencyGraph dependencyGraph;

		World(Engine* engine, std::string filename, WorldLoadStatus* status) : resources(engine->device, 1 + engine->nextQueueIndex) {
			device = engine->device;
			this->status = status;
			engine->nextQueueIndex = !engine->nextQueueIndex;

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

			isValid = dependencyGraph.init(engine->device, &engine->renderer, &resources, &lua, config, status);
			status->currentStep = WORLD_LOAD_STEP_FINISHED;
			if (status->isCancelled) cleanup();
		};

		World(Engine* engine, sol::table worldConfig, WorldLoadStatus* status) : resources(engine->device, 1 + engine->nextQueueIndex) {
			device = engine->device;
			this->status = status;
			engine->nextQueueIndex = !engine->nextQueueIndex;

			setupState(engine);

			isValid = dependencyGraph.init(engine->device, &engine->renderer, &resources, &lua, worldConfig, status);
			status->currentStep = WORLD_LOAD_STEP_FINISHED;
			if (status->isCancelled) cleanup();
		};

		uint32_t createEntities(uint32_t amount = 1);

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

		// Contains resources meant to be per-thread
		// Later this will be split amongst different worker threads
		// But for now each world just gets its own thread and corresponding resources
		ThreadResources resources;

		void setupState(Engine* engine);
	};
}
