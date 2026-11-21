/**************************************************************************/
/*  renderer_compositor_rd.cpp                                            */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "renderer_compositor_rd.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.h"
#include "servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.h"
#include "servers/rendering/renderer_rd/renderer_canvas_render_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/utilities.h"
#include "servers/rendering/rendering_server_types.h"

void RendererCompositorRD::blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
    const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
{
    Error err = RD::screen_prepare_for_drawing(p_screen);
    if (err != OK) {
        return;
    }

    BlitPipelines blit_pipelines =
        _get_blit_pipelines_for_format(RD::screen_get_framebuffer_format(p_screen));

    RD::DrawListID draw_list = RD::draw_list_begin_for_screen(p_screen);
	// magneta overlay
    // RD::DrawListID draw_list = RD::draw_list_begin_for_screen(p_screen, Color(1, 0, 1, 1));
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
        RID rd_texture = RendererRD::TextureStorage::render_target_get_rd_texture(
            p_render_targets[i].render_target);
        ERR_CONTINUE(rd_texture.is_null());

        RDC::TextureFormat tex_format = RD::texture_get_format(rd_texture);
        int tex_w = tex_format.width;
        int tex_h = tex_format.height;
		static uint64_t blit_dbg = 0;
        if (tex_w <= 0 || tex_h <= 0) {
            continue;
        }

        BlitMode mode = p_render_targets[i].lens_distortion.apply
                            ? BLIT_MODE_LENS
                            : (p_render_targets[i].multi_view.use_layer ? BLIT_MODE_USE_LAYER
                                                                        : BLIT_MODE_NORMAL);

        HashMap<RID, RID>::Iterator it = render_target_descriptors.find(rd_texture);
        RID uniform_set;

        if (it && !RD::uniform_set_is_valid(it->value)) {
            render_target_descriptors.remove(it);
            it = HashMap<RID, RID>::Iterator();
        }

        if (!it) {
            for (HashMap<RID, RID>::Iterator E = render_target_descriptors.begin(); E;) {
                HashMap<RID, RID>::Iterator next = E;
                ++next;
                if (!RD::uniform_set_is_valid(E->value)) {
                    render_target_descriptors.remove(E);
                }
                E = next;
            }

            Vector<RD::Uniform> uniforms;
            RD::Uniform u;
            u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
            u.binding = 0;
            u.append_id(blit->sampler);
            u.append_id(rd_texture);
            uniforms.push_back(u);

            uniform_set = RD::uniform_set_create(
                uniforms, blit->shader.version_get_shader(blit->shader_version, 0), 0);
            ERR_CONTINUE(uniform_set.is_null());

            render_target_descriptors.insert(rd_texture, uniform_set);
        }
        else {
            uniform_set = it->value;
        }

        Size2 screen_size(RD::screen_get_width(p_screen), RD::screen_get_height(p_screen));
        if (screen_size.width <= 0.0f || screen_size.height <= 0.0f) {
            continue;
        }

        Rect2 dst_rect = Rect2((float)p_render_targets[i].dst_rect.position.x,
            (float)p_render_targets[i].dst_rect.position.y,
            (float)p_render_targets[i].dst_rect.size.width,
            (float)p_render_targets[i].dst_rect.size.height);

        if (dst_rect.size.width <= 0.0f || dst_rect.size.height <= 0.0f ||
            dst_rect.position.x > 32768.0f || dst_rect.position.x < -32768.0f ||
            dst_rect.size.width > 32768.0f || dst_rect.size.height > 32768.0f) {
            dst_rect = Rect2(Point2(), screen_size);
        }

        Rect2 src_rect = Rect2((float)p_render_targets[i].src_rect.position.x,
            (float)p_render_targets[i].src_rect.position.y,
            (float)p_render_targets[i].src_rect.size.width,
            (float)p_render_targets[i].src_rect.size.height);

        if (src_rect.size.width <= 0.0f || src_rect.size.height <= 0.0f ||
            src_rect.position.x > 32768.0f || src_rect.position.x < -32768.0f ||
            src_rect.size.width > 32768.0f || src_rect.size.height > 32768.0f) {
            src_rect = Rect2(0.0f, 0.0f, (float)tex_w, (float)tex_h);
        }

        RD::draw_list_bind_render_pipeline(draw_list, blit_pipelines.pipelines[mode]);
        RD::draw_list_bind_index_array(draw_list, blit->array);
        RD::draw_list_bind_uniform_set(draw_list, uniform_set, 0);

        const int screen_rotation_degrees = -RD::screen_get_pre_rotation_degrees(p_screen);
        float screen_rotation = Math::deg_to_rad((float)screen_rotation_degrees);

        blit->push_constant.rotation_cos = Math::cos(screen_rotation);
        blit->push_constant.rotation_sin = Math::sin(screen_rotation);
        if (screen_rotation_degrees % 180 != 0) {
            SWAP(screen_size.width, screen_size.height);
        }

        blit->push_constant.src_rect[0] = src_rect.position.x / tex_w;
        blit->push_constant.src_rect[1] = src_rect.position.y / tex_h;
        blit->push_constant.src_rect[2] = src_rect.size.width / tex_w;
        blit->push_constant.src_rect[3] = src_rect.size.height / tex_h;

        blit->push_constant.dst_rect[0] = dst_rect.position.x / screen_size.width;
        blit->push_constant.dst_rect[1] = dst_rect.position.y / screen_size.height;
        blit->push_constant.dst_rect[2] = dst_rect.size.width / screen_size.width;
        blit->push_constant.dst_rect[3] = dst_rect.size.height / screen_size.height;

        blit->push_constant.layer = p_render_targets[i].multi_view.layer;
        blit->push_constant.eye_center[0] = p_render_targets[i].lens_distortion.eye_center.x;
        blit->push_constant.eye_center[1] = p_render_targets[i].lens_distortion.eye_center.y;
        blit->push_constant.k1 = p_render_targets[i].lens_distortion.k1;
        blit->push_constant.k2 = p_render_targets[i].lens_distortion.k2;
        blit->push_constant.upscale = p_render_targets[i].lens_distortion.upscale;
        blit->push_constant.aspect_ratio = p_render_targets[i].lens_distortion.aspect_ratio;
        blit->push_constant.source_is_srgb =
            !RendererRD::TextureStorage::render_target_is_using_hdr(p_render_targets[i].render_target);
        blit->push_constant.use_debanding =
            RendererRD::TextureStorage::render_target_is_using_debanding(p_render_targets[i].render_target);
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
	if (time_roll_over <= 0.0) {
		time_roll_over = 3600.0;
	}
	time = Math::fmod(time, time_roll_over);

	if (RendererCanvasRender::set_time) {
		RendererCanvasRender::set_time(time);
	}
	RendererSceneRenderRD::set_time(time, frame_step);
}

void RendererCompositorRD::end_frame(bool p_present) { RD::swap_buffers(p_present); }

void RendererCompositorRD::initialize()
{
	if (blit) {
		return;
	}

	UniformSetCacheRD::initialize();
	FramebufferCacheRD::initialize();

	RendererRD::TextureStorage::initialize();
	RendererRD::MaterialStorage::initialize();
	RendererRD::MeshStorage::initialize();
	RendererRD::Fog::initialize();
	RendererCanvasRenderRD::initialize();
	RendererRD::TextureStorage::_tex_blit_shader_initialize();
	RendererSceneRenderRD::initialize();
	bind_canvas_render();
	bind_mesh_storage();

	{
		blit = memnew(Blit);
		Vector<String> blit_modes;
		blit_modes.push_back("\n");
		blit_modes.push_back("\n#define USE_LAYER\n");
		blit_modes.push_back("\n#define USE_LAYER\n#define APPLY_LENS_DISTORTION\n");
		blit_modes.push_back("\n");

		blit->shader.initialize(blit_modes);
		blit->shader_version = blit->shader.version_create();
		RID s_rid = blit->shader.version_get_shader(blit->shader_version, 0);
		(void)s_rid;

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
	static bool finalized = false;
	if (finalized) {
		return;
	}
	finalized = true;

	RendererRD::TextureStorage::_tex_blit_shader_free();
	RendererCanvasRenderRD::finalize();
	RendererSceneRenderRD::finalize();
	RendererRD::Fog::finalize();
	RendererRD::GI::finalize();
	RendererRD::ParticlesStorage::finalize();
	RendererRD::LightStorage::finalize();
	RendererRD::MeshStorage::finalize();
	RendererRD::MaterialStorage::finalize();
	RendererRD::TextureStorage::finalize();

	render_target_descriptors.clear();

	if (blit) {
		blit->shader.version_free(blit->shader_version);
		RD::free_rid(blit->index_buffer);
		RD::free_rid(blit->sampler);

		memdelete(blit);
		blit = nullptr;
	}

	FramebufferCacheRD::finalize();
	UniformSetCacheRD::finalize();
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
		return;
	}

	BlitPipelines blit_pipelines = _get_blit_pipelines_for_format(
		RD::screen_get_framebuffer_format(DisplayServerEnums::MAIN_WINDOW_ID));

	RID texture = RendererRD::TextureStorage::texture_allocate();
	RendererRD::TextureStorage::texture_2d_initialize(texture, p_image);
	RID rd_texture = RendererRD::TextureStorage::texture_get_rd_texture(texture, false);

	RDC::SamplerState sampler_state;
	sampler_state.min_filter =
		p_use_filter ? RDC::SAMPLER_FILTER_LINEAR : RDC::SAMPLER_FILTER_NEAREST;
	sampler_state.mag_filter =
		p_use_filter ? RDC::SAMPLER_FILTER_LINEAR : RDC::SAMPLER_FILTER_NEAREST;
	sampler_state.max_lod = 0;
	RID sampler = RD::sampler_create(sampler_state);

	RID uset;
	{
		Vector<RD::Uniform> uniforms;
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
		u.binding = 0;
		u.append_id(sampler);
		u.append_id(rd_texture);
		uniforms.push_back(u);
		uset = RD::uniform_set_create(
			uniforms, blit->shader.version_get_shader(blit->shader_version, BLIT_MODE_NORMAL), 0);
	}

	Size2 window_size = DisplayServer::window_get_size();

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

	RendererRD::TextureStorage::texture_free(texture);
	RD::free_rid(sampler);
}

