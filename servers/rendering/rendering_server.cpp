/**************************************************************************/
/*  rendering_server.cpp                                                  */
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

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "rendering_server.h"
#include "servers/rendering/renderer_canvas_cull.h"
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_warnings.h"

RID RenderingServer::get_test_texture()
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

void RenderingServer::_free_internal_rids()
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

RID RenderingServer::get_white_texture()
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

uint32_t RenderingServer::mesh_surface_get_format_offset(
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

uint32_t RenderingServer::mesh_surface_get_format_vertex_stride(uint32_t p_format, int p_vertex_len)
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

uint32_t RenderingServer::mesh_surface_get_format_normal_tangent_stride(
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

uint32_t RenderingServer::mesh_surface_get_format_attribute_stride(
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
	return astr;
}

uint32_t RenderingServer::mesh_surface_get_format_skin_stride(uint32_t p_format, int p_vertex_len)
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

uint32_t RenderingServer::mesh_surface_get_format_index_stride(uint32_t p_format, int p_vertex_len)
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

void RenderingServer::mesh_surface_make_offsets_from_format(uint64_t p_format, int p_vertex_len,
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

Rect2 RenderingServer::debug_canvas_item_get_rect(RID p_item)
{
#ifdef TOOLS_ENABLED
	return _debug_canvas_item_get_rect(p_item);
#else
	return Rect2();
#endif
}

int RenderingServer::global_shader_uniform_type_get_shader_datatype(
	RSE::GlobalShaderParameterType p_type)
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

String RenderingServer::get_current_rendering_driver_name()
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here.
	return ::OS::get_singleton()->get_current_rendering_driver_name();
}

String RenderingServer::get_current_rendering_method()
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here.
	return ::OS::get_singleton()->get_current_rendering_method();
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

void RenderingServer::mesh_add_surface_from_planes(RID p_mesh, const Vector<Plane>& p_planes)
{
	Geometry3D::MeshData mdata = Geometry3D::build_convex_mesh(p_planes);
	mesh_add_surface_from_mesh_data(p_mesh, mdata);
}

#ifndef DISABLE_DEPRECATED
void RenderingServer::set_boot_image(
	const Ref<Image>& p_image, const Color& p_color, bool p_scale, bool p_use_filter)
{
	RSE::SplashStretchMode stretch_mode =
		p_scale ? RSE::SPLASH_STRETCH_MODE_KEEP : RSE::SPLASH_STRETCH_MODE_DISABLED;
	set_boot_image_with_stretch(p_image, p_color, stretch_mode, p_use_filter);
}
#endif

RID RenderingServer::instance_create2(RID p_base, RID p_scenario)
{
	RID instance = instance_create();
	instance_set_base(instance, p_base);
	instance_set_scenario(instance, p_scenario);
	return instance;
}

bool RenderingServer::is_render_loop_enabled() { return data->render_loop_enabled; }

void RenderingServer::set_render_loop_enabled(bool p_enabled)
{
	data->render_loop_enabled = p_enabled;
}

void RenderingServer::fix_surface_compatibility(
	RenderingServerTypes::SurfaceData& p_surface, const String& p_path)
{
}

RID RenderingServer::_make_test_cube() { return RID(); }

void RenderingServer::mesh_add_surface_from_mesh_data(
	RID p_mesh, const Geometry3D::MeshData& p_mesh_data)
{
}

RID RenderingServer::make_sphere_mesh(int p_lats, int p_lons, real_t p_radius) { return RID(); }

void RenderingServer::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options)
{
}

void RenderingServer::init()
{
	RSG::canvas = memnew(RendererCanvasCull);
	RSG::viewport = memnew(RendererViewport);
	RendererSceneCull* sr = memnew(RendererSceneCull);
	RSG::camera_attributes = memnew(RendererCameraAttributes);
	RSG::scene = sr;
	RSG::rasterizer = RendererCompositor::create();
	RSG::utilities = RSG::rasterizer->get_utilities();
	RSG::rasterizer->initialize();
	RSG::light_storage = RSG::rasterizer->get_light_storage();
	RSG::material_storage = RSG::rasterizer->get_material_storage();
	RSG::mesh_storage = RSG::rasterizer->get_mesh_storage();
	RSG::particles_storage = RSG::rasterizer->get_particles_storage();
	RSG::texture_storage = RSG::rasterizer->get_texture_storage();
	RSG::gi = RSG::rasterizer->get_gi();
	RSG::fog = RSG::rasterizer->get_fog();
}

void RenderingServer::finish()
{
	if (data && data->backend.test_cube.is_valid()) {
		free_rid(data->backend.test_cube);
	}

	if (RSG::canvas) {
		RSG::canvas->finalize();
		memdelete(RSG::canvas);
	}
	if (RSG::rasterizer) {
		RSG::rasterizer->finalize();
		memdelete(RSG::rasterizer);
	}
	if (RSG::viewport) {
		memdelete(RSG::viewport);
	}
	if (RSG::scene) {
		memdelete(RSG::scene);
	}
	if (RSG::camera_attributes) {
		memdelete(RSG::camera_attributes);
	}
}

void RenderingServer::sync() {}

void RenderingServer::tick()
{
	if (RSG::canvas) {
		RSG::canvas->tick();
	}
	if (RSG::scene) {
		RSG::scene->tick();
	}
}

void RenderingServer::pre_draw(bool p_will_draw)
{
	if (RSG::scene) {
		RSG::scene->pre_draw(p_will_draw);
	}
}

/* STATE & QUERIES */

double RenderingServer::get_frame_setup_time_cpu()
{
	return data ? data->backend.frame_setup_time : 0.0;
}

bool RenderingServer::has_changed() { return data && data->backend.changes > 0; }

Color RenderingServer::get_default_clear_color()
{
	return RSG::texture_storage ? RSG::texture_storage->get_default_clear_color() : Color();
}

void RenderingServer::set_default_clear_color(const Color& p_color)
{
	if (RSG::texture_storage) {
		RSG::texture_storage->set_default_clear_color(p_color);
	}
}

uint64_t RenderingServer::get_rendering_info(RSE::RenderingInfo p_info)
{
	if (!RSG::viewport || !RSG::canvas_render || !RSG::scene || !RSG::utilities) {
		return 0;
	}

	if (p_info == RSE::RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME) {
		return RSG::viewport->get_total_objects_drawn();
	}
	else if (p_info == RSE::RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME) {
		return RSG::viewport->get_total_primitives_drawn();
	}
	else if (p_info == RSE::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME) {
		return RSG::viewport->get_total_draw_calls_used();
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_CANVAS) {
		return RSG::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_CANVAS);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_MESH) {
		return RSG::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_MESH) +
			   RSG::scene->get_pipeline_compilations(RSE::PIPELINE_SOURCE_MESH);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_SURFACE) {
		return RSG::scene->get_pipeline_compilations(RSE::PIPELINE_SOURCE_SURFACE);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_DRAW) {
		return RSG::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_DRAW) +
			   RSG::scene->get_pipeline_compilations(RSE::PIPELINE_SOURCE_DRAW);
	}
	else if (p_info == RSE::RENDERING_INFO_PIPELINE_COMPILATIONS_SPECIALIZATION) {
		return RSG::canvas_render->get_pipeline_compilations(RSE::PIPELINE_SOURCE_SPECIALIZATION) +
			   RSG::scene->get_pipeline_compilations(RSE::PIPELINE_SOURCE_SPECIALIZATION);
	}
	return RSG::utilities->get_rendering_info(p_info);
}

RenderingDeviceEnums::DeviceType RenderingServer::get_video_adapter_type()
{
	return RSG::utilities ? RSG::utilities->get_video_adapter_type()
						  : RenderingDeviceEnums::DEVICE_TYPE_OTHER;
}

void RenderingServer::set_frame_profiling_enabled(bool p_enable)
{
	if (RSG::utilities) {
		RSG::utilities->capturing_timestamps = p_enable;
	}
}

uint64_t RenderingServer::get_frame_profile_frame()
{
	return data ? data->backend.frame_profile_frame : 0;
}

Vector<RenderingServerTypes::FrameProfileArea> RenderingServer::get_frame_profile()
{
	return data ? data->backend.frame_profile : Vector<RenderingServerTypes::FrameProfileArea>();
}

void RenderingServer::sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir)
{
	if (RSG::scene) {
		RSG::scene->sdfgi_set_debug_probe_select(p_position, p_dir);
	}
}

void RenderingServer::set_print_gpu_profile(bool p_enable)
{
	if (RSG::utilities) {
		RSG::utilities->capturing_timestamps = p_enable;
	}
	if (data) {
		data->backend.print_gpu_profile = p_enable;
	}
}

RID RenderingServer::get_test_cube()
{
	if (!data) {
		return RID();
	}
	if (!data->backend.test_cube.is_valid()) {
		data->backend.test_cube = _make_test_cube();
	}
	return data->backend.test_cube;
}

bool RenderingServer::has_os_feature(const String& p_feature)
{
	return RSG::utilities ? RSG::utilities->has_os_feature(p_feature) : false;
}

void RenderingServer::set_debug_generate_wireframes(bool p_generate)
{
	if (RSG::utilities) {
		RSG::utilities->set_debug_generate_wireframes(p_generate);
	}
}

bool RenderingServer::is_low_end() { return RendererCompositor::is_low_end(); }

Size2i RenderingServer::get_maximum_viewport_size()
{
	return RSG::utilities ? RSG::utilities->get_maximum_viewport_size() : Size2i();
}

