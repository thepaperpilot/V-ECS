#pragma once

#include "Worker.h"

#include <vector>
#include <mutex>

namespace vecs {

	class JobManager {
	public:
		std::vector<Worker*> workerThreads;

		JobManager(Engine* engine) {
			this->engine = engine;
		}

		void init();
		uint32_t getQueueIndex(uint32_t desiredIndex, uint32_t maxQueues);
		std::mutex* getQueueLock(uint32_t queueIndex);
		void resetFrame();
		void windowRefresh();

		void cleanup();

	private:
		Engine* engine;

		uint32_t overlap;
		std::vector<std::mutex*> queueLocks;
	};
}
