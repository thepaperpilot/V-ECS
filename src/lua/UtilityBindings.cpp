#include "UtilityBindings.h"

#include "../ecs/World.h"
#include "../ecs/WorldLoadStatus.h"
#include "../engine/Debugger.h"
#include "../engine/Engine.h"

#include <filesystem>
#include <glm/glm.hpp>

void traverseResources(std::vector<std::string>* resources, std::filesystem::path filepath, std::string extension) {
	for (const auto& entry : std::filesystem::directory_iterator(filepath)) {
		if (extension == "" || (!entry.is_directory() && entry.path().extension() == extension))
			resources->push_back(entry.path().string());
		if (extension != "" && entry.is_directory()) {
			traverseResources(resources, entry, extension);
		}
	}
}

void vecs::UtilityBindings::setupState(sol::state& lua, Worker* worker, Engine* engine) {
	lua.new_enum("debugLevels",
		"Error", DEBUG_LEVEL_ERROR,
		"Warn", DEBUG_LEVEL_WARN,
		"Info", DEBUG_LEVEL_INFO,
		"Verbose", DEBUG_LEVEL_VERBOSE
	);
	lua.new_enum("worldLoadStep",
		"Setup", WORLD_LOAD_STEP_SETUP,
		"PreInit", WORLD_LOAD_STEP_PREINIT,
		"Init", WORLD_LOAD_STEP_INIT,
		"PostInit", WORLD_LOAD_STEP_POSTINIT,
		"Finishing", WORLD_LOAD_STEP_FINISHING,
		"Finished", WORLD_LOAD_STEP_FINISHED
	);
	lua.new_enum("functionStatus",
		"NotAvailable", DEPENDENCY_FUNCTION_NOT_AVAILABLE,
		"Waiting", DEPENDENCY_FUNCTION_WAITING,
		"Active", DEPENDENCY_FUNCTION_ACTIVE,
		"Complete", DEPENDENCY_FUNCTION_COMPLETE
	);
	lua.new_enum("sizes",
		"Float", sizeof(float),
		"Vec2", sizeof(glm::vec2),
		"Vec3", sizeof(glm::vec3),
		"Vec4", sizeof(glm::vec4),
		"Mat4", sizeof(glm::mat4)
	);

	lua["getResources"] = [](std::string path, std::string extension) -> sol::as_table_t<std::vector<std::string>> {
		std::vector<std::string> resources;
		auto filepath = std::filesystem::path("./resources/" + path).make_preferred();
		traverseResources(&resources, filepath, extension);
		return resources;
	};

	lua["openURL"] = [](std::string url) {
		system(("start " + url).c_str());
	};

	auto eng = lua["engine"] = lua.create_table_with();
	eng["getVsyncEnabled"] = [engine]() -> bool { return engine->vsyncEnabled; };
	eng["setVsyncEnabled"] = [engine](bool vsyncEnabled) {
		engine->vsyncEnabled = vsyncEnabled;
		engine->needsRefresh = true;
	};

	// debugger
	lua["debugger"] = lua.create_table_with(
		// Originally this was called "getLog", so in lua you'd do `debugger.getLog()`, but it was using it as a property getter instead of a function getter
		"getLogs", []() -> sol::as_table_t<std::vector<Log>> { return Debugger::getLog(); },
		"addLog", [](DebugLevel level, std::string message) { Debugger::addLog(level, message); },
		"clearLog", []() { Debugger::clearLog(); }
	);
	lua["print"] = [](sol::object message, sol::this_state Ls) { Debugger::addLog(DEBUG_LEVEL_INFO, sol::state_view(Ls)["tostring"](message)); };
	lua.new_usertype<Log>("log",
		"new", sol::no_constructor,
		"level", &Log::level,
		"message", &Log::message
	);

	// time namespace
	lua["time"] = lua.create_table_with(
		"getDeltaTime", [worker]() -> float { return worker->getWorld()->deltaTime; }
	);

	// world loading
	lua["loadWorld"] = [engine](std::string filename) -> WorldLoadStatus* { return engine->setWorld(filename); };
	lua.new_usertype<WorldLoadStatus>("worldLoadStatus",
		sol::no_constructor,
		"currentStep", &WorldLoadStatus::currentStep,
		"isCancelled", &WorldLoadStatus::isCancelled,
		"getSystems", [](WorldLoadStatus status) ->sol::as_table_t <std::vector<DependencyNodeLoadStatus*>> { return status.systems; },
		"getRenderers", [](WorldLoadStatus status) -> sol::as_table_t<std::vector<DependencyNodeLoadStatus*>> { return status.renderers; }
	);
	lua.new_usertype<DependencyNodeLoadStatus>("nodeLoadStatus",
		sol::no_constructor,
		"name", &DependencyNodeLoadStatus::name,
		"preInitStatus", &DependencyNodeLoadStatus::preInitStatus,
		"initStatus", &DependencyNodeLoadStatus::initStatus
	);
}
