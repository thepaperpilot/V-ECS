#include "DependencyGraph.h"

#include <iostream>

using namespace vecs;

void DependencyGraph::init(Device* device, Renderer* renderer, sol::state* lua, sol::table config) {
	this->config = config;

	sol::table systems = config["systems"];
	sol::table renderers = config["renderers"];

	// the systems and renderers tables are giving me a size of 0 since they're non-sequential
	// so instead of resizing the nodes vector here, we just emplace each element into it

	// Build list of nodes
	uint8_t i = 0;
	std::map<std::string, DependencyNode*> systemsMap;
	for (auto kvp : systems) {
		sol::type type = kvp.second.get_type();
		sol::table system;
		if (type == sol::type::string) {
			std::string filename = "resources/" + kvp.second.as<std::string>();
			system = lua->script_file(filename);
			systems[kvp.first] = system;
		} else if (type == sol::type::table) {
			system = systems[kvp.first];
		} else continue;
		nodes.emplace_back(new DependencyNode(device, renderer, DEPENDENCY_NODE_TYPE_SYSTEM, config, system));
		systemsMap[kvp.first.as<std::string>()] = nodes[i];
		i++;
	}
	std::map<std::string, DependencyNode*> renderersMap;
	for (auto kvp : renderers) {
		sol::type type = kvp.second.get_type();
		sol::table subrenderer;
		if (type == sol::type::string) {
			std::string filename = "resources/" + kvp.second.as<std::string>();
			subrenderer = lua->script_file(filename);
			renderers[kvp.first] = subrenderer;
		} else if (type == sol::type::table) {
			subrenderer = renderers[kvp.first];
		} else continue;
		nodes.emplace_back(new DependencyNode(device, renderer, DEPENDENCY_NODE_TYPE_RENDERER, config, subrenderer));
		renderersMap[kvp.first.as<std::string>()] = nodes[i];
		i++;
	}

	// Make each node's list of dependents and dependencies
	for (auto node : nodes) {
		node->createEdges(systemsMap, renderersMap);
	}

	// Find nodes without any dependencies and store them in a list of nodes we can start with first
	for (auto node : nodes) {
		if (node->dependencies.empty())
			leaves.emplace_back(node);
	}
}

void DependencyGraph::execute() {
	for (auto node : nodes) {
		node->dependenciesRemaining = node->dependencies.size();

		// Some renderers need to perform some work at the start of our frame
		// e.g. imgui needs to create a new frame, and it has to start after event handling
		node->startFrame(config);
	}

	for (auto node : leaves)
		node->execute(config);
}

void DependencyGraph::windowRefresh(bool numImagesChanged, int imageCount) {
	for (auto node : nodes) {
		node->windowRefresh(numImagesChanged, imageCount);
	}
}

void DependencyGraph::cleanup() {
	for (auto node : nodes) {
		node->cleanup();
	}
}

DependencyNode::DependencyNode(Device* device, Renderer* renderer, DependencyNodeType type, sol::table worldConfig, sol::table config) {
	this->type = type;
	this->config = config;

	if (type == DEPENDENCY_NODE_TYPE_RENDERER)
		subrenderer = new SubRenderer(device, renderer, worldConfig, config);
	else {
		if (config["init"].get_type() == sol::type::function) {
			auto result = config["init"](config, worldConfig);
			if (!result.valid()) {
				sol::error err = result;
				std::cout << "[LUA] " << err.what() << std::endl;
				return;
			}
		}
	}
}

void DependencyNode::createEdges(std::map<std::string, DependencyNode*> systemsMap, std::map<std::string, DependencyNode*> renderersMap) {
	sol::optional<sol::table> dependenciesTable = config["dependencies"];
	if (dependenciesTable.has_value()) {
		for (auto kvp : dependenciesTable.value()) {
			std::string dependencyType = kvp.second.as<std::string>();
			DependencyNode* dependency;
			if (dependencyType == "system")
				dependency = systemsMap[kvp.first.as<std::string>()];
			else if (dependencyType == "renderer")
				dependency = renderersMap[kvp.first.as<std::string>()];
			else continue;

			if (dependency == nullptr) {
				std::cout << "Dependency " << kvp.first.as<std::string>() << " (" << dependencyType << ") not found!" << std::endl;
				continue;
			}

			dependencies.emplace_back(dependency);
			dependency->dependents.emplace_back(this);
		}
	}

	sol::optional<sol::table> forwardDependenciesTable = config["forwardDependencies"];
	if (forwardDependenciesTable.has_value()) {
		for (auto kvp : forwardDependenciesTable.value()) {
			std::string dependencyType = kvp.second.as<std::string>();
			DependencyNode* dependent;
			if (dependencyType == "system")
				dependent = systemsMap[kvp.first.as<std::string>()];
			else if (dependencyType == "renderer")
				dependent = renderersMap[kvp.first.as<std::string>()];
			else continue;

			if (dependent == nullptr) {
				std::cout << "Forward dependency " << kvp.first.as<std::string>() << " (" << dependencyType << ") not found!" << std::endl;
				continue;
			}

			dependents.emplace_back(dependent);
			dependent->dependencies.emplace_back(this);
		}
	}
}

void DependencyNode::startFrame(sol::table worldConfig) {
	if (type == DEPENDENCY_NODE_TYPE_RENDERER && config["startFrame"].get_type() == sol::type::function) {
		auto result = config["startFrame"](config, worldConfig, subrenderer);
		if (!result.valid()) {
			sol::error err = result;
			std::cout << "[LUA] " << err.what() << std::endl;
			return;
		}
	}
}

void DependencyNode::execute(sol::table worldConfig) {
	if (type == DEPENDENCY_NODE_TYPE_SYSTEM) {
		if (config["update"].get_type() == sol::type::function) {
			auto result = config["update"](config, worldConfig);
			if (!result.valid()) {
				sol::error err = result;
				std::cout << "[LUA] " << err.what() << std::endl;
				return;
			}
		}		
	} else if (type == DEPENDENCY_NODE_TYPE_RENDERER) {
		subrenderer->buildCommandBuffer();
	}

	for (auto node : dependents) {
		node->dependenciesRemaining--;
		// The way this is setup, circular dependencies won't cause an error,
		// it'll just not run any involved nodes
		if (node->dependenciesRemaining == 0)
			node->execute(worldConfig);
	}
}

void DependencyNode::windowRefresh(bool numImagesChanged, int imageCount) {
	if (subrenderer != nullptr)
		subrenderer->windowRefresh(numImagesChanged, imageCount);
}

void DependencyNode::cleanup() {
	if (subrenderer != nullptr)
		subrenderer->cleanup();
}