void RenderingServer::set_physics_interpolation_enabled(bool p_enabled)
{
	if (RSG::canvas) {
		RSG::canvas->set_physics_interpolation_enabled(p_enabled);
	}
	if (RSG::scene) {
		RSG::scene->set_physics_interpolation_enabled(p_enabled);
	}
}

bool RenderingServer::is_on_render_thread() { return true; }

void RenderingServer::global_shader_parameters_clear()
{
	if (RSG::material_storage) {
		RSG::material_storage->global_shader_parameters_clear();
	}
}

void RenderingServer::global_shader_parameters_load_settings(bool p_load_textures)
{
	if (RSG::material_storage) {
		RSG::material_storage->global_shader_parameters_load_settings(p_load_textures);
	}
}

RID RenderingServer::texture_2d_create(const Ref<Image>& p_image)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_2d_initialize(ret, p_image);
	return ret;
}

RID RenderingServer::texture_2d_layered_create(
	const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_2d_layered_initialize(ret, p_layers, p_layered_type);
	return ret;
}

RID RenderingServer::texture_3d_create(Image::Format p_format, int p_width, int p_height,
	int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_3d_initialize(
		ret, p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	return ret;
}

RID RenderingServer::texture_2d_placeholder_create()
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_2d_placeholder_initialize(ret);
	return ret;
}

RID RenderingServer::texture_2d_layered_placeholder_create(RSE::TextureLayeredType p_layered_type)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_2d_layered_placeholder_initialize(ret, p_layered_type);
	return ret;
}

RID RenderingServer::texture_3d_placeholder_create()
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_3d_placeholder_initialize(ret);
	return ret;
}

RID RenderingServer::texture_proxy_create(RID p_base)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_proxy_initialize(ret, p_base);
	return ret;
}

RID RenderingServer::texture_drawable_create(int p_width, int p_height,
	RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_with_mipmaps)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_drawable_initialize(
		ret, p_width, p_height, p_format, p_color, p_with_mipmaps);
	return ret;
}

void RenderingServer::texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
{
	RSG::texture_storage->texture_2d_update(p_texture, p_image, p_layer);
}

void RenderingServer::texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
{
	RSG::texture_storage->texture_3d_update(p_texture, p_data);
}

void RenderingServer::texture_proxy_update(RID p_proxy, RID p_base)
{
	RSG::texture_storage->texture_proxy_update(p_proxy, p_base);
}

Ref<Image> RenderingServer::texture_2d_get(RID p_texture)
{
	return RSG::texture_storage->texture_2d_get(p_texture);
}

Ref<Image> RenderingServer::texture_2d_layer_get(RID p_texture, int p_layer)
{
	return RSG::texture_storage->texture_2d_layer_get(p_texture, p_layer);
}

Vector<Ref<Image>> RenderingServer::texture_3d_get(RID p_texture)
{
	return RSG::texture_storage->texture_3d_get(p_texture);
}

void RenderingServer::texture_replace(RID p_texture, RID p_by_texture)
{
	RSG::texture_storage->texture_replace(p_texture, p_by_texture);
}

void RenderingServer::texture_set_size_override(RID p_texture, int p_width, int p_height)
{
	RSG::texture_storage->texture_set_size_override(p_texture, p_width, p_height);
}

void RenderingServer::texture_set_path(RID p_texture, const String& p_path)
{
	RSG::texture_storage->texture_set_path(p_texture, p_path);
}

RID RenderingServer::texture_drawable_get_default_material()
{
	return RSG::texture_storage->texture_drawable_get_default_material();
}

void RenderingServer::texture_drawable_generate_mipmaps(RID p_texture)
{
	RSG::texture_storage->texture_drawable_generate_mipmaps(p_texture);
}

uint64_t RenderingServer::texture_get_native_handle(RID p_texture, bool p_srgb)
{
	return RSG::texture_storage->texture_get_native_handle(p_texture, p_srgb);
}

void RenderingServer::canvas_set_modulate(RID p_canvas, const Color& p_color)
{
	RSG::canvas->canvas_set_modulate(p_canvas, p_color);
}

void RenderingServer::canvas_set_parent(RID p_canvas, RID p_parent, float p_scale)
{
	RSG::canvas->canvas_set_parent(p_canvas, p_parent, p_scale);
}

void RenderingServer::canvas_set_item_repeat(
	RID p_item, const Point2& p_repeat_size, int p_repeat_times)
{
	RSG::canvas->canvas_set_item_repeat(p_item, p_repeat_size, p_repeat_times);
}

void RenderingServer::canvas_texture_set_shading_parameters(
	RID p_texture, const Color& p_color, float p_shininess)
{
	RSG::canvas->canvas_texture_set_shading_parameters(p_texture, p_color, p_shininess);
}

void RenderingServer::canvas_texture_set_texture_filter(
	RID p_texture, RSE::CanvasItemTextureFilter p_filter)
{
	RSG::canvas->canvas_texture_set_texture_filter(p_texture, p_filter);
}

void RenderingServer::canvas_texture_set_texture_repeat(
	RID p_texture, RSE::CanvasItemTextureRepeat p_repeat)
{
	RSG::canvas->canvas_texture_set_texture_repeat(p_texture, p_repeat);
}

void RenderingServer::canvas_item_set_parent(RID p_item, RID p_parent)
{
	RSG::canvas->canvas_item_set_parent(p_item, p_parent);
}

void RenderingServer::canvas_item_set_visible(RID p_item, bool p_visible)
{
	RSG::canvas->canvas_item_set_visible(p_item, p_visible);
}

void RenderingServer::canvas_item_set_transform(RID p_item, const Transform2D& p_transform)
{
	RSG::canvas->canvas_item_set_transform(p_item, p_transform);
}

void RenderingServer::canvas_item_set_clip(RID p_item, bool p_clip)
{
	RSG::canvas->canvas_item_set_clip(p_item, p_clip);
}

void RenderingServer::canvas_item_set_custom_rect(
	RID p_item, bool p_custom_rect, const Rect2& p_rect)
{
	RSG::canvas->canvas_item_set_custom_rect(p_item, p_custom_rect, p_rect);
}

void RenderingServer::canvas_item_set_modulate(RID p_item, const Color& p_color)
{
	RSG::canvas->canvas_item_set_modulate(p_item, p_color);
}

void RenderingServer::canvas_item_set_self_modulate(RID p_item, const Color& p_color)
{
	RSG::canvas->canvas_item_set_self_modulate(p_item, p_color);
}

void RenderingServer::canvas_item_set_draw_behind_parent(RID p_item, bool p_enable)
{
	RSG::canvas->canvas_item_set_draw_behind_parent(p_item, p_enable);
}

void RenderingServer::canvas_item_set_use_identity_transform(RID p_item, bool p_enable)
{
	RSG::canvas->canvas_item_set_use_identity_transform(p_item, p_enable);
}

void RenderingServer::canvas_item_set_sort_children_by_y(RID p_item, bool p_enable)
{
	RSG::canvas->canvas_item_set_sort_children_by_y(p_item, p_enable);
}

void RenderingServer::canvas_item_set_z_index(RID p_item, int p_z)
{
	RSG::canvas->canvas_item_set_z_index(p_item, p_z);
}

void RenderingServer::canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable)
{
	RSG::canvas->canvas_item_set_z_as_relative_to_parent(p_item, p_enable);
}

void RenderingServer::canvas_item_set_copy_to_backbuffer(
	RID p_item, bool p_enable, const Rect2& p_rect)
{
	RSG::canvas->canvas_item_set_copy_to_backbuffer(p_item, p_enable, p_rect);
}

void RenderingServer::canvas_item_clear(RID p_item) { RSG::canvas->canvas_item_clear(p_item); }

void RenderingServer::canvas_item_set_draw_index(RID p_item, int p_index)
{
	RSG::canvas->canvas_item_set_draw_index(p_item, p_index);
}

void RenderingServer::canvas_item_set_material(RID p_item, RID p_material)
{
	RSG::canvas->canvas_item_set_material(p_item, p_material);
}

void RenderingServer::canvas_item_set_use_parent_material(RID p_item, bool p_enable)
{
	RSG::canvas->canvas_item_set_use_parent_material(p_item, p_enable);
}

void RenderingServer::canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
	float p_clear_margin, bool p_fit_empty, float p_fit_margin, bool p_blur_mipmaps)
{
	RSG::canvas->canvas_item_set_canvas_group_mode(
		p_item, p_mode, p_clear_margin, p_fit_empty, p_fit_margin, p_blur_mipmaps);
}

void RenderingServer::canvas_item_set_interpolated(RID p_item, bool p_interpolated)
{
	RSG::canvas->canvas_item_set_interpolated(p_item, p_interpolated);
}

void RenderingServer::canvas_item_reset_physics_interpolation(RID p_item)
{
	RSG::canvas->canvas_item_reset_physics_interpolation(p_item);
}

void RenderingServer::canvas_item_set_light_mask(RID p_item, int p_mask)
{
	RSG::canvas->canvas_item_set_light_mask(p_item, p_mask);
}

void RenderingServer::canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer)
{
	RSG::canvas->canvas_item_set_visibility_layer(p_item, p_visibility_layer);
}

void RenderingServer::canvas_item_set_default_texture_filter(
	RID p_item, RSE::CanvasItemTextureFilter p_filter)
{
	RSG::canvas->canvas_item_set_default_texture_filter(p_item, p_filter);
}

void RenderingServer::canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
	const Color& p_color, float p_width, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_line(p_item, p_from, p_to, p_color, p_width, p_antialiased);
}

