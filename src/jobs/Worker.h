#pragma once

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <mutex>
#include <vulkan/vulkan.h>

#include "JobQueue.h"

namespace vecs {

	// Forward Declarations
	class Device;
	class Engine;
	class JobManager;
	class World;

	class Worker {
	public:
		Engine* engine;

		sol::state lua;

		VkCommandPool commandPool;
		VkQueue graphicsQueue;
		std::mutex* queueLock;

		bool active = false;
		Job* job = nullptr;
		bool stealPersistent = true;

		ParallelData parallelDataPool[MAX_JOB_COUNT];
		uint32_t allocatedParallelData = 0;

		Worker(Engine* engine, World* world = nullptr);

		World* getWorld();
		virtual Job* allocateJob();
		virtual ParallelData* allocateParallelData();
		virtual Job* getJob();
		void pushJob(Job* job);
		void finish(Job* job);
		virtual void resetFrame();
		VkCommandBuffer getCommandBuffer();

		void init(uint32_t queueIndex, std::mutex* queueLock = nullptr);
		void start();
		bool work(Job* job = nullptr);
		void cleanup();
		void createInheritanceInfo(uint32_t newSize = 0);

	protected:
		Job jobPool[MAX_JOB_COUNT];
		uint32_t allocatedJobs = 0;
		uint32_t allocatedCommandBuffers = 0;
		JobQueue queue;
		JobQueue persistentQueue;

	private:
		Device* device;

		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkCommandBufferInheritanceInfo> inheritanceInfo;

		World* world;
		std::thread* thread;

		void run();
	};
}
