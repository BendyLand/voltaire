/**************************************************************************/
/*  rasterizer_canvas_gles3.cpp                                           */
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

#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/rasterizer_gles3.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "core/math/transform_interpolator.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/rendering_server_types.h"

void RasterizerCanvasGLES3::_update_transform_2d_to_mat4(
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

void RasterizerCanvasGLES3::_update_transform_2d_to_mat2x4(
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

void RasterizerCanvasGLES3::_update_transform_2d_to_mat2x3(
	const Transform2D& p_transform, float* p_mat2x3)
{
	p_mat2x3[0] = p_transform.columns[0][0];
	p_mat2x3[1] = p_transform.columns[0][1];
	p_mat2x3[2] = p_transform.columns[1][0];
	p_mat2x3[3] = p_transform.columns[1][1];
	p_mat2x3[4] = p_transform.columns[2][0];
	p_mat2x3[5] = p_transform.columns[2][1];
}

void RasterizerCanvasGLES3::_update_transform_to_mat4(const Transform3D& p_transform, float* p_mat4)
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

void RasterizerCanvasGLES3::canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list,
	const Color& p_modulate, Light* p_light_list, Light* p_directional_light_list,
	const Transform2D& p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter,
	RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
	bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info)
{
	Transform2D canvas_transform_inverse = p_canvas_transform.affine_inverse();

	// Clear out any state that may have been left from the 3D pass.
	reset_canvas();

	if (state.canvas_instance_data_buffers[state.current_data_buffer_index].fence != GLsync()) {
		GLint syncStatus;
		glGetSynciv(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence,
			GL_SYNC_STATUS, 1, nullptr, &syncStatus);
		if (syncStatus == GL_UNSIGNALED) {
			// If older than 2 frames, wait for sync OpenGL can have up to 3 frames in flight, any
			// more and we need to sync anyway.
			if (state.canvas_instance_data_buffers[state.current_data_buffer_index]
					.last_frame_used < RendererCompositor::get_frame_number() - 2) {
#ifndef WEB_ENABLED
				// On web, we do nothing as the glSubBufferData will force a sync anyway and WebGL
				// does not like waiting.
				glClientWaitSync(
					state.canvas_instance_data_buffers[state.current_data_buffer_index].fence, 0,
					100000000); // wait for up to 100ms
#endif
				state.canvas_instance_data_buffers[state.current_data_buffer_index]
					.last_frame_used = RendererCompositor::get_frame_number();
				glDeleteSync(
					state.canvas_instance_data_buffers[state.current_data_buffer_index].fence);
				state.canvas_instance_data_buffers[state.current_data_buffer_index].fence =
					GLsync();
			}
			else {
				// Used in last frame or frame before that. OpenGL can get up to two frames behind,
				// so these buffers may still be in use Allocate a new buffer and use that.
				_allocate_instance_data_buffer();
			}
		}
		else {
			// Already finished all rendering commands, we can use it.
			state.canvas_instance_data_buffers[state.current_data_buffer_index].last_frame_used =
				RendererCompositor::get_frame_number();
			glDeleteSync(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence);
			state.canvas_instance_data_buffers[state.current_data_buffer_index].fence = GLsync();
		}
	}

	// setup directional lights if exist

	uint32_t light_count = 0;
	uint32_t directional_light_count = 0;
	{
		Light* l = p_directional_light_list;
		uint32_t index = 0;

		while (l) {
			if (index == data.max_lights_per_render) {
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

			state.light_uniforms[index].position[0] = -canvas_light_dir.x;
			state.light_uniforms[index].position[1] = -canvas_light_dir.y;

			_update_transform_2d_to_mat2x4(
				clight->shadow.directional_xform, state.light_uniforms[index].shadow_matrix);

			state.light_uniforms[index].height = l->height; // 0..1 here

			for (int i = 0; i < 4; i++) {
				state.light_uniforms[index].shadow_color[i] =
					uint8_t(CLAMP(int32_t(l->shadow_color[i] * 255.0), 0, 255));
				state.light_uniforms[index].color[i] = l->color[i];
			}

			state.light_uniforms[index].color[3] *=
				l->energy; // use alpha for energy, so base color can go separate

			if (state.shadow_fb != 0) {
				state.light_uniforms[index].shadow_pixel_size =
					(1.0 / state.shadow_texture_size) * (1.0 + l->shadow_smooth);
				state.light_uniforms[index].shadow_z_far_inv = 1.0 / clight->shadow.z_far;
				state.light_uniforms[index].shadow_y_ofs = clight->shadow.y_offset;
			}
			else {
				state.light_uniforms[index].shadow_pixel_size = 1.0;
				state.light_uniforms[index].shadow_z_far_inv = 1.0;
				state.light_uniforms[index].shadow_y_ofs = 0;
			}

			state.light_uniforms[index].flags = l->blend_mode << LIGHT_FLAGS_BLEND_SHIFT;
			state.light_uniforms[index].flags |= l->shadow_filter << LIGHT_FLAGS_FILTER_SHIFT;

			if (clight->shadow.enabled) {
				state.light_uniforms[index].flags |= LIGHT_FLAGS_HAS_SHADOW;
			}

			l->render_index_cache = index;

			index++;
			l = l->next_ptr;
		}

		light_count = index;
		directional_light_count = light_count;
		state.using_directional_lights = directional_light_count > 0;
	}

	// setup lights if exist

	{
		Light* l = p_light_list;
		uint32_t index = light_count;

		while (l) {
			if (index == data.max_lights_per_render) {
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
			if (!Renderer::canvas->_interpolation_data.interpolation_enabled || !l->interpolated) {
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
			state.light_uniforms[index].position[0] = canvas_light_pos.x;
			state.light_uniforms[index].position[1] = canvas_light_pos.y;

			_update_transform_2d_to_mat2x4(
				l->light_shader_xform.affine_inverse(), state.light_uniforms[index].matrix);
			_update_transform_2d_to_mat2x4(
				l->xform_cache.affine_inverse(), state.light_uniforms[index].shadow_matrix);

			state.light_uniforms[index].height =
				l->height *
				(p_canvas_transform.columns[0].length() + p_canvas_transform.columns[1].length()) *
				0.5; // approximate height conversion to the canvas size, since all calculations are
					 // done in canvas coords to avoid precision loss
			for (int i = 0; i < 4; i++) {
				state.light_uniforms[index].shadow_color[i] =
					uint8_t(CLAMP(int32_t(l->shadow_color[i] * 255.0), 0, 255));
				state.light_uniforms[index].color[i] = l->color[i];
			}

			state.light_uniforms[index].color[3] *=
				l->energy; // use alpha for energy, so base color can go separate

			if (state.shadow_fb != 0) {
				state.light_uniforms[index].shadow_pixel_size =
					(1.0 / state.shadow_texture_size) * (1.0 + l->shadow_smooth);
				state.light_uniforms[index].shadow_z_far_inv = 1.0 / clight->shadow.z_far;
				state.light_uniforms[index].shadow_y_ofs = clight->shadow.y_offset;
			}
			else {
				state.light_uniforms[index].shadow_pixel_size = 1.0;
				state.light_uniforms[index].shadow_z_far_inv = 1.0;
				state.light_uniforms[index].shadow_y_ofs = 0;
			}

			state.light_uniforms[index].flags = l->blend_mode << LIGHT_FLAGS_BLEND_SHIFT;
			state.light_uniforms[index].flags |= l->shadow_filter << LIGHT_FLAGS_FILTER_SHIFT;

			if (clight->shadow.enabled) {
				state.light_uniforms[index].flags |= LIGHT_FLAGS_HAS_SHADOW;
			}

			if (clight->texture.is_valid()) {
				Rect2 atlas_rect =
					GLES3::TextureStorage::texture_atlas_get_texture_rect(
						clight->texture);
				state.light_uniforms[index].atlas_rect[0] = atlas_rect.position.x;
				state.light_uniforms[index].atlas_rect[1] = atlas_rect.position.y;
				state.light_uniforms[index].atlas_rect[2] = atlas_rect.size.width;
				state.light_uniforms[index].atlas_rect[3] = atlas_rect.size.height;

			}
			else {
				state.light_uniforms[index].atlas_rect[0] = 0;
				state.light_uniforms[index].atlas_rect[1] = 0;
				state.light_uniforms[index].atlas_rect[2] = 0;
				state.light_uniforms[index].atlas_rect[3] = 0;
			}

			l->render_index_cache = index;

			index++;
			l = l->next_ptr;
		}

		light_count = index;
	}

	if (light_count > 0) {
		glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_UNIFORM_LOCATION,
			state.canvas_instance_data_buffers[state.current_data_buffer_index].light_ubo);

#ifdef WEB_ENABLED
		glBufferSubData(
			GL_UNIFORM_BUFFER, 0, sizeof(LightUniform) * light_count, state.light_uniforms);
#else
		// On Desktop and mobile we map the memory without synchronizing for maximum speed.
		void* ubo = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(GLES3::LightUniform) * light_count,
			GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		memcpy(ubo, state.light_uniforms, sizeof(GLES3::LightUniform) * light_count);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
#endif

		GLuint texture_atlas = GLES3::TextureStorage::texture_atlas_get_texture();
		if (texture_atlas == 0) {
			GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
				GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
			texture_atlas = tex->tex_id;
		}
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 2);
		glBindTexture(GL_TEXTURE_2D, texture_atlas);
		GLuint shadow_tex = state.shadow_texture;
		if (shadow_tex == 0) {
			GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
				GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
			shadow_tex = tex->tex_id;
		}
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 3);
		glBindTexture(GL_TEXTURE_2D, shadow_tex);
	}

	{
		// update canvas state uniform buffer
		StateBuffer state_buffer;

		Size2i ssize = GLES3::TextureStorage::render_target_get_size(p_to_render_target);

		// If we've overridden the render target's color texture, then we need
		// to invert the Y axis, so 2D texture appear right side up.
		// We're probably rendering directly to an XR device.
		float y_scale =
			GLES3::TextureStorage::render_target_get_override_color(p_to_render_target).is_valid() ? -2.0f
																							 : 2.0f;

		Transform3D screen_transform;
		screen_transform.translate_local(-(ssize.width / 2.0f), -(ssize.height / 2.0f), 0.0f);
		screen_transform.scale(Vector3(2.0f / ssize.width, y_scale / ssize.height, 1.0f));
		_update_transform_to_mat4(screen_transform, state_buffer.screen_transform);
		_update_transform_2d_to_mat4(p_canvas_transform, state_buffer.canvas_transform);

		Transform2D normal_transform = p_canvas_transform;
		normal_transform.columns[0].normalize();
		normal_transform.columns[1].normalize();
		normal_transform.columns[2] = Vector2();
		_update_transform_2d_to_mat4(normal_transform, state_buffer.canvas_normal_transform);

		state_buffer.canvas_modulate[0] = p_modulate.r;
		state_buffer.canvas_modulate[1] = p_modulate.g;
		state_buffer.canvas_modulate[2] = p_modulate.b;
		state_buffer.canvas_modulate[3] = p_modulate.a;

		Size2 render_target_size = GLES3::TextureStorage::render_target_get_size(p_to_render_target);
		state_buffer.screen_pixel_size[0] = 1.0 / render_target_size.x;
		state_buffer.screen_pixel_size[1] = 1.0 / render_target_size.y;

		state_buffer.time = state.time;
		state_buffer.use_pixel_snap = p_snap_2d_vertices_to_pixel;

		state_buffer.directional_light_count = directional_light_count;

		Vector2 canvas_scale = p_canvas_transform.get_scale();

		state_buffer.sdf_to_screen[0] = render_target_size.width / canvas_scale.x;
		state_buffer.sdf_to_screen[1] = render_target_size.height / canvas_scale.y;

		state_buffer.screen_to_sdf[0] = 1.0 / state_buffer.sdf_to_screen[0];
		state_buffer.screen_to_sdf[1] = 1.0 / state_buffer.sdf_to_screen[1];

		Rect2 sdf_rect = GLES3::TextureStorage::render_target_get_sdf_rect(p_to_render_target);
		Rect2 sdf_tex_rect(sdf_rect.position / canvas_scale, sdf_rect.size / canvas_scale);

		state_buffer.sdf_to_tex[0] = 1.0 / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[1] = 1.0 / sdf_tex_rect.size.height;
		state_buffer.sdf_to_tex[2] = -sdf_tex_rect.position.x / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[3] = -sdf_tex_rect.position.y / sdf_tex_rect.size.height;

		state_buffer.tex_to_sdf = 1.0 / ((canvas_scale.x + canvas_scale.y) * 0.5);

		glBindBufferBase(GL_UNIFORM_BUFFER, BASE_UNIFORM_LOCATION,
			state.canvas_instance_data_buffers[state.current_data_buffer_index].state_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(StateBuffer), &state_buffer, GL_STREAM_DRAW);

		GLuint global_buffer = GLES3::MaterialStorage::global_shader_parameters_get_uniform_buffer();

		glBindBufferBase(GL_UNIFORM_BUFFER, GLOBAL_UNIFORM_LOCATION, global_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 5);
	glBindTexture(
		GL_TEXTURE_2D, GLES3::TextureStorage::render_target_get_sdf_texture(p_to_render_target));

	{
		state.default_filter = p_default_filter;
		state.default_repeat = p_default_repeat;
	}

	Size2 render_target_size = GLES3::TextureStorage::render_target_get_size(p_to_render_target);
	glViewport(0, 0, render_target_size.x, render_target_size.y);

	r_sdf_used = false;
	int item_count = 0;
	bool backbuffer_cleared = false;
	bool time_used = false;
	bool material_screen_texture_cached = false;
	bool material_screen_texture_mipmaps_cached = false;
	Rect2 back_buffer_rect;
	bool backbuffer_copy = false;
	bool backbuffer_gen_mipmaps = false;
	bool update_skeletons = false;

	RCR::Item* ci = p_item_list;
	RCR::Item* canvas_group_owner = nullptr;
	bool skip_item = false;

	state.last_item_index = 0;

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

		// Check material for something that may change flow of rendering, but do not bind for now.
		RID material = ci->material_owner == nullptr ? ci->material : ci->material_owner->material;
		if (material.is_valid()) {
			GLES3::MaterialData* md =
				GLES3::MaterialStorage::material_get_data(material, RSE::SHADER_CANVAS_ITEM);
			if (md && md->shader_data->valid) {
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
						GLES3::MeshStorage::mesh_instance_check_for_update(cm->mesh_instance);
						GLES3::MeshStorage::mesh_instance_set_canvas_item_transform(
							cm->mesh_instance, canvas_transform_inverse * ci->final_transform);
						update_skeletons = true;
					}
				}
				c = c->next;
			}
		}

		if (ci->canvas_group_owner != nullptr) {
			if (canvas_group_owner == nullptr) {
				if (update_skeletons) {
					GLES3::MeshStorage::update_mesh_instances();
					update_skeletons = false;
				}
				// Canvas group begins here, render until before this item
				_render_items(p_to_render_target, item_count, canvas_transform_inverse,
					p_light_list, r_sdf_used, false, r_render_info,
					material_screen_texture_mipmaps_cached);
				item_count = 0;

				if (ci->canvas_group_owner->canvas_group->mode !=
					RSE::CANVAS_GROUP_MODE_TRANSPARENT) {
					Rect2i group_rect = ci->canvas_group_owner->global_rect_cache;
					GLES3::TextureStorage::render_target_copy_to_back_buffer(
						p_to_render_target, group_rect, false);
					if (ci->canvas_group_owner->canvas_group->mode ==
						RSE::CANVAS_GROUP_MODE_CLIP_AND_DRAW) {
						ci->canvas_group_owner->use_canvas_group = false;
						items[item_count++] = ci->canvas_group_owner;
					}
				}
				else if (!backbuffer_cleared) {
					GLES3::TextureStorage::render_target_clear_back_buffer(
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
				GLES3::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}
			_render_items(p_to_render_target, item_count, canvas_transform_inverse, p_light_list,
				r_sdf_used, true, r_render_info, material_screen_texture_mipmaps_cached);
			item_count = 0;

			if (ci->canvas_group->blur_mipmaps) {
				GLES3::TextureStorage::render_target_gen_back_buffer_mipmaps(
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
			if (update_skeletons) {
				GLES3::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}
			// render anything pending, including clearing if no items

			_render_items(p_to_render_target, item_count, canvas_transform_inverse, p_light_list,
				r_sdf_used, false, r_render_info, material_screen_texture_mipmaps_cached);
			item_count = 0;

			GLES3::TextureStorage::render_target_copy_to_back_buffer(
				p_to_render_target, back_buffer_rect, backbuffer_gen_mipmaps);

			backbuffer_copy = false;
			material_screen_texture_cached =
				true; // After a backbuffer copy, screen texture makes no further copies.
			material_screen_texture_mipmaps_cached = backbuffer_gen_mipmaps;
			backbuffer_gen_mipmaps = false;
		}

		if (backbuffer_gen_mipmaps) {
			GLES3::TextureStorage::render_target_gen_back_buffer_mipmaps(
				p_to_render_target, back_buffer_rect);

			backbuffer_gen_mipmaps = false;
			material_screen_texture_mipmaps_cached = true;
		}

		// just add all items for now
		if (skip_item) {
			skip_item = false;
		}
		else {
			items[item_count++] = ci;
		}

		if (!ci->next || item_count == MAX_RENDER_ITEMS - 1) {
			if (update_skeletons) {
				GLES3::MeshStorage::update_mesh_instances();
				update_skeletons = false;
			}
			_render_items(p_to_render_target, item_count, canvas_transform_inverse, p_light_list,
				r_sdf_used, canvas_group_owner != nullptr, r_render_info,
				material_screen_texture_mipmaps_cached);
			// then reset
			item_count = 0;
		}

		ci = ci->next;
	}

	if (time_used) {
		Renderer::redraw_request();
	}

	state.canvas_instance_data_buffers[state.current_data_buffer_index].fence =
		glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	// Clear out state used in 2D pass
	reset_canvas();
	state.current_data_buffer_index =
		(state.current_data_buffer_index + 1) % state.canvas_instance_data_buffers.size();
	state.current_instance_buffer_index = 0;
}

void RasterizerCanvasGLES3::_render_items(RID p_to_render_target, int p_item_count,
	const Transform2D& p_canvas_transform_inverse, Light* p_lights, bool& r_sdf_used,
	bool p_to_backbuffer, RenderingServerTypes::RenderInfo* r_render_info,
	bool p_backbuffer_has_mipmaps)
{
	canvas_begin(p_to_render_target, p_to_backbuffer, p_backbuffer_has_mipmaps);

	if (p_item_count <= 0) {
		// Nothing to draw, just call canvas_begin() to clear the render target and return.
		return;
	}

	uint32_t index = 0;
	RCR::Item* current_clip = nullptr;
	GLES3::ShaderData* shader_data_cache = nullptr;

	// Record Batches.
	// First item always forms its own batch.
	bool batch_broken = false;
	_new_batch(batch_broken);

	// Override the start position and index as we want to start from where we finished off last
	// time.
	state.canvas_instance_batches[state.current_batch_index].start = state.last_item_index;
	index = 0;

	for (int i = 0; i < p_item_count; i++) {
		RCR::Item* ci = items[i];

		if (ci->final_clip_owner != state.canvas_instance_batches[state.current_batch_index].clip) {
			_new_batch(batch_broken);
			state.canvas_instance_batches[state.current_batch_index].clip = ci->final_clip_owner;
			current_clip = ci->final_clip_owner;
		}

		RID material = ci->material_owner == nullptr ? ci->material : ci->material_owner->material;
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

		if (material != state.canvas_instance_batches[state.current_batch_index].material) {
			_new_batch(batch_broken);

			GLES3::MaterialData* material_data = nullptr;
			if (material.is_valid()) {
				material_data =
					GLES3::MaterialStorage::material_get_data(material, RSE::SHADER_CANVAS_ITEM);
			}
			shader_data_cache = nullptr;
			if (material_data) {
				if (material_data->shader_data->version.is_valid() &&
					material_data->shader_data->valid) {
					shader_data_cache = material_data->shader_data;
				}
			}

			state.canvas_instance_batches[state.current_batch_index].material = material;
			state.canvas_instance_batches[state.current_batch_index].material_data = material_data;
			if (shader_data_cache) {
				state.canvas_instance_batches[state.current_batch_index].vertex_input_mask =
					shader_data_cache->vertex_input_mask;
			}
		}

		GLES3::ShaderData::BlendMode blend_mode =
			shader_data_cache ? shader_data_cache->blend_mode
							  : GLES3::ShaderData::BLEND_MODE_MIX;

		if (!ci->repeat_size.x && !ci->repeat_size.y) {
			_record_item_commands(ci, p_to_render_target, p_canvas_transform_inverse, current_clip,
				blend_mode, p_lights, index, batch_broken, r_sdf_used, Point2());
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
					_record_item_commands(ci, p_to_render_target, p_canvas_transform_inverse,
						current_clip, blend_mode, p_lights, index, batch_broken, r_sdf_used,
						offset);
				}
			}
		}
	}

	if (state.current_batch_index == 0 && state.canvas_instance_batches[0].instance_count == 0) {
		// Nothing to render, just return.
		return;
	}

	// Copy over all data needed for rendering.
	glBindBuffer(
		GL_ARRAY_BUFFER, state.canvas_instance_data_buffers[state.current_data_buffer_index]
							 .instance_buffers[state.current_instance_buffer_index]);
#ifdef WEB_ENABLED
	glBufferSubData(GL_ARRAY_BUFFER, state.last_item_index * sizeof(InstanceData),
		sizeof(InstanceData) * index, state.instance_data_array);
#else
	// On Desktop and mobile we map the memory without synchronizing for maximum speed.
	void* buffer = glMapBufferRange(GL_ARRAY_BUFFER, state.last_item_index * sizeof(GLES3::InstanceData),
		index * sizeof(GLES3::InstanceData), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	memcpy(buffer, state.instance_data_array, index * sizeof(GLES3::InstanceData));
	glUnmapBuffer(GL_ARRAY_BUFFER);
#endif

	glDisable(GL_SCISSOR_TEST);
	current_clip = nullptr;

	GLES3::ShaderData::BlendMode last_blend_mode = GLES3::ShaderData::BLEND_MODE_MIX;
	Color last_blend_color;

	state.current_tex = RID();

	const uint64_t base_specialization =
		GLES3::Config::float_texture_supported ? 0 : CanvasShaderGLES3::USE_RGBA_SHADOWS;

	for (uint32_t i = 0; i <= state.current_batch_index; i++) {
		// Skipping when there is no instances.
		if (state.canvas_instance_batches[i].instance_count == 0) {
			continue;
		}

		// setup clip
		if (current_clip != state.canvas_instance_batches[i].clip) {
			current_clip = state.canvas_instance_batches[i].clip;
			if (current_clip) {
				glEnable(GL_SCISSOR_TEST);
				glScissor(current_clip->final_clip_rect.position.x,
					current_clip->final_clip_rect.position.y, current_clip->final_clip_rect.size.x,
					current_clip->final_clip_rect.size.y);
			}
			else {
				glDisable(GL_SCISSOR_TEST);
			}
		}

		GLES3::MaterialData* material_data = state.canvas_instance_batches[i].material_data;
		CanvasShaderGLES3::ShaderVariant variant = CanvasShaderGLES3::MODE_DEFAULT;
		uint64_t specialization = state.canvas_instance_batches[i].specialization;
		specialization |= base_specialization;
		RID shader_version = data.canvas_shader_default_version;

		if (material_data) {
			if (material_data->shader_data->version.is_valid() &&
				material_data->shader_data->valid) {
				// Bind uniform buffer and textures
				material_data->bind_uniforms();
				shader_version = material_data->shader_data->version;
			}
		}

		bool success = GLES3::MaterialStorage::shaders->canvas_shader.version_bind_shader(
			shader_version, variant, specialization);
		if (!success) {
			continue;
		}

		// Bind per-batch uniforms.
		GLES3::MaterialStorage::shaders->canvas_shader.version_set_uniform(CanvasShaderGLES3::BATCH_FLAGS,
			state.canvas_instance_batches[i].flags, shader_version, variant, specialization);
		GLES3::MaterialStorage::shaders->canvas_shader.version_set_uniform(
			CanvasShaderGLES3::SPECULAR_SHININESS_IN,
			state.canvas_instance_batches[i].specular_shininess, shader_version, variant,
			specialization);

		GLES3::ShaderData::BlendMode blend_mode = state.canvas_instance_batches[i].blend_mode;
		Color blend_color = state.canvas_instance_batches[i].blend_color;

		if (last_blend_mode != blend_mode || last_blend_color != blend_color) {
			if (last_blend_mode == GLES3::ShaderData::BLEND_MODE_DISABLED) {
				// re-enable it
				glEnable(GL_BLEND);
			}
			else if (blend_mode == GLES3::ShaderData::BLEND_MODE_DISABLED) {
				// disable it
				glDisable(GL_BLEND);
			}

			switch (blend_mode) {
			case GLES3::ShaderData::BLEND_MODE_DISABLED: {
				// Nothing to do here.
			} break;
			case GLES3::ShaderData::BLEND_MODE_LCD: {
				glBlendEquation(GL_FUNC_ADD);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(
						GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				}
				else {
					glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_ONE);
				}
				glBlendColor(blend_color.r, blend_color.g, blend_color.b, blend_color.a);

			} break;
			case GLES3::ShaderData::BLEND_MODE_MIX: {
				glBlendEquation(GL_FUNC_ADD);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(
						GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				}
				else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
			case GLES3::ShaderData::BLEND_MODE_ADD: {
				glBlendEquation(GL_FUNC_ADD);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				}
				else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}

			} break;
			case GLES3::ShaderData::BLEND_MODE_SUB: {
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				}
				else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}
			} break;
			case GLES3::ShaderData::BLEND_MODE_MUL: {
				glBlendEquation(GL_FUNC_ADD);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
				}
				else {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
				}

			} break;
			case GLES3::ShaderData::BLEND_MODE_PMALPHA: {
				glBlendEquation(GL_FUNC_ADD);
				if (state.transparent_render_target) {
					glBlendFuncSeparate(
						GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				}
				else {
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
			default: {
			} break;
			}
			last_blend_mode = blend_mode;
			last_blend_color = blend_color;
		}

		_render_batch(p_lights, i, r_render_info);
	}

	glDisable(GL_SCISSOR_TEST);
	state.current_batch_index = 0;
	state.canvas_instance_batches.clear();
	state.last_item_index += index;
}