void RenderingServer::canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_polyline(p_item, p_points, p_colors, p_width, p_antialiased);
}

void RenderingServer::canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, float p_width, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_multiline(p_item, p_points, p_colors, p_width, p_antialiased);
}

void RenderingServer::canvas_item_add_rect(
	RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_rect(p_item, p_rect, p_color, p_antialiased);
}

void RenderingServer::canvas_item_add_circle(
	RID p_item, const Point2& p_pos, float p_radius, const Color& p_color, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_circle(p_item, p_pos, p_radius, p_color, p_antialiased);
}

void RenderingServer::canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
	float p_minor, const Color& p_color, bool p_antialiased)
{
	RSG::canvas->canvas_item_add_ellipse(p_item, p_pos, p_major, p_minor, p_color, p_antialiased);
}

void RenderingServer::canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
	bool p_tile, const Color& p_modulate, bool p_transpose)
{
	RSG::canvas->canvas_item_add_texture_rect(
		p_item, p_rect, p_texture, p_tile, p_modulate, p_transpose);
}

void RenderingServer::canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose,
	bool p_clip_uv)
{
	RSG::canvas->canvas_item_add_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void RenderingServer::canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate, int p_outline_size,
	float p_px_range, float p_scale)
{
	RSG::canvas->canvas_item_add_msdf_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
}

void RenderingServer::canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
	RID p_texture, const Rect2& p_src_rect, const Color& p_modulate)
{
	RSG::canvas->canvas_item_add_lcd_texture_rect_region(
		p_item, p_rect, p_texture, p_src_rect, p_modulate);
}

void RenderingServer::canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect,
	const Rect2& p_source, RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
	RSE::NinePatchAxisMode p_x_axis_mode, RSE::NinePatchAxisMode p_y_axis_mode, bool p_draw_center,
	const Color& p_modulate)
{
	RSG::canvas->canvas_item_add_nine_patch(p_item, p_rect, p_source, p_texture, p_topleft,
		p_bottomright, p_x_axis_mode, p_y_axis_mode, p_draw_center, p_modulate);
}

void RenderingServer::canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	RSG::canvas->canvas_item_add_primitive(p_item, p_points, p_colors, p_uvs, p_texture);
}

void RenderingServer::canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture)
{
	RSG::canvas->canvas_item_add_polygon(p_item, p_points, p_colors, p_uvs, p_texture);
}

void RenderingServer::canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones, const Vector<float>& p_weights, RID p_texture, int p_count)
{
	RSG::canvas->canvas_item_add_triangle_array(
		p_item, p_indices, p_points, p_colors, p_uvs, p_bones, p_weights, p_texture, p_count);
}

void RenderingServer::canvas_item_add_mesh(RID p_item, const RID& p_mesh,
	const Transform2D& p_transform, const Color& p_modulate, RID p_texture)
{
	RSG::canvas->canvas_item_add_mesh(p_item, p_mesh, p_transform, p_modulate, p_texture);
}

void RenderingServer::canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture)
{
	RSG::canvas->canvas_item_add_multimesh(p_item, p_mesh, p_texture);
}

void RenderingServer::canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform)
{
	RSG::canvas->canvas_item_add_set_transform(p_item, p_transform);
}

void RenderingServer::canvas_item_add_animation_slice(RID p_item, double p_animation_length,
	double p_slice_begin, double p_slice_end, double p_offset)
{
	RSG::canvas->canvas_item_add_animation_slice(
		p_item, p_animation_length, p_slice_begin, p_slice_end, p_offset);
}

void RenderingServer::canvas_item_attach_skeleton(RID p_item, RID p_skeleton)
{
	RSG::canvas->canvas_item_attach_skeleton(p_item, p_skeleton);
}

void RenderingServer::viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count)
{
	RSG::viewport->viewport_set_size(p_viewport, p_width, p_height, p_view_count);
}

void RenderingServer::viewport_set_active(RID p_viewport, bool p_active)
{
	RSG::viewport->viewport_set_active(p_viewport, p_active);
}

void RenderingServer::viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport)
{
	RSG::viewport->viewport_set_parent_viewport(p_viewport, p_parent_viewport);
}

void RenderingServer::viewport_attach_to_screen(
	RID p_viewport, const Rect2& p_rect, DisplayServerEnums::WindowID p_screen)
{
	RSG::viewport->viewport_attach_to_screen(p_viewport, p_rect, p_screen);
}

void RenderingServer::viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable)
{
	RSG::viewport->viewport_set_render_direct_to_screen(p_viewport, p_enable);
}

void RenderingServer::viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode)
{
	RSG::viewport->viewport_set_update_mode(p_viewport, p_mode);
}

RSE::ViewportUpdateMode RenderingServer::viewport_get_update_mode(RID p_viewport)
{
	return RSG::viewport->viewport_get_update_mode(p_viewport);
}

void RenderingServer::viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode)
{
	RSG::viewport->viewport_set_clear_mode(p_viewport, p_clear_mode);
}

RID RenderingServer::viewport_get_render_target(RID p_viewport)
{
	return RSG::viewport->viewport_get_render_target(p_viewport);
}

RID RenderingServer::viewport_get_texture(RID p_viewport)
{
	return RSG::viewport->viewport_get_texture(p_viewport);
}

void RenderingServer::viewport_set_disable_3d(RID p_viewport, bool p_disable)
{
	RSG::viewport->viewport_set_disable_3d(p_viewport, p_disable);
}

void RenderingServer::viewport_set_disable_2d(RID p_viewport, bool p_disable)
{
	RSG::viewport->viewport_set_disable_2d(p_viewport, p_disable);
}

void RenderingServer::viewport_set_scenario(RID p_viewport, RID p_scenario)
{
	RSG::viewport->viewport_set_scenario(p_viewport, p_scenario);
}

void RenderingServer::viewport_attach_canvas(RID p_viewport, RID p_canvas)
{
	RSG::viewport->viewport_attach_canvas(p_viewport, p_canvas);
}

void RenderingServer::viewport_remove_canvas(RID p_viewport, RID p_canvas)
{
	RSG::viewport->viewport_remove_canvas(p_viewport, p_canvas);
}

void RenderingServer::viewport_set_canvas_transform(
	RID p_viewport, RID p_canvas, const Transform2D& p_offset)
{
	RSG::viewport->viewport_set_canvas_transform(p_viewport, p_canvas, p_offset);
}

void RenderingServer::viewport_set_transparent_background(RID p_viewport, bool p_enabled)
{
	RSG::viewport->viewport_set_transparent_background(p_viewport, p_enabled);
}

void RenderingServer::viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr)
{
	RSG::viewport->viewport_set_use_hdr_2d(p_viewport, p_use_hdr);
}

bool RenderingServer::viewport_is_using_hdr_2d(RID p_viewport)
{
	return RSG::viewport->viewport_is_using_hdr_2d(p_viewport);
}

void RenderingServer::viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled)
{
	RSG::viewport->viewport_set_snap_2d_transforms_to_pixel(p_viewport, p_enabled);
}

void RenderingServer::viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled)
{
	RSG::viewport->viewport_set_snap_2d_vertices_to_pixel(p_viewport, p_enabled);
}

void RenderingServer::viewport_set_global_canvas_transform(
	RID p_viewport, const Transform2D& p_transform)
{
	RSG::viewport->viewport_set_global_canvas_transform(p_viewport, p_transform);
}

void RenderingServer::viewport_set_canvas_stacking(
	RID p_viewport, RID p_canvas, int p_layer, int p_sublayer)
{
	RSG::viewport->viewport_set_canvas_stacking(p_viewport, p_canvas, p_layer, p_sublayer);
}

void RenderingServer::viewport_set_sdf_oversize_and_scale(
	RID p_viewport, RSE::ViewportSDFOversize p_oversize, RSE::ViewportSDFScale p_scale)
{
	RSG::viewport->viewport_set_sdf_oversize_and_scale(p_viewport, p_oversize, p_scale);
}

void RenderingServer::viewport_set_positional_shadow_atlas_size(
	RID p_viewport, int p_size, bool p_16_bits)
{
	RSG::viewport->viewport_set_positional_shadow_atlas_size(p_viewport, p_size, p_16_bits);
}

void RenderingServer::viewport_set_positional_shadow_atlas_quadrant_subdivision(
	RID p_viewport, int p_quadrant, int p_subdiv)
{
	RSG::viewport->viewport_set_positional_shadow_atlas_quadrant_subdivision(
		p_viewport, p_quadrant, p_subdiv);
}

void RenderingServer::viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	RSG::viewport->viewport_set_msaa_3d(p_viewport, p_msaa);
}

void RenderingServer::viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa)
{
	RSG::viewport->viewport_set_msaa_2d(p_viewport, p_msaa);
}

void RenderingServer::viewport_set_screen_space_aa(
	RID p_viewport, RSE::ViewportScreenSpaceAA p_mode)
{
	RSG::viewport->viewport_set_screen_space_aa(p_viewport, p_mode);
}

void RenderingServer::viewport_set_use_taa(RID p_viewport, bool p_use_taa)
{
	RSG::viewport->viewport_set_use_taa(p_viewport, p_use_taa);
}

void RenderingServer::viewport_set_use_debanding(RID p_viewport, bool p_use_debanding)
{
	RSG::viewport->viewport_set_use_debanding(p_viewport, p_use_debanding);
}

