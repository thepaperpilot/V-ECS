#pragma once

#define SOL_DEFAULT_PASS_ON_ERROR 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

#include <atomic>

namespace vecs {

	static const uint32_t MAX_JOB_COUNT = 4096;
	static const uint32_t MASK = MAX_JOB_COUNT - 1u;

	// Forward Declarations
	class Archetype;
	class LuaVal;
	class Worker;
	class World;

	enum JobType {
		// Used for jobs with no function
		JOB_TYPE_DUMMY,
		// Used for loading worlds
		JOB_TYPE_PREINIT,
		JOB_TYPE_PREINIT_NODE,
		JOB_TYPE_INIT,
		JOB_TYPE_INIT_NODE,
		JOB_TYPE_POSTINIT,
		JOB_TYPE_POSTINIT_NODE,
		JOB_TYPE_FINISH,
		// Used for starting each node each frame
		JOB_TYPE_EXECUTE,
		JOB_TYPE_CASCADE,
		// Lua-created jobs
		JOB_TYPE_PARALLEL,
		JOB_TYPE_NORMAL
	};

	struct ParallelData {
		uint32_t start;
		uint32_t end;
		bool inUse = false;
		bool inBuffer = true;
	};

	struct Job {
	public:
		World* world;
		sol::bytecode function;
		LuaVal* data;
		void* extra; // only used by internal jobs that need something besides the LuaVal. Not passed to function
		bool inBuffer;
		bool persistent;
		JobType type;
		Job* parent;
		std::atomic_uint16_t unfinishedJobs;
		std::atomic_uint16_t continuationCount;
		Job* continuations[15];
		// TODO padding?
	};

	class JobQueue {
	public:
		JobQueue(Worker* worker) { this->worker = worker; }

		void push(Job* job);
		Job* steal();
		Job* pop();

	private:
		Job* queue[MAX_JOB_COUNT];
		volatile long bottom = 0;
		volatile long top = 0;

		Worker* worker;
	};
}