void RasterizerCanvasGLES3::_record_item_commands(const RCR::Item* p_item, RID p_render_target,
	const Transform2D& p_canvas_transform_inverse, RCR::Item*& current_clip,
	GLES3::ShaderData::BlendMode p_blend_mode, Light* p_lights, uint32_t& r_index,
	bool& r_batch_broken, bool& r_sdf_used, const Point2& p_repeat_offset)
{
	RSE::CanvasItemTextureFilter texture_filter =
		p_item->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? state.default_filter
																		  : p_item->texture_filter;
	const uint64_t specialization_command_mask =
		~(CanvasShaderGLES3::USE_NINEPATCH | CanvasShaderGLES3::USE_PRIMITIVE |
			CanvasShaderGLES3::USE_ATTRIBUTES | CanvasShaderGLES3::USE_INSTANCING);

	if (texture_filter != state.canvas_instance_batches[state.current_batch_index].filter) {
		_new_batch(r_batch_broken);

		state.canvas_instance_batches[state.current_batch_index].filter = texture_filter;
	}

	Transform2D base_transform = p_item->final_transform;
	if (p_item->repeat_source_item && (p_repeat_offset.x || p_repeat_offset.y)) {
		base_transform.columns[2] +=
			p_item->repeat_source_item->final_transform.basis_xform(p_repeat_offset);
	}
	base_transform = p_canvas_transform_inverse * base_transform;

	Transform2D draw_transform; // Used by transform command

	Color base_color = p_item->final_modulate;
	uint32_t base_flags = 0;
	Size2 texpixel_size;

	bool reclip = false;

	bool skipping = false;

	// TODO: consider making lights a per-batch property and then baking light operations in the
	// shader for better performance.
	uint32_t lights[4] = {0, 0, 0, 0};

	uint16_t light_count = 0;
	uint16_t shadow_mask = 0;

	{
		Light* light = p_lights;

		while (light) {
			if (light->render_index_cache >= 0 && p_item->light_mask & light->item_mask &&
				p_item->z_final >= light->z_min && p_item->z_final <= light->z_max &&
				p_item->global_rect_cache.intersects(light->rect_cache)) {
				uint32_t light_index = light->render_index_cache;
				lights[light_count >> 2] |= light_index << ((light_count & 3) * 8);

				if (p_item->light_mask & light->item_shadow_mask) {
					shadow_mask |= 1 << light_count;
				}

				light_count++;

				if (light_count == data.max_lights_per_item - 1) {
					break;
				}
			}
			light = light->next_ptr;
		}

		base_flags |= light_count << INSTANCE_FLAGS_LIGHT_COUNT_SHIFT;
		base_flags |= shadow_mask << INSTANCE_FLAGS_SHADOW_MASKED_SHIFT;
	}

	bool lights_disabled = light_count == 0 && !state.using_directional_lights;

	if (lights_disabled !=
		bool(state.canvas_instance_batches[state.current_batch_index].specialization &
			 CanvasShaderGLES3::DISABLE_LIGHTING)) {
		_new_batch(r_batch_broken);
		state.canvas_instance_batches[state.current_batch_index].specialization ^=
			CanvasShaderGLES3::DISABLE_LIGHTING;
	}

	const RSE::CanvasItemTextureRepeat base_texture_repeat =
		p_item->texture_repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? state.default_repeat
																		  : p_item->texture_repeat;
	const RCR::Item::Command* c = p_item->commands;
	while (c) {
		if (skipping && c->type != RCR::Item::Command::TYPE_ANIMATION_SLICE) {
			c = c->next;
			continue;
		}

		if (c->type != RCR::Item::Command::TYPE_MESH) {
			// For Meshes, this gets updated below.
			_update_transform_2d_to_mat2x3(
				base_transform * draw_transform, state.instance_data_array[r_index].world);
		}

		// Zero out most fields.
		for (int i = 0; i < 4; i++) {
			state.instance_data_array[r_index].modulation[i] = 0.0;
			state.instance_data_array[r_index].ninepatch_margins[i] = 0.0;
			state.instance_data_array[r_index].src_rect[i] = 0.0;
			state.instance_data_array[r_index].dst_rect[i] = 0.0;
			state.instance_data_array[r_index].lights[i] = uint32_t(0);
		}
		state.instance_data_array[r_index].color_texture_pixel_size[0] = 0.0;
		state.instance_data_array[r_index].color_texture_pixel_size[1] = 0.0;

		state.instance_data_array[r_index].lights[0] = lights[0];
		state.instance_data_array[r_index].lights[1] = lights[1];
		state.instance_data_array[r_index].lights[2] = lights[2];
		state.instance_data_array[r_index].lights[3] = lights[3];

		state.instance_data_array[r_index].flags = base_flags;
		state.instance_data_array[r_index].instance_uniforms_ofs =
			p_item->instance_allocated_shader_uniforms_offset;

		Color blend_color = base_color;
		GLES3::ShaderData::BlendMode blend_mode = p_blend_mode;
		RSE::CanvasItemTextureRepeat texture_repeat = base_texture_repeat;
		if (c->type == RCR::Item::Command::TYPE_RECT) {
			const RCR::Item::CommandRect* rect = reinterpret_cast<const RCR::Item::CommandRect*>(c);
			if (rect->flags & CANVAS_RECT_LCD) {
				blend_mode = GLES3::ShaderData::BLEND_MODE_LCD;
				blend_color = rect->modulate * base_color;
			}

			if (rect->flags & CANVAS_RECT_TILE) {
				texture_repeat = RSE::CanvasItemTextureRepeat::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED;
			}
		}
		if (texture_repeat != state.canvas_instance_batches[state.current_batch_index].repeat) {
			_new_batch(r_batch_broken);
			state.canvas_instance_batches[state.current_batch_index].repeat = texture_repeat;
		}

		if (blend_mode != state.canvas_instance_batches[state.current_batch_index].blend_mode ||
			blend_color != state.canvas_instance_batches[state.current_batch_index].blend_color) {
			_new_batch(r_batch_broken);
			state.canvas_instance_batches[state.current_batch_index].blend_mode = blend_mode;
			state.canvas_instance_batches[state.current_batch_index].blend_color = blend_color;
		}

		switch (c->type) {
		case RCR::Item::Command::TYPE_RECT: {
			const RCR::Item::CommandRect* rect = reinterpret_cast<const RCR::Item::CommandRect*>(c);
			if (rect->texture != state.canvas_instance_batches[state.current_batch_index].tex ||
				state.canvas_instance_batches[state.current_batch_index].command_type !=
					RCR::Item::Command::TYPE_RECT) {
				_new_batch(r_batch_broken);
				state.canvas_instance_batches[state.current_batch_index].tex = rect->texture;
				state.canvas_instance_batches[state.current_batch_index].command_type =
					RCR::Item::Command::TYPE_RECT;
				state.canvas_instance_batches[state.current_batch_index].command = c;
				state.canvas_instance_batches[state.current_batch_index].specialization &=
					specialization_command_mask;
				state.canvas_instance_batches[state.current_batch_index].flags = 0;
			}

			_prepare_canvas_texture(rect->texture,
				state.canvas_instance_batches[state.current_batch_index].filter,
				state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
				texpixel_size);

			Rect2 src_rect;
			Rect2 dst_rect;

			if (rect->texture != RID()) {
				src_rect = (rect->flags & CANVAS_RECT_REGION)
							   ? Rect2(rect->source.position * texpixel_size,
									 rect->source.size * texpixel_size)
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
					state.instance_data_array[r_index].flags |= INSTANCE_FLAGS_TRANSPOSE_RECT;
				}

				if (rect->flags & CANVAS_RECT_CLIP_UV) {
					state.instance_data_array[r_index].flags |= INSTANCE_FLAGS_CLIP_RECT_UV;
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

			if (rect->flags & CANVAS_RECT_MSDF) {
				state.instance_data_array[r_index].flags |= INSTANCE_FLAGS_USE_MSDF;
				state.instance_data_array[r_index].msdf[0] = rect->px_range; // Pixel range.
				state.instance_data_array[r_index].msdf[1] = rect->outline;	 // Outline size.
				state.instance_data_array[r_index].msdf[2] = 0.f;			 // Reserved.
				state.instance_data_array[r_index].msdf[3] = 0.f;			 // Reserved.
			}
			else if (rect->flags & CANVAS_RECT_LCD) {
				state.instance_data_array[r_index].flags |= INSTANCE_FLAGS_USE_LCD;
			}

			state.instance_data_array[r_index].modulation[0] = rect->modulate.r * base_color.r;
			state.instance_data_array[r_index].modulation[1] = rect->modulate.g * base_color.g;
			state.instance_data_array[r_index].modulation[2] = rect->modulate.b * base_color.b;
			state.instance_data_array[r_index].modulation[3] = rect->modulate.a * base_color.a;

			state.instance_data_array[r_index].src_rect[0] = src_rect.position.x;
			state.instance_data_array[r_index].src_rect[1] = src_rect.position.y;
			state.instance_data_array[r_index].src_rect[2] = src_rect.size.width;
			state.instance_data_array[r_index].src_rect[3] = src_rect.size.height;

			state.instance_data_array[r_index].dst_rect[0] = dst_rect.position.x;
			state.instance_data_array[r_index].dst_rect[1] = dst_rect.position.y;
			state.instance_data_array[r_index].dst_rect[2] = dst_rect.size.width;
			state.instance_data_array[r_index].dst_rect[3] = dst_rect.size.height;

			_add_to_batch(r_index, r_batch_broken);
		} break;

		case RCR::Item::Command::TYPE_NINEPATCH: {
			const RCR::Item::CommandNinePatch* np = reinterpret_cast<const RCR::Item::CommandNinePatch*>(c);

			if (np->texture != state.canvas_instance_batches[state.current_batch_index].tex ||
				state.canvas_instance_batches[state.current_batch_index].command_type !=
					RCR::Item::Command::TYPE_NINEPATCH) {
				_new_batch(r_batch_broken);
				state.canvas_instance_batches[state.current_batch_index].tex = np->texture;
				state.canvas_instance_batches[state.current_batch_index].command_type =
					RCR::Item::Command::TYPE_NINEPATCH;
				state.canvas_instance_batches[state.current_batch_index].command = c;
				state.canvas_instance_batches[state.current_batch_index].specialization &=
					specialization_command_mask;
				state.canvas_instance_batches[state.current_batch_index].specialization |=
					CanvasShaderGLES3::USE_NINEPATCH;
				state.canvas_instance_batches[state.current_batch_index].flags = 0;
			}

			_prepare_canvas_texture(np->texture,
				state.canvas_instance_batches[state.current_batch_index].filter,
				state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
				texpixel_size);

			Rect2 src_rect;
			Rect2 dst_rect(
				np->rect.position.x, np->rect.position.y, np->rect.size.x, np->rect.size.y);

			if (np->texture == RID()) {
				texpixel_size = Size2(1, 1);
				src_rect = Rect2(0, 0, 1, 1);

			}
			else {
				if (np->source != Rect2()) {
					src_rect = Rect2(np->source.position.x * texpixel_size.width,
						np->source.position.y * texpixel_size.height,
						np->source.size.x * texpixel_size.width,
						np->source.size.y * texpixel_size.height);
					state.instance_data_array[r_index].color_texture_pixel_size[0] =
						1.0 / np->source.size.width;
					state.instance_data_array[r_index].color_texture_pixel_size[1] =
						1.0 / np->source.size.height;

				}
				else {
					src_rect = Rect2(0, 0, 1, 1);
				}
			}

			state.instance_data_array[r_index].modulation[0] = np->color.r * base_color.r;
			state.instance_data_array[r_index].modulation[1] = np->color.g * base_color.g;
			state.instance_data_array[r_index].modulation[2] = np->color.b * base_color.b;
			state.instance_data_array[r_index].modulation[3] = np->color.a * base_color.a;

			state.instance_data_array[r_index].src_rect[0] = src_rect.position.x;
			state.instance_data_array[r_index].src_rect[1] = src_rect.position.y;
			state.instance_data_array[r_index].src_rect[2] = src_rect.size.width;
			state.instance_data_array[r_index].src_rect[3] = src_rect.size.height;

			state.instance_data_array[r_index].dst_rect[0] = dst_rect.position.x;
			state.instance_data_array[r_index].dst_rect[1] = dst_rect.position.y;
			state.instance_data_array[r_index].dst_rect[2] = dst_rect.size.width;
			state.instance_data_array[r_index].dst_rect[3] = dst_rect.size.height;

			state.instance_data_array[r_index].flags |= int(np->axis_x)
														<< INSTANCE_FLAGS_NINEPATCH_H_MODE_SHIFT;
			state.instance_data_array[r_index].flags |= int(np->axis_y)
														<< INSTANCE_FLAGS_NINEPATCH_V_MODE_SHIFT;

			if (np->draw_center) {
				state.instance_data_array[r_index].flags |= INSTANCE_FLAGS_NINEPACH_DRAW_CENTER;
			}

			state.instance_data_array[r_index].ninepatch_margins[0] = np->margin[SIDE_LEFT];
			state.instance_data_array[r_index].ninepatch_margins[1] = np->margin[SIDE_TOP];
			state.instance_data_array[r_index].ninepatch_margins[2] = np->margin[SIDE_RIGHT];
			state.instance_data_array[r_index].ninepatch_margins[3] = np->margin[SIDE_BOTTOM];

			_add_to_batch(r_index, r_batch_broken);

			// Restore if overridden.
			state.instance_data_array[r_index].color_texture_pixel_size[0] = texpixel_size.x;
			state.instance_data_array[r_index].color_texture_pixel_size[1] = texpixel_size.y;
		} break;

		case RCR::Item::Command::TYPE_POLYGON: {
			const RCR::Item::CommandPolygon* polygon = reinterpret_cast<const RCR::Item::CommandPolygon*>(c);

			// Polygon's can't be batched, so always create a new batch
			_new_batch(r_batch_broken);

			state.canvas_instance_batches[state.current_batch_index].tex = polygon->texture;
			state.canvas_instance_batches[state.current_batch_index].command_type =
				RCR::Item::Command::TYPE_POLYGON;
			state.canvas_instance_batches[state.current_batch_index].command = c;
			state.canvas_instance_batches[state.current_batch_index].specialization &=
				specialization_command_mask;
			state.canvas_instance_batches[state.current_batch_index].specialization |=
				CanvasShaderGLES3::USE_ATTRIBUTES;
			state.canvas_instance_batches[state.current_batch_index].flags = 0;

			_prepare_canvas_texture(polygon->texture,
				state.canvas_instance_batches[state.current_batch_index].filter,
				state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
				texpixel_size);

			state.instance_data_array[r_index].modulation[0] = base_color.r;
			state.instance_data_array[r_index].modulation[1] = base_color.g;
			state.instance_data_array[r_index].modulation[2] = base_color.b;
			state.instance_data_array[r_index].modulation[3] = base_color.a;

			for (int j = 0; j < 4; j++) {
				state.instance_data_array[r_index].src_rect[j] = 0;
				state.instance_data_array[r_index].dst_rect[j] = 0;
				state.instance_data_array[r_index].ninepatch_margins[j] = 0;
			}

			_add_to_batch(r_index, r_batch_broken);
		} break;

		case RCR::Item::Command::TYPE_PRIMITIVE: {
			const RCR::Item::CommandPrimitive* primitive = reinterpret_cast<const RCR::Item::CommandPrimitive*>(c);

			if (primitive->point_count !=
					state.canvas_instance_batches[state.current_batch_index].primitive_points ||
				state.canvas_instance_batches[state.current_batch_index].command_type !=
					RCR::Item::Command::TYPE_PRIMITIVE ||
				primitive->texture !=
					state.canvas_instance_batches[state.current_batch_index].tex) {
				_new_batch(r_batch_broken);
				state.canvas_instance_batches[state.current_batch_index].tex = primitive->texture;
				state.canvas_instance_batches[state.current_batch_index].primitive_points =
					primitive->point_count;
				state.canvas_instance_batches[state.current_batch_index].command_type =
					RCR::Item::Command::TYPE_PRIMITIVE;
				state.canvas_instance_batches[state.current_batch_index].command = c;
				state.canvas_instance_batches[state.current_batch_index].specialization &=
					specialization_command_mask;
				state.canvas_instance_batches[state.current_batch_index].specialization |=
					CanvasShaderGLES3::USE_PRIMITIVE;
				state.canvas_instance_batches[state.current_batch_index].flags = 0;
			}

			_prepare_canvas_texture(state.canvas_instance_batches[state.current_batch_index].tex,
				state.canvas_instance_batches[state.current_batch_index].filter,
				state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
				texpixel_size);

			for (uint32_t j = 0; j < MIN(3u, primitive->point_count); j++) {
				state.instance_data_array[r_index].points[j * 2 + 0] = primitive->points[j].x;
				state.instance_data_array[r_index].points[j * 2 + 1] = primitive->points[j].y;
				state.instance_data_array[r_index].uvs[j * 2 + 0] = primitive->uvs[j].x;
				state.instance_data_array[r_index].uvs[j * 2 + 1] = primitive->uvs[j].y;
				Color col = primitive->colors[j] * base_color;
				state.instance_data_array[r_index].colors[j * 2 + 0] =
					(uint32_t(Math::make_half_float(col.g)) << 16) | Math::make_half_float(col.r);
				state.instance_data_array[r_index].colors[j * 2 + 1] =
					(uint32_t(Math::make_half_float(col.a)) << 16) | Math::make_half_float(col.b);
			}

			_add_to_batch(r_index, r_batch_broken);

			if (primitive->point_count == 4) {
				// Reset base data.
				_update_transform_2d_to_mat2x3(
					base_transform * draw_transform, state.instance_data_array[r_index].world);
				_prepare_canvas_texture(
					state.canvas_instance_batches[state.current_batch_index].tex,
					state.canvas_instance_batches[state.current_batch_index].filter,
					state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
					texpixel_size);
				state.instance_data_array[r_index].lights[0] = lights[0];
				state.instance_data_array[r_index].lights[1] = lights[1];
				state.instance_data_array[r_index].lights[2] = lights[2];
				state.instance_data_array[r_index].lights[3] = lights[3];
				state.instance_data_array[r_index].flags = base_flags;
				state.instance_data_array[r_index].instance_uniforms_ofs =
					p_item->instance_allocated_shader_uniforms_offset;

				for (uint32_t j = 0; j < 3; j++) {
					int offset = j == 0 ? 0 : 1;
					// Second triangle in the quad. Uses vertices 0, 2, 3.
					state.instance_data_array[r_index].points[j * 2 + 0] =
						primitive->points[j + offset].x;
					state.instance_data_array[r_index].points[j * 2 + 1] =
						primitive->points[j + offset].y;
					state.instance_data_array[r_index].uvs[j * 2 + 0] =
						primitive->uvs[j + offset].x;
					state.instance_data_array[r_index].uvs[j * 2 + 1] =
						primitive->uvs[j + offset].y;
					Color col = primitive->colors[j + offset] * base_color;
					state.instance_data_array[r_index].colors[j * 2 + 0] =
						(uint32_t(Math::make_half_float(col.g)) << 16) |
						Math::make_half_float(col.r);
					state.instance_data_array[r_index].colors[j * 2 + 1] =
						(uint32_t(Math::make_half_float(col.a)) << 16) |
						Math::make_half_float(col.b);
				}

				_add_to_batch(r_index, r_batch_broken);
			}
		} break;

		case RCR::Item::Command::TYPE_MESH:
		case RCR::Item::Command::TYPE_MULTIMESH:
		case RCR::Item::Command::TYPE_PARTICLES: {
			// Meshes can't be batched, so always create a new batch.
			_new_batch(r_batch_broken);

			Color modulate(1, 1, 1, 1);
			state.canvas_instance_batches[state.current_batch_index].specialization &=
				specialization_command_mask;
			state.canvas_instance_batches[state.current_batch_index].specialization |=
				CanvasShaderGLES3::USE_ATTRIBUTES;
			state.canvas_instance_batches[state.current_batch_index].flags = 0;
			if (c->type == RCR::Item::Command::TYPE_MESH) {
				const RCR::Item::CommandMesh* m = reinterpret_cast<const RCR::Item::CommandMesh*>(c);
				state.canvas_instance_batches[state.current_batch_index].tex = m->texture;
				_update_transform_2d_to_mat2x3(base_transform * draw_transform * m->transform,
					state.instance_data_array[r_index].world);
				modulate = m->modulate;

			}
			else if (c->type == RCR::Item::Command::TYPE_MULTIMESH) {
				const RCR::Item::CommandMultiMesh* mm = reinterpret_cast<const RCR::Item::CommandMultiMesh*>(c);
				state.canvas_instance_batches[state.current_batch_index].tex = mm->texture;
				state.canvas_instance_batches[state.current_batch_index].specialization |=
					CanvasShaderGLES3::USE_INSTANCING;

				if (GLES3::MeshStorage::multimesh_uses_colors(mm->multimesh)) {
					state.canvas_instance_batches[state.current_batch_index].flags |=
						BATCH_FLAGS_INSTANCING_HAS_COLORS;
				}
				if (GLES3::MeshStorage::multimesh_uses_custom_data(mm->multimesh)) {
					state.canvas_instance_batches[state.current_batch_index].flags |=
						BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;
				}
			}
			else if (c->type == RCR::Item::Command::TYPE_PARTICLES) {
				const RCR::Item::CommandParticles* pt = reinterpret_cast<const RCR::Item::CommandParticles*>(c);
				RID particles = pt->particles;
				state.canvas_instance_batches[state.current_batch_index].tex = pt->texture;
				state.canvas_instance_batches[state.current_batch_index].specialization |=
					CanvasShaderGLES3::USE_INSTANCING;
				state.canvas_instance_batches[state.current_batch_index].flags |=
					BATCH_FLAGS_INSTANCING_HAS_COLORS;
				state.canvas_instance_batches[state.current_batch_index].flags |=
					BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;

				if (GLES3::ParticlesStorage::particles_has_collision(particles) &&
					GLES3::TextureStorage::render_target_is_sdf_enabled(p_render_target)) {
					// Pass collision information.
					Transform2D xform = p_item->final_transform;

					GLuint sdf_texture =
						GLES3::TextureStorage::render_target_get_sdf_texture(p_render_target);

					Rect2 to_screen;
					{
						Rect2 sdf_rect =
							GLES3::TextureStorage::render_target_get_sdf_rect(p_render_target);

						to_screen.size =
							Vector2(1.0 / sdf_rect.size.width, 1.0 / sdf_rect.size.height);
						to_screen.position = -sdf_rect.position * to_screen.size;
					}

					GLES3::ParticlesStorage::particles_set_canvas_sdf_collision(
						pt->particles, true, xform, to_screen, sdf_texture);
				}
				else {
					GLES3::ParticlesStorage::particles_set_canvas_sdf_collision(
						pt->particles, false, Transform2D(), Rect2(), 0);
				}
				r_sdf_used |= GLES3::ParticlesStorage::particles_has_collision(particles);
			}

			state.canvas_instance_batches[state.current_batch_index].command = c;
			state.canvas_instance_batches[state.current_batch_index].command_type = c->type;

			_prepare_canvas_texture(state.canvas_instance_batches[state.current_batch_index].tex,
				state.canvas_instance_batches[state.current_batch_index].filter,
				state.canvas_instance_batches[state.current_batch_index].repeat, r_index,
				texpixel_size);

			state.instance_data_array[r_index].modulation[0] = base_color.r * modulate.r;
			state.instance_data_array[r_index].modulation[1] = base_color.g * modulate.g;
			state.instance_data_array[r_index].modulation[2] = base_color.b * modulate.b;
			state.instance_data_array[r_index].modulation[3] = base_color.a * modulate.a;

			for (int j = 0; j < 4; j++) {
				state.instance_data_array[r_index].src_rect[j] = 0;
				state.instance_data_array[r_index].dst_rect[j] = 0;
				state.instance_data_array[r_index].ninepatch_margins[j] = 0;
			}
			_add_to_batch(r_index, r_batch_broken);
		} break;

		case RCR::Item::Command::TYPE_TRANSFORM: {
			const RCR::Item::CommandTransform* transform = reinterpret_cast<const RCR::Item::CommandTransform*>(c);
			draw_transform = transform->xform;
		} break;

		case RCR::Item::Command::TYPE_CLIP_IGNORE: {
			const RCR::Item::CommandClipIgnore* ci = reinterpret_cast<const RCR::Item::CommandClipIgnore*>(c);
			if (current_clip) {
				if (ci->ignore != reclip) {
					_new_batch(r_batch_broken);
					if (ci->ignore) {
						state.canvas_instance_batches[state.current_batch_index].clip = nullptr;
						reclip = true;
					}
					else {
						state.canvas_instance_batches[state.current_batch_index].clip =
							current_clip;
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

	if (current_clip && reclip) {
		// will make it re-enable clipping if needed afterwards
		current_clip = nullptr;
	}
}

_FORCE_INLINE_ static uint32_t _indices_to_primitives(
	RSE::PrimitiveType p_primitive, uint32_t p_indices)
{
	static const uint32_t divisor[RSE::PRIMITIVE_MAX] = {1, 2, 1, 3, 1};
	static const uint32_t subtractor[RSE::PRIMITIVE_MAX] = {0, 0, 1, 0, 2};
	return (p_indices - subtractor[p_primitive]) / divisor[p_primitive];
}

void RasterizerCanvasGLES3::_render_batch(
	Light* p_lights, uint32_t p_index, RenderingServerTypes::RenderInfo* r_render_info)
{
	ERR_FAIL_NULL(state.canvas_instance_batches[p_index].command);

	// Used by Polygon and Mesh.
	static const GLenum prim[5] = {
		GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP};

	_bind_canvas_texture(state.canvas_instance_batches[p_index].tex,
		state.canvas_instance_batches[p_index].filter,
		state.canvas_instance_batches[p_index].repeat);

	switch (state.canvas_instance_batches[p_index].command_type) {
	case RCR::Item::Command::TYPE_RECT:
	case RCR::Item::Command::TYPE_NINEPATCH: {
		glBindVertexArray(data.indexed_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER,
			state.canvas_instance_data_buffers[state.current_data_buffer_index]
				.instance_buffers[state.canvas_instance_batches[p_index].instance_buffer_index]);
		uint32_t range_start = state.canvas_instance_batches[p_index].start * sizeof(GLES3::InstanceData);
		_enable_attributes(range_start, false);

		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
			state.canvas_instance_batches[p_index].instance_count);
		glBindVertexArray(0);

		if (r_render_info) {
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME] +=
				state.canvas_instance_batches[p_index].instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				2 * state.canvas_instance_batches[p_index].instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}

	} break;

	case RCR::Item::Command::TYPE_POLYGON: {
		const RCR::Item::CommandPolygon* polygon = reinterpret_cast<const RCR::Item::CommandPolygon*>(
			state.canvas_instance_batches[p_index].command);

		GLES3::PolygonBuffers* pb = polygon_buffers.polygons.getptr(polygon->polygon.polygon_id);
		ERR_FAIL_NULL(pb);

		glBindVertexArray(pb->vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER,
			state.canvas_instance_data_buffers[state.current_data_buffer_index]
				.instance_buffers[state.canvas_instance_batches[p_index].instance_buffer_index]);

		uint32_t range_start = state.canvas_instance_batches[p_index].start * sizeof(GLES3::InstanceData);
		_enable_attributes(range_start, false);

		if (pb->color_disabled && pb->color != Color(1.0, 1.0, 1.0, 1.0)) {
			glVertexAttrib4f(RSE::ARRAY_COLOR, pb->color.r, pb->color.g, pb->color.b, pb->color.a);
		}

		if (pb->index_buffer != 0) {
			glDrawElementsInstanced(
				prim[polygon->primitive], pb->count, GL_UNSIGNED_INT, nullptr, 1);
		}
		else {
			glDrawArraysInstanced(prim[polygon->primitive], 0, pb->count, 1);
		}
		glBindVertexArray(0);

		if (pb->color_disabled && pb->color != Color(1.0, 1.0, 1.0, 1.0)) {
			// Reset so this doesn't pollute other draw calls.
			glVertexAttrib4f(RSE::ARRAY_COLOR, 1.0, 1.0, 1.0, 1.0);
		}

		if (r_render_info) {
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME]++;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				_indices_to_primitives(polygon->primitive, pb->count);
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}
	} break;

	case RCR::Item::Command::TYPE_PRIMITIVE: {
		glBindVertexArray(data.canvas_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER,
			state.canvas_instance_data_buffers[state.current_data_buffer_index]
				.instance_buffers[state.canvas_instance_batches[p_index].instance_buffer_index]);
		uint32_t range_start = state.canvas_instance_batches[p_index].start * sizeof(GLES3::InstanceData);
		_enable_attributes(range_start, true);

		const GLenum primitive[5] = {GL_POINTS, GL_POINTS, GL_LINES, GL_TRIANGLES, GL_TRIANGLES};
		int instance_count = state.canvas_instance_batches[p_index].instance_count;
		ERR_FAIL_COND(instance_count <= 0);
		if (instance_count >= 1) {
			glDrawArraysInstanced(
				primitive[state.canvas_instance_batches[p_index].primitive_points], 0,
				state.canvas_instance_batches[p_index].primitive_points, instance_count);
		}
		if (r_render_info) {
			const RSE::PrimitiveType rs_primitive[5] = {RSE::PRIMITIVE_POINTS,
				RSE::PRIMITIVE_POINTS, RSE::PRIMITIVE_LINES, RSE::PRIMITIVE_TRIANGLES,
				RSE::PRIMITIVE_TRIANGLES};
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME] += instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
				_indices_to_primitives(
					rs_primitive[state.canvas_instance_batches[p_index].primitive_points],
					state.canvas_instance_batches[p_index].primitive_points) *
				instance_count;
			r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
							   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
		}

	} break;

	case RCR::Item::Command::TYPE_MESH:
	case RCR::Item::Command::TYPE_MULTIMESH:
	case RCR::Item::Command::TYPE_PARTICLES: {
		RID mesh;
		RID mesh_instance;
		uint32_t instance_count = 1;
		GLuint instance_buffer = 0;
		uint32_t instance_stride = 0;
		uint32_t instance_color_offset = 0;
		bool instance_uses_color = false;
		bool instance_uses_custom_data = false;
		bool use_instancing = false;

		if (state.canvas_instance_batches[p_index].command_type == RCR::Item::Command::TYPE_MESH) {
			const RCR::Item::CommandMesh* m = reinterpret_cast<const RCR::Item::CommandMesh*>(
				state.canvas_instance_batches[p_index].command);
			mesh = m->mesh;
			mesh_instance = m->mesh_instance;

		}
		else if (state.canvas_instance_batches[p_index].command_type ==
				   RCR::Item::Command::TYPE_MULTIMESH) {
			const RCR::Item::CommandMultiMesh* mm = reinterpret_cast<const RCR::Item::CommandMultiMesh*>(
				state.canvas_instance_batches[p_index].command);
			RID multimesh = mm->multimesh;
			mesh = GLES3::MeshStorage::multimesh_get_mesh(multimesh);

			if (GLES3::MeshStorage::multimesh_get_transform_format(multimesh) !=
				RSE::MULTIMESH_TRANSFORM_2D) {
				break;
			}

			instance_count = GLES3::MeshStorage::multimesh_get_instances_to_draw(multimesh);

			if (instance_count == 0) {
				break;
			}

			instance_buffer = GLES3::MeshStorage::multimesh_get_gl_buffer(multimesh);
			instance_stride = GLES3::MeshStorage::multimesh_get_stride(multimesh);
			instance_color_offset = GLES3::MeshStorage::multimesh_get_color_offset(multimesh);
			instance_uses_color = GLES3::MeshStorage::multimesh_uses_colors(multimesh);
			instance_uses_custom_data = GLES3::MeshStorage::multimesh_uses_custom_data(multimesh);
			use_instancing = true;

		}
		else if (state.canvas_instance_batches[p_index].command_type ==
				   RCR::Item::Command::TYPE_PARTICLES) {
			const RCR::Item::CommandParticles* pt = reinterpret_cast<const RCR::Item::CommandParticles*>(
				state.canvas_instance_batches[p_index].command);
			RID particles = pt->particles;
			mesh = GLES3::ParticlesStorage::particles_get_draw_pass_mesh(particles, 0);

			ERR_BREAK(GLES3::ParticlesStorage::particles_get_mode(particles) != RSE::PARTICLES_MODE_2D);
			GLES3::ParticlesStorage::particles_request_process(particles);

			if (GLES3::ParticlesStorage::particles_is_inactive(particles)) {
				break;
			}

			Renderer::redraw_request(); // Active particles means redraw request.

			int dpc = GLES3::ParticlesStorage::particles_get_draw_passes(particles);
			if (dpc == 0) {
				break; // Nothing to draw.
			}

			instance_count = GLES3::ParticlesStorage::particles_get_amount(particles);
			instance_buffer = GLES3::ParticlesStorage::particles_get_gl_buffer(particles);
			instance_stride =
				12; // 8 bytes for instance transform and 4 bytes for packed color and custom.
			instance_color_offset = 8; // 8 bytes for instance transform.
			instance_uses_color = true;
			instance_uses_custom_data = true;
			use_instancing = true;
		}

		ERR_FAIL_COND(mesh.is_null());

		uint32_t surf_count = GLES3::MeshStorage::mesh_get_surface_count(mesh);

		for (uint32_t j = 0; j < surf_count; j++) {
			void* surface = GLES3::MeshStorage::mesh_get_surface(mesh, j);

			RSE::PrimitiveType primitive = GLES3::MeshStorage::mesh_surface_get_primitive(surface);
			ERR_CONTINUE(primitive < 0 || primitive >= RSE::PRIMITIVE_MAX);

			GLuint vertex_array_gl = 0;
			GLuint index_array_gl = 0;

			uint64_t vertex_input_mask = state.canvas_instance_batches[p_index].vertex_input_mask;

			if (mesh_instance.is_valid()) {
				GLES3::MeshStorage::mesh_instance_surface_get_vertex_arrays_and_format(
					mesh_instance, j, vertex_input_mask, false, vertex_array_gl);
			}
			else {
				GLES3::MeshStorage::mesh_surface_get_vertex_arrays_and_format(
					surface, vertex_input_mask, false, vertex_array_gl);
			}

			index_array_gl = GLES3::MeshStorage::mesh_surface_get_index_buffer(surface, 0);
			bool use_index_buffer = false;
			glBindVertexArray(vertex_array_gl);
			glBindBuffer(GL_ARRAY_BUFFER,
				state.canvas_instance_data_buffers[state.current_data_buffer_index].instance_buffers
					[state.canvas_instance_batches[p_index].instance_buffer_index]);

			uint32_t range_start =
				state.canvas_instance_batches[p_index].start * sizeof(GLES3::InstanceData);
			_enable_attributes(range_start, false, instance_count);

			if (index_array_gl != 0) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_array_gl);
				use_index_buffer = true;
			}

			if (use_instancing) {
				if (instance_buffer == 0) {
					break;
				}
				// Bind instance buffers.
				glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, instance_stride * sizeof(float),
					CAST_INT_TO_UCHAR_PTR(0));
				glVertexAttribDivisor(1, 1);
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, instance_stride * sizeof(float),
					CAST_INT_TO_UCHAR_PTR(4 * 4));
				glVertexAttribDivisor(2, 1);

				if (instance_uses_color || instance_uses_custom_data) {
					glEnableVertexAttribArray(5);
					glVertexAttribIPointer(5, 4, GL_UNSIGNED_INT, instance_stride * sizeof(float),
						CAST_INT_TO_UCHAR_PTR(instance_color_offset * sizeof(float)));
					glVertexAttribDivisor(5, 1);
				}
				else {
					// Set all default instance color and custom data values to 1.0 or 0.0 using a
					// compressed format.
					uint16_t zero = Math::make_half_float(0.0f);
					uint16_t one = Math::make_half_float(1.0f);
					GLuint default_color = (uint32_t(one) << 16) | one;
					GLuint default_custom = (uint32_t(zero) << 16) | zero;
					glVertexAttribI4ui(
						5, default_color, default_color, default_custom, default_custom);
				}
			}

			GLenum primitive_gl = prim[int(primitive)];

			uint32_t vertex_count =
				GLES3::MeshStorage::mesh_surface_get_vertices_drawn_count(surface);

			if (use_index_buffer) {
				glDrawElementsInstanced(primitive_gl, vertex_count,
					GLES3::MeshStorage::mesh_surface_get_index_type(surface), nullptr,
					instance_count);
			}
			else {
				glDrawArraysInstanced(primitive_gl, 0, vertex_count, instance_count);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			if (use_instancing) {
				glDisableVertexAttribArray(5);
				glDisableVertexAttribArray(8);
				glDisableVertexAttribArray(9);
				glDisableVertexAttribArray(10);
			}
			if (r_render_info) {
				// Meshes, Particles, and MultiMesh are always just one object with one draw call.
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME]++;
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
					_indices_to_primitives(primitive, vertex_count) * instance_count;
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

void RasterizerCanvasGLES3::_add_to_batch(uint32_t& r_index, bool& r_batch_broken)
{
	state.canvas_instance_batches[state.current_batch_index].instance_count++;
	r_index++;
	if (r_index + state.last_item_index >= data.max_instances_per_buffer) {
		// Copy over all data needed for rendering right away
		// then go back to recording item commands.
		glBindBuffer(
			GL_ARRAY_BUFFER, state.canvas_instance_data_buffers[state.current_data_buffer_index]
								 .instance_buffers[state.current_instance_buffer_index]);
#ifdef WEB_ENABLED
		glBufferSubData(GL_ARRAY_BUFFER, state.last_item_index * sizeof(InstanceData),
			sizeof(InstanceData) * r_index, state.instance_data_array);
#else
		// On Desktop and mobile we map the memory without synchronizing for maximum speed.
		void* buffer =
			glMapBufferRange(GL_ARRAY_BUFFER, state.last_item_index * sizeof(GLES3::InstanceData),
				r_index * sizeof(GLES3::InstanceData), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		memcpy(buffer, state.instance_data_array, r_index * sizeof(GLES3::InstanceData));
		glUnmapBuffer(GL_ARRAY_BUFFER);
#endif
		_allocate_instance_buffer();
		r_index = 0;
		state.last_item_index = 0;
		r_batch_broken = false; // Force a new batch to be created
		_new_batch(r_batch_broken);
		state.canvas_instance_batches[state.current_batch_index].start = 0;
	}
}

void RasterizerCanvasGLES3::_new_batch(bool& r_batch_broken)
{
	if (state.canvas_instance_batches.is_empty()) {
		GLES3::Batch new_batch;
		new_batch.instance_buffer_index = state.current_instance_buffer_index;
		state.canvas_instance_batches.push_back(new_batch);
		return;
	}

	if (r_batch_broken ||
		state.canvas_instance_batches[state.current_batch_index].instance_count == 0) {
		return;
	}

	r_batch_broken = true;

	// Copy the properties of the current batch, we will manually update the things that changed.
	GLES3::Batch new_batch = state.canvas_instance_batches[state.current_batch_index];
	new_batch.instance_count = 0;
	new_batch.start = state.canvas_instance_batches[state.current_batch_index].start +
					  state.canvas_instance_batches[state.current_batch_index].instance_count;
	new_batch.instance_buffer_index = state.current_instance_buffer_index;
	state.current_batch_index++;
	state.canvas_instance_batches.push_back(new_batch);
}

void RasterizerCanvasGLES3::_enable_attributes(uint32_t p_start, bool p_primitive, uint32_t p_rate)
{
	uint32_t split = p_primitive ? 13 : 14;
	for (uint32_t i = 8; i < split; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, sizeof(GLES3::InstanceData),
			CAST_INT_TO_UCHAR_PTR(p_start + (i - 8) * 4 * sizeof(float)));
		glVertexAttribDivisor(i, p_rate);
	}
	for (uint32_t i = split; i <= 15; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribIPointer(i, 4, GL_UNSIGNED_INT, sizeof(GLES3::InstanceData),
			CAST_INT_TO_UCHAR_PTR(p_start + (i - 8) * 4 * sizeof(float)));
		glVertexAttribDivisor(i, p_rate);
	}
}

RID RasterizerCanvasGLES3::light_create()
{
	CanvasLight canvas_light;
	return canvas_light_owner.make_rid(canvas_light);
}

void RasterizerCanvasGLES3::light_set_texture(RID p_rid, RID p_texture)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(cl);
	if (cl->texture == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !GLES3::TextureStorage::owns_texture(p_texture));

	if (cl->texture.is_valid()) {
		GLES3::TextureStorage::texture_remove_from_texture_atlas(cl->texture);
	}
	cl->texture = p_texture;

	if (cl->texture.is_valid()) {
		GLES3::TextureStorage::texture_add_to_texture_atlas(cl->texture);
	}
}

void RasterizerCanvasGLES3::light_set_use_shadow(RID p_rid, bool p_enable)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(cl);

	cl->shadow.enabled = p_enable;
}

void RasterizerCanvasGLES3::light_update_shadow(RID p_rid, int p_shadow_index,
	const Transform2D& p_light_xform, int p_light_mask, float p_near, float p_far,
	LightOccluderInstance* p_occluders, const Rect2& p_light_rect)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_COND(!cl->shadow.enabled);

	_update_shadow_atlas();

	cl->shadow.z_far = p_far;
	cl->shadow.y_offset = float(p_shadow_index * 2 + 1) / float(data.max_lights_per_render * 2);

	glBindFramebuffer(GL_FRAMEBUFFER, state.shadow_fb);
	glViewport(0, p_shadow_index * 2, state.shadow_texture_size, 2);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	glEnable(GL_SCISSOR_TEST);
	glScissor(0, p_shadow_index * 2, state.shadow_texture_size, 2);
	glClearColor(p_far, p_far, p_far, 1.0);
	RasterizerUtilGLES3::clear_depth(1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	RSE::CanvasOccluderPolygonCullMode cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;

	CanvasOcclusionShaderGLES3::ShaderVariant variant =
		GLES3::Config::float_texture_supported ? CanvasOcclusionShaderGLES3::MODE_SHADOW
											   : CanvasOcclusionShaderGLES3::MODE_SHADOW_RGBA;
	bool success = shadow_render.shader.version_bind_shader(shadow_render.shader_version, variant);
	if (!success) {
		return;
	}

	Projection projection;
	{
		real_t fov = 90;
		real_t nearp = p_near;
		real_t farp = p_far;
		real_t aspect = 1.0;

		real_t ymax = nearp * Math::tan(Math::deg_to_rad(fov * 0.5));
		real_t ymin = -ymax;
		real_t xmin = ymin * aspect;
		real_t xmax = ymax * aspect;

		projection.set_frustum(xmin, xmax, ymin, ymax, nearp, farp);
	}

	// Precomputed:
	// Vector3 cam_target = Basis::from_euler(Vector3(0, 0, Math::TAU * ((i + 3)
	// / 4.0))).xform(Vector3(0, 1, 0)); projection = projection *
	// Projection(Transform3D().looking_at(cam_targets[i], Vector3(0, 0, -1)).affine_inverse());
	const Projection projections[4] = {
		projection * Projection(Vector4(0, 0, -1, 0), Vector4(1, 0, 0, 0), Vector4(0, -1, 0, 0),
						 Vector4(0, 0, 0, 1)),

		projection * Projection(Vector4(-1, 0, 0, 0), Vector4(0, 0, -1, 0), Vector4(0, -1, 0, 0),
						 Vector4(0, 0, 0, 1)),

		projection * Projection(Vector4(0, 0, 1, 0), Vector4(-1, 0, 0, 0), Vector4(0, -1, 0, 0),
						 Vector4(0, 0, 0, 1)),

		projection * Projection(Vector4(1, 0, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, -1, 0, 0),
						 Vector4(0, 0, 0, 1))

	};

	for (int i = 0; i < 4; i++) {
		glViewport((state.shadow_texture_size / 4) * i, p_shadow_index * 2,
			(state.shadow_texture_size / 4), 2);

		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::PROJECTION,
			projections[i], shadow_render.shader_version, variant);

		static const Vector2 directions[4] = {
			Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1)};
		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::DIRECTION,
			directions[i].x, directions[i].y, shadow_render.shader_version, variant);
		shadow_render.shader.version_set_uniform(
			CanvasOcclusionShaderGLES3::Z_FAR, p_far, shadow_render.shader_version, variant);

		LightOccluderInstance* instance = p_occluders;

		while (instance) {
			OccluderPolygon* co = occluder_polygon_owner.get_or_null(instance->occluder);

			if (!co || co->vertex_array == 0 || !(p_light_mask & instance->light_mask)) {
				instance = instance->next;
				continue;
			}

			Transform2D modelview = p_light_xform * instance->xform_cache;
			shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW1,
				modelview.columns[0][0], modelview.columns[1][0], 0, modelview.columns[2][0],
				shadow_render.shader_version, variant);
			shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW2,
				modelview.columns[0][1], modelview.columns[1][1], 0, modelview.columns[2][1],
				shadow_render.shader_version, variant);

			if (co->cull_mode != cull_mode) {
				if (co->cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED) {
					glDisable(GL_CULL_FACE);
				}
				else {
					if (cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED) {
						// Last time was disabled, so enable and set proper face.
						glEnable(GL_CULL_FACE);
					}
					glCullFace(co->cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE
								   ? GL_FRONT
								   : GL_BACK);
				}
				cull_mode = co->cull_mode;
			}

			glBindVertexArray(co->vertex_array);
			glDrawElements(GL_TRIANGLES, 3 * co->line_point_count, GL_UNSIGNED_SHORT, nullptr);

			instance = instance->next;
		}
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
}

