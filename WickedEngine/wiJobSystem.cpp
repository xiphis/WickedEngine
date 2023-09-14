#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBacklog.h"
#include "wiPlatform.h"
#include "wiTimer.h"

#include <memory>
#include <algorithm>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <oneapi/tbb/task_arena.h>

#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_PS5
#include "wiJobSystem_PS5.h"
#endif // PLATFORM_PS5

namespace wi::jobsystem
{
	struct Job
	{
		std::function<void(JobArgs)> task;
		context* ctx;
		uint32_t groupID;
		uint32_t groupJobOffset;
		uint32_t groupJobEnd;
		uint32_t sharedmemory_size;
	};

	// This structure is responsible to stop worker thread loops.
	//	Once this is destroyed, worker threads will be woken up and end their loops.
	struct InternalState
	{
		uint32_t numCores = 0;
		uint32_t numThreads = 0;
		std::unique_ptr<oneapi::tbb::task_arena> arena;
		void ShutDown()
		{
			if (arena) {
				if (arena->is_active()) {
					arena->terminate();
				}
			}
		}
		~InternalState()
		{
			ShutDown();
		}
	} static internal_state;

	context::~context() noexcept {
		if (IsBusy(*this)) {
			wi::backlog::post_backlog("wi::jobsystem::context destruction of busy context", wi::backlog::LogLevel::Fatal);
		}
		Wait(*this);
	}

	void Initialize(uint32_t maxThreadCount)
	{
		if (internal_state.numThreads > 0)
			return;
		maxThreadCount = std::max(1u, maxThreadCount);

		wi::Timer timer;

		// Retrieve the number of hardware threads in this system:
		internal_state.numCores = std::thread::hardware_concurrency();
		uint32_t default_concurrency = (uint32_t) tbb::info::default_concurrency();
		internal_state.numThreads = std::min(maxThreadCount, std::max(1u, default_concurrency));

		// Calculate the actual number of worker threads we want (-1 main thread):
		internal_state.arena.reset(new oneapi::tbb::task_arena(internal_state.numThreads));

		internal_state.arena->initialize();

		wi::backlog::post_backlog("wi::jobsystem Initialized with [" + std::to_string(internal_state.numCores) + " cores] [" + std::to_string(internal_state.numThreads) + " max concurrency] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	void ShutDown()
	{
		internal_state.ShutDown();
	}

	uint32_t GetThreadCount()
	{
		return internal_state.numThreads;
	}

    static void Execute(Job &job) {
		internal_state.arena->execute([job] {
			job.ctx->group.run([job] {
				JobArgs args;
				args.groupID = job.groupID;
				if (job.sharedmemory_size > 0)
				{
					thread_local static wi::vector<uint8_t> shared_allocation_data;
					shared_allocation_data.reserve(job.sharedmemory_size);
					args.sharedmemory = shared_allocation_data.data();
				}
				else
				{
					args.sharedmemory = nullptr;
				}

				for (uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
				{
					args.jobIndex = j;
					args.groupIndex = j - job.groupJobOffset;
					args.isFirstJobInGroup = (j == job.groupJobOffset);
					args.isLastJobInGroup = (j == job.groupJobEnd - 1);
					job.task(args);
				}

				job.ctx->counter.fetch_sub(1);
			});
		});
	}

	void Execute(context& ctx, const std::function<void(JobArgs)>& task)
	{
		// Context state is updated:
		ctx.counter.fetch_add(1);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.groupID = 0;
		job.groupJobOffset = 0;
		job.groupJobEnd = 1;
		job.sharedmemory_size = 0;

		Execute(job);
	}

	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobArgs)>& task, size_t sharedmemory_size)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}

		const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

		// Context state is updated:
		ctx.counter.fetch_add(groupCount);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.sharedmemory_size = (uint32_t)sharedmemory_size;

		for (uint32_t groupID = 0; groupID < groupCount; ++groupID)
		{
			// For each group, generate one real job:
			job.groupID = groupID;
			job.groupJobOffset = groupID * groupSize;
			job.groupJobEnd = std::min(job.groupJobOffset + groupSize, jobCount);

			Execute(job);
		}
	}

	uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
	{
		// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
		return (jobCount + groupSize - 1) / groupSize;
	}

	bool IsBusy(const context& ctx)
	{
		// Whenever the context label is greater than zero, it means that there is still work that needs to be done
		return ctx.counter.load() > 0;
	}

	void Wait(const context& ctx)
	{
		//if (IsBusy(ctx))
		const_cast<oneapi::tbb::task_group&>(ctx.group).wait();
	}
}
