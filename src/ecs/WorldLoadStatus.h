#pragma once

#include <vector>

namespace vecs {

	enum WorldLoadStep {
		WORLD_LOAD_STEP_SETUP,
		WORLD_LOAD_STEP_PREINIT,
		WORLD_LOAD_STEP_INIT,
		WORLD_LOAD_STEP_FINISHED
	};

	enum DependencyFunctionStatus {
		DEPENDENCY_FUNCTION_NOT_AVAILABLE,
		DEPENDENCY_FUNCTION_WAITING,
		DEPENDENCY_FUNCTION_ACTIVE,
		DEPENDENCY_FUNCTION_COMPLETE
	};

	class DependencyNodeLoadStatus {
	public:
		std::string name;
		DependencyFunctionStatus preInitStatus;
		DependencyFunctionStatus initStatus;

		DependencyNodeLoadStatus(std::string name, DependencyFunctionStatus preInitStatus, DependencyFunctionStatus initStatus) {
			this->name = name;
			this->preInitStatus = preInitStatus;
			this->initStatus = initStatus;
		}
	};

	// This is an object used for lua scripts to access the current state of a world
	// currently being loaded asynchronously
	class WorldLoadStatus {
	public:
		WorldLoadStep currentStep = WORLD_LOAD_STEP_SETUP;
		bool isCancelled = false;
		std::vector<DependencyNodeLoadStatus*> systems;
		std::vector<DependencyNodeLoadStatus*> renderers;
	};
}