void RendererCompositorRD::bind_canvas_render()
{
	RendererCanvasRender::canvas_render_items = &RendererCanvasRenderRD::canvas_render_items;
	RendererCanvasRender::set_time = &RendererCanvasRenderRD::set_time;
	RendererCanvasRender::request_polygon = &RendererCanvasRenderRD::request_polygon;
	RendererCanvasRender::free_polygon = &RendererCanvasRenderRD::free_polygon;
	RendererCanvasRender::light_create = &RendererCanvasRenderRD::light_create;
	RendererCanvasRender::light_set_texture = &RendererCanvasRenderRD::light_set_texture;
	RendererCanvasRender::light_set_use_shadow = &RendererCanvasRenderRD::light_set_use_shadow;
	RendererCanvasRender::light_update_shadow = &RendererCanvasRenderRD::light_update_shadow;
	RendererCanvasRender::light_update_directional_shadow =
		&RendererCanvasRenderRD::light_update_directional_shadow;
	RendererCanvasRender::render_sdf = &RendererCanvasRenderRD::render_sdf;
	RendererCanvasRender::occluder_polygon_create =
		&RendererCanvasRenderRD::occluder_polygon_create;
	RendererCanvasRender::occluder_polygon_set_shape =
		&RendererCanvasRenderRD::occluder_polygon_set_shape;
	RendererCanvasRender::occluder_polygon_set_cull_mode =
		&RendererCanvasRenderRD::occluder_polygon_set_cull_mode;
	RendererCanvasRender::set_shadow_texture_size =
		&RendererCanvasRenderRD::set_shadow_texture_size;
	RendererCanvasRender::free = &RendererCanvasRenderRD::free;
	RendererCanvasRender::update = &RendererCanvasRenderRD::update;
	RendererCanvasRender::set_debug_redraw = &RendererCanvasRenderRD::set_debug_redraw;
	RendererCanvasRender::get_pipeline_compilations =
		&RendererCanvasRenderRD::get_pipeline_compilations;
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

RendererCompositorRD::BlitPipelines RendererCompositorRD::_get_blit_pipelines_for_format(
	RenderingDevice::FramebufferFormatID format)
{
	HashMap<RenderingDevice::FramebufferFormatID, BlitPipelines>::Iterator it =
		blit->pipelines_by_format.find(format);
	if (it != blit->pipelines_by_format.end()) {
		return it->value;
	}

	BlitPipelines pipelines;
	for (int i = 0; i < BLIT_MODE_MAX; i++) {
		pipelines.pipelines[i] = RD::render_pipeline_create(
			blit->shader.version_get_shader(blit->shader_version, i), format, RDC::INVALID_ID,
			RDC::RENDER_PRIMITIVE_TRIANGLES, RDC::PipelineRasterizationState(),
			RDC::PipelineMultisampleState(), RDC::PipelineDepthStencilState(),
			i == BLIT_MODE_NORMAL_ALPHA ? RDC::PipelineColorBlendState::create_blend()
										: RDC::PipelineColorBlendState::create_disabled(),
			0);
	}
	blit->pipelines_by_format.insert(format, pipelines);
	return pipelines;
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/rd_caches.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/rendering_device_binds.h"
#include "core/os/memory.h"

// --- Merged from servers/rendering/renderer_rd/framebuffer_cache_rd.cpp ---

void FramebufferCacheRD::_invalidate(Cache* p_cache)
{
	if (!data) {
		return;
	}

	if (p_cache->prev) {
		p_cache->prev->next = p_cache->next;
	}
	else {
		// At beginning of table
		uint32_t table_idx = p_cache->hash % HASH_TABLE_SIZE;
		data->hash_table[table_idx] = p_cache->next;
	}

	if (p_cache->next) {
		p_cache->next->prev = p_cache->prev;
	}

	data->cache_allocator.free(p_cache);
	data->cache_instances_used--;
}

void FramebufferCacheRD::_framebuffer_invalidation_callback(void* p_userdata)
{
	_invalidate(reinterpret_cast<Cache*>(p_userdata));
}

void FramebufferCacheRD::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void FramebufferCacheRD::finalize()
{
	if (!data) {
		return;
	}
	if (data->cache_instances_used > 0) {
		ERR_PRINT("At exit: " + itos(data->cache_instances_used) +
				  " framebuffer cache instance(s) still in use.");
	}
	memdelete(data);
	data = nullptr;
}

// --- Merged from servers/rendering/renderer_rd/pipeline_cache_rd.cpp ---

RID PipelineCacheRD::_generate_version(RD::VertexFormatID p_vertex_format_id,
	RD::FramebufferFormatID p_framebuffer_format_id, bool p_wireframe, uint32_t p_render_pass,
	uint32_t p_bool_specializations)
{
	RDC::PipelineMultisampleState multisample_state_version = multisample_state;
	multisample_state_version.sample_count =
		RD::framebuffer_format_get_texture_samples(p_framebuffer_format_id, p_render_pass);

	bool wireframe = p_wireframe;

	RDC::PipelineRasterizationState raster_state_version = rasterization_state;
	raster_state_version.wireframe = wireframe;

	Vector<RDC::PipelineSpecializationConstant> specialization_constants =
		base_specialization_constants;

	uint32_t bool_index = 0;
	uint32_t bool_specializations = p_bool_specializations;
	while (bool_specializations) {
		RDC::PipelineSpecializationConstant sc;
		sc.bool_value = bool(bool_specializations & (1 << bool_index));
		sc.constant_id = bool_index;
		sc.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
		specialization_constants.push_back(sc);
		bool_specializations &= ~(1 << bool_index);
		bool_index++;
	}

	RID pipeline = RD::render_pipeline_create(shader, p_framebuffer_format_id, p_vertex_format_id,
		render_primitive, raster_state_version, multisample_state_version, depth_stencil_state,
		blend_state, dynamic_state_flags, p_render_pass, specialization_constants);
	ERR_FAIL_COND_V(pipeline.is_null(), RID());
	versions = static_cast<Version*>(memrealloc(versions, sizeof(Version) * (version_count + 1)));
	versions[version_count].framebuffer_id = p_framebuffer_format_id;
	versions[version_count].vertex_id = p_vertex_format_id;
	versions[version_count].wireframe = wireframe;
	versions[version_count].pipeline = pipeline;
	versions[version_count].render_pass = p_render_pass;
	versions[version_count].bool_specializations = p_bool_specializations;
	version_count++;
	return pipeline;
}

void PipelineCacheRD::_clear()
{
	// TODO: Clear should probably recompile all the variants already compiled instead to avoid
	// stalls? Needs discussion.
	if (versions) {
		for (uint32_t i = 0; i < version_count; i++) {
			// shader may be gone, so this may not be valid
			if (RD::render_pipeline_is_valid(versions[i].pipeline)) {
				RD::free_rid(versions[i].pipeline);
			}
		}
		version_count = 0;
		memfree(versions);
		versions = nullptr;
	}
}

void PipelineCacheRD::setup(RID p_shader, RDC::RenderPrimitive p_primitive,
	const RDC::PipelineRasterizationState& p_rasterization_state,
	RDC::PipelineMultisampleState p_multisample,
	const RDC::PipelineDepthStencilState& p_depth_stencil_state,
	const RDC::PipelineColorBlendState& p_blend_state, int p_dynamic_state_flags,
	const Vector<RDC::PipelineSpecializationConstant>& p_base_specialization_constants)
{
	ERR_FAIL_COND(p_shader.is_null());
	_clear();
	shader = p_shader;
	render_primitive = p_primitive;
	rasterization_state = p_rasterization_state;
	multisample_state = p_multisample;
	depth_stencil_state = p_depth_stencil_state;
	blend_state = p_blend_state;
	dynamic_state_flags = p_dynamic_state_flags;
	base_specialization_constants = p_base_specialization_constants;
}

void PipelineCacheRD::update_specialization_constants(
	const Vector<RDC::PipelineSpecializationConstant>& p_base_specialization_constants)
{
	base_specialization_constants = p_base_specialization_constants;
	_clear();
}

void PipelineCacheRD::update_shader(RID p_shader)
{
	ERR_FAIL_COND(p_shader.is_null());
	_clear();
	setup(p_shader, render_primitive, rasterization_state, multisample_state, depth_stencil_state,
		blend_state, dynamic_state_flags);
}

void PipelineCacheRD::clear()
{
	_clear();
	shader = RID(); // clear shader
}

PipelineCacheRD::PipelineCacheRD()
{
	version_count = 0;
	versions = nullptr;
}

PipelineCacheRD::~PipelineCacheRD() { _clear(); }

// --- Merged from servers/rendering/renderer_rd/uniform_set_cache_rd.cpp ---

void UniformSetCacheRD::_invalidate(Cache* p_cache)
{
	if (!data) {
		return;
	}

	if (p_cache->prev) {
		p_cache->prev->next = p_cache->next;
	}
	else {
		// At beginning of table
		uint32_t table_idx = p_cache->hash % HASH_TABLE_SIZE;
		data->hash_table[table_idx] = p_cache->next;
	}

	if (p_cache->next) {
		p_cache->next->prev = p_cache->prev;
	}

	data->cache_allocator.free(p_cache);
	data->cache_instances_used--;
}

void UniformSetCacheRD::_uniform_set_invalidation_callback(void* p_userdata)
{
	_invalidate(reinterpret_cast<Cache*>(p_userdata));
}

void UniformSetCacheRD::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void UniformSetCacheRD::finalize()
{
	if (!data) {
		return;
	}
	if (data->cache_instances_used > 0) {
		ERR_PRINT("At exit: " + itos(data->cache_instances_used) +
				  " uniform set cache instance(s) still in use.");
	}
	memdelete(data);
	data = nullptr;
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/renderer_canvas_render_rd.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/transform_interpolator.h"
#include "core/templates/fixed_vector.h"
#include "renderer_canvas_render_rd.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_canvas_cull.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"

void RendererCanvasRenderRD::_update_transform_2d_to_mat4(
	const Transform2D& p_transform, float* p_mat4)
{
	p_mat4[0] = p_transform.columns[0][0];
	p_mat4[1] = p_transform.columns[0][1];
	p_mat4[2] = 0;
	p_mat4[3] = 0;
	p_mat4[4] = p_transform.columns[1][0];
	p_mat4[5] = p_transform.columns[1][1];
	p_mat4[6] = 0;
	p_mat4[7] = 0;
	p_mat4[8] = 0;
	p_mat4[9] = 0;
	p_mat4[10] = 1;
	p_mat4[11] = 0;
	p_mat4[12] = p_transform.columns[2][0];
	p_mat4[13] = p_transform.columns[2][1];
	p_mat4[14] = 0;
	p_mat4[15] = 1;
}

void RendererCanvasRenderRD::_update_transform_2d_to_mat2x4(
	const Transform2D& p_transform, float* p_mat2x4)
{
	p_mat2x4[0] = p_transform.columns[0][0];
	p_mat2x4[1] = p_transform.columns[1][0];
	p_mat2x4[2] = 0;
	p_mat2x4[3] = p_transform.columns[2][0];

	p_mat2x4[4] = p_transform.columns[0][1];
	p_mat2x4[5] = p_transform.columns[1][1];
	p_mat2x4[6] = 0;
	p_mat2x4[7] = p_transform.columns[2][1];
}

void RendererCanvasRenderRD::_update_transform_2d_to_mat2x3(
	const Transform2D& p_transform, float* p_mat2x3)
{
	p_mat2x3[0] = p_transform.columns[0][0];
	p_mat2x3[1] = p_transform.columns[0][1];
	p_mat2x3[2] = p_transform.columns[1][0];
	p_mat2x3[3] = p_transform.columns[1][1];
	p_mat2x3[4] = p_transform.columns[2][0];
	p_mat2x3[5] = p_transform.columns[2][1];
}

void RendererCanvasRenderRD::_update_transform_to_mat4(
	const Transform3D& p_transform, float* p_mat4)
{
	p_mat4[0] = p_transform.basis.rows[0][0];
	p_mat4[1] = p_transform.basis.rows[1][0];
	p_mat4[2] = p_transform.basis.rows[2][0];
	p_mat4[3] = 0;
	p_mat4[4] = p_transform.basis.rows[0][1];
	p_mat4[5] = p_transform.basis.rows[1][1];
	p_mat4[6] = p_transform.basis.rows[2][1];
	p_mat4[7] = 0;
	p_mat4[8] = p_transform.basis.rows[0][2];
	p_mat4[9] = p_transform.basis.rows[1][2];
	p_mat4[10] = p_transform.basis.rows[2][2];
	p_mat4[11] = 0;
	p_mat4[12] = p_transform.origin.x;
	p_mat4[13] = p_transform.origin.y;
	p_mat4[14] = p_transform.origin.z;
	p_mat4[15] = 1;
}

PolygonID RendererCanvasRenderRD::request_polygon(
	const Vector<int>& p_indices, const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, const Vector<int>& p_bones, const Vector<float>& p_weights,
	int p_count)
{
	// Care must be taken to generate array formats
	// in ways where they could be reused, so we will
	// put single-occurring elements first, and repeated
	// elements later. This way the generated formats are
	// the same no matter the length of the arrays.
	// This dramatically reduces the amount of pipeline objects
	// that need to be created for these formats.

	uint32_t vertex_count = p_points.size();
	uint32_t stride = 2; // vertices always repeat
	if ((uint32_t)p_colors.size() == vertex_count || p_colors.size() == 1) {
		stride += 4;
	}
	if ((uint32_t)p_uvs.size() == vertex_count) {
		stride += 2;
	}
	if ((uint32_t)p_bones.size() == vertex_count * 4 &&
		(uint32_t)p_weights.size() == vertex_count * 4) {
		stride += 4;
	}

	uint32_t buffer_size = stride * p_points.size();

	Vector<uint8_t> polygon_buffer;
	polygon_buffer.resize(buffer_size * sizeof(float));
	Vector<RDC::VertexAttribute> descriptions;
	descriptions.resize(5);
	Vector<RID> buffers;
	buffers.resize(5);

	{
		uint8_t* r = polygon_buffer.ptrw();
		float* fptr = reinterpret_cast<float*>(r);
		uint32_t* uptr = reinterpret_cast<uint32_t*>(r);
		uint32_t base_offset = 0;
		{ // vertices
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
			vd.offset = base_offset * sizeof(float);
			vd.location = RSE::ARRAY_VERTEX;
			vd.stride = stride * sizeof(float);

			descriptions.write[0] = vd;

			const Vector2* points_ptr = p_points.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				fptr[base_offset + i * stride + 0] = points_ptr[i].x;
				fptr[base_offset + i * stride + 1] = points_ptr[i].y;
			}

			base_offset += 2;
		}

		// colors
		if ((uint32_t)p_colors.size() == vertex_count || p_colors.size() == 1) {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
			vd.offset = base_offset * sizeof(float);
			vd.location = RSE::ARRAY_COLOR;
			vd.stride = stride * sizeof(float);

			descriptions.write[1] = vd;

			if (p_colors.size() == 1) {
				Color color = p_colors[0];
				for (uint32_t i = 0; i < vertex_count; i++) {
					fptr[base_offset + i * stride + 0] = color.r;
					fptr[base_offset + i * stride + 1] = color.g;
					fptr[base_offset + i * stride + 2] = color.b;
					fptr[base_offset + i * stride + 3] = color.a;
				}
			}
			else {
				const Color* color_ptr = p_colors.ptr();

				for (uint32_t i = 0; i < vertex_count; i++) {
					fptr[base_offset + i * stride + 0] = color_ptr[i].r;
					fptr[base_offset + i * stride + 1] = color_ptr[i].g;
					fptr[base_offset + i * stride + 2] = color_ptr[i].b;
					fptr[base_offset + i * stride + 3] = color_ptr[i].a;
				}
			}
			base_offset += 4;
		}
		else {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_COLOR;
			vd.stride = 0;

			descriptions.write[1] = vd;
			buffers.write[1] = RendererRD::MeshStorage::mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_COLOR);
		}

		// uvs
		if ((uint32_t)p_uvs.size() == vertex_count) {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
			vd.offset = base_offset * sizeof(float);
			vd.location = RSE::ARRAY_TEX_UV;
			vd.stride = stride * sizeof(float);

			descriptions.write[2] = vd;

			const Vector2* uv_ptr = p_uvs.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				fptr[base_offset + i * stride + 0] = uv_ptr[i].x;
				fptr[base_offset + i * stride + 1] = uv_ptr[i].y;
			}
			base_offset += 2;
		}
		else {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_TEX_UV;
			vd.stride = 0;

			descriptions.write[2] = vd;
			buffers.write[2] = RendererRD::MeshStorage::mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_TEX_UV);
		}

		// bones
		if ((uint32_t)p_indices.size() == vertex_count * 4 &&
			(uint32_t)p_weights.size() == vertex_count * 4) {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R16G16B16A16_UINT;
			vd.offset = base_offset * sizeof(float);
			vd.location = RSE::ARRAY_BONES;
			vd.stride = stride * sizeof(float);

			descriptions.write[3] = vd;

			const int* bone_ptr = p_bones.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				uint16_t* bone16w = (uint16_t*)&uptr[base_offset + i * stride];

				bone16w[0] = bone_ptr[i * 4 + 0];
				bone16w[1] = bone_ptr[i * 4 + 1];
				bone16w[2] = bone_ptr[i * 4 + 2];
				bone16w[3] = bone_ptr[i * 4 + 3];
			}

			base_offset += 2;
		}
		else {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32B32A32_UINT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_BONES;
			vd.stride = 0;

			descriptions.write[3] = vd;
			buffers.write[3] = RendererRD::MeshStorage::mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_BONES);
		}

		// weights
		if ((uint32_t)p_weights.size() == vertex_count * 4) {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
			vd.offset = base_offset * sizeof(float);
			vd.location = RSE::ARRAY_WEIGHTS;
			vd.stride = stride * sizeof(float);

			descriptions.write[4] = vd;

			const float* weight_ptr = p_weights.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				uint16_t* weight16w = (uint16_t*)&uptr[base_offset + i * stride];

				weight16w[0] = CLAMP(weight_ptr[i * 4 + 0] * 65535, 0, 65535);
				weight16w[1] = CLAMP(weight_ptr[i * 4 + 1] * 65535, 0, 65535);
				weight16w[2] = CLAMP(weight_ptr[i * 4 + 2] * 65535, 0, 65535);
				weight16w[3] = CLAMP(weight_ptr[i * 4 + 3] * 65535, 0, 65535);
			}

			base_offset += 2;
		}
		else {
			RDC::VertexAttribute vd;
			vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_WEIGHTS;
			vd.stride = 0;

			descriptions.write[4] = vd;
			buffers.write[4] = RendererRD::MeshStorage::mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_WEIGHTS);
		}

		// check that everything is as it should be
		ERR_FAIL_COND_V(base_offset != stride, 0); // bug
	}

	RD::VertexFormatID vertex_id = RD::vertex_format_create(descriptions);
	ERR_FAIL_COND_V(vertex_id == RDC::INVALID_ID, 0);

	RendererRD::PolygonBuffers pb;
	pb.vertex_buffer = RD::vertex_buffer_create(polygon_buffer.size(), polygon_buffer);
	for (int i = 0; i < descriptions.size(); i++) {
		if (buffers[i] == RID()) { // if put in vertex, use as vertex
			buffers.write[i] = pb.vertex_buffer;
		}
	}

	pb.vertex_array = RD::vertex_array_create(p_points.size(), vertex_id, buffers);
	pb.primitive_count = vertex_count;

	if (p_indices.size()) {
		// create indices, as indices were requested
		Vector<uint8_t> index_buffer;
		index_buffer.resize(p_count * sizeof(int32_t));
		{
			uint8_t* w = index_buffer.ptrw();
			memcpy(w, p_indices.ptr(), sizeof(int32_t) * p_indices.size());
		}
		pb.index_buffer =
			RD::index_buffer_create(p_count, RDC::INDEX_BUFFER_FORMAT_UINT32, index_buffer);
		pb.indices = RD::index_array_create(pb.index_buffer, 0, p_count);
		pb.primitive_count = p_count;
	}

	pb.vertex_format_id = vertex_id;

	PolygonID id = RendererRD::polygon_buffers.last_id++;

	RendererRD::polygon_buffers.polygons[id] = pb;

	return id;
}

void RendererCanvasRenderRD::free_polygon(PolygonID p_polygon)
{
	RendererRD::PolygonBuffers* pb_ptr = RendererRD::polygon_buffers.polygons.getptr(p_polygon);
	ERR_FAIL_NULL(pb_ptr);

	RendererRD::PolygonBuffers& pb = *pb_ptr;

	if (pb.indices.is_valid()) {
		RD::free_rid(pb.indices);
	}
	if (pb.index_buffer.is_valid()) {
		RD::free_rid(pb.index_buffer);
	}

	RD::free_rid(pb.vertex_array);
	RD::free_rid(pb.vertex_buffer);

	RendererRD::polygon_buffers.polygons.erase(p_polygon);
}

static RDC::RenderPrimitive _primitive_type_to_render_primitive(RSE::PrimitiveType p_primitive)
{
	switch (p_primitive) {
	case RSE::PRIMITIVE_POINTS:
		return RDC::RENDER_PRIMITIVE_POINTS;
	case RSE::PRIMITIVE_LINES:
		return RDC::RENDER_PRIMITIVE_LINES;
	case RSE::PRIMITIVE_LINE_STRIP:
		return RDC::RENDER_PRIMITIVE_LINESTRIPS;
	case RSE::PRIMITIVE_TRIANGLES:
		return RDC::RENDER_PRIMITIVE_TRIANGLES;
	case RSE::PRIMITIVE_TRIANGLE_STRIP:
		return RDC::RENDER_PRIMITIVE_TRIANGLE_STRIPS;
	default:
		return RDC::RENDER_PRIMITIVE_MAX;
	}
}

_FORCE_INLINE_ static uint32_t _indices_to_primitives(
	RSE::PrimitiveType p_primitive, uint32_t p_indices)
{
	static const uint32_t divisor[RSE::PRIMITIVE_MAX] = {1, 2, 1, 3, 1};
	static const uint32_t subtractor[RSE::PRIMITIVE_MAX] = {0, 0, 1, 0, 2};
	return (p_indices - subtractor[p_primitive]) / divisor[p_primitive];
}

