/**************************************************************************/
/*  gles3_effects.h                                                       */
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

#pragma once

#ifdef GLES3_ENABLED

#include "drivers/gles3/shaders/effects/copy.glsl.gen.h"
#include "drivers/gles3/shaders/effects/cubemap_filter.glsl.gen.h"
#include "drivers/gles3/shaders/effects/glow.glsl.gen.h"
#include "drivers/gles3/shaders/effects/post.glsl.gen.h"
#include "drivers/gles3/shaders/feed.glsl.gen.h"

namespace GLES3
{

class CopyEffects final
{
public:
	CopyEffects() = delete;
	~CopyEffects() = delete;
	CopyEffects(const CopyEffects&) = delete;
	CopyEffects& operator=(const CopyEffects&) = delete;

private:
	struct Copy
	{
		CopyShaderGLES3 shader;
		RID shader_version;
	};

	struct Data
	{
		Copy copy;

		// Use for full-screen effects. Slightly more efficient than screen_quad as this eliminates
		// pixel overdraw along the diagonal.
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;

		// Use for rect-based effects.
		GLuint quad = 0;
		GLuint quad_array = 0;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	// These functions assume that a framebuffer and texture are bound already. They only manage the
	// shader, uniforms, and vertex array.
	static void copy_to_rect(const Rect2& p_rect, bool p_linear_to_srgb = false);
	static void copy_to_rect_3d(const Rect2& p_rect, float p_layer, int p_type, float p_lod = 0.0f,
		bool p_linear_to_srgb = false);
	static void copy_with_lens_distortion(const Rect2& p_rect, float p_layer,
		const Vector2& p_eye_center, float p_k1, float p_k2, float p_upscale,
		float p_aspect_ration, bool p_linear_to_srgb = false);
	static void copy_to_and_from_rect(const Rect2& p_rect);
	static void copy_screen(float p_multiply = 1.0);
	static void copy_with_exposure(float p_exposure, float p_multiply);
	static void copy_cube_to_rect(const Rect2& p_rect);
	static void copy_cube_to_panorama(float p_mip_level);
	static void bilinear_blur(GLuint p_source_texture, int p_mipmap_count, const Rect2i& p_region);
	static void gaussian_blur(
		GLuint p_source_texture, int p_mipmap_count, const Rect2i& p_region, const Size2i& p_size);
	static void set_color(const Color& p_color, const Rect2i& p_region);
	static void draw_screen_triangle();
	static void draw_screen_quad();
};

class CubemapFilter final
{
public:
	CubemapFilter() = delete;
	~CubemapFilter() = delete;
	CubemapFilter(const CubemapFilter&) = delete;
	CubemapFilter& operator=(const CubemapFilter&) = delete;

private:
	struct CMF
	{
		CubemapFilterShaderGLES3 shader;
		RID shader_version;
	};

	struct Data
	{
		CMF cubemap_filter;

		// Use for full-screen effects. Slightly more efficient than screen_quad as this eliminates
		// pixel overdraw along the diagonal.
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;

		uint32_t ggx_samples = 128;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static void filter_radiance(GLuint p_source_cubemap, GLuint p_dest_cubemap,
		GLuint p_dest_framebuffer, int p_source_size, int p_mipmap_count, int p_layer);
};

class FeedEffects final
{
public:
	FeedEffects() = delete;
	~FeedEffects() = delete;
	FeedEffects(const FeedEffects&) = delete;
	FeedEffects& operator=(const FeedEffects&) = delete;

private:
	struct Feed
	{
		FeedShaderGLES3 shader;
		RID shader_version;
	};

	struct Data
	{
		Feed feed;
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;
	};

	static inline Data* data = nullptr;

	static void draw_screen_triangle();

public:
	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static void draw();
};

class Glow final
{
public:
	Glow() = delete;
	~Glow() = delete;
	Glow(const Glow&) = delete;
	Glow& operator=(const Glow&) = delete;

	struct Level
	{
		Size2i size;
		GLuint color = 0;
		GLuint fbo = 0;
	};

private:
	struct GlowShader
	{
		GlowShaderGLES3 shader;
		RID shader_version;
	};

	struct Data
	{
		GlowShader glow;

		float luminance_multiplier = 1.0;

		float glow_intensity = 1.0;
		float glow_bloom = 0.0;
		float glow_hdr_bleed_threshold = 1.0;
		float glow_hdr_bleed_scale = 2.0;
		float glow_hdr_luminance_cap = 12.0;

		// Use for full-screen effects. Slightly more efficient than screen_quad as this eliminates
		// pixel overdraw along the diagonal.
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;
	};

	static inline Data* data = nullptr;

	static void _draw_screen_triangle();

public:
	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static void set_intensity(float p_value)
	{
		if (data) {
			data->glow_intensity = p_value;
		}
	}

	static void set_luminance_multiplier(float p_luminance_multiplier)
	{
		if (data) {
			data->luminance_multiplier = p_luminance_multiplier;
		}
	}

	static void set_glow_bloom(float p_bloom)
	{
		if (data) {
			data->glow_bloom = p_bloom;
		}
	}

	static void set_glow_hdr_bleed_threshold(float p_threshold)
	{
		if (data) {
			data->glow_hdr_bleed_threshold = p_threshold;
		}
	}

	static void set_glow_hdr_bleed_scale(float p_scale)
	{
		if (data) {
			data->glow_hdr_bleed_scale = p_scale;
		}
	}

	static void set_glow_hdr_luminance_cap(float p_cap)
	{
		if (data) {
			data->glow_hdr_luminance_cap = p_cap;
		}
	}

	static void process_glow(GLuint p_source_color, Size2i p_size, const Level* p_glow_buffers,
		uint32_t p_view = 0, bool p_use_multiview = false);
};

class PostEffects final
{
public:
	PostEffects() = delete;
	~PostEffects() = delete;
	PostEffects(const PostEffects&) = delete;
	PostEffects& operator=(const PostEffects&) = delete;

private:
	struct Post
	{
		PostShaderGLES3 shader;
		RID shader_version;
	};

	struct Data
	{
		Post post;

		// Use for full-screen effects. Slightly more efficient than screen_quad as this eliminates
		// pixel overdraw along the diagonal.
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;
	};

	static inline Data* data = nullptr;

	static void _draw_screen_triangle();

public:
	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static void post_copy(GLuint p_dest_framebuffer, Size2i p_dest_size, GLuint p_source_color,
		GLuint p_source_depth, bool p_ssao_enabled, int p_ssao_quality_level, float p_ssao_strength,
		float p_ssao_radius, Size2i p_source_size, float p_luminance_multiplier,
		const Glow::Level* p_glow_buffers, float p_glow_intensity, float p_srgb_white,
		uint32_t p_view = 0, bool p_use_multiview = false, uint64_t p_spec_constants = 0,
		bool p_bilinear_filtering = true);
};

} // namespace GLES3

#endif // GLES3_ENABLED
