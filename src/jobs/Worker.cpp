#include "Worker.h"

#include "../ecs/World.h"
#include "../ecs/WorldLoadStatus.h"
#include "../engine/Device.h"
#include "../engine/Engine.h"
#include "../lua/ECSBindings.h"
#include "../lua/GLFWBindings.h"
#include "../lua/imguiBindings.h"
#include "../lua/JobBindings.h"
#include "../lua/LuaVal.h"
#include "../lua/MathBindings.h"
#include "../lua/NoiseBindings.h"
#include "../lua/RenderingBindings.h"
#include "../lua/UtilityBindings.h"

using namespace vecs;

// Used to keep workers asleep when there's no job to work on
std::condition_variable wakeCondition;
std::mutex wakeMutex;

Worker::Worker(Engine* engine, World* world) : queue(this), persistentQueue(this) {
	this->engine = engine;
	this->world = world;

	// Mark jobs as in a buffer
	for (int i = 0; i < MAX_JOB_COUNT; i++) {
		jobPool[i].inBuffer = true;
	}
}

World* Worker::getWorld() {
	if (job != nullptr)
		return job->world;
	return world;
}

Job* Worker::allocateJob() {
	Job* job;
	int jobsLeft = MAX_JOB_COUNT;
	do {
		// Check if we've done the whole loop
		if (jobsLeft == 0) {
			job = new Job();
			job->inBuffer = false;
			break;
		}
		allocatedJobs++;
		job = &jobPool[allocatedJobs & (MAX_JOB_COUNT - 1)];
		jobsLeft--;
	} while (job->unfinishedJobs != 0);
	job->world = nullptr;
	return job;
}

ParallelData* Worker::allocateParallelData() {
	ParallelData* data;
	int dataLeft = MAX_JOB_COUNT;
	do {
		// Check if we've done the whole loop
		if (dataLeft == 0) {
			ParallelData* data = new ParallelData();
			data->inBuffer = false;
			break;
		}
		allocatedParallelData++;
		data = &parallelDataPool[allocatedParallelData & (MAX_JOB_COUNT - 1)];
		dataLeft--;
	} while (data->inUse);
	data->inUse = true;
	return data;
}

Job* Worker::getJob() {
	Job* job = queue.pop();
	if (job == nullptr) {
		// If our normal queue is empty try our persistent queue
		job = persistentQueue.pop();
	}
	if (job == nullptr) {
		// our own queues are empty so find a random other queue to try to steal from
		Worker* worker = engine->jobManager.workerThreads[std::rand() % engine->jobManager.workerThreads.size()];
		job = worker->queue.steal();
		// also try their persistent queue if we're flagged to
		if (job == nullptr && stealPersistent)
			job = worker->persistentQueue.steal();
	}
	if (job == nullptr && engine->world != nullptr) {
		// next try finding a job from the active world's worker
		job = engine->world->worker.queue.steal();
		// also try their persistent queue if we're flagged to
		if (job == nullptr && stealPersistent)
			job = engine->world->worker.persistentQueue.steal();
	}
	if (job == nullptr && engine->nextWorld != nullptr) {
		// if we still don't have a job, find if there's a loading world we can take a job from
		job = engine->nextWorld->worker.queue.steal();
		// also try their persistent queue if we're flagged to
		if (job == nullptr && stealPersistent)
			job = engine->nextWorld->worker.persistentQueue.steal();
	}
	// job may still equal nullptr at this point if there were no jobs to get
	// instead of using complicated set of semaphores we'll just query if there's a worker to steal from again
	return job;
}

void Worker::pushJob(Job* job) {
	if (job->persistent)
		persistentQueue.push(job);
	else
		queue.push(job);
	wakeCondition.notify_one();
}

void Worker::finish(Job* job) {
	if (job->unfinishedJobs.fetch_sub(1) == 1) { // this means we just decremented to 0
		// Signal to parent job we're done
		if (job->parent)
			finish(job->parent);

		// Run continuation jobs
		for (uint16_t i = 0; i < job->continuationCount; i++) {
			pushJob(job->continuations[i]);
		}

		// Destroy this if it wasn't in a buffer
		if (!job->inBuffer)
			delete job;
	}
}

