#pragma once

#include <vulkan/vulkan.h>
#include <imnodes.h>
#include <unordered_set>

#include "../engine/Buffer.h"
#include "../events/GLFWEvents.h"
#include "../jobs/DependencyGraph.h"
#include "../jobs/Worker.h"

// Forward Declarations
struct ImFontAtlas;

namespace vecs {

	// Forward Declarations
	class Archetype;
	class Engine;
	class EntityQuery;
	class WorldLoadStatus;
	class LuaVal;

	// The World contains subrenderers and systems
	// and handles updating them as appropriate
	class World {
	public:
		sol::table config;

		bool isValid = false;
		bool isDisposed = false;
		WorldLoadStatus* status;

		ImFontAtlas* fonts = nullptr;

		// imnodes context
		imnodes::EditorContext* nodeEditorContext;

		// Start at 1 so that 0 can be used to represent an invalid entity
		std::atomic_uint32_t nextEntity = 1;

		double deltaTime = 0;

		// Each world has its own Worker that won't actually be ran, but exists for lua scripts to be
		// able to access Vulkan resources and to have a job queue to add to, which will be stolen out of
		Worker worker;

		DependencyGraph dependencyGraph;

		World(Engine* engine, std::string filename, WorldLoadStatus* status, bool waitUntilLoaded = false);
		World(Engine* engine, sol::table worldConfig, WorldLoadStatus* status, bool waitUntilLoaded = false);

		uint32_t createEntities(uint32_t amount = 1);
		uint32_t createEntity(LuaVal* components);

		Archetype* getArchetype(std::unordered_set<std::string> componentTypes, LuaVal* sharedComponents = nullptr);

		void addQuery(EntityQuery* query);

		void update(double deltaTime);
		void windowRefresh(int imageCount);

		void addBuffer(Buffer buffer);

		void cleanup();

	private:
		Device* device;
		
		// Store a list of filters added by our systems. Each tracks which entities meet a specific
		// criteria of components it needs and/or disallows, and contains pointers for functions
		// to run whenever an entity is added to or removed from the filtered entity list
		std::vector<EntityQuery*> queries;

		// Each unique set of components is managed by an archetype
		std::vector<Archetype*> archetypes;
		std::shared_mutex archetypesMutex;

		std::vector<Buffer> buffers;
		std::mutex buffersMutex;

		// Reference to several archetypes for GLFW events
		Archetype* mouseMoveEventArchetype;
		Archetype* leftMousePressEventArchetype;
		Archetype* leftMouseReleaseEventArchetype;
		Archetype* rightMousePressEventArchetype;
		Archetype* rightMouseReleaseEventArchetype;
		Archetype* horizontalScrollEventArchetype;
		Archetype* verticalScrollEventArchetype;
		Archetype* keyPressEventArchetype;
		Archetype* keyReleaseEventArchetype;
		Archetype* windowResizeEventArchetype;

		void setupEvents();
		bool mouseMoveEventCallback(MouseMoveEvent* event);
		bool leftMousePressEventCallback(LeftMousePressEvent* event);
		bool leftMouseReleaseEventCallback(LeftMouseReleaseEvent* event);
		bool rightMousePressEventCallback(RightMousePressEvent* event);
		bool rightMouseReleaseEventCallback(RightMouseReleaseEvent* event);
		bool horizontalScrollEventCallback(HorizontalScrollEvent* event);
		bool verticalScrollEventCallback(VerticalScrollEvent* event);
		bool keyPressEventCallback(KeyPressEvent* event);
		bool keyReleaseEventCallback(KeyReleaseEvent* event);
		bool windowResizeEventCallback(WindowResizeEvent* event);
	};
}
