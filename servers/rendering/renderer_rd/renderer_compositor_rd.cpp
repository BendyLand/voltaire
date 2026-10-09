/**************************************************************************/
/*  renderer_compositor_rd.cpp                                            */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "renderer_compositor_rd.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.h"
#include "servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.h"
#include "servers/rendering/renderer_rd/storage_rd/utilities.h"
#include "servers/rendering/rendering_server_types.h"

void RendererCompositorRD::blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
{
	Error err = RD::screen_prepare_for_drawing(p_screen);
	if (err != OK) {
		// Window is minimized and does not have valid swapchain, skip drawing without printing
		// errors.
		return;
	}

	BlitPipelines blit_pipelines =
		_get_blit_pipelines_for_format(RD::screen_get_framebuffer_format(p_screen));

	RD::DrawListID draw_list = RD::draw_list_begin_for_screen(p_screen);
	ERR_FAIL_COND(draw_list == RDC::INVALID_ID);

	const RDC::ColorSpace color_space = RD::screen_get_color_space(p_screen);
	const float reference_luminance =
		RD::get_context_driver()->window_get_hdr_output_reference_luminance(p_screen);
	const float linear_luminance_scale =
		RD::get_context_driver()->window_get_hdr_output_linear_luminance_scale(p_screen);
	const float output_max_value =
		RD::get_context_driver()->window_get_output_max_linear_value(p_screen);
	const float reference_multiplier =
		_compute_reference_multiplier(color_space, reference_luminance, linear_luminance_scale);

	for (int i = 0; i < p_amount; i++) {
		RID rd_texture =
			texture_storage->render_target_get_rd_texture(p_render_targets[i].render_target);
		ERR_CONTINUE(rd_texture.is_null());

		BlitMode mode = p_render_targets[i].lens_distortion.apply
							? BLIT_MODE_LENS
							: (p_render_targets[i].multi_view.use_layer ? BLIT_MODE_USE_LAYER
																		: BLIT_MODE_NORMAL);

		HashMap<RID, RID>::Iterator it = render_target_descriptors.find(rd_texture);

		Size2 screen_size(RD::screen_get_width(p_screen), RD::screen_get_height(p_screen));

		RD::draw_list_bind_render_pipeline(draw_list, blit_pipelines.pipelines[mode]);
		RD::draw_list_bind_index_array(draw_list, blit->array);
		RD::draw_list_bind_uniform_set(draw_list, it->value, 0);

		// We need to invert the phone rotation.
		const int screen_rotation_degrees = -RD::screen_get_pre_rotation_degrees(p_screen);
		float screen_rotation = Math::deg_to_rad((float)screen_rotation_degrees);

		blit->push_constant.rotation_cos = Math::cos(screen_rotation);
		blit->push_constant.rotation_sin = Math::sin(screen_rotation);
		// Swap width and height when the orientation is not the native one.
		if (screen_rotation_degrees % 180 != 0) {
			SWAP(screen_size.width, screen_size.height);
		}
		blit->push_constant.src_rect[0] = p_render_targets[i].src_rect.position.x;
		blit->push_constant.src_rect[1] = p_render_targets[i].src_rect.position.y;
		blit->push_constant.src_rect[2] = p_render_targets[i].src_rect.size.width;
		blit->push_constant.src_rect[3] = p_render_targets[i].src_rect.size.height;
		blit->push_constant.dst_rect[0] =
			p_render_targets[i].dst_rect.position.x / screen_size.width;
		blit->push_constant.dst_rect[1] =
			p_render_targets[i].dst_rect.position.y / screen_size.height;
		blit->push_constant.dst_rect[2] =
			p_render_targets[i].dst_rect.size.width / screen_size.width;
		blit->push_constant.dst_rect[3] =
			p_render_targets[i].dst_rect.size.height / screen_size.height;
		blit->push_constant.layer = p_render_targets[i].multi_view.layer;
		blit->push_constant.eye_center[0] = p_render_targets[i].lens_distortion.eye_center.x;
		blit->push_constant.eye_center[1] = p_render_targets[i].lens_distortion.eye_center.y;
		blit->push_constant.k1 = p_render_targets[i].lens_distortion.k1;
		blit->push_constant.k2 = p_render_targets[i].lens_distortion.k2;
		blit->push_constant.upscale = p_render_targets[i].lens_distortion.upscale;
		blit->push_constant.aspect_ratio = p_render_targets[i].lens_distortion.aspect_ratio;
		blit->push_constant.source_is_srgb =
			!texture_storage->render_target_is_using_hdr(p_render_targets[i].render_target);
		blit->push_constant.use_debanding =
			texture_storage->render_target_is_using_debanding(p_render_targets[i].render_target);
		blit->push_constant.target_color_space = color_space;
		blit->push_constant.reference_multiplier = reference_multiplier;
		blit->push_constant.output_max_value = output_max_value;

		RD::draw_list_set_push_constant(draw_list, &blit->push_constant, sizeof(BlitPushConstant));
		RD::draw_list_draw(draw_list, true);
	}

	RD::draw_list_end();
}