void Worker::resetFrame() {
	allocatedCommandBuffers = 0;
}

VkCommandBuffer Worker::getCommandBuffer() {
	// Find index of the command buffer to get
	uint32_t imageIndex = allocatedCommandBuffers * engine->renderer.imageCount + engine->renderer.imageIndex;
	allocatedCommandBuffers++;

	// Resize array of command buffers as necessary
	if (imageIndex >= commandBuffers.size()) {
		uint32_t newSize = commandBuffers.size() / engine->renderer.imageCount;
		if (newSize == 0) newSize = 1;
		newSize *= engine->renderer.imageCount;
		while (imageIndex >= newSize)
			newSize *= 2;
		
		createInheritanceInfo(newSize);

		std::vector<VkCommandBuffer> newBuffers = device->createCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, newSize - commandBuffers.size(), commandPool);
		commandBuffers.insert(commandBuffers.end(), newBuffers.begin(), newBuffers.end());
	}

	// Begin the command buffer
	VkCommandBuffer activeCommandBuffer = commandBuffers[imageIndex];
	device->beginSecondaryCommandBuffer(activeCommandBuffer, &inheritanceInfo[imageIndex]);

	return activeCommandBuffer;
}

void Worker::init(uint32_t queueIndex, std::mutex* queueLock) {
	this->queueLock = queueLock;
	device = engine->device;
	commandPool = device->createCommandPool(device->queueFamilyIndices.graphics.value());
	vkGetDeviceQueue(*device, device->queueFamilyIndices.graphics.value(), queueIndex, &graphicsQueue);

	// Setup lua state
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

	UtilityBindings::setupState(lua, this, engine);
	GLFWBindings::setupState(lua, this, engine->window);
	MathBindings::setupState(lua);
	ECSBindings::setupState(lua, this);
	NoiseBindings::setupState(lua, engine->fastestSimd);
	RenderingBindings::setupState(lua, this, device);
	imguiBindings::setupState(lua, this, engine, device);
	LuaValBindings::setupState(lua);
	JobBindings::setupState(lua, this);
}

void Worker::start() {
	thread = new std::thread(&Worker::run, this);
}

