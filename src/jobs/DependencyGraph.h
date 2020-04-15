#pragma once

#include "../rendering/SubRenderer.h"

#include <vector>
#include <map>

#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

namespace vecs {

	enum DependencyNodeType {
		DEPENDENCY_NODE_TYPE_SYSTEM,
		DEPENDENCY_NODE_TYPE_RENDERER
	};

	class DependencyNode {
	public:
		std::vector<DependencyNode*> dependencies;
		std::vector<DependencyNode*> dependents;

		uint8_t dependenciesRemaining;

		DependencyNode(Device* device, Renderer* renderer, DependencyNodeType type, sol::table worldConfig, sol::table config);

		void init(sol::table worldConfig);

		// Use the node's config and fill dependencies and dependents using these maps of names to their respective nodes
		// We do this outside the constructor because we need all the nodes to exist before we can start linking them
		void createEdges(std::map<std::string, DependencyNode*> systemsMap, std::map<std::string, DependencyNode*> renderersMap);

		void startFrame(sol::table worldConfig);
		// In the future we'll create a number of worker threads, with their own VkQueues,
		// and execute will add any number of jobs to a job queue that each worker thread will pull from
		// Whenever the last job for a dependency node is completed,
		// it'll check for any nodes that now have all their dependencies met, and execute them
		void execute(sol::table worldConfig);
		void windowRefresh(bool numImagesChanged, int imageCount);

		void cleanup();

	private:
		DependencyNodeType type;
		sol::table config;
		SubRenderer* subrenderer = nullptr;
		sol::function update;
	};

	class DependencyGraph {
	public:
		void init(Device* device, Renderer* renderer, sol::state* lua, sol::table config);

		void execute();
		void windowRefresh(bool numImagesChanged, int imageCount);

		void cleanup();

	private:
		sol::table config;

		std::vector<DependencyNode*> nodes;
		std::vector<DependencyNode*> leaves;
	};
}