void RendererCompositorRD::begin_frame(double frame_step)
{
	frame++;
	delta = frame_step;
	time += frame_step;

	double time_roll_over = GLOBAL_GET_CACHED(double, "rendering/limits/time/time_rollover_secs");
	time = Math::fmod(time, time_roll_over);

	canvas->set_time(time);
	scene->set_time(time, frame_step);
}

void RendererCompositorRD::end_frame(bool p_present) { RD::swap_buffers(p_present); }

void RendererCompositorRD::initialize()
{
	RendererRD::Utilities::initialize();
	{
		blit = memnew(Blit);
		Vector<String> blit_modes;
		blit_modes.push_back("\n");
		blit_modes.push_back("\n#define USE_LAYER\n");
		blit_modes.push_back("\n#define USE_LAYER\n#define APPLY_LENS_DISTORTION\n");
		blit_modes.push_back("\n");

		blit->shader.initialize(blit_modes);
		blit->shader_version = blit->shader.version_create();

		// create index array for copy shader
		Vector<uint8_t> pv;
		pv.resize(6 * 2);
		{
			uint8_t* w = pv.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			p16[0] = 0;
			p16[1] = 1;
			p16[2] = 2;
			p16[3] = 0;
			p16[4] = 2;
			p16[5] = 3;
		}
		blit->index_buffer = RD::index_buffer_create(6, RDC::INDEX_BUFFER_FORMAT_UINT16, pv);
		blit->array = RD::index_array_create(blit->index_buffer, 0, 6);
		blit->sampler = RD::sampler_create(RDC::SamplerState());
	}
}

void RendererCompositorRD::finalize()
{
	texture_storage->_tex_blit_shader_free();
	memdelete(canvas);
	memdelete(fog);
	memdelete(particles_storage);
	memdelete(light_storage);
	memdelete(material_storage);
	memdelete(texture_storage);
	RendererRD::Utilities::finalize();

	if (blit) {
		memdelete(blit);
		blit = nullptr;
	}
	// only need to erase these, the rest are erased by cascade
	blit->shader.version_free(blit->shader_version);
	RD::free_rid(blit->index_buffer);
	RD::free_rid(blit->sampler);
}

float RendererCompositorRD::_compute_reference_multiplier(RDC::ColorSpace p_color_space,
	const float p_reference_luminance, const float p_linear_luminance_scale)
{
	switch (p_color_space) {
	case RDC::COLOR_SPACE_REC709_LINEAR:
		return p_reference_luminance / p_linear_luminance_scale;
	default:
		return 1.0f;
	}
}