RID RendererCanvasRenderRD::_create_base_uniform_set(RID p_to_render_target, bool p_backbuffer)
{
	// re create canvas state
	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.binding = 1;
		u.append_id(RendererRD::state.canvas_state_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 2;
		u.append_id(RendererRD::state.lights_storage_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 3;
		u.append_id(RendererRD::TextureStorage::decal_atlas_get_texture());
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 4;
		u.append_id(RendererRD::state.shadow_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER;
		u.binding = 5;
		u.append_id(RendererRD::state.shadow_sampler);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 6;
		RID screen;
		if (p_backbuffer) {
			screen = RendererRD::TextureStorage::render_target_get_rd_texture(p_to_render_target);
		}
		else {
			screen =
				RendererRD::TextureStorage::render_target_get_rd_backbuffer(p_to_render_target);
			if (screen.is_null()) { // unallocated backbuffer
				screen = RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
			}
		}
		u.append_id(screen);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 7;
		RID sdf = RendererRD::TextureStorage::render_target_get_sdf_texture(p_to_render_target);
		u.append_id(sdf);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 9;
		u.append_id(RendererRD::MaterialStorage::global_shader_uniforms_get_storage_buffer());
		uniforms.push_back(u);
	}

	RendererRD::MaterialStorage::samplers_rd_get_default().append_uniforms(
		uniforms, SAMPLERS_BINDING_FIRST_INDEX);

	RID uniform_set =
		RD::uniform_set_create(uniforms, RendererRD::shader.default_version_rd_shader, BASE_UNIFORM_SET);
	if (p_backbuffer) {
		RendererRD::TextureStorage::render_target_set_backbuffer_uniform_set(
			p_to_render_target, uniform_set);
	}
	else {
		RendererRD::TextureStorage::render_target_set_framebuffer_uniform_set(
			p_to_render_target, uniform_set);
	}

	return uniform_set;
}

void RendererCanvasRenderRD::canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list,
	const Color& p_modulate, Light* p_light_list, Light* p_directional_light_list,
	const Transform2D& p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter,
	RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
	bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info)
{
	r_sdf_used = false;
	int item_count = 0;

	// setup canvas state uniforms if needed

	Transform2D canvas_transform_inverse = p_canvas_transform.affine_inverse();

	// setup directional lights if exist

	uint32_t light_count = 0;
	uint32_t directional_light_count = 0;
	{
		Light* l = p_directional_light_list;
		uint32_t index = 0;

		while (l) {
			if (index == MAX_LIGHTS_PER_RENDER) {
				l->render_index_cache = -1;
				l = l->next_ptr;
				continue;
			}

			CanvasLight* clight = canvas_light_owner.get_or_null(l->light_internal);
			if (!clight) { // unused or invalid texture
				l->render_index_cache = -1;
				l = l->next_ptr;
				ERR_CONTINUE(!clight);
			}

			Vector2 canvas_light_dir = l->xform_cache.columns[1].normalized();

			RendererRD::state.light_uniforms[index].position[0] = -canvas_light_dir.x;
			RendererRD::state.light_uniforms[index].position[1] = -canvas_light_dir.y;

			_update_transform_2d_to_mat2x4(
				clight->shadow.directional_xform, RendererRD::state.light_uniforms[index].shadow_matrix);

			RendererRD::state.light_uniforms[index].height = l->height; // 0..1 here

			for (int i = 0; i < 4; i++) {
				RendererRD::state.light_uniforms[index].shadow_color[i] =
					uint8_t(CLAMP(int32_t(l->shadow_color[i] * 255.0), 0, 255));
				RendererRD::state.light_uniforms[index].color[i] = l->color[i];
			}

			RendererRD::state.light_uniforms[index].color[3] *=
				l->energy; // use alpha for energy, so base color can go separate

			if (RendererRD::state.shadow_fb.is_valid()) {
				RendererRD::state.light_uniforms[index].shadow_pixel_size =
					(1.0 / RendererRD::state.shadow_texture_size) * (1.0 + l->shadow_smooth);
				RendererRD::state.light_uniforms[index].shadow_z_far_inv = 1.0 / clight->shadow.z_far;
				RendererRD::state.light_uniforms[index].shadow_y_ofs = clight->shadow.y_offset;
			}
			else {
				RendererRD::state.light_uniforms[index].shadow_pixel_size = 1.0;
				RendererRD::state.light_uniforms[index].shadow_z_far_inv = 1.0;
				RendererRD::state.light_uniforms[index].shadow_y_ofs = 0;
			}

			RendererRD::state.light_uniforms[index].flags = l->blend_mode << LIGHT_FLAGS_BLEND_SHIFT;
			RendererRD::state.light_uniforms[index].flags |= l->shadow_filter << LIGHT_FLAGS_FILTER_SHIFT;
			if (clight->shadow.enabled) {
				RendererRD::state.light_uniforms[index].flags |= LIGHT_FLAGS_HAS_SHADOW;
			}

			l->render_index_cache = index;

			index++;
			l = l->next_ptr;
		}

		light_count = index;
		directional_light_count = light_count;
		using_directional_lights = directional_light_count > 0;
	}

	// setup lights if exist

	{
		Light* l = p_light_list;
		uint32_t index = light_count;

		while (l) {
			if (index == MAX_LIGHTS_PER_RENDER) {
				l->render_index_cache = -1;
				l = l->next_ptr;
				continue;
			}

			CanvasLight* clight = canvas_light_owner.get_or_null(l->light_internal);
			if (!clight) { // unused or invalid texture
				l->render_index_cache = -1;
				l = l->next_ptr;
				ERR_CONTINUE(!clight);
			}

			Transform2D final_xform;
			if (!Renderer::canvas->_interpolation_data.interpolation_enabled || !l->interpolated ||
				!l->on_interpolate_transform_list) {
				final_xform = l->xform_curr;
			}
			else {
				real_t f = Engine::get_physics_interpolation_fraction();
				TransformInterpolator::interpolate_transform_2d(
					l->xform_prev, l->xform_curr, final_xform, f);
			}
			// Convert light position to canvas coordinates, as all computation is done in canvas
			// coordinates to avoid precision loss.
			Vector2 canvas_light_pos = p_canvas_transform.xform(final_xform.get_origin());
			RendererRD::state.light_uniforms[index].position[0] = canvas_light_pos.x;
			RendererRD::state.light_uniforms[index].position[1] = canvas_light_pos.y;

			_update_transform_2d_to_mat2x4(
				l->light_shader_xform.affine_inverse(), RendererRD::state.light_uniforms[index].matrix);
			_update_transform_2d_to_mat2x4(
				l->xform_cache.affine_inverse(), RendererRD::state.light_uniforms[index].shadow_matrix);

			RendererRD::state.light_uniforms[index].height =
				l->height *
				(p_canvas_transform.columns[0].length() + p_canvas_transform.columns[1].length()) *
				0.5; // approximate height conversion to the canvas size, since all calculations are
					 // done in canvas coords to avoid precision loss
			for (int i = 0; i < 4; i++) {
				RendererRD::state.light_uniforms[index].shadow_color[i] =
					uint8_t(CLAMP(int32_t(l->shadow_color[i] * 255.0), 0, 255));
				RendererRD::state.light_uniforms[index].color[i] = l->color[i];
			}

			RendererRD::state.light_uniforms[index].color[3] *=
				l->energy; // use alpha for energy, so base color can go separate

			if (RendererRD::state.shadow_fb.is_valid()) {
				RendererRD::state.light_uniforms[index].shadow_pixel_size =
					(1.0 / RendererRD::state.shadow_texture_size) * (1.0 + l->shadow_smooth);
				RendererRD::state.light_uniforms[index].shadow_z_far_inv = 1.0 / clight->shadow.z_far;
				RendererRD::state.light_uniforms[index].shadow_y_ofs = clight->shadow.y_offset;
			}
			else {
				RendererRD::state.light_uniforms[index].shadow_pixel_size = 1.0;
				RendererRD::state.light_uniforms[index].shadow_z_far_inv = 1.0;
				RendererRD::state.light_uniforms[index].shadow_y_ofs = 0;
			}

			RendererRD::state.light_uniforms[index].flags = l->blend_mode << LIGHT_FLAGS_BLEND_SHIFT;
			RendererRD::state.light_uniforms[index].flags |= l->shadow_filter << LIGHT_FLAGS_FILTER_SHIFT;
			if (clight->shadow.enabled) {
				RendererRD::state.light_uniforms[index].flags |= LIGHT_FLAGS_HAS_SHADOW;
			}

			if (clight->texture.is_valid()) {
				Rect2 atlas_rect =
					RendererRD::TextureStorage::decal_atlas_get_texture_rect(clight->texture);
				RendererRD::state.light_uniforms[index].atlas_rect[0] = atlas_rect.position.x;
				RendererRD::state.light_uniforms[index].atlas_rect[1] = atlas_rect.position.y;
				RendererRD::state.light_uniforms[index].atlas_rect[2] = atlas_rect.size.width;
				RendererRD::state.light_uniforms[index].atlas_rect[3] = atlas_rect.size.height;

			}
			else {
				RendererRD::state.light_uniforms[index].atlas_rect[0] = 0;
				RendererRD::state.light_uniforms[index].atlas_rect[1] = 0;
				RendererRD::state.light_uniforms[index].atlas_rect[2] = 0;
				RendererRD::state.light_uniforms[index].atlas_rect[3] = 0;
			}

			l->render_index_cache = index;

			index++;
			l = l->next_ptr;
		}

		light_count = index;
	}

	if (light_count > 0) {
		RD::buffer_update(RendererRD::state.lights_storage_buffer, 0,
			sizeof(RendererRD::LightUniform) * light_count, &RendererRD::state.light_uniforms[0]);
	}

	bool use_linear_colors =
		RendererRD::TextureStorage::render_target_is_using_hdr(p_to_render_target);

	{
		// update canvas state uniform buffer
		RendererRD::State::Buffer state_buffer;

		Size2i ssize = RendererRD::TextureStorage::render_target_get_size(p_to_render_target);

		Transform3D screen_transform;
		screen_transform.translate_local(-(ssize.width / 2.0f), -(ssize.height / 2.0f), 0.0f);
		screen_transform.scale(Vector3(2.0f / ssize.width, 2.0f / ssize.height, 1.0f));
		_update_transform_to_mat4(screen_transform, state_buffer.screen_transform);
		_update_transform_2d_to_mat4(p_canvas_transform, state_buffer.canvas_transform);

		Transform2D normal_transform = p_canvas_transform;
		normal_transform.columns[0].normalize();
		normal_transform.columns[1].normalize();
		normal_transform.columns[2] = Vector2();
		_update_transform_2d_to_mat4(normal_transform, state_buffer.canvas_normal_transform);

		Color modulate = p_modulate;
		if (use_linear_colors) {
			modulate = p_modulate.srgb_to_linear();
		}
		state_buffer.canvas_modulate[0] = modulate.r;
		state_buffer.canvas_modulate[1] = modulate.g;
		state_buffer.canvas_modulate[2] = modulate.b;
		state_buffer.canvas_modulate[3] = modulate.a;

		Size2 render_target_size =
			RendererRD::TextureStorage::render_target_get_size(p_to_render_target);
		state_buffer.screen_pixel_size[0] = 1.0 / render_target_size.x;
		state_buffer.screen_pixel_size[1] = 1.0 / render_target_size.y;

		state_buffer.time = RendererRD::state.time;
		state_buffer.use_pixel_snap = p_snap_2d_vertices_to_pixel;

		state_buffer.directional_light_count = directional_light_count;

		Vector2 canvas_scale = p_canvas_transform.get_scale();

		state_buffer.sdf_to_screen[0] = render_target_size.width / canvas_scale.x;
		state_buffer.sdf_to_screen[1] = render_target_size.height / canvas_scale.y;

		state_buffer.screen_to_sdf[0] = 1.0 / state_buffer.sdf_to_screen[0];
		state_buffer.screen_to_sdf[1] = 1.0 / state_buffer.sdf_to_screen[1];

		Rect2 sdf_rect = RendererRD::TextureStorage::render_target_get_sdf_rect(p_to_render_target);
		Rect2 sdf_tex_rect(sdf_rect.position / canvas_scale, sdf_rect.size / canvas_scale);

		state_buffer.sdf_to_tex[0] = 1.0 / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[1] = 1.0 / sdf_tex_rect.size.height;
		state_buffer.sdf_to_tex[2] = -sdf_tex_rect.position.x / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[3] = -sdf_tex_rect.position.y / sdf_tex_rect.size.height;

		// print_line("w: " + itos(ssize.width) + " s: " + rtos(canvas_scale));
		state_buffer.tex_to_sdf = 1.0 / ((canvas_scale.x + canvas_scale.y) * 0.5);
		state_buffer.shadow_pixel_size = 1.0f / (float)(RendererRD::state.shadow_texture_size);

		state_buffer.flags = use_linear_colors ? CANVAS_FLAGS_CONVERT_ATTRIBUTES_TO_LINEAR : 0;

		RD::buffer_update(RendererRD::state.canvas_state_buffer, 0,
			sizeof(RendererRD::State::Buffer), &state_buffer);
	}

	{ // default filter/repeat
		default_filter = p_default_filter;
		default_repeat = p_default_repeat;
	}

	RCR::Item* ci = p_item_list;

	// fill the list until rendering is possible.
	bool material_screen_texture_cached = false;
	bool material_screen_texture_mipmaps_cached = false;

	Rect2 back_buffer_rect;
	bool backbuffer_copy = false;
	bool backbuffer_gen_mipmaps = false;

	RCR::Item* canvas_group_owner = nullptr;
	bool skip_item = false;

	bool update_skeletons = false;
	bool time_used = false;

	bool backbuffer_cleared = false;

	RenderTarget to_render_target;
	to_render_target.render_target = p_to_render_target;
	to_render_target.use_linear_colors = use_linear_colors;

	while (ci) {
		if (ci->copy_back_buffer && canvas_group_owner == nullptr) {
			backbuffer_copy = true;

			if (ci->copy_back_buffer->full) {
				back_buffer_rect = Rect2();
			}
			else {
				back_buffer_rect = ci->copy_back_buffer->rect;
			}
		}

		RID material = ci->material_owner == nullptr ? ci->material : ci->material_owner->material;

		if (material.is_valid()) {
			RendererRD::CanvasMaterialData* md =
				static_cast<RendererRD::CanvasMaterialData*>(RendererRD::MaterialStorage::material_get_data(
					material, RendererRD::MaterialStorage::SHADER_TYPE_2D));
			if (md && md->shader_data->is_valid()) {
				if (md->shader_data->uses_screen_texture && canvas_group_owner == nullptr) {
					if (!material_screen_texture_cached) {
						backbuffer_copy = true;
						back_buffer_rect = Rect2();
						backbuffer_gen_mipmaps = md->shader_data->uses_screen_texture_mipmaps;
					}
					else if (!material_screen_texture_mipmaps_cached) {
						backbuffer_gen_mipmaps = md->shader_data->uses_screen_texture_mipmaps;
					}
				}

				if (md->shader_data->uses_sdf) {
					r_sdf_used = true;
				}
				if (md->shader_data->uses_time) {
					time_used = true;
				}
			}
		}

		if (ci->skeleton.is_valid()) {
			const RCR::Item::Command* c = ci->commands;

			while (c) {
				if (c->type == RCR::Item::Command::TYPE_MESH) {
					const RCR::Item::CommandMesh* cm = reinterpret_cast<const RCR::Item::CommandMesh*>(c);
					if (cm->mesh_instance.is_valid()) {
						RendererRD::MeshStorage::mesh_instance_check_for_update(cm->mesh_instance);
						RendererRD::MeshStorage::mesh_instance_set_canvas_item_transform(
							cm->mesh_instance, canvas_transform_inverse * ci->final_transform);
						update_skeletons = true;
					}
				}
				c = c->next;
			}
		}

		if (ci->canvas_group_owner != nullptr) {
			if (canvas_group_owner == nullptr) {
				// Canvas group begins here, render until before this item
				if (update_skeletons) {
					RendererRD::MeshStorage::update_mesh_instances();
					update_skeletons = false;
				}
				_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
					p_light_list, r_sdf_used, false, r_render_info);
				item_count = 0;

				if (ci->canvas_group_owner->canvas_group->mode !=
					RSE::CANVAS_GROUP_MODE_TRANSPARENT) {
					Rect2i group_rect = ci->canvas_group_owner->global_rect_cache;
					RendererRD::TextureStorage::render_target_copy_to_back_buffer(
						p_to_render_target, group_rect, false);
					if (ci->canvas_group_owner->canvas_group->mode ==
						RSE::CANVAS_GROUP_MODE_CLIP_AND_DRAW) {
						ci->canvas_group_owner->use_canvas_group = false;
						items[item_count++] = ci->canvas_group_owner;
					}
				}
				else if (!backbuffer_cleared) {
					RendererRD::TextureStorage::render_target_clear_back_buffer(
						p_to_render_target, Rect2i(), Color(0, 0, 0, 0));
					backbuffer_cleared = true;
				}

				backbuffer_copy = false;
				canvas_group_owner = ci->canvas_group_owner; // continue until owner found
			}

			ci->canvas_group_owner = nullptr; // must be cleared
		}

		if (canvas_group_owner == nullptr && ci->canvas_group != nullptr &&
			ci->canvas_group->mode != RSE::CANVAS_GROUP_MODE_CLIP_AND_DRAW) {
			skip_item = true;
		}

		if (ci == canvas_group_owner) {
			if (update_skeletons) {
				RendererRD::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}

			_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
				p_light_list, r_sdf_used, true, r_render_info);
			item_count = 0;

			if (ci->canvas_group->blur_mipmaps) {
				RendererRD::TextureStorage::render_target_gen_back_buffer_mipmaps(
					p_to_render_target, ci->global_rect_cache);
			}

			canvas_group_owner = nullptr;
			// Backbuffer is dirty now and needs to be re-cleared if another CanvasGroup needs it.
			backbuffer_cleared = false;

			// Tell the renderer to paint this as a canvas group
			ci->use_canvas_group = true;
		}
		else {
			ci->use_canvas_group = false;
		}

		if (backbuffer_copy) {
			// render anything pending, including clearing if no items
			if (update_skeletons) {
				RendererRD::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}

			_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
				p_light_list, r_sdf_used, false, r_render_info);
			item_count = 0;

			RendererRD::TextureStorage::render_target_copy_to_back_buffer(
				p_to_render_target, back_buffer_rect, backbuffer_gen_mipmaps);

			backbuffer_copy = false;
			material_screen_texture_cached =
				true; // After a backbuffer copy, screen texture makes no further copies.
			material_screen_texture_mipmaps_cached = backbuffer_gen_mipmaps;
			backbuffer_gen_mipmaps = false;
		}

		if (backbuffer_gen_mipmaps) {
			RendererRD::TextureStorage::render_target_gen_back_buffer_mipmaps(
				p_to_render_target, back_buffer_rect);

			backbuffer_gen_mipmaps = false;
			material_screen_texture_mipmaps_cached = true;
		}

		if (skip_item) {
			skip_item = false;
		}
		else {
			items[item_count++] = ci;
		}

		if (!ci->next || item_count == MAX_RENDER_ITEMS - 1) {
			if (update_skeletons) {
				RendererRD::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}

			_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
				p_light_list, r_sdf_used, canvas_group_owner != nullptr, r_render_info);
			// then reset
			item_count = 0;
		}

		ci = ci->next;
	}

	if (time_used) {
		Renderer::redraw_request();
	}

	texture_info_map.clear();

	// Save the previous instance data pointer in case more items are rendered in the same frame.
	RendererRD::state.prev_instance_data = RendererRD::state.instance_data;
	RendererRD::state.prev_instance_data_index = RendererRD::state.instance_data_index;

	RendererRD::state.instance_data = nullptr;
	if (RendererRD::state.instance_data_index > 0) {
		// If there was any remaining instance data, it must be flushed.
		RID buf = RendererRD::state.instance_buffers._get(0);
		RD::buffer_flush(buf);
		RendererRD::state.instance_data_index = 0;
	}
}

RID RendererCanvasRenderRD::light_create()
{
	CanvasLight canvas_light;
	return canvas_light_owner.make_rid(canvas_light);
}

void RendererCanvasRenderRD::light_set_texture(RID p_rid, RID p_texture)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(cl);
	if (cl->texture == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !RendererRD::TextureStorage::owns_texture(p_texture));

	if (cl->texture.is_valid()) {
		RendererRD::TextureStorage::texture_remove_from_decal_atlas(cl->texture);
	}
	cl->texture = p_texture;

	if (cl->texture.is_valid()) {
		RendererRD::TextureStorage::texture_add_to_decal_atlas(cl->texture);
	}
}

void RendererCanvasRenderRD::light_set_use_shadow(RID p_rid, bool p_enable)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(cl);

	cl->shadow.enabled = p_enable;
}

void RendererCanvasRenderRD::_update_shadow_atlas()
{
	if (RendererRD::state.shadow_fb == RID()) {
		// ah, we lack the shadow texture..
		RD::free_rid(RendererRD::state.shadow_texture); // erase placeholder

		Vector<RID> fb_textures;

		{ // texture
			RDC::TextureFormat tf;
			tf.texture_type = RDC::TEXTURE_TYPE_2D;
			tf.width = RendererRD::state.shadow_texture_size;
			tf.height = MAX_LIGHTS_PER_RENDER * 2;
			tf.usage_bits =
				RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
			tf.format = RDC::DATA_FORMAT_R32_SFLOAT;

			RendererRD::state.shadow_texture = RD::texture_create(tf, RDD::TextureView());
			fb_textures.push_back(RendererRD::state.shadow_texture);
		}
		{
			RDC::TextureFormat tf;
			tf.texture_type = RDC::TEXTURE_TYPE_2D;
			tf.width = RendererRD::state.shadow_texture_size;
			tf.height = MAX_LIGHTS_PER_RENDER * 2;
			tf.usage_bits = RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			tf.format = RDC::DATA_FORMAT_D32_SFLOAT;
			tf.is_discardable = true;
			// chunks to write
			RendererRD::state.shadow_depth_texture = RD::texture_create(tf, RDD::TextureView());
			fb_textures.push_back(RendererRD::state.shadow_depth_texture);
		}

		RendererRD::state.shadow_fb = RD::framebuffer_create(fb_textures);
	}
}