void RenderingServer::viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels)
{
	RSG::viewport->viewport_set_mesh_lod_threshold(p_viewport, p_pixels);
}

int RenderingServer::viewport_get_render_info(
	RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info)
{
	return RSG::viewport->viewport_get_render_info(p_viewport, p_type, p_info);
}

void RenderingServer::viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw)
{
	RSG::viewport->viewport_set_debug_draw(p_viewport, p_draw);
}

void RenderingServer::viewport_set_measure_render_time(RID p_viewport, bool p_enable)
{
	RSG::viewport->viewport_set_measure_render_time(p_viewport, p_enable);
}

double RenderingServer::viewport_get_measured_render_time_cpu(RID p_viewport)
{
	return RSG::viewport->viewport_get_measured_render_time_cpu(p_viewport);
}

double RenderingServer::viewport_get_measured_render_time_gpu(RID p_viewport)
{
	return RSG::viewport->viewport_get_measured_render_time_gpu(p_viewport);
}

RID RenderingServer::viewport_find_from_screen_attachment(DisplayServerEnums::WindowID p_id)
{
	return RSG::viewport->viewport_find_from_screen_attachment(p_id);
}

void RenderingServer::viewport_set_scaling_3d_mode(
	RID p_viewport, RSE::ViewportScaling3DMode p_scaling_3d_mode)
{
	RSG::viewport->viewport_set_scaling_3d_mode(p_viewport, p_scaling_3d_mode);
}

void RenderingServer::viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale)
{
	RSG::viewport->viewport_set_scaling_3d_scale(p_viewport, p_scaling_3d_scale);
}

void RenderingServer::viewport_set_fsr_sharpness(RID p_viewport, float p_fsr_sharpness)
{
	RSG::viewport->viewport_set_fsr_sharpness(p_viewport, p_fsr_sharpness);
}

void RenderingServer::viewport_set_texture_mipmap_bias(RID p_viewport, float p_texture_mipmap_bias)
{
	RSG::viewport->viewport_set_texture_mipmap_bias(p_viewport, p_texture_mipmap_bias);
}

void RenderingServer::viewport_set_anisotropic_filtering_level(
	RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
{
	RSG::viewport->viewport_set_anisotropic_filtering_level(
		p_viewport, p_anisotropic_filtering_level);
}

void RenderingServer::viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask)
{
	RSG::viewport->viewport_set_canvas_cull_mask(p_viewport, p_canvas_cull_mask);
}

void RenderingServer::viewport_set_vrs_update_mode(
	RID p_viewport, RSE::ViewportVRSUpdateMode p_mode)
{
	RSG::viewport->viewport_set_vrs_update_mode(p_viewport, p_mode);
}

void RenderingServer::viewport_set_vrs_texture(RID p_viewport, RID p_texture)
{
	RSG::viewport->viewport_set_vrs_texture(p_viewport, p_texture);
}

void RenderingServer::scenario_set_environment(RID p_scenario, RID p_environment)
{
	RSG::scene->scenario_set_environment(p_scenario, p_environment);
}

void RenderingServer::scenario_set_fallback_environment(RID p_scenario, RID p_environment)
{
	RSG::scene->scenario_set_fallback_environment(p_scenario, p_environment);
}

void RenderingServer::scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes)
{
	RSG::scene->scenario_set_camera_attributes(p_scenario, p_camera_attributes);
}

void RenderingServer::scenario_set_compositor(RID p_scenario, RID p_compositor)
{
	RSG::scene->scenario_set_compositor(p_scenario, p_compositor);
}

void RenderingServer::instance_set_base(RID p_instance, RID p_base)
{
	RSG::scene->instance_set_base(p_instance, p_base);
}

void RenderingServer::instance_set_scenario(RID p_instance, RID p_scenario)
{
	RSG::scene->instance_set_scenario(p_instance, p_scenario);
}

void RenderingServer::instance_set_layer_mask(RID p_instance, uint32_t p_mask)
{
	RSG::scene->instance_set_layer_mask(p_instance, p_mask);
}

void RenderingServer::instance_set_pivot_data(
	RID p_instance, float p_sorting_offset, bool p_use_aabb_center)
{
	RSG::scene->instance_set_pivot_data(p_instance, p_sorting_offset, p_use_aabb_center);
}

void RenderingServer::instance_set_transform(RID p_instance, const Transform3D& p_transform)
{
	RSG::scene->instance_set_transform(p_instance, p_transform);
}

void RenderingServer::instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight)
{
	RSG::scene->instance_set_blend_shape_weight(p_instance, p_shape, p_weight);
}

void RenderingServer::instance_set_surface_override_material(
	RID p_instance, int p_surface, RID p_material)
{
	RSG::scene->instance_set_surface_override_material(p_instance, p_surface, p_material);
}

void RenderingServer::instance_set_visible(RID p_instance, bool p_visible)
{
	RSG::scene->instance_set_visible(p_instance, p_visible);
}

void RenderingServer::instance_teleport(RID p_instance)
{
	RSG::scene->instance_teleport(p_instance);
}

void RenderingServer::instance_attach_skeleton(RID p_instance, RID p_skeleton)
{
	RSG::scene->instance_attach_skeleton(p_instance, p_skeleton);
}

void RenderingServer::instance_set_extra_visibility_margin(RID p_instance, real_t p_margin)
{
	RSG::scene->instance_set_extra_visibility_margin(p_instance, p_margin);
}

void RenderingServer::instance_geometry_set_flag(
	RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled)
{
	RSG::scene->instance_geometry_set_flag(p_instance, p_flags, p_enabled);
}

void RenderingServer::instance_geometry_set_cast_shadows_setting(
	RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting)
{
	RSG::scene->instance_geometry_set_cast_shadows_setting(p_instance, p_shadow_casting_setting);
}

void RenderingServer::instance_geometry_set_material_override(RID p_instance, RID p_material)
{
	RSG::scene->instance_geometry_set_material_override(p_instance, p_material);
}

void RenderingServer::instance_geometry_set_material_overlay(RID p_instance, RID p_material)
{
	RSG::scene->instance_geometry_set_material_overlay(p_instance, p_material);
}

void RenderingServer::instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias)
{
	RSG::scene->instance_geometry_set_lod_bias(p_instance, p_lod_bias);
}

void RenderingServer::camera_set_perspective(
	RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far)
{
	RSG::scene->camera_set_perspective(p_camera, p_fovy_degrees, p_z_near, p_z_far);
}

void RenderingServer::camera_set_orthogonal(
	RID p_camera, float p_size, float p_z_near, float p_z_far)
{
	RSG::scene->camera_set_orthogonal(p_camera, p_size, p_z_near, p_z_far);
}

void RenderingServer::camera_set_frustum(
	RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far)
{
	RSG::scene->camera_set_frustum(p_camera, p_size, p_offset, p_z_near, p_z_far);
}

void RenderingServer::camera_set_transform(RID p_camera, const Transform3D& p_transform)
{
	RSG::scene->camera_set_transform(p_camera, p_transform);
}

RID RenderingServer::mesh_create() { return RSG::mesh_storage->mesh_allocate(); }

void RenderingServer::mesh_add_surface(
	RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
{
	RSG::mesh_storage->mesh_add_surface(p_mesh, p_surface);
}

void RenderingServer::mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
{
	RSG::mesh_storage->mesh_set_blend_shape_count(p_mesh, p_blend_shape_count);
}

void RenderingServer::mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
{
	RSG::mesh_storage->mesh_set_blend_shape_mode(p_mesh, p_mode);
}

void RenderingServer::mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb)
{
	RSG::mesh_storage->mesh_set_custom_aabb(p_mesh, p_aabb);
}

void RenderingServer::mesh_set_path(RID p_mesh, const String& p_path)
{
	RSG::mesh_storage->mesh_set_path(p_mesh, p_path);
}

void RenderingServer::mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh)
{
	RSG::mesh_storage->mesh_set_shadow_mesh(p_mesh, p_shadow_mesh);
}

void RenderingServer::mesh_clear(RID p_mesh) { RSG::mesh_storage->mesh_clear(p_mesh); }

void RenderingServer::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
{
	RSG::mesh_storage->mesh_surface_set_material(p_mesh, p_surface, p_material);
}

void RenderingServer::mesh_surface_update_vertex_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RSG::mesh_storage->mesh_surface_update_vertex_region(p_mesh, p_surface, p_offset, p_data);
}

void RenderingServer::mesh_surface_update_attribute_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RSG::mesh_storage->mesh_surface_update_attribute_region(p_mesh, p_surface, p_offset, p_data);
}

void RenderingServer::mesh_surface_update_skin_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	RSG::mesh_storage->mesh_surface_update_skin_region(p_mesh, p_surface, p_offset, p_data);
}

RenderingServerTypes::SurfaceData RenderingServer::mesh_get_surface(RID p_mesh, int p_surface)
{
	return RSG::mesh_storage->mesh_get_surface(p_mesh, p_surface);
}

RID RenderingServer::shader_create()
{
	RID ret = RSG::material_storage->shader_allocate();
	RSG::material_storage->shader_initialize(ret, false);
	return ret;
}

RID RenderingServer::shader_create_from_code(const String& p_code, const String& p_path_hint)
{
	RID shader = RSG::material_storage->shader_allocate();
	RSG::material_storage->shader_initialize(shader, false);
	RSG::material_storage->shader_set_path_hint(shader, p_path_hint);
	RSG::material_storage->shader_set_code(shader, p_code);
	return shader;
}

