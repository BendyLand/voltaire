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
#include "rendering_server.compat.inc"
#include "rendering_server.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_warnings.h"

RenderingServer* RenderingServer::singleton = nullptr;
RenderingServer* (*RenderingServer::create_func)() = nullptr;

RenderingServer* RenderingServer::get_singleton() { return singleton; }

RenderingServer* RenderingServer::create()
{
	ERR_FAIL_COND_V(singleton, nullptr);

	if (create_func) {
		return create_func();
	}

	return nullptr;
}

RID RenderingServer::get_test_texture()
{
	if (test_texture.is_valid()) {
		return test_texture;
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

	Ref<Image> data =
		memnew(Image(TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, false, Image::FORMAT_RGB8, test_data));

	test_texture = texture_2d_create(data);

	return test_texture;
}

void RenderingServer::_free_internal_rids()
{
	if (test_texture.is_valid()) {
		free_rid(test_texture);
	}
	if (white_texture.is_valid()) {
		free_rid(white_texture);
	}
	if (test_material.is_valid()) {
		free_rid(test_material);
	}
}

RID RenderingServer::get_white_texture()
{
	if (white_texture.is_valid()) {
		return white_texture;
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
	white_texture = texture_2d_create(white);
	return white_texture;
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
	uint32_t p_format, int p_vertex_len, int p_array_index) const
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

uint32_t RenderingServer::mesh_surface_get_format_vertex_stride(
	uint32_t p_format, int p_vertex_len) const
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
	uint32_t p_format, int p_vertex_len) const
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
	uint32_t p_format, int p_vertex_len) const
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

uint32_t RenderingServer::mesh_surface_get_format_skin_stride(
	uint32_t p_format, int p_vertex_len) const
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

uint32_t RenderingServer::mesh_surface_get_format_index_stride(
	uint32_t p_format, int p_vertex_len) const
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
	uint32_t& r_normal_element_size, uint32_t& r_attrib_element_size,
	uint32_t& r_skin_element_size) const
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

RenderingDevice* RenderingServer::get_rendering_device() const
{
	// Return the rendering device we're using globally.
	return RenderingDevice::get_singleton();
}

RenderingDevice* RenderingServer::create_local_rendering_device() const
{
	RenderingDevice* device = RenderingDevice::get_singleton();
	if (!device) {
		return nullptr;
	}
	return device->create_local_device();
}

String RenderingServer::get_current_rendering_driver_name() const
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here.
	return ::OS::get_singleton()->get_current_rendering_driver_name();
}

String RenderingServer::get_current_rendering_method() const
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

bool RenderingServer::is_render_loop_enabled() const { return render_loop_enabled; }

void RenderingServer::set_render_loop_enabled(bool p_enabled) { render_loop_enabled = p_enabled; }

RenderingServer::RenderingServer()
{
	// ERR_FAIL_COND(singleton);

	singleton = this;
}

RenderingServer::~RenderingServer() { singleton = nullptr; }


