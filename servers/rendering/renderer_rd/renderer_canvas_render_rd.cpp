/**************************************************************************/
/*  renderer_canvas_render_rd.cpp                                         */
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
#include "core/math/geometry_2d.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/transform_interpolator.h"
#include "core/templates/fixed_vector.h"
#include "renderer_canvas_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_server_default.h"

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

RendererCanvasRender::PolygonID RendererCanvasRenderRD::request_polygon(
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

	RendererRD::MeshStorage* mesh_storage = RendererRD::MeshStorage::get_singleton();

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
	Vector<RD::VertexAttribute> descriptions;
	descriptions.resize(5);
	Vector<RID> buffers;
	buffers.resize(5);

	{
		uint8_t* r = polygon_buffer.ptrw();
		float* fptr = reinterpret_cast<float*>(r);
		uint32_t* uptr = reinterpret_cast<uint32_t*>(r);
		uint32_t base_offset = 0;
		{ // vertices
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32_SFLOAT;
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
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
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
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_COLOR;
			vd.stride = 0;

			descriptions.write[1] = vd;
			buffers.write[1] = mesh_storage->mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_COLOR);
		}

		// uvs
		if ((uint32_t)p_uvs.size() == vertex_count) {
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32_SFLOAT;
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
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_TEX_UV;
			vd.stride = 0;

			descriptions.write[2] = vd;
			buffers.write[2] = mesh_storage->mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_TEX_UV);
		}

		// bones
		if ((uint32_t)p_indices.size() == vertex_count * 4 &&
			(uint32_t)p_weights.size() == vertex_count * 4) {
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R16G16B16A16_UINT;
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
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32B32A32_UINT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_BONES;
			vd.stride = 0;

			descriptions.write[3] = vd;
			buffers.write[3] = mesh_storage->mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_BONES);
		}

		// weights
		if ((uint32_t)p_weights.size() == vertex_count * 4) {
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R16G16B16A16_UNORM;
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
			RD::VertexAttribute vd;
			vd.format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
			vd.offset = 0;
			vd.location = RSE::ARRAY_WEIGHTS;
			vd.stride = 0;

			descriptions.write[4] = vd;
			buffers.write[4] = mesh_storage->mesh_get_default_rd_buffer(
				RendererRD::MeshStorage::DEFAULT_RD_BUFFER_WEIGHTS);
		}

		// check that everything is as it should be
		ERR_FAIL_COND_V(base_offset != stride, 0); // bug
	}

	RD::VertexFormatID vertex_id = RD::get_singleton()->vertex_format_create(descriptions);
	ERR_FAIL_COND_V(vertex_id == RD::INVALID_ID, 0);

	PolygonBuffers pb;
	pb.vertex_buffer =
		RD::get_singleton()->vertex_buffer_create(polygon_buffer.size(), polygon_buffer);
	for (int i = 0; i < descriptions.size(); i++) {
		if (buffers[i] == RID()) { // if put in vertex, use as vertex
			buffers.write[i] = pb.vertex_buffer;
		}
	}

	pb.vertex_array = RD::get_singleton()->vertex_array_create(p_points.size(), vertex_id, buffers);
	pb.primitive_count = vertex_count;

	if (p_indices.size()) {
		// create indices, as indices were requested
		Vector<uint8_t> index_buffer;
		index_buffer.resize(p_count * sizeof(int32_t));
		{
			uint8_t* w = index_buffer.ptrw();
			memcpy(w, p_indices.ptr(), sizeof(int32_t) * p_indices.size());
		}
		pb.indices = RD::get_singleton()->index_array_create(pb.index_buffer, 0, p_count);
		pb.primitive_count = p_count;
	}

	pb.vertex_format_id = vertex_id;

	PolygonID id = polygon_buffers.last_id++;

	polygon_buffers.polygons[id] = pb;

	return id;
}

void RendererCanvasRenderRD::free_polygon(PolygonID p_polygon)
{
	PolygonBuffers* pb_ptr = polygon_buffers.polygons.getptr(p_polygon);
	ERR_FAIL_NULL(pb_ptr);

	PolygonBuffers& pb = *pb_ptr;

	if (pb.indices.is_valid()) {
		RD::get_singleton()->free_rid(pb.indices);
	}
	if (pb.index_buffer.is_valid()) {
		RD::get_singleton()->free_rid(pb.index_buffer);
	}

	RD::get_singleton()->free_rid(pb.vertex_array);
	RD::get_singleton()->free_rid(pb.vertex_buffer);

	polygon_buffers.polygons.erase(p_polygon);
}

////////////////////