RID RendererCanvasRenderRD::occluder_polygon_create()
{
	OccluderPolygon occluder;
	occluder.line_point_count = 0;
	occluder.sdf_point_count = 0;
	occluder.sdf_index_count = 0;
	occluder.cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
	return occluder_polygon_owner.make_rid(occluder);
}

void RendererCanvasRenderRD::occluder_polygon_set_shape(
	RID p_occluder, const Vector<Vector2>& p_points, bool p_closed)
{
	OccluderPolygon* oc = occluder_polygon_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(oc);

	Vector<Vector2> lines;

	if (p_points.size()) {
		int lc = p_points.size() * 2;

		lines.resize(lc - (p_closed ? 0 : 2));
		{
			Vector2* w = lines.ptrw();
			const Vector2* r = p_points.ptr();

			int max = lc / 2;
			if (!p_closed) {
				max--;
			}
			for (int i = 0; i < max; i++) {
				Vector2 a = r[i];
				Vector2 b = r[(i + 1) % (lc / 2)];
				w[i * 2 + 0] = a;
				w[i * 2 + 1] = b;
			}
		}
	}

	if ((oc->line_point_count != lines.size() || lines.is_empty()) && oc->vertex_array.is_valid()) {
		RD::free_rid(oc->vertex_array);
		RD::free_rid(oc->vertex_buffer);
		RD::free_rid(oc->index_array);
		RD::free_rid(oc->index_buffer);

		oc->vertex_array = RID();
		oc->vertex_buffer = RID();
		oc->index_array = RID();
		oc->index_buffer = RID();

		oc->line_point_count = lines.size();
	}

	if (lines.size()) {
		oc->line_point_count = lines.size();
		Vector<uint8_t> geometry;
		Vector<uint8_t> indices;
		int lc = lines.size();

		geometry.resize(lc * 6 * sizeof(float));
		indices.resize(lc * 3 * sizeof(uint16_t));

		{
			uint8_t* vw = geometry.ptrw();
			float* vwptr = reinterpret_cast<float*>(vw);
			uint8_t* iw = indices.ptrw();
			uint16_t* iwptr = (uint16_t*)iw;

			const Vector2* lr = lines.ptr();

			const int POLY_HEIGHT = 16384;

			for (int i = 0; i < lc / 2; i++) {
				vwptr[i * 12 + 0] = lr[i * 2 + 0].x;
				vwptr[i * 12 + 1] = lr[i * 2 + 0].y;
				vwptr[i * 12 + 2] = POLY_HEIGHT;

				vwptr[i * 12 + 3] = lr[i * 2 + 1].x;
				vwptr[i * 12 + 4] = lr[i * 2 + 1].y;
				vwptr[i * 12 + 5] = POLY_HEIGHT;

				vwptr[i * 12 + 6] = lr[i * 2 + 1].x;
				vwptr[i * 12 + 7] = lr[i * 2 + 1].y;
				vwptr[i * 12 + 8] = -POLY_HEIGHT;

				vwptr[i * 12 + 9] = lr[i * 2 + 0].x;
				vwptr[i * 12 + 10] = lr[i * 2 + 0].y;
				vwptr[i * 12 + 11] = -POLY_HEIGHT;

				iwptr[i * 6 + 0] = i * 4 + 0;
				iwptr[i * 6 + 1] = i * 4 + 1;
				iwptr[i * 6 + 2] = i * 4 + 2;

				iwptr[i * 6 + 3] = i * 4 + 2;
				iwptr[i * 6 + 4] = i * 4 + 3;
				iwptr[i * 6 + 5] = i * 4 + 0;
			}
		}

		// if same buffer len is being set, just use buffer_update to avoid a pipeline flush

		if (oc->vertex_array.is_null()) {
			// create from scratch
			// vertices
			oc->vertex_buffer = RD::vertex_buffer_create(lc * 6 * sizeof(float), geometry);

			Vector<RID> buffer;
			buffer.push_back(oc->vertex_buffer);
			oc->vertex_array =
				RD::vertex_array_create(4 * lc / 2, shadow_render.vertex_format, buffer);
			oc->index_array = RD::index_array_create(oc->index_buffer, 0, 3 * lc);
		}
	}

	// sdf

	Vector<int> sdf_indices;

	if (p_points.size()) {
		if (p_closed) {
			sdf_indices = Geometry2D::triangulate_polygon(p_points);
			oc->sdf_is_lines = false;
		}
		else {
			int max = p_points.size();
			sdf_indices.resize(max * 2);

			int* iw = sdf_indices.ptrw();
			for (int i = 0; i < max; i++) {
				iw[i * 2 + 0] = i;
				iw[i * 2 + 1] = (i + 1) % max;
			}
			oc->sdf_is_lines = true;
		}
	}

	if (((oc->sdf_index_count != sdf_indices.size() && oc->sdf_point_count != p_points.size()) ||
			p_points.is_empty()) &&
		oc->sdf_vertex_array.is_valid()) {
		RD::free_rid(oc->sdf_vertex_array);
		RD::free_rid(oc->sdf_vertex_buffer);
		RD::free_rid(oc->sdf_index_array);
		RD::free_rid(oc->sdf_index_buffer);

		oc->sdf_vertex_array = RID();
		oc->sdf_vertex_buffer = RID();
		oc->sdf_index_array = RID();
		oc->sdf_index_buffer = RID();

		oc->sdf_index_count = sdf_indices.size();
		oc->sdf_point_count = p_points.size();

		oc->sdf_is_lines = false;
	}

	if (sdf_indices.size()) {
		if (oc->sdf_vertex_array.is_null()) {
			// create from scratch
			// vertices
#ifdef REAL_T_IS_DOUBLE
			PackedFloat32Array float_points;
			float_points.resize(p_points.size() * 2);
			float* float_points_ptr = (float*)float_points.ptrw();
			for (int i = 0; i < p_points.size(); i++) {
				float_points_ptr[i * 2] = p_points[i].x;
				float_points_ptr[i * 2 + 1] = p_points[i].y;
			}
			oc->sdf_vertex_buffer = RD::vertex_buffer_create(
				p_points.size() * 2 * sizeof(float), float_points.span().reinterpret<uint8_t>());
#else
			oc->sdf_vertex_buffer = RD::vertex_buffer_create(
				p_points.size() * 2 * sizeof(float), p_points.span().reinterpret<uint8_t>());
#endif
			oc->sdf_index_array =
				RD::index_array_create(oc->sdf_index_buffer, 0, sdf_indices.size());

			Vector<RID> buffer;
			buffer.push_back(oc->sdf_vertex_buffer);
			oc->sdf_vertex_array =
				RD::vertex_array_create(p_points.size(), shadow_render.sdf_vertex_format, buffer);
		}
	}
}

void RendererCanvasRenderRD::occluder_polygon_set_cull_mode(
	RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode)
{
	OccluderPolygon* oc = occluder_polygon_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(oc);
	oc->cull_mode = p_mode;
}

void RendererRD::CanvasShaderData::_clear_vertex_input_mask_cache()
{
	for (uint32_t i = 0; i < VERTEX_INPUT_MASKS_SIZE; i++) {
		vertex_input_masks[i].store(0);
	}
}

void RendererRD::CanvasShaderData::_create_pipeline(PipelineKey p_pipeline_key)
{
#if PRINT_PIPELINE_COMPILATION_KEYS
	print_line("HASH:", p_pipeline_key.hash(), "VERSION:", version,
		"VARIANT:", p_pipeline_key.variant, "FRAMEBUFFER:", p_pipeline_key.framebuffer_format_id,
		"VERTEX:", p_pipeline_key.vertex_format_id, "PRIMITIVE:", p_pipeline_key.render_primitive,
		"SPEC PACKED #0:", p_pipeline_key.shader_specialization.packed_0,
		"LCD:", p_pipeline_key.lcd_blend);
#endif

	RendererRD::MaterialStorage::ShaderData::BlendMode blend_mode_rd =
		RendererRD::MaterialStorage::ShaderData::BlendMode(blend_mode);
	RDC::PipelineColorBlendState blend_state;
	RDC::PipelineColorBlendState::Attachment attachment;
	uint32_t dynamic_state_flags = 0;
	if (p_pipeline_key.lcd_blend) {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_CONSTANT_COLOR;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		dynamic_state_flags = RDC::DYNAMIC_STATE_BLEND_CONSTANTS;
	}
	else {
		attachment =
			RendererRD::MaterialStorage::ShaderData::blend_mode_to_blend_attachment(blend_mode_rd);
	}

	blend_state.attachments.push_back(attachment);

	RDC::PipelineMultisampleState multisample_state;
	multisample_state.sample_count =
		RD::framebuffer_format_get_texture_samples(p_pipeline_key.framebuffer_format_id, 0);

	// Convert the specialization from the key to pipeline specialization constants.
	Vector<RDC::PipelineSpecializationConstant> specialization_constants;
	RDC::PipelineSpecializationConstant sc;
	sc.constant_id = 0;
	sc.int_value = p_pipeline_key.shader_specialization.packed_0;
	sc.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
	specialization_constants.push_back(sc);

	RID shader_rid = get_shader(p_pipeline_key.variant, p_pipeline_key.ubershader);
	ERR_FAIL_COND(shader_rid.is_null());

	RID pipeline = RD::render_pipeline_create(shader_rid, p_pipeline_key.framebuffer_format_id,
		p_pipeline_key.vertex_format_id, p_pipeline_key.render_primitive,
		RDC::PipelineRasterizationState(), multisample_state, RDC::PipelineDepthStencilState(),
		blend_state, dynamic_state_flags, 0, specialization_constants);
	ERR_FAIL_COND(pipeline.is_null());

	pipeline_hash_map.add_compiled_pipeline(p_pipeline_key.hash(), pipeline);
}

bool RendererRD::CanvasShaderData::is_animated() const { return false; }