bool Worker::work(Job* job) {
	// Get a job if none specified
	this->job = job == nullptr ? getJob() : job;
	// Re-assign it back to job for convenience
	job = this->job;
	if (job) {
		// Execute job
		switch (job->type) {
		case JOB_TYPE_PREINIT:
			((DependencyGraph*)job->extra)->preInit(this);
			break;
		case JOB_TYPE_PREINIT_NODE:
			((DependencyNode*)job->extra)->preInit(this);
			break;
		case JOB_TYPE_INIT:
			((DependencyGraph*)job->extra)->init(this);
			break;
		case JOB_TYPE_INIT_NODE:
			((DependencyNode*)job->extra)->init(this);
			break;
		case JOB_TYPE_POSTINIT:
			((DependencyGraph*)job->extra)->postInit(this);
			break;
		case JOB_TYPE_POSTINIT_NODE:
			((DependencyNode*)job->extra)->postInit(this);
			break;
		case JOB_TYPE_FINISH: {
			auto graph = (DependencyGraph*)job->extra;
			graph->finish(this);
			if (!graph->status->isCancelled)
				getWorld()->isValid = true;
			break;
		}
		case JOB_TYPE_EXECUTE: {
			auto node = (DependencyNode*)job->extra;
			// Create follow up job to start dependent nodes
			Job* cascadeJob = allocateJob();
			cascadeJob->type = JOB_TYPE_CASCADE;
			cascadeJob->world = getWorld();
			cascadeJob->unfinishedJobs = 1;
			cascadeJob->extra = node;
			cascadeJob->parent = job->parent;
			cascadeJob->continuationCount = 0;
			job->continuations[0] = cascadeJob;
			job->continuationCount = 1;
			// Execute this node
			node->execute(this);
			break;
		}
		case JOB_TYPE_CASCADE: {
			auto node = (DependencyNode*)job->extra;
			for (auto node : node->dependents) {
				// The way this is setup, circular dependencies won't cause an error,
				// it'll just not run any involved nodes
				// TODO handle parent jobs not running
				if (node->dependenciesRemaining.fetch_sub(1) == 1) {
					Job* nodeJob = allocateJob();
					nodeJob->type = JOB_TYPE_EXECUTE;
					nodeJob->world = getWorld();
					nodeJob->unfinishedJobs = 1;
					nodeJob->extra = node;
					nodeJob->parent = job->parent;
					nodeJob->continuationCount = 0;
					pushJob(nodeJob);
				}
			}
			// Reset the job parent because we just needed it as a reference,
			// but this shouldn't count towards its job total
			job->parent = nullptr;
			break;
		}
		case JOB_TYPE_PARALLEL: {
			auto parData = (ParallelData*)job->extra;
			auto loadResult = lua.load(job->function.as_string_view());
			if (!loadResult.valid()) {
				sol::error err = loadResult;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA][JOB] " + std::string(err.what()));
			} else {
				auto result = loadResult(job->data, parData->start, parData->end);
				if (!result.valid()) {
					sol::error err = result;
					Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA][JOB] " + std::string(err.what()));
				}
			}
			delete job->data;
			if (parData->inBuffer)
				parData->inUse = false;
			else
				delete parData;
			break;
		}
		case JOB_TYPE_NORMAL: {
			auto loadResult = lua.load(job->function.as_string_view());
			if (!loadResult.valid()) {
				sol::error err = loadResult;
				Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA][JOB] " + std::string(err.what()));
			} else {
				auto result = loadResult(job->data);
				if (!result.valid()) {
					sol::error err = result;
					Debugger::addLog(DEBUG_LEVEL_ERROR, "[LUA][JOB] " + std::string(err.what()));
					// When handling errors, mark parent and continuation jobs recursively as complete
					// and if its a preinit, init, or finish function also set status->isCancelled to true
				}
			}
			delete job->data;
			break;
		}
		case JOB_TYPE_DUMMY:
			break;
		default: 
			Debugger::addLog(DEBUG_LEVEL_WARN, "Can't complete job with invalid type " + job->type);
			break;
		}

		finish(job);
		this->job = nullptr;
		return true;
	}
	return false;
}

void Worker::cleanup() {
	// Stop our worker thread
	if (active) {
		active = false;
		wakeCondition.notify_all();
		thread->join();
	}

	// Destroy our command buffers
	vkFreeCommandBuffers(*device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Destroy our command pool
    vkDestroyCommandPool(*device, commandPool, nullptr);
}

void Worker::createInheritanceInfo(uint32_t newSize) {
	if (!newSize) newSize = inheritanceInfo.size();
	else inheritanceInfo.resize(newSize);
	for (size_t i = 0; i < newSize; i++) {
		inheritanceInfo[i] = {};
		inheritanceInfo[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo[i].framebuffer = engine->renderer.swapChainFramebuffers[i % engine->renderer.imageCount];
		inheritanceInfo[i].renderPass = engine->renderer.renderPass;
		inheritanceInfo[i].subpass = 0;
	}
}

void Worker::run() {
	active = true;
	while (active) {
		if (!work()) {
			// Sleep until there is work
			// Note that finding a job will not check every other worker's queue,
			// so its possible to sleep when there is work to do. Over long periods of time,
			// sleeping like this (as opposed to spinlocking) will massively reduce the CPU
			// usage while idling (e.g. when submitting commands to the GPU) and on average
			// will still spread the work fairly evenly across the workers. Missing a queue
			// to steal from only happens when there is at least one empty queue. That is, the fewer
			// empty queues there are, the less likely a miss is; so when there's lots of work, it's
			// unlikely to sleep. This can cause issues if one worker thread spawns a ton of jobs
			// and the others are all empty, so ideally we'll make our parallel_for work recursively
			// instead of iteratively at some point
			std::unique_lock<std::mutex> lock(wakeMutex);
			wakeCondition.wait(lock);
		}
	}
}
