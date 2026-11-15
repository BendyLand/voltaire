/**************************************************************************/
/*  rasterizer_dummy.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "servers/rendering/dummy/rasterizer_dummy.h"

#include "servers/display/display_server.h"
#include "servers/rendering/dummy/rasterizer_canvas_dummy.h"
#include "servers/rendering/dummy/rasterizer_scene_dummy.h"
#include "servers/rendering/dummy/storage/material_storage.h"
#include "servers/rendering/dummy/storage/particles_storage.h"
#include "servers/rendering/dummy/storage/texture_storage.h"
#include "servers/rendering/dummy/storage/utilities.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/storage/particles_storage.h"

void RasterizerDummy::initialize()
{
	RendererDummy::Utilities::initialize();
	RendererDummy::TextureStorage::initialize();
	RendererDummy::MaterialStorage::initialize();
	RendererDummy::MeshStorage::initialize();
	RendererDummy::LightStorage::initialize();
	RendererDummy::ParticlesStorage::initialize();
	RendererDummy::GI::initialize();
	RendererDummy::Fog::initialize();
	RasterizerCanvasDummy::initialize();

	if (!scene) {
		scene = memnew(RasterizerSceneDummy);
	}
	if (Renderer::scene) {
		Renderer::scene->set_scene_render(scene);
	}
}

void RasterizerDummy::begin_frame(double frame_step)
{
	frame++;
	delta = frame_step;
	time += frame_step;

	if (RendererCanvasRender::set_time) {
		RendererCanvasRender::set_time(time);
	}
	if (scene) {
		scene->set_time(time, frame_step);
	}
}

void RasterizerDummy::end_frame(bool p_present)
{
	if (p_present) {
		DisplayServer::get_singleton()->swap_buffers();
	}
}

void RasterizerDummy::finalize()
{
	if (Renderer::scene && Renderer::scene->scene_render == scene) {
		Renderer::scene->scene_render = nullptr;
	}
	if (scene) {
		memdelete(scene);
		scene = nullptr;
	}

	RasterizerCanvasDummy::finalize();
	RendererDummy::Fog::finalize();
	RendererDummy::GI::finalize();
	RendererDummy::ParticlesStorage::finalize();
	RendererDummy::LightStorage::finalize();
	RendererDummy::MeshStorage::finalize();
	RendererDummy::MaterialStorage::finalize();
	RendererDummy::TextureStorage::finalize();
	RendererDummy::Utilities::finalize();
}

void RasterizerDummy::make_current()
{
	RendererCompositor::low_end = false;
	RendererCompositor::bind_compositor<RasterizerDummy>();
}

void RasterizerDummy::bind_texture_storage()
{
	RendererTextureStorage::bind<RendererDummy::TextureStorage>();
}

void RasterizerDummy::bind_material_storage()
{
	RendererMaterialStorage::bind<RendererDummy::MaterialStorage>();
}

void RasterizerDummy::bind_canvas()
{
	RendererCanvasRender::bind<RasterizerCanvasDummy>();
	RendererCanvasRender::initialize = &RasterizerCanvasDummy::initialize;
	RendererCanvasRender::finalize = &RasterizerCanvasDummy::finalize;
	RendererCanvasRender::set_time = &RasterizerCanvasDummy::set_time;
	RendererCanvasRender::update = &RasterizerCanvasDummy::update;
	RendererCanvasRender::free = &RasterizerCanvasDummy::free;
}

void RasterizerDummy::bind_particles_storage()
{
	RendererParticlesStorage::bind<RendererDummy::ParticlesStorage>();
}

void RasterizerDummy::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&RendererDummy::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&RendererDummy::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free =
		&RendererDummy::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&RendererDummy::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&RendererDummy::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call =
		&RendererDummy::Utilities::visibility_notifier_call;

	RendererUtilities::free = &RendererDummy::Utilities::free;
	RendererUtilities::get_base_type = &RendererDummy::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &RendererDummy::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin =
		&RendererDummy::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &RendererDummy::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&RendererDummy::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&RendererDummy::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&RendererDummy::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&RendererDummy::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
		&RendererDummy::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &RendererDummy::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&RendererDummy::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &RendererDummy::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &RendererDummy::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &RendererDummy::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &RendererDummy::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor =
		&RendererDummy::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &RendererDummy::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&RendererDummy::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size =
		&RendererDummy::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings =
		&RendererDummy::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&RendererDummy::Utilities::get_maximum_uniform_buffer_size;
}

RendererSceneRender* RasterizerDummy::get_scene() { return scene; }

RendererDummy::Utilities* RasterizerDummy::get_utilities() { return utilities; }