static RD::RenderPrimitive _primitive_type_to_render_primitive(RSE::PrimitiveType p_primitive)
{
	switch (p_primitive) {
	case RSE::PRIMITIVE_POINTS:
		return RD::RENDER_PRIMITIVE_POINTS;
	case RSE::PRIMITIVE_LINES:
		return RD::RENDER_PRIMITIVE_LINES;
	case RSE::PRIMITIVE_LINE_STRIP:
		return RD::RENDER_PRIMITIVE_LINESTRIPS;
	case RSE::PRIMITIVE_TRIANGLES:
		return RD::RENDER_PRIMITIVE_TRIANGLES;
	case RSE::PRIMITIVE_TRIANGLE_STRIP:
		return RD::RENDER_PRIMITIVE_TRIANGLE_STRIPS;
	default:
		return RD::RENDER_PRIMITIVE_MAX;
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
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();

	// re create canvas state
	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.binding = 1;
		u.append_id(state.canvas_state_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 2;
		u.append_id(state.lights_storage_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 3;
		u.append_id(RendererRD::TextureStorage::get_singleton()->decal_atlas_get_texture());
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 4;
		u.append_id(state.shadow_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
		u.binding = 5;
		u.append_id(state.shadow_sampler);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 6;
		RID screen;
		if (p_backbuffer) {
			screen = texture_storage->render_target_get_rd_texture(p_to_render_target);
		}
		else {
			screen = texture_storage->render_target_get_rd_backbuffer(p_to_render_target);
			if (screen.is_null()) { // unallocated backbuffer
				screen = RendererRD::TextureStorage::get_singleton()->texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
			}
		}
		u.append_id(screen);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 7;
		RID sdf = texture_storage->render_target_get_sdf_texture(p_to_render_target);
		u.append_id(sdf);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 9;
		u.append_id(RendererRD::MaterialStorage::get_singleton()
						->global_shader_uniforms_get_storage_buffer());
		uniforms.push_back(u);
	}

	material_storage->samplers_rd_get_default().append_uniforms(
		uniforms, SAMPLERS_BINDING_FIRST_INDEX);

	RID uniform_set = RD::get_singleton()->uniform_set_create(
		uniforms, shader.default_version_rd_shader, BASE_UNIFORM_SET);
	if (p_backbuffer) {
		texture_storage->render_target_set_backbuffer_uniform_set(p_to_render_target, uniform_set);
	}
	else {
		texture_storage->render_target_set_framebuffer_uniform_set(p_to_render_target, uniform_set);
	}

	return uniform_set;
}

void RendererCanvasRenderRD::canvas_render_items(RID p_to_render_target, Item* p_item_list,
	const Color& p_modulate, Light* p_light_list, Light* p_directional_light_list,
	const Transform2D& p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter,
	RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
	bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info)
{
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();
	RendererRD::MeshStorage* mesh_storage = RendererRD::MeshStorage::get_singleton();

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

			if (state.shadow_fb.is_valid()) {
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
			if (!RSG::canvas->_interpolation_data.interpolation_enabled || !l->interpolated ||
				!l->on_interpolate_transform_list) {
				final_xform = l->xform_curr;
			}
			else {
				real_t f = Engine::get_singleton()->get_physics_interpolation_fraction();
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

			if (state.shadow_fb.is_valid()) {
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
					RendererRD::TextureStorage::get_singleton()->decal_atlas_get_texture_rect(
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

	bool use_linear_colors = texture_storage->render_target_is_using_hdr(p_to_render_target);

	{
		// update canvas state uniform buffer
		State::Buffer state_buffer;

		Size2i ssize = texture_storage->render_target_get_size(p_to_render_target);

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

		Size2 render_target_size = texture_storage->render_target_get_size(p_to_render_target);
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

		Rect2 sdf_rect = texture_storage->render_target_get_sdf_rect(p_to_render_target);
		Rect2 sdf_tex_rect(sdf_rect.position / canvas_scale, sdf_rect.size / canvas_scale);

		state_buffer.sdf_to_tex[0] = 1.0 / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[1] = 1.0 / sdf_tex_rect.size.height;
		state_buffer.sdf_to_tex[2] = -sdf_tex_rect.position.x / sdf_tex_rect.size.width;
		state_buffer.sdf_to_tex[3] = -sdf_tex_rect.position.y / sdf_tex_rect.size.height;

		// print_line("w: " + itos(ssize.width) + " s: " + rtos(canvas_scale));
		state_buffer.tex_to_sdf = 1.0 / ((canvas_scale.x + canvas_scale.y) * 0.5);
		state_buffer.shadow_pixel_size = 1.0f / (float)(state.shadow_texture_size);

		state_buffer.flags = use_linear_colors ? CANVAS_FLAGS_CONVERT_ATTRIBUTES_TO_LINEAR : 0;

	}

	{ // default filter/repeat
		default_filter = p_default_filter;
		default_repeat = p_default_repeat;
	}

	Item* ci = p_item_list;

	// fill the list until rendering is possible.
	bool material_screen_texture_cached = false;
	bool material_screen_texture_mipmaps_cached = false;

	Rect2 back_buffer_rect;
	bool backbuffer_copy = false;
	bool backbuffer_gen_mipmaps = false;

	Item* canvas_group_owner = nullptr;
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
			CanvasMaterialData* md =
				static_cast<CanvasMaterialData*>(material_storage->material_get_data(
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
			const Item::Command* c = ci->commands;

			while (c) {
				if (c->type == Item::Command::TYPE_MESH) {
					const Item::CommandMesh* cm = static_cast<const Item::CommandMesh*>(c);
					if (cm->mesh_instance.is_valid()) {
						mesh_storage->mesh_instance_check_for_update(cm->mesh_instance);
						mesh_storage->mesh_instance_set_canvas_item_transform(
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
					mesh_storage->update_mesh_instances();
					update_skeletons = false;
				}
				_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
					p_light_list, r_sdf_used, false, r_render_info);
				item_count = 0;

				if (ci->canvas_group_owner->canvas_group->mode !=
					RSE::CANVAS_GROUP_MODE_TRANSPARENT) {
					Rect2i group_rect = ci->canvas_group_owner->global_rect_cache;
					texture_storage->render_target_copy_to_back_buffer(
						p_to_render_target, group_rect, false);
					if (ci->canvas_group_owner->canvas_group->mode ==
						RSE::CANVAS_GROUP_MODE_CLIP_AND_DRAW) {
						ci->canvas_group_owner->use_canvas_group = false;
						items[item_count++] = ci->canvas_group_owner;
					}
				}
				else if (!backbuffer_cleared) {
					texture_storage->render_target_clear_back_buffer(
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
				mesh_storage->update_mesh_instances();
				update_skeletons = false;
			}

			_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
				p_light_list, r_sdf_used, true, r_render_info);
			item_count = 0;

			if (ci->canvas_group->blur_mipmaps) {
				texture_storage->render_target_gen_back_buffer_mipmaps(
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
				mesh_storage->update_mesh_instances();
				update_skeletons = false;
			}

			_render_batch_items(to_render_target, item_count, canvas_transform_inverse,
				p_light_list, r_sdf_used, false, r_render_info);
			item_count = 0;

			texture_storage->render_target_copy_to_back_buffer(
				p_to_render_target, back_buffer_rect, backbuffer_gen_mipmaps);

			backbuffer_copy = false;
			material_screen_texture_cached =
				true; // After a backbuffer copy, screen texture makes no further copies.
			material_screen_texture_mipmaps_cached = backbuffer_gen_mipmaps;
			backbuffer_gen_mipmaps = false;
		}

		if (backbuffer_gen_mipmaps) {
			texture_storage->render_target_gen_back_buffer_mipmaps(
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
				mesh_storage->update_mesh_instances();
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
		RenderingServerDefault::redraw_request();
	}

	texture_info_map.clear();

	// Save the previous instance data pointer in case more items are rendered in the same frame.
	state.prev_instance_data = state.instance_data;
	state.prev_instance_data_index = state.instance_data_index;

	state.instance_data = nullptr;
	if (state.instance_data_index > 0) {
		// If there was any remaining instance data, it must be flushed.
		RID buf = state.instance_buffers._get(0);
		RD::get_singleton()->buffer_flush(buf);
		state.instance_data_index = 0;
	}
}

RID RendererCanvasRenderRD::light_create()
{
	CanvasLight canvas_light;
	return canvas_light_owner.make_rid(canvas_light);
}

void RendererCanvasRenderRD::light_set_texture(RID p_rid, RID p_texture)
{
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();

	CanvasLight* cl = canvas_light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(cl);
	if (cl->texture == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !texture_storage->owns_texture(p_texture));

	if (cl->texture.is_valid()) {
		texture_storage->texture_remove_from_decal_atlas(cl->texture);
	}
	cl->texture = p_texture;

	if (cl->texture.is_valid()) {
		texture_storage->texture_add_to_decal_atlas(cl->texture);
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
	if (state.shadow_fb == RID()) {
		// ah, we lack the shadow texture..
		RD::get_singleton()->free_rid(state.shadow_texture); // erase placeholder

		Vector<RID> fb_textures;

		{ // texture
			RD::TextureFormat tf;
			tf.texture_type = RD::TEXTURE_TYPE_2D;
			tf.width = state.shadow_texture_size;
			tf.height = MAX_LIGHTS_PER_RENDER * 2;
			tf.usage_bits = RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
			tf.format = RD::DATA_FORMAT_R32_SFLOAT;

			state.shadow_texture = RD::get_singleton()->texture_create(tf, RD::TextureView());
			fb_textures.push_back(state.shadow_texture);
		}
		{
			RD::TextureFormat tf;
			tf.texture_type = RD::TEXTURE_TYPE_2D;
			tf.width = state.shadow_texture_size;
			tf.height = MAX_LIGHTS_PER_RENDER * 2;
			tf.usage_bits = RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			tf.format = RD::DATA_FORMAT_D32_SFLOAT;
			tf.is_discardable = true;
			// chunks to write
			state.shadow_depth_texture = RD::get_singleton()->texture_create(tf, RD::TextureView());
			fb_textures.push_back(state.shadow_depth_texture);
		}

		state.shadow_fb = RD::get_singleton()->framebuffer_create(fb_textures);
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
		RD::get_singleton()->free_rid(oc->vertex_array);
		RD::get_singleton()->free_rid(oc->vertex_buffer);
		RD::get_singleton()->free_rid(oc->index_array);
		RD::get_singleton()->free_rid(oc->index_buffer);

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
			oc->vertex_buffer =
				RD::get_singleton()->vertex_buffer_create(lc * 6 * sizeof(float), geometry);

			Vector<RID> buffer;
			buffer.push_back(oc->vertex_buffer);
			oc->vertex_array = RD::get_singleton()->vertex_array_create(
				4 * lc / 2, shadow_render.vertex_format, buffer);
			oc->index_array = RD::get_singleton()->index_array_create(oc->index_buffer, 0, 3 * lc);
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
		RD::get_singleton()->free_rid(oc->sdf_vertex_array);
		RD::get_singleton()->free_rid(oc->sdf_vertex_buffer);
		RD::get_singleton()->free_rid(oc->sdf_index_array);
		RD::get_singleton()->free_rid(oc->sdf_index_buffer);

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
			oc->sdf_vertex_buffer = RD::get_singleton()->vertex_buffer_create(
				p_points.size() * 2 * sizeof(float), float_points.span().reinterpret<uint8_t>());
#else
			oc->sdf_vertex_buffer = RD::get_singleton()->vertex_buffer_create(
				p_points.size() * 2 * sizeof(float), p_points.span().reinterpret<uint8_t>());
#endif
			oc->sdf_index_array = RD::get_singleton()->index_array_create(
				oc->sdf_index_buffer, 0, sdf_indices.size());

			Vector<RID> buffer;
			buffer.push_back(oc->sdf_vertex_buffer);
			oc->sdf_vertex_array = RD::get_singleton()->vertex_array_create(
				p_points.size(), shadow_render.sdf_vertex_format, buffer);
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

void RendererCanvasRenderRD::CanvasShaderData::_clear_vertex_input_mask_cache()
{
	for (uint32_t i = 0; i < VERTEX_INPUT_MASKS_SIZE; i++) {
		vertex_input_masks[i].store(0);
	}
}

void RendererCanvasRenderRD::CanvasShaderData::_create_pipeline(PipelineKey p_pipeline_key)
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
	RD::PipelineColorBlendState blend_state;
	RD::PipelineColorBlendState::Attachment attachment;
	uint32_t dynamic_state_flags = 0;
	if (p_pipeline_key.lcd_blend) {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_CONSTANT_COLOR;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		dynamic_state_flags = RD::DYNAMIC_STATE_BLEND_CONSTANTS;
	}
	else {
		attachment =
			RendererRD::MaterialStorage::ShaderData::blend_mode_to_blend_attachment(blend_mode_rd);
	}

	blend_state.attachments.push_back(attachment);

	RD::PipelineMultisampleState multisample_state;
	multisample_state.sample_count = RD::get_singleton()->framebuffer_format_get_texture_samples(
		p_pipeline_key.framebuffer_format_id, 0);

	// Convert the specialization from the key to pipeline specialization constants.
	Vector<RD::PipelineSpecializationConstant> specialization_constants;
	RD::PipelineSpecializationConstant sc;
	sc.constant_id = 0;
	sc.int_value = p_pipeline_key.shader_specialization.packed_0;
	sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
	specialization_constants.push_back(sc);

	RID shader_rid = get_shader(p_pipeline_key.variant, p_pipeline_key.ubershader);
	ERR_FAIL_COND(shader_rid.is_null());

	RID pipeline = RD::get_singleton()->render_pipeline_create(shader_rid,
		p_pipeline_key.framebuffer_format_id, p_pipeline_key.vertex_format_id,
		p_pipeline_key.render_primitive, RD::PipelineRasterizationState(), multisample_state,
		RD::PipelineDepthStencilState(), blend_state, dynamic_state_flags, 0,
		specialization_constants);
	ERR_FAIL_COND(pipeline.is_null());

	pipeline_hash_map.add_compiled_pipeline(p_pipeline_key.hash(), pipeline);
}

bool RendererCanvasRenderRD::CanvasShaderData::is_animated() const { return false; }

bool RendererCanvasRenderRD::CanvasShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode
RendererCanvasRenderRD::CanvasShaderData::get_native_source_code() const
{
	RendererCanvasRenderRD* canvas_singleton =
		static_cast<RendererCanvasRenderRD*>(RendererCanvasRender::singleton);
	MutexLock lock(canvas_singleton->shader.mutex);
	return canvas_singleton->shader.canvas_shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> RendererCanvasRenderRD::CanvasShaderData::get_native_shader_and_version() const
{
	RendererCanvasRenderRD* canvas_singleton =
		static_cast<RendererCanvasRenderRD*>(RendererCanvasRender::singleton);
	return {&canvas_singleton->shader.canvas_shader, version};
}

uint64_t RendererCanvasRenderRD::CanvasShaderData::get_vertex_input_mask(
	ShaderVariant p_shader_variant, bool p_ubershader)
{
	// Vertex input masks require knowledge of the shader. Since querying the shader can be
	// expensive due to high contention and the necessary mutex, we cache the result instead.
	uint32_t input_mask_index = p_shader_variant + (p_ubershader ? SHADER_VARIANT_MAX : 0);
	uint64_t input_mask = vertex_input_masks[input_mask_index].load(std::memory_order_relaxed);
	if (input_mask == 0) {
		RID shader_rid = get_shader(p_shader_variant, p_ubershader);
		ERR_FAIL_COND_V(shader_rid.is_null(), 0);

		input_mask = RD::get_singleton()->shader_get_vertex_input_attribute_mask(shader_rid);
		vertex_input_masks[input_mask_index].store(input_mask, std::memory_order_relaxed);
	}

	return input_mask;
}

bool RendererCanvasRenderRD::CanvasShaderData::is_valid() const
{
	if (version.is_valid()) {
		RendererCanvasRenderRD* canvas_singleton =
			static_cast<RendererCanvasRenderRD*>(RendererCanvasRender::singleton);
		MutexLock lock(canvas_singleton->shader.mutex);
		return canvas_singleton->shader.canvas_shader.version_is_valid(version);
	}
	else {
		return false;
	}
}

RendererCanvasRenderRD::CanvasShaderData::CanvasShaderData()
{
	RendererCanvasRenderRD* canvas_singleton =
		static_cast<RendererCanvasRenderRD*>(RendererCanvasRender::singleton);
	pipeline_hash_map.set_creation_object_and_function(this, &CanvasShaderData::_create_pipeline);
	pipeline_hash_map.set_compilations(
		&canvas_singleton->shader.pipeline_compilations[0], &canvas_singleton->shader.mutex);
}

RendererRD::MaterialStorage::ShaderData* RendererCanvasRenderRD::_create_shader_func()
{
	CanvasShaderData* shader_data = memnew(CanvasShaderData);
	return shader_data;
}

RendererCanvasRenderRD::CanvasMaterialData::~CanvasMaterialData()
{
	free_parameters_uniform_set(uniform_set);
	free_parameters_uniform_set(uniform_set_srgb);
}

RendererRD::MaterialStorage::MaterialData* RendererCanvasRenderRD::_create_material_func(
	CanvasShaderData* p_shader)
{
	CanvasMaterialData* material_data = memnew(CanvasMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

void RendererCanvasRenderRD::set_time(double p_time) { state.time = p_time; }

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
	if (p_size == state.shadow_texture_size) {
		return;
	}
	state.shadow_texture_size = p_size;
	if (state.shadow_fb.is_valid()) {
		RD::get_singleton()->free_rid(state.shadow_texture);
		RD::get_singleton()->free_rid(state.shadow_depth_texture);
		state.shadow_fb = RID();

		{
			// create a default shadow texture to keep uniform set happy (and that it gets erased
			// when a new one is created)
			RD::TextureFormat tf;
			tf.texture_type = RD::TEXTURE_TYPE_2D;
			tf.width = 4;
			tf.height = 4;
			tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT;
			tf.format = RD::DATA_FORMAT_R32_SFLOAT;

			state.shadow_texture = RD::get_singleton()->texture_create(tf, RD::TextureView());
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
	RendererCanvasRenderRD* canvas_singleton =
		static_cast<RendererCanvasRenderRD*>(RendererCanvasRender::singleton);
	MutexLock lock(canvas_singleton->shader.mutex);
	return shader.pipeline_compilations[p_source];
}

void RendererCanvasRenderRD::_record_item_commands(const Item* p_item, RenderTarget p_render_target,
	const Transform2D& p_base_transform, Item*& r_current_clip, Light* p_lights,
	bool& r_batch_broken, bool& r_sdf_used, Batch*& r_current_batch)
{
	const RSE::CanvasItemTextureFilter texture_filter =
		p_item->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? default_filter
																		  : p_item->texture_filter;
	const RSE::CanvasItemTextureRepeat texture_repeat =
		p_item->texture_repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? default_repeat
																		  : p_item->texture_repeat;

	Transform2D base_transform = p_base_transform;

	InstanceData template_instance;
	memset(&template_instance, 0, sizeof(InstanceData));

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

	const Item::Command* c = p_item->commands;
	while (c) {
		if (skipping && c->type != Item::Command::TYPE_ANIMATION_SLICE) {
			c = c->next;
			continue;
		}

		switch (c->type) {
		case Item::Command::TYPE_RECT: {
			const Item::CommandRect* rect = static_cast<const Item::CommandRect*>(c);

			// 1: If commands are different, start a new batch.
			if (r_current_batch->command_type != Item::Command::TYPE_RECT) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = Item::Command::TYPE_RECT;
				r_current_batch->command = c;
				// default variant
				r_current_batch->shader_variant = SHADER_VARIANT_QUAD;
				r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_TRIANGLES;
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
				r_current_batch->shader_variant = SHADER_VARIANT_QUAD;
				r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_TRIANGLES;
			}

			bool has_msdf = bool(rect->flags & CANVAS_RECT_MSDF);
			TextureState tex_state(
				rect->texture, texture_filter, rect_repeat, has_msdf, use_linear_colors);
			TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
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

			InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);
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

		case Item::Command::TYPE_NINEPATCH: {
			const Item::CommandNinePatch* np = static_cast<const Item::CommandNinePatch*>(c);

			if (r_current_batch->command_type != Item::Command::TYPE_NINEPATCH) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = Item::Command::TYPE_NINEPATCH;
				r_current_batch->command = c;
				r_current_batch->has_blend = false;
				r_current_batch->shader_variant = SHADER_VARIANT_NINEPATCH;
				r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_TRIANGLES;
				r_current_batch->flags = 0;
				r_current_batch->use_msdf = false;
				r_current_batch->use_lcd = false;
			}

			TextureState tex_state(
				np->texture, texture_filter, texture_repeat, false, use_linear_colors);
			TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
				_prepare_batch_texture_info(np->texture, tex_state, tex_info);
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

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

		case Item::Command::TYPE_POLYGON: {
			const Item::CommandPolygon* polygon = static_cast<const Item::CommandPolygon*>(c);

			// Polygon's can't be batched, so always create a new batch
			r_current_batch = _new_batch(r_batch_broken);

			r_current_batch->command_type = Item::Command::TYPE_POLYGON;
			r_current_batch->has_blend = false;
			r_current_batch->command = c;
			r_current_batch->flags = 0;
			r_current_batch->use_msdf = false;
			r_current_batch->use_lcd = false;

			TextureState tex_state(
				polygon->texture, texture_filter, texture_repeat, false, use_linear_colors);
			TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
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
													  ? SHADER_VARIANT_ATTRIBUTES_POINTS
													  : SHADER_VARIANT_ATTRIBUTES;
				r_current_batch->render_primitive =
					_primitive_type_to_render_primitive(polygon->primitive);
			}

			InstanceData* instance_data =
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

		case Item::Command::TYPE_PRIMITIVE: {
			const Item::CommandPrimitive* primitive = static_cast<const Item::CommandPrimitive*>(c);

			if (primitive->point_count != r_current_batch->primitive_points ||
				r_current_batch->command_type != Item::Command::TYPE_PRIMITIVE) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->command_type = Item::Command::TYPE_PRIMITIVE;
				r_current_batch->has_blend = false;
				r_current_batch->command = c;
				r_current_batch->primitive_points = primitive->point_count;
				r_current_batch->flags = 0;

				ERR_CONTINUE(primitive->point_count == 0 || primitive->point_count > 4);

				switch (primitive->point_count) {
				case 1:
					r_current_batch->shader_variant = SHADER_VARIANT_PRIMITIVE_POINTS;
					r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_POINTS;
					break;
				case 2:
					r_current_batch->shader_variant = SHADER_VARIANT_PRIMITIVE;
					r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_LINES;
					break;
				case 3:
				case 4:
					r_current_batch->shader_variant = SHADER_VARIANT_PRIMITIVE;
					r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_TRIANGLES;
					break;
				default:
					// Unknown point count.
					break;
				}
			}

			TextureState tex_state(
				primitive->texture, texture_filter, texture_repeat, false, use_linear_colors);
			TextureInfo* tex_info = texture_info_map.getptr(tex_state);
			if (!tex_info) {
				tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
				_prepare_batch_texture_info(primitive->texture, tex_state, tex_info);
			}

			if (r_current_batch->tex_info != tex_info) {
				r_current_batch = _new_batch(r_batch_broken);
				r_current_batch->tex_info = tex_info;
			}

			InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

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

		case Item::Command::TYPE_MESH:
		case Item::Command::TYPE_MULTIMESH:
		case Item::Command::TYPE_PARTICLES: {
			// Mesh's can't be batched, so always create a new batch
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->command = c;
			r_current_batch->command_type = c->type;
			r_current_batch->has_blend = false;
			r_current_batch->flags = 0;
			r_current_batch->use_msdf = false;
			r_current_batch->use_lcd = false;

			InstanceData* instance_data = nullptr;

			Color modulate(1, 1, 1, 1);
			if (c->type == Item::Command::TYPE_MESH) {
				const Item::CommandMesh* m = static_cast<const Item::CommandMesh*>(c);
				TextureState tex_state(
					m->texture, texture_filter, texture_repeat, false, use_linear_colors);
				TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
					_prepare_batch_texture_info(m->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				r_current_batch->mesh_instance_count = 1;
				_update_transform_2d_to_mat2x3(
					base_transform * draw_transform * m->transform, instance_data->world);
				modulate = m->modulate;
			}
			else if (c->type == Item::Command::TYPE_MULTIMESH) {
				RendererRD::MeshStorage* mesh_storage = RendererRD::MeshStorage::get_singleton();

				const Item::CommandMultiMesh* mm = static_cast<const Item::CommandMultiMesh*>(c);
				RID multimesh = mm->multimesh;

				if (mesh_storage->multimesh_get_transform_format(multimesh) !=
					RSE::MULTIMESH_TRANSFORM_2D) {
					break;
				}

				r_current_batch->mesh_instance_count =
					mesh_storage->multimesh_get_instances_to_draw(multimesh);
				if (r_current_batch->mesh_instance_count == 0) {
					break;
				}

				TextureState tex_state(
					mm->texture, texture_filter, texture_repeat, false, use_linear_colors);
				TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
					_prepare_batch_texture_info(mm->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				r_current_batch->flags |= 1; // multimesh, trails disabled

				if (mesh_storage->multimesh_uses_colors(mm->multimesh)) {
					r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_COLORS;
				}
				if (mesh_storage->multimesh_uses_custom_data(mm->multimesh)) {
					r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;
				}
			}
			else if (c->type == Item::Command::TYPE_PARTICLES) {
				RendererRD::TextureStorage* texture_storage =
					RendererRD::TextureStorage::get_singleton();
				RendererRD::ParticlesStorage* particles_storage =
					RendererRD::ParticlesStorage::get_singleton();

				const Item::CommandParticles* pt = static_cast<const Item::CommandParticles*>(c);
				TextureState tex_state(
					pt->texture, texture_filter, texture_repeat, false, use_linear_colors);
				TextureInfo* tex_info = texture_info_map.getptr(tex_state);
				if (!tex_info) {
					tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
					_prepare_batch_texture_info(pt->texture, tex_state, tex_info);
				}
				r_current_batch->tex_info = tex_info;
				instance_data = new_instance_data(*r_current_batch, template_instance, true);

				uint32_t divisor = 1;
				r_current_batch->mesh_instance_count =
					particles_storage->particles_get_amount(pt->particles, divisor);
				r_current_batch->flags |= (divisor & BATCH_FLAGS_INSTANCING_MASK);
				r_current_batch->mesh_instance_count /= divisor;

				RID particles = pt->particles;

				r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_COLORS;
				r_current_batch->flags |= BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA;

				if (particles_storage->particles_has_collision(particles) &&
					texture_storage->render_target_is_sdf_enabled(p_render_target.render_target)) {
					// Pass collision information.
					Transform2D xform = p_item->final_transform;

					RID sdf_texture = texture_storage->render_target_get_sdf_texture(
						p_render_target.render_target);

					Rect2 to_screen;
					{
						Rect2 sdf_rect = texture_storage->render_target_get_sdf_rect(
							p_render_target.render_target);

						to_screen.size =
							Vector2(1.0 / sdf_rect.size.width, 1.0 / sdf_rect.size.height);
						to_screen.position = -sdf_rect.position * to_screen.size;
					}

					particles_storage->particles_set_canvas_sdf_collision(
						pt->particles, true, xform, to_screen, sdf_texture);
				}
				else {
					particles_storage->particles_set_canvas_sdf_collision(
						pt->particles, false, Transform2D(), Rect2(), RID());
				}
				r_sdf_used |= particles_storage->particles_has_collision(particles);
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

		case Item::Command::TYPE_TRANSFORM: {
			const Item::CommandTransform* transform = static_cast<const Item::CommandTransform*>(c);
			draw_transform = transform->xform;
			_update_transform_2d_to_mat2x3(
				base_transform * transform->xform, template_instance.world);
		} break;

		case Item::Command::TYPE_CLIP_IGNORE: {
			const Item::CommandClipIgnore* ci = static_cast<const Item::CommandClipIgnore*>(c);
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

		case Item::Command::TYPE_ANIMATION_SLICE: {
			const Item::CommandAnimationSlice* as =
				static_cast<const Item::CommandAnimationSlice*>(c);
			double current_time = RSG::rasterizer->get_total_time();
			double local_time = Math::fposmod(current_time - as->offset, as->animation_length);
			skipping = !(local_time >= as->slice_begin && local_time < as->slice_end);

			RenderingServerDefault::redraw_request(); // animation visible means redraw request
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
		if (r_current_batch->command_type != Item::Command::TYPE_RECT) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->command_type = Item::Command::TYPE_RECT;
			// it is ok to be null for a TYPE_RECT
			r_current_batch->command = nullptr;
			// default variant
			r_current_batch->shader_variant = SHADER_VARIANT_QUAD;
			r_current_batch->render_primitive = RD::RENDER_PRIMITIVE_TRIANGLES;
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

		TextureState tex_state(
			default_canvas_texture, texture_filter, texture_repeat, false, use_linear_colors);
		TextureInfo* tex_info = texture_info_map.getptr(tex_state);
		if (!tex_info) {
			tex_info = &texture_info_map.insert(tex_state, TextureInfo())->value;
			_prepare_batch_texture_info(default_canvas_texture, tex_state, tex_info);
		}

		if (r_current_batch->tex_info != tex_info) {
			r_current_batch = _new_batch(r_batch_broken);
			r_current_batch->tex_info = tex_info;
		}

		_update_transform_2d_to_mat2x3(base_transform, template_instance.world);
		InstanceData* instance_data = new_instance_data(*r_current_batch, template_instance);

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

		p_item->debug_redraw_time -= RSG::rasterizer->get_frame_delta_time();

		RenderingServerDefault::redraw_request();

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
	RD::get_singleton()->uniform_set_set_invalidation_callback(p_rid, nullptr, nullptr);
	RD::get_singleton()->free_rid(p_rid);
}

void RendererCanvasRenderRD::_uniform_set_invalidation_callback(void* p_userdata)
{
	const RIDSetKey* key = static_cast<RIDSetKey*>(p_userdata);
	static_cast<RendererCanvasRenderRD*>(singleton)->rid_set_to_uniform_set.erase(*key);
}

void RendererCanvasRenderRD::_canvas_texture_invalidation_callback(bool p_deleted, void* p_userdata)
{
	KeyValue<RID, TightLocalVector<RID>>* kv =
		static_cast<KeyValue<RID, TightLocalVector<RID>>*>(p_userdata);
	RD* rd = RD::get_singleton();
	for (RID rid : kv->value) {
		// The invalidation callback will also take care of clearing rid_set_to_uniform_set cache.
		rd->free_rid(rid);
	}
	kv->value.clear();
	if (p_deleted) {
		static_cast<RendererCanvasRenderRD*>(singleton)->canvas_texture_to_uniform_set.erase(
			kv->key);
	}
}

void RendererCanvasRenderRD::_render_batch(RD::DrawListID p_draw_list,
	CanvasShaderData* p_shader_data, RenderingDevice::FramebufferFormatID p_framebuffer_format,
	Light* p_lights, const Batch* p_batch, RenderingServerTypes::RenderInfo* r_render_info)
{
	{
		RendererRD::TextureStorage* ts = RendererRD::TextureStorage::get_singleton();

		RIDSetKey key(p_batch->tex_info->state);

		const RID* uniform_set = rid_set_to_uniform_set.getptr(key);
		if (uniform_set == nullptr) {
			RD::Uniform* uniform_ptrw = state.batch_texture_uniforms.ptrw();
			uniform_ptrw[0] = RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 0, p_batch->tex_info->diffuse);
			uniform_ptrw[1] = RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 1, p_batch->tex_info->normal);
			uniform_ptrw[2] = RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 2, p_batch->tex_info->specular);
			uniform_ptrw[3] = RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 3, p_batch->tex_info->sampler);

			RID rid = RD::get_singleton()->uniform_set_create(
				state.batch_texture_uniforms, shader.default_version_rd_shader, BATCH_UNIFORM_SET);
			ERR_FAIL_COND_MSG(rid.is_null(), "Failed to create uniform set for batch.");

			const RIDCache::Pair* iter = rid_set_to_uniform_set.insert(key, rid);
			uniform_set = &iter->data;
			RD::get_singleton()->uniform_set_set_invalidation_callback(
				rid, RendererCanvasRenderRD::_uniform_set_invalidation_callback, (void*)&iter->key);

			// If this is a CanvasTexture, it must be tracked so that any changes to the diffuse,
			// normal, or specular channels invalidate all associated uniform sets.
			if (ts->owns_canvas_texture(p_batch->tex_info->state.texture)) {
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
				ts->canvas_texture_set_invalidation_callback(p_batch->tex_info->state.texture,
					RendererCanvasRenderRD::_canvas_texture_invalidation_callback, kv);
			}
		}

		if (state.current_batch_uniform_set != *uniform_set) {
			state.current_batch_uniform_set = *uniform_set;
			RD::get_singleton()->draw_list_bind_uniform_set(
				p_draw_list, *uniform_set, BATCH_UNIFORM_SET);
		}
	}

	RID pipeline;
	PipelineKey pipeline_key;
	pipeline_key.framebuffer_format_id = p_framebuffer_format;
	pipeline_key.variant = p_batch->shader_variant;
	pipeline_key.render_primitive = p_batch->render_primitive;
	pipeline_key.shader_specialization.use_lighting = p_batch->use_lighting;
	pipeline_key.shader_specialization.use_msdf = p_batch->use_msdf;
	pipeline_key.shader_specialization.use_lcd = p_batch->use_lcd;
	pipeline_key.lcd_blend = p_batch->has_blend;

	switch (p_batch->command_type) {
	case Item::Command::TYPE_POLYGON: {
		ERR_FAIL_NULL(p_batch->command);
		PushConstantAttributes push_constant = p_batch->push_constant_attributes();

		const Item::CommandPolygon* polygon =
			static_cast<const Item::CommandPolygon*>(p_batch->command);

		PolygonBuffers* pb = polygon_buffers.polygons.getptr(polygon->polygon.polygon_id);
		ERR_FAIL_NULL(pb);

		pipeline_key.vertex_format_id = pb->vertex_format_id;
		pipeline =
			_get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key, push_constant);
		RD::get_singleton()->draw_list_bind_render_pipeline(p_draw_list, pipeline);

		RD::get_singleton()->draw_list_set_push_constant(
			p_draw_list, &push_constant, sizeof(push_constant));
		RD::get_singleton()->draw_list_bind_vertex_array(p_draw_list, pb->vertex_array);
		if (pb->indices.is_valid()) {
			RD::get_singleton()->draw_list_bind_index_array(p_draw_list, pb->indices);
		}

		RD::get_singleton()->draw_list_draw(p_draw_list, pb->indices.is_valid());
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

	case Item::Command::TYPE_MESH:
	case Item::Command::TYPE_MULTIMESH:
	case Item::Command::TYPE_PARTICLES: {
		ERR_FAIL_NULL(p_batch->command);

		PushConstantAttributes push_constant = p_batch->push_constant_attributes();

		RendererRD::MeshStorage* mesh_storage = RendererRD::MeshStorage::get_singleton();
		RendererRD::ParticlesStorage* particles_storage =
			RendererRD::ParticlesStorage::get_singleton();

		RID mesh;
		RID mesh_instance;

		if (p_batch->command_type == Item::Command::TYPE_MESH) {
			const Item::CommandMesh* m = static_cast<const Item::CommandMesh*>(p_batch->command);
			mesh = m->mesh;
			mesh_instance = m->mesh_instance;
		}
		else if (p_batch->command_type == Item::Command::TYPE_MULTIMESH) {
			const Item::CommandMultiMesh* mm =
				static_cast<const Item::CommandMultiMesh*>(p_batch->command);
			RID multimesh = mm->multimesh;
			mesh = mesh_storage->multimesh_get_mesh(multimesh);

			RID uniform_set = mesh_storage->multimesh_get_2d_uniform_set(
				multimesh, shader.default_version_rd_shader, TRANSFORMS_UNIFORM_SET);
			RD::get_singleton()->draw_list_bind_uniform_set(
				p_draw_list, uniform_set, TRANSFORMS_UNIFORM_SET);
		}
		else if (p_batch->command_type == Item::Command::TYPE_PARTICLES) {
			const Item::CommandParticles* pt =
				static_cast<const Item::CommandParticles*>(p_batch->command);
			RID particles = pt->particles;
			mesh = particles_storage->particles_get_draw_pass_mesh(particles, 0);

			ERR_BREAK(particles_storage->particles_get_mode(particles) != RSE::PARTICLES_MODE_2D);
			particles_storage->particles_request_process(particles);

			if (particles_storage->particles_is_inactive(particles)) {
				break;
			}

			RenderingServerDefault::redraw_request(); // Active particles means redraw request.

			int dpc = particles_storage->particles_get_draw_passes(particles);
			if (dpc == 0) {
				break; // Nothing to draw.
			}

			RID uniform_set = particles_storage->particles_get_instance_buffer_uniform_set(
				pt->particles, shader.default_version_rd_shader, TRANSFORMS_UNIFORM_SET);
			RD::get_singleton()->draw_list_bind_uniform_set(
				p_draw_list, uniform_set, TRANSFORMS_UNIFORM_SET);
		}

		if (mesh.is_null()) {
			break;
		}

		uint32_t surf_count = mesh_storage->mesh_get_surface_count(mesh);

		for (uint32_t j = 0; j < surf_count; j++) {
			void* surface = mesh_storage->mesh_get_surface(mesh, j);

			RSE::PrimitiveType primitive = mesh_storage->mesh_surface_get_primitive(surface);
			ERR_CONTINUE(primitive < 0 || primitive >= RSE::PRIMITIVE_MAX);

			RID vertex_array;
			pipeline_key.variant = primitive == RSE::PRIMITIVE_POINTS
									   ? SHADER_VARIANT_ATTRIBUTES_POINTS
									   : SHADER_VARIANT_ATTRIBUTES;
			pipeline_key.render_primitive = _primitive_type_to_render_primitive(primitive);
			pipeline_key.vertex_format_id = RD::INVALID_FORMAT_ID;

			pipeline = _get_pipeline_specialization_or_ubershader(p_shader_data, pipeline_key,
				push_constant, mesh_instance, surface, j, &vertex_array);
			RD::get_singleton()->draw_list_bind_render_pipeline(p_draw_list, pipeline);

			RD::get_singleton()->draw_list_set_push_constant(
				p_draw_list, &push_constant, sizeof(push_constant));

			RID index_array = mesh_storage->mesh_surface_get_index_array(surface, 0);

			if (index_array.is_valid()) {
				RD::get_singleton()->draw_list_bind_index_array(p_draw_list, index_array);
			}

			RD::get_singleton()->draw_list_bind_vertex_array(p_draw_list, vertex_array);
			RD::get_singleton()->draw_list_draw(
				p_draw_list, index_array.is_valid(), p_batch->mesh_instance_count);

			if (r_render_info) {
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME]++;
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME] +=
					_indices_to_primitives(
						primitive, mesh_storage->mesh_surface_get_vertices_drawn_count(surface)) *
					p_batch->mesh_instance_count;
				r_render_info->info[RSE::VIEWPORT_RENDER_INFO_TYPE_CANVAS]
								   [RSE::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME]++;
			}
		}
	} break;
	case Item::Command::TYPE_TRANSFORM:
	case Item::Command::TYPE_CLIP_IGNORE:
	case Item::Command::TYPE_ANIMATION_SLICE: {
		// Can ignore these as they only impact batch creation.
	} break;
	}
}

RendererCanvasRenderRD::InstanceData* RendererCanvasRenderRD::new_instance_data(
	Batch& p_current_batch, const InstanceData& template_instance, bool p_use_push_data)
{
	InstanceData* instance_data = nullptr;

	if (unlikely(p_use_push_data)) {
		instance_data = &p_current_batch.push_data;
		// instance_count must be > 0 to indicate the batch has been used when calling _new_batch,
		// so we set a flag.
		p_current_batch.instance_count = PUSH_DATA_INSTANCE_COUNT;
	}
	else {
		// Return the intermediary instance data to prevent the caller from accidentally reading
		// write-combined memory pages, which has huge performance implications.
		instance_data = &state.intermediary_instance_data;
	}

	memcpy(instance_data, &template_instance, sizeof(InstanceData));
	return instance_data;
}

RendererCanvasRenderRD::Batch* RendererCanvasRenderRD::_new_batch(bool& r_batch_broken)
{
	if (state.canvas_instance_batches.is_empty()) {
		Batch new_batch;
		// First try to reuse previous instance buffer if possible.
		if (state.prev_instance_data &&
			state.prev_instance_data_index < state.max_instances_per_buffer) {
			bool must_remap = state.instance_buffers.prepare_for_map(true);
			// must_remap will be false if we're preparing to map the buffer for the same frame and
			// can reuse the existing UMA buffer.
			if (!must_remap) {
				state.instance_data = state.prev_instance_data;
				state.instance_data_index = state.prev_instance_data_index;
			}
			state.prev_instance_data = nullptr;
			state.prev_instance_data_index = 0;
		}
		// This will still be a valid point when multiple calls to _render_batch_items
		// are made in the same draw call.
		if (state.instance_data == nullptr) {
			// If there is no existing instance buffer, we must allocate a new one.
			_allocate_instance_buffer();
		}
		else {
			// Otherwise, just use the existing one from where it last left off.
			new_batch.start = state.instance_data_index;
		}
		new_batch.instance_buffer = state.instance_buffers._get(0);
		state.canvas_instance_batches.push_back(new_batch);
		return state.canvas_instance_batches.ptr();
	}

	if (r_batch_broken ||
		state.canvas_instance_batches[state.current_batch_index].instance_count == 0) {
		return &state.canvas_instance_batches[state.current_batch_index];
	}

	r_batch_broken = true;

	// Copy the properties of the current batch, we will manually update the things that changed.
	Batch new_batch = state.canvas_instance_batches[state.current_batch_index];
	new_batch.instance_count = 0;
	new_batch.start = state.instance_data_index;
	memset(&new_batch.push_data, 0, sizeof(new_batch.push_data));
	state.current_batch_index++;
	state.canvas_instance_batches.push_back(new_batch);
	return &state.canvas_instance_batches[state.current_batch_index];
}

void RendererCanvasRenderRD::_add_to_batch(bool& r_batch_broken, Batch*& r_current_batch)
{
	DEV_ASSERT(r_current_batch->command_type == Item::Command::TYPE_RECT ||
			   r_current_batch->command_type == Item::Command::TYPE_NINEPATCH ||
			   r_current_batch->command_type == Item::Command::TYPE_PRIMITIVE);
	r_current_batch->instance_count++;
	memcpy(&state.instance_data[state.instance_data_index], &state.intermediary_instance_data,
		sizeof(InstanceData));
	state.instance_data_index++;
	if (state.instance_data_index >= state.max_instances_per_buffer) {
		RD::get_singleton()->buffer_flush(r_current_batch->instance_buffer);
		state.instance_data = nullptr;
		_allocate_instance_buffer();
		state.instance_data_index = 0;
		r_batch_broken = false; // Force a new batch to be created
		r_current_batch = _new_batch(r_batch_broken);
		r_current_batch->instance_buffer = state.instance_buffers._get(0);
	}
}

void RendererCanvasRenderRD::_allocate_instance_buffer()
{
	state.instance_buffers.prepare_for_upload();
	state.instance_data =
		reinterpret_cast<InstanceData*>(state.instance_buffers.map_raw_for_upload(0));
}

void RendererCanvasRenderRD::_prepare_batch_texture_info(
	RID p_texture, TextureState& p_state, TextureInfo* p_info)
{
	if (p_texture.is_null()) {
		p_texture = default_canvas_texture;
	}

	RendererRD::TextureStorage::CanvasTextureInfo info =
		RendererRD::TextureStorage::get_singleton()->canvas_texture_get_info(p_texture,
			p_state.texture_filter(), p_state.texture_repeat(), p_state.linear_colors(),
			p_state.texture_is_data());
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

RendererCanvasRenderRD::~RendererCanvasRenderRD()
{
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();

	// canvas state

	material_storage->material_free(default_canvas_group_material);
	material_storage->shader_free(default_canvas_group_shader);

	material_storage->material_free(default_clip_children_material);
	material_storage->shader_free(default_clip_children_shader);

	{
		if (state.canvas_state_buffer.is_valid()) {
			RD::get_singleton()->free_rid(state.canvas_state_buffer);
		}

		memdelete_arr(state.light_uniforms);
		RD::get_singleton()->free_rid(state.lights_storage_buffer);
	}

	// shadow rendering
	{
		shadow_render.shader.version_free(shadow_render.shader_version);
		// this will also automatically clear all pipelines
		RD::get_singleton()->free_rid(state.shadow_sampler);
	}

	// buffers
	{
		RD::get_singleton()->free_rid(shader.quad_index_array);
		RD::get_singleton()->free_rid(shader.quad_index_buffer);
		// primitives are erase by dependency
	}

	if (state.shadow_fb.is_valid()) {
		RD::get_singleton()->free_rid(state.shadow_depth_texture);
	}
	RD::get_singleton()->free_rid(state.shadow_texture);

	if (state.shadow_occluder_buffer.is_valid()) {
		RD::get_singleton()->free_rid(state.shadow_occluder_buffer);
	}

	state.instance_buffers.uninit();

	// Disable the callback, as we're tearing everything down
	texture_storage->canvas_texture_set_invalidation_callback(
		default_canvas_texture, nullptr, nullptr);
	texture_storage->canvas_texture_free(default_canvas_texture);
	// pipelines don't need freeing, they are all gone after shaders are gone

	memdelete(shader.default_version_data);
}