void RendererCompositorRD::set_boot_image_with_stretch(const Ref<Image>& p_image,
	const Color& p_color, RSE::SplashStretchMode p_stretch_mode, bool p_use_filter)
{
	if (p_image.is_null() || p_image->is_empty()) {
		return;
	}

	Error err = RD::screen_prepare_for_drawing(DisplayServerEnums::MAIN_WINDOW_ID);
	if (err != OK) {
		// Window is minimized and does not have valid swapchain, skip drawing without printing
		// errors.
		return;
	}

	BlitPipelines blit_pipelines = _get_blit_pipelines_for_format(
		RD::screen_get_framebuffer_format(DisplayServerEnums::MAIN_WINDOW_ID));

	RID texture = texture_storage->texture_allocate();
	texture_storage->texture_2d_initialize(texture, p_image);
	RID rd_texture = texture_storage->texture_get_rd_texture(texture, false);

	RDC::SamplerState sampler_state;
	sampler_state.min_filter =
		p_use_filter ? RDC::SAMPLER_FILTER_LINEAR : RDC::SAMPLER_FILTER_NEAREST;
	sampler_state.mag_filter =
		p_use_filter ? RDC::SAMPLER_FILTER_LINEAR : RDC::SAMPLER_FILTER_NEAREST;
	sampler_state.max_lod = 0;
	RID sampler = RD::sampler_create(sampler_state);

	RID uset;

	Size2 window_size = DisplayServer::get_singleton()->window_get_size();

	Rect2 screenrect = RenderingServerTypes::get_splash_stretched_screen_rect(
		p_image->get_size(), window_size, p_stretch_mode);
	screenrect.position /= window_size;
	screenrect.size /= window_size;

	const RDC::ColorSpace color_space =
		RD::screen_get_color_space(DisplayServerEnums::MAIN_WINDOW_ID);
	const float reference_luminance =
		RD::get_context_driver()->window_get_hdr_output_reference_luminance(
			DisplayServerEnums::MAIN_WINDOW_ID);
	const float linear_luminance_scale =
		RD::get_context_driver()->window_get_hdr_output_linear_luminance_scale(
			DisplayServerEnums::MAIN_WINDOW_ID);
	const float output_max_value = RD::get_context_driver()->window_get_output_max_linear_value(
		DisplayServerEnums::MAIN_WINDOW_ID);
	const float reference_multiplier =
		_compute_reference_multiplier(color_space, reference_luminance, linear_luminance_scale);

	Color clear_color = p_color;
	if (color_space != RDC::COLOR_SPACE_REC709_NONLINEAR_SRGB) {
		// draw_list_begin_for_screen requires linear-encoded Color when using an HDR buffer.
		clear_color = p_color.srgb_to_linear();

		clear_color.r *= reference_multiplier;
		clear_color.g *= reference_multiplier;
		clear_color.b *= reference_multiplier;
	}

	RD::DrawListID draw_list =
		RD::draw_list_begin_for_screen(DisplayServerEnums::MAIN_WINDOW_ID, clear_color);

	RD::draw_list_bind_render_pipeline(draw_list, blit_pipelines.pipelines[BLIT_MODE_NORMAL_ALPHA]);
	RD::draw_list_bind_index_array(draw_list, blit->array);
	RD::draw_list_bind_uniform_set(draw_list, uset, 0);

	const int screen_rotation_degrees =
		-RD::screen_get_pre_rotation_degrees(DisplayServerEnums::MAIN_WINDOW_ID);
	float screen_rotation = Math::deg_to_rad((float)screen_rotation_degrees);
	blit->push_constant.rotation_cos = Math::cos(screen_rotation);
	blit->push_constant.rotation_sin = Math::sin(screen_rotation);
	blit->push_constant.src_rect[0] = 0.0;
	blit->push_constant.src_rect[1] = 0.0;
	blit->push_constant.src_rect[2] = 1.0;
	blit->push_constant.src_rect[3] = 1.0;
	blit->push_constant.dst_rect[0] = screenrect.position.x;
	blit->push_constant.dst_rect[1] = screenrect.position.y;
	blit->push_constant.dst_rect[2] = screenrect.size.width;
	blit->push_constant.dst_rect[3] = screenrect.size.height;
	blit->push_constant.layer = 0;
	blit->push_constant.eye_center[0] = 0;
	blit->push_constant.eye_center[1] = 0;
	blit->push_constant.k1 = 0;
	blit->push_constant.k2 = 0;
	blit->push_constant.upscale = 1.0;
	blit->push_constant.aspect_ratio = 1.0;
	blit->push_constant.source_is_srgb = true;
	blit->push_constant.use_debanding = false;
	blit->push_constant.target_color_space = color_space;
	blit->push_constant.reference_multiplier = reference_multiplier;
	blit->push_constant.output_max_value = output_max_value;

	RD::draw_list_set_push_constant(draw_list, &blit->push_constant, sizeof(BlitPushConstant));
	RD::draw_list_draw(draw_list, true);

	RD::draw_list_end();

	RD::swap_buffers(true);

	texture_storage->texture_free(texture);
	RD::free_rid(sampler);
}

RendererCompositorRD::BlitPipelines RendererCompositorRD::_get_blit_pipelines_for_format(long)
{
	return RendererCompositorRD::BlitPipelines();
}