void RenderingServer::shader_set_code(RID p_shader, const String& p_code)
{
	RSG::material_storage->shader_set_code(p_shader, p_code);
}

void RenderingServer::shader_set_path_hint(RID p_shader, const String& p_path)
{
	RSG::material_storage->shader_set_path_hint(p_shader, p_path);
}

void RenderingServer::shader_set_default_texture_parameter(
	RID p_shader, const StringName& p_name, RID p_texture, int p_index)
{
	RSG::material_storage->shader_set_default_texture_parameter(
		p_shader, p_name, p_texture, p_index);
}

RID RenderingServer::material_create()
{
	RID ret = RSG::material_storage->material_allocate();
	RSG::material_storage->material_initialize(ret);
	return ret;
}

void RenderingServer::material_set_shader(RID p_shader_material, RID p_shader)
{
	RSG::material_storage->material_set_shader(p_shader_material, p_shader);
}

void RenderingServer::material_set_render_priority(RID p_material, int priority)
{
	RSG::material_storage->material_set_render_priority(p_material, priority);
}

void RenderingServer::material_set_next_pass(RID p_material, RID p_next_material)
{
	RSG::material_storage->material_set_next_pass(p_material, p_next_material);
}

void RenderingServer::material_set_use_debanding(bool p_enable)
{
	if (RSG::scene) {
		RSG::scene->material_set_use_debanding(p_enable);
	}
}

void RenderingServer::free_rid(RID p_rid)
{
	if (!p_rid.is_valid()) {
		return;
	}

	if (RSG::viewport && RSG::viewport->free(p_rid)) {
		return;
	}
	if (RSG::scene && RSG::scene->free(p_rid)) {
		return;
	}
	if (RSG::utilities && RSG::utilities->free(p_rid)) {
		return;
	}
}

RID RenderingServer::canvas_create()
{
	RID ret = RSG::canvas->canvas_allocate();
	RSG::canvas->canvas_initialize(ret);
	return ret;
}

RID RenderingServer::canvas_texture_create()
{
	RID ret = RSG::canvas->canvas_texture_allocate();
	RSG::canvas->canvas_texture_initialize(ret);
	return ret;
}

RID RenderingServer::canvas_item_create()
{
	RID ret = RSG::canvas->canvas_item_allocate();
	RSG::canvas->canvas_item_initialize(ret);
	return ret;
}

RID RenderingServer::canvas_light_create()
{
	RID ret = RSG::canvas->canvas_light_allocate();
	RSG::canvas->canvas_light_initialize(ret);
	return ret;
}

RID RenderingServer::canvas_light_occluder_create()
{
	RID ret = RSG::canvas->canvas_light_occluder_allocate();
	RSG::canvas->canvas_light_occluder_initialize(ret);
	return ret;
}

RID RenderingServer::canvas_occluder_polygon_create()
{
	RID ret = RSG::canvas->canvas_occluder_polygon_allocate();
	RSG::canvas->canvas_occluder_polygon_initialize(ret);
	return ret;
}

RID RenderingServer::viewport_create()
{
	RID ret = RSG::viewport->viewport_allocate();
	RSG::viewport->viewport_initialize(ret);
	return ret;
}

RID RenderingServer::scenario_create()
{
	RID ret = RSG::scene->scenario_allocate();
	RSG::scene->scenario_initialize(ret);
	return ret;
}

RID RenderingServer::instance_create()
{
	RID ret = RSG::scene->instance_allocate();
	RSG::scene->instance_initialize(ret);
	return ret;
}

RID RenderingServer::camera_create()
{
	RID ret = RSG::scene->camera_allocate();
	RSG::scene->camera_initialize(ret);
	return ret;
}

String RenderingServer::get_video_adapter_name()
{
	return RSG::utilities ? RSG::utilities->get_video_adapter_name() : String();
}

String RenderingServer::get_video_adapter_vendor()
{
	return RSG::utilities ? RSG::utilities->get_video_adapter_vendor() : String();
}

String RenderingServer::get_video_adapter_api_version()
{
	return RSG::utilities ? RSG::utilities->get_video_adapter_api_version() : String();
}

void RenderingServer::set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
	RSE::SplashStretchMode p_mode, bool p_use_filter)
{
	if (RSG::rasterizer) {
		RSG::rasterizer->set_boot_image_with_stretch(p_image, p_color, p_mode, p_use_filter);
	}
}

void RenderingServer::canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated)
{
	RSG::canvas->canvas_light_occluder_set_interpolated(p_occluder, p_interpolated);
}

void RenderingServer::canvas_light_occluder_reset_physics_interpolation(RID p_occluder)
{
	RSG::canvas->canvas_light_occluder_reset_physics_interpolation(p_occluder);
}

void RenderingServer::canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas)
{
	RSG::canvas->canvas_light_occluder_attach_to_canvas(p_occluder, p_canvas);
}

void RenderingServer::canvas_light_occluder_set_transform(
	RID p_occluder, const Transform2D& p_xform)
{
	RSG::canvas->canvas_light_occluder_set_transform(p_occluder, p_xform);
}

void RenderingServer::canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask)
{
	RSG::canvas->canvas_light_occluder_set_light_mask(p_occluder, p_mask);
}

void RenderingServer::canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable)
{
	RSG::canvas->canvas_light_occluder_set_as_sdf_collision(p_occluder, p_enable);
}

void RenderingServer::canvas_occluder_polygon_set_shape(
	RID p_polygon, const Vector<Vector2>& p_shape, bool p_closed)
{
	RSG::canvas->canvas_occluder_polygon_set_shape(p_polygon, p_shape, p_closed);
}

void RenderingServer::canvas_occluder_polygon_set_cull_mode(
	RID p_polygon, RSE::CanvasOccluderPolygonCullMode p_mode)
{
	RSG::canvas->canvas_occluder_polygon_set_cull_mode(p_polygon, p_mode);
}

Rect2 RenderingServer::_debug_canvas_item_get_rect(RID p_item)
{
	return RSG::canvas ? RSG::canvas->_debug_canvas_item_get_rect(p_item) : Rect2();
}

void RenderingServer::canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset)
{
	RSG::canvas->canvas_light_set_texture_offset(p_light, p_offset);
}

void RenderingServer::canvas_light_attach_to_canvas(RID p_light, RID p_canvas)
{
	RSG::canvas->canvas_light_attach_to_canvas(p_light, p_canvas);
}

void RenderingServer::canvas_light_set_transform(RID p_light, const Transform2D& p_transform)
{
	RSG::canvas->canvas_light_set_transform(p_light, p_transform);
}

void RenderingServer::canvas_light_set_texture_scale(RID p_light, float p_scale)
{
	RSG::canvas->canvas_light_set_texture_scale(p_light, p_scale);
}

void RenderingServer::canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode)
{
	RSG::canvas->canvas_light_set_mode(p_light, p_mode);
}

void RenderingServer::canvas_light_set_directional_distance(RID p_light, float p_distance)
{
	RSG::canvas->canvas_light_set_directional_distance(p_light, p_distance);
}

void RenderingServer::canvas_light_set_interpolated(RID p_light, bool p_interpolated)
{
	RSG::canvas->canvas_light_set_interpolated(p_light, p_interpolated);
}

void RenderingServer::canvas_light_set_enabled(RID p_light, bool p_enabled)
{
	RSG::canvas->canvas_light_set_enabled(p_light, p_enabled);
}

void RenderingServer::canvas_light_set_color(RID p_light, const Color& p_color)
{
	RSG::canvas->canvas_light_set_color(p_light, p_color);
}

void RenderingServer::canvas_light_set_height(RID p_light, float p_height)
{
	RSG::canvas->canvas_light_set_height(p_light, p_height);
}

void RenderingServer::canvas_light_set_energy(RID p_light, float p_energy)
{
	RSG::canvas->canvas_light_set_energy(p_light, p_energy);
}

void RenderingServer::canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z)
{
	RSG::canvas->canvas_light_set_z_range(p_light, p_min_z, p_max_z);
}

void RenderingServer::canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer)
{
	RSG::canvas->canvas_light_set_layer_range(p_light, p_min_layer, p_max_layer);
}

void RenderingServer::canvas_light_set_item_cull_mask(RID p_light, int p_mask)
{
	RSG::canvas->canvas_light_set_item_cull_mask(p_light, p_mask);
}

void RenderingServer::canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask)
{
	RSG::canvas->canvas_light_set_item_shadow_cull_mask(p_light, p_mask);
}

void RenderingServer::canvas_light_set_shadow_enabled(RID p_light, bool p_enabled)
{
	RSG::canvas->canvas_light_set_shadow_enabled(p_light, p_enabled);
}

void RenderingServer::canvas_light_set_shadow_filter(
	RID p_light, RSE::CanvasLightShadowFilter p_filter)
{
	RSG::canvas->canvas_light_set_shadow_filter(p_light, p_filter);
}

void RenderingServer::canvas_light_set_shadow_color(RID p_light, const Color& p_color)
{
	RSG::canvas->canvas_light_set_shadow_color(p_light, p_color);
}

void RenderingServer::canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode)
{
	RSG::canvas->canvas_light_set_blend_mode(p_light, p_mode);
}

void RenderingServer::canvas_light_reset_physics_interpolation(RID p_light)
{
	RSG::canvas->canvas_light_reset_physics_interpolation(p_light);
}

void RenderingServer::canvas_light_set_shadow_smooth(RID p_light, float p_smooth)
{
	RSG::canvas->canvas_light_set_shadow_smooth(p_light, p_smooth);
}