void RasterizerCanvasGLES3::light_update_directional_shadow(RID p_rid, int p_shadow_index,
	const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
	const Rect2& p_clip_rect, LightOccluderInstance* p_occluders)
{
	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_COND(!cl->shadow.enabled);

	_update_shadow_atlas();

	Vector2 light_dir = p_light_xform.columns[1].normalized();

	Vector2 center = p_clip_rect.get_center();

	float to_edge_distance =
		Math::abs(light_dir.dot(p_clip_rect.get_support(-light_dir)) - light_dir.dot(center));

	Vector2 from_pos = center - light_dir * (to_edge_distance + p_cull_distance);
	float distance = to_edge_distance * 2.0 + p_cull_distance;
	float half_size =
		p_clip_rect.size.length() * 0.5; // shadow length, must keep this no matter the angle

	cl->shadow.z_far = distance;
	cl->shadow.y_offset = float(p_shadow_index * 2 + 1) / float(data.max_lights_per_render * 2);

	Transform2D to_light_xform;

	to_light_xform[2] = from_pos;
	to_light_xform[1] = light_dir;
	to_light_xform[0] = -light_dir.orthogonal();

	to_light_xform.invert();

	glBindFramebuffer(GL_FRAMEBUFFER, state.shadow_fb);
	glViewport(0, p_shadow_index * 2, state.shadow_texture_size, 2);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	glEnable(GL_SCISSOR_TEST);
	glScissor(0, p_shadow_index * 2, state.shadow_texture_size, 2);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	RasterizerUtilGLES3::clear_depth(1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	RSE::CanvasOccluderPolygonCullMode cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;

	CanvasOcclusionShaderGLES3::ShaderVariant variant =
		GLES3::Config::float_texture_supported ? CanvasOcclusionShaderGLES3::MODE_SHADOW
											   : CanvasOcclusionShaderGLES3::MODE_SHADOW_RGBA;
	bool success = shadow_render.shader.version_bind_shader(shadow_render.shader_version, variant);
	if (!success) {
		return;
	}

	Projection projection;
	projection.set_orthogonal(-half_size, half_size, -0.5, 0.5, 0.0, distance);
	projection =
		projection *
		Projection(Transform3D().looking_at(Vector3(0, 1, 0), Vector3(0, 0, -1)).affine_inverse());

	shadow_render.shader.version_set_uniform(
		CanvasOcclusionShaderGLES3::PROJECTION, projection, shadow_render.shader_version, variant);
	shadow_render.shader.version_set_uniform(
		CanvasOcclusionShaderGLES3::DIRECTION, 0.0, 1.0, shadow_render.shader_version, variant);
	shadow_render.shader.version_set_uniform(
		CanvasOcclusionShaderGLES3::Z_FAR, distance, shadow_render.shader_version, variant);

	LightOccluderInstance* instance = p_occluders;

	while (instance) {
		OccluderPolygon* co = occluder_polygon_owner.get_or_null(instance->occluder);

		if (!co || co->vertex_array == 0 || !(p_light_mask & instance->light_mask)) {
			instance = instance->next;
			continue;
		}

		Transform2D modelview = to_light_xform * instance->xform_cache;
		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW1,
			modelview.columns[0][0], modelview.columns[1][0], 0, modelview.columns[2][0],
			shadow_render.shader_version, variant);
		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW2,
			modelview.columns[0][1], modelview.columns[1][1], 0, modelview.columns[2][1],
			shadow_render.shader_version, variant);

		if (co->cull_mode != cull_mode) {
			if (co->cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED) {
				glDisable(GL_CULL_FACE);
			}
			else {
				if (cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED) {
					// Last time was disabled, so enable and set proper face.
					glEnable(GL_CULL_FACE);
				}
				glCullFace(co->cull_mode == RSE::CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE ? GL_FRONT
																						: GL_BACK);
			}
			cull_mode = co->cull_mode;
		}

		glBindVertexArray(co->vertex_array);
		glDrawElements(GL_TRIANGLES, 3 * co->line_point_count, GL_UNSIGNED_SHORT, nullptr);

		instance = instance->next;
	}

	Transform2D to_shadow;
	to_shadow.columns[0].x = 1.0 / -(half_size * 2.0);
	to_shadow.columns[2].x = 0.5;

	cl->shadow.directional_xform = to_shadow * to_light_xform;

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
}

