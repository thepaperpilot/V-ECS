#include "JobBindings.h"

#include "../ecs/Archetype.h"
#include "../jobs/JobQueue.h"
#include "../jobs/Worker.h"
#include "../lua/LuaVal.h"

using namespace vecs;

Job* createParallel(Worker* worker, sol::function jobFunction, LuaVal* data, Archetype* archetype, double maxEntityCount, uint32_t start, uint32_t end) {
	archetype->mutex.lock_shared();
	// Find size of entities
	auto it = archetype->entities.begin();
	while (*it < start)
		it++;

	auto beginIt = it;
	uint32_t numEntities = 0;
	while (it != archetype->entities.end() && *it < end) {
		it++;
		numEntities++;
	}
	it = beginIt;

	uint32_t numJobs = ceil(numEntities / maxEntityCount);

	// Create the root job
	Job* rootJob = worker->allocateJob();
	rootJob->type = JOB_TYPE_DUMMY;
	rootJob->world = worker->getWorld();
	rootJob->unfinishedJobs = 1 + numJobs;
	rootJob->persistent = false;
	rootJob->parent = nullptr;
	rootJob->continuationCount = 0;

	uint32_t i = 0;
	World* world = worker->getWorld();
	while (i < numEntities) {
		// Calculate the number of entities to handle in this sub-job
		uint32_t size = ceil(numEntities - i) / numJobs;
		numJobs--;

		ParallelData* parData = worker->allocateParallelData();
		parData->start = *it;
		// I was getting occasional crashes with de-referencing *it
		// when using std::advance(it, size);
		for (int i = 0; i < size && it != std::prev(archetype->entities.end()); i++)
			it++;
		parData->end = *it;
		i += size;

		// Create the subjob
		Job* job = worker->allocateJob();
		job->function = jobFunction.dump();
		// Copy data to job
		// TODO avoid creating so many allocations?
		assert(data->type == LUA_TYPE_TABLE);
		job->data = new LuaVal(std::get<LuaVal::MapType*>(data->value));
		job->parent = rootJob;
		job->extra = parData;
		job->type = JOB_TYPE_PARALLEL;
		job->unfinishedJobs = 1;
		job->persistent = false;
		job->continuationCount = 0;
		job->world = world;

		worker->pushJob(job);
	}
	archetype->mutex.unlock_shared();

	// TODO do a binary split

	return rootJob;
}

void vecs::JobBindings::setupState(sol::state& lua, Worker* worker) {
	// We have different types of jobs which take different inputs and will have different execution methods
	// But essentially each of these do the following:
	// - allocate job
	// - setup job values
	//		since our allocated job may have lingering data from past job, we set/reset each value
	// - return the job
	//		note we don't start the job, allowing the user to call setParent or anything else they need to do,
	//		before calling submit which will actually add it to this worker's job queue
	// Additionally we also have variants of each job that will take an EnityQuery. That will create a job that
	//  will run on every entity in that query, splitting the entity list in half for efficient work-stealing
	// When submitting jobs you can also specify that they should be an "extended" job,
	//  which will got to a special worker that prioritizes extended jobs, from its own allocator ring.
	//  Extended jobs cannot be stolen away from that worker, nor will they prevent the system or renderer that submitted
	//  the job to be marked as incomplete. That means they can take longer than one frame and won't lock up rendering. 
	//  These are intended for "background" work like asynchronously loading content. 
	lua["jobs"] = lua.create_table_with(
		"create", sol::overload(
			[worker](sol::function jobFunction, LuaVal* data) -> Job* {
				Job* job = worker->allocateJob();

				job->function = jobFunction.dump();
				// Copy data to job
				// TODO avoid creating so many allocations?
				assert(data->type == LUA_TYPE_TABLE);
				job->data = new LuaVal(std::get<LuaVal::MapType*>(data->value));
				job->parent = nullptr;
				job->type = JOB_TYPE_NORMAL;
				job->unfinishedJobs = 1;
				job->persistent = false;
				job->world = worker->getWorld();
				job->continuationCount = 0;

				return job;
			},
			[worker]() -> Job* {
				Job* job = worker->allocateJob();
				job->type = JOB_TYPE_DUMMY;
				job->world = worker->getWorld();
				job->unfinishedJobs = 1;
				job->persistent = false;
				job->parent = nullptr;
				job->continuationCount = 0;
				return job;
			}
				),
		"createParallel", sol::overload(
			[worker](sol::function jobFunction, LuaVal* data, Archetype* archetype, double maxEntityCount) -> Job* {
				if (archetype->numEntities > 0)
					return createParallel(worker, jobFunction, data, archetype, maxEntityCount, *archetype->entities.begin(), *archetype->entities.rbegin());
				else {
					Job* job = worker->allocateJob();
					job->type = JOB_TYPE_DUMMY;
					job->parent = nullptr;
					job->unfinishedJobs = 1;
					job->persistent = false;
					job->continuationCount = 0;
					return job;
				}
			},
			[worker](sol::function jobFunction, LuaVal* data, Archetype* archetype, double maxEntityCount, uint32_t start, uint32_t end) -> Job* {
				return createParallel(worker, jobFunction, data, archetype, maxEntityCount, start, end);
			}
		)
	);
	lua.new_usertype<Job>("job",
		sol::no_constructor,
		"setParent", [worker](Job* job, Job* parent) {
			if (job->parent != nullptr)
				worker->finish(job->parent);
			parent->unfinishedJobs++;
			job->parent = parent;
		},
		"submit", [worker](Job* job) {
			if (job->parent == nullptr && !job->persistent) {
				worker->job->unfinishedJobs++;
				job->parent = worker->job;
			}
			worker->pushJob(job);
		},
		"persistent", sol::property(&Job::persistent)
	);
}
