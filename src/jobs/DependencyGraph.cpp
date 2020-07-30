#include "DependencyGraph.h"

#include "../ecs/WorldLoadStatus.h"
#include "../engine/Debugger.h"
#include "../engine/Engine.h"
#include "../lua/LuaVal.h"
#include "../rendering/Renderer.h"
#include "../rendering/SubRenderer.h"
#include "Worker.h"

#include <iostream>

using namespace vecs;

Job* DependencyGraph::load(Engine* engine, Worker* worker, sol::table config, WorldLoadStatus* status) {
	sol::table systems = config["systems"];
	sol::table renderers = config["renderers"];
	this->engine = engine;
	this->status = status;
	
	// the systems and renderers tables are giving me a size of 0 since they're non-sequential
	// so instead of resizing the nodes vector here, we just emplace each element into it

	// Build list of nodes
	uint8_t i = 0;
	for (auto kvp : systems) {
		sol::type type = kvp.second.get_type();
		LuaVal system;
		if (type == sol::type::string) {
			std::string filename = "resources/" + kvp.second.as<std::string>();
			auto result = worker->lua.script_file(filename);
			if (!result.valid()) {
				sol::error err = result;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Failed to load world. Attempted to load system at \"" + filename + "\" but lua parsing failed with error:\n[LUA] " + std::string(err.what()));
				status->isCancelled = true;
				return nullptr;
			}
			if (result.get_type() != sol::type::table) {
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Failed to load world. Attempted to load system at \"" + filename + "\" but a table wasn't returned");
				status->isCancelled = true;
				return nullptr;
			}
			system = LuaVal::fromTable(result);
		} else if (type == sol::type::table) {
			system = LuaVal::fromTable(systems[kvp.first]);
		} else continue;

		// Setup dependency node load status
		LuaVal::MapType* systemMap = std::get<LuaVal::MapType*>(system.value);
		DependencyNodeLoadStatus* nodeStatus = new DependencyNodeLoadStatus(kvp.first.as<std::string>(),
			system.get("preInit").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE,
			system.get("init").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE,
			system.get("postInit").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE);
		nodes.emplace_back(new DependencyNode(this, DEPENDENCY_NODE_TYPE_SYSTEM, system, nodeStatus));
		status->systems.emplace_back(nodeStatus);
		systemsMap[kvp.first.as<std::string>()] = nodes[i];
		i++;
	}
	for (auto kvp : renderers) {
		sol::type type = kvp.second.get_type();
		LuaVal subrenderer;
		if (type == sol::type::string) {
			std::string filename = "resources/" + kvp.second.as<std::string>();
			auto result = worker->lua.script_file(filename);
			if (!result.valid()) {
				sol::error err = result;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Failed to load world. Attempted to load renderer at \"" + filename + "\" but lua parsing failed with error:\n[LUA] " + std::string(err.what()));
				status->isCancelled = true;
				return nullptr;
			}
			if (result.get_type() != sol::type::table) {
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Failed to load world. Attempted to load system at \"" + filename + "\" but a table wasn't returned");
				status->isCancelled = true;
				return nullptr;
			}
			subrenderer = LuaVal::fromTable(result);
		} else if (type == sol::type::table) {
			subrenderer = LuaVal::fromTable(renderers[kvp.first]);
		} else continue;

		// Setup dependency node load status
		LuaVal::MapType* systemMap = std::get<LuaVal::MapType*>(subrenderer.value);
		DependencyNodeLoadStatus* nodeStatus = new DependencyNodeLoadStatus(kvp.first.as<std::string>(),
			subrenderer.get("preInit").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE,
			subrenderer.get("init").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE,
			subrenderer.get("postInit").type == LUA_TYPE_FUNCTION ? DEPENDENCY_FUNCTION_WAITING : DEPENDENCY_FUNCTION_NOT_AVAILABLE);
		nodes.emplace_back(new DependencyNode(this, DEPENDENCY_NODE_TYPE_RENDERER, subrenderer, nodeStatus));
		status->renderers.emplace_back(nodeStatus);
		renderersMap[kvp.first.as<std::string>()] = nodes[i];
		i++;
	}

	World* world = worker->getWorld();
	Job* preInitJob = worker->allocateJob();
	preInitJob->type = JOB_TYPE_PREINIT;
	preInitJob->world = world;
	preInitJob->extra = this;
	preInitJob->parent = nullptr;
	preInitJob->persistent = true;
	preInitJob->unfinishedJobs = 1;

	Job* initJob = worker->allocateJob();
	initJob->type = JOB_TYPE_INIT;
	initJob->world = world;
	initJob->extra = this;
	initJob->parent = nullptr;
	initJob->persistent = true;
	initJob->unfinishedJobs = 1;

	Job* postInitJob = worker->allocateJob();
	postInitJob->type = JOB_TYPE_POSTINIT;
	postInitJob->world = world;
	postInitJob->extra = this;
	postInitJob->parent = nullptr;
	postInitJob->persistent = true;
	postInitJob->unfinishedJobs = 1;

	Job* finalizeJob = worker->allocateJob();
	finalizeJob->type = JOB_TYPE_FINISH;
	finalizeJob->world = world;
	finalizeJob->extra = this;
	finalizeJob->parent = nullptr;
	finalizeJob->persistent = true;
	finalizeJob->unfinishedJobs = 1;
	finalizeJob->continuationCount = 0;

	preInitJob->continuations[0] = initJob;
	preInitJob->continuationCount = 1;

	initJob->continuations[0] = postInitJob;
	initJob->continuationCount = 1;

	postInitJob->continuations[0] = finalizeJob;
	postInitJob->continuationCount = 1;

	// start loading world job
	worker->pushJob(preInitJob);

	// return the last job to be completed
	return finalizeJob;
}