void RendererCompositorRD::bind_mesh_storage()
{
	RendererMeshStorage::mesh_allocate = &RendererRD::MeshStorage::mesh_allocate;
	RendererMeshStorage::mesh_initialize = &RendererRD::MeshStorage::mesh_initialize;
	RendererMeshStorage::mesh_free = &RendererRD::MeshStorage::mesh_free;
	RendererMeshStorage::mesh_set_blend_shape_count =
		&RendererRD::MeshStorage::mesh_set_blend_shape_count;
	RendererMeshStorage::mesh_needs_instance = &RendererRD::MeshStorage::mesh_needs_instance;
	RendererMeshStorage::mesh_add_surface = &RendererRD::MeshStorage::mesh_add_surface;
	RendererMeshStorage::mesh_get_blend_shape_count =
		&RendererRD::MeshStorage::mesh_get_blend_shape_count;
	RendererMeshStorage::mesh_set_blend_shape_mode =
		&RendererRD::MeshStorage::mesh_set_blend_shape_mode;
	RendererMeshStorage::mesh_get_blend_shape_mode =
		&RendererRD::MeshStorage::mesh_get_blend_shape_mode;
	RendererMeshStorage::mesh_surface_update_vertex_region =
		&RendererRD::MeshStorage::mesh_surface_update_vertex_region;
	RendererMeshStorage::mesh_surface_update_attribute_region =
		&RendererRD::MeshStorage::mesh_surface_update_attribute_region;
	RendererMeshStorage::mesh_surface_update_skin_region =
		&RendererRD::MeshStorage::mesh_surface_update_skin_region;
	RendererMeshStorage::mesh_surface_update_index_region =
		&RendererRD::MeshStorage::mesh_surface_update_index_region;
	RendererMeshStorage::mesh_surface_set_material =
		&RendererRD::MeshStorage::mesh_surface_set_material;
	RendererMeshStorage::mesh_surface_get_material =
		&RendererRD::MeshStorage::mesh_surface_get_material;
	RendererMeshStorage::mesh_get_surface = [](RID p_mesh, int p_surface) {
		return RendererRD::MeshStorage::mesh_get_surface(p_mesh, p_surface);
	};
	RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid =
		&RendererRD::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid =
		&RendererRD::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid =
		&RendererRD::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid =
		&RendererRD::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
	RendererMeshStorage::mesh_get_surface_count = &RendererRD::MeshStorage::mesh_get_surface_count;
	RendererMeshStorage::mesh_set_custom_aabb = &RendererRD::MeshStorage::mesh_set_custom_aabb;
	RendererMeshStorage::mesh_get_custom_aabb = &RendererRD::MeshStorage::mesh_get_custom_aabb;
	RendererMeshStorage::mesh_get_aabb = &RendererRD::MeshStorage::mesh_get_aabb;
	RendererMeshStorage::mesh_set_path = &RendererRD::MeshStorage::mesh_set_path;
	RendererMeshStorage::mesh_get_path = &RendererRD::MeshStorage::mesh_get_path;
	RendererMeshStorage::mesh_set_shadow_mesh = &RendererRD::MeshStorage::mesh_set_shadow_mesh;
	RendererMeshStorage::mesh_clear = &RendererRD::MeshStorage::mesh_clear;
	RendererMeshStorage::mesh_surface_remove = &RendererRD::MeshStorage::mesh_surface_remove;
	RendererMeshStorage::mesh_debug_usage = &RendererRD::MeshStorage::mesh_debug_usage;

	RendererMeshStorage::mesh_instance_create = &RendererRD::MeshStorage::mesh_instance_create;
	RendererMeshStorage::mesh_instance_free = &RendererRD::MeshStorage::mesh_instance_free;
	RendererMeshStorage::mesh_instance_set_skeleton =
		&RendererRD::MeshStorage::mesh_instance_set_skeleton;
	RendererMeshStorage::mesh_instance_set_blend_shape_weight =
		&RendererRD::MeshStorage::mesh_instance_set_blend_shape_weight;
	RendererMeshStorage::mesh_instance_check_for_update =
		&RendererRD::MeshStorage::mesh_instance_check_for_update;
	RendererMeshStorage::mesh_instance_set_canvas_item_transform =
		&RendererRD::MeshStorage::mesh_instance_set_canvas_item_transform;
	RendererMeshStorage::update_mesh_instances = &RendererRD::MeshStorage::update_mesh_instances;

	RendererMeshStorage::_multimesh_allocate = &RendererRD::MeshStorage::_multimesh_allocate;
	RendererMeshStorage::_multimesh_initialize = &RendererRD::MeshStorage::_multimesh_initialize;
	RendererMeshStorage::_multimesh_free = &RendererRD::MeshStorage::_multimesh_free;
	RendererMeshStorage::_multimesh_allocate_data =
		&RendererRD::MeshStorage::_multimesh_allocate_data;
	RendererMeshStorage::_multimesh_get_instance_count =
		&RendererRD::MeshStorage::_multimesh_get_instance_count;
	RendererMeshStorage::_multimesh_set_mesh = &RendererRD::MeshStorage::_multimesh_set_mesh;
	RendererMeshStorage::_multimesh_instance_set_transform =
		&RendererRD::MeshStorage::_multimesh_instance_set_transform;
	RendererMeshStorage::_multimesh_instance_set_transform_2d =
		&RendererRD::MeshStorage::_multimesh_instance_set_transform_2d;
	RendererMeshStorage::_multimesh_instance_set_color =
		&RendererRD::MeshStorage::_multimesh_instance_set_color;
	RendererMeshStorage::_multimesh_instance_set_custom_data =
		&RendererRD::MeshStorage::_multimesh_instance_set_custom_data;
	RendererMeshStorage::_multimesh_set_custom_aabb =
		&RendererRD::MeshStorage::_multimesh_set_custom_aabb;
	RendererMeshStorage::_multimesh_get_custom_aabb =
		&RendererRD::MeshStorage::_multimesh_get_custom_aabb;
	RendererMeshStorage::_multimesh_get_mesh = &RendererRD::MeshStorage::_multimesh_get_mesh;
	RendererMeshStorage::_multimesh_instance_get_transform =
		&RendererRD::MeshStorage::_multimesh_instance_get_transform;
	RendererMeshStorage::_multimesh_instance_get_transform_2d =
		&RendererRD::MeshStorage::_multimesh_instance_get_transform_2d;
	RendererMeshStorage::_multimesh_instance_get_color =
		&RendererRD::MeshStorage::_multimesh_instance_get_color;
	RendererMeshStorage::_multimesh_instance_get_custom_data =
		&RendererRD::MeshStorage::_multimesh_instance_get_custom_data;
	RendererMeshStorage::_multimesh_set_buffer = &RendererRD::MeshStorage::_multimesh_set_buffer;
	RendererMeshStorage::_multimesh_get_command_buffer_rd_rid =
		&RendererRD::MeshStorage::_multimesh_get_command_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer_rd_rid =
		&RendererRD::MeshStorage::_multimesh_get_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer = &RendererRD::MeshStorage::_multimesh_get_buffer;
	RendererMeshStorage::_multimesh_set_visible_instances =
		&RendererRD::MeshStorage::_multimesh_set_visible_instances;
	RendererMeshStorage::_multimesh_get_visible_instances =
		&RendererRD::MeshStorage::_multimesh_get_visible_instances;
	RendererMeshStorage::_multimesh_get_aabb = &RendererRD::MeshStorage::_multimesh_get_aabb;
	RendererMeshStorage::_multimesh_get_interpolator =
		&RendererRD::MeshStorage::_multimesh_get_interpolator;

	RendererMeshStorage::skeleton_allocate = &RendererRD::MeshStorage::skeleton_allocate;
	RendererMeshStorage::skeleton_initialize = &RendererRD::MeshStorage::skeleton_initialize;
	RendererMeshStorage::skeleton_free = &RendererRD::MeshStorage::skeleton_free;
	RendererMeshStorage::skeleton_allocate_data = &RendererRD::MeshStorage::skeleton_allocate_data;
	RendererMeshStorage::skeleton_set_base_transform_2d =
		&RendererRD::MeshStorage::skeleton_set_base_transform_2d;
	RendererMeshStorage::skeleton_get_bone_count =
		&RendererRD::MeshStorage::skeleton_get_bone_count;
	RendererMeshStorage::skeleton_bone_set_transform =
		&RendererRD::MeshStorage::skeleton_bone_set_transform;
	RendererMeshStorage::skeleton_bone_get_transform =
		&RendererRD::MeshStorage::skeleton_bone_get_transform;
	RendererMeshStorage::skeleton_bone_set_transform_2d =
		&RendererRD::MeshStorage::skeleton_bone_set_transform_2d;
	RendererMeshStorage::skeleton_bone_get_transform_2d =
		&RendererRD::MeshStorage::skeleton_bone_get_transform_2d;
	RendererMeshStorage::skeleton_update_dependency =
		&RendererRD::MeshStorage::skeleton_update_dependency;
}