bool RendererRD::CanvasShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode
RendererRD::CanvasShaderData::get_native_source_code() const
{
	MutexLock lock(shader.mutex);
	return shader.canvas_shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> RendererRD::CanvasShaderData::get_native_shader_and_version() const
{
	return {&shader.canvas_shader, version};
}

uint64_t RendererRD::CanvasShaderData::get_vertex_input_mask(
	ShaderVariant p_shader_variant, bool p_ubershader)
{
	// Vertex input masks require knowledge of the shader. Since querying the shader can be
	// expensive due to high contention and the necessary mutex, we cache the result instead.
	uint32_t input_mask_index = p_shader_variant + (p_ubershader ? SHADER_VARIANT_MAX : 0);
	uint64_t input_mask = vertex_input_masks[input_mask_index].load(std::memory_order_relaxed);
	if (input_mask == 0) {
		RID shader_rid = get_shader(p_shader_variant, p_ubershader);
		ERR_FAIL_COND_V(shader_rid.is_null(), 0);

		input_mask = RD::shader_get_vertex_input_attribute_mask(shader_rid);
		vertex_input_masks[input_mask_index].store(input_mask, std::memory_order_relaxed);
	}

	return input_mask;
}

bool RendererRD::CanvasShaderData::is_valid() const
{
	if (version.is_valid()) {
		MutexLock lock(shader.mutex);
		return shader.canvas_shader.version_is_valid(version);
	}
	else {
		return false;
	}
}

RendererRD::CanvasShaderData::CanvasShaderData()
{
	pipeline_hash_map.set_creation_object_and_function(this, &CanvasShaderData::_create_pipeline);
	pipeline_hash_map.set_compilations(&shader.pipeline_compilations[0], &shader.mutex);
}

RendererRD::MaterialStorage::ShaderData* RendererCanvasRenderRD::_create_shader_func()
{
	RendererRD::CanvasShaderData* shader_data = memnew(RendererRD::CanvasShaderData);
	return shader_data;
}

RendererRD::CanvasMaterialData::~CanvasMaterialData()
{
	free_parameters_uniform_set(uniform_set);
	free_parameters_uniform_set(uniform_set_srgb);
}

RendererRD::MaterialStorage::MaterialData* RendererCanvasRenderRD::_create_material_func(
	RendererRD::CanvasShaderData* p_shader)
{
	RendererRD::CanvasMaterialData* material_data = memnew(RendererRD::CanvasMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

void RendererCanvasRenderRD::set_time(double p_time) { RendererRD::state.time = p_time; }

void RendererCanvasRenderRD::update() {}

bool RendererCanvasRenderRD::free(RID p_rid)
{
	if (canvas_light_owner.owns(p_rid)) {
		CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(cl, false);
		light_set_use_shadow(p_rid, false);
		canvas_light_owner.free(p_rid);
	}
	else if (occluder_polygon_owner.owns(p_rid)) {
		occluder_polygon_set_shape(p_rid, Vector<Vector2>(), false);
		occluder_polygon_owner.free(p_rid);
	}
	else {
		return false;
	}

	return true;
}

void RendererCanvasRenderRD::set_shadow_texture_size(int p_size)
{
	p_size = MAX(1, Math::nearest_power_of_2_templated(p_size));
	if (p_size == RendererRD::state.shadow_texture_size) {
		return;
	}
	RendererRD::state.shadow_texture_size = p_size;
	if (RendererRD::state.shadow_fb.is_valid()) {
		RD::free_rid(RendererRD::state.shadow_texture);
		RD::free_rid(RendererRD::state.shadow_depth_texture);
		RendererRD::state.shadow_fb = RID();

		{
			// create a default shadow texture to keep uniform set happy (and that it gets erased
			// when a new one is created)
			RDC::TextureFormat tf;
			tf.texture_type = RDC::TEXTURE_TYPE_2D;
			tf.width = 4;
			tf.height = 4;
			tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT;
			tf.format = RDC::DATA_FORMAT_R32_SFLOAT;

			RendererRD::state.shadow_texture = RD::texture_create(tf, RDD::TextureView());
		}
	}
}

void RendererCanvasRenderRD::set_debug_redraw(bool p_enabled, double p_time, const Color& p_color)
{
	debug_redraw = p_enabled;
	debug_redraw_time = p_time;
	debug_redraw_color = p_color;
}

uint32_t RendererCanvasRenderRD::get_pipeline_compilations(RSE::PipelineSource p_source)
{
	MutexLock lock(RendererRD::shader.mutex);
	return RendererRD::shader.pipeline_compilations[p_source];
}

void RendererCanvasRenderRD::_render_batch_items(RenderTarget p_to_render_target, int p_item_count,
	const Transform2D& p_canvas_transform_inverse, Light* p_lights, bool& r_sdf_used,
	bool p_to_backbuffer, RenderingServerTypes::RenderInfo* r_render_info)
{
	// Record batches
	{
		RCR::Item* current_clip = nullptr;

		// First item always forms its own batch.
		bool batch_broken = false;
		RendererRD::Batch* current_batch = _new_batch(batch_broken);

		for (int i = 0; i < p_item_count; i++) {
			RCR::Item* ci = items[i];

			if (ci->final_clip_owner != current_batch->clip) {
				current_batch = _new_batch(batch_broken);
				current_batch->clip = ci->final_clip_owner;
				current_clip = ci->final_clip_owner;
			}

			RID material =
				ci->material_owner == nullptr ? ci->material : ci->material_owner->material;

			if (ci->use_canvas_group) {
				if (ci->canvas_group->mode == RSE::CANVAS_GROUP_MODE_CLIP_AND_DRAW) {
					material = default_clip_children_material;
				}
				else {
					if (material.is_null()) {
						if (ci->canvas_group->mode == RSE::CANVAS_GROUP_MODE_CLIP_ONLY) {
							material = default_clip_children_material;
						}
						else {
							material = default_canvas_group_material;
						}
					}
				}
			}

			if (material != current_batch->material) {
				current_batch = _new_batch(batch_broken);

				RendererRD::CanvasMaterialData* material_data = nullptr;
				if (material.is_valid()) {
					material_data = static_cast<RendererRD::CanvasMaterialData*>(
						RendererRD::MaterialStorage::material_get_data(
							material, RendererRD::MaterialStorage::SHADER_TYPE_2D));
				}

				current_batch->material = material;
				current_batch->material_data = material_data;
			}

			if (ci->repeat_source_item == nullptr || ci->repeat_size == Vector2()) {
				Transform2D base_transform = p_canvas_transform_inverse * ci->final_transform;
				_record_item_commands(ci, p_to_render_target, base_transform, current_clip,
					p_lights, batch_broken, r_sdf_used, current_batch);
			}
			else {
				Point2 start_pos = ci->repeat_size * -(ci->repeat_times / 2);
				Point2 offset;
				int repeat_times_x = ci->repeat_size.x ? ci->repeat_times : 0;
				int repeat_times_y = ci->repeat_size.y ? ci->repeat_times : 0;
				for (int ry = 0; ry <= repeat_times_y; ry++) {
					offset.y = start_pos.y + ry * ci->repeat_size.y;
					for (int rx = 0; rx <= repeat_times_x; rx++) {
						offset.x = start_pos.x + rx * ci->repeat_size.x;
						Transform2D base_transform = ci->final_transform;
						base_transform.columns[2] +=
							ci->repeat_source_item->final_transform.basis_xform(offset);
						base_transform = p_canvas_transform_inverse * base_transform;
						_record_item_commands(ci, p_to_render_target, base_transform, current_clip,
							p_lights, batch_broken, r_sdf_used, current_batch);
					}
				}
			}
		}
	}

	if (RendererRD::state.canvas_instance_batches.is_empty()) {
		return;
	}

	// Render batches
	RID framebuffer;
	RID fb_uniform_set;
	bool clear = false;
	Color clear_color;

	if (p_to_backbuffer) {
		framebuffer = RendererRD::TextureStorage::render_target_get_rd_backbuffer_framebuffer(
			p_to_render_target.render_target);
		fb_uniform_set = RendererRD::TextureStorage::render_target_get_backbuffer_uniform_set(
			p_to_render_target.render_target);
	}
	else {
		framebuffer = RendererRD::TextureStorage::render_target_get_rd_framebuffer(
			p_to_render_target.render_target);
		RendererRD::TextureStorage::render_target_set_msaa_needs_resolve(
			p_to_render_target.render_target, false);

		if (RendererRD::TextureStorage::render_target_is_clear_requested(
				p_to_render_target.render_target)) {
			clear = true;
			clear_color = RendererRD::TextureStorage::render_target_get_clear_request_color(
				p_to_render_target.render_target);
			if (RendererRD::TextureStorage::render_target_is_using_hdr(
					p_to_render_target.render_target)) {
				clear_color = clear_color.srgb_to_linear();
			}
			RendererRD::TextureStorage::render_target_disable_clear_request(
				p_to_render_target.render_target);
		}
		fb_uniform_set = RendererRD::TextureStorage::render_target_get_framebuffer_uniform_set(
			p_to_render_target.render_target);
	}

	if (fb_uniform_set.is_null() || !RD::uniform_set_is_valid(fb_uniform_set)) {
		fb_uniform_set = _create_base_uniform_set(p_to_render_target.render_target, p_to_backbuffer);
	}

	RD::FramebufferFormatID fb_format = RD::framebuffer_get_format(framebuffer);

	Vector<Color> clear_colors;
	if (clear) {
		clear_colors.push_back(clear_color);
	}
	RD::DrawListID draw_list = RD::draw_list_begin(framebuffer,
		clear ? RD::DRAW_CLEAR_COLOR_0 : RD::DRAW_DEFAULT_ALL, clear_colors, 1.0f, 0, Rect2(),
		RDC::BreadcrumbMarker::UI_PASS);

	RD::draw_list_bind_uniform_set(draw_list, fb_uniform_set, BASE_UNIFORM_SET);
	RD::draw_list_bind_uniform_set(
		draw_list, RendererRD::state.default_transforms_uniform_set, TRANSFORMS_UNIFORM_SET);

	RCR::Item* current_clip = nullptr;
	RendererRD::state.current_batch_uniform_set = RID();

	for (uint32_t i = 0; i <= RendererRD::state.current_batch_index; i++) {
		RendererRD::Batch* current_batch = &RendererRD::state.canvas_instance_batches[i];
		if (current_batch->instance_count == 0) {
			continue;
		}

		if (current_clip != current_batch->clip) {
			current_clip = current_batch->clip;
			if (current_clip) {
				RD::draw_list_enable_scissor(draw_list, current_clip->final_clip_rect);
			}
			else {
				RD::draw_list_disable_scissor(draw_list);
			}
		}

		RendererRD::CanvasShaderData* shader_data = RendererRD::shader.default_version_data;
		RendererRD::CanvasMaterialData* material_data = current_batch->material_data;
		if (material_data) {
			if (material_data->shader_data->version.is_valid() &&
				material_data->shader_data->is_valid()) {
				shader_data = material_data->shader_data;
				RID uniform_set =
					RendererRD::TextureStorage::render_target_is_using_hdr(
						p_to_render_target.render_target)
						? material_data->uniform_set
						: material_data->uniform_set_srgb;
				if (uniform_set.is_valid() && RD::uniform_set_is_valid(uniform_set)) {
					RD::draw_list_bind_uniform_set(draw_list, uniform_set, MATERIAL_UNIFORM_SET);
					material_data->set_as_used();
				}
			}
		}

		_render_batch(draw_list, shader_data, fb_format, p_lights, current_batch, r_render_info);
	}

	RD::draw_list_end();

	RendererRD::state.current_batch_index = 0;
	RendererRD::state.canvas_instance_batches.clear();
}

void RendererCanvasRenderRD::_record_item_commands(const RCR::Item* p_item, RenderTarget p_render_target,
	const Transform2D& p_base_transform, RCR::Item*& r_current_clip, Light* p_lights,
	bool& r_batch_broken, bool& r_sdf_used, RendererRD::Batch*& r_current_batch)
{
	const RSE::CanvasItemTextureFilter texture_filter =
		p_item->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? default_filter
																		  : p_item->texture_filter;
	const RSE::CanvasItemTextureRepeat texture_repeat =
		p_item->texture_repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? default_repeat
																		  : p_item->texture_repeat;

	Transform2D base_transform = p_base_transform;

	RendererRD::InstanceData template_instance;
	memset(&template_instance, 0, sizeof(RendererRD::InstanceData));

	Transform2D draw_transform; // Used by transform command
	_update_transform_2d_to_mat2x3(base_transform, template_instance.world);

	Color base_color = p_item->final_modulate;
	bool use_linear_colors = p_render_target.use_linear_colors;
	template_instance.instance_uniforms_ofs =
		static_cast<uint32_t>(p_item->instance_allocated_shader_uniforms_offset);

	bool reclip = false;

	bool skipping = false;

	uint16_t light_count = 0;
	uint16_t shadow_mask = 0;

	{
		Light* light = p_lights;

		while (light) {
			if (light->render_index_cache >= 0 && p_item->light_mask & light->item_mask &&
				p_item->z_final >= light->z_min && p_item->z_final <= light->z_max &&
				p_item->global_rect_cache.intersects(light->rect_cache)) {
				uint32_t light_index = light->render_index_cache;
				// TODO: consider making lights a per-batch property and then baking light
				// operations in the shader for better performance.
				template_instance.lights[light_count >> 2] |= light_index
															  << ((light_count & 3) * 8);

				if (p_item->light_mask & light->item_shadow_mask) {
					shadow_mask |= 1 << light_count;
				}

				light_count++;

				if (light_count == MAX_LIGHTS_PER_ITEM - 1) {
					break;
				}
			}
			light = light->next_ptr;
		}

		template_instance.flags |= light_count << INSTANCE_FLAGS_LIGHT_COUNT_SHIFT;
		template_instance.flags |= shadow_mask << INSTANCE_FLAGS_SHADOW_MASKED_SHIFT;
	}

	bool use_lighting = (light_count > 0 || using_directional_lights);

	if (use_lighting != r_current_batch->use_lighting) {
		r_current_batch = _new_batch(r_batch_broken);
		r_current_batch->use_lighting = use_lighting;
	}

	const RCR::Item::Command* c = p_item->commands;
	while (c) {
		if (skipping && c->type != RCR::Item::Command::TYPE_ANIMATION_SLICE) {
			c = c->next;
			continue;
		}

		switch (c->type) {
		case RCR::Item::Command::TYPE_RECT: {
			const RCR::Item::CommandRect* rect = reinterpret_cast<const RCR::Item::CommandRect*>(c);

			// 1: If commands are different, start a new batch.
			if (r_current_batch->command_type != RCR::Item::Command::TYPE_RECT) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = RCR::Item::Command::TYPE_RECT;
				r_current_batch->command = c;
				// default variant
				r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_QUAD;
				r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
				r_current_batch->flags = 0;
			}

			RSE::CanvasItemTextureRepeat rect_repeat = texture_repeat;
			if (bool(rect->flags & CANVAS_RECT_TILE)) {
				rect_repeat = RSE::CanvasItemTextureRepeat::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED;
			}

			Color modulated = rect->modulate * base_color;
			if (use_linear_colors) {
				modulated = modulated.srgb_to_linear();
			}

			bool has_blend = bool(rect->flags & CANVAS_RECT_LCD);
			// Start a new batch if the blend mode has changed,
			// or blend mode is enabled and the modulation has changed.
			if (has_blend != r_current_batch->has_blend ||
				(has_blend && modulated != r_current_batch->modulate)) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->has_blend = has_blend;
				r_current_batch->modulate = modulated;
				r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_QUAD;
				r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
			}

			bool has_msdf = bool(rect->flags & CANVAS_RECT_MSDF);
			RendererRD::TextureState tex_state(
				rect->texture, texture_filter, rect_repeat, has_msdf, use_linear_colors);
			RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
				_prepare_batch_texture_info(rect->texture, tex_state, tex_info);
			}

			if (has_msdf != r_current_batch->use_msdf ||
				rect->px_range != r_current_batch->msdf_pix_range ||
				rect->outline != r_current_batch->msdf_outline) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->use_msdf = has_msdf;
				r_current_batch->msdf_pix_range = rect->px_range;
				r_current_batch->msdf_outline = rect->outline;
			}

			bool has_lcd = bool(rect->flags & CANVAS_RECT_LCD);
			if (has_lcd != r_current_batch->use_lcd) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->use_lcd = has_lcd;
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			RendererRD::InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);
			Rect2 src_rect;
			Rect2 dst_rect;

			if (rect->texture.is_valid()) {
				src_rect = (rect->flags & CANVAS_RECT_REGION)
							   ? Rect2(rect->source.position * tex_info->texpixel_size,
									 rect->source.size * tex_info->texpixel_size)
							   : Rect2(0, 0, 1, 1);
				dst_rect = Rect2(rect->rect.position, rect->rect.size);

				if (dst_rect.size.width < 0) {
					dst_rect.position.x += dst_rect.size.width;
					dst_rect.size.width *= -1;
				}
				if (dst_rect.size.height < 0) {
					dst_rect.position.y += dst_rect.size.height;
					dst_rect.size.height *= -1;
				}

				if (rect->flags & CANVAS_RECT_FLIP_H) {
					src_rect.size.x *= -1;
				}

				if (rect->flags & CANVAS_RECT_FLIP_V) {
					src_rect.size.y *= -1;
				}

				if (rect->flags & CANVAS_RECT_TRANSPOSE) {
					instance_data->flags |= INSTANCE_FLAGS_TRANSPOSE_RECT;
				}

				if (rect->flags & CANVAS_RECT_CLIP_UV) {
					instance_data->flags |= INSTANCE_FLAGS_CLIP_RECT_UV;
				}

			}
			else {
				dst_rect = Rect2(rect->rect.position, rect->rect.size);

				if (dst_rect.size.width < 0) {
					dst_rect.position.x += dst_rect.size.width;
					dst_rect.size.width *= -1;
				}
				if (dst_rect.size.height < 0) {
					dst_rect.position.y += dst_rect.size.height;
					dst_rect.size.height *= -1;
				}

				src_rect = Rect2(0, 0, 1, 1);
			}

			instance_data->modulation[0] = modulated.r;
			instance_data->modulation[1] = modulated.g;
			instance_data->modulation[2] = modulated.b;
			instance_data->modulation[3] = modulated.a;

			instance_data->src_rect[0] = src_rect.position.x;
			instance_data->src_rect[1] = src_rect.position.y;
			instance_data->src_rect[2] = src_rect.size.width;
			instance_data->src_rect[3] = src_rect.size.height;

			instance_data->dst_rect[0] = dst_rect.position.x;
			instance_data->dst_rect[1] = dst_rect.position.y;
			instance_data->dst_rect[2] = dst_rect.size.width;
			instance_data->dst_rect[3] = dst_rect.size.height;

			_add_to_batch(r_batch_broken, r_current_batch);
		} break;

		case RCR::Item::Command::TYPE_NINEPATCH: {
			const RCR::Item::CommandNinePatch* np = reinterpret_cast<const RCR::Item::CommandNinePatch*>(c);

			if (r_current_batch->command_type != RCR::Item::Command::TYPE_NINEPATCH) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = RCR::Item::Command::TYPE_NINEPATCH;
				r_current_batch->command = c;
				r_current_batch->has_blend = false;
				r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_NINEPATCH;
				r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
				r_current_batch->flags = 0;
				r_current_batch->use_msdf = false;
				r_current_batch->use_lcd = false;
			}

			RendererRD::TextureState tex_state(
				np->texture, texture_filter, texture_repeat, false, use_linear_colors);
			RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
				_prepare_batch_texture_info(np->texture, tex_state, tex_info);
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			RendererRD::InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

			Rect2 src_rect;
			Rect2 dst_rect(
				np->rect.position.x, np->rect.position.y, np->rect.size.x, np->rect.size.y);

			if (np->texture.is_valid() && np->source != Rect2()) {
				src_rect = Rect2(np->source.position.x * tex_info->texpixel_size.width,
					np->source.position.y * tex_info->texpixel_size.height,
					np->source.size.x * tex_info->texpixel_size.width,
					np->source.size.y * tex_info->texpixel_size.height);
				instance_data->ninepatch_pixel_size[0] = 1.0 / np->source.size.width;
				instance_data->ninepatch_pixel_size[1] = 1.0 / np->source.size.height;
			}
			else {
				src_rect = Rect2(0, 0, 1, 1);
				// Set the default ninepatch pixel size to the full texture size.
				instance_data->ninepatch_pixel_size[0] = tex_info->texpixel_size.width;
				instance_data->ninepatch_pixel_size[1] = tex_info->texpixel_size.height;
			}

			Color modulated = np->color * base_color;
			if (use_linear_colors) {
				modulated = modulated.srgb_to_linear();
			}

			instance_data->modulation[0] = modulated.r;
			instance_data->modulation[1] = modulated.g;
			instance_data->modulation[2] = modulated.b;
			instance_data->modulation[3] = modulated.a;

			instance_data->src_rect[0] = src_rect.position.x;
			instance_data->src_rect[1] = src_rect.position.y;
			instance_data->src_rect[2] = src_rect.size.width;
			instance_data->src_rect[3] = src_rect.size.height;

			instance_data->dst_rect[0] = dst_rect.position.x;
			instance_data->dst_rect[1] = dst_rect.position.y;
			instance_data->dst_rect[2] = dst_rect.size.width;
			instance_data->dst_rect[3] = dst_rect.size.height;

			instance_data->flags |= int(np->axis_x) << INSTANCE_FLAGS_NINEPATCH_H_MODE_SHIFT;
			instance_data->flags |= int(np->axis_y) << INSTANCE_FLAGS_NINEPATCH_V_MODE_SHIFT;

			if (np->draw_center) {
				instance_data->flags |= INSTANCE_FLAGS_NINEPACH_DRAW_CENTER;
			}

			instance_data->ninepatch_margins[0] = np->margin[SIDE_LEFT];
			instance_data->ninepatch_margins[1] = np->margin[SIDE_TOP];
			instance_data->ninepatch_margins[2] = np->margin[SIDE_RIGHT];
			instance_data->ninepatch_margins[3] = np->margin[SIDE_BOTTOM];

			_add_to_batch(r_batch_broken, r_current_batch);
		} break;

		case RCR::Item::Command::TYPE_POLYGON: {
			const RCR::Item::CommandPolygon* polygon = reinterpret_cast<const RCR::Item::CommandPolygon*>(c);

			// Polygon's can't be batched, so always create a new batch
			r_current_batch = _new_batch(r_batch_broken);

			r_current_batch->command_type = RCR::Item::Command::TYPE_POLYGON;
			r_current_batch->has_blend = false;
			r_current_batch->command = c;
			r_current_batch->flags = 0;
			r_current_batch->use_msdf = false;
			r_current_batch->use_lcd = false;

			RendererRD::TextureState tex_state(
				polygon->texture, texture_filter, texture_repeat, false, use_linear_colors);
			RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
				_prepare_batch_texture_info(polygon->texture, tex_state, tex_info);
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			// pipeline variant
			{
				ERR_CONTINUE(polygon->primitive < 0 || polygon->primitive >= RSE::PRIMITIVE_MAX);
				r_current_batch->shader_variant = polygon->primitive == RSE::PRIMITIVE_POINTS
													  ? RendererRD::SHADER_VARIANT_ATTRIBUTES_POINTS
													  : RendererRD::SHADER_VARIANT_ATTRIBUTES;
				r_current_batch->render_primitive =
					_primitive_type_to_render_primitive(polygon->primitive);
			}

			RendererRD::InstanceData* instance_data =
				new_instance_data(*r_current_batch, template_instance, true);

			Color color = base_color;
			if (use_linear_colors) {
				color = color.srgb_to_linear();
			}

			instance_data->modulation[0] = color.r;
			instance_data->modulation[1] = color.g;
			instance_data->modulation[2] = color.b;
			instance_data->modulation[3] = color.a;
		} break;

		case RCR::Item::Command::TYPE_PRIMITIVE: {
			const RCR::Item::CommandPrimitive* primitive = reinterpret_cast<const RCR::Item::CommandPrimitive*>(c);

			if (primitive->point_count != r_current_batch->primitive_points ||
				r_current_batch->command_type != RCR::Item::Command::TYPE_PRIMITIVE) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = RCR::Item::Command::TYPE_PRIMITIVE;
				r_current_batch->has_blend = false;
				r_current_batch->command = c;
				r_current_batch->primitive_points = primitive->point_count;
				r_current_batch->flags = 0;

				ERR_CONTINUE(primitive->point_count == 0 || primitive->point_count > 4);

				switch (primitive->point_count) {
				case 1:
					r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_PRIMITIVE_POINTS;
					r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_POINTS;
					break;
				case 2:
					r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_PRIMITIVE;
					r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_LINES;
					break;
				case 3:
				case 4:
					r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_PRIMITIVE;
					r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
					break;
				default:
					// Unknown point count.
					break;
				}
			}

			RendererRD::TextureState tex_state(
				primitive->texture, texture_filter, texture_repeat, false, use_linear_colors);
			RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
				_prepare_batch_texture_info(primitive->texture, tex_state, tex_info);
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			RendererRD::InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

			for (uint32_t j = 0; j < MIN(3u, primitive->point_count); j++) {
				instance_data->points[j * 2 + 0] = primitive->points[j].x;
				instance_data->points[j * 2 + 1] = primitive->points[j].y;
				instance_data->uvs[j * 2 + 0] = primitive->uvs[j].x;
				instance_data->uvs[j * 2 + 1] = primitive->uvs[j].y;
				Color col = primitive->colors[j] * base_color;
				if (use_linear_colors) {
					col = col.srgb_to_linear();
				}
				instance_data->colors[j * 2 + 0] =
					(uint32_t(Math::make_half_float(col.g)) << 16) | Math::make_half_float(col.r);
				instance_data->colors[j * 2 + 1] =
					(uint32_t(Math::make_half_float(col.a)) << 16) | Math::make_half_float(col.b);
			}

			_add_to_batch(r_batch_broken, r_current_batch);

			if (primitive->point_count == 4) {
				instance_data = new_instance_data(*r_current_batch, template_instance);

				for (uint32_t j = 0; j < 3; j++) {
					int offset = j == 0 ? 0 : 1;
					// Second triangle in the quad. Uses vertices 0, 2, 3.
					instance_data->points[j * 2 + 0] = primitive->points[j + offset].x;
					instance_data->points[j * 2 + 1] = primitive->points[j + offset].y;
					instance_data->uvs[j * 2 + 0] = primitive->uvs[j + offset].x;
					instance_data->uvs[j * 2 + 1] = primitive->uvs[j + offset].y;
					Color col = primitive->colors[j + offset] * base_color;
					if (use_linear_colors) {
						col = col.srgb_to_linear();
					}
					instance_data->colors[j * 2 + 0] =
						(uint32_t(Math::make_half_float(col.g)) << 16) |
						Math::make_half_float(col.r);
					instance_data->colors[j * 2 + 1] =
						(uint32_t(Math::make_half_float(col.a)) << 16) |
						Math::make_half_float(col.b);
				}

				_add_to_batch(r_batch_broken, r_current_batch);
			}
		} break;

		case RCR::Item::Command::TYPE_MESH:
		case RCR::Item::Command::TYPE_MULTIMESH:
		case RCR::Item::Command::TYPE_PARTICLES: {
			// Mesh's can't be batched, so always create a new batch
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->command = c;
			r_current_batch->command_type = c->type;
			r_current_batch->has_blend = false;
			r_current_batch->flags = 0;
			r_current_batch->use_msdf = false;
			r_current_batch->use_lcd = false;

			RendererRD::InstanceData* instance_data = nullptr;

			Color modulate(1, 1, 1, 1);
			if (c->type == RCR::Item::Command::TYPE_MESH) {
				const RCR::Item::CommandMesh* m = reinterpret_cast<const RCR::Item::CommandMesh*>(c);
				RendererRD::TextureState tex_state(
					m->texture, texture_filter, texture_repeat, false, use_linear_colors);
				RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
					_prepare_batch_texture_info(m->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				r_current_batch->mesh_instance_count = 1;
				_update_transform_2d_to_mat2x3(
					base_transform * draw_transform * m->transform, instance_data->world);
				modulate = m->modulate;
			}
			else if (c->type == RCR::Item::Command::TYPE_MULTIMESH) {
				const RCR::Item::CommandMultiMesh* mm = reinterpret_cast<const RCR::Item::CommandMultiMesh*>(c);
				RID multimesh = mm->multimesh;

				if (RendererRD::MeshStorage::multimesh_get_transform_format(multimesh) !=
					RSE::MULTIMESH_TRANSFORM_2D) {
					break;
				}

				r_current_batch->mesh_instance_count =
					RendererRD::MeshStorage::multimesh_get_instances_to_draw(multimesh);
				if (r_current_batch->mesh_instance_count == 0) {
					break;
				}

				RendererRD::TextureState tex_state(
					mm->texture, texture_filter, texture_repeat, false, use_linear_colors);
				RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
					_prepare_batch_texture_info(mm->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				r_current_batch->flags |= 1; // multimesh, trails disabled

				if (RendererRD::MeshStorage::multimesh_uses_colors(mm->multimesh)) {
					r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_COLORS;
				}
				if (RendererRD::MeshStorage::multimesh_uses_custom_data(mm->multimesh)) {
					r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;
				}
			}
			else if (c->type == RCR::Item::Command::TYPE_PARTICLES) {
				const RCR::Item::CommandParticles* pt = reinterpret_cast<const RCR::Item::CommandParticles*>(c);
				RendererRD::TextureState tex_state(
					pt->texture, texture_filter, texture_repeat, false, use_linear_colors);
				RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
					_prepare_batch_texture_info(pt->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				uint32_t divisor = 1;
				r_current_batch->mesh_instance_count =
					RendererRD::ParticlesStorage::particles_get_amount(pt->particles, divisor);
				r_current_batch->flags |= (divisor & BATCH_FLAGS_INSTANCING_MASK);
				r_current_batch->mesh_instance_count /= divisor;

				RID particles = pt->particles;

				r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_COLORS;
				r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;

				if (RendererRD::ParticlesStorage::particles_has_collision(particles) &&
					RendererRD::TextureStorage::render_target_is_sdf_enabled(
						p_render_target.render_target)) {
					// Pass collision information.
					Transform2D xform = p_item->final_transform;

					RID sdf_texture = RendererRD::TextureStorage::render_target_get_sdf_texture(
						p_render_target.render_target);

					Rect2 to_screen;
					{
						Rect2 sdf_rect = RendererRD::TextureStorage::render_target_get_sdf_rect(
							p_render_target.render_target);

						to_screen.size =
							Vector2(1.0 / sdf_rect.size.width, 1.0 / sdf_rect.size.height);
						to_screen.position = -sdf_rect.position * to_screen.size;
					}

					RendererRD::ParticlesStorage::particles_set_canvas_sdf_collision(
						pt->particles, true, xform, to_screen, sdf_texture);
				}
				else {
					RendererRD::ParticlesStorage::particles_set_canvas_sdf_collision(
						pt->particles, false, Transform2D(), Rect2(), RID());
				}
				r_sdf_used |= RendererRD::ParticlesStorage::particles_has_collision(particles);
			}

			Color modulated = modulate * base_color;
			if (use_linear_colors) {
				modulated = modulated.srgb_to_linear();
			}

			instance_data->modulation[0] = modulated.r;
			instance_data->modulation[1] = modulated.g;
			instance_data->modulation[2] = modulated.b;
			instance_data->modulation[3] = modulated.a;
		} break;

		case RCR::Item::Command::TYPE_TRANSFORM: {
			const RCR::Item::CommandTransform* transform = reinterpret_cast<const RCR::Item::CommandTransform*>(c);
			draw_transform = transform->xform;
			_update_transform_2d_to_mat2x3(
				base_transform * transform->xform, template_instance.world);
		} break;

		case RCR::Item::Command::TYPE_CLIP_IGNORE: {
			const RCR::Item::CommandClipIgnore* ci = reinterpret_cast<const RCR::Item::CommandClipIgnore*>(c);
			if (r_current_clip) {
				if (ci->ignore != reclip) {
					r_current_batch = _new_batch(r_batch_broken);
					if (ci->ignore) {
						r_current_batch->clip = nullptr;
						reclip = true;
					}
					else {
						r_current_batch->clip = r_current_clip;
						reclip = false;
					}
				}
			}
		} break;

		case RCR::Item::Command::TYPE_ANIMATION_SLICE: {
			const RCR::Item::CommandAnimationSlice* as =
				reinterpret_cast<const RCR::Item::CommandAnimationSlice*>(c);
			double current_time = RendererCompositor::get_total_time();
			double local_time = Math::fposmod(current_time - as->offset, as->animation_length);
			skipping = !(local_time >= as->slice_begin && local_time < as->slice_end);

			Renderer::redraw_request(); // animation visible means redraw request
		} break;
		}

		c = c->next;
		r_batch_broken = false;
	}

#ifdef DEBUG_ENABLED
	if (debug_redraw && p_item->debug_redraw_time > 0.0) {
		Color dc = debug_redraw_color;
		dc.a *= p_item->debug_redraw_time / debug_redraw_time;

		// 1: If commands are different, start a new batch.
		if (r_current_batch->command_type != RCR::Item::Command::TYPE_RECT) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->command_type = RCR::Item::Command::TYPE_RECT;
			// it is ok to be null for a TYPE_RECT
			r_current_batch->command = nullptr;
			// default variant
			r_current_batch->shader_variant = RendererRD::SHADER_VARIANT_QUAD;
			r_current_batch->render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
			r_current_batch->flags = 0;
		}

		// 2: If the current batch has lighting, start a new batch.
		if (r_current_batch->use_lighting) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->use_lighting = false;
		}

		// 3: If the current batch has blend, start a new batch.
		if (r_current_batch->has_blend) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->has_blend = false;
		}

		RendererRD::TextureState tex_state(
			default_canvas_texture, texture_filter, texture_repeat, false, use_linear_colors);
		RendererRD::TextureInfo* tex_info = texture_info_map.getptr(tex_state);
		if (!tex_info) {
			tex_info = &texture_info_map.insert(tex_state, RendererRD::TextureInfo())->value;
			_prepare_batch_texture_info(default_canvas_texture, tex_state, tex_info);
		}

		if (r_current_batch->tex_info != tex_info) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->tex_info = tex_info;
		}

		_update_transform_2d_to_mat2x3(base_transform, template_instance.world);
		RendererRD::InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

		Rect2 src_rect;
		Rect2 dst_rect;

		dst_rect = p_item->rect;
		if (dst_rect.size.width < 0) {
			dst_rect.position.x += dst_rect.size.width;
			dst_rect.size.width *= -1;
		}
		if (dst_rect.size.height < 0) {
			dst_rect.position.y += dst_rect.size.height;
			dst_rect.size.height *= -1;
		}

		src_rect = Rect2(0, 0, 1, 1);

		instance_data->modulation[0] = dc.r;
		instance_data->modulation[1] = dc.g;
		instance_data->modulation[2] = dc.b;
		instance_data->modulation[3] = dc.a;

		instance_data->src_rect[0] = src_rect.position.x;
		instance_data->src_rect[1] = src_rect.position.y;
		instance_data->src_rect[2] = src_rect.size.width;
		instance_data->src_rect[3] = src_rect.size.height;

		instance_data->dst_rect[0] = dst_rect.position.x;
		instance_data->dst_rect[1] = dst_rect.position.y;
		instance_data->dst_rect[2] = dst_rect.size.width;
		instance_data->dst_rect[3] = dst_rect.size.height;

		_add_to_batch(r_batch_broken, r_current_batch);

		p_item->debug_redraw_time -= RendererCompositor::get_frame_delta_time();

		Renderer::redraw_request();

		r_batch_broken = false;
	}