void DependencyGraph::preInit(Worker* worker) {
	status->currentStep = WORLD_LOAD_STEP_PREINIT;
	worker->job->parent = nullptr;
	World* world = worker->getWorld();
	for (auto node : nodes) {
		if (node->type == DEPENDENCY_NODE_TYPE_SYSTEM && node->status->preInitStatus == DEPENDENCY_FUNCTION_NOT_AVAILABLE) continue;
		worker->job->unfinishedJobs++;
		Job* nodeJob = worker->allocateJob();
		nodeJob->type = JOB_TYPE_PREINIT_NODE;
		nodeJob->world = world;
		nodeJob->unfinishedJobs = 1;
		nodeJob->extra = node;
		nodeJob->persistent = true;
		nodeJob->parent = worker->job;
		nodeJob->continuationCount = 0;
		worker->pushJob(nodeJob);
	}
}

void DependencyGraph::init(Worker* worker) {
	status->currentStep = WORLD_LOAD_STEP_INIT;
	worker->job->parent = nullptr;
	World* world = worker->getWorld();
	for (auto node : nodes) {
		if (node->status->initStatus == DEPENDENCY_FUNCTION_NOT_AVAILABLE) continue;
		worker->job->unfinishedJobs++;
		Job* nodeJob = worker->allocateJob();
		nodeJob->type = JOB_TYPE_INIT_NODE;
		nodeJob->world = world;
		nodeJob->unfinishedJobs = 1;
		nodeJob->extra = node;
		nodeJob->persistent = true;
		nodeJob->parent = worker->job;
		nodeJob->continuationCount = 0;
		worker->pushJob(nodeJob);
	}
}

void DependencyGraph::postInit(Worker* worker) {
	status->currentStep = WORLD_LOAD_STEP_POSTINIT;
	worker->job->parent = nullptr;
	World* world = worker->getWorld();
	for (auto node : nodes) {
		if (node->status->postInitStatus == DEPENDENCY_FUNCTION_NOT_AVAILABLE) continue;
		worker->job->unfinishedJobs++;
		Job* nodeJob = worker->allocateJob();
		nodeJob->type = JOB_TYPE_POSTINIT_NODE;
		nodeJob->world = world;
		nodeJob->unfinishedJobs = 1;
		nodeJob->extra = node;
		nodeJob->persistent = true;
		nodeJob->parent = worker->job;
		nodeJob->continuationCount = 0;
		worker->pushJob(nodeJob);
	}
}

void DependencyGraph::finish(Worker* worker) {
	status->currentStep = WORLD_LOAD_STEP_FINISHING;

	// Create the edges of our graph
	for (auto node : nodes) {
		node->createEdges(systemsMap, renderersMap);
	}

	// Find nodes without any dependencies and store them in a list of nodes we can start with first
	for (auto node : nodes) {
		if (node->dependencies.empty())
			leaves.emplace_back(node);
	}

	status->currentStep = WORLD_LOAD_STEP_FINISHED;
}