RID RenderingServer::lightmap_create()
{
	RID ret = RSG::light_storage->lightmap_allocate();
	RSG::light_storage->lightmap_initialize(ret);
	return ret;
}

PackedVector3Array RenderingServer::lightmap_get_probe_capture_points(RID p_lightmap)
{
	return RSG::light_storage->lightmap_get_probe_capture_points(p_lightmap);
}

PackedColorArray RenderingServer::lightmap_get_probe_capture_sh(RID p_lightmap)
{
	return RSG::light_storage->lightmap_get_probe_capture_sh(p_lightmap);
}

PackedInt32Array RenderingServer::lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
{
	return RSG::light_storage->lightmap_get_probe_capture_tetrahedra(p_lightmap);
}

PackedInt32Array RenderingServer::lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
{
	return RSG::light_storage->lightmap_get_probe_capture_bsp_tree(p_lightmap);
}

void RenderingServer::lightmap_set_probe_capture_data(RID p_lightmap,
	const PackedVector3Array& p_points, const PackedColorArray& p_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree)
{
	RSG::light_storage->lightmap_set_probe_capture_data(
		p_lightmap, p_points, p_sh, p_tetrahedra, p_bsp_tree);
}

void RenderingServer::lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
{
	RSG::light_storage->lightmap_set_probe_bounds(p_lightmap, p_bounds);
}

void RenderingServer::lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
{
	RSG::light_storage->lightmap_set_probe_interior(p_lightmap, p_interior);
}

void RenderingServer::lightmap_set_baked_exposure_normalization(
	RID p_lightmap, float p_normalization)
{
	RSG::light_storage->lightmap_set_baked_exposure_normalization(p_lightmap, p_normalization);
}

void RenderingServer::lightmap_set_textures(
	RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
{
	RSG::light_storage->lightmap_set_textures(p_lightmap, p_light, p_uses_spherical_harmonics);
}

void RenderingServer::lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
{
	RSG::light_storage->lightmap_set_shadowmask_textures(p_lightmap, p_shadow);
}

void RenderingServer::lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode)
{
	RSG::light_storage->lightmap_set_shadowmask_mode(p_lightmap, p_mode);
}

RSE::ShadowmaskMode RenderingServer::lightmap_get_shadowmask_mode(RID p_lightmap)
{
	return RSG::light_storage->lightmap_get_shadowmask_mode(p_lightmap);
}

void RenderingServer::light_set_negative(RID p_light, bool p_enable)
{
	RSG::light_storage->light_set_negative(p_light, p_enable);
}

void RenderingServer::light_set_distance_fade(
	RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
{
	RSG::light_storage->light_set_distance_fade(p_light, p_enabled, p_begin, p_shadow, p_length);
}

void RenderingServer::light_set_cull_mask(RID p_light, uint32_t p_mask)
{
	RSG::light_storage->light_set_cull_mask(p_light, p_mask);
}

void RenderingServer::light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
{
	RSG::light_storage->light_set_reverse_cull_face_mode(p_light, p_enabled);
}

void RenderingServer::light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
{
	RSG::light_storage->light_set_shadow_caster_mask(p_light, p_caster_mask);
}

void RenderingServer::light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
{
	RSG::light_storage->light_set_bake_mode(p_light, p_bake_mode);
}

void RenderingServer::light_directional_set_blend_splits(RID p_light, bool p_enable)
{
	RSG::light_storage->light_directional_set_blend_splits(p_light, p_enable);
}

void RenderingServer::light_directional_set_sky_mode(
	RID p_light, RSE::LightDirectionalSkyMode p_mode)
{
	RSG::light_storage->light_directional_set_sky_mode(p_light, p_mode);
}

void RenderingServer::light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode)
{
	RSG::light_storage->light_omni_set_shadow_mode(p_light, p_mode);
}

void RenderingServer::light_area_set_normalize_energy(RID p_light, bool p_enable)
{
	RSG::light_storage->light_area_set_normalize_energy(p_light, p_enable);
}

RID RenderingServer::camera_attributes_create()
{
	RID ret = RSG::camera_attributes->camera_attributes_allocate();
	RSG::camera_attributes->camera_attributes_initialize(ret);
	return ret;
}

void RenderingServer::camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
	float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale)
{
	RSG::camera_attributes->camera_attributes_set_auto_exposure(
		p_camera_attributes, p_enable, p_min_sensitivity, p_max_sensitivity, p_speed, p_scale);
}

void RenderingServer::camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
	float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
	float p_near_transition, float p_amount)
{
	RSG::camera_attributes->camera_attributes_set_dof_blur(p_camera_attributes, p_far_enable,
		p_far_distance, p_far_transition, p_near_enable, p_near_distance, p_near_transition,
		p_amount);
}

void RenderingServer::camera_attributes_set_exposure(
	RID p_camera_attributes, float p_multiplier, float p_normalization)
{
	RSG::camera_attributes->camera_attributes_set_exposure(
		p_camera_attributes, p_multiplier, p_normalization);
}

void RenderingServer::environment_set_sky_orientation(RID p_env, const Basis& p_orientation)
{
	RSG::scene->environment_set_sky_orientation(p_env, p_orientation);
}

void RenderingServer::environment_set_fog_depth(
	RID p_env, float p_curve, float p_begin, float p_end)
{
	RSG::scene->environment_set_fog_depth(p_env, p_curve, p_begin, p_end);
}

void RenderingServer::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
	const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
	float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
	float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect)
{
	RSG::scene->environment_set_volumetric_fog(p_env, p_enable, p_density, p_albedo, p_emission,
		p_emission_energy, p_anisotropy, p_length, p_detail_spread, p_gi_inject,
		p_temporal_reprojection, p_temporal_reprojection_amount, p_ambient_inject, p_sky_affect);
}

void RenderingServer::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
	float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
	RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map)
{
	RSG::scene->environment_set_glow(p_env, p_enable, p_levels, p_intensity, p_strength, p_mix,
		p_bloom_threshold, p_blend_mode, p_hdr_bleed_threshold, p_hdr_bleed_scale,
		p_hdr_luminance_cap, p_glow_map_strength, p_glow_map);
}

void RenderingServer::environment_set_sky(RID p_env, RID p_sky)
{
	RSG::scene->environment_set_sky(p_env, p_sky);
}

void RenderingServer::environment_set_sky_custom_fov(RID p_env, float p_scale)
{
	RSG::scene->environment_set_sky_custom_fov(p_env, p_scale);
}

void RenderingServer::environment_set_bg_color(RID p_env, const Color& p_color)
{
	RSG::scene->environment_set_bg_color(p_env, p_color);
}

void RenderingServer::environment_set_bg_energy(RID p_env, float p_energy, float p_multiplier)
{
	RSG::scene->environment_set_bg_energy(p_env, p_energy, p_multiplier);
}

void RenderingServer::environment_set_canvas_max_layer(RID p_env, int p_max_layer)
{
	RSG::scene->environment_set_canvas_max_layer(p_env, p_max_layer);
}

void RenderingServer::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id)
{
	RSG::scene->environment_set_camera_feed_id(p_env, p_camera_feed_id);
}

void RenderingServer::environment_set_ambient_light(RID p_env, const Color& p_color,
	RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution,
	RSE::EnvironmentReflectionSource p_reflection_source)
{
	RSG::scene->environment_set_ambient_light(
		p_env, p_color, p_ambient, p_energy, p_sky_contribution, p_reflection_source);
}

void RenderingServer::environment_set_tonemap(
	RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white)
{
	RSG::scene->environment_set_tonemap(p_env, p_tone_mapper, p_exposure, p_white);
}

void RenderingServer::environment_set_tonemap_agx_contrast(RID p_env, float p_contrast)
{
	RSG::scene->environment_set_tonemap_agx_contrast(p_env, p_contrast);
}

void RenderingServer::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps,
	float p_fade_in, float p_fade_out, float p_depth_tolerance)
{
	RSG::scene->environment_set_ssr(
		p_env, p_enable, p_max_steps, p_fade_in, p_fade_out, p_depth_tolerance);
}

void RenderingServer::environment_set_ssao(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_power, float p_detail, float p_horizon, float p_sharpness,
	float p_light_affect, float p_ao_channel_affect)
{
	RSG::scene->environment_set_ssao(p_env, p_enable, p_radius, p_intensity, p_power, p_detail,
		p_horizon, p_sharpness, p_light_affect, p_ao_channel_affect);
}

void RenderingServer::environment_set_ssil(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_sharpness, float p_normal_rejection)
{
	RSG::scene->environment_set_ssil(
		p_env, p_enable, p_radius, p_intensity, p_sharpness, p_normal_rejection);
}

void RenderingServer::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
	float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
	float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
	float p_probe_bias)
{
	RSG::scene->environment_set_sdfgi(p_env, p_enable, p_cascades, p_min_cell_size, p_y_scale,
		p_use_occlusion, p_bounce_feedback, p_read_sky, p_energy, p_normal_bias, p_probe_bias);
}

void RenderingServer::environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
	float p_light_energy, float p_sun_scatter, float p_density, float p_height,
	float p_height_density, float p_aerial_perspective, float p_sky_affect,
	RSE::EnvironmentFogMode p_mode)
{
	RSG::scene->environment_set_fog(p_env, p_enable, p_light_color, p_light_energy, p_sun_scatter,
		p_density, p_height, p_height_density, p_aerial_perspective, p_sky_affect, p_mode);
}