#endif

	if (r_current_clip && reclip) {
		// will make it re-enable clipping if needed afterwards
		r_current_clip = nullptr;
	}
}

void RendererCanvasRenderRD::_before_evict(RendererCanvasRenderRD::RIDSetKey& p_key, RID& p_rid)
{
	RD::uniform_set_set_invalidation_callback(p_rid, nullptr, nullptr);
	RD::free_rid(p_rid);
}

void RendererCanvasRenderRD::_uniform_set_invalidation_callback(void* p_userdata)
{
	const RIDSetKey* key = static_cast<RIDSetKey*>(p_userdata);
	rid_set_to_uniform_set.erase(*key);
}

void RendererCanvasRenderRD::_canvas_texture_invalidation_callback(bool p_deleted, void* p_userdata)
{
	KeyValue<RID, TightLocalVector<RID>>* kv =
		static_cast<KeyValue<RID, TightLocalVector<RID>>*>(p_userdata);
	for (RID rid : kv->value) {
		// The invalidation callback will also take care of clearing rid_set_to_uniform_set cache.
		RD::free_rid(rid);
	}
	kv->value.clear();
	if (p_deleted) {
		canvas_texture_to_uniform_set.erase(kv->key);
	}
}

void RendererCanvasRenderRD::_render_batch(RD::DrawListID p_draw_list,
	RendererRD::CanvasShaderData* p_shader_data, RenderingDevice::FramebufferFormatID p_framebuffer_format,
	Light* p_lights, const RendererRD::Batch* p_batch, RenderingServerTypes::RenderInfo* r_render_info)
{
	{
		RIDSetKey key(p_batch->tex_info->state);

		const RID* uniform_set = rid_set_to_uniform_set.getptr(key);
		if (uniform_set == nullptr) {
			RD::Uniform* uniform_ptrw = RendererRD::state.batch_texture_uniforms.ptrw();
			uniform_ptrw[0] = RD::Uniform(RDC::UNIFORM_TYPE_TEXTURE, 0, p_batch->tex_info->diffuse);
			uniform_ptrw[1] = RD::Uniform(RDC::UNIFORM_TYPE_TEXTURE, 1, p_batch->tex_info->normal);
			uniform_ptrw[2] =
				RD::Uniform(RDC::UNIFORM_TYPE_TEXTURE, 2, p_batch->tex_info->specular);
			uniform_ptrw[3] = RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, 3, p_batch->tex_info->sampler);

			RID rid = RD::uniform_set_create(
				RendererRD::state.batch_texture_uniforms, RendererRD::shader.default_version_rd_shader, BATCH_UNIFORM_SET);
			ERR_FAIL_COND_MSG(rid.is_null(), "Failed to create uniform set for batch.");

			const RIDCache::Pair* iter = rid_set_to_uniform_set.insert(key, rid);
			uniform_set = &iter->data;
			RD::uniform_set_set_invalidation_callback(
				rid, RendererCanvasRenderRD::_uniform_set_invalidation_callback, (void*)&iter->key);

			// If this is a CanvasTexture, it must be tracked so that any changes to the diffuse,
			// normal, or specular channels invalidate all associated uniform sets.
			if (RendererRD::TextureStorage::owns_canvas_texture(p_batch->tex_info->state.texture)) {
				KeyValue<RID, TightLocalVector<RID>>* kv = nullptr;
				if (HashMap<RID, TightLocalVector<RID>>::Iterator i =
						canvas_texture_to_uniform_set.find(p_batch->tex_info->state.texture);
					i == canvas_texture_to_uniform_set.end()) {
					kv = &*canvas_texture_to_uniform_set.insert(
						p_batch->tex_info->state.texture, {*uniform_set});
				}
				else {
					i->value.push_back(rid);
					kv = &*i;
				}
				RendererRD::TextureStorage::canvas_texture_set_invalidation_callback(
					p_batch->tex_info->state.texture,
					RendererCanvasRenderRD::_canvas_texture_invalidation_callback, kv);
			}
		}

		if (RendererRD::state.current_batch_uniform_set != *uniform_set) {
			RendererRD::state.current_batch_uniform_set = *uniform_set;
			RD::draw_list_bind_uniform_set(p_draw_list, *uniform_set, BATCH_UNIFORM_SET);
		}
	}

	RID pipeline;
	RendererRD::PipelineKey pipeline_key;
	pipeline_key.framebuffer_format_id = p_framebuffer_format;
	pipeline_key.variant = p_batch->shader_variant;
	pipeline_key.render_primitive = p_batch->render_primitive;
	pipeline_key.shader_specialization.use_lighting = p_batch->use_lighting;
	pipeline_key.shader_specialization.use_msdf = p_batch->use_msdf;
	pipeline_key.shader_specialization.use_lcd = p_batch->use_lcd;
	pipeline_key.lcd_blend = p_batch->has_blend;

	switch (p_batch->command_type) {
	case RCR::Item::Command::TYPE_RECT:
	case RCR::Item::Command::TYPE_NINEPATCH: {
		RendererRD::PushConstant push_constant = p_batch->push_constant();

		pipeline_key.vertex_format_id = RendererRD::shader.quad_vertex_format_id;
		pipeline =
			_get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key, push_constant);
		RD::draw_list_bind_render_pipeline(p_draw_list, pipeline);
		if (p_batch->has_blend) {
			RD::draw_list_set_blend_constants(p_draw_list, p_batch->modulate);
		}

		RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(push_constant));
		FixedVector<RID, 1> vb = {p_batch->instance_buffer};
		FixedVector<uint64_t, 1> vo = {
			uint64_t(p_batch->start) * sizeof(RendererRD::InstanceData)};
		RD::draw_list_bind_vertex_buffers_format(
			p_draw_list, RendererRD::shader.quad_vertex_format_id, 1, vb, vo);
		RD::draw_list_bind_index_array(p_draw_list, RendererRD::shader.quad_index_array);
		RD::draw_list_draw(p_draw_list, true, p_batch->instance_count);

		if (r_render_info) {
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME] +=
				p_batch->instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				2 * p_batch->instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}
	} break;

	case RCR::Item::Command::TYPE_POLYGON: {
		ERR_FAIL_NULL(p_batch->command);
		RendererRD::PushConstantAttributes push_constant = p_batch->push_constant_attributes();

		const RCR::Item::CommandPolygon* polygon =
			reinterpret_cast<const RCR::Item::CommandPolygon*>(p_batch->command);

		RendererRD::PolygonBuffers* pb = RendererRD::polygon_buffers.polygons.getptr(polygon->polygon.polygon_id);
		ERR_FAIL_NULL(pb);

		pipeline_key.vertex_format_id = pb->vertex_format_id;
		pipeline =
			_get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key, push_constant);
		RD::draw_list_bind_render_pipeline(p_draw_list, pipeline);

		RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(push_constant));
		RD::draw_list_bind_vertex_array(p_draw_list, pb->vertex_array);
		if (pb->indices.is_valid()) {
			RD::draw_list_bind_index_array(p_draw_list, pb->indices);
		}

		RD::draw_list_draw(p_draw_list, pb->indices.is_valid());
		if (r_render_info) {
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME]++;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				_indices_to_primitives(polygon->primitive, pb->primitive_count);
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}
	} break;

	case RCR::Item::Command::TYPE_PRIMITIVE: {
		ERR_FAIL_NULL(p_batch->command);

		const RCR::Item::CommandPrimitive* primitive =
			reinterpret_cast<const RCR::Item::CommandPrimitive*>(p_batch->command);

		RendererRD::PushConstant push_constant = p_batch->push_constant();
		pipeline_key.vertex_format_id = RendererRD::shader.primitive_vertex_format_id;
		pipeline =
			_get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key, push_constant);
		RD::draw_list_bind_render_pipeline(p_draw_list, pipeline);

		RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(push_constant));
		FixedVector<RID, 1> vb = {p_batch->instance_buffer};
		FixedVector<uint64_t, 1> vo = {
			uint64_t(p_batch->start) * sizeof(RendererRD::InstanceData)};
		RD::draw_list_bind_vertex_buffers_format(
			p_draw_list, RendererRD::shader.primitive_vertex_format_id, 1, vb, vo);
		RD::draw_list_bind_index_array(
			p_draw_list, primitive_arrays.index_array[MIN(3u, primitive->point_count) - 1]);
		uint32_t instance_count = p_batch->instance_count;
		RD::draw_list_draw(p_draw_list, true, instance_count);

		if (r_render_info) {
			const RSE::PrimitiveType rs_primitive[5] = {RSE::PRIMITIVE_POINTS,
				RSE::PRIMITIVE_POINTS, RSE::PRIMITIVE_LINES, RSE::PRIMITIVE_TRIANGLES,
				RSE::PRIMITIVE_TRIANGLES};
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME] += instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				_indices_to_primitives(
					rs_primitive[p_batch->primitive_points], p_batch->primitive_points) *
				instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}
	} break;

	case RCR::Item::Command::TYPE_MESH:
	case RCR::Item::Command::TYPE_MULTIMESH:
	case RCR::Item::Command::TYPE_PARTICLES: {
		ERR_FAIL_NULL(p_batch->command);

		RendererRD::PushConstantAttributes push_constant = p_batch->push_constant_attributes();

		RID mesh;
		RID mesh_instance;

		if (p_batch->command_type == RCR::Item::Command::TYPE_MESH) {
			const RCR::Item::CommandMesh* m = reinterpret_cast<const RCR::Item::CommandMesh*>(p_batch->command);
			mesh = m->mesh;
			mesh_instance = m->mesh_instance;
		}
		else if (p_batch->command_type == RCR::Item::Command::TYPE_MULTIMESH) {
			const RCR::Item::CommandMultiMesh* mm =
				reinterpret_cast<const RCR::Item::CommandMultiMesh*>(p_batch->command);
			RID multimesh = mm->multimesh;
			mesh = RendererRD::MeshStorage::_multimesh_get_mesh(multimesh);

			RID uniform_set = RendererRD::MeshStorage::multimesh_get_2d_uniform_set(
				multimesh, RendererRD::shader.default_version_rd_shader, TRANSFORMS_UNIFORM_SET);
			RD::draw_list_bind_uniform_set(p_draw_list, uniform_set, TRANSFORMS_UNIFORM_SET);
		}
		else if (p_batch->command_type == RCR::Item::Command::TYPE_PARTICLES) {
			const RCR::Item::CommandParticles* pt =
				reinterpret_cast<const RCR::Item::CommandParticles*>(p_batch->command);
			RID particles = pt->particles;
			mesh = RendererRD::ParticlesStorage::particles_get_draw_pass_mesh(particles, 0);

			ERR_BREAK(RendererRD::ParticlesStorage::particles_get_mode(particles) != RSE::PARTICLES_MODE_2D);
			RendererRD::ParticlesStorage::particles_request_process(particles);

			if (RendererRD::ParticlesStorage::particles_is_inactive(particles)) {
				break;
			}

			Renderer::redraw_request(); // Active particles means redraw request.

			int dpc = RendererRD::ParticlesStorage::particles_get_draw_passes(particles);
			if (dpc == 0) {
				break; // Nothing to draw.
			}

			RID uniform_set = RendererRD::ParticlesStorage::particles_get_instance_buffer_uniform_set(
				pt->particles, RendererRD::shader.default_version_rd_shader, TRANSFORMS_UNIFORM_SET);
			RD::draw_list_bind_uniform_set(p_draw_list, uniform_set, TRANSFORMS_UNIFORM_SET);
		}

		if (mesh.is_null()) {
			break;
		}

		uint32_t surf_count = RendererRD::MeshStorage::mesh_get_surface_count(mesh);

		for (uint32_t j = 0; j < surf_count; j++) {
			void* surface = RendererRD::MeshStorage::mesh_get_surface(mesh, j);

			RSE::PrimitiveType primitive =
				RendererRD::MeshStorage::mesh_surface_get_primitive(surface);
			ERR_CONTINUE(primitive < 0 || primitive >= RSE::PRIMITIVE_MAX);

			RID vertex_array;
			pipeline_key.variant = primitive == RSE::PRIMITIVE_POINTS
									   ? RendererRD::SHADER_VARIANT_ATTRIBUTES_POINTS
									   : RendererRD::SHADER_VARIANT_ATTRIBUTES;
			pipeline_key.render_primitive = _primitive_type_to_render_primitive(primitive);
			pipeline_key.vertex_format_id = RD::INVALID_FORMAT_ID;

			pipeline = _get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key,
				push_constant, mesh_instance, surface, j, &vertex_array);
			RD::draw_list_bind_render_pipeline(p_draw_list, pipeline);

			RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(push_constant));

			RID index_array = RendererRD::MeshStorage::mesh_surface_get_index_array(surface, 0);

			if (index_array.is_valid()) {
				RD::draw_list_bind_index_array(p_draw_list, index_array);
			}

			RD::draw_list_bind_vertex_array(p_draw_list, vertex_array);
			RD::draw_list_draw(p_draw_list, index_array.is_valid(), p_batch->mesh_instance_count);

			if (r_render_info) {
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME]++;
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
					_indices_to_primitives(primitive,
						RendererRD::MeshStorage::mesh_surface_get_vertices_drawn_count(surface)) *
					p_batch->mesh_instance_count;
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
			}
		}
	} break;
	case RCR::Item::Command::TYPE_TRANSFORM:
	case RCR::Item::Command::TYPE_CLIP_IGNORE:
	case RCR::Item::Command::TYPE_ANIMATION_SLICE: {
		// Can ignore these as they only impact batch creation.
	} break;
	}
}

