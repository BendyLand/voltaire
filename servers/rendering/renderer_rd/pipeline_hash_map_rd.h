/**************************************************************************/
/*  pipeline_hash_map_rd.h                                                */
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

#pragma once

#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_enums.h"

#define PRINT_PIPELINE_COMPILATION_KEYS 0

template <typename Key, typename CreationClass, typename CreationFunction> class PipelineHashMapRD
{
private:
	CreationClass* creation_object = nullptr;
	CreationFunction creation_function = nullptr;
	Mutex* compilations_mutex = nullptr;
	uint32_t* compilations = nullptr;
	RBMap<uint32_t, RID> hash_map;
	LocalVector<Pair<uint32_t, RID>> compiled_queue;
	Mutex compiled_queue_mutex;
	RBSet<uint32_t> compilation_set;
	Mutex local_mutex;

public:
	void add_compiled_pipeline(uint32_t p_hash, RID p_pipeline)
	{
		compiled_queue_mutex.lock();
		compiled_queue.push_back({p_hash, p_pipeline});
		compiled_queue_mutex.unlock();
	}

	// Start compilation of a pipeline ahead of time in the background. Returns true if the
	// compilation was started, false if it wasn't required. Source is only used for collecting
	// statistics.
	void compile_pipeline(
		const Key& p_key, uint32_t p_key_hash, RSE::PipelineSource p_source, bool p_high_priority)
	{
		DEV_ASSERT(
			(creation_object != nullptr) && (creation_function != nullptr) &&
			"Creation object and function was not set before attempting to compile a pipeline.");

		MutexLock local_lock(local_mutex);
		if (compilation_set.has(p_key_hash)) {
			// Check if the pipeline was already submitted.
			return;
		}

		// Record the pipeline as submitted, a task can't be started for it again.
		compilation_set.insert(p_key_hash);

		if (compilations_mutex != nullptr) {
			MutexLock compilations_lock(*compilations_mutex);
			compilations[p_source]++;
		}

#if PRINT_PIPELINE_COMPILATION_KEYS
		String source_name = "UNKNOWN";
		switch (p_source) {
		case RSE::PIPELINE_SOURCE_CANVAS:
			source_name = "CANVAS";
			break;
		case RSE::PIPELINE_SOURCE_MESH:
			source_name = "MESH";
			break;
		case RSE::PIPELINE_SOURCE_SURFACE:
			source_name = "SURFACE";
			break;
		case RSE::PIPELINE_SOURCE_DRAW:
			source_name = "DRAW";
			break;
		case RSE::PIPELINE_SOURCE_SPECIALIZATION:
			source_name = "SPECIALIZATION";
			break;
		}

		print_line("HASH:", p_key_hash, "SOURCE:", source_name);
#endif
	}

	// Set the external pipeline compilations array to increase the counters on every time a
	// pipeline is compiled.
	void set_compilations(uint32_t* p_compilations, Mutex* p_compilations_mutex)
	{
		compilations = p_compilations;
		compilations_mutex = p_compilations_mutex;
	}

	void set_creation_object_and_function(
		CreationClass* p_creation_object, CreationFunction p_creation_function)
	{
		creation_object = p_creation_object;
		creation_function = p_creation_function;
	}

	PipelineHashMapRD() {}

	~PipelineHashMapRD() {}
};


