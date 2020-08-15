#include "JobManager.h"

#include "../engine/Device.h"
#include "../engine/Engine.h"

#include <thread>

using namespace vecs;

// We create a number of worker threads equal to the hardware concurrency minus one for the main thread,
// with a minimum of one so that there is always at least one worker so loading worlds will still work
// Additionally each worker will be assigned a VkQueue index, where Renderer gets index 0, 2 are reserved
// for worlds (active world and world being loaded), and 2 onwards assigned to the worker threads. Since
// the hardware will have a limit on how many threads available (which will be less than required on all
// but nvidia GPUs), we'll mod that index by the number of queues available to get the actual assigned queue.
// To make sure a queue isn't used from different threads simultaneously we'll calculate the overlap where 0
// means there's no overlap and being equal or greater than the number of available queues means complete overlap.
// If a modded index is less than the overlap number then it needs to use a mutex from an array the size of
// the available mutexes (which will be left empty if overlap is 0 as an optimization). 
// This should overall be fairly optimal assuming jobs are well distributed across workers. Since we're not using
// a pool of queues/mutexes (which I think would incur higher overhead) its possible for two threads to contest
// when another queue is free.
// There's one more consideration: The first queue is being given to the renderer, so if at all possible we don't
// want that queue locked up, slowing down the refresh rate. Therefore, in the case where overlap is below
// availableQueues, we'd like to skip over index 0, which complicates calculating the modded index a bit.
void JobManager::init() {
	size_t numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

	// Create threads
	workerThreads.reserve(numThreads);
	for (size_t i = 0; i < numThreads; i++) {
		Worker* worker = new Worker(engine);
		workerThreads.emplace_back(worker);
		// Make every odd worker not steal persistent jobs,
		// to help ensure there's generally workers idling when each frame starts
		// (otherwise that frame would have to wait for potentially long tasks to finish,
		// causing a stutter)
		if (i % 2 == 1) worker->stealPersistent = false;
	}

	// Calculate overlap
	size_t availableQueues = engine->device->queueFamilyIndices.graphicsQueueCount;
	size_t desiredQueues = numThreads + 3; // 1 for engine and 2 for worlds
	// Ternary to prevent underflow
	overlap = availableQueues > desiredQueues ? 0 : desiredQueues - availableQueues;

	// Create our queue mutexes (mutices?)
	if (overlap != 0) {
		size_t numLocks = std::min(availableQueues, (size_t)overlap);
		queueLocks.resize(numLocks);
		for (uint32_t i = 0; i < numLocks; i++)
			queueLocks[i] = new std::mutex();
	}

	for (size_t i = 0; i < numThreads; i++) {
		// If overlap is 1, then our max queue index should be the amount we requested in Device.cpp
		uint32_t queueIndex = getQueueIndex(i + 3, overlap > 0 ? availableQueues : desiredQueues);
		workerThreads[i]->init(queueIndex, getQueueLock(queueIndex));
		workerThreads[i]->start();
	}
}

uint32_t JobManager::getQueueIndex(uint32_t desiredIndex, uint32_t maxQueues) {
	// The first couple indices are the most important, so allocate queues from the back first
	// This means inverting the index (by doing max - index - 1), unless the index is higher than the max
	if (desiredIndex >= maxQueues)
		return desiredIndex % maxQueues;
	return maxQueues - (desiredIndex % maxQueues) - 1;
}

std::mutex* JobManager::getQueueLock(uint32_t queueIndex) {
	if (queueIndex < overlap) return queueLocks[queueIndex];
	return nullptr;
}

void JobManager::resetFrame() {
	for (auto worker : workerThreads)
		worker->resetFrame();
}

void JobManager::windowRefresh() {
	for (auto worker : workerThreads)
		worker->createInheritanceInfo();
}

void JobManager::cleanup() {
	for (Worker* worker : workerThreads)
		worker->cleanup();
}
