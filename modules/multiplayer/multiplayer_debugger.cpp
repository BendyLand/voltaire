/**************************************************************************/
/*  multiplayer_debugger.cpp                                              */
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
#include "multiplayer_debugger.h"
#include "multiplayer_synchronizer.h"
#include "scene/main/node.h"
#include "scene_replication_config.h"

// BandwidthProfiler

int MultiplayerDebugger::BandwidthProfiler::bandwidth_usage(
	const Vector<BandwidthFrame>& p_buffer, int p_pointer)
{
	ERR_FAIL_COND_V(p_buffer.is_empty(), 0);
	int total_bandwidth = 0;

	uint64_t timestamp = OS::get_singleton()->get_ticks_msec();
	uint64_t final_timestamp = timestamp - 1000;

	int i = (p_pointer + p_buffer.size() - 1) % p_buffer.size();

	while (i != p_pointer && p_buffer[i].packet_size > 0) {
		if (p_buffer[i].timestamp < final_timestamp) {
			return total_bandwidth;
		}
		total_bandwidth += p_buffer[i].packet_size;
		i = (i + p_buffer.size() - 1) % p_buffer.size();
	}

	ERR_FAIL_COND_V_MSG(i == p_pointer, total_bandwidth,
		"Reached the end of the bandwidth profiler buffer, values might be inaccurate.");
	return total_bandwidth;
}