void DependencyGraph::execute(Worker* worker) {
	// Setup frame
	for (auto node : nodes) {
		node->dependenciesRemaining = node->dependencies.size();

		// Some renderers need to perform some work at the start of our frame
		// e.g. imgui needs to create a new frame, and it has to start after event handling
		node->startFrame(worker);
	}

	// Create container job so we know when all of this frame's jobs are done
	// No need to start it since its a dummy job and we just want to track when its complete
	Job* executeJob = worker->allocateJob();
	executeJob->type = JOB_TYPE_DUMMY;
	executeJob->world = worker->getWorld();
	executeJob->unfinishedJobs = nodes.size();
	executeJob->persistent = false;
	executeJob->parent = nullptr;
	executeJob->continuationCount = 0;

	// Start initial jobs
	World* world = worker->getWorld();
	for (auto node : leaves) {
		Job* nodeJob = worker->allocateJob();
		nodeJob->type = JOB_TYPE_EXECUTE;
		nodeJob->world = world;
		nodeJob->unfinishedJobs = 1;
		nodeJob->persistent = false;
		nodeJob->extra = node;
		nodeJob->parent = executeJob;
		nodeJob->continuationCount = 0;
		worker->pushJob(nodeJob);
	}

	// Do work until all jobs are finished
	while (executeJob->unfinishedJobs > 0)
		worker->work();
}

void DependencyGraph::windowRefresh(int imageCount) {
	for (auto node : nodes)
		node->windowRefresh(imageCount);
}

void DependencyGraph::cleanup() {
	for (auto node : nodes) {
		node->cleanup();
	}
}

// TODO abstract out calling LuaVal functions?
void DependencyNode::preInit(Worker* worker) {
	status->preInitStatus = DEPENDENCY_FUNCTION_ACTIVE;

	if (type == DEPENDENCY_NODE_TYPE_RENDERER)
		subrenderer = new SubRenderer(&config, worker, graph->engine->device, &graph->engine->renderer, status);
	else {
		LuaVal preInit = config.get("preInit");
		if (preInit.type == LUA_TYPE_FUNCTION) {
			sol::load_result loadResult = worker->lua.load(std::get<sol::bytecode>(preInit.value).as_string_view());
			if (!loadResult.valid()) {
				sol::error err = loadResult;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
				status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
				// TODO cancel world loading and future jobs
				return;
			}

			auto result = loadResult(config);
			if (!result.valid()) {
				sol::error err = result;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
				status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
				// TODO cancel world loading and future jobs
				return;
			}
		}
	}

	status->preInitStatus = DEPENDENCY_FUNCTION_COMPLETE;
}

void DependencyNode::init(Worker* worker) {
	status->initStatus = DEPENDENCY_FUNCTION_ACTIVE;

	LuaVal init = config.get("init");
	if (init.type == LUA_TYPE_FUNCTION) {
		sol::load_result loadResult = worker->lua.load(std::get<sol::bytecode>(init.value).as_string_view());
		if (!loadResult.valid()) {
			sol::error err = loadResult;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->initStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}

		auto result = type == DEPENDENCY_NODE_TYPE_RENDERER ? loadResult(config, subrenderer) : loadResult(config);

		if (!result.valid()) {
			sol::error err = result;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->initStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}
	}

	status->initStatus = DEPENDENCY_FUNCTION_COMPLETE;
}

void DependencyNode::postInit(Worker* worker) {
	status->postInitStatus = DEPENDENCY_FUNCTION_ACTIVE;

	LuaVal init = config.get("postInit");
	if (init.type == LUA_TYPE_FUNCTION) {
		sol::load_result loadResult = worker->lua.load(std::get<sol::bytecode>(init.value).as_string_view());
		if (!loadResult.valid()) {
			sol::error err = loadResult;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->postInitStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}

		auto result = type == DEPENDENCY_NODE_TYPE_RENDERER ? loadResult(config, subrenderer) : loadResult(config);

		if (!result.valid()) {
			sol::error err = result;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->postInitStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}
	}

	status->postInitStatus = DEPENDENCY_FUNCTION_COMPLETE;
}

