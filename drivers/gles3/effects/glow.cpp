/**************************************************************************/
/*  glow.cpp                                                              */
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

#include "glow.h"

#ifdef GLES3_ENABLED

#include "drivers/gles3/storage/texture_storage.h"

using namespace GLES3;

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

#endif // GLES3_ENABLED
