/**************************************************************************/
/*  renderer.cpp                                                          */
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

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "renderer.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_canvas_cull.h"
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/camera_attributes_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/storage/texture_storage.h"
#include "servers/rendering/storage/utilities.h"

RID Renderer::get_test_texture()
{
	if (data->test_texture.is_valid()) {
		return data->test_texture;
	};

#define TEST_TEXTURE_SIZE 256

	Vector<uint8_t> test_data;
	test_data.resize(TEST_TEXTURE_SIZE * TEST_TEXTURE_SIZE * 3);

	{
		uint8_t* w = test_data.ptrw();

		for (int x = 0; x < TEST_TEXTURE_SIZE; x++) {
			for (int y = 0; y < TEST_TEXTURE_SIZE; y++) {
				Color c;
				int r = 255 - (x + y) / 2;

				if ((x % (TEST_TEXTURE_SIZE / 8)) < 2 || (y % (TEST_TEXTURE_SIZE / 8)) < 2) {
					c.r = y;
					c.g = r;
					c.b = x;

				}
				else {
					c.r = r;
					c.g = x;
					c.b = y;
				}

				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 0] = uint8_t(CLAMP(c.r, 0, 255));
				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 1] = uint8_t(CLAMP(c.g, 0, 255));
				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 2] = uint8_t(CLAMP(c.b, 0, 255));
			}
		}
	}

	Ref<Image> img_data =
		memnew(Image(TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, false, Image::FORMAT_RGB8, test_data));

	data->test_texture = texture_2d_create(img_data);

	return data->test_texture;
}

void Renderer::_free_internal_rids()
{
	if (data->test_texture.is_valid()) {
		free_rid(data->test_texture);
	}
	if (data->white_texture.is_valid()) {
		free_rid(data->white_texture);
	}
	if (data->test_material.is_valid()) {
		free_rid(data->test_material);
	}
}

RID Renderer::get_white_texture()
{
	if (data->white_texture.is_valid()) {
		return data->white_texture;
	}

	Vector<uint8_t> wt;
	wt.resize(16 * 3);
	{
		uint8_t* w = wt.ptrw();
		for (int i = 0; i < 16 * 3; i++) {
			w[i] = 255;
		}
	}
	Ref<Image> white = memnew(Image(4, 4, false, Image::FORMAT_RGB8, wt));
	data->white_texture = texture_2d_create(white);
	return data->white_texture;
}

void _get_axis_angle(
	const Vector3& p_normal, const Vector4& p_tangent, float& r_angle, Vector3& r_axis)
{
	Vector3 normal = p_normal.normalized();
	Vector3 tangent = Vector3(p_tangent.x, p_tangent.y, p_tangent.z).normalized();
	float d = p_tangent.w;
	Vector3 binormal = normal.cross(tangent).normalized();
	real_t angle;

	Basis tbn = Basis();
	tbn.rows[0] = tangent;
	tbn.rows[1] = binormal;
	tbn.rows[2] = normal;
	tbn.get_axis_angle(r_axis, angle);
	r_angle = float(angle);

	if (d < 0.0) {
		r_angle = CLAMP((1.0 - r_angle / Math::PI) * 0.5, 0.0, 0.49999);
	}
	else {
		r_angle = CLAMP((r_angle / Math::PI) * 0.5 + 0.5, 0.500008, 1.0);
	}
}

// The inputs to this function should match the outputs of _get_axis_angle. I.e. p_axis is a
// normalized vector and p_angle includes the binormal direction.
void _get_tbn_from_axis_angle(
	const Vector3& p_axis, float p_angle, Vector3& r_normal, Vector4& r_tangent)
{
	float binormal_sign = p_angle > 0.5 ? 1.0 : -1.0;
	float angle = Math::abs(p_angle * 2.0 - 1.0) * Math::PI;

	Basis tbn = Basis(p_axis, angle);
	Vector3 tan = tbn.rows[0];
	r_tangent = Vector4(tan.x, tan.y, tan.z, binormal_sign);
	r_normal = tbn.rows[2];
}

AABB _compute_aabb_from_points(const Vector3* p_data, int p_length)
{
	if (p_length == 0) {
		return AABB();
	}

	Vector3 min = p_data[0];
	Vector3 max = p_data[0];

	for (int i = 1; i < p_length; ++i) {
		min = min.min(p_data[i]);
		max = max.max(p_data[i]);
	}

	return AABB(min, max - min);
}

uint32_t Renderer::mesh_surface_get_format_offset(
	uint32_t p_format, int p_vertex_len, int p_array_index)
{
	ERR_FAIL_INDEX_V(p_array_index, RSE::ARRAY_MAX, 0);
	p_format = uint64_t(p_format) & ~RSE::ARRAY_FORMAT_INDEX;
	uint32_t offsets[RSE::ARRAY_MAX];
	uint32_t vstr;
	uint32_t ntstr;
	uint32_t astr;
	uint32_t sstr;
	mesh_surface_make_offsets_from_format(
		p_format, p_vertex_len, 0, offsets, vstr, ntstr, astr, sstr);
	return offsets[p_array_index];
}

uint32_t Renderer::mesh_surface_get_format_vertex_stride(uint32_t p_format, int p_vertex_len)
{
	p_format = uint64_t(p_format) & ~RSE::ARRAY_FORMAT_INDEX;
	uint32_t offsets[RSE::ARRAY_MAX];
	uint32_t vstr;
	uint32_t ntstr;
	uint32_t astr;
	uint32_t sstr;
	mesh_surface_make_offsets_from_format(
		p_format, p_vertex_len, 0, offsets, vstr, ntstr, astr, sstr);
	return vstr;
}

uint32_t Renderer::mesh_surface_get_format_normal_tangent_stride(
	uint32_t p_format, int p_vertex_len)
{
	p_format = uint64_t(p_format) & ~RSE::ARRAY_FORMAT_INDEX;
	uint32_t offsets[RSE::ARRAY_MAX];
	uint32_t vstr;
	uint32_t ntstr;
	uint32_t astr;
	uint32_t sstr;
	mesh_surface_make_offsets_from_format(
		p_format, p_vertex_len, 0, offsets, vstr, ntstr, astr, sstr);
	return ntstr;
}

uint32_t Renderer::mesh_surface_get_format_attribute_stride(uint32_t p_format, int p_vertex_len)
{
	p_format = uint64_t(p_format) & ~RSE::ARRAY_FORMAT_INDEX;
	uint32_t offsets[RSE::ARRAY_MAX];
	uint32_t vstr;
	uint32_t ntstr;
	uint32_t astr;
	uint32_t sstr;
	mesh_surface_make_offsets_from_format(
		p_format, p_vertex_len, 0, offsets, vstr, ntstr, astr, sstr);
	return astr;
}

uint32_t Renderer::mesh_surface_get_format_skin_stride(uint32_t p_format, int p_vertex_len)
{
	p_format = uint64_t(p_format) & ~RSE::ARRAY_FORMAT_INDEX;
	uint32_t offsets[RSE::ARRAY_MAX];
	uint32_t vstr;
	uint32_t ntstr;
	uint32_t astr;
	uint32_t sstr;
	mesh_surface_make_offsets_from_format(
		p_format, p_vertex_len, 0, offsets, vstr, ntstr, astr, sstr);
	return sstr;
}

uint32_t Renderer::mesh_surface_get_format_index_stride(uint32_t p_format, int p_vertex_len)
{
	if (!(p_format & RSE::ARRAY_FORMAT_INDEX)) {
		return 0;
	}

	// Determine whether using 16 or 32 bits indices.
	if (p_vertex_len <= (1 << 16) && p_vertex_len > 0) {
		return 2;
	}
	else {
		return 4;
	}
}

void Renderer::mesh_surface_make_offsets_from_format(uint64_t p_format, int p_vertex_len,
	int p_index_len, uint32_t* r_offsets, uint32_t& r_vertex_element_size,
	uint32_t& r_normal_element_size, uint32_t& r_attrib_element_size, uint32_t& r_skin_element_size)
{
	r_vertex_element_size = 0;
	r_normal_element_size = 0;
	r_attrib_element_size = 0;
	r_skin_element_size = 0;

	uint32_t* size_accum = nullptr;

	for (int i = 0; i < RSE::ARRAY_MAX; i++) {
		r_offsets[i] = 0; // Reset

		if (i == RSE::ARRAY_VERTEX) {
			size_accum = &r_vertex_element_size;
		}
		else if (i == RSE::ARRAY_NORMAL) {
			size_accum = &r_normal_element_size;
		}
		else if (i == RSE::ARRAY_COLOR) {
			size_accum = &r_attrib_element_size;
		}
		else if (i == RSE::ARRAY_BONES) {
			size_accum = &r_skin_element_size;
		}

		if (!(p_format & (1ULL << i))) { // No array
			continue;
		}

		int elem_size = 0;

		switch (i) {
		case RSE::ARRAY_VERTEX: {
			if (p_format & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
				elem_size = 2;
			}
			else {
				elem_size = (p_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) ? 2 : 3;
			}

			elem_size *= sizeof(float);
		} break;
		case RSE::ARRAY_NORMAL: {
			elem_size = 4;
		} break;
		case RSE::ARRAY_TANGENT: {
			elem_size = (p_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) ? 0 : 4;
		} break;
		case RSE::ARRAY_COLOR: {
			elem_size = 4;
		} break;
		case RSE::ARRAY_TEX_UV: {
			elem_size = (p_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) ? 4 : 8;
		} break;
		case RSE::ARRAY_TEX_UV2: {
			elem_size = (p_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) ? 4 : 8;
		} break;
		case RSE::ARRAY_CUSTOM0:
		case RSE::ARRAY_CUSTOM1:
		case RSE::ARRAY_CUSTOM2:
		case RSE::ARRAY_CUSTOM3: {
			uint64_t format =
				(p_format >> (RSE::ARRAY_FORMAT_CUSTOM_BASE +
								 (RSE::ARRAY_FORMAT_CUSTOM_BITS * (i - RSE::ARRAY_CUSTOM0)))) &
				RSE::ARRAY_FORMAT_CUSTOM_MASK;
			switch (format) {
			case RSE::ARRAY_CUSTOM_RGBA8_UNORM: {
				elem_size = 4;
			} break;
			case RSE::ARRAY_CUSTOM_RGBA8_SNORM: {
				elem_size = 4;
			} break;
			case RSE::ARRAY_CUSTOM_RG_HALF: {
				elem_size = 4;
			} break;
			case RSE::ARRAY_CUSTOM_RGBA_HALF: {
				elem_size = 8;
			} break;
			case RSE::ARRAY_CUSTOM_R_FLOAT: {
				elem_size = 4;
			} break;
			case RSE::ARRAY_CUSTOM_RG_FLOAT: {
				elem_size = 8;
			} break;
			case RSE::ARRAY_CUSTOM_RGB_FLOAT: {
				elem_size = 12;
			} break;
			case RSE::ARRAY_CUSTOM_RGBA_FLOAT: {
				elem_size = 16;
			} break;
			}
		} break;
		case RSE::ARRAY_WEIGHTS: {
			uint32_t bone_count = (p_format & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS) ? 8 : 4;
			elem_size = sizeof(uint16_t) * bone_count;

		} break;
		case RSE::ARRAY_BONES: {
			uint32_t bone_count = (p_format & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS) ? 8 : 4;
			elem_size = sizeof(uint16_t) * bone_count;
		} break;
		case RSE::ARRAY_INDEX: {
			if (p_index_len <= 0) {
				ERR_PRINT("index_array_len==NO_INDEX_ARRAY");
				break;
			}
			/* determine whether using 16 or 32 bits indices */
			if (p_vertex_len <= (1 << 16) && p_vertex_len > 0) {
				elem_size = 2;
			}
			else {
				elem_size = 4;
			}
			r_offsets[i] = elem_size;
			continue;
		}
		default: {
			ERR_FAIL();
		}
		}

		if (size_accum != nullptr) {
			r_offsets[i] = (*size_accum);
			if (i == RSE::ARRAY_NORMAL || i == RSE::ARRAY_TANGENT) {
				r_offsets[i] += r_vertex_element_size * p_vertex_len;
			}
			(*size_accum) += elem_size;
		}
		else {
			r_offsets[i] = 0;
		}
	}
}

Rect2 Renderer::debug_canvas_item_get_rect(RID p_item)
{
#ifdef TOOLS_ENABLED
	return _debug_canvas_item_get_rect(p_item);
#else
	return Rect2();
#endif
}

int Renderer::global_shader_uniform_type_get_shader_datatype(RSE::GlobalShaderParameterType p_type)
{
	switch (p_type) {
	case RSE::GLOBAL_VAR_TYPE_BOOL:
		return ShaderLanguage::TYPE_BOOL;
	case RSE::GLOBAL_VAR_TYPE_BVEC2:
		return ShaderLanguage::TYPE_BVEC2;
	case RSE::GLOBAL_VAR_TYPE_BVEC3:
		return ShaderLanguage::TYPE_BVEC3;
	case RSE::GLOBAL_VAR_TYPE_BVEC4:
		return ShaderLanguage::TYPE_BVEC4;
	case RSE::GLOBAL_VAR_TYPE_INT:
		return ShaderLanguage::TYPE_INT;
	case RSE::GLOBAL_VAR_TYPE_IVEC2:
		return ShaderLanguage::TYPE_IVEC2;
	case RSE::GLOBAL_VAR_TYPE_IVEC3:
		return ShaderLanguage::TYPE_IVEC3;
	case RSE::GLOBAL_VAR_TYPE_IVEC4:
		return ShaderLanguage::TYPE_IVEC4;
	case RSE::GLOBAL_VAR_TYPE_RECT2I:
		return ShaderLanguage::TYPE_IVEC4;
	case RSE::GLOBAL_VAR_TYPE_UINT:
		return ShaderLanguage::TYPE_UINT;
	case RSE::GLOBAL_VAR_TYPE_UVEC2:
		return ShaderLanguage::TYPE_UVEC2;
	case RSE::GLOBAL_VAR_TYPE_UVEC3:
		return ShaderLanguage::TYPE_UVEC3;
	case RSE::GLOBAL_VAR_TYPE_UVEC4:
		return ShaderLanguage::TYPE_UVEC4;
	case RSE::GLOBAL_VAR_TYPE_FLOAT:
		return ShaderLanguage::TYPE_FLOAT;
	case RSE::GLOBAL_VAR_TYPE_VEC2:
		return ShaderLanguage::TYPE_VEC2;
	case RSE::GLOBAL_VAR_TYPE_VEC3:
		return ShaderLanguage::TYPE_VEC3;
	case RSE::GLOBAL_VAR_TYPE_VEC4:
		return ShaderLanguage::TYPE_VEC4;
	case RSE::GLOBAL_VAR_TYPE_COLOR:
		return ShaderLanguage::TYPE_VEC4;
	case RSE::GLOBAL_VAR_TYPE_RECT2:
		return ShaderLanguage::TYPE_VEC4;
	case RSE::GLOBAL_VAR_TYPE_MAT2:
		return ShaderLanguage::TYPE_MAT2;
	case RSE::GLOBAL_VAR_TYPE_MAT3:
		return ShaderLanguage::TYPE_MAT3;
	case RSE::GLOBAL_VAR_TYPE_MAT4:
		return ShaderLanguage::TYPE_MAT4;
	case RSE::GLOBAL_VAR_TYPE_TRANSFORM_2D:
		return ShaderLanguage::TYPE_MAT3;
	case RSE::GLOBAL_VAR_TYPE_TRANSFORM:
		return ShaderLanguage::TYPE_MAT4;
	case RSE::GLOBAL_VAR_TYPE_SAMPLER2D:
		return ShaderLanguage::TYPE_SAMPLER2D;
	case RSE::GLOBAL_VAR_TYPE_SAMPLER2DARRAY:
		return ShaderLanguage::TYPE_SAMPLER2DARRAY;
	case RSE::GLOBAL_VAR_TYPE_SAMPLER3D:
		return ShaderLanguage::TYPE_SAMPLER3D;
	case RSE::GLOBAL_VAR_TYPE_SAMPLERCUBE:
		return ShaderLanguage::TYPE_SAMPLERCUBE;
	case RSE::GLOBAL_VAR_TYPE_SAMPLEREXT:
		return ShaderLanguage::TYPE_SAMPLEREXT;
	default:
		return ShaderLanguage::TYPE_MAX; // Invalid or not found.
	}
}

String Renderer::get_current_rendering_driver_name()
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here.
	return ::OS::get_current_rendering_driver_name();
}

String Renderer::get_current_rendering_method()
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here.
	return ::OS::get_current_rendering_method();
}

Vector<uint8_t> _convert_surface_version_1_to_surface_version_2(uint64_t p_format,
	Vector<uint8_t> p_vertex_data, uint32_t p_vertex_count, uint32_t p_old_stride,
	uint32_t p_vertex_size, uint32_t p_normal_size, uint32_t p_position_stride,
	uint32_t p_normal_tangent_stride)
{
	Vector<uint8_t> new_vertex_data;
	new_vertex_data.resize(p_vertex_data.size());
	uint8_t* dst_vertex_ptr = new_vertex_data.ptrw();

	const uint8_t* src_vertex_ptr = p_vertex_data.ptr();

	uint32_t position_size = p_position_stride * p_vertex_count;

	for (uint32_t j = 0; j < RSE::ARRAY_COLOR; j++) {
		if (!(p_format & (1ULL << j))) {
			continue;
		}
		switch (j) {
		case RSE::ARRAY_VERTEX: {
			if (p_format & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
				for (uint32_t i = 0; i < p_vertex_count; i++) {
					const float* src = (const float*)&src_vertex_ptr[i * p_old_stride];
					float* dst = (float*)&dst_vertex_ptr[i * p_position_stride];
					dst[0] = src[0];
					dst[1] = src[1];
				}
			}
			else {
				for (uint32_t i = 0; i < p_vertex_count; i++) {
					const float* src = (const float*)&src_vertex_ptr[i * p_old_stride];
					float* dst = (float*)&dst_vertex_ptr[i * p_position_stride];
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
				}
			}
		} break;
		case RSE::ARRAY_NORMAL: {
			for (uint32_t i = 0; i < p_vertex_count; i++) {
				const uint16_t* src =
					(const uint16_t*)&src_vertex_ptr[i * p_old_stride + p_vertex_size];
				uint16_t* dst =
					(uint16_t*)&dst_vertex_ptr[i * p_normal_tangent_stride + position_size];

				dst[0] = src[0];
				dst[1] = src[1];
			}
		} break;
		case RSE::ARRAY_TANGENT: {
			for (uint32_t i = 0; i < p_vertex_count; i++) {
				const uint16_t* src =
					(const uint16_t*)&src_vertex_ptr[i * p_old_stride + p_vertex_size +
													 p_normal_size];
				uint16_t* dst = (uint16_t*)&dst_vertex_ptr[i * p_normal_tangent_stride +
														   position_size + p_normal_size];

				dst[0] = src[0];
				dst[1] = src[1];
			}
		} break;
		}
	}
	return new_vertex_data;
}

void Renderer::mesh_add_surface_from_planes(RID p_mesh, const Vector<Plane>& p_planes)
{
	Geometry3D::MeshData mdata = Geometry3D::build_convex_mesh(p_planes);
	mesh_add_surface_from_mesh_data(p_mesh, mdata);
}

#ifndef DISABLE_DEPRECATED
void Renderer::set_boot_image(
	const Ref<Image>& p_image, const Color& p_color, bool p_scale, bool p_use_filter)
{
	RSE::SplashStretchMode stretch_mode =
		p_scale ? RSE::SPLASH_STRETCH_MODE_KEEP : RSE::SPLASH_STRETCH_MODE_DISABLED;
	set_boot_image_with_stretch(p_image, p_color, stretch_mode, p_use_filter);
}
#endif

RID Renderer::instance_create2(RID p_base, RID p_scenario)
{
	RID instance = instance_create();
	instance_set_base(instance, p_base);
	instance_set_scenario(instance, p_scenario);
	return instance;
}

bool Renderer::is_render_loop_enabled() { return data->render_loop_enabled; }

void Renderer::set_render_loop_enabled(bool p_enabled) { data->render_loop_enabled = p_enabled; }

void Renderer::fix_surface_compatibility(
	RenderingServerTypes::SurfaceData& p_surface, const String& p_path)
{
}

RID Renderer::_make_test_cube() { return RID(); }

void Renderer::mesh_add_surface_from_mesh_data(RID p_mesh, const Geometry3D::MeshData& p_mesh_data)
{
}

RID Renderer::make_sphere_mesh(int p_lats, int p_lons, real_t p_radius) { return RID(); }

void Renderer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options)
{
}

void Renderer::initialize()
{
	data = memnew(Data);
	RendererCameraAttributes::initialize();
	RendererCompositorStorage::initialize();
	RendererEnvironmentStorage::initialize();
	RendererCompositor::create();

	viewport = memnew(RendererViewport);
	canvas = memnew(RendererCanvasCull);
	RendererSceneCull::initialize();
	RendererCompositor::initialize();
}

void Renderer::finalize()
{
	RendererCompositor::finalize();
	memdelete(canvas);
	canvas = nullptr;
	RendererSceneCull::finalize();
	memdelete(viewport);
	viewport = nullptr;
	particles_storage = nullptr;
	fog = nullptr;
	canvas_render = nullptr;
	RendererEnvironmentStorage::finalize();
	RendererCompositorStorage::finalize();
	RendererCameraAttributes::finalize();
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void Renderer::sync() {}

void Renderer::tick()
{
	if (Renderer::canvas) {
		Renderer::canvas->tick();
	}
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::tick();
	}
}

void Renderer::pre_draw(bool p_will_draw)
{
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::pre_draw(p_will_draw);
	}
}

/* STATE & QUERIES */

double Renderer::get_frame_setup_time_cpu() { return data ? data->backend.frame_setup_time : 0.0; }

bool Renderer::has_changed() { return data && data->backend.changes > 0; }

Color Renderer::get_default_clear_color()
{
	return RendererTextureStorage::get_default_clear_color();
}

void Renderer::set_default_clear_color(const Color& p_color)
{
	RendererTextureStorage::set_default_clear_color(p_color);
}

uint64_t Renderer::get_rendering_info(RSE::RenderingInfo p_info)
{
	if (!Renderer::viewport || !Renderer::canvas_render || !RendererSceneCull::is_initialized()) {
		return 0;
	}

	if (p_info == RSE::RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME) {
		return Renderer::viewport->get_total_objects_drawn();
	}
	else if (p_info == RSE::RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME) {
		return Renderer::viewport->get_total_primitives_drawn();
	}
	else if (p_info == RSE::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME) {
		return Renderer::viewport->get_total_draw_calls_used();
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_CANVAS) {
		return Renderer::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_CANVAS);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_MESH) {
		return Renderer::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_MESH) +
			   RendererSceneCull::get_pipeline_compilations(RSE::PIPELINE_SOURCE_MESH);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_SURFACE) {
		return RendererSceneCull::get_pipeline_compilations(RSE::PIPELINE_SOURCE_SURFACE);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_DRAW) {
		return Renderer::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_DRAW) +
			   RendererSceneCull::get_pipeline_compilations(RSE::PIPELINE_SOURCE_DRAW);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_SPECIALIZATION) {
		return Renderer::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_SPECIALIZATION) +
			   RendererSceneCull::get_pipeline_compilations(RSE::PIPELINE_SOURCE_SPECIALIZATION);
	}
	return RendererUtilities::get_rendering_info(p_info);
}

RenderingDeviceEnums::DeviceType Renderer::get_video_adapter_type()
{
	return RendererUtilities::get_video_adapter_type();
}

void Renderer::set_frame_profiling_enabled(bool p_enable)
{
	RendererUtilities::capturing_timestamps = p_enable;
}

uint64_t Renderer::get_frame_profile_frame()
{
	return data ? data->backend.frame_profile_frame : 0;
}

Vector<RenderingServerTypes::FrameProfileArea> Renderer::get_frame_profile()
{
	return data ? data->backend.frame_profile : Vector<RenderingServerTypes::FrameProfileArea>();
}

void Renderer::sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir)
{
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::sdfgi_set_debug_probe_select(p_position, p_dir);
	}
}

void Renderer::set_print_gpu_profile(bool p_enable)
{
	RendererUtilities::capturing_timestamps = p_enable;
	if (data) {
		data->backend.print_gpu_profile = p_enable;
	}
}

RID Renderer::get_test_cube()
{
	if (!data) {
		return RID();
	}
	if (!data->backend.test_cube.is_valid()) {
		data->backend.test_cube = _make_test_cube();
	}
	return data->backend.test_cube;
}

bool Renderer::has_os_feature(const String& p_feature)
{
	return RendererUtilities::has_os_feature(p_feature);
}

void Renderer::set_debug_generate_wireframes(bool p_generate)
{
	RendererUtilities::set_debug_generate_wireframes(p_generate);
}

bool Renderer::is_low_end() { return RendererCompositor::is_low_end(); }

Size2i Renderer::get_maximum_viewport_size()
{
	return RendererUtilities::get_maximum_viewport_size();
}

void Renderer::set_physics_interpolation_enabled(bool p_enabled)
{
	if (Renderer::canvas) {
		Renderer::canvas->set_physics_interpolation_enabled(p_enabled);
	}
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::set_physics_interpolation_enabled(p_enabled);
	}
}

bool Renderer::is_on_render_thread() { return true; }

void Renderer::global_shader_parameters_clear()
{
	RendererMaterialStorage::global_shader_parameters_clear();
}

void Renderer::global_shader_parameters_load_settings(bool p_load_textures)
{
	RendererMaterialStorage::global_shader_parameters_load_settings(p_load_textures);
}

RID Renderer::texture_2d_create(const Ref<Image>& p_image)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_2d_initialize(ret, p_image);
	return ret;
}

RID Renderer::texture_2d_layered_create(
	const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_2d_layered_initialize(ret, p_layers, p_layered_type);
	return ret;
}

RID Renderer::texture_3d_create(Image::Format p_format, int p_width, int p_height, int p_depth,
	bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_3d_initialize(
		ret, p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	return ret;
}

RID Renderer::texture_2d_placeholder_create()
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_2d_placeholder_initialize(ret);
	return ret;
}

RID Renderer::texture_2d_layered_placeholder_create(RSE::TextureLayeredType p_layered_type)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_2d_layered_placeholder_initialize(ret, p_layered_type);
	return ret;
}

RID Renderer::texture_3d_placeholder_create()
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_3d_placeholder_initialize(ret);
	return ret;
}

RID Renderer::texture_proxy_create(RID p_base)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_proxy_initialize(ret, p_base);
	return ret;
}

RID Renderer::texture_drawable_create(int p_width, int p_height,
	RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_with_mipmaps)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_drawable_initialize(
		ret, p_width, p_height, p_format, p_color, p_with_mipmaps);
	return ret;
}

void Renderer::texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
{
	RendererTextureStorage::texture_2d_update(p_texture, p_image, p_layer);
}

void Renderer::texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
{
	RendererTextureStorage::texture_3d_update(p_texture, p_data);
}

void Renderer::texture_proxy_update(RID p_proxy, RID p_base)
{
	RendererTextureStorage::texture_proxy_update(p_proxy, p_base);
}

Ref<Image> Renderer::texture_2d_get(RID p_texture)
{
	return RendererTextureStorage::texture_2d_get(p_texture);
}

Ref<Image> Renderer::texture_2d_layer_get(RID p_texture, int p_layer)
{
	return RendererTextureStorage::texture_2d_layer_get(p_texture, p_layer);
}

Vector<Ref<Image>> Renderer::texture_3d_get(RID p_texture)
{
	return RendererTextureStorage::texture_3d_get(p_texture);
}

void Renderer::texture_replace(RID p_texture, RID p_by_texture)
{
	RendererTextureStorage::texture_replace(p_texture, p_by_texture);
}

void Renderer::texture_set_size_override(RID p_texture, int p_width, int p_height)
{
	RendererTextureStorage::texture_set_size_override(p_texture, p_width, p_height);
}

void Renderer::texture_set_path(RID p_texture, const String& p_path)
{
	RendererTextureStorage::texture_set_path(p_texture, p_path);
}

RID Renderer::texture_drawable_get_default_material()
{
	return RendererTextureStorage::texture_drawable_get_default_material();
}

void Renderer::texture_drawable_generate_mipmaps(RID p_texture)
{
	RendererTextureStorage::texture_drawable_generate_mipmaps(p_texture);
}

uint64_t Renderer::texture_get_native_handle(RID p_texture, bool p_srgb)
{
	return RendererTextureStorage::texture_get_native_handle(p_texture, p_srgb);
}

void Renderer::canvas_set_modulate(RID p_canvas, const Color& p_color)
{
	Renderer::canvas->canvas_set_modulate(p_canvas, p_color);
}

void Renderer::canvas_set_parent(RID p_canvas, RID p_parent, float p_scale)
{
	Renderer::canvas->canvas_set_parent(p_canvas, p_parent, p_scale);
}

void Renderer::canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times)
{
	Renderer::canvas->canvas_set_item_repeat(p_item, p_repeat_size, p_repeat_times);
}

void Renderer::canvas_texture_set_channel(
	RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
{
	Renderer::canvas->canvas_texture_set_channel(p_canvas_texture, p_channel, p_texture);
}

void Renderer::canvas_texture_set_shading_parameters(
	RID p_texture, const Color& p_color, float p_shininess)
{
	Renderer::canvas->canvas_texture_set_shading_parameters(p_texture, p_color, p_shininess);
}

void Renderer::canvas_texture_set_texture_filter(
	RID p_texture, RSE::CanvasItemTextureFilter p_filter)
{
	Renderer::canvas->canvas_texture_set_texture_filter(p_texture, p_filter);
}

void Renderer::canvas_texture_set_texture_repeat(
	RID p_texture, RSE::CanvasItemTextureRepeat p_repeat)
{
	Renderer::canvas->canvas_texture_set_texture_repeat(p_texture, p_repeat);
}

void Renderer::canvas_item_set_parent(RID p_item, RID p_parent)
{
	Renderer::canvas->canvas_item_set_parent(p_item, p_parent);
}

void Renderer::canvas_item_set_visible(RID p_item, bool p_visible)
{
	Renderer::canvas->canvas_item_set_visible(p_item, p_visible);
}

void Renderer::canvas_item_set_transform(RID p_item, const Transform2D& p_transform)
{
	Renderer::canvas->canvas_item_set_transform(p_item, p_transform);
}

void Renderer::canvas_item_set_clip(RID p_item, bool p_clip)
{
	Renderer::canvas->canvas_item_set_clip(p_item, p_clip);
}

void Renderer::canvas_item_set_custom_rect(RID p_item, bool p_custom_rect, const Rect2& p_rect)
{
	Renderer::canvas->canvas_item_set_custom_rect(p_item, p_custom_rect, p_rect);
}

void Renderer::canvas_item_set_modulate(RID p_item, const Color& p_color)
{
	Renderer::canvas->canvas_item_set_modulate(p_item, p_color);
}

void Renderer::canvas_item_set_self_modulate(RID p_item, const Color& p_color)
{
	Renderer::canvas->canvas_item_set_self_modulate(p_item, p_color);
}

void Renderer::canvas_item_set_draw_behind_parent(RID p_item, bool p_enable)
{
	Renderer::canvas->canvas_item_set_draw_behind_parent(p_item, p_enable);
}

void Renderer::canvas_item_set_use_identity_transform(RID p_item, bool p_enable)
{
	Renderer::canvas->canvas_item_set_use_identity_transform(p_item, p_enable);
}

void Renderer::canvas_item_set_sort_children_by_y(RID p_item, bool p_enable)
{
	Renderer::canvas->canvas_item_set_sort_children_by_y(p_item, p_enable);
}

void Renderer::canvas_item_set_z_index(RID p_item, int p_z)
{
	Renderer::canvas->canvas_item_set_z_index(p_item, p_z);
}

void Renderer::canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable)
{
	Renderer::canvas->canvas_item_set_z_as_relative_to_parent(p_item, p_enable);
}

void Renderer::canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect)
{
	Renderer::canvas->canvas_item_set_copy_to_backbuffer(p_item, p_enable, p_rect);
}

void Renderer::canvas_item_clear(RID p_item) { Renderer::canvas->canvas_item_clear(p_item); }

void Renderer::canvas_item_set_draw_index(RID p_item, int p_index)
{
	Renderer::canvas->canvas_item_set_draw_index(p_item, p_index);
}

void Renderer::canvas_item_set_material(RID p_item, RID p_material)
{
	Renderer::canvas->canvas_item_set_material(p_item, p_material);
}

void Renderer::canvas_item_set_use_parent_material(RID p_item, bool p_enable)
{
	Renderer::canvas->canvas_item_set_use_parent_material(p_item, p_enable);
}

void Renderer::canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
	float p_clear_margin, bool p_fit_empty, float p_fit_margin, bool p_blur_mipmaps)
{
	Renderer::canvas->canvas_item_set_canvas_group_mode(
		p_item, p_mode, p_clear_margin, p_fit_empty, p_fit_margin, p_blur_mipmaps);
}

void Renderer::canvas_item_set_interpolated(RID p_item, bool p_interpolated)
{
	Renderer::canvas->canvas_item_set_interpolated(p_item, p_interpolated);
}

void Renderer::canvas_item_reset_physics_interpolation(RID p_item)
{
	Renderer::canvas->canvas_item_reset_physics_interpolation(p_item);
}

void Renderer::canvas_item_set_light_mask(RID p_item, int p_mask)
{
	Renderer::canvas->canvas_item_set_light_mask(p_item, p_mask);
}

void Renderer::canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer)
{
	Renderer::canvas->canvas_item_set_visibility_layer(p_item, p_visibility_layer);
}

void Renderer::canvas_item_set_default_texture_filter(
	RID p_item, RSE::CanvasItemTextureFilter p_filter)
{
	Renderer::canvas->canvas_item_set_default_texture_filter(p_item, p_filter);
}

void Renderer::canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
	const Color& p_color, float p_width, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_line(p_item, p_from, p_to, p_color, p_width, p_antialiased);
}

void Renderer::canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_polyline(p_item, p_points, p_colors, p_width, p_antialiased);
}

void Renderer::canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_multiline(p_item, p_points, p_colors, p_width, p_antialiased);
}

void Renderer::canvas_item_add_rect(
	RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_rect(p_item, p_rect, p_color, p_antialiased);
}

void Renderer::canvas_item_add_circle(
	RID p_item, const Point2& p_pos, float p_radius, const Color& p_color, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_circle(p_item, p_pos, p_radius, p_color, p_antialiased);
}

void Renderer::canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
	float p_minor, const Color& p_color, bool p_antialiased)
{
	Renderer::canvas->canvas_item_add_ellipse(p_item, p_pos, p_major, p_minor, p_color, p_antialiased);
}

void Renderer::canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
	bool p_tile, const Color& p_modulate, bool p_transpose)
{
	Renderer::canvas->canvas_item_add_texture_rect(
		p_item, p_rect, p_texture, p_tile, p_modulate, p_transpose);
}

void Renderer::canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv)
{
	Renderer::canvas->canvas_item_add_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void Renderer::canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate, int p_outline_size,
	float p_px_range, float p_scale)
{
	Renderer::canvas->canvas_item_add_msdf_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
}

void Renderer::canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate)
{
	Renderer::canvas->canvas_item_add_lcd_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate);
}

void Renderer::canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
	RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
	RSE::NinePatchAxisMode p_x_axis_mode, RSE::NinePatchAxisMode p_y_axis_mode, bool p_draw_center,
	const Color& p_modulate)
{
	Renderer::canvas->canvas_item_add_nine_patch(p_item, p_rect, p_source, p_texture, p_topleft,
		p_bottomright, p_x_axis_mode, p_y_axis_mode, p_draw_center, p_modulate);
}

void Renderer::canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	Renderer::canvas->canvas_item_add_primitive(p_item, p_points, p_colors, p_uvs, p_texture);
}

void Renderer::canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	Renderer::canvas->canvas_item_add_polygon(p_item, p_points, p_colors, p_uvs, p_texture);
}

void Renderer::canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones, const Vector<float>& p_weights, RID p_texture, int p_count)
{
	Renderer::canvas->canvas_item_add_triangle_array(
		p_item, p_indices, p_points, p_colors, p_uvs, p_bones, p_weights, p_texture, p_count);
}

void Renderer::canvas_item_add_mesh(RID p_item, const RID& p_mesh, const Transform2D& p_transform,
	const Color& p_modulate, RID p_texture)
{
	Renderer::canvas->canvas_item_add_mesh(p_item, p_mesh, p_transform, p_modulate, p_texture);
}

void Renderer::canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture)
{
	Renderer::canvas->canvas_item_add_multimesh(p_item, p_mesh, p_texture);
}

void Renderer::canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform)
{
	Renderer::canvas->canvas_item_add_set_transform(p_item, p_transform);
}

void Renderer::canvas_item_add_animation_slice(RID p_item, double p_animation_length,
	double p_slice_begin, double p_slice_end, double p_offset)
{
	Renderer::canvas->canvas_item_add_animation_slice(
		p_item, p_animation_length, p_slice_begin, p_slice_end, p_offset);
}

void Renderer::canvas_item_attach_skeleton(RID p_item, RID p_skeleton)
{
	Renderer::canvas->canvas_item_attach_skeleton(p_item, p_skeleton);
}

void Renderer::viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count)
{
	Renderer::viewport->viewport_set_size(p_viewport, p_width, p_height, p_view_count);
}

void Renderer::viewport_set_active(RID p_viewport, bool p_active)
{
	Renderer::viewport->viewport_set_active(p_viewport, p_active);
}

void Renderer::viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport)
{
	Renderer::viewport->viewport_set_parent_viewport(p_viewport, p_parent_viewport);
}

void Renderer::viewport_attach_to_screen(
	RID p_viewport, const Rect2& p_rect, DisplayServerEnums::WindowID p_screen)
{
	Renderer::viewport->viewport_attach_to_screen(p_viewport, p_rect, p_screen);
}

void Renderer::viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable)
{
	Renderer::viewport->viewport_set_render_direct_to_screen(p_viewport, p_enable);
}

void Renderer::viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode)
{
	Renderer::viewport->viewport_set_update_mode(p_viewport, p_mode);
}

RSE::ViewportUpdateMode Renderer::viewport_get_update_mode(RID p_viewport)
{
	return Renderer::viewport->viewport_get_update_mode(p_viewport);
}

void Renderer::viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode)
{
	Renderer::viewport->viewport_set_clear_mode(p_viewport, p_clear_mode);
}

RID Renderer::viewport_get_render_target(RID p_viewport)
{
	return Renderer::viewport->viewport_get_render_target(p_viewport);
}

RID Renderer::viewport_get_texture(RID p_viewport)
{
	return Renderer::viewport->viewport_get_texture(p_viewport);
}

void Renderer::viewport_set_disable_3d(RID p_viewport, bool p_disable)
{
	Renderer::viewport->viewport_set_disable_3d(p_viewport, p_disable);
}

void Renderer::viewport_set_disable_2d(RID p_viewport, bool p_disable)
{
	Renderer::viewport->viewport_set_disable_2d(p_viewport, p_disable);
}

void Renderer::viewport_set_scenario(RID p_viewport, RID p_scenario)
{
	Renderer::viewport->viewport_set_scenario(p_viewport, p_scenario);
}

void Renderer::viewport_attach_canvas(RID p_viewport, RID p_canvas)
{
	Renderer::viewport->viewport_attach_canvas(p_viewport, p_canvas);
}

void Renderer::viewport_remove_canvas(RID p_viewport, RID p_canvas)
{
	Renderer::viewport->viewport_remove_canvas(p_viewport, p_canvas);
}

void Renderer::viewport_set_canvas_transform(
	RID p_viewport, RID p_canvas, const Transform2D& p_offset)
{
	Renderer::viewport->viewport_set_canvas_transform(p_viewport, p_canvas, p_offset);
}

void Renderer::viewport_set_transparent_background(RID p_viewport, bool p_enabled)
{
	Renderer::viewport->viewport_set_transparent_background(p_viewport, p_enabled);
}

void Renderer::viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr)
{
	Renderer::viewport->viewport_set_use_hdr_2d(p_viewport, p_use_hdr);
}

bool Renderer::viewport_is_using_hdr_2d(RID p_viewport)
{
	return Renderer::viewport->viewport_is_using_hdr_2d(p_viewport);
}

void Renderer::viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled)
{
	Renderer::viewport->viewport_set_snap_2d_transforms_to_pixel(p_viewport, p_enabled);
}

void Renderer::viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled)
{
	Renderer::viewport->viewport_set_snap_2d_vertices_to_pixel(p_viewport, p_enabled);
}

void Renderer::viewport_set_global_canvas_transform(RID p_viewport, const Transform2D& p_transform)
{
	Renderer::viewport->viewport_set_global_canvas_transform(p_viewport, p_transform);
}

void Renderer::viewport_set_canvas_stacking(
	RID p_viewport, RID p_canvas, int p_layer, int p_sublayer)
{
	Renderer::viewport->viewport_set_canvas_stacking(p_viewport, p_canvas, p_layer, p_sublayer);
}

void Renderer::viewport_set_sdf_oversize_and_scale(
	RID p_viewport, RSE::ViewportSDFOversize p_oversize, RSE::ViewportSDFScale p_scale)
{
	Renderer::viewport->viewport_set_sdf_oversize_and_scale(p_viewport, p_oversize, p_scale);
}

void Renderer::viewport_set_positional_shadow_atlas_size(RID p_viewport, int p_size, bool p_16_bits)
{
	Renderer::viewport->viewport_set_positional_shadow_atlas_size(p_viewport, p_size, p_16_bits);
}

void Renderer::viewport_set_positional_shadow_atlas_quadrant_subdivision(
	RID p_viewport, int p_quadrant, int p_subdiv)
{
	Renderer::viewport->viewport_set_positional_shadow_atlas_quadrant_subdivision(
		p_viewport, p_quadrant, p_subdiv);
}

void Renderer::viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	Renderer::viewport->viewport_set_msaa_3d(p_viewport, p_msaa);
}

void Renderer::viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	Renderer::viewport->viewport_set_msaa_2d(p_viewport, p_msaa);
}

void Renderer::viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode)
{
	Renderer::viewport->viewport_set_screen_space_aa(p_viewport, p_mode);
}

void Renderer::viewport_set_use_taa(RID p_viewport, bool p_use_taa)
{
	Renderer::viewport->viewport_set_use_taa(p_viewport, p_use_taa);
}

void Renderer::viewport_set_use_debanding(RID p_viewport, bool p_use_debanding)
{
	Renderer::viewport->viewport_set_use_debanding(p_viewport, p_use_debanding);
}

void Renderer::viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels)
{
	Renderer::viewport->viewport_set_mesh_lod_threshold(p_viewport, p_pixels);
}

int Renderer::viewport_get_render_info(
	RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info)
{
	return Renderer::viewport->viewport_get_render_info(p_viewport, p_type, p_info);
}

void Renderer::viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw)
{
	Renderer::viewport->viewport_set_debug_draw(p_viewport, p_draw);
}

void Renderer::viewport_set_measure_render_time(RID p_viewport, bool p_enable)
{
	Renderer::viewport->viewport_set_measure_render_time(p_viewport, p_enable);
}

double Renderer::viewport_get_measured_render_time_cpu(RID p_viewport)
{
	return Renderer::viewport->viewport_get_measured_render_time_cpu(p_viewport);
}

double Renderer::viewport_get_measured_render_time_gpu(RID p_viewport)
{
	return Renderer::viewport->viewport_get_measured_render_time_gpu(p_viewport);
}

RID Renderer::viewport_find_from_screen_attachment(DisplayServerEnums::WindowID p_id)
{
	return Renderer::viewport->viewport_find_from_screen_attachment(p_id);
}

void Renderer::viewport_set_scaling_3d_mode(
	RID p_viewport, RSE::ViewportScaling3DMode p_scaling_3d_mode)
{
	Renderer::viewport->viewport_set_scaling_3d_mode(p_viewport, p_scaling_3d_mode);
}

void Renderer::viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale)
{
	Renderer::viewport->viewport_set_scaling_3d_scale(p_viewport, p_scaling_3d_scale);
}

void Renderer::viewport_set_fsr_sharpness(RID p_viewport, float p_fsr_sharpness)
{
	Renderer::viewport->viewport_set_fsr_sharpness(p_viewport, p_fsr_sharpness);
}

void Renderer::viewport_set_texture_mipmap_bias(RID p_viewport, float p_texture_mipmap_bias)
{
	Renderer::viewport->viewport_set_texture_mipmap_bias(p_viewport, p_texture_mipmap_bias);
}

void Renderer::viewport_set_anisotropic_filtering_level(
	RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
{
	Renderer::viewport->viewport_set_anisotropic_filtering_level(
		p_viewport, p_anisotropic_filtering_level);
}

void Renderer::viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask)
{
	Renderer::viewport->viewport_set_canvas_cull_mask(p_viewport, p_canvas_cull_mask);
}

void Renderer::viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode)
{
	Renderer::viewport->viewport_set_vrs_update_mode(p_viewport, p_mode);
}

void Renderer::viewport_set_vrs_texture(RID p_viewport, RID p_texture)
{
	Renderer::viewport->viewport_set_vrs_texture(p_viewport, p_texture);
}

void Renderer::scenario_set_environment(RID p_scenario, RID p_environment)
{
	RendererSceneCull::scenario_set_environment(p_scenario, p_environment);
}

void Renderer::scenario_set_fallback_environment(RID p_scenario, RID p_environment)
{
	RendererSceneCull::scenario_set_fallback_environment(p_scenario, p_environment);
}

void Renderer::scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes)
{
	RendererSceneCull::scenario_set_camera_attributes(p_scenario, p_camera_attributes);
}

void Renderer::scenario_set_compositor(RID p_scenario, RID p_compositor)
{
	RendererSceneCull::scenario_set_compositor(p_scenario, p_compositor);
}

void Renderer::instance_set_base(RID p_instance, RID p_base)
{
	RendererSceneCull::instance_set_base(p_instance, p_base);
}

void Renderer::instance_set_scenario(RID p_instance, RID p_scenario)
{
	RendererSceneCull::instance_set_scenario(p_instance, p_scenario);
}

void Renderer::instance_set_layer_mask(RID p_instance, uint32_t p_mask)
{
	RendererSceneCull::instance_set_layer_mask(p_instance, p_mask);
}

void Renderer::instance_set_pivot_data(
	RID p_instance, float p_sorting_offset, bool p_use_aabb_center)
{
	RendererSceneCull::instance_set_pivot_data(p_instance, p_sorting_offset, p_use_aabb_center);
}

void Renderer::instance_set_transform(RID p_instance, const Transform3D& p_transform)
{
	RendererSceneCull::instance_set_transform(p_instance, p_transform);
}

void Renderer::instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight)
{
	RendererSceneCull::instance_set_blend_shape_weight(p_instance, p_shape, p_weight);
}

void Renderer::instance_set_surface_override_material(RID p_instance, int p_surface, RID p_material)
{
	RendererSceneCull::instance_set_surface_override_material(p_instance, p_surface, p_material);
}

void Renderer::instance_set_visible(RID p_instance, bool p_visible)
{
	RendererSceneCull::instance_set_visible(p_instance, p_visible);
}

void Renderer::instance_teleport(RID p_instance) { RendererSceneCull::instance_teleport(p_instance); }

void Renderer::instance_attach_skeleton(RID p_instance, RID p_skeleton)
{
	RendererSceneCull::instance_attach_skeleton(p_instance, p_skeleton);
}

void Renderer::instance_set_extra_visibility_margin(RID p_instance, real_t p_margin)
{
	RendererSceneCull::instance_set_extra_visibility_margin(p_instance, p_margin);
}

void Renderer::instance_geometry_set_flag(
	RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled)
{
	RendererSceneCull::instance_geometry_set_flag(p_instance, p_flags, p_enabled);
}

void Renderer::instance_geometry_set_cast_shadows_setting(
	RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting)
{
	RendererSceneCull::instance_geometry_set_cast_shadows_setting(p_instance, p_shadow_casting_setting);
}

void Renderer::instance_geometry_set_material_override(RID p_instance, RID p_material)
{
	RendererSceneCull::instance_geometry_set_material_override(p_instance, p_material);
}

void Renderer::instance_geometry_set_material_overlay(RID p_instance, RID p_material)
{
	RendererSceneCull::instance_geometry_set_material_overlay(p_instance, p_material);
}

void Renderer::instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias)
{
	RendererSceneCull::instance_geometry_set_lod_bias(p_instance, p_lod_bias);
}

void Renderer::camera_set_perspective(
	RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far)
{
	RendererSceneCull::camera_set_perspective(p_camera, p_fovy_degrees, p_z_near, p_z_far);
}

void Renderer::camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far)
{
	RendererSceneCull::camera_set_orthogonal(p_camera, p_size, p_z_near, p_z_far);
}

void Renderer::camera_set_frustum(
	RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far)
{
	RendererSceneCull::camera_set_frustum(p_camera, p_size, p_offset, p_z_near, p_z_far);
}

void Renderer::camera_set_transform(RID p_camera, const Transform3D& p_transform)
{
	RendererSceneCull::camera_set_transform(p_camera, p_transform);
}

RID Renderer::mesh_create() { return RendererMeshStorage::mesh_allocate(); }

void Renderer::mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
{
	RendererMeshStorage::mesh_add_surface(p_mesh, p_surface);
}

void Renderer::mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
{
	RendererMeshStorage::mesh_set_blend_shape_count(p_mesh, p_blend_shape_count);
}

void Renderer::mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
{
	RendererMeshStorage::mesh_set_blend_shape_mode(p_mesh, p_mode);
}

void Renderer::mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb)
{
	RendererMeshStorage::mesh_set_custom_aabb(p_mesh, p_aabb);
}

void Renderer::mesh_set_path(RID p_mesh, const String& p_path)
{
	RendererMeshStorage::mesh_set_path(p_mesh, p_path);
}

void Renderer::mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh)
{
	RendererMeshStorage::mesh_set_shadow_mesh(p_mesh, p_shadow_mesh);
}

void Renderer::mesh_clear(RID p_mesh) { RendererMeshStorage::mesh_clear(p_mesh); }

void Renderer::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
{
	RendererMeshStorage::mesh_surface_set_material(p_mesh, p_surface, p_material);
}

void Renderer::mesh_surface_update_vertex_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RendererMeshStorage::mesh_surface_update_vertex_region(p_mesh, p_surface, p_offset, p_data);
}

void Renderer::mesh_surface_update_attribute_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RendererMeshStorage::mesh_surface_update_attribute_region(p_mesh, p_surface, p_offset, p_data);
}

void Renderer::mesh_surface_update_skin_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RendererMeshStorage::mesh_surface_update_skin_region(p_mesh, p_surface, p_offset, p_data);
}

RenderingServerTypes::SurfaceData Renderer::mesh_get_surface(RID p_mesh, int p_surface)
{
	return RendererMeshStorage::mesh_get_surface(p_mesh, p_surface);
}

RID Renderer::shader_create()
{
	RID ret = RendererMaterialStorage::shader_allocate();
	RendererMaterialStorage::shader_initialize(ret, false);
	return ret;
}

RID Renderer::shader_create_from_code(const String& p_code, const String& p_path_hint)
{
	RID shader = RendererMaterialStorage::shader_allocate();
	RendererMaterialStorage::shader_initialize(shader, false);
	RendererMaterialStorage::shader_set_path_hint(shader, p_path_hint);
	RendererMaterialStorage::shader_set_code(shader, p_code);
	return shader;
}

void Renderer::shader_set_code(RID p_shader, const String& p_code)
{
	RendererMaterialStorage::shader_set_code(p_shader, p_code);
}

void Renderer::shader_set_path_hint(RID p_shader, const String& p_path)
{
	RendererMaterialStorage::shader_set_path_hint(p_shader, p_path);
}

void Renderer::shader_set_default_texture_parameter(
	RID p_shader, const String& p_name, RID p_texture, int p_index)
{
	RendererMaterialStorage::shader_set_default_texture_parameter(
		p_shader, p_name, p_texture, p_index);
}

RID Renderer::material_create()
{
	RID ret = RendererMaterialStorage::material_allocate();
	RendererMaterialStorage::material_initialize(ret);
	return ret;
}

void Renderer::material_set_shader(RID p_shader_material, RID p_shader)
{
	RendererMaterialStorage::material_set_shader(p_shader_material, p_shader);
}

void Renderer::material_set_render_priority(RID p_material, int priority)
{
	RendererMaterialStorage::material_set_render_priority(p_material, priority);
}

void Renderer::material_set_next_pass(RID p_material, RID p_next_material)
{
	RendererMaterialStorage::material_set_next_pass(p_material, p_next_material);
}

void Renderer::material_set_use_debanding(bool p_enable)
{
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::material_set_use_debanding(p_enable);
	}
}

void Renderer::free_rid(RID p_rid)
{
	if (!p_rid.is_valid()) {
		return;
	}

	if (Renderer::viewport && Renderer::viewport->free(p_rid)) {
		return;
	}
	if (RendererSceneCull::is_initialized() && RendererSceneCull::free(p_rid)) {
		return;
	}
	if (RendererUtilities::free(p_rid)) {
		return;
	}
}

RID Renderer::canvas_create()
{
	RID ret = Renderer::canvas->canvas_allocate();
	Renderer::canvas->canvas_initialize(ret);
	return ret;
}

RID Renderer::canvas_texture_create()
{
	RID ret = Renderer::canvas->canvas_texture_allocate();
	Renderer::canvas->canvas_texture_initialize(ret);
	return ret;
}

RID Renderer::canvas_item_create()
{
	RID ret = Renderer::canvas->canvas_item_allocate();
	Renderer::canvas->canvas_item_initialize(ret);
	return ret;
}

RID Renderer::canvas_light_create()
{
	RID ret = Renderer::canvas->canvas_light_allocate();
	Renderer::canvas->canvas_light_initialize(ret);
	return ret;
}

RID Renderer::canvas_light_occluder_create()
{
	RID ret = Renderer::canvas->canvas_light_occluder_allocate();
	Renderer::canvas->canvas_light_occluder_initialize(ret);
	return ret;
}

RID Renderer::canvas_occluder_polygon_create()
{
	RID ret = Renderer::canvas->canvas_occluder_polygon_allocate();
	Renderer::canvas->canvas_occluder_polygon_initialize(ret);
	return ret;
}

RID Renderer::viewport_create()
{
	RID ret = Renderer::viewport->viewport_allocate();
	Renderer::viewport->viewport_initialize(ret);
	return ret;
}

RID Renderer::scenario_create()
{
	RID ret = RendererSceneCull::scenario_allocate();
	RendererSceneCull::scenario_initialize(ret);
	return ret;
}

RID Renderer::instance_create()
{
	RID ret = RendererSceneCull::instance_allocate();
	RendererSceneCull::instance_initialize(ret);
	return ret;
}

RID Renderer::camera_create()
{
	RID ret = RendererSceneCull::camera_allocate();
	RendererSceneCull::camera_initialize(ret);
	return ret;
}

String Renderer::get_video_adapter_name() { return RendererUtilities::get_video_adapter_name(); }

String Renderer::get_video_adapter_vendor()
{
	return RendererUtilities::get_video_adapter_vendor();
}

String Renderer::get_video_adapter_api_version()
{
	return RendererUtilities::get_video_adapter_api_version();
}

void Renderer::set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
	RSE::SplashStretchMode p_mode, bool p_use_filter)
{
	if (RendererCompositor::is_initialized()) {
		RendererCompositor::set_boot_image_with_stretch(p_image, p_color, p_mode, p_use_filter);
	}
}

void Renderer::canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated)
{
	Renderer::canvas->canvas_light_occluder_set_interpolated(p_occluder, p_interpolated);
}

void Renderer::canvas_light_occluder_reset_physics_interpolation(RID p_occluder)
{
	Renderer::canvas->canvas_light_occluder_reset_physics_interpolation(p_occluder);
}

void Renderer::canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas)
{
	Renderer::canvas->canvas_light_occluder_attach_to_canvas(p_occluder, p_canvas);
}

void Renderer::canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform)
{
	Renderer::canvas->canvas_light_occluder_set_transform(p_occluder, p_xform);
}

void Renderer::canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask)
{
	Renderer::canvas->canvas_light_occluder_set_light_mask(p_occluder, p_mask);
}

void Renderer::canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable)
{
	Renderer::canvas->canvas_light_occluder_set_as_sdf_collision(p_occluder, p_enable);
}

void Renderer::canvas_occluder_polygon_set_shape(
	RID p_polygon, const Vector<Vector2>& p_shape, bool p_closed)
{
	Renderer::canvas->canvas_occluder_polygon_set_shape(p_polygon, p_shape, p_closed);
}

void Renderer::canvas_occluder_polygon_set_cull_mode(
	RID p_polygon, RSE::CanvasOccluderPolygonCullMode p_mode)
{
	Renderer::canvas->canvas_occluder_polygon_set_cull_mode(p_polygon, p_mode);
}

Rect2 Renderer::_debug_canvas_item_get_rect(RID p_item)
{
	return Renderer::canvas ? Renderer::canvas->_debug_canvas_item_get_rect(p_item) : Rect2();
}

void Renderer::canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset)
{
	Renderer::canvas->canvas_light_set_texture_offset(p_light, p_offset);
}

void Renderer::canvas_light_attach_to_canvas(RID p_light, RID p_canvas)
{
	Renderer::canvas->canvas_light_attach_to_canvas(p_light, p_canvas);
}

void Renderer::canvas_light_set_transform(RID p_light, const Transform2D& p_transform)
{
	Renderer::canvas->canvas_light_set_transform(p_light, p_transform);
}

void Renderer::canvas_light_set_texture_scale(RID p_light, float p_scale)
{
	Renderer::canvas->canvas_light_set_texture_scale(p_light, p_scale);
}

void Renderer::canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode)
{
	Renderer::canvas->canvas_light_set_mode(p_light, p_mode);
}

void Renderer::canvas_light_set_directional_distance(RID p_light, float p_distance)
{
	Renderer::canvas->canvas_light_set_directional_distance(p_light, p_distance);
}

void Renderer::canvas_light_set_interpolated(RID p_light, bool p_interpolated)
{
	Renderer::canvas->canvas_light_set_interpolated(p_light, p_interpolated);
}

void Renderer::canvas_light_set_enabled(RID p_light, bool p_enabled)
{
	Renderer::canvas->canvas_light_set_enabled(p_light, p_enabled);
}

void Renderer::canvas_light_set_color(RID p_light, const Color& p_color)
{
	Renderer::canvas->canvas_light_set_color(p_light, p_color);
}

void Renderer::canvas_light_set_height(RID p_light, float p_height)
{
	Renderer::canvas->canvas_light_set_height(p_light, p_height);
}

void Renderer::canvas_light_set_energy(RID p_light, float p_energy)
{
	Renderer::canvas->canvas_light_set_energy(p_light, p_energy);
}

void Renderer::canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z)
{
	Renderer::canvas->canvas_light_set_z_range(p_light, p_min_z, p_max_z);
}

void Renderer::canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer)
{
	Renderer::canvas->canvas_light_set_layer_range(p_light, p_min_layer, p_max_layer);
}

void Renderer::canvas_light_set_item_cull_mask(RID p_light, int p_mask)
{
	Renderer::canvas->canvas_light_set_item_cull_mask(p_light, p_mask);
}

void Renderer::canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask)
{
	Renderer::canvas->canvas_light_set_item_shadow_cull_mask(p_light, p_mask);
}

void Renderer::canvas_light_set_shadow_enabled(RID p_light, bool p_enabled)
{
	Renderer::canvas->canvas_light_set_shadow_enabled(p_light, p_enabled);
}

void Renderer::canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter)
{
	Renderer::canvas->canvas_light_set_shadow_filter(p_light, p_filter);
}

void Renderer::canvas_light_set_shadow_color(RID p_light, const Color& p_color)
{
	Renderer::canvas->canvas_light_set_shadow_color(p_light, p_color);
}

void Renderer::canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode)
{
	Renderer::canvas->canvas_light_set_blend_mode(p_light, p_mode);
}

void Renderer::canvas_light_reset_physics_interpolation(RID p_light)
{
	Renderer::canvas->canvas_light_reset_physics_interpolation(p_light);
}

void Renderer::canvas_light_set_shadow_smooth(RID p_light, float p_smooth)
{
	Renderer::canvas->canvas_light_set_shadow_smooth(p_light, p_smooth);
}

RID Renderer::lightmap_create()
{
	RID ret = RendererLightStorage::lightmap_allocate();
	RendererLightStorage::lightmap_initialize(ret);
	return ret;
}

PackedVector3Array Renderer::lightmap_get_probe_capture_points(RID p_lightmap)
{
	return RendererLightStorage::lightmap_get_probe_capture_points(p_lightmap);
}

PackedColorArray Renderer::lightmap_get_probe_capture_sh(RID p_lightmap)
{
	return RendererLightStorage::lightmap_get_probe_capture_sh(p_lightmap);
}

PackedInt32Array Renderer::lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
{
	return RendererLightStorage::lightmap_get_probe_capture_tetrahedra(p_lightmap);
}

PackedInt32Array Renderer::lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
{
	return RendererLightStorage::lightmap_get_probe_capture_bsp_tree(p_lightmap);
}

void Renderer::lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
	const PackedColorArray& p_sh, const PackedInt32Array& p_tetrahedra,
	const PackedInt32Array& p_bsp_tree)
{
	RendererLightStorage::lightmap_set_probe_capture_data(
		p_lightmap, p_points, p_sh, p_tetrahedra, p_bsp_tree);
}

void Renderer::lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
{
	RendererLightStorage::lightmap_set_probe_bounds(p_lightmap, p_bounds);
}

void Renderer::lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
{
	RendererLightStorage::lightmap_set_probe_interior(p_lightmap, p_interior);
}

void Renderer::lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_normalization)
{
	RendererLightStorage::lightmap_set_baked_exposure_normalization(p_lightmap, p_normalization);
}

void Renderer::lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
{
	RendererLightStorage::lightmap_set_textures(p_lightmap, p_light, p_uses_spherical_harmonics);
}

void Renderer::lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
{
	RendererLightStorage::lightmap_set_shadowmask_textures(p_lightmap, p_shadow);
}

void Renderer::lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode)
{
	RendererLightStorage::lightmap_set_shadowmask_mode(p_lightmap, p_mode);
}

RSE::ShadowmaskMode Renderer::lightmap_get_shadowmask_mode(RID p_lightmap)
{
	return RendererLightStorage::lightmap_get_shadowmask_mode(p_lightmap);
}

void Renderer::light_set_negative(RID p_light, bool p_enable)
{
	RendererLightStorage::light_set_negative(p_light, p_enable);
}

void Renderer::light_set_distance_fade(
	RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
{
	RendererLightStorage::light_set_distance_fade(p_light, p_enabled, p_begin, p_shadow, p_length);
}

void Renderer::light_set_cull_mask(RID p_light, uint32_t p_mask)
{
	RendererLightStorage::light_set_cull_mask(p_light, p_mask);
}

void Renderer::light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
{
	RendererLightStorage::light_set_reverse_cull_face_mode(p_light, p_enabled);
}

void Renderer::light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
{
	RendererLightStorage::light_set_shadow_caster_mask(p_light, p_caster_mask);
}

void Renderer::light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
{
	RendererLightStorage::light_set_bake_mode(p_light, p_bake_mode);
}

void Renderer::light_directional_set_blend_splits(RID p_light, bool p_enable)
{
	RendererLightStorage::light_directional_set_blend_splits(p_light, p_enable);
}

void Renderer::light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode)
{
	RendererLightStorage::light_directional_set_sky_mode(p_light, p_mode);
}

void Renderer::light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode)
{
	RendererLightStorage::light_omni_set_shadow_mode(p_light, p_mode);
}

void Renderer::light_area_set_normalize_energy(RID p_light, bool p_enable)
{
	RendererLightStorage::light_area_set_normalize_energy(p_light, p_enable);
}

RID Renderer::camera_attributes_create()
{
	RID ret = RendererCameraAttributes::camera_attributes_allocate();
	RendererCameraAttributes::camera_attributes_initialize(ret);
	return ret;
}

void Renderer::camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
	float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale)
{
	RendererCameraAttributes::camera_attributes_set_auto_exposure(
		p_camera_attributes, p_enable, p_min_sensitivity, p_max_sensitivity, p_speed, p_scale);
}

void Renderer::camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
	float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
	float p_near_transition, float p_amount)
{
	RendererCameraAttributes::camera_attributes_set_dof_blur(p_camera_attributes, p_far_enable,
		p_far_distance, p_far_transition, p_near_enable, p_near_distance, p_near_transition,
		p_amount);
}

void Renderer::camera_attributes_set_exposure(
	RID p_camera_attributes, float p_multiplier, float p_normalization)
{
	RendererCameraAttributes::camera_attributes_set_exposure(
		p_camera_attributes, p_multiplier, p_normalization);
}

void Renderer::environment_set_sky_orientation(RID p_env, const Basis& p_orientation)
{
	RendererSceneCull::environment_set_sky_orientation(p_env, p_orientation);
}

void Renderer::environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end)
{
	RendererSceneCull::environment_set_fog_depth(p_env, p_curve, p_begin, p_end);
}

void Renderer::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
	const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
	float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
	float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect)
{
	RendererSceneCull::environment_set_volumetric_fog(p_env, p_enable, p_density, p_albedo, p_emission,
		p_emission_energy, p_anisotropy, p_length, p_detail_spread, p_gi_inject,
		p_temporal_reprojection, p_temporal_reprojection_amount, p_ambient_inject, p_sky_affect);
}

void Renderer::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
	float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
	RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map)
{
	RendererSceneCull::environment_set_glow(p_env, p_enable, p_levels, p_intensity, p_strength, p_mix,
		p_bloom_threshold, p_blend_mode, p_hdr_bleed_threshold, p_hdr_bleed_scale,
		p_hdr_luminance_cap, p_glow_map_strength, p_glow_map);
}

void Renderer::environment_set_sky(RID p_env, RID p_sky)
{
	RendererSceneCull::environment_set_sky(p_env, p_sky);
}

void Renderer::environment_set_sky_custom_fov(RID p_env, float p_scale)
{
	RendererSceneCull::environment_set_sky_custom_fov(p_env, p_scale);
}

void Renderer::environment_set_bg_color(RID p_env, const Color& p_color)
{
	RendererSceneCull::environment_set_bg_color(p_env, p_color);
}

void Renderer::environment_set_bg_energy(RID p_env, float p_energy, float p_multiplier)
{
	RendererSceneCull::environment_set_bg_energy(p_env, p_energy, p_multiplier);
}

void Renderer::environment_set_canvas_max_layer(RID p_env, int p_max_layer)
{
	RendererSceneCull::environment_set_canvas_max_layer(p_env, p_max_layer);
}

void Renderer::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id)
{
	RendererSceneCull::environment_set_camera_feed_id(p_env, p_camera_feed_id);
}

void Renderer::environment_set_ambient_light(RID p_env, const Color& p_color,
	RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution,
	RSE::EnvironmentReflectionSource p_reflection_source)
{
	RendererSceneCull::environment_set_ambient_light(
		p_env, p_color, p_ambient, p_energy, p_sky_contribution, p_reflection_source);
}

void Renderer::environment_set_tonemap(
	RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white)
{
	RendererSceneCull::environment_set_tonemap(p_env, p_tone_mapper, p_exposure, p_white);
}

void Renderer::environment_set_tonemap_agx_contrast(RID p_env, float p_contrast)
{
	RendererSceneCull::environment_set_tonemap_agx_contrast(p_env, p_contrast);
}

void Renderer::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_in,
	float p_fade_out, float p_depth_tolerance)
{
	RendererSceneCull::environment_set_ssr(
		p_env, p_enable, p_max_steps, p_fade_in, p_fade_out, p_depth_tolerance);
}

void Renderer::environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
	float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
	float p_ao_channel_affect)
{
	RendererSceneCull::environment_set_ssao(p_env, p_enable, p_radius, p_intensity, p_power, p_detail,
		p_horizon, p_sharpness, p_light_affect, p_ao_channel_affect);
}

void Renderer::environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
	float p_sharpness, float p_normal_rejection)
{
	RendererSceneCull::environment_set_ssil(
		p_env, p_enable, p_radius, p_intensity, p_sharpness, p_normal_rejection);
}

void Renderer::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
	float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
	float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
	float p_probe_bias)
{
	RendererSceneCull::environment_set_sdfgi(p_env, p_enable, p_cascades, p_min_cell_size, p_y_scale,
		p_use_occlusion, p_bounce_feedback, p_read_sky, p_energy, p_normal_bias, p_probe_bias);
}

void Renderer::environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
	float p_light_energy, float p_sun_scatter, float p_density, float p_height,
	float p_height_density, float p_aerial_perspective, float p_sky_affect,
	RSE::EnvironmentFogMode p_mode)
{
	RendererSceneCull::environment_set_fog(p_env, p_enable, p_light_color, p_light_energy, p_sun_scatter,
		p_density, p_height, p_height_density, p_aerial_perspective, p_sky_affect, p_mode);
}

void Renderer::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
	float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction)
{
	RendererSceneCull::environment_set_adjustment(p_env, p_enable, p_brightness, p_contrast, p_saturation,
		p_use_1d_color_correction, p_color_correction);
}

RID Renderer::compositor_create()
{
	RID ret = RendererSceneCull::compositor_allocate();
	RendererSceneCull::compositor_initialize(ret);
	return ret;
}

void Renderer::compositor_effect_set_enabled(RID p_effect, bool p_enabled)
{
	RendererSceneCull::compositor_effect_set_enabled(p_effect, p_enabled);
}

void Renderer::compositor_effect_set_flag(
	RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set)
{
	RendererSceneCull::compositor_effect_set_flag(p_effect, p_flag, p_set);
}

RID Renderer::sky_create()
{
	RID ret = RendererSceneCull::sky_allocate();
	RendererSceneCull::sky_initialize(ret);
	return ret;
}

void Renderer::sky_set_radiance_size(RID p_sky, int p_radiance_size)
{
	RendererSceneCull::sky_set_radiance_size(p_sky, p_radiance_size);
}

void Renderer::sky_set_mode(RID p_sky, RSE::SkyMode p_mode)
{
	RendererSceneCull::sky_set_mode(p_sky, p_mode);
}

void Renderer::sky_set_material(RID p_sky, RID p_material)
{
	RendererSceneCull::sky_set_material(p_sky, p_material);
}

RID Renderer::occluder_create()
{
	RID ret = RendererSceneCull::occluder_allocate();
	RendererSceneCull::occluder_initialize(ret);
	return ret;
}

void Renderer::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	RendererSceneCull::occluder_set_mesh(p_occluder, p_vertices, p_indices);
}

RID Renderer::multimesh_create()
{
	RID ret = RendererMeshStorage::multimesh_allocate();
	RendererMeshStorage::multimesh_initialize(ret);
	return ret;
}

void Renderer::multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	RendererMeshStorage::multimesh_allocate_data(p_multimesh, p_instances, p_transform_format,
		p_use_colors, p_use_custom_data, p_use_indirect);
}

AABB Renderer::multimesh_get_aabb(RID p_multimesh)
{
	return RendererMeshStorage::multimesh_get_aabb(p_multimesh);
}

void Renderer::multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color)
{
	RendererMeshStorage::multimesh_instance_set_color(p_multimesh, p_index, p_color);
}

void Renderer::multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	RendererMeshStorage::multimesh_instance_set_custom_data(p_multimesh, p_index, p_color);
}

Vector<float> Renderer::multimesh_get_buffer(RID p_multimesh)
{
	return RendererMeshStorage::multimesh_get_buffer(p_multimesh);
}

void Renderer::multimesh_set_visible_instances(RID p_multimesh, int p_visible)
{
	RendererMeshStorage::multimesh_set_visible_instances(p_multimesh, p_visible);
}

void Renderer::multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	RendererMeshStorage::multimesh_instance_set_transform_2d(p_multimesh, p_index, p_transform);
}

Transform3D Renderer::multimesh_instance_get_transform(RID p_multimesh, int p_index)
{
	return RendererMeshStorage::multimesh_instance_get_transform(p_multimesh, p_index);
}

Transform2D Renderer::multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
{
	return RendererMeshStorage::multimesh_instance_get_transform_2d(p_multimesh, p_index);
}

void Renderer::multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	RendererMeshStorage::multimesh_set_custom_aabb(p_multimesh, p_aabb);
}

void Renderer::multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
	RendererMeshStorage::multimesh_set_buffer(p_multimesh, p_buffer);
}

void Renderer::multimesh_set_buffer_interpolated(
	RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev)
{
	RendererMeshStorage::multimesh_set_buffer_interpolated(p_multimesh, p_buffer, p_buffer_prev);
}

void Renderer::multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	RendererMeshStorage::multimesh_set_mesh(p_multimesh, p_mesh);
}

void Renderer::multimesh_set_physics_interpolation_quality(
	RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality)
{
	RendererMeshStorage::multimesh_set_physics_interpolation_quality(p_multimesh, p_quality);
}

void Renderer::multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	RendererMeshStorage::multimesh_instance_set_transform(p_multimesh, p_index, p_transform);
}

Color Renderer::multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	return RendererMeshStorage::multimesh_instance_get_color(p_multimesh, p_index);
}

Color Renderer::multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	return RendererMeshStorage::multimesh_instance_get_custom_data(p_multimesh, p_index);
}

void Renderer::multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index)
{
	RendererMeshStorage::multimesh_instance_reset_physics_interpolation(p_multimesh, p_index);
}

void Renderer::multimesh_instances_reset_physics_interpolation(RID p_multimesh)
{
	RendererMeshStorage::multimesh_instances_reset_physics_interpolation(p_multimesh);
}

void Renderer::multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated)
{
	RendererMeshStorage::multimesh_set_physics_interpolated(p_multimesh, p_interpolated);
}

RID Renderer::skeleton_create()
{
	RID ret = RendererMeshStorage::skeleton_allocate();
	RendererMeshStorage::skeleton_initialize(ret);
	return ret;
}

void Renderer::skeleton_bone_set_transform_2d(
	RID p_skeleton, int p_bone, const Transform2D& p_transform)
{
	RendererMeshStorage::skeleton_bone_set_transform_2d(p_skeleton, p_bone, p_transform);
}

void Renderer::skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform)
{
	RendererMeshStorage::skeleton_set_base_transform_2d(p_skeleton, p_base_transform);
}

RID Renderer::texture_external_create(int p_width, int p_height, uint64_t p_external_buffer)
{
	RID ret = RendererTextureStorage::texture_allocate();
	RendererTextureStorage::texture_external_initialize(ret, p_width, p_height, p_external_buffer);
	return ret;
}

void Renderer::texture_external_update(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
	RendererTextureStorage::texture_external_update(
		p_texture, p_width, p_height, p_external_buffer);
}

RID Renderer::decal_create()
{
	RID ret = RendererTextureStorage::decal_allocate();
	RendererTextureStorage::decal_initialize(ret);
	return ret;
}

void Renderer::decal_set_modulate(RID p_decal, const Color& p_modulate)
{
	RendererTextureStorage::decal_set_modulate(p_decal, p_modulate);
}

void Renderer::decal_set_emission_energy(RID p_decal, float p_energy)
{
	RendererTextureStorage::decal_set_emission_energy(p_decal, p_energy);
}

void Renderer::decal_set_albedo_mix(RID p_decal, float p_mix)
{
	RendererTextureStorage::decal_set_albedo_mix(p_decal, p_mix);
}

void Renderer::decal_set_fade(RID p_decal, float p_above, float p_below)
{
	RendererTextureStorage::decal_set_fade(p_decal, p_above, p_below);
}

void Renderer::decal_set_normal_fade(RID p_decal, float p_fade)
{
	RendererTextureStorage::decal_set_normal_fade(p_decal, p_fade);
}

void Renderer::decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length)
{
	RendererTextureStorage::decal_set_distance_fade(p_decal, p_enabled, p_begin, p_length);
}

RID Renderer::particles_create()
{
	RID ret = Renderer::particles_storage->particles_allocate();
	Renderer::particles_storage->particles_initialize(ret);
	return ret;
}

void Renderer::particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode)
{
	Renderer::particles_storage->particles_set_mode(p_particles, p_mode);
}

void Renderer::particles_set_draw_passes(RID p_particles, int p_count)
{
	Renderer::particles_storage->particles_set_draw_passes(p_particles, p_count);
}

void Renderer::particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh)
{
	Renderer::particles_storage->particles_set_draw_pass_mesh(p_particles, p_pass, p_mesh);
}

void Renderer::particles_restart(RID p_particles)
{
	Renderer::particles_storage->particles_restart(p_particles);
}

void Renderer::particles_set_emitting(RID p_particles, bool p_emitting)
{
	Renderer::particles_storage->particles_set_emitting(p_particles, p_emitting);
}

void Renderer::particles_set_seed(RID p_particles, uint32_t p_seed)
{
	Renderer::particles_storage->particles_set_seed(p_particles, p_seed);
}

void Renderer::particles_request_process_time(
	RID p_particles, real_t p_time, real_t p_interpolation)
{
	Renderer::particles_storage->particles_request_process_time(p_particles, p_time, p_interpolation);
}

void Renderer::particles_set_one_shot(RID p_particles, bool p_one_shot)
{
	Renderer::particles_storage->particles_set_one_shot(p_particles, p_one_shot);
}

void Renderer::particles_set_emission_transform(RID p_particles, const Transform3D& p_transform)
{
	Renderer::particles_storage->particles_set_emission_transform(p_particles, p_transform);
}

void Renderer::particles_set_use_local_coordinates(RID p_particles, bool p_enable)
{
	Renderer::particles_storage->particles_set_use_local_coordinates(p_particles, p_enable);
}

AABB Renderer::particles_get_current_aabb(RID p_particles)
{
	return Renderer::particles_storage->particles_get_current_aabb(p_particles);
}

void Renderer::particles_emit(RID p_particles, const Transform3D& p_transform,
	const Vector3& p_velocity, const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Renderer::particles_storage->particles_emit(
		p_particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}

void Renderer::particles_set_amount(RID p_particles, int p_amount)
{
	Renderer::particles_storage->particles_set_amount(p_particles, p_amount);
}

void Renderer::particles_set_amount_ratio(RID p_particles, float p_ratio)
{
	Renderer::particles_storage->particles_set_amount_ratio(p_particles, p_ratio);
}

void Renderer::particles_set_lifetime(RID p_particles, double p_lifetime)
{
	Renderer::particles_storage->particles_set_lifetime(p_particles, p_lifetime);
}

void Renderer::particles_set_fixed_fps(RID p_particles, int p_fps)
{
	Renderer::particles_storage->particles_set_fixed_fps(p_particles, p_fps);
}

void Renderer::particles_set_fractional_delta(RID p_particles, bool p_enable)
{
	Renderer::particles_storage->particles_set_fractional_delta(p_particles, p_enable);
}

void Renderer::particles_set_interpolate(RID p_particles, bool p_enable)
{
	Renderer::particles_storage->particles_set_interpolate(p_particles, p_enable);
}

void Renderer::particles_set_pre_process_time(RID p_particles, double p_time)
{
	Renderer::particles_storage->particles_set_pre_process_time(p_particles, p_time);
}

void Renderer::particles_set_explosiveness_ratio(RID p_particles, float p_ratio)
{
	Renderer::particles_storage->particles_set_explosiveness_ratio(p_particles, p_ratio);
}

void Renderer::particles_set_randomness_ratio(RID p_particles, float p_ratio)
{
	Renderer::particles_storage->particles_set_randomness_ratio(p_particles, p_ratio);
}

void Renderer::particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order)
{
	Renderer::particles_storage->particles_set_draw_order(p_particles, p_order);
}

void Renderer::particles_set_speed_scale(RID p_particles, double p_scale)
{
	Renderer::particles_storage->particles_set_speed_scale(p_particles, p_scale);
}

void Renderer::particles_set_collision_base_size(RID p_particles, float p_size)
{
	Renderer::particles_storage->particles_set_collision_base_size(p_particles, p_size);
}

void Renderer::particles_set_transform_align_channel_filter(
	RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel)
{
	Renderer::particles_storage->particles_set_transform_align_channel_filter(p_particles, p_channel);
}

void Renderer::particles_set_interp_to_end(RID p_particles, float p_interp)
{
	Renderer::particles_storage->particles_set_interp_to_end(p_particles, p_interp);
}

void Renderer::particles_set_trails(RID p_particles, bool p_enable, float p_length_sec)
{
	Renderer::particles_storage->particles_set_trails(p_particles, p_enable, p_length_sec);
}

void Renderer::particles_set_transform_align_axis(
	RID p_particles, RSE::ParticlesTransformAlignAxis p_axis)
{
	Renderer::particles_storage->particles_set_transform_align_axis(p_particles, p_axis);
}

RID Renderer::particles_collision_create()
{
	RID ret = Renderer::particles_storage->particles_collision_allocate();
	Renderer::particles_storage->particles_collision_initialize(ret);
	return ret;
}

void Renderer::particles_collision_set_collision_type(
	RID p_particles_collision, RSE::ParticlesCollisionType p_type)
{
	Renderer::particles_storage->particles_collision_set_collision_type(p_particles_collision, p_type);
}

void Renderer::particles_collision_height_field_update(RID p_particles_collision)
{
	Renderer::particles_storage->particles_collision_height_field_update(p_particles_collision);
}

void Renderer::particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask)
{
	Renderer::particles_storage->particles_collision_set_cull_mask(p_particles_collision, p_cull_mask);
}

void Renderer::particles_collision_set_field_texture(RID p_particles_collision, RID p_texture)
{
	Renderer::particles_storage->particles_collision_set_field_texture(p_particles_collision, p_texture);
}

void Renderer::particles_collision_set_height_field_mask(RID p_particles_collision, uint32_t p_mask)
{
	Renderer::particles_storage->particles_collision_set_height_field_mask(p_particles_collision, p_mask);
}

void Renderer::particles_collision_set_attractor_strength(
	RID p_particles_collision, real_t p_strength)
{
	Renderer::particles_storage->particles_collision_set_attractor_strength(
		p_particles_collision, p_strength);
}

void Renderer::particles_collision_set_attractor_attenuation(
	RID p_particles_collision, real_t p_attenuation)
{
	Renderer::particles_storage->particles_collision_set_attractor_attenuation(
		p_particles_collision, p_attenuation);
}

void Renderer::particles_collision_set_sphere_radius(
	RID p_particles_collision, real_t p_radius)
{
	Renderer::particles_storage->particles_collision_set_sphere_radius(
		p_particles_collision, p_radius);
}

void Renderer::particles_collision_set_box_extents(
	RID p_particles_collision, const Vector3& p_extents)
{
	Renderer::particles_storage->particles_collision_set_box_extents(
		p_particles_collision, p_extents);
}

void Renderer::canvas_light_set_texture(RID p_light, RID p_texture)
{
	Renderer::canvas->canvas_light_set_texture(p_light, p_texture);
}

RID Renderer::directional_light_create()
{
	RID ret = RendererLightStorage::directional_light_allocate();
	RendererLightStorage::directional_light_initialize(ret);
	return ret;
}

RID Renderer::omni_light_create()
{
	RID ret = RendererLightStorage::omni_light_allocate();
	RendererLightStorage::omni_light_initialize(ret);
	return ret;
}

RID Renderer::spot_light_create()
{
	RID ret = RendererLightStorage::spot_light_allocate();
	RendererLightStorage::spot_light_initialize(ret);
	return ret;
}

RID Renderer::area_light_create()
{
	RID ret = RendererLightStorage::area_light_allocate();
	RendererLightStorage::area_light_initialize(ret);
	return ret;
}

void Renderer::light_set_color(RID p_light, const Color& p_color)
{
	RendererLightStorage::light_set_color(p_light, p_color);
}

void Renderer::light_set_param(RID p_light, RSE::LightParam p_param, float p_value)
{
	RendererLightStorage::light_set_param(p_light, p_param, p_value);
}

void Renderer::light_set_shadow(RID p_light, bool p_enabled)
{
	RendererLightStorage::light_set_shadow(p_light, p_enabled);
}

void Renderer::light_set_projector(RID p_light, RID p_texture)
{
	RendererLightStorage::light_set_projector(p_light, p_texture);
}

void Renderer::light_directional_set_shadow_mode(
	RID p_light, RSE::LightDirectionalShadowMode p_mode)
{
	RendererLightStorage::light_directional_set_shadow_mode(p_light, p_mode);
}

void Renderer::light_area_set_size(RID p_light, const Vector2& p_size)
{
	RendererLightStorage::light_area_set_size(p_light, p_size);
}

void Renderer::light_area_set_texture(RID p_light, RID p_texture)
{
	RendererLightStorage::light_area_set_texture(p_light, p_texture);
}

RID Renderer::fog_volume_create()
{
	RID ret = Renderer::fog->fog_volume_allocate();
	Renderer::fog->fog_volume_initialize(ret);
	return ret;
}

void Renderer::fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape)
{
	Renderer::fog->fog_volume_set_shape(p_fog_volume, p_shape);
}

RID Renderer::voxel_gi_create()
{
	RID ret = RendererGI::voxel_gi_allocate();
	RendererGI::voxel_gi_initialize(ret);
	return ret;
}

void Renderer::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
	const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	RendererGI::voxel_gi_allocate_data(p_voxel_gi, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
}

Vector<uint8_t> Renderer::voxel_gi_get_octree_cells(RID p_voxel_gi)
{
	return RendererGI::voxel_gi_get_octree_cells(p_voxel_gi);
}

Vector<uint8_t> Renderer::voxel_gi_get_data_cells(RID p_voxel_gi)
{
	return RendererGI::voxel_gi_get_data_cells(p_voxel_gi);
}

Vector<uint8_t> Renderer::voxel_gi_get_distance_field(RID p_voxel_gi)
{
	return RendererGI::voxel_gi_get_distance_field(p_voxel_gi);
}

Vector<int> Renderer::voxel_gi_get_level_counts(RID p_voxel_gi)
{
	return RendererGI::voxel_gi_get_level_counts(p_voxel_gi);
}

void Renderer::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range)
{
	RendererGI::voxel_gi_set_dynamic_range(p_voxel_gi, p_range);
}

void Renderer::voxel_gi_set_propagation(RID p_voxel_gi, float p_propagation)
{
	RendererGI::voxel_gi_set_propagation(p_voxel_gi, p_propagation);
}

void Renderer::voxel_gi_set_energy(RID p_voxel_gi, float p_energy)
{
	RendererGI::voxel_gi_set_energy(p_voxel_gi, p_energy);
}

void Renderer::voxel_gi_set_bias(RID p_voxel_gi, float p_bias)
{
	RendererGI::voxel_gi_set_bias(p_voxel_gi, p_bias);
}

void Renderer::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_bias)
{
	RendererGI::voxel_gi_set_normal_bias(p_voxel_gi, p_bias);
}

void Renderer::voxel_gi_set_interior(RID p_voxel_gi, bool p_interior)
{
	RendererGI::voxel_gi_set_interior(p_voxel_gi, p_interior);
}

void Renderer::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_use_two_bounces)
{
	RendererGI::voxel_gi_set_use_two_bounces(p_voxel_gi, p_use_two_bounces);
}

void Renderer::voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_normalization)
{
	RendererGI::voxel_gi_set_baked_exposure_normalization(p_voxel_gi, p_normalization);
}

RID Renderer::reflection_probe_create()
{
	RID ret = RendererLightStorage::reflection_probe_allocate();
	RendererLightStorage::reflection_probe_initialize(ret);
	return ret;
}

void Renderer::reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
{
	RendererLightStorage::reflection_probe_set_ambient_color(p_probe, p_color);
}

void Renderer::reflection_probe_set_intensity(RID p_probe, float p_intensity)
{
	RendererLightStorage::reflection_probe_set_intensity(p_probe, p_intensity);
}

void Renderer::reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
{
	RendererLightStorage::reflection_probe_set_ambient_energy(p_probe, p_energy);
}

void Renderer::reflection_probe_set_max_distance(RID p_probe, float p_distance)
{
	RendererLightStorage::reflection_probe_set_max_distance(p_probe, p_distance);
}

void Renderer::reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
{
	RendererLightStorage::reflection_probe_set_mesh_lod_threshold(p_probe, p_ratio);
}

void Renderer::reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable)

{
	RendererLightStorage::reflection_probe_set_enable_box_projection(p_probe, p_enable);
}

void Renderer::reflection_probe_set_as_interior(RID p_probe, bool p_enable)
{
	RendererLightStorage::reflection_probe_set_as_interior(p_probe, p_enable);
}

void Renderer::reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
{
	RendererLightStorage::reflection_probe_set_enable_shadows(p_probe, p_enable);
}

void Renderer::reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
{
	RendererLightStorage::reflection_probe_set_cull_mask(p_probe, p_layers);
}

void Renderer::reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
{
	RendererLightStorage::reflection_probe_set_reflection_mask(p_probe, p_layers);
}

void Renderer::reflection_probe_set_update_mode(RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
{
	RendererLightStorage::reflection_probe_set_update_mode(p_probe, p_mode);
}

void Renderer::draw(bool p_swap_buffers, double frame_step)
{
	RendererCompositor::begin_frame(frame_step);

	if (RendererUtilities::update_dirty_resources) {
		RendererUtilities::update_dirty_resources();
	}

	if (canvas) {
		canvas->update();
	}

	if (viewport) {
		viewport->draw_viewports(p_swap_buffers);
	}

	if (RendererCanvasRender::update) {
		RendererCanvasRender::update();
	}

	RendererCompositor::end_frame(p_swap_buffers);
}

// ============================================================================
// Merged from servers/rendering/dummy/rasterizer_dummy.cpp
// ============================================================================

#include "servers/rendering/dummy/rasterizer_dummy.h"

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/storage/camera_attributes_storage.h"
#include "servers/rendering/storage/particles_storage.h"

/* LightStorage */

void RendererDummy::LightStorage::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void RendererDummy::LightStorage::finalize()
{
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

bool RendererDummy::LightStorage::free(RID p_rid)
{
	if (owns_lightmap(p_rid)) {
		lightmap_free(p_rid);
		return true;
	}
	else if (owns_lightmap_instance(p_rid)) {
		lightmap_instance_free(p_rid);
		return true;
	}

	return false;
}

RID RendererDummy::LightStorage::lightmap_allocate()
{
	return _get_data()->lightmap_owner.allocate_rid();
}

void RendererDummy::LightStorage::lightmap_initialize(RID p_lightmap)
{
	_get_data()->lightmap_owner.initialize_rid(p_lightmap, Lightmap());
}

void RendererDummy::LightStorage::lightmap_free(RID p_rid)
{
	if (!data) {
		return;
	}
	lightmap_set_textures(p_rid, RID(), false);
	data->lightmap_owner.free(p_rid);
}

RID RendererDummy::LightStorage::lightmap_instance_create(RID p_lightmap)
{
	LightmapInstance li;
	li.lightmap = p_lightmap;
	return _get_data()->lightmap_instance_owner.make_rid(li);
}

void RendererDummy::LightStorage::lightmap_instance_free(RID p_lightmap)
{
	if (!data) {
		return;
	}
	data->lightmap_instance_owner.free(p_lightmap);
}

/* MaterialStorage */

void RendererDummy::MaterialStorage::global_shader_parameter_remove(const String& p_name)
{
	Data* d = _get_data();
	if (!d->global_shader_variables.has(p_name)) {
		return;
	}

	d->global_shader_variables.erase(p_name);
}

Vector<String> RendererDummy::MaterialStorage::global_shader_parameter_get_list()
{
	Data* d = _get_data();
	Vector<String> names;
	for (const KeyValue<String, RSE::GlobalShaderParameterType>& E : d->global_shader_variables) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

RID RendererDummy::MaterialStorage::shader_allocate()
{
	return _get_data()->shader_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	_get_data()->shader_owner.initialize_rid(p_rid, DummyShader());
}

void RendererDummy::MaterialStorage::shader_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyShader* shader = data->shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	data->shader_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	Data* d = _get_data();
	DummyShader* shader = d->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);
	if (p_code.is_empty()) {
		return;
	}

	String mode_string = ShaderLanguage::get_shader_type(p_code);

	RSE::ShaderMode new_mode;
	if (mode_string == "canvas_item") {
		new_mode = RSE::SHADER_CANVAS_ITEM;
	}
	else if (mode_string == "particles") {
		new_mode = RSE::SHADER_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_mode = RSE::SHADER_SPATIAL;
	}
	else if (mode_string == "sky") {
		new_mode = RSE::SHADER_SKY;
	}
	else if (mode_string == "fog") {
		new_mode = RSE::SHADER_FOG;
	}
	else if (mode_string == "texture_blit") {
		new_mode = RSE::SHADER_TEXTURE_BLIT;
	}
	else {
		new_mode = RSE::SHADER_MAX;
		ERR_FAIL_MSG("Shader type " + mode_string + " not supported in Dummy renderer.");
	}
	ShaderCompiler::IdentifierActions actions;
	actions.uniforms = &shader->uniforms;
	ShaderCompiler::GeneratedCode gen_code;

	Error err = d->dummy_compiler->compile(new_mode, p_code, &actions, "", gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");
}

RID RendererDummy::MaterialStorage::material_allocate()
{
	return _get_data()->material_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::material_initialize(RID p_rid)
{
	_get_data()->material_owner.initialize_rid(p_rid, DummyMaterial());
}

void RendererDummy::MaterialStorage::material_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyMaterial* material = data->material_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(material);

	data->material_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->shader = p_shader;
}

void RendererDummy::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->next_pass = p_next_material;
}

RSE::GlobalShaderParameterType RendererDummy::MaterialStorage::global_shader_parameter_get_type(
	const String& p_name)
{
	return RSE::GLOBAL_VAR_TYPE_MAX;
}

void RendererDummy::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void RendererDummy::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
}

void RendererDummy::MaterialStorage::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->dummy_compiler) {
		data->dummy_compiler = memnew(ShaderCompiler);
		ShaderCompiler::DefaultIdentifierActions actions;
		data->dummy_compiler->initialize(actions);
	}
}

void RendererDummy::MaterialStorage::finalize()
{
	if (!data) {
		return;
	}
	if (data->dummy_compiler) {
		memdelete(data->dummy_compiler);
		data->dummy_compiler = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

/* Utilities */

RSE::InstanceType RendererDummy::Utilities::get_base_type(RID p_rid)
{
	if (RendererDummy::MeshStorage::owns_mesh(p_rid)) {
		return RSE::INSTANCE_MESH;
	}
	else if (RendererDummy::MeshStorage::owns_multimesh(p_rid)) {
		return RSE::INSTANCE_MULTIMESH;
	}
	else if (RendererDummy::LightStorage::owns_lightmap(p_rid)) {
		return RSE::INSTANCE_LIGHTMAP;
	}
	return RSE::INSTANCE_NONE;
}

bool RendererDummy::Utilities::free(RID p_rid)
{
	if (RendererDummy::LightStorage::free(p_rid)) {
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_texture(p_rid)) {
		RendererDummy::TextureStorage::texture_free(p_rid);
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_canvas_texture(p_rid)) {
		RendererDummy::TextureStorage::canvas_texture_free(p_rid);
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_render_target(p_rid)) {
		RendererDummy::TextureStorage::render_target_free(p_rid);
		return true;
	}
	else if (RendererDummy::MeshStorage::owns_mesh(p_rid)) {
		RendererDummy::MeshStorage::mesh_free(p_rid);
		return true;
	}
	else if (RendererDummy::MeshStorage::owns_multimesh(p_rid)) {
		RendererDummy::MeshStorage::multimesh_free(p_rid);
		return true;
	}
	else if (RendererDummy::MaterialStorage::owns_shader(p_rid)) {
		RendererDummy::MaterialStorage::shader_free(p_rid);
		return true;
	}
	else if (RendererDummy::MaterialStorage::owns_material(p_rid)) {
		RendererDummy::MaterialStorage::material_free(p_rid);
		return true;
	}
	return false;
}

void RendererDummy::Utilities::base_update_dependency(RID p_base, DependencyTracker* p_instance)
{
	if (RendererDummy::MeshStorage::owns_mesh(p_base)) {
		DummyMesh* mesh = RendererDummy::MeshStorage::get_mesh(p_base);
		p_instance->update_dependency(&mesh->dependency);
	}
}

/* RasterizerSceneDummy */

bool RasterizerSceneDummy::free(RID p_rid)
{
	if (is_environment(p_rid)) {
		environment_free(p_rid);
		return true;
	}
	else if (is_compositor(p_rid)) {
		compositor_free(p_rid);
		return true;
	}
	else if (is_compositor_effect(p_rid)) {
		compositor_effect_free(p_rid);
		return true;
	}
	else if (RendererCameraAttributes::owns_camera_attributes(p_rid)) {
		RendererCameraAttributes::camera_attributes_free(p_rid);
		return true;
	}
	else {
		return false;
	}
}

/* RasterizerDummy */

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
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::set_scene_render(scene);
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
		DisplayServer::swap_buffers();
	}
}

void RasterizerDummy::finalize()
{
	if (RendererSceneCull::is_initialized() && RendererSceneCull::get_scene_render() == scene) {
		RendererSceneCull::set_scene_render(nullptr);
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

// ============================================================================
// Merged from servers/rendering/renderer_canvas_cull.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "core/math/transform_interpolator.h"
#include "renderer_canvas_cull.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/storage/texture_storage.h"

// Use the same antialiasing feather size as flat StyleBox's default
// (but doubled, as it's specified for both sides here).
// This value is empirically determined to provide good antialiasing quality
// while not making lines appear too soft.
const static float FEATHER_SIZE = 1.25f;

static RendererCanvasCull* _canvas_cull_singleton = nullptr;

void RendererCanvasCull::_dependency_changed(
	Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker)
{
	Item* item = (Item*)p_tracker->userdata;

	switch (p_notification) {
	case Dependency::DEPENDENCY_CHANGED_MATERIAL: {
		_canvas_cull_singleton->_item_queue_update(item, true);
	} break;
	default: {
	} break;
	}
}

void RendererCanvasCull::_dependency_deleted(const RID& p_dependency, DependencyTracker* p_tracker)
{
	Item* item = (Item*)p_tracker->userdata;

	if (p_dependency == item->material) {
		_canvas_cull_singleton->canvas_item_set_material(item->self, RID());
	}
	_canvas_cull_singleton->_item_queue_update(item, true);
}

void RendererCanvasCull::_render_canvas_item_tree(RID p_to_render_target,
	Canvas::ChildItem* p_child_items, int p_child_item_count, const Transform2D& p_transform,
	const Rect2& p_clip_rect, const Color& p_modulate, RendererCanvasRender::Light* p_lights,
	RendererCanvasRender::Light* p_directional_lights,
	RSE::CanvasItemTextureFilter p_default_filter, RSE::CanvasItemTextureRepeat p_default_repeat,
	bool p_snap_2d_vertices_to_pixel, uint32_t p_canvas_cull_mask,
	RenderingServerTypes::RenderInfo* r_render_info)
{
	RENDER_TIMESTAMP("Cull CanvasItem Tree");

	// This is used to avoid passing the camera transform down the rendering
	// function calls, as it won't be used in 99% of cases, because the camera
	// transform is normally concatenated with the item global transform.
	_current_camera_transform = p_transform;

	memset(z_list, 0, z_range * sizeof(RCR::Item*));
	memset(z_last_list, 0, z_range * sizeof(RCR::Item*));

	for (int i = 0; i < p_child_item_count; i++) {
		_cull_canvas_item(p_child_items[i].item, p_transform, p_clip_rect, Color(1, 1, 1, 1), 0,
			z_list, z_last_list, nullptr, nullptr, false, p_canvas_cull_mask, Point2(), 1, nullptr);
	}

	RCR::Item* list = nullptr;
	RCR::Item* list_end = nullptr;

	for (int i = 0; i < z_range; i++) {
		if (!z_list[i]) {
			continue;
		}
		if (!list) {
			list = z_list[i];
			list_end = z_last_list[i];
		}
		else {
			list_end->next = z_list[i];
			list_end = z_last_list[i];
		}
	}

	RENDER_TIMESTAMP("Render CanvasItems");

	bool sdf_flag;
	Renderer::canvas_render->canvas_render_items(p_to_render_target, list, p_modulate, p_lights,
		p_directional_lights, p_transform, p_default_filter, p_default_repeat,
		p_snap_2d_vertices_to_pixel, sdf_flag, r_render_info);
	if (sdf_flag) {
		sdf_used = true;
	}
}

void RendererCanvasCull::_collect_ysort_children(RendererCanvasCull::Item* p_canvas_item,
	RendererCanvasCull::Item* p_material_owner, const Color& p_modulate,
	RendererCanvasCull::Item** r_items, int& r_index, int& r_ysort_children_count, int p_z,
	uint32_t p_canvas_cull_mask)
{
	int child_item_count = p_canvas_item->child_items.size();
	RendererCanvasCull::Item** child_items = p_canvas_item->child_items.ptrw();
	for (int i = 0; i < child_item_count; i++) {
		if (child_items[i]->visible) {
			if (child_items[i]->visibility_layer & p_canvas_cull_mask) {
				// To y-sort according to the item's final position, physics interpolation
				// and transform snapping need to be applied before y-sorting.
				Transform2D child_xform;
				if (!_interpolation_data.interpolation_enabled || !child_items[i]->interpolated ||
					!child_items[i]->on_interpolate_transform_list) {
					child_xform = child_items[i]->xform_curr;
				}
				else {
					real_t f = Engine::get_physics_interpolation_fraction();
					TransformInterpolator::interpolate_transform_2d(
						child_items[i]->xform_prev, child_items[i]->xform_curr, child_xform, f);
				}

				if (snapping_2d_transforms_to_pixel) {
					child_xform.columns[2] = (child_xform.columns[2] + Point2(0.5, 0.5)).floor();
				}

				r_items[r_index] = child_items[i];
				child_items[i]->ysort_xform = p_canvas_item->ysort_xform * child_xform;
				child_items[i]->material_owner =
					child_items[i]->use_parent_material ? p_material_owner : nullptr;
				child_items[i]->ysort_modulate = p_modulate;
				child_items[i]->ysort_index = r_index;
				child_items[i]->ysort_parent_abs_z_index = p_z;

				if (!child_items[i]->repeat_source) {
					child_items[i]->repeat_size = p_canvas_item->repeat_size;
					child_items[i]->repeat_times = p_canvas_item->repeat_times;
					child_items[i]->repeat_source_item = p_canvas_item->repeat_source_item;
				}

				// Y sorted canvas items are flattened into r_items. Calculate their absolute z
				// index to use when rendering r_items.
				int abs_z = 0;
				if (child_items[i]->z_relative) {
					abs_z = CLAMP(p_z + child_items[i]->z_index, RSE::CANVAS_ITEM_Z_MIN,
						RSE::CANVAS_ITEM_Z_MAX);
				}
				else {
					abs_z = child_items[i]->z_index;
				}

				r_index++;

				if (child_items[i]->sort_y) {
					_collect_ysort_children(child_items[i],
						child_items[i]->use_parent_material ? p_material_owner : child_items[i],
						p_modulate * child_items[i]->modulate, r_items, r_index,
						r_ysort_children_count, abs_z, p_canvas_cull_mask);
				}
			}
			else {
				r_ysort_children_count--;
				if (child_items[i]->sort_y) {
					r_ysort_children_count -= child_items[i]->ysort_children_count;
				}
			}
		}
	}
}

int RendererCanvasCull::_count_ysort_children(RendererCanvasCull::Item* p_canvas_item)
{
	int ysort_children_count = 0;
	int child_item_count = p_canvas_item->child_items.size();
	RendererCanvasCull::Item* const* child_items = p_canvas_item->child_items.ptr();
	for (int i = 0; i < child_item_count; i++) {
		if (child_items[i]->visible) {
			ysort_children_count++;
			if (child_items[i]->sort_y) {
				if (child_items[i]->ysort_children_count == -1) {
					child_items[i]->ysort_children_count = _count_ysort_children(child_items[i]);
				}
				ysort_children_count += child_items[i]->ysort_children_count;
			}
		}
	}
	return ysort_children_count;
}

void RendererCanvasCull::_mark_ysort_dirty(RendererCanvasCull::Item* ysort_owner)
{
	do {
		ysort_owner->ysort_children_count = -1;
		ysort_owner = canvas_item_owner.owns(ysort_owner->parent)
						  ? canvas_item_owner.get_or_null(ysort_owner->parent)
						  : nullptr;
	} while (ysort_owner && ysort_owner->sort_y);
}

void RendererCanvasCull::_attach_canvas_item_for_draw(RendererCanvasCull::Item* ci,
	RendererCanvasCull::Item* p_canvas_clip, RCR::Item** r_z_list,
	RCR::Item** r_z_last_list, const Transform2D& p_transform,
	const Rect2& p_clip_rect, Rect2 p_global_rect, const Color& p_modulate, int p_z,
	RendererCanvasCull::Item* p_material_owner, bool p_use_canvas_group,
	RCR::Item* r_canvas_group_from)
{
	if (ci->copy_back_buffer) {
		ci->copy_back_buffer->screen_rect =
			p_transform.xform(ci->copy_back_buffer->rect).intersection(p_clip_rect);
	}

	if (p_use_canvas_group) {
		int zidx = p_z - RSE::CANVAS_ITEM_Z_MIN;
		if (r_canvas_group_from == nullptr) {
			// no list before processing this item, means must put stuff in group from the beginning
			// of list.
			r_canvas_group_from = r_z_list[zidx];
		}
		else {
			// there was a list before processing, so begin group from this one.
			r_canvas_group_from = r_canvas_group_from->next;
		}

		if (r_canvas_group_from) {
			// Has a place to begin the group from!

			// compute a global rect (in global coords) for children in the same z layer
			Rect2 rect_accum;
			RCR::Item* c = r_canvas_group_from;
			while (c) {
				if (c == r_canvas_group_from) {
					rect_accum = c->global_rect_cache;
				}
				else {
					rect_accum = rect_accum.merge(c->global_rect_cache);
				}

				c = c->next;
			}

			// We have two choices now, if user has drawn something, we must assume users wants to
			// draw the "mask", so compute the size based on this. If nothing has been drawn, we
			// just take it over and draw it ourselves.
			if (ci->canvas_group->fit_empty &&
				(ci->commands == nullptr ||
					(ci->commands->next == nullptr &&
						ci->commands->type == RendererCanvasCull::Item::Command::TYPE_RECT &&
						(reinterpret_cast<RendererCanvasCull::Item::CommandRect*>(ci->commands)->flags &
							RendererCanvasRender::CANVAS_RECT_IS_GROUP)))) {
				// No commands, or sole command is the one used to draw, so we (re)create the draw
				// command.
				ci->clear();

				if (rect_accum == Rect2()) {
					rect_accum.size = Size2(1, 1);
				}

				rect_accum = rect_accum.grow(ci->canvas_group->fit_margin);

				// draw it?
				RCR::Item::CommandRect* crect =
					ci->alloc_command<RCR::Item::CommandRect>();

				crect->flags =
					RendererCanvasRender::CANVAS_RECT_IS_GROUP; // so we can recognize it later
				crect->rect = p_transform.affine_inverse().xform(rect_accum);
				crect->modulate = Color(1, 1, 1, 1);

				// the global rect is used to do the copying, so update it
				p_global_rect =
					rect_accum.grow(ci->canvas_group->clear_margin); // grow again by clear margin
				p_global_rect.position += p_clip_rect.position;
			}
			else {
				p_global_rect.position -= p_clip_rect.position;

				p_global_rect = p_global_rect.merge(rect_accum); // must use both rects for this
				p_global_rect =
					p_global_rect.grow(ci->canvas_group->clear_margin); // grow by clear margin

				p_global_rect.position += p_clip_rect.position;
			}

			// Very important that this is cleared after used in RendererCanvasRender to avoid
			// potential crashes.
			r_canvas_group_from->canvas_group_owner = ci;
		}
	}

	if (((ci->commands != nullptr || ci->visibility_notifier) &&
			p_clip_rect.intersects(p_global_rect, true)) ||
		ci->vp_render || ci->copy_back_buffer) {
		// Something to draw?

		if (ci->update_when_visible) {
			Renderer::redraw_request();
		}

		if (ci->commands != nullptr || ci->copy_back_buffer) {
			ci->final_transform =
				!ci->use_identity_transform ? p_transform : _current_camera_transform;
			ci->final_modulate = p_modulate * ci->self_modulate;
			ci->global_rect_cache = p_global_rect;
			ci->global_rect_cache.position -= p_clip_rect.position;
			ci->light_masked = false;

			int zidx = p_z - RSE::CANVAS_ITEM_Z_MIN;

			if (r_z_last_list[zidx]) {
				r_z_last_list[zidx]->next = ci;
				r_z_last_list[zidx] = ci;

			}
			else {
				r_z_list[zidx] = ci;
				r_z_last_list[zidx] = ci;
			}

			ci->z_final = p_z;

			ci->next = nullptr;
		}

		if (ci->visibility_notifier) {
			if (!ci->visibility_notifier->visible_element.in_list()) {
				visibility_notifier_list.add(&ci->visibility_notifier->visible_element);
				ci->visibility_notifier->just_visible = true;
			}

			ci->visibility_notifier->visible_in_frame = RendererCompositor::get_frame_number();
		}
	}
	else if (ci->repeat_source) {
		// If repeat source does not draw itself it still needs transform updated as its child
		// items' repeat offsets are relative to it.
		ci->final_transform = p_transform;
	}
}

void RendererCanvasCull::_cull_canvas_item(Item* p_canvas_item, const Transform2D& p_parent_xform,
	const Rect2& p_clip_rect, const Color& p_modulate, int p_z,
	RCR::Item** r_z_list, RCR::Item** r_z_last_list,
	Item* p_canvas_clip, Item* p_material_owner, bool p_is_already_y_sorted,
	uint32_t p_canvas_cull_mask, const Point2& p_repeat_size, int p_repeat_times,
	RCR::Item* p_repeat_source_item)
{
	Item* ci = p_canvas_item;

	if (!ci->visible) {
		return;
	}

	if (!(ci->visibility_layer & p_canvas_cull_mask)) {
		return;
	}

	if (ci->children_order_dirty) {
		ci->child_items.sort_custom<ItemIndexSort>();
		ci->children_order_dirty = false;
	}

	if (ci->use_parent_material && p_material_owner) {
		ci->material_owner = p_material_owner;
	}
	else {
		p_material_owner = ci;
		ci->material_owner = nullptr;
	}

	Color modulate = ci->modulate * p_modulate;

	if (modulate.a < 0.007) {
		return;
	}

	Rect2 rect = ci->get_rect();

	if (ci->visibility_notifier) {
		if (ci->visibility_notifier->area.size != Vector2()) {
			rect = rect.merge(ci->visibility_notifier->area);
		}
	}

	// Always calculate final transform as if not using identity xform.
	// This is so the expected transform is passed to children.
	// However, if use_identity_xform is set,
	// we can override the transform for rendering purposes for this item only.
	Transform2D self_xform;
	Transform2D final_xform;
	if (p_is_already_y_sorted) {
		// Y-sorted item's final transform is calculated before y-sorting,
		// and is passed as `p_parent_xform` afterwards. No need to recalculate.
		final_xform = p_parent_xform;
	}
	else {
		if (!_interpolation_data.interpolation_enabled || !ci->interpolated ||
			!ci->on_interpolate_transform_list) {
			self_xform = ci->xform_curr;
		}
		else {
			real_t f = Engine::get_physics_interpolation_fraction();
			TransformInterpolator::interpolate_transform_2d(
				ci->xform_prev, ci->xform_curr, self_xform, f);
		}

		Transform2D parent_xform = p_parent_xform;

		if (snapping_2d_transforms_to_pixel) {
			self_xform.columns[2] = (self_xform.columns[2] + Point2(0.5, 0.5)).floor();
			parent_xform.columns[2] = (parent_xform.columns[2] + Point2(0.5, 0.5)).floor();
		}

		final_xform = parent_xform * self_xform;
	}

	Point2 repeat_size = p_repeat_size;
	int repeat_times = p_repeat_times;
	RCR::Item* repeat_source_item = p_repeat_source_item;

	if (ci->repeat_source) {
		repeat_size = ci->repeat_size;
		repeat_times = ci->repeat_times;
		repeat_source_item = ci;
	}
	else {
		ci->repeat_size = repeat_size;
		ci->repeat_times = repeat_times;
		ci->repeat_source_item = repeat_source_item;
	}

	Rect2 global_rect;
	if (!p_canvas_item->use_identity_transform) {
		global_rect = final_xform.xform(rect);
	}
	else {
		global_rect = _current_camera_transform.xform(rect);
	}
	if (repeat_source_item && (repeat_size.x || repeat_size.y)) {
		// Top-left repeated rect.
		Rect2 corner_rect = global_rect;
		corner_rect.position -=
			repeat_source_item->final_transform.basis_xform((repeat_times / 2) * repeat_size);
		global_rect = corner_rect;

		// Plus top-right repeated rect.
		Size2 size_x_offset =
			repeat_source_item->final_transform.basis_xform(repeat_times * Size2(repeat_size.x, 0));
		corner_rect.position += size_x_offset;
		global_rect = global_rect.merge(corner_rect);

		// Plus bottom-right repeated rect.
		corner_rect.position +=
			repeat_source_item->final_transform.basis_xform(repeat_times * Size2(0, repeat_size.y));
		global_rect = global_rect.merge(corner_rect);

		// Plus bottom-left repeated rect.
		corner_rect.position -= size_x_offset;
		global_rect = global_rect.merge(corner_rect);
	}
	global_rect.position += p_clip_rect.position;

	int child_item_count = ci->child_items.size();
	Item** child_items = ci->child_items.ptrw();

	if (ci->clip) {
		if (p_canvas_clip != nullptr) {
			ci->final_clip_rect = p_canvas_clip->final_clip_rect.intersection(global_rect);
		}
		else {
			ci->final_clip_rect = p_clip_rect.intersection(global_rect);
		}
		if (ci->final_clip_rect.size.width < 0.5 || ci->final_clip_rect.size.height < 0.5) {
			// The clip rect area is 0, so don't draw the item.
			return;
		}
		ci->final_clip_rect.position = ci->final_clip_rect.position.round();
		ci->final_clip_rect.size = ci->final_clip_rect.size.round();
		ci->final_clip_owner = ci;

	}
	else {
		ci->final_clip_owner = p_canvas_clip;
	}

	int parent_z = p_z;
	if (ci->z_relative) {
		p_z = CLAMP(p_z + ci->z_index, RSE::CANVAS_ITEM_Z_MIN, RSE::CANVAS_ITEM_Z_MAX);
	}
	else {
		p_z = ci->z_index;
	}

	if (ci->sort_y) {
		if (!p_is_already_y_sorted) {
			if (ci->ysort_children_count == -1) {
				ci->ysort_children_count = _count_ysort_children(ci);
			}

			child_item_count = ci->ysort_children_count + 1;
			child_items = (Item**)alloca(child_item_count * sizeof(Item*));

			ci->ysort_xform = Transform2D();
			ci->ysort_modulate = Color(ci->modulate[0] ? 1 / ci->modulate[0] : 0,
				ci->modulate[1] ? 1 / ci->modulate[1] : 0,
				ci->modulate[2] ? 1 / ci->modulate[2] : 0,
				ci->modulate[3] ? 1 / ci->modulate[3] : 0);
			ci->ysort_index = 0;
			ci->ysort_parent_abs_z_index = parent_z;
			child_items[0] = ci;
			int i = 1;
			_collect_ysort_children(ci, p_material_owner, Color(1, 1, 1, 1), child_items, i,
				child_item_count, p_z, p_canvas_cull_mask);

			SortArray<Item*, ItemYSort> sorter;
			sorter.sort(child_items, child_item_count);

			for (i = 0; i < child_item_count; i++) {
				_cull_canvas_item(child_items[i], final_xform * child_items[i]->ysort_xform,
					p_clip_rect, modulate * child_items[i]->ysort_modulate,
					child_items[i]->ysort_parent_abs_z_index, r_z_list, r_z_last_list,
					(Item*)ci->final_clip_owner, (Item*)child_items[i]->material_owner, true,
					p_canvas_cull_mask, child_items[i]->repeat_size, child_items[i]->repeat_times,
					child_items[i]->repeat_source_item);
			}
		}
		else {
			RCR::Item* canvas_group_from = nullptr;
			bool use_canvas_group = ci->canvas_group != nullptr &&
									(ci->canvas_group->fit_empty || ci->commands != nullptr);
			if (use_canvas_group) {
				int zidx = p_z - RSE::CANVAS_ITEM_Z_MIN;
				canvas_group_from = r_z_last_list[zidx];
			}

			_attach_canvas_item_for_draw(ci, p_canvas_clip, r_z_list, r_z_last_list, final_xform,
				p_clip_rect, global_rect, modulate, p_z, p_material_owner, use_canvas_group,
				canvas_group_from);
		}
	}
	else {
		RCR::Item* canvas_group_from = nullptr;
		bool use_canvas_group =
			ci->canvas_group != nullptr && (ci->canvas_group->fit_empty || ci->commands != nullptr);
		if (use_canvas_group) {
			int zidx = p_z - RSE::CANVAS_ITEM_Z_MIN;
			canvas_group_from = r_z_last_list[zidx];
		}

		for (int i = 0; i < child_item_count; i++) {
			if (!child_items[i]->behind && !use_canvas_group) {
				continue;
			}
			_cull_canvas_item(child_items[i], final_xform, p_clip_rect, modulate, p_z, r_z_list,
				r_z_last_list, (Item*)ci->final_clip_owner, p_material_owner, false,
				p_canvas_cull_mask, repeat_size, repeat_times, repeat_source_item);
		}
		_attach_canvas_item_for_draw(ci, p_canvas_clip, r_z_list, r_z_last_list, final_xform,
			p_clip_rect, global_rect, modulate, p_z, p_material_owner, use_canvas_group,
			canvas_group_from);
		for (int i = 0; i < child_item_count; i++) {
			if (child_items[i]->behind || use_canvas_group) {
				continue;
			}
			_cull_canvas_item(child_items[i], final_xform, p_clip_rect, modulate, p_z, r_z_list,
				r_z_last_list, (Item*)ci->final_clip_owner, p_material_owner, false,
				p_canvas_cull_mask, repeat_size, repeat_times, repeat_source_item);
		}
	}
}

void RendererCanvasCull::render_canvas(RID p_render_target, Canvas* p_canvas,
	const Transform2D& p_transform, RendererCanvasRender::Light* p_lights,
	RendererCanvasRender::Light* p_directional_lights, const Rect2& p_clip_rect,
	RSE::CanvasItemTextureFilter p_default_filter, RSE::CanvasItemTextureRepeat p_default_repeat,
	bool p_snap_2d_transforms_to_pixel, bool p_snap_2d_vertices_to_pixel, uint32_t canvas_cull_mask,
	RenderingServerTypes::RenderInfo* r_render_info)
{
	sdf_used = false;
	snapping_2d_transforms_to_pixel = p_snap_2d_transforms_to_pixel;

	if (p_canvas->children_order_dirty) {
		p_canvas->child_items.sort();
		p_canvas->children_order_dirty = false;
	}

	int l = p_canvas->child_items.size();
	Canvas::ChildItem* ci = p_canvas->child_items.ptrw();

	_render_canvas_item_tree(p_render_target, ci, l, p_transform, p_clip_rect, p_canvas->modulate,
		p_lights, p_directional_lights, p_default_filter, p_default_repeat,
		p_snap_2d_vertices_to_pixel, canvas_cull_mask, r_render_info);
}

bool RendererCanvasCull::was_sdf_used() { return sdf_used; }

RID RendererCanvasCull::canvas_allocate() { return canvas_owner.allocate_rid(); }

void RendererCanvasCull::canvas_initialize(RID p_rid) { canvas_owner.initialize_rid(p_rid); }

void RendererCanvasCull::canvas_set_item_mirroring(
	RID p_canvas, RID p_item, const Point2& p_mirroring)
{
	Canvas* canvas = canvas_owner.get_or_null(p_canvas);
	ERR_FAIL_NULL(canvas);
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	int idx = canvas->find_item(canvas_item);
	ERR_FAIL_COND(idx == -1);

	bool is_repeat_source = (p_mirroring.x || p_mirroring.y);
	canvas_item->repeat_source = is_repeat_source;
	canvas_item->repeat_source_item = is_repeat_source ? canvas_item : nullptr;
	canvas_item->repeat_size = p_mirroring;
	canvas_item->repeat_times = 1;
}

void RendererCanvasCull::canvas_set_item_repeat(
	RID p_item, const Point2& p_repeat_size, int p_repeat_times)
{
	ERR_FAIL_COND(p_repeat_times < 0);
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	bool is_repeat_source = (p_repeat_size.x || p_repeat_size.y) && p_repeat_times;
	canvas_item->repeat_source = is_repeat_source;
	canvas_item->repeat_source_item = is_repeat_source ? canvas_item : nullptr;
	canvas_item->repeat_size = p_repeat_size;
	canvas_item->repeat_times = p_repeat_times;
}

void RendererCanvasCull::canvas_set_modulate(RID p_canvas, const Color& p_color)
{
	Canvas* canvas = canvas_owner.get_or_null(p_canvas);
	ERR_FAIL_NULL(canvas);
	canvas->modulate = p_color;
}

void RendererCanvasCull::canvas_set_disable_scale(bool p_disable) { disable_scale = p_disable; }

void RendererCanvasCull::canvas_set_parent(RID p_canvas, RID p_parent, float p_scale)
{
	Canvas* canvas = canvas_owner.get_or_null(p_canvas);
	ERR_FAIL_NULL(canvas);

	canvas->parent = p_parent;
	canvas->parent_scale = p_scale;
}

RID RendererCanvasCull::canvas_item_allocate() { return canvas_item_owner.allocate_rid(); }

void RendererCanvasCull::canvas_item_initialize(RID p_rid)
{
	canvas_item_owner.initialize_rid(p_rid);
	Item* instance = canvas_item_owner.get_or_null(p_rid);
	instance->self = p_rid;
}

void RendererCanvasCull::canvas_item_set_parent(RID p_item, RID p_parent)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	if (canvas_item->parent.is_valid()) {
		if (canvas_owner.owns(canvas_item->parent)) {
			Canvas* canvas = canvas_owner.get_or_null(canvas_item->parent);
			canvas->erase_item(canvas_item);
		}
		else if (canvas_item_owner.owns(canvas_item->parent)) {
			Item* item_owner = canvas_item_owner.get_or_null(canvas_item->parent);
			item_owner->child_items.erase(canvas_item);

			if (item_owner->sort_y) {
				_mark_ysort_dirty(item_owner);
			}
		}

		canvas_item->parent = RID();
	}

	if (p_parent.is_valid()) {
		if (canvas_owner.owns(p_parent)) {
			Canvas* canvas = canvas_owner.get_or_null(p_parent);
			Canvas::ChildItem ci;
			ci.item = canvas_item;
			canvas->child_items.push_back(ci);
			canvas->children_order_dirty = true;
		}
		else if (canvas_item_owner.owns(p_parent)) {
			Item* item_owner = canvas_item_owner.get_or_null(p_parent);
			item_owner->child_items.push_back(canvas_item);
			item_owner->children_order_dirty = true;

			if (item_owner->sort_y) {
				_mark_ysort_dirty(item_owner);
			}

		}
		else {
			ERR_FAIL_MSG("Invalid parent.");
		}
	}

	canvas_item->parent = p_parent;
}

void RendererCanvasCull::canvas_item_set_visible(RID p_item, bool p_visible)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->visible = p_visible;

	_mark_ysort_dirty(canvas_item);
}

void RendererCanvasCull::canvas_item_set_light_mask(RID p_item, int p_mask)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->light_mask = p_mask;
}

void RendererCanvasCull::canvas_item_set_transform(RID p_item, const Transform2D& p_transform)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	if (_interpolation_data.interpolation_enabled && canvas_item->interpolated) {
		if (!canvas_item->on_interpolate_transform_list) {
			_interpolation_data.canvas_item_transform_update_list_curr->push_back(p_item);
			canvas_item->on_interpolate_transform_list = true;
		}
		else {
			DEV_ASSERT(_interpolation_data.canvas_item_transform_update_list_curr->size() > 0);
		}
	}

	canvas_item->xform_curr = p_transform;
}

void RendererCanvasCull::canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->visibility_layer = p_visibility_layer;
}

uint32_t RendererCanvasCull::canvas_item_get_visibility_layer(RID p_item)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	if (!canvas_item) {
		return 0;
	}
	return canvas_item->visibility_layer;
}

void RendererCanvasCull::canvas_item_set_clip(RID p_item, bool p_clip)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->clip = p_clip;
}

void RendererCanvasCull::canvas_item_set_distance_field_mode(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->distance_field = p_enable;
}

void RendererCanvasCull::canvas_item_set_custom_rect(
	RID p_item, bool p_custom_rect, const Rect2& p_rect)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->custom_rect = p_custom_rect;
	canvas_item->rect = p_rect;
}

void RendererCanvasCull::canvas_item_set_modulate(RID p_item, const Color& p_color)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->modulate = p_color;
}

void RendererCanvasCull::canvas_item_set_self_modulate(RID p_item, const Color& p_color)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->self_modulate = p_color;
}

void RendererCanvasCull::canvas_item_set_draw_behind_parent(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->behind = p_enable;
}

void RendererCanvasCull::canvas_item_set_use_identity_transform(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->use_identity_transform = p_enable;
}

void RendererCanvasCull::canvas_item_set_update_when_visible(RID p_item, bool p_update)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->update_when_visible = p_update;
}

// Compensate the width added by the antialiasing feather by reducing the base line width.
// For line widths lower than or equal to 1.0, this is done with a multiplier,
// while line widths greater than 1.0 use a constant offset clamped to a width of 1.0.
// While the offset is not equal to `FEATHER_SIZE`, this is empirically determined
// to give a good result on various foreground/background colors.
// This method assumes the check for whether the line is antialiased
// has already been done.
float RendererCanvasCull::canvas_item_get_compensated_antialiasing_width(float p_width) const
{
	if (p_width > 0.0f) {
		if (p_width <= (FEATHER_SIZE * 2.0f + CMP_EPSILON)) {
			return p_width * 0.5f;
		}
		else if (p_width <= (FEATHER_SIZE * 4.0f + CMP_EPSILON)) {
			// Progressively lerp between the two methods (multiplier and offset).
			return Math::remap(p_width, FEATHER_SIZE * 2.0f, FEATHER_SIZE * 4.0f, p_width * 0.5f,
				p_width - FEATHER_SIZE * 0.5f);
		}
		else {
			// Use a constant offset.
			return p_width - FEATHER_SIZE * 0.5f;
		}
	}

	return p_width;
}

void RendererCanvasCull::canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
	const Color& p_color, float p_width, bool p_antialiased)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandPrimitive* line = canvas_item->alloc_command<Item::CommandPrimitive>();
	ERR_FAIL_NULL(line);

	Vector2 diff = (p_from - p_to);
	Vector2 dir = diff.orthogonal().normalized();

	if (p_antialiased) {
		p_width = canvas_item_get_compensated_antialiasing_width(p_width);
	}

	Vector2 t = dir * p_width * 0.5;

	Vector2 begin_left;
	Vector2 begin_right;
	Vector2 end_left;
	Vector2 end_right;

	if (p_width >= 0.0) {
		begin_left = p_from + t;
		begin_right = p_from - t;
		end_left = p_to + t;
		end_right = p_to - t;

		line->points[0] = begin_left;
		line->points[1] = begin_right;
		line->points[2] = end_right;
		line->points[3] = end_left;
		line->point_count = 4;
	}
	else {
		begin_left = p_from;
		begin_right = p_from;
		end_left = p_to;
		end_right = p_to;

		line->points[0] = p_from;
		line->points[1] = p_to;
		line->point_count = 2;
	}
	for (uint32_t i = 0; i < line->point_count; i++) {
		line->colors[i] = p_color;
	}

	if (p_antialiased) {
		float border_size = FEATHER_SIZE;

		if (0.0f <= p_width && p_width < 1.0f) {
			border_size *= p_width;
		}
		Vector2 dir2 = diff.normalized();

		Vector2 border = dir * border_size;
		Vector2 border2 = dir2 * border_size;

		Color transparent = Color(p_color, 0.0);

		{
			Item::CommandPrimitive* left_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(left_border);

			left_border->points[0] = begin_left;
			left_border->points[1] = begin_left + border;
			left_border->points[2] = end_left + border;
			left_border->points[3] = end_left;

			left_border->colors[0] = p_color;
			left_border->colors[1] = transparent;
			left_border->colors[2] = transparent;
			left_border->colors[3] = p_color;

			left_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* right_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(right_border);

			right_border->points[0] = begin_right;
			right_border->points[1] = begin_right - border;
			right_border->points[2] = end_right - border;
			right_border->points[3] = end_right;

			right_border->colors[0] = p_color;
			right_border->colors[1] = transparent;
			right_border->colors[2] = transparent;
			right_border->colors[3] = p_color;

			right_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_border);

			top_border->points[0] = begin_left;
			top_border->points[1] = begin_left + border2;
			top_border->points[2] = begin_right + border2;
			top_border->points[3] = begin_right;

			top_border->colors[0] = p_color;
			top_border->colors[1] = transparent;
			top_border->colors[2] = transparent;
			top_border->colors[3] = p_color;

			top_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_border);

			bottom_border->points[0] = end_left;
			bottom_border->points[1] = end_left - border2;
			bottom_border->points[2] = end_right - border2;
			bottom_border->points[3] = end_right;

			bottom_border->colors[0] = p_color;
			bottom_border->colors[1] = transparent;
			bottom_border->colors[2] = transparent;
			bottom_border->colors[3] = p_color;

			bottom_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_left_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_left_corner);

			top_left_corner->points[0] = begin_left;
			top_left_corner->points[1] = begin_left + border2;
			top_left_corner->points[2] = begin_left + border + border2;
			top_left_corner->points[3] = begin_left + border;

			top_left_corner->colors[0] = p_color;
			top_left_corner->colors[1] = transparent;
			top_left_corner->colors[2] = transparent;
			top_left_corner->colors[3] = transparent;

			top_left_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_right_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_right_corner);

			top_right_corner->points[0] = begin_right;
			top_right_corner->points[1] = begin_right + border2;
			top_right_corner->points[2] = begin_right - border + border2;
			top_right_corner->points[3] = begin_right - border;

			top_right_corner->colors[0] = p_color;
			top_right_corner->colors[1] = transparent;
			top_right_corner->colors[2] = transparent;
			top_right_corner->colors[3] = transparent;

			top_right_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_left_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_left_corner);

			bottom_left_corner->points[0] = end_left;
			bottom_left_corner->points[1] = end_left - border2;
			bottom_left_corner->points[2] = end_left + border - border2;
			bottom_left_corner->points[3] = end_left + border;

			bottom_left_corner->colors[0] = p_color;
			bottom_left_corner->colors[1] = transparent;
			bottom_left_corner->colors[2] = transparent;
			bottom_left_corner->colors[3] = transparent;

			bottom_left_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_right_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_right_corner);

			bottom_right_corner->points[0] = end_right;
			bottom_right_corner->points[1] = end_right - border2;
			bottom_right_corner->points[2] = end_right - border - border2;
			bottom_right_corner->points[3] = end_right - border;

			bottom_right_corner->colors[0] = p_color;
			bottom_right_corner->colors[1] = transparent;
			bottom_right_corner->colors[2] = transparent;
			bottom_right_corner->colors[3] = transparent;

			bottom_right_corner->point_count = 4;
		}
	}
}

static Vector2 compute_polyline_segment_dir(
	const Vector<Point2>& p_points, int p_index, const Vector2& p_prev_segment_dir)
{
	int point_count = p_points.size();

	bool is_last_point = (p_index == point_count - 1);

	Vector2 segment_dir;

	if (is_last_point) {
		segment_dir = p_prev_segment_dir;
	}
	else {
		segment_dir = (p_points[p_index + 1] - p_points[p_index]).normalized();

		if (segment_dir.is_zero_approx()) {
			segment_dir = p_prev_segment_dir;
		}
	}

	return segment_dir;
}

static Vector2 compute_polyline_edge_offset_clamped(
	const Vector2& p_segment_dir, const Vector2& p_prev_segment_dir)
{
	Vector2 bisector;
	float length = 1.0f;

	bisector =
		(p_prev_segment_dir * p_segment_dir.length() - p_segment_dir * p_prev_segment_dir.length())
			.normalized();

	float angle = std::atan2(bisector.cross(p_prev_segment_dir), bisector.dot(p_prev_segment_dir));
	float sin_angle = std::sin(angle);

	if (!Math::is_zero_approx(sin_angle) && !p_segment_dir.is_equal_approx(p_prev_segment_dir)) {
		length = 1.0f / sin_angle;
		length = CLAMP(length, -3.0f, 3.0f);
	}
	else {
		bisector = p_segment_dir.orthogonal();
	}

	if (bisector.is_zero_approx()) {
		bisector = p_segment_dir.orthogonal();
	}

	return bisector * length;
}

void RendererCanvasCull::canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	ERR_FAIL_COND(p_points.size() < 2);
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Color color = Color(1, 1, 1, 1);

	Vector<int> indices;
	int point_count = p_points.size();

	Item::CommandPolygon* pline = canvas_item->alloc_command<Item::CommandPolygon>();
	ERR_FAIL_NULL(pline);

	if (p_antialiased) {
		p_width = canvas_item_get_compensated_antialiasing_width(p_width);
	}

	if (p_width < 0) {
		if (p_antialiased) {
			WARN_PRINT("Antialiasing is not supported for thin polylines drawn using line strips "
					   "(`p_width < 0`).");
		}

		pline->primitive = RSE::PRIMITIVE_LINE_STRIP;

		if (p_colors.size() == 1 || p_colors.size() == point_count) {
			pline->polygon.create(indices, p_points, p_colors);
		}
		else {
			Vector<Color> colors;
			if (p_colors.is_empty()) {
				colors.push_back(color);
			}
			else {
				colors.resize(point_count);
				Color* colors_ptr = colors.ptrw();
				for (int i = 0; i < point_count; i++) {
					if (i < p_colors.size()) {
						color = p_colors[i];
					}
					colors_ptr[i] = color;
				}
			}
			pline->polygon.create(indices, p_points, colors);
		}
		return;
	}

	int polyline_point_count = point_count * 2;

	bool loop = p_points[0].is_equal_approx(p_points[point_count - 1]);
	Vector2 first_segment_dir;
	Vector2 last_segment_dir;

	// Search for first non-zero vector between two segments.
	for (int i = 1; i < point_count; i++) {
		first_segment_dir = (p_points[i] - p_points[i - 1]).normalized();

		if (!first_segment_dir.is_zero_approx()) {
			break;
		}
	}

	// Search for last non-zero vector between two segments.
	for (int i = point_count - 1; i >= 1; i--) {
		last_segment_dir = (p_points[i] - p_points[i - 1]).normalized();

		if (!last_segment_dir.is_zero_approx()) {
			break;
		}
	}

	PackedColorArray colors;
	PackedVector2Array points;

	// Additional 2+2 vertices to antialias begin+end of the middle triangle strip.
	colors.resize(polyline_point_count + ((p_antialiased && !loop) ? 4 : 0));
	points.resize(polyline_point_count + ((p_antialiased && !loop) ? 4 : 0));

	Vector2* points_ptr = points.ptrw();
	Color* colors_ptr = colors.ptrw();

	if (p_antialiased) {
		float border_size = FEATHER_SIZE;
		if (p_width < 1.0f) {
			border_size *= p_width;
		}
		Color color2 = Color(1, 1, 1, 0);

		Item::CommandPolygon* pline_left = canvas_item->alloc_command<Item::CommandPolygon>();
		ERR_FAIL_NULL(pline_left);

		Item::CommandPolygon* pline_right = canvas_item->alloc_command<Item::CommandPolygon>();
		ERR_FAIL_NULL(pline_right);

		PackedColorArray colors_left;
		PackedVector2Array points_left;

		PackedColorArray colors_right;
		PackedVector2Array points_right;

		// 2+2 additional vertices for begin+end corners.
		// 1 additional vertex to swap the orientation of the triangles within the end corner's
		// quad.
		colors_left.resize(polyline_point_count + (loop ? 0 : 5));
		points_left.resize(polyline_point_count + (loop ? 0 : 5));

		colors_right.resize(polyline_point_count + (loop ? 0 : 5));
		points_right.resize(polyline_point_count + (loop ? 0 : 5));

		Color* colors_left_ptr = colors_left.ptrw();
		Vector2* points_left_ptr = points_left.ptrw();

		Vector2* points_right_ptr = points_right.ptrw();
		Color* colors_right_ptr = colors_right.ptrw();

		Vector2 prev_segment_dir;
		for (int i = 0; i < point_count; i++) {
			bool is_first_point = (i == 0);
			bool is_last_point = (i == point_count - 1);

			Vector2 segment_dir = compute_polyline_segment_dir(p_points, i, prev_segment_dir);
			if (is_first_point && loop) {
				prev_segment_dir = last_segment_dir;
			}
			else if (is_last_point && loop) {
				prev_segment_dir = first_segment_dir;
			}

			Vector2 base_edge_offset;
			if (is_first_point && !loop) {
				base_edge_offset = first_segment_dir.orthogonal();
			}
			else if (is_last_point && !loop) {
				base_edge_offset = last_segment_dir.orthogonal();
			}
			else {
				base_edge_offset =
					compute_polyline_edge_offset_clamped(segment_dir, prev_segment_dir);
			}

			Vector2 edge_offset = base_edge_offset * (p_width * 0.5f);
			Vector2 border = base_edge_offset * border_size;
			Vector2 pos = p_points[i];

			int j = i * 2 + (loop ? 0 : 2);

			points_ptr[j + 0] = pos + edge_offset;
			points_ptr[j + 1] = pos - edge_offset;

			points_left_ptr[j + 0] = pos + edge_offset;
			points_left_ptr[j + 1] = pos + edge_offset + border;

			points_right_ptr[j + 0] = pos - edge_offset;
			points_right_ptr[j + 1] = pos - edge_offset - border;

			if (i < p_colors.size()) {
				color = p_colors[i];
				color2 = Color(color.r, color.g, color.b, 0);
			}

			colors_ptr[j + 0] = color;
			colors_ptr[j + 1] = color;

			colors_left_ptr[j + 0] = color;
			colors_left_ptr[j + 1] = color2;

			colors_right_ptr[j + 0] = color;
			colors_right_ptr[j + 1] = color2;

			if (is_first_point && !loop) {
				Vector2 begin_border = -segment_dir * border_size;

				points_ptr[0] = pos + edge_offset + begin_border;
				points_ptr[1] = pos - edge_offset + begin_border;

				colors_ptr[0] = color2;
				colors_ptr[1] = color2;

				points_left_ptr[0] = pos + edge_offset + begin_border;
				points_left_ptr[1] = pos + edge_offset + begin_border + border;

				colors_left_ptr[0] = color2;
				colors_left_ptr[1] = color2;

				points_right_ptr[0] = pos - edge_offset + begin_border;
				points_right_ptr[1] = pos - edge_offset + begin_border - border;

				colors_right_ptr[0] = color2;
				colors_right_ptr[1] = color2;
			}

			if (is_last_point && !loop) {
				Vector2 end_border = prev_segment_dir * border_size;
				int end_index = polyline_point_count + 2;

				points_ptr[end_index + 0] = pos + edge_offset + end_border;
				points_ptr[end_index + 1] = pos - edge_offset + end_border;

				colors_ptr[end_index + 0] = color2;
				colors_ptr[end_index + 1] = color2;

				// Swap orientation of the triangles within both end corner quads so the visual
				// seams between triangles goes from the edge corner. Done by going back to the edge
				// corner (1 additional vertex / zero-area triangle per left/right corner).
				points_left_ptr[end_index + 0] = pos + edge_offset;
				points_left_ptr[end_index + 1] = pos + edge_offset + end_border + border;
				points_left_ptr[end_index + 2] = pos + edge_offset + end_border;

				colors_left_ptr[end_index + 0] = color;
				colors_left_ptr[end_index + 1] = color2;
				colors_left_ptr[end_index + 2] = color2;

				points_right_ptr[end_index + 0] = pos - edge_offset;
				points_right_ptr[end_index + 1] = pos - edge_offset + end_border - border;
				points_right_ptr[end_index + 2] = pos - edge_offset + end_border;

				colors_right_ptr[end_index + 0] = color;
				colors_right_ptr[end_index + 1] = color2;
				colors_right_ptr[end_index + 2] = color2;
			}

			prev_segment_dir = segment_dir;
		}

		pline_left->primitive = RSE::PRIMITIVE_TRIANGLE_STRIP;
		pline_left->polygon.create(indices, points_left, colors_left);

		pline_right->primitive = RSE::PRIMITIVE_TRIANGLE_STRIP;
		pline_right->polygon.create(indices, points_right, colors_right);
	}
	else {
		// Makes a single triangle strip for drawing the line.

		Vector2 prev_segment_dir;
		for (int i = 0; i < point_count; i++) {
			bool is_first_point = (i == 0);
			bool is_last_point = (i == point_count - 1);

			Vector2 segment_dir = compute_polyline_segment_dir(p_points, i, prev_segment_dir);
			if (is_first_point && loop) {
				prev_segment_dir = last_segment_dir;
			}
			else if (is_last_point && loop) {
				prev_segment_dir = first_segment_dir;
			}

			Vector2 base_edge_offset;
			if (is_first_point && !loop) {
				base_edge_offset = first_segment_dir.orthogonal();
			}
			else if (is_last_point && !loop) {
				base_edge_offset = last_segment_dir.orthogonal();
			}
			else {
				base_edge_offset =
					compute_polyline_edge_offset_clamped(segment_dir, prev_segment_dir);
			}

			Vector2 edge_offset = base_edge_offset * (p_width * 0.5f);
			Vector2 pos = p_points[i];

			points_ptr[i * 2 + 0] = pos + edge_offset;
			points_ptr[i * 2 + 1] = pos - edge_offset;

			if (i < p_colors.size()) {
				color = p_colors[i];
			}

			colors_ptr[i * 2 + 0] = color;
			colors_ptr[i * 2 + 1] = color;

			prev_segment_dir = segment_dir;
		}
	}

	pline->primitive = RSE::PRIMITIVE_TRIANGLE_STRIP;
	pline->polygon.create(indices, points, colors);
}

void RendererCanvasCull::canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	ERR_FAIL_COND(p_points.is_empty() || p_points.size() % 2 != 0);
	ERR_FAIL_COND(p_colors.size() != 1 && p_colors.size() * 2 != p_points.size());

	// We don't need to compensate width for antialiasing here,
	// since the width is already compensated in `canvas_item_add_line()`.

	// TODO: `canvas_item_add_line`(`multiline`, `polyline`) share logic, should factor out.
	if (p_width < 0) {
		if (p_antialiased) {
			WARN_PRINT("Antialiasing is not supported for thin multilines drawn using line strips "
					   "(`p_width < 0`).");
		}
		Item* canvas_item = canvas_item_owner.get_or_null(p_item);
		ERR_FAIL_NULL(canvas_item);

		Vector<Color> colors;
		if (p_colors.size() == 1) {
			colors = p_colors;
		}
		else { //} else if (p_colors.size() << 1 == p_points.size()) {
			colors.resize(p_points.size());
			Color* colors_ptr = colors.ptrw();
			for (int i = 0; i < p_colors.size(); i++) {
				Color color = p_colors[i];
				colors_ptr[i * 2 + 0] = color;
				colors_ptr[i * 2 + 1] = color;
			}
		}

		Item::CommandPolygon* pline = canvas_item->alloc_command<Item::CommandPolygon>();
		ERR_FAIL_NULL(pline);
		pline->primitive = RSE::PRIMITIVE_LINES;
		pline->polygon.create(Vector<int>(), p_points, colors);
	}
	else {
		if (p_colors.size() == 1) {
			Color color = p_colors[0];
			for (int i = 0; i < p_points.size() >> 1; i++) {
				Vector2 from = p_points[i * 2 + 0];
				Vector2 to = p_points[i * 2 + 1];

				canvas_item_add_line(p_item, from, to, color, p_width, p_antialiased);
			}
		}
		else { //} else if (p_colors.size() << 1 == p_points.size()) {
			for (int i = 0; i < p_colors.size(); i++) {
				Color color = p_colors[i];
				Vector2 from = p_points[i * 2 + 0];
				Vector2 to = p_points[i * 2 + 1];

				canvas_item_add_line(p_item, from, to, color, p_width, p_antialiased);
			}
		}
	}
}

void RendererCanvasCull::canvas_item_add_rect(
	RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	// Adjust the rectangle size to account for the antialiasing width.
	const Rect2& rect_adjusted = p_antialiased ? p_rect.grow(-FEATHER_SIZE * 0.25f) : p_rect;

	Item::CommandRect* rect = canvas_item->alloc_command<Item::CommandRect>();
	ERR_FAIL_NULL(rect);
	rect->modulate = p_color;
	rect->rect = rect_adjusted;

	// Add feathers.
	if (p_antialiased) {
		float border_size = FEATHER_SIZE;

		const real_t size = MIN(rect_adjusted.size.width, rect_adjusted.size.height);
		if (0.0f <= size && size < 1.0f) {
			border_size *= size;
		}

		const Vector2 vec_down = Vector2(0.0f, rect_adjusted.size.height);
		const Vector2 vec_right = Vector2(rect_adjusted.size.width, 0.0f);

		const Vector2 begin_left = rect_adjusted.position;
		const Vector2 begin_right = rect_adjusted.position + vec_down;
		const Vector2 end_left = rect_adjusted.position + vec_right;
		const Vector2 end_right = rect_adjusted.position + rect_adjusted.size;

		const Vector2 dir = Vector2(0.0f, -1.0f);
		const Vector2 dir2 = Vector2(-1.0f, 0.0f);
		const Vector2 border = dir * border_size;
		const Vector2 border2 = dir2 * border_size;

		Color transparent = Color(p_color, 0.0);

		{
			Item::CommandPrimitive* left_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(left_border);

			left_border->points[0] = begin_left;
			left_border->points[1] = begin_left + border;
			left_border->points[2] = end_left + border;
			left_border->points[3] = end_left;

			left_border->colors[0] = p_color;
			left_border->colors[1] = transparent;
			left_border->colors[2] = transparent;
			left_border->colors[3] = p_color;

			left_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* right_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(right_border);

			right_border->points[0] = begin_right;
			right_border->points[1] = begin_right - border;
			right_border->points[2] = end_right - border;
			right_border->points[3] = end_right;

			right_border->colors[0] = p_color;
			right_border->colors[1] = transparent;
			right_border->colors[2] = transparent;
			right_border->colors[3] = p_color;

			right_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_border);

			top_border->points[0] = begin_left;
			top_border->points[1] = begin_left + border2;
			top_border->points[2] = begin_right + border2;
			top_border->points[3] = begin_right;

			top_border->colors[0] = p_color;
			top_border->colors[1] = transparent;
			top_border->colors[2] = transparent;
			top_border->colors[3] = p_color;

			top_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_border =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_border);

			bottom_border->points[0] = end_left;
			bottom_border->points[1] = end_left - border2;
			bottom_border->points[2] = end_right - border2;
			bottom_border->points[3] = end_right;

			bottom_border->colors[0] = p_color;
			bottom_border->colors[1] = transparent;
			bottom_border->colors[2] = transparent;
			bottom_border->colors[3] = p_color;

			bottom_border->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_left_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_left_corner);

			top_left_corner->points[0] = begin_left;
			top_left_corner->points[1] = begin_left + border2;
			top_left_corner->points[2] = begin_left + border + border2;
			top_left_corner->points[3] = begin_left + border;

			top_left_corner->colors[0] = p_color;
			top_left_corner->colors[1] = transparent;
			top_left_corner->colors[2] = transparent;
			top_left_corner->colors[3] = transparent;

			top_left_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* top_right_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(top_right_corner);

			top_right_corner->points[0] = begin_right;
			top_right_corner->points[1] = begin_right + border2;
			top_right_corner->points[2] = begin_right - border + border2;
			top_right_corner->points[3] = begin_right - border;

			top_right_corner->colors[0] = p_color;
			top_right_corner->colors[1] = transparent;
			top_right_corner->colors[2] = transparent;
			top_right_corner->colors[3] = transparent;

			top_right_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_left_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_left_corner);

			bottom_left_corner->points[0] = end_left;
			bottom_left_corner->points[1] = end_left - border2;
			bottom_left_corner->points[2] = end_left + border - border2;
			bottom_left_corner->points[3] = end_left + border;

			bottom_left_corner->colors[0] = p_color;
			bottom_left_corner->colors[1] = transparent;
			bottom_left_corner->colors[2] = transparent;
			bottom_left_corner->colors[3] = transparent;

			bottom_left_corner->point_count = 4;
		}
		{
			Item::CommandPrimitive* bottom_right_corner =
				canvas_item->alloc_command<Item::CommandPrimitive>();
			ERR_FAIL_NULL(bottom_right_corner);

			bottom_right_corner->points[0] = end_right;
			bottom_right_corner->points[1] = end_right - border2;
			bottom_right_corner->points[2] = end_right - border - border2;
			bottom_right_corner->points[3] = end_right - border;

			bottom_right_corner->colors[0] = p_color;
			bottom_right_corner->colors[1] = transparent;
			bottom_right_corner->colors[2] = transparent;
			bottom_right_corner->colors[3] = transparent;

			bottom_right_corner->point_count = 4;
		}
	}
}

void RendererCanvasCull::canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
	float p_minor, const Color& p_color, bool p_antialiased)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	static const int ellipse_segments = 64;

	float major = p_major;
	float minor = p_minor;
	if (p_antialiased) {
		// Adjust the diameter to account for the antialiasing width.
		major = MAX(0.0f, major - FEATHER_SIZE * 0.25f);
		minor = MAX(0.0f, minor - FEATHER_SIZE * 0.25f);
	}

	{
		Item::CommandPolygon* ellipse = canvas_item->alloc_command<Item::CommandPolygon>();
		ERR_FAIL_NULL(ellipse);

		ellipse->primitive = RSE::PRIMITIVE_TRIANGLES;

		Vector<int> indices;
		Vector<Vector2> points;

		points.resize(ellipse_segments + 2);
		Vector2* points_ptr = points.ptrw();

		// Store ellipse center in the last point.
		points_ptr[ellipse_segments + 1] = p_pos;

		const real_t ellipse_point_step = Math::TAU / ellipse_segments;

		for (int i = 0; i < ellipse_segments + 1; i++) {
			float angle = i * ellipse_point_step;
			points_ptr[i].x = Math::cos(angle) * major;
			points_ptr[i].y = Math::sin(angle) * minor;
			points_ptr[i] += p_pos;
		}

		indices.resize(ellipse_segments * 3);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < ellipse_segments; i++) {
			indices_ptr[i * 3 + 0] = ellipse_segments + 1;
			indices_ptr[i * 3 + 1] = i;
			indices_ptr[i * 3 + 2] = i + 1;
		}

		Vector<Color> color;
		color.push_back(p_color);
		ellipse->polygon.create(indices, points, color);
	}

	if (p_antialiased) {
		float border_size = FEATHER_SIZE;

		const float max_axis = fmax(major, minor) * 2.0f;
		if (0.0f <= max_axis && max_axis < 1.0f) {
			border_size *= max_axis * 0.5f;
		}

		Item::CommandPolygon* feather = canvas_item->alloc_command<Item::CommandPolygon>();
		ERR_FAIL_NULL(feather);
		feather->primitive = RSE::PRIMITIVE_TRIANGLE_STRIP;

		Color transparent = Color(p_color, 0.0);

		Vector<int> indices;
		Vector<Color> colors;
		Vector<Vector2> points;

		points.resize(2 * ellipse_segments + 2);
		colors.resize(2 * ellipse_segments + 2);

		const real_t ellipse_point_step = Math::TAU / ellipse_segments;

		Vector2* points_ptr = points.ptrw();
		Color* colors_ptr = colors.ptrw();

		for (int i = 0; i < ellipse_segments + 1; i++) {
			const float angle = i * ellipse_point_step;
			const float c = Math::cos(angle);
			const float s = Math::sin(angle);

			points_ptr[i * 2].x = c * major;
			points_ptr[i * 2].y = s * minor;
			points_ptr[i * 2] += p_pos;

			points_ptr[i * 2 + 1].x = c * (major + border_size);
			points_ptr[i * 2 + 1].y = s * (minor + border_size);
			points_ptr[i * 2 + 1] += p_pos;

			colors_ptr[i * 2] = p_color;
			colors_ptr[i * 2 + 1] = transparent;
		}

		feather->polygon.create(indices, points, colors);
	}
}

void RendererCanvasCull::canvas_item_add_circle(
	RID p_item, const Point2& p_pos, float p_radius, const Color& p_color, bool p_antialiased)
{
	canvas_item_add_ellipse(p_item, p_pos, p_radius, p_radius, p_color, p_antialiased);
}

void RendererCanvasCull::canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect,
	RID p_texture, bool p_tile, const Color& p_modulate, bool p_transpose)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandRect* rect = canvas_item->alloc_command<Item::CommandRect>();
	ERR_FAIL_NULL(rect);
	rect->modulate = p_modulate;
	rect->rect = p_rect;
	rect->flags = 0;
	if (p_tile) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_TILE;
		rect->flags |= RendererCanvasRender::CANVAS_RECT_REGION;
		rect->source = Rect2(0, 0, Math::abs(p_rect.size.width), Math::abs(p_rect.size.height));
	}

	if (p_rect.size.x < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->rect.size.x = -rect->rect.size.x;
	}
	if (p_rect.size.y < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->rect.size.y = -rect->rect.size.y;
	}
	if (p_transpose) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_TRANSPOSE;
		SWAP(rect->rect.size.x, rect->rect.size.y);
	}

	rect->texture = p_texture;
}

void RendererCanvasCull::canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate, int p_outline_size,
	float p_px_range, float p_scale)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandRect* rect = canvas_item->alloc_command<Item::CommandRect>();
	ERR_FAIL_NULL(rect);
	rect->modulate = p_modulate;
	rect->rect = p_rect;

	rect->texture = p_texture;

	rect->source = p_src_rect;
	rect->flags = RendererCanvasRender::CANVAS_RECT_REGION | RendererCanvasRender::CANVAS_RECT_MSDF;

	if (p_rect.size.x < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->rect.size.x = -rect->rect.size.x;
	}
	if (p_src_rect.size.x < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->source.size.x = -rect->source.size.x;
	}
	if (p_rect.size.y < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->rect.size.y = -rect->rect.size.y;
	}
	if (p_src_rect.size.y < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->source.size.y = -rect->source.size.y;
	}
	rect->outline = (float)p_outline_size / p_scale / 4.0;
	rect->px_range = p_px_range;
}

void RendererCanvasCull::canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandRect* rect = canvas_item->alloc_command<Item::CommandRect>();
	ERR_FAIL_NULL(rect);
	rect->modulate = p_modulate;
	rect->rect = p_rect;

	rect->texture = p_texture;

	rect->source = p_src_rect;
	rect->flags = RendererCanvasRender::CANVAS_RECT_REGION | RendererCanvasRender::CANVAS_RECT_LCD;

	if (p_rect.size.x < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->rect.size.x = -rect->rect.size.x;
	}
	if (p_src_rect.size.x < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->source.size.x = -rect->source.size.x;
	}
	if (p_rect.size.y < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->rect.size.y = -rect->rect.size.y;
	}
	if (p_src_rect.size.y < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->source.size.y = -rect->source.size.y;
	}
}

void RendererCanvasCull::canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose,
	bool p_clip_uv)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandRect* rect = canvas_item->alloc_command<Item::CommandRect>();
	ERR_FAIL_NULL(rect);
	rect->modulate = p_modulate;
	rect->rect = p_rect;

	rect->texture = p_texture;

	rect->source = p_src_rect;
	rect->flags = RendererCanvasRender::CANVAS_RECT_REGION;

	if (p_rect.size.x < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->rect.size.x = -rect->rect.size.x;
	}
	if (p_src_rect.size.x < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_H;
		rect->source.size.x = -rect->source.size.x;
	}
	if (p_rect.size.y < 0) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->rect.size.y = -rect->rect.size.y;
	}
	if (p_src_rect.size.y < 0) {
		rect->flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_V;
		rect->source.size.y = -rect->source.size.y;
	}

	if (p_transpose) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_TRANSPOSE;
		SWAP(rect->rect.size.x, rect->rect.size.y);
	}

	if (p_clip_uv) {
		rect->flags |= RendererCanvasRender::CANVAS_RECT_CLIP_UV;
	}
}

void RendererCanvasCull::canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect,
	const Rect2& p_source, RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
	RSE::NinePatchAxisMode p_x_axis_mode, RSE::NinePatchAxisMode p_y_axis_mode, bool p_draw_center,
	const Color& p_modulate)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandNinePatch* style = canvas_item->alloc_command<Item::CommandNinePatch>();
	ERR_FAIL_NULL(style);

	style->texture = p_texture;

	style->rect = p_rect;
	style->source = p_source;
	style->draw_center = p_draw_center;
	style->color = p_modulate;
	style->margin[SIDE_LEFT] = p_topleft.x;
	style->margin[SIDE_TOP] = p_topleft.y;
	style->margin[SIDE_RIGHT] = p_bottomright.x;
	style->margin[SIDE_BOTTOM] = p_bottomright.y;
	style->axis_x = p_x_axis_mode;
	style->axis_y = p_y_axis_mode;
}

void RendererCanvasCull::canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	uint32_t pc = p_points.size();
	ERR_FAIL_COND(pc == 0 || pc > 4);

	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandPrimitive* prim = canvas_item->alloc_command<Item::CommandPrimitive>();
	ERR_FAIL_NULL(prim);

	for (int i = 0; i < p_points.size(); i++) {
		prim->points[i] = p_points[i];
		if (i < p_uvs.size()) {
			prim->uvs[i] = p_uvs[i];
		}
		if (i < p_colors.size()) {
			prim->colors[i] = p_colors[i];
		}
		else if (p_colors.size()) {
			prim->colors[i] = p_colors[0];
		}
		else {
			prim->colors[i] = Color(1, 1, 1, 1);
		}
	}

	prim->point_count = p_points.size();

	prim->texture = p_texture;
}

void RendererCanvasCull::canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
#ifdef DEBUG_ENABLED
	int pointcount = p_points.size();
	ERR_FAIL_COND(pointcount < 3);
	int color_size = p_colors.size();
	int uv_size = p_uvs.size();
	ERR_FAIL_COND(color_size != 0 && color_size != 1 && color_size != pointcount);
	ERR_FAIL_COND(uv_size != 0 && (uv_size != pointcount));
#endif
	Vector<int> indices = Geometry2D::triangulate_polygon(p_points);
	ERR_FAIL_COND_MSG(indices.is_empty(), "Invalid polygon data, triangulation failed.");

	Item::CommandPolygon* polygon = canvas_item->alloc_command<Item::CommandPolygon>();
	ERR_FAIL_NULL(polygon);
	polygon->primitive = RSE::PRIMITIVE_TRIANGLES;
	polygon->texture = p_texture;
	polygon->polygon.create(indices, p_points, p_colors, p_uvs);
}

void RendererCanvasCull::canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones, const Vector<float>& p_weights, RID p_texture, int p_count)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	int vertex_count = p_points.size();
	ERR_FAIL_COND(vertex_count == 0);
	ERR_FAIL_COND(!p_colors.is_empty() && p_colors.size() != vertex_count && p_colors.size() != 1);
	ERR_FAIL_COND(!p_uvs.is_empty() && p_uvs.size() != vertex_count);
	ERR_FAIL_COND(!p_bones.is_empty() && p_bones.size() != vertex_count * 4);
	ERR_FAIL_COND(!p_weights.is_empty() && p_weights.size() != vertex_count * 4);

	Item::CommandPolygon* polygon = canvas_item->alloc_command<Item::CommandPolygon>();
	ERR_FAIL_NULL(polygon);

	polygon->texture = p_texture;

	polygon->polygon.create(p_indices, p_points, p_colors, p_uvs, p_bones, p_weights, p_count);

	polygon->primitive = RSE::PRIMITIVE_TRIANGLES;
}

void RendererCanvasCull::canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandTransform* tr = canvas_item->alloc_command<Item::CommandTransform>();
	ERR_FAIL_NULL(tr);
	tr->xform = p_transform;
}

void RendererCanvasCull::canvas_item_add_mesh(RID p_item, const RID& p_mesh,
	const Transform2D& p_transform, const Color& p_modulate, RID p_texture)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	ERR_FAIL_COND(!p_mesh.is_valid());

	Item::CommandMesh* m = canvas_item->alloc_command<Item::CommandMesh>();
	ERR_FAIL_NULL(m);
	m->mesh = p_mesh;
	if (canvas_item->skeleton.is_valid()) {
		m->mesh_instance = RendererMeshStorage::mesh_instance_create(p_mesh);
		RendererMeshStorage::mesh_instance_set_skeleton(m->mesh_instance, canvas_item->skeleton);
	}

	m->texture = p_texture;

	m->transform = p_transform;
	m->modulate = p_modulate;
}

void RendererCanvasCull::canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandParticles* part = canvas_item->alloc_command<Item::CommandParticles>();
	ERR_FAIL_NULL(part);
	part->particles = p_particles;

	part->texture = p_texture;

	// take the chance and request processing for them, at least once until they become visible
	// again
	Renderer::particles_storage->particles_request_process(p_particles);
}

void RendererCanvasCull::canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandMultiMesh* mm = canvas_item->alloc_command<Item::CommandMultiMesh>();
	ERR_FAIL_NULL(mm);
	mm->multimesh = p_mesh;

	mm->texture = p_texture;
}

void RendererCanvasCull::canvas_item_add_clip_ignore(RID p_item, bool p_ignore)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandClipIgnore* ci = canvas_item->alloc_command<Item::CommandClipIgnore>();
	ERR_FAIL_NULL(ci);
	ci->ignore = p_ignore;
}

void RendererCanvasCull::canvas_item_add_animation_slice(RID p_item, double p_animation_length,
	double p_slice_begin, double p_slice_end, double p_offset)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	Item::CommandAnimationSlice* as = canvas_item->alloc_command<Item::CommandAnimationSlice>();
	ERR_FAIL_NULL(as);
	as->animation_length = p_animation_length;
	as->slice_begin = p_slice_begin;
	as->slice_end = p_slice_end;
	as->offset = p_offset;
}

void RendererCanvasCull::canvas_item_set_sort_children_by_y(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->sort_y = p_enable;

	_mark_ysort_dirty(canvas_item);
}

void RendererCanvasCull::canvas_item_set_z_index(RID p_item, int p_z)
{
	ERR_FAIL_COND(p_z < RSE::CANVAS_ITEM_Z_MIN || p_z > RSE::CANVAS_ITEM_Z_MAX);

	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->z_index = p_z;
}

void RendererCanvasCull::canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->z_relative = p_enable;
}

void RendererCanvasCull::canvas_item_attach_skeleton(RID p_item, RID p_skeleton)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	if (canvas_item->skeleton == p_skeleton) {
		return;
	}
	canvas_item->skeleton = p_skeleton;

	Item::Command* c = canvas_item->commands;

	while (c) {
		if (c->type == Item::Command::TYPE_MESH) {
			Item::CommandMesh* cm = reinterpret_cast<Item::CommandMesh*>(c);
			if (canvas_item->skeleton.is_valid()) {
				if (cm->mesh_instance.is_null()) {
					cm->mesh_instance = RendererMeshStorage::mesh_instance_create(cm->mesh);
				}
				RendererMeshStorage::mesh_instance_set_skeleton(
					cm->mesh_instance, canvas_item->skeleton);
			}
			else {
				if (cm->mesh_instance.is_valid()) {
					RendererMeshStorage::mesh_instance_free(cm->mesh_instance);
					cm->mesh_instance = RID();
				}
			}
		}
		c = c->next;
	}
}

void RendererCanvasCull::canvas_item_set_copy_to_backbuffer(
	RID p_item, bool p_enable, const Rect2& p_rect)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	if (p_enable && (canvas_item->copy_back_buffer == nullptr)) {
		canvas_item->copy_back_buffer = memnew(RCR::Item::CopyBackBuffer);
	}
	if (!p_enable && (canvas_item->copy_back_buffer != nullptr)) {
		memdelete(canvas_item->copy_back_buffer);
		canvas_item->copy_back_buffer = nullptr;
	}

	if (p_enable) {
		canvas_item->copy_back_buffer->rect = p_rect;
		canvas_item->copy_back_buffer->full = p_rect == Rect2();
	}
}

void RendererCanvasCull::canvas_item_clear(RID p_item)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->clear();

#ifdef DEBUG_ENABLED
	if (debug_redraw) {
		canvas_item->debug_redraw_time = debug_redraw_time;
	}
#endif
}

void RendererCanvasCull::canvas_item_set_draw_index(RID p_item, int p_index)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->index = p_index;

	if (canvas_item_owner.owns(canvas_item->parent)) {
		Item* canvas_item_parent = canvas_item_owner.get_or_null(canvas_item->parent);
		canvas_item_parent->children_order_dirty = true;
		return;
	}

	Canvas* canvas = canvas_owner.get_or_null(canvas_item->parent);
	if (canvas) {
		canvas->children_order_dirty = true;
		return;
	}
}

void RendererCanvasCull::canvas_item_set_material(RID p_item, RID p_material)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->material = p_material;
	_item_queue_update(canvas_item, true);
}

void RendererCanvasCull::canvas_item_set_use_parent_material(RID p_item, bool p_enable)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	canvas_item->use_parent_material = p_enable;
	_item_queue_update(canvas_item, true);
}

void RendererCanvasCull::canvas_item_set_debug_redraw(bool p_enabled)
{
	debug_redraw = p_enabled;
	Renderer::canvas_render->set_debug_redraw(p_enabled, debug_redraw_time, debug_redraw_color);
}

bool RendererCanvasCull::canvas_item_get_debug_redraw() const { return debug_redraw; }

void RendererCanvasCull::canvas_item_set_interpolated(RID p_item, bool p_interpolated)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	canvas_item->interpolated = p_interpolated;
}

void RendererCanvasCull::canvas_item_reset_physics_interpolation(RID p_item)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	canvas_item->xform_prev = canvas_item->xform_curr;
}

// Useful especially for origin shifting.
void RendererCanvasCull::canvas_item_transform_physics_interpolation(
	RID p_item, const Transform2D& p_transform)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);
	canvas_item->xform_prev = p_transform * canvas_item->xform_prev;
	canvas_item->xform_curr = p_transform * canvas_item->xform_curr;
}

void RendererCanvasCull::canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
	float p_clear_margin, bool p_fit_empty, float p_fit_margin, bool p_blur_mipmaps)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(canvas_item);

	if (p_mode == RSE::CANVAS_GROUP_MODE_DISABLED) {
		if (canvas_item->canvas_group != nullptr) {
			memdelete(canvas_item->canvas_group);
			canvas_item->canvas_group = nullptr;
		}
	}
	else {
		if (canvas_item->canvas_group == nullptr) {
			canvas_item->canvas_group = memnew(RCR::Item::CanvasGroup);
		}
		canvas_item->canvas_group->mode = p_mode;
		canvas_item->canvas_group->fit_empty = p_fit_empty;
		canvas_item->canvas_group->fit_margin = p_fit_margin;
		canvas_item->canvas_group->blur_mipmaps = p_blur_mipmaps;
		canvas_item->canvas_group->clear_margin = p_clear_margin;
	}
}

RID RendererCanvasCull::canvas_light_allocate() { return canvas_light_owner.allocate_rid(); }

void RendererCanvasCull::canvas_light_initialize(RID p_rid)
{
	canvas_light_owner.initialize_rid(p_rid);
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_rid);
	clight->light_internal = Renderer::canvas_render->light_create();
}

void RendererCanvasCull::update()
{
	while (SelfList<Item>* E = _item_update_list.first()) {
		_item_update_list.remove(E);
	}
}

void RendererCanvasCull::canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	if (clight->mode == p_mode) {
		return;
	}

	RID canvas = clight->canvas;

	if (canvas.is_valid()) {
		canvas_light_attach_to_canvas(p_light, RID());
	}

	clight->mode = p_mode;

	if (canvas.is_valid()) {
		canvas_light_attach_to_canvas(p_light, canvas);
	}
}

void RendererCanvasCull::canvas_light_attach_to_canvas(RID p_light, RID p_canvas)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	if (clight->canvas.is_valid()) {
		Canvas* canvas = canvas_owner.get_or_null(clight->canvas);
		if (clight->mode == RSE::CANVAS_LIGHT_MODE_POINT) {
			canvas->lights.erase(clight);
		}
		else {
			canvas->directional_lights.erase(clight);
		}
	}

	if (!canvas_owner.owns(p_canvas)) {
		p_canvas = RID();
	}

	clight->canvas = p_canvas;

	if (clight->canvas.is_valid()) {
		Canvas* canvas = canvas_owner.get_or_null(clight->canvas);
		if (clight->mode == RSE::CANVAS_LIGHT_MODE_POINT) {
			canvas->lights.insert(clight);
		}
		else {
			canvas->directional_lights.insert(clight);
		}
	}
}

void RendererCanvasCull::canvas_light_set_enabled(RID p_light, bool p_enabled)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->enabled = p_enabled;
}

void RendererCanvasCull::canvas_light_set_texture_scale(RID p_light, float p_scale)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->scale = p_scale;
}

void RendererCanvasCull::canvas_light_set_transform(RID p_light, const Transform2D& p_transform)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	if (_interpolation_data.interpolation_enabled && clight->interpolated) {
		if (!clight->on_interpolate_transform_list) {
			_interpolation_data.canvas_light_transform_update_list_curr->push_back(p_light);
			clight->on_interpolate_transform_list = true;
		}
		else {
			DEV_ASSERT(_interpolation_data.canvas_light_transform_update_list_curr->size() > 0);
		}
	}

	clight->xform_curr = p_transform;
}

void RendererCanvasCull::canvas_light_set_texture(RID p_light, RID p_texture)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	if (clight->texture == p_texture) {
		return;
	}
	clight->texture = p_texture;
	clight->version++;
	Renderer::canvas_render->light_set_texture(clight->light_internal, p_texture);
}

void RendererCanvasCull::canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->texture_offset = p_offset;
}

void RendererCanvasCull::canvas_light_set_color(RID p_light, const Color& p_color)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->color = p_color;
}

void RendererCanvasCull::canvas_light_set_height(RID p_light, float p_height)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->height = p_height;
}

void RendererCanvasCull::canvas_light_set_energy(RID p_light, float p_energy)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->energy = p_energy;
}

void RendererCanvasCull::canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->z_min = p_min_z;
	clight->z_max = p_max_z;
}

void RendererCanvasCull::canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->layer_max = p_max_layer;
	clight->layer_min = p_min_layer;
}

void RendererCanvasCull::canvas_light_set_item_cull_mask(RID p_light, int p_mask)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->item_mask = p_mask;
}

void RendererCanvasCull::canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->item_shadow_mask = p_mask;
}

void RendererCanvasCull::canvas_light_set_directional_distance(RID p_light, float p_distance)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->directional_distance = p_distance;
}

void RendererCanvasCull::canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->blend_mode = p_mode;
}

void RendererCanvasCull::canvas_light_set_shadow_enabled(RID p_light, bool p_enabled)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	if (clight->use_shadow == p_enabled) {
		return;
	}
	clight->use_shadow = p_enabled;
	clight->version++;
	Renderer::canvas_render->light_set_use_shadow(clight->light_internal, clight->use_shadow);
}

void RendererCanvasCull::canvas_light_set_shadow_filter(
	RID p_light, RSE::CanvasLightShadowFilter p_filter)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->shadow_filter = p_filter;
}

void RendererCanvasCull::canvas_light_set_shadow_color(RID p_light, const Color& p_color)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);

	clight->shadow_color = p_color;
}

void RendererCanvasCull::canvas_light_set_shadow_smooth(RID p_light, float p_smooth)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);
	clight->shadow_smooth = p_smooth;
}

void RendererCanvasCull::canvas_light_set_interpolated(RID p_light, bool p_interpolated)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);
	clight->interpolated = p_interpolated;
}

void RendererCanvasCull::canvas_light_reset_physics_interpolation(RID p_light)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);
	clight->xform_prev = clight->xform_curr;
}

void RendererCanvasCull::canvas_light_transform_physics_interpolation(
	RID p_light, const Transform2D& p_transform)
{
	RendererCanvasRender::Light* clight = canvas_light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(clight);
	clight->xform_prev = p_transform * clight->xform_prev;
	clight->xform_curr = p_transform * clight->xform_curr;
}

RID RendererCanvasCull::canvas_light_occluder_allocate()
{
	return canvas_light_occluder_owner.allocate_rid();
}

void RendererCanvasCull::canvas_light_occluder_initialize(RID p_rid)
{
	return canvas_light_occluder_owner.initialize_rid(p_rid);
}

void RendererCanvasCull::canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	if (occluder->canvas.is_valid()) {
		Canvas* canvas = canvas_owner.get_or_null(occluder->canvas);
		canvas->occluders.erase(occluder);
	}

	if (!canvas_owner.owns(p_canvas)) {
		p_canvas = RID();
	}

	occluder->canvas = p_canvas;

	if (occluder->canvas.is_valid()) {
		Canvas* canvas = canvas_owner.get_or_null(occluder->canvas);
		canvas->occluders.insert(occluder);
	}
}

void RendererCanvasCull::canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->enabled = p_enabled;
}

void RendererCanvasCull::canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	if (occluder->polygon.is_valid()) {
		LightOccluderPolygon* occluder_poly =
			canvas_light_occluder_polygon_owner.get_or_null(occluder->polygon);
		if (occluder_poly) {
			occluder_poly->owners.erase(occluder);
		}
	}

	occluder->polygon = p_polygon;
	occluder->occluder = RID();

	if (occluder->polygon.is_valid()) {
		LightOccluderPolygon* occluder_poly =
			canvas_light_occluder_polygon_owner.get_or_null(p_polygon);
		if (!occluder_poly) {
			occluder->polygon = RID();
			ERR_FAIL_NULL(occluder_poly);
		}
		else {
			occluder_poly->owners.insert(occluder);
			occluder->occluder = occluder_poly->occluder;
			occluder->aabb_cache = occluder_poly->aabb;
			occluder->cull_cache = occluder_poly->cull_mode;
		}
	}
}

void RendererCanvasCull::canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->sdf_collision = p_enable;
}

void RendererCanvasCull::canvas_light_occluder_set_transform(
	RID p_occluder, const Transform2D& p_xform)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	if (_interpolation_data.interpolation_enabled && occluder->interpolated) {
		if (!occluder->on_interpolate_transform_list) {
			_interpolation_data.canvas_light_occluder_transform_update_list_curr->push_back(
				p_occluder);
			occluder->on_interpolate_transform_list = true;
		}
		else {
			DEV_ASSERT(
				_interpolation_data.canvas_light_occluder_transform_update_list_curr->size() > 0);
		}
	}

	occluder->xform_curr = p_xform;
}

void RendererCanvasCull::canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->light_mask = p_mask;
}

void RendererCanvasCull::canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	occluder->interpolated = p_interpolated;
}

void RendererCanvasCull::canvas_light_occluder_reset_physics_interpolation(RID p_occluder)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	occluder->xform_prev = occluder->xform_curr;
}

void RendererCanvasCull::canvas_light_occluder_transform_physics_interpolation(
	RID p_occluder, const Transform2D& p_transform)
{
	RendererCanvasRender::LightOccluderInstance* occluder =
		canvas_light_occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	occluder->xform_prev = p_transform * occluder->xform_prev;
	occluder->xform_curr = p_transform * occluder->xform_curr;
}

RID RendererCanvasCull::canvas_occluder_polygon_allocate()
{
	return canvas_light_occluder_polygon_owner.allocate_rid();
}

void RendererCanvasCull::canvas_occluder_polygon_initialize(RID p_rid)
{
	canvas_light_occluder_polygon_owner.initialize_rid(p_rid);
	LightOccluderPolygon* occluder_poly = canvas_light_occluder_polygon_owner.get_or_null(p_rid);
	occluder_poly->occluder = Renderer::canvas_render->occluder_polygon_create();
}

void RendererCanvasCull::canvas_occluder_polygon_set_shape(
	RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed)
{
	LightOccluderPolygon* occluder_poly =
		canvas_light_occluder_polygon_owner.get_or_null(p_occluder_polygon);
	ERR_FAIL_NULL(occluder_poly);

	uint32_t pc = p_shape.size();
	ERR_FAIL_COND(pc < 2);

	occluder_poly->aabb = Rect2();
	const Vector2* r = p_shape.ptr();
	for (uint32_t i = 0; i < pc; i++) {
		if (i == 0) {
			occluder_poly->aabb.position = r[i];
		}
		else {
			occluder_poly->aabb.expand_to(r[i]);
		}
	}

	Renderer::canvas_render->occluder_polygon_set_shape(occluder_poly->occluder, p_shape, p_closed);

	for (RendererCanvasRender::LightOccluderInstance* E : occluder_poly->owners) {
		E->aabb_cache = occluder_poly->aabb;
	}
}

void RendererCanvasCull::canvas_occluder_polygon_set_cull_mode(
	RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode)
{
	LightOccluderPolygon* occluder_poly =
		canvas_light_occluder_polygon_owner.get_or_null(p_occluder_polygon);
	ERR_FAIL_NULL(occluder_poly);
	occluder_poly->cull_mode = p_mode;
	Renderer::canvas_render->occluder_polygon_set_cull_mode(occluder_poly->occluder, p_mode);
	for (RendererCanvasRender::LightOccluderInstance* E : occluder_poly->owners) {
		E->cull_cache = p_mode;
	}
}

void RendererCanvasCull::canvas_set_shadow_texture_size(int p_size)
{
	Renderer::canvas_render->set_shadow_texture_size(p_size);
}

RID RendererCanvasCull::canvas_texture_allocate()
{
	return RendererTextureStorage::canvas_texture_allocate();
}

void RendererCanvasCull::canvas_texture_initialize(RID p_rid)
{
	RendererTextureStorage::canvas_texture_initialize(p_rid);
}

void RendererCanvasCull::canvas_texture_set_channel(
	RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
{
	RendererTextureStorage::canvas_texture_set_channel(p_canvas_texture, p_channel, p_texture);
}

void RendererCanvasCull::canvas_texture_set_shading_parameters(
	RID p_canvas_texture, const Color& p_base_color, float p_shininess)
{
	RendererTextureStorage::canvas_texture_set_shading_parameters(
		p_canvas_texture, p_base_color, p_shininess);
}

void RendererCanvasCull::canvas_texture_set_texture_filter(
	RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
{
	RendererTextureStorage::canvas_texture_set_texture_filter(p_canvas_texture, p_filter);
}

void RendererCanvasCull::canvas_texture_set_texture_repeat(
	RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
{
	RendererTextureStorage::canvas_texture_set_texture_repeat(p_canvas_texture, p_repeat);
}

void RendererCanvasCull::canvas_item_set_default_texture_filter(
	RID p_item, RSE::CanvasItemTextureFilter p_filter)
{
	Item* ci = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(ci);
	ci->texture_filter = p_filter;
}

void RendererCanvasCull::canvas_item_set_default_texture_repeat(
	RID p_item, RSE::CanvasItemTextureRepeat p_repeat)
{
	Item* ci = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL(ci);
	ci->texture_repeat = p_repeat;
}

Rect2 RendererCanvasCull::_debug_canvas_item_get_rect(RID p_item)
{
	Item* canvas_item = canvas_item_owner.get_or_null(p_item);
	ERR_FAIL_NULL_V(canvas_item, Rect2());
	return canvas_item->get_rect();
}

void RendererCanvasCull::_item_queue_update(Item* p_item, bool p_update_dependencies)
{
	if (p_update_dependencies) {
		p_item->update_dependencies = true;
	}

	if (!p_item->update_item.in_list()) {
		_item_update_list.add(&p_item->update_item);
	}
}

void RendererCanvasCull::tick()
{
	if (_interpolation_data.interpolation_enabled) {
		update_interpolation_tick(true);
	}
}

void RendererCanvasCull::update_interpolation_tick(bool p_process)
{
#define GODOT_UPDATE_INTERPOLATION_TICK(m_list_prev, m_list_curr, m_type, m_owner_list)            \
	/* Detect any that were on the previous transform list that are no longer active. */           \
	for (unsigned int n = 0; n < _interpolation_data.m_list_prev->size(); n++) {                   \
		const RID& rid = (*_interpolation_data.m_list_prev)[n];                                    \
		m_type* item = m_owner_list.get_or_null(rid);                                              \
		/* no longer active? (either the instance deleted or no longer being transformed) */       \
		if (item && !item->on_interpolate_transform_list) {                                        \
			item->xform_prev = item->xform_curr;                                                   \
		}                                                                                          \
	}                                                                                              \
	/* and now                                                                                     \
for any in the transform list (being actively interpolated), */                                    \
	/* keep the previous transform value up to date and ready for next tick */                     \
	if (p_process) {                                                                               \
		for (unsigned int n = 0; n < _interpolation_data.m_list_curr->size(); n++) {               \
			const RID& rid = (*_interpolation_data.m_list_curr)[n];                                \
			m_type* item = m_owner_list.get_or_null(rid);                                          \
			if (item) {                                                                            \
				item->xform_prev = item->xform_curr;                                               \
				item->on_interpolate_transform_list = false;                                       \
			}                                                                                      \
		}                                                                                          \
	}                                                                                              \
	SWAP(_interpolation_data.m_list_curr, _interpolation_data.m_list_prev);                        \
	_interpolation_data.m_list_curr->clear();

	GODOT_UPDATE_INTERPOLATION_TICK(canvas_item_transform_update_list_prev,
		canvas_item_transform_update_list_curr, Item, canvas_item_owner);
	GODOT_UPDATE_INTERPOLATION_TICK(canvas_light_transform_update_list_prev,
		canvas_light_transform_update_list_curr, RendererCanvasRender::Light, canvas_light_owner);
	GODOT_UPDATE_INTERPOLATION_TICK(canvas_light_occluder_transform_update_list_prev,
		canvas_light_occluder_transform_update_list_curr,
		RendererCanvasRender::LightOccluderInstance, canvas_light_occluder_owner);

#undef GODOT_UPDATE_INTERPOLATION_TICK
}

void RendererCanvasCull::InterpolationData::notify_free_canvas_item(
	RID p_rid, RendererCanvasCull::Item& r_canvas_item)
{
	r_canvas_item.on_interpolate_transform_list = false;

	if (!interpolation_enabled) {
		return;
	}

	// If the instance was on any of the lists, remove.
	canvas_item_transform_update_list_curr->erase_multiple_unordered(p_rid);
	canvas_item_transform_update_list_prev->erase_multiple_unordered(p_rid);
}

void RendererCanvasCull ::InterpolationData::notify_free_canvas_light(
	RID p_rid, RendererCanvasRender::Light& r_canvas_light)
{
	r_canvas_light.on_interpolate_transform_list = false;

	if (!interpolation_enabled) {
		return;
	}

	// If the instance was on any of the lists, remove.
	canvas_light_transform_update_list_curr->erase_multiple_unordered(p_rid);
	canvas_light_transform_update_list_prev->erase_multiple_unordered(p_rid);
}

void RendererCanvasCull::InterpolationData::notify_free_canvas_light_occluder(
	RID p_rid, RendererCanvasRender::LightOccluderInstance& r_canvas_light_occluder)
{
	r_canvas_light_occluder.on_interpolate_transform_list = false;

	if (!interpolation_enabled) {
		return;
	}

	// If the instance was on any of the lists, remove.
	canvas_light_occluder_transform_update_list_curr->erase_multiple_unordered(p_rid);
	canvas_light_occluder_transform_update_list_prev->erase_multiple_unordered(p_rid);
}

RendererCanvasCull::RendererCanvasCull()
{
	z_list = (RCR::Item**)memalloc(z_range * sizeof(RCR::Item*));
	z_last_list =
		(RCR::Item**)memalloc(z_range * sizeof(RCR::Item*));
	_canvas_cull_singleton = this;
}

RendererCanvasCull::~RendererCanvasCull()
{
	if (z_list) {
		memfree(z_list);
		z_list = nullptr;
	}
	if (z_last_list) {
		memfree(z_last_list);

z_last_list = nullptr;
	}
	_canvas_cull_singleton = nullptr;
}

void RendererCanvasCull::finalize() {}



// ============================================================================
// Merged from servers/rendering/renderer_viewport.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/transform_interpolator.h"
#include "core/os/os.h"
#include "renderer_viewport.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_canvas_cull.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/renderer_scene_occlusion_cull.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/storage/texture_storage.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

static Transform2D _canvas_get_transform(RendererViewport::Viewport* p_viewport,
	RendererCanvasCull::Canvas* p_canvas, RendererViewport::Viewport::CanvasData* p_canvas_data,
	const Vector2& p_vp_size)
{
	Transform2D xf = p_viewport->global_transform;

	Vector2 pixel_snap_offset;
	if (p_viewport->snap_2d_transforms_to_pixel) {
		// We use `floor(p + 0.5)` to snap canvas items, but `ceil(p - 0.5)`
		// to snap viewport transform because the viewport transform is inverse
		// to the camera transform. Also, if the viewport size is not divisible
		// by 2, the center point is offset by 0.5 px and we need to add 0.5
		// before rounding to cancel it out.
		pixel_snap_offset.x = (p_viewport->size.width % 2) ? 0.0 : -0.5;
		pixel_snap_offset.y = (p_viewport->size.height % 2) ? 0.0 : -0.5;
	}

	float scale = 1.0;
	if (p_viewport->canvas_map.has(p_canvas->parent)) {
		Transform2D c_xform = p_viewport->canvas_map[p_canvas->parent].transform;
		if (p_viewport->snap_2d_transforms_to_pixel) {
			c_xform.columns[2] =
				(c_xform.columns[2] * p_canvas->parent_scale + pixel_snap_offset).ceil() /
				p_canvas->parent_scale;
		}
		xf = xf * c_xform;
		scale = p_canvas->parent_scale;
	}

	Transform2D c_xform = p_canvas_data->transform;
	if (p_viewport->snap_2d_transforms_to_pixel) {
		c_xform.columns[2] = (c_xform.columns[2] + pixel_snap_offset).ceil();
	}
	xf = xf * c_xform;

	if (scale != 1.0 && !Renderer::canvas->disable_scale) {
		Vector2 pivot = p_vp_size * 0.5;
		Transform2D xfpivot;
		xfpivot.set_origin(pivot);
		Transform2D xfscale;
		xfscale.scale(Vector2(scale, scale));

		xf = xfpivot.affine_inverse() * xf;
		xf = xfscale * xf;
		xf = xfpivot * xf;
	}

	return xf;
}

Vector<RendererViewport::Viewport*> RendererViewport::_sort_active_viewports()
{
	// We need to sort the viewports in a "topological order", children first and
	// parents last. We also need to keep sibling viewports in the original order
	// from top to bottom.

	Vector<Viewport*> result;
	List<Viewport*> nodes;

	for (int i = active_viewports.size() - 1; i >= 0; --i) {
		Viewport* viewport = active_viewports[i];
		if (viewport->parent.is_valid()) {
			continue;
		}

		nodes.push_back(viewport);
		result.insert(0, viewport);
	}

	while (!nodes.is_empty()) {
		const Viewport* node = nodes.front()->get();
		nodes.pop_front();

		for (int i = active_viewports.size() - 1; i >= 0; --i) {
			Viewport* child = active_viewports[i];
			if (child->parent != node->self) {
				continue;
			}

			if (!nodes.find(child)) {
				nodes.push_back(child);
				result.insert(0, child);
			}
		}
	}

	return result;
}

void RendererViewport::_configure_3d_render_buffers(Viewport* p_viewport)
{
	if (p_viewport->render_buffers.is_valid()) {
		if (p_viewport->size.width == 0 || p_viewport->size.height == 0) {
			p_viewport->render_buffers.unref();
		}
		else {
			const float EPSILON = 0.0001;
			float scaling_3d_scale = p_viewport->scaling_3d_scale;
			RSE::ViewportScaling3DMode scaling_3d_mode = p_viewport->scaling_3d_mode;
			bool upscaler_available = p_viewport->fsr_enabled;
			RSE::ViewportScaling3DType scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);

			if ((!upscaler_available || (scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_SPATIAL)) &&
				scaling_3d_scale >= (1.0 - EPSILON) && scaling_3d_scale <= (1.0 + EPSILON)) {
				// No 3D scaling for spatial modes? Ignore scaling mode, this just introduces
				// overhead.
				// - Mobile can't perform optimal path
				// - FSR does an extra pass (or 2 extra passes if 2D-MSAA is enabled)
				// Scaling = 1.0 on FSR2 and MetalFX temporal has benefits
				scaling_3d_scale = 1.0;
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
			}

			if (scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF &&
				scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR &&
				scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_NEAREST &&
				OS::get_current_rendering_method() == "gl_compatibility") {
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
				scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);
				WARN_PRINT_ONCE("MetalFX and FSR upscaling are not supported in the Compatibility "
								"renderer. Falling back to bilinear scaling.");
			}

			if ((scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR ||
					scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR2 ||
					scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL) &&
				OS::get_current_rendering_method() == "mobile") {
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
				scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);
				WARN_PRINT_ONCE("MetalFX temporal and FSR upscaling are not supported in the "
								"Mobile renderer. Falling back to bilinear scaling.");
			}

			if (scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL &&
				!RD::has_feature(RDC::SUPPORTS_METALFX_TEMPORAL)) {
				if (RD::has_feature(RDC::SUPPORTS_METALFX_SPATIAL)) {
					// Prefer MetalFX spatial if it is supported, which will be much more efficient
					// than FSR2, as the hardware already will struggle with FSR2.
					scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL;
					WARN_PRINT_ONCE(
						"MetalFX temporal upscaling is not supported by the current renderer or "
						"hardware. Falling back to MetalFX Spatial scaling.");
				}
				else {
					scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_FSR2;
					WARN_PRINT_ONCE("MetalFX upscaling is not supported by the current renderer or "
									"hardware. Falling back to FSR 2 scaling.");
				}
				scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);
			}

			if (scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL &&
				!RD::has_feature(RDC::SUPPORTS_METALFX_SPATIAL)) {
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_FSR;
				WARN_PRINT_ONCE("MetalFX spatial upscaling is not supported by the current "
								"renderer or hardware. Falling back to FSR scaling.");
			}

			RSE::ViewportMSAA msaa_3d = p_viewport->msaa_3d;

			// If MetalFX Temporal upscaling is supported, verify limits.
			if (scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL) {
				double min_scale =
					(double)RD::limit_get(RDC::LIMIT_METALFX_TEMPORAL_SCALER_MIN_SCALE) /
					1'000'000.0;
				double max_scale =
					(double)RD::limit_get(RDC::LIMIT_METALFX_TEMPORAL_SCALER_MAX_SCALE) /
					1'000'000.0;
				if ((double)scaling_3d_scale < min_scale || (double)scaling_3d_scale > max_scale) {
					scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_FSR2;
					WARN_PRINT_ONCE(
						vformat("MetalFX temporal upscaling scale is outside limits; scale must be "
								"between %f and %f. Falling back to FSR 2 3D resolution scaling.",
							min_scale, max_scale));
				}
				else if (msaa_3d != RSE::VIEWPORT_MSAA_DISABLED) {
					WARN_PRINT_ONCE("MetalFX temporal upscaling does not support 3D MSAA. "
									"Disabling 3D MSAA internally.");
					msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
				}
			}

			bool scaling_3d_is_not_bilinear =
				scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF &&
				scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
			bool use_taa = p_viewport->use_taa;

			if (scaling_3d_is_not_bilinear && scaling_3d_scale >= (1.0 + EPSILON)) {
				// FSR, MetalFX, and nearest-neighbor scaling are not designed for downsampling.
				// Fall back to bilinear scaling.
				WARN_PRINT_ONCE(
					"FSR, MetalFX, and nearest-neighbor 3D resolution scaling are not designed for "
					"downsampling. Falling back to bilinear 3D resolution scaling.");
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
				scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);
			}

			if (scaling_3d_is_not_bilinear &&
				scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_NEAREST && !upscaler_available) {
				// FSR is not actually available.
				// Fall back to bilinear scaling.
				WARN_PRINT_ONCE("FSR 3D resolution scaling is not available. Falling back to "
								"bilinear 3D resolution scaling.");
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
				scaling_type = RSE::scaling_3d_mode_type(scaling_3d_mode);
			}

			if (use_taa && scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL) {
				// Temporal upscalers can't be used with TAA.
				// Turn it off and prefer using the temporal upscaler.
				WARN_PRINT_ONCE("FSR 2 or MetalFX Temporal is not compatible with TAA. Disabling "
								"TAA internally.");
				use_taa = false;
			}

			int target_width;
			int target_height;
			int render_width;
			int render_height;

			switch (scaling_3d_mode) {
			case RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR:
			case RSE::VIEWPORT_SCALING_3D_MODE_NEAREST:
				// Clamp 3D rendering resolution to reasonable values supported on most hardware.
				// This prevents freezing the engine or outright crashing on lower-end GPUs.
				target_width = p_viewport->size.width;
				target_height = p_viewport->size.height;
				render_width = CLAMP(target_width * scaling_3d_scale, 1, 16384);
				render_height = CLAMP(target_height * scaling_3d_scale, 1, 16384);
				break;
			case RSE::VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL:
			case RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL:
			case RSE::VIEWPORT_SCALING_3D_MODE_FSR:
			case RSE::VIEWPORT_SCALING_3D_MODE_FSR2:
				target_width = p_viewport->size.width;
				target_height = p_viewport->size.height;
				render_width = MAX(target_width * scaling_3d_scale,
					1.0); // target_width / (target_width * scaling)
				render_height = MAX(target_height * scaling_3d_scale, 1.0);
				break;
			case RSE::VIEWPORT_SCALING_3D_MODE_OFF:
				target_width = p_viewport->size.width;
				target_height = p_viewport->size.height;
				render_width = target_width;
				render_height = target_height;
				break;
			default:
				// This is an unknown mode.
				WARN_PRINT_ONCE(vformat(
					"Unknown scaling mode: %d. Disabling 3D resolution scaling.", scaling_3d_mode));
				scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
				scaling_3d_scale = 1.0;
				target_width = p_viewport->size.width;
				target_height = p_viewport->size.height;
				render_width = target_width;
				render_height = target_height;
				break;
			}

			uint32_t jitter_phase_count = 0;
			if (scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL) {
				// Implementation has been copied from ffxFsr2GetJitterPhaseCount.
				// Also used for MetalFX Temporal scaling.
				jitter_phase_count =
					uint32_t(8.0f * std::pow(float(target_width) / render_width, 2.0f));
			}
			else if (use_taa) {
				// Default jitter count for TAA.
				jitter_phase_count = 16;
			}

			p_viewport->internal_size = Size2(render_width, render_height);
			p_viewport->jitter_phase_count = jitter_phase_count;

			// At resolution scales lower than 1.0, use negative texture mipmap bias
			// to compensate for the loss of sharpness.
			const float texture_mipmap_bias =
				std::log2(MIN(scaling_3d_scale, 1.0)) + p_viewport->texture_mipmap_bias;

			RenderSceneBuffersConfiguration rb_config;
			rb_config.set_render_target(p_viewport->render_target);
			rb_config.set_internal_size(Size2i(render_width, render_height));
			rb_config.set_target_size(Size2(target_width, target_height));
			rb_config.set_view_count(p_viewport->view_count);
			rb_config.set_scaling_3d_mode(scaling_3d_mode);
			rb_config.set_msaa_3d(msaa_3d);
			rb_config.set_screen_space_aa(p_viewport->screen_space_aa);
			rb_config.set_fsr_sharpness(p_viewport->fsr_sharpness);
			rb_config.set_texture_mipmap_bias(texture_mipmap_bias);
			rb_config.set_anisotropic_filtering_level(p_viewport->anisotropic_filtering_level);
			rb_config.set_use_taa(use_taa);
			rb_config.set_use_debanding(p_viewport->use_debanding);

			p_viewport->render_buffers->configure(&rb_config);
		}
	}
}

void RendererViewport::_draw_3d(Viewport* p_viewport) {}

void RendererViewport::_draw_viewport(Viewport* p_viewport)
{
	if (p_viewport->measure_render_time) {
		String rt_id = "vp_begin_" + itos(p_viewport->self.get_id());
		RendererUtilities::capture_timestamp(rt_id);
		timestamp_vp_map[rt_id] = p_viewport->self;
	}

	if (OS::get_current_rendering_method() == "gl_compatibility") {
		// This is currently needed for GLES to keep the current window being rendered to up to date
		DisplayServer::gl_window_make_current(p_viewport->viewport_to_screen);
	}

	/* Camera should always be BEFORE any other 3D */

	bool can_draw_2d =
		!p_viewport->disable_2d &&
		p_viewport->view_count == 1; // Stereo rendering does not support 2D, no depth data
	bool scenario_draw_canvas_bg =
		false; // draw canvas, or some layer of it, as BG for 3D instead of in front
	int scenario_canvas_max_layer = 0;
	bool force_clear_render_target = false;

	for (int i = 0; i < RSE::VIEWPORT_RENDER_INFO_TYPE_MAX; i++) {
		for (int j = 0; j < RSE::VIEWPORT_RENDER_INFO_MAX; j++) {
			p_viewport->render_info.info[i][j] = 0;
		}
	}

	if (RendererSceneCull::is_scenario(p_viewport->scenario)) {
		RID environment = RendererSceneCull::scenario_get_environment(p_viewport->scenario);
		if (RendererSceneCull::is_environment(environment)) {
			if (can_draw_2d && !viewport_is_environment_disabled(p_viewport)) {
				scenario_draw_canvas_bg =
					RendererSceneCull::environment_get_background(environment) == RSE::ENV_BG_CANVAS;
				scenario_canvas_max_layer =
					RendererSceneCull::environment_get_canvas_max_layer(environment);
			}
			else if (RendererSceneCull::environment_get_background(environment) ==
					   RSE::ENV_BG_CANVAS) {
				// The scene renderer will still copy over the last frame, so we need to clear the
				// render target.
				force_clear_render_target = true;
			}
		}

		p_viewport->window_output_max_value = 1.0;
		DisplayServerEnums::WindowID parent_window = _get_containing_window(p_viewport);
		if (RD::data && parent_window != DisplayServerEnums::INVALID_WINDOW_ID) {
			RenderingContextDriver* context_driver = RD::get_context_driver();
			if (context_driver->window_get_hdr_output_enabled(parent_window)) {
				p_viewport->window_output_max_value =
					context_driver->window_get_output_max_linear_value(parent_window);
			}
		}
	}

	bool can_draw_3d = RendererSceneCull::is_camera(p_viewport->camera) && !p_viewport->disable_3d;

	if ((scenario_draw_canvas_bg || can_draw_3d) && !p_viewport->render_buffers.is_valid()) {
		// wants to draw 3D but there is no render buffer, create
		p_viewport->render_buffers = RendererSceneCull::render_buffers_create();

		_configure_3d_render_buffers(p_viewport);
	}

	Color bgcolor = p_viewport->transparent_bg ? Color(0, 0, 0, 0)
											   : RendererTextureStorage::get_default_clear_color();

	if (p_viewport->clear_mode != RSE::VIEWPORT_CLEAR_NEVER) {
		RendererTextureStorage::render_target_request_clear(p_viewport->render_target, bgcolor);
		if (p_viewport->clear_mode == RSE::VIEWPORT_CLEAR_ONLY_NEXT_FRAME) {
			p_viewport->clear_mode = RSE::VIEWPORT_CLEAR_NEVER;
		}
	}

	if (!scenario_draw_canvas_bg && can_draw_3d) {
		if (force_clear_render_target) {
			RendererTextureStorage::render_target_do_clear_request(p_viewport->render_target);
		}
		_draw_3d(p_viewport);
	}

	if (can_draw_2d) {
		RBMap<Viewport::CanvasKey, Viewport::CanvasData*> canvas_map;

		Rect2 clip_rect(0, 0, p_viewport->size.x, p_viewport->size.y);
		RendererCanvasRender::Light* lights = nullptr;
		RendererCanvasRender::Light* lights_with_shadow = nullptr;

		RendererCanvasRender::Light* directional_lights = nullptr;
		RendererCanvasRender::Light* directional_lights_with_shadow = nullptr;

		if (p_viewport->sdf_active) {
			// Process SDF.

			Rect2 sdf_rect =
				RendererTextureStorage::render_target_get_sdf_rect(p_viewport->render_target);

			RendererCanvasRender::LightOccluderInstance* occluders = nullptr;

			// Make list of occluders.
			for (KeyValue<RID, Viewport::CanvasData>& E : p_viewport->canvas_map) {
				RendererCanvasCull::Canvas* canvas =
					static_cast<RendererCanvasCull::Canvas*>(E.value.canvas);
				Transform2D xf =
					_canvas_get_transform(p_viewport, canvas, &E.value, clip_rect.size);

				for (RendererCanvasRender::LightOccluderInstance* F : canvas->occluders) {
					if (!F->enabled) {
						continue;
					}

					if (!Renderer::canvas->_interpolation_data.interpolation_enabled ||
						!F->interpolated) {
						F->xform_cache = xf * F->xform_curr;
					}
					else {
						real_t f = Engine::get_physics_interpolation_fraction();
						TransformInterpolator::interpolate_transform_2d(
							F->xform_prev, F->xform_curr, F->xform_cache, f);
						F->xform_cache = xf * F->xform_cache;
					}

					if (sdf_rect.intersects_transformed(F->xform_cache, F->aabb_cache)) {
						F->next = occluders;
						occluders = F;
					}
				}
			}

			Renderer::canvas_render->render_sdf(p_viewport->render_target, occluders);
			RendererTextureStorage::render_target_mark_sdf_enabled(p_viewport->render_target, true);

			p_viewport->sdf_active = false; // If used, gets set active again.
		}
		else {
			RendererTextureStorage::render_target_mark_sdf_enabled(
				p_viewport->render_target, false);
		}

		Rect2 shadow_rect;

		int shadow_count = 0;
		int directional_light_count = 0;

		RENDER_TIMESTAMP("Cull 2D Lights");
		for (KeyValue<RID, Viewport::CanvasData>& E : p_viewport->canvas_map) {
			RendererCanvasCull::Canvas* canvas =
				static_cast<RendererCanvasCull::Canvas*>(E.value.canvas);

			Transform2D xf = _canvas_get_transform(p_viewport, canvas, &E.value, clip_rect.size);

			// Find lights in canvas.

			for (RendererCanvasRender::Light* F : canvas->lights) {
				RendererCanvasRender::Light* cl = F;
				if (cl->enabled && cl->texture.is_valid()) {
					// not super efficient..
					Size2 tsize = RendererTextureStorage::texture_size_with_proxy(cl->texture);
					tsize *= cl->scale;

					Vector2 offset = tsize / 2.0;
					Rect2 local_rect = Rect2(-offset + cl->texture_offset, tsize);

					if (!Renderer::canvas->_interpolation_data.interpolation_enabled ||
						!cl->interpolated) {
						cl->xform_cache = xf * cl->xform_curr;
					}
					else {
						real_t f = Engine::get_physics_interpolation_fraction();
						TransformInterpolator::interpolate_transform_2d(
							cl->xform_prev, cl->xform_curr, cl->xform_cache, f);
						cl->xform_cache = xf * cl->xform_cache;
					}

					cl->rect_cache = cl->xform_cache.xform(local_rect);

					if (clip_rect.intersects(cl->rect_cache)) {
						cl->filter_next_ptr = lights;
						lights = cl;
						Transform2D scale;
						scale.scale(local_rect.size);
						scale.columns[2] = local_rect.position;
						cl->light_shader_xform = cl->xform_cache * scale;
						if (cl->use_shadow) {
							cl->shadows_next_ptr = lights_with_shadow;
							if (lights_with_shadow == nullptr) {
								shadow_rect = cl->rect_cache;
							}
							else {
								shadow_rect = shadow_rect.merge(cl->rect_cache);
							}
							lights_with_shadow = cl;
							cl->radius_cache = local_rect.size.length();
						}
					}
				}
			}

			for (RendererCanvasRender::Light* F : canvas->directional_lights) {
				RendererCanvasRender::Light* cl = F;
				if (cl->enabled) {
					cl->filter_next_ptr = directional_lights;
					directional_lights = cl;
					if (!Renderer::canvas->_interpolation_data.interpolation_enabled ||
						!cl->interpolated) {
						cl->xform_cache = xf * cl->xform_curr;
					}
					else {
						real_t f = Engine::get_physics_interpolation_fraction();
						TransformInterpolator::interpolate_transform_2d(
							cl->xform_prev, cl->xform_curr, cl->xform_cache, f);
						cl->xform_cache = xf * cl->xform_cache;
					}
					cl->xform_cache.columns[2] = Vector2(); // translation is pointless
					if (cl->use_shadow) {
						cl->shadows_next_ptr = directional_lights_with_shadow;
						directional_lights_with_shadow = cl;
					}

					directional_light_count++;

					if (directional_light_count == RSE::MAX_2D_DIRECTIONAL_LIGHTS) {
						break;
					}
				}
			}

			canvas_map[Viewport::CanvasKey(E.key, E.value.layer, E.value.sublayer)] = &E.value;
		}

		if (lights_with_shadow) {
			// update shadows if any

			RendererCanvasRender::LightOccluderInstance* occluders = nullptr;

			RENDER_TIMESTAMP("> Render PointLight2D Shadows");
			RENDER_TIMESTAMP("Cull LightOccluder2Ds");

			// make list of occluders
			for (KeyValue<RID, Viewport::CanvasData>& E : p_viewport->canvas_map) {
				RendererCanvasCull::Canvas* canvas =
					static_cast<RendererCanvasCull::Canvas*>(E.value.canvas);
				Transform2D xf =
					_canvas_get_transform(p_viewport, canvas, &E.value, clip_rect.size);

				for (RendererCanvasRender::LightOccluderInstance* F : canvas->occluders) {
					if (!F->enabled) {
						continue;
					}
					if (!Renderer::canvas->_interpolation_data.interpolation_enabled ||
						!F->interpolated) {
						F->xform_cache = xf * F->xform_curr;
					}
					else {
						real_t f = Engine::get_physics_interpolation_fraction();
						TransformInterpolator::interpolate_transform_2d(
							F->xform_prev, F->xform_curr, F->xform_cache, f);
						F->xform_cache = xf * F->xform_cache;
					}
					if (shadow_rect.intersects_transformed(F->xform_cache, F->aabb_cache)) {
						F->next = occluders;
						occluders = F;
					}
				}
			}
			// update the light shadowmaps with them

			RendererCanvasRender::Light* light = lights_with_shadow;
			while (light) {
				RENDER_TIMESTAMP("Render PointLight2D Shadow");

				Renderer::canvas_render->light_update_shadow(light->light_internal, shadow_count++,
					light->xform_cache.affine_inverse(), light->item_shadow_mask,
					light->radius_cache / 1000.0, light->radius_cache * 1.1, occluders,
					light->rect_cache);
				light = light->shadows_next_ptr;
			}

			RENDER_TIMESTAMP("< Render PointLight2D Shadows");
		}

		if (directional_lights_with_shadow) {
			// update shadows if any
			RendererCanvasRender::Light* light = directional_lights_with_shadow;
			while (light) {
				Vector2 light_dir =
					-light->xform_cache.columns[1].normalized(); // Y is light direction
				float cull_distance = light->directional_distance;

				Vector2 light_dir_sign;
				light_dir_sign.x = (Math::abs(light_dir.x) < CMP_EPSILON)
									   ? 0.0
									   : ((light_dir.x > 0.0) ? 1.0 : -1.0);
				light_dir_sign.y = (Math::abs(light_dir.y) < CMP_EPSILON)
									   ? 0.0
									   : ((light_dir.y > 0.0) ? 1.0 : -1.0);

				Vector2 points[6];
				int point_count = 0;

				for (int j = 0; j < 4; j++) {
					static const Vector2 signs[4] = {
						Vector2(1, 1), Vector2(1, 0), Vector2(0, 0), Vector2(0, 1)};
					Vector2 sign_cmp = signs[j] * 2.0 - Vector2(1.0, 1.0);
					Vector2 point = clip_rect.position + clip_rect.size * signs[j];

					if (sign_cmp == light_dir_sign) {
						// both point in same direction, plot offsetted
						points[point_count++] = point + light_dir * cull_distance;
					}
					else if (sign_cmp.x == light_dir_sign.x || sign_cmp.y == light_dir_sign.y) {
						int next_j = (j + 1) % 4;
						Vector2 next_sign_cmp = signs[next_j] * 2.0 - Vector2(1.0, 1.0);

						// one point in the same direction, plot segment

						if (next_sign_cmp.x == light_dir_sign.x ||
							next_sign_cmp.y == light_dir_sign.y) {
							if (light_dir_sign.x != 0.0 || light_dir_sign.y != 0.0) {
								points[point_count++] = point;
							}
							points[point_count++] = point + light_dir * cull_distance;
						}
						else {
							points[point_count++] = point + light_dir * cull_distance;
							if (light_dir_sign.x != 0.0 || light_dir_sign.y != 0.0) {
								points[point_count++] = point;
							}
						}
					}
					else {
						// plot normally
						points[point_count++] = point;
					}
				}

				Vector2 xf_points[6];

				RendererCanvasRender::LightOccluderInstance* occluders = nullptr;

				RENDER_TIMESTAMP("> Render DirectionalLight2D Shadows");

				// Make list of occluders.
				for (KeyValue<RID, Viewport::CanvasData>& E : p_viewport->canvas_map) {
					RendererCanvasCull::Canvas* canvas =
						static_cast<RendererCanvasCull::Canvas*>(E.value.canvas);
					Transform2D xf =
						_canvas_get_transform(p_viewport, canvas, &E.value, clip_rect.size);

					for (RendererCanvasRender::LightOccluderInstance* F : canvas->occluders) {
						if (!F->enabled) {
							continue;
						}
						if (!Renderer::canvas->_interpolation_data.interpolation_enabled ||
							!F->interpolated) {
							F->xform_cache = xf * F->xform_curr;
						}
						else {
							real_t f = Engine::get_physics_interpolation_fraction();
							TransformInterpolator::interpolate_transform_2d(
								F->xform_prev, F->xform_curr, F->xform_cache, f);
							F->xform_cache = xf * F->xform_cache;
						}
						Transform2D localizer = F->xform_cache.affine_inverse();

						for (int j = 0; j < point_count; j++) {
							xf_points[j] = localizer.xform(points[j]);
						}
						if (F->aabb_cache.intersects_filled_polygon(xf_points, point_count)) {
							F->next = occluders;
							occluders = F;
						}
					}
				}

				Renderer::canvas_render->light_update_directional_shadow(light->light_internal,
					shadow_count++, light->xform_cache, light->item_shadow_mask, cull_distance,
					clip_rect, occluders);

				light = light->shadows_next_ptr;
			}

			RENDER_TIMESTAMP("< Render DirectionalLight2D Shadows");
		}

		if (scenario_draw_canvas_bg && canvas_map.begin() &&
			canvas_map.begin()->key.get_layer() > scenario_canvas_max_layer) {
			// There may be an outstanding clear request if a clear was requested, but no 2D
			// elements were drawn. Clear now otherwise we copy over garbage from the render target.
			RendererTextureStorage::render_target_do_clear_request(p_viewport->render_target);
			if (!can_draw_3d) {
				RendererSceneCull::render_empty_scene(p_viewport->render_buffers,
					p_viewport->scenario, p_viewport->shadow_atlas,
					p_viewport->window_output_max_value);
			}
			else {
				_draw_3d(p_viewport);
			}
			scenario_draw_canvas_bg = false;
		}

		int canvas_idx = 0;
		for (const KeyValue<Viewport::CanvasKey, Viewport::CanvasData*>& E : canvas_map) {
			RendererCanvasCull::Canvas* canvas =
				static_cast<RendererCanvasCull::Canvas*>(E.value->canvas);

			Transform2D xform = _canvas_get_transform(p_viewport, canvas, E.value, clip_rect.size);

			RendererCanvasRender::Light* canvas_lights = nullptr;
			RendererCanvasRender::Light* canvas_directional_lights = nullptr;

			RendererCanvasRender::Light* ptr = lights;
			while (ptr) {
				if (E.value->layer >= ptr->layer_min && E.value->layer <= ptr->layer_max) {
					ptr->next_ptr = canvas_lights;
					canvas_lights = ptr;
				}
				ptr = ptr->filter_next_ptr;
			}

			ptr = directional_lights;
			while (ptr) {
				if (E.value->layer >= ptr->layer_min && E.value->layer <= ptr->layer_max) {
					ptr->next_ptr = canvas_directional_lights;
					canvas_directional_lights = ptr;
				}
				ptr = ptr->filter_next_ptr;
			}

			RENDER_TIMESTAMP("> Render Canvas " + itos(canvas_idx));

			Renderer::canvas->render_canvas(p_viewport->render_target, canvas, xform, canvas_lights,
				canvas_directional_lights, clip_rect, p_viewport->texture_filter,
				p_viewport->texture_repeat, p_viewport->snap_2d_transforms_to_pixel,
				p_viewport->snap_2d_vertices_to_pixel, p_viewport->canvas_cull_mask,
				&p_viewport->render_info);

			RENDER_TIMESTAMP("< Render Canvas " + itos(canvas_idx));

			if (Renderer::canvas->was_sdf_used()) {
				p_viewport->sdf_active = true;
			}

			if (scenario_draw_canvas_bg && E.key.get_layer() >= scenario_canvas_max_layer) {
				// There may be an outstanding clear request if a clear was requested, but no 2D
				// elements were drawn. Clear now otherwise we copy over garbage from the render
				// target.
				RendererTextureStorage::render_target_do_clear_request(p_viewport->render_target);
				if (!can_draw_3d) {
					RendererSceneCull::render_empty_scene(p_viewport->render_buffers,
						p_viewport->scenario, p_viewport->shadow_atlas,
						p_viewport->window_output_max_value);
				}
				else {
					_draw_3d(p_viewport);
				}

				scenario_draw_canvas_bg = false;
			}

			canvas_idx++;
		}

		if (scenario_draw_canvas_bg) {
			// There may be an outstanding clear request if a clear was requested, but no 2D
			// elements were drawn. Clear now otherwise we copy over garbage from the render target.
			RendererTextureStorage::render_target_do_clear_request(p_viewport->render_target);
			if (!can_draw_3d) {
				RendererSceneCull::render_empty_scene(p_viewport->render_buffers,
					p_viewport->scenario, p_viewport->shadow_atlas,
					p_viewport->window_output_max_value);
			}
			else {
				_draw_3d(p_viewport);
			}
		}
	}

	if (RendererTextureStorage::render_target_is_clear_requested(p_viewport->render_target)) {
		// was never cleared in the end, force clear it
		RendererTextureStorage::render_target_do_clear_request(p_viewport->render_target);
	}

	if (RendererTextureStorage::render_target_get_msaa_needs_resolve(p_viewport->render_target)) {
		WARN_PRINT_ONCE("2D MSAA is enabled while there is no 2D content. Disable 2D MSAA for "
						"better performance.");
		RendererTextureStorage::render_target_do_msaa_resolve(p_viewport->render_target);
	}

	if (p_viewport->measure_render_time) {
		String rt_id = "vp_end_" + itos(p_viewport->self.get_id());
		RendererUtilities::capture_timestamp(rt_id);
		timestamp_vp_map[rt_id] = p_viewport->self;
	}
}

DisplayServerEnums::WindowID RendererViewport::_get_containing_window(Viewport* p_viewport)
{
	if (p_viewport->viewport_to_screen != DisplayServerEnums::INVALID_WINDOW_ID) {
		return p_viewport->viewport_to_screen;
	}

	if (p_viewport->parent.is_valid()) {
		Viewport* parent = viewport_owner.get_or_null(p_viewport->parent);
		if (parent) {
			return _get_containing_window(parent);
		}
	}

	return DisplayServerEnums::INVALID_WINDOW_ID;
}

RID RendererViewport::viewport_allocate() { return viewport_owner.allocate_rid(); }

void RendererViewport::viewport_initialize(RID p_rid)
{
	viewport_owner.initialize_rid(p_rid);
	Viewport* viewport = viewport_owner.get_or_null(p_rid);
	viewport->self = p_rid;
	viewport->render_target = RendererTextureStorage::render_target_create();
	viewport->shadow_atlas = RendererLightStorage::shadow_atlas_create();
	viewport->viewport_render_direct_to_screen = false;

	viewport->fsr_enabled = RendererCompositor::is_low_end() && !viewport->disable_3d;
}

#ifndef XR_DISABLED
void RendererViewport::viewport_set_use_xr(RID p_viewport, bool p_use_xr)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->use_xr == p_use_xr) {
		return;
	}

	viewport->use_xr = p_use_xr;

	// Re-configure the 3D render buffers when disabling XR. They'll get
	// re-configured when enabling XR in draw_viewports().
	if (!p_use_xr) {
		viewport->view_count = 1;
		_configure_3d_render_buffers(viewport);
	}
}
#endif // !XR_DISABLED

void RendererViewport::viewport_set_scaling_3d_mode(
	RID p_viewport, RSE::ViewportScaling3DMode p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	const String rendering_method = OS::get_current_rendering_method();
	if (rendering_method != "forward_plus") {
		if (p_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR) {
			WARN_PRINT_ONCE_ED(
				"FSR1 3D scaling is only available when using the Forward+ renderer.");
			return;
		}
		if (p_mode == RSE::VIEWPORT_SCALING_3D_MODE_FSR2) {
			WARN_PRINT_ONCE_ED(
				"FSR2 3D scaling is only available when using the Forward+ renderer.");
			return;
		}
		if (p_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL) {
			WARN_PRINT_ONCE_ED(
				"MetalFX Temporal 3D scaling is only available when using the Forward+ renderer.");
			return;
		}
	}
	if (rendering_method == "gl_compatibility" &&
		p_mode == RSE::VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL) {
		WARN_PRINT_ONCE_ED("MetalFX Spatial 3D scaling is only available when using the Forward+ "
						   "or Mobile renderer.");
		return;
	}

	if (viewport->scaling_3d_mode == p_mode) {
		return;
	}

	bool motion_vectors_before = _viewport_requires_motion_vectors(viewport);
	viewport->scaling_3d_mode = p_mode;

	bool motion_vectors_after = _viewport_requires_motion_vectors(viewport);
	if (motion_vectors_before != motion_vectors_after) {
		num_viewports_with_motion_vectors += motion_vectors_after ? 1 : -1;
	}

	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_fsr_sharpness(RID p_viewport, float p_sharpness)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->fsr_sharpness = p_sharpness;
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_texture_mipmap_bias(RID p_viewport, float p_mipmap_bias)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->texture_mipmap_bias = p_mipmap_bias;
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_anisotropic_filtering_level(
	RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->anisotropic_filtering_level = p_anisotropic_filtering_level;
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	// Clamp to reasonable values that are actually useful.
	// Values above 2.0 don't serve a practical purpose since the viewport
	// isn't displayed with mipmaps.
	if (viewport->scaling_3d_scale == CLAMP(p_scaling_3d_scale, 0.1, 2.0)) {
		return;
	}

	viewport->scaling_3d_scale = CLAMP(p_scaling_3d_scale, 0.1, 2.0);
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_size(
	RID p_viewport, int p_width, int p_height, int p_view_count)
{
	ERR_FAIL_COND(p_width < 0 || p_height < 0 || p_view_count < 0);

	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	_viewport_set_size(viewport, p_width, p_height, p_view_count);
}

void RendererViewport::_viewport_set_size(
	Viewport* p_viewport, int p_width, int p_height, uint32_t p_view_count)
{
	Size2i new_size(p_width, p_height);
	if (p_viewport->size != new_size || p_viewport->view_count != p_view_count) {
		p_viewport->size = new_size;
		p_viewport->view_count = p_view_count;

		RendererTextureStorage::render_target_set_size(
			p_viewport->render_target, p_width, p_height, p_view_count);
		_configure_3d_render_buffers(p_viewport);

		p_viewport->occlusion_buffer_dirty = true;
	}
}

bool RendererViewport::_viewport_requires_motion_vectors(Viewport* p_viewport)
{
	return p_viewport->use_taa ||
		   RSE::scaling_3d_mode_type(p_viewport->scaling_3d_mode) ==
			   RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL ||
		   p_viewport->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_MOTION_VECTORS ||
		   p_viewport->force_motion_vectors;
}

void RendererViewport::viewport_set_active(RID p_viewport, bool p_active)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (p_active) {
		ERR_FAIL_COND_MSG(
			active_viewports.has(viewport), "Can't make active a Viewport that is already active.");
		viewport->occlusion_buffer_dirty = true;
		active_viewports.push_back(viewport);
	}
	else {
		active_viewports.erase(viewport);
	}

	sorted_active_viewports_dirty = true;
}

void RendererViewport::viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->parent = p_parent_viewport;
}

void RendererViewport::viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->clear_mode = p_clear_mode;
}

void RendererViewport::viewport_attach_to_screen(
	RID p_viewport, const Rect2& p_rect, DisplayServerEnums::WindowID p_screen)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (p_screen != DisplayServerEnums::INVALID_WINDOW_ID) {
		// If using OpenGL we can optimize this operation by rendering directly to system_fbo
		// instead of rendering to fbo and copying to system_fbo after
		if (RendererCompositor::is_low_end() && viewport->viewport_render_direct_to_screen) {
			RendererTextureStorage::render_target_set_size(
				viewport->render_target, p_rect.size.x, p_rect.size.y, viewport->view_count);
			RendererTextureStorage::render_target_set_position(
				viewport->render_target, p_rect.position.x, p_rect.position.y);
		}

		viewport->viewport_to_screen_rect = p_rect;
		viewport->viewport_to_screen = p_screen;
	}
	else {
		// if render_direct_to_screen was used, reset size and position
		if (RendererCompositor::is_low_end() && viewport->viewport_render_direct_to_screen) {
			RendererTextureStorage::render_target_set_position(viewport->render_target, 0, 0);
			RendererTextureStorage::render_target_set_size(
				viewport->render_target, viewport->size.x, viewport->size.y, viewport->view_count);
		}

		viewport->viewport_to_screen_rect = Rect2();
		viewport->viewport_to_screen = DisplayServerEnums::INVALID_WINDOW_ID;
	}
}

void RendererViewport::viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (p_enable == viewport->viewport_render_direct_to_screen) {
		return;
	}

	// if disabled, reset render_target size and position
	if (!p_enable) {
		RendererTextureStorage::render_target_set_position(viewport->render_target, 0, 0);
		RendererTextureStorage::render_target_set_size(
			viewport->render_target, viewport->size.x, viewport->size.y, viewport->view_count);
	}

	RendererTextureStorage::render_target_set_direct_to_screen(viewport->render_target, p_enable);
	viewport->viewport_render_direct_to_screen = p_enable;

	// if attached to screen already, setup screen size and position, this needs to happen after
	// setting flag to avoid an unnecessary buffer allocation
	if (RendererCompositor::is_low_end() && viewport->viewport_to_screen_rect != Rect2() &&
		p_enable) {
		RendererTextureStorage::render_target_set_size(viewport->render_target,
			viewport->viewport_to_screen_rect.size.x, viewport->viewport_to_screen_rect.size.y,
			viewport->view_count);
		RendererTextureStorage::render_target_set_position(viewport->render_target,
			viewport->viewport_to_screen_rect.position.x,
			viewport->viewport_to_screen_rect.position.y);
	}
}

void RendererViewport::viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->update_mode = p_mode;
}

RSE::ViewportUpdateMode RendererViewport::viewport_get_update_mode(RID p_viewport) const
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, RSE::VIEWPORT_UPDATE_DISABLED);

	return viewport->update_mode;
}

RID RendererViewport::viewport_get_render_target(RID p_viewport) const
{
	const Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, RID());

	return viewport->render_target;
}

RID RendererViewport::viewport_get_texture(RID p_viewport) const
{
	const Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, RID());

	return RendererTextureStorage::render_target_get_texture(viewport->render_target);
}

RID RendererViewport::viewport_get_occluder_debug_texture(RID p_viewport) const
{
	const Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, RID());

	if (viewport->use_occlusion_culling &&
		viewport->debug_draw == RSE::VIEWPORT_DEBUG_DRAW_OCCLUDERS) {
		return RendererSceneOcclusionCull::buffer_get_debug_texture(p_viewport);
	}
	return RID();
}

void RendererViewport::viewport_set_prev_camera_data(
	RID p_viewport, const RendererSceneRender::CameraData* p_camera_data)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	uint64_t frame = RendererCompositor::get_frame_number();
	if (viewport->prev_camera_data_frame != frame) {
		viewport->prev_camera_data = *p_camera_data;
		viewport->prev_camera_data_frame = frame;
	}
}

const RendererSceneRender::CameraData* RendererViewport::viewport_get_prev_camera_data(
	RID p_viewport)
{
	const Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, nullptr);
	return &viewport->prev_camera_data;
}

void RendererViewport::viewport_set_disable_2d(RID p_viewport, bool p_disable)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->disable_2d = p_disable;
}

void RendererViewport::viewport_set_environment_mode(
	RID p_viewport, RSE::ViewportEnvironmentMode p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->disable_environment = p_mode;
}

bool RendererViewport::viewport_is_environment_disabled(Viewport* viewport)
{
	ERR_FAIL_NULL_V(viewport, false);

	if (viewport->parent.is_valid() &&
		viewport->disable_environment == RSE::VIEWPORT_ENVIRONMENT_INHERIT) {
		Viewport* parent = viewport_owner.get_or_null(viewport->parent);
		return viewport_is_environment_disabled(parent);
	}
	return viewport->disable_environment == RSE::VIEWPORT_ENVIRONMENT_DISABLED;
}

void RendererViewport::viewport_set_disable_3d(RID p_viewport, bool p_disable)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->disable_3d = p_disable;
}

void RendererViewport::viewport_attach_camera(RID p_viewport, RID p_camera)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->camera = p_camera;
}

void RendererViewport::viewport_set_scenario(RID p_viewport, RID p_scenario)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->scenario.is_valid()) {
		RendererSceneCull::scenario_remove_viewport_visibility_mask(viewport->scenario, p_viewport);
	}

	viewport->scenario = p_scenario;
	if (viewport->use_occlusion_culling) {
		RendererSceneOcclusionCull::buffer_set_scenario(p_viewport, p_scenario);
	}
}

void RendererViewport::viewport_attach_canvas(RID p_viewport, RID p_canvas)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	ERR_FAIL_COND(viewport->canvas_map.has(p_canvas));
	RendererCanvasCull::Canvas* canvas = Renderer::canvas->canvas_owner.get_or_null(p_canvas);
	ERR_FAIL_NULL(canvas);

	canvas->viewports.insert(p_viewport);
	viewport->canvas_map[p_canvas] = Viewport::CanvasData();
	viewport->canvas_map[p_canvas].layer = 0;
	viewport->canvas_map[p_canvas].sublayer = 0;
	viewport->canvas_map[p_canvas].canvas = canvas;
}

void RendererViewport::viewport_remove_canvas(RID p_viewport, RID p_canvas)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererCanvasCull::Canvas* canvas = Renderer::canvas->canvas_owner.get_or_null(p_canvas);
	ERR_FAIL_NULL(canvas);

	viewport->canvas_map.erase(p_canvas);
	canvas->viewports.erase(p_viewport);
}

void RendererViewport::viewport_set_canvas_transform(
	RID p_viewport, RID p_canvas, const Transform2D& p_offset)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	ERR_FAIL_COND(!viewport->canvas_map.has(p_canvas));
	viewport->canvas_map[p_canvas].transform = p_offset;
}

void RendererViewport::viewport_set_transparent_background(RID p_viewport, bool p_enabled)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	if (viewport->transparent_bg == p_enabled) {
		return;
	}

	RendererTextureStorage::render_target_set_transparent(viewport->render_target, p_enabled);
	viewport->transparent_bg = p_enabled;
}

void RendererViewport::viewport_set_global_canvas_transform(
	RID p_viewport, const Transform2D& p_transform)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->global_transform = p_transform;
}

void RendererViewport::viewport_set_canvas_stacking(
	RID p_viewport, RID p_canvas, int p_layer, int p_sublayer)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	ERR_FAIL_COND(!viewport->canvas_map.has(p_canvas));
	viewport->canvas_map[p_canvas].layer = p_layer;
	viewport->canvas_map[p_canvas].sublayer = p_sublayer;
}

void RendererViewport::viewport_set_positional_shadow_atlas_size(
	RID p_viewport, int p_size, bool p_16_bits)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->shadow_atlas_size = p_size;
	viewport->shadow_atlas_16_bits = p_16_bits;

	RendererLightStorage::shadow_atlas_set_size(
		viewport->shadow_atlas, viewport->shadow_atlas_size, viewport->shadow_atlas_16_bits);
}

void RendererViewport::viewport_set_positional_shadow_atlas_quadrant_subdivision(
	RID p_viewport, int p_quadrant, int p_subdiv)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererLightStorage::shadow_atlas_set_quadrant_subdivision(
		viewport->shadow_atlas, p_quadrant, p_subdiv);
}

void RendererViewport::viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->msaa_2d == p_msaa) {
		return;
	}
	viewport->msaa_2d = p_msaa;
	RendererTextureStorage::render_target_set_msaa(viewport->render_target, p_msaa);
}

void RendererViewport::viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->msaa_3d == p_msaa) {
		return;
	}
	viewport->msaa_3d = p_msaa;
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr_2d)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->use_hdr_2d == p_use_hdr_2d) {
		return;
	}
	viewport->use_hdr_2d = p_use_hdr_2d;
	RendererTextureStorage::render_target_set_use_hdr(viewport->render_target, p_use_hdr_2d);
	_configure_3d_render_buffers(viewport);
}

bool RendererViewport::viewport_is_using_hdr_2d(RID p_viewport) const
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, false);

	return viewport->use_hdr_2d;
}

void RendererViewport::viewport_set_screen_space_aa(
	RID p_viewport, RSE::ViewportScreenSpaceAA p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	if (OS::get_current_rendering_method() == "gl_compatibility" &&
		p_mode != RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED) {
		WARN_PRINT_ONCE_ED(
			"Screen-space AA is only available when using the Forward+ or Mobile renderer.");
		return;
	}

	if (viewport->screen_space_aa == p_mode) {
		return;
	}
	viewport->screen_space_aa = p_mode;
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_use_taa(RID p_viewport, bool p_use_taa)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	if (OS::get_current_rendering_method() != "forward_plus") {
		WARN_PRINT_ONCE_ED("TAA is only available when using the Forward+ renderer.");
		return;
	}

	if (viewport->use_taa == p_use_taa) {
		return;
	}

	bool motion_vectors_before = _viewport_requires_motion_vectors(viewport);
	viewport->use_taa = p_use_taa;

	bool motion_vectors_after = _viewport_requires_motion_vectors(viewport);
	if (motion_vectors_before != motion_vectors_after) {
		num_viewports_with_motion_vectors += motion_vectors_after ? 1 : -1;
	}

	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_use_debanding(RID p_viewport, bool p_use_debanding)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->use_debanding == p_use_debanding) {
		return;
	}
	viewport->use_debanding = p_use_debanding;
	RendererTextureStorage::render_target_set_use_debanding(
		viewport->render_target, p_use_debanding);
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_force_motion_vectors(
	RID p_viewport, bool p_force_motion_vectors)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	_viewport_set_force_motion_vectors(viewport, p_force_motion_vectors);
}

void RendererViewport::_viewport_set_force_motion_vectors(
	RendererViewport::Viewport* p_viewport, bool p_force_motion_vectors)
{
	if (p_viewport->force_motion_vectors == p_force_motion_vectors) {
		return;
	}

	bool motion_vectors_before = _viewport_requires_motion_vectors(p_viewport);
	p_viewport->force_motion_vectors = p_force_motion_vectors;

	bool motion_vectors_after = _viewport_requires_motion_vectors(p_viewport);
	if (motion_vectors_before != motion_vectors_after) {
		num_viewports_with_motion_vectors += motion_vectors_after ? 1 : -1;
	}

	_configure_3d_render_buffers(p_viewport);
}

void RendererViewport::viewport_set_use_occlusion_culling(
	RID p_viewport, bool p_use_occlusion_culling)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	if (viewport->use_occlusion_culling == p_use_occlusion_culling) {
		return;
	}
	viewport->use_occlusion_culling = p_use_occlusion_culling;

	if (viewport->use_occlusion_culling) {
		RendererSceneOcclusionCull::add_buffer(p_viewport);
		RendererSceneOcclusionCull::buffer_set_scenario(p_viewport, viewport->scenario);
	}
	else {
		RendererSceneOcclusionCull::remove_buffer(p_viewport);
	}

	viewport->occlusion_buffer_dirty = true;
}

void RendererViewport::viewport_set_occlusion_rays_per_thread(int p_rays_per_thread)
{
	if (occlusion_rays_per_thread == p_rays_per_thread) {
		return;
	}

	occlusion_rays_per_thread = p_rays_per_thread;

	for (int i = 0; i < active_viewports.size(); i++) {
		active_viewports[i]->occlusion_buffer_dirty = true;
	}
}

void RendererViewport::viewport_set_occlusion_culling_build_quality(
	RSE::ViewportOcclusionCullingBuildQuality p_quality)
{
	RendererSceneOcclusionCull::set_build_quality(p_quality);
}

void RendererViewport::viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->mesh_lod_threshold = p_pixels;
}

int RendererViewport::viewport_get_render_info(
	RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info)
{
	ERR_FAIL_INDEX_V(p_type, RSE::VIEWPORT_RENDER_INFO_TYPE_MAX, -1);
	ERR_FAIL_INDEX_V(p_info, RSE::VIEWPORT_RENDER_INFO_MAX, -1);

	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	if (!viewport) {
		return 0; // there should be a lock here..
	}

	return viewport->render_info.info[p_type][p_info];
}

void RendererViewport::viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	bool motion_vectors_before = _viewport_requires_motion_vectors(viewport);
	viewport->debug_draw = p_draw;

	bool motion_vectors_after = _viewport_requires_motion_vectors(viewport);
	if (motion_vectors_before != motion_vectors_after) {
		num_viewports_with_motion_vectors += motion_vectors_after ? 1 : -1;
	}
}

void RendererViewport::viewport_set_measure_render_time(RID p_viewport, bool p_enable)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->measure_render_time = p_enable;
}

float RendererViewport::viewport_get_measured_render_time_cpu(RID p_viewport) const
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, 0);

	return double(viewport->time_cpu_end - viewport->time_cpu_begin) / 1000.0;
}

float RendererViewport::viewport_get_measured_render_time_gpu(RID p_viewport) const
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL_V(viewport, 0);

	return double((viewport->time_gpu_end - viewport->time_gpu_begin) / 1000) / 1000.0;
}

void RendererViewport::viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	viewport->snap_2d_transforms_to_pixel = p_enabled;
}

void RendererViewport::viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	viewport->snap_2d_vertices_to_pixel = p_enabled;
}

void RendererViewport::viewport_set_default_canvas_item_texture_filter(
	RID p_viewport, RSE::CanvasItemTextureFilter p_filter)
{
	ERR_FAIL_COND_MSG(p_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT,
		"Viewport does not accept DEFAULT as texture filter (it's the topmost choice already).)");
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->texture_filter = p_filter;
}

void RendererViewport::viewport_set_default_canvas_item_texture_repeat(
	RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat)
{
	ERR_FAIL_COND_MSG(p_repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT,
		"Viewport does not accept DEFAULT as texture repeat (it's the topmost choice already).)");
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	viewport->texture_repeat = p_repeat;
}

void RendererViewport::viewport_set_sdf_oversize_and_scale(
	RID p_viewport, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererTextureStorage::render_target_set_sdf_size_and_scale(
		viewport->render_target, p_size, p_scale);
}

RID RendererViewport::viewport_find_from_screen_attachment(DisplayServerEnums::WindowID p_id) const
{
	RID* rids = nullptr;
	uint32_t rid_count = viewport_owner.get_rid_count();
	rids = (RID*)alloca(sizeof(RID) * rid_count);
	viewport_owner.fill_owned_buffer(rids);
	for (uint32_t i = 0; i < rid_count; i++) {
		Viewport* viewport = viewport_owner.get_or_null(rids[i]);
		if (viewport->viewport_to_screen == p_id) {
			return rids[i];
		}
	}
	return RID();
}

void RendererViewport::viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererTextureStorage::render_target_set_vrs_mode(viewport->render_target, p_mode);
	_configure_3d_render_buffers(viewport);
}

void RendererViewport::viewport_set_vrs_update_mode(
	RID p_viewport, RSE::ViewportVRSUpdateMode p_mode)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererTextureStorage::render_target_set_vrs_update_mode(viewport->render_target, p_mode);
}

void RendererViewport::viewport_set_vrs_texture(RID p_viewport, RID p_texture)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);

	RendererTextureStorage::render_target_set_vrs_texture(viewport->render_target, p_texture);
	_configure_3d_render_buffers(viewport);
}

bool RendererViewport::free(RID p_rid)
{
	if (viewport_owner.owns(p_rid)) {
		Viewport* viewport = viewport_owner.get_or_null(p_rid);

		RendererTextureStorage::render_target_free(viewport->render_target);
		RendererLightStorage::shadow_atlas_free(viewport->shadow_atlas);
		if (viewport->render_buffers.is_valid()) {
			viewport->render_buffers.unref();
		}

		while (viewport->canvas_map.begin()) {
			viewport_remove_canvas(p_rid, viewport->canvas_map.begin()->key);
		}

		viewport_set_scenario(p_rid, RID());
		active_viewports.erase(viewport);
		sorted_active_viewports_dirty = true;

		if (viewport->use_occlusion_culling) {
			RendererSceneOcclusionCull::remove_buffer(p_rid);
		}

		if (_viewport_requires_motion_vectors(viewport)) {
			num_viewports_with_motion_vectors--;
		}

		viewport_owner.free(p_rid);

		return true;
	}

	return false;
}

void RendererViewport::handle_timestamp(
	String p_timestamp, uint64_t p_cpu_time, uint64_t p_gpu_time)
{
	RID* vp = timestamp_vp_map.getptr(p_timestamp);
	if (!vp) {
		return;
	}

	Viewport* viewport = viewport_owner.get_or_null(*vp);
	if (!viewport) {
		return;
	}

	if (p_timestamp.begins_with("vp_begin")) {
		viewport->time_cpu_begin = p_cpu_time;
		viewport->time_gpu_begin = p_gpu_time;
	}

	if (p_timestamp.begins_with("vp_end")) {
		viewport->time_cpu_end = p_cpu_time;
		viewport->time_gpu_end = p_gpu_time;
	}
}

void RendererViewport::viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask)
{
	Viewport* viewport = viewport_owner.get_or_null(p_viewport);
	ERR_FAIL_NULL(viewport);
	viewport->canvas_cull_mask = p_canvas_cull_mask;
}

// Workaround for setting this on thread.
void RendererViewport::call_set_vsync_mode(
	DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window)
{
	DisplayServer::window_set_vsync_mode(p_mode, p_window);
}

int RendererViewport::get_total_objects_drawn() const { return total_objects_drawn; }

int RendererViewport::get_total_primitives_drawn() const { return total_vertices_drawn; }

int RendererViewport::get_total_draw_calls_used() const { return total_draw_calls_used; }

int RendererViewport::get_num_viewports_with_motion_vectors() const
{
	return num_viewports_with_motion_vectors;
}

void RendererViewport::draw_viewports(bool p_swap_buffers)
{
	draw_viewports_pass++;
	total_objects_drawn = 0;
	total_vertices_drawn = 0;
	total_draw_calls_used = 0;

	uint32_t rid_count = viewport_owner.get_rid_count();

	if (rid_count == 0) {
		if (p_swap_buffers) {
			DisplayServer::swap_buffers();
		}
		return;
	}

	LocalVector<RID> rids;
	rids.resize(rid_count);
	viewport_owner.fill_owned_buffer(rids.ptr());

	LocalVector<Viewport*> viewports;
	for (uint32_t i = 0; i < rid_count; i++) {
		Viewport* v = viewport_owner.get_or_null(rids[i]);
		if (!v) {
			continue;
		}

		if (v->update_mode == RenderingServerEnums::VIEWPORT_UPDATE_DISABLED) {
			continue;
		}

		if (v->update_mode == RenderingServerEnums::VIEWPORT_UPDATE_ONCE) {
			v->update_mode = RenderingServerEnums::VIEWPORT_UPDATE_DISABLED;
		}

		viewports.push_back(v);
	}

	viewports.sort_custom<ViewportSort>();

	HashMap<DisplayServerEnums::WindowID, LocalVector<RenderingServerTypes::BlitToScreen>>
		blit_to_screen;

	for (uint32_t i = 0; i < viewports.size(); i++) {
		Viewport* v = viewports[i];
		_draw_viewport(v);
		if (v->viewport_to_screen != DisplayServerEnums::INVALID_WINDOW_ID &&
			v->render_target.is_valid()) {
			RenderingServerTypes::BlitToScreen blit = {};
			blit.render_target = v->render_target;
			blit.src_rect = Rect2(0, 0, v->size.x, v->size.y);

			if (v->viewport_to_screen_rect.size.x > 0.0f &&
				v->viewport_to_screen_rect.size.y > 0.0f &&
				v->viewport_to_screen_rect.size.x <= 16384.0f &&
				v->viewport_to_screen_rect.size.y <= 16384.0f) {
				blit.dst_rect = v->viewport_to_screen_rect;
			}
			else {
				blit.dst_rect = Rect2(0, 0, v->size.x, v->size.y);
			}

			blit_to_screen[v->viewport_to_screen].push_back(blit);
		}
	}

	for (KeyValue<DisplayServerEnums::WindowID, LocalVector<RenderingServerTypes::BlitToScreen>>&
			 E : blit_to_screen) {
		RendererCompositor::blit_render_targets_to_screen(E.key, E.value.ptr(), E.value.size());
	}

	if (p_swap_buffers) {
		DisplayServer::swap_buffers();
	}
}

// ============================================================================
// Merged from servers/rendering/renderer_scene_render.cpp
// ============================================================================

#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/display/display_server.h"
#include "core/os/os.h"

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
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_render.h"

// --- Merged from servers/rendering/renderer_scene_render.cpp ---

void RendererSceneRender::CameraData::set_camera(const Transform3D p_transform,
	const Projection p_projection, bool p_is_orthogonal, bool p_vaspect,
	const Vector2& p_taa_jitter, float p_taa_frame_count, uint32_t p_visible_layers)
{
	view_count = 1;
	is_orthogonal = p_is_orthogonal;
	vaspect = p_vaspect;

	main_transform = p_transform;
	main_projection = p_projection;

	visible_layers = p_visible_layers;
	view_offset[0] = Transform3D();
	view_projection[0] = p_projection;
	taa_jitter = p_taa_jitter;
	taa_frame_count = p_taa_frame_count;
}

void RendererSceneRender::CameraData::set_multiview_camera(uint32_t p_view_count,
	const Transform3D* p_transforms, const Projection* p_projections, bool p_is_orthogonal,
	bool p_vaspect, uint32_t p_visible_layers)
{
	ERR_FAIL_COND_MSG(p_view_count != 2, "Incorrect view count for stereoscopic view");

	visible_layers = p_visible_layers;
	view_count = p_view_count;
	is_orthogonal = p_is_orthogonal;
	vaspect = p_vaspect;
	Vector<Plane> planes[2];

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our center transform

	// 1. obtain our planes
	for (uint32_t v = 0; v < view_count; v++) {
		planes[v] = p_projections[v].get_projection_planes(p_transforms[v]);
	}

	// 2. average and normalize plane normals to obtain z vector, cross them to obtain y vector, and
	// from there the x vector for combined camera basis.
	Vector3 n0 = planes[0][Projection::PLANE_LEFT].normal;
	Vector3 n1 = planes[1][Projection::PLANE_RIGHT].normal;
	Vector3 z = (n0 + n1).normalized();
	Vector3 y = n0.cross(n1).normalized();
	Vector3 x = y.cross(z).normalized();
	y = z.cross(x).normalized();
	main_transform.basis.set_columns(x, y, z);

	// 3. create a horizon plane with one of the eyes and the up vector as normal.
	Plane horizon(y, p_transforms[0].origin);

	// 4. Intersect horizon, left and right to obtain the combined camera origin.
	ERR_FAIL_COND_MSG(!horizon.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_RIGHT], &main_transform.origin),
		"Can't determine camera origin");

	// handy to have the inverse of the transform we just build
	Transform3D main_transform_inv = main_transform.inverse();

	// 5. figure out far plane, this could use some improvement, we may have our far plane too close
	// like this, not sure if this matters
	Vector3 far_center = (planes[0][Projection::PLANE_FAR].get_center() +
							 planes[1][Projection::PLANE_FAR].get_center()) *
						 0.5;
	Plane far_plane = Plane(-z, far_center);

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our top/bottom planes

	// 6. Intersect far and left planes with top planes from both eyes, save the point with highest
	// y as top_left.
	Vector3 top_left, other;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[0][Projection::PLANE_TOP], &top_left),
		"Can't determine left camera far/left/top vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_TOP], &other),
		"Can't determine right camera far/left/top vector");
	if (y.dot(top_left) < y.dot(other)) {
		top_left = other;
	}

	// 7. Intersect far and left planes with bottom planes from both eyes, save the point with
	// lowest y as bottom_left.
	Vector3 bottom_left;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[0][Projection::PLANE_BOTTOM], &bottom_left),
		"Can't determine left camera far/left/bottom vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_BOTTOM], &other),
		"Can't determine right camera far/left/bottom vector");
	if (y.dot(other) < y.dot(bottom_left)) {
		bottom_left = other;
	}

	// 8. Intersect far and right planes with top planes from both eyes, save the point with highest
	// y as top_right.
	Vector3 top_right;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_RIGHT],
						  planes[0][Projection::PLANE_TOP], &top_right),
		"Can't determine left camera far/right/top vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_RIGHT],
						  planes[1][Projection::PLANE_TOP], &other),
		"Can't determine right camera far/right/top vector");
	if (y.dot(top_right) < y.dot(other)) {
		top_right = other;
	}

	//  9. Intersect far and right planes with bottom planes from both eyes, save the point with
	//  lowest y as bottom_right.
	Vector3 bottom_right;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_RIGHT],
						  planes[0][Projection::PLANE_BOTTOM], &bottom_right),
		"Can't determine left camera far/right/bottom vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_RIGHT],
						  planes[1][Projection::PLANE_BOTTOM], &other),
		"Can't determine right camera far/right/bottom vector");
	if (y.dot(other) < y.dot(bottom_right)) {
		bottom_right = other;
	}

	// 10. Create top plane with these points: camera origin, top_left, top_right
	Plane top(main_transform.origin, top_left, top_right);

	// 11. Create bottom plane with these points: camera origin, bottom_left, bottom_right
	Plane bottom(main_transform.origin, bottom_left, bottom_right);

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our near plane points

	// 12. Create a near plane using -camera z and the eye further along in that axis.
	Plane near_plane;
	Vector3 neg_z = -z;
	if (neg_z.dot(p_transforms[1].origin) < neg_z.dot(p_transforms[0].origin)) {
		near_plane = Plane(neg_z, p_transforms[0].origin);
	}
	else {
		near_plane = Plane(neg_z, p_transforms[1].origin);
	}

	// 13. Intersect near plane with bottm/left planes, to obtain min_vec then top/right to obtain
	// max_vec
	Vector3 min_vec;
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(bottom, planes[0][Projection::PLANE_LEFT], &min_vec),
		"Can't determine left camera near/left/bottom vector");
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(bottom, planes[1][Projection::PLANE_LEFT], &other),
		"Can't determine right camera near/left/bottom vector");
	if (x.dot(other) < x.dot(min_vec)) {
		min_vec = other;
	}

	Vector3 max_vec;
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(top, planes[0][Projection::PLANE_RIGHT], &max_vec),
		"Can't determine left camera near/right/top vector");
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(top, planes[1][Projection::PLANE_RIGHT], &other),
		"Can't determine right camera near/right/top vector");
	if (x.dot(max_vec) < x.dot(other)) {
		max_vec = other;
	}

	// 14. transform these points by the inverse camera to obtain local_min_vec and local_max_vec
	Vector3 local_min_vec = main_transform_inv.xform(min_vec);
	Vector3 local_max_vec = main_transform_inv.xform(max_vec);

	// 15. get x and y from these to obtain left, top, right bottom for the frustum. Get the
	// distance from near plane to camera origin to obtain near, and the distance from the far plane
	// to the camera origin to obtain far.
	float z_near = -near_plane.distance_to(main_transform.origin);
	float z_far = -far_plane.distance_to(main_transform.origin);

	// 16. Use this to build the combined camera matrix.
	main_projection.set_frustum(
		local_min_vec.x, local_max_vec.x, local_min_vec.y, local_max_vec.y, z_near, z_far);

	// 3. Copy our view data
	for (uint32_t v = 0; v < view_count; v++) {
		view_offset[v] = main_transform_inv * p_transforms[v];
		view_projection[v] = p_projections[v] * Projection(view_offset[v].inverse());
	}
}

RID RendererSceneRender::compositor_effect_allocate()
{
	return RendererCompositorStorage::compositor_effect_allocate();
}

void RendererSceneRender::compositor_effect_initialize(RID p_rid)
{
	RendererCompositorStorage::compositor_effect_initialize(p_rid);
}

void RendererSceneRender::compositor_effect_free(RID p_rid)
{
	RendererCompositorStorage::compositor_effect_free(p_rid);
}

bool RendererSceneRender::is_compositor_effect(RID p_effect)
{
	return RendererCompositorStorage::is_compositor_effect(p_effect);
}

void RendererSceneRender::compositor_effect_set_enabled(RID p_effect, bool p_enabled)
{
	RendererCompositorStorage::compositor_effect_set_enabled(p_effect, p_enabled);
}

void RendererSceneRender::compositor_effect_set_flag(
	RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set)
{
	RendererCompositorStorage::compositor_effect_set_flag(p_effect, p_flag, p_set);
}

RID RendererSceneRender::compositor_allocate() { return RendererCompositorStorage::compositor_allocate(); }

void RendererSceneRender::compositor_initialize(RID p_rid)
{
	RendererCompositorStorage::compositor_initialize(p_rid);
}

void RendererSceneRender::compositor_free(RID p_rid) { RendererCompositorStorage::compositor_free(p_rid); }

bool RendererSceneRender::is_compositor(RID p_rid)
{
	return RendererCompositorStorage::is_compositor(p_rid);
}

RID RendererSceneRender::environment_allocate()
{
	return RendererEnvironmentStorage::environment_allocate();
}

void RendererSceneRender::environment_initialize(RID p_rid)
{
	RendererEnvironmentStorage::environment_initialize(p_rid);
}

void RendererSceneRender::environment_free(RID p_rid)
{
	RendererEnvironmentStorage::environment_free(p_rid);
}

bool RendererSceneRender::is_environment(RID p_rid)
{
	return RendererEnvironmentStorage::is_environment(p_rid);
}

void RendererSceneRender::environment_set_background(RID p_env, RSE::EnvironmentBG p_bg)
{
	RendererEnvironmentStorage::environment_set_background(p_env, p_bg);
}

void RendererSceneRender::environment_set_sky(RID p_env, RID p_sky)
{
	RendererEnvironmentStorage::environment_set_sky(p_env, p_sky);
}

void RendererSceneRender::environment_set_sky_custom_fov(RID p_env, float p_scale)
{
	RendererEnvironmentStorage::environment_set_sky_custom_fov(p_env, p_scale);
}

void RendererSceneRender::environment_set_sky_orientation(RID p_env, const Basis& p_orientation)
{
	RendererEnvironmentStorage::environment_set_sky_orientation(p_env, p_orientation);
}

void RendererSceneRender::environment_set_bg_color(RID p_env, const Color& p_color)
{
	RendererEnvironmentStorage::environment_set_bg_color(p_env, p_color);
}

void RendererSceneRender::environment_set_bg_energy(
	RID p_env, float p_multiplier, float p_exposure_value)
{
	RendererEnvironmentStorage::environment_set_bg_energy(p_env, p_multiplier, p_exposure_value);
}

void RendererSceneRender::environment_set_canvas_max_layer(RID p_env, int p_max_layer)
{
	RendererEnvironmentStorage::environment_set_canvas_max_layer(p_env, p_max_layer);
}

void RendererSceneRender::environment_set_ambient_light(RID p_env, const Color& p_color,
	RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution,
	RSE::EnvironmentReflectionSource p_reflection_source)
{
	RendererEnvironmentStorage::environment_set_ambient_light(
		p_env, p_color, p_ambient, p_energy, p_sky_contribution, p_reflection_source);
}

RSE::EnvironmentBG RendererSceneRender::environment_get_background(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_background(p_env);
}

RID RendererSceneRender::environment_get_sky(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky(p_env);
}

float RendererSceneRender::environment_get_sky_custom_fov(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky_custom_fov(p_env);
}

Basis RendererSceneRender::environment_get_sky_orientation(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky_orientation(p_env);
}

Color RendererSceneRender::environment_get_bg_color(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_color(p_env);
}

float RendererSceneRender::environment_get_bg_energy_multiplier(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_energy_multiplier(p_env);
}

float RendererSceneRender::environment_get_bg_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_intensity(p_env);
}

int RendererSceneRender::environment_get_canvas_max_layer(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_canvas_max_layer(p_env);
}

RSE::EnvironmentAmbientSource RendererSceneRender::environment_get_ambient_source(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_source(p_env);
}

Color RendererSceneRender::environment_get_ambient_light(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_light(p_env);
}

float RendererSceneRender::environment_get_ambient_light_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_light_energy(p_env);
}

float RendererSceneRender::environment_get_ambient_sky_contribution(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_sky_contribution(p_env);
}

RSE::EnvironmentReflectionSource RendererSceneRender::environment_get_reflection_source(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_reflection_source(p_env);
}

void RendererSceneRender::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id)
{
	RendererEnvironmentStorage::environment_set_camera_feed_id(p_env, p_camera_feed_id);
}

int RendererSceneRender::environment_get_camera_feed_id(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_camera_feed_id(p_env);
}

void RendererSceneRender::environment_set_tonemap(
	RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white)
{
	RendererEnvironmentStorage::environment_set_tonemap(p_env, p_tone_mapper, p_exposure, p_white);
}

RSE::EnvironmentToneMapper RendererSceneRender::environment_get_tone_mapper(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_tone_mapper(p_env);
}

float RendererSceneRender::environment_get_exposure(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_exposure(p_env);
}

float RendererSceneRender::environment_get_white(
	RID p_env, bool p_limit_agx_white, float p_output_max_value)
{
	return RendererEnvironmentStorage::environment_get_white(p_env, p_limit_agx_white, p_output_max_value);
}

void RendererSceneRender::environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast)
{
	RendererEnvironmentStorage::environment_set_tonemap_agx_contrast(p_env, p_agx_contrast);
}

float RendererSceneRender::environment_get_tonemap_agx_contrast(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_tonemap_agx_contrast(p_env);
}

RendererEnvironmentStorage::TonemapParameters
RendererSceneRender::environment_get_tonemap_parameters(
	RID p_env, bool p_limit_agx_white, float p_output_max_value)
{
	return RendererEnvironmentStorage::environment_get_tonemap_parameters(
		p_env, p_limit_agx_white, p_output_max_value);
}

void RendererSceneRender::environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
	float p_light_energy, float p_sun_scatter, float p_density, float p_height,
	float p_height_density, float p_aerial_perspective, float p_sky_affect,
	RSE::EnvironmentFogMode p_mode)
{
	RendererEnvironmentStorage::environment_set_fog(p_env, p_enable, p_light_color, p_light_energy,
		p_sun_scatter, p_density, p_height, p_height_density, p_aerial_perspective, p_sky_affect,
		p_mode);
}

bool RendererSceneRender::environment_get_fog_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_enabled(p_env);
}

RSE::EnvironmentFogMode RendererSceneRender::environment_get_fog_mode(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_mode(p_env);
}

Color RendererSceneRender::environment_get_fog_light_color(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_light_color(p_env);
}

float RendererSceneRender::environment_get_fog_light_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_light_energy(p_env);
}

float RendererSceneRender::environment_get_fog_sun_scatter(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_sun_scatter(p_env);
}

float RendererSceneRender::environment_get_fog_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_density(p_env);
}

float RendererSceneRender::environment_get_fog_sky_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_sky_affect(p_env);
}

float RendererSceneRender::environment_get_fog_height(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_height(p_env);
}

float RendererSceneRender::environment_get_fog_height_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_height_density(p_env);
}

float RendererSceneRender::environment_get_fog_aerial_perspective(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_aerial_perspective(p_env);
}

void RendererSceneRender::environment_set_fog_depth(
	RID p_env, float p_curve, float p_begin, float p_end)
{
	RendererEnvironmentStorage::environment_set_fog_depth(p_env, p_curve, p_begin, p_end);
}

float RendererSceneRender::environment_get_fog_depth_curve(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_curve(p_env);
}

float RendererSceneRender::environment_get_fog_depth_begin(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_begin(p_env);
}

float RendererSceneRender::environment_get_fog_depth_end(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_end(p_env);
}

void RendererSceneRender::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
	const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
	float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
	float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect)
{
	RendererEnvironmentStorage::environment_set_volumetric_fog(p_env, p_enable, p_density, p_albedo,
		p_emission, p_emission_energy, p_anisotropy, p_length, p_detail_spread, p_gi_inject,
		p_temporal_reprojection, p_temporal_reprojection_amount, p_ambient_inject, p_sky_affect);
}

bool RendererSceneRender::environment_get_volumetric_fog_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_enabled(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_density(p_env);
}

Color RendererSceneRender::environment_get_volumetric_fog_scattering(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_scattering(p_env);
}

Color RendererSceneRender::environment_get_volumetric_fog_emission(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_emission(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_emission_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_emission_energy(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_anisotropy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_anisotropy(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_length(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_length(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_detail_spread(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_detail_spread(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_gi_inject(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_gi_inject(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_sky_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_sky_affect(p_env);
}

bool RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection_amount(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_ambient_inject(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_ambient_inject(p_env);
}

void RendererSceneRender::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
	float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
	RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map)
{
	RendererEnvironmentStorage::environment_set_glow(p_env, p_enable, p_levels, p_intensity, p_strength,
		p_mix, p_bloom_threshold, p_blend_mode, p_hdr_bleed_threshold, p_hdr_bleed_scale,
		p_hdr_luminance_cap, p_glow_map_strength, p_glow_map);
}

bool RendererSceneRender::environment_get_glow_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_enabled(p_env);
}

Vector<float> RendererSceneRender::environment_get_glow_levels(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_levels(p_env);
}

float RendererSceneRender::environment_get_glow_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_intensity(p_env);
}

float RendererSceneRender::environment_get_glow_strength(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_strength(p_env);
}

float RendererSceneRender::environment_get_glow_bloom(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_bloom(p_env);
}

float RendererSceneRender::environment_get_glow_mix(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_mix(p_env);
}

RSE::EnvironmentGlowBlendMode RendererSceneRender::environment_get_glow_blend_mode(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_blend_mode(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_bleed_threshold(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_bleed_threshold(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_luminance_cap(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_luminance_cap(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_bleed_scale(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_bleed_scale(p_env);
}

float RendererSceneRender::environment_get_glow_map_strength(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_map_strength(p_env);
}

RID RendererSceneRender::environment_get_glow_map(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_map(p_env);
}

void RendererSceneRender::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps,
	float p_fade_int, float p_fade_out, float p_depth_tolerance)
{
	RendererEnvironmentStorage::environment_set_ssr(
		p_env, p_enable, p_max_steps, p_fade_int, p_fade_out, p_depth_tolerance);
}

bool RendererSceneRender::environment_get_ssr_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_enabled(p_env);
}

int RendererSceneRender::environment_get_ssr_max_steps(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_max_steps(p_env);
}

float RendererSceneRender::environment_get_ssr_fade_in(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_fade_in(p_env);
}

float RendererSceneRender::environment_get_ssr_fade_out(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_fade_out(p_env);
}

float RendererSceneRender::environment_get_ssr_depth_tolerance(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_depth_tolerance(p_env);
}

void RendererSceneRender::environment_set_ssao(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_power, float p_detail, float p_horizon, float p_sharpness,
	float p_light_affect, float p_ao_channel_affect)
{
	RendererEnvironmentStorage::environment_set_ssao(p_env, p_enable, p_radius, p_intensity, p_power,
		p_detail, p_horizon, p_sharpness, p_light_affect, p_ao_channel_affect);
}

bool RendererSceneRender::environment_get_ssao_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_enabled(p_env);
}

float RendererSceneRender::environment_get_ssao_radius(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_radius(p_env);
}

float RendererSceneRender::environment_get_ssao_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_intensity(p_env);
}

float RendererSceneRender::environment_get_ssao_power(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_power(p_env);
}

float RendererSceneRender::environment_get_ssao_detail(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_detail(p_env);
}

float RendererSceneRender::environment_get_ssao_horizon(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_horizon(p_env);
}

float RendererSceneRender::environment_get_ssao_sharpness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_sharpness(p_env);
}

float RendererSceneRender::environment_get_ssao_direct_light_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_direct_light_affect(p_env);
}

float RendererSceneRender::environment_get_ssao_ao_channel_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_ao_channel_affect(p_env);
}

void RendererSceneRender::environment_set_ssil(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_sharpness, float p_normal_rejection)
{
	RendererEnvironmentStorage::environment_set_ssil(
		p_env, p_enable, p_radius, p_intensity, p_sharpness, p_normal_rejection);
}

bool RendererSceneRender::environment_get_ssil_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_enabled(p_env);
}

float RendererSceneRender::environment_get_ssil_radius(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_radius(p_env);
}

float RendererSceneRender::environment_get_ssil_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_intensity(p_env);
}

float RendererSceneRender::environment_get_ssil_sharpness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_sharpness(p_env);
}

float RendererSceneRender::environment_get_ssil_normal_rejection(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_normal_rejection(p_env);
}

void RendererSceneRender::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
	float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
	float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
	float p_probe_bias)
{
	RendererEnvironmentStorage::environment_set_sdfgi(p_env, p_enable, p_cascades, p_min_cell_size,
		p_y_scale, p_use_occlusion, p_bounce_feedback, p_read_sky, p_energy, p_normal_bias,
		p_probe_bias);
}

bool RendererSceneRender::environment_get_sdfgi_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_enabled(p_env);
}

int RendererSceneRender::environment_get_sdfgi_cascades(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_cascades(p_env);
}

float RendererSceneRender::environment_get_sdfgi_min_cell_size(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_min_cell_size(p_env);
}

bool RendererSceneRender::environment_get_sdfgi_use_occlusion(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_use_occlusion(p_env);
}

float RendererSceneRender::environment_get_sdfgi_bounce_feedback(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_bounce_feedback(p_env);
}

bool RendererSceneRender::environment_get_sdfgi_read_sky_light(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_read_sky_light(p_env);
}

float RendererSceneRender::environment_get_sdfgi_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_energy(p_env);
}

float RendererSceneRender::environment_get_sdfgi_normal_bias(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_normal_bias(p_env);
}

float RendererSceneRender::environment_get_sdfgi_probe_bias(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_probe_bias(p_env);
}

RSE::EnvironmentSDFGIYScale RendererSceneRender::environment_get_sdfgi_y_scale(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_y_scale(p_env);
}

void RendererSceneRender::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
	float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction)
{
	RendererEnvironmentStorage::environment_set_adjustment(p_env, p_enable, p_brightness, p_contrast,
		p_saturation, p_use_1d_color_correction, p_color_correction);
}

bool RendererSceneRender::environment_get_adjustments_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_enabled(p_env);
}

float RendererSceneRender::environment_get_adjustments_brightness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_brightness(p_env);
}

float RendererSceneRender::environment_get_adjustments_contrast(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_contrast(p_env);
}

float RendererSceneRender::environment_get_adjustments_saturation(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_saturation(p_env);
}

bool RendererSceneRender::environment_get_use_1d_color_correction(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_use_1d_color_correction(p_env);
}

RID RendererSceneRender::environment_get_color_correction(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_color_correction(p_env);
}

RenderGeometryInstance* RendererSceneRender::geometry_instance_create(RID p_base)
{
	return nullptr;
}

void RendererSceneRender::geometry_instance_free(RenderGeometryInstance* p_geometry_instance) {}

uint32_t RendererSceneRender::geometry_instance_get_pair_mask() { return 0; }

uint32_t RendererSceneRender::get_max_lights_total() { return 0; }

uint32_t RendererSceneRender::get_max_lights_per_mesh() { return 0; }

void RendererSceneRender::mesh_generate_pipelines(RID p_mesh, bool p_background_compilation) {}

uint32_t RendererSceneRender::get_pipeline_compilations(
	RenderingServerEnums::PipelineSource p_source)
{
	return 0;
}

void RendererSceneRender::sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers,
	RID p_environment, const Vector3& p_world_position)
{
}

int RendererSceneRender::sdfgi_get_pending_region_count(
	const Ref<RenderSceneBuffers>& p_render_buffers) const
{
	return 0;
}

AABB RendererSceneRender::sdfgi_get_pending_region_bounds(
	const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const
{
	return AABB();
}

uint32_t RendererSceneRender::sdfgi_get_pending_region_cascade(
	const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const
{
	return 0;
}

void RendererSceneRender::environment_set_ssr_half_size(bool p_enable) {}

void RendererSceneRender::environment_set_ssr_roughness_quality(
	RenderingServerEnums::EnvironmentSSRRoughnessQuality p_quality)
{
}

void RendererSceneRender::environment_set_ssao_quality(
	RenderingServerEnums::EnvironmentSSAOQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
}

void RendererSceneRender::environment_set_ssil_quality(
	RenderingServerEnums::EnvironmentSSILQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
}

void RendererSceneRender::sub_surface_scattering_set_quality(
	RenderingServerEnums::SubSurfaceScatteringQuality p_quality)
{
}

void RendererSceneRender::sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) {}

RID RendererSceneRender::sky_allocate() { return RID(); }

void RendererSceneRender::sky_initialize(RID p_rid) {}

void RendererSceneRender::sky_set_radiance_size(RID p_sky, int p_radiance_size) {}

void RendererSceneRender::sky_set_mode(RID p_sky, RenderingServerEnums::SkyMode p_mode) {}

void RendererSceneRender::sky_set_material(RID p_sky, RID p_material) {}

Ref<Image> RendererSceneRender::sky_bake_panorama(
	RID p_sky, float p_energy, bool p_bake_irradiance, const Vector2i& p_size)
{
	return Ref<Image>();
}

void RendererSceneRender::environment_set_volumetric_fog_volume_size(int p_size, int p_depth) {}

void RendererSceneRender::environment_set_volumetric_fog_filter_active(bool p_enable) {}

void RendererSceneRender::environment_glow_set_use_bicubic_upscale(bool p_enable) {}

void RendererSceneRender::environment_set_sdfgi_ray_count(
	RenderingServerEnums::EnvironmentSDFGIRayCount p_count)
{
}

void RendererSceneRender::environment_set_sdfgi_frames_to_converge(
	RenderingServerEnums::EnvironmentSDFGIFramesToConverge p_frames)
{
}

void RendererSceneRender::environment_set_sdfgi_frames_to_update_light(
	RenderingServerEnums::EnvironmentSDFGIFramesToUpdateLight p_frames)
{
}

Ref<Image> RendererSceneRender::environment_bake_panorama(
	RID p_env, bool p_bake_irradiance, const Vector2i& p_size)
{
	return Ref<Image>();
}

void RendererSceneRender::positional_soft_shadow_filter_set_quality(
	RenderingServerEnums::ShadowQuality p_quality)
{
}

void RendererSceneRender::directional_soft_shadow_filter_set_quality(
	RenderingServerEnums::ShadowQuality p_quality)
{
}

RID RendererSceneRender::fog_volume_instance_create(RID p_fog_volume) { return RID(); }

void RendererSceneRender::fog_volume_instance_set_transform(
	RID p_instance, const Transform3D& p_transform)
{
}

void RendererSceneRender::fog_volume_instance_set_active(RID p_instance, bool p_active) {}

RID RendererSceneRender::fog_volume_instance_get_volume(RID p_instance) const { return RID(); }

Vector3 RendererSceneRender::fog_volume_instance_get_position(RID p_instance) const
{
	return Vector3();
}

RID RendererSceneRender::voxel_gi_instance_create(RID p_base) { return RID(); }

void RendererSceneRender::voxel_gi_instance_set_transform_to_data(
	RID p_probe, const Transform3D& p_xform)
{
}

bool RendererSceneRender::voxel_gi_needs_update(RID p_probe) const { return false; }

void RendererSceneRender::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
}

void RendererSceneRender::voxel_gi_set_quality(RenderingServerEnums::VoxelGIQuality p_quality) {}

void RendererSceneRender::render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
	const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
	const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
	const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
	const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
	const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
	RID p_compositor, RID p_shadow_atlas, RID p_occlusion_buffer, RID p_reflection_atlas,
	RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
	const RenderShadowData* p_render_shadow_data, int p_render_shadow_count,
	const RenderSDFGIData* p_render_sdfgi_data, int p_render_sdfgi_count,
	float p_sdfgi_cascade_size, const RenderSDFGIUpdateData* p_render_sdfgi_update_data,
	RenderingServerTypes::RenderInfo* p_render_info)
{
}

void RendererSceneRender::render_material(const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal,
	const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
	const Rect2i& p_region)
{
}

void RendererSceneRender::render_particle_collider_heightfield(RID p_collider,
	const Transform3D& p_transform, const PagedArray<RenderGeometryInstance*>& p_instances)
{
}

void RendererSceneRender::set_scene_pass(uint64_t p_pass) {}

void RendererSceneRender::set_time(double p_time, double p_step) {}

void RendererSceneRender::set_debug_draw_mode(RenderingServerEnums::ViewportDebugDraw p_debug_draw)
{
}

Ref<RenderSceneBuffers> RendererSceneRender::render_buffers_create()
{
	return Ref<RenderSceneBuffers>();
}

void RendererSceneRender::gi_set_use_half_resolution(bool p_enable) {}

void RendererSceneRender::screen_space_roughness_limiter_set_active(
	bool p_enable, float p_amount, float p_limit)
{
}

bool RendererSceneRender::screen_space_roughness_limiter_is_active() { return false; }

bool RendererSceneRender::free(RID p_rid) { return false; }

void RendererSceneRender::sdfgi_set_debug_probe_select(
	const Vector3& p_position, const Vector3& p_dir)
{
}

void RendererSceneRender::decals_set_filter(RenderingServerEnums::DecalFilter p_filter) {}

void RendererSceneRender::light_projectors_set_filter(
	RenderingServerEnums::LightProjectorFilter p_filter)
{
}

void RendererSceneRender::lightmaps_set_bicubic_filter(bool p_enable) {}

void RendererSceneRender::material_set_use_debanding(bool p_enable) {}

void RendererSceneRender::update() {}

// --- Merged from servers/rendering/renderer_compositor.cpp ---

Error RendererCompositor::create()
{
	String driver_name = "";
	if (DisplayServer::is_initialized()) {
		driver_name = OS::get_current_rendering_driver_name();
	}

	if (driver_name == "dummy" || driver_name.is_empty()) {
		RasterizerDummy::make_current();
		return OK;
	}

#if defined(GLES3_ENABLED)
	if (driver_name == "opengl3" || driver_name == "opengl3_es" || driver_name == "opengl3_angle") {
		RasterizerGLES3::make_current(driver_name == "opengl3");
		return OK;
	}
#endif

#if defined(RD_ENABLED)
	if (!low_end) {
		low_end = false;
		bind_compositor<RendererCompositorRD>();
		if (DisplayServer::is_initialized()) {
			Error err = DisplayServer::init_rendering_device();
			ERR_FAIL_COND_V_MSG(
				err != OK, err, "Failed to initialize RenderingDevice via DisplayServer.");
		}
		return OK;
	}
#endif

	RasterizerDummy::make_current();
	return OK;
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

// --- Merged from servers/rendering/renderer_canvas_render.cpp ---

const Rect2& RCR::Item::get_rect() const
{
	if (custom_rect || (!rect_dirty && !update_when_visible && skeleton == RID())) {
		return rect;
	}

	// must update rect

	if (commands == nullptr) {
		rect = Rect2();
		rect_dirty = false;
		return rect;
	}

	Transform2D xf;
	bool found_xform = false;
	bool first = true;

	const Item::Command* c = commands;

	while (c) {
		Rect2 r;

		switch (c->type) {
		case Item::Command::TYPE_RECT: {
			const Item::CommandRect* crect = reinterpret_cast<const Item::CommandRect*>(c);
			r = crect->rect;

		} break;
		case Item::Command::TYPE_NINEPATCH: {
			const Item::CommandNinePatch* style = reinterpret_cast<const Item::CommandNinePatch*>(c);
			r = style->rect;
		} break;

		case Item::Command::TYPE_POLYGON: {
			const Item::CommandPolygon* polygon = reinterpret_cast<const Item::CommandPolygon*>(c);
			r = polygon->polygon.rect_cache;
		} break;
		case Item::Command::TYPE_PRIMITIVE: {
			const Item::CommandPrimitive* primitive = reinterpret_cast<const Item::CommandPrimitive*>(c);
			for (uint32_t j = 0; j < primitive->point_count; j++) {
				if (j == 0) {
					r.position = primitive->points[0];
				}
				else {
					r.expand_to(primitive->points[j]);
				}
			}
		} break;
		case Item::Command::TYPE_MESH: {
			const Item::CommandMesh* mesh = reinterpret_cast<const Item::CommandMesh*>(c);
			AABB aabb = RendererMeshStorage::mesh_get_aabb(mesh->mesh, skeleton);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break;
		case Item::Command::TYPE_MULTIMESH: {
			const Item::CommandMultiMesh* multimesh = reinterpret_cast<const Item::CommandMultiMesh*>(c);
			AABB aabb = RendererMeshStorage::multimesh_get_aabb(multimesh->multimesh);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break;
		case Item::Command::TYPE_PARTICLES: {
			const Item::CommandParticles* particles_cmd =
				reinterpret_cast<const Item::CommandParticles*>(c);
			if (particles_cmd->particles.is_valid()) {
				AABB aabb = Renderer::particles_storage->particles_get_aabb(particles_cmd->particles);
				r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
			}

		} break;
		case Item::Command::TYPE_TRANSFORM: {
			const Item::CommandTransform* transform = reinterpret_cast<const Item::CommandTransform*>(c);
			xf = transform->xform;
			found_xform = true;
			[[fallthrough]];
		}
		default: {
			c = c->next;
			continue;
		}
		}

		if (found_xform) {
			r = xf.xform(r);
		}

		if (first) {
			rect = r;
			first = false;
		}
		else {
			rect = rect.merge(r);
		}
		c = c->next;
	}

	rect_dirty = false;
	return rect;
}

RCR::Item::CommandMesh::~CommandMesh()
{
	if (mesh_instance.is_valid()) {
		RendererMeshStorage::mesh_instance_free(mesh_instance);
	}
}


void Polygon::create(const Vector<int>& p_indices, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones,
	const Vector<float>& p_weights, int p_count)
{
	ERR_FAIL_COND(polygon_id != 0);
	int count = p_count < 0 ? p_indices.size() : p_count * 3;
	ERR_FAIL_COND(count > p_indices.size());
	{
		uint32_t pc = p_points.size();
		const Vector2* v2 = p_points.ptr();
		rect_cache.position = *v2;
		for (uint32_t i = 1; i < pc; i++) {
			rect_cache.expand_to(v2[i]);
		}
	}
	polygon_id = RendererCanvasRender::request_polygon(
		p_indices, p_points, p_colors, p_uvs, p_bones, p_weights, count);
}

Polygon::~Polygon()
{
	if (polygon_id && RendererCanvasRender::free_polygon) {
		RendererCanvasRender::free_polygon(polygon_id);
	}
}

// --- Merged from servers/rendering/renderer_geometry_instance.cpp ---

void RenderGeometryInstance::set_skeleton(RID p_skeleton) {
	if (!data) {
		return;
	}
	data->skeleton = p_skeleton;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_override(RID p_override) {
	if (!data) {
		return;
	}
	data->material_override = p_override;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_overlay(RID p_overlay) {
	if (!data) {
		return;
	}
	data->material_overlay = p_overlay;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_surface_materials(const Vector<RID> &p_materials) {
	if (!data) {
		return;
	}
	data->surface_materials = p_materials;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_mesh_instance(RID p_mesh_instance) {
	mesh_instance = p_mesh_instance;

	_mark_dirty();
}

void RenderGeometryInstance::set_transform(const Transform3D &p_transform, const AABB &p_aabb, const AABB &p_transformed_aabb) {
	transform = p_transform;
	mirror = p_transform.basis.determinant() < 0;
	if (data) {
		data->aabb = p_aabb;
	}
	transformed_aabb = p_transformed_aabb;

	Vector3 model_scale_vec = p_transform.basis.get_scale_abs();
	// handle non uniform scale here

	float max_scale = MAX(model_scale_vec.x, MAX(model_scale_vec.y, model_scale_vec.z));
	float min_scale = MIN(model_scale_vec.x, MIN(model_scale_vec.y, model_scale_vec.z));
	non_uniform_scale = max_scale >= 0.0 && (min_scale / max_scale) < 0.999;

	lod_model_scale = max_scale;
}

void RenderGeometryInstance::set_pivot_data(float p_sorting_offset, bool p_use_aabb_center) {
	sorting_offset = p_sorting_offset;
	use_aabb_center = p_use_aabb_center;
}

void RenderGeometryInstance::set_lod_bias(float p_lod_bias) {
	lod_bias = p_lod_bias;
}

void RenderGeometryInstance::set_layer_mask(uint32_t p_layer_mask) {
	layer_mask = p_layer_mask;
}

void RenderGeometryInstance::set_fade_range(bool p_enable_near, float p_near_begin, float p_near_end, bool p_enable_far, float p_far_begin, float p_far_end) {
	fade_near = p_enable_near;
	fade_near_begin = p_near_begin;
	fade_near_end = p_near_end;
	fade_far = p_enable_far;
	fade_far_begin = p_far_begin;
	fade_far_end = p_far_end;
}

void RenderGeometryInstance::set_parent_fade_alpha(float p_alpha) {
	parent_fade_alpha = p_alpha;
}

void RenderGeometryInstance::set_transparency(float p_transparency) {
	force_alpha = CLAMP(1.0 - p_transparency, 0, 1);
}

void RenderGeometryInstance::set_use_baked_light(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_baked_light = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_use_dynamic_gi(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_dynamic_gi = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_instance_shader_uniforms_offset(int32_t p_offset) {
	shader_uniforms_offset = p_offset;

	_mark_dirty();
}

void RenderGeometryInstance::set_cast_double_sided_shadows(bool p_enable) {
	if (!data) {
		return;
	}
	data->cast_double_sided_shadows = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::reset_motion_vectors() {
}

Transform3D RenderGeometryInstance::get_transform() {
	return transform;
}

AABB RenderGeometryInstance::get_aabb() {
	return data ? data->aabb : AABB();
}

// ============================================================================
// Merged from servers/rendering/storage/renderer_storage.cpp
// ============================================================================

#include "servers/rendering/storage/camera_attributes_storage.h"

#ifdef DEBUG_ENABLED
#include "core/os/os.h"
#endif
#include "servers/rendering/renderer_scene_render.h"

#ifdef DEBUG_ENABLED
#include "core/os/os.h"
#endif
#include "core/config/engine.h"
#include "core/math/transform_interpolator.h"
#include "servers/rendering/storage/mesh_storage.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep // Needed to bind RSE enums.
#include "servers/rendering/storage/utilities.h"

// --- Merged from servers/rendering/storage/camera_attributes_storage.cpp ---

void RendererCameraAttributes::initialize()
{
	if (_initialized) {
		return;
	}
	dof_blur_quality = RSE::DOF_BLUR_QUALITY_MEDIUM;
	dof_blur_bokeh_shape = RSE::DOF_BOKEH_HEXAGON;
	dof_blur_use_jitter = false;
	auto_exposure_counter = 2;
	_initialized = true;
}

void RendererCameraAttributes::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

RID RendererCameraAttributes::camera_attributes_allocate()
{
	return camera_attributes_owner.allocate_rid();
}

void RendererCameraAttributes::camera_attributes_initialize(RID p_rid)
{
	camera_attributes_owner.initialize_rid(p_rid, CameraAttributes());
}

void RendererCameraAttributes::camera_attributes_free(RID p_rid)
{
	camera_attributes_owner.free(p_rid);
}

void RendererCameraAttributes::camera_attributes_set_dof_blur_quality(
	RSE::DOFBlurQuality p_quality, bool p_use_jitter)
{
	dof_blur_quality = p_quality;
	dof_blur_use_jitter = p_use_jitter;
}

void RendererCameraAttributes::camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape)
{
	dof_blur_bokeh_shape = p_shape;
}

void RendererCameraAttributes::camera_attributes_set_dof_blur(RID p_camera_attributes,
	bool p_far_enable, float p_far_distance, float p_far_transition, bool p_near_enable,
	float p_near_distance, float p_near_transition, float p_amount)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "gl_compatibility" &&
		(p_far_enable || p_near_enable)) {
		WARN_PRINT_ONCE_ED(
			"Depth of field blur is only available when using the Forward+ or Mobile renderer.");
	}
#endif
	cam_attributes->dof_blur_far_enabled = p_far_enable;
	cam_attributes->dof_blur_far_distance = p_far_distance;
	cam_attributes->dof_blur_far_transition = p_far_transition;

	cam_attributes->dof_blur_near_enabled = p_near_enable;
	cam_attributes->dof_blur_near_distance = p_near_distance;
	cam_attributes->dof_blur_near_transition = p_near_transition;

	cam_attributes->dof_blur_amount = p_amount;
}

bool RendererCameraAttributes::camera_attributes_get_dof_far_enabled(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, false);
	return cam_attributes->dof_blur_far_enabled;
}

float RendererCameraAttributes::camera_attributes_get_dof_far_distance(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_far_distance;
}

float RendererCameraAttributes::camera_attributes_get_dof_far_transition(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_far_transition;
}

bool RendererCameraAttributes::camera_attributes_get_dof_near_enabled(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, false);
	return cam_attributes->dof_blur_near_enabled;
}

float RendererCameraAttributes::camera_attributes_get_dof_near_distance(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_near_distance;
}

float RendererCameraAttributes::camera_attributes_get_dof_near_transition(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_near_transition;
}

float RendererCameraAttributes::camera_attributes_get_dof_blur_amount(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_amount;
}

void RendererCameraAttributes::camera_attributes_set_exposure(
	RID p_camera_attributes, float p_multiplier, float p_exposure_normalization)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
	cam_attributes->exposure_multiplier = p_multiplier;
	cam_attributes->exposure_normalization = p_exposure_normalization;
}

float RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 1.0);

	return cam_attributes->exposure_multiplier * cam_attributes->exposure_normalization;
}

void RendererCameraAttributes::camera_attributes_set_auto_exposure(RID p_camera_attributes,
	bool p_enable, float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
	if (!cam_attributes->use_auto_exposure && p_enable) {
		cam_attributes->auto_exposure_version = ++auto_exposure_counter;
	}
#ifdef DEBUG_ENABLED
	if ((OS::get_current_rendering_method() == "gl_compatibility" ||
			OS::get_current_rendering_method() == "mobile") &&
		p_enable) {
		WARN_PRINT_ONCE_ED("Auto-exposure is only available when using the Forward+ renderer.");
	}
#endif
	cam_attributes->use_auto_exposure = p_enable;
	cam_attributes->auto_exposure_min_sensitivity = p_min_sensitivity;
	cam_attributes->auto_exposure_max_sensitivity = p_max_sensitivity;
	cam_attributes->auto_exposure_adjust_speed = p_speed;
	cam_attributes->auto_exposure_scale = p_scale;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_min_sensitivity(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_min_sensitivity;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_max_sensitivity(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_max_sensitivity;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_adjust_speed(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_adjust_speed;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_scale(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_scale;
}

uint64_t RendererCameraAttributes::camera_attributes_get_auto_exposure_version(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0);
	return cam_attributes->auto_exposure_version;
}

// --- Merged from servers/rendering/storage/compositor_storage.cpp ---

void RendererCompositorStorage::initialize()
{
	if (_initialized) {
		return;
	}
	num_compositor_effects_with_motion_vectors = 0;
	_initialized = true;
}

void RendererCompositorStorage::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

RID RendererCompositorStorage::compositor_effect_allocate()
{
	return compositor_effects_owner.allocate_rid();
}

void RendererCompositorStorage::compositor_effect_initialize(RID p_rid)
{
	compositor_effects_owner.initialize_rid(p_rid, CompositorEffect());
}

bool RendererCompositorStorage::compositor_effect_get_enabled(RID p_effect)
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, false);

	return effect->is_enabled;
}

RSE::CompositorEffectCallbackType RendererCompositorStorage::compositor_effect_get_callback_type(
	RID p_effect)
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_MAX);

	return effect->callback_type;
}

RID RendererCompositorStorage::compositor_allocate() { return compositor_owner.allocate_rid(); }

void RendererCompositorStorage::compositor_initialize(RID p_rid)
{
	compositor_owner.initialize_rid(p_rid, Compositor());
}

void RendererCompositorStorage::compositor_free(RID p_rid) { compositor_owner.free(p_rid); }

void RendererCompositorStorage::compositor_set_compositor_effects(
	RID p_compositor, const Vector<RID>& p_effects)
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL(compositor);

	compositor->compositor_effects.clear();
	for (const RID& effect : p_effects) {
		if (is_compositor_effect(effect)) {
			compositor->compositor_effects.push_back(effect);
		}
	}
}

Vector<RID> RendererCompositorStorage::compositor_get_compositor_effects(
	RID p_compositor, RSE::CompositorEffectCallbackType p_callback_type, bool p_enabled_only)
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL_V(compositor, Vector<RID>());

	if (p_enabled_only || p_callback_type != RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY) {
		Vector<RID> effects;

		for (RID rid : compositor->compositor_effects) {
			if ((!p_enabled_only || compositor_effect_get_enabled(rid)) &&
				(p_callback_type == RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY ||
					compositor_effect_get_callback_type(rid) == p_callback_type)) {
				effects.push_back(rid);
			}
		}

		return effects;
	}
	else {
		return compositor->compositor_effects;
	}
}

void RendererCompositorStorage::compositor_effect_set_flag(
	RID p_effect, RenderingServerEnums::CompositorEffectFlags p_flag, bool p_set)
{
}

void RendererCompositorStorage::compositor_effect_set_enabled(RID p_effect, bool p_enabled) {}

bool RendererCompositorStorage::compositor_effect_get_flag(
	RID p_effect, RenderingServerEnums::CompositorEffectFlags p_flag)
{
	return false;
}

void RendererCompositorStorage::compositor_effect_free(RID p_effect) {}

// --- Merged from servers/rendering/storage/environment_storage.cpp ---

// Storage

void RendererEnvironmentStorage::initialize()
{
	if (_initialized) {
		return;
	}
	_initialized = true;
}

void RendererEnvironmentStorage::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

// Environment

RID RendererEnvironmentStorage::environment_allocate() {
	return environment_owner.allocate_rid();
}

void RendererEnvironmentStorage::environment_initialize(RID p_rid) {
	environment_owner.initialize_rid(p_rid, Environment());
}

void RendererEnvironmentStorage::environment_free(RID p_rid) {
	environment_owner.free(p_rid);
}

// Background

void RendererEnvironmentStorage::environment_set_background(RID p_env, RSE::EnvironmentBG p_bg) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->background = p_bg;
}

void RendererEnvironmentStorage::environment_set_sky(RID p_env, RID p_sky) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky = p_sky;
}

void RendererEnvironmentStorage::environment_set_sky_custom_fov(RID p_env, float p_scale) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky_custom_fov = p_scale;
}

void RendererEnvironmentStorage::environment_set_sky_orientation(RID p_env, const Basis &p_orientation) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky_orientation = p_orientation;
}

void RendererEnvironmentStorage::environment_set_bg_color(RID p_env, const Color &p_color) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->bg_color = p_color;
}

void RendererEnvironmentStorage::environment_set_bg_energy(RID p_env, float p_multiplier, float p_intensity) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->bg_energy_multiplier = p_multiplier;
	env->bg_intensity = p_intensity;
}

void RendererEnvironmentStorage::environment_set_canvas_max_layer(RID p_env, int p_max_layer) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->canvas_max_layer = p_max_layer;
}

void RendererEnvironmentStorage::environment_set_ambient_light(RID p_env, const Color &p_color, RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution, RSE::EnvironmentReflectionSource p_reflection_source) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->ambient_light = p_color;
	env->ambient_source = p_ambient;
	env->ambient_light_energy = p_energy;
	env->ambient_sky_contribution = p_sky_contribution;
	env->reflection_source = p_reflection_source;
}

RSE::EnvironmentBG RendererEnvironmentStorage::environment_get_background(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_BG_CLEAR_COLOR);
	return env->background;
}

RID RendererEnvironmentStorage::environment_get_sky(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->sky;
}

float RendererEnvironmentStorage::environment_get_sky_custom_fov(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->sky_custom_fov;
}

Basis RendererEnvironmentStorage::environment_get_sky_orientation(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Basis());
	return env->sky_orientation;
}

Color RendererEnvironmentStorage::environment_get_bg_color(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color());
	return env->bg_color;
}

float RendererEnvironmentStorage::environment_get_bg_energy_multiplier(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->bg_energy_multiplier;
}

float RendererEnvironmentStorage::environment_get_bg_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->bg_intensity;
}

int RendererEnvironmentStorage::environment_get_canvas_max_layer(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0);
	return env->canvas_max_layer;
}

RSE::EnvironmentAmbientSource RendererEnvironmentStorage::environment_get_ambient_source(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_AMBIENT_SOURCE_BG);
	return env->ambient_source;
}

Color RendererEnvironmentStorage::environment_get_ambient_light(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color());
	return env->ambient_light;
}

float RendererEnvironmentStorage::environment_get_ambient_light_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ambient_light_energy;
}

float RendererEnvironmentStorage::environment_get_ambient_sky_contribution(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ambient_sky_contribution;
}

RSE::EnvironmentReflectionSource RendererEnvironmentStorage::environment_get_reflection_source(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_REFLECTION_SOURCE_BG);
	return env->reflection_source;
}

void RendererEnvironmentStorage::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->camera_feed_id = p_camera_feed_id;
}

int RendererEnvironmentStorage::environment_get_camera_feed_id(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, -1);
	return env->camera_feed_id;
}

// Tonemap

void RendererEnvironmentStorage::environment_set_tonemap(RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->exposure = p_exposure;
	env->tone_mapper = p_tone_mapper;
	env->white = p_white;
}

RSE::EnvironmentToneMapper RendererEnvironmentStorage::environment_get_tone_mapper(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_TONE_MAPPER_LINEAR);
	return env->tone_mapper;
}

float RendererEnvironmentStorage::environment_get_exposure(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->exposure;
}

float RendererEnvironmentStorage::environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);

	// Glow with screen blend mode does not work when white < 1.0, so make sure
	// it is at least 1.0 for all tonemappers:
	if (env->tone_mapper == RSE::ENV_TONE_MAPPER_LINEAR) {
		return p_output_max_value;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_FILMIC || env->tone_mapper == RSE::ENV_TONE_MAPPER_ACES) {
		// Filmic and ACES only support SDR; their white is stable regardless
		// of output_max_value.
		return MAX(1.0, env->white);
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_AGX) {
		// AgX works best with a high white. 2.0 is the minimum required for
		// good behavior with Mobile rendering method.
		if (p_limit_agx_white) {
			return 2.0;
		} else {
			float agx_white = MAX(2.0, env->white);
			// Instead of constraining by matching the output_max_value, constrain
			// by multiplying to ensure the desired non-uniform scaling behavior
			// is maintained in the shoulder.
			return agx_white * p_output_max_value;
		}
	} else { // Reinhard
		// The Reinhard tonemapper is not designed to have a white parameter
		// that is less than the output max value. This is especially important
		// in the variable Extended Dynamic Range (EDR) paradigm where the
		// output max value may change to be greater or less than the white
		// parameter, depending on the available dynamic range.
		return MAX(p_output_max_value, env->white);
	}
}

void RendererEnvironmentStorage::environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->tonemap_agx_contrast = p_agx_contrast;
}

float RendererEnvironmentStorage::environment_get_tonemap_agx_contrast(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->tonemap_agx_contrast;
}

RendererEnvironmentStorage::TonemapParameters RendererEnvironmentStorage::environment_get_tonemap_parameters(RID p_env, bool p_limit_agx_white, float p_output_max_value) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, TonemapParameters());

	float white = environment_get_white(p_env, p_limit_agx_white, p_output_max_value);
	TonemapParameters tonemap_parameters = TonemapParameters();

	if (env->tone_mapper == RSE::ENV_TONE_MAPPER_LINEAR) {
		// Linear has no tonemapping parameters
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_REINHARD) {
		tonemap_parameters.white_squared = (white * white) / p_output_max_value;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_FILMIC) {
		// These constants must match those in the shader code.
		// exposure_bias: Input scale (color *= bias, white *= bias) to make the brightness consistent with other tonemappers
		// also useful to scale the input to the range that the tonemapper is designed for (some require very high input values).
		// Has no effect on the curve's general shape or visual properties.
		const float exposure_bias = 2.0f;
		const float A = 0.22f * exposure_bias * exposure_bias; // bias baked into constants for performance
		const float B = 0.30f * exposure_bias;
		const float C = 0.10f;
		const float D = 0.20f;
		const float E = 0.01f;
		const float F = 0.30f;

		tonemap_parameters.white_tonemapped = ((white * (A * white + C * B) + D * E) / (white * (A * white + B) + D * F)) - E / F;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_ACES) {
		// These constants must match those in the shader code.
		const float exposure_bias = 1.8f;
		const float A = 0.0245786f;
		const float B = 0.000090537f;
		const float C = 0.983729f;
		const float D = 0.432951f;
		const float E = 0.238081f;

		white *= exposure_bias;
		float white_tonemapped = (white * (white + A) - B) / (white * (C * white + D) + E);
		tonemap_parameters.white_tonemapped = white_tonemapped;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_AGX) {
		// Calculate allenwp tonemapping curve parameters on the CPU to improve shader performance.
		// Source and details: https://allenwp.com/blog/2025/05/29/allenwp-tonemapping-curve/

		// These constants must match the those in the shader code.
		// 18% "middle gray" is perceptually 50% of the brightness of reference white.
		const float awp_crossover_point = 0.18;
		// When output_max_value and/or awp_crossover_point are no longer constant, awp_shoulder_max can
		// be calculated on the CPU and passed in as tonemap_parameters.tonemap_e.
		const float awp_shoulder_max = p_output_max_value - awp_crossover_point;

		float awp_high_clip = white;

		// awp_toe_a is a solution generated by Mathematica that ensures intersection at awp_crossover_point.
		float awp_toe_a = ((1.0 / awp_crossover_point) - 1.0) * pow(awp_crossover_point, env->tonemap_agx_contrast);
		// Slope formula is simply the derivative of the toe function with an input of awp_crossover_point.
		float awp_slope_denom = pow(awp_crossover_point, env->tonemap_agx_contrast) + awp_toe_a;
		float awp_slope = (env->tonemap_agx_contrast * pow(awp_crossover_point, env->tonemap_agx_contrast - 1.0) * awp_toe_a) / (awp_slope_denom * awp_slope_denom);

		float awp_w = awp_high_clip - awp_crossover_point;
		awp_w = awp_w * awp_w;
		awp_w = awp_w / awp_shoulder_max;
		awp_w = awp_w * awp_slope;

		tonemap_parameters.awp_contrast = env->tonemap_agx_contrast;
		tonemap_parameters.awp_toe_a = awp_toe_a;
		tonemap_parameters.awp_slope = awp_slope;
		tonemap_parameters.awp_w = awp_w;
	}

	return tonemap_parameters;
}

// Fog

void RendererEnvironmentStorage::environment_set_fog(RID p_env, bool p_enable, const Color &p_light_color, float p_light_energy, float p_sun_scatter, float p_density, float p_height, float p_height_density, float p_fog_aerial_perspective, float p_sky_affect, RSE::EnvironmentFogMode p_mode) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->fog_enabled = p_enable;
	env->fog_mode = p_mode;
	env->fog_light_color = p_light_color;
	env->fog_light_energy = p_light_energy;
	env->fog_sun_scatter = p_sun_scatter;
	env->fog_density = p_density;
	env->fog_height = p_height;
	env->fog_height_density = p_height_density;
	env->fog_aerial_perspective = p_fog_aerial_perspective;
	env->fog_sky_affect = p_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_fog_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->fog_enabled;
}

RSE::EnvironmentFogMode RendererEnvironmentStorage::environment_get_fog_mode(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_FOG_MODE_EXPONENTIAL);
	return env->fog_mode;
}

Color RendererEnvironmentStorage::environment_get_fog_light_color(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(0.5, 0.6, 0.7));
	return env->fog_light_color;
}

float RendererEnvironmentStorage::environment_get_fog_light_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->fog_light_energy;
}

float RendererEnvironmentStorage::environment_get_fog_sun_scatter(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_sun_scatter;
}

float RendererEnvironmentStorage::environment_get_fog_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.001);
	return env->fog_density;
}

float RendererEnvironmentStorage::environment_get_fog_height(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_height;
}

float RendererEnvironmentStorage::environment_get_fog_height_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_height_density;
}

float RendererEnvironmentStorage::environment_get_fog_aerial_perspective(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_aerial_perspective;
}

float RendererEnvironmentStorage::environment_get_fog_sky_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_sky_affect;
}

// Depth Fog

void RendererEnvironmentStorage::environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->fog_depth_curve = p_curve;
	env->fog_depth_begin = p_begin;
	env->fog_depth_end = p_end;
}

float RendererEnvironmentStorage::environment_get_fog_depth_curve(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_curve;
}

float RendererEnvironmentStorage::environment_get_fog_depth_begin(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_begin;
}

float RendererEnvironmentStorage::environment_get_fog_depth_end(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_end;
}

// Volumetric Fog

void RendererEnvironmentStorage::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density, const Color &p_albedo, const Color &p_emission, float p_emission_energy, float p_anisotropy, float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection, float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Volumetric fog is only available when using the Forward+ renderer.");
	}
#endif
	env->volumetric_fog_enabled = p_enable;
	env->volumetric_fog_density = p_density;
	env->volumetric_fog_scattering = p_albedo;
	env->volumetric_fog_emission = p_emission;
	env->volumetric_fog_emission_energy = p_emission_energy;
	env->volumetric_fog_anisotropy = p_anisotropy;
	env->volumetric_fog_length = p_length;
	env->volumetric_fog_detail_spread = p_detail_spread;
	env->volumetric_fog_gi_inject = p_gi_inject;
	env->volumetric_fog_temporal_reprojection = p_temporal_reprojection;
	env->volumetric_fog_temporal_reprojection_amount = p_temporal_reprojection_amount;
	env->volumetric_fog_ambient_inject = p_ambient_inject;
	env->volumetric_fog_sky_affect = p_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_volumetric_fog_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->volumetric_fog_enabled;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.01);
	return env->volumetric_fog_density;
}

Color RendererEnvironmentStorage::environment_get_volumetric_fog_scattering(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(1, 1, 1));
	return env->volumetric_fog_scattering;
}

Color RendererEnvironmentStorage::environment_get_volumetric_fog_emission(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(0, 0, 0));
	return env->volumetric_fog_emission;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_emission_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_emission_energy;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_anisotropy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.2);
	return env->volumetric_fog_anisotropy;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_length(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 64.0);
	return env->volumetric_fog_length;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_detail_spread(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->volumetric_fog_detail_spread;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_gi_inject(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_gi_inject;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_sky_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, true);
	return env->volumetric_fog_temporal_reprojection;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.9);
	return env->volumetric_fog_temporal_reprojection_amount;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_ambient_inject(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_ambient_inject;
}

// GLOW

void RendererEnvironmentStorage::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels, float p_intensity, float p_strength, float p_mix, float p_bloom_threshold, RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold, float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	ERR_FAIL_COND_MSG(p_levels.size() != 7, "Size of array of glow levels must be 7");
	env->glow_enabled = p_enable;
	env->glow_levels = p_levels;
	env->glow_intensity = p_intensity;
	env->glow_strength = p_strength;
	env->glow_mix = p_mix;
	env->glow_bloom = p_bloom_threshold;
	env->glow_blend_mode = p_blend_mode;
	env->glow_hdr_bleed_threshold = p_hdr_bleed_threshold;
	env->glow_hdr_bleed_scale = p_hdr_bleed_scale;
	env->glow_hdr_luminance_cap = p_hdr_luminance_cap;
	env->glow_map_strength = p_glow_map_strength;
	env->glow_map = p_glow_map;
}

bool RendererEnvironmentStorage::environment_get_glow_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->glow_enabled;
}

Vector<float> RendererEnvironmentStorage::environment_get_glow_levels(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Vector<float>());
	return env->glow_levels;
}

float RendererEnvironmentStorage::environment_get_glow_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.3);
	return env->glow_intensity;
}

float RendererEnvironmentStorage::environment_get_glow_strength(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->glow_strength;
}

float RendererEnvironmentStorage::environment_get_glow_bloom(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->glow_bloom;
}

float RendererEnvironmentStorage::environment_get_glow_mix(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.01);
	return env->glow_mix;
}

RSE::EnvironmentGlowBlendMode RendererEnvironmentStorage::environment_get_glow_blend_mode(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_GLOW_BLEND_MODE_SCREEN);
	return env->glow_blend_mode;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_bleed_threshold(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->glow_hdr_bleed_threshold;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_luminance_cap(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 12.0);
	return env->glow_hdr_luminance_cap;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_bleed_scale(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->glow_hdr_bleed_scale;
}

float RendererEnvironmentStorage::environment_get_glow_map_strength(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->glow_map_strength;
}

RID RendererEnvironmentStorage::environment_get_glow_map(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->glow_map;
}

// SSR

void RendererEnvironmentStorage::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int, float p_fade_out, float p_depth_tolerance) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space reflections (SSR) are only available when using the Forward+ renderer.");
	}
#endif
	env->ssr_enabled = p_enable;
	env->ssr_max_steps = p_max_steps;
	env->ssr_fade_in = p_fade_int;
	env->ssr_fade_out = p_fade_out;
	env->ssr_depth_tolerance = p_depth_tolerance;
}

bool RendererEnvironmentStorage::environment_get_ssr_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssr_enabled;
}

int RendererEnvironmentStorage::environment_get_ssr_max_steps(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 64);
	return env->ssr_max_steps;
}

float RendererEnvironmentStorage::environment_get_ssr_fade_in(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.15);
	return env->ssr_fade_in;
}

float RendererEnvironmentStorage::environment_get_ssr_fade_out(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->ssr_fade_out;
}

float RendererEnvironmentStorage::environment_get_ssr_depth_tolerance(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->ssr_depth_tolerance;
}

// SSAO

void RendererEnvironmentStorage::environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect, float p_ao_channel_affect) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "mobile" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space ambient occlusion (SSAO) is only available when using the Forward+ or Compatibility renderers.");
	}
#endif
	env->ssao_enabled = p_enable;
	env->ssao_radius = p_radius;
	env->ssao_intensity = p_intensity;
	env->ssao_power = p_power;
	env->ssao_detail = p_detail;
	env->ssao_horizon = p_horizon;
	env->ssao_sharpness = p_sharpness;
	env->ssao_direct_light_affect = p_light_affect;
	env->ssao_ao_channel_affect = p_ao_channel_affect;
}

bool RendererEnvironmentStorage::environment_get_ssao_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssao_enabled;
}

float RendererEnvironmentStorage::environment_get_ssao_radius(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssao_radius;
}

float RendererEnvironmentStorage::environment_get_ssao_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->ssao_intensity;
}

float RendererEnvironmentStorage::environment_get_ssao_power(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.5);
	return env->ssao_power;
}

float RendererEnvironmentStorage::environment_get_ssao_detail(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->ssao_detail;
}

float RendererEnvironmentStorage::environment_get_ssao_horizon(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.06);
	return env->ssao_horizon;
}

float RendererEnvironmentStorage::environment_get_ssao_sharpness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.98);
	return env->ssao_sharpness;
}

float RendererEnvironmentStorage::environment_get_ssao_direct_light_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->ssao_direct_light_affect;
}

float RendererEnvironmentStorage::environment_get_ssao_ao_channel_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->ssao_ao_channel_affect;
}

// SSIL

void RendererEnvironmentStorage::environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_sharpness, float p_normal_rejection) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space indirect lighting (SSIL) is only available when using the Forward+ renderer.");
	}
#endif
	env->ssil_enabled = p_enable;
	env->ssil_radius = p_radius;
	env->ssil_intensity = p_intensity;
	env->ssil_sharpness = p_sharpness;
	env->ssil_normal_rejection = p_normal_rejection;
}

bool RendererEnvironmentStorage::environment_get_ssil_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssil_enabled;
}

float RendererEnvironmentStorage::environment_get_ssil_radius(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 5.0);
	return env->ssil_radius;
}

float RendererEnvironmentStorage::environment_get_ssil_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssil_intensity;
}

float RendererEnvironmentStorage::environment_get_ssil_sharpness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.98);
	return env->ssil_sharpness;
}

float RendererEnvironmentStorage::environment_get_ssil_normal_rejection(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssil_normal_rejection;
}

// SDFGI

void RendererEnvironmentStorage::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades, float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion, float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias, float p_probe_bias) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("SDFGI is only available when using the Forward+ renderer.");
	}
#endif
	env->sdfgi_enabled = p_enable;
	env->sdfgi_cascades = p_cascades;
	env->sdfgi_min_cell_size = p_min_cell_size;
	env->sdfgi_use_occlusion = p_use_occlusion;
	env->sdfgi_bounce_feedback = p_bounce_feedback;
	env->sdfgi_read_sky_light = p_read_sky;
	env->sdfgi_energy = p_energy;
	env->sdfgi_normal_bias = p_normal_bias;
	env->sdfgi_probe_bias = p_probe_bias;
	env->sdfgi_y_scale = p_y_scale;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->sdfgi_enabled;
}

int RendererEnvironmentStorage::environment_get_sdfgi_cascades(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 4);
	return env->sdfgi_cascades;
}

float RendererEnvironmentStorage::environment_get_sdfgi_min_cell_size(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.2);
	return env->sdfgi_min_cell_size;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_use_occlusion(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->sdfgi_use_occlusion;
}

float RendererEnvironmentStorage::environment_get_sdfgi_bounce_feedback(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->sdfgi_bounce_feedback;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_read_sky_light(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, true);
	return env->sdfgi_read_sky_light;
}

float RendererEnvironmentStorage::environment_get_sdfgi_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->sdfgi_energy;
}

float RendererEnvironmentStorage::environment_get_sdfgi_normal_bias(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.1);
	return env->sdfgi_normal_bias;
}

float RendererEnvironmentStorage::environment_get_sdfgi_probe_bias(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.1);
	return env->sdfgi_probe_bias;
}

RSE::EnvironmentSDFGIYScale RendererEnvironmentStorage::environment_get_sdfgi_y_scale(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_SDFGI_Y_SCALE_75_PERCENT);
	return env->sdfgi_y_scale;
}

// Adjustments

void RendererEnvironmentStorage::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);

	env->adjustments_enabled = p_enable;
	// Scale brightness via the nonlinear sRGB transfer function to provide a
	// somewhat perceptually uniform brightness adjustment.
	env->adjustments_brightness = p_brightness < 0.04045f ? p_brightness * (1.0f / 12.92f) : Math::pow(float((p_brightness + 0.055f) * (1.0f / (1.055f))), 2.4f);
	env->adjustments_contrast = p_contrast;
	env->adjustments_saturation = p_saturation;
	env->use_1d_color_correction = p_use_1d_color_correction;
	env->color_correction = p_color_correction;
}

bool RendererEnvironmentStorage::environment_get_adjustments_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->adjustments_enabled;
}

float RendererEnvironmentStorage::environment_get_adjustments_brightness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_brightness;
}

float RendererEnvironmentStorage::environment_get_adjustments_contrast(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_contrast;
}

float RendererEnvironmentStorage::environment_get_adjustments_saturation(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_saturation;
}

bool RendererEnvironmentStorage::environment_get_use_1d_color_correction(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->use_1d_color_correction;
}

RID RendererEnvironmentStorage::environment_get_color_correction(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->color_correction;
}

// --- Merged from servers/rendering/storage/mesh_storage.cpp ---

void RendererMeshStorage::multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		mmi->_transform_format = p_transform_format;
		mmi->_use_colors = p_use_colors;
		mmi->_use_custom_data = p_use_custom_data;
		mmi->_num_instances = p_instances;

		mmi->_vf_size_xform = p_transform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
		mmi->_vf_size_color = p_use_colors ? 4 : 0;
		mmi->_vf_size_data = p_use_custom_data ? 4 : 0;

		mmi->_stride = mmi->_vf_size_xform + mmi->_vf_size_color + mmi->_vf_size_data;

		int size_in_floats = p_instances * mmi->_stride;
		mmi->_data_curr.resize_initialized(size_in_floats);
		mmi->_data_prev.resize_initialized(size_in_floats);
		mmi->_data_interpolated.resize_initialized(size_in_floats);
	}

	_multimesh_allocate_data(p_multimesh, p_instances, p_transform_format, p_use_colors,
		p_use_custom_data, p_use_indirect);
}

int RendererMeshStorage::multimesh_get_instance_count(RID p_multimesh)
{
	return _multimesh_get_instance_count(p_multimesh);
}

void RendererMeshStorage::multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	_multimesh_set_mesh(p_multimesh, p_mesh);
}

void RendererMeshStorage::multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_xform != 12);

		int start = p_index * mmi->_stride;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		const Transform3D& t = p_transform;
		ptr[0] = t.basis.rows[0][0];
		ptr[1] = t.basis.rows[0][1];
		ptr[2] = t.basis.rows[0][2];
		ptr[3] = t.origin.x;
		ptr[4] = t.basis.rows[1][0];
		ptr[5] = t.basis.rows[1][1];
		ptr[6] = t.basis.rows[1][2];
		ptr[7] = t.origin.y;
		ptr[8] = t.basis.rows[2][0];
		ptr[9] = t.basis.rows[2][1];
		ptr[10] = t.basis.rows[2][2];
		ptr[11] = t.origin.z;

		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_instance_set_transform(p_multimesh, p_index, p_transform);
}

void RendererMeshStorage::multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_xform != 8);

		int start = p_index * mmi->_stride;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		const Transform2D& t = p_transform;

		ptr[0] = t.columns[0][0];
		ptr[1] = t.columns[1][0];
		ptr[2] = 0;
		ptr[3] = t.columns[2][0];
		ptr[4] = t.columns[0][1];
		ptr[5] = t.columns[1][1];
		ptr[6] = 0;
		ptr[7] = t.columns[2][1];

		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_instance_set_transform_2d(p_multimesh, p_index, p_transform);
}

void RendererMeshStorage::multimesh_instance_set_color(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_color == 0);

		int start = (p_index * mmi->_stride) + mmi->_vf_size_xform;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		if (mmi->_vf_size_color == 4) {
			for (int n = 0; n < 4; n++) {
				ptr[n] = p_color[n];
			}
		}
		else {
#ifdef DEV_ENABLED
			// The options are currently 4 or zero, but just in case this changes in future...
			ERR_FAIL_COND(mmi->_vf_size_color != 0);
#endif
		}
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
		return;
	}

	_multimesh_instance_set_color(p_multimesh, p_index, p_color);
}

void RendererMeshStorage::multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_data == 0);

		int start = (p_index * mmi->_stride) + mmi->_vf_size_xform + mmi->_vf_size_color;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		if (mmi->_vf_size_data == 4) {
			for (int n = 0; n < 4; n++) {
				ptr[n] = p_color[n];
			}
		}
		else {
#ifdef DEV_ENABLED
			// The options are currently 4 or zero, but just in case this changes in future...
			ERR_FAIL_COND(mmi->_vf_size_data != 0);
#endif
		}
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
		return;
	}

	_multimesh_instance_set_custom_data(p_multimesh, p_index, p_color);
}

void RendererMeshStorage::multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	_multimesh_set_custom_aabb(p_multimesh, p_aabb);
}

AABB RendererMeshStorage::multimesh_get_custom_aabb(RID p_multimesh)
{
	return _multimesh_get_custom_aabb(p_multimesh);
}

RID RendererMeshStorage::multimesh_get_mesh(RID p_multimesh)
{
	return _multimesh_get_mesh(p_multimesh);
}

Transform3D RendererMeshStorage::multimesh_instance_get_transform(
	RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_transform(p_multimesh, p_index);
}

Transform2D RendererMeshStorage::multimesh_instance_get_transform_2d(
	RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_transform_2d(p_multimesh, p_index);
}

Color RendererMeshStorage::multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_color(p_multimesh, p_index);
}

Color RendererMeshStorage::multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_custom_data(p_multimesh, p_index);
}

void RendererMeshStorage::multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND_MSG(p_buffer.size() != mmi->_data_curr.size(),
			"Buffer should have " + itos(mmi->_data_curr.size()) + " elements, got " +
				itos(p_buffer.size()) + " instead.");

		mmi->_data_curr = p_buffer;
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_set_buffer(p_multimesh, p_buffer);
}

RID RendererMeshStorage::multimesh_get_command_buffer_rd_rid(RID p_multimesh)
{
	return _multimesh_get_command_buffer_rd_rid(p_multimesh);
}

RID RendererMeshStorage::multimesh_get_buffer_rd_rid(RID p_multimesh)
{
	return _multimesh_get_buffer_rd_rid(p_multimesh);
}

Vector<float> RendererMeshStorage::multimesh_get_buffer(RID p_multimesh)
{
	return _multimesh_get_buffer(p_multimesh);
}

void RendererMeshStorage::multimesh_set_buffer_interpolated(
	RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		ERR_FAIL_COND_MSG(p_buffer.size() != mmi->_data_curr.size(),
			"Buffer for current frame should have " + itos(mmi->_data_curr.size()) +
				" elements, got " + itos(p_buffer.size()) + " instead.");
		ERR_FAIL_COND_MSG(p_buffer_prev.size() != mmi->_data_prev.size(),
			"Buffer for previous frame should have " + itos(mmi->_data_prev.size()) +
				" elements, got " + itos(p_buffer_prev.size()) + " instead.");

		// We are assuming that mmi->interpolated is the case. (Can possibly assert this?)
		// Even if this flag hasn't been set - just calling this function suggests interpolation is
		// desired.
		mmi->_data_prev = p_buffer_prev;
		mmi->_data_curr = p_buffer;
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
	}
}

void RendererMeshStorage::multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		if (p_interpolated == mmi->interpolated) {
			return;
		}

		mmi->interpolated = p_interpolated;

		// If we are turning on physics interpolation, as a convenience,
		// we want to get the current buffer data from the backend,
		// and reset all the instances.
		if (p_interpolated) {
			mmi->_data_curr = _multimesh_get_buffer(p_multimesh);
			mmi->_data_prev = mmi->_data_curr;
			mmi->_data_interpolated = mmi->_data_curr;
		}
	}
}

void RendererMeshStorage::multimesh_set_physics_interpolation_quality(
	RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality)
{
	ERR_FAIL_COND((p_quality < 0) || (p_quality > 1));
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		mmi->quality = (int)p_quality;
	}
}

void RendererMeshStorage::multimesh_instance_reset_physics_interpolation(
	RID p_multimesh, int p_index)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		ERR_FAIL_INDEX(p_index, mmi->_num_instances);

		float* w = mmi->_data_prev.ptrw();
		const float* r = mmi->_data_curr.ptr();
		int start = p_index * mmi->_stride;

		for (int n = 0; n < mmi->_stride; n++) {
			w[start + n] = r[start + n];
		}
	}
}

void RendererMeshStorage::multimesh_instances_reset_physics_interpolation(RID p_multimesh)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->_data_curr.size()) {
		// We don't want to invoke COW here, so copy the data directly.
		ERR_FAIL_COND(mmi->_data_prev.size() != mmi->_data_curr.size());
		float* w = mmi->_data_prev.ptrw();
		const float* r = mmi->_data_curr.ptr();
		memcpy(w, r, sizeof(float) * mmi->_data_curr.size());
	}
}

void RendererMeshStorage::multimesh_set_visible_instances(RID p_multimesh, int p_visible)
{
	return _multimesh_set_visible_instances(p_multimesh, p_visible);
}

int RendererMeshStorage::multimesh_get_visible_instances(RID p_multimesh)
{
	return _multimesh_get_visible_instances(p_multimesh);
}

AABB RendererMeshStorage::multimesh_get_aabb(RID p_multimesh)
{
	return _multimesh_get_aabb(p_multimesh);
}

void RendererMeshStorage::_multimesh_add_to_interpolation_lists(
	RID p_multimesh, MultiMeshInterpolator& r_mmi)
{
	if (!r_mmi.on_interpolate_update_list) {
		r_mmi.on_interpolate_update_list = true;
		_interpolation_data.multimesh_interpolate_update_list.push_back(p_multimesh);
	}

	if (!r_mmi.on_transform_update_list) {
		r_mmi.on_transform_update_list = true;
		_interpolation_data.multimesh_transform_update_list_curr->push_back(p_multimesh);
	}
}

void InterpolationData::notify_free_multimesh(RID p_rid)
{
	// If the instance was on any of the lists, remove.
	multimesh_interpolate_update_list.erase_multiple_unordered(p_rid);
	multimesh_transform_update_lists[0].erase_multiple_unordered(p_rid);
	multimesh_transform_update_lists[1].erase_multiple_unordered(p_rid);
}

void RendererMeshStorage::update_interpolation_tick(bool p_process)
{
	// Detect any that were on the previous transform list that are no longer active,
	// we should remove them from the interpolate list.

	for (unsigned int n = 0; n < _interpolation_data.multimesh_transform_update_list_prev->size();
		 n++) {
		const RID& rid = (*_interpolation_data.multimesh_transform_update_list_prev)[n];

		bool active = true;

		// No longer active? (Either the instance deleted or no longer being transformed.)

		MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
		if (mmi && !mmi->on_transform_update_list) {
			active = false;
			mmi->on_interpolate_update_list = false;

			// Make sure the most recent transform is set...
			mmi->_data_interpolated =
				mmi->_data_curr; // TODO: Copy data rather than use Packed = function?

			// ... and that both prev and current are the same, just in case of any interpolations.
			mmi->_data_prev = mmi->_data_curr;

			// Update the actual stable buffer to the backend.
			_multimesh_set_buffer(rid, mmi->_data_interpolated);
		}

		if (!mmi) {
			active = false;
		}

		if (!active) {
			_interpolation_data.multimesh_interpolate_update_list.erase(rid);
		}
	}

	if (p_process) {
		for (unsigned int i = 0;
			 i < _interpolation_data.multimesh_transform_update_list_curr->size(); i++) {
			const RID& rid = (*_interpolation_data.multimesh_transform_update_list_curr)[i];

			MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
			if (mmi) {
				// Reset for next tick.
				mmi->on_transform_update_list = false;
				mmi->_data_prev = mmi->_data_curr;
			}
		}
	}

	// If any have left the transform list, remove from the interpolate list.

	// We maintain a mirror list for the transform updates, so we can detect when an instance
	// is no longer being transformed, and remove it from the interpolate list.
	SWAP(_interpolation_data.multimesh_transform_update_list_curr,
		_interpolation_data.multimesh_transform_update_list_prev);

	// Prepare for the next iteration.
	_interpolation_data.multimesh_transform_update_list_curr->clear();
}

void RendererMeshStorage::update_interpolation_frame(bool p_process)
{
	if (p_process) {
		// Only need 32 bits for interpolation, don't use real_t.
		float f = Engine::get_physics_interpolation_fraction();

		for (unsigned int c = 0; c < _interpolation_data.multimesh_interpolate_update_list.size();
			 c++) {
			const RID& rid = _interpolation_data.multimesh_interpolate_update_list[c];

			// We could use the TransformInterpolator here to slerp transforms, but that might be
			// too expensive, so just using a Basis lerp for now.
			MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
			if (mmi) {
				// Make sure arrays are the correct size.
				DEV_ASSERT(mmi->_data_prev.size() == mmi->_data_curr.size());

				if (mmi->_data_interpolated.size() < mmi->_data_curr.size()) {
					mmi->_data_interpolated.resize(mmi->_data_curr.size());
				}
				DEV_ASSERT(mmi->_data_interpolated.size() >= mmi->_data_curr.size());

				DEV_ASSERT((mmi->_data_curr.size() % mmi->_stride) == 0);
				int num = mmi->_data_curr.size() / mmi->_stride;

				const float* pf_prev = mmi->_data_prev.ptr();
				const float* pf_curr = mmi->_data_curr.ptr();
				float* pf_int = mmi->_data_interpolated.ptrw();

				bool use_lerp = mmi->quality == 0;

				// Temporary transform (needed for swizzling).
				Transform3D tp, tc, tr; // (transform prev, curr and result)

				// Test for cache friendliness versus doing branchless.
				for (int n = 0; n < num; n++) {
					// Transform.
					if (use_lerp) {
						for (int i = 0; i < mmi->_vf_size_xform; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}
					}
					else {
						// Silly swizzling, this will slow things down.
						// No idea why it is using this format...
						// ... maybe due to the shader.
						tp.basis.rows[0][0] = pf_prev[0];
						tp.basis.rows[0][1] = pf_prev[1];
						tp.basis.rows[0][2] = pf_prev[2];
						tp.basis.rows[1][0] = pf_prev[4];
						tp.basis.rows[1][1] = pf_prev[5];
						tp.basis.rows[1][2] = pf_prev[6];
						tp.basis.rows[2][0] = pf_prev[8];
						tp.basis.rows[2][1] = pf_prev[9];
						tp.basis.rows[2][2] = pf_prev[10];
						tp.origin.x = pf_prev[3];
						tp.origin.y = pf_prev[7];
						tp.origin.z = pf_prev[11];

						tc.basis.rows[0][0] = pf_curr[0];
						tc.basis.rows[0][1] = pf_curr[1];
						tc.basis.rows[0][2] = pf_curr[2];
						tc.basis.rows[1][0] = pf_curr[4];
						tc.basis.rows[1][1] = pf_curr[5];
						tc.basis.rows[1][2] = pf_curr[6];
						tc.basis.rows[2][0] = pf_curr[8];
						tc.basis.rows[2][1] = pf_curr[9];
						tc.basis.rows[2][2] = pf_curr[10];
						tc.origin.x = pf_curr[3];
						tc.origin.y = pf_curr[7];
						tc.origin.z = pf_curr[11];

						TransformInterpolator::interpolate_transform_3d(tp, tc, tr, f);

						pf_int[0] = tr.basis.rows[0][0];
						pf_int[1] = tr.basis.rows[0][1];
						pf_int[2] = tr.basis.rows[0][2];
						pf_int[4] = tr.basis.rows[1][0];
						pf_int[5] = tr.basis.rows[1][1];
						pf_int[6] = tr.basis.rows[1][2];
						pf_int[8] = tr.basis.rows[2][0];
						pf_int[9] = tr.basis.rows[2][1];
						pf_int[10] = tr.basis.rows[2][2];
						pf_int[3] = tr.origin.x;
						pf_int[7] = tr.origin.y;
						pf_int[11] = tr.origin.z;
					}

					pf_prev += mmi->_vf_size_xform;
					pf_curr += mmi->_vf_size_xform;
					pf_int += mmi->_vf_size_xform;

					// Color.
					if (mmi->_vf_size_color == 4) {
						for (int i = 0; i < 4; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}

						pf_prev += 4;
						pf_curr += 4;
						pf_int += 4;
					}

					// Custom data.
					if (mmi->_vf_size_data == 4) {
						for (int i = 0; i < 4; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}

						pf_prev += 4;
						pf_curr += 4;
						pf_int += 4;
					}
				}

				_multimesh_set_buffer(rid, mmi->_data_interpolated);

				// TODO: Make sure AABBs are constantly up to date through the interpolation?
				// NYI.
			}
		}
	}
}

// --- Merged from servers/rendering/storage/render_scene_buffers.cpp ---



// --- Merged from servers/rendering/storage/utilities.cpp ---

void Dependency::changed_notify(DependencyChangedNotification p_notification) {
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		if (E.key->changed_callback) {
			E.key->changed_callback(p_notification, E.key);
		}
	}
}

void Dependency::deleted_notify(const RID &p_rid) {
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		if (E.key->deleted_callback) {
			E.key->deleted_callback(p_rid, E.key);
		}
	}
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		E.key->dependencies.erase(this);
	}
	instances.clear();
}

Dependency::~Dependency() {
#ifdef DEBUG_ENABLED
	if (instances.size()) {
		WARN_PRINT("Leaked instance dependency: Bug - did not call instance_notify_deleted when freeing.");
		for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
			E.key->dependencies.erase(this);
		}
	}
#endif
}