void RasterizerCanvasGLES3::_update_shadow_atlas()
{
	if (state.shadow_fb == 0) {
		glActiveTexture(GL_TEXTURE0);

		glGenFramebuffers(1, &state.shadow_fb);
		glBindFramebuffer(GL_FRAMEBUFFER, state.shadow_fb);

		glGenRenderbuffers(1, &state.shadow_depth_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, state.shadow_depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, state.shadow_texture_size,
			data.max_lights_per_render * 2);
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, state.shadow_depth_buffer);

		glGenTextures(1, &state.shadow_texture);
		glBindTexture(GL_TEXTURE_2D, state.shadow_texture);
		if (GLES3::Config::float_texture_supported) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, state.shadow_texture_size,
				data.max_lights_per_render * 2, 0, GL_RED, GL_FLOAT, nullptr);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, state.shadow_texture_size,
				data.max_lights_per_render * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.shadow_texture, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteFramebuffers(1, &state.shadow_fb);
			glDeleteTextures(1, &state.shadow_texture);
			glDeleteRenderbuffers(1, &state.shadow_depth_buffer);
			state.shadow_fb = 0;
			state.shadow_texture = 0;
			state.shadow_depth_buffer = 0;
			WARN_PRINT("Could not create RCR::CanvasItem shadow atlas, status: " +
					   GLES3::TextureStorage::get_framebuffer_error(status));
		}
		GLES3::Utilities::texture_allocated_data(state.shadow_texture,
			state.shadow_texture_size * data.max_lights_per_render * 2 * 4,
			"2D shadow atlas texture");
		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	}
}

void RasterizerCanvasGLES3::render_sdf(RID p_render_target, LightOccluderInstance* p_occluders)
{
	GLuint fb = GLES3::TextureStorage::render_target_get_sdf_framebuffer(p_render_target);
	Rect2i rect = GLES3::TextureStorage::render_target_get_sdf_rect(p_render_target);

	Transform2D to_sdf;
	to_sdf.columns[0] *= rect.size.width;
	to_sdf.columns[1] *= rect.size.height;
	to_sdf.columns[2] = rect.position;

	Transform2D to_clip;
	to_clip.columns[0] *= 2.0;
	to_clip.columns[1] *= 2.0;
	to_clip.columns[2] = -Vector2(1.0, 1.0);

	to_clip = to_clip * to_sdf.affine_inverse();

	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glViewport(0, 0, rect.size.width, rect.size.height);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	CanvasOcclusionShaderGLES3::ShaderVariant variant = CanvasOcclusionShaderGLES3::MODE_SDF;
	bool success = shadow_render.shader.version_bind_shader(shadow_render.shader_version, variant);
	if (!success) {
		return;
	}

	shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::PROJECTION, Projection(),
		shadow_render.shader_version, variant);
	shadow_render.shader.version_set_uniform(
		CanvasOcclusionShaderGLES3::DIRECTION, 0.0, 0.0, shadow_render.shader_version, variant);
	shadow_render.shader.version_set_uniform(
		CanvasOcclusionShaderGLES3::Z_FAR, 0.0, shadow_render.shader_version, variant);

	LightOccluderInstance* instance = p_occluders;

	while (instance) {
		OccluderPolygon* oc = occluder_polygon_owner.get_or_null(instance->occluder);

		if (!oc || oc->sdf_vertex_array == 0 || !instance->sdf_collision) {
			instance = instance->next;
			continue;
		}

		Transform2D modelview = to_clip * instance->xform_cache;
		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW1,
			modelview.columns[0][0], modelview.columns[1][0], 0, modelview.columns[2][0],
			shadow_render.shader_version, variant);
		shadow_render.shader.version_set_uniform(CanvasOcclusionShaderGLES3::MODELVIEW2,
			modelview.columns[0][1], modelview.columns[1][1], 0, modelview.columns[2][1],
			shadow_render.shader_version, variant);

		glBindVertexArray(oc->sdf_vertex_array);
		glDrawElements(oc->sdf_is_lines ? GL_LINES : GL_TRIANGLES, oc->sdf_index_count,
			GL_UNSIGNED_INT, nullptr);

		instance = instance->next;
	}

	GLES3::TextureStorage::render_target_sdf_process(p_render_target); // done rendering, process it
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

RID RasterizerCanvasGLES3::occluder_polygon_create()
{
	OccluderPolygon occluder;

	return occluder_polygon_owner.make_rid(occluder);
}

void RasterizerCanvasGLES3::occluder_polygon_set_shape(
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

	if (oc->line_point_count != lines.size() && oc->vertex_array != 0) {
		glDeleteVertexArrays(1, &oc->vertex_array);
		GLES3::Utilities::buffer_free_data(oc->vertex_buffer);
		GLES3::Utilities::buffer_free_data(oc->index_buffer);

		oc->vertex_array = 0;
		oc->vertex_buffer = 0;
		oc->index_buffer = 0;
	}

	if (lines.size()) {
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

		if (oc->vertex_array == 0) {
			oc->line_point_count = lc;
			glGenVertexArrays(1, &oc->vertex_array);
			glBindVertexArray(oc->vertex_array);
			glGenBuffers(1, &oc->vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, oc->vertex_buffer);

			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, oc->vertex_buffer,
				lc * 6 * sizeof(float), geometry.ptr(), GL_STATIC_DRAW,
				"Occluder polygon vertex buffer");

			glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
			glVertexAttribPointer(
				RSE::ARRAY_VERTEX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

			glGenBuffers(1, &oc->index_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oc->index_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER, oc->index_buffer,
				3 * lc * sizeof(uint16_t), indices.ptr(), GL_STATIC_DRAW,
				"Occluder polygon index buffer");

			glBindVertexArray(0);
		}
		else {
			glBindBuffer(GL_ARRAY_BUFFER, oc->vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, lc * 6 * sizeof(float), geometry.ptr(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oc->index_buffer);
			glBufferData(
				GL_ELEMENT_ARRAY_BUFFER, 3 * lc * sizeof(uint16_t), indices.ptr(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

	if (oc->sdf_index_count != sdf_indices.size() && oc->sdf_point_count != p_points.size() &&
		oc->sdf_vertex_array != 0) {
		glDeleteVertexArrays(1, &oc->sdf_vertex_array);
		GLES3::Utilities::buffer_free_data(oc->sdf_vertex_buffer);
		GLES3::Utilities::buffer_free_data(oc->sdf_index_buffer);

		oc->sdf_vertex_array = 0;
		oc->sdf_vertex_buffer = 0;
		oc->sdf_index_buffer = 0;

		oc->sdf_index_count = sdf_indices.size();
		oc->sdf_point_count = p_points.size();
	}

	if (sdf_indices.size()) {
		if (oc->sdf_vertex_array == 0) {
			oc->sdf_index_count = sdf_indices.size();
			oc->sdf_point_count = p_points.size();
			glGenVertexArrays(1, &oc->sdf_vertex_array);
			glBindVertexArray(oc->sdf_vertex_array);
			glGenBuffers(1, &oc->sdf_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, oc->sdf_vertex_buffer);

			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, oc->sdf_vertex_buffer,
				oc->sdf_point_count * 2 * sizeof(float), p_points.ptr(), GL_STATIC_DRAW,
				"Occluder polygon SDF vertex buffer");

			glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
			glVertexAttribPointer(
				RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

			glGenBuffers(1, &oc->sdf_index_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oc->sdf_index_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER, oc->sdf_index_buffer,
				oc->sdf_index_count * sizeof(uint32_t), sdf_indices.ptr(), GL_STATIC_DRAW,
				"Occluder polygon SDF index buffer");

			glBindVertexArray(0);
		}
		else {
			glBindBuffer(GL_ARRAY_BUFFER, oc->sdf_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, p_points.size() * 2 * sizeof(float), p_points.ptr(),
				GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oc->sdf_index_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sdf_indices.size() * sizeof(uint32_t),
				sdf_indices.ptr(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
}

void RasterizerCanvasGLES3::occluder_polygon_set_cull_mode(
	RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode)
{
	OccluderPolygon* oc = occluder_polygon_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(oc);
	oc->cull_mode = p_mode;
}

void RasterizerCanvasGLES3::set_shadow_texture_size(int p_size)
{
	p_size = Math::nearest_power_of_2_templated(p_size);

	if (p_size > GLES3::Config::max_texture_size) {
		p_size = GLES3::Config::max_texture_size;
		WARN_PRINT("Attempting to set RCR::CanvasItem shadow atlas size to " + itos(p_size) +
				   " which is beyond limit of " + itos(GLES3::Config::max_texture_size) +
				   "supported by hardware.");
	}

	if (p_size == state.shadow_texture_size) {
		return;
	}
	state.shadow_texture_size = p_size;

	if (state.shadow_fb != 0) {
		glDeleteFramebuffers(1, &state.shadow_fb);
		GLES3::Utilities::texture_free_data(state.shadow_texture);
		glDeleteRenderbuffers(1, &state.shadow_depth_buffer);
		state.shadow_fb = 0;
		state.shadow_texture = 0;
		state.shadow_depth_buffer = 0;
	}
	_update_shadow_atlas();
}

bool RasterizerCanvasGLES3::free(RID p_rid)
{
	if (canvas_light_owner.owns(p_rid)) {
		CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(cl, false);
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

void RasterizerCanvasGLES3::update() {}

void RasterizerCanvasGLES3::canvas_begin(
	RID p_to_render_target, bool p_to_backbuffer, bool p_backbuffer_has_mipmaps)
{
	GLES3::RenderTarget* render_target = GLES3::TextureStorage::get_render_target(p_to_render_target);

	if (p_to_backbuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, render_target->backbuffer_fbo);
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 4);
		GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
			GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
		glBindTexture(GL_TEXTURE_2D, tex->tex_id);
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, render_target->fbo);
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 4);
		glBindTexture(GL_TEXTURE_2D, render_target->backbuffer);
		if (render_target->backbuffer != 0) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,
				p_backbuffer_has_mipmaps ? render_target->mipmap_count - 1 : 0);
		}
	}

	if (render_target->is_transparent || p_to_backbuffer) {
		state.transparent_render_target = true;
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		state.transparent_render_target = false;
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
	}

	if (render_target && render_target->clear_requested) {
		const Color& col = render_target->clear_color;
		glClearColor(col.r, col.g, col.b, render_target->is_transparent ? col.a : 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		render_target->clear_requested = false;
	}

	glActiveTexture(GL_TEXTURE0);
	GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
		GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
	glBindTexture(GL_TEXTURE_2D, tex->tex_id);
}

void RasterizerCanvasGLES3::_bind_canvas_texture(RID p_texture,
	RSE::CanvasItemTextureFilter p_base_filter, RSE::CanvasItemTextureRepeat p_base_repeat)
{
	if (p_texture == RID()) {
		p_texture = default_canvas_texture;
	}

	if (state.current_tex == p_texture && state.current_filter_mode == p_base_filter &&
		state.current_repeat_mode == p_base_repeat) {
		return;
	}

	state.current_tex = p_texture;
	state.current_filter_mode = p_base_filter;
	state.current_repeat_mode = p_base_repeat;

	GLES3::CanvasTexture* ct = nullptr;

	GLES3::Texture* t = GLES3::TextureStorage::get_texture(p_texture);

	if (t) {
		ERR_FAIL_NULL(t->canvas_texture);
		ct = t->canvas_texture;
		if (t->render_target) {
			t->render_target->used_in_frame = true;
		}
	}
	else {
		ct = GLES3::TextureStorage::get_canvas_texture(p_texture);
	}

	if (!ct) {
		// Invalid Texture RID.
		_bind_canvas_texture(default_canvas_texture, p_base_filter, p_base_repeat);
		return;
	}

	RSE::CanvasItemTextureFilter filter =
		ct->texture_filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? ct->texture_filter
																	  : p_base_filter;
	ERR_FAIL_COND(filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT);

	RSE::CanvasItemTextureRepeat repeat =
		ct->texture_repeat != RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? ct->texture_repeat
																	  : p_base_repeat;
	ERR_FAIL_COND(repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT);

	GLES3::Texture* texture = GLES3::TextureStorage::get_texture(ct->diffuse);

	if (!texture) {
		GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
			GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex->tex_id);
	}
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->tex_id);
		texture->gl_set_filter(filter);
		texture->gl_set_repeat(repeat);
		if (texture->render_target) {
			texture->render_target->used_in_frame = true;
		}
	}

	GLES3::Texture* normal_map = GLES3::TextureStorage::get_texture(ct->normal_map);

	if (!normal_map) {
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 6);
		GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
			GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_NORMAL));
		glBindTexture(GL_TEXTURE_2D, tex->tex_id);
	}
	else {
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 6);
		glBindTexture(GL_TEXTURE_2D, normal_map->tex_id);
		normal_map->gl_set_filter(filter);
		normal_map->gl_set_repeat(repeat);
		if (normal_map->render_target) {
			normal_map->render_target->used_in_frame = true;
		}
	}

	GLES3::Texture* specular_map = GLES3::TextureStorage::get_texture(ct->specular);

	if (!specular_map) {
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 7);
		GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
			GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE));
		glBindTexture(GL_TEXTURE_2D, tex->tex_id);
	}
	else {
		glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 7);
		glBindTexture(GL_TEXTURE_2D, specular_map->tex_id);
		specular_map->gl_set_filter(filter);
		specular_map->gl_set_repeat(repeat);
		if (specular_map->render_target) {
			specular_map->render_target->used_in_frame = true;
		}
	}
}

void RasterizerCanvasGLES3::_prepare_canvas_texture(RID p_texture,
	RSE::CanvasItemTextureFilter p_base_filter, RSE::CanvasItemTextureRepeat p_base_repeat,
	uint32_t& r_index, Size2& r_texpixel_size)
{
	if (p_texture == RID()) {
		p_texture = default_canvas_texture;
	}

	GLES3::CanvasTexture* ct = nullptr;

	GLES3::Texture* t = GLES3::TextureStorage::get_texture(p_texture);

	if (t) {
		// regular texture
		if (!t->canvas_texture) {
			t->canvas_texture = memnew(GLES3::CanvasTexture);
			t->canvas_texture->diffuse = p_texture;
		}

		ct = t->canvas_texture;
	}
	else {
		ct = GLES3::TextureStorage::get_canvas_texture(p_texture);
	}

	if (!ct) {
		// Invalid Texture RID.
		_prepare_canvas_texture(
			default_canvas_texture, p_base_filter, p_base_repeat, r_index, r_texpixel_size);
		return;
	}

	GLES3::Texture* texture = GLES3::TextureStorage::get_texture(ct->diffuse);
	Size2i size_cache;

	// Cache default white resource ID.
	const RID default_texture_id =
		GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_WHITE);

	// If no texture is assigned, assign default white.
	if (!texture) {
		ct->diffuse = default_texture_id;
	}

	// Enforce a 1x1 size if default white texture.
	size_cache =
		ct->diffuse == default_texture_id ? Size2i(1, 1) : Size2i(texture->width, texture->height);

	GLES3::Texture* normal_map = GLES3::TextureStorage::get_texture(ct->normal_map);

	if (ct->specular_color.a < 0.999) {
		state.canvas_instance_batches[state.current_batch_index].flags |=
			BATCH_FLAGS_DEFAULT_SPECULAR_MAP_USED;
	}
	else {
		state.canvas_instance_batches[state.current_batch_index].flags &=
			~BATCH_FLAGS_DEFAULT_SPECULAR_MAP_USED;
	}

	if (normal_map) {
		state.canvas_instance_batches[state.current_batch_index].flags |=
			BATCH_FLAGS_DEFAULT_NORMAL_MAP_USED;
	}
	else {
		state.canvas_instance_batches[state.current_batch_index].flags &=
			~BATCH_FLAGS_DEFAULT_NORMAL_MAP_USED;
	}

	state.canvas_instance_batches[state.current_batch_index].specular_shininess =
		uint32_t(CLAMP(ct->specular_color.a * 255.0, 0, 255)) << 24;
	state.canvas_instance_batches[state.current_batch_index].specular_shininess |=
		uint32_t(CLAMP(ct->specular_color.b * 255.0, 0, 255)) << 16;
	state.canvas_instance_batches[state.current_batch_index].specular_shininess |=
		uint32_t(CLAMP(ct->specular_color.g * 255.0, 0, 255)) << 8;
	state.canvas_instance_batches[state.current_batch_index].specular_shininess |=
		uint32_t(CLAMP(ct->specular_color.r * 255.0, 0, 255));

	r_texpixel_size.x = 1.0 / float(size_cache.x);
	r_texpixel_size.y = 1.0 / float(size_cache.y);

	state.instance_data_array[r_index].color_texture_pixel_size[0] = r_texpixel_size.x;
	state.instance_data_array[r_index].color_texture_pixel_size[1] = r_texpixel_size.y;
}

void RasterizerCanvasGLES3::reset_canvas()
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

	glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0 + GLES3::Config::max_texture_image_units - 3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::draw_lens_distortion_rect(
	const Rect2& p_rect, float p_k1, float p_k2, const Vector2& p_eye_center, float p_oversample)
{
}

PolygonID RasterizerCanvasGLES3::request_polygon(const Vector<int>& p_indices,
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones, const Vector<float>& p_weights, int p_count)
{
	// We interleave the vertex data into one big VBO to improve cache coherence
	uint32_t vertex_count = p_points.size();
	uint32_t stride = 2;
	if ((uint32_t)p_colors.size() == vertex_count) {
		stride += 4;
	}
	if ((uint32_t)p_uvs.size() == vertex_count) {
		stride += 2;
	}
	if ((uint32_t)p_bones.size() == vertex_count * 4 &&
		(uint32_t)p_weights.size() == vertex_count * 4) {
		stride += 4;
	}

	GLES3::PolygonBuffers pb;
	glGenBuffers(1, &pb.vertex_buffer);
	glGenVertexArrays(1, &pb.vertex_array);
	glBindVertexArray(pb.vertex_array);
	pb.count = vertex_count;
	pb.index_buffer = 0;

	uint32_t buffer_size = stride * p_points.size();

	Vector<uint8_t> polygon_buffer;
	polygon_buffer.resize(buffer_size * sizeof(float));
	{
		glBindBuffer(GL_ARRAY_BUFFER, pb.vertex_buffer);
		uint8_t* r = polygon_buffer.ptrw();
		float* fptr = reinterpret_cast<float*>(r);
		uint32_t* uptr = (uint32_t*)r;
		uint32_t base_offset = 0;
		{
			// Always uses vertex positions
			glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
			glVertexAttribPointer(
				RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), nullptr);
			const Vector2* points_ptr = p_points.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				fptr[base_offset + i * stride + 0] = points_ptr[i].x;
				fptr[base_offset + i * stride + 1] = points_ptr[i].y;
			}

			base_offset += 2;
		}

		// Next add colors
		if ((uint32_t)p_colors.size() == vertex_count) {
			glEnableVertexAttribArray(RSE::ARRAY_COLOR);
			glVertexAttribPointer(RSE::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, stride * sizeof(float),
				CAST_INT_TO_UCHAR_PTR(base_offset * sizeof(float)));

			const Color* color_ptr = p_colors.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				fptr[base_offset + i * stride + 0] = color_ptr[i].r;
				fptr[base_offset + i * stride + 1] = color_ptr[i].g;
				fptr[base_offset + i * stride + 2] = color_ptr[i].b;
				fptr[base_offset + i * stride + 3] = color_ptr[i].a;
			}
			base_offset += 4;
		}
		else {
			glDisableVertexAttribArray(RSE::ARRAY_COLOR);
			pb.color_disabled = true;
			pb.color = p_colors.size() == 1 ? p_colors[0] : Color(1.0, 1.0, 1.0, 1.0);
		}

		if ((uint32_t)p_uvs.size() == vertex_count) {
			glEnableVertexAttribArray(RSE::ARRAY_TEX_UV);
			glVertexAttribPointer(RSE::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float),
				CAST_INT_TO_UCHAR_PTR(base_offset * sizeof(float)));

			const Vector2* uv_ptr = p_uvs.ptr();

			for (uint32_t i = 0; i < vertex_count; i++) {
				fptr[base_offset + i * stride + 0] = uv_ptr[i].x;
				fptr[base_offset + i * stride + 1] = uv_ptr[i].y;
			}

			base_offset += 2;
		}
		else {
			glDisableVertexAttribArray(RSE::ARRAY_TEX_UV);
		}

		if ((uint32_t)p_indices.size() == vertex_count * 4 &&
			(uint32_t)p_weights.size() == vertex_count * 4) {
			glEnableVertexAttribArray(RSE::ARRAY_BONES);
			glVertexAttribPointer(RSE::ARRAY_BONES, 4, GL_UNSIGNED_INT, GL_FALSE,
				stride * sizeof(float), CAST_INT_TO_UCHAR_PTR(base_offset * sizeof(float)));

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
			glDisableVertexAttribArray(RSE::ARRAY_BONES);
		}

		if ((uint32_t)p_weights.size() == vertex_count * 4) {
			glEnableVertexAttribArray(RSE::ARRAY_WEIGHTS);
			glVertexAttribPointer(RSE::ARRAY_WEIGHTS, 4, GL_FLOAT, GL_FALSE, stride * sizeof(float),
				CAST_INT_TO_UCHAR_PTR(base_offset * sizeof(float)));

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
			glDisableVertexAttribArray(RSE::ARRAY_WEIGHTS);
		}

		ERR_FAIL_COND_V(base_offset != stride, 0);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, pb.vertex_buffer,
			vertex_count * stride * sizeof(float), polygon_buffer.ptr(), GL_STATIC_DRAW,
			"Polygon 2D vertex buffer");
	}

	if (p_indices.size()) {
		// create indices, as indices were requested
		Vector<uint8_t> index_buffer;
		index_buffer.resize(p_count * sizeof(int32_t));
		{
			uint8_t* w = index_buffer.ptrw();
			memcpy(w, p_indices.ptr(), sizeof(int32_t) * p_count);
		}
		glGenBuffers(1, &pb.index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pb.index_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER, pb.index_buffer,
			p_count * 4, index_buffer.ptr(), GL_STATIC_DRAW, "Polygon 2D index buffer");
		pb.count = p_count;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	PolygonID id = polygon_buffers.last_id++;

	polygon_buffers.polygons[id] = pb;

	return id;
}