void RenderingServer::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
	float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction)
{
	RSG::scene->environment_set_adjustment(p_env, p_enable, p_brightness, p_contrast, p_saturation,
		p_use_1d_color_correction, p_color_correction);
}

RID RenderingServer::compositor_create()
{
	RID ret = RSG::scene->compositor_allocate();
	RSG::scene->compositor_initialize(ret);
	return ret;
}

void RenderingServer::compositor_effect_set_enabled(RID p_effect, bool p_enabled)
{
	RSG::scene->compositor_effect_set_enabled(p_effect, p_enabled);
}

void RenderingServer::compositor_effect_set_flag(
	RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set)
{
	RSG::scene->compositor_effect_set_flag(p_effect, p_flag, p_set);
}

RID RenderingServer::sky_create()
{
	RID ret = RSG::scene->sky_allocate();
	RSG::scene->sky_initialize(ret);
	return ret;
}

void RenderingServer::sky_set_radiance_size(RID p_sky, int p_radiance_size)
{
	RSG::scene->sky_set_radiance_size(p_sky, p_radiance_size);
}

void RenderingServer::sky_set_mode(RID p_sky, RSE::SkyMode p_mode)
{
	RSG::scene->sky_set_mode(p_sky, p_mode);
}

void RenderingServer::sky_set_material(RID p_sky, RID p_material)
{
	RSG::scene->sky_set_material(p_sky, p_material);
}

RID RenderingServer::occluder_create()
{
	RID ret = RSG::scene->occluder_allocate();
	RSG::scene->occluder_initialize(ret);
	return ret;
}

void RenderingServer::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	RSG::scene->occluder_set_mesh(p_occluder, p_vertices, p_indices);
}

RID RenderingServer::multimesh_create()
{
	RID ret = RSG::mesh_storage->multimesh_allocate();
	RSG::mesh_storage->multimesh_initialize(ret);
	return ret;
}

void RenderingServer::multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	RSG::mesh_storage->multimesh_allocate_data(p_multimesh, p_instances, p_transform_format,
		p_use_colors, p_use_custom_data, p_use_indirect);
}

AABB RenderingServer::multimesh_get_aabb(RID p_multimesh)
{
	return RSG::mesh_storage->multimesh_get_aabb(p_multimesh);
}

void RenderingServer::multimesh_instance_set_color(
	RID p_multimesh, int p_index, const Color& p_color)
{
	RSG::mesh_storage->multimesh_instance_set_color(p_multimesh, p_index, p_color);
}

void RenderingServer::multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	RSG::mesh_storage->multimesh_instance_set_custom_data(p_multimesh, p_index, p_color);
}

Vector<float> RenderingServer::multimesh_get_buffer(RID p_multimesh)
{
	return RSG::mesh_storage->multimesh_get_buffer(p_multimesh);
}

void RenderingServer::multimesh_set_visible_instances(RID p_multimesh, int p_visible)
{
	RSG::mesh_storage->multimesh_set_visible_instances(p_multimesh, p_visible);
}

void RenderingServer::multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	RSG::mesh_storage->multimesh_instance_set_transform_2d(p_multimesh, p_index, p_transform);
}

Transform3D RenderingServer::multimesh_instance_get_transform(RID p_multimesh, int p_index)
{
	return RSG::mesh_storage->multimesh_instance_get_transform(p_multimesh, p_index);
}

Transform2D RenderingServer::multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
{
	return RSG::mesh_storage->multimesh_instance_get_transform_2d(p_multimesh, p_index);
}

void RenderingServer::multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	RSG::mesh_storage->multimesh_set_custom_aabb(p_multimesh, p_aabb);
}

void RenderingServer::multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
	RSG::mesh_storage->multimesh_set_buffer(p_multimesh, p_buffer);
}

void RenderingServer::multimesh_set_buffer_interpolated(
	RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev)
{
	RSG::mesh_storage->multimesh_set_buffer_interpolated(p_multimesh, p_buffer, p_buffer_prev);
}

void RenderingServer::multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	RSG::mesh_storage->multimesh_set_mesh(p_multimesh, p_mesh);
}

void RenderingServer::multimesh_set_physics_interpolation_quality(
	RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality)
{
	RSG::mesh_storage->multimesh_set_physics_interpolation_quality(p_multimesh, p_quality);
}

void RenderingServer::multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	RSG::mesh_storage->multimesh_instance_set_transform(p_multimesh, p_index, p_transform);
}

Color RenderingServer::multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	return RSG::mesh_storage->multimesh_instance_get_color(p_multimesh, p_index);
}

Color RenderingServer::multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	return RSG::mesh_storage->multimesh_instance_get_custom_data(p_multimesh, p_index);
}

void RenderingServer::multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index)
{
	RSG::mesh_storage->multimesh_instance_reset_physics_interpolation(p_multimesh, p_index);
}

void RenderingServer::multimesh_instances_reset_physics_interpolation(RID p_multimesh)
{
	RSG::mesh_storage->multimesh_instances_reset_physics_interpolation(p_multimesh);
}

void RenderingServer::multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated)
{
	RSG::mesh_storage->multimesh_set_physics_interpolated(p_multimesh, p_interpolated);
}

RID RenderingServer::skeleton_create()
{
	RID ret = RSG::mesh_storage->skeleton_allocate();
	RSG::mesh_storage->skeleton_initialize(ret);
	return ret;
}

void RenderingServer::skeleton_bone_set_transform_2d(
	RID p_skeleton, int p_bone, const Transform2D& p_transform)
{
	RSG::mesh_storage->skeleton_bone_set_transform_2d(p_skeleton, p_bone, p_transform);
}

void RenderingServer::skeleton_set_base_transform_2d(
	RID p_skeleton, const Transform2D& p_base_transform)
{
	RSG::mesh_storage->skeleton_set_base_transform_2d(p_skeleton, p_base_transform);
}

RID RenderingServer::texture_external_create(int p_width, int p_height, uint64_t p_external_buffer)
{
	RID ret = RSG::texture_storage->texture_allocate();
	RSG::texture_storage->texture_external_initialize(ret, p_width, p_height, p_external_buffer);
	return ret;
}

void RenderingServer::texture_external_update(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
	RSG::texture_storage->texture_external_update(p_texture, p_width, p_height, p_external_buffer);
}

RID RenderingServer::decal_create()
{
	RID ret = RSG::texture_storage->decal_allocate();
	RSG::texture_storage->decal_initialize(ret);
	return ret;
}

void RenderingServer::decal_set_modulate(RID p_decal, const Color& p_modulate)
{
	RSG::texture_storage->decal_set_modulate(p_decal, p_modulate);
}

void RenderingServer::decal_set_emission_energy(RID p_decal, float p_energy)
{
	RSG::texture_storage->decal_set_emission_energy(p_decal, p_energy);
}

void RenderingServer::decal_set_albedo_mix(RID p_decal, float p_mix)
{
	RSG::texture_storage->decal_set_albedo_mix(p_decal, p_mix);
}

void RenderingServer::decal_set_fade(RID p_decal, float p_above, float p_below)
{
	RSG::texture_storage->decal_set_fade(p_decal, p_above, p_below);
}

void RenderingServer::decal_set_normal_fade(RID p_decal, float p_fade)
{
	RSG::texture_storage->decal_set_normal_fade(p_decal, p_fade);
}

void RenderingServer::decal_set_distance_fade(
	RID p_decal, bool p_enabled, float p_begin, float p_length)
{
	RSG::texture_storage->decal_set_distance_fade(p_decal, p_enabled, p_begin, p_length);
}

RID RenderingServer::particles_create()
{
	RID ret = RSG::particles_storage->particles_allocate();
	RSG::particles_storage->particles_initialize(ret);
	return ret;
}

void RenderingServer::particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode)
{
	RSG::particles_storage->particles_set_mode(p_particles, p_mode);
}

void RenderingServer::particles_set_draw_passes(RID p_particles, int p_count)
{
	RSG::particles_storage->particles_set_draw_passes(p_particles, p_count);
}

void RenderingServer::particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh)
{
	RSG::particles_storage->particles_set_draw_pass_mesh(p_particles, p_pass, p_mesh);
}

void RenderingServer::particles_restart(RID p_particles)
{
	RSG::particles_storage->particles_restart(p_particles);
}

void RenderingServer::particles_set_emitting(RID p_particles, bool p_emitting)
{
	RSG::particles_storage->particles_set_emitting(p_particles, p_emitting);
}

void RenderingServer::particles_set_seed(RID p_particles, uint32_t p_seed)
{
	RSG::particles_storage->particles_set_seed(p_particles, p_seed);
}

void RenderingServer::particles_request_process_time(
	RID p_particles, real_t p_time, real_t p_interpolation)
{
	RSG::particles_storage->particles_request_process_time(p_particles, p_time, p_interpolation);
}

void RenderingServer::particles_set_one_shot(RID p_particles, bool p_one_shot)
{
	RSG::particles_storage->particles_set_one_shot(p_particles, p_one_shot);
}

void RenderingServer::particles_set_emission_transform(
	RID p_particles, const Transform3D& p_transform)
{
	RSG::particles_storage->particles_set_emission_transform(p_particles, p_transform);
}

void RenderingServer::particles_set_use_local_coordinates(RID p_particles, bool p_enable)
{
	RSG::particles_storage->particles_set_use_local_coordinates(p_particles, p_enable);
}

AABB RenderingServer::particles_get_current_aabb(RID p_particles)
{
	return RSG::particles_storage->particles_get_current_aabb(p_particles);
}

