/**************************************************************************/
/*  gles3_effects.cpp                                                     */
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

#include "gles3_effects.h"

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