void RasterizerCanvasGLES3::free_polygon(PolygonID p_polygon)
{
	GLES3::PolygonBuffers* pb_ptr = polygon_buffers.polygons.getptr(p_polygon);
	ERR_FAIL_NULL(pb_ptr);

	GLES3::PolygonBuffers& pb = *pb_ptr;

	if (pb.index_buffer != 0) {
		GLES3::Utilities::buffer_free_data(pb.index_buffer);
	}

	glDeleteVertexArrays(1, &pb.vertex_array);
	GLES3::Utilities::buffer_free_data(pb.vertex_buffer);

	polygon_buffers.polygons.erase(p_polygon);
}

// Creates a new uniform buffer and uses it right away
// This expands the instance buffer continually
// In theory allocations can reach as high as number of windows * 3 frames
// because OpenGL can start rendering subsequent frames before finishing the current one
void RasterizerCanvasGLES3::_allocate_instance_data_buffer()
{
	GLuint new_buffers[3];
	glGenBuffers(3, new_buffers);
	// Batch UBO.
	glBindBuffer(GL_ARRAY_BUFFER, new_buffers[0]);
	GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, new_buffers[0],
		data.max_instance_buffer_size, nullptr, GL_STREAM_DRAW,
		"2D Batch UBO[" + itos(state.current_data_buffer_index) + "][0]");
	// Light uniform buffer.
	glBindBuffer(GL_UNIFORM_BUFFER, new_buffers[1]);
	GLES3::Utilities::buffer_allocate_data(GL_UNIFORM_BUFFER, new_buffers[1],
		sizeof(GLES3::LightUniform) * data.max_lights_per_render, nullptr, GL_STREAM_DRAW,
		"2D Lights UBO[" + itos(state.current_data_buffer_index) + "]");
	// State buffer.
	glBindBuffer(GL_UNIFORM_BUFFER, new_buffers[2]);
	GLES3::Utilities::buffer_allocate_data(GL_UNIFORM_BUFFER, new_buffers[2], sizeof(StateBuffer),
		nullptr, GL_STREAM_DRAW, "2D State UBO[" + itos(state.current_data_buffer_index) + "]");

	state.current_data_buffer_index = (state.current_data_buffer_index + 1);
	GLES3::DataBuffer db;
	db.instance_buffers.push_back(new_buffers[0]);
	db.light_ubo = new_buffers[1];
	db.state_ubo = new_buffers[2];
	db.last_frame_used = RendererCompositor::get_frame_number();
	state.canvas_instance_data_buffers.insert(state.current_data_buffer_index, db);
	state.current_data_buffer_index =
		state.current_data_buffer_index % state.canvas_instance_data_buffers.size();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RasterizerCanvasGLES3::_allocate_instance_buffer()
{
	state.current_instance_buffer_index++;

	if (int(state.current_instance_buffer_index) <
		state.canvas_instance_data_buffers[state.current_data_buffer_index]
			.instance_buffers.size()) {
		// We already allocated another buffer in a previous frame, so we can just use it.
		return;
	}

	GLuint new_buffer;
	glGenBuffers(1, &new_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, new_buffer);
	GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, new_buffer,
		data.max_instance_buffer_size, nullptr, GL_STREAM_DRAW,
		"Batch UBO[" + itos(state.current_data_buffer_index) + "][" +
			itos(state.canvas_instance_data_buffers[state.current_data_buffer_index]
					 .instance_buffers.size()) +
			"]");

	state.canvas_instance_data_buffers[state.current_data_buffer_index].instance_buffers.push_back(
		new_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::set_time(double p_time) { state.time = p_time; }

void RasterizerCanvasGLES3::initialize()
{
	glVertexAttrib4f(RSE::ARRAY_COLOR, 1.0, 1.0, 1.0, 1.0);

	polygon_buffers.last_id = 1;
	// quad buffer
	{
		glGenBuffers(1, &data.canvas_quad_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, data.canvas_quad_vertices);

		const float qv[8] = {0, 0, 0, 1, 1, 1, 1, 0};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, qv, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &data.canvas_quad_array);
		glBindVertexArray(data.canvas_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data.canvas_quad_vertices);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}

	{
		// particle quad buffers

		glGenBuffers(1, &data.particle_quad_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, data.particle_quad_vertices);
		{
			// quad of size 1, with pivot on the center for particles, then regular UVS. Color is
			// general plus fetched from particle
			const float qv[16] = {
				-0.5, -0.5, 0.0, 0.0, -0.5, 0.5, 0.0, 1.0, 0.5, 0.5, 1.0, 1.0, 0.5, -0.5, 1.0, 0.0};

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, qv, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data.particle_quad_array);
		glBindVertexArray(data.particle_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data.particle_quad_vertices);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_TEX_UV);
		glVertexAttribPointer(
			RSE::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(8));
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}

	// ninepatch buffers
	{
		// array buffer
		glGenBuffers(1, &data.ninepatch_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, data.ninepatch_vertices);

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (16 + 16) * 2, nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// element buffer
		glGenBuffers(1, &data.ninepatch_elements);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ninepatch_elements);

#define _EIDX(y, x) (y * 4 + x)
		uint8_t elems[3 * 2 * 9] = {// first row

			_EIDX(0, 0), _EIDX(0, 1), _EIDX(1, 1), _EIDX(1, 1), _EIDX(1, 0), _EIDX(0, 0),

			_EIDX(0, 1), _EIDX(0, 2), _EIDX(1, 2), _EIDX(1, 2), _EIDX(1, 1), _EIDX(0, 1),

			_EIDX(0, 2), _EIDX(0, 3), _EIDX(1, 3), _EIDX(1, 3), _EIDX(1, 2), _EIDX(0, 2),

			// second row

			_EIDX(1, 0), _EIDX(1, 1), _EIDX(2, 1), _EIDX(2, 1), _EIDX(2, 0), _EIDX(1, 0),

			// the center one would be here, but we'll put it at the end
			// so it's easier to disable the center and be able to use
			// one draw call for both

			_EIDX(1, 2), _EIDX(1, 3), _EIDX(2, 3), _EIDX(2, 3), _EIDX(2, 2), _EIDX(1, 2),

			// third row

			_EIDX(2, 0), _EIDX(2, 1), _EIDX(3, 1), _EIDX(3, 1), _EIDX(3, 0), _EIDX(2, 0),

			_EIDX(2, 1), _EIDX(2, 2), _EIDX(3, 2), _EIDX(3, 2), _EIDX(3, 1), _EIDX(2, 1),

			_EIDX(2, 2), _EIDX(2, 3), _EIDX(3, 3), _EIDX(3, 3), _EIDX(3, 2), _EIDX(2, 2),

			// center field

			_EIDX(1, 1), _EIDX(1, 2), _EIDX(2, 2), _EIDX(2, 2), _EIDX(2, 1), _EIDX(1, 1)};
