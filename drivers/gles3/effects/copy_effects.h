/**************************************************************************/
/*  copy_effects.h                                                        */
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

} // namespace GLES3

#endif // GLES3_ENABLED
