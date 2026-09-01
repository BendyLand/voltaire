/**************************************************************************/
/*  jolt_job_system.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "core/os/os.h"
#include "core/os/time.h"
#include "jolt_job_system.h"
// must stay last
#include <Jolt/Physics/PhysicsSettings.h>

void JoltJobSystem::Job::_execute(void* p_user_data)
{
	Job* job = static_cast<Job*>(p_user_data);

#ifdef DEBUG_ENABLED
	const uint64_t time_start = Time::get_singleton()->get_ticks_usec();
#endif

	job->Execute();

#ifdef DEBUG_ENABLED
	const uint64_t time_end = Time::get_singleton()->get_ticks_usec();
	const uint64_t time_elapsed = time_end - time_start;

	timings_lock.lock();
	timings_by_job[job->name] += time_elapsed;
	timings_lock.unlock();
#endif

	job->Release();
}

JoltJobSystem::Job::Job(const char* p_name, JPH::ColorArg p_color, JPH::JobSystem* p_job_system,
	const JPH::JobSystem::JobFunction& p_job_function, JPH::uint32 p_dependency_count)
	: JPH::JobSystem::Job(p_name, p_color, p_job_system, p_job_function, p_dependency_count)
#ifdef DEBUG_ENABLED
	  ,
	  name(p_name)
#endif
{
}

void JoltJobSystem::Job::push_completed(Job* p_job)
{
	Job* prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		p_job->completed_next = prev_head;
	} while (!completed_head.compare_exchange_weak(
		prev_head, p_job, std::memory_order_release, std::memory_order_relaxed));
}

JoltJobSystem::Job* JoltJobSystem::Job::pop_completed()
{
	Job* prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		if (prev_head == nullptr) {
			return nullptr;
		}
	} while (!completed_head.compare_exchange_weak(prev_head, prev_head->completed_next,
		std::memory_order_acquire, std::memory_order_relaxed));

	return prev_head;
}

int JoltJobSystem::GetMaxConcurrency() const { return thread_count; }

JPH::JobHandle JoltJobSystem::CreateJob(const char* p_name, JPH::ColorArg p_color,
	const JPH::JobSystem::JobFunction& p_job_function, JPH::uint32 p_dependency_count)
{
	Job* job = nullptr;

	while (true) {
		JPH::uint32 job_index =
			jobs.ConstructObject(p_name, p_color, this, p_job_function, p_dependency_count);

		if (job_index != JPH::FixedSizeFreeList<Job>::cInvalidObjectIndex) {
			job = &jobs.Get(job_index);
			break;
		}

		WARN_PRINT_ONCE("Jolt Physics job system exceeded the maximum number of jobs. This should "
						"not happen. Please report this. Waiting for jobs to become available...");

		OS::get_singleton()->delay_usec(100);

		_reclaim_jobs();
	}

	// This will increment the job's reference count, so must happen before we queue the job
	JPH::JobHandle job_handle(job);

	if (p_dependency_count == 0) {
		QueueJob(job);
	}

	return job_handle;
}

void JoltJobSystem::QueueJob(JPH::JobSystem::Job* p_job) { static_cast<Job*>(p_job)->queue(); }

void JoltJobSystem::QueueJobs(JPH::JobSystem::Job** p_jobs, JPH::uint p_job_count)
{
	for (JPH::uint i = 0; i < p_job_count; ++i) {
		QueueJob(p_jobs[i]);
	}
}

void JoltJobSystem::FreeJob(JPH::JobSystem::Job* p_job)
{
	Job::push_completed(static_cast<Job*>(p_job));
}

void JoltJobSystem::_reclaim_jobs()
{
	while (Job* job = Job::pop_completed()) {
		jobs.DestructObject(job);
	}
}

void JoltJobSystem::post_step() { _reclaim_jobs(); }


