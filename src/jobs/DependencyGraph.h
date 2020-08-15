#pragma once

#include "../lua/LuaVal.h"

#include <vector>
#include <map>
#include <atomic>

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

namespace vecs {

	// Forward Declarations
	class DependencyGraph;
	class DependencyNodeLoadStatus;
	class Device;
	class Renderer;
	class SubRenderer;
	class Worker;
	class WorldLoadStatus;

	enum DependencyNodeType {
		DEPENDENCY_NODE_TYPE_SYSTEM,
		DEPENDENCY_NODE_TYPE_RENDERER
	};

	class DependencyNode {
		friend class DependencyGraph;
	public:
		std::vector<DependencyNode*> dependencies;
		std::vector<DependencyNode*> dependents;

		std::atomic_uint8_t dependenciesRemaining;

		LuaVal config;

		DependencyNode(DependencyGraph* graph, DependencyNodeType type, LuaVal config, DependencyNodeLoadStatus* status) {
			this->graph = graph;
			this->type = type;
			this->config = config;
			this->status = status;
		}

		void preInit(Worker* worker);
		void init(Worker* worker);
		void postInit(Worker* worker);
		// Use the node's config and fill dependencies and dependents using these maps of names to their respective nodes
		// We do this outside the constructor because we need all the nodes to exist before we can start linking them
		void createEdges(std::map<std::string, DependencyNode*> systemsMap, std::map<std::string, DependencyNode*> renderersMap);

		void startFrame(Worker* worker);
		void execute(Worker* worker);
		void windowRefresh(int imageCount);

		void cleanup();

	private:
		DependencyGraph* graph;

		DependencyNodeType type;
		DependencyNodeLoadStatus* status;

		SubRenderer* subrenderer = nullptr;
	};

	class DependencyGraph {
		friend class DependencyNode;
	public:
		// Only used during world loading process
		WorldLoadStatus* status;

		Job* load(Engine* engine, Worker* worker, sol::table config, WorldLoadStatus* status);
		void preInit(Worker* worker);
		void init(Worker* worker);
		void postInit(Worker* worker);
		void finish(Worker* worker);

		void execute(Worker* worker);
		void windowRefresh(int imageCount);

		void cleanup();

	private:
		Engine* engine;

		std::vector<DependencyNode*> nodes;
		std::vector<DependencyNode*> leaves;

		std::map<std::string, DependencyNode*> systemsMap;
		std::map<std::string, DependencyNode*> renderersMap;
	};
}