#undef _EIDX

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elems), elems, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	if (GLES3::Config::max_uniform_buffer_size < 65536) {
		data.max_lights_per_render = 64;
	}
	else {
		data.max_lights_per_render = 256;
	}

	// Reserve 3 Uniform Buffers for instance data Frame N, N+1 and N+2
	data.max_instance_buffer_size =
		data.max_instances_per_buffer *
		sizeof(GLES3::InstanceData); // 16,384 instances * 128 bytes = 2,097,152 bytes = 2,048 kb
	state.canvas_instance_data_buffers.resize(3);
	state.canvas_instance_batches.reserve(200);

	for (int i = 0; i < 3; i++) {
		GLuint new_buffers[3];
		glGenBuffers(3, new_buffers);
		// Batch UBO.
		glBindBuffer(GL_ARRAY_BUFFER, new_buffers[0]);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, new_buffers[0],
			data.max_instance_buffer_size, nullptr, GL_STREAM_DRAW, "Batch UBO[0][0]");
		// Light uniform buffer.
		glBindBuffer(GL_UNIFORM_BUFFER, new_buffers[1]);
		GLES3::Utilities::buffer_allocate_data(GL_UNIFORM_BUFFER, new_buffers[1],
			sizeof(GLES3::LightUniform) * data.max_lights_per_render, nullptr, GL_STREAM_DRAW,
			"2D lights UBO[0]");
		// State buffer.
		glBindBuffer(GL_UNIFORM_BUFFER, new_buffers[2]);
		GLES3::Utilities::buffer_allocate_data(GL_UNIFORM_BUFFER, new_buffers[2],
			sizeof(StateBuffer), nullptr, GL_STREAM_DRAW, "2D state UBO[0]");
		GLES3::DataBuffer db;
		db.instance_buffers.push_back(new_buffers[0]);
		db.light_ubo = new_buffers[1];
		db.state_ubo = new_buffers[2];
		db.last_frame_used = 0;
		db.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		state.canvas_instance_data_buffers[i] = db;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	state.instance_data_array = memnew_arr(GLES3::InstanceData, data.max_instances_per_buffer);
	state.light_uniforms = memnew_arr(GLES3::LightUniform, data.max_lights_per_render);

	{
		const uint32_t indices[6] = {0, 2, 1, 3, 2, 0};
		glGenVertexArrays(1, &data.indexed_quad_array);
		glBindVertexArray(data.indexed_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data.canvas_quad_vertices);
		glGenBuffers(1, &data.indexed_quad_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.indexed_quad_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6, indices, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	String global_defines;
	global_defines += "#define MAX_GLOBAL_SHADER_UNIFORMS 256\n"; // TODO: this is arbitrary for now
	global_defines += "#define MAX_LIGHTS " + itos(data.max_lights_per_render) + "\n";

	GLES3::MaterialStorage::shaders->canvas_shader.initialize(global_defines, 1);
	data.canvas_shader_default_version =
		GLES3::MaterialStorage::shaders->canvas_shader.version_create();

	shadow_render.shader.initialize();
	shadow_render.shader_version = shadow_render.shader.version_create();

	{
		default_canvas_group_shader = GLES3::MaterialStorage::shader_allocate();
		GLES3::MaterialStorage::shader_initialize(default_canvas_group_shader);

		GLES3::MaterialStorage::shader_set_code(default_canvas_group_shader, R"(
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
		default_canvas_group_material = GLES3::MaterialStorage::material_allocate();
		GLES3::MaterialStorage::material_initialize(default_canvas_group_material);

		GLES3::MaterialStorage::material_set_shader(
			default_canvas_group_material, default_canvas_group_shader);
	}

	{
		default_clip_children_shader = GLES3::MaterialStorage::shader_allocate();
		GLES3::MaterialStorage::shader_initialize(default_clip_children_shader);

		GLES3::MaterialStorage::shader_set_code(default_clip_children_shader, R"(
// Default clip children shader.

shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_nearest;

void fragment() {
	vec4 c = textureLod(screen_texture, SCREEN_UV, 0.0);
	COLOR.rgb = c.rgb;
}
)");
		default_clip_children_material = GLES3::MaterialStorage::material_allocate();
		GLES3::MaterialStorage::material_initialize(default_clip_children_material);

		GLES3::MaterialStorage::material_set_shader(
			default_clip_children_material, default_clip_children_shader);
	}

	default_canvas_texture = GLES3::TextureStorage::canvas_texture_allocate();
	GLES3::TextureStorage::canvas_texture_initialize(default_canvas_texture);

	state.time = 0.0;
}

void RasterizerCanvasGLES3::finalize()
{
	GLES3::MaterialStorage::shaders->canvas_shader.version_free(data.canvas_shader_default_version);
	shadow_render.shader.version_free(shadow_render.shader_version);
	GLES3::MaterialStorage::material_free(default_canvas_group_material);
	GLES3::MaterialStorage::shader_free(default_canvas_group_shader);
	GLES3::MaterialStorage::material_free(default_clip_children_material);
	GLES3::MaterialStorage::shader_free(default_clip_children_shader);

	glDeleteBuffers(1, &data.canvas_quad_vertices);
	glDeleteVertexArrays(1, &data.canvas_quad_array);

	glDeleteBuffers(1, &data.particle_quad_vertices);
	glDeleteVertexArrays(1, &data.particle_quad_array);

	glDeleteBuffers(1, &data.ninepatch_vertices);
	glDeleteBuffers(1, &data.ninepatch_elements);

	glDeleteBuffers(1, &data.indexed_quad_buffer);
	glDeleteVertexArrays(1, &data.indexed_quad_array);

	GLES3::TextureStorage::canvas_texture_free(default_canvas_texture);
	memdelete_arr(state.instance_data_array);
	memdelete_arr(state.light_uniforms);

	if (state.shadow_fb != 0) {
		glDeleteFramebuffers(1, &state.shadow_fb);
		GLES3::Utilities::texture_free_data(state.shadow_texture);
		glDeleteRenderbuffers(1, &state.shadow_depth_buffer);
		state.shadow_fb = 0;
		state.shadow_texture = 0;
		state.shadow_depth_buffer = 0;
	}

	for (uint32_t i = 0; i < state.canvas_instance_data_buffers.size(); i++) {
		if (state.canvas_instance_data_buffers[i].fence != GLsync()) {
			glDeleteSync(state.canvas_instance_data_buffers[i].fence);
		}
		for (int j = 0; j < state.canvas_instance_data_buffers[i].instance_buffers.size(); j++) {
			if (state.canvas_instance_data_buffers[i].instance_buffers[j]) {
				GLES3::Utilities::buffer_free_data(
					state.canvas_instance_data_buffers[i].instance_buffers[j]);
			}
		}
		if (state.canvas_instance_data_buffers[i].light_ubo) {
			GLES3::Utilities::buffer_free_data(state.canvas_instance_data_buffers[i].light_ubo);
		}
		if (state.canvas_instance_data_buffers[i].state_ubo) {
			GLES3::Utilities::buffer_free_data(state.canvas_instance_data_buffers[i].state_ubo);
		}
	}
}

#endif // GLES3_ENABLED



// ============================================================================
// Merged from drivers/gles3/shader_gles3.cpp
// ============================================================================

#include "shader_gles3.h"

#ifdef GLES3_ENABLED

#include "core/io/dir_access.h"
#include "core/string/ustring.h"
#include "drivers/gles3/storage/utilities.h"

#ifndef WEB_ENABLED
#include "core/io/file_access.h"
#endif

static String _mkid(const String& p_id)
{
	String id = "m_" + p_id.replace("__", "_dus_");
	return id.replace("__", "_dus_"); // doubleunderscore is reserved in glsl
}

void ShaderGLES3::_add_stage(const char* p_code, StageType p_stage_type)
{
	Vector<String> lines = String::utf8(p_code).split("\n");

	String text;

	for (int i = 0; i < lines.size(); i++) {
		const String& l = lines[i];
		bool push_chunk = false;

		StageTemplate::Chunk chunk;

		if (l.begins_with("#GLOBALS")) {
			switch (p_stage_type) {
			case STAGE_TYPE_VERTEX:
				chunk.type = StageTemplate::Chunk::TYPE_VERTEX_GLOBALS;
				break;
			case STAGE_TYPE_FRAGMENT:
				chunk.type = StageTemplate::Chunk::TYPE_FRAGMENT_GLOBALS;
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
		else {
			text += l + "\n";
		}

		if (push_chunk) {
			if (text != String()) {
				StageTemplate::Chunk text_chunk;
				text_chunk.type = StageTemplate::Chunk::TYPE_TEXT;
				text_chunk.text = text.utf8();
				stage_templates[p_stage_type].chunks.push_back(text_chunk);
				text = String();
			}
			stage_templates[p_stage_type].chunks.push_back(chunk);
		}

		if (text != String()) {
			StageTemplate::Chunk text_chunk;
			text_chunk.type = StageTemplate::Chunk::TYPE_TEXT;
			text_chunk.text = text.utf8();
			stage_templates[p_stage_type].chunks.push_back(text_chunk);
			text = String();
		}
	}
}

void ShaderGLES3::_setup(const char* p_vertex_code, const char* p_fragment_code, const char* p_name,
	int p_uniform_count, const char** p_uniform_names, int p_ubo_count, const UBOPair* p_ubos,
	int p_feedback_count, const Feedback* p_feedback, int p_texture_count,
	const TexUnitPair* p_tex_units, int p_specialization_count,
	const Specialization* p_specializations, int p_variant_count, const char** p_variants)
{
	name = p_name;

	for (int i = 0; i < STAGE_TYPE_MAX; i++) {
	    stage_templates[i].chunks.clear();
	}

	if (p_vertex_code) {
		_add_stage(p_vertex_code, STAGE_TYPE_VERTEX);
	}
	if (p_fragment_code) {
		_add_stage(p_fragment_code, STAGE_TYPE_FRAGMENT);
	}

	uniform_names = p_uniform_names;
	uniform_count = p_uniform_count;
	ubo_pairs = p_ubos;
	ubo_count = p_ubo_count;
	texunit_pairs = p_tex_units;
	texunit_pair_count = p_texture_count;
	specializations = p_specializations;
	specialization_count = p_specialization_count;
	specialization_default_mask = 0;
	for (int i = 0; i < specialization_count; i++) {
		if (specializations[i].default_value) {
			specialization_default_mask |= (uint64_t(1) << uint64_t(i));
		}
	}
	variant_defines = p_variants;
	variant_count = p_variant_count;
	feedbacks = p_feedback;
	feedback_count = p_feedback_count;

	StringBuilder tohash;
	tohash.append("[Vertex]");
	tohash.append(p_vertex_code ? String::utf8(p_vertex_code) : "");
	tohash.append("[Fragment]");
	tohash.append(p_fragment_code ? String::utf8(p_fragment_code) : "");

	tohash.append("[gl_implementation]");
	const String& vendor = String::utf8((const char*)glGetString(GL_VENDOR));
	tohash.append(vendor.is_empty() ? "unknown" : vendor);
	const String& renderer = String::utf8((const char*)glGetString(GL_RENDERER));
	tohash.append(renderer.is_empty() ? "unknown" : renderer);
	const String& version = String::utf8((const char*)glGetString(GL_VERSION));
	tohash.append(version.is_empty() ? "unknown" : version);

	base_sha256 = tohash.as_string().sha256_text();
}

RID ShaderGLES3::version_create()
{
	// initialize() was never called
	ERR_FAIL_COND_V(variant_count == 0, RID());

	Version version;
	return version_owner.make_rid(version);
}

void ShaderGLES3::_build_variant_code(StringBuilder& builder, uint32_t p_variant,
	const Version* p_version, StageType p_stage_type, uint64_t p_specialization)
{
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		builder.append("#version 330\n");
		builder.append("#define USE_GLES_OVER_GL\n");
	}
	else {
		builder.append("#version 300 es\n");
	}

	if (GLES3::Config::polyfill_half2float) {
		builder.append("#define USE_HALF2FLOAT\n");
	}

	for (int i = 0; i < specialization_count; i++) {
		if (p_specialization & (uint64_t(1) << uint64_t(i))) {
			builder.append("#define " + String(specializations[i].name) + "\n");
		}
	}
	if (p_version->uniforms.size()) {
		builder.append("#define MATERIAL_UNIFORMS_USED\n");
	}
	for (const KeyValue<String, CharString>& E : p_version->code_sections) {
		builder.append(String("#define ") + String(E.key) + "_CODE_USED\n");
	}

	builder.append("\n"); // make sure defines begin at newline
	builder.append(general_defines.get_data());
	builder.append(variant_defines[p_variant]);
	builder.append("\n");
	for (int j = 0; j < p_version->custom_defines.size(); j++) {
		builder.append(p_version->custom_defines[j].get_data());
	}
	builder.append("\n"); // make sure defines begin at newline

	// Optional support for external textures.
	if (GLES3::Config::external_texture_supported) {
		builder.append("#extension GL_OES_EGL_image_external : enable\n");
		builder.append("#extension GL_OES_EGL_image_external_essl3 : enable\n");
	}
	else {
		builder.append("#define samplerExternalOES sampler2D\n");
	}

	// Insert multiview extension loading, because it needs to appear before
	// any non-preprocessor code (like the "precision highp..." lines below).
	builder.append("#ifdef USE_MULTIVIEW\n");
	builder.append("#if defined(GL_OVR_multiview2)\n");
	builder.append("#extension GL_OVR_multiview2 : require\n");
	builder.append("#elif defined(GL_OVR_multiview)\n");
	builder.append("#extension GL_OVR_multiview : require\n");
	builder.append("#endif\n");
	if (p_stage_type == StageType::STAGE_TYPE_VERTEX) {
		builder.append("layout(num_views=2) in;\n");
	}
	builder.append("#define ViewIndex gl_ViewID_OVR\n");
	builder.append("#define MAX_VIEWS 2\n");
	builder.append("#else\n");
	builder.append("#define ViewIndex uint(0)\n");
	builder.append("#define MAX_VIEWS 1\n");
	builder.append("#endif\n");

	// Default to highp precision unless specified otherwise.
	builder.append("precision highp float;\n");
	builder.append("precision highp int;\n");
	if (!RasterizerUtilGLES3::is_gles_over_gl()) {
		builder.append("precision highp sampler2D;\n");
		builder.append("precision highp samplerCube;\n");
		builder.append("precision highp sampler2DArray;\n");
		builder.append("precision highp sampler3D;\n");
	}

	const StageTemplate& stage_template = stage_templates[p_stage_type];
	for (uint32_t i = 0; i < stage_template.chunks.size(); i++) {
		const StageTemplate::Chunk& chunk = stage_template.chunks[i];
		switch (chunk.type) {
		case StageTemplate::Chunk::TYPE_MATERIAL_UNIFORMS: {
			builder.append(String::utf8(
				p_version->uniforms.get_data())); // uniforms (same for vertex and fragment)
		} break;
		case StageTemplate::Chunk::TYPE_VERTEX_GLOBALS: {
			builder.append(String::utf8(p_version->vertex_globals.get_data())); // vertex globals
		} break;
		case StageTemplate::Chunk::TYPE_FRAGMENT_GLOBALS: {
			builder.append(
				String::utf8(p_version->fragment_globals.get_data())); // fragment globals
		} break;
		case StageTemplate::Chunk::TYPE_CODE: {
			if (p_version->code_sections.has(chunk.code)) {
				builder.append(String::utf8(p_version->code_sections[chunk.code].get_data()));
			}
		} break;
		case StageTemplate::Chunk::TYPE_TEXT: {
			builder.append(String::utf8(chunk.text.get_data()));
		} break;
		}
	}
}

static void _display_error_with_code(const String& p_error, const String& p_code)
{
	int line = 1;
	Vector<String> lines = p_code.split("\n");

	for (int j = 0; j < lines.size(); j++) {
		__print_line(itos(line) + ": " + lines[j]);
		line++;
	}

	ERR_PRINT(p_error);
}

void ShaderGLES3::_get_uniform_locations(Version::Specialization& spec, Version* p_version)
{
	glUseProgram(spec.id);

	spec.uniform_location.resize(uniform_count);
	for (int i = 0; i < uniform_count; i++) {
		spec.uniform_location[i] = glGetUniformLocation(spec.id, uniform_names[i]);
	}

	for (int i = 0; i < texunit_pair_count; i++) {
		GLint loc = glGetUniformLocation(spec.id, texunit_pairs[i].name);
		if (loc >= 0) {
			if (texunit_pairs[i].index < 0) {
				glUniform1i(loc, max_image_units + texunit_pairs[i].index);
			}
			else {
				glUniform1i(loc, texunit_pairs[i].index);
			}
		}
	}

	for (int i = 0; i < ubo_count; i++) {
		GLint loc = glGetUniformBlockIndex(spec.id, ubo_pairs[i].name);
		if (loc >= 0) {
			glUniformBlockBinding(spec.id, loc, ubo_pairs[i].index);
		}
	}
	// textures
	int texture_index = 0;
	for (uint32_t i = 0; i < p_version->texture_uniforms.size(); i++) {
		String native_uniform_name = _mkid(p_version->texture_uniforms[i].name);
		GLint location = glGetUniformLocation(spec.id, (native_uniform_name).ascii().get_data());
		Vector<int32_t> texture_uniform_bindings;
		int texture_count = p_version->texture_uniforms[i].array_size;
		for (int j = 0; j < texture_count; j++) {
			texture_uniform_bindings.append(texture_index + base_texture_index);
			texture_index++;
		}
		glUniform1iv(location, texture_uniform_bindings.size(), texture_uniform_bindings.ptr());
	}

	glUseProgram(0);
}

void ShaderGLES3::_compile_specialization(Version::Specialization& spec, uint32_t p_variant,
	Version* p_version, uint64_t p_specialization)
{
	spec.id = glCreateProgram();
	spec.ok = false;
	GLint status;

	// vertex stage
	{
		StringBuilder builder;
		_build_variant_code(builder, p_variant, p_version, STAGE_TYPE_VERTEX, p_specialization);

		spec.vert_id = glCreateShader(GL_VERTEX_SHADER);
		String builder_string = builder.as_string();
		CharString cs = builder_string.utf8();
		const char* cstr = cs.ptr();
		GLint cstr_len = cs.length();
		glShaderSource(spec.vert_id, 1, &cstr, &cstr_len);
		glCompileShader(spec.vert_id);

		glGetShaderiv(spec.vert_id, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			GLsizei iloglen;
			glGetShaderiv(spec.vert_id, GL_INFO_LOG_LENGTH, &iloglen);

			if (iloglen < 0) {
				glDeleteShader(spec.vert_id);
				glDeleteProgram(spec.id);
				spec.id = 0;

				ERR_PRINT("No OpenGL vertex shader compiler log.");
			}
			else {
				if (iloglen == 0) {
					iloglen = 4096; // buggy driver (Adreno 220+)
				}

				char* ilogmem = (char*)Memory::alloc_static_zeroed(iloglen + 1);
				glGetShaderInfoLog(spec.vert_id, iloglen, &iloglen, ilogmem);

				String err_string = name + ": Vertex shader compilation failed:\n";

				err_string += ilogmem;

				_display_error_with_code(err_string, builder_string);

				Memory::free_static(ilogmem);
				glDeleteShader(spec.vert_id);
				glDeleteProgram(spec.id);
				spec.id = 0;
			}

			ERR_FAIL();
		}
	}

	// fragment stage
	{
		StringBuilder builder;
		_build_variant_code(builder, p_variant, p_version, STAGE_TYPE_FRAGMENT, p_specialization);

		spec.frag_id = glCreateShader(GL_FRAGMENT_SHADER);
		String builder_string = builder.as_string();
		CharString cs = builder_string.utf8();
		const char* cstr = cs.ptr();
		GLint cstr_len = cs.length();
		glShaderSource(spec.frag_id, 1, &cstr, &cstr_len);
		glCompileShader(spec.frag_id);

		glGetShaderiv(spec.frag_id, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			GLsizei iloglen;
			glGetShaderiv(spec.frag_id, GL_INFO_LOG_LENGTH, &iloglen);

			if (iloglen < 0) {
				glDeleteShader(spec.frag_id);
				glDeleteProgram(spec.id);
				spec.id = 0;

				ERR_PRINT("No OpenGL fragment shader compiler log.");
			}
			else {
				if (iloglen == 0) {
					iloglen = 4096; // buggy driver (Adreno 220+)
				}

				char* ilogmem = (char*)Memory::alloc_static_zeroed(iloglen + 1);
				glGetShaderInfoLog(spec.frag_id, iloglen, &iloglen, ilogmem);

				String err_string = name + ": Fragment shader compilation failed:\n";

				err_string += ilogmem;

				_display_error_with_code(err_string, builder_string);

				Memory::free_static(ilogmem);
				glDeleteShader(spec.frag_id);
				glDeleteProgram(spec.id);
				spec.id = 0;
			}

			ERR_FAIL();
		}
	}

	glAttachShader(spec.id, spec.frag_id);
	glAttachShader(spec.id, spec.vert_id);

	// If feedback exists, set it up.

	if (feedback_count) {
		Vector<const char*> feedback;
		for (int i = 0; i < feedback_count; i++) {
			if (feedbacks[i].specialization == 0 ||
				(feedbacks[i].specialization & p_specialization)) {
				// Specialization for this feedback is enabled
				feedback.push_back(feedbacks[i].name);
			}
		}

		if (feedback.size()) {
			glTransformFeedbackVaryings(
				spec.id, feedback.size(), feedback.ptr(), GL_INTERLEAVED_ATTRIBS);
		}
	}

	glLinkProgram(spec.id);

	glGetProgramiv(spec.id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLsizei iloglen;
		glGetProgramiv(spec.id, GL_INFO_LOG_LENGTH, &iloglen);

		if (iloglen < 0) {
			glDeleteShader(spec.frag_id);
			glDeleteShader(spec.vert_id);
			glDeleteProgram(spec.id);
			spec.id = 0;

			ERR_PRINT("No OpenGL program link log. Something is wrong.");
			ERR_FAIL();
		}

		if (iloglen == 0) {
			iloglen = 4096; // buggy driver (Adreno 220+)
		}

		char* ilogmem = (char*)Memory::alloc_static(iloglen + 1);
		ilogmem[iloglen] = '\0';
		glGetProgramInfoLog(spec.id, iloglen, &iloglen, ilogmem);

		String err_string = name + ": Program linking failed:\n";

		err_string += ilogmem;

		_display_error_with_code(err_string, String());

		Memory::free_static(ilogmem);
		glDeleteShader(spec.frag_id);
		glDeleteShader(spec.vert_id);
		glDeleteProgram(spec.id);
		spec.id = 0;

		ERR_FAIL();
	}

	_get_uniform_locations(spec, p_version);

	spec.ok = true;
}

RenderingServerTypes::ShaderNativeSourceCode ShaderGLES3::version_get_native_source_code(
	RID p_version)
{
	Version* version = version_owner.get_or_null(p_version);
	RenderingServerTypes::ShaderNativeSourceCode source_code;
	ERR_FAIL_NULL_V(version, source_code);

	source_code.versions.resize(variant_count);

	for (int i = 0; i < source_code.versions.size(); i++) {
		// vertex stage

		{
			StringBuilder builder;
			_build_variant_code(
				builder, i, version, STAGE_TYPE_VERTEX, specialization_default_mask);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "vertex";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}

		// fragment stage
		{
			StringBuilder builder;
			_build_variant_code(
				builder, i, version, STAGE_TYPE_FRAGMENT, specialization_default_mask);

			RenderingServerTypes::ShaderNativeSourceCode::Version::Stage stage;
			stage.name = "fragment";
			stage.code = builder.as_string();

			source_code.versions.write[i].stages.push_back(stage);
		}
	}

	return source_code;
}

String ShaderGLES3::_version_get_sha1(Version* p_version) const
{
	StringBuilder hash_build;

	hash_build.append("[uniforms]");
	hash_build.append(p_version->uniforms.get_data());
	hash_build.append("[vertex_globals]");
	hash_build.append(p_version->vertex_globals.get_data());
	hash_build.append("[fragment_globals]");
	hash_build.append(p_version->fragment_globals.get_data());

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
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		hash_build.append("[gl]");
	}
	else {
		hash_build.append("[gles]");
	}

	return hash_build.as_string().sha1_text();
}

#ifndef WEB_ENABLED // not supported in webgl
static const char* shader_file_header = "GLSC";
static const uint32_t cache_file_version = 3;
#endif

bool ShaderGLES3::_load_from_cache(Version* p_version)
{
#ifdef WEB_ENABLED // not supported in webgl
	return false;
#else
#if !defined(ANDROID_ENABLED) && !defined(IOS_ENABLED)
	if (RasterizerUtilGLES3::is_gles_over_gl() &&
		(glProgramBinary == nullptr)) { // ARB_get_program_binary extension not available.
		return false;
	}
#endif
	String sha1 = _version_get_sha1(p_version);
	String path =
		shader_cache_dir.path_join(name).path_join(base_sha256).path_join(sha1) + ".cache";

	Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
	if (f.is_null()) {
		return false;
	}

	char header[5] = {};
	f->get_buffer((uint8_t*)header, 4);
	ERR_FAIL_COND_V(header != String(shader_file_header), false);

	uint32_t file_version = f->get_32();
	if (file_version != cache_file_version) {
		return false; // wrong version
	}

	int cache_variant_count = static_cast<int>(f->get_32());
	ERR_FAIL_COND_V_MSG(cache_variant_count != variant_count, false,
		"shader cache variant count mismatch, expected " + itos(variant_count) + " got " +
			itos(cache_variant_count)); // should not happen but check

	LocalVector<AHashMap<uint64_t, Version::Specialization>> variants;
	for (int i = 0; i < cache_variant_count; i++) {
		uint32_t cache_specialization_count = f->get_32();
		AHashMap<uint64_t, Version::Specialization> variant;
		for (uint32_t j = 0; j < cache_specialization_count; j++) {
			uint64_t specialization_key = f->get_64();
			uint32_t variant_size = f->get_32();
			if (variant_size == 0) {
				continue;
			}
			uint32_t variant_format = f->get_32();
			Vector<uint8_t> variant_bytes;
			variant_bytes.resize(variant_size);

			uint32_t br = f->get_buffer(variant_bytes.ptrw(), variant_size);

			ERR_FAIL_COND_V(br != variant_size, false);

			Version::Specialization specialization;

			specialization.id = glCreateProgram();
			if (feedback_count) {
				Vector<const char*> feedback;
				for (int feedback_index = 0; feedback_index < feedback_count; feedback_index++) {
					if (feedbacks[feedback_index].specialization == 0 ||
						(feedbacks[feedback_index].specialization & specialization_key)) {
						// Specialization for this feedback is enabled.
						feedback.push_back(feedbacks[feedback_index].name);
					}
				}

				if (!feedback.is_empty()) {
					glTransformFeedbackVaryings(
						specialization.id, feedback.size(), feedback.ptr(), GL_INTERLEAVED_ATTRIBS);
				}
			}
			glProgramBinary(
				specialization.id, variant_format, variant_bytes.ptr(), variant_bytes.size());

			GLint link_status = 0;
			glGetProgramiv(specialization.id, GL_LINK_STATUS, &link_status);
			if (link_status != GL_TRUE) {
				WARN_PRINT_ONCE("Failed to load cached shader, recompiling.");
				return false;
			}

			_get_uniform_locations(specialization, p_version);

			specialization.ok = true;

			variant.insert(specialization_key, specialization);
		}
		variants.push_back(std::move(variant));
	}
	p_version->variants = variants;

	return true;
#endif // WEB_ENABLED
}

void ShaderGLES3::_save_to_cache(Version* p_version)
{
#ifdef WEB_ENABLED // not supported in webgl
	return;
#else
	ERR_FAIL_COND(!shader_cache_dir_valid);
#if !defined(ANDROID_ENABLED) && !defined(IOS_ENABLED)
	if (RasterizerUtilGLES3::is_gles_over_gl() &&
		(glGetProgramBinary == nullptr)) { // ARB_get_program_binary extension not available.
		return;
	}
#endif
	String sha1 = _version_get_sha1(p_version);
	String path =
		shader_cache_dir.path_join(name).path_join(base_sha256).path_join(sha1) + ".cache";

	Error error;
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE, &error);
	ERR_FAIL_COND(f.is_null());
	f->store_buffer((const uint8_t*)shader_file_header, 4);
	f->store_32(cache_file_version);
	f->store_32(variant_count);

	for (int i = 0; i < variant_count; i++) {
		int cache_specialization_count = p_version->variants[i].size();
		f->store_32(cache_specialization_count);

		for (KeyValue<uint64_t, ShaderGLES3::Version::Specialization>& kv :
			p_version->variants[i]) {
			const uint64_t specialization_key = kv.key;
			f->store_64(specialization_key);

			const Version::Specialization* specialization = &kv.value;
			GLint program_size = 0;
			glGetProgramiv(specialization->id, GL_PROGRAM_BINARY_LENGTH, &program_size);
			if (program_size == 0) {
				f->store_32(0);
				continue;
			}
			PackedByteArray compiled_program;
			compiled_program.resize(program_size);
			GLenum binary_format = 0;
			glGetProgramBinary(
				specialization->id, program_size, nullptr, &binary_format, compiled_program.ptrw());
			if (program_size != compiled_program.size()) {
				f->store_32(0);
				continue;
			}
			f->store_32(program_size);
			f->store_32(binary_format);
			f->store_buffer(compiled_program.ptr(), compiled_program.size());
		}
	}
#endif // WEB_ENABLED
}

void ShaderGLES3::_clear_version(Version* p_version)
{
	// Variants not compiled yet, just return
	if (p_version->variants.is_empty()) {
		return;
	}

	for (int i = 0; i < variant_count; i++) {
		for (KeyValue<uint64_t, Version::Specialization>& kv : p_version->variants[i]) {
			if (kv.value.id != 0) {
				glDeleteShader(kv.value.vert_id);
				glDeleteShader(kv.value.frag_id);
				glDeleteProgram(kv.value.id);
			}
		}
	}

	p_version->variants.clear();
}

void ShaderGLES3::_initialize_version(Version* p_version)
{
	ERR_FAIL_COND(p_version->variants.size() > 0);
	bool use_cache = shader_cache_dir_valid &&
					 !(feedback_count > 0 &&
						 GLES3::Config::disable_transform_feedback_shader_cache);
	if (use_cache && _load_from_cache(p_version)) {
		return;
	}
	p_version->variants.reserve(variant_count);
	for (int i = 0; i < variant_count; i++) {
		p_version->variants.push_back(AHashMap<uint64_t, Version::Specialization>());
		Version::Specialization spec;
		_compile_specialization(spec, i, p_version, specialization_default_mask);
		p_version->variants[i].insert(specialization_default_mask, spec);
	}
	if (use_cache) {
		_save_to_cache(p_version);
	}
}

void ShaderGLES3::version_set_code(RID p_version, const HashMap<String, String>& p_code,
	const String& p_uniforms, const String& p_vertex_globals, const String& p_fragment_globals,
	const Vector<String>& p_custom_defines,
	const LocalVector<ShaderGLES3::TextureUniformData>& p_texture_uniforms, bool p_initialize)
{
	Version* version = version_owner.get_or_null(p_version);
	ERR_FAIL_NULL(version);

	_clear_version(version); // clear if existing

	version->vertex_globals = p_vertex_globals.utf8();
	version->fragment_globals = p_fragment_globals.utf8();
	version->uniforms = p_uniforms.utf8();
	version->code_sections.clear();
	version->texture_uniforms = p_texture_uniforms;
	for (const KeyValue<String, String>& E : p_code) {
		version->code_sections[String(E.key.to_upper())] = E.value.utf8();
	}

	version->custom_defines.clear();
	for (int i = 0; i < p_custom_defines.size(); i++) {
		version->custom_defines.push_back(p_custom_defines[i].utf8());
	}

	if (p_initialize) {
		_initialize_version(version);
	}
}

bool ShaderGLES3::version_is_valid(RID p_version)
{
	Version* version = version_owner.get_or_null(p_version);
	return version != nullptr;
}

bool ShaderGLES3::version_free(RID p_version)
{
	if (version_owner.owns(p_version)) {
		Version* version = version_owner.get_or_null(p_version);
		_clear_version(version);
		version_owner.free(p_version);
	}
	else {
		return false;
	}

	return true;
}

bool ShaderGLES3::shader_cache_cleanup_on_start = false;

ShaderGLES3::ShaderGLES3() {}

void ShaderGLES3::initialize(const String& p_general_defines, int p_base_texture_index)
{
	general_defines = p_general_defines.utf8();
	base_texture_index = p_base_texture_index;

	if (shader_cache_dir != String()) {
		StringBuilder hash_build;

		hash_build.append("[base_hash]");
		hash_build.append(base_sha256);
		hash_build.append("[general_defines]");
		hash_build.append(general_defines.get_data());
		for (int i = 0; i < variant_count; i++) {
			hash_build.append("[variant_defines:" + itos(i) + "]");
			hash_build.append(variant_defines[i]);
		}

		base_sha256 = hash_build.as_string().sha256_text();

		Ref<DirAccess> d = DirAccess::open(shader_cache_dir);
		ERR_FAIL_COND(d.is_null());
		if (d->change_dir(name) != OK) {
			Error err = d->make_dir(name);
			ERR_FAIL_COND(err != OK);
			d->change_dir(name);
		}

		// erase other versions?
		if (shader_cache_cleanup_on_start) {
		}
		//
		if (d->change_dir(base_sha256) != OK) {
			Error err = d->make_dir(base_sha256);
			ERR_FAIL_COND(err != OK);
		}
		shader_cache_dir_valid = true;

		print_verbose("Shader '" + name + "' SHA256: " + base_sha256);
	}

	ERR_FAIL_COND(!GLES3::Config::is_initialized());
	max_image_units = GLES3::Config::max_texture_image_units;
}

void ShaderGLES3::set_shader_cache_dir(const String& p_dir) { shader_cache_dir = p_dir; }

void ShaderGLES3::set_shader_cache_save_compressed(bool p_enable)
{
	shader_cache_save_compressed = p_enable;
}

void ShaderGLES3::set_shader_cache_save_compressed_zstd(bool p_enable)
{
	shader_cache_save_compressed_zstd = p_enable;
}

void ShaderGLES3::set_shader_cache_save_debug(bool p_enable) { shader_cache_save_debug = p_enable; }

String ShaderGLES3::shader_cache_dir;
bool ShaderGLES3::shader_cache_save_compressed = true;
bool ShaderGLES3::shader_cache_save_compressed_zstd = true;
bool ShaderGLES3::shader_cache_save_debug = true;

ShaderGLES3::~ShaderGLES3()
{
	LocalVector<RID> remaining = version_owner.get_owned_list();
	if (remaining.size()) {
		ERR_PRINT(itos(remaining.size()) + " shaders of type " + name + " were never freed");
		for (RID& rid : remaining) {
			version_free(rid);
		}
	}
}
#endif



// ============================================================================
// Merged from drivers/gles3/rasterizer_gles3.cpp
// ============================================================================

#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "rasterizer_gles3.h"
#include "servers/rendering/storage/utilities.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/rendering_server_types.h"

#define _EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB 0x8242
#define _EXT_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB 0x8243
#define _EXT_DEBUG_CALLBACK_FUNCTION_ARB 0x8244
#define _EXT_DEBUG_CALLBACK_USER_PARAM_ARB 0x8245
#define _EXT_DEBUG_SOURCE_API_ARB 0x8246
#define _EXT_DEBUG_SOURCE_WINDOW_SYSTEM_ARB 0x8247
#define _EXT_DEBUG_SOURCE_SHADER_COMPILER_ARB 0x8248
#define _EXT_DEBUG_SOURCE_THIRD_PARTY_ARB 0x8249
#define _EXT_DEBUG_SOURCE_APPLICATION_ARB 0x824A
#define _EXT_DEBUG_SOURCE_OTHER_ARB 0x824B
#define _EXT_DEBUG_TYPE_ERROR_ARB 0x824C
#define _EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB 0x824D
#define _EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB 0x824E
#define _EXT_DEBUG_TYPE_PORTABILITY_ARB 0x824F
#define _EXT_DEBUG_TYPE_PERFORMANCE_ARB 0x8250
#define _EXT_DEBUG_TYPE_OTHER_ARB 0x8251
#define _EXT_DEBUG_TYPE_MARKER_ARB 0x8268
#define _EXT_MAX_DEBUG_MESSAGE_LENGTH_ARB 0x9143
#define _EXT_MAX_DEBUG_LOGGED_MESSAGES_ARB 0x9144
#define _EXT_DEBUG_LOGGED_MESSAGES_ARB 0x9145
#define _EXT_DEBUG_SEVERITY_HIGH_ARB 0x9146
#define _EXT_DEBUG_SEVERITY_MEDIUM_ARB 0x9147
#define _EXT_DEBUG_SEVERITY_LOW_ARB 0x9148
#define _EXT_DEBUG_OUTPUT 0x92E0

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GLAPIENTRY
#if defined(WINDOWS_ENABLED)
#define GLAPIENTRY APIENTRY
#else
#define GLAPIENTRY
#endif
#endif

#include <platform_gl.h>

#if defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
#include <platform_egl.h>
#endif

#if defined(GLAD_ENABLED) || defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
// We can debug GL if we use GLAD or EGL, so not on iOS or Web.
#define GL_DEBUG_CALLBACK

#ifdef _WIN32
#define strcpy strcpy_s
#endif
#endif // GLAD_ENABLED || EGL_ENABLED || ANDROID_ENABLED

#ifdef WINDOWS_ENABLED
bool RasterizerGLES3::screen_flipped_y = false;
#endif

void RasterizerGLES3::begin_frame(double frame_step)
{
	frame++;
	delta = frame_step;

	time_total += frame_step;

	RasterizerCanvasGLES3::set_time(time_total);
	scene->set_time(time_total, frame_step);

	GLES3::Utilities::_capture_timestamps_begin();

	// scene->iteration();
}

void RasterizerGLES3::bind_mesh_storage()
{
	RendererMeshStorage::mesh_allocate = &GLES3::MeshStorage::mesh_allocate;
	RendererMeshStorage::mesh_initialize = &GLES3::MeshStorage::mesh_initialize;
	RendererMeshStorage::mesh_set_blend_shape_count =
		&GLES3::MeshStorage::mesh_set_blend_shape_count;
	RendererMeshStorage::mesh_needs_instance = &GLES3::MeshStorage::mesh_needs_instance;
	RendererMeshStorage::mesh_add_surface = &GLES3::MeshStorage::mesh_add_surface;
	RendererMeshStorage::mesh_get_blend_shape_count =
		&GLES3::MeshStorage::mesh_get_blend_shape_count;
	RendererMeshStorage::mesh_set_blend_shape_mode = &GLES3::MeshStorage::mesh_set_blend_shape_mode;
	RendererMeshStorage::mesh_get_blend_shape_mode = &GLES3::MeshStorage::mesh_get_blend_shape_mode;
	RendererMeshStorage::mesh_surface_update_vertex_region =
		&GLES3::MeshStorage::mesh_surface_update_vertex_region;
	RendererMeshStorage::mesh_surface_update_attribute_region =
		&GLES3::MeshStorage::mesh_surface_update_attribute_region;
	RendererMeshStorage::mesh_surface_update_skin_region =
		&GLES3::MeshStorage::mesh_surface_update_skin_region;
	RendererMeshStorage::mesh_surface_update_index_region =
		&GLES3::MeshStorage::mesh_surface_update_index_region;
	RendererMeshStorage::mesh_surface_set_material = &GLES3::MeshStorage::mesh_surface_set_material;
	RendererMeshStorage::mesh_surface_get_material = &GLES3::MeshStorage::mesh_surface_get_material;
	RendererMeshStorage::mesh_get_surface = [](RID p_mesh, int p_surface) {
		return GLES3::MeshStorage::mesh_get_surface(p_mesh, p_surface);
	};
	RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
	RendererMeshStorage::mesh_get_surface_count = &GLES3::MeshStorage::mesh_get_surface_count;
	RendererMeshStorage::mesh_set_custom_aabb = &GLES3::MeshStorage::mesh_set_custom_aabb;
	RendererMeshStorage::mesh_get_custom_aabb = &GLES3::MeshStorage::mesh_get_custom_aabb;
	RendererMeshStorage::mesh_get_aabb = &GLES3::MeshStorage::mesh_get_aabb;
	RendererMeshStorage::mesh_set_path = &GLES3::MeshStorage::mesh_set_path;
	RendererMeshStorage::mesh_get_path = &GLES3::MeshStorage::mesh_get_path;
	RendererMeshStorage::mesh_set_shadow_mesh = &GLES3::MeshStorage::mesh_set_shadow_mesh;
	RendererMeshStorage::mesh_clear = &GLES3::MeshStorage::mesh_clear;
	RendererMeshStorage::mesh_surface_remove = &GLES3::MeshStorage::mesh_surface_remove;
	RendererMeshStorage::mesh_debug_usage = &GLES3::MeshStorage::mesh_debug_usage;

	RendererMeshStorage::mesh_instance_create = &GLES3::MeshStorage::mesh_instance_create;
	RendererMeshStorage::mesh_instance_free = &GLES3::MeshStorage::mesh_instance_free;
	RendererMeshStorage::mesh_instance_set_skeleton =
		&GLES3::MeshStorage::mesh_instance_set_skeleton;
	RendererMeshStorage::mesh_instance_set_blend_shape_weight =
		&GLES3::MeshStorage::mesh_instance_set_blend_shape_weight;
	RendererMeshStorage::mesh_instance_check_for_update =
		&GLES3::MeshStorage::mesh_instance_check_for_update;
	RendererMeshStorage::mesh_instance_set_canvas_item_transform =
		&GLES3::MeshStorage::mesh_instance_set_canvas_item_transform;
	RendererMeshStorage::update_mesh_instances = &GLES3::MeshStorage::update_mesh_instances;

	RendererMeshStorage::_multimesh_allocate = &GLES3::MeshStorage::_multimesh_allocate;
	RendererMeshStorage::_multimesh_initialize = &GLES3::MeshStorage::_multimesh_initialize;
	RendererMeshStorage::_multimesh_free = &GLES3::MeshStorage::_multimesh_free;
	RendererMeshStorage::_multimesh_allocate_data = &GLES3::MeshStorage::_multimesh_allocate_data;
	RendererMeshStorage::_multimesh_get_instance_count =
		&GLES3::MeshStorage::_multimesh_get_instance_count;
	RendererMeshStorage::_multimesh_set_mesh = &GLES3::MeshStorage::_multimesh_set_mesh;
	RendererMeshStorage::_multimesh_instance_set_transform =
		&GLES3::MeshStorage::_multimesh_instance_set_transform;
	RendererMeshStorage::_multimesh_instance_set_transform_2d =
		&GLES3::MeshStorage::_multimesh_instance_set_transform_2d;
	RendererMeshStorage::_multimesh_instance_set_color =
		&GLES3::MeshStorage::_multimesh_instance_set_color;
	RendererMeshStorage::_multimesh_instance_set_custom_data =
		&GLES3::MeshStorage::_multimesh_instance_set_custom_data;
	RendererMeshStorage::_multimesh_set_custom_aabb =
		&GLES3::MeshStorage::_multimesh_set_custom_aabb;
	RendererMeshStorage::_multimesh_get_custom_aabb =
		&GLES3::MeshStorage::_multimesh_get_custom_aabb;
	RendererMeshStorage::_multimesh_get_mesh = &GLES3::MeshStorage::_multimesh_get_mesh;
	RendererMeshStorage::_multimesh_instance_get_transform =
		&GLES3::MeshStorage::_multimesh_instance_get_transform;
	RendererMeshStorage::_multimesh_instance_get_transform_2d =
		&GLES3::MeshStorage::_multimesh_instance_get_transform_2d;
	RendererMeshStorage::_multimesh_instance_get_color =
		&GLES3::MeshStorage::_multimesh_instance_get_color;
	RendererMeshStorage::_multimesh_instance_get_custom_data =
		&GLES3::MeshStorage::_multimesh_instance_get_custom_data;
	RendererMeshStorage::_multimesh_set_buffer = &GLES3::MeshStorage::_multimesh_set_buffer;
	RendererMeshStorage::_multimesh_get_command_buffer_rd_rid =
		&GLES3::MeshStorage::_multimesh_get_command_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer_rd_rid =
		&GLES3::MeshStorage::_multimesh_get_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer = &GLES3::MeshStorage::_multimesh_get_buffer;
	RendererMeshStorage::_multimesh_set_visible_instances =
		&GLES3::MeshStorage::_multimesh_set_visible_instances;
	RendererMeshStorage::_multimesh_get_visible_instances =
		&GLES3::MeshStorage::_multimesh_get_visible_instances;
	RendererMeshStorage::_multimesh_get_aabb = &GLES3::MeshStorage::_multimesh_get_aabb;
	RendererMeshStorage::_multimesh_get_interpolator =
		&GLES3::MeshStorage::_multimesh_get_interpolator;

	RendererMeshStorage::skeleton_allocate = &GLES3::MeshStorage::skeleton_allocate;
	RendererMeshStorage::skeleton_initialize = &GLES3::MeshStorage::skeleton_initialize;
	RendererMeshStorage::skeleton_free = &GLES3::MeshStorage::skeleton_free;
	RendererMeshStorage::skeleton_allocate_data = &GLES3::MeshStorage::skeleton_allocate_data;
	RendererMeshStorage::skeleton_set_base_transform_2d =
		&GLES3::MeshStorage::skeleton_set_base_transform_2d;
	RendererMeshStorage::skeleton_get_bone_count = &GLES3::MeshStorage::skeleton_get_bone_count;
	RendererMeshStorage::skeleton_bone_set_transform =
		&GLES3::MeshStorage::skeleton_bone_set_transform;
	RendererMeshStorage::skeleton_bone_get_transform =
		&GLES3::MeshStorage::skeleton_bone_get_transform;
	RendererMeshStorage::skeleton_bone_set_transform_2d =
		&GLES3::MeshStorage::skeleton_bone_set_transform_2d;
	RendererMeshStorage::skeleton_bone_get_transform_2d =
		&GLES3::MeshStorage::skeleton_bone_get_transform_2d;
	RendererMeshStorage::skeleton_update_dependency =
		&GLES3::MeshStorage::skeleton_update_dependency;
}

void RasterizerGLES3::end_frame(bool p_swap_buffers) { GLES3::Utilities::capture_timestamps_end(); }

void RasterizerGLES3::gl_end_frame(bool p_swap_buffers)
{
	if (p_swap_buffers) {
		DisplayServer::swap_buffers();
	}
	else {
		glFinish();
	}
}

#ifdef GL_DEBUG_CALLBACK
static void GLAPIENTRY _gl_debug_print(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	// These are ultimately annoying, so removing for now.
	if (type == _EXT_DEBUG_TYPE_OTHER_ARB || type == _EXT_DEBUG_TYPE_PERFORMANCE_ARB ||
		type == _EXT_DEBUG_TYPE_MARKER_ARB) {
		return;
	}

	char debSource[256], debType[256], debSev[256];

	if (source == _EXT_DEBUG_SOURCE_API_ARB) {
		strcpy(debSource, "OpenGL");
	}
	else if (source == _EXT_DEBUG_SOURCE_WINDOW_SYSTEM_ARB) {
		strcpy(debSource, "Windows");
	}
	else if (source == _EXT_DEBUG_SOURCE_SHADER_COMPILER_ARB) {
		strcpy(debSource, "Shader Compiler");
	}
	else if (source == _EXT_DEBUG_SOURCE_THIRD_PARTY_ARB) {
		strcpy(debSource, "Third Party");
	}
	else if (source == _EXT_DEBUG_SOURCE_APPLICATION_ARB) {
		strcpy(debSource, "Application");
	}
	else if (source == _EXT_DEBUG_SOURCE_OTHER_ARB) {
		strcpy(debSource, "Other");
	}
	else {
		ERR_FAIL_MSG(
			vformat("GL ERROR: Invalid or unhandled source '%d' in debug callback.", source));
	}

	if (type == _EXT_DEBUG_TYPE_ERROR_ARB) {
		strcpy(debType, "Error");
	}
	else if (type == _EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB) {
		strcpy(debType, "Deprecated behavior");
	}
	else if (type == _EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) {
		strcpy(debType, "Undefined behavior");
	}
	else if (type == _EXT_DEBUG_TYPE_PORTABILITY_ARB) {
		strcpy(debType, "Portability");
	}
	else {
		ERR_FAIL_MSG(vformat("GL ERROR: Invalid or unhandled type '%d' in debug callback.", type));
	}

	if (severity == _EXT_DEBUG_SEVERITY_HIGH_ARB) {
		strcpy(debSev, "High");
	}
	else if (severity == _EXT_DEBUG_SEVERITY_MEDIUM_ARB) {
		strcpy(debSev, "Medium");
	}
	else if (severity == _EXT_DEBUG_SEVERITY_LOW_ARB) {
		strcpy(debSev, "Low");
	}
	else {
		ERR_FAIL_MSG(
			vformat("GL ERROR: Invalid or unhandled severity '%d' in debug callback.", severity));
	}

	String output = String() + "GL ERROR: Source: " + debSource + "\tType: " + debType +
					"\tID: " + itos(id) + "\tSeverity: " + debSev + "\tMessage: " + message;

	ERR_PRINT(output);
}
#endif // GL_DEBUG_CALLBACK

typedef void(GLAPIENTRY* DEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const char* message, const void* userParam);

typedef void(GLAPIENTRY* DebugMessageCallbackARB)(DEBUGPROCARB callback, const void* userParam);

#if defined(GLAD_ENABLED) && defined(EGL_ENABLED)
void* _egl_load_function_wrapper(const char* p_name) { return (void*)eglGetProcAddress(p_name); }
#endif

void RasterizerGLES3::initialize()
{
#ifdef GLAD_ENABLED
	bool glad_loaded = false;

#ifdef EGL_ENABLED
#if defined(EGL_STATIC)
	bool has_egl = true;
#else
	bool has_egl = (eglGetProcAddress != nullptr);
#endif

	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (has_egl && !glad_loaded && gladLoadGL((GLADloadfunc)&_egl_load_function_wrapper)) {
			glad_loaded = true;
		}
	}
	else {
		if (has_egl && !glad_loaded && gladLoadGLES2((GLADloadfunc)&_egl_load_function_wrapper)) {
			glad_loaded = true;
		}
	}
#endif // EGL_ENABLED

	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (!glad_loaded && gladLoaderLoadGL()) {
			glad_loaded = true;
		}
#ifdef GLES_API_ENABLED
	}
	else {
		if (!glad_loaded && gladLoaderLoadGLES2()) {
			glad_loaded = true;
		}
#endif
	}

	ERR_FAIL_COND_MSG(!glad_loaded, "Error initializing GLAD.");