RendererRD::InstanceData* RendererCanvasRenderRD::new_instance_data(
	RendererRD::Batch& p_current_batch, const RendererRD::InstanceData& template_instance, bool p_use_push_data)
{
	RendererRD::InstanceData* instance_data = nullptr;

	if (unlikely(p_use_push_data)) {
		instance_data = &p_current_batch.push_data;
		// instance_count must be > 0 to indicate the batch has been used when calling _new_batch,
		// so we set a flag.
		p_current_batch.instance_count = PUSH_DATA_INSTANCE_COUNT;
	}
	else {
		// Return the intermediary instance data to prevent the caller from accidentally reading
		// write-combined memory pages, which has huge performance implications.
		instance_data = &RendererRD::state.intermediary_instance_data;
	}

	memcpy(instance_data, &template_instance, sizeof(RendererRD::InstanceData));
	return instance_data;
}

RendererRD::Batch* RendererCanvasRenderRD::_new_batch(bool& r_batch_broken)
{
	if (RendererRD::state.canvas_instance_batches.is_empty()) {
		RendererRD::Batch new_batch;
		// First try to reuse previous instance buffer if possible.
		if (RendererRD::state.prev_instance_data &&
			RendererRD::state.prev_instance_data_index < RendererRD::state.max_instances_per_buffer) {
			bool must_remap = RendererRD::state.instance_buffers.prepare_for_map(true);
			// must_remap will be false if we're preparing to map the buffer for the same frame and
			// can reuse the existing UMA buffer.
			if (!must_remap) {
				RendererRD::state.instance_data = RendererRD::state.prev_instance_data;
				RendererRD::state.instance_data_index = RendererRD::state.prev_instance_data_index;
			}
			RendererRD::state.prev_instance_data = nullptr;
			RendererRD::state.prev_instance_data_index = 0;
		}
		// This will still be a valid point when multiple calls to _render_batch_items
		// are made in the same draw call.
		if (RendererRD::state.instance_data == nullptr) {
			// If there is no existing instance buffer, we must allocate a new one.
			_allocate_instance_buffer();
		}
		else {
			// Otherwise, just use the existing one from where it last left off.
			new_batch.start = RendererRD::state.instance_data_index;
		}
		new_batch.instance_buffer = RendererRD::state.instance_buffers._get(0);
		RendererRD::state.canvas_instance_batches.push_back(new_batch);
		return RendererRD::state.canvas_instance_batches.ptr();
	}

	if (r_batch_broken ||
		RendererRD::state.canvas_instance_batches[RendererRD::state.current_batch_index].instance_count == 0) {
		return &RendererRD::state.canvas_instance_batches[RendererRD::state.current_batch_index];
	}

	r_batch_broken = true;

	// Copy the properties of the current batch, we will manually update the things that changed.
	RendererRD::Batch new_batch = RendererRD::state.canvas_instance_batches[RendererRD::state.current_batch_index];
	new_batch.instance_count = 0;
	new_batch.start = RendererRD::state.instance_data_index;
	memset(&new_batch.push_data, 0, sizeof(new_batch.push_data));
	RendererRD::state.current_batch_index++;
	RendererRD::state.canvas_instance_batches.push_back(new_batch);
	return &RendererRD::state.canvas_instance_batches[RendererRD::state.current_batch_index];
}

void RendererCanvasRenderRD::_add_to_batch(bool& r_batch_broken, RendererRD::Batch*& r_current_batch)
{
	DEV_ASSERT(r_current_batch->command_type == RCR::Item::Command::TYPE_RECT ||
			   r_current_batch->command_type == RCR::Item::Command::TYPE_NINEPATCH ||
			   r_current_batch->command_type == RCR::Item::Command::TYPE_PRIMITIVE);
	r_current_batch->instance_count++;
	memcpy(&RendererRD::state.instance_data[RendererRD::state.instance_data_index], &RendererRD::state.intermediary_instance_data,
		sizeof(RendererRD::InstanceData));
	RendererRD::state.instance_data_index++;
	if (RendererRD::state.instance_data_index >= RendererRD::state.max_instances_per_buffer) {
		RD::buffer_flush(r_current_batch->instance_buffer);
		RendererRD::state.instance_data = nullptr;
		_allocate_instance_buffer();
		RendererRD::state.instance_data_index = 0;
		r_batch_broken = false; // Force a new batch to be created
		r_current_batch = _new_batch(r_batch_broken);
		r_current_batch->instance_buffer = RendererRD::state.instance_buffers._get(0);
	}
}

void RendererCanvasRenderRD::_allocate_instance_buffer()
{
	RendererRD::state.instance_buffers.prepare_for_upload();
	RendererRD::state.instance_data =
		reinterpret_cast<RendererRD::InstanceData*>(RendererRD::state.instance_buffers.map_raw_for_upload(0));
}

void RendererCanvasRenderRD::_prepare_batch_texture_info(
	RID p_texture, RendererRD::TextureState& p_state, RendererRD::TextureInfo* p_info)
{
	if (p_texture.is_null()) {
		p_texture = default_canvas_texture;
	}

	RendererRD::TextureStorage::CanvasTextureInfo info =
		RendererRD::TextureStorage::canvas_texture_get_info(p_texture, p_state.texture_filter(),
			p_state.texture_repeat(), p_state.linear_colors(), p_state.texture_is_data());
	// something odd happened
	if (info.is_null()) {
		_prepare_batch_texture_info(default_canvas_texture, p_state, p_info);
		return;
	}

	p_info->state = p_state;
	p_info->diffuse = info.diffuse;
	p_info->normal = info.normal;
	p_info->specular = info.specular;
	p_info->sampler = info.sampler;

	// cache values to be copied to instance data
	if (info.specular_color.a < 0.999) {
		p_info->flags |= BATCH_FLAGS_DEFAULT_SPECULAR_MAP_USED;
	}

	if (info.use_normal) {
		p_info->flags |= BATCH_FLAGS_DEFAULT_NORMAL_MAP_USED;
	}

	uint8_t a = uint8_t(CLAMP(info.specular_color.a * 255.0, 0.0, 255.0));
	uint8_t b = uint8_t(CLAMP(info.specular_color.b * 255.0, 0.0, 255.0));
	uint8_t g = uint8_t(CLAMP(info.specular_color.g * 255.0, 0.0, 255.0));
	uint8_t r = uint8_t(CLAMP(info.specular_color.r * 255.0, 0.0, 255.0));
	p_info->specular_shininess =
		uint32_t(a) << 24 | uint32_t(b) << 16 | uint32_t(g) << 8 | uint32_t(r);

	p_info->texpixel_size = Vector2(1.0 / float(info.size.width), 1.0 / float(info.size.height));
}