void RenderingServer::particles_emit(RID p_particles, const Transform3D& p_transform,
	const Vector3& p_velocity, const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	RSG::particles_storage->particles_emit(
		p_particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}

void RenderingServer::particles_set_amount(RID p_particles, int p_amount)
{
	RSG::particles_storage->particles_set_amount(p_particles, p_amount);
}

void RenderingServer::particles_set_amount_ratio(RID p_particles, float p_ratio)
{
	RSG::particles_storage->particles_set_amount_ratio(p_particles, p_ratio);
}

void RenderingServer::particles_set_lifetime(RID p_particles, double p_lifetime)
{
	RSG::particles_storage->particles_set_lifetime(p_particles, p_lifetime);
}

void RenderingServer::particles_set_fixed_fps(RID p_particles, int p_fps)
{
	RSG::particles_storage->particles_set_fixed_fps(p_particles, p_fps);
}

void RenderingServer::particles_set_fractional_delta(RID p_particles, bool p_enable)
{
	RSG::particles_storage->particles_set_fractional_delta(p_particles, p_enable);
}

void RenderingServer::particles_set_interpolate(RID p_particles, bool p_enable)
{
	RSG::particles_storage->particles_set_interpolate(p_particles, p_enable);
}

void RenderingServer::particles_set_pre_process_time(RID p_particles, double p_time)
{
	RSG::particles_storage->particles_set_pre_process_time(p_particles, p_time);
}

void RenderingServer::particles_set_explosiveness_ratio(RID p_particles, float p_ratio)
{
	RSG::particles_storage->particles_set_explosiveness_ratio(p_particles, p_ratio);
}

void RenderingServer::particles_set_randomness_ratio(RID p_particles, float p_ratio)
{
	RSG::particles_storage->particles_set_randomness_ratio(p_particles, p_ratio);
}

void RenderingServer::particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order)
{
	RSG::particles_storage->particles_set_draw_order(p_particles, p_order);
}

void RenderingServer::particles_set_speed_scale(RID p_particles, double p_scale)
{
	RSG::particles_storage->particles_set_speed_scale(p_particles, p_scale);
}

void RenderingServer::particles_set_collision_base_size(RID p_particles, float p_size)
{
	RSG::particles_storage->particles_set_collision_base_size(p_particles, p_size);
}

void RenderingServer::particles_set_transform_align_channel_filter(
	RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel)
{
	RSG::particles_storage->particles_set_transform_align_channel_filter(p_particles, p_channel);
}

void RenderingServer::particles_set_interp_to_end(RID p_particles, float p_interp)
{
	RSG::particles_storage->particles_set_interp_to_end(p_particles, p_interp);
}

void RenderingServer::particles_set_trails(RID p_particles, bool p_enable, float p_length_sec)
{
	RSG::particles_storage->particles_set_trails(p_particles, p_enable, p_length_sec);
}

void RenderingServer::particles_set_transform_align_axis(
	RID p_particles, RSE::ParticlesTransformAlignAxis p_axis)
{
	RSG::particles_storage->particles_set_transform_align_axis(p_particles, p_axis);
}

RID RenderingServer::particles_collision_create()
{
	RID ret = RSG::particles_storage->particles_collision_allocate();
	RSG::particles_storage->particles_collision_initialize(ret);
	return ret;
}

void RenderingServer::particles_collision_set_collision_type(
	RID p_particles_collision, RSE::ParticlesCollisionType p_type)
{
	RSG::particles_storage->particles_collision_set_collision_type(p_particles_collision, p_type);
}

void RenderingServer::particles_collision_height_field_update(RID p_particles_collision)
{
	RSG::particles_storage->particles_collision_height_field_update(p_particles_collision);
}

void RenderingServer::particles_collision_set_cull_mask(
	RID p_particles_collision, uint32_t p_cull_mask)
{
	RSG::particles_storage->particles_collision_set_cull_mask(p_particles_collision, p_cull_mask);
}

void RenderingServer::particles_collision_set_field_texture(
	RID p_particles_collision, RID p_texture)
{
	RSG::particles_storage->particles_collision_set_field_texture(p_particles_collision, p_texture);
}

void RenderingServer::particles_collision_set_height_field_mask(
	RID p_particles_collision, uint32_t p_mask)
{
	RSG::particles_storage->particles_collision_set_height_field_mask(
		p_particles_collision, p_mask);
}

void RenderingServer::particles_collision_set_attractor_strength(
	RID p_particles_collision, real_t p_strength)
{
	RSG::particles_storage->particles_collision_set_attractor_strength(
		p_particles_collision, p_strength);
}

void RenderingServer::particles_collision_set_attractor_attenuation(
	RID p_particles_collision, real_t p_attenuation)
{
	RSG::particles_storage->particles_collision_set_attractor_attenuation(
		p_particles_collision, p_attenuation);
}

RID RenderingServer::fog_volume_create()
{
	RID ret = RSG::fog->fog_volume_allocate();
	RSG::fog->fog_volume_initialize(ret);
	return ret;
}

void RenderingServer::fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape)
{
	RSG::fog->fog_volume_set_shape(p_fog_volume, p_shape);
}

RID RenderingServer::voxel_gi_create()
{
	RID ret = RSG::gi->voxel_gi_allocate();
	RSG::gi->voxel_gi_initialize(ret);
	return ret;
}

void RenderingServer::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
	const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	RSG::gi->voxel_gi_allocate_data(p_voxel_gi, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
}

Vector<uint8_t> RenderingServer::voxel_gi_get_octree_cells(RID p_voxel_gi)
{
	return RSG::gi->voxel_gi_get_octree_cells(p_voxel_gi);
}

Vector<uint8_t> RenderingServer::voxel_gi_get_data_cells(RID p_voxel_gi)
{
	return RSG::gi->voxel_gi_get_data_cells(p_voxel_gi);
}

Vector<uint8_t> RenderingServer::voxel_gi_get_distance_field(RID p_voxel_gi)
{
	return RSG::gi->voxel_gi_get_distance_field(p_voxel_gi);
}

Vector<int> RenderingServer::voxel_gi_get_level_counts(RID p_voxel_gi)
{
	return RSG::gi->voxel_gi_get_level_counts(p_voxel_gi);
}

void RenderingServer::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range)
{
	RSG::gi->voxel_gi_set_dynamic_range(p_voxel_gi, p_range);
}

void RenderingServer::voxel_gi_set_propagation(RID p_voxel_gi, float p_propagation)
{
	RSG::gi->voxel_gi_set_propagation(p_voxel_gi, p_propagation);
}

void RenderingServer::voxel_gi_set_energy(RID p_voxel_gi, float p_energy)
{
	RSG::gi->voxel_gi_set_energy(p_voxel_gi, p_energy);
}

void RenderingServer::voxel_gi_set_bias(RID p_voxel_gi, float p_bias)
{
	RSG::gi->voxel_gi_set_bias(p_voxel_gi, p_bias);
}

void RenderingServer::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_bias)
{
	RSG::gi->voxel_gi_set_normal_bias(p_voxel_gi, p_bias);
}

void RenderingServer::voxel_gi_set_interior(RID p_voxel_gi, bool p_interior)
{
	RSG::gi->voxel_gi_set_interior(p_voxel_gi, p_interior);
}

void RenderingServer::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_use_two_bounces)
{
	RSG::gi->voxel_gi_set_use_two_bounces(p_voxel_gi, p_use_two_bounces);
}

void RenderingServer::voxel_gi_set_baked_exposure_normalization(
	RID p_voxel_gi, float p_normalization)
{
	RSG::gi->voxel_gi_set_baked_exposure_normalization(p_voxel_gi, p_normalization);
}

RID RenderingServer::reflection_probe_create()
{
	RID ret = RSG::light_storage->reflection_probe_allocate();
	RSG::light_storage->reflection_probe_initialize(ret);
	return ret;
}

void RenderingServer::reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
{
	RSG::light_storage->reflection_probe_set_ambient_color(p_probe, p_color);
}

void RenderingServer::reflection_probe_set_intensity(RID p_probe, float p_intensity)
{
	RSG::light_storage->reflection_probe_set_intensity(p_probe, p_intensity);
}

void RenderingServer::reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
{
	RSG::light_storage->reflection_probe_set_ambient_energy(p_probe, p_energy);
}

void RenderingServer::reflection_probe_set_max_distance(RID p_probe, float p_distance)
{
	RSG::light_storage->reflection_probe_set_max_distance(p_probe, p_distance);
}

void RenderingServer::reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
{
	RSG::light_storage->reflection_probe_set_mesh_lod_threshold(p_probe, p_ratio);
}

void RenderingServer::reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable)
{
	RSG::light_storage->reflection_probe_set_enable_box_projection(p_probe, p_enable);
}

void RenderingServer::reflection_probe_set_as_interior(RID p_probe, bool p_enable)
{
	RSG::light_storage->reflection_probe_set_as_interior(p_probe, p_enable);
}

void RenderingServer::reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
{
	RSG::light_storage->reflection_probe_set_enable_shadows(p_probe, p_enable);
}

void RenderingServer::reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
{
	RSG::light_storage->reflection_probe_set_cull_mask(p_probe, p_layers);
}

void RenderingServer::reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
{
	RSG::light_storage->reflection_probe_set_reflection_mask(p_probe, p_layers);
}

void RenderingServer::reflection_probe_set_update_mode(
	RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
{
	RSG::light_storage->reflection_probe_set_update_mode(p_probe, p_mode);
}