#endif // GLAD_ENABLED

	// Print adapter details only AFTER GLAD has populated glGetString
	Engine::print_header(vformat("OpenGL API %s - Compatibility - Using Device: %s - %s",
		Renderer::get_video_adapter_api_version(), Renderer::get_video_adapter_vendor(),
		Renderer::get_video_adapter_name()));
	if (Engine::get_gpu_index() >= 0) {
		WARN_PRINT(
			"The Compatibility renderer does not support overriding the GPU with the --gpu-index "
			"command line argument. Falling back to the default GPU for OpenGL applications.");
	}

#ifdef GL_DEBUG_CALLBACK
	// Setup OpenGL debug callback, via DebugMessageCallbackARB (GLAD or EGL).

#ifdef GLAD_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose()) {
			if (GLAD_GL_ARB_debug_output) {
				glEnable(_EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB((GLDEBUGPROCARB)_gl_debug_print, nullptr);
				glEnable(_EXT_DEBUG_OUTPUT);
			}
			else {
				__print_line("OpenGL debugging not supported!");
			}
		}
	}

#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose() && GLAD_GL_ARB_debug_output) {
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_ERROR_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB,
				_EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB, _EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr,
				GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB,
				_EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB, _EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr,
				GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_PORTABILITY_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_PERFORMANCE_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_OTHER_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
		}
	}
#endif // GL_API_ENABLED
#endif // GLAD_ENABLED

#if defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
#ifdef GLES_API_ENABLED
	if (!RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose()) {
			DebugMessageCallbackARB callback =
				(DebugMessageCallbackARB)eglGetProcAddress("glDebugMessageCallback");
			if (!callback) {
				callback = (DebugMessageCallbackARB)eglGetProcAddress("glDebugMessageCallbackKHR");
			}

			if (callback) {
				__print_line("godot: ENABLING GL DEBUG");
				glEnable(_EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				callback((DEBUGPROCARB)_gl_debug_print, nullptr);
				glEnable(_EXT_DEBUG_OUTPUT);
			}
		}
	}
#endif // GLES_API_ENABLED
#endif // EGL_ENABLED || ANDROID_ENABLED

#endif // GL_DEBUG_CALLBACK

	{
		// Setup shader cache.

		String shader_cache_dir = Engine::get_shader_cache_path();
		if (shader_cache_dir.is_empty()) {
			shader_cache_dir = "user://";
		}
		Ref<DirAccess> da = DirAccess::open(shader_cache_dir);
		if (da.is_null()) {
			ERR_PRINT("Can't create shader cache folder, no shader caching will happen: " +
					  shader_cache_dir);
		}
		else {
			Error err = da->change_dir("shader_cache");
			if (err != OK) {
				err = da->make_dir("shader_cache");
			}
			if (err != OK) {
				ERR_PRINT("Can't create shader cache folder, no shader caching will happen: " +
						  shader_cache_dir);
			}
			else {
				shader_cache_dir = shader_cache_dir.path_join("shader_cache");
				if (!Engine::is_editor_hint()) {
					shader_cache_dir = String(); // disable only if not editor
				}
				if (!shader_cache_dir.is_empty()) {
					ShaderGLES3::set_shader_cache_dir(shader_cache_dir);
				}
			}
		}
	}

	// OpenGL needs to be initialized before initializing the Rasterizers
	GLES3::Config::initialize();
	GLES3::Utilities::initialize();
	GLES3::TextureStorage::initialize();
	GLES3::MaterialStorage::initialize();
	GLES3::MeshStorage::initialize();
	GLES3::LightStorage::initialize();
	GLES3::ParticlesStorage::initialize();
	GLES3::CopyEffects::initialize();
	GLES3::CubemapFilter::initialize();
	GLES3::Glow::initialize();
	GLES3::PostEffects::initialize();
	GLES3::FeedEffects::initialize();
	GLES3::GI::initialize();
	GLES3::Fog::initialize();

	RasterizerCanvasGLES3::initialize();
	scene = memnew(RasterizerSceneGLES3(texture_storage, material_storage, light_storage));
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::set_scene_render(scene);
	}
	printf("[LIFECYCLE 1] Scene allocated: scene=%p, vptr=%p\n", (void*)scene, *(void**)scene);
	fflush(stdout);
	RendererTextureStorage::bind<GLES3::TextureStorage>();
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	GLES3::TextureStorage::_tex_blit_shader_initialize();

	// Disable OpenGL linear to sRGB conversion, because Godot will always do this conversion
	// itself.
	if (GLES3::Config::srgb_framebuffer_supported) {
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}

void RasterizerGLES3::finalize()
{
	static bool finalized = false;
	if (finalized) {
		return;
	}
	finalized = true;
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	GLES3::TextureStorage::_tex_blit_shader_free();
	printf("[LIFECYCLE 4] About to memdelete(scene): scene=%p, vptr=%p\n", (void*)scene,
		scene ? *(void**)scene : nullptr);
	fflush(stdout);
	if (RendererSceneCull::is_initialized() && RendererSceneCull::get_scene_render() == scene) {
		RendererSceneCull::set_scene_render(nullptr);
	}
	if (scene) {
		memdelete(scene);
		scene = nullptr;
	}
	RasterizerCanvasGLES3::finalize();
	GLES3::GI::finalize();
	GLES3::Fog::finalize();
	GLES3::PostEffects::finalize();
	GLES3::Glow::finalize();
	GLES3::CubemapFilter::finalize();
	GLES3::CopyEffects::finalize();
	GLES3::FeedEffects::finalize();
	GLES3::ParticlesStorage::finalize();
	GLES3::MeshStorage::finalize();
	GLES3::MaterialStorage::finalize();
	GLES3::TextureStorage::finalize();
	GLES3::Utilities::finalize();
	GLES3::Config::finalize();
}

void RasterizerGLES3::make_current(bool p_gles_over_gl)
{
	RasterizerUtilGLES3::set_gles_over_gl(p_gles_over_gl);
	OS::set_gles_over_gl(p_gles_over_gl);

	RendererCompositor::low_end = true;
	RendererCompositor::bind_compositor<RasterizerGLES3>();
}

void RasterizerGLES3::_blit_render_target_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen& p_blit, bool p_first)
{
	GLES3::RenderTarget* rt = GLES3::TextureStorage::get_render_target(p_blit.render_target);
	ERR_FAIL_NULL(rt);

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

	if (p_first) {
		if (p_blit.dst_rect.position != Vector2() || p_blit.dst_rect.size != rt->size) {
			Size2i max_vp = GLES3::Utilities::get_maximum_viewport_size();
			glViewport(0, 0, max_vp[0], max_vp[1]);
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	Vector2 screen_rect_end = p_blit.dst_rect.get_end();
	Vector2 p1 = Vector2(p_blit.dst_rect.position.x, screen_rect_end.y);
	Vector2 p2 = Vector2(screen_rect_end.x, p_blit.dst_rect.position.y);
	Vector2 size = p2 - p1;

	bool flip_y = !rt->overridden.is_overridden;
	bool linear_to_srgb = false;

	Rect2 screenrect = Rect2(Vector2(0.0, flip_y ? 1.0 : 0.0), Vector2(1.0, flip_y ? -1.0 : 1.0));
	glViewport(int(p1.x), int(DisplayServer::window_get_size(p_screen).y - p1.y),
		int(size.x), int(-size.y));

	GLenum target = (rt->view_count > 1) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(target, rt->color);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	if (p_blit.lens_distortion.apply &&
		(p_blit.lens_distortion.k1 != 0.0 || p_blit.lens_distortion.k2)) {
		GLES3::CopyEffects::copy_with_lens_distortion(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0,
			p_blit.lens_distortion.eye_center, p_blit.lens_distortion.k1, p_blit.lens_distortion.k2,
			p_blit.lens_distortion.upscale, p_blit.lens_distortion.aspect_ratio, linear_to_srgb);
	}
	else if (rt->view_count > 1) {
		GLES3::CopyEffects::copy_to_rect_3d(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0, GLES3::Texture::TYPE_LAYERED,
			0.0, linear_to_srgb);
	}
	else {
		GLES3::CopyEffects::copy_to_rect(screenrect, linear_to_srgb);
	}

	glBindTexture(target, 0);
}

// is this p_screen useless in a multi window environment?
void RasterizerGLES3::blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	// Querying the actual window size from the DisplayServer would deadlock in separate
	// render thread mode, so set the biggest viewport the implementation supports to
	// ensure the entire window is covered (including across resizes).
	Size2i max_vp = GLES3::Utilities::get_maximum_viewport_size();
	glViewport(0, 0, max_vp[0], max_vp[1]);
	// glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < p_amount; i++) {
		_blit_render_target_to_screen(p_screen, p_render_targets[i], i == 0);
	}
}

void RasterizerGLES3::set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
	RSE::SplashStretchMode p_stretch_mode, bool p_use_filter)
{
	if (p_image.is_null() || p_image->is_empty()) {
		return;
	}

	Size2i win_size = DisplayServer::window_get_size();

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glViewport(0, 0, win_size.width, win_size.height);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glClearColor(p_color.r, p_color.g, p_color.b, OS::is_layered_allowed() ? p_color.a : 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	RID texture = GLES3::TextureStorage::texture_allocate();
	GLES3::TextureStorage::texture_2d_initialize(texture, p_image);

	Rect2 screenrect = RenderingServerTypes::get_splash_stretched_screen_rect(
		p_image->get_size(), win_size, p_stretch_mode);

#ifdef WINDOWS_ENABLED
	if (!screen_flipped_y)
#endif
	{
		// Flip Y.
		screenrect.position.y = win_size.y - screenrect.position.y;
		screenrect.size.y = -screenrect.size.y;
	}

	// Normalize texture coordinates to window size.
	screenrect.position /= win_size;
	screenrect.size /= win_size;

	GLES3::Texture* t = GLES3::TextureStorage::get_texture(texture);
	t->gl_set_filter(p_use_filter ? RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR
								  : RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t->tex_id);
	GLES3::CopyEffects::copy_to_rect(screenrect);
	glBindTexture(GL_TEXTURE_2D, 0);

	gl_end_frame(true);

	GLES3::TextureStorage::texture_free(texture);
}

void RasterizerGLES3::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&GLES3::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&GLES3::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free = &GLES3::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&GLES3::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&GLES3::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call = &GLES3::Utilities::visibility_notifier_call;

	RendererUtilities::free = &GLES3::Utilities::free;
	RendererUtilities::get_base_type = &GLES3::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &GLES3::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin = &GLES3::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &GLES3::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&GLES3::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&GLES3::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&GLES3::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&GLES3::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
&GLES3::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &GLES3::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&GLES3::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &GLES3::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &GLES3::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &GLES3::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &GLES3::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor = &GLES3::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &GLES3::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&GLES3::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size = &GLES3::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings = &GLES3::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&GLES3::Utilities::get_maximum_uniform_buffer_size;
}

#endif // GLES3_ENABLED

// ============================================================================
// Merged from drivers/gles3/effects/gles3_effects.cpp
// ============================================================================

#include "drivers/gles3/storage/utilities.h"

#ifdef GLES3_ENABLED

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "servers/rendering/rendering_server_enums.h"

using namespace GLES3;

// CopyEffects

void CopyEffects::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->copy.shader.initialize();
	data->copy.shader_version = data->copy.shader.version_create();
	data->copy.shader.version_bind_shader(data->copy.shader_version, CopyShaderGLES3::MODE_DEFAULT);

	{ // Screen Triangle.
		glGenBuffers(1, &data->screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->screen_triangle_array);
		glBindVertexArray(data->screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}

	{ // Screen Quad
		glGenBuffers(1, &data->quad);
		glBindBuffer(GL_ARRAY_BUFFER, data->quad);

		const float qv[12] = {
			-1.0f,
			-1.0f,
			1.0f,
			-1.0f,
			1.0f,
			1.0f,
			-1.0f,
			-1.0f,
			1.0f,
			1.0f,
			-1.0f,
			1.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->quad_array);
		glBindVertexArray(data->quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->quad);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}
}

void CopyEffects::finalize()
{
	if (!data) {
		return;
	}

	glDeleteBuffers(1, &data->screen_triangle);
	glDeleteVertexArrays(1, &data->screen_triangle_array);
	glDeleteBuffers(1, &data->quad);
	glDeleteVertexArrays(1, &data->quad_array);
	data->copy.shader.version_free(data->copy.shader_version);

	memdelete(data);
	data = nullptr;
}

void CopyEffects::copy_to_rect(const Rect2& p_rect, bool p_linear_to_srgb)
{
	ERR_FAIL_NULL(data);

	uint64_t specializations = p_linear_to_srgb ? CopyShaderGLES3::CONVERT_LINEAR_TO_SRGB : 0;

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_COPY_SECTION, specializations);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version,
		CopyShaderGLES3::MODE_COPY_SECTION, specializations);
	draw_screen_quad();
}

void CopyEffects::copy_to_rect_3d(
	const Rect2& p_rect, float p_layer, int p_type, float p_lod, bool p_linear_to_srgb)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(p_type != Texture::TYPE_LAYERED && p_type != Texture::TYPE_3D);

	CopyShaderGLES3::ShaderVariant variant = p_type == Texture::TYPE_LAYERED
												 ? CopyShaderGLES3::MODE_COPY_SECTION_2D_ARRAY
												 : CopyShaderGLES3::MODE_COPY_SECTION_3D;
	uint64_t specializations = p_linear_to_srgb ? CopyShaderGLES3::CONVERT_LINEAR_TO_SRGB : 0;

	bool success =
		data->copy.shader.version_bind_shader(data->copy.shader_version, variant, specializations);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version, variant,
		specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::LAYER, p_layer, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::LOD, p_lod, data->copy.shader_version, variant, specializations);
	draw_screen_quad();
}

