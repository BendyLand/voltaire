/**************************************************************************/
/*  utilities.h                                                           */
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

#include "servers/rendering/storage/utilities.h"

namespace RendererDummy
{

class Utilities final
{
public:
    Utilities() = delete;
    Utilities(const Utilities&) = delete;
    Utilities& operator=(const Utilities&) = delete;
    ~Utilities() = delete;

    static void initialize() {}
    static void finalize() {}

    static RSE::InstanceType get_base_type(RID p_rid);
    static bool free(RID p_rid);

    static void base_update_dependency(RID p_base, DependencyTracker* p_instance);

    static RID visibility_notifier_allocate() { return RID(); }
    static void visibility_notifier_initialize(RID p_notifier) {}
    static void visibility_notifier_free(RID p_notifier) {}
    static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb) {}
    static AABB visibility_notifier_get_aabb(RID p_notifier) { return AABB(); }
    static void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred) {}

    static void capture_timestamps_begin() {}
    static void capture_timestamp(const String& p_name) {}
    static uint32_t get_captured_timestamps_count() { return 0; }
    static uint64_t get_captured_timestamps_frame() { return 0; }
    static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) { return 0; }
    static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) { return 0; }
    static String get_captured_timestamp_name(uint32_t p_index) { return String(); }

    static void update_dirty_resources() {}
    static void set_debug_generate_wireframes(bool p_generate) {}

    static bool has_os_feature(const String& p_feature)
    {
        return p_feature == "rgtc" || p_feature == "bptc" || p_feature == "s3tc" || p_feature == "etc2";
    }

    static void update_memory_info() {}

    static uint64_t get_rendering_info(RSE::RenderingInfo p_info) { return 0; }

    static String get_video_adapter_name() { return String(); }
    static String get_video_adapter_vendor() { return String(); }
    static RenderingDeviceEnums::DeviceType get_video_adapter_type()
    {
        return RenderingDeviceEnums::DeviceType::DEVICE_TYPE_OTHER;
    }
    static String get_video_adapter_api_version() { return String(); }

    static Size2i get_maximum_viewport_size() { return Size2i(); }

    static uint32_t get_maximum_shader_varyings()
    {
        return 31;
    }

    static uint64_t get_maximum_uniform_buffer_size()
    {
        return 65536;
    }
};

} // namespace RendererDummy