void DependencyNode::createEdges(std::map<std::string, DependencyNode*> systemsMap, std::map<std::string, DependencyNode*> renderersMap) {
	LuaVal dependenciesTable = config.get("dependencies");
	if (dependenciesTable.type == LUA_TYPE_TABLE) {
		for (auto kvp : *std::get<LuaVal::MapType*>(dependenciesTable.value)) {
			if (kvp.first.type != LUA_TYPE_STRING) {
				Debugger::addLog(DEBUG_LEVEL_WARN, "Dependencies dictionary contained non-string key");
				continue;
			}
			if (kvp.second.type != LUA_TYPE_STRING) {
				Debugger::addLog(DEBUG_LEVEL_WARN, "Dependencies dictionary contained non-string value");
				continue;
			}

			DependencyNode* dependency;
			std::string dependencyType = std::get<std::string>(kvp.second.value);
			if (dependencyType == "system")
				dependency = systemsMap[std::get<std::string>(kvp.first.value)];
			else if (dependencyType == "renderer")
				dependency = renderersMap[std::get<std::string>(kvp.first.value)];
			else continue;

			if (dependency == nullptr) {
				Debugger::addLog(DEBUG_LEVEL_WARN, status->name + " dependency " + std::get<std::string>(kvp.first.value) + " (" + dependencyType + ") not found!");
				continue;
			}

			dependencies.emplace_back(dependency);
			dependency->dependents.emplace_back(this);
		}
	}

	LuaVal forwardDependenciesTable = config.get("forwardDependencies");
	if (forwardDependenciesTable.type == LUA_TYPE_TABLE) {
		for (auto kvp : *std::get<LuaVal::MapType*>(forwardDependenciesTable.value)) {
			if (kvp.first.type != LUA_TYPE_STRING) {
				Debugger::addLog(DEBUG_LEVEL_WARN, "Forward dependencies dictionary contained non-string key");
				continue;
			}
			if (kvp.second.type != LUA_TYPE_STRING) {
				Debugger::addLog(DEBUG_LEVEL_WARN, "Forward dependencies dictionary contained non-string value");
				continue;
			}

			DependencyNode* dependent;
			std::string dependencyType = std::get<std::string>(kvp.second.value);
			if (dependencyType == "system")
				dependent = systemsMap[std::get<std::string>(kvp.first.value)];
			else if (dependencyType == "renderer")
				dependent = renderersMap[std::get<std::string>(kvp.first.value)];
			else continue;

			if (dependent == nullptr) {
				Debugger::addLog(DEBUG_LEVEL_WARN, status->name + " forward dependency " + std::get<std::string>(kvp.first.value) + " (" + dependencyType + ") not found!");
				continue;
			}

			dependents.emplace_back(dependent);
			dependent->dependencies.emplace_back(this);
		}
	}
}

void DependencyNode::startFrame(Worker* worker) {
	if (type == DEPENDENCY_NODE_TYPE_RENDERER) {
		subrenderer->allocatedDescriptorSets = 0;
	}

	LuaVal startFrame = config.get("startFrame");
	if (startFrame.type == LUA_TYPE_FUNCTION) {
		auto loadResult = worker->lua.load(std::get<sol::bytecode>(startFrame.value).as_string_view());
		if (!loadResult.valid()) {
			sol::error err = loadResult;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}

		auto result = loadResult(config, subrenderer);
		if (!result.valid()) {
			sol::error err = result;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
			// TODO cancel world loading and future jobs
			return;
		}
	}
}

void DependencyNode::execute(Worker* worker) {
	LuaVal update = type == DEPENDENCY_NODE_TYPE_SYSTEM ? config.get("update") : config.get("render");
	if (update.type == LUA_TYPE_FUNCTION) {
		auto loadResult = worker->lua.load(std::get<sol::bytecode>(update.value).as_string_view());
		if (!loadResult.valid()) {
			sol::error err = loadResult;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
			return;
		}

		auto result = type == DEPENDENCY_NODE_TYPE_RENDERER ? loadResult(config, subrenderer) : loadResult(config);

		if (!result.valid()) {
			sol::error err = result;
			Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA] " + std::string(err.what()));
			status->preInitStatus = DEPENDENCY_FUNCTION_ERROR;
			return;
		}
	}
}

void DependencyNode::windowRefresh(int imageCount) {
	if (subrenderer != nullptr)
		subrenderer->windowRefresh(imageCount);
}

void DependencyNode::cleanup() {
	if (subrenderer != nullptr)
		subrenderer->cleanup();
}