void CopyEffects::copy_with_lens_distortion(const Rect2& p_rect, float p_layer,
	const Vector2& p_eye_center, float p_k1, float p_k2, float p_upscale, float p_aspect_ration,
	bool p_linear_to_srgb)
{
	ERR_FAIL_NULL(data);

	CopyShaderGLES3::ShaderVariant variant = CopyShaderGLES3::MODE_LENS_DISTORTION;

	uint64_t specializations = p_linear_to_srgb ? CopyShaderGLES3::CONVERT_LINEAR_TO_SRGB : 0;

	bool success =
		data->copy.shader.version_bind_shader(data->copy.shader_version, variant, specializations);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version, variant,
		specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::LAYER, p_layer, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::LOD, 0.0, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(CopyShaderGLES3::EYE_CENTER, p_eye_center.x,
		p_eye_center.y, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::K1, p_k1, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::K2, p_k2, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(
		CopyShaderGLES3::UPSCALE, p_upscale, data->copy.shader_version, variant, specializations);
	data->copy.shader.version_set_uniform(CopyShaderGLES3::ASPECT_RATIO, p_aspect_ration,
		data->copy.shader_version, variant, specializations);

	draw_screen_quad();
}

void CopyEffects::copy_to_and_from_rect(const Rect2& p_rect)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_COPY_SECTION_SOURCE);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version,
		CopyShaderGLES3::MODE_COPY_SECTION_SOURCE);
	data->copy.shader.version_set_uniform(CopyShaderGLES3::SOURCE_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version,
		CopyShaderGLES3::MODE_COPY_SECTION_SOURCE);

	draw_screen_quad();
}

void CopyEffects::copy_screen(float p_multiply)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_SCREEN);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::MULTIPLY, p_multiply,
		data->copy.shader_version, CopyShaderGLES3::MODE_SCREEN);

	draw_screen_triangle();
}

void CopyEffects::copy_with_exposure(float p_exposure, float p_multiply)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_APPLY_LINEAR_EXPOSURE_TO_SRGB);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::EXPOSURE, p_exposure,
		data->copy.shader_version, CopyShaderGLES3::MODE_APPLY_LINEAR_EXPOSURE_TO_SRGB);
	data->copy.shader.version_set_uniform(CopyShaderGLES3::MULTIPLY, p_multiply,
		data->copy.shader_version, CopyShaderGLES3::MODE_APPLY_LINEAR_EXPOSURE_TO_SRGB);

	draw_screen_triangle();
}

void CopyEffects::copy_cube_to_rect(const Rect2& p_rect)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_CUBE_TO_OCTAHEDRAL);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_rect.position.x,
		p_rect.position.y, p_rect.size.x, p_rect.size.y, data->copy.shader_version,
		CopyShaderGLES3::MODE_CUBE_TO_OCTAHEDRAL);
	draw_screen_quad();
}

void CopyEffects::copy_cube_to_panorama(float p_mip_level)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_CUBE_TO_PANORAMA);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::MIP_LEVEL, p_mip_level,
		data->copy.shader_version, CopyShaderGLES3::MODE_CUBE_TO_PANORAMA);
	draw_screen_quad();
}

// Intended for efficiently mipmapping textures.
void CopyEffects::bilinear_blur(GLuint p_source_texture, int p_mipmap_count, const Rect2i& p_region)
{
	GLuint framebuffers[2];
	glGenFramebuffers(2, framebuffers);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
	glFramebufferTexture2D(
		GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_source_texture, 0);

	Rect2i source_region = p_region;
	Rect2i dest_region = p_region;
	for (int i = 1; i < p_mipmap_count; i++) {
		dest_region.position.x >>= 1;
		dest_region.position.y >>= 1;
		dest_region.size = Size2i(dest_region.size.x >> 1, dest_region.size.y >> 1).maxi(1);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[i % 2]);
		glFramebufferTexture2D(
			GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_source_texture, i);
		glBlitFramebuffer(source_region.position.x, source_region.position.y,
			source_region.position.x + source_region.size.x,
			source_region.position.y + source_region.size.y, dest_region.position.x,
			dest_region.position.y, dest_region.position.x + dest_region.size.x,
			dest_region.position.y + dest_region.size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[i % 2]);
		source_region = dest_region;
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDeleteFramebuffers(2, framebuffers);
}

// Intended for approximating a gaussian blur. Used for 2D backbuffer mipmaps. Slightly less
// efficient than bilinear_blur().
void CopyEffects::gaussian_blur(
	GLuint p_source_texture, int p_mipmap_count, const Rect2i& p_region, const Size2i& p_size)
{
	ERR_FAIL_NULL(data);

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_source_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Size2i base_size = p_size;

	Rect2i source_region = p_region;
	Rect2i dest_region = p_region;

	Size2 float_size = Size2(p_size);
	Rect2 normalized_source_region = Rect2(p_region);
	normalized_source_region.position = normalized_source_region.position / float_size;
	normalized_source_region.size = normalized_source_region.size / float_size;
	Rect2 normalized_dest_region = Rect2(p_region);
	for (int i = 1; i < p_mipmap_count; i++) {
		dest_region.position.x >>= 1;
		dest_region.position.y >>= 1;
		dest_region.size = Size2i(dest_region.size.x >> 1, dest_region.size.y >> 1).maxi(1);
		base_size.x >>= 1;
		base_size.y >>= 1;

		glBindTexture(GL_TEXTURE_2D, p_source_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_source_texture, i);
#ifdef DEV_ENABLED
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			WARN_PRINT("Could not bind Gaussian blur framebuffer, status: " +
					   GLES3::TextureStorage::get_framebuffer_error(status));
		}
#endif

		glViewport(0, 0, base_size.x, base_size.y);

		bool success = data->copy.shader.version_bind_shader(
			data->copy.shader_version, CopyShaderGLES3::MODE_GAUSSIAN_BLUR);
		if (!success) {
			return;
		}

		float_size = Size2(base_size);
		normalized_dest_region.position = Size2(dest_region.position) / float_size;
		normalized_dest_region.size = Size2(dest_region.size) / float_size;

		data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION,
			normalized_dest_region.position.x, normalized_dest_region.position.y,
			normalized_dest_region.size.x, normalized_dest_region.size.y, data->copy.shader_version,
			CopyShaderGLES3::MODE_GAUSSIAN_BLUR);
		data->copy.shader.version_set_uniform(CopyShaderGLES3::SOURCE_SECTION,
			normalized_source_region.position.x, normalized_source_region.position.y,
			normalized_source_region.size.x, normalized_source_region.size.y,
			data->copy.shader_version, CopyShaderGLES3::MODE_GAUSSIAN_BLUR);
		data->copy.shader.version_set_uniform(CopyShaderGLES3::PIXEL_SIZE, 1.0 / float_size.x,
			1.0 / float_size.y, data->copy.shader_version, CopyShaderGLES3::MODE_GAUSSIAN_BLUR);

		draw_screen_quad();

		source_region = dest_region;
		normalized_source_region = normalized_dest_region;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDeleteFramebuffers(1, &framebuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, p_mipmap_count - 1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glViewport(0, 0, p_size.x, p_size.y);
}

void CopyEffects::set_color(const Color& p_color, const Rect2i& p_region)
{
	ERR_FAIL_NULL(data);

	bool success = data->copy.shader.version_bind_shader(
		data->copy.shader_version, CopyShaderGLES3::MODE_SIMPLE_COLOR);
	if (!success) {
		return;
	}

	data->copy.shader.version_set_uniform(CopyShaderGLES3::COPY_SECTION, p_region.position.x,
		p_region.position.y, p_region.size.x, p_region.size.y, data->copy.shader_version,
		CopyShaderGLES3::MODE_SIMPLE_COLOR);
	data->copy.shader.version_set_uniform(CopyShaderGLES3::COLOR_IN, p_color,
		data->copy.shader_version, CopyShaderGLES3::MODE_SIMPLE_COLOR);
	draw_screen_quad();
}

void CopyEffects::draw_screen_triangle()
{
	ERR_FAIL_NULL(data);

	glBindVertexArray(data->screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void CopyEffects::draw_screen_quad()
{
	ERR_FAIL_NULL(data);

	glBindVertexArray(data->quad_array);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

// CubemapFilter

void CubemapFilter::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	{
		String defines;
		defines += "\n#define MAX_SAMPLE_COUNT " + itos(data->ggx_samples) + "\n";
		data->cubemap_filter.shader.initialize(defines);
		data->cubemap_filter.shader_version = data->cubemap_filter.shader.version_create();
	}

	{ // Screen Triangle.
		glGenBuffers(1, &data->screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			-1.0f,
			3.0f,
			3.0f,
			-1.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->screen_triangle_array);
		glBindVertexArray(data->screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}
}

void CubemapFilter::finalize()
{
	if (!data) {
		return;
	}

	glDeleteBuffers(1, &data->screen_triangle);
	glDeleteVertexArrays(1, &data->screen_triangle_array);
	data->cubemap_filter.shader.version_free(data->cubemap_filter.shader_version);

	memdelete(data);
	data = nullptr;
}

static Vector3 importance_sample_GGX(Vector2 xi, float roughness4)
{
	// Compute distribution direction
	float phi = 2.0 * Math::PI * xi.x;
	float cos_theta = std::sqrt((1.0 - xi.y) / (1.0 + (roughness4 - 1.0) * xi.y));
	float sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

	// Convert to spherical direction
	Vector3 half_vector;
	half_vector.x = sin_theta * std::cos(phi);
	half_vector.y = sin_theta * std::sin(phi);
	half_vector.z = cos_theta;

	return half_vector;
}

static float distribution_GGX(float NdotH, float roughness4)
{
	float NdotH2 = NdotH * NdotH;
	float denom = (NdotH2 * (roughness4 - 1.0) + 1.0);
	denom = Math::PI * denom * denom;

	return roughness4 / denom;
}

static float radical_inverse_vdC(uint32_t bits)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
	bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
	bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);

	return float(bits) * 2.3283064365386963e-10;
}

static Vector2 hammersley(uint32_t i, uint32_t N)
{
	return Vector2(float(i) / float(N), radical_inverse_vdC(i));
}

void CubemapFilter::filter_radiance(GLuint p_source_cubemap, GLuint p_dest_cubemap,
	GLuint p_dest_framebuffer, int p_source_size, int p_mipmap_count, int p_layer)
{
	ERR_FAIL_NULL(data);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, p_source_cubemap);
	glBindFramebuffer(GL_FRAMEBUFFER, p_dest_framebuffer);

	CubemapFilterShaderGLES3::ShaderVariant mode = CubemapFilterShaderGLES3::MODE_DEFAULT;

	if (p_layer == 0) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		// Copy over base layer without filtering.
		mode = CubemapFilterShaderGLES3::MODE_COPY;
	}

	int size = p_source_size >> p_layer;
	glViewport(0, 0, size, size);
	glBindVertexArray(data->screen_triangle_array);

	bool success =
		data->cubemap_filter.shader.version_bind_shader(data->cubemap_filter.shader_version, mode);
	if (!success) {
		return;
	}

	if (p_layer > 0) {
		const uint32_t sample_counts[5] = {
			1, data->ggx_samples / 16, data->ggx_samples / 8, data->ggx_samples / 4, data->ggx_samples};
		uint32_t sample_count = sample_counts[MIN(4, p_layer)];

		float roughness = float(p_layer) / (p_mipmap_count - 1);
		roughness *= roughness; // Convert to non-perceptual roughness.
		float roughness4 = roughness * roughness;
		roughness4 *= roughness4;

		float solid_angle_texel = 4.0 * Math::PI / float(6 * size * size);

		LocalVector<float> sample_directions;
		sample_directions.resize(4 * sample_count);

		uint32_t index = 0;
		float weight = 0.0;
		for (uint32_t i = 0; i < sample_count; i++) {
			Vector2 xi = hammersley(i, sample_count);
			Vector3 dir = importance_sample_GGX(xi, roughness4);
			Vector3 light_vec = (2.0 * dir.z * dir - Vector3(0.0, 0.0, 1.0));

			if (light_vec.z <= 0.0) {
				continue;
			}

			sample_directions[index * 4] = light_vec.x;
			sample_directions[index * 4 + 1] = light_vec.y;
			sample_directions[index * 4 + 2] = light_vec.z;

			float D = distribution_GGX(dir.z, roughness4);
			float pdf = D * dir.z / (4.0 * dir.z) + 0.0001;

			float solid_angle_sample = 1.0 / (float(sample_count) * pdf + 0.0001);

			float mip_level =
				MAX(0.5 * std::log2(solid_angle_sample / solid_angle_texel) + float(MAX(1, p_layer - 3)),
					1.0);

			sample_directions[index * 4 + 3] = mip_level;
			weight += light_vec.z;
			index++;
		}

		glUniform4fv(
			data->cubemap_filter.shader.version_get_uniform(
				CubemapFilterShaderGLES3::SAMPLE_DIRECTIONS_MIP,
				data->cubemap_filter.shader_version, mode),
			sample_count, sample_directions.ptr());
		data->cubemap_filter.shader.version_set_uniform(
			CubemapFilterShaderGLES3::WEIGHT, weight, data->cubemap_filter.shader_version, mode);
		data->cubemap_filter.shader.version_set_uniform(
			CubemapFilterShaderGLES3::SAMPLE_COUNT, index, data->cubemap_filter.shader_version, mode);
	}

	for (int i = 0; i < 6; i++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, p_dest_cubemap, p_layer);
#ifdef DEBUG_ENABLED
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			WARN_PRINT("Could not bind sky radiance face: " + itos(i) +
					   ", status: " + GLES3::TextureStorage::get_framebuffer_error(status));
		}
#endif
		data->cubemap_filter.shader.version_set_uniform(
			CubemapFilterShaderGLES3::FACE_ID, i, data->cubemap_filter.shader_version, mode);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

// FeedEffects

void FeedEffects::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->feed.shader.initialize();
	data->feed.shader_version = data->feed.shader.version_create();
	data->feed.shader.version_bind_shader(data->feed.shader_version, FeedShaderGLES3::MODE_DEFAULT);

	{ // Screen Triangle.
		glGenBuffers(1, &data->screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->screen_triangle_array);
		glBindVertexArray(data->screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}
}

void FeedEffects::finalize()
{
	if (!data) {
		return;
	}

	glDeleteBuffers(1, &data->screen_triangle);
	glDeleteVertexArrays(1, &data->screen_triangle_array);
	data->feed.shader.version_free(data->feed.shader_version);

	memdelete(data);
	data = nullptr;
}

void FeedEffects::draw()
{
	ERR_FAIL_NULL(data);

	bool success = data->feed.shader.version_bind_shader(
		data->feed.shader_version, FeedShaderGLES3::MODE_DEFAULT, FeedShaderGLES3::USE_EXTERNAL_SAMPLER);
	if (!success) {
		OS::print("Godot : FeedShaderGLES3 Could not bind version_bind_shader USE_EXTERNAL_SAMPLER");
		return;
	}

	draw_screen_triangle();
}

void FeedEffects::draw_screen_triangle()
{
	ERR_FAIL_NULL(data);

	glBindVertexArray(data->screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

// Glow

void Glow::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->glow.shader.initialize();
	data->glow.shader_version = data->glow.shader.version_create();

	{ // Screen Triangle.
		glGenBuffers(1, &data->screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->screen_triangle_array);
		glBindVertexArray(data->screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}
}

void Glow::finalize()
{
	if (!data) {
		return;
	}

	glDeleteBuffers(1, &data->screen_triangle);
	glDeleteVertexArrays(1, &data->screen_triangle_array);
	data->glow.shader.version_free(data->glow.shader_version);

	memdelete(data);
	data = nullptr;
}

void Glow::_draw_screen_triangle()
{
	ERR_FAIL_NULL(data);

	glBindVertexArray(data->screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void Glow::process_glow(GLuint p_source_color, Size2i p_size, const Glow::Level* p_glow_buffers,
	uint32_t p_view, bool p_use_multiview)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(p_source_color == 0);
	ERR_FAIL_COND(p_glow_buffers[3].color == 0);

	// Reset some OpenGL state...
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Start with our filter pass
	{
		glBindFramebuffer(GL_FRAMEBUFFER, p_glow_buffers[0].fbo);
		glViewport(0, 0, p_glow_buffers[0].size.x, p_glow_buffers[0].size.y);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(p_use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, p_source_color);

		uint64_t specialization = p_use_multiview ? GlowShaderGLES3::USE_MULTIVIEW : 0;
		bool success = data->glow.shader.version_bind_shader(
			data->glow.shader_version, GlowShaderGLES3::MODE_FILTER, specialization);
		if (!success) {
			return;
		}

		data->glow.shader.version_set_uniform(GlowShaderGLES3::PIXEL_SIZE,
			1.0 / p_glow_buffers[0].size.x, 1.0 / p_glow_buffers[0].size.y,
			data->glow.shader_version, GlowShaderGLES3::MODE_FILTER, specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::VIEW, float(p_view),
			data->glow.shader_version, GlowShaderGLES3::MODE_FILTER, specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::LUMINANCE_MULTIPLIER,
			data->luminance_multiplier, data->glow.shader_version, GlowShaderGLES3::MODE_FILTER,
			specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::GLOW_BLOOM, data->glow_bloom,
			data->glow.shader_version, GlowShaderGLES3::MODE_FILTER, specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::GLOW_HDR_THRESHOLD,
			data->glow_hdr_bleed_threshold, data->glow.shader_version, GlowShaderGLES3::MODE_FILTER,
			specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::GLOW_HDR_SCALE,
			data->glow_hdr_bleed_scale, data->glow.shader_version, GlowShaderGLES3::MODE_FILTER,
			specialization);
		data->glow.shader.version_set_uniform(GlowShaderGLES3::GLOW_LUMINANCE_CAP,
			data->glow_hdr_luminance_cap, data->glow.shader_version, GlowShaderGLES3::MODE_FILTER,
			specialization);

		_draw_screen_triangle();
	}

	// Continue with downsampling
	{
		bool success = data->glow.shader.version_bind_shader(
			data->glow.shader_version, GlowShaderGLES3::MODE_DOWNSAMPLE, 0);
		if (!success) {
			return;
		}

		for (int i = 1; i < 4; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, p_glow_buffers[i].fbo);
			glViewport(0, 0, p_glow_buffers[i].size.x, p_glow_buffers[i].size.y);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, p_glow_buffers[i - 1].color);

			data->glow.shader.version_set_uniform(GlowShaderGLES3::PIXEL_SIZE,
				1.0 / p_glow_buffers[i].size.x, 1.0 / p_glow_buffers[i].size.y,
				data->glow.shader_version, GlowShaderGLES3::MODE_DOWNSAMPLE);

			_draw_screen_triangle();
		}
	}

	// Now upsample
	{
		bool success = data->glow.shader.version_bind_shader(
			data->glow.shader_version, GlowShaderGLES3::MODE_UPSAMPLE, 0);
		if (!success) {
			return;
		}

		for (int i = 2; i >= 0; i--) {
			glBindFramebuffer(GL_FRAMEBUFFER, p_glow_buffers[i].fbo);
			glViewport(0, 0, p_glow_buffers[i].size.x, p_glow_buffers[i].size.y);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, p_glow_buffers[i + 1].color);

			data->glow.shader.version_set_uniform(GlowShaderGLES3::PIXEL_SIZE,
				1.0 / p_glow_buffers[i].size.x, 1.0 / p_glow_buffers[i].size.y,
				data->glow.shader_version, GlowShaderGLES3::MODE_UPSAMPLE);

			_draw_screen_triangle();
		}
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

// PostEffects

void PostEffects::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->post.shader.initialize();
	data->post.shader_version = data->post.shader.version_create();
	data->post.shader.version_bind_shader(data->post.shader_version, PostShaderGLES3::MODE_DEFAULT);

	{ // Screen Triangle.
		glGenBuffers(1, &data->screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

		glGenVertexArrays(1, &data->screen_triangle_array);
		glBindVertexArray(data->screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, data->screen_triangle);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}
}

void PostEffects::finalize()
{
	if (!data) {
		return;
	}

	glDeleteBuffers(1, &data->screen_triangle);
	glDeleteVertexArrays(1, &data->screen_triangle_array);
	data->post.shader.version_free(data->post.shader_version);

	memdelete(data);
	data = nullptr;
}

void PostEffects::_draw_screen_triangle()
{
	ERR_FAIL_NULL(data);

	glBindVertexArray(data->screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void PostEffects::post_copy(GLuint p_dest_framebuffer, Size2i p_dest_size, GLuint p_source_color,
	GLuint p_source_depth, bool p_ssao_enabled, int p_ssao_quality_level, float p_ssao_strength,
	float p_ssao_radius, Size2i p_source_size, float p_luminance_multiplier,
	const Glow::Level* p_glow_buffers, float p_glow_intensity, float p_srgb_white, uint32_t p_view,
	bool p_use_multiview, uint64_t p_spec_constants, bool p_bilinear_filtering)
{
	ERR_FAIL_NULL(data);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, p_dest_framebuffer);
	glViewport(0, 0, p_dest_size.x, p_dest_size.y);

	PostShaderGLES3::ShaderVariant mode = PostShaderGLES3::MODE_DEFAULT;
	uint64_t flags = p_spec_constants;
	if (p_use_multiview) {
		flags |= PostShaderGLES3::USE_MULTIVIEW;
	}
	if (p_glow_buffers != nullptr) {
		flags |= PostShaderGLES3::USE_GLOW;
	}
	if (p_ssao_enabled) {
		if (p_ssao_quality_level == RSE::ENV_SSAO_QUALITY_VERY_LOW) {
			flags |= PostShaderGLES3::USE_SSAO_ABYSS;
		}
		else if (p_ssao_quality_level == RSE::ENV_SSAO_QUALITY_LOW) {
			flags |= PostShaderGLES3::USE_SSAO_LOW;
		}
		else if (p_ssao_quality_level == RSE::ENV_SSAO_QUALITY_HIGH) {
			flags |= PostShaderGLES3::USE_SSAO_HIGH;
		}
		else if (p_ssao_quality_level == RSE::ENV_SSAO_QUALITY_ULTRA) {
			flags |= PostShaderGLES3::USE_SSAO_MEGA;
		}
		else {
			flags |= PostShaderGLES3::USE_SSAO_MED;
		}
	}
	if (p_luminance_multiplier != 1.0) {
		flags |= PostShaderGLES3::USE_LUMINANCE_MULTIPLIER;
	}

	bool success = data->post.shader.version_bind_shader(data->post.shader_version, mode, flags);
	if (!success) {
		return;
	}

	GLenum texture_target = p_use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture_target, p_source_color);

	glTexParameteri(
		texture_target, GL_TEXTURE_MAG_FILTER, p_bilinear_filtering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(
		texture_target, GL_TEXTURE_MIN_FILTER, p_bilinear_filtering ? GL_LINEAR : GL_NEAREST);

	if (p_ssao_enabled) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(texture_target, p_source_depth);
		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // Thanks to mrjustaguy!
		glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		data->post.shader.version_set_uniform(
			PostShaderGLES3::SSAO_INTENSITY, p_ssao_strength, data->post.shader_version, mode, flags);
		data->post.shader.version_set_uniform(
			PostShaderGLES3::SSAO_RADIUS_FRAC, p_ssao_radius, data->post.shader_version, mode, flags);
		data->post.shader.version_set_uniform(
			PostShaderGLES3::SSAO_PRN_UV, // This converts the UV coordinate into a pseudo-random
										  // number.
			p_source_size.x * 1.087f * ((1.0f + sqrt(5.0f)) / 2.0f),
			p_source_size.y * 1.087f * ((9.0f + sqrt(221.0f)) / 10.0f), data->post.shader_version,
			mode, flags);
	}

	if (p_glow_buffers != nullptr) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, p_glow_buffers[0].color);

		data->post.shader.version_set_uniform(PostShaderGLES3::PIXEL_SIZE, 1.0 / p_source_size.x,
			1.0 / p_source_size.y, data->post.shader_version, mode, flags);
		data->post.shader.version_set_uniform(
			PostShaderGLES3::GLOW_INTENSITY, p_glow_intensity, data->post.shader_version, mode, flags);
		data->post.shader.version_set_uniform(
			PostShaderGLES3::SRGB_WHITE, p_srgb_white, data->post.shader_version, mode, flags);
	}

	data->post.shader.version_set_uniform(
		PostShaderGLES3::VIEW, float(p_view), data->post.shader_version, mode, flags);
	data->post.shader.version_set_uniform(PostShaderGLES3::LUMINANCE_MULTIPLIER,
		p_luminance_multiplier, data->post.shader_version, mode, flags);

	_draw_screen_triangle();

	// Reset state
	if (p_glow_buffers != nullptr) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (p_ssao_enabled) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(texture_target, 0);
	}

	// Return back to nearest
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(texture_target, 0);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

#endif // GLES3_ENABLED
