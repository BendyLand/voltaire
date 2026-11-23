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
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
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
#include "servers/rendering/rendering_device.h"
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
#include "core/templates/span.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
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



// ============================================================================
// Merged from servers/rendering/renderer_rd/environment/rd_environment.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "core/config/project_settings.h"
#include "core/math/math_defs.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "core/math/geometry_3d.h"

// --- Merged from servers/rendering/renderer_rd/environment/sky.cpp ---

using namespace RendererRD;

#define RB_SCOPE_SKY SNAME("sky_buffers")
#define RB_HALF_TEXTURE SNAME("half_texture")
#define RB_QUARTER_TEXTURE SNAME("quarter_texture")

bool SkyRD::SkyShaderData::is_animated() const { return false; }

bool SkyRD::SkyShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode SkyRD::SkyShaderData::get_native_source_code() const
{
	return RendererSceneRenderRD::data->sky.sky_shader.shader.version_get_native_source_code(
		version);
}

Pair<ShaderRD*, RID> SkyRD::SkyShaderData::get_native_shader_and_version() const
{
	return {&RendererSceneRenderRD::data->sky.sky_shader.shader, version};
}

SkyRD::SkyShaderData::~SkyShaderData()
{
	ERR_FAIL_NULL(RendererSceneRenderRD::data);
	// pipeline variants will clear themselves if shader is gone
	if (version.is_valid()) {
		RendererSceneRenderRD::data->sky.sky_shader.shader.version_free(version);
	}
}

SkyRD::SkyMaterialData::~SkyMaterialData() { free_parameters_uniform_set(uniform_set); }

static _FORCE_INLINE_ void store_transform_3x3(const Basis& p_basis, float* p_array)
{
	p_array[0] = p_basis.rows[0][0];
	p_array[1] = p_basis.rows[1][0];
	p_array[2] = p_basis.rows[2][0];
	p_array[3] = 0;
	p_array[4] = p_basis.rows[0][1];
	p_array[5] = p_basis.rows[1][1];
	p_array[6] = p_basis.rows[2][1];
	p_array[7] = 0;
	p_array[8] = p_basis.rows[0][2];
	p_array[9] = p_basis.rows[1][2];
	p_array[10] = p_basis.rows[2][2];
	p_array[11] = 0;
}

void SkyRD::_render_sky(RD::DrawListID p_list, float p_time, RID p_fb, PipelineCacheRD* p_pipeline,
	RID p_uniform_set, RID p_texture_set, const Projection& p_projection,
	const Basis& p_orientation, const Vector3& p_position, float p_luminance_multiplier,
	float p_brightness_multiplier, float p_border_size)
{
	SkyPushConstant sky_push_constant;

	memset(&sky_push_constant, 0, sizeof(SkyPushConstant));

	// We only need key components of our projection matrix
	sky_push_constant.projection[0] = p_projection.columns[2][0];
	sky_push_constant.projection[1] = p_projection.columns[0][0];
	sky_push_constant.projection[2] = p_projection.columns[2][1];
	sky_push_constant.projection[3] = p_projection.columns[1][1];

	sky_push_constant.position[0] = p_position.x;
	sky_push_constant.position[1] = p_position.y;
	sky_push_constant.position[2] = p_position.z;
	sky_push_constant.time = p_time;
	sky_push_constant.border_size[0] = p_border_size;
	sky_push_constant.border_size[1] = 1.0f - p_border_size * 2.0;
	sky_push_constant.luminance_multiplier = p_luminance_multiplier;
	sky_push_constant.brightness_multiplier = p_brightness_multiplier;
	store_transform_3x3(p_orientation, sky_push_constant.orientation);

	RenderingDevice::FramebufferFormatID fb_format = RD::framebuffer_get_format(p_fb);

	RD::DrawListID draw_list = p_list;

	RD::draw_list_bind_render_pipeline(
		draw_list, p_pipeline->get_render_pipeline(
					   RDC::INVALID_ID, fb_format, false, RD::draw_list_get_current_pass()));

	// Update uniform sets.
	{
		RD::draw_list_bind_uniform_set(draw_list, sky_scene_state.uniform_set, SKY_SET_UNIFORMS);
		if (p_uniform_set.is_valid() &&
			RD::uniform_set_is_valid(p_uniform_set)) { // Material may not have a uniform set.
			RD::draw_list_bind_uniform_set(draw_list, p_uniform_set, SKY_SET_MATERIAL);
		}
		RD::draw_list_bind_uniform_set(draw_list, p_texture_set, SKY_SET_TEXTURES);
		// Fog uniform set can be invalidated before drawing, so validate at draw time
		if (sky_scene_state.fog_uniform_set.is_valid() &&
			RD::uniform_set_is_valid(sky_scene_state.fog_uniform_set)) {
			RD::draw_list_bind_uniform_set(draw_list, sky_scene_state.fog_uniform_set, SKY_SET_FOG);
		}
		else {
			RD::draw_list_bind_uniform_set(
				draw_list, sky_scene_state.default_fog_uniform_set, SKY_SET_FOG);
		}
	}

	RD::draw_list_set_push_constant(draw_list, &sky_push_constant, sizeof(SkyPushConstant));

	RD::draw_list_draw(draw_list, false, 1u, 3u);
}

void SkyRD::ReflectionData::clear_reflection_data()
{
	layers.clear();
	radiance_base_octmap = RID();
	if (downsampled_radiance_octmap.is_valid()) {
		RD::free_rid(downsampled_radiance_octmap);
	}
	downsampled_radiance_octmap = RID();
	downsampled_layer.mipmaps.clear();
	coefficient_buffer = RID();
}

void SkyRD::Sky::free_radiance()
{
	if (radiance.is_valid()) {
		RD::free_rid(radiance);
		radiance = RID();
	}
	if (radiance_first_layer_slice.is_valid()) {
		if (RD::texture_is_valid(radiance_first_layer_slice)) {
			RD::free_rid(radiance_first_layer_slice);
		}
		radiance_first_layer_slice = RID();
	}
}

void SkyRD::Sky::free()
{
	free_radiance();
	reflection.clear_reflection_data();

	if (uniform_buffer.is_valid()) {
		RD::free_rid(uniform_buffer);
		uniform_buffer = RID();
	}

	if (material.is_valid()) {
		material = RID();
	}
}

RID SkyRD::Sky::get_textures(SkyTextureSetVersion p_version, RID p_default_shader_rd,
	bool p_is_multiview, Ref<RenderSceneBuffersRD> p_render_buffers)
{
	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 0;
		if (radiance.is_valid() && p_version <= SKY_TEXTURE_SET_QUARTER_RES) {
			u.append_id(
				radiance_first_layer_slice.is_valid() ? radiance_first_layer_slice : radiance);
		}
		else {
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 1; // half res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 2 &&
				reflection.layers[0].mipmaps[1].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_HALF_RES) {
				u.append_id(reflection.layers[0].mipmaps[1].view);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			}
		}
		else {
			RID half_texture = p_render_buffers->has_texture(RB_SCOPE_SKY, RB_HALF_TEXTURE)
								   ? p_render_buffers->get_texture(RB_SCOPE_SKY, RB_HALF_TEXTURE)
								   : RID();
			if (half_texture.is_valid() && p_version != SKY_TEXTURE_SET_HALF_RES) {
				u.append_id(half_texture);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 2; // quarter res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 3 &&
				reflection.layers[0].mipmaps[2].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_QUARTER_RES) {
				u.append_id(reflection.layers[0].mipmaps[2].view);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			}
		}
		else {
			RID quarter_texture =
				p_render_buffers->has_texture(RB_SCOPE_SKY, RB_QUARTER_TEXTURE)
					? p_render_buffers->get_texture(RB_SCOPE_SKY, RB_QUARTER_TEXTURE)
					: RID();
			if (quarter_texture.is_valid() && p_version != SKY_TEXTURE_SET_QUARTER_RES) {
				u.append_id(quarter_texture);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}

	return UniformSetCacheRD::get_cache_vec(p_default_shader_rd, SKY_SET_TEXTURES, uniforms);
}

bool SkyRD::Sky::set_radiance_size(int p_radiance_size)
{
	ERR_FAIL_COND_V(p_radiance_size < 32 || p_radiance_size > 2048, false);
	if (radiance_size == p_radiance_size) {
		return false;
	}
	radiance_size = p_radiance_size;

	if (mode == RSE::SKY_MODE_REALTIME && radiance_size != REAL_TIME_SIZE) {
		WARN_PRINT(vformat("Realtime Skies can only use a radiance size of %d. Radiance size will "
						   "be set to %d internally.",
			REAL_TIME_SIZE, REAL_TIME_SIZE));
		radiance_size = REAL_TIME_SIZE;
	}

	free_radiance();
	reflection.clear_reflection_data();

	return true;
}

int SkyRD::Sky::get_radiance_size() const { return radiance_size; }

bool SkyRD::Sky::set_mode(RSE::SkyMode p_mode)
{
	if (mode == p_mode) {
		return false;
	}

	mode = p_mode;

	if (mode == RSE::SKY_MODE_REALTIME && radiance_size != REAL_TIME_SIZE) {
		WARN_PRINT(vformat("Realtime Skies can only use a radiance size of %d. Radiance size will "
						   "be set to %d internally.",
			REAL_TIME_SIZE, REAL_TIME_SIZE));
		set_radiance_size(REAL_TIME_SIZE);
	}

	free_radiance();
	reflection.clear_reflection_data();

	return true;
}

bool SkyRD::Sky::set_material(RID p_material)
{
	if (material == p_material) {
		return false;
	}

	material = p_material;
	return true;
}

Ref<Image> SkyRD::Sky::bake_panorama(float p_energy, int p_roughness_layers, const Size2i& p_size)
{
	if (radiance.is_valid()) {
		ERR_FAIL_COND_V(!RendererRD::CopyEffects::is_initialized(), Ref<Image>());

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT; // Could be RGBA16
		tf.width = p_size.width;
		tf.height = p_size.height;
		tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

		RID rad_tex = RD::texture_create(tf, RDD::TextureView());
		RendererRD::CopyEffects::copy_octmap_to_panorama(radiance, rad_tex, p_size,
			p_roughness_layers, reflection.layers.size() > 1,
			Size2(uv_border_size, 1.0f - uv_border_size * 2.0));
		Vector<uint8_t> data = RD::texture_get_data(rad_tex, 0);
		RD::free_rid(rad_tex);

		Ref<Image> img =
			Image::create_from_data(p_size.width, p_size.height, false, Image::FORMAT_RGBAF, data);
		for (int i = 0; i < p_size.width; i++) {
			for (int j = 0; j < p_size.height; j++) {
				Color c = img->get_pixel(i, j);
				c.r *= p_energy;
				c.g *= p_energy;
				c.b *= p_energy;
				img->set_pixel(i, j, c);
			}
		}
		return img;
	}

	return Ref<Image>();
}

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_func()
{
	SkyShaderData* shader_data = memnew(SkyShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_funcs()
{
	return RendererSceneRenderRD::data->sky._create_sky_shader_func();
}

RendererRD::MaterialStorage::MaterialData* SkyRD::_create_sky_material_func(SkyShaderData* p_shader)
{
	SkyMaterialData* material_data = memnew(SkyMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

RendererRD::MaterialStorage::MaterialData* SkyRD::_create_sky_material_funcs(
	RendererRD::MaterialStorage::ShaderData* p_shader)
{
	return RendererSceneRenderRD::data->sky._create_sky_material_func(
		static_cast<SkyShaderData*>(p_shader));
}

RID SkyRD::SkySceneState::get_fog_only_texture_uniform_set(
	RID p_default_shader_rd, bool p_is_multiview)
{
	RID& uniform_set_rid =
		p_is_multiview ? fog_only_texture_multiview_uniform_set : fog_only_texture_uniform_set;

	if (uniform_set_rid.is_null()) {
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 0;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 1;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 2;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}

		uniform_set_rid = RD::uniform_set_create(uniforms, p_default_shader_rd, SKY_SET_TEXTURES);
	}

	return uniform_set_rid;
}

void SkyRD::set_texture_format(RDC::DataFormat p_texture_format)
{
	texture_format = p_texture_format;
}

SkyRD::~SkyRD()
{
	// cleanup anything created in init...
	if (sky_shader.default_material.is_valid()) {
		SkyMaterialData* md = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_shader.default_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
		if (md && md->shader_data && md->shader_data->version.is_valid()) {
			sky_shader.shader.version_free(md->shader_data->version);
			md->shader_data->version = RID();
		}
		MaterialStorage::material_free(sky_shader.default_material);
		sky_shader.default_material = RID();
	}
	if (sky_shader.default_shader.is_valid()) {
		MaterialStorage::shader_free(sky_shader.default_shader);
		sky_shader.default_shader = RID();
	}
	if (sky_scene_state.directional_light_buffer.is_valid()) {
		RD::free_rid(sky_scene_state.directional_light_buffer);
	}
	if (sky_scene_state.uniform_buffer.is_valid()) {
		RD::free_rid(sky_scene_state.uniform_buffer);
	}
	if (sky_scene_state.directional_lights) {
		memdelete_arr(sky_scene_state.directional_lights);
	}
	if (sky_scene_state.last_frame_directional_lights) {
		memdelete_arr(sky_scene_state.last_frame_directional_lights);
	}
	if (sky_scene_state.fog_material.is_valid()) {
		MaterialStorage::material_free(sky_scene_state.fog_material);
	}
	if (sky_scene_state.fog_shader.is_valid()) {
		MaterialStorage::shader_free(sky_scene_state.fog_shader);
	}

	if (RD::uniform_set_is_valid(sky_scene_state.uniform_set)) {
		RD::free_rid(sky_scene_state.uniform_set);
	}

	if (RD::uniform_set_is_valid(sky_scene_state.default_fog_uniform_set)) {
		RD::free_rid(sky_scene_state.default_fog_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sky_scene_state.fog_only_texture_uniform_set)) {
		RD::free_rid(sky_scene_state.fog_only_texture_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_multiview_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sky_scene_state.fog_only_texture_multiview_uniform_set)) {
		RD::free_rid(sky_scene_state.fog_only_texture_multiview_uniform_set);
	}
}

void SkyRD::setup_sky(const RenderDataRD* p_render_data, const Size2i p_screen_size)
{
	ERR_FAIL_COND(p_render_data->environment.is_null());

	ERR_FAIL_COND(p_render_data->render_buffers.is_null());

	// make sure we support our view count
	ERR_FAIL_COND(p_render_data->scene_data->view_count == 0);
	ERR_FAIL_COND(p_render_data->scene_data->view_count > RendererSceneRender::MAX_RENDER_VIEWS);

	SkyMaterialData* material_data = _get_sky_material_data(p_render_data->environment);
	ERR_FAIL_NULL(material_data);

	SkyShaderData* shader_data = material_data->shader_data;
	ERR_FAIL_NULL(shader_data);

	material_data->set_as_used();

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_render_data->environment));
	if (sky) {
		// Save our screen size; our buffers will already have been cleared.
		sky->screen_size.x = p_screen_size.x < 4 ? 4 : p_screen_size.x;
		sky->screen_size.y = p_screen_size.y < 4 ? 4 : p_screen_size.y;

		RSE::SkyMode sky_mode = sky->mode;

		if (sky_mode == RSE::SKY_MODE_AUTOMATIC) {
			bool sun_scatter_enabled =
				RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) &&
				RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) >
					0.001;

			if ((shader_data->uses_time || shader_data->uses_position) &&
				sky->radiance_size == Sky::REAL_TIME_SIZE) {
				sky_mode = RSE::SKY_MODE_REALTIME;
			}
			else if (shader_data->uses_light || sun_scatter_enabled ||
					   shader_data->ubo_size > 0) {
				sky_mode = RSE::SKY_MODE_INCREMENTAL;
			}
			else {
				sky_mode = RSE::SKY_MODE_QUALITY;
			}

			if (sky_mode != sky->internal_mode) {
				sky->internal_mode = sky_mode;

				sky->free_radiance();
				sky->reflection.clear_reflection_data();
			}
		}
		else {
			sky->internal_mode = sky_mode;
		}

		// Trigger updating radiance buffers.
		if (sky->radiance.is_null()) {
			invalidate_sky(sky);
			update_dirty_skys();
		}

		if (shader_data->uses_time && p_render_data->scene_data->time - sky->prev_time > 0.00001) {
			sky->prev_time = p_render_data->scene_data->time;
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_aerial_perspective(
				p_render_data->environment) != sky->prev_fog_aerial_perspective) {
			sky->prev_fog_aerial_perspective =
				RendererSceneRender::environment_get_fog_aerial_perspective(
					p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_light_color(p_render_data->environment) !=
			sky->prev_fog_light_color) {
			sky->prev_fog_light_color =
				RendererSceneRender::environment_get_fog_light_color(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) !=
			sky->prev_fog_sun_scatter) {
			sky->prev_fog_sun_scatter =
				RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) !=
			sky->prev_fog_enabled) {
			sky->prev_fog_enabled =
				RendererSceneRender::environment_get_fog_enabled(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_density(p_render_data->environment) !=
			sky->prev_fog_density) {
			sky->prev_fog_density =
				RendererSceneRender::environment_get_fog_density(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment) !=
			sky->prev_fog_sky_affect) {
			sky->prev_fog_sky_affect =
				RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment) !=
			sky->prev_fog_light_energy) {
			sky->prev_fog_light_energy =
				RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (material_data != sky->prev_material_data) {
			sky->prev_material_data = material_data;
			sky->reflection.dirty = true;
		}

		if (material_data->uniform_set_updated) {
			material_data->uniform_set_updated = false;
			sky->reflection.dirty = true;
		}

		if (!p_render_data->scene_data->cam_transform.origin.is_equal_approx(sky->prev_position) &&
			shader_data->uses_position) {
			sky->prev_position = p_render_data->scene_data->cam_transform.origin;
			sky->reflection.dirty = true;
		}
	}

	bool sun_scatter_enabled =
		RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) &&
		RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) > 0.001;
	sky_scene_state.ubo.directional_light_count = 0;
	if (shader_data->uses_light || sun_scatter_enabled) {
		const PagedArray<RID>& lights = *p_render_data->lights;
		// Run through the list of lights in the scene and pick out the Directional Lights.
		// This can't be done in RenderSceneRenderRD::_setup lights because that needs to be called
		// after the depth prepass, but this runs before the depth prepass.
		for (int i = 0; i < (int)lights.size(); i++) {
			if (!RendererRD::LightStorage::owns_light_instance(lights[i])) {
				continue;
			}
			RID base = RendererRD::LightStorage::light_instance_get_base_light(lights[i]);

			ERR_CONTINUE(base.is_null());

			RSE::LightType type = RendererRD::LightStorage::light_get_type(base);
			if (type == RSE::LIGHT_DIRECTIONAL &&
				RendererRD::LightStorage::light_directional_get_sky_mode(base) !=
					RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_ONLY) {
				SkyDirectionalLightData& sky_light_data =
					sky_scene_state.directional_lights[sky_scene_state.ubo.directional_light_count];
				Transform3D light_transform =
					RendererRD::LightStorage::light_instance_get_base_transform(lights[i]);
				Vector3 world_direction =
					light_transform.basis.xform(Vector3(0, 0, 1)).normalized();

				sky_light_data.direction[0] = world_direction.x;
				sky_light_data.direction[1] = world_direction.y;
				sky_light_data.direction[2] = world_direction.z;

				float sign = RendererRD::LightStorage::light_is_negative(base) ? -1 : 1;
				sky_light_data.energy =
					sign * RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_ENERGY);

				if (RendererSceneRenderRD::is_using_physical_light_units()) {
					sky_light_data.energy *=
						RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_INTENSITY);
				}

				if (p_render_data->camera_attributes.is_valid()) {
					sky_light_data.energy *=
						RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
				}

				Color linear_col = RendererRD::LightStorage::light_get_color(base).srgb_to_linear();
				sky_light_data.color[0] = linear_col.r;
				sky_light_data.color[1] = linear_col.g;
				sky_light_data.color[2] = linear_col.b;

				sky_light_data.enabled = true;

				float angular_diameter =
					RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_SIZE);
				sky_light_data.size = Math::deg_to_rad(angular_diameter);
				sky_scene_state.ubo.directional_light_count++;
				if (sky_scene_state.ubo.directional_light_count >=
					sky_scene_state.max_directional_lights) {
					break;
				}
			}
		}
		// Check whether the directional_light_buffer changes.
		bool light_data_dirty = false;

		// Light buffer is dirty if we have fewer or more lights.
		// If we have fewer lights, make sure that old lights are disabled.
		if (sky_scene_state.ubo.directional_light_count !=
			sky_scene_state.last_frame_directional_light_count) {
			light_data_dirty = true;
			for (uint32_t i = sky_scene_state.ubo.directional_light_count;
				 i < sky_scene_state.max_directional_lights; i++) {
				sky_scene_state.directional_lights[i] = {};
				sky_scene_state.directional_lights[i].enabled = false;
				sky_scene_state.last_frame_directional_lights[i] = {};
				sky_scene_state.last_frame_directional_lights[i].enabled = false;
			}
		}

		if (!light_data_dirty) {
			for (uint32_t i = 0; i < sky_scene_state.ubo.directional_light_count; i++) {
				if (sky_scene_state.directional_lights[i].direction[0] !=
						sky_scene_state.last_frame_directional_lights[i].direction[0] ||
					sky_scene_state.directional_lights[i].direction[1] !=
						sky_scene_state.last_frame_directional_lights[i].direction[1] ||
					sky_scene_state.directional_lights[i].direction[2] !=
						sky_scene_state.last_frame_directional_lights[i].direction[2] ||
					sky_scene_state.directional_lights[i].energy !=
						sky_scene_state.last_frame_directional_lights[i].energy ||
					sky_scene_state.directional_lights[i].color[0] !=
						sky_scene_state.last_frame_directional_lights[i].color[0] ||
					sky_scene_state.directional_lights[i].color[1] !=
						sky_scene_state.last_frame_directional_lights[i].color[1] ||
					sky_scene_state.directional_lights[i].color[2] !=
						sky_scene_state.last_frame_directional_lights[i].color[2] ||
					sky_scene_state.directional_lights[i].enabled !=
						sky_scene_state.last_frame_directional_lights[i].enabled ||
					sky_scene_state.directional_lights[i].size !=
						sky_scene_state.last_frame_directional_lights[i].size) {
					light_data_dirty = true;
					break;
				}
			}
		}

		if (light_data_dirty) {
			SkyDirectionalLightData* temp = sky_scene_state.last_frame_directional_lights;
			sky_scene_state.last_frame_directional_lights = sky_scene_state.directional_lights;
			sky_scene_state.directional_lights = temp;
			sky_scene_state.last_frame_directional_light_count =
				sky_scene_state.ubo.directional_light_count;
			if (sky) {
				sky->reflection.dirty = true;
			}
		}
	}

	// Setup fog variables.
	sky_scene_state.ubo.volumetric_fog_enabled = false;
	if (p_render_data->render_buffers->has_custom_data(RB_SCOPE_FOG)) {
		Ref<RendererRD::Fog::VolumetricFog> fog =
			p_render_data->render_buffers->get_custom_data(RB_SCOPE_FOG);
		sky_scene_state.ubo.volumetric_fog_enabled = true;

		float fog_end = fog->length;
		if (fog_end > 0.0) {
			sky_scene_state.ubo.volumetric_fog_inv_length = 1.0 / fog_end;
		}
		else {
			sky_scene_state.ubo.volumetric_fog_inv_length = 1.0;
		}

		float fog_detail_spread = fog->spread; // Reverse lookup.
		if (fog_detail_spread > 0.0) {
			sky_scene_state.ubo.volumetric_fog_detail_spread = 1.0 / fog_detail_spread;
		}
		else {
			sky_scene_state.ubo.volumetric_fog_detail_spread = 1.0;
		}

		sky_scene_state.fog_uniform_set = fog->sky_uniform_set;
	}

	sky_scene_state.view_count = p_render_data->scene_data->view_count;
	sky_scene_state.cam_transform = p_render_data->scene_data->cam_transform;

	Projection correction;
	correction.set_depth_correction(p_render_data->scene_data->flip_y, true);
	correction.add_jitter_offset(p_render_data->scene_data->taa_jitter);

	Projection projection = p_render_data->scene_data->cam_projection;

	float custom_fov =
		RendererSceneRender::environment_get_sky_custom_fov(p_render_data->environment);

	if (custom_fov && sky_scene_state.view_count == 1) {
		// With custom fov we don't support stereo...
		float near_plane = projection.get_z_near();
		float far_plane = projection.get_z_far();
		float aspect = projection.get_aspect();

		projection.set_perspective(custom_fov, aspect, near_plane, far_plane);
	}

	sky_scene_state.cam_projection = correction * projection;

	// Our info in our UBO is only used if we're rendering stereo.
	for (uint32_t i = 0; i < p_render_data->scene_data->view_count; i++) {
		Projection view_inv_projection =
			(correction * p_render_data->scene_data->view_projection[i]).inverse();
		if (p_render_data->scene_data->view_count > 1) {
			// Reprojection is used when we need to have things in combined space.
			RendererRD::MaterialStorage::store_camera(
				sky_scene_state.cam_projection * view_inv_projection,
				sky_scene_state.ubo.combined_reprojection[i]);
		}
		else {
			// This is unused so just reset to identity.
			Projection ident;
			RendererRD::MaterialStorage::store_camera(
				ident, sky_scene_state.ubo.combined_reprojection[i]);
		}

		RendererRD::MaterialStorage::store_camera(
			view_inv_projection, sky_scene_state.ubo.view_inv_projections[i]);
		sky_scene_state.ubo.view_eye_offsets[i][0] =
			p_render_data->scene_data->view_eye_offset[i].x;
		sky_scene_state.ubo.view_eye_offsets[i][1] =
			p_render_data->scene_data->view_eye_offset[i].y;
		sky_scene_state.ubo.view_eye_offsets[i][2] =
			p_render_data->scene_data->view_eye_offset[i].z;
		sky_scene_state.ubo.view_eye_offsets[i][3] = 0.0;
	}

	sky_scene_state.ubo.z_far = p_render_data->scene_data->view_projection[0]
									.get_z_far(); // Should be the same for all projection.
	sky_scene_state.ubo.fog_enabled =
		RendererSceneRender::environment_get_fog_enabled(p_render_data->environment);
	sky_scene_state.ubo.fog_density =
		RendererSceneRender::environment_get_fog_density(p_render_data->environment);
	sky_scene_state.ubo.fog_aerial_perspective =
		RendererSceneRender::environment_get_fog_aerial_perspective(p_render_data->environment);
	Color fog_color =
		RendererSceneRender::environment_get_fog_light_color(p_render_data->environment)
			.srgb_to_linear();
	float fog_energy =
		RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment);
	sky_scene_state.ubo.fog_light_color[0] = fog_color.r * fog_energy;
	sky_scene_state.ubo.fog_light_color[1] = fog_color.g * fog_energy;
	sky_scene_state.ubo.fog_light_color[2] = fog_color.b * fog_energy;
	sky_scene_state.ubo.fog_sun_scatter =
		RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment);

	sky_scene_state.ubo.fog_sky_affect =
		RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment);
	sky_scene_state.ubo.volumetric_fog_sky_affect =
		RendererSceneRender::environment_get_volumetric_fog_sky_affect(p_render_data->environment);
	sky_scene_state.ubo.fog_use_legacy_blending =
		RendererSceneRenderRD::fog_use_legacy_blending_get();
}

void SkyRD::draw_sky(RD::DrawListID p_draw_list, Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_env, RID p_fb, double p_time, float p_luminance_multiplier, float p_brightness_multiplier)
{
	ERR_FAIL_COND(p_render_buffers.is_null());
	ERR_FAIL_COND(p_env.is_null());

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_env));

	SkyMaterialData* material_data = _get_sky_material_data(p_env);
	ERR_FAIL_NULL(material_data);

	SkyShaderData* shader_data = material_data->shader_data;
	ERR_FAIL_NULL(shader_data);

	material_data->set_as_used();

	Basis sky_transform = RendererSceneRender::environment_get_sky_orientation(p_env);
	sky_transform.invert();

	// Camera
	Projection projection = sky_scene_state.cam_projection;

	sky_transform = sky_transform * sky_scene_state.cam_transform.basis;

	bool is_multiview = sky_scene_state.view_count > 1;

	PipelineCacheRD* pipeline =
		&shader_data
			 ->pipelines[is_multiview ? SKY_VERSION_BACKGROUND_MULTIVIEW : SKY_VERSION_BACKGROUND];

	RID default_shader_rd = sky_shader.get_default_shader_rd(is_multiview);

	RID texture_uniform_set;
	float border_size = 0.0;
	if (sky) {
		texture_uniform_set = sky->get_textures(
			SKY_TEXTURE_SET_BACKGROUND, default_shader_rd, is_multiview, p_render_buffers);
		border_size = sky->uv_border_size;
	}
	else {
		texture_uniform_set =
			sky_scene_state.get_fog_only_texture_uniform_set(default_shader_rd, is_multiview);
	}

	_render_sky(p_draw_list, p_time, p_fb, pipeline, material_data->uniform_set,
		texture_uniform_set, projection, sky_transform, sky_scene_state.cam_transform.origin,
		p_luminance_multiplier, p_brightness_multiplier, border_size);
}

void SkyRD::invalidate_sky(Sky* p_sky)
{
	if (!p_sky->dirty) {
		p_sky->dirty = true;
		p_sky->dirty_list = dirty_sky_list;
		dirty_sky_list = p_sky;
	}
}

void SkyRD::update_dirty_skys()
{
	bool use_raster_effect = (RendererRD::CopyEffects::get_raster_effects() &
								 RendererRD::CopyEffects::RASTER_EFFECT_OCTMAP) != 0;
	Sky* sky = dirty_sky_list;

	while (sky) {
		// update sky configuration if texture is missing

		// TODO See if we can move this into `update_radiance_buffers` and remove our dirty_sky
		// logic. As this is basically a duplicate of the logic in reflection probes we could move
		// this logic into RenderSceneBuffersRD and use that from both places.
		if (sky->radiance.is_null()) {
			int mipmaps = Image::get_image_required_mipmaps(
							  sky->radiance_size, sky->radiance_size, Image::FORMAT_RGBAH) +
						  1;

			int layers = roughness_layers;
			bool use_realtime =
				sky->mode == RSE::SKY_MODE_REALTIME || sky->internal_mode == RSE::SKY_MODE_REALTIME;
			if (use_realtime) {
				layers = Sky::REAL_TIME_ROUGHNESS_LAYERS;
			}

			if (sky_use_octmap_array) {
				mipmaps -= 2; //  reduce the number of mipmaps to keep the border size reasonable.
				// Double size to approximate texel density of cubemaps + add border for proper
				// filtering/mipmapping.
				uint32_t padding_pixels = (1 << (mipmaps - 1));
				uint32_t w = sky->radiance_size * 2 + padding_pixels * 2;
				uint32_t h = w;
				sky->uv_border_size = float(padding_pixels) / float(w);

				// Array (higher quality, more memory).
				RDC::TextureFormat tf;
				tf.array_layers = layers;
				tf.format = texture_format;
				tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
				tf.mipmaps = mipmaps;
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RDC::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::texture_create(tf, RDD::TextureView());

				// Create view into the first layer slice for user shaders.
				sky->radiance_first_layer_slice = RD::texture_create_shared_from_slice(
					RD::TextureView(), sky->radiance, 0, 0, mipmaps, RDC::TEXTURE_SLICE_2D, 1);

				sky->reflection.update_reflection_data(w, mipmaps, true, sky->radiance, 0,
					use_realtime, roughness_layers, texture_format, sky->uv_border_size);
			}
			else {
				// Double size to approximate texel density of cubemaps + add border for proper
				// filtering/mipmapping.
				uint32_t padding_pixels = (1 << (MIN(mipmaps, layers) - 1));
				uint32_t w = sky->radiance_size * 2 + padding_pixels * 2;
				uint32_t h = w;
				sky->uv_border_size = float(padding_pixels) / float(w);

				// Single texture (lower quality, less memory).
				RDC::TextureFormat tf;
				tf.format = texture_format;
				tf.mipmaps = MIN(mipmaps, layers);
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RDC::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::texture_create(tf, RDD::TextureView());

				DEV_ASSERT(sky->radiance_first_layer_slice.is_null());

				sky->reflection.update_reflection_data(w, MIN(mipmaps, layers), false,
					sky->radiance, 0, use_realtime, roughness_layers, texture_format,
					sky->uv_border_size);
			}
		}

		sky->reflection.dirty = true;
		sky->processing_layer = 0;

		Sky* next = sky->dirty_list;
		sky->dirty_list = nullptr;
		sky->dirty = false;
		sky = next;
	}

	dirty_sky_list = nullptr;
}

SkyRD::SkyMaterialData* SkyRD::_get_sky_material_data(RID p_env)
{
	ERR_FAIL_COND_V(p_env.is_null(), nullptr);

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_env));
	RSE::EnvironmentBG background = RendererSceneRender::environment_get_background(p_env);

	SkyMaterialData* material_data = nullptr;
	RID sky_material;

	if (background == RSE::ENV_BG_CLEAR_COLOR || background == RSE::ENV_BG_COLOR) {
		sky_material = sky_scene_state.fog_material;
		material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	}
	else if (sky) {
		sky_material = sky_get_material(RendererSceneRender::environment_get_sky(p_env));

		if (sky_material.is_valid()) {
			material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
				sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
			if (!material_data || !material_data->shader_data->valid) {
				material_data = nullptr;
			}
		}
	}

	if (!material_data) {
		sky_material = sky_shader.default_material;
		material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	}

	return material_data;
}

RID SkyRD::sky_get_material(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, RID());

	return sky->material;
}

float SkyRD::sky_get_baked_exposure(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 1.0);

	return sky->baked_exposure;
}

RID SkyRD::allocate_sky_rid() { return sky_owner.allocate_rid(); }

void SkyRD::initialize_sky_rid(RID p_rid) { sky_owner.initialize_rid(p_rid, Sky()); }

SkyRD::Sky* SkyRD::get_sky(RID p_sky) const { return sky_owner.get_or_null(p_sky); }

void SkyRD::free_sky(RID p_sky)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	sky->free();
	sky_owner.free(p_sky);
}

void SkyRD::sky_set_radiance_size(RID p_sky, int p_radiance_size)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_radiance_size(p_radiance_size)) {
		invalidate_sky(sky);
	}
}

int SkyRD::sky_get_radiance_size(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 0);

	return sky->get_radiance_size();
}

void SkyRD::sky_set_mode(RID p_sky, RSE::SkyMode p_mode)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_mode(p_mode)) {
		invalidate_sky(sky);
	}
}

void SkyRD::sky_set_material(RID p_sky, RID p_material)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_material(p_material)) {
		invalidate_sky(sky);
	}
}

Ref<Image> SkyRD::sky_bake_panorama(
	RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, Ref<Image>());

	update_dirty_skys();

	return sky->bake_panorama(p_energy, p_bake_irradiance ? roughness_layers : 0, p_size);
}

RID SkyRD::sky_get_radiance_texture_rd(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, RID());

	return sky->radiance;
}

float SkyRD::sky_get_uv_border_size(RID p_sky)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 1.0);

	return sky->uv_border_size;
}

void RendererRD::SkyRD::ReflectionData::update_reflection_data(int p_size, int p_mipmaps,
	bool p_use_float, RID p_radiance_texture, int p_roughness_layers, bool p_use_fast_filter,
	int p_sample_count, RenderingDeviceCommons::DataFormat p_format, float p_intensity)
{
}

void RendererRD::SkyRD::ReflectionData::create_reflection_importance_sample(
	bool p_use_float, int p_sample_count, unsigned int p_layers)
{
}

void RendererRD::SkyRD::ReflectionData::create_reflection_fast_filter(bool p_use_float) {}

void RendererRD::SkyRD::update_res_buffers(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_sky,
	double p_time, float p_step, float p_luminance_multiplier)
{
}

void RendererRD::SkyRD::update_radiance_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_sky, const Vector3& p_cam_pos, double p_time, float p_step, float p_luminance_multiplier)
{
}

RID RendererRD::SkyRD::SkyShader::get_default_shader_rd(bool p_wireframe) { return RID(); }

RendererRD::SkyRD::SkyRD() {}

void RendererRD::SkyRD::SkyShaderData::set_code(const String& p_code) {}

void SkyRD::init()
{
	ShaderCompiler::DefaultIdentifierActions actions;

	actions.renames["COLOR"] = "color";
	actions.renames["ALPHA"] = "alpha";
	actions.renames["EYEDIR"] = "eye_dir";
	actions.renames["POSITION"] = "position";
	actions.renames["TIME"] = "time";
	actions.renames["PI"] = "3.14159265358979323846";
	actions.renames["TAU"] = "6.28318530717958647692";
	actions.renames["E"] = "2.71828182845904523536";
	actions.renames["HALF_RES_COLOR"] = "half_res_color";
	actions.renames["QUARTER_RES_COLOR"] = "quarter_res_color";
	actions.renames["RADIANCE"] = "radiance";
	actions.renames["FOG"] = "custom_fog";
	actions.renames["SKY_COORDS"] = "sky_coords";
	actions.renames["SCREEN_UV"] = "screen_uv";

	actions.renames["LIGHT0_DIRECTION"] = "light_directional_data[0].direction";
	actions.renames["LIGHT0_COLOR"] = "light_directional_data[0].color";
	actions.renames["LIGHT0_ENERGY"] = "light_directional_data[0].energy";
	actions.renames["LIGHT0_SIZE"] = "light_directional_data[0].size";
	actions.renames["LIGHT0_ENABLED"] = "light_directional_data[0].enabled";

	actions.renames["LIGHT1_DIRECTION"] = "light_directional_data[1].direction";
	actions.renames["LIGHT1_COLOR"] = "light_directional_data[1].color";
	actions.renames["LIGHT1_ENERGY"] = "light_directional_data[1].energy";
	actions.renames["LIGHT1_SIZE"] = "light_directional_data[1].size";
	actions.renames["LIGHT1_ENABLED"] = "light_directional_data[1].enabled";

	actions.renames["LIGHT2_DIRECTION"] = "light_directional_data[2].direction";
	actions.renames["LIGHT2_COLOR"] = "light_directional_data[2].color";
	actions.renames["LIGHT2_ENERGY"] = "light_directional_data[2].energy";
	actions.renames["LIGHT2_SIZE"] = "light_directional_data[2].size";
	actions.renames["LIGHT2_ENABLED"] = "light_directional_data[2].enabled";

	actions.renames["LIGHT3_DIRECTION"] = "light_directional_data[3].direction";
	actions.renames["LIGHT3_COLOR"] = "light_directional_data[3].color";
	actions.renames["LIGHT3_ENERGY"] = "light_directional_data[3].energy";
	actions.renames["LIGHT3_SIZE"] = "light_directional_data[3].size";
	actions.renames["LIGHT3_ENABLED"] = "light_directional_data[3].enabled";

	actions.render_mode_defines["use_half_res"] = "#define USE_HALF_RES\n";
	actions.render_mode_defines["use_quarter_res"] = "#define USE_QUARTER_RES\n";
	actions.render_mode_defines["disable_fog"] = "#define DISABLE_FOG\n";
	actions.render_mode_defines["use_debanding"] = "#define USE_DEBANDING\n";

	sky_shader.compiler.initialize(actions);

	MaterialStorage::shader_set_data_request_function(
		MaterialStorage::SHADER_TYPE_SKY, _create_sky_shader_funcs);
	MaterialStorage::material_set_data_request_function(
		MaterialStorage::SHADER_TYPE_SKY, _create_sky_material_funcs);

	sky_shader.default_shader = MaterialStorage::shader_allocate();
	MaterialStorage::shader_initialize(sky_shader.default_shader);
	MaterialStorage::shader_set_code(sky_shader.default_shader,
		R"(
shader_type sky;

void sky() {
	COLOR = vec3(0.0);
}
)");

	sky_shader.default_material = MaterialStorage::material_allocate();
	MaterialStorage::material_initialize(sky_shader.default_material);
	MaterialStorage::material_set_shader(
		sky_shader.default_material, sky_shader.default_shader);
}
#undef RB_SCOPE_SKY
#undef RB_HALF_TEXTURE
#undef RB_QUARTER_TEXTURE

// --- Merged from servers/rendering/renderer_rd/environment/gi.cpp ---

using namespace RendererRD;

const Vector3i GI::SDFGI::Cascade::DIRTY_ALL = Vector3i(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);

RID GI::voxel_gi_allocate() { return voxel_gi_owner.allocate_rid(); }

void GI::voxel_gi_free(RID p_voxel_gi)
{
	voxel_gi_allocate_data(p_voxel_gi, Transform3D(), AABB(), Vector3i(), Vector<uint8_t>(),
		Vector<uint8_t>(), Vector<uint8_t>(), Vector<int>()); // deallocate
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	voxel_gi->dependency.deleted_notify(p_voxel_gi);
	voxel_gi_owner.free(p_voxel_gi);
}

void GI::voxel_gi_initialize(RID p_voxel_gi)
{
	voxel_gi_owner.initialize_rid(p_voxel_gi, VoxelGI());
}

void GI::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
	const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	if (voxel_gi->octree_buffer.is_valid()) {
		RD::free_rid(voxel_gi->octree_buffer);
		RD::free_rid(voxel_gi->data_buffer);
		if (voxel_gi->sdf_texture.is_valid()) {
			RD::free_rid(voxel_gi->sdf_texture);
		}

		voxel_gi->sdf_texture = RID();
		voxel_gi->octree_buffer = RID();
		voxel_gi->data_buffer = RID();
		voxel_gi->octree_buffer_size = 0;
		voxel_gi->data_buffer_size = 0;
		voxel_gi->cell_count = 0;
	}

	voxel_gi->to_cell_xform = p_to_cell_xform;
	voxel_gi->bounds = p_aabb;
	voxel_gi->octree_size = p_octree_size;
	voxel_gi->level_counts = p_level_counts;

	if (p_octree_cells.size()) {
		ERR_FAIL_COND(p_octree_cells.size() % 32 != 0); // cells size must be a multiple of 32

		uint32_t cell_count = p_octree_cells.size() / 32;

		ERR_FAIL_COND(p_data_cells.size() != (int)cell_count * 16); // see that data size matches

		voxel_gi->cell_count = cell_count;
		voxel_gi->octree_buffer = RD::storage_buffer_create(p_octree_cells.size(), p_octree_cells);
		voxel_gi->octree_buffer_size = p_octree_cells.size();
		voxel_gi->data_buffer = RD::storage_buffer_create(p_data_cells.size(), p_data_cells);
		voxel_gi->data_buffer_size = p_data_cells.size();

		if (p_distance_field.size()) {
			RDC::TextureFormat tf;
			tf.format = RDC::DATA_FORMAT_R8_UNORM;
			tf.width = voxel_gi->octree_size.x;
			tf.height = voxel_gi->octree_size.y;
			tf.depth = voxel_gi->octree_size.z;
			tf.texture_type = RDC::TEXTURE_TYPE_3D;
			tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT |
							RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			Vector<Vector<uint8_t>> s;
			s.push_back(p_distance_field);
			voxel_gi->sdf_texture = RD::texture_create(tf, RDD::TextureView(), s);
			RD::set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
		}
#if 0
			{
				RD::TextureFormat tf;
				tf.format = RD::DATA_FORMAT_R8_UNORM;
				tf.width = voxel_gi->octree_size.x;
				tf.height = voxel_gi->octree_size.y;
				tf.depth = voxel_gi->octree_size.z;
				tf.type = RD::TEXTURE_TYPE_3D;
				tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UNORM);
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UINT);
				voxel_gi->sdf_texture = RD::texture_create(tf, RD::TextureView());
				RD::set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
			}
			RID shared_tex;
			{
				RD::TextureView tv;
				tv.format_override = RD::DATA_FORMAT_R8_UINT;
				shared_tex = RD::texture_create_shared(tv, voxel_gi->sdf_texture);
			}
			//update SDF texture
			Vector<RD::Uniform> uniforms;
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 1;
				u.append_id(voxel_gi->octree_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 2;
				u.append_id(voxel_gi->data_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
				u.binding = 3;
				u.append_id(shared_tex);
				uniforms.push_back(u);
			}

			RID uniform_set = RD::uniform_set_create(uniforms, voxel_gi_sdf_shader_version_shader, 0);

			{
				uint32_t push_constant[4] = { 0, 0, 0, 0 };

				for (int i = 0; i < voxel_gi->level_counts.size() - 1; i++) {
					push_constant[0] += voxel_gi->level_counts[i];
				}
				push_constant[1] = push_constant[0] + voxel_gi->level_counts[voxel_gi->level_counts.size() - 1];

				print_line("offset: " + itos(push_constant[0]));
				print_line("size: " + itos(push_constant[1]));
				//create SDF
				RD::ComputeListID compute_list = RD::compute_list_begin();
				RD::compute_list_bind_compute_pipeline(compute_list, voxel_gi_sdf_shader_pipeline);
				RD::compute_list_bind_uniform_set(compute_list, uniform_set, 0);
				RD::compute_list_set_push_constant(compute_list, push_constant, sizeof(uint32_t) * 4);
				RD::compute_list_dispatch(compute_list, voxel_gi->octree_size.x / 4, voxel_gi->octree_size.y / 4, voxel_gi->octree_size.z / 4);
				RD::compute_list_end();
			}

			RD::free(uniform_set);
			RD::free(shared_tex);
		}
#endif
	}

	voxel_gi->version++;
	voxel_gi->data_version++;

	voxel_gi->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB GI::voxel_gi_get_bounds(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, AABB());

	return voxel_gi->bounds;
}

Vector3i GI::voxel_gi_get_octree_size(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector3i());
	return voxel_gi->octree_size;
}

Vector<uint8_t> GI::voxel_gi_get_octree_cells(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->octree_buffer.is_valid()) {
		return RD::buffer_get_data(voxel_gi->octree_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_data_cells(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::buffer_get_data(voxel_gi->data_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_distance_field(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::texture_get_data(voxel_gi->sdf_texture, 0);
	}
	return Vector<uint8_t>();
}

Vector<int> GI::voxel_gi_get_level_counts(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<int>());

	return voxel_gi->level_counts;
}

Transform3D GI::voxel_gi_get_to_cell_xform(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Transform3D());

	return voxel_gi->to_cell_xform;
}

void GI::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->dynamic_range = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_dynamic_range(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);

	return voxel_gi->dynamic_range;
}

void GI::voxel_gi_set_propagation(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->propagation = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_propagation(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->propagation;
}

void GI::voxel_gi_set_energy(RID p_voxel_gi, float p_energy)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->energy = p_energy;
}

float GI::voxel_gi_get_energy(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->energy;
}

void GI::voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->baked_exposure = p_baked_exposure;
}

float GI::voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->baked_exposure;
}

void GI::voxel_gi_set_bias(RID p_voxel_gi, float p_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->bias = p_bias;
}

float GI::voxel_gi_get_bias(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->bias;
}

void GI::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_normal_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->normal_bias = p_normal_bias;
}

float GI::voxel_gi_get_normal_bias(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->normal_bias;
}

void GI::voxel_gi_set_interior(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->interior = p_enable;
}

void GI::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->use_two_bounces = p_enable;
	voxel_gi->version++;
}

bool GI::voxel_gi_is_using_two_bounces(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->use_two_bounces;
}

bool GI::voxel_gi_is_interior(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->interior;
}

uint32_t GI::voxel_gi_get_version(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->version;
}

uint32_t GI::voxel_gi_get_data_version(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->data_version;
}

RID GI::voxel_gi_get_octree_buffer(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->octree_buffer;
}

RID GI::voxel_gi_get_data_buffer(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->data_buffer;
}

RID GI::voxel_gi_get_sdf_texture(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());

	return voxel_gi->sdf_texture;
}

Dependency* GI::voxel_gi_get_dependency(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, nullptr);

	return &voxel_gi->dependency;
}

void GI::sdfgi_reset() { sdfgi_current_version++; }

static RID create_clear_texture(const RDC::TextureFormat& p_format, const String& p_name)
{
	RID texture = RD::texture_create(p_format, RDD::TextureView());
	ERR_FAIL_COND_V_MSG(texture.is_null(), RID(), String("Cannot create texture: ") + p_name);

	RD::set_resource_name(texture, p_name);
	RD::texture_clear(texture, Color(0, 0, 0, 0), 0, p_format.mipmaps, 0, p_format.array_layers);

	return texture;
}

void GI::SDFGI::free_data()
{
	// we don't free things here, we handle SDFGI differently at the moment destructing the object
	// when it needs to change.
}

GI::SDFGI::~SDFGI()
{
	for (const SDFGI::Cascade& c : cascades) {
		RD::free_rid(c.light_data);
		RD::free_rid(c.light_aniso_0_tex);
		RD::free_rid(c.light_aniso_1_tex);
		RD::free_rid(c.sdf_tex);
		RD::free_rid(c.solid_cell_dispatch_buffer_storage);
		RD::free_rid(c.solid_cell_dispatch_buffer_call);
		RD::free_rid(c.solid_cell_buffer);
		RD::free_rid(c.lightprobe_history_tex);
		RD::free_rid(c.lightprobe_average_tex);
		RD::free_rid(c.lights_buffer);
	}

	RD::free_rid(render_albedo);
	RD::free_rid(render_emission);
	RD::free_rid(render_emission_aniso);

	RD::free_rid(render_sdf[0]);
	RD::free_rid(render_sdf[1]);

	RD::free_rid(render_sdf_half[0]);
	RD::free_rid(render_sdf_half[1]);

	for (int i = 0; i < 8; i++) {
		RD::free_rid(render_occlusion[i]);
	}

	RD::free_rid(render_geom_facing);

	RD::free_rid(lightprobe_data);
	RD::free_rid(lightprobe_history_scroll);
	RD::free_rid(lightprobe_average_scroll);
	RD::free_rid(occlusion_data);
	RD::free_rid(ambient_texture);

	RD::free_rid(cascades_ubo);

	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::uniform_set_is_valid(debug_uniform_set[v])) {
			RD::free_rid(debug_uniform_set[v]);
		}
		debug_uniform_set[v] = RID();
	}

	if (RD::uniform_set_is_valid(debug_probes_uniform_set)) {
		RD::free_rid(debug_probes_uniform_set);
	}
	debug_probes_uniform_set = RID();

	if (debug_probes_scene_data_ubo.is_valid()) {
		RD::free_rid(debug_probes_scene_data_ubo);
		debug_probes_scene_data_ubo = RID();
	}
}

void GI::SDFGI::update(RID p_env, const Vector3& p_world_position)
{
	bounce_feedback = RendererSceneRender::environment_get_sdfgi_bounce_feedback(p_env);
	energy = RendererSceneRender::environment_get_sdfgi_energy(p_env);
	normal_bias = RendererSceneRender::environment_get_sdfgi_normal_bias(p_env);
	probe_bias = RendererSceneRender::environment_get_sdfgi_probe_bias(p_env);
	reads_sky = RendererSceneRender::environment_get_sdfgi_read_sky_light(p_env);

	int32_t drag_margin = (cascade_size / SDFGI::PROBE_DIVISOR) / 2;

	for (SDFGI::Cascade& cascade : cascades) {
		cascade.dirty_regions = Vector3i();

		Vector3 probe_half_size =
			Vector3(1, 1, 1) * cascade.cell_size * float(cascade_size / SDFGI::PROBE_DIVISOR) * 0.5;
		probe_half_size = Vector3(0, 0, 0);

		Vector3 world_position = p_world_position;
		world_position.y *= y_mult;
		Vector3i pos_in_cascade = Vector3i((world_position + probe_half_size) / cascade.cell_size);

		for (int j = 0; j < 3; j++) {
			if (pos_in_cascade[j] < cascade.position[j]) {
				while (pos_in_cascade[j] < (cascade.position[j] - drag_margin)) {
					cascade.position[j] -= drag_margin * 2;
					cascade.dirty_regions[j] += drag_margin * 2;
				}
			}
			else if (pos_in_cascade[j] > cascade.position[j]) {
				while (pos_in_cascade[j] > (cascade.position[j] + drag_margin)) {
					cascade.position[j] += drag_margin * 2;
					cascade.dirty_regions[j] -= drag_margin * 2;
				}
			}

			if (cascade.dirty_regions[j] == 0) {
				continue; // not dirty
			}
			else if (uint32_t(Math::abs(cascade.dirty_regions[j])) >= cascade_size) {
				// moved too much, just redraw everything (make all dirty)
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
				break;
			}
		}

		if (cascade.dirty_regions != Vector3i() &&
			cascade.dirty_regions != SDFGI::Cascade::DIRTY_ALL) {
			// see how much the total dirty volume represents from the total volume
			uint32_t total_volume = cascade_size * cascade_size * cascade_size;
			uint32_t safe_volume = 1;
			for (int j = 0; j < 3; j++) {
				safe_volume *= cascade_size - Math::abs(cascade.dirty_regions[j]);
			}
			uint32_t dirty_volume = total_volume - safe_volume;
			if (dirty_volume > (safe_volume / 2)) {
				// more than half the volume is dirty, make all dirty so its only rendered once
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
			}
		}
	}
}

int GI::SDFGI::get_pending_region_data(
	int p_region, Vector3i& r_local_offset, Vector3i& r_local_size, AABB& r_bounds) const
{
	int dirty_count = 0;
	for (uint32_t i = 0; i < cascades.size(); i++) {
		const SDFGI::Cascade& c = cascades[i];

		if (c.dirty_regions == SDFGI::Cascade::DIRTY_ALL) {
			if (dirty_count == p_region) {
				r_local_offset = Vector3i();
				r_local_size = Vector3i(1, 1, 1) * cascade_size;

				r_bounds.position =
					Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + c.position)) *
					c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				r_bounds.size = Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				return i;
			}
			dirty_count++;
		}
		else {
			for (int j = 0; j < 3; j++) {
				if (c.dirty_regions[j] != 0) {
					if (dirty_count == p_region) {
						Vector3i from = Vector3i(0, 0, 0);
						Vector3i to = Vector3i(1, 1, 1) * cascade_size;

						if (c.dirty_regions[j] > 0) {
							// fill from the beginning
							to[j] = c.dirty_regions[j];
						}
						else {
							// fill from the end
							from[j] = to[j] + c.dirty_regions[j];
						}

						for (int k = 0; k < j; k++) {
							// "chip" away previous regions to avoid re-voxelizing the same thing
							if (c.dirty_regions[k] > 0) {
								from[k] += c.dirty_regions[k];
							}
							else if (c.dirty_regions[k] < 0) {
								to[k] += c.dirty_regions[k];
							}
						}

						r_local_offset = from;
						r_local_size = to - from;

						r_bounds.position =
							Vector3(from + Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) +
									c.position) *
							c.cell_size * Vector3(1, 1.0 / y_mult, 1);
						r_bounds.size =
							Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);

						return i;
					}

					dirty_count++;
				}
			}
		}
	}
	return -1;
}

void GI::SDFGI::update_cascades()
{
	// update cascades
	SDFGI::Cascade::UBO cascade_data[SDFGI::MAX_CASCADES];
	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < cascades.size(); i++) {
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;

		cascade_data[i].offset[0] = pos.x;
		cascade_data[i].offset[1] = pos.y;
		cascade_data[i].offset[2] = pos.z;
		cascade_data[i].to_cell = 1.0 / cascades[i].cell_size;
		cascade_data[i].probe_offset[0] = cascades[i].position.x / probe_divisor;
		cascade_data[i].probe_offset[1] = cascades[i].position.y / probe_divisor;
		cascade_data[i].probe_offset[2] = cascades[i].position.z / probe_divisor;
		cascade_data[i].pad = 0;
	}
}

void GI::SDFGI::pre_process_gi(const Transform3D& p_transform, RenderDataRD* p_render_data)
{
	if (p_render_data->sdfgi_update_data == nullptr) {
		return;
	}

	/* Update general SDFGI Buffer */

	SDFGIData sdfgi_data;

	sdfgi_data.grid_size[0] = cascade_size;
	sdfgi_data.grid_size[1] = cascade_size;
	sdfgi_data.grid_size[2] = cascade_size;

	sdfgi_data.max_cascades = cascades.size();
	sdfgi_data.probe_axis_size = probe_axis_count;
	sdfgi_data.cascade_probe_size[0] =
		sdfgi_data.probe_axis_size - 1; // float version for performance
	sdfgi_data.cascade_probe_size[1] = sdfgi_data.probe_axis_size - 1;
	sdfgi_data.cascade_probe_size[2] = sdfgi_data.probe_axis_size - 1;

	float csize = cascade_size;
	sdfgi_data.probe_to_uvw = 1.0 / float(sdfgi_data.cascade_probe_size[0]);
	sdfgi_data.use_occlusion = uses_occlusion;
	// sdfgi_data.energy = energy;

	sdfgi_data.y_mult = y_mult;

	float cascade_voxel_size = (csize / sdfgi_data.cascade_probe_size[0]);
	float occlusion_clamp = (cascade_voxel_size - 0.5) / cascade_voxel_size;
	sdfgi_data.occlusion_clamp[0] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[1] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[2] = occlusion_clamp;
	sdfgi_data.normal_bias = (normal_bias / csize) * sdfgi_data.cascade_probe_size[0];

	// vec2 tex_pixel_size = 1.0 / vec2(ivec2( (OCT_SIZE+2) * params.probe_axis_size *
	// params.probe_axis_size, (OCT_SIZE+2) * params.probe_axis_size ) ); vec3 probe_uv_offset =
	// (ivec3(OCT_SIZE+2,OCT_SIZE+2,(OCT_SIZE+2) * params.probe_axis_size)) * tex_pixel_size.xyx;

	uint32_t oct_size = SDFGI::LIGHTPROBE_OCT_SIZE;

	sdfgi_data.lightprobe_tex_pixel_size[0] =
		1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[1] = 1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[2] = 1.0;

	sdfgi_data.energy = energy;

	sdfgi_data.lightprobe_uv_offset[0] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[0];
	sdfgi_data.lightprobe_uv_offset[1] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[1];
	sdfgi_data.lightprobe_uv_offset[2] = float((oct_size + 2) * sdfgi_data.probe_axis_size) *
										 sdfgi_data.lightprobe_tex_pixel_size[0];

	sdfgi_data.occlusion_renormalize[0] = 0.5;
	sdfgi_data.occlusion_renormalize[1] = 1.0;
	sdfgi_data.occlusion_renormalize[2] = 1.0 / float(sdfgi_data.max_cascades);

	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < sdfgi_data.max_cascades; i++) {
		SDFGIData::ProbeCascadeData& c = sdfgi_data.cascades[i];
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;
		Vector3 cam_origin = p_transform.origin;
		cam_origin.y *= y_mult;
		pos -= cam_origin; // make pos local to camera, to reduce numerical error
		c.position[0] = pos.x;
		c.position[1] = pos.y;
		c.position[2] = pos.z;
		c.to_probe =
			1.0 / (float(cascade_size) * cascades[i].cell_size / float(probe_axis_count - 1));

		Vector3i probe_ofs = cascades[i].position / probe_divisor;
		c.probe_world_offset[0] = probe_ofs.x;
		c.probe_world_offset[1] = probe_ofs.y;
		c.probe_world_offset[2] = probe_ofs.z;

		c.to_cell = 1.0 / cascades[i].cell_size;
		c.exposure_normalization = 1.0;
		if (p_render_data->camera_attributes.is_valid()) {
			float exposure_normalization =
				RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
					p_render_data->camera_attributes);
			c.exposure_normalization =
				exposure_normalization / cascades[i].baked_exposure_normalization;
		}
	}

	/* Update dynamic lights in SDFGI cascades */

	for (uint32_t i = 0; i < cascades.size(); i++) {
		SDFGI::Cascade& cascade = cascades[i];

		SDFGIShader::Light lights[SDFGI::MAX_DYNAMIC_LIGHTS];
		uint32_t idx = 0;
		for (uint32_t j = 0;
			 j < (uint32_t)p_render_data->sdfgi_update_data->directional_lights->size(); j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->directional_lights->get(j);
			ERR_CONTINUE(!RendererRD::LightStorage::owns_light_instance(light_instance));

			RID light = RendererRD::LightStorage::light_instance_get_base_light(light_instance);
			Transform3D light_transform =
				RendererRD::LightStorage::light_instance_get_base_transform(light_instance);

			if (RendererLightStorage::light_directional_get_sky_mode(light) ==
				RSE::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			dir.y *= y_mult;
			dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Color color = RendererLightStorage::light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RSE::LIGHT_DIRECTIONAL;
			lights[idx].energy =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::is_using_physical_light_units()) {
				lights[idx].energy *=
					RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RendererLightStorage::light_has_shadow(light);

			idx++;
		}

		AABB cascade_aabb;
		cascade_aabb.position =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascade.position)) *
			cascade.cell_size;
		cascade_aabb.size = Vector3(1, 1, 1) * cascade_size * cascade.cell_size;

		for (uint32_t j = 0; j < p_render_data->sdfgi_update_data->positional_light_count; j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->positional_light_instances[j];
			ERR_CONTINUE(!RendererRD::LightStorage::owns_light_instance(light_instance));

			RID light = RendererRD::LightStorage::light_instance_get_base_light(light_instance);
			AABB light_aabb =
				RendererRD::LightStorage::light_instance_get_base_aabb(light_instance);
			Transform3D light_transform =
				RendererRD::LightStorage::light_instance_get_base_transform(light_instance);

			uint32_t max_sdfgi_cascade = RendererLightStorage::light_get_max_sdfgi_cascade(light);
			if (i > max_sdfgi_cascade) {
				continue;
			}

			if (!cascade_aabb.intersects(light_aabb)) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			Vector2 area_size = RendererLightStorage::light_area_get_size(light);
			// faster to not do this here
			// dir.y *= y_mult;
			// dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Vector3 pos = light_transform.origin;
			pos.y *= y_mult;
			lights[idx].position[0] = pos.x;
			lights[idx].position[1] = pos.y;
			lights[idx].position[2] = pos.z;
			Color color = RendererLightStorage::light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RendererLightStorage::light_get_type(light);

			lights[idx].energy =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::is_using_physical_light_units()) {
				lights[idx].energy *=
					RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);

				// Convert from Luminous Power to Luminous Intensity
				if (lights[idx].type == RSE::LIGHT_OMNI) {
					lights[idx].energy *= 1.0 / (Math::PI * 4.0);
				}
				else if (lights[idx].type == RSE::LIGHT_SPOT) {
					// Spot Lights are not physically accurate, Luminous Intensity should change in
					// relation to the cone angle. We make this assumption to keep them easy to
					// control.
					lights[idx].energy *= 1.0 / Math::PI;
				}
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RendererLightStorage::light_has_shadow(light);
			lights[idx].attenuation =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ATTENUATION);
			lights[idx].radius =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_RANGE);
			lights[idx].cos_spot_angle = Math::cos(Math::deg_to_rad(
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_SPOT_ANGLE)));
			lights[idx].inv_spot_attenuation = 1.0f / RendererLightStorage::light_get_param(
														  light, RSE::LIGHT_PARAM_SPOT_ATTENUATION);

			if (lights[idx].type == RSE::LIGHT_AREA) {
				Vector3 area_vec_a = light_transform.basis.get_column(0).normalized() * area_size.x;
				Vector3 area_vec_b = light_transform.basis.get_column(1).normalized() * area_size.y;
				Rect2 proj_rect = RendererRD::TextureStorage::area_light_atlas_get_texture_rect(
					RendererLightStorage::light_area_get_texture(light));
				lights[idx].area_width[0] = area_vec_a.x;
				lights[idx].area_width[1] = area_vec_a.y;
				lights[idx].area_width[2] = area_vec_a.z;
				lights[idx].area_height[0] = area_vec_b.x;
				lights[idx].area_height[1] = area_vec_b.y;
				lights[idx].area_height[2] = area_vec_b.z;
				lights[idx].area_projector_rect[0] = proj_rect.position.x;
				lights[idx].area_projector_rect[1] = proj_rect.position.y;
				lights[idx].area_projector_rect[2] = proj_rect.size.x;
				lights[idx].area_projector_rect[3] = proj_rect.size.y;

				Size2i texture_size =
					proj_rect.size * RendererRD::TextureStorage::area_light_atlas_get_size();
				lights[idx].cos_spot_angle =
					MIN(Math::floor(Math::log2(MAX(MIN(texture_size.x, texture_size.y), 1.0f))),
						RendererRD::TextureStorage::area_light_atlas_get_mipmaps()) -
					1.0f; // max mipmaps
				lights[idx].inv_spot_attenuation =
					1.0f / (lights[idx].radius + area_size.length() / 2.0f); // center range

				if (RendererLightStorage::light_area_get_normalize_energy(light)) {
					// normalization to make larger lights output same amount of light as smaller
					// lights with same energy
					float surface_area = area_size.x * area_size.y;
					lights[idx].energy /= surface_area;
				}
			}

			idx++;
		}

		cascade_dynamic_light_count[i] = idx;
	}
}

void GI::VoxelGIInstance::free_resources()
{
	if (texture.is_valid()) {
		RD::free_rid(texture);
		RD::free_rid(write_buffer);

		texture = RID();
		write_buffer = RID();
		mipmaps.clear();
	}

	for (int i = 0; i < dynamic_maps.size(); i++) {
		RD::free_rid(dynamic_maps[i].texture);
		RD::free_rid(dynamic_maps[i].depth);

		// these only exist on the first level...
		if (dynamic_maps[i].fb_depth.is_valid()) {
			RD::free_rid(dynamic_maps[i].fb_depth);
		}
		if (dynamic_maps[i].albedo.is_valid()) {
			RD::free_rid(dynamic_maps[i].albedo);
		}
		if (dynamic_maps[i].normal.is_valid()) {
			RD::free_rid(dynamic_maps[i].normal);
		}
		if (dynamic_maps[i].orm.is_valid()) {
			RD::free_rid(dynamic_maps[i].orm);
		}
	}
	dynamic_maps.clear();
}

void GI::VoxelGIInstance::debug(RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	if (mipmaps.is_empty()) {
		return;
	}

	Projection cam_transform = (p_camera_with_transform * Projection(transform)) *
							   Projection(GI::voxel_gi_get_to_cell_xform(probe).affine_inverse());

	int level = 0;
	Vector3i octree_size = GI::voxel_gi_get_octree_size(probe);

	VoxelGIDebugPushConstant push_constant;
	push_constant.alpha = p_alpha;
	push_constant.dynamic_range = GI::voxel_gi_get_dynamic_range(probe);
	push_constant.cell_offset = mipmaps[level].cell_offset;
	push_constant.level = level;

	push_constant.bounds[0] = octree_size.x >> level;
	push_constant.bounds[1] = octree_size.y >> level;
	push_constant.bounds[2] = octree_size.z >> level;
	push_constant.pad = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			push_constant.projection[i * 4 + j] = cam_transform.columns[i][j];
		}
	}

	if (GI::voxel_gi_debug_uniform_set.is_valid()) {
		RD::free_rid(GI::voxel_gi_debug_uniform_set);
	}
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 1;
		u.append_id(GI::voxel_gi_get_data_buffer(probe));
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 2;
		u.append_id(texture);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER;
		u.binding = 3;
		u.append_id(MaterialStorage::sampler_rd_get_default(
			RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
		uniforms.push_back(u);
	}

	int cell_count;
	if (!p_emission && p_lighting && has_dynamic_object_data) {
		cell_count = push_constant.bounds[0] * push_constant.bounds[1] * push_constant.bounds[2];
	}
	else {
		cell_count = mipmaps[level].cell_count;
	}

	voxel_gi_debug_uniform_set =
		RD::uniform_set_create(uniforms, GI::voxel_gi_debug_shader_version_shaders[0], 0);

	int voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_COLOR;
	if (p_emission) {
		voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_EMISSION;
	}
	else if (p_lighting) {
		voxel_gi_debug_pipeline =
			has_dynamic_object_data ? VOXEL_GI_DEBUG_LIGHT_FULL : VOXEL_GI_DEBUG_LIGHT;
	}
	RD::draw_list_bind_render_pipeline(p_draw_list,
		GI::voxel_gi_debug_shader_version_pipelines[voxel_gi_debug_pipeline].get_render_pipeline(
			RDC::INVALID_ID, RD::framebuffer_get_format(p_framebuffer)));
	RD::draw_list_bind_uniform_set(p_draw_list, GI::voxel_gi_debug_uniform_set, 0);
	RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(VoxelGIDebugPushConstant));
	RD::draw_list_draw(p_draw_list, false, cell_count, 36);
}

void GI::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;

	for (int v = 0; v < SHADER_SPECIALIZATION_VARIATIONS; v++) {
		for (int i = 0; i < MODE_MAX; i++) {
			pipelines[v][i].free();
		}
	}

	sdfgi_shader.debug_pipeline.free();

	for (int i = 0; i < SDFGIShader::DIRECT_LIGHT_MODE_MAX; i++) {
		sdfgi_shader.direct_light_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::INTEGRATE_MODE_MAX; i++) {
		sdfgi_shader.integrate_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::PRE_PROCESS_MAX; i++) {
		sdfgi_shader.preprocess_pipeline[i].free();
	}

	for (int i = 0; i < VOXEL_GI_SHADER_VERSION_MAX; i++) {
		voxel_gi_lighting_shader_version_pipelines[i].free();
	}

	if (shader_version.is_valid()) {
		shader.version_free(shader_version);
		shader_version = RID();
	}
	if (voxel_gi_lighting_shader_version.is_valid()) {
		voxel_gi_shader.version_free(voxel_gi_lighting_shader_version);
		voxel_gi_lighting_shader_version = RID();
	}
	if (voxel_gi_debug_shader_version.is_valid()) {
		voxel_gi_debug_shader.version_free(voxel_gi_debug_shader_version);
		voxel_gi_debug_shader_version = RID();
	}
	if (sdfgi_shader.preprocess_shader.is_valid()) {
		sdfgi_shader.preprocess.version_free(sdfgi_shader.preprocess_shader);
		sdfgi_shader.preprocess_shader = RID();
	}
	if (sdfgi_shader.direct_light_shader.is_valid()) {
		sdfgi_shader.direct_light.version_free(sdfgi_shader.direct_light_shader);
		sdfgi_shader.direct_light_shader = RID();
	}
	if (sdfgi_shader.integrate_shader.is_valid()) {
		sdfgi_shader.integrate.version_free(sdfgi_shader.integrate_shader);
		sdfgi_shader.integrate_shader = RID();
	}
	if (sdfgi_shader.debug_shader_version.is_valid()) {
		sdfgi_shader.debug.version_free(sdfgi_shader.debug_shader_version);
		sdfgi_shader.debug_shader_version = RID();
	}
	if (sdfgi_shader.debug_probes_shader_version.is_valid()) {
		sdfgi_shader.debug_probes.version_free(sdfgi_shader.debug_probes_shader_version);
		sdfgi_shader.debug_probes_shader_version = RID();
	}
}

void GI::free()
{
	if (default_voxel_gi_buffer.is_valid()) {
		RD::free_rid(default_voxel_gi_buffer);
	}
	if (voxel_gi_lights_uniform.is_valid()) {
		RD::free_rid(voxel_gi_lights_uniform);
	}
	if (sdfgi_ubo.is_valid()) {
		RD::free_rid(sdfgi_ubo);
	}

	if (voxel_gi_lights) {
		memdelete_arr(voxel_gi_lights);
	}
}

Ref<GI::SDFGI> GI::create_sdfgi(
	RID p_env, const Vector3& p_world_position, uint32_t p_requested_history_size)
{
	Ref<SDFGI> sdfgi;
	sdfgi.instantiate();

	sdfgi->create(p_env, p_world_position, p_requested_history_size);

	return sdfgi;
}

void GI::setup_voxel_gi_instances(RenderDataRD* p_render_data,
	Ref<RenderSceneBuffersRD> p_render_buffers, const Transform3D& p_transform,
	const PagedArray<RID>& p_voxel_gi_instances, uint32_t& r_voxel_gi_instances_used)
{
	ERR_FAIL_COND(p_render_buffers.is_null());

	r_voxel_gi_instances_used = 0;

	Ref<RenderBuffersGI> rbgi = p_render_buffers->get_custom_data(RB_SCOPE_GI);
	ERR_FAIL_COND(rbgi.is_null());

	RID voxel_gi_buffer = rbgi->get_voxel_gi_buffer();
	VoxelGIData voxel_gi_data[MAX_VOXEL_GI_INSTANCES];

	bool voxel_gi_instances_changed = false;

	Transform3D to_camera;
	to_camera.origin = p_transform.origin; // only translation, make local

	for (int i = 0; i < MAX_VOXEL_GI_INSTANCES; i++) {
		RID texture;
		if (i < (int)p_voxel_gi_instances.size()) {
			VoxelGIInstance* gipi = voxel_gi_instance_owner.get_or_null(p_voxel_gi_instances[i]);

			if (gipi) {
				texture = gipi->texture;
				VoxelGIData& gipd = voxel_gi_data[i];

				RID base_probe = gipi->probe;

				Transform3D to_cell = voxel_gi_get_to_cell_xform(gipi->probe) *
									  gipi->transform.affine_inverse() * to_camera;

				gipd.xform[0] = to_cell.basis.rows[0][0];
				gipd.xform[1] = to_cell.basis.rows[1][0];
				gipd.xform[2] = to_cell.basis.rows[2][0];
				gipd.xform[3] = 0;
				gipd.xform[4] = to_cell.basis.rows[0][1];
				gipd.xform[5] = to_cell.basis.rows[1][1];
				gipd.xform[6] = to_cell.basis.rows[2][1];
				gipd.xform[7] = 0;
				gipd.xform[8] = to_cell.basis.rows[0][2];
				gipd.xform[9] = to_cell.basis.rows[1][2];
				gipd.xform[10] = to_cell.basis.rows[2][2];
				gipd.xform[11] = 0;
				gipd.xform[12] = to_cell.origin.x;
				gipd.xform[13] = to_cell.origin.y;
				gipd.xform[14] = to_cell.origin.z;
				gipd.xform[15] = 1;

				Vector3 bounds = voxel_gi_get_octree_size(base_probe);

				gipd.bounds[0] = bounds.x;
				gipd.bounds[1] = bounds.y;
				gipd.bounds[2] = bounds.z;

				gipd.dynamic_range =
					voxel_gi_get_dynamic_range(base_probe) * voxel_gi_get_energy(base_probe);
				gipd.bias = voxel_gi_get_bias(base_probe);
				gipd.normal_bias = voxel_gi_get_normal_bias(base_probe);
				gipd.blend_ambient = !voxel_gi_is_interior(base_probe);
				gipd.mipmaps = gipi->mipmaps.size();
				gipd.exposure_normalization = 1.0;
				if (p_render_data->camera_attributes.is_valid()) {
					float exposure_normalization =
						RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
					gipd.exposure_normalization =
						exposure_normalization /
						voxel_gi_get_baked_exposure_normalization(base_probe);
				}
			}

			r_voxel_gi_instances_used++;
		}

		if (texture == RID()) {
			texture = RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		}

		if (texture != rbgi->voxel_gi_textures[i]) {
			voxel_gi_instances_changed = true;
			rbgi->voxel_gi_textures[i] = texture;
		}
	}

	if (voxel_gi_instances_changed) {
		for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
			if (RD::uniform_set_is_valid(rbgi->uniform_set[v])) {
				RD::free_rid(rbgi->uniform_set[v]);
			}
			rbgi->uniform_set[v] = RID();
		}

		if (p_render_buffers->has_custom_data(RB_SCOPE_FOG)) {
			// VoxelGI instances have changed, so we need to update volumetric fog.
			Ref<RendererRD::Fog::VolumetricFog> fog =
				p_render_buffers->get_custom_data(RB_SCOPE_FOG);
			fog->sync_gi_dependent_sets_validity(true);
		}
	}

	if (p_voxel_gi_instances.size() > 0) {
		RD::draw_command_begin_label("VoxelGIs Setup");
		RD::draw_command_end_label();
	}
}

void GI::RenderBuffersGI::free_data()
{
	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::uniform_set_is_valid(uniform_set[v])) {
			RD::free_rid(uniform_set[v]);
		}
		uniform_set[v] = RID();
	}

	if (scene_data_ubo.is_valid()) {
		RD::free_rid(scene_data_ubo);
		scene_data_ubo = RID();
	}

	if (voxel_gi_buffer.is_valid()) {
		RD::free_rid(voxel_gi_buffer);
		voxel_gi_buffer = RID();
	}
}

RID GI::voxel_gi_instance_create(RID p_base)
{
	VoxelGIInstance voxel_gi;
	voxel_gi.probe = p_base;
	RID rid = voxel_gi_instance_owner.make_rid(voxel_gi);
	return rid;
}

void GI::voxel_gi_instance_free(RID p_rid)
{
	GI::VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_rid);
	voxel_gi->free_resources();
	voxel_gi_instance_owner.free(p_rid);
}

void GI::voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->transform = p_xform;
}

bool GI::voxel_gi_needs_update(RID p_probe)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(voxel_gi, false);

	return voxel_gi->last_probe_version != voxel_gi_get_version(voxel_gi->probe);
}

void GI::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->update(p_update_light_instances, p_light_instances, p_dynamic_objects);
}

void GI::debug_voxel_gi(RID p_voxel_gi, RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->debug(
		p_draw_list, p_framebuffer, p_camera_with_transform, p_lighting, p_emission, p_alpha);
}

void GI::enable_vrs_shader_group() { shader.enable_group(GROUP_VRS); }

RID RendererRD::GI::RenderBuffersGI::get_voxel_gi_buffer() { return RID(); }

void RendererRD::GI::VoxelGIInstance::update(bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
}

void RendererRD::GI::process_gi(Ref<RenderSceneBuffersRD> p_render_buffers,
	const RID* p_color_buffers, RID p_dest_buffer, RID p_environment, unsigned int p_view_count,
	const Projection* p_projections, const Vector3* p_eye_offsets,
	const Transform3D& p_cam_transform, const PagedArray<RID>& p_position_buffers)
{
}

void RendererRD::GI::initialize(RendererRD::SkyRD* p_sky)
{
	if (_initialized) {
		return;
	}

	// 1. Main GI gather shader
	{
		Vector<String> modes;
		modes.push_back("\n#define MODE_VOXEL_GI\n");
		modes.push_back("\n#define MODE_VOXEL_GI\n#define MODE_VOXEL_GI_WITHOUT_SAMPLER\n");
		modes.push_back("\n#define MODE_SDFGI\n");
		modes.push_back("\n#define MODE_VOXEL_GI\n#define MODE_SDFGI\n");
		modes.push_back(
			"\n#define MODE_VOXEL_GI\n#define MODE_VOXEL_GI_WITHOUT_SAMPLER\n#define MODE_SDFGI\n");

		shader.initialize(modes);
		shader_version = shader.version_create();
	}

	// 2. Voxel GI lighting & debug shaders
	{
		Vector<String> modes;
		modes.push_back("\n#define MODE_COMPUTE_LIGHT\n");
		modes.push_back("\n#define MODE_COMPUTE_SECOND_BOUNCE\n");
		modes.push_back("\n#define MODE_COMPUTE_MIPMAP\n");
		modes.push_back("\n#define MODE_WRITE_TEXTURE\n");
		modes.push_back("\n#define MODE_DYNAMIC_OBJECT_LIGHTING\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_WRITE\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_PLOT\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_WRITE_PLOT\n");

		voxel_gi_shader.initialize(modes);
		voxel_gi_lighting_shader_version = voxel_gi_shader.version_create();

		Vector<String> debug_modes;
		debug_modes.push_back("\n#define MODE_DEBUG_COLOR\n");
		debug_modes.push_back("\n#define MODE_DEBUG_LIGHT\n");
		debug_modes.push_back("\n#define MODE_DEBUG_EMISSION\n");
		debug_modes.push_back("\n#define MODE_DEBUG_LIGHT_FULL\n");

		voxel_gi_debug_shader.initialize(debug_modes);
		voxel_gi_debug_shader_version = voxel_gi_debug_shader.version_create();
	}

	// 3. SDFGI compute pipeline stages
	{
		Vector<String> preprocess_modes;
		preprocess_modes.push_back("\n#define MODE_SCROLL\n");
		preprocess_modes.push_back("\n#define MODE_SCROLL_OCCLUSION\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_INITIALIZE\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_INITIALIZE_HALF\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_OPTIMIZED\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_UPSCALE\n");
		preprocess_modes.push_back("\n#define MODE_OCCLUSION\n");
		preprocess_modes.push_back("\n#define MODE_STORE\n");

		sdfgi_shader.preprocess.initialize(preprocess_modes);
		sdfgi_shader.preprocess_shader = sdfgi_shader.preprocess.version_create();

		Vector<String> direct_light_modes;
		direct_light_modes.push_back("\n#define MODE_STATIC\n");
		direct_light_modes.push_back("\n#define MODE_DYNAMIC\n");

		sdfgi_shader.direct_light.initialize(direct_light_modes);
		sdfgi_shader.direct_light_shader = sdfgi_shader.direct_light.version_create();

		Vector<String> integrate_modes;
		integrate_modes.push_back("\n#define MODE_PROCESS\n");
		integrate_modes.push_back("\n#define MODE_STORE\n");
		integrate_modes.push_back("\n#define MODE_SCROLL\n");
		integrate_modes.push_back("\n#define MODE_SCROLL_STORE\n");

		sdfgi_shader.integrate.initialize(integrate_modes);
		sdfgi_shader.integrate_shader = sdfgi_shader.integrate.version_create();

		Vector<String> debug_modes;
		debug_modes.push_back("\n");

		sdfgi_shader.debug.initialize(debug_modes);
		sdfgi_shader.debug_shader_version = sdfgi_shader.debug.version_create();

		Vector<String> debug_probes_modes;
		debug_probes_modes.push_back("\n#define MODE_PROBES\n");
		debug_probes_modes.push_back("\n#define MODE_PROBES\n#define MODE_MULTIVIEW\n");
		debug_probes_modes.push_back("\n#define MODE_VISIBILITY\n");
		debug_probes_modes.push_back("\n#define MODE_VISIBILITY\n#define MODE_MULTIVIEW\n");

		sdfgi_shader.debug_probes.initialize(debug_probes_modes);
		sdfgi_shader.debug_probes_shader_version = sdfgi_shader.debug_probes.version_create();
	}

	_initialized = true;
}

// --- Merged from servers/rendering/renderer_rd/environment/fog.cpp ---

using namespace RendererRD;


void Fog::initialize() {}

void Fog::finalize() {}

int Fog::_get_fog_shader_group()
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	if (use_vulkan_memory_model) {
		return use_32_bit_atomics
				   ? VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL
				   : VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL_NO_ATOMICS;
	}
	else {
		return use_32_bit_atomics ? VolumetricFogShader::SHADER_GROUP_BASE
								  : VolumetricFogShader::SHADER_GROUP_NO_ATOMICS;
	}
}

int Fog::_get_fog_variant()
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? 2 : 0) + (use_32_bit_atomics ? 0 : 1);
}

int Fog::_get_fog_process_variant(int p_idx)
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? (VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX * 2)
									: 0) +
		   (use_32_bit_atomics ? 0 : VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX) +
		   p_idx;
}

/* FOG VOLUMES */

RID Fog::fog_volume_allocate() { return fog_volume_owner.allocate_rid(); }

void Fog::fog_volume_initialize(RID p_rid) { fog_volume_owner.initialize_rid(p_rid, FogVolume()); }

void Fog::fog_volume_free(RID p_rid)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_rid);
	fog_volume->dependency.deleted_notify(p_rid);
	fog_volume_owner.free(p_rid);
}

Dependency* Fog::fog_volume_get_dependency(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, nullptr);

	return &fog_volume->dependency;
}

void Fog::fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	if (p_shape == fog_volume->shape) {
		return;
	}

	fog_volume->shape = p_shape;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_size(RID p_fog_volume, const Vector3& p_size)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	fog_volume->size = p_size;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_material(RID p_fog_volume, RID p_material)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);
	fog_volume->material = p_material;
}

RID Fog::fog_volume_get_material(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RID());

	return fog_volume->material;
}

RSE::FogVolumeShape Fog::fog_volume_get_shape(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RSE::FOG_VOLUME_SHAPE_BOX);

	return fog_volume->shape;
}

AABB Fog::fog_volume_get_aabb(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, AABB());

	switch (fog_volume->shape) {
	case RSE::FOG_VOLUME_SHAPE_ELLIPSOID:
	case RSE::FOG_VOLUME_SHAPE_CONE:
	case RSE::FOG_VOLUME_SHAPE_CYLINDER:
	case RSE::FOG_VOLUME_SHAPE_BOX: {
		AABB aabb;
		aabb.position = -fog_volume->size / 2;
		aabb.size = fog_volume->size;
		return aabb;
	}
	default: {
		// Need some size otherwise will get culled
		return AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	}
	}
}

Vector3 Fog::fog_volume_get_size(RID p_fog_volume)
{
	const FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, Vector3());
	return fog_volume->size;
}

Fog::FogMaterialData::~FogMaterialData() { free_parameters_uniform_set(uniform_set); }

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_func()
{
	FogShaderData* shader_data = memnew(FogShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_funcs()
{
	return Fog::_create_fog_shader_func();
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_func(FogShaderData* p_shader)
{
	FogMaterialData* material_data = memnew(FogMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_funcs(
	RendererRD::MaterialStorage::ShaderData* p_shader)
{
	return Fog::_create_fog_material_func(static_cast<FogShaderData*>(p_shader));
}

RID Fog::fog_volume_instance_create(RID p_fog_volume)
{
	FogVolumeInstance fvi;
	fvi.volume = p_fog_volume;
	return fog_volume_instance_owner.make_rid(fvi);
}

void Fog::fog_instance_free(RID p_rid) { fog_volume_instance_owner.free(p_rid); }

void Fog::free_fog_shader()
{
	for (int i = 0; i < VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX; i++) {
		volumetric_fog.process_pipelines[i].free();
	}
	if (volumetric_fog.process_shader_version.is_valid()) {
		volumetric_fog.process_shader.version_free(volumetric_fog.process_shader_version);
	}
	if (volumetric_fog.volume_ubo.is_valid()) {
		RD::free_rid(volumetric_fog.volume_ubo);
	}
	if (volumetric_fog.params_ubo.is_valid()) {
		RD::free_rid(volumetric_fog.params_ubo);
	}
	if (volumetric_fog.default_shader.is_valid()) {
		MaterialStorage::shader_free(volumetric_fog.default_shader);
	}
	if (volumetric_fog.default_material.is_valid()) {
		MaterialStorage::material_free(volumetric_fog.default_material);
	}
}

void Fog::FogShaderData::set_code(const String& p_code)
{
	// compile

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	if (code.is_empty()) {
		return; // just invalid, but no error
	}

	ShaderCompiler::GeneratedCode gen_code;
	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["fog"] = ShaderCompiler::STAGE_COMPUTE;

	uses_time = false;

	actions.usage_flag_pointers["TIME"] = &uses_time;

	actions.uniforms = &uniforms;

	Error err = RendererRD::Fog::volumetric_fog.compiler.compile(
		RSE::SHADER_FOG, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Fog shader compilation failed.");

	if (version.is_null()) {
		version = RendererRD::Fog::volumetric_fog.shader.version_create();
	}
	else {
		pipeline.free();
	}

	RendererRD::Fog::volumetric_fog.shader.version_set_compute_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_COMPUTE], gen_code.defines);
	ERR_FAIL_COND(!RendererRD::Fog::volumetric_fog.shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool Fog::FogShaderData::is_animated() const { return false; }

bool Fog::FogShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode Fog::FogShaderData::get_native_source_code() const
{
	return RendererRD::Fog::volumetric_fog.shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> Fog::FogShaderData::get_native_shader_and_version() const
{
	return {&RendererRD::Fog::volumetric_fog.shader, version};
}

Fog::FogShaderData::~FogShaderData()
{
	pipeline.free();

	if (version.is_valid()) {
		RendererRD::Fog::volumetric_fog.shader.version_free(version);
	}
}

bool Fog::VolumetricFog::sync_gi_dependent_sets_validity(bool p_ensure_freed)
{
	bool null = gi_dependent_sets.process_uniform_set_density.is_null();
	bool valid = !null && RD::uniform_set_is_valid(
							  gi_dependent_sets.process_uniform_set_density);

#ifdef DEV_ENABLED
	// It's all-or-nothing, or something else has changed that requires dev attention.
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set.is_null());
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set2.is_null());
	DEV_ASSERT(
		valid == RD::uniform_set_is_valid(gi_dependent_sets.process_uniform_set));
	DEV_ASSERT(
		valid == RD::uniform_set_is_valid(gi_dependent_sets.process_uniform_set2));
#endif

	if (valid) {
		if (p_ensure_freed) {
			RD::free_rid(gi_dependent_sets.process_uniform_set_density);
			RD::free_rid(gi_dependent_sets.process_uniform_set);
			RD::free_rid(gi_dependent_sets.process_uniform_set2);
			valid = false;
		}
	}

	if (!valid && !null) {
		gi_dependent_sets = {};
	}

	return valid;
}

void Fog::VolumetricFog::init(const Vector3i& fog_size, RID p_sky_shader)
{
	width = fog_size.x;
	height = fog_size.y;
	depth = fog_size.z;
	atomic_type = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT)
					  ? RDC::UNIFORM_TYPE_IMAGE
					  : RDC::UNIFORM_TYPE_STORAGE_BUFFER;

	RDC::TextureFormat tf;
	tf.format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	tf.width = fog_size.x;
	tf.height = fog_size.y;
	tf.depth = fog_size.z;
	tf.texture_type = RDC::TEXTURE_TYPE_3D;
	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

	light_density_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(light_density_map, "Fog light-density map");

	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT |
					RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;

	prev_light_density_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(
		prev_light_density_map, "Fog previous light-density map");
	RD::texture_clear(prev_light_density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);

	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;

	fog_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(fog_map, "Fog map");

	if (atomic_type == RDC::UNIFORM_TYPE_STORAGE_BUFFER) {
		Vector<uint8_t> dm;
		dm.resize_initialized(fog_size.x * fog_size.y * fog_size.z * 4);

		density_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(density_map, "Fog density map");
		light_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(light_map, "Fog light map");
		emissive_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(emissive_map, "Fog emissive map");
	}
	else {
		tf.format = RDC::DATA_FORMAT_R32_UINT;
		tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT |
						RDC::TEXTURE_USAGE_STORAGE_ATOMIC_BIT;
		density_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(density_map, "Fog density map");
		RD::texture_clear(density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		light_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(light_map, "Fog light map");
		RD::texture_clear(light_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		emissive_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(emissive_map, "Fog emissive map");
		RD::texture_clear(emissive_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
	}

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.append_id(fog_map);
		uniforms.push_back(u);
	}

	sky_uniform_set = RD::uniform_set_create(
		uniforms, p_sky_shader, RendererRD::SkyRD::SKY_SET_FOG);
}

Fog::VolumetricFog::~VolumetricFog()
{
	RD::free_rid(prev_light_density_map);
	RD::free_rid(light_density_map);
	RD::free_rid(fog_map);
	RD::free_rid(density_map);
	RD::free_rid(light_map);
	RD::free_rid(emissive_map);

	if (fog_uniform_set.is_valid() && RD::uniform_set_is_valid(fog_uniform_set)) {
		RD::free_rid(fog_uniform_set);
	}
	if (copy_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(copy_uniform_set)) {
		RD::free_rid(copy_uniform_set);
	}

	sync_gi_dependent_sets_validity(true);

	if (sdfgi_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sdfgi_uniform_set)) {
		RD::free_rid(sdfgi_uniform_set);
	}
	if (sky_uniform_set.is_valid() && RD::uniform_set_is_valid(sky_uniform_set)) {
		RD::free_rid(sky_uniform_set);
	}
}

Vector3i Fog::_point_get_position_in_froxel_volume(const Vector3& p_point, float fog_end,
	const Vector2& fog_near_size, const Vector2& fog_far_size, float volumetric_fog_detail_spread,
	const Vector3& fog_size, const Transform3D& p_cam_transform)
{
	Vector3 view_position = p_cam_transform.affine_inverse().xform(p_point);
	view_position.z = MIN(view_position.z, -0.01); // Clamp to the front of camera
	Vector3 fog_position = Vector3(0, 0, 0);

	view_position.y = -view_position.y;
	fog_position.z = -view_position.z / fog_end;
	fog_position.x =
		(view_position.x /
			(2 * (fog_near_size.x * (1.0 - fog_position.z) + fog_far_size.x * fog_position.z))) +
		0.5;
	fog_position.y =
		(view_position.y /
			(2 * (fog_near_size.y * (1.0 - fog_position.z) + fog_far_size.y * fog_position.z))) +
		0.5;
	fog_position.z = Math::pow(float(fog_position.z), float(1.0 / volumetric_fog_detail_spread));
	fog_position = fog_position * fog_size - Vector3(0.5, 0.5, 0.5);

	fog_position = fog_position.clamp(Vector3(), fog_size);

	return Vector3i(fog_position);
}


// --- Merged from servers/rendering/renderer_rd/effects/copy_effects.cpp ---
#include <thirdparty/misc/cubemap_coeffs.h>
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/rendering_device.h"

using namespace RendererRD;

/* CopyEffects */

void CopyEffects::initialize(uint32_t p_raster_effects)
{
	if (data) {
		return;
	}
	data = memnew(Data);
	data->raster_effects = p_raster_effects;
}

void CopyEffects::finalize()
{
	if (!data) {
		return;
	}

	for (int i = 0; i < BLUR_MODE_MAX; i++) {
		data->blur_raster.pipelines[i].clear();
	}
	if (data->blur_raster.shader_version.is_valid()) {
		data->blur_raster.shader.version_free(data->blur_raster.shader_version);
	}
	if (data->blur_raster.glow_sampler.is_valid()) {
		RD::free_rid(data->blur_raster.glow_sampler);
	}

	for (int i = 0; i < COPY_MODE_MAX; i++) {
		data->copy.pipelines[i].free();
	}
	if (data->copy.shader_version.is_valid()) {
		data->copy.shader.version_free(data->copy.shader_version);
	}

	for (int i = 0; i < COPY_TO_FB_MAX; i++) {
		data->copy_to_fb.pipelines[i].clear();
	}
	if (data->copy_to_fb.shader_version.is_valid()) {
		data->copy_to_fb.shader.version_free(data->copy_to_fb.shader_version);
	}

	data->cube_to_dp.pipeline.clear();
	if (data->cube_to_dp.shader_version.is_valid()) {
		data->cube_to_dp.shader.version_free(data->cube_to_dp.shader_version);
	}

	data->cube_to_octmap.pipeline.clear();
	if (data->cube_to_octmap.shader_version.is_valid()) {
		data->cube_to_octmap.shader.version_free(data->cube_to_octmap.shader_version);
	}

	if (data->octmap_downsampler.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			data->octmap_downsampler.raster_pipeline.clear();
			data->octmap_downsampler.raster_shader.version_free(
				data->octmap_downsampler.shader_version);
		}
		else {
			for (int i = 0; i < DOWNSAMPLER_MODE_COMPUTE_MAX; i++) {
				data->octmap_downsampler.compute_pipelines[i].free();
			}
			data->octmap_downsampler.compute_shader.version_free(
				data->octmap_downsampler.shader_version);
		}
	}

	if (data->filter.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			for (int i = 0; i < FILTER_MODE_RASTER_MAX; i++) {
				data->filter.raster_pipelines[i].clear();
			}
			data->filter.raster_shader.version_free(data->filter.shader_version);
		}
		else {
			for (int i = 0; i < FILTER_MODE_COMPUTE_MAX; i++) {
				data->filter.compute_pipelines[i].free();
			}
			data->filter.compute_shader.version_free(data->filter.shader_version);
		}
	}
	if (data->filter.coefficient_buffer.is_valid()) {
		RD::free_rid(data->filter.coefficient_buffer);
	}

	if (data->roughness.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			data->roughness.raster_pipeline.clear();
			data->roughness.raster_shader.version_free(data->roughness.shader_version);
		}
		else {
			for (int i = 0; i < ROUGHNESS_MODE_MAX; i++) {
				data->roughness.compute_pipelines[i].free();
			}
			data->roughness.compute_shader.version_free(data->roughness.shader_version);
		}
	}

	for (int i = 0; i < SPECULAR_MERGE_MAX; i++) {
		data->specular_merge.pipelines[i].clear();
	}
	if (data->specular_merge.shader_version.is_valid()) {
		data->specular_merge.shader.version_free(data->specular_merge.shader_version);
	}

	memdelete(data);
	data = nullptr;
}

void CopyEffects::copy_to_rect(RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_rect,
	bool p_flip_y, bool p_force_luminance, bool p_all_source, bool p_8_bit_dst, bool p_alpha_to_one,
	bool p_sanitize_inf_nan)
{
}

void CopyEffects::copy_octmap_to_panorama(RID p_source_octmap, RID p_dest_panorama,
	const Size2i& p_panorama_size, float p_lod, bool p_is_array,
	const Size2& p_source_octmap_border_size)
{
}

void CopyEffects::copy_depth_to_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2i& p_rect, bool p_flip_y)
{
}

void CopyEffects::copy_depth_to_rect_and_linearize(RID p_source_rd_texture, RID p_dest_texture,
	const Rect2i& p_rect, bool p_flip_y, float p_z_near, float p_z_far)
{
}

void CopyEffects::copy_to_fb_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2i& p_rect, bool p_flip_y, bool p_force_luminance, bool p_alpha_to_zero, bool p_srgb,
	RID p_secondary, bool p_multiview, bool alpha_to_one, bool p_linear, bool p_normal,
	const Rect2& p_src_rect, float p_linear_luminance_multiplier, bool p_bilinear_filtering)
{
}

void CopyEffects::copy_to_atlas_fb(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2& p_uv_rect, RD::DrawListID p_draw_list, bool p_flip_y, bool p_panorama)
{
}

void CopyEffects::copy_to_drawlist(RD::DrawListID p_draw_list, RD::FramebufferFormatID p_fb_format,
	RID p_source_rd_texture, bool p_linear, float p_linear_luminance_multiplier)
{
}

void CopyEffects::copy_raster(RID p_source_texture, RID p_dest_framebuffer) {}

static constexpr int _compute_dispatch_size(bool p_use_array)
{
	constexpr int SIZE = 320;
	constexpr int GROUP = 64;
	constexpr int LEVELS = 6; // One less than Sky::REAL_TIME_ROUGHNESS_LAYERS.
	int size = 0;
	if (p_use_array) {
		size = SIZE * SIZE * LEVELS;
	}
	else {
		int dim = SIZE;
		for (int i = 0; i < LEVELS && dim >= 2; i++) {
			size += dim * dim;
			dim >>= 1;
		}
	}

	return (size + GROUP - 1) / GROUP;
}

void CopyEffects::gaussian_blur(RID p_source_rd_texture, RID p_texture, const Rect2i& p_region,
	const Size2i& p_size, bool p_8bit_dst)
{
}

void CopyEffects::gaussian_blur_raster(
	RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_region, const Size2i& p_size)
{
}

void CopyEffects::gaussian_glow(RID p_source_rd_texture, RID p_back_texture, const Size2i& p_size,
	float p_strength, bool p_first_pass, float p_luminance_cap, float p_exposure, float p_bloom,
	float p_hdr_bleed_threshold, float p_hdr_bleed_scale, RID p_auto_exposure,
	float p_auto_exposure_scale)
{
}

void CopyEffects::gaussian_glow_downsample_raster(RID p_source_rd_texture, RID p_dest_texture,
	float p_luminance_multiplier, const Size2i& p_size, float p_strength, bool p_first_pass,
	float p_luminance_cap, float p_exposure, float p_bloom, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale)
{
}

void CopyEffects::gaussian_glow_upsample_raster(RID p_source_rd_texture, RID p_dest_texture,
	RID p_blend_texture, float p_luminance_multiplier, const Size2i& p_source_size,
	const Size2i& p_dest_size, float p_level, float p_base_strength, bool p_use_debanding)
{
}

void CopyEffects::make_mipmap(RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size) {}

void CopyEffects::make_mipmap_raster(
	RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size)
{
}

void CopyEffects::set_color(
	RID p_dest_texture, const Color& p_color, const Rect2i& p_region, bool p_8bit_dst)
{
}

void CopyEffects::set_color_raster(RID p_dest_texture, const Color& p_color, const Rect2i& p_region)
{
}

void CopyEffects::copy_cubemap_to_dp(RID p_source_rd_texture, RID p_dst_framebuffer,
	const Rect2& p_rect, const Vector2& p_dst_size, float p_z_near, float p_z_far, bool p_dp_flip)
{
}

void CopyEffects::copy_cubemap_to_octmap(
	RID p_source_rd_texture, RID p_dst_framebuffer, float p_border_size)
{
}

void CopyEffects::octmap_downsample(
	RID p_source_octmap, RID p_dest_octmap, const Size2i& p_size, float p_border_size)
{
}

void CopyEffects::octmap_downsample_raster(
	RID p_source_octmap, RID p_dest_framebuffer, const Size2i& p_size, float p_border_size)
{
}

void CopyEffects::octmap_filter(
	RID p_source_octmap, const Vector<RID>& p_dest_octmap, bool p_use_array, float p_border_size)
{
}

void CopyEffects::octmap_filter_raster(
	RID p_source_octmap, RID p_dest_framebuffer, uint32_t p_mip_level, float p_border_size)
{
}

void CopyEffects::octmap_roughness(RID p_source_rd_texture, RID p_dest_texture,
	uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
	float p_border_size)
{
}

void CopyEffects::octmap_roughness_raster(RID p_source_rd_texture, RID p_dest_framebuffer,
	uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
	float p_border_size)
{
}

void CopyEffects::merge_specular(RID p_dest_framebuffer, RID p_specular, RID p_base,
	RID p_reflection, uint32_t p_view_count)
{
}

/* Resolve */

Resolve::~Resolve()
{
	if (prefer_raster_effects) {
		resolve_raster.shader.version_free(resolve_raster.shader_version);
	}
	else {
		resolve.shader.version_free(resolve.shader_version);
	}
}

void Resolve::resolve_depth(
	RID p_source_depth, RID p_dest_depth, Vector2i p_screen_size, int p_samples)
{
}

void RendererRD::Resolve::resolve_gi(RID p_source_color, RID p_source_depth, RID p_dest_color,
	RID p_dest_depth, RID p_velocity, RID p_normal, Vector2i p_size, int p_samples)
{
}

Resolve::Resolve(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	if (prefer_raster_effects) {
		Vector<String> modes;
		modes.push_back("\n");

		resolve_raster.shader.initialize(modes);
		resolve_raster.shader_version = resolve_raster.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n#define MODE_GI\n");
		modes.push_back("\n#define MODE_GI\n#define MODE_VOXEL_GI\n");
		modes.push_back("\n#define MODE_DEPTH\n");

		resolve.shader.initialize(modes);
		resolve.shader_version = resolve.shader.version_create();
	}
}

/* RoughnessLimiter */

RoughnessLimiter::~RoughnessLimiter() { shader.version_free(shader_version); }

/* SortEffects */

SortEffects::~SortEffects() { shader.version_free(shader_version); }

/* MotionVectorsStore */

MotionVectorsStore::MotionVectorsStore()
{
	Vector<String> modes;
	modes.push_back("");

	motion_shader.initialize(modes);
	shader_version = motion_shader.version_create();
}

MotionVectorsStore::~MotionVectorsStore() { motion_shader.version_free(shader_version); }

// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/post_effects.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h"
#include "servers/rendering/renderer_rd/effects/smaa_search_tex.gen.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

using namespace RendererRD;

/* BokehDOF */

BokehDOF::~BokehDOF()
{
	for (int i = 0; i < BOKEH_MAX; i++) {
		bokeh.compute_pipelines[i].free();
	}

	if (prefer_raster_effects) {
		bokeh.raster_shader.version_free(bokeh.shader_version);
	}
	else {
		bokeh.compute_shader.version_free(bokeh.shader_version);
	}
}

void RendererRD::BokehDOF::bokeh_dof_compute(const RendererRD::BokehDOF::BokehBuffers& p_buffers,
	RID p_dest_framebuffer, float p_bokeh_size, float p_blur_size, bool p_use_jitter)
{
}

BokehDOF::BokehDOF(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	Vector<String> modes;
	modes.push_back("\n#define MODE_GEN_BLUR_SIZE\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_CIRCULAR\n");
	modes.push_back("\n#define MODE_COMPOSITE\n");

	if (prefer_raster_effects) {
		bokeh.raster_shader.initialize(modes);
		bokeh.shader_version = bokeh.raster_shader.version_create();
	}
	else {
		bokeh.compute_shader.initialize(modes);
		bokeh.shader_version = bokeh.compute_shader.version_create();
	}
}

/* ToneMapper */

ToneMapper::~ToneMapper()
{
	if (using_mobile_version) {
		tonemap_mobile.shader.version_free(tonemap_mobile.shader_version);
	}
	else {
		tonemap.shader.version_free(tonemap.shader_version);
	}
}

void RendererRD::ToneMapper::tonemapper(
	RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings)
{
}

void RendererRD::ToneMapper::tonemapper_subpass(long p_subpass, RID p_framebuffer,
	long p_render_pass, const RendererRD::ToneMapper::TonemapSettings& p_settings)
{
}

ToneMapper::ToneMapper(bool p_use_mobile_version)
{
	using_mobile_version = p_use_mobile_version;

	if (using_mobile_version) {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define SUBPASS\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap_mobile.shader.initialize(modes);
		tonemap_mobile.shader_version = tonemap_mobile.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back(
			"\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap.shader.initialize(modes);
		tonemap.shader_version = tonemap.shader.version_create();
	}
}

/* Luminance */

Luminance::~Luminance()
{
	if (prefer_raster_effects) {
		luminance_reduce_raster.shader.version_free(luminance_reduce_raster.shader_version);
	}
	else {
		luminance_reduce.shader.version_free(luminance_reduce.shader_version);
	}
}

void Luminance::LuminanceBuffers::set_prefer_raster_effects(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
}

void Luminance::LuminanceBuffers::configure(RenderSceneBuffersRD* p_render_buffers)
{
	Size2i internal_size = p_render_buffers->get_internal_size();
	int w = internal_size.x;
	int h = internal_size.y;

	while (true) {
		w = MAX(w / 8, 1);
		h = MAX(h / 8, 1);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R32_SFLOAT;
		tf.width = w;
		tf.height = h;

		bool final = w == 1 && h == 1;

		if (prefer_raster_effects) {
			tf.usage_bits =
				RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
		}
		else {
			tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT;
		}

		if (final) {
			tf.usage_bits |= RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
		}

		RID texture = RD::texture_create(tf, RDD::TextureView());
		reduce.push_back(texture);

		if (final) {
			current = RD::texture_create(tf, RDD::TextureView());
			RD::texture_clear(current, Color(0.0, 0.0, 0.0), 0u, 1u, 0u, 1u);
			break;
		}
	}
}

void Luminance::LuminanceBuffers::free_data()
{
	for (int i = 0; i < reduce.size(); i++) {
		RD::free_rid(reduce[i]);
	}
	reduce.clear();

	if (current.is_valid()) {
		RD::free_rid(current);
		current = RID();
	}
}

Ref<Luminance::LuminanceBuffers> Luminance::get_luminance_buffers(
	Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		return p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
	}

	Ref<LuminanceBuffers> buffers;
	buffers.instantiate();
	buffers->set_prefer_raster_effects(prefer_raster_effects);
	buffers->configure(p_render_buffers.ptr());

	p_render_buffers->set_custom_data(RB_LUMINANCE_BUFFERS, buffers);

	return buffers;
}

RID Luminance::get_current_luminance_buffer(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		Ref<LuminanceBuffers> buffers = p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
		return buffers->current;
	}

	return RID();
}

void RendererRD::Luminance::luminance_reduction(RID p_source_texture, Vector2i p_size,
	Ref<RendererRD::Luminance::LuminanceBuffers> p_buffers, float p_min_luminance,
	float p_max_luminance, float p_frame_time, bool p_immediate)
{
}

Luminance::Luminance(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
	if (prefer_raster_effects) {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce_raster.shader.initialize(modes);
		luminance_reduce_raster.shader_version = luminance_reduce_raster.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce.shader.initialize(modes);
		luminance_reduce.shader_version = luminance_reduce.shader.version_create();

		for (int i = 0; i < LUMINANCE_REDUCE_MAX; i++) {
			luminance_reduce.pipelines[i] = RD::compute_pipeline_create(
				luminance_reduce.shader.version_get_shader(luminance_reduce.shader_version, i));
		}
	}
}

/* SMAA */

SMAA::~SMAA()
{
	RD::free_rid(smaa.search_tex);
	RD::free_rid(smaa.area_tex);

	smaa.edge_shader.version_free(smaa.edge_shader_version);
	smaa.weight_shader.version_free(smaa.weight_shader_version);
	smaa.blend_shader.version_free(smaa.blend_shader_version);
}

void SMAA::allocate_render_targets(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	RSE::ViewportScaling3DType scaling_type =
		RSE::scaling_3d_mode_type(p_render_buffers->get_scaling_3d_mode());
	bool use_upscaled_texture = p_render_buffers->has_upscaled_texture() &&
								scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL;
	Size2i full_size = use_upscaled_texture ? p_render_buffers->get_target_size()
											: p_render_buffers->get_internal_size();

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here.

	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_EDGES, RDC::DATA_FORMAT_R8G8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_BLEND, RDC::DATA_FORMAT_R8G8B8A8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_STENCIL, smaa.stencil_format,
		RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RDC::TEXTURE_SAMPLES_1, full_size, 1, 1,
		true, true);
}

void RendererRD::SMAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_dest_framebuffer,
	RID p_source_color, bool p_use_srgb)
{
}

SMAA::SMAA()
{
	edge_detection_threshold = 0.1f;

	// 1. Shaders initialization
	Vector<String> edge_modes;
	edge_modes.push_back("\n#define COLOR_EDGE_DETECTION\n");
	edge_modes.push_back("\n#define LUMA_EDGE_DETECTION\n");

	smaa.edge_shader.initialize(edge_modes);
	smaa.edge_shader_version = smaa.edge_shader.version_create();

	Vector<String> empty_modes;
	empty_modes.push_back("\n");

	smaa.weight_shader.initialize(empty_modes);
	smaa.weight_shader_version = smaa.weight_shader.version_create();

	Vector<String> blend_modes;
	blend_modes.push_back("\n");
	blend_modes.push_back("\n#define USE_8_BIT_DEBANDING\n");
	blend_modes.push_back("\n#define USE_10_BIT_DEBANDING\n");

	smaa.blend_shader.initialize(blend_modes);
	smaa.blend_shader_version = smaa.blend_shader.version_create();

	// 2. Load precomputed search texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(search_tex_png));
		memcpy(png_data.ptrw(), search_tex_png, sizeof(search_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.search_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}

	// 3. Load precomputed area texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(area_tex_png));
		memcpy(png_data.ptrw(), area_tex_png, sizeof(area_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8G8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.area_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}
}

/* DebugEffects */

void DebugEffects::_create_frustum_arrays()
{
	if (frustum.vertex_buffer.is_null()) {
		// Create vertex buffer, but don't put data in it yet
		frustum.vertex_buffer = RD::vertex_buffer_create(8 * sizeof(float) * 3, Vector<uint8_t>());

		Vector<RDC::VertexAttribute> attributes;
		Vector<RID> buffers;
		RDC::VertexAttribute vd;

		vd.location = 0;
		vd.stride = sizeof(float) * 3;
		vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;

		attributes.push_back(vd);
		buffers.push_back(frustum.vertex_buffer);

		frustum.vertex_format = RD::vertex_format_create(attributes);
		frustum.vertex_array = RD::vertex_array_create(8, frustum.vertex_format, buffers);
	}

	if (frustum.index_buffer.is_null()) {
		uint16_t indices[6 * 2 * 3] = {
			// Far
			0, 1, 2, // FLT, FLB, FRT
			1, 3, 2, // FLB, FRB, FRT
			// Near
			4, 6, 5, // NLT, NRT, NLB
			6, 7, 5, // NRT, NRB, NLB
			// Left
			0, 4, 1, // FLT, NLT, FLB
			4, 5, 1, // NLT, NLB, FLB
			// Right
			6, 2, 7, // NRT, FRT, NRB
			2, 3, 7, // FRT, FRB, NRB
			// Top
			0, 2, 4, // FLT, FRT, NLT
			2, 6, 4, // FRT, NRT, NLT
			// Bottom
			5, 7, 1, // NLB, NRB, FLB,
			7, 3, 1	 // NRB, FRB, FLB
		};

		// Create our index_array
		PackedByteArray data;
		data.resize(6 * 2 * 3 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 6 * 2 * 3; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.index_buffer =
			RD::index_buffer_create(6 * 2 * 3, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.index_array = RD::index_array_create(frustum.index_buffer, 0, 6 * 2 * 3);
	}

	if (frustum.lines_buffer.is_null()) {
		uint16_t indices[12 * 2] = {
			0, 1, // FLT - FLB
			1, 3, // FLB - FRB
			3, 2, // FRB - FRT
			2, 0, // FRT - FLT

			4, 6, // NLT - NRT
			6, 7, // NRT - NRB
			7, 5, // NRB - NLB
			5, 4, // NLB - NLT

			0, 4, // FLT - NLT
			1, 5, // FLB - NLB
			2, 6, // FRT - NRT
			3, 7  // FRB - NRB
		};

		// Create our lines_array
		PackedByteArray data;
		data.resize(12 * 2 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 12 * 2; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.lines_buffer =
			RD::index_buffer_create(12 * 2, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.lines_array = RD::index_array_create(frustum.lines_buffer, 0, 12 * 2);
	}
}

DebugEffects::DebugEffects()
{
	// 1. Initialize frustum geometry buffers
	_create_frustum_arrays();

	// 2. Shadow frustum shader & version
	Vector<String> shadow_frustum_modes;
	shadow_frustum_modes.push_back("\n#define MODE_TRANSPARENT\n");
	shadow_frustum_modes.push_back("\n#define MODE_WIREFRAME\n");

	shadow_frustum.shader.initialize(shadow_frustum_modes);
	shadow_frustum.shader_version = shadow_frustum.shader.version_create();

	// 3. Motion vectors shader & version
	Vector<String> motion_vectors_modes;
	motion_vectors_modes.push_back("\n");

	motion_vectors.shader.initialize(motion_vectors_modes);
	motion_vectors.shader_version = motion_vectors.shader.version_create();
}

DebugEffects::~DebugEffects()
{
	shadow_frustum.shader.version_free(shadow_frustum.shader_version);

	// Destroy vertex buffer and array.
	if (frustum.vertex_buffer.is_valid()) {
		RD::free_rid(frustum.vertex_buffer); // Array gets freed as dependency.
	}

	// Destroy index buffer and array,
	if (frustum.index_buffer.is_valid()) {
		RD::free_rid(frustum.index_buffer); // Array gets freed as dependency.
	}

	// Destroy lines buffer and array.
	if (frustum.lines_buffer.is_valid()) {
		RD::free_rid(frustum.lines_buffer); // Array gets freed as dependency.
	}

	motion_vectors.shader.version_free(motion_vectors.shader_version);
}

void RendererRD::DebugEffects::draw_shadow_frustum(RID p_dest_framebuffer,
	const Projection& p_projection, const Transform3D& p_transform, RID p_depth, Rect2 p_rect)
{
}

/* VRS */

VRS::~VRS() { vrs_shader.shader.version_free(vrs_shader.shader_version); }

Size2i VRS::get_vrs_texture_size(const Size2i p_base_size) const
{
	Size2i vrs_texel_size = RD::vrs_get_texel_size();
	return Size2i((p_base_size.x + vrs_texel_size.x - 1) / vrs_texel_size.x,
		(p_base_size.y + vrs_texel_size.y - 1) / vrs_texel_size.y);
}

void VRS::update_vrs_texture(RID p_vrs_fb, RID p_render_target)
{
	RSE::ViewportVRSMode vrs_mode =
		RendererRD::TextureStorage::render_target_get_vrs_mode(p_render_target);
	RSE::ViewportVRSUpdateMode vrs_update_mode =
		RendererRD::TextureStorage::render_target_get_vrs_update_mode(p_render_target);

	if (vrs_mode != RSE::VIEWPORT_VRS_DISABLED &&
		vrs_update_mode != RSE::VIEWPORT_VRS_UPDATE_DISABLED) {
		RD::draw_command_begin_label("VRS Setup");

		if (vrs_mode == RSE::VIEWPORT_VRS_TEXTURE) {
			RID vrs_texture =
				RendererRD::TextureStorage::render_target_get_vrs_texture(p_render_target);
			if (vrs_texture.is_valid()) {
				RID rd_texture = RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
				int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);
				if (rd_texture.is_valid()) {
					// Copy into our density buffer
					copy_vrs(rd_texture, p_vrs_fb, layers > 1);
				}
			}
#ifndef XR_DISABLED
		}
		else if (vrs_mode == RSE::VIEWPORT_VRS_XR) {
			Ref<XRInterface> interface = XRServer::get_primary_interface();
			if (interface.is_valid() &&
				interface->get_vrs_texture_format() == XRInterface::XR_VRS_TEXTURE_FORMAT_UNIFIED) {
				RID vrs_texture = interface->get_vrs_texture();
				if (vrs_texture.is_valid()) {
					RID rd_texture =
						RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
					int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);

					if (rd_texture.is_valid()) {
						// Copy into our density buffer
						copy_vrs(rd_texture, p_vrs_fb, layers > 1);
					}
				}
			}
#endif // XR_DISABLED
		}

		if (vrs_update_mode == RSE::VIEWPORT_VRS_UPDATE_ONCE) {
			RendererRD::TextureStorage::render_target_set_vrs_update_mode(
				p_render_target, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
		}

		RD::draw_command_end_label();
	}
}

void RendererRD::VRS::copy_vrs(RID p_source, RID p_dest, bool p_is_mobile) {}

VRS::VRS()
{
	Vector<String> modes;
	modes.push_back("\n");
	vrs_shader.shader.initialize(modes);
	vrs_shader.shader_version = vrs_shader.shader.version_create();
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/ss_effects.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"

using namespace RendererRD;

static _FORCE_INLINE_ void store_camera(const Projection& p_mtx, float* p_array)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			p_array[i * 4 + j] = p_mtx.columns[i][j];
		}
	}
}

void SSEffects::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void SSEffects::finalize()
{
	if (!data) {
		return;
	}

	{
		// Cleanup SS Reflections
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX; i++) {
			data->ssr.downsample_pipelines[i].free();
		}
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_HIZ_MAX; i++) {
			data->ssr.hiz_pipelines[i].free();
		}
		data->ssr.ssr_pipeline.free();
		data->ssr.filter_pipeline.free();
		data->ssr.resolve_pipeline.free();

		if (data->ssr.downsample_shader_version.is_valid()) {
			data->ssr.downsample_shader.version_free(data->ssr.downsample_shader_version);
		}
		if (data->ssr.hiz_shader_version.is_valid()) {
			data->ssr.hiz_shader.version_free(data->ssr.hiz_shader_version);
		}
		if (data->ssr.ssr_shader_version.is_valid()) {
			data->ssr.ssr_shader.version_free(data->ssr.ssr_shader_version);
		}
		if (data->ssr.filter_shader_version.is_valid()) {
			data->ssr.filter_shader.version_free(data->ssr.filter_shader_version);
		}
		if (data->ssr.resolve_shader_version.is_valid()) {
			data->ssr.resolve_shader.version_free(data->ssr.resolve_shader_version);
		}

		if (data->ssr.ubo.is_valid()) {
			RD::free_rid(data->ssr.ubo);
		}
	}

	{
		// Cleanup SS downsampler
		for (int i = 0; i < SS_EFFECTS_MAX; i++) {
			data->ss_effects.pipelines[i].free();
		}

		if (data->ss_effects.downsample_shader_version.is_valid()) {
			data->ss_effects.downsample_shader.version_free(
				data->ss_effects.downsample_shader_version);
		}

		if (data->ss_effects.mirror_sampler.is_valid()) {
			RD::free_rid(data->ss_effects.mirror_sampler);
		}
		if (data->ss_effects.gather_constants_buffer.is_valid()) {
			RD::free_rid(data->ss_effects.gather_constants_buffer);
		}
	}

	{
		// Cleanup SSIL
		for (int i = 0; i < SSIL_MAX; i++) {
			data->ssil.pipelines[i].free();
		}

		if (data->ssil.blur_shader_version.is_valid()) {
			data->ssil.blur_shader.version_free(data->ssil.blur_shader_version);
		}
		if (data->ssil.gather_shader_version.is_valid()) {
			data->ssil.gather_shader.version_free(data->ssil.gather_shader_version);
		}
		if (data->ssil.interleave_shader_version.is_valid()) {
			data->ssil.interleave_shader.version_free(data->ssil.interleave_shader_version);
		}
		if (data->ssil.importance_map_shader_version.is_valid()) {
			data->ssil.importance_map_shader.version_free(data->ssil.importance_map_shader_version);
		}

		if (data->ssil.importance_map_load_counter.is_valid()) {
			RD::free_rid(data->ssil.importance_map_load_counter);
		}
		if (data->ssil.projection_uniform_buffer.is_valid()) {
			RD::free_rid(data->ssil.projection_uniform_buffer);
		}
	}

	{
		// Cleanup SSAO
		for (int i = 0; i < SSAO_MAX; i++) {
			data->ssao.pipelines[i].free();
		}

		if (data->ssao.blur_shader_version.is_valid()) {
			data->ssao.blur_shader.version_free(data->ssao.blur_shader_version);
		}
		if (data->ssao.gather_shader_version.is_valid()) {
			data->ssao.gather_shader.version_free(data->ssao.gather_shader_version);
		}
		if (data->ssao.interleave_shader_version.is_valid()) {
			data->ssao.interleave_shader.version_free(data->ssao.interleave_shader_version);
		}
		if (data->ssao.importance_map_shader_version.is_valid()) {
			data->ssao.importance_map_shader.version_free(data->ssao.importance_map_shader_version);
		}

		if (data->ssao.importance_map_load_counter.is_valid()) {
			RD::free_rid(data->ssao.importance_map_load_counter);
		}
	}

	{
		// Cleanup Subsurface scattering
		for (int i = 0; i < SUBSURFACE_SCATTERING_MODE_MAX; i++) {
			data->sss.pipelines[i].free();
		}

		if (data->sss.shader_version.is_valid()) {
			data->sss.shader.version_free(data->sss.shader_version);
		}
	}

	memdelete(data);
	data = nullptr;
}

void SSEffects::allocate_last_frame_buffer(
	Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr)
{
	ERR_FAIL_NULL(data);

	Size2i last_frame_size = p_render_buffers->get_internal_size();
	uint32_t mipmaps = 1;
	uint32_t view_count = p_render_buffers->get_view_count();

	if (!p_use_ssil && p_use_ssr && data->ssr_half_size) {
		last_frame_size /= 2;
	}

	if (p_use_ssil) {
		mipmaps = 6;
	}

	bool should_create = true;
	bool has_texture = p_render_buffers->has_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);

	if (has_texture) {
		RID last_frame_texture = p_render_buffers->get_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);
		RDC::TextureFormat texture_format = RD::texture_get_format(last_frame_texture);
		should_create = texture_format.width != (uint32_t)last_frame_size.width ||
						texture_format.height != (uint32_t)last_frame_size.height ||
						texture_format.mipmaps != mipmaps ||
						texture_format.array_layers != view_count;
	}

	if (should_create) {
		if (has_texture) {
			p_render_buffers->clear_context(RB_SCOPE_SSLF);
		}

		RID last_frame_texture = p_render_buffers->create_texture(RB_SCOPE_SSLF, RB_LAST_FRAME,
			RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
				RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT,
			RDC::TEXTURE_SAMPLES_1, last_frame_size, view_count, mipmaps);
		RD::texture_clear(last_frame_texture, Color(0, 0, 0, 0), 0, mipmaps, 0, view_count);
	}
}

void SSEffects::copy_internal_texture_to_last_frame(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	ERR_FAIL_NULL(data);

	uint32_t mipmaps = p_render_buffers->get_texture_format(RB_SCOPE_SSLF, RB_LAST_FRAME).mipmaps;
	for (uint32_t v = 0; v < p_render_buffers->get_view_count(); v++) {
		for (uint32_t m = 0; m < mipmaps; m++) {
			RID source;
			if (m == 0) {
				source = p_render_buffers->get_internal_texture(v);
			}
			else {
				source =
					p_render_buffers->get_texture_slice(RB_SCOPE_SSLF, RB_LAST_FRAME, v, m - 1);
			}

			RID dest = p_render_buffers->get_texture_slice(RB_SCOPE_SSLF, RB_LAST_FRAME, v, m);

			Size2i source_size = RD::texture_size(source);
			Size2i dest_size = RD::texture_size(dest);

			if (m == 0 && source_size == dest_size) {
				CopyEffects::copy_to_rect(source, dest, Rect2i(Vector2i(), source_size), false,
					false, false, false, false, true);
			}
			else {
				CopyEffects::make_mipmap(source, dest, dest_size);
			}
		}
	}
}

/* SSIL */

void SSEffects::ssil_set_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ERR_FAIL_NULL(data);

	data->ssil_quality = p_quality;
	data->ssil_half_size = p_half_size;
	data->ssil_adaptive_target = p_adaptive_target;
	data->ssil_blur_passes = p_blur_passes;
	data->ssil_fadeout_from = p_fadeout_from;
	data->ssil_fadeout_to = p_fadeout_to;
}

void SSEffects::ssil_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSILRenderBuffers& p_ssil_buffers, const SSILSettings& p_settings)
{
	ERR_FAIL_NULL(data);

	if (p_ssil_buffers.half_size != data->ssil_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSIL);
	}

	p_ssil_buffers.half_size = data->ssil_half_size;
	if (p_ssil_buffers.half_size) {
		p_ssil_buffers.buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssil_buffers.buffer_height = (p_settings.full_screen_size.y + 3) / 4;
		p_ssil_buffers.half_buffer_width = (p_settings.full_screen_size.x + 7) / 8;
		p_ssil_buffers.half_buffer_height = (p_settings.full_screen_size.y + 7) / 8;
	}
	else {
		p_ssil_buffers.buffer_width = (p_settings.full_screen_size.x + 1) / 2;
		p_ssil_buffers.buffer_height = (p_settings.full_screen_size.y + 1) / 2;
		p_ssil_buffers.half_buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssil_buffers.half_buffer_height = (p_settings.full_screen_size.y + 3) / 4;
	}

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i full_size = Size2i(p_ssil_buffers.buffer_width, p_ssil_buffers.buffer_height);
	Size2i half_size = Size2i(p_ssil_buffers.half_buffer_width, p_ssil_buffers.half_buffer_height);

	// We create our intermediate and final results as render buffers.
	// These are automatically cached and cleaned up when our viewport resizes
	// or when our viewport gets destroyed.

	if (!p_render_buffers->has_texture(
			RB_SCOPE_SSIL, RB_FINAL)) { // We don't strictly have to check if it exists but we only
										// want to clear it when we create it...
		RID final = p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_FINAL,
			RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
				RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT);
		RD::texture_clear(final, Color(0, 0, 0, 0), 0, 1, 0, view_count);
	}

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here

	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED,
		RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED_PONG,
		RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_EDGES, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_MAP, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_PONG, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
}

void SSEffects::ssao_set_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ERR_FAIL_NULL(data);

	data->ssao_quality = p_quality;
	data->ssao_half_size = p_half_size;
	data->ssao_adaptive_target = p_adaptive_target;
	data->ssao_blur_passes = p_blur_passes;
	data->ssao_fadeout_from = p_fadeout_from;
	data->ssao_fadeout_to = p_fadeout_to;
}

void SSEffects::ssao_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSAORenderBuffers& p_ssao_buffers, const SSAOSettings& p_settings)
{
	ERR_FAIL_NULL(data);

	if (p_ssao_buffers.half_size != data->ssao_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSAO);
	}

	p_ssao_buffers.half_size = data->ssao_half_size;
	if (data->ssao_half_size) {
		p_ssao_buffers.buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssao_buffers.buffer_height = (p_settings.full_screen_size.y + 3) / 4;
		p_ssao_buffers.half_buffer_width = (p_settings.full_screen_size.x + 7) / 8;
		p_ssao_buffers.half_buffer_height = (p_settings.full_screen_size.y + 7) / 8;
	}
	else {
		p_ssao_buffers.buffer_width = (p_settings.full_screen_size.x + 1) / 2;
		p_ssao_buffers.buffer_height = (p_settings.full_screen_size.y + 1) / 2;
		p_ssao_buffers.half_buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssao_buffers.half_buffer_height = (p_settings.full_screen_size.y + 3) / 4;
	}

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i full_size = Size2i(p_ssao_buffers.buffer_width, p_ssao_buffers.buffer_height);
	Size2i half_size = Size2i(p_ssao_buffers.half_buffer_width, p_ssao_buffers.half_buffer_height);

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here

	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED, RDC::DATA_FORMAT_R8G8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED_PONG,
		RDC::DATA_FORMAT_R8G8_UNORM, RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_MAP, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_PONG, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_FINAL, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1);
}

void SSEffects::ssr_set_half_size(bool p_half_size)
{
	ERR_FAIL_NULL(data);
	data->ssr_half_size = p_half_size;
}

void SSEffects::ssr_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSRRenderBuffers& p_ssr_buffers, const RDC::DataFormat p_color_format)
{
	ERR_FAIL_NULL(data);

	if (p_ssr_buffers.half_size != data->ssr_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSR);
	}

	Vector2i internal_size = p_render_buffers->get_internal_size();
	p_ssr_buffers.size = data->ssr_half_size ? (internal_size / 2) : internal_size;

	uint32_t cur_width = p_ssr_buffers.size.width;
	uint32_t cur_height = p_ssr_buffers.size.height;
	p_ssr_buffers.mipmaps = 1;

	while (cur_width > 1 && cur_height > 1) {
		if (cur_width > 1) {
			cur_width /= 2;
		}
		if (cur_height > 1) {
			cur_height /= 2;
		}
		++p_ssr_buffers.mipmaps;
	}

	p_ssr_buffers.half_size = data->ssr_half_size;

	uint32_t view_count = p_render_buffers->get_view_count();

	if (data->ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_NORMAL_ROUGHNESS,
			RDC::DATA_FORMAT_R8G8B8A8_UNORM,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
			p_ssr_buffers.size, view_count);
	}

	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_HIZ, RDC::DATA_FORMAT_R32_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_SSR, p_color_format,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_MIP_LEVEL, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count);

	if (data->ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_FINAL, p_color_format,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
			internal_size, view_count);
	}
}

void SSEffects::sss_set_quality(RSE::SubSurfaceScatteringQuality p_quality)
{
	ERR_FAIL_NULL(data);
	data->sss_quality = p_quality;
}

RSE::SubSurfaceScatteringQuality SSEffects::sss_get_quality()
{
	ERR_FAIL_NULL_V(data, RSE::SUB_SURFACE_SCATTERING_QUALITY_DISABLED);
	return data->sss_quality;
}

void SSEffects::sss_set_scale(float p_scale, float p_depth_scale)
{
	ERR_FAIL_NULL(data);
	data->sss_scale = p_scale;
	data->sss_depth_scale = p_depth_scale;
}

void SSEffects::sub_surface_scattering(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_diffuse,
	RID p_depth, const Projection& p_camera, const Size2i& p_screen_size)
{
}

void SSEffects::screen_space_reflection(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSRRenderBuffers& p_ssr_buffers, const RID* p_normal_roughness_slices, int p_max_steps,
	float p_fade_in, float p_fade_out, float p_tolerance, const Projection* p_projections,
	const Projection* p_reprojections, const Vector3* p_eye_offsets)
{
}

void SSEffects::screen_space_indirect_lighting(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSILRenderBuffers& p_ssil_buffers, uint32_t p_view, RID p_normal_buffer,
	const Projection& p_projection, const Projection& p_last_projection,
	const SSILSettings& p_settings)
{
}

void SSEffects::generate_ssao(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSAORenderBuffers& p_ssao_buffers, uint32_t p_view, RID p_normal_buffer,
	const Projection& p_projection, const SSAOSettings& p_settings)
{
}

void SSEffects::downsample_depth(
	Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_view, const Projection& p_projection)
{
}

void SSEffects::gather_ssil(RD::ComputeListID p_compute_list, const RID* p_ssil_slices,
	const RID* p_edges_slices, const SSILSettings& p_settings, bool p_adaptive_base_pass,
	RID p_gather_uniform_set, RID p_importance_map_uniform_set, RID p_projection_uniform_set)
{
}

void SSEffects::gather_ssao(RD::ComputeListID p_compute_list, const RID* p_ao_slices,
	const SSAOSettings& p_settings, bool p_adaptive_base_pass, RID p_gather_uniform_set,
	RID p_importance_map_uniform_set)
{
}



// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/upscale_effects.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/rendering_device.h"

#ifdef METAL_ENABLED
#include "drivers/metal/rendering_device_driver_metal3.h"
#include "drivers/metal/rendering_device_driver_metal3.h"

#include <MetalFX/MetalFX.hpp>
#endif

#ifndef _MSC_VER
#include <cwchar>
#define wcscpy_s wcscpy
#endif

using namespace RendererRD;

/* FSR */

FSR::~FSR()
{
	pipeline.free();
	fsr_shader.version_free(shader_version);
}

void RendererRD::FSR::ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) {}

void RendererRD::FSR::process(
	Ref<RenderSceneBuffersRD> p_buffers, RID p_color_image, RID p_dst_image)
{
}

FSR::FSR()
{
	Vector<String> modes;
	modes.push_back("\n");
	modes.push_back("\n#define SAMPLE_SLOW_FALLBACK\n");

	fsr_shader.initialize(modes);
	shader_version = fsr_shader.version_create();
}

/* FSR2 */

static RDC::TextureType ffx_resource_type_to_rd_texture_type(FfxResourceType p_type)
{
	switch (p_type) {
	case FFX_RESOURCE_TYPE_TEXTURE1D:
		return RDC::TEXTURE_TYPE_1D;
	case FFX_RESOURCE_TYPE_TEXTURE2D:
		return RDC::TEXTURE_TYPE_2D;
	case FFX_RESOURCE_TYPE_TEXTURE3D:
		return RDC::TEXTURE_TYPE_3D;
	default:
		return RDC::TEXTURE_TYPE_MAX;
	}
}

static FfxResourceType rd_texture_type_to_ffx_resource_type(RDC::TextureType p_type)
{
	switch (p_type) {
	case RDC::TEXTURE_TYPE_1D:
		return FFX_RESOURCE_TYPE_TEXTURE1D;
	case RDC::TEXTURE_TYPE_2D:
		return FFX_RESOURCE_TYPE_TEXTURE2D;
	case RDC::TEXTURE_TYPE_3D:
		return FFX_RESOURCE_TYPE_TEXTURE3D;
	default:
		return FFX_RESOURCE_TYPE_BUFFER;
	}
}

static RDC::DataFormat ffx_surface_format_to_rd_format(FfxSurfaceFormat p_format)
{
	switch (p_format) {
	case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
		return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16B16A16_UNORM:
		return RDC::DATA_FORMAT_R16G16B16A16_UNORM;
	case FFX_SURFACE_FORMAT_R32G32_FLOAT:
		return RDC::DATA_FORMAT_R32G32_SFLOAT;
	case FFX_SURFACE_FORMAT_R32_UINT:
		return RDC::DATA_FORMAT_R32_UINT;
	case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
		return RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32;
	case FFX_SURFACE_FORMAT_R16G16_FLOAT:
		return RDC::DATA_FORMAT_R16G16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16_UINT:
		return RDC::DATA_FORMAT_R16G16_UINT;
	case FFX_SURFACE_FORMAT_R16_FLOAT:
		return RDC::DATA_FORMAT_R16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16_UINT:
		return RDC::DATA_FORMAT_R16_UINT;
	case FFX_SURFACE_FORMAT_R16_UNORM:
		return RDC::DATA_FORMAT_R16_UNORM;
	case FFX_SURFACE_FORMAT_R16_SNORM:
		return RDC::DATA_FORMAT_R16_SNORM;
	case FFX_SURFACE_FORMAT_R8_UNORM:
		return RDC::DATA_FORMAT_R8_UNORM;
	case FFX_SURFACE_FORMAT_R8_UINT:
		return RDC::DATA_FORMAT_R8_UINT;
	case FFX_SURFACE_FORMAT_R8G8_UNORM:
		return RDC::DATA_FORMAT_R8G8_UNORM;
	case FFX_SURFACE_FORMAT_R32_FLOAT:
		return RDC::DATA_FORMAT_R32_SFLOAT;
	default:
		return RDC::DATA_FORMAT_MAX;
	}
}

static FfxSurfaceFormat rd_format_to_ffx_surface_format(RDC::DataFormat p_format)
{
	switch (p_format) {
	case RDC::DATA_FORMAT_R32G32B32A32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
	case RDC::DATA_FORMAT_R16G16B16A16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
	case RDC::DATA_FORMAT_R16G16B16A16_UNORM:
		return FFX_SURFACE_FORMAT_R16G16B16A16_UNORM;
	case RDC::DATA_FORMAT_R32G32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32G32_FLOAT;
	case RDC::DATA_FORMAT_R32_UINT:
		return FFX_SURFACE_FORMAT_R32_UINT;
	case RDC::DATA_FORMAT_R8G8B8A8_UNORM:
		return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
	case RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32:
		return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
	case RDC::DATA_FORMAT_R16G16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16G16_FLOAT;
	case RDC::DATA_FORMAT_R16G16_UINT:
		return FFX_SURFACE_FORMAT_R16G16_UINT;
	case RDC::DATA_FORMAT_R16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16_FLOAT;
	case RDC::DATA_FORMAT_R16_UINT:
		return FFX_SURFACE_FORMAT_R16_UINT;
	case RDC::DATA_FORMAT_R16_UNORM:
		return FFX_SURFACE_FORMAT_R16_UNORM;
	case RDC::DATA_FORMAT_R16_SNORM:
		return FFX_SURFACE_FORMAT_R16_SNORM;
	case RDC::DATA_FORMAT_R8_UNORM:
		return FFX_SURFACE_FORMAT_R8_UNORM;
	case RDC::DATA_FORMAT_R8_UINT:
		return FFX_SURFACE_FORMAT_R8_UINT;
	case RDC::DATA_FORMAT_R8G8_UNORM:
		return FFX_SURFACE_FORMAT_R8G8_UNORM;
	case RDC::DATA_FORMAT_R32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32_FLOAT;
	default:
		return FFX_SURFACE_FORMAT_UNKNOWN;
	}
}

static uint32_t ffx_usage_to_rd_usage_flags(uint32_t p_flags)
{
	uint32_t ret = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

	if (p_flags & FFX_RESOURCE_USAGE_RENDERTARGET) {
		ret |= RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (p_flags & FFX_RESOURCE_USAGE_UAV) {
		ret |= RDC::TEXTURE_USAGE_STORAGE_BIT;
		ret |= RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		ret |= RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
	}

	return ret;
}

static FfxErrorCode get_device_capabilities_rd(FfxFsr2Interface* p_backend_interface,
	FfxDeviceCapabilities* p_out_device_capabilities, FfxDevice p_device)
{
	FSR2Effect::Device& effect_device = *reinterpret_cast<FSR2Effect::Device*>(p_device);

	*p_out_device_capabilities = effect_device.capabilities;

	return FFX_OK;
}

static FfxErrorCode destroy_backend_context_rd(FfxFsr2Interface* p_backend_interface)
{
	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);

	for (uint32_t i = 0; i < FSR2_UBO_RING_BUFFER_SIZE; i++) {
		RD::free_rid(scratch.ubo_ring_buffer[i]);
	}

	return FFX_OK;
}

static FfxErrorCode create_resource_rd(FfxFsr2Interface* p_backend_interface,
	const FfxCreateResourceDescription* p_create_resource_description,
	FfxResourceInternal* p_out_resource)
{
	// FSR2's base implementation won't issue a call to create a heap type that isn't just default
	// on its own, so we can safely ignore it as RD does not expose this concept.
	ERR_FAIL_COND_V(p_create_resource_description->heapType != FFX_HEAP_TYPE_DEFAULT,
		FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	FfxResourceDescription res_desc = p_create_resource_description->resourceDescription;

	// FSR2's base implementation never requests buffer creation.
	ERR_FAIL_COND_V(res_desc.type != FFX_RESOURCE_TYPE_TEXTURE1D &&
						res_desc.type != FFX_RESOURCE_TYPE_TEXTURE2D &&
						res_desc.type != FFX_RESOURCE_TYPE_TEXTURE3D,
		FFX_ERROR_INVALID_ARGUMENT);

	if (res_desc.mipCount == 0) {
		// Mipmap count must be derived from the resource's dimensions.
		res_desc.mipCount = uint32_t(
			1 + std::floor(std::log2(MAX(MAX(res_desc.width, res_desc.height), res_desc.depth))));
	}

	Vector<PackedByteArray> initial_data;
	if (p_create_resource_description->initDataSize) {
		PackedByteArray byte_array;
		byte_array.resize(p_create_resource_description->initDataSize);
		memcpy(byte_array.ptrw(), p_create_resource_description->initData,
			p_create_resource_description->initDataSize);
		initial_data.push_back(byte_array);
	}

	RDC::TextureFormat texture_format;
	texture_format.texture_type = ffx_resource_type_to_rd_texture_type(res_desc.type);
	texture_format.format = ffx_surface_format_to_rd_format(res_desc.format);
	texture_format.usage_bits = ffx_usage_to_rd_usage_flags(p_create_resource_description->usage);
	texture_format.width = res_desc.width;
	texture_format.height = res_desc.height;
	texture_format.depth = res_desc.depth;
	texture_format.mipmaps = res_desc.mipCount;
	texture_format.is_discardable = true;

	RID texture = RD::texture_create(texture_format, RDD::TextureView(), initial_data);
	ERR_FAIL_COND_V(texture.is_null(), FFX_ERROR_BACKEND_API_ERROR);

	RD::set_resource_name(texture, String(p_create_resource_description->name));

	// Add the resource to the storage and use the internal index to reference it.
	p_out_resource->internalIndex =
		scratch.resources.add(texture, false, p_create_resource_description->id, res_desc);

	return FFX_OK;
}

static FfxErrorCode register_resource_rd(FfxFsr2Interface* p_backend_interface,
	const FfxResource* p_in_resource, FfxResourceInternal* p_out_resource)
{
	if (p_in_resource->resource == nullptr) {
		// Null resource case.
		p_out_resource->internalIndex = -1;
		return FFX_OK;
	}

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	const RID& rid = *reinterpret_cast<const RID*>(p_in_resource->resource);
	ERR_FAIL_COND_V(rid.is_null(), FFX_ERROR_INVALID_ARGUMENT);

	// Add the resource to the storage and use the internal index to reference it.
	p_out_resource->internalIndex = scratch.resources.add(
		rid, true, FSR2Context::RESOURCE_ID_DYNAMIC, p_in_resource->description);

	return FFX_OK;
}

static FfxErrorCode unregister_resources_rd(FfxFsr2Interface* p_backend_interface)
{
	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	LocalVector<uint32_t> dynamic_list_copy(scratch.resources.dynamic_list);
	for (uint32_t i : dynamic_list_copy) {
		scratch.resources.remove(i);
	}

	return FFX_OK;
}

static FfxResourceDescription get_resource_description_rd(
	FfxFsr2Interface* p_backend_interface, FfxResourceInternal p_resource)
{
	if (p_resource.internalIndex != -1) {
		FSR2Context::Scratch& scratch =
			*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
		return scratch.resources.descriptions[p_resource.internalIndex];
	}
	else {
		return {};
	}
}

static FfxErrorCode destroy_resource_rd(
	FfxFsr2Interface* p_backend_interface, FfxResourceInternal p_resource)
{
	if (p_resource.internalIndex != -1) {
		FSR2Context::Scratch& scratch =
			*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
		if (scratch.resources.rids[p_resource.internalIndex].is_valid()) {
			RD::free_rid(scratch.resources.rids[p_resource.internalIndex]);
			scratch.resources.remove(p_resource.internalIndex);
		}
	}

	return FFX_OK;
}

static FfxErrorCode destroy_pipeline_rd(
	FfxFsr2Interface* p_backend_interface, FfxPipelineState* p_pipeline)
{
	// We don't want to destroy pipelines when the FSR2 API deems it necessary as it'll do so
	// whenever the context is destroyed.

	return FFX_OK;
}

static FfxErrorCode schedule_gpu_job_rd(
	FfxFsr2Interface* p_backend_interface, const FfxGpuJobDescription* p_job)
{
	ERR_FAIL_NULL_V(p_backend_interface, FFX_ERROR_INVALID_ARGUMENT);
	ERR_FAIL_NULL_V(p_job, FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	scratch.gpu_jobs.push_back(*p_job);

	return FFX_OK;
}

static FfxErrorCode execute_gpu_job_clear_float_rd(
	FSR2Context::Scratch& p_scratch, const FfxClearFloatJobDescription& p_job)
{
	RID resource = p_scratch.resources.rids[p_job.target.internalIndex];
	FfxResourceDescription& desc = p_scratch.resources.descriptions[p_job.target.internalIndex];

	ERR_FAIL_COND_V(desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);

	Color color(p_job.color[0], p_job.color[1], p_job.color[2], p_job.color[3]);
	RD::texture_clear(resource, color, 0, desc.mipCount, 0, 1);

	return FFX_OK;
}

static FfxErrorCode execute_gpu_job_copy_rd(
	FSR2Context::Scratch& p_scratch, const FfxCopyJobDescription& p_job)
{
	RID src = p_scratch.resources.rids[p_job.src.internalIndex];
	RID dst = p_scratch.resources.rids[p_job.dst.internalIndex];
	FfxResourceDescription& src_desc = p_scratch.resources.descriptions[p_job.src.internalIndex];
	FfxResourceDescription& dst_desc = p_scratch.resources.descriptions[p_job.dst.internalIndex];

	ERR_FAIL_COND_V(src_desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);
	ERR_FAIL_COND_V(dst_desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);

	for (uint32_t mip_level = 0; mip_level < src_desc.mipCount; mip_level++) {
		RD::texture_copy(src, dst, Vector3(0, 0, 0), Vector3(0, 0, 0),
			Vector3(src_desc.width, src_desc.height, src_desc.depth), mip_level, mip_level, 0, 0);
	}

	return FFX_OK;
}

static FfxErrorCode execute_gpu_jobs_rd(
	FfxFsr2Interface* p_backend_interface, FfxCommandList p_command_list)
{
	ERR_FAIL_NULL_V(p_backend_interface, FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	FfxErrorCode error_code = FFX_OK;
	for (const FfxGpuJobDescription& job : scratch.gpu_jobs) {
		switch (job.jobType) {
		case FFX_GPU_JOB_CLEAR_FLOAT: {
			error_code = execute_gpu_job_clear_float_rd(scratch, job.clearJobDescriptor);
		} break;
		case FFX_GPU_JOB_COPY: {
			error_code = execute_gpu_job_copy_rd(scratch, job.copyJobDescriptor);
		} break;
		default: {
			error_code = FFX_ERROR_INVALID_ARGUMENT;
		} break;
		}
		if (error_code != FFX_OK) {
			scratch.gpu_jobs.clear();
			return error_code;
		}
	}
	scratch.gpu_jobs.clear();
	return FFX_OK;
}

static FfxResource get_resource_rd(RID* p_rid, const wchar_t* p_name)
{
	FfxResource res = {};
	if (p_rid->is_null()) {
		return res;
	}

	wcscpy_s(res.name, p_name);

	RDC::TextureFormat texture_format = RD::texture_get_format(*p_rid);
	res.description.type = rd_texture_type_to_ffx_resource_type(texture_format.texture_type);
	res.description.format = rd_format_to_ffx_surface_format(texture_format.format);
	res.description.width = texture_format.width;
	res.description.height = texture_format.height;
	res.description.depth = texture_format.depth;
	res.description.mipCount = texture_format.mipmaps;
	res.description.flags = FFX_RESOURCE_FLAGS_NONE;
	res.resource = reinterpret_cast<void*>(p_rid);
	res.isDepth = texture_format.usage_bits & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	return res;
}

FSR2Context::~FSR2Context() { ffxFsr2ContextDestroy(&fsr_context); }

FSR2Effect::FSR2Effect()
{
	FfxDeviceCapabilities& capabilities = device.capabilities;
	capabilities.minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
	capabilities.waveLaneCountMin = 32;
	capabilities.waveLaneCountMax = 32;
	capabilities.fp16Supported = RD::has_feature(RDC::Features::SUPPORTS_HALF_FLOAT);
	capabilities.raytracingSupported = false;

	String general_defines = "\n#define FFX_GPU\n"
							 "\n#define FFX_GLSL 1\n"
							 "\n#define FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS 1\n"
							 "\n#define FFX_FSR2_OPTION_HDR_COLOR_INPUT 1\n"
							 "\n#define FFX_FSR2_OPTION_INVERTED_DEPTH 1\n"
							 "\n#define FFX_FSR2_OPTION_GODOT_REACTIVE_MASK_CLAMP 1\n"
							 "\n#define FFX_FSR2_OPTION_GODOT_DERIVE_INVALID_MOTION_VECTORS 1\n";

	Vector<String> modes_single;
	modes_single.push_back("");

	Vector<String> modes_with_fp16;
	modes_with_fp16.push_back("");
	modes_with_fp16.push_back("\n#define FFX_HALF 1\n");

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_DEPTH_CLIP];
		pass.shader = &shaders.depth_clip;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {
			FfxResourceBinding{0, 0, L"r_reconstructed_previous_nearest_depth"},
			FfxResourceBinding{1, 0, L"r_dilated_motion_vectors"},
			FfxResourceBinding{2, 0, L"r_dilatedDepth"},
			FfxResourceBinding{3, 0, L"r_reactive_mask"},
			FfxResourceBinding{4, 0, L"r_transparency_and_composition_mask"},
			FfxResourceBinding{6, 0, L"r_previous_dilated_motion_vectors"},
			FfxResourceBinding{7, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{8, 0, L"r_input_color_jittered"},
			FfxResourceBinding{9, 0, L"r_input_depth"},
			FfxResourceBinding{10, 0, L"r_input_exposure"}};

		pass.storage_bindings = {// FSR2_BIND_UAV_DEPTH_CLIP (11) does not point to anything.
			FfxResourceBinding{12, 0, L"rw_dilated_reactive_masks"},
			FfxResourceBinding{13, 0, L"rw_prepared_input_color"}};

		pass.uniform_bindings = {FfxResourceBinding{14, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH];
		pass.shader = &shaders.reconstruct_previous_depth;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{1, 0, L"r_input_depth"},
			FfxResourceBinding{2, 0, L"r_input_color_jittered"},
			FfxResourceBinding{3, 0, L"r_input_exposure"},
			FfxResourceBinding{4, 0, L"r_luma_history"}};

		pass.storage_bindings = {
			FfxResourceBinding{5, 0, L"rw_reconstructed_previous_nearest_depth"},
			FfxResourceBinding{6, 0, L"rw_dilated_motion_vectors"},
			FfxResourceBinding{7, 0, L"rw_dilatedDepth"},
			FfxResourceBinding{8, 0, L"rw_prepared_input_color"},
			FfxResourceBinding{9, 0, L"rw_luma_history"},
			// FSR2_BIND_UAV_LUMA_INSTABILITY (10) does not point to anything.
			FfxResourceBinding{11, 0, L"rw_lock_input_luma"}};

		pass.uniform_bindings = {FfxResourceBinding{12, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_LOCK];
		pass.shader = &shaders.lock;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_lock_input_luma"}};

		pass.storage_bindings = {FfxResourceBinding{1, 0, L"rw_new_locks"},
			FfxResourceBinding{2, 0, L"rw_reconstructed_previous_nearest_depth"}};

		pass.uniform_bindings = {FfxResourceBinding{3, 0, L"cbFSR2"}};
	}

	{
		Vector<String> accumulate_modes_with_fp16;
		accumulate_modes_with_fp16.push_back("\n");
		accumulate_modes_with_fp16.push_back("\n#define FFX_FSR2_OPTION_APPLY_SHARPENING 1\n");
		accumulate_modes_with_fp16.push_back("\n#define FFX_HALF 1\n");
		accumulate_modes_with_fp16.push_back(
			"\n#define FFX_HALF 1\n#define FFX_FSR2_OPTION_APPLY_SHARPENING 1\n");

		// Workaround: Disable FP16 path for the accumulate pass on NVIDIA due to reduced occupancy
		// and high VRAM throughput.
		const bool fp16_path_supported = RD::get_device_vendor_name() != "NVIDIA";
		Pass& pass = device.passes[FFX_FSR2_PASS_ACCUMULATE];
		pass.shader = &shaders.accumulate;
		pass.shader->initialize(accumulate_modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported && fp16_path_supported ? 2 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_exposure"},
			FfxResourceBinding{1, 0, L"r_dilated_reactive_masks"},
			FfxResourceBinding{2, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{3, 0, L"r_internal_upscaled_color"},
			FfxResourceBinding{4, 0, L"r_lock_status"}, FfxResourceBinding{5, 0, L"r_input_depth"},
			FfxResourceBinding{6, 0, L"r_prepared_input_color"},
			// FSR2_BIND_SRV_LUMA_INSTABILITY(7) does not point to anything.
			FfxResourceBinding{8, 0, L"r_lanczos_lut"},
			FfxResourceBinding{9, 0, L"r_upsample_maximum_bias_lut"},
			FfxResourceBinding{10, 0, L"r_imgMips"}, FfxResourceBinding{11, 0, L"r_auto_exposure"},
			FfxResourceBinding{12, 0, L"r_luma_history"}};

		pass.storage_bindings = {FfxResourceBinding{13, 0, L"rw_internal_upscaled_color"},
			FfxResourceBinding{14, 0, L"rw_lock_status"},
			FfxResourceBinding{15, 0, L"rw_upscaled_output"},
			FfxResourceBinding{16, 0, L"rw_new_locks"},
			FfxResourceBinding{17, 0, L"rw_luma_history"}};

		pass.uniform_bindings = {FfxResourceBinding{18, 0, L"cbFSR2"}};

		// Sharpen pass is a clone of the accumulate pass with the sharpening variant.
		Pass& sharpen_pass = device.passes[FFX_FSR2_PASS_ACCUMULATE_SHARPEN];
		sharpen_pass = pass;
		sharpen_pass.shader_variant = pass.shader_variant + 1;
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_RCAS];
		pass.shader = &shaders.rcas;
		pass.shader->initialize(modes_single, general_defines);
		pass.shader_version = pass.shader->version_create();

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_exposure"},
			FfxResourceBinding{1, 0, L"r_rcas_input"}};

		pass.storage_bindings = {FfxResourceBinding{2, 0, L"rw_upscaled_output"}};

		pass.uniform_bindings = {
			FfxResourceBinding{3, 0, L"cbFSR2"}, FfxResourceBinding{4, 0, L"cbRCAS"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID];
		pass.shader = &shaders.compute_luminance_pyramid;
		pass.shader->initialize(modes_single, general_defines);
		pass.shader_version = pass.shader->version_create();

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_color_jittered"}};

		pass.storage_bindings = {FfxResourceBinding{1, 0, L"rw_spd_global_atomic"},
			FfxResourceBinding{2, 0, L"rw_img_mip_shading_change"},
			FfxResourceBinding{3, 0, L"rw_img_mip_5"},
			FfxResourceBinding{4, 0, L"rw_auto_exposure"}};

		pass.uniform_bindings = {
			FfxResourceBinding{5, 0, L"cbFSR2"}, FfxResourceBinding{6, 0, L"cbSPD"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_GENERATE_REACTIVE];
		pass.shader = &shaders.autogen_reactive;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_opaque_only"},
			FfxResourceBinding{1, 0, L"r_input_color_jittered"}};

		pass.storage_bindings = {FfxResourceBinding{2, 0, L"rw_output_autoreactive"}};

		pass.uniform_bindings = {
			FfxResourceBinding{3, 0, L"cbGenerateReactive"}, FfxResourceBinding{4, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_TCR_AUTOGENERATE];
		pass.shader = &shaders.tcr_autogen;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_opaque_only"},
			FfxResourceBinding{1, 0, L"r_input_color_jittered"},
			FfxResourceBinding{2, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{3, 0, L"r_input_prev_color_pre_alpha"},
			FfxResourceBinding{4, 0, L"r_input_prev_color_post_alpha"},
			FfxResourceBinding{5, 0, L"r_reactive_mask"},
			FfxResourceBinding{6, 0, L"r_transparency_and_composition_mask"},
			FfxResourceBinding{13, 0, L"r_input_depth"}};

		pass.storage_bindings = {FfxResourceBinding{7, 0, L"rw_output_autoreactive"},
			FfxResourceBinding{8, 0, L"rw_output_autocomposition"},
			FfxResourceBinding{9, 0, L"rw_output_prev_color_pre_alpha"},
			FfxResourceBinding{10, 0, L"rw_output_prev_color_post_alpha"}};

		pass.uniform_bindings = {
			FfxResourceBinding{11, 0, L"cbFSR2"}, FfxResourceBinding{12, 0, L"cbGenerateReactive"}};
	}

	RDC::SamplerState state;
	state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
	state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
	state.repeat_u = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.repeat_v = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.repeat_w = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.min_lod = -1000.0f;
	state.max_lod = 1000.0f;
	state.anisotropy_max = 1.0;
	device.point_clamp_sampler = RD::sampler_create(state);
	ERR_FAIL_COND(device.point_clamp_sampler.is_null());

	state.mag_filter = RDC::SAMPLER_FILTER_LINEAR;
	state.min_filter = RDC::SAMPLER_FILTER_LINEAR;
	device.linear_clamp_sampler = RD::sampler_create(state);
	ERR_FAIL_COND(device.linear_clamp_sampler.is_null());
}

FSR2Effect::~FSR2Effect()
{
	RD::free_rid(device.point_clamp_sampler);
	RD::free_rid(device.linear_clamp_sampler);

	for (uint32_t i = 0; i < FFX_FSR2_PASS_COUNT; i++) {
		device.passes[i].shader->version_free(device.passes[i].shader_version);
	}
}

FSR2Context* FSR2Effect::create_context(Size2i p_internal_size, Size2i p_target_size)
{
	FSR2Context* context = memnew(RendererRD::FSR2Context);
	context->fsr_desc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_DEPTH_INVERTED;
	context->fsr_desc.maxRenderSize.width = p_internal_size.x;
	context->fsr_desc.maxRenderSize.height = p_internal_size.y;
	context->fsr_desc.displaySize.width = p_target_size.x;
	context->fsr_desc.displaySize.height = p_target_size.y;
	context->fsr_desc.device = &device;

	FfxFsr2Interface& functions = context->fsr_desc.callbacks;
	functions.fpGetDeviceCapabilities = get_device_capabilities_rd;
	functions.fpDestroyBackendContext = destroy_backend_context_rd;
	functions.fpCreateResource = create_resource_rd;
	functions.fpRegisterResource = register_resource_rd;
	functions.fpUnregisterResources = unregister_resources_rd;
	functions.fpGetResourceDescription = get_resource_description_rd;
	functions.fpDestroyResource = destroy_resource_rd;
	functions.fpDestroyPipeline = destroy_pipeline_rd;
	functions.fpScheduleGpuJob = schedule_gpu_job_rd;
	functions.fpExecuteGpuJobs = execute_gpu_jobs_rd;
	functions.scratchBuffer = &context->scratch;
	functions.scratchBufferSize = sizeof(context->scratch);

	FfxErrorCode result = ffxFsr2ContextCreate(&context->fsr_context, &context->fsr_desc);
	if (result == FFX_OK) {
		return context;
	}
	else {
		memdelete(context);
		return nullptr;
	}
}

void FSR2Effect::upscale(const Parameters& p_params)
{
	// TODO: Transparency & Composition mask is not implemented.
	FfxFsr2DispatchDescription dispatch_desc = {};
	RID color = p_params.color;
	RID depth = p_params.depth;
	RID velocity = p_params.velocity;
	RID reactive = p_params.reactive;
	RID exposure = p_params.exposure;
	RID output = p_params.output;
	dispatch_desc.commandList = nullptr;
	dispatch_desc.color = get_resource_rd(&color, L"color");
	dispatch_desc.depth = get_resource_rd(&depth, L"depth");
	dispatch_desc.motionVectors = get_resource_rd(&velocity, L"velocity");
	dispatch_desc.reactive = get_resource_rd(&reactive, L"reactive");
	dispatch_desc.exposure = get_resource_rd(&exposure, L"exposure");
	dispatch_desc.transparencyAndComposition = {};
	dispatch_desc.output = get_resource_rd(&output, L"output");
	dispatch_desc.colorOpaqueOnly = {};
	dispatch_desc.jitterOffset.x = p_params.jitter.x;
	dispatch_desc.jitterOffset.y = p_params.jitter.y;
	dispatch_desc.motionVectorScale.x = float(p_params.internal_size.width);
	dispatch_desc.motionVectorScale.y = float(p_params.internal_size.height);
	dispatch_desc.reset = p_params.reset_accumulation;
	dispatch_desc.renderSize.width = p_params.internal_size.width;
	dispatch_desc.renderSize.height = p_params.internal_size.height;
	dispatch_desc.enableSharpening = (p_params.sharpness > 1e-6f);
	dispatch_desc.sharpness = p_params.sharpness;
	dispatch_desc.frameTimeDelta = p_params.delta_time;
	dispatch_desc.preExposure = 1.0f;
	dispatch_desc.cameraNear = p_params.z_near;
	dispatch_desc.cameraFar = p_params.z_far;
	dispatch_desc.cameraFovAngleVertical = p_params.fovy;
	dispatch_desc.viewSpaceToMetersFactor = 1.0f;
	dispatch_desc.enableAutoReactive = false;
	dispatch_desc.autoTcThreshold = 1.0f;
	dispatch_desc.autoTcScale = 1.0f;
	dispatch_desc.autoReactiveScale = 1.0f;
	dispatch_desc.autoReactiveMax = 1.0f;

	RendererRD::MaterialStorage::store_camera(
		p_params.reprojection, dispatch_desc.reprojectionMatrix);

	FfxErrorCode result = ffxFsr2ContextDispatch(&p_params.context->fsr_context, &dispatch_desc);
	ERR_FAIL_COND(result != FFX_OK);
}

/* TAA */

TAA::~TAA() { taa_shader.version_free(shader_version); }

void TAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RDC::DataFormat p_format,
	float p_z_near, float p_z_far)
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i internal_size = p_render_buffers->get_internal_size();
	Size2i target_size = p_render_buffers->get_target_size();

	bool just_allocated = false;
	if (!p_render_buffers->has_texture(SNAME("taa"), SNAME("history"))) {
		uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;

		p_render_buffers->create_texture(SNAME("taa"), SNAME("history"), p_format, usage_bits);
		p_render_buffers->create_texture(SNAME("taa"), SNAME("temp"), p_format, usage_bits);

		p_render_buffers->create_texture(
			SNAME("taa"), SNAME("prev_velocity"), RDC::DATA_FORMAT_R16G16_SFLOAT, usage_bits);

		just_allocated = true;
	}

	RD::draw_command_begin_label("TAA");

	for (uint32_t v = 0; v < view_count; v++) {
		// Get our (cached) slices
		RID internal_texture = p_render_buffers->get_internal_texture(v);
		RID velocity_buffer = p_render_buffers->get_velocity_buffer(false, v);
		RID taa_history = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("history"), v, 0);
		RID taa_prev_velocity =
			p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("prev_velocity"), v, 0);

		if (!just_allocated) {
			RID depth_texture = p_render_buffers->get_depth_texture(v);
			RID taa_temp = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("temp"), v, 0);
			resolve(internal_texture, taa_temp, depth_texture, velocity_buffer, taa_prev_velocity,
				taa_history, Size2(internal_size.x, internal_size.y), p_z_near, p_z_far);
			CopyEffects::copy_to_rect(
				taa_temp, internal_texture, Rect2(0, 0, internal_size.x, internal_size.y));
		}

		CopyEffects::copy_to_rect(
			internal_texture, taa_history, Rect2(0, 0, internal_size.x, internal_size.y));
		CopyEffects::copy_to_rect(
			velocity_buffer, taa_prev_velocity, Rect2(0, 0, target_size.x, target_size.y));
	}

	RD::draw_command_end_label();
}

void RendererRD::TAA::resolve(RID p_dest_framebuffer, RID p_current_color, RID p_previous_color,
	RID p_velocity, RID p_depth, RID p_feedback, Vector2 p_jitter, float p_feedback_weight,
	float p_scale)
{
}

#ifdef METAL_ENABLED

/* MetalFX Spatial Scaler */

MFXSpatialContext::~MFXSpatialContext()
{
	if (scaler) {
		scaler->release();
	}
}

MFXSpatialEffect::MFXSpatialEffect() {}

MFXSpatialEffect::~MFXSpatialEffect() {}

void MFXSpatialEffect::callback(
	RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata)
{
	MDCommandBufferBase* obj = (MDCommandBufferBase*)(p_command_buffer.id);
	obj->end();

	MTL::Texture* src_texture = reinterpret_cast<MTL::Texture*>(p_userdata->src.id);
	MTL::Texture* dst_texture = reinterpret_cast<MTL::Texture*>(p_userdata->dst.id);

	MTLFX::SpatialScalerBase* scaler = p_userdata->scaler;
	scaler->setColorTexture(src_texture);
	scaler->setOutputTexture(dst_texture);
	MTLFX::SpatialScaler* s = static_cast<MTLFX::SpatialScaler*>(scaler);
	MTL3::MDCommandBuffer* cmd = (MTL3::MDCommandBuffer*)(p_command_buffer.id);
	s->encodeToCommandBuffer(cmd->get_command_buffer());
	obj->retain_resource(scaler);

	CallbackArgs::free(&p_userdata);
}

void MFXSpatialEffect::ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	p_render_buffers->ensure_mfx(this);
}

void MFXSpatialEffect::process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_src, RID p_dst)
{
	MFXSpatialContext* ctx = p_render_buffers->get_mfx_spatial_context();
	DEV_ASSERT(ctx); // this should have been done by the caller via ensure_context

	CallbackArgs* userdata = args_allocator.alloc(this,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_src)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_dst)), *ctx);
	RD::CallbackResource res[2] = {
		{.rid = p_src, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_dst, .usage = RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE}};
	RD::driver_callback_add((RDD::DriverCallback)MFXSpatialEffect::callback, userdata,
		VectorView<RD::CallbackResource>(res, 2));
}

MFXSpatialContext* MFXSpatialEffect::create_context(CreateParams p_params) const
{
	DEV_ASSERT(RD::has_feature(RD::SUPPORTS_METALFX_SPATIAL));

	RenderingDeviceDriverMetal* rdd = (RenderingDeviceDriverMetal*)RD::get_device_driver();
	PixelFormats& pf = rdd->get_pixel_formats();
	MTL::Device* dev = rdd->get_device();

	NS::SharedPtr<MTLFX::SpatialScalerDescriptor> desc =
		NS::TransferPtr(MTLFX::SpatialScalerDescriptor::alloc()->init());
	desc->setInputWidth((NS::UInteger)p_params.input_size.width);
	desc->setInputHeight((NS::UInteger)p_params.input_size.height);

	desc->setOutputWidth((NS::UInteger)p_params.output_size.width);
	desc->setOutputHeight((NS::UInteger)p_params.output_size.height);

	desc->setColorTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.input_format));
	desc->setOutputTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.output_format));
	desc->setColorProcessingMode(MTLFX::SpatialScalerColorProcessingModeLinear);

	MFXSpatialContext* context = memnew(MFXSpatialContext);
	context->scaler = desc->newSpatialScaler(dev);

	return context;
}

#ifdef METAL_MFXTEMPORAL_ENABLED

/* MetalFX Temporal Scaler */

MFXTemporalContext::~MFXTemporalContext()
{
	if (scaler) {
		scaler->release();
	}
}

MFXTemporalEffect::MFXTemporalEffect() {}
MFXTemporalEffect::~MFXTemporalEffect() {}

MFXTemporalContext* MFXTemporalEffect::create_context(CreateParams p_params) const
{
	DEV_ASSERT(RD::has_feature(RD::SUPPORTS_METALFX_TEMPORAL));

	RenderingDeviceDriverMetal* rdd = (RenderingDeviceDriverMetal*)RD::get_device_driver();
	PixelFormats& pf = rdd->get_pixel_formats();
	MTL::Device* dev = rdd->get_device();

	NS::SharedPtr<MTLFX::TemporalScalerDescriptor> desc =
		NS::TransferPtr(MTLFX::TemporalScalerDescriptor::alloc()->init());
	desc->setInputWidth((NS::UInteger)p_params.input_size.width);
	desc->setInputHeight((NS::UInteger)p_params.input_size.height);

	desc->setOutputWidth((NS::UInteger)p_params.output_size.width);
	desc->setOutputHeight((NS::UInteger)p_params.output_size.height);

	desc->setColorTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.input_format));
	desc->setDepthTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.depth_format));
	desc->setMotionTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.motion_format));
	desc->setAutoExposureEnabled(false);

	desc->setOutputTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.output_format));

	MFXTemporalContext* context = memnew(MFXTemporalContext);
	context->scaler = desc->newTemporalScaler(dev);
	context->scaler->setMotionVectorScaleX(p_params.motion_vector_scale.x);
	context->scaler->setMotionVectorScaleY(p_params.motion_vector_scale.y);
	context->scaler->setDepthReversed(true);

	return context;
}

void MFXTemporalEffect::process(
	RendererRD::MFXTemporalContext* p_ctx, RendererRD::MFXTemporalEffect::Params p_params)
{
	CallbackArgs* userdata = args_allocator.alloc(this,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.src)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.depth)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.motion)),
		p_params.exposure.is_valid()
			? RDD::TextureID(
				  RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.exposure))
			: RDD::TextureID(),
		p_params.jitter_offset,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.dst)), *p_ctx,
		p_params.reset);
	RD::CallbackResource res[3] = {
		{.rid = p_params.src, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_params.depth, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_params.dst, .usage = RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE},
	};
	RD::driver_callback_add((RDD::DriverCallback)MFXTemporalEffect::callback, userdata,
		VectorView<RD::CallbackResource>(res, 3));
}

void MFXTemporalEffect::callback(
	RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata)
{
	MDCommandBufferBase* obj = (MDCommandBufferBase*)(p_command_buffer.id);
	obj->end();

	MTL::Texture* src_texture = reinterpret_cast<MTL::Texture*>(p_userdata->src.id);
	MTL::Texture* depth = reinterpret_cast<MTL::Texture*>(p_userdata->depth.id);
	MTL::Texture* motion = reinterpret_cast<MTL::Texture*>(p_userdata->motion.id);
	MTL::Texture* exposure = reinterpret_cast<MTL::Texture*>(p_userdata->exposure.id);

	MTL::Texture* dst_texture = reinterpret_cast<MTL::Texture*>(p_userdata->dst.id);

	MTLFX::TemporalScalerBase* scaler = p_userdata->scaler;
	scaler->setReset(p_userdata->reset);
	scaler->setColorTexture(src_texture);
	scaler->setDepthTexture(depth);
	scaler->setMotionTexture(motion);
	scaler->setExposureTexture(exposure);
	scaler->setJitterOffsetX(p_userdata->jitter_offset.x);
	scaler->setJitterOffsetY(p_userdata->jitter_offset.y);
	scaler->setOutputTexture(dst_texture);
	MTLFX::TemporalScaler* s = static_cast<MTLFX::TemporalScaler*>(scaler);
	MTL3::MDCommandBuffer* cmd = (MTL3::MDCommandBuffer*)(p_command_buffer.id);
	s->encodeToCommandBuffer(cmd->get_command_buffer());
	obj->retain_resource(scaler);

	CallbackArgs::free(&p_userdata);
}

#endif // METAL_MFXTEMPORAL_ENABLED

#endif // METAL_ENABLED


// --- Merged from servers/rendering/renderer_rd/renderer_scene_render_rd.cpp ---
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/os/memory.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep. RENDER_TIMESTAMP macro uses RSG.

// --- Merged from servers/rendering/renderer_rd/renderer_scene_render_rd.cpp ---

void get_vogel_disk(float* r_kernel, int p_sample_count)
{
	const float golden_angle = 2.4;

	for (int i = 0; i < p_sample_count; i++) {
		float r = Math::sqrt(float(i) + 0.5) / Math::sqrt(float(p_sample_count));
		float theta = float(i) * golden_angle;

		r_kernel[i * 4] = Math::cos(theta) * r;
		r_kernel[i * 4 + 1] = Math::sin(theta) * r;
	}
}

RID RendererSceneRenderRD::sky_allocate() { return data->sky.allocate_sky_rid(); }

void RendererSceneRenderRD::sky_initialize(RID p_rid) { data->sky.initialize_sky_rid(p_rid); }

void RendererSceneRenderRD::sky_set_radiance_size(RID p_sky, int p_radiance_size)
{
	data->sky.sky_set_radiance_size(p_sky, p_radiance_size);
}

void RendererSceneRenderRD::sky_set_mode(RID p_sky, RSE::SkyMode p_mode)
{
	data->sky.sky_set_mode(p_sky, p_mode);
}

void RendererSceneRenderRD::sky_set_material(RID p_sky, RID p_material)
{
	data->sky.sky_set_material(p_sky, p_material);
}

Ref<Image> RendererSceneRenderRD::sky_bake_panorama(
	RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size)
{
	return data->sky.sky_bake_panorama(p_sky, p_energy, p_bake_irradiance, p_size);
}

void RendererSceneRenderRD::environment_glow_set_use_bicubic_upscale(bool p_enable)
{
	data->glow_bicubic_upscale = p_enable;
}

void RendererSceneRenderRD::environment_set_volumetric_fog_volume_size(int p_size, int p_depth)
{
	data->volumetric_fog_size = p_size;
	data->volumetric_fog_depth = p_depth;
}

void RendererSceneRenderRD::environment_set_volumetric_fog_filter_active(bool p_enable)
{
	data->volumetric_fog_filter_active = p_enable;
}

void RendererSceneRenderRD::environment_set_sdfgi_ray_count(
	RSE::EnvironmentSDFGIRayCount p_ray_count)
{
	RendererRD::GI::sdfgi_ray_count = p_ray_count;
}

void RendererSceneRenderRD::environment_set_sdfgi_frames_to_converge(
	RSE::EnvironmentSDFGIFramesToConverge p_frames)
{
	RendererRD::GI::sdfgi_frames_to_converge = p_frames;
}

void RendererSceneRenderRD::environment_set_sdfgi_frames_to_update_light(
	RSE::EnvironmentSDFGIFramesToUpdateLight p_update)
{
	RendererRD::GI::sdfgi_frames_to_update_light = p_update;
}

Ref<Image> RendererSceneRenderRD::environment_bake_panorama(
	RID p_env, bool p_bake_irradiance, const Size2i& p_size)
{
	ERR_FAIL_COND_V(p_env.is_null(), Ref<Image>());

	RSE::EnvironmentBG environment_background = RendererSceneRender::environment_get_background(p_env);

	if (environment_background == RSE::ENV_BG_CAMERA_FEED ||
		environment_background == RSE::ENV_BG_CANVAS ||
		environment_background == RSE::ENV_BG_KEEP) {
		return Ref<Image>(); // nothing to bake
	}

	RSE::EnvironmentAmbientSource ambient_source = RendererSceneRender::RendererSceneRender::environment_get_ambient_source(p_env);

	bool use_ambient_light = false;
	bool use_octmap = false;
	if (ambient_source == RSE::ENV_AMBIENT_SOURCE_BG &&
		(environment_background == RSE::ENV_BG_CLEAR_COLOR ||
			environment_background == RSE::ENV_BG_COLOR)) {
		use_ambient_light = true;
	}
	else {
		use_octmap = (ambient_source == RSE::ENV_AMBIENT_SOURCE_BG &&
						 environment_background == RSE::ENV_BG_SKY) ||
					 ambient_source == RSE::ENV_AMBIENT_SOURCE_SKY;
		use_ambient_light = use_octmap || ambient_source == RSE::ENV_AMBIENT_SOURCE_COLOR;
	}
	use_octmap = use_octmap || (environment_background == RSE::ENV_BG_SKY &&
								   RendererSceneRender::RendererSceneRender::environment_get_sky(p_env).is_valid());

	Color ambient_color;
	float ambient_color_sky_mix = 0.0;
	if (use_ambient_light) {
		ambient_color_sky_mix = RendererSceneRender::RendererSceneRender::environment_get_ambient_sky_contribution(p_env);
		const float ambient_energy = RendererSceneRender::RendererSceneRender::environment_get_ambient_light_energy(p_env);
		ambient_color = RendererSceneRender::RendererSceneRender::environment_get_ambient_light(p_env);
		ambient_color = ambient_color.srgb_to_linear();
		ambient_color.r *= ambient_energy;
		ambient_color.g *= ambient_energy;
		ambient_color.b *= ambient_energy;
	}

	if (use_octmap) {
		Ref<Image> panorama = sky_bake_panorama(RendererSceneRender::RendererSceneRender::environment_get_sky(p_env),
			RendererSceneRender::RendererSceneRender::environment_get_bg_energy_multiplier(p_env), p_bake_irradiance, p_size);
		if (use_ambient_light && panorama.is_valid()) {
			for (int x = 0; x < p_size.width; x++) {
				for (int y = 0; y < p_size.height; y++) {
					panorama->set_pixel(
						x, y, ambient_color.lerp(panorama->get_pixel(x, y), ambient_color_sky_mix));
				}
			}
		}
		return panorama;
	}
	else {
		const float bg_energy_multiplier = RendererSceneRender::RendererSceneRender::environment_get_bg_energy_multiplier(p_env);
		Color panorama_color = ((environment_background == RSE::ENV_BG_CLEAR_COLOR)
									? RendererTextureStorage::get_default_clear_color()
									: RendererSceneRender::RendererSceneRender::environment_get_bg_color(p_env));
		panorama_color = panorama_color.srgb_to_linear();
		panorama_color.r *= bg_energy_multiplier;
		panorama_color.g *= bg_energy_multiplier;
		panorama_color.b *= bg_energy_multiplier;

		if (use_ambient_light) {
			panorama_color = ambient_color.lerp(panorama_color, ambient_color_sky_mix);
		}

		Ref<Image> panorama =
			Image::create_empty(p_size.width, p_size.height, false, Image::FORMAT_RGBAF);
		panorama->fill(panorama_color);
		return panorama;
	}
}

/* REFLECTION PROBE */

RID RendererSceneRenderRD::reflection_probe_create_framebuffer(RID p_color, RID p_depth)
{
	Vector<RID> fb;
	fb.push_back(p_color);
	fb.push_back(p_depth);
	return RD::framebuffer_create(fb);
}

/* FOG VOLUME INSTANCE */

RID RendererSceneRenderRD::fog_volume_instance_create(RID p_fog_volume)
{
	return RendererRD::Fog::fog_volume_instance_create(p_fog_volume);
}

void RendererSceneRenderRD::fog_volume_instance_set_transform(
	RID p_fog_volume_instance, const Transform3D& p_transform)
{
	RendererRD::Fog::fog_volume_instance_set_transform(
		p_fog_volume_instance, p_transform);
}

void RendererSceneRenderRD::fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active)
{
	RendererRD::Fog::fog_volume_instance_set_active(
		p_fog_volume_instance, p_active);
}

RID RendererSceneRenderRD::fog_volume_instance_get_volume(RID p_fog_volume_instance)
{
	return RendererRD::Fog::fog_volume_instance_get_volume(p_fog_volume_instance);
}

Vector3 RendererSceneRenderRD::fog_volume_instance_get_position(RID p_fog_volume_instance)
{
	return RendererRD::Fog::fog_volume_instance_get_position(
		p_fog_volume_instance);
}

/* VOXEL data->GI */

RID RendererSceneRenderRD::voxel_gi_instance_create(RID p_base)
{
	return RendererRD::GI::voxel_gi_instance_create(p_base);
}

void RendererSceneRenderRD::voxel_gi_instance_set_transform_to_data(
	RID p_probe, const Transform3D& p_xform)
{
	if (!is_dynamic_gi_supported()) {
		return;
	}

	RendererRD::GI::voxel_gi_instance_set_transform_to_data(p_probe, p_xform);
}

bool RendererSceneRenderRD::voxel_gi_needs_update(RID p_probe)
{
	if (!is_dynamic_gi_supported()) {
		return false;
	}

	return RendererRD::GI::voxel_gi_needs_update(p_probe);
}

void RendererSceneRenderRD::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
	if (!is_dynamic_gi_supported()) {
		return;
	}

	RendererRD::GI::voxel_gi_update(p_probe, p_update_light_instances, p_light_instances, p_dynamic_objects);
}

void RendererSceneRenderRD::_debug_sdfgi_probes(Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_framebuffer, const uint32_t p_view_count, const Projection* p_camera_with_transforms)
{
	ERR_FAIL_COND(p_render_buffers.is_null());

	if (!p_render_buffers->has_custom_data(RB_SCOPE_SDFGI)) {
		return; // nothing to debug
	}

	Ref<RendererRD::GI::SDFGI> sdfgi = p_render_buffers->get_custom_data(RB_SCOPE_SDFGI);

	sdfgi->debug_probes(p_framebuffer, p_view_count, p_camera_with_transforms);
}

////////////////////////////////
Ref<RenderSceneBuffers> RendererSceneRenderRD::render_buffers_create()
{
	Ref<RenderSceneBuffersRD> rb;
	rb.instantiate();

	rb->set_can_be_storage(_render_buffers_can_be_storage());
	rb->set_max_cluster_elements(data->max_cluster_elements);
	rb->set_preferred_data_format(_render_buffers_get_preferred_color_format());
	if (data->vrs) {
		rb->set_vrs(data->vrs);
	}

	setup_render_buffer_data(rb);

	return rb;
}

bool RendererSceneRenderRD::_compositor_effects_has_flag(const RenderDataRD* p_render_data,
	RSE::CompositorEffectFlags p_flag, RSE::CompositorEffectCallbackType p_callback_type)
{
	if (p_render_data->compositor.is_null()) {
		return false;
	}

	if (p_render_data->reflection_probe.is_valid()) {
		return false;
	}

	ERR_FAIL_COND_V(!RendererCompositorStorage::is_compositor(p_render_data->compositor), false);
	Vector<RID> re_rids = RendererCompositorStorage::compositor_get_compositor_effects(
		p_render_data->compositor, p_callback_type, true);

	for (RID rid : re_rids) {
		if (RendererCompositorStorage::compositor_effect_get_flag(rid, p_flag)) {
			return true;
		}
	}

	return false;
}

bool RendererSceneRenderRD::_has_compositor_effect(
	RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data)
{
	if (p_render_data->compositor.is_null()) {
		return false;
	}

	if (p_render_data->reflection_probe.is_valid()) {
		return false;
	}

	ERR_FAIL_COND_V(!RendererCompositorStorage::is_compositor(p_render_data->compositor), false);

	Vector<RID> effects = RendererCompositorStorage::compositor_get_compositor_effects(
		p_render_data->compositor, p_callback_type, true);

	return effects.size() > 0;
}

void RendererSceneRenderRD::_render_buffers_ensure_screen_texture(const RenderDataRD* p_render_data)
{
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	if (!rb->has_internal_texture()) {
		// We're likely rendering reflection probes where we can't use our backbuffers.
		return;
	}

	bool can_use_storage = _render_buffers_can_be_storage();
	Size2i size = rb->get_internal_size();

	// When upscaling, the blur texture needs to be at the target size for post-processing to work.
	// We prefer to use a dedicated backbuffer copy texture instead if the blur texture is not an
	// option so shader effects work correctly.
	Size2i target_size = rb->get_target_size();
	bool internal_size_matches =
		(size.width == target_size.width) && (size.height == target_size.height);
	bool reuse_blur_texture = !rb->has_upscaled_texture() || internal_size_matches;
	if (reuse_blur_texture) {
		rb->allocate_blur_textures();
	}
	else {
		if (!rb->has_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR)) {
			uint32_t usage_bits =
				RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
			usage_bits |= can_use_storage ? RDC::TEXTURE_USAGE_STORAGE_BIT
										  : RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
			// This needs to have mipmaps if any shader needs textureLod to work on screen_texture
			uint32_t mipmaps_required =
				Image::get_image_required_mipmaps(size.x, size.y, Image::FORMAT_RGBAH);
			rb->create_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR, rb->get_base_data_format(),
				usage_bits, RenderingDeviceCommons::TEXTURE_SAMPLES_1, {0, 0}, 0U,
				mipmaps_required);
		}
	}
}

void RendererSceneRenderRD::_render_buffers_copy_screen_texture(const RenderDataRD* p_render_data)
{
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	if (!rb->has_internal_texture()) {
		// We're likely rendering reflection probes where we can't use our backbuffers.
		return;
	}

	RD::draw_command_begin_label("Copy Screen Texture");

	String texture_name;
	bool can_use_storage = _render_buffers_can_be_storage();
	Size2i size = rb->get_internal_size();

	// When upscaling, the blur texture needs to be at the target size for post-processing to work.
	// We prefer to use a dedicated backbuffer copy texture instead if the blur texture is not an
	// option so shader effects work correctly.
	Size2i target_size = rb->get_target_size();
	bool internal_size_matches =
		(size.width == target_size.width) && (size.height == target_size.height);
	bool reuse_blur_texture = !rb->has_upscaled_texture() || internal_size_matches;
	if (reuse_blur_texture) {
		texture_name = RB_TEX_BLUR_0;
	}
	else {
		texture_name = RB_TEX_BACK_COLOR;
	}

	for (uint32_t v = 0; v < rb->get_view_count(); v++) {
		RID texture = rb->get_internal_texture(v);
		int mipmaps = int(rb->get_texture_format(RB_SCOPE_BUFFERS, texture_name).mipmaps);
		RID dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, texture_name, v, 0);

		if (can_use_storage) {
			RendererRD::CopyEffects::copy_to_rect(texture, dest, Rect2i(0, 0, size.x, size.y));
		}
		else {
			RID fb = FramebufferCacheRD::get_cache(dest);
			RendererRD::CopyEffects::copy_to_fb_rect(texture, fb, Rect2i(0, 0, size.x, size.y));
		}

		for (int i = 1; i < mipmaps; i++) {
			RID source = dest;
			dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, texture_name, v, i);
			Size2i msize = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, texture_name, i);

			if (can_use_storage) {
				RendererRD::CopyEffects::make_mipmap(source, dest, msize);
			}
			else {
				RendererRD::CopyEffects::make_mipmap_raster(source, dest, msize);
			}
		}
	}

	RD::draw_command_end_label();
}

void RendererSceneRenderRD::_render_buffers_ensure_depth_texture(const RenderDataRD* p_render_data)
{
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	if (!rb->has_depth_texture()) {
		// We're likely rendering reflection probes where we can't use our backbuffers.
		return;
	}

	// Note, this only creates our back depth texture if we haven't already created it.
	uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT;
	usage_bits |= RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;
	usage_bits |= RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT; // Set this as color attachment because
														  // we're copying data into it, it's not
														  // actually used as a depth buffer

	rb->create_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_DEPTH, RDC::DATA_FORMAT_R32_SFLOAT, usage_bits,
		RDC::TEXTURE_SAMPLES_1);
}

void RendererSceneRenderRD::_render_buffers_copy_depth_texture(
	const RenderDataRD* p_render_data, bool p_use_msaa)
{
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	if (!rb->has_depth_texture()) {
		// We're likely rendering reflection probes where we can't use our backbuffers.
		return;
	}

	RD::draw_command_begin_label("Copy Depth Texture");

	bool can_use_storage = _render_buffers_can_be_storage();
	Size2i size = rb->get_internal_size();
	for (uint32_t v = 0; v < p_render_data->scene_data->view_count; v++) {
		RID depth_texture = rb->get_depth_texture(v);
		RID depth_back_texture = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BACK_DEPTH, v, 0);

		if (can_use_storage) {
			RendererRD::CopyEffects::copy_depth_to_rect(
				depth_texture, depth_back_texture, Rect2i(0, 0, size.x, size.y));
		}
		else {
			RID depth_back_fb = FramebufferCacheRD::get_cache(depth_back_texture);
			if (p_use_msaa) {
				static const int texture_multisamples[RSE::VIEWPORT_MSAA_MAX] = {1, 2, 4, 8};

				data->resolve_effects->resolve_depth_raster(
					rb->get_depth_msaa(v), depth_back_fb, texture_multisamples[rb->get_msaa_3d()]);
			}
			else {
				RendererRD::CopyEffects::copy_to_fb_rect(
					depth_texture, depth_back_fb, Rect2i(0, 0, size.x, size.y));
			}
		}
	}

	RD::draw_command_end_label();
}

void RendererSceneRenderRD::_render_buffers_post_process_and_tonemap(
	const RenderDataRD* p_render_data, bool p_use_msaa)
{
	ERR_FAIL_NULL(p_render_data);

	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	ERR_FAIL_COND_MSG(p_render_data->reflection_probe.is_valid(),
		"Post processes should not be applied on reflection probes.");

	// Glow, auto exposure and DoF (if enabled).

	Size2i target_size = rb->get_target_size();
	bool can_use_effects =
		target_size.x >= 8 && target_size.y >= 8; // FIXME I think this should check internal size,
												  // we do all our post processing at this size...
	can_use_effects &= _debug_draw_can_use_effects(data->debug_draw);
	bool can_use_storage = _render_buffers_can_be_storage();

	RSE::ViewportScaling3DMode scale_mode = rb->get_scaling_3d_mode();
	bool use_upscaled_texture = rb->has_upscaled_texture() &&
								(scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR2 ||
									scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL);
	SpatialUpscaler* spatial_upscaler = nullptr;
	if (can_use_effects) {
		if (scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR) {
			spatial_upscaler = data->fsr;
		}
		else if (scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL) {
#if METAL_ENABLED
			spatial_upscaler = mfx_spatial;
#endif
		}
	}

	bool use_fxaa = rb->get_screen_space_aa() == RSE::VIEWPORT_SCREEN_SPACE_AA_FXAA;
	bool use_smaa = data->smaa && rb->get_screen_space_aa() == RSE::VIEWPORT_SCREEN_SPACE_AA_SMAA;
	// If doing bilinear or nearest scaling + FXAA / SMAA, the framebuffer must be scaled in a
	// framebuffer copy after AA is applied.
	bool using_scaling_pass =
		spatial_upscaler ||
		((use_fxaa || use_smaa) && (scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR ||
									   scale_mode == RSE::VIEWPORT_SCALING_3D_MODE_NEAREST));

	RID render_target = rb->get_render_target();
	RID color_texture =
		use_upscaled_texture ? rb->get_upscaled_texture() : rb->get_internal_texture();
	Size2i color_size = use_upscaled_texture ? target_size : rb->get_internal_size();

	bool dest_is_msaa_2d =
		rb->get_view_count() == 1 &&
		RendererRD::TextureStorage::render_target_get_msaa(render_target) != RSE::VIEWPORT_MSAA_DISABLED;

	bool using_dof =
		RendererCameraAttributes::camera_attributes_uses_dof(p_render_data->camera_attributes);

	if (using_dof && p_render_data->transparent_bg) {
		WARN_PRINT_ONCE("Depth of field is not supported in viewports with a transparent "
						"background. Disabling DoF in transparent viewport.");
		using_dof = false;
	}

	if (can_use_effects && using_dof) {
		RENDER_TIMESTAMP("Depth of Field");
		RD::draw_command_begin_label("DOF");

		rb->allocate_blur_textures();

		RendererRD::BokehDOF::BokehBuffers buffers;

		// Textures we use
		buffers.base_texture_size = color_size;
		buffers.secondary_texture = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 0, 0);
		buffers.half_texture[0] = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, 0, 0);
		buffers.half_texture[1] = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 0, 1);

		if (can_use_storage) {
			for (uint32_t i = 0; i < rb->get_view_count(); i++) {
				buffers.base_texture = use_upscaled_texture ? rb->get_upscaled_texture(i)
															: rb->get_internal_texture(i);
				buffers.depth_texture = rb->get_depth_texture(i);

				// In stereo p_render_data->z_near and p_render_data->z_far can be offset for our
				// combined frustum.
				float z_near = p_render_data->scene_data->view_projection[i].get_z_near();
				float z_far = p_render_data->scene_data->view_projection[i].get_z_far();
				data->bokeh_dof->bokeh_dof_compute(buffers, p_render_data->camera_attributes, z_near,
					z_far, p_render_data->scene_data->cam_orthogonal);
			};
		}
		else {
			// Set framebuffers.
			buffers.secondary_fb = rb->weight_buffers[1].fb;
			buffers.half_fb[0] = rb->weight_buffers[2].fb;
			buffers.half_fb[1] = rb->weight_buffers[3].fb;
			buffers.weight_texture[0] = rb->weight_buffers[0].weight;
			buffers.weight_texture[1] = rb->weight_buffers[1].weight;
			buffers.weight_texture[2] = rb->weight_buffers[2].weight;
			buffers.weight_texture[3] = rb->weight_buffers[3].weight;

			// Set weight buffers.
			buffers.base_weight_fb = rb->weight_buffers[0].fb;

			for (uint32_t i = 0; i < rb->get_view_count(); i++) {
				buffers.base_texture = use_upscaled_texture ? rb->get_upscaled_texture(i)
															: rb->get_internal_texture(i);
				buffers.depth_texture =
					p_use_msaa ? rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BACK_DEPTH, i, 0)
							   : rb->get_depth_texture(i);
				buffers.base_fb = FramebufferCacheRD::get_cache(
					buffers.base_texture); // TODO move this into bokeh_dof_raster, we can do this
										   // internally

				// In stereo p_render_data->z_near and p_render_data->z_far can be offset for our
				// combined frustum.
				float z_near = p_render_data->scene_data->view_projection[i].get_z_near();
				float z_far = p_render_data->scene_data->view_projection[i].get_z_far();
				data->bokeh_dof->bokeh_dof_raster(buffers, p_render_data->camera_attributes, z_near,
					z_far, p_render_data->scene_data->cam_orthogonal);
			}
		}
		RD::draw_command_end_label();
	}

	float auto_exposure_scale = 1.0;

	if (can_use_effects && RendererCameraAttributes::camera_attributes_uses_auto_exposure(
							   p_render_data->camera_attributes)) {
		RENDER_TIMESTAMP("Auto exposure");

		RD::draw_command_begin_label("Auto Exposure");

		Ref<RendererRD::Luminance::LuminanceBuffers> luminance_buffers =
			data->luminance->get_luminance_buffers(rb);

		uint64_t auto_exposure_version =
			RendererCameraAttributes::camera_attributes_get_auto_exposure_version(
				p_render_data->camera_attributes);
		bool set_immediate = auto_exposure_version != rb->get_auto_exposure_version();
		rb->set_auto_exposure_version(auto_exposure_version);

		double step = RendererCameraAttributes::camera_attributes_get_auto_exposure_adjust_speed(
						  p_render_data->camera_attributes) *
					  data->time_step;
		float auto_exposure_min_sensitivity =
			RendererCameraAttributes::camera_attributes_get_auto_exposure_min_sensitivity(
				p_render_data->camera_attributes);
		float auto_exposure_max_sensitivity =
			RendererCameraAttributes::camera_attributes_get_auto_exposure_max_sensitivity(
				p_render_data->camera_attributes);
		data->luminance->luminance_reduction(rb->get_internal_texture(), rb->get_internal_size(),
			luminance_buffers, auto_exposure_min_sensitivity, auto_exposure_max_sensitivity, step,
			set_immediate);

		// Swap final reduce with prev luminance.

		auto_exposure_scale = RendererCameraAttributes::camera_attributes_get_auto_exposure_scale(
			p_render_data->camera_attributes);

		Renderer::redraw_request(); // Redraw all the time if auto exposure rendering
												  // is on.
		RD::draw_command_end_label();
	}

	if (can_use_effects && p_render_data->environment.is_valid() &&
		RendererSceneRender::environment_get_glow_enabled(p_render_data->environment)) {
		RENDER_TIMESTAMP("Glow");

		rb->allocate_blur_textures();

		int mipmaps = int(rb->get_texture_format(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1).mipmaps);
		Vector<float> glow_levels = RendererSceneRender::environment_get_glow_levels(p_render_data->environment);
		bool use_debanding =
			rb->get_use_debanding() && !RendererRD::TextureStorage::render_target_is_using_hdr(render_target);

		int max_glow_index = -1;
		int min_glow_level = RSE::MAX_GLOW_LEVELS;
		for (int i = 0; i < RSE::MAX_GLOW_LEVELS; i++) {
			if (glow_levels[i] > 0.01) {
				max_glow_index = MAX(max_glow_index, i);
				min_glow_level = MIN(min_glow_level, i);
			}
		}

		max_glow_index = MIN(max_glow_index, mipmaps - 1);

		float luminance_multiplier = rb->get_luminance_multiplier();
		if (can_use_storage) {
			RD::draw_command_begin_label("Gaussian Glow");
			RID luminance_texture;
			if (RendererCameraAttributes::camera_attributes_uses_auto_exposure(
					p_render_data->camera_attributes)) {
				luminance_texture = data->luminance->get_current_luminance_buffer(
					rb); // this will return and empty RID if we don't have an auto exposure buffer
			}
			for (uint32_t l = 0; l < rb->get_view_count(); l++) {
				Size2i vp_size = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, 0);
				RID source = rb->get_internal_texture(l);
				RID dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, 0);
				RendererRD::CopyEffects::gaussian_glow(source, dest, vp_size,
					RendererSceneRender::environment_get_glow_strength(p_render_data->environment), true,
					RendererSceneRender::environment_get_glow_hdr_luminance_cap(p_render_data->environment),
					RendererSceneRender::environment_get_exposure(p_render_data->environment),
					RendererSceneRender::environment_get_glow_bloom(p_render_data->environment),
					RendererSceneRender::environment_get_glow_hdr_bleed_threshold(p_render_data->environment),
					RendererSceneRender::environment_get_glow_hdr_bleed_scale(p_render_data->environment),
					luminance_texture, auto_exposure_scale);

				for (int i = 1; i < (max_glow_index + 1); i++) {
					source = dest;
					vp_size = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, i);
					dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, i);
					RendererRD::CopyEffects::gaussian_glow(source, dest, vp_size,
						RendererSceneRender::environment_get_glow_strength(p_render_data->environment));
				}
			}
			RD::draw_command_end_label();
		}
		else {
			// For the mobile renderer we blur down and up the mip chain. Which works out to
			// (2*level-1) passes. This allows us to gather our levels at low resolutions and
			// ultimately save a lot of texture read bandwidth. The tradeoff is that we need to use
			// single-pass blur to minimize the number of render passes.

			RID source;
			RID dest;

			for (uint32_t l = 0; l < rb->get_view_count(); l++) {
				RD::draw_command_begin_label("Gaussian Glow downsample");

				Size2i source_size = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_COLOR, 0);

				source = rb->get_internal_texture(l);
				dest = rb->get_texture_slice(
					RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, 1); // Level 1 is quarter res.

				RendererRD::CopyEffects::gaussian_glow_downsample_raster(source, dest, luminance_multiplier,
					source_size, RendererSceneRender::environment_get_glow_strength(p_render_data->environment), true,
					RendererSceneRender::environment_get_glow_hdr_luminance_cap(p_render_data->environment),
					RendererSceneRender::environment_get_exposure(p_render_data->environment),
					RendererSceneRender::environment_get_glow_bloom(p_render_data->environment),
					RendererSceneRender::environment_get_glow_hdr_bleed_threshold(p_render_data->environment),
					RendererSceneRender::environment_get_glow_hdr_bleed_scale(p_render_data->environment));

				Size2i vp_size;
				for (int i = 1; i < (max_glow_index + 1); i++) {
					source = dest;
					vp_size = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, i);
					dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, i + 1);

					RendererRD::CopyEffects::gaussian_glow_downsample_raster(source, dest,
						luminance_multiplier, vp_size,
						RendererSceneRender::environment_get_glow_strength(p_render_data->environment));
				}
				RD::draw_command_end_label();
				RD::draw_command_begin_label("Gaussian Glow upsample");

				if (max_glow_index <= 0) {
					// Only layer 1 is visible, just copy over.
					source = RendererRD::TextureStorage::texture_rd_get_default(RendererRD::TextureStorage::
							DEFAULT_RD_TEXTURE_BLACK); // Technically a waste, but oh well. I'm not
													   // optimizing for the case of only level 1.
					vp_size = rb->get_texture_slice_size(
						RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 2); // RB_TEX_BLUR_0 is double the size of
															 // RB_TEX_BLUR_1, so go up a mip level.
					dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, l, 2);
					RID blend_tex = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, 1);
					source_size = vp_size;

					RendererRD::CopyEffects::gaussian_glow_upsample_raster(source, dest, blend_tex,
						luminance_multiplier, source_size, vp_size, glow_levels[0], 0.0,
						use_debanding);
				}

				for (int i = max_glow_index - 1; i >= 0; i--) {
					source = dest;
					source_size =
						rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, i + 3);
					vp_size = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0,
						i + 2); // RB_TEX_BLUR_0 is double the size of RB_TEX_BLUR_1, so go up a mip
								// level.
					dest = rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, l, i + 2);
					RID blend_tex =
						rb->get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, l, i + 1);

					RendererRD::CopyEffects::gaussian_glow_upsample_raster(source, dest, blend_tex,
						luminance_multiplier, source_size, vp_size, glow_levels[i],
						i == (max_glow_index - 1) ? glow_levels[i + 1] : 1.0, use_debanding);
				}
				RD::draw_command_end_label();
			}
		}
	}

	{
		RENDER_TIMESTAMP("Tonemap");
		RD::draw_command_begin_label("Tonemap");

		RendererRD::ToneMapper::TonemapSettings tonemap;

		bool using_hdr = RendererRD::TextureStorage::render_target_is_using_hdr(render_target);

		tonemap.exposure_texture = data->luminance->get_current_luminance_buffer(rb);
		if (can_use_effects &&
			RendererCameraAttributes::camera_attributes_uses_auto_exposure(
				p_render_data->camera_attributes) &&
			tonemap.exposure_texture.is_valid()) {
			tonemap.use_auto_exposure = true;
			tonemap.auto_exposure_scale = auto_exposure_scale;
		}
		else {
			tonemap.exposure_texture = RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
		}

		if (can_use_effects && p_render_data->environment.is_valid() &&
			RendererSceneRender::environment_get_glow_enabled(p_render_data->environment)) {
			tonemap.use_glow = true;
			tonemap.glow_mode = RendererSceneRender::environment_get_glow_blend_mode(p_render_data->environment);
			tonemap.glow_intensity =
				tonemap.glow_mode == RSE::ENV_GLOW_BLEND_MODE_MIX
					? RendererSceneRender::environment_get_glow_mix(p_render_data->environment)
					: RendererSceneRender::environment_get_glow_intensity(p_render_data->environment);
			for (int i = 0; i < RSE::MAX_GLOW_LEVELS; i++) {
				tonemap.glow_levels[i] = RendererSceneRender::environment_get_glow_levels(p_render_data->environment)[i];
			}

			Size2i msize = rb->get_texture_slice_size(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, 0);
			tonemap.glow_texture_size.x = msize.width;
			tonemap.glow_texture_size.y = msize.height;
			tonemap.glow_use_bicubic_upscale = data->glow_bicubic_upscale;

			if (can_use_storage) {
				tonemap.glow_texture = rb->get_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1);
			}
			else {
				tonemap.glow_texture = rb->get_texture_slice(
					RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 0, 2, rb->get_view_count());
			}

			if (RendererSceneRender::environment_get_glow_map(p_render_data->environment).is_valid()) {
				tonemap.glow_map_strength =
					RendererSceneRender::environment_get_glow_map_strength(p_render_data->environment);
				tonemap.glow_map = RendererRD::TextureStorage::texture_get_rd_texture(
					RendererSceneRender::environment_get_glow_map(p_render_data->environment));
			}
			else {
				tonemap.glow_map_strength = 0.0f;
				tonemap.glow_map = RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
			}

		}
		else {
			tonemap.glow_texture = RendererRD::TextureStorage::texture_rd_get_default(
				rb->get_view_count() > 1
					? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK
					: RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK);
			tonemap.glow_map = RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
		}

		tonemap.use_fxaa = use_fxaa;
		tonemap.texture_size = Vector2i(color_size.x, color_size.y);

		if (p_render_data->environment.is_valid()) {
			// When we are using RGB10A2 render buffer format, our scene
			// is limited to a maximum of 2.0. In this case we should limit
			// the max white of tonemappers, specifically AgX which defaults
			// to a high white value.
			bool limit_agx_white =
				rb->get_base_data_format() == RDC::DATA_FORMAT_A2B10G10R10_UNORM_PACK32;

			// When using HDR 2D, we use the parent window's output max value.
			// Otherwise, we're tonemapping to an SDR low bit depth buffer, so
			// we need to use SDR range with a max value of 1.0.
			float max_value = using_hdr ? p_render_data->window_output_max_value : 1.0;

			tonemap.tonemap_mode = RendererSceneRender::environment_get_tone_mapper(p_render_data->environment);
			RendererEnvironmentStorage::TonemapParameters params =
				RendererSceneRender::environment_get_tonemap_parameters(
					p_render_data->environment, limit_agx_white, max_value);
			tonemap.tonemapper_params[0] = params.tonemapper_params[0];
			tonemap.tonemapper_params[1] = params.tonemapper_params[1];
			tonemap.tonemapper_params[2] = params.tonemapper_params[2];
			tonemap.tonemapper_params[3] = params.tonemapper_params[3];
			tonemap.white =
				RendererSceneRender::environment_get_white(p_render_data->environment, limit_agx_white, max_value);
			tonemap.exposure = RendererSceneRender::environment_get_exposure(p_render_data->environment);
			tonemap.max_value = max_value;
		}

		tonemap.use_color_correction = false;
		tonemap.use_1d_color_correction = false;
		tonemap.color_correction_texture = RendererRD::TextureStorage::texture_rd_get_default(
			RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		tonemap.convert_to_srgb = !using_hdr;

		if (can_use_effects && p_render_data->environment.is_valid()) {
			tonemap.use_bcs = RendererSceneRender::environment_get_adjustments_enabled(p_render_data->environment);
			tonemap.brightness = RendererSceneRender::environment_get_adjustments_brightness(p_render_data->environment);
			tonemap.contrast = RendererSceneRender::environment_get_adjustments_contrast(p_render_data->environment);
			tonemap.saturation = RendererSceneRender::environment_get_adjustments_saturation(p_render_data->environment);
			if (RendererSceneRender::environment_get_adjustments_enabled(p_render_data->environment) &&
				RendererSceneRender::environment_get_color_correction(p_render_data->environment).is_valid()) {
				tonemap.use_color_correction = true;
				tonemap.use_1d_color_correction =
					RendererSceneRender::environment_get_use_1d_color_correction(p_render_data->environment);
				tonemap.color_correction_texture = RendererRD::TextureStorage::texture_get_rd_texture(
					RendererSceneRender::environment_get_color_correction(p_render_data->environment),
					!tonemap.convert_to_srgb);
			}
		}

		tonemap.luminance_multiplier = rb->get_luminance_multiplier();
		tonemap.view_count = rb->get_view_count();

		RID dest_fb;
		RDC::DataFormat dest_fb_format;
		if (using_scaling_pass || use_smaa) {
			// If we use a spatial upscaler to upscale or data->SMAA to antialias we need to write our
			// result into an intermediate buffer. Note that this is cached so we only create the
			// texture the first time.
			dest_fb_format = rb->get_base_data_format();
			RID dest_texture =
				rb->create_texture(SNAME("Tonemapper"), SNAME("destination"), dest_fb_format,
					RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
						RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
					RDC::TEXTURE_SAMPLES_1, color_size, 0, 1, true, true);
			dest_fb = FramebufferCacheRD::get_cache(dest_texture);
			tonemap.dest_texture_size = color_size;
		}
		else {
			// If we do a nearest or bilinear upscale, we just render into our render target and our
			// shader will upscale automatically. Target size in this case is lying as we never get
			// our real target size communicated. Bit nasty but...

			if (dest_is_msaa_2d) {
				dest_fb = FramebufferCacheRD::get_cache(
					RendererRD::TextureStorage::render_target_get_rd_texture_msaa(render_target));
				RendererRD::TextureStorage::render_target_set_msaa_needs_resolve(
					render_target, true); // Make sure this gets resolved.
			}
			else {
				dest_fb = RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target);
			}

			tonemap.dest_texture_size = RendererRD::TextureStorage::render_target_get_size(render_target);
			tonemap.bilinear_filtering = scale_mode != RSE::VIEWPORT_SCALING_3D_MODE_NEAREST;
		}

		tonemap.debanding_mode =
			RendererRD::ToneMapper::TonemapSettings::DebandingMode::DEBANDING_MODE_DISABLED;
		if (rb->get_use_debanding() && !using_hdr) {
			if (use_smaa) {
				// data->SMAA will apply 8-bit debanding.
				if (!can_use_storage) {
					// When can_use_storage is false, the intermediate destination buffer will be 10
					// bit, so additional tonemap debanding is needed. This stage of debanding is
					// especially needed for glow effects.
					tonemap.debanding_mode = RendererRD::ToneMapper::TonemapSettings::
						DebandingMode::DEBANDING_MODE_10_BIT;
				}
			}
			else {
				// The final destination buffer of this viewport will be 8-bit integer, so match
				// that format.
				tonemap.debanding_mode =
					RendererRD::ToneMapper::TonemapSettings::DebandingMode::DEBANDING_MODE_8_BIT;
			}
		}

		if (can_use_storage) {
			data->tone_mapper->tonemapper(color_texture, dest_fb, tonemap);
		}
		else {
			data->tone_mapper->tonemapper_mobile(color_texture, dest_fb, tonemap);
		}

		RD::draw_command_end_label();
	}

	if (use_smaa) {
		RENDER_TIMESTAMP("data->SMAA");
		RD::draw_command_begin_label("data->SMAA");

		bool using_hdr = RendererRD::TextureStorage::render_target_is_using_hdr(render_target);

		RID dest_fb;
		if (using_scaling_pass) {
			rb->create_texture(SNAME("data->SMAA"), SNAME("destination"), rb->get_base_data_format(),
				RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
					RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
				RDC::TEXTURE_SAMPLES_1, Size2i(), 0, 1, true, true);
		}
		if (rb->get_view_count() > 1) {
			for (uint32_t v = 0; v < rb->get_view_count(); v++) {
				RID source_texture =
					rb->get_texture_slice(SNAME("Tonemapper"), SNAME("destination"), v, 0);

				RID dest_texture;
				if (using_scaling_pass) {
					dest_texture = rb->get_texture_slice(SNAME("data->SMAA"), SNAME("destination"), v, 0);
				}
				else {
					dest_texture =
						RendererRD::TextureStorage::render_target_get_rd_texture_slice(render_target, v);
				}
				dest_fb = FramebufferCacheRD::get_cache(dest_texture);

				data->smaa->process(rb, source_texture, dest_fb, rb->get_use_debanding() && !using_hdr);
			}
		}
		else {
			RID source_texture = rb->get_texture(SNAME("Tonemapper"), SNAME("destination"));

			if (using_scaling_pass) {
				RID dest_texture = rb->create_texture(SNAME("data->SMAA"), SNAME("destination"),
					rb->get_base_data_format(),
					RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
						RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
					RDC::TEXTURE_SAMPLES_1, Size2i(), 0, 1, true, true);
				dest_fb = FramebufferCacheRD::get_cache(dest_texture);
			}
			else {
				if (dest_is_msaa_2d) {
					dest_fb = FramebufferCacheRD::get_cache(
						RendererRD::TextureStorage::render_target_get_rd_texture_msaa(render_target));
					RendererRD::TextureStorage::render_target_set_msaa_needs_resolve(
						render_target, true); // Make sure this gets resolved.
				}
				else {
					dest_fb = RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target);
				}
			}

			data->smaa->process(rb, source_texture, dest_fb, rb->get_use_debanding() && !using_hdr);
		}

		RD::draw_command_end_label();
	}

	if (rb.is_valid() && using_scaling_pass) {
		RD::draw_command_begin_label(
			spatial_upscaler ? spatial_upscaler->get_label() : "3D Viewport Scaling");

		if (spatial_upscaler) {
			spatial_upscaler->ensure_context(rb);

			for (uint32_t v = 0; v < rb->get_view_count(); v++) {
				RID source_texture = rb->get_texture_slice(
					use_smaa ? SNAME("data->SMAA") : SNAME("Tonemapper"), SNAME("destination"), v, 0);
				RID dest_texture =
					RendererRD::TextureStorage::render_target_get_rd_texture_slice(render_target, v);

				spatial_upscaler->process(rb, source_texture, dest_texture);
			}
		}
		else {
			// If no spatial upscaler is set, we are using bilinear or nearest filtering for
			// scaling. This is only necessary if screen-space antialiasing is active, otherwise
			// this happens automatically during tonemapping.
			RID source_texture = rb->get_texture(
				use_smaa ? SNAME("data->SMAA") : SNAME("Tonemapper"), SNAME("destination"));
			RID dest_texture = RendererRD::TextureStorage::render_target_get_rd_texture(render_target);
			RID dest_fb = FramebufferCacheRD::get_cache(dest_texture);
			RendererRD::CopyEffects::copy_to_fb_rect(source_texture, dest_fb,
				Rect2i(Point2i(), rb->get_target_size()), false, false, false, false, RID(),
				rb->get_view_count() > 1, false, false, false, Rect2(), 1.0,
				scale_mode != RSE::VIEWPORT_SCALING_3D_MODE_NEAREST);
		}

		if (dest_is_msaa_2d) {
			// We can't upscale directly into our MSAA buffer so we need to do a copy
			RID source_texture = RendererRD::TextureStorage::render_target_get_rd_texture(render_target);
			RID dest_fb = FramebufferCacheRD::get_cache(
				RendererRD::TextureStorage::render_target_get_rd_texture_msaa(render_target));
			RendererRD::CopyEffects::copy_to_fb_rect(
				source_texture, dest_fb, Rect2i(Point2i(), rb->get_target_size()));

			RendererRD::TextureStorage::render_target_set_msaa_needs_resolve(
				render_target, true); // Make sure this gets resolved.
		}

		RD::draw_command_end_label();
	}

	RendererRD::TextureStorage::render_target_disable_clear_request(render_target);
}

void RendererSceneRenderRD::_post_process_subpass(
	RID p_source_texture, RID p_framebuffer, const RenderDataRD* p_render_data)
{
	RD::draw_command_begin_label("Post Process Subpass");

	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	// FIXME: Our input it our internal_texture, shouldn't this be using internal_size ??
	// Seeing we don't support data->FSR in our mobile renderer right now target_size = internal_size...
	Size2i target_size = rb->get_target_size();
	bool can_use_effects =
		target_size.x >= 8 && target_size.y >= 8 && data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_DISABLED;

	RD::DrawListID draw_list = RD::draw_list_switch_to_next_pass();

	RendererRD::ToneMapper::TonemapSettings tonemap;

	bool using_hdr = RendererRD::TextureStorage::render_target_is_using_hdr(rb->get_render_target());

	if (p_render_data->environment.is_valid()) {
		// When we are using RGB10A2 render buffer format, our scene
		// is limited to a maximum of 2.0. In this case we should limit
		// the max white of tonemappers, specifically AgX which defaults
		// to a high white value.
		bool limit_agx_white =
			rb->get_base_data_format() == RDC::DATA_FORMAT_A2B10G10R10_UNORM_PACK32;

		// When using HDR 2D, we use the parent window's output max value.
		// Otherwise, we're tonemapping to an SDR low bit depth buffer, so
		// we need to use SDR range with a max value of 1.0.
		float max_value = using_hdr ? p_render_data->window_output_max_value : 1.0;

		tonemap.tonemap_mode = RendererSceneRender::environment_get_tone_mapper(p_render_data->environment);
		RendererEnvironmentStorage::TonemapParameters params = RendererSceneRender::environment_get_tonemap_parameters(
			p_render_data->environment, limit_agx_white, max_value);
		tonemap.tonemapper_params[0] = params.tonemapper_params[0];
		tonemap.tonemapper_params[1] = params.tonemapper_params[1];
		tonemap.tonemapper_params[2] = params.tonemapper_params[2];
		tonemap.tonemapper_params[3] = params.tonemapper_params[3];
		tonemap.exposure = RendererSceneRender::environment_get_exposure(p_render_data->environment);
		tonemap.white =
			RendererSceneRender::environment_get_white(p_render_data->environment, limit_agx_white, max_value);
		tonemap.max_value = max_value;
	}

	// We don't support glow or auto exposure here, if they are needed, don't use subpasses!
	// The problem is that we need to use the result so far and process them before we can
	// apply this to our results.
	if (can_use_effects && p_render_data->environment.is_valid() &&
		RendererSceneRender::environment_get_glow_enabled(p_render_data->environment)) {
		ERR_FAIL_MSG("Glow is not supported when using subpasses.");
	}

	if (can_use_effects && RendererCameraAttributes::camera_attributes_uses_auto_exposure(
							   p_render_data->camera_attributes)) {
		ERR_FAIL_MSG("Auto Exposure is not supported when using subpasses.");
	}

	tonemap.use_glow = false;
	tonemap.glow_texture = RendererRD::TextureStorage::texture_rd_get_default(
		rb->get_view_count() > 1 ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK
								 : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK);
	tonemap.glow_map = RendererRD::TextureStorage::texture_rd_get_default(
		RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
	tonemap.use_auto_exposure = false;
	tonemap.exposure_texture = RendererRD::TextureStorage::texture_rd_get_default(
		RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);

	tonemap.use_color_correction = false;
	tonemap.use_1d_color_correction = false;
	tonemap.color_correction_texture = RendererRD::TextureStorage::texture_rd_get_default(
		RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
	tonemap.convert_to_srgb = !using_hdr;

	if (can_use_effects && p_render_data->environment.is_valid()) {
		tonemap.use_bcs = RendererSceneRender::environment_get_adjustments_enabled(p_render_data->environment);
		tonemap.brightness = RendererSceneRender::environment_get_adjustments_brightness(p_render_data->environment);
		tonemap.contrast = RendererSceneRender::environment_get_adjustments_contrast(p_render_data->environment);
		tonemap.saturation = RendererSceneRender::environment_get_adjustments_saturation(p_render_data->environment);
		if (RendererSceneRender::environment_get_adjustments_enabled(p_render_data->environment) &&
			RendererSceneRender::environment_get_color_correction(p_render_data->environment).is_valid()) {
			tonemap.use_color_correction = true;
			tonemap.use_1d_color_correction =
				RendererSceneRender::environment_get_use_1d_color_correction(p_render_data->environment);
			tonemap.color_correction_texture = RendererRD::TextureStorage::texture_get_rd_texture(
				RendererSceneRender::environment_get_color_correction(p_render_data->environment),
				!tonemap.convert_to_srgb);
		}
	}

	tonemap.texture_size = Vector2i(target_size.x, target_size.y);

	tonemap.luminance_multiplier = rb->get_luminance_multiplier();
	tonemap.view_count = rb->get_view_count();

	if (rb->get_use_debanding() && !using_hdr) {
		tonemap.debanding_mode =
			RendererRD::ToneMapper::TonemapSettings::DebandingMode::DEBANDING_MODE_8_BIT;
	}
	else {
		tonemap.debanding_mode =
			RendererRD::ToneMapper::TonemapSettings::DebandingMode::DEBANDING_MODE_DISABLED;
	}

	data->tone_mapper->tonemapper_subpass(draw_list, p_source_texture,
		RD::framebuffer_get_format(p_framebuffer), tonemap);

	RD::draw_command_end_label();
}

void RendererSceneRenderRD::_disable_clear_request(const RenderDataRD* p_render_data)
{
	ERR_FAIL_COND(p_render_data->render_buffers.is_null());

	RendererRD::TextureStorage::render_target_disable_clear_request(
		p_render_data->render_buffers->get_render_target());
}

bool RendererSceneRenderRD::_debug_draw_can_use_effects(RSE::ViewportDebugDraw p_debug_draw)
{
	bool can_use_effects = true;
	switch (p_debug_draw) {
	// No debug draw, use camera effects
	case RSE::VIEWPORT_DEBUG_DRAW_DISABLED:
		can_use_effects = true;
		break;
	// Modes that completely override rendering to draw debug information should disable camera
	// effects.
	case RSE::VIEWPORT_DEBUG_DRAW_UNSHADED:
	case RSE::VIEWPORT_DEBUG_DRAW_OVERDRAW:
	case RSE::VIEWPORT_DEBUG_DRAW_WIREFRAME:
	case RSE::VIEWPORT_DEBUG_DRAW_VOXEL_GI_ALBEDO:
	case RSE::VIEWPORT_DEBUG_DRAW_CLUSTER_OMNI_LIGHTS:
	case RSE::VIEWPORT_DEBUG_DRAW_CLUSTER_SPOT_LIGHTS:
	case RSE::VIEWPORT_DEBUG_DRAW_CLUSTER_AREA_LIGHTS:
	case RSE::VIEWPORT_DEBUG_DRAW_CLUSTER_DECALS:
	case RSE::VIEWPORT_DEBUG_DRAW_CLUSTER_REFLECTION_PROBES:
	case RSE::VIEWPORT_DEBUG_DRAW_INTERNAL_BUFFER:
		can_use_effects = false;
		break;
	// Modes that draws information over part of the viewport needs camera effects because we see
	// partially the normal draw mode.
	case RSE::VIEWPORT_DEBUG_DRAW_SHADOW_ATLAS:
	case RSE::VIEWPORT_DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS:
	case RSE::VIEWPORT_DEBUG_DRAW_DECAL_ATLAS:
	case RSE::VIEWPORT_DEBUG_DRAW_AREA_LIGHT_ATLAS:
	case RSE::VIEWPORT_DEBUG_DRAW_MOTION_VECTORS:
	// Modes that draws a buffer over viewport needs camera effects because if the buffer is not
	// available it will be equivalent to normal draw mode.
	case RSE::VIEWPORT_DEBUG_DRAW_NORMAL_BUFFER:
	case RSE::VIEWPORT_DEBUG_DRAW_SSAO:
	case RSE::VIEWPORT_DEBUG_DRAW_SSIL:
	case RSE::VIEWPORT_DEBUG_DRAW_SDFGI:
	case RSE::VIEWPORT_DEBUG_DRAW_GI_BUFFER:
	case RSE::VIEWPORT_DEBUG_DRAW_OCCLUDERS:
		can_use_effects = true;
		break;
	// Other debug draw modes keep camera effects.
	case RSE::VIEWPORT_DEBUG_DRAW_LIGHTING:
	case RSE::VIEWPORT_DEBUG_DRAW_VOXEL_GI_LIGHTING:
	case RSE::VIEWPORT_DEBUG_DRAW_VOXEL_GI_EMISSION:
	case RSE::VIEWPORT_DEBUG_DRAW_SCENE_LUMINANCE:
	case RSE::VIEWPORT_DEBUG_DRAW_PSSM_SPLITS:
	case RSE::VIEWPORT_DEBUG_DRAW_SDFGI_PROBES:
	case RSE::VIEWPORT_DEBUG_DRAW_DISABLE_LOD:
		can_use_effects = true;
		break;
	default:
		break;
	}

	return can_use_effects;
}

void RendererSceneRenderRD::_render_buffers_debug_draw(const RenderDataRD* p_render_data)
{
	Ref<RenderSceneBuffersRD> rb = p_render_data->render_buffers;
	ERR_FAIL_COND(rb.is_null());

	RID render_target = rb->get_render_target();

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_SHADOW_ATLAS) {
		if (p_render_data->shadow_atlas.is_valid()) {
			RID shadow_atlas_texture =
				RendererRD::LightStorage::shadow_atlas_get_texture(
					p_render_data->shadow_atlas);

			if (shadow_atlas_texture.is_null()) {
				shadow_atlas_texture = RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK);
			}

			Size2 rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);
			RendererRD::CopyEffects::copy_to_fb_rect(shadow_atlas_texture,
				RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
				Rect2i(Vector2(), rtsize / 2), false, true);
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS) {
		if (RendererRD::LightStorage::directional_shadow_get_texture()
				.is_valid()) {
			RID shadow_atlas_texture =
				RendererRD::LightStorage::directional_shadow_get_texture();
			Size2i rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);
			RID dest_fb = RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target);

			// Determine our display size, try and keep square by using the smallest edge.
			Size2i size = 2 * rtsize / 3;
			if (size.x < size.y) {
				size.y = size.x;
			}
			else if (size.y < size.x) {
				size.x = size.y;
			}

			RendererRD::CopyEffects::copy_to_fb_rect(
				shadow_atlas_texture, dest_fb, Rect2i(Vector2(), size), false, true);

			// Visualize our view frustum to show coverage.
			for (int i = 0; i < p_render_data->render_shadow_count; i++) {
				RID light = p_render_data->render_shadows[i].light;
				RID base = RendererRD::LightStorage::light_instance_get_base_light(light);

				if (RendererRD::LightStorage::light_get_type(base) == RSE::LIGHT_DIRECTIONAL) {
					data->debug_effects->draw_shadow_frustum(light,
						p_render_data->scene_data->cam_projection,
						p_render_data->scene_data->cam_transform, dest_fb, Rect2(Size2(), size));
				}
			}
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_DECAL_ATLAS) {
		RID decal_atlas = RendererRD::TextureStorage::decal_atlas_get_texture();

		if (decal_atlas.is_valid()) {
			Size2i rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);

			RendererRD::CopyEffects::copy_to_fb_rect(decal_atlas,
				RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
				Rect2i(Vector2(), rtsize / 2), false, false, true);
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_AREA_LIGHT_ATLAS) {
		RID area_light_atlas =
			RendererRD::TextureStorage::area_light_atlas_get_texture();

		if (area_light_atlas.is_valid()) {
			Size2i rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);

			RendererRD::CopyEffects::copy_to_fb_rect(area_light_atlas,
				RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
				Rect2i(Vector2(), rtsize / 2), false, false, true);
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_SCENE_LUMINANCE) {
		RID luminance_texture = data->luminance->get_current_luminance_buffer(rb);
		if (luminance_texture.is_valid()) {
			Size2i rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);

			RendererRD::CopyEffects::copy_to_fb_rect(luminance_texture,
				RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
				Rect2(Vector2(), rtsize / 8), false, true);
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_INTERNAL_BUFFER) {
		Size2 rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);
		RendererRD::CopyEffects::copy_to_fb_rect(rb->get_internal_texture(),
			RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
			Rect2(Vector2(), rtsize), false, false);
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_NORMAL_BUFFER &&
		_render_buffers_get_normal_texture(rb).is_valid()) {
		Size2 rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);
		RendererRD::CopyEffects::copy_to_fb_rect(_render_buffers_get_normal_texture(rb),
			RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
			Rect2(Vector2(), rtsize), false, false, false, false, RID(), false, false, false, true);
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_OCCLUDERS) {
		if (p_render_data->occluder_debug_tex.is_valid()) {
			Size2i rtsize = RendererRD::TextureStorage::render_target_get_size(render_target);
			RendererRD::CopyEffects::copy_to_fb_rect(
				RendererRD::TextureStorage::texture_get_rd_texture(p_render_data->occluder_debug_tex),
				RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target),
				Rect2i(Vector2(), rtsize), true, false);
		}
	}

	if (data->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_MOTION_VECTORS &&
		_render_buffers_get_velocity_texture(rb).is_valid()) {
		RID velocity = _render_buffers_get_velocity_texture(rb);
		RID depth = rb->get_depth_texture();
		RID dest_fb = RendererRD::TextureStorage::render_target_get_rd_framebuffer(render_target);
		Size2i resolution = rb->get_internal_size();

		data->debug_effects->draw_motion_vectors(velocity, depth, dest_fb,
			p_render_data->scene_data->cam_projection, p_render_data->scene_data->cam_transform,
			p_render_data->scene_data->prev_cam_projection,
			p_render_data->scene_data->prev_cam_transform, resolution);
	}
}

RID RendererSceneRenderRD::render_buffers_get_default_voxel_gi_buffer()
{
	return RendererRD::GI::default_voxel_gi_buffer;
}

RDC::DataFormat RendererSceneRenderRD::_render_buffers_get_preferred_color_format()
{
	return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
}

bool RendererSceneRenderRD::_render_buffers_can_be_storage() { return true; }

void RendererSceneRenderRD::gi_set_use_half_resolution(bool p_enable)
{
	RendererRD::GI::half_resolution = p_enable;
}

void RendererSceneRenderRD::positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality)
{
	ERR_FAIL_INDEX_MSG(p_quality, RSE::SHADOW_QUALITY_MAX,
		"Shadow quality too high, please see Renderer's ShadowQuality enum");

	if (data->shadows_quality != p_quality) {
		data->shadows_quality = p_quality;

		switch (data->shadows_quality) {
		case RSE::SHADOW_QUALITY_HARD: {
			data->penumbra_shadow_samples = 4;
			data->soft_shadow_samples = 0;
			data->shadows_quality_radius = 1.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_VERY_LOW: {
			data->penumbra_shadow_samples = 4;
			data->soft_shadow_samples = 1;
			data->shadows_quality_radius = 1.5;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_LOW: {
			data->penumbra_shadow_samples = 8;
			data->soft_shadow_samples = 4;
			data->shadows_quality_radius = 2.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_MEDIUM: {
			data->penumbra_shadow_samples = 12;
			data->soft_shadow_samples = 8;
			data->shadows_quality_radius = 2.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_HIGH: {
			data->penumbra_shadow_samples = 24;
			data->soft_shadow_samples = 16;
			data->shadows_quality_radius = 3.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_ULTRA: {
			data->penumbra_shadow_samples = 32;
			data->soft_shadow_samples = 32;
			data->shadows_quality_radius = 4.0;
		} break;
		case RSE::SHADOW_QUALITY_MAX:
			break;
		}
		get_vogel_disk(data->penumbra_shadow_kernel, data->penumbra_shadow_samples);
		get_vogel_disk(data->soft_shadow_kernel, data->soft_shadow_samples);
	}

	_update_shader_quality_settings();
}

void RendererSceneRenderRD::directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality)
{
	ERR_FAIL_INDEX_MSG(p_quality, RSE::SHADOW_QUALITY_MAX,
		"Shadow quality too high, please see Renderer's ShadowQuality enum");

	if (data->directional_shadow_quality != p_quality) {
		data->directional_shadow_quality = p_quality;

		switch (data->directional_shadow_quality) {
		case RSE::SHADOW_QUALITY_HARD: {
			data->directional_penumbra_shadow_samples = 4;
			data->directional_soft_shadow_samples = 0;
			data->directional_shadow_quality_radius = 1.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_VERY_LOW: {
			data->directional_penumbra_shadow_samples = 4;
			data->directional_soft_shadow_samples = 1;
			data->directional_shadow_quality_radius = 1.5;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_LOW: {
			data->directional_penumbra_shadow_samples = 8;
			data->directional_soft_shadow_samples = 4;
			data->directional_shadow_quality_radius = 2.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_MEDIUM: {
			data->directional_penumbra_shadow_samples = 12;
			data->directional_soft_shadow_samples = 8;
			data->directional_shadow_quality_radius = 2.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_HIGH: {
			data->directional_penumbra_shadow_samples = 24;
			data->directional_soft_shadow_samples = 16;
			data->directional_shadow_quality_radius = 3.0;
		} break;
		case RSE::SHADOW_QUALITY_SOFT_ULTRA: {
			data->directional_penumbra_shadow_samples = 32;
			data->directional_soft_shadow_samples = 32;
			data->directional_shadow_quality_radius = 4.0;
		} break;
		case RSE::SHADOW_QUALITY_MAX:
			break;
		}
		get_vogel_disk(data->directional_penumbra_shadow_kernel, data->directional_penumbra_shadow_samples);
		get_vogel_disk(data->directional_soft_shadow_kernel, data->directional_soft_shadow_samples);
	}

	_update_shader_quality_settings();
}

void RendererSceneRenderRD::decals_set_filter(RSE::DecalFilter p_filter)
{
	if (data->decals_filter == p_filter) {
		return;
	}
	data->decals_filter = p_filter;
	_update_shader_quality_settings();
}

void RendererSceneRenderRD::light_projectors_set_filter(RSE::LightProjectorFilter p_filter)
{
	if (data->light_projectors_filter == p_filter) {
		return;
	}
	data->light_projectors_filter = p_filter;
	_update_shader_quality_settings();
}

void RendererSceneRenderRD::lightmaps_set_bicubic_filter(bool p_enable)
{
	if (data->lightmap_filter_bicubic == p_enable) {
		return;
	}
	data->lightmap_filter_bicubic = p_enable;
	_update_shader_quality_settings();
}

void RendererSceneRenderRD::material_set_use_debanding(bool p_enable)
{
	data->material_use_debanding = p_enable;
	_update_shader_quality_settings();
}

int RendererSceneRenderRD::get_roughness_layers() { return data->sky.roughness_layers; }

bool RendererSceneRenderRD::is_using_radiance_octmap_array()
{
	return data->sky.sky_use_octmap_array;
}

void RendererSceneRenderRD::_update_vrs(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers.is_null()) {
		return;
	}

	RID render_target = p_render_buffers->get_render_target();
	if (render_target.is_null()) {
		// must be rendering reflection probes
		return;
	}

	if (data->vrs) {
		RSE::ViewportVRSMode vrs_mode = RendererRD::TextureStorage::render_target_get_vrs_mode(render_target);
		if (vrs_mode != RSE::VIEWPORT_VRS_DISABLED) {
			RID vrs_texture = p_render_buffers->get_texture(RB_SCOPE_VRS, RB_TEXTURE);

			// We use get_cache_multipass instead of get_cache_multiview because the default
			// behavior is for our vrs_texture to be used as the data->VRS attachment. In this particular
			// case we're writing to it so it needs to be set as our color attachment

			Vector<RID> textures;
			textures.push_back(vrs_texture);

			Vector<RD::FramebufferPass> passes;
			RD::FramebufferPass pass;
			pass.color_attachments.push_back(0);
			passes.push_back(pass);

			RID vrs_fb = FramebufferCacheRD::get_cache_multipass(
				textures, passes, p_render_buffers->get_view_count());

			data->vrs->update_vrs_texture(vrs_fb, p_render_buffers->get_render_target());
		}
	}
}

bool RendererSceneRenderRD::_needs_post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi)
{
	if (p_render_data->render_buffers.is_valid()) {
		if (p_render_data->render_buffers->has_custom_data(RB_SCOPE_SDFGI)) {
			return true;
		}
	}
	return false;
}

void RendererSceneRenderRD::_post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi)
{
	if (p_render_data->render_buffers.is_valid() && p_use_gi) {
		if (!p_render_data->render_buffers->has_custom_data(RB_SCOPE_SDFGI)) {
			return;
		}

		Ref<RendererRD::GI::SDFGI> sdfgi =
			p_render_data->render_buffers->get_custom_data(RB_SCOPE_SDFGI);
		sdfgi->update_probes(p_render_data->environment,
			data->sky.sky_owner.get_or_null(RendererSceneRender::environment_get_sky(p_render_data->environment)));
	}
}

void RendererSceneRenderRD::render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
	const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
	const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
	const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
	const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
	const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
	RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas,
	RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
	const RenderShadowData* p_render_shadows, int p_render_shadow_count,
	const RenderSDFGIData* p_render_sdfgi_regions, int p_render_sdfgi_region_count,
	float p_window_output_max_value, const RenderSDFGIUpdateData* p_sdfgi_update_data,
	RenderingServerTypes::RenderInfo* r_render_info)
{
	// getting this here now so we can direct call a bunch of things more easily
	ERR_FAIL_COND(p_render_buffers.is_null());
	Ref<RenderSceneBuffersRD> rb = p_render_buffers;
	ERR_FAIL_COND(rb.is_null());

	// setup scene data
	RenderSceneDataRD scene_data;
	{
		// Our first camera is used by default
		scene_data.cam_transform = p_camera_data->main_transform;
		scene_data.cam_projection = p_camera_data->main_projection;
		scene_data.cam_orthogonal = p_camera_data->is_orthogonal;
		scene_data.camera_visible_layers = p_camera_data->visible_layers;
		scene_data.taa_jitter = p_camera_data->taa_jitter;

		scene_data.taa_frame_count = p_camera_data->taa_frame_count;
		scene_data.main_cam_transform = p_camera_data->main_transform;
		scene_data.flip_y = !p_reflection_probe.is_valid();

		scene_data.view_count = p_camera_data->view_count;
		for (uint32_t v = 0; v < p_camera_data->view_count; v++) {
			scene_data.view_eye_offset[v] = p_camera_data->view_offset[v].origin;
			scene_data.view_projection[v] = p_camera_data->view_projection[v];
		}

		scene_data.prev_cam_transform = p_prev_camera_data->main_transform;
		scene_data.prev_cam_projection = p_prev_camera_data->main_projection;
		scene_data.prev_taa_jitter = p_prev_camera_data->taa_jitter;

		for (uint32_t v = 0; v < p_camera_data->view_count; v++) {
			scene_data.prev_view_projection[v] = p_prev_camera_data->view_projection[v];
		}

		scene_data.z_near = p_camera_data->main_projection.get_z_near();
		scene_data.z_far = p_camera_data->main_projection.get_z_far();

		// this should be the same for all cameras..
		const float lod_distance_multiplier = p_camera_data->main_projection.get_lod_multiplier();

		// Also, take into account resolution scaling for the multiplier, since we have more leeway
		// with quality degradation visibility. Conversely, allow upwards scaling, too, for
		// increased mesh detail at high res.
		const float scaling_3d_scale = GLOBAL_GET_CACHED(float, "rendering/scaling_3d/scale");
		scene_data.lod_distance_multiplier = lod_distance_multiplier * (1.0 / scaling_3d_scale);

		if (get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_DISABLE_LOD) {
			scene_data.screen_mesh_lod_threshold = 0.0;
		}
		else {
			scene_data.screen_mesh_lod_threshold = p_screen_mesh_lod_threshold;
		}

		if (p_shadow_atlas.is_valid()) {
			int shadow_atlas_size = RendererRD::LightStorage::shadow_atlas_get_size(p_shadow_atlas);
			scene_data.shadow_atlas_pixel_size.x = 1.0 / shadow_atlas_size;
			scene_data.shadow_atlas_pixel_size.y = 1.0 / shadow_atlas_size;
		}
		{
			int directional_shadow_size = RendererRD::LightStorage::directional_shadow_get_size();
			scene_data.directional_shadow_pixel_size.x = 1.0 / directional_shadow_size;
			scene_data.directional_shadow_pixel_size.y = 1.0 / directional_shadow_size;
		}

		if (p_environment.is_valid()) {
			RID sky_rid = RendererSceneRender::environment_get_sky(p_environment);
			if (sky_rid.is_valid()) {
				int radiance_size = data->sky.sky_get_radiance_size(sky_rid);
				scene_data.radiance_pixel_size = 1.0f / radiance_size;
				float uv_border_size = data->sky.sky_get_uv_border_size(sky_rid);
				scene_data.radiance_border_size = uv_border_size;
			}
		}

		if (p_reflection_atlas.is_valid()) {
			float border_size = RendererRD::LightStorage::reflection_atlas_get_border_size(p_reflection_atlas);
			scene_data.reflection_atlas_border_size.x = border_size;
			scene_data.reflection_atlas_border_size.y = 1.0f - border_size * 2.0;
		}

		scene_data.time = data->time;
		scene_data.time_step = data->time_step;
	}

	// assign render data
	RenderDataRD render_data;
	{
		render_data.render_buffers = rb;
		render_data.scene_data = &scene_data;

		render_data.instances = &p_instances;
		render_data.lights = &p_lights;
		render_data.reflection_probes = &p_reflection_probes;
		render_data.voxel_gi_instances = &p_voxel_gi_instances;
		render_data.decals = &p_decals;
		render_data.lightmaps = &p_lightmaps;
		render_data.fog_volumes = &p_fog_volumes;
		render_data.environment = p_environment;
		render_data.compositor = p_compositor;
		render_data.camera_attributes = p_camera_attributes;
		render_data.shadow_atlas = p_shadow_atlas;
		render_data.occluder_debug_tex = p_occluder_debug_tex;
		render_data.reflection_atlas = p_reflection_atlas;
		render_data.reflection_probe = p_reflection_probe;
		render_data.reflection_probe_pass = p_reflection_probe_pass;

		render_data.render_shadows = p_render_shadows;
		render_data.render_shadow_count = p_render_shadow_count;
		render_data.render_sdfgi_regions = p_render_sdfgi_regions;
		render_data.render_sdfgi_region_count = p_render_sdfgi_region_count;
		render_data.sdfgi_update_data = p_sdfgi_update_data;
		render_data.window_output_max_value = p_window_output_max_value;

		render_data.render_info = r_render_info;

		if (p_render_buffers.is_valid() && p_reflection_probe.is_null()) {
			render_data.transparent_bg =
				RendererRD::TextureStorage::render_target_get_transparent(rb->get_render_target());
			render_data.render_region =
				RendererRD::TextureStorage::render_target_get_render_region(rb->get_render_target());
		}
	}

	PagedArray<RID> empty;

	if (get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_UNSHADED ||
		get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_OVERDRAW) {
		render_data.lights = &empty;
		render_data.reflection_probes = &empty;
		render_data.voxel_gi_instances = &empty;
		render_data.lightmaps = &empty;
	}

	if (get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_UNSHADED ||
		get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_OVERDRAW ||
		get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_LIGHTING ||
		get_debug_draw_mode() == RSE::VIEWPORT_DEBUG_DRAW_PSSM_SPLITS) {
		render_data.decals = &empty;
	}

	Color clear_color;
	if (p_render_buffers.is_valid() && p_reflection_probe.is_null()) {
		clear_color =
			RendererRD::TextureStorage::render_target_get_clear_request_color(rb->get_render_target());
	}
	else {
		clear_color = RendererTextureStorage::get_default_clear_color();
	}

	// calls _pre_opaque_render between depth pre-pass and opaque pass
	_render_scene(&render_data, clear_color);
}

void RendererSceneRenderRD::render_material(const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal,
	const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
	const Rect2i& p_region)
{
	_render_material(p_cam_transform, p_cam_projection, p_cam_orthogonal, p_instances,
		p_framebuffer, p_region, 1.0);
}

void RendererSceneRenderRD::render_particle_collider_heightfield(RID p_collider,
	const Transform3D& p_transform, const PagedArray<RenderGeometryInstance*>& p_instances)
{
	ERR_FAIL_COND(!RendererRD::ParticlesStorage::particles_collision_is_heightfield(p_collider));
	Vector3 extents = RendererRD::ParticlesStorage::particles_collision_get_extents(p_collider) *
					  p_transform.basis.get_scale();
	Projection cm;
	cm.set_orthogonal(-extents.x, extents.x, -extents.z, extents.z, 0, extents.y * 2.0);

	Vector3 cam_pos = p_transform.origin;
	cam_pos.y += extents.y;

	Transform3D cam_xform;
	cam_xform.set_look_at(cam_pos, cam_pos - p_transform.basis.get_column(Vector3::AXIS_Y),
		-p_transform.basis.get_column(Vector3::AXIS_Z).normalized());

	RID fb = RendererRD::ParticlesStorage::particles_collision_get_heightfield_framebuffer(p_collider);

	_render_particle_collider_heightfield(fb, cam_xform, cm, p_instances);
}

bool RendererSceneRenderRD::free(RID p_rid)
{
	if (RendererSceneRender::is_environment(p_rid)) {
		RendererSceneRender::environment_free(p_rid);
	}
	else if (RendererSceneRender::is_compositor(p_rid)) {
		RendererSceneRender::compositor_free(p_rid);
	}
	else if (RendererSceneRender::is_compositor_effect(p_rid)) {
		RendererSceneRender::compositor_effect_free(p_rid);
	}
	else if (RendererCameraAttributes::owns_camera_attributes(p_rid)) {
		RendererCameraAttributes::camera_attributes_free(p_rid);
	}
	else if (RendererRD::GI::voxel_gi_instance_owns(p_rid)) {
		RendererRD::GI::voxel_gi_instance_free(p_rid);
	}
	else if (data->sky.sky_owner.owns(p_rid)) {
		data->sky.update_dirty_skys();
		data->sky.free_sky(p_rid);
	}
	else if (RendererRD::Fog::owns_fog_volume_instance(p_rid)) {
		RendererRD::Fog::fog_instance_free(p_rid);
	}
	else {
		return false;
	}

	return true;
}

void RendererSceneRenderRD::set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw)
{
	data->debug_draw = p_debug_draw;
}

void RendererSceneRenderRD::update() { data->sky.update_dirty_skys(); }

void RendererSceneRenderRD::set_time(double p_time, double p_step)
{
	data->time = p_time;
	data->time_step = p_step;
}

void RendererSceneRenderRD::screen_space_roughness_limiter_set_active(
	bool p_enable, float p_amount, float p_limit)
{
	data->screen_space_roughness_limiter = p_enable;
	data->screen_space_roughness_limiter_amount = p_amount;
	data->screen_space_roughness_limiter_limit = p_limit;
}

bool RendererSceneRenderRD::screen_space_roughness_limiter_is_active()
{
	return data->screen_space_roughness_limiter;
}

float RendererSceneRenderRD::screen_space_roughness_limiter_get_amount()
{
	return data->screen_space_roughness_limiter_amount;
}

float RendererSceneRenderRD::screen_space_roughness_limiter_get_limit()
{
	return data->screen_space_roughness_limiter_limit;
}

void RendererSceneRenderRD::sdfgi_set_debug_probe_select(
	const Vector3& p_position, const Vector3& p_dir)
{
	RendererRD::GI::sdfgi_debug_probe_pos = p_position;
	RendererRD::GI::sdfgi_debug_probe_dir = p_dir;
}

bool RendererSceneRenderRD::is_vrs_supported()
{
	return RD::has_feature(RDC::SUPPORTS_ATTACHMENT_VRS);
}

bool RendererSceneRenderRD::is_dynamic_gi_supported()
{
	// usable by default (unless low end = true)
	return true;
}

bool RendererSceneRenderRD::is_volumetric_supported()
{
	// usable by default (unless low end = true)
	return true;
}

void RendererSceneRenderRD::initialize()
{
	data = memnew(RendererSceneRenderRD::Data);

	RendererRD::CopyEffects::initialize(0);
	RendererRD::SSEffects::initialize();

	data->bokeh_dof = memnew(RendererRD::BokehDOF(false));
	data->debug_effects = memnew(RendererRD::DebugEffects);
	data->luminance = memnew(RendererRD::Luminance(false));
	data->smaa = memnew(RendererRD::SMAA);
	data->tone_mapper = memnew(RendererRD::ToneMapper(false));
	data->fsr = memnew(RendererRD::FSR);
	data->vrs = memnew(RendererRD::VRS);
	data->resolve_effects = memnew(RendererRD::Resolve(false));
#ifdef METAL_ENABLED
	data->mfx_spatial = memnew(RendererRD::MFXSpatialEffect);
#endif
	data->sky.init();
	RendererRD::GI::initialize(&data->sky);

	_update_shader_quality_settings();
}

void RendererSceneRenderRD::finalize()
{
	if (!data) {
		return;
	}
	RendererRD::GI::free();
	if (data->directional_penumbra_shadow_kernel) {
		memdelete_arr(data->directional_penumbra_shadow_kernel);
	}
	if (data->directional_soft_shadow_kernel) {
		memdelete_arr(data->directional_soft_shadow_kernel);
	}
	if (data->penumbra_shadow_kernel) {
		memdelete_arr(data->penumbra_shadow_kernel);
	}
	if (data->soft_shadow_kernel) {
		memdelete_arr(data->soft_shadow_kernel);
	}
#ifdef METAL_ENABLED
	if (data->mfx_spatial) {
		memdelete(data->mfx_spatial);
	}
#endif
	if (data->resolve_effects) {
		memdelete(data->resolve_effects);
	}
	if (data->vrs) {
		memdelete(data->vrs);
	}
	if (data->fsr) {
		memdelete(data->fsr);
	}
	if (data->tone_mapper) {
		memdelete(data->tone_mapper);
	}
	if (data->smaa) {
		memdelete(data->smaa);
	}
	if (data->luminance) {
		memdelete(data->luminance);
	}
	if (data->debug_effects) {
		memdelete(data->debug_effects);
	}
	if (data->bokeh_dof) {
		memdelete(data->bokeh_dof);
	}
	RendererRD::ForwardIDStorage::reset();
	RendererRD::SSEffects::finalize();
	RendererRD::CopyEffects::finalize();
	memdelete(data);
	data = nullptr;
}

uint32_t RendererSceneRenderRD::get_max_elements()
{
	return GLOBAL_GET_CACHED(uint32_t, "rendering/limits/cluster_builder/max_clustered_elements");
}

void RendererSceneRenderRD::setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers) {}

void RendererSceneRenderRD::_render_scene(RenderDataRD* p_render_data, const Color& p_default_color)
{
}

void RendererSceneRenderRD::_render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances,
	RID p_framebuffer, const Rect2i& p_region)
{
}

void RendererSceneRenderRD::_render_material(const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal,
	const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
	const Rect2i& p_region, float p_exposure_normalization)
{
}

void RendererSceneRenderRD::_render_sdfgi(Ref<RenderSceneBuffersRD> p_render_buffers,
	const Vector3i& p_from, const Vector3i& p_size, const AABB& p_bounds,
	const PagedArray<RenderGeometryInstance*>& p_instances, const RID& p_albedo_texture,
	const RID& p_emission_texture, const RID& p_emission_aniso_texture,
	const RID& p_geom_facing_texture, float p_exposure_normalization)
{
}

void RendererSceneRenderRD::_render_particle_collider_heightfield(RID p_fb,
	const Transform3D& p_cam_transform, const Projection& p_cam_projection,
	const PagedArray<RenderGeometryInstance*>& p_instances)
{
}

RID RendererSceneRenderRD::_render_buffers_get_normal_texture(
	Ref<RenderSceneBuffersRD> p_render_buffers)
{
	return RID();
}

RID RendererSceneRenderRD::_render_buffers_get_velocity_texture(
	Ref<RenderSceneBuffersRD> p_render_buffers)
{
	return RID();
}

void RendererSceneRenderRD::base_uniforms_changed() {}

void RendererSceneRenderRD::_process_compositor_effects(
	RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data)
{
}

void RendererRD::Fog::volumetric_fog_update(
	const RendererRD::Fog::VolumetricFogSettings& p_settings, const Projection& p_cam_projection,
	const Transform3D& p_cam_transform, const Transform3D& p_view_transform, RID p_environment,
	int p_directional_light_count, bool p_use_temporal_reprojection, int p_frame_index, int p_step,
	const PagedArray<RID>& p_lights)
{
}

// --- Merged from servers/rendering/renderer_rd/cluster_builder_rd.cpp ---

ClusterBuilderSharedDataRD::ClusterBuilderSharedDataRD() {}

ClusterBuilderSharedDataRD::~ClusterBuilderSharedDataRD()
{
	RD::free_rid(sphere_vertex_buffer);
	RD::free_rid(sphere_index_buffer);
	RD::free_rid(cone_vertex_buffer);
	RD::free_rid(cone_index_buffer);
	RD::free_rid(box_vertex_buffer);
	RD::free_rid(box_index_buffer);

	cluster_render.cluster_render_shader.version_free(cluster_render.shader_version);
	cluster_store.cluster_store_shader.version_free(cluster_store.shader_version);
	cluster_debug.cluster_debug_shader.version_free(cluster_debug.shader_version);
}

/////////////////////////////

void ClusterBuilderRD::_clear()
{
	if (cluster_buffer.is_null()) {
		return;
	}

	RD::free_rid(cluster_buffer);
	RD::free_rid(cluster_render_buffer);
	RD::free_rid(element_buffer);
	cluster_buffer = RID();
	cluster_render_buffer = RID();
	element_buffer = RID();

	memfree(render_elements);

	render_elements = nullptr;
	render_element_max = 0;
	render_element_count = 0;

	RD::free_rid(framebuffer);
	framebuffer = RID();

	cluster_render_uniform_set = RID();
	cluster_store_uniform_set = RID();
}

void ClusterBuilderRD::setup(Size2i p_screen_size, uint32_t p_max_elements, RID p_depth_buffer,
	RID p_depth_buffer_sampler, RID p_color_buffer)
{
	ERR_FAIL_COND(p_max_elements == 0);
	ERR_FAIL_COND(p_screen_size.x < 1);
	ERR_FAIL_COND(p_screen_size.y < 1);

	_clear();

	screen_size = p_screen_size;

	cluster_screen_size.width =
		Math::division_round_up((uint32_t)p_screen_size.width, cluster_size);
	cluster_screen_size.height =
		Math::division_round_up((uint32_t)p_screen_size.height, cluster_size);

	max_elements_by_type = p_max_elements;
	if (max_elements_by_type % 32) { // Needs to be aligned to 32.
		max_elements_by_type += 32 - (max_elements_by_type % 32);
	}

	cluster_buffer_size = cluster_screen_size.x * cluster_screen_size.y *
						  (max_elements_by_type / 32 + 32) * ELEMENT_TYPE_MAX * 4;

	render_element_max = max_elements_by_type * ELEMENT_TYPE_MAX;

	uint32_t element_tag_bits_size = render_element_max / 32;
	uint32_t element_tag_depth_bits_size = render_element_max;

	cluster_render_buffer_size =
		cluster_screen_size.x * cluster_screen_size.y *
		(element_tag_bits_size + element_tag_depth_bits_size) *
		4; // Tag bits (element was used) and tag depth (depth range in which it was used).

	cluster_render_buffer = RD::storage_buffer_create(cluster_render_buffer_size);
	cluster_buffer = RD::storage_buffer_create(cluster_buffer_size);

	render_elements =
		static_cast<RenderElementData*>(memalloc(sizeof(RenderElementData) * render_element_max));
	render_element_count = 0;

	element_buffer =
		RD::storage_buffer_create(sizeof(RenderElementData) * render_element_max);

	uint32_t div_value = 1 << divisor;
	if (use_msaa) {
		framebuffer = RD::framebuffer_create_empty(
			p_screen_size / div_value, RDC::TEXTURE_SAMPLES_4);
	}
	else {
		framebuffer = RD::framebuffer_create_empty(p_screen_size / div_value);
	}

	{
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER;
			u.binding = 1;
			u.append_id(state_uniform);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 2;
			u.append_id(element_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 3;
			u.append_id(cluster_render_buffer);
			uniforms.push_back(u);
		}

		cluster_render_uniform_set =
			RD::uniform_set_create(uniforms, shared->cluster_render.shader, 0);
	}

	{
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 1;
			u.append_id(cluster_render_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 2;
			u.append_id(cluster_buffer);
			uniforms.push_back(u);
		}

		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 3;
			u.append_id(element_buffer);
			uniforms.push_back(u);
		}

		cluster_store_uniform_set =
			RD::uniform_set_create(uniforms, shared->cluster_store.shader, 0);
	}

	if (p_color_buffer.is_valid()) {
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 1;
			u.append_id(cluster_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_IMAGE;
			u.binding = 2;
			u.append_id(p_color_buffer);
			uniforms.push_back(u);
		}

		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 3;
			u.append_id(p_depth_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER;
			u.binding = 4;
			u.append_id(p_depth_buffer_sampler);
			uniforms.push_back(u);
		}

		debug_uniform_set =
			RD::uniform_set_create(uniforms, shared->cluster_debug.shader, 0);
	}
	else {
		debug_uniform_set = RID();
	}
}

void ClusterBuilderRD::begin(
	const Transform3D& p_view_transform, const Projection& p_cam_projection, bool p_flip_y)
{
	view_xform = p_view_transform.affine_inverse();
	projection = p_cam_projection;
	z_near = projection.get_z_near();
	z_far = projection.get_z_far();
	camera_orthogonal = p_cam_projection.is_orthogonal();
	adjusted_projection = projection;
	if (!camera_orthogonal) {
		adjusted_projection.adjust_perspective_znear(0.0001);
	}

	Projection correction;
	correction.set_depth_correction(p_flip_y);
	projection = correction * projection;
	adjusted_projection = correction * adjusted_projection;

	// Reset counts.
	render_element_count = 0;
	for (uint32_t i = 0; i < ELEMENT_TYPE_MAX; i++) {
		cluster_count_by_type[i] = 0;
	}
}

RID ClusterBuilderRD::get_cluster_buffer() const { return cluster_buffer; }

uint32_t ClusterBuilderRD::get_cluster_size() const { return cluster_size; }

uint32_t ClusterBuilderRD::get_max_cluster_elements() const { return max_elements_by_type; }

void ClusterBuilderRD::set_shared(ClusterBuilderSharedDataRD* p_shared) { shared = p_shared; }

ClusterBuilderRD::~ClusterBuilderRD()
{
	_clear();
	RD::free_rid(state_uniform);
}



void ClusterBuilderRD::debug(ClusterBuilderRD::ElementType) {}

ClusterBuilderRD::ClusterBuilderRD() {}

void ClusterBuilderRD::bake_cluster() {}

// ============================================================================
// Merged from servers/rendering/renderer_rd/shader_rd.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/version.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/renderer_rd/shader_rd.h"

#define ENABLE_SHADER_CACHE 1

void ShaderRD::_add_stage(const char* p_code, StageType p_stage_type)
{
	Vector<String> lines = String(p_code).split("\n");

	String text;

	int line_count = lines.size();
	for (int i = 0; i < line_count; i++) {
		const String& l = lines[i];
		bool push_chunk = false;

		StageTemplate::Chunk chunk;

		if (l.begins_with("#VERSION_DEFINES")) {
			chunk.type = StageTemplate::Chunk::TYPE_VERSION_DEFINES;
			push_chunk = true;
		}
		else if (l.begins_with("#GLOBALS")) {
			switch (p_stage_type) {
			case STAGE_TYPE_VERTEX:
				chunk.type = StageTemplate::Chunk::TYPE_VERTEX_GLOBALS;
				break;
			case STAGE_TYPE_FRAGMENT:
				chunk.type = StageTemplate::Chunk::TYPE_FRAGMENT_GLOBALS;
				break;
			case STAGE_TYPE_COMPUTE:
				chunk.type = StageTemplate::Chunk::TYPE_COMPUTE_GLOBALS;
				break;
			case STAGE_TYPE_RAYGEN:
				chunk.type = StageTemplate::Chunk::TYPE_RAYGEN_GLOBALS;
				break;
			case STAGE_TYPE_ANY_HIT:
				chunk.type = StageTemplate::Chunk::TYPE_ANY_HIT_GLOBALS;
				break;
			case STAGE_TYPE_CLOSEST_HIT:
				chunk.type = StageTemplate::Chunk::TYPE_CLOSEST_HIT_GLOBALS;
				break;
			case STAGE_TYPE_MISS:
				chunk.type = StageTemplate::Chunk::TYPE_MISS_GLOBALS;
				break;
			case STAGE_TYPE_INTERSECTION:
				chunk.type = StageTemplate::Chunk::TYPE_INTERSECTION_GLOBALS;
				break;
			default: {
			}
			}

			push_chunk = true;
		}
		else if (l.begins_with("#MATERIAL_UNIFORMS")) {
			chunk.type = StageTemplate::Chunk::TYPE_MATERIAL_UNIFORMS;
			push_chunk = true;
		}
		else if (l.begins_with("#CODE")) {
			chunk.type = StageTemplate::Chunk::TYPE_CODE;
			push_chunk = true;
			chunk.code =
				l.replace_first("#CODE", String()).remove_char(':').strip_edges().to_upper();
		}
		else if (l.begins_with("#include ")) {
			String include_file = l.replace("#include ", "").strip_edges();
			if (include_file[0] == '"') {
				int end_pos = include_file.find_char('"', 1);
				if (end_pos >= 0) {
					include_file = include_file.substr(1, end_pos - 1);

					String include_code = ShaderIncludeDB::get_built_in_include_file(include_file);
					if (!include_code.is_empty()) {
						// Add these lines into our parse list so we parse them as well.
						Vector<String> include_lines = include_code.split("\n");

						for (int j = include_lines.size() - 1; j >= 0; j--) {
							lines.insert(i + 1, include_lines[j]);
						}

						line_count = lines.size();
					}
					else {
						// Add it in as is.
						text += l + "\n";
					}
				}
				else {
					// Add it in as is.
					text += l + "\n";
				}
			}
			else {
				// Add it in as is.
				text += l + "\n";
			}
		}
		else {
			text += l + "\n";
		}

		if (push_chunk) {
			if (!text.is_empty()) {
				StageTemplate::Chunk text_chunk;
				text_chunk.type = StageTemplate::Chunk::TYPE_TEXT;
				text_chunk.text = text.utf8();
				stage_templates[p_stage_type].chunks.push_back(text_chunk);
				text = String();
			}
			stage_templates[p_stage_type].chunks.push_back(chunk);
		}
	}

	if (!text.is_empty()) {
		StageTemplate::Chunk text_chunk;
		text_chunk.type = StageTemplate::Chunk::TYPE_TEXT;
		text_chunk.text = text.utf8();
		stage_templates[p_stage_type].chunks.push_back(text_chunk);
		text = String();
	}
}

void ShaderRD::setup(const StageData& p_data)
{
	if (p_data.is_raytracing) {
		setup_raytracing(p_data.raygen_code, p_data.any_hit_code, p_data.closest_hit_code,
			p_data.miss_code, p_data.intersection_code, p_data.name);
	}
	else {
		setup(p_data.vertex_code, p_data.fragment_code, p_data.compute_code, p_data.name);
	}
}

void ShaderRD::setup(const char* p_vertex_code, const char* p_fragment_code,
	const char* p_compute_code, const char* p_name)
{
	name = p_name;

	if (p_compute_code) {
		_add_stage(p_compute_code, STAGE_TYPE_COMPUTE);
		pipeline_type = RDC::PIPELINE_TYPE_COMPUTE;
	}
	else {
		pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;
		if (p_vertex_code) {
			_add_stage(p_vertex_code, STAGE_TYPE_VERTEX);
		}
		if (p_fragment_code) {
			_add_stage(p_fragment_code, STAGE_TYPE_FRAGMENT);
		}
	}

	StringBuilder tohash;
	tohash.append("[GodotVersionNumber]");
	tohash.append(VLTR_VERSION_NUMBER);
	tohash.append("[GodotVersionHash]");
	tohash.append(VLTR_VERSION_HASH);
	tohash.append("[Vertex]");
	tohash.append(p_vertex_code ? p_vertex_code : "");
	tohash.append("[Fragment]");
	tohash.append(p_fragment_code ? p_fragment_code : "");
	tohash.append("[Compute]");
	tohash.append(p_compute_code ? p_compute_code : "");
	tohash.append("[DebugInfo]");
	tohash.append(Engine::is_generate_spirv_debug_info_enabled() ? "1" : "0");

	base_sha256 = tohash.as_string().sha256_text();
}

void ShaderRD::setup_raytracing(const char* p_raygen_code, const char* p_any_hit_code,
	const char* p_closest_hit_code, const char* p_miss_code, const char* p_intersection_code,
	const char* p_name)
{
	name = p_name;

	pipeline_type = RDC::PIPELINE_TYPE_RAYTRACING;
	if (p_raygen_code) {
		_add_stage(p_raygen_code, STAGE_TYPE_RAYGEN);
	}
	if (p_any_hit_code) {
		_add_stage(p_any_hit_code, STAGE_TYPE_ANY_HIT);
	}
	if (p_closest_hit_code) {
		_add_stage(p_closest_hit_code, STAGE_TYPE_CLOSEST_HIT);
	}
	if (p_miss_code) {
		_add_stage(p_miss_code, STAGE_TYPE_MISS);
	}
	if (p_intersection_code) {
		_add_stage(p_intersection_code, STAGE_TYPE_INTERSECTION);
	}

	StringBuilder tohash;
	tohash.append("[GodotVersionNumber]");
	tohash.append(VLTR_VERSION_NUMBER);
	tohash.append("[GodotVersionHash]");
	tohash.append(VLTR_VERSION_HASH);
	tohash.append("[Raygen]");
	tohash.append(p_raygen_code ? p_raygen_code : "");
	tohash.append("[AnyHit]");
	tohash.append(p_any_hit_code ? p_any_hit_code : "");
	tohash.append("[ClosestHit]");
	tohash.append(p_closest_hit_code ? p_closest_hit_code : "");
	tohash.append("[Miss]");
	tohash.append(p_miss_code ? p_miss_code : "");
	tohash.append("[Intersection]");
	tohash.append(p_intersection_code ? p_intersection_code : "");
	tohash.append("[DebugInfo]");
	tohash.append(Engine::is_generate_spirv_debug_info_enabled() ? "1" : "0");

	base_sha256 = tohash.as_string().sha256_text();
}

RID ShaderRD::version_create(bool p_embedded)
{
	// initialize() was never called
	ERR_FAIL_COND_V(group_to_variant_map.is_empty(), RID());

	Version version;
	version.dirty = true;
	version.valid = false;
	version.initialize_needed = true;
	version.embedded = p_embedded;
	version.variants.clear();
	version.variant_data.clear();

	version.mutex = memnew(Mutex);
	RID rid = version_owner.make_rid(version);
	{
		MutexLock lock(versions_mutex);
		version_mutexes.insert(rid, version.mutex);
	}

	if (p_embedded) {
		MutexLock lock(shader_versions_embedded_set_mutex);
		shader_versions_embedded_set.insert({this, rid});
	}

	return rid;
}

void ShaderRD::_clear_version(Version* p_version, bool p_skip_compilation)
{
	(void)p_skip_compilation;

	// Clear versions if they exist.
	if (!p_version->variants.is_empty()) {
		for (int i = 0; i < variant_defines.size(); i++) {
			if (p_version->variants[i].is_valid()) {
				RD::free_rid(p_version->variants[i]);
			}
		}

		p_version->variants.clear();
		p_version->variant_data.clear();
	}
}

void ShaderRD::_build_variant_code(StringBuilder& builder, uint32_t p_variant,
	const Version* p_version, const StageTemplate& p_template)
{
	for (const StageTemplate::Chunk& chunk : p_template.chunks) {
		switch (chunk.type) {
		case StageTemplate::Chunk::TYPE_VERSION_DEFINES: {
			builder.append("\n"); // make sure defines begin at newline
			builder.append(general_defines.get_data());
			builder.append(variant_defines[p_variant].text.get_data());
			for (int j = 0; j < p_version->custom_defines.size(); j++) {
				builder.append(p_version->custom_defines[j].get_data());
			}
			builder.append("\n"); // make sure defines begin at newline
			if (p_version->uniforms.size()) {
				builder.append("#define MATERIAL_UNIFORMS_USED\n");
			}
			for (const KeyValue<String, CharString>& E : p_version->code_sections) {
				builder.append(String("#define ") + String(E.key) + "_CODE_USED\n");
			}
			builder.append(String("#define RENDER_DRIVER_") +
						   OS::get_current_rendering_driver_name().to_upper() + "\n");
			builder.append("#define samplerExternalOES sampler2D\n");
			builder.append("#define textureExternalOES texture2D\n");
		} break;
		case StageTemplate::Chunk::TYPE_MATERIAL_UNIFORMS: {
			builder.append(
				p_version->uniforms.get_data()); // uniforms (same for vertex and fragment)
		} break;
		case StageTemplate::Chunk::TYPE_VERTEX_GLOBALS: {
			builder.append(p_version->vertex_globals.get_data()); // vertex globals
		} break;
		case StageTemplate::Chunk::TYPE_FRAGMENT_GLOBALS: {
			builder.append(p_version->fragment_globals.get_data()); // fragment globals
		} break;
		case StageTemplate::Chunk::TYPE_COMPUTE_GLOBALS: {
			builder.append(p_version->compute_globals.get_data()); // compute globals
		} break;
		case StageTemplate::Chunk::TYPE_RAYGEN_GLOBALS: {
			builder.append(p_version->raygen_globals.get_data()); // raygen globals
		} break;
		case StageTemplate::Chunk::TYPE_ANY_HIT_GLOBALS: {
			builder.append(p_version->any_hit_globals.get_data()); // any_hit globals
		} break;
		case StageTemplate::Chunk::TYPE_CLOSEST_HIT_GLOBALS: {
			builder.append(p_version->closest_hit_globals.get_data()); // closest_hit globals
		} break;
		case StageTemplate::Chunk::TYPE_MISS_GLOBALS: {
			builder.append(p_version->miss_globals.get_data()); // miss globals
		} break;
		case StageTemplate::Chunk::TYPE_INTERSECTION_GLOBALS: {
			builder.append(p_version->intersection_globals.get_data()); // intersection globals
		} break;
		case StageTemplate::Chunk::TYPE_CODE: {
			if (p_version->code_sections.has(chunk.code)) {
				builder.append(p_version->code_sections[chunk.code].get_data());
			}
		} break;
		case StageTemplate::Chunk::TYPE_TEXT: {
			builder.append(chunk.text.get_data());
		} break;
		}
	}
}

Vector<String> ShaderRD::_build_variant_stage_sources(uint32_t p_variant, CompileData p_data)
{
	if (!variants_enabled[p_variant]) {
		return Vector<String>(); // Variant is disabled, return.
	}

	Vector<String> stage_sources;
	stage_sources.resize(RDC::SHADER_STAGE_MAX);

	if (pipeline_type == RDC::PIPELINE_TYPE_COMPUTE) {
		// Compute stage.
		StringBuilder builder;
		_build_variant_code(
			builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_COMPUTE]);
		stage_sources.write[RDC::SHADER_STAGE_COMPUTE] = builder.as_string();
	}
	else if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
		{
			// Raygen stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_RAYGEN]);
			stage_sources.write[RDC::SHADER_STAGE_RAYGEN] = builder.as_string();
		}

		{
			// Any hit stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_ANY_HIT]);
			stage_sources.write[RDC::SHADER_STAGE_ANY_HIT] = builder.as_string();
		}

		{
			// Closest hit stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_CLOSEST_HIT]);
			stage_sources.write[RDC::SHADER_STAGE_CLOSEST_HIT] = builder.as_string();
		}

		{
			// Miss stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_MISS]);
			stage_sources.write[RDC::SHADER_STAGE_MISS] = builder.as_string();
		}

		{
			// Intersection stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_INTERSECTION]);
			stage_sources.write[RDC::SHADER_STAGE_INTERSECTION] = builder.as_string();
		}
	}
	else {
		{
			// Vertex stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_VERTEX]);
			stage_sources.write[RDC::SHADER_STAGE_VERTEX] = builder.as_string();
		}

		{
			// Fragment stage.
			StringBuilder builder;
			_build_variant_code(
				builder, p_variant, p_data.version, stage_templates[STAGE_TYPE_FRAGMENT]);
			stage_sources.write[RDC::SHADER_STAGE_FRAGMENT] = builder.as_string();
		}
	}

	return stage_sources;
}

Vector<String> ShaderRD::version_build_variant_stage_sources(RID p_version, int p_variant)
{
	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL_V(version, Vector<String>());

	if (version->dirty) {
		_initialize_version(version);
	}

	CompileData compile_data;
	compile_data.version = version;
	compile_data.group = variant_to_group[p_variant];
	return _build_variant_stage_sources(p_variant, compile_data);
}

RenderingServerTypes::ShaderNativeSourceCode ShaderRD::version_get_native_source_code(RID p_version)
{
	Version* version = version_owner.get_or_null(p_version);
	RenderingServerTypes::ShaderNativeSourceCode source_code;
	ERR_FAIL_NULL_V(version, source_code);

	MutexLock lock(*version->mutex);

	source_code.versions.resize(variant_defines.size());

	for (int i = 0; i < source_code.versions.size(); i++) {
		if (pipeline_type == RDC::PIPELINE_TYPE_RASTERIZATION) {
			// Vertex stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_VERTEX]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "vertex";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}

		if (pipeline_type == RDC::PIPELINE_TYPE_RASTERIZATION) {
			// Fragment stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_FRAGMENT]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "fragment";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}

		if (pipeline_type == RDC::PIPELINE_TYPE_COMPUTE) {
			// Compute stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_COMPUTE]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "compute";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}

		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			// Raygen stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_RAYGEN]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "raygen";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			// Any hit stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_ANY_HIT]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "any_hit";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			// Closest hit stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_CLOSEST_HIT]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "closest_hit";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			// Miss stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_MISS]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "miss";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			// Intersection stage.

			StringBuilder builder;
			_build_variant_code(builder, i, version, stage_templates[STAGE_TYPE_INTERSECTION]);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "intersection";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
	}

	return source_code;
}

String ShaderRD::version_get_cache_file_relative_path(
	RID p_version, int p_group, const String& p_api_name)
{
	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL_V(version, String());

	return _get_cache_file_relative_path(version, p_group, p_api_name);
}

String ShaderRD::_version_get_sha1(Version* p_version) const
{
	StringBuilder hash_build;

	hash_build.append("[uniforms]");
	hash_build.append(p_version->uniforms.get_data());
	hash_build.append("[vertex_globals]");
	hash_build.append(p_version->vertex_globals.get_data());
	hash_build.append("[fragment_globals]");
	hash_build.append(p_version->fragment_globals.get_data());
	hash_build.append("[compute_globals]");
	hash_build.append(p_version->compute_globals.get_data());
	hash_build.append("[raygen_globals]");
	hash_build.append(p_version->raygen_globals.get_data());
	hash_build.append("[any_hit_globals]");
	hash_build.append(p_version->any_hit_globals.get_data());
	hash_build.append("[closest_hit_globals]");
	hash_build.append(p_version->closest_hit_globals.get_data());
	hash_build.append("[miss_globals]");
	hash_build.append(p_version->miss_globals.get_data());
	hash_build.append("[intersection_globals]");
	hash_build.append(p_version->intersection_globals.get_data());

	Vector<String> code_sections;
	for (const KeyValue<String, CharString>& E : p_version->code_sections) {
		code_sections.push_back(E.key);
	}
	code_sections.sort();

	for (int i = 0; i < code_sections.size(); i++) {
		hash_build.append(String("[code:") + String(code_sections[i]) + "]");
		hash_build.append(p_version->code_sections[code_sections[i]].get_data());
	}
	for (int i = 0; i < p_version->custom_defines.size(); i++) {
		hash_build.append("[custom_defines:" + itos(i) + "]");
		hash_build.append(p_version->custom_defines[i].get_data());
	}

	return hash_build.as_string().sha1_text();
}

static const char* shader_file_header = "GDSC";
static const uint32_t cache_file_version = 4;

String ShaderRD::_get_cache_file_relative_path(
	Version* p_version, int p_group, const String& p_api_name)
{
	String sha1 = _version_get_sha1(p_version);
	return name.path_join(group_sha256[p_group]).path_join(sha1) + "." + p_api_name + ".cache";
}

String ShaderRD::_get_cache_file_path(
	Version* p_version, int p_group, const String& p_api_name, bool p_user_dir)
{
	const String& shader_cache_dir = p_user_dir ? shader_cache_user_dir : shader_cache_res_dir;
	String relative_path = _get_cache_file_relative_path(p_version, p_group, p_api_name);
	return shader_cache_dir.path_join(relative_path);
}

void ShaderRD::_save_to_cache(Version* p_version, int p_group)
{
	ERR_FAIL_COND(!shader_cache_user_dir_valid);
	String api_safe_name = String(RD::get_device_api_name()).validate_filename().to_lower();
	const String& path = _get_cache_file_path(p_version, p_group, api_safe_name, true);
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	ERR_FAIL_COND(f.is_null());

	PackedByteArray shader_cache_bytes =
		ShaderRD::save_shader_cache_bytes(group_to_variant_map[p_group], p_version->variant_data);
	f->store_buffer(shader_cache_bytes);
}

void ShaderRD::_allocate_placeholders(Version* p_version, int p_group)
{
	ERR_FAIL_COND(p_version->variants.is_empty());

	for (uint32_t i = 0; i < group_to_variant_map[p_group].size(); i++) {
		int variant_id = group_to_variant_map[p_group][i];
		RID shader = RD::shader_create_placeholder();
		{
			p_version->variants.write[variant_id] = shader;
		}
	}
}

void ShaderRD::_compile_ensure_finished(Version* p_version)
{
	if (p_version->initialize_needed || p_version->variants.is_empty() || p_version->valid) {
		return;
	}

	// Wait for compilation of existing groups if necessary.
	for (int i = 0; i < group_enabled.size(); i++) {
		if (group_enabled[i]) {
			_compile_version_end(p_version, i);
		}
	}
}

void ShaderRD::_version_set(Version* p_version, const HashMap<String, String>& p_code,
	const Vector<String>& p_custom_defines)
{
	p_version->code_sections.clear();
	for (const KeyValue<String, String>& E : p_code) {
		p_version->code_sections[String(E.key.to_upper())] = E.value.utf8();
	}

	p_version->custom_defines.clear();
	for (const String& custom_define : p_custom_defines) {
		p_version->custom_defines.push_back(custom_define.utf8());
	}

	p_version->dirty = true;
	if (p_version->initialize_needed) {
		_initialize_version(p_version);
		for (int i = 0; i < group_enabled.size(); i++) {
			if (!group_enabled[i]) {
				_allocate_placeholders(p_version, i);
				continue;
			}
			_compile_version_start(p_version, i);
		}
		p_version->initialize_needed = false;
	}
}

void ShaderRD::version_set_code(RID p_version, const HashMap<String, String>& p_code,
	const String& p_uniforms, const String& p_vertex_globals, const String& p_fragment_globals,
	const Vector<String>& p_custom_defines)
{
	ERR_FAIL_COND(pipeline_type != RDC::PIPELINE_TYPE_RASTERIZATION);

	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL(version);

	MutexLock lock(*version->mutex);

	_compile_ensure_finished(version);

	version->vertex_globals = p_vertex_globals.utf8();
	version->fragment_globals = p_fragment_globals.utf8();
	version->uniforms = p_uniforms.utf8();

	_version_set(version, p_code, p_custom_defines);
}

void ShaderRD::version_set_compute_code(RID p_version, const HashMap<String, String>& p_code,
	const String& p_uniforms, const String& p_compute_globals,
	const Vector<String>& p_custom_defines)
{
	ERR_FAIL_COND(pipeline_type != RDC::PIPELINE_TYPE_COMPUTE);

	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL(version);

	MutexLock lock(*version->mutex);

	_compile_ensure_finished(version);

	version->compute_globals = p_compute_globals.utf8();
	version->uniforms = p_uniforms.utf8();

	_version_set(version, p_code, p_custom_defines);
}

void ShaderRD::version_set_raytracing_code(RID p_version, const HashMap<String, String>& p_code,
	const String& p_uniforms, const String& p_raygen_globals, const String& p_any_hit_globals,
	const String& p_closest_hit_globals, const String& p_miss_globals,
	const String& p_intersection_globals, const Vector<String>& p_custom_defines)
{
	ERR_FAIL_COND(pipeline_type != RDC::PIPELINE_TYPE_RAYTRACING);

	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL(version);

	version->raygen_globals = p_raygen_globals.utf8();
	version->any_hit_globals = p_any_hit_globals.utf8();
	version->closest_hit_globals = p_closest_hit_globals.utf8();
	version->miss_globals = p_miss_globals.utf8();
	version->intersection_globals = p_intersection_globals.utf8();
	version->uniforms = p_uniforms.utf8();

	_version_set(version, p_code, p_custom_defines);
}

bool ShaderRD::version_is_valid(RID p_version)
{
	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL_V(version, false);

	MutexLock lock(*version->mutex);

	if (version->dirty) {
		_initialize_version(version);
		for (int i = 0; i < group_enabled.size(); i++) {
			if (!group_enabled[i]) {
				_allocate_placeholders(version, i);
				continue;
			}
			_compile_version_start(version, i);
		}
	}

	_compile_ensure_finished(version);

	return version->valid;
}

bool ShaderRD::version_free(RID p_version)
{
	if (version_owner.owns(p_version)) {
		{
			MutexLock lock(versions_mutex);
			version_mutexes.erase(p_version);
		}

		Version* version = version_owner.get_or_null(p_version);
		if (version->embedded) {
			MutexLock lock(shader_versions_embedded_set_mutex);
			shader_versions_embedded_set.erase({this, p_version});
		}

		version->mutex->lock();
		_clear_version(version);
		version_owner.free(p_version);
		version->mutex->unlock();
		memdelete(version->mutex);
	}
	else {
		return false;
	}

	return true;
}

void ShaderRD::set_variant_enabled(int p_variant, bool p_enabled)
{
	ERR_FAIL_COND(version_owner.get_rid_count() > 0); // versions exist
	ERR_FAIL_INDEX(p_variant, variants_enabled.size());
	variants_enabled.write[p_variant] = p_enabled;
}

bool ShaderRD::is_variant_enabled(int p_variant) const
{
	ERR_FAIL_INDEX_V(p_variant, variants_enabled.size(), false);
	return variants_enabled[p_variant];
}

int64_t ShaderRD::get_variant_count() const { return variants_enabled.size(); }

int ShaderRD::get_variant_to_group(int p_variant) const { return variant_to_group[p_variant]; }

void ShaderRD::enable_group(int p_group)
{
	ERR_FAIL_INDEX(p_group, group_enabled.size());

	if (group_enabled[p_group]) {
		// Group already enabled, do nothing.
		return;
	}

	group_enabled.write[p_group] = true;

	// Compile all versions again to include the new group.
	for (const RID& version_rid : version_owner.get_owned_list()) {
		Version* version = version_owner.get_or_null(version_rid);
		version->mutex->lock();
		_compile_version_start(version, p_group);
		version->mutex->unlock();
	}
}

bool ShaderRD::is_group_enabled(int p_group) const { return group_enabled[p_group]; }

int64_t ShaderRD::get_group_count() const { return group_enabled.size(); }

const LocalVector<int>& ShaderRD::get_group_to_variants(int p_group) const
{
	return group_to_variant_map[p_group];
}

const String& ShaderRD::get_name() const { return name; }

const Vector<uint64_t>& ShaderRD::get_dynamic_buffers() const { return dynamic_buffers; }

bool ShaderRD::shader_cache_cleanup_on_start = false;

ShaderRD::ShaderRD()
{
	// Do not feel forced to use this, in most cases it makes little to no difference.
	bool use_32_threads = false;
	if (RD::get_device_vendor_name() == "NVIDIA") {
		use_32_threads = true;
	}
	String base_compute_define_text;
	if (use_32_threads) {
		base_compute_define_text = "\n#define NATIVE_LOCAL_GROUP_SIZE 32\n#define "
								   "NATIVE_LOCAL_SIZE_2D_X 8\n#define NATIVE_LOCAL_SIZE_2D_Y 4\n";
	}
	else {
		base_compute_define_text = "\n#define NATIVE_LOCAL_GROUP_SIZE 64\n#define "
								   "NATIVE_LOCAL_SIZE_2D_X 8\n#define NATIVE_LOCAL_SIZE_2D_Y 8\n";
	}

	base_compute_defines = base_compute_define_text.ascii();
}

ShaderRD::ShaderRD(const StageData& p_data) : ShaderRD()
{
	setup(p_data);
}

void ShaderRD::initialize(const Vector<String>& p_variant_defines, const String& p_general_defines,
	const Vector<RD::PipelineImmutableSampler>& p_immutable_samplers,
	const Vector<uint64_t>& p_dynamic_buffers)
{
	ERR_FAIL_COND(variant_defines.size());
	ERR_FAIL_COND(p_variant_defines.is_empty());

	general_defines = p_general_defines.utf8();
	immutable_samplers = p_immutable_samplers;
	dynamic_buffers = p_dynamic_buffers;

	// When initialized this way, there is just one group and its always enabled.
	group_to_variant_map.insert(0, LocalVector<int>{});
	group_enabled.push_back(true);

	for (int i = 0; i < p_variant_defines.size(); i++) {
		variant_defines.push_back(VariantDefine(0, p_variant_defines[i], true));
		variants_enabled.push_back(true);
		variant_to_group.push_back(0);
		group_to_variant_map[0].push_back(i);
	}

	if (!shader_cache_user_dir.is_empty() || !shader_cache_res_dir.is_empty()) {
		group_sha256.resize(1);
		_initialize_cache();
	}
}

void ShaderRD::_initialize_cache()
{
	shader_cache_user_dir_valid = !shader_cache_user_dir.is_empty();
	shader_cache_res_dir_valid = !shader_cache_res_dir.is_empty();
	if (!shader_cache_user_dir_valid) {
		return;
	}

	for (const KeyValue<int, LocalVector<int>>& E : group_to_variant_map) {
		StringBuilder hash_build;

		hash_build.append("[base_hash]");
		hash_build.append(base_sha256);
		hash_build.append("[general_defines]");
		hash_build.append(general_defines.get_data());
		hash_build.append("[group_id]");
		hash_build.append(itos(E.key));
		for (uint32_t i = 0; i < E.value.size(); i++) {
			hash_build.append("[variant_defines:" + itos(E.value[i]) + "]");
			hash_build.append(variant_defines[E.value[i]].text.get_data());
		}

		for (const uint64_t dyn_buffer : dynamic_buffers) {
			hash_build.append("[dynamic_buffer]");
			hash_build.append(uitos(dyn_buffer));
		}

		group_sha256[E.key] = hash_build.as_string().sha256_text();

		if (!shader_cache_user_dir.is_empty()) {
			// Validate if it's possible to write to all the directories required by in the user
			// directory.
			Ref<DirAccess> d = DirAccess::open(shader_cache_user_dir);
			if (d.is_null()) {
				shader_cache_user_dir_valid = false;
				ERR_FAIL_MSG(
					vformat("Unable to open shader cache directory at %s.", shader_cache_user_dir));
			}

			if (d->change_dir(name) != OK) {
				Error err = d->make_dir(name);
				if (err != OK) {
					shader_cache_user_dir_valid = false;
					ERR_FAIL_MSG(vformat("Unable to create shader cache directory %s at %s.", name,
						shader_cache_user_dir));
				}

				d->change_dir(name);
			}

			if (d->change_dir(group_sha256[E.key]) != OK) {
				Error err = d->make_dir(group_sha256[E.key]);
				if (err != OK) {
					shader_cache_user_dir_valid = false;
					ERR_FAIL_MSG(vformat("Unable to create shader cache directory %s/%s at %s.",
						name, group_sha256[E.key], shader_cache_user_dir));
				}
			}
		}

		print_verbose(
			"Shader '" + name + "' (group " + itos(E.key) + ") SHA256: " + group_sha256[E.key]);
	}
}

// Same as above, but allows specifying shader compilation groups.
void ShaderRD::initialize(const Vector<VariantDefine>& p_variant_defines,
	const String& p_general_defines,
	const Vector<RD::PipelineImmutableSampler>& p_immutable_samplers,
	const Vector<uint64_t>& p_dynamic_buffers)
{
	ERR_FAIL_COND(variant_defines.size());
	ERR_FAIL_COND(p_variant_defines.is_empty());

	general_defines = p_general_defines.utf8();
	immutable_samplers = p_immutable_samplers;
	dynamic_buffers = p_dynamic_buffers;

	int max_group_id = 0;

	for (int i = 0; i < p_variant_defines.size(); i++) {
		// Fill variant array.
		variant_defines.push_back(p_variant_defines[i]);
		variants_enabled.push_back(true);
		variant_to_group.push_back(p_variant_defines[i].group);

		// Map variant array index to group id, so we can iterate over groups later.
		if (!group_to_variant_map.has(p_variant_defines[i].group)) {
			group_to_variant_map.insert(p_variant_defines[i].group, LocalVector<int>{});
		}
		group_to_variant_map[p_variant_defines[i].group].push_back(i);

		// Track max size.
		if (p_variant_defines[i].group > max_group_id) {
			max_group_id = p_variant_defines[i].group;
		}
	}

	// Set all to groups to false, then enable those that should be default.
	group_enabled.resize_initialized(max_group_id + 1);
	bool* enabled_ptr = group_enabled.ptrw();
	for (int i = 0; i < p_variant_defines.size(); i++) {
		if (p_variant_defines[i].default_enabled) {
			enabled_ptr[p_variant_defines[i].group] = true;
		}
	}

	if (!shader_cache_user_dir.is_empty()) {
		group_sha256.resize(max_group_id + 1);
		_initialize_cache();
	}
}

void ShaderRD::shaders_embedded_set_lock() { shader_versions_embedded_set_mutex.lock(); }

const ShaderRD::ShaderVersionPairSet& ShaderRD::shaders_embedded_set_get()
{
	return shader_versions_embedded_set;
}

void ShaderRD::shaders_embedded_set_unlock() { shader_versions_embedded_set_mutex.unlock(); }

void ShaderRD::set_shader_cache_user_dir(const String& p_dir) { shader_cache_user_dir = p_dir; }

const String& ShaderRD::get_shader_cache_user_dir() { return shader_cache_user_dir; }

void ShaderRD::set_shader_cache_res_dir(const String& p_dir) { shader_cache_res_dir = p_dir; }

const String& ShaderRD::get_shader_cache_res_dir() { return shader_cache_res_dir; }

void ShaderRD::set_shader_cache_save_compressed(bool p_enable)
{
	shader_cache_save_compressed = p_enable;
}

void ShaderRD::set_shader_cache_save_compressed_zstd(bool p_enable)
{
	shader_cache_save_compressed_zstd = p_enable;
}

void ShaderRD::set_shader_cache_save_debug(bool p_enable) { shader_cache_save_debug = p_enable; }

Vector<RDC::ShaderStageSPIRVData> ShaderRD::compile_stages(
	const Vector<String>& p_stage_sources, const Vector<uint64_t>& p_dynamic_buffers)
{
	RDC::ShaderStageSPIRVData stage;
	Vector<RDC::ShaderStageSPIRVData> stages;
	String error;
	RDC::ShaderStage compilation_failed_stage = RDC::SHADER_STAGE_MAX;
	bool compilation_failed = false;
	for (int64_t i = 0; i < p_stage_sources.size() && !compilation_failed; i++) {
		if (p_stage_sources[i].is_empty()) {
			continue;
		}

		stage.spirv = RD::shader_compile_spirv_from_source(
			RDC::ShaderStage(i), p_stage_sources[i], RDC::SHADER_LANGUAGE_GLSL, &error);
		stage.dynamic_buffers = p_dynamic_buffers;
		stage.shader_stage = RDC::ShaderStage(i);
		if (!stage.spirv.is_empty()) {
			stages.push_back(stage);

		}
		else {
			compilation_failed_stage = RDC::ShaderStage(i);
			compilation_failed = true;
		}
	}

	if (compilation_failed) {
		ERR_PRINT(
			"Error compiling " +
			String(compilation_failed_stage == RDC::SHADER_STAGE_COMPUTE
					   ? "Compute "
					   : (compilation_failed_stage == RDC::SHADER_STAGE_VERTEX ? "Vertex"
																			   : "Fragment")) +
			" shader.");
		ERR_PRINT(error);

#ifdef DEBUG_ENABLED
		ERR_PRINT("code:\n" + p_stage_sources[compilation_failed_stage].get_with_code_lines());
#endif

		return Vector<RDC::ShaderStageSPIRVData>();
	}
	else {
		return stages;
	}
}

PackedByteArray ShaderRD::save_shader_cache_bytes(
	const LocalVector<int>& p_variants, const Vector<Vector<uint8_t>>& p_variant_data)
{
	uint32_t variant_count = p_variants.size();
	PackedByteArray bytes;
	int64_t total_size = 0;
	total_size += 4 + sizeof(uint32_t) * 2;
	for (uint32_t i = 0; i < variant_count; i++) {
		total_size += sizeof(uint32_t) + p_variant_data[p_variants[i]].size();
	}

	bytes.resize(total_size);

	uint8_t* bytes_ptr = bytes.ptrw();
	memcpy(bytes_ptr, shader_file_header, 4);
	bytes_ptr += 4;

	*(uint32_t*)(bytes_ptr) = cache_file_version;
	bytes_ptr += sizeof(uint32_t);

	*(uint32_t*)(bytes_ptr) = variant_count;
	bytes_ptr += sizeof(uint32_t);

	for (uint32_t i = 0; i < variant_count; i++) {
		int variant_id = p_variants[i];
		*(uint32_t*)(bytes_ptr) = uint32_t(p_variant_data[variant_id].size());
		bytes_ptr += sizeof(uint32_t);

		memcpy(bytes_ptr, p_variant_data[variant_id].ptr(), p_variant_data[variant_id].size());
		bytes_ptr += p_variant_data[variant_id].size();
	}

	DEV_ASSERT((bytes.ptrw() + bytes.size()) == bytes_ptr);
	return bytes;
}

String ShaderRD::shader_cache_user_dir;
String ShaderRD::shader_cache_res_dir;
bool ShaderRD::shader_cache_save_compressed = true;
bool ShaderRD::shader_cache_save_compressed_zstd = true;
bool ShaderRD::shader_cache_save_debug = true;

ShaderRD::~ShaderRD()
{
	if (version_owner.get_rid_count() > 0) {
		ERR_PRINT(itos(version_owner.get_rid_count()) + " shaders of type " + String("ShaderRD") +
				  " were never freed");
		LocalVector<RID> remaining = version_owner.get_owned_list();
		for (const RID& E : remaining) {
			Version* v = version_owner.get_or_null(E);
			if (v) {
				_clear_version(v, true);
			}
			version_owner.free(E);
		}
	}
}

void ShaderRD::_compile_version_start(ShaderRD::Version* p_version, int p_group)

{
	// If not using background worker threads, compilation can run directly here or in
	// _compile_version_end
}

void ShaderRD::_compile_version_end(ShaderRD::Version* p_version, int p_group)
{
	ERR_FAIL_COND(!group_to_variant_map.has(p_group));
	const LocalVector<int>& group_variants = group_to_variant_map[p_group];

	CompileData compile_data;
	compile_data.version = p_version;
	compile_data.group = p_group;

	for (uint32_t i = 0; i < group_variants.size(); i++) {
		_compile_variant(i, compile_data);
	}

	bool all_ok = true;
	for (uint32_t i = 0; i < group_variants.size(); i++) {
		int variant_idx = group_variants[i];
		if (variants_enabled[variant_idx] && p_version->variants[variant_idx].is_null()) {
			all_ok = false;
			break;
		}
	}

	if (all_ok) {
		p_version->valid = true;
	}
}

void ShaderRD::_initialize_version(ShaderRD::Version* p_version)
{
	p_version->variants.resize(variant_defines.size());
	p_version->variant_data.resize(variant_defines.size());
	p_version->initialize_needed = false;
	p_version->dirty = false;
}

void ShaderRD::_compile_variant(uint32_t p_variant, CompileData p_data)
{
	uint32_t variant = group_to_variant_map[p_data.group][p_variant];
	if (!variants_enabled[variant]) {
		return; // Variant is disabled, return.
	}

	Vector<String> variant_stage_sources = _build_variant_stage_sources(variant, p_data);
	Vector<RDC::ShaderStageSPIRVData> variant_stages =
		compile_stages(variant_stage_sources, dynamic_buffers);
	ERR_FAIL_COND(variant_stages.is_empty());

	Vector<uint8_t> shader_data =
		RD::shader_compile_binary_from_spirv(variant_stages, name + ":" + itos(variant));
	ERR_FAIL_COND(shader_data.is_empty());

	{
		p_data.version->variants.write[variant] =
			RD::shader_create_from_bytecode(shader_data, p_data.version->variants[variant]);
		p_data.version->variant_data.write[variant] = shader_data;
	}
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/storage_rd/material_storage.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/math/projection.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_render.h"

using namespace RendererRD;

_FORCE_INLINE_ static void _fill_std140_ubo_value(ShaderLanguage::DataType type,
	const Vector<ShaderLanguage::Scalar>& value, uint8_t* data, bool p_use_linear_color)
{
	switch (type) {
	case ShaderLanguage::TYPE_BOOL: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
	} break;
	case ShaderLanguage::TYPE_BVEC2: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC3: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC4: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;
		gui[3] = value[3].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_INT: {
		int32_t* gui = (int32_t*)data;
		gui[0] = value[0].sint;

	} break;
	case ShaderLanguage::TYPE_IVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_UINT: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].uint;

	} break;
	case ShaderLanguage::TYPE_UVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_UVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].uint;
		}

	} break;
	case ShaderLanguage::TYPE_UVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_FLOAT: {
		float* gui = reinterpret_cast<float*>(data);
		gui[0] = value[0].real;

	} break;
	case ShaderLanguage::TYPE_VEC2: {
		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC3: {
		Color c = Color(value[0].real, value[1].real, value[2].real);
		if (p_use_linear_color) {
			c = c.srgb_to_linear();
		}

		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 3; i++) {
			gui[i] = c[i];
		}

	} break;
	case ShaderLanguage::TYPE_VEC4: {
		Color c = Color(value[0].real, value[1].real, value[2].real, value[3].real);
		if (p_use_linear_color) {
			c = c.srgb_to_linear();
		}

		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 4; i++) {
			gui[i] = c[i];
		}
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		float* gui = reinterpret_cast<float*>(data);

		// in std140 members of mat2 are treated as vec4s
		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = 0;
		gui[3] = 0;
		gui[4] = value[2].real;
		gui[5] = value[3].real;
		gui[6] = 0;
		gui[7] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		float* gui = reinterpret_cast<float*>(data);

		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = value[2].real;
		gui[3] = 0;
		gui[4] = value[3].real;
		gui[5] = value[4].real;
		gui[6] = value[5].real;
		gui[7] = 0;
		gui[8] = value[6].real;
		gui[9] = value[7].real;
		gui[10] = value[8].real;
		gui[11] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 16; i++) {
			gui[i] = value[i].real;
		}
	} break;
	default: {
	}
	}
}

_FORCE_INLINE_ static void _fill_std140_ubo_empty(
	ShaderLanguage::DataType type, int p_array_size, uint8_t* data)
{
	if (p_array_size <= 0) {
		p_array_size = 1;
	}

	switch (type) {
	case ShaderLanguage::TYPE_BOOL:
	case ShaderLanguage::TYPE_INT:
	case ShaderLanguage::TYPE_UINT:
	case ShaderLanguage::TYPE_FLOAT: {
		memset(data, 0, 4 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC2:
	case ShaderLanguage::TYPE_IVEC2:
	case ShaderLanguage::TYPE_UVEC2:
	case ShaderLanguage::TYPE_VEC2: {
		memset(data, 0, 8 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC3:
	case ShaderLanguage::TYPE_IVEC3:
	case ShaderLanguage::TYPE_UVEC3:
	case ShaderLanguage::TYPE_VEC3: {
		memset(data, 0, 12 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC4:
	case ShaderLanguage::TYPE_IVEC4:
	case ShaderLanguage::TYPE_UVEC4:
	case ShaderLanguage::TYPE_VEC4: {
		memset(data, 0, 16 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		memset(data, 0, 32 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		memset(data, 0, 48 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		memset(data, 0, 64 * p_array_size);
	} break;

	default: {
	}
	}
}

void RendererRD::MaterialStorage::ShaderData::set_path_hint(const String& p_hint) { path = p_hint; }

void RendererRD::MaterialStorage::ShaderData::set_default_texture_parameter(
	const String& p_name, RID p_texture, int p_index)
{
	if (!p_texture.is_valid()) {
		if (default_texture_params.has(p_name) && default_texture_params[p_name].has(p_index)) {
			default_texture_params[p_name].erase(p_index);

			if (default_texture_params[p_name].is_empty()) {
				default_texture_params.erase(p_name);
			}
		}
	}
	else {
		if (!default_texture_params.has(p_name)) {
			default_texture_params[p_name] = HashMap<int, RID>();
		}
		default_texture_params[p_name][p_index] = p_texture;
	}
}

void RendererRD::MaterialStorage::ShaderData::get_instance_param_list(
	List<InstanceShaderParam>* p_param_list) const
{
}

bool RendererRD::MaterialStorage::ShaderData::is_parameter_texture(const String& p_param) const
{
	if (!uniforms.has(p_param)) {
		return false;
	}

	return uniforms[p_param].is_texture();
}

RDC::PipelineColorBlendState::Attachment
RendererRD::MaterialStorage::ShaderData::blend_mode_to_blend_attachment(BlendMode p_mode)
{
	RDC::PipelineColorBlendState::Attachment attachment;

	switch (p_mode) {
	case BLEND_MODE_MIX: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	} break;
	case BLEND_MODE_ADD: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
	} break;
	case BLEND_MODE_SUB: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_REVERSE_SUBTRACT;
		attachment.color_blend_op = RDC::BLEND_OP_REVERSE_SUBTRACT;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
	} break;
	case BLEND_MODE_MUL: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_DST_COLOR;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ZERO;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_DST_ALPHA;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ZERO;
	} break;
	case BLEND_MODE_ALPHA_TO_COVERAGE: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ZERO;
	} break;
	case BLEND_MODE_PREMULTIPLIED_ALPHA: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RDC::BLEND_OP_ADD;
		attachment.color_blend_op = RDC::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RDC::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	} break;
	case BLEND_MODE_DISABLED:
	default: {
		// Use default attachment values.
	} break;
	}

	return attachment;
}

bool RendererRD::MaterialStorage::ShaderData::blend_mode_uses_blend_alpha(BlendMode p_mode)
{
	switch (p_mode) {
	case BLEND_MODE_MIX:
		return false;
	case BLEND_MODE_ADD:
		return true;
	case BLEND_MODE_SUB:
		return true;
	case BLEND_MODE_MUL:
		return true;
	case BLEND_MODE_ALPHA_TO_COVERAGE:
		return false;
	case BLEND_MODE_PREMULTIPLIED_ALPHA:
		return true;
	case BLEND_MODE_DISABLED:
	default:
		return false;
	}
}

RendererRD::MaterialStorage::MaterialData::~MaterialData()
{
	if (global_buffer_E) {
		// unregister global buffers
		MaterialStorage::global_shader_uniforms.materials_using_buffer.erase(global_buffer_E);
	}

	if (global_texture_E) {
		// unregister global textures

		for (const KeyValue<String, uint64_t>& E : used_global_textures) {
			GlobalShaderUniforms::Variable* v =
				MaterialStorage::global_shader_uniforms.variables.getptr(E.key);
			if (v) {
				v->texture_materials.erase(self);
			}
		}
		// unregister material from those using global textures
		MaterialStorage::global_shader_uniforms.materials_using_texture.erase(global_texture_E);
	}

	for (int i = 0; i < 2; i++) {
		if (uniform_buffer[i].is_valid()) {
			RD::free_rid(uniform_buffer[i]);
		}
	}

	if (is_tex_blit) {
		free_parameters_uniform_set(tex_blit_uniform_set);
	}
}

RID RendererRD::MaterialStorage::MaterialData::get_default_texture_id(
	ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint)
{
	RID rd_texture;

	switch (p_type) {
	case ShaderLanguage::TYPE_ISAMPLER2D:
	case ShaderLanguage::TYPE_USAMPLER2D:
	case ShaderLanguage::TYPE_SAMPLER2D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture =
				RendererRD::TextureStorage::texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_TRANSPARENT);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ANISOTROPY: {
			rd_texture =
				RendererRD::TextureStorage::texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_ANISO);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
			rd_texture =
				RendererRD::TextureStorage::texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_NORMAL);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL: {
			rd_texture =
				RendererRD::TextureStorage::texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_NORMAL);
		} break;
		default: {
			rd_texture =
				RendererRD::TextureStorage::texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_SAMPLERCUBE: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_TRANSPARENT);
		} break;
		default: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_WHITE);
		} break;
		}
	} break;
	case ShaderLanguage::TYPE_SAMPLERCUBEARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_WHITE: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_WHITE);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_TRANSPARENT);
		} break;
		default: { // previously this only had the black texture available. Keeping black as the
				   // default to minimize breaking anything.
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_BLACK);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER3D:
	case ShaderLanguage::TYPE_USAMPLER3D:
	case ShaderLanguage::TYPE_SAMPLER3D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_TRANSPARENT);
		} break;
		default: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
	case ShaderLanguage::TYPE_USAMPLER2DARRAY:
	case ShaderLanguage::TYPE_SAMPLER2DARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_TRANSPARENT);
		} break;
		default: {
			rd_texture = RendererRD::TextureStorage::texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE);
		} break;
		}
	} break;

	default: {
	}
	}

	return rd_texture;
}

void RendererRD::MaterialStorage::MaterialData::free_parameters_uniform_set(RID p_uniform_set)
{
	if (p_uniform_set.is_valid() && RD::uniform_set_is_valid(p_uniform_set)) {
		RD::uniform_set_set_invalidation_callback(p_uniform_set, nullptr, nullptr);
		RD::free_rid(p_uniform_set);
	}
}

void RendererRD::MaterialStorage::MaterialData::set_as_used()
{
	for (int i = 0; i < render_target_cache.size(); i++) {
		render_target_cache[i]->was_used = true;
	}
}

bool RendererRD::MaterialStorage::ShaderData::is_animated() const { return false; }

bool RendererRD::MaterialStorage::ShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode
RendererRD::MaterialStorage::ShaderData::get_native_source_code() const
{
	if (is_tex_blit) {
		return TextureStorage::tex_blit_shader.shader.version_get_native_source_code(
			tex_blit_version);
	}
	return RenderingServerTypes::ShaderNativeSourceCode();
}

Pair<ShaderRD*, RID> RendererRD::MaterialStorage::ShaderData::get_native_shader_and_version() const
{
	if (is_tex_blit) {
		return {&TextureStorage::tex_blit_shader.shader, tex_blit_version};
	}
	return {nullptr, RID()};
}

RendererRD::MaterialStorage::ShaderData::~ShaderData()
{
	if (is_tex_blit && tex_blit_version.is_valid()) {
		TextureStorage::tex_blit_shader.shader.version_free(tex_blit_version);
	}
}

RendererRD::MaterialStorage::ShaderData* RendererRD::MaterialStorage::_create_tex_blit_shader_func()
{
	MaterialStorage::ShaderData* shader_data = memnew(MaterialStorage::ShaderData);
	shader_data->is_tex_blit = true;
	return shader_data;
}

RendererRD::MaterialStorage::MaterialData* RendererRD::MaterialStorage::_create_tex_blit_material_func(
	MaterialStorage::ShaderData* p_shader)
{
	MaterialStorage::MaterialData* material_data = memnew(MaterialStorage::MaterialData);
	material_data->is_tex_blit = true;
	material_data->tex_blit_shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

template void Samplers::append_uniforms(
	LocalVector<RD::Uniform>& p_uniforms, int p_first_index) const;

template void Samplers::append_uniforms(Vector<RD::Uniform>& p_uniforms, int p_first_index) const;

template <typename Collection>
void Samplers::append_uniforms(Collection& p_uniforms, int p_first_index) const
{
	// Binding ids are aligned with samplers_inc.glsl.
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 0,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST][RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 1,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR][RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 2,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 3,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 4,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 5,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 6,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST][RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 7,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR][RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 8,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 9,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 10,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RDC::UNIFORM_TYPE_SAMPLER, p_first_index + 11,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
}

bool Samplers::is_valid() const { return rids[1][1].is_valid(); }

bool Samplers::is_null() const { return rids[1][1].is_null(); }

bool RendererRD::MaterialStorage::free(RID p_rid)
{
	if (owns_shader(p_rid)) {
		shader_free(p_rid);
		return true;
	}
	else if (owns_material(p_rid)) {
		material_free(p_rid);
		return true;
	}

	return false;
}

int32_t RendererRD::MaterialStorage::_global_shader_uniform_allocate(uint32_t p_elements)
{
	int32_t idx = 0;
	while (idx + p_elements <= global_shader_uniforms.buffer_size) {
		if (global_shader_uniforms.buffer_usage[idx].elements == 0) {
			bool valid = true;
			for (uint32_t i = 1; i < p_elements; i++) {
				if (global_shader_uniforms.buffer_usage[idx + i].elements > 0) {
					valid = false;
					idx += i + global_shader_uniforms.buffer_usage[idx + i].elements;
					break;
				}
			}

			if (!valid) {
				continue; // if not valid, idx is in new position
			}

			return idx;
		}
		else {
			idx += global_shader_uniforms.buffer_usage[idx].elements;
		}
	}

	return -1;
}

void RendererRD::MaterialStorage::_global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements)
{
	int32_t prev_chunk = -1;

	for (int32_t i = 0; i < p_elements; i++) {
		int32_t chunk = (p_index + i) / GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;
		if (chunk != prev_chunk) {
			if (!global_shader_uniforms.buffer_dirty_regions[chunk]) {
				global_shader_uniforms.buffer_dirty_regions[chunk] = true;
				global_shader_uniforms.buffer_dirty_region_count++;
			}
		}

		prev_chunk = chunk;
	}
}

void RendererRD::MaterialStorage::global_shader_parameter_remove(const String& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return;
	}
	const GlobalShaderUniforms::Variable& gv = global_shader_uniforms.variables[p_name];

	if (gv.buffer_index >= 0) {
		global_shader_uniforms.buffer_usage[gv.buffer_index].elements = 0;
		global_shader_uniforms.must_update_buffer_materials = true;
	}
	else {
		global_shader_uniforms.must_update_texture_materials = true;
	}

	global_shader_uniforms.variables.erase(p_name);
}

Vector<String> RendererRD::MaterialStorage::global_shader_parameter_get_list()
{
	if (!Engine::is_editor_hint()) {
		ERR_FAIL_V_MSG(Vector<String>(), "This function should never be used outside the "
											 "editor, it can severely damage performance.");
	}

	Vector<String> names;
	for (const KeyValue<String, GlobalShaderUniforms::Variable>& E :
		global_shader_uniforms.variables) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

RSE::GlobalShaderParameterType RendererRD::MaterialStorage::global_shader_parameter_get_type_internal(
	const String& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return RSE::GLOBAL_VAR_TYPE_MAX;
	}

	return global_shader_uniforms.variables[p_name].type;
}

RSE::GlobalShaderParameterType RendererRD::MaterialStorage::global_shader_parameter_get_type(
	const String& p_name)
{
	if (!Engine::is_editor_hint()) {
		ERR_FAIL_V_MSG(RSE::GLOBAL_VAR_TYPE_MAX, "This function should never be used outside the "
												 "editor, it can severely damage performance.");
	}

	return global_shader_parameter_get_type_internal(p_name);
}

void RendererRD::MaterialStorage::global_shader_parameters_clear()
{
	global_shader_uniforms.variables.clear(); // not right but for now enough
}

RID RendererRD::MaterialStorage::global_shader_uniforms_get_storage_buffer()
{
	return global_shader_uniforms.buffer;
}

int32_t RendererRD::MaterialStorage::global_shader_parameters_instance_allocate(RID p_instance)
{
	ERR_FAIL_COND_V(global_shader_uniforms.instance_buffer_pos.has(p_instance), -1);
	int32_t pos = _global_shader_uniform_allocate(ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES);
	global_shader_uniforms.instance_buffer_pos[p_instance] = pos; // save anyway
	ERR_FAIL_COND_V_MSG(pos < 0, -1,
		"Too many instances using shader instance variables. Increase buffer size in Project "
		"Settings.");
	global_shader_uniforms.buffer_usage[pos].elements =
		ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES;
	return pos;
}

void RendererRD::MaterialStorage::global_shader_parameters_instance_free(RID p_instance)
{
	ERR_FAIL_COND(!global_shader_uniforms.instance_buffer_pos.has(p_instance));
	int32_t pos = global_shader_uniforms.instance_buffer_pos[p_instance];
	if (pos >= 0) {
		global_shader_uniforms.buffer_usage[pos].elements = 0;
	}
	global_shader_uniforms.instance_buffer_pos.erase(p_instance);
}

void RendererRD::MaterialStorage::_update_global_shader_uniforms()
{
	if (global_shader_uniforms.buffer_dirty_region_count > 0) {
		uint32_t total_regions = 1 + (global_shader_uniforms.buffer_size /
										 GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE);
		if (total_regions / global_shader_uniforms.buffer_dirty_region_count <= 4) {
			// 25% of regions dirty, just update all buffer
			memset(global_shader_uniforms.buffer_dirty_regions, 0, sizeof(bool) * total_regions);
		}
		else {
			uint32_t region_byte_size = sizeof(GlobalShaderUniforms::Value) *
										GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;

			for (uint32_t i = 0; i < total_regions; i++) {
				if (global_shader_uniforms.buffer_dirty_regions[i]) {
					global_shader_uniforms.buffer_dirty_regions[i] = false;
				}
			}
		}

		global_shader_uniforms.buffer_dirty_region_count = 0;
	}

	if (global_shader_uniforms.must_update_buffer_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_buffer) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, true, false);
		}

		global_shader_uniforms.must_update_buffer_materials = false;
	}

	if (global_shader_uniforms.must_update_texture_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_texture) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, false, true);
		}

		global_shader_uniforms.must_update_texture_materials = false;
	}
}

RID RendererRD::MaterialStorage::shader_allocate() { return shader_owner.allocate_rid(); }

void RendererRD::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	Shader shader;
	shader.mutex = memnew(Mutex);
	shader.data = nullptr;
	shader.type = SHADER_TYPE_MAX;
	shader.embedded = p_embedded;

	shader_owner.initialize_rid(p_rid, shader);

	if (p_embedded) {
		// Add to the global embedded set.
		MutexLock lock(embedded_set_mutex);
		embedded_set.insert(p_rid);
	}
}

void RendererRD::MaterialStorage::shader_free(RID p_rid)
{
	Shader* shader = shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	{
		MutexLock lock(*shader->mutex);

		// make material unreference this
		while (shader->owners.size()) {
			material_set_shader((*shader->owners.begin())->self, RID());
		}

		// clear data if exists
		memdelete(shader->data);
	}

	if (shader->embedded) {
		// Remove from the global embedded set.
		MutexLock lock(embedded_set_mutex);
		embedded_set.erase(p_rid);
	}

	memdelete(shader->mutex);

	shader_owner.free(p_rid);
}

void RendererRD::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	MutexLock lock(*shader->mutex);

	shader->code = p_code;
	String mode_string = ShaderLanguage::get_shader_type(p_code);

	ShaderType new_type;
	if (mode_string == "canvas_item") {
		new_type = SHADER_TYPE_2D;
	}
	else if (mode_string == "particles") {
		new_type = SHADER_TYPE_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_type = SHADER_TYPE_3D;
	}
	else if (mode_string == "sky") {
		new_type = SHADER_TYPE_SKY;
	}
	else if (mode_string == "fog") {
		new_type = SHADER_TYPE_FOG;
	}
	else if (mode_string == "texture_blit") {
		new_type = SHADER_TYPE_TEXTURE_BLIT;
	}
	else {
		new_type = SHADER_TYPE_MAX;
	}

	if (new_type != shader->type) {
		if (shader->data) {
			memdelete(shader->data);
			shader->data = nullptr;
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			material->shader_type = new_type;
			if (material->data) {
				memdelete(material->data);
				material->data = nullptr;
			}
		}

		shader->type = new_type;

		if (new_type < SHADER_TYPE_MAX && shader_data_request_func[new_type]) {
			shader->data = shader_data_request_func[new_type]();
		}
		else {
			shader->type = SHADER_TYPE_MAX; // invalid
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			if (shader->data) {
				material->data = material_get_data_request_function(new_type)(shader->data);
				material->data->self = material->self;
				material->data->set_next_pass(material->next_pass);
				material->data->set_render_priority(material->priority);
			}
			material->shader_type = new_type;
		}

		if (shader->data) {
			for (const KeyValue<String, HashMap<int, RID>>& E :
				shader->default_texture_parameter) {
				for (const KeyValue<int, RID>& E2 : E.value) {
					shader->data->set_default_texture_parameter(E.key, E2.value, E2.key);
				}
			}
		}
	}

	if (shader->data) {
		shader->data->set_path_hint(shader->path_hint);
		shader->data->set_code(p_code);
	}

	for (Material* E : shader->owners) {
		Material* material = E;
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		_material_queue_update(material, true, true);
	}
}

void RendererRD::MaterialStorage::shader_set_path_hint(RID p_shader, const String& p_path)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->path_hint = p_path;
	if (shader->data) {
		shader->data->set_path_hint(p_path);
	}
}

String RendererRD::MaterialStorage::shader_get_code(RID p_shader)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, String());
	return shader->code;
}

void RendererRD::MaterialStorage::shader_set_default_texture_parameter(
	RID p_shader, const String& p_name, RID p_texture, int p_index)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	if (p_texture.is_valid() && TextureStorage::owns_texture(p_texture)) {
		if (!shader->default_texture_parameter.has(p_name)) {
			shader->default_texture_parameter[p_name] = HashMap<int, RID>();
		}
		shader->default_texture_parameter[p_name][p_index] = p_texture;
	}
	else {
		if (shader->default_texture_parameter.has(p_name) &&
			shader->default_texture_parameter[p_name].has(p_index)) {
			shader->default_texture_parameter[p_name].erase(p_index);

			if (shader->default_texture_parameter[p_name].is_empty()) {
				shader->default_texture_parameter.erase(p_name);
			}
		}
	}
	if (shader->data) {
		shader->data->set_default_texture_parameter(p_name, p_texture, p_index);
	}

	{
		MutexLock lock(*shader->mutex);

		for (Material* E : shader->owners) {
			Material* material = E;
			_material_queue_update(material, false, true);
		}
	}
}

RID RendererRD::MaterialStorage::shader_get_default_texture_parameter(
	RID p_shader, const String& p_name, int p_index)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());
	if (shader->default_texture_parameter.has(p_name) &&
		shader->default_texture_parameter[p_name].has(p_index)) {
		return shader->default_texture_parameter[p_name][p_index];
	}

	return RID();
}

void RendererRD::MaterialStorage::shader_set_data_request_function(
	ShaderType p_shader_type, ShaderDataRequestFunction p_function)
{
	ERR_FAIL_INDEX(p_shader_type, SHADER_TYPE_MAX);
	shader_data_request_func[p_shader_type] = p_function;
}

RendererRD::MaterialStorage::ShaderData* RendererRD::MaterialStorage::shader_get_data(RID p_shader)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, nullptr);
	return shader->data;
}

RenderingServerTypes::ShaderNativeSourceCode RendererRD::MaterialStorage::shader_get_native_source_code(
	RID p_shader)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RenderingServerTypes::ShaderNativeSourceCode());
	if (shader->data) {
		return shader->data->get_native_source_code();
	}
	return RenderingServerTypes::ShaderNativeSourceCode();
}

void RendererRD::MaterialStorage::shader_embedded_set_lock() { embedded_set_mutex.lock(); }

const HashSet<RID>& RendererRD::MaterialStorage::shader_embedded_set_get() { return embedded_set; }

void RendererRD::MaterialStorage::shader_embedded_set_unlock() { embedded_set_mutex.unlock(); }

void RendererRD::MaterialStorage::_material_uniform_set_erased(void* p_material)
{
	RID rid = *(RID*)p_material;
	Material* material = MaterialStorage::get_material(rid);
	if (material) {
		if (material->data) {
			// Uniform set may be gone because a dependency was erased. This happens
			// if a texture is deleted, so re-create it.
			MaterialStorage::_material_queue_update(material, false, true);
		}
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	}
}

void RendererRD::MaterialStorage::_material_queue_update(Material* material, bool p_uniform, bool p_texture)
{
	MutexLock lock(material_update_list_mutex);
	material->uniform_dirty = material->uniform_dirty || p_uniform;
	material->texture_dirty = material->texture_dirty || p_texture;

	if (material->update_element.in_list()) {
		return;
	}

	material_update_list.add(&material->update_element);
}

RID RendererRD::MaterialStorage::material_allocate() { return material_owner.allocate_rid(); }

void RendererRD::MaterialStorage::material_initialize(RID p_rid)
{
	material_owner.initialize_rid(p_rid);
	Material* material = material_owner.get_or_null(p_rid);
	material->self = p_rid;
}

void RendererRD::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->data) {
		memdelete(material->data);
		material->data = nullptr;
	}

	if (material->shader) {
		{
			MutexLock lock(*material->shader->mutex);
			material->shader->owners.erase(material);
		}

		material->shader = nullptr;
		material->shader_type = SHADER_TYPE_MAX;
	}

	if (p_shader.is_null()) {
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		material->shader_id = 0;
		return;
	}

	Shader* shader = get_shader(p_shader);
	ERR_FAIL_NULL(shader);

	MutexLock lock(*shader->mutex);

	material->shader = shader;
	material->shader_type = shader->type;
	material->shader_id = p_shader.get_local_index();
	shader->owners.insert(material);

	if (shader->type == SHADER_TYPE_MAX) {
		return;
	}

	ERR_FAIL_NULL(shader->data);

	material->data = material_data_request_func[shader->type](shader->data);
	material->data->self = p_material;
	material->data->set_next_pass(material->next_pass);
	material->data->set_render_priority(material->priority);
	// updating happens later
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	_material_queue_update(material, true, true);
}

RendererRD::MaterialStorage::ShaderData* RendererRD::MaterialStorage::material_get_shader_data(RID p_material)
{
	const MaterialStorage::Material* material = MaterialStorage::get_material(p_material);
	if (material && material->shader && material->shader->data) {
		return material->shader->data;
	}

	return nullptr;
}

void RendererRD::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->next_pass == p_next_material) {
		return;
	}

	material->next_pass = p_next_material;
	if (material->data) {
		material->data->set_next_pass(p_next_material);
	}

	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

void RendererRD::MaterialStorage::material_set_render_priority(RID p_material, int priority)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	material->priority = priority;
	if (material->data) {
		material->data->set_render_priority(priority);
	}
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

bool RendererRD::MaterialStorage::material_is_animated(RID p_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, false);
	if (material->shader && material->shader->data) {
		if (material->shader->data->is_animated()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_is_animated(material->next_pass);
		}
	}
	return false; // by default nothing is animated
}

bool RendererRD::MaterialStorage::material_casts_shadows(RID p_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, true);
	if (material->shader && material->shader->data) {
		if (material->shader->data->casts_shadows()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_casts_shadows(material->next_pass);
		}
	}
	return true; // by default everything casts shadows
}

RSE::CullMode RendererRD::MaterialStorage::material_get_cull_mode(RID p_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, RSE::CULL_MODE_DISABLED);
	ERR_FAIL_NULL_V(material->shader, RSE::CULL_MODE_DISABLED);
	if (material->shader->type == ShaderType::SHADER_TYPE_3D && material->shader->data) {
		RendererSceneRenderImplementation::SceneShaderForwardClustered::ShaderData* sd_clustered =
			dynamic_cast<
				RendererSceneRenderImplementation::SceneShaderForwardClustered::ShaderData*>(
				material->shader->data);
		if (sd_clustered) {
			return (RSE::CullMode)sd_clustered->cull_mode;
		}

		RendererSceneRenderImplementation::SceneShaderForwardMobile::ShaderData* sd_mobile =
			dynamic_cast<RendererSceneRenderImplementation::SceneShaderForwardMobile::ShaderData*>(
				material->shader->data);
		if (sd_mobile) {
			return (RSE::CullMode)sd_mobile->cull_mode;
		}
	}
	return RSE::CULL_MODE_DISABLED;
}

void RendererRD::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	if (material->shader && material->shader->data) {
		material->shader->data->get_instance_param_list(r_parameters);

		if (material->next_pass.is_valid()) {
			material_get_instance_shader_parameters(material->next_pass, r_parameters);
		}
	}
}

void RendererRD::MaterialStorage::material_update_dependency(RID p_material, DependencyTracker* p_instance)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	p_instance->update_dependency(&material->dependency);
	if (material->next_pass.is_valid()) {
		material_update_dependency(material->next_pass, p_instance);
	}
}

Samplers RendererRD::MaterialStorage::samplers_rd_allocate(
	float p_mipmap_bias, RSE::ViewportAnisotropicFiltering anisotropic_filtering_level)
{
	Samplers samplers;
	samplers.mipmap_bias = p_mipmap_bias;
	samplers.anisotropic_filtering_level = (int)anisotropic_filtering_level;
	samplers.use_nearest_mipmap_filter =
		GLOBAL_GET_CACHED(bool, "rendering/textures/default_filters/use_nearest_mipmap_filter");

	RDC::SamplerFilter mip_filter = samplers.use_nearest_mipmap_filter ? RDC::SAMPLER_FILTER_NEAREST
																	   : RDC::SAMPLER_FILTER_LINEAR;
	float anisotropy_max = float(1 << samplers.anisotropic_filtering_level);

	for (int i = 1; i < RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX; i++) {
		for (int j = 1; j < RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX; j++) {
			RDC::SamplerState sampler_state;
			switch (i) {
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.max_lod = 0;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.max_lod = 0;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;

			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
				sampler_state.use_anisotropy = true;
				sampler_state.anisotropy_max = anisotropy_max;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC: {
				sampler_state.mag_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RDC::SAMPLER_FILTER_LINEAR;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
				sampler_state.use_anisotropy = true;
				sampler_state.anisotropy_max = anisotropy_max;

			} break;
			default: {
			}
			}
			switch (j) {
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED: {
				sampler_state.repeat_u = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
				sampler_state.repeat_v = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
				sampler_state.repeat_w = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;

			} break;
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED: {
				sampler_state.repeat_u = RDC::SAMPLER_REPEAT_MODE_REPEAT;
				sampler_state.repeat_v = RDC::SAMPLER_REPEAT_MODE_REPEAT;
				sampler_state.repeat_w = RDC::SAMPLER_REPEAT_MODE_REPEAT;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_MIRROR: {
				sampler_state.repeat_u = RDC::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
				sampler_state.repeat_v = RDC::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
				sampler_state.repeat_w = RDC::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
			} break;
			default: {
			}
			}

			samplers.rids[i][j] = RD::sampler_create(sampler_state);
		}
	}

	return samplers;
}

void RendererRD::MaterialStorage::samplers_rd_free(Samplers& p_samplers)
{
	for (int i = 1; i < RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX; i++) {
		for (int j = 1; j < RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX; j++) {
			if (p_samplers.rids[i][j].is_valid()) {
				RD::free_rid(p_samplers.rids[i][j]);
				p_samplers.rids[i][j] = RID();
			}
		}
	}
}

void RendererRD::MaterialStorage::material_set_data_request_function(
	ShaderType p_shader_type, MaterialStorage::MaterialDataRequestFunction p_function)
{
	ERR_FAIL_INDEX(p_shader_type, SHADER_TYPE_MAX);
	material_data_request_func[p_shader_type] = p_function;
}

RendererRD::MaterialStorage::MaterialDataRequestFunction RendererRD::MaterialStorage::material_get_data_request_function(
	ShaderType p_shader_type)
{
	ERR_FAIL_INDEX_V(p_shader_type, SHADER_TYPE_MAX, nullptr);
	return material_data_request_func[p_shader_type];
}

void RendererRD::MaterialStorage::_update_queued_materials()
{
	SelfList<Material>::List copy;
	{
		MutexLock lock(material_update_list_mutex);
		while (SelfList<Material>* E = material_update_list.first()) {
			DEV_ASSERT(E == &E->self()->update_element);
			material_update_list.remove(E);
			copy.add(E);
		}
	}

	while (SelfList<Material>* E = copy.first()) {
		Material* material = E->self();
		copy.remove(E);
		material->texture_dirty = false;
		material->uniform_dirty = false;
	}
}

void RendererRD::MaterialStorage::ShaderData::set_code(const String& p_code) {}

void RendererRD::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void RendererRD::MaterialStorage::initialize()
{
	default_samplers = samplers_rd_allocate();

	{
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
		quad_index_buffer = RD::index_buffer_create(6, RDC::INDEX_BUFFER_FORMAT_UINT16, pv);
		quad_index_array = RD::index_array_create(quad_index_buffer, 0, 6);
	}

	for (int i = 0; i < SHADER_TYPE_MAX; i++) {
		shader_data_request_func[i] = nullptr;
		material_data_request_func[i] = nullptr;
	}

	static_assert(sizeof(GlobalShaderUniforms::Value) == 16);

	global_shader_uniforms.buffer_size = 65536;
	global_shader_uniforms.buffer_values =
		memnew_arr(GlobalShaderUniforms::Value, global_shader_uniforms.buffer_size);
	memset(global_shader_uniforms.buffer_values, 0,
		sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size);
	global_shader_uniforms.buffer_usage =
		memnew_arr(GlobalShaderUniforms::ValueUsage, global_shader_uniforms.buffer_size);
	global_shader_uniforms.buffer_dirty_regions = memnew_arr(bool,
		1 + (global_shader_uniforms.buffer_size / GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE));
	memset(global_shader_uniforms.buffer_dirty_regions, 0,
		sizeof(bool) * (1 + (global_shader_uniforms.buffer_size /
								GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE)));
	global_shader_uniforms.buffer = RD::storage_buffer_create(
		sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size);
}

void RendererRD::MaterialStorage::finalize()
{
	LocalVector<RID> owned_materials = material_owner.get_owned_list();
	for (const RID& E : owned_materials) {
		material_free(E);
	}
	LocalVector<RID> owned_shaders = shader_owner.get_owned_list();
	for (const RID& E : owned_shaders) {
		shader_free(E);
	}
	global_shader_uniforms.variables.clear();
	global_shader_uniforms.materials_using_buffer.clear();
	global_shader_uniforms.materials_using_texture.clear();
	global_shader_uniforms.instance_buffer_pos.clear();
	if (global_shader_uniforms.buffer.is_valid()) {
		RD::free_rid(global_shader_uniforms.buffer);
		global_shader_uniforms.buffer = RID();
	}
	if (global_shader_uniforms.buffer_values) {
		memdelete_arr(global_shader_uniforms.buffer_values);
		global_shader_uniforms.buffer_values = nullptr;
	}
	if (global_shader_uniforms.buffer_usage) {
		memdelete_arr(global_shader_uniforms.buffer_usage);
		global_shader_uniforms.buffer_usage = nullptr;
	}
	if (global_shader_uniforms.buffer_dirty_regions) {
		memdelete_arr(global_shader_uniforms.buffer_dirty_regions);
		global_shader_uniforms.buffer_dirty_regions = nullptr;
	}
	if (default_samplers.is_valid()) {
		samplers_rd_free(default_samplers);
	}
	if (quad_index_array.is_valid()) {
		RD::free_rid(quad_index_array);
		quad_index_array = RID();
	}
	if (quad_index_buffer.is_valid()) {
		RD::free_rid(quad_index_buffer);
		quad_index_buffer = RID();
	}
	// Clear global uniform tables and cache maps
	global_shader_parameters_clear();
}

void RendererRD::MaterialStorage::material_free(RID p_rid)
{
	Material* material = material_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(material);

	material_set_shader(p_rid, RID());

	if (material->data) {
		memdelete(material->data);
		material->data = nullptr;
	}

	material_owner.free(p_rid);
}
