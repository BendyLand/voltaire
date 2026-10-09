/**************************************************************************/
/*  renderer_compositor.cpp                                               */
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
/* without limitation the rights to use, copy, modify, merge, publish,   */
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

#include "renderer_compositor.h"
#include "servers/display/display_server.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#endif

#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
#endif

#include "servers/rendering/dummy/rasterizer_dummy.h"

#ifndef XR_DISABLED
#include "core/config/project_settings.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

Error RendererCompositor::create()
{
	if (_create_func != nullptr) {
		return _create_func();
	}
#if defined(RD_ENABLED)
	low_end = false;
	bind_compositor<RendererCompositorRD>();
	DisplayServer* ds = DisplayServer::get_singleton();
	if (ds != nullptr) {
		Error err = ds->init_rendering_device();
		ERR_FAIL_COND_V_MSG(
			err != OK, err, "Failed to initialize RenderingDevice via DisplayServer.");
	}
	RendererRD::MeshStorage::initialize();
	RendererCompositorRD::initialize();
	return OK;
#elif defined(GLES3_ENABLED)
	low_end = true;
	bind_compositor<RasterizerGLES3>();
	GLES3::MeshStorage::initialize();
	RasterizerGLES3::initialize();
	return OK;
#else
	low_end = true;
	bind_compositor<RasterizerDummy>();
	RasterizerDummy::initialize();
	return OK;
#endif
}

bool RendererCompositor::is_xr_enabled()
{
#ifndef XR_DISABLED
	return xr_enabled;
#else
	return false;
#endif
}

void RendererCompositorRD::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&RendererRD::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&RendererRD::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free = &RendererRD::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&RendererRD::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&RendererRD::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call = &RendererRD::Utilities::visibility_notifier_call;

	RendererUtilities::free = &RendererRD::Utilities::free;
	RendererUtilities::get_base_type = &RendererRD::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &RendererRD::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin = &RendererRD::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &RendererRD::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&RendererRD::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&RendererRD::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&RendererRD::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&RendererRD::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
		&RendererRD::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &RendererRD::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&RendererRD::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &RendererRD::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &RendererRD::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &RendererRD::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &RendererRD::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor = &RendererRD::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &RendererRD::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&RendererRD::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size =
		&RendererRD::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings =
		&RendererRD::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&RendererRD::Utilities::get_maximum_uniform_buffer_size;
}