void RendererCanvasRenderRD::initialize()
{
	// preallocate slots for uniform set 3
	RendererRD::state.batch_texture_uniforms.resize(4);

	{ // shader variants
		String global_defines;
		global_defines += "#define MAX_LIGHTS " + itos(MAX_LIGHTS_PER_RENDER) + "\n";
		global_defines += "\n#define SAMPLERS_BINDING_FIRST_INDEX " +
						  itos(SAMPLERS_BINDING_FIRST_INDEX) + "\n";

		RendererRD::state.light_uniforms =
			memnew_arr(RendererRD::LightUniform, MAX_LIGHTS_PER_RENDER);
		Vector<String> variants;
		const uint32_t ubershader_iterations = 1;
		for (uint32_t ubershader = 0; ubershader < ubershader_iterations; ubershader++) {
			const String base_define = ubershader ? "\n#define UBERSHADER\n" : "";
			variants.push_back(base_define + ""); // SHADER_VARIANT_QUAD
			variants.push_back(base_define + "#define USE_NINEPATCH\n"); // SHADER_VARIANT_NINEPATCH
			variants.push_back(base_define + "#define USE_PRIMITIVE\n"); // SHADER_VARIANT_PRIMITIVE
			variants.push_back(base_define +
							   "#define USE_PRIMITIVE\n#define USE_POINT_SIZE\n"); // PRIMITIVE_POINTS
			variants.push_back(
				base_define + "#define USE_ATTRIBUTES\n"); // SHADER_VARIANT_ATTRIBUTES
			variants.push_back(base_define +
							   "#define USE_ATTRIBUTES\n#define USE_POINT_SIZE\n"); // ATTRIBUTES_POINTS
		}

		RendererRD::shader.canvas_shader.initialize(variants, global_defines, {}, {});

		RendererRD::shader.default_version_data = memnew(RendererRD::CanvasShaderData);
		RendererRD::shader.default_version_data->version =
			RendererRD::shader.canvas_shader.version_create();
		RendererRD::shader.default_version_data->blend_mode =
			RendererRD::MaterialStorage::ShaderData::BLEND_MODE_MIX;
		RendererRD::shader.default_version_rd_shader =
			RendererRD::shader.default_version_data->get_shader(
				RendererRD::SHADER_VARIANT_QUAD, false);
	}

	{
		// shader compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["VERTEX"] = "vertex";
		actions.renames["LIGHT_VERTEX"] = "light_vertex";
		actions.renames["SHADOW_VERTEX"] = "shadow_vertex";
		actions.renames["UV"] = "uv";
		actions.renames["POINT_SIZE"] = "point_size";

		actions.renames["MODEL_MATRIX"] = "model_matrix";
		actions.renames["CANVAS_MATRIX"] = "canvas_data.canvas_transform";
		actions.renames["SCREEN_MATRIX"] = "canvas_data.screen_transform";
		actions.renames["TIME"] = "canvas_data.time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.renames["AT_LIGHT_PASS"] = "false";
		actions.renames["INSTANCE_CUSTOM"] = "instance_custom";

		actions.renames["COLOR"] = "color";
		actions.renames["NORMAL"] = "normal";
		actions.renames["NORMAL_MAP"] = "normal_map";
		actions.renames["NORMAL_MAP_DEPTH"] = "normal_map_depth";
		actions.renames["TEXTURE"] = "color_texture";
		actions.renames["TEXTURE_PIXEL_SIZE"] = "read_draw_data_color_texture_pixel_size";
		actions.renames["NORMAL_TEXTURE"] = "normal_texture";
		actions.renames["SPECULAR_SHININESS_TEXTURE"] = "specular_texture";
		actions.renames["SPECULAR_SHININESS"] = "specular_shininess";
		actions.renames["SCREEN_UV"] = "screen_uv";
		actions.renames["REGION_RECT"] = "region_rect";
		actions.renames["SCREEN_PIXEL_SIZE"] = "canvas_data.screen_pixel_size";
		actions.renames["FRAGCOORD"] = "gl_FragCoord";
		actions.renames["POINT_COORD"] = "gl_PointCoord";
		actions.renames["INSTANCE_ID"] = "gl_InstanceIndex";
		actions.renames["VERTEX_ID"] = "gl_VertexIndex";

		actions.renames["CUSTOM0"] = "custom0";
		actions.renames["CUSTOM1"] = "custom1";

		actions.renames["LIGHT_POSITION"] = "light_position";
		actions.renames["LIGHT_DIRECTION"] = "light_direction";
		actions.renames["LIGHT_IS_DIRECTIONAL"] = "is_directional";
		actions.renames["LIGHT_COLOR"] = "light_color";
		actions.renames["LIGHT_ENERGY"] = "light_energy";
		actions.renames["LIGHT"] = "light";
		actions.renames["SHADOW_MODULATE"] = "shadow_modulate";

		actions.renames["texture_sdf"] = "texture_sdf";
		actions.renames["texture_sdf_normal"] = "texture_sdf_normal";
		actions.renames["sdf_to_screen_uv"] = "sdf_to_screen_uv";
		actions.renames["screen_uv_to_sdf"] = "screen_uv_to_sdf";

		actions.usage_defines["COLOR"] = "#define COLOR_USED\n";
		actions.usage_defines["SCREEN_UV"] = "#define SCREEN_UV_USED\n";
		actions.usage_defines["SCREEN_PIXEL_SIZE"] = "@SCREEN_UV";
		actions.usage_defines["NORMAL"] = "#define NORMAL_USED\n";
		actions.usage_defines["NORMAL_MAP"] = "#define NORMAL_MAP_USED\n";
		actions.usage_defines["SPECULAR_SHININESS"] = "#define SPECULAR_SHININESS_USED\n";
		actions.usage_defines["POINT_SIZE"] = "#define USE_POINT_SIZE\n";
		actions.usage_defines["CUSTOM0"] = "#define CUSTOM0_USED\n";
		actions.usage_defines["CUSTOM1"] = "#define CUSTOM1_USED\n";

		actions.render_mode_defines["skip_vertex_transform"] = "#define SKIP_TRANSFORM_USED\n";
		actions.render_mode_defines["unshaded"] = "#define MODE_UNSHADED\n";
		actions.render_mode_defines["light_only"] = "#define MODE_LIGHT_ONLY\n";
		actions.render_mode_defines["world_vertex_coords"] = "#define USE_WORLD_VERTEX_COORDS\n";

		actions.custom_samplers["TEXTURE"] = "texture_sampler";
		actions.custom_samplers["NORMAL_TEXTURE"] = "texture_sampler";
		actions.custom_samplers["SPECULAR_SHININESS_TEXTURE"] = "texture_sampler";
		actions.base_texture_binding_index = 1;
		actions.texture_layout_set = MATERIAL_UNIFORM_SET;
		actions.base_uniform_string = "material.";
		actions.default_filter = ShaderLanguage::FILTER_LINEAR;
		actions.default_repeat = ShaderLanguage::REPEAT_DISABLE;
		actions.base_varying_index = 9;

		actions.global_buffer_array_variable = "global_shader_uniforms.data";
		actions.instance_uniform_index_variable = "read_draw_data_instance_offset";

		RendererRD::shader.compiler.initialize(actions);
	}

	{ // shadow rendering
		Vector<String> versions;
		versions.push_back("\n#define MODE_SHADOW\n");
		versions.push_back("\n#define MODE_SHADOW\n#define POSITIONAL_SHADOW\n");
		versions.push_back("\n#define MODE_SDF\n");
		shadow_render.shader.initialize(versions);

		{
			Vector<RD::AttachmentFormat> attachments;

			RD::AttachmentFormat af_color;
			af_color.format = RDC::DATA_FORMAT_R32_SFLOAT;
			af_color.usage_flags =
				RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			attachments.push_back(af_color);

			RD::AttachmentFormat af_depth;
			af_depth.format = RDC::DATA_FORMAT_D32_SFLOAT;
			af_depth.usage_flags = RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			attachments.push_back(af_depth);

			shadow_render.framebuffer_format = RD::framebuffer_format_create(attachments);
		}

		{
			Vector<RD::AttachmentFormat> attachments;

			RD::AttachmentFormat af_color;
			af_color.format = RDC::DATA_FORMAT_R8_UNORM;
			af_color.usage_flags = RDC::TEXTURE_USAGE_SAMPLING_BIT |
								   RDC::TEXTURE_USAGE_STORAGE_BIT |
								   RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			attachments.push_back(af_color);

			shadow_render.sdf_framebuffer_format = RD::framebuffer_format_create(attachments);
		}

		Vector<RDC::VertexAttribute> vf;
		RDC::VertexAttribute vd;
		vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
		vd.stride = sizeof(float) * 3;
		vd.location = 0;
		vd.offset = 0;
		vf.push_back(vd);
		shadow_render.vertex_format = RD::vertex_format_create(vf);

		vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		vd.stride = sizeof(float) * 2;
		vf.write[0] = vd;
		shadow_render.sdf_vertex_format = RD::vertex_format_create(vf);

		shadow_render.shader_version = shadow_render.shader.version_create();

		for (int i = 0; i < 2; i++) {
			RDC::PipelineRasterizationState rs;
			RDC::PipelineDepthStencilState ds;
			ds.enable_depth_write = true;
			ds.enable_depth_test = true;
			ds.depth_compare_operator = RDC::COMPARE_OP_LESS;
			shadow_render.render_pipelines[i] = RD::render_pipeline_create(
				shadow_render.shader.version_get_shader(
					shadow_render.shader_version, ShadowRenderMode(i)),
				shadow_render.framebuffer_format, shadow_render.vertex_format,
				RDC::RENDER_PRIMITIVE_TRIANGLES, rs, RDC::PipelineMultisampleState(), ds,
				RDC::PipelineColorBlendState::create_disabled(), 0);
		}

		for (int i = 0; i < 2; i++) {
			shadow_render.sdf_render_pipelines[i] = RD::render_pipeline_create(
				shadow_render.shader.version_get_shader(
					shadow_render.shader_version, SHADOW_RENDER_MODE_SDF),
				shadow_render.sdf_framebuffer_format, shadow_render.sdf_vertex_format,
				i == 0 ? RDC::RENDER_PRIMITIVE_TRIANGLES : RDC::RENDER_PRIMITIVE_LINES,
				RDC::PipelineRasterizationState(), RDC::PipelineMultisampleState(),
				RDC::PipelineDepthStencilState(), RDC::PipelineColorBlendState::create_disabled(),
				0);
		}

		RD::shader_destroy_modules(shadow_render.shader.version_get_shader(
			shadow_render.shader_version, SHADOW_RENDER_MODE_DIRECTIONAL_SHADOW));
		RD::shader_destroy_modules(shadow_render.shader.version_get_shader(
			shadow_render.shader_version, SHADOW_RENDER_MODE_POSITIONAL_SHADOW));
		RD::shader_destroy_modules(shadow_render.shader.version_get_shader(
			shadow_render.shader_version, SHADOW_RENDER_MODE_SDF));
	}

	{ // bindings
		RendererRD::state.canvas_state_buffer =
			RD::uniform_buffer_create(sizeof(RendererRD::State::Buffer));
		RendererRD::state.lights_storage_buffer =
			RD::storage_buffer_create(sizeof(RendererRD::LightUniform) * MAX_LIGHTS_PER_RENDER);

		RDC::SamplerState shadow_sampler_state;
		shadow_sampler_state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
		shadow_sampler_state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
		shadow_sampler_state.repeat_u = RDC::SAMPLER_REPEAT_MODE_REPEAT;
		RendererRD::state.shadow_sampler = RD::sampler_create(shadow_sampler_state);
	}

	{
		RendererRD::polygon_buffers.last_id = 1;
	}

	{ // default index buffer
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
		RendererRD::shader.quad_index_buffer =
			RD::index_buffer_create(6, RDC::INDEX_BUFFER_FORMAT_UINT16, pv);
		RendererRD::shader.quad_index_array =
			RD::index_array_create(RendererRD::shader.quad_index_buffer, 0, 6);
	}

	{
		Vector<RDC::VertexAttribute> vf;
		uint32_t offset = 0;
		RDC::VertexAttribute vd;
		vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		vd.stride = sizeof(RendererRD::InstanceData);
		vd.frequency = RDC::VERTEX_FREQUENCY_INSTANCE;
		vd.location = 8;
		vd.binding = 0;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_A

		vd.location = 9;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_B

		vd.location = 10;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_C

		vd.location = 11;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_D

		vd.location = 12;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_E

		uint32_t attrib_F_index = vf.size();
		vd.location = 13;
		vd.offset = offset;
		offset += sizeof(float) * 4;
		vf.push_back(vd); // attrib_F (RECT, NINEPATCH)

		vd.format = RDC::DATA_FORMAT_R32G32B32A32_UINT;
		vd.location = 14;
		vd.offset = offset;
		offset += sizeof(uint32_t) * 4;
		vf.push_back(vd); // attrib_G

		vd.location = 15;
		vd.offset = offset;
		offset += sizeof(uint32_t) * 4;
		vf.push_back(vd); // attrib_H

		// RECT, NINEPATCH
		RendererRD::shader.quad_vertex_format_id = RD::vertex_format_create(vf);

		// PRIMITIVE
		vf.write[attrib_F_index].format = RDC::DATA_FORMAT_R32G32B32A32_UINT;
		RendererRD::shader.primitive_vertex_format_id = RD::vertex_format_create(vf);
	}

	{ // primitive
		primitive_arrays.index_array[0] =
			RD::index_array_create(RendererRD::shader.quad_index_buffer, 0, 1);
		primitive_arrays.index_array[1] =
			RD::index_array_create(RendererRD::shader.quad_index_buffer, 0, 2);
		primitive_arrays.index_array[2] =
			RD::index_array_create(RendererRD::shader.quad_index_buffer, 0, 3);
		primitive_arrays.index_array[3] =
			RD::index_array_create(RendererRD::shader.quad_index_buffer, 0, 6);
	}

	{
		// default shadow texture to keep uniform set happy
		RDC::TextureFormat tf;
		tf.texture_type = RDC::TEXTURE_TYPE_2D;
		tf.width = 4;
		tf.height = 4;
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT;
		tf.format = RDC::DATA_FORMAT_R32_SFLOAT;

		RendererRD::state.shadow_texture = RD::texture_create(tf, RDD::TextureView());
	}

	{
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(RendererRD::MeshStorage::get_default_rd_storage_buffer());
			uniforms.push_back(u);
		}

		RendererRD::state.default_transforms_uniform_set = RD::uniform_set_create(
			uniforms, RendererRD::shader.default_version_rd_shader, TRANSFORMS_UNIFORM_SET);
	}

	default_canvas_texture = RendererRD::TextureStorage::canvas_texture_allocate();
	RendererRD::TextureStorage::canvas_texture_initialize(default_canvas_texture);

	RendererRD::TextureStorage::CanvasTextureInfo info =
		RendererRD::TextureStorage::canvas_texture_get_info(
			default_canvas_texture, default_filter, default_repeat, false, false);
	default_texture_info.diffuse = info.diffuse;
	default_texture_info.normal = info.normal;
	default_texture_info.specular = info.specular;
	default_texture_info.sampler = info.sampler;

	RendererRD::state.shadow_texture_size = 2048;

	RendererRD::MaterialStorage::shader_set_data_request_function(
		RendererRD::MaterialStorage::SHADER_TYPE_2D, _create_shader_funcs);
	RendererRD::MaterialStorage::material_set_data_request_function(
		RendererRD::MaterialStorage::SHADER_TYPE_2D, _create_material_funcs);

	RendererRD::state.time = 0;

	{
		default_canvas_group_shader = RendererRD::MaterialStorage::shader_allocate();
		RendererRD::MaterialStorage::shader_initialize(default_canvas_group_shader);

		RendererRD::MaterialStorage::shader_set_code(default_canvas_group_shader, R"(
// Default CanvasGroup shader.

shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_nearest;

void fragment() {
	vec4 c = textureLod(screen_texture, SCREEN_UV, 0.0);

	if (c.a > 0.0001) {
		c.rgb /= c.a;
	}

	COLOR *= c;
}
)");
		default_canvas_group_material = RendererRD::MaterialStorage::material_allocate();
		RendererRD::MaterialStorage::material_initialize(default_canvas_group_material);
		RendererRD::MaterialStorage::material_set_shader(
			default_canvas_group_material, default_canvas_group_shader);
	}

	{
		default_clip_children_shader = RendererRD::MaterialStorage::shader_allocate();
		RendererRD::MaterialStorage::shader_initialize(default_clip_children_shader);

		RendererRD::MaterialStorage::shader_set_code(default_clip_children_shader, R"(
// Default clip children shader.

shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_nearest;

void fragment() {
	vec4 c = textureLod(screen_texture, SCREEN_UV, 0.0);
	COLOR.rgb = c.rgb;
}
)");
		default_clip_children_material = RendererRD::MaterialStorage::material_allocate();
		RendererRD::MaterialStorage::material_initialize(default_clip_children_material);
		RendererRD::MaterialStorage::material_set_shader(
			default_clip_children_material, default_clip_children_shader);
	}

	{
		uint32_t cache_size = 1024;
		rid_set_to_uniform_set.set_capacity(cache_size);
	}

	{
		RendererRD::state.max_instances_per_buffer = 16384;
		RendererRD::state.max_instance_buffer_size =
			RendererRD::state.max_instances_per_buffer * sizeof(RendererRD::InstanceData);
		RendererRD::state.canvas_instance_batches.reserve(200);
		RendererRD::state.instance_buffers.set_vertex_size(
			0, RendererRD::state.max_instance_buffer_size);
	}
}

void RendererCanvasRenderRD::finalize()
{
	if (default_canvas_group_material.is_valid()) {
		RendererRD::MaterialStorage::material_free(default_canvas_group_material);
		default_canvas_group_material = RID();
	}
	if (default_canvas_group_shader.is_valid()) {
		RendererRD::MaterialStorage::shader_free(default_canvas_group_shader);
		default_canvas_group_shader = RID();
	}

	if (default_clip_children_material.is_valid()) {
		RendererRD::MaterialStorage::material_free(default_clip_children_material);
		default_clip_children_material = RID();
	}
	if (default_clip_children_shader.is_valid()) {
		RendererRD::MaterialStorage::shader_free(default_clip_children_shader);
		default_clip_children_shader = RID();
	}

	{
		if (RendererRD::state.canvas_state_buffer.is_valid()) {
			RD::free_rid(RendererRD::state.canvas_state_buffer);
		}

		if (RendererRD::state.light_uniforms) {
			memdelete_arr(RendererRD::state.light_uniforms);
			RendererRD::state.light_uniforms = nullptr;
		}
		if (RendererRD::state.lights_storage_buffer.is_valid()) {
			RD::free_rid(RendererRD::state.lights_storage_buffer);
		}
	}

	// shadow rendering
	{
		if (shadow_render.shader_version.is_valid()) {
			shadow_render.shader.version_free(shadow_render.shader_version);
			shadow_render.shader_version = RID();
		}
		// this will also automatically clear all pipelines
		if (RendererRD::state.shadow_sampler.is_valid()) {
			RD::free_rid(RendererRD::state.shadow_sampler);
		}
	}

	// buffers
	{
		if (RendererRD::shader.quad_index_array.is_valid()) {
			RD::free_rid(RendererRD::shader.quad_index_array);
		}
		if (RendererRD::shader.quad_index_buffer.is_valid()) {
			RD::free_rid(RendererRD::shader.quad_index_buffer);
		}
		// primitives are erase by dependency
	}

	if (RendererRD::state.shadow_fb.is_valid()) {
		RD::free_rid(RendererRD::state.shadow_depth_texture);
	}
	if (RendererRD::state.shadow_texture.is_valid()) {
		RD::free_rid(RendererRD::state.shadow_texture);
	}

	if (RendererRD::state.shadow_occluder_buffer.is_valid()) {
		RD::free_rid(RendererRD::state.shadow_occluder_buffer);
	}

	RendererRD::state.instance_buffers.uninit();

	if (default_canvas_texture.is_valid()) {
		// Disable the callback, as we're tearing everything down
		RendererRD::TextureStorage::canvas_texture_set_invalidation_callback(
			default_canvas_texture, nullptr, nullptr);
		RendererRD::TextureStorage::canvas_texture_free(default_canvas_texture);
		default_canvas_texture = RID();
	}
	// pipelines don't need freeing, they are all gone after shaders are gone

	if (RendererRD::shader.default_version_data) {
		memdelete(RendererRD::shader.default_version_data);
		RendererRD::shader.default_version_data = nullptr;
	}
}

RID RendererCanvasRenderRD::_get_pipeline_specialization_or_ubershader(
	RendererRD::CanvasShaderData* p_shader_data, RendererRD::PipelineKey& r_pipeline_key,
	RendererRD::PushConstant& r_push_constant, RID p_mesh_instance, void* p_surface,
	uint32_t p_surface_index, RID* r_vertex_array)
{
	r_pipeline_key.ubershader = 0;

	const uint32_t ubershader_iterations = 1;
	while (r_pipeline_key.ubershader < ubershader_iterations) {
		if (r_vertex_array != nullptr) {
			uint64_t input_mask = p_shader_data->get_vertex_input_mask(
				r_pipeline_key.variant, r_pipeline_key.ubershader);
			if (p_mesh_instance.is_valid()) {
				RendererRD::MeshStorage::mesh_instance_surface_get_vertex_arrays_and_format(
					p_mesh_instance, p_surface_index, input_mask, false, false, *r_vertex_array,
					r_pipeline_key.vertex_format_id);
			}
			else {
				RendererRD::MeshStorage::mesh_surface_get_vertex_arrays_and_format(p_surface,
					input_mask, false, false, *r_vertex_array, r_pipeline_key.vertex_format_id);
			}
		}

		if (r_pipeline_key.ubershader) {
			r_push_constant.shader_specialization = r_pipeline_key.shader_specialization;
			r_pipeline_key.shader_specialization = {};
		}
		else {
			r_push_constant.shader_specialization = {};
		}

		bool wait_for_compilation = r_pipeline_key.ubershader || ubershader_iterations == 1;
		RSE::PipelineSource source = RSE::PIPELINE_SOURCE_CANVAS;
		RID pipeline = p_shader_data->pipeline_hash_map.get_pipeline(
			r_pipeline_key, r_pipeline_key.hash(), wait_for_compilation, source);
		if (pipeline.is_valid()) {
			return pipeline;
		}

		r_pipeline_key.ubershader++;
	}

	return RID();
}

RID RendererRD::CanvasShaderData::get_shader(
	ShaderVariant p_shader_variant, bool p_ubershader) const
{
	if (version.is_valid()) {
		uint32_t variant_index = p_shader_variant + (p_ubershader ? SHADER_VARIANT_MAX : 0);
		MutexLock lock(shader.mutex);
		return shader.canvas_shader.version_get_shader(version, variant_index);
	}
	else {
		return RID();
	}
}

RendererRD::CanvasShaderData::~CanvasShaderData()
{
	pipeline_hash_map.clear_pipelines();

	if (version.is_valid()) {
		MutexLock lock(shader.mutex);
		shader.canvas_shader.version_free(version);
	}
}

void RendererCanvasRenderRD::light_update_shadow(RID p_light, int p_shadow_index,
	const Transform2D& p_light_xform, int p_light_mask, float p_near, float p_far,
	LightOccluderInstance* p_occluders, const Rect2& p_light_rect)
{
}

void RendererCanvasRenderRD::light_update_directional_shadow(RID p_light, int p_shadow_index,
	const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
	const Rect2& p_clip_rect, LightOccluderInstance* p_occluders)
{
}

void RendererCanvasRenderRD::render_sdf(
	RID p_render_target, LightOccluderInstance* p_occluders)
{
}

void RendererRD::CanvasShaderData::set_code(const String& p_code)
{
	code = p_code;
	ubo_size = 0;
	uniforms.clear();
	uses_screen_texture = false;
	uses_screen_texture_mipmaps = false;
	uses_sdf = false;
	uses_time = false;
	_clear_vertex_input_mask_cache();

	if (code.is_empty()) {
		return;
	}

	ShaderCompiler::GeneratedCode gen_code;

	blend_mode = BLEND_MODE_MIX;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["vertex"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["fragment"] = ShaderCompiler::STAGE_FRAGMENT;
	actions.entry_point_stages["light"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_mode, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_mode, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_mode, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_mode, BLEND_MODE_MUL);
	actions.render_mode_values["blend_premul_alpha"] =
		Pair<int*, int>(&blend_mode, BLEND_MODE_PREMULTIPLIED_ALPHA);
	actions.render_mode_values["blend_disabled"] =
		Pair<int*, int>(&blend_mode, BLEND_MODE_DISABLED);

	actions.usage_flag_pointers["texture_sdf"] = &uses_sdf;
	actions.usage_flag_pointers["texture_sdf_normal"] = &uses_sdf;
	actions.usage_flag_pointers["TIME"] = &uses_time;

	actions.uniforms = &uniforms;

	MutexLock lock(shader.mutex);

	Error err = shader.compiler.compile(RSE::SHADER_CANVAS_ITEM, code, &actions, path, gen_code);
	if (err != OK) {
		if (version.is_valid()) {
			shader.canvas_shader.version_free(version);
			version = RID();
		}
		ERR_FAIL_MSG("Shader compilation failed.");
	}

	uses_screen_texture_mipmaps = gen_code.uses_screen_texture_mipmaps;
	uses_screen_texture = gen_code.uses_screen_texture;

	pipeline_hash_map.clear_pipelines();

	if (version.is_null()) {
		version = shader.canvas_shader.version_create(false);
	}

	shader.canvas_shader.version_set_code(version, gen_code.code, gen_code.uniforms,
		gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines);

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;
}


