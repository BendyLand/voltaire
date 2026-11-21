/**************************************************************************/
/*  utilities.cpp                                                         */
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

#include "utilities.h"

#include <platform_gl.h>

// RasterizerUtilGLES3

bool RasterizerUtilGLES3::gles_over_gl = true;

void RasterizerUtilGLES3::clear_depth(float p_depth)
{
#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		glClearDepth(p_depth);
	}
#endif // GL_API_ENABLED
#ifdef GLES_API_ENABLED
	if (!RasterizerUtilGLES3::is_gles_over_gl()) {
		glClearDepthf(p_depth);
	}
#endif // GLES_API_ENABLED
}

void RasterizerUtilGLES3::clear_stencil(int32_t p_stencil)
{
	glClearStencil(p_stencil);
}

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/light_storage.h"
#include "drivers/gles3/storage/material_storage.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/storage/particles_storage.h"
#include "drivers/gles3/storage/texture_storage.h"

#ifdef ANDROID_ENABLED
#include <platform_egl.h>
#endif

#ifdef WEB_ENABLED
#include <emscripten/html5_webgl.h>
#endif

using namespace GLES3;

// GLES3::Config

#define _GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

void Config::initialize()
{
	if (_initialized) {
		return;
	}
	_initialized = true;

	extensions.clear();
	polyfill_half2float = true;
	disable_particles_workaround = false;
	disable_transform_feedback_shader_cache = false;

#ifdef WEB_ENABLED
	// Starting with Emscripten 3.1.51, glGetStringi(GL_EXTENSIONS, i) will only ever return
	// a fixed list of extensions, regardless of what additional extensions are enabled. This
	// isn't very useful for us in determining which extensions we can rely on here. So, instead
	// we use emscripten_webgl_get_supported_extensions() to get all supported extensions, which
	// is what Emscripten 3.1.50 and earlier do.
	{
		char* extension_array_string = emscripten_webgl_get_supported_extensions();
		PackedStringArray extension_array = String((const char*)extension_array_string).split(" ");
		extensions.reserve(extension_array.size() * 2);
		for (const String& s : extension_array) {
			extensions.insert(s);
			extensions.insert("GL_" + s);
		}
		free(extension_array_string);
	}
#else
	{
		GLint max_extensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max_extensions);
		for (int i = 0; i < max_extensions; i++) {
			const GLubyte* s = glGetStringi(GL_EXTENSIONS, i);
			if (!s) {
				break;
			}
			extensions.insert((const char*)s);
		}
	}
#endif

	bptc_supported = extensions.has("GL_ARB_texture_compression_bptc") ||
					 extensions.has("GL_EXT_texture_compression_bptc");
	astc_3d_supported = extensions.has("GL_OES_texture_compression_astc");
	astc_hdr_supported = astc_3d_supported || extensions.has("GL_KHR_texture_compression_astc_hdr");
	astc_layered_supported =
		astc_hdr_supported || extensions.has("GL_KHR_texture_compression_astc_sliced_3d");
	astc_supported = astc_layered_supported ||
					 extensions.has("GL_KHR_texture_compression_astc_ldr") ||
					 extensions.has("WEBGL_compressed_texture_astc");

	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		float_texture_supported = true;
		float_texture_linear_supported = true;
		etc2_supported = false;
		s3tc_supported = true;
		rgtc_supported = true; // RGTC - core since OpenGL version 3.0
		srgb_framebuffer_supported = true;
		unorm16_texture_supported = true;
	}
	else {
		float_texture_supported = extensions.has("GL_EXT_color_buffer_float");
		float_texture_linear_supported = extensions.has("GL_OES_texture_float_linear");
		etc2_supported = true;
#if defined(ANDROID_ENABLED) || defined(IOS_ENABLED)
		s3tc_supported = false;
#else
		s3tc_supported = extensions.has("GL_EXT_texture_compression_dxt1") ||
						 extensions.has("GL_EXT_texture_compression_s3tc") ||
						 extensions.has("WEBGL_compressed_texture_s3tc");
#endif
		rgtc_supported = extensions.has("GL_EXT_texture_compression_rgtc") ||
						 extensions.has("GL_ARB_texture_compression_rgtc");
		srgb_framebuffer_supported = extensions.has("GL_EXT_sRGB_write_control");
		unorm16_texture_supported = extensions.has("GL_EXT_texture_norm16");
	}

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_vertex_texture_image_units);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_image_units);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport_size);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
	glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_buffer_size);
	GLint max_vertex_output;
	glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &max_vertex_output);
	GLint max_fragment_input;
	glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &max_fragment_input);
	max_shader_varyings = (uint32_t)MIN(max_vertex_output, max_fragment_input) / 4;

	// sanity clamp buffer size to 16K..1MB
	max_uniform_buffer_size = CLAMP(max_uniform_buffer_size, 16384, 1048576);

	support_anisotropic_filter = extensions.has("GL_EXT_texture_filter_anisotropic");
	if (support_anisotropic_filter) {
		glGetFloatv(_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropic_level);
	}

	glGetIntegerv(GL_MAX_SAMPLES, &msaa_max_samples);
#ifdef WEB_ENABLED
	msaa_supported = (msaa_max_samples > 0);
#else
	msaa_supported = true;
#endif
#ifndef IOS_ENABLED
#ifdef WEB_ENABLED
	msaa_multiview_supported = extensions.has("OCULUS_multiview");
	rt_msaa_multiview_supported = msaa_multiview_supported;
#else
	msaa_multiview_supported = extensions.has("GL_EXT_multiview_texture_multisample");
#endif

	multiview_supported = extensions.has("OCULUS_multiview") ||
						  extensions.has("GL_OVR_multiview2") || extensions.has("GL_OVR_multiview");
#endif

#ifdef ANDROID_ENABLED
	// These are GLES only
	rt_msaa_supported = extensions.has("GL_EXT_multisampled_render_to_texture");
	rt_msaa_multiview_supported = extensions.has("GL_OVR_multiview_multisampled_render_to_texture");
	external_texture_supported = extensions.has("GL_OES_EGL_image_external_essl3");

	if (multiview_supported) {
		eglFramebufferTextureMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
				"glFramebufferTextureMultiviewOVR");
		if (eglFramebufferTextureMultiviewOVR == nullptr) {
			multiview_supported = false;
		}
	}

	if (msaa_multiview_supported) {
		eglTexStorage3DMultisample =
			(PFNGLTEXSTORAGE3DMULTISAMPLEPROC)eglGetProcAddress("glTexStorage3DMultisample");
		if (eglTexStorage3DMultisample == nullptr) {
			msaa_multiview_supported = false;
		}
	}

	if (rt_msaa_supported) {
		eglFramebufferTexture2DMultisampleEXT =
			(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress(
				"glFramebufferTexture2DMultisampleEXT");
		if (eglFramebufferTexture2DMultisampleEXT == nullptr) {
			rt_msaa_supported = false;
		}
	}

	if (rt_msaa_multiview_supported) {
		eglFramebufferTextureMultisampleMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)eglGetProcAddress(
				"glFramebufferTextureMultisampleMultiviewOVR");
		if (eglFramebufferTextureMultisampleMultiviewOVR == nullptr) {
			rt_msaa_multiview_supported = false;
		}
	}

	if (external_texture_supported) {
		eglEGLImageTargetTexture2DOES =
			(PFNEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
		if (eglEGLImageTargetTexture2DOES == nullptr) {
			external_texture_supported = false;
		}
	}
#endif

	// Adreno 3xx Compatibility
	const String rendering_device_name = String::utf8((const char*)glGetString(GL_RENDERER));
	if (rendering_device_name.left(13) == "Adreno (TM) 3") {
		disable_particles_workaround = true;

		// ignore driver version 331+
		const String gl_version = String::utf8((const char*)glGetString(GL_VERSION));
		String driver_version = gl_version.get_slice("V@", 1).get_slicec(' ', 0);
		if (driver_version.is_valid_float() && driver_version.to_float() >= 331.0) {
			// disable_particles_workaround = false;
		}
	}
	else if (rendering_device_name.contains("PowerVR")) {
		disable_transform_feedback_shader_cache = true;
	}

	if (OS::get_current_rendering_driver_name() == "opengl3_angle") {
		polyfill_half2float = false;
	}
#ifdef WEB_ENABLED
	polyfill_half2float = false;
#endif
}

void Config::finalize()
{
	if (!_initialized) {
		return;
	}

	extensions.reset();
	_initialized = false;
}

// RenderSceneBuffersGLES3

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#endif

RenderSceneBuffersGLES3::RenderSceneBuffersGLES3()
{
	for (int i = 0; i < 4; i++) {
		glow.levels[i].color = 0;
		glow.levels[i].fbo = 0;
	}
}

RenderSceneBuffersGLES3::~RenderSceneBuffersGLES3() { free_render_buffer_data(); }

void RenderSceneBuffersGLES3::_rt_attach_textures(GLuint p_color, GLuint p_depth, GLsizei p_samples,
	uint32_t p_view_count, bool p_depth_has_stencil)
{
	if (p_view_count > 1) {
		if (p_samples > 1) {
#if defined(ANDROID_ENABLED) || defined(WEB_ENABLED)
			glFramebufferTextureMultisampleMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, p_color, 0, p_samples, 0, p_view_count);
			glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER,
				p_depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, p_depth, 0,
				p_samples, 0, p_view_count);
#else
			ERR_PRINT_ONCE("Multiview MSAA isn't supported on this platform.");
#endif
		}
		else {
#ifndef IOS_ENABLED
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, p_color, 0, 0, p_view_count);
			glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER,
				p_depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, p_depth, 0,
				0, p_view_count);
#else
			ERR_PRINT_ONCE("Multiview isn't supported on this platform.");
#endif
		}
	}
	else {
		if (p_samples > 1) {
#ifdef ANDROID_ENABLED
			glFramebufferTexture2DMultisampleEXT(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_color, 0, p_samples);
			glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER,
				p_depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_2D, p_depth, 0, p_samples);
#else
			ERR_PRINT_ONCE(
				"MSAA via EXT_multisampled_render_to_texture isn't supported on this platform.");
#endif
		}
		else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_color, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER,
				p_depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_2D, p_depth, 0);
		}
	}
}

GLuint RenderSceneBuffersGLES3::_rt_get_cached_fbo(
	GLuint p_color, GLuint p_depth, GLsizei p_samples, uint32_t p_view_count)
{
	FBDEF new_fbo;

#if defined(ANDROID_ENABLED) || defined(WEB_ENABLED)
	// There shouldn't be more then 3 entries in this...
	for (const FBDEF& cached_fbo : msaa3d.cached_fbos) {
		if (cached_fbo.color == p_color && cached_fbo.depth == p_depth) {
			return cached_fbo.fbo;
		}
	}

	new_fbo.color = p_color;
	new_fbo.depth = p_depth;

	glGenFramebuffers(1, &new_fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, new_fbo.fbo);

	_rt_attach_textures(p_color, p_depth, p_samples, p_view_count, true);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		WARN_PRINT("Could not create 3D MSAA framebuffer, status: " +
				   GLES3::TextureStorage::get_framebuffer_error(status));

		glDeleteFramebuffers(1, &new_fbo.fbo);

		new_fbo.fbo = 0;
	}
	else {
		// cache it!
		msaa3d.cached_fbos.push_back(new_fbo);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
#endif

	return new_fbo.fbo;
}

void RenderSceneBuffersGLES3::configure(const RenderSceneBuffersConfiguration* p_config)
{
	free_render_buffer_data();

	internal_size = p_config->get_internal_size();
	target_size = p_config->get_target_size();
	scaling_3d_mode = p_config->get_scaling_3d_mode();
	render_target = p_config->get_render_target();
	msaa3d.mode = p_config->get_msaa_3d();
	view_count = GLES3::Config::multiview_supported ? p_config->get_view_count() : 1;

	bool use_multiview = view_count > 1;

	// Get color format data from our render target so we match those
	if (render_target.is_valid()) {
		color_internal_format =
			GLES3::TextureStorage::render_target_get_color_internal_format(render_target);
		color_format = GLES3::TextureStorage::render_target_get_color_format(render_target);
		color_type = GLES3::TextureStorage::render_target_get_color_type(render_target);
		color_format_size = GLES3::TextureStorage::render_target_get_color_format_size(render_target);
	}
	else {
		// reflection probe? or error?
		color_internal_format = GL_RGBA8;
		color_format = GL_RGBA;
		color_type = GL_UNSIGNED_BYTE;
		color_format_size = 4;
	}

	// Check our scaling mode
	if (scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF && internal_size.x == 0 &&
		internal_size.y == 0) {
		// Disable, no size set.
		scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	}
	else if (scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF &&
			   internal_size == target_size) {
		// If size matches, we won't use scaling.
		scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	}
	else if (scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF &&
			   !(scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR ||
				   scaling_3d_mode == RSE::VIEWPORT_SCALING_3D_MODE_NEAREST)) {
		WARN_PRINT_ONCE("The Compatibility rendering method only supports bilinear and "
						"nearest-neighbor scaling. Falling back to bilinear scaling.");
		scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
	}

	// Check if we support MSAA.
	if (msaa3d.mode != RSE::VIEWPORT_MSAA_DISABLED && internal_size.x == 0 &&
		internal_size.y == 0) {
		// Disable, no size set.
		msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
	}
	else if (!use_multiview && msaa3d.mode != RSE::VIEWPORT_MSAA_DISABLED &&
			   !GLES3::Config::msaa_supported && !GLES3::Config::rt_msaa_supported) {
		WARN_PRINT_ONCE("MSAA is not supported on this device.");
		msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
	}
	else if (use_multiview && msaa3d.mode != RSE::VIEWPORT_MSAA_DISABLED &&
			   !GLES3::Config::msaa_multiview_supported &&
			   !GLES3::Config::rt_msaa_multiview_supported) {
		WARN_PRINT_ONCE("Multiview MSAA is not supported on this device.");
		msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
	}
}

void RenderSceneBuffersGLES3::_check_render_buffers()
{
	ERR_FAIL_COND(view_count == 0);

	bool use_internal_buffer = scaling_3d_mode != RSE::VIEWPORT_SCALING_3D_MODE_OFF ||
							   apply_environment_effects_in_post || apply_canvas_bg_exposure;
	GLenum depth_format = GL_DEPTH24_STENCIL8;
	uint32_t depth_format_size = 4;
	bool use_multiview = view_count > 1;

	if (!use_internal_buffer && internal3d.color != 0) {
		_clear_intermediate_buffers();
	}

	if ((!use_internal_buffer || internal3d.color != 0) &&
		(msaa3d.mode == RSE::VIEWPORT_MSAA_DISABLED || msaa3d.color != 0)) {
		// already setup!
		return;
	}

	if (use_internal_buffer && internal3d.color == 0) {
		// Setup our internal buffer.
		GLenum texture_target = use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

		// Create our color buffer.
		glGenTextures(1, &internal3d.color);
		glBindTexture(texture_target, internal3d.color);

		if (use_multiview) {
			glTexImage3D(texture_target, 0, color_internal_format, internal_size.x, internal_size.y,
				view_count, 0, color_format, color_type, nullptr);
		}
		else {
			glTexImage2D(texture_target, 0, color_internal_format, internal_size.x, internal_size.y,
				0, color_format, color_type, nullptr);
		}

		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLES3::Utilities::texture_allocated_data(internal3d.color,
			internal_size.x * internal_size.y * view_count * color_format_size, "3D color texture");

		// Create our depth buffer.
		glGenTextures(1, &internal3d.depth);
		glBindTexture(texture_target, internal3d.depth);

		if (use_multiview) {
			glTexImage3D(texture_target, 0, depth_format, internal_size.x, internal_size.y,
				view_count, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		}
		else {
			glTexImage2D(texture_target, 0, depth_format, internal_size.x, internal_size.y, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		}

		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLES3::Utilities::texture_allocated_data(internal3d.depth,
			internal_size.x * internal_size.y * view_count * depth_format_size, "3D depth texture");

		// Create our internal 3D FBO.
		glGenFramebuffers(1, &internal3d.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, internal3d.fbo);

#ifndef IOS_ENABLED
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, internal3d.color, 0, 0, view_count);
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, internal3d.depth, 0, 0, view_count);
		}
		else {
#else
		{
#endif
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, internal3d.color, 0);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texture_target, internal3d.depth, 0);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			_clear_intermediate_buffers();
			WARN_PRINT("Could not create 3D internal buffers, status: " +
					   GLES3::TextureStorage::get_framebuffer_error(status));
		}

		glBindTexture(texture_target, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	}

	if (msaa3d.mode != RSE::VIEWPORT_MSAA_DISABLED && msaa3d.color == 0) {
		// Setup MSAA.
		const GLsizei samples[] = {1, 2, 4, 8};
		msaa3d.samples = samples[msaa3d.mode];

		// Constrain by limits of OpenGL driver.
		if (msaa3d.samples > GLES3::Config::msaa_max_samples) {
			msaa3d.samples = GLES3::Config::msaa_max_samples;
		}

		if (!use_multiview && !GLES3::Config::rt_msaa_supported) {
			msaa3d.needs_resolve = true;
			msaa3d.check_fbo_cache = false;

			// Create our color buffer.
			glGenRenderbuffers(1, &msaa3d.color);
			glBindRenderbuffer(GL_RENDERBUFFER, msaa3d.color);

			glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa3d.samples, color_internal_format,
				internal_size.x, internal_size.y);
			GLES3::Utilities::render_buffer_allocated_data(msaa3d.color,
				internal_size.x * internal_size.y * view_count * 4 * msaa3d.samples,
				"MSAA 3D color render buffer");

			// Create our depth buffer.
			glGenRenderbuffers(1, &msaa3d.depth);
			glBindRenderbuffer(GL_RENDERBUFFER, msaa3d.depth);

			glRenderbufferStorageMultisample(
				GL_RENDERBUFFER, msaa3d.samples, depth_format, internal_size.x, internal_size.y);
			GLES3::Utilities::render_buffer_allocated_data(msaa3d.depth,
				internal_size.x * internal_size.y * view_count * depth_format_size * msaa3d.samples,
				"MSAA 3D depth render buffer");

			// Create our MSAA 3D FBO.
			glGenFramebuffers(1, &msaa3d.fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, msaa3d.fbo);

			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa3d.color);
			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa3d.depth);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				_clear_msaa3d_buffers();
				msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
				WARN_PRINT("Could not create 3D MSAA buffers, status: " +
						   GLES3::TextureStorage::get_framebuffer_error(status));
			}

			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
#if !defined(IOS_ENABLED) && !defined(WEB_ENABLED)
		}
		else if (use_multiview && !GLES3::Config::rt_msaa_multiview_supported) {
			msaa3d.needs_resolve = true;
			msaa3d.check_fbo_cache = false;

			// Create our color buffer.
			glGenTextures(1, &msaa3d.color);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.color);

#ifdef ANDROID_ENABLED
			glTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.samples,
				color_internal_format, internal_size.x, internal_size.y, view_count, GL_TRUE);
#else
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.samples,
				color_internal_format, internal_size.x, internal_size.y, view_count, GL_TRUE);
#endif

			GLES3::Utilities::texture_allocated_data(msaa3d.color,
				internal_size.x * internal_size.y * view_count * color_format_size * msaa3d.samples,
				"MSAA 3D color texture");

			// Create our depth buffer.
			glGenTextures(1, &msaa3d.depth);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.depth);

#ifdef ANDROID_ENABLED
			glTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.samples, depth_format,
				internal_size.x, internal_size.y, view_count, GL_TRUE);
#else
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, msaa3d.samples, depth_format,
				internal_size.x, internal_size.y, view_count, GL_TRUE);
#endif

			GLES3::Utilities::texture_allocated_data(msaa3d.depth,
				internal_size.x * internal_size.y * view_count * depth_format_size * msaa3d.samples,
				"MSAA 3D depth texture");

			// Create our MSAA 3D FBO.
			glGenFramebuffers(1, &msaa3d.fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, msaa3d.fbo);

			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, msaa3d.color, 0, 0, view_count);
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, msaa3d.depth, 0, 0, view_count);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				_clear_msaa3d_buffers();
				msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
				WARN_PRINT("Could not create 3D MSAA buffers, status: " +
						   GLES3::TextureStorage::get_framebuffer_error(status));
			}

			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
#endif
#if defined(ANDROID_ENABLED) || defined(WEB_ENABLED) // Only supported on OpenGLES!
		}
		else if (!use_internal_buffer) {
			msaa3d.needs_resolve = false;
			msaa3d.check_fbo_cache = true;
		}
		else if (use_internal_buffer) {
			msaa3d.needs_resolve = false;
			msaa3d.check_fbo_cache = false;

			glGenFramebuffers(1, &msaa3d.fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, msaa3d.fbo);

			_rt_attach_textures(
				internal3d.color, internal3d.depth, msaa3d.samples, view_count, true);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				_clear_msaa3d_buffers();
				msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
				WARN_PRINT("Could not create 3D MSAA framebuffer, status: " +
						   GLES3::TextureStorage::get_framebuffer_error(status));
			}

			glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
#endif
		}
		else {
			WARN_PRINT_ONCE("MSAA is not supported on this device.");
			msaa3d.mode = RSE::VIEWPORT_MSAA_DISABLED;
			msaa3d.samples = 1;
			msaa3d.check_fbo_cache = false;
		}
	}
	else {
		msaa3d.samples = 1;
		msaa3d.check_fbo_cache = false;
	}
}

void RenderSceneBuffersGLES3::configure_for_probe(Size2i p_size)
{
	internal_size = p_size;
	target_size = p_size;
	scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	view_count = 1;
}

void RenderSceneBuffersGLES3::_clear_msaa3d_buffers()
{
	for (const FBDEF& cached_fbo : msaa3d.cached_fbos) {
		GLuint fbo = cached_fbo.fbo;
		glDeleteFramebuffers(1, &fbo);
	}
	msaa3d.cached_fbos.clear();

	if (msaa3d.fbo) {
		glDeleteFramebuffers(1, &msaa3d.fbo);
		msaa3d.fbo = 0;
	}

	if (msaa3d.color != 0) {
		if (view_count == 1) {
			GLES3::Utilities::render_buffer_free_data(msaa3d.color);
		}
		else {
			GLES3::Utilities::texture_free_data(msaa3d.color);
		}
		msaa3d.color = 0;
	}

	if (msaa3d.depth != 0) {
		if (view_count == 1) {
			GLES3::Utilities::render_buffer_free_data(msaa3d.depth);
		}
		else {
			GLES3::Utilities::texture_free_data(msaa3d.depth);
		}
		msaa3d.depth = 0;
	}
}

void RenderSceneBuffersGLES3::_clear_intermediate_buffers()
{
	if (internal3d.fbo) {
		glDeleteFramebuffers(1, &internal3d.fbo);
		internal3d.fbo = 0;
	}

	if (internal3d.color != 0) {
		GLES3::Utilities::texture_free_data(internal3d.color);
		internal3d.color = 0;
	}

	if (internal3d.depth != 0) {
		GLES3::Utilities::texture_free_data(internal3d.depth);
		internal3d.depth = 0;
	}
}

void RenderSceneBuffersGLES3::check_backbuffer(bool p_need_color, bool p_need_depth)
{
	// Setup our back buffer
	if (backbuffer3d.fbo == 0) {
		glGenFramebuffers(1, &backbuffer3d.fbo);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, backbuffer3d.fbo);

	bool use_multiview = view_count > 1 && GLES3::Config::multiview_supported;
	GLenum texture_target = use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	GLenum depth_format = GL_DEPTH24_STENCIL8;
	uint32_t depth_format_size = 4;

	if (backbuffer3d.color == 0 && p_need_color) {
		glGenTextures(1, &backbuffer3d.color);
		glBindTexture(texture_target, backbuffer3d.color);

		if (use_multiview) {
			glTexImage3D(texture_target, 0, color_internal_format, internal_size.x, internal_size.y,
				view_count, 0, color_format, color_type, nullptr);
		}
		else {
			glTexImage2D(texture_target, 0, color_internal_format, internal_size.x, internal_size.y,
				0, color_format, color_type, nullptr);
		}

		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLES3::Utilities::texture_allocated_data(backbuffer3d.color,
			internal_size.x * internal_size.y * view_count * color_format_size,
			"3D Back buffer color texture");

#ifndef IOS_ENABLED
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, backbuffer3d.color, 0, 0, view_count);
		}
		else {
#else
		{
#endif
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, backbuffer3d.color, 0);
		}
	}

	if (backbuffer3d.depth == 0 && p_need_depth) {
		glGenTextures(1, &backbuffer3d.depth);
		glBindTexture(texture_target, backbuffer3d.depth);

		if (use_multiview) {
			glTexImage3D(texture_target, 0, depth_format, internal_size.x, internal_size.y,
				view_count, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		}
		else {
			glTexImage2D(texture_target, 0, depth_format, internal_size.x, internal_size.y, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		}

		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLES3::Utilities::texture_allocated_data(backbuffer3d.depth,
			internal_size.x * internal_size.y * view_count * depth_format_size,
			"3D back buffer depth texture");

#ifndef IOS_ENABLED
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, backbuffer3d.depth, 0, 0, view_count);
		}
		else {
#else
		{
#endif
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texture_target, backbuffer3d.depth, 0);
		}
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		_clear_back_buffers();
		WARN_PRINT("Could not create 3D back buffers, status: " +
				   GLES3::TextureStorage::get_framebuffer_error(status));
	}

	glBindTexture(texture_target, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

void RenderSceneBuffersGLES3::_clear_back_buffers()
{
	if (backbuffer3d.fbo) {
		glDeleteFramebuffers(1, &backbuffer3d.fbo);
		backbuffer3d.fbo = 0;
	}

	if (backbuffer3d.color != 0) {
		GLES3::Utilities::texture_free_data(backbuffer3d.color);
		backbuffer3d.color = 0;
	}

	if (backbuffer3d.depth != 0) {
		GLES3::Utilities::texture_free_data(backbuffer3d.depth);
		backbuffer3d.depth = 0;
	}
}

void RenderSceneBuffersGLES3::set_apply_environment_effects_in_post(bool p_apply_in_post)
{
	apply_environment_effects_in_post = p_apply_in_post;
}

void RenderSceneBuffersGLES3::set_apply_canvas_bg_exposure(bool p_apply_canvas_bg_exposure)
{
	apply_canvas_bg_exposure = p_apply_canvas_bg_exposure;
}

void RenderSceneBuffersGLES3::check_glow_buffers()
{
	if (glow.levels[0].color != 0) {
		// already have these setup..
		return;
	}

	Size2i level_size = internal_size;
	for (int i = 0; i < 4; i++) {
		level_size = Size2i(level_size.x >> 1, level_size.y >> 1).maxi(4);

		glow.levels[i].size = level_size;

		// Create our texture
		glGenTextures(1, &glow.levels[i].color);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, glow.levels[i].color);

		glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, level_size.x, level_size.y, 0,
			color_format, color_type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		GLES3::Utilities::texture_allocated_data(glow.levels[i].color,
			level_size.x * level_size.y * color_format_size,
			String("Glow buffer ") + String::num_int64(i));

		// Create our FBO
		glGenFramebuffers(1, &glow.levels[i].fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, glow.levels[i].fbo);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glow.levels[i].color, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			WARN_PRINT("Could not create glow buffers, status: " +
					   GLES3::TextureStorage::get_framebuffer_error(status));
			_clear_glow_buffers();
			break;
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

void RenderSceneBuffersGLES3::_clear_glow_buffers()
{
	for (int i = 0; i < 4; i++) {
		if (glow.levels[i].fbo != 0) {
			glDeleteFramebuffers(1, &glow.levels[i].fbo);
			glow.levels[i].fbo = 0;
		}

		if (glow.levels[i].color != 0) {
			GLES3::Utilities::texture_free_data(glow.levels[i].color);
			glow.levels[i].color = 0;
		}
	}
}

void RenderSceneBuffersGLES3::free_render_buffer_data()
{
	_clear_msaa3d_buffers();
	_clear_intermediate_buffers();
	_clear_back_buffers();
	_clear_glow_buffers();
}

GLuint RenderSceneBuffersGLES3::get_render_fbo()
{
	GLuint rt_fbo = 0;

	_check_render_buffers();

	if (msaa3d.check_fbo_cache) {
		GLuint color = GLES3::TextureStorage::render_target_get_color(render_target);
		GLuint depth = GLES3::TextureStorage::render_target_get_depth(render_target);

		rt_fbo = _rt_get_cached_fbo(color, depth, msaa3d.samples, view_count);
		if (rt_fbo == 0) {
			// Somehow couldn't obtain this? Just render without MSAA.
			rt_fbo = GLES3::TextureStorage::render_target_get_fbo(render_target);
		}
	}
	else if (msaa3d.fbo != 0) {
		// We have an MSAA fbo, render to our MSAA buffer
		return msaa3d.fbo;
	}
	else if (internal3d.fbo != 0) {
		// We have an internal buffer, render to our internal buffer!
		return internal3d.fbo;
	}
	else {
		rt_fbo = GLES3::TextureStorage::render_target_get_fbo(render_target);
	}

	if (GLES3::TextureStorage::render_target_is_reattach_textures(render_target)) {
		GLuint color = GLES3::TextureStorage::render_target_get_color(render_target);
		GLuint depth = GLES3::TextureStorage::render_target_get_depth(render_target);
		bool depth_has_stencil =
			GLES3::TextureStorage::render_target_get_depth_has_stencil(render_target);

		glBindFramebuffer(GL_FRAMEBUFFER, rt_fbo);
		_rt_attach_textures(color, depth, msaa3d.samples, view_count, depth_has_stencil);
		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	}

	return rt_fbo;
}

// GLES3::Utilities

void Utilities::initialize()
{
	frame = 0;
	for (int i = 0; i < FRAME_COUNT; i++) {
		frames[i].index = 0;
		glGenQueries(max_timestamp_query_elements, frames[i].queries);

		frames[i].timestamp_names.resize(max_timestamp_query_elements);
		frames[i].timestamp_cpu_values.resize(max_timestamp_query_elements);
		frames[i].timestamp_count = 0;

		frames[i].timestamp_result_names.resize(max_timestamp_query_elements);
		frames[i].timestamp_cpu_result_values.resize(max_timestamp_query_elements);
		frames[i].timestamp_result_values.resize(max_timestamp_query_elements);
		frames[i].timestamp_result_count = 0;
	}
}

void Utilities::finalize()
{
	for (int i = 0; i < FRAME_COUNT; i++) {
		glDeleteQueries(max_timestamp_query_elements, frames[i].queries);
	}

	if (texture_mem_cache) {
		uint32_t leaked_data_size = 0;
		for (const KeyValue<GLuint, ResourceAllocation>& E : texture_allocs_cache) {
#ifdef DEV_ENABLED
			ERR_PRINT(E.value.name + ": leaked " + itos(E.value.size) + " bytes.");
#else
			ERR_PRINT("Texture with GL ID of " + itos(E.key) + ": leaked " + itos(E.value.size) +
					  " bytes.");
#endif
			leaked_data_size += E.value.size;
		}
		if (leaked_data_size < texture_mem_cache) {
			ERR_PRINT("Texture cache is not empty. There may be an additional texture leak of " +
					  itos(texture_mem_cache - leaked_data_size) + " bytes.");
		}
	}

	if (render_buffer_mem_cache) {
		uint32_t leaked_data_size = 0;
		for (const KeyValue<GLuint, ResourceAllocation>& E : render_buffer_allocs_cache) {
#ifdef DEV_ENABLED
			ERR_PRINT(E.value.name + ": leaked " + itos(E.value.size) + " bytes.");
#else
			ERR_PRINT("Render buffer with GL ID of " + itos(E.key) + ": leaked " +
					  itos(E.value.size) + " bytes.");
#endif
			leaked_data_size += E.value.size;
		}
		if (leaked_data_size < render_buffer_mem_cache) {
			ERR_PRINT("Render buffer cache is not empty. There may be an additional render buffer "
					  "leak of " +
					  itos(render_buffer_mem_cache - leaked_data_size) + " bytes.");
		}
	}

	if (buffer_mem_cache) {
		uint32_t leaked_data_size = 0;

		for (const KeyValue<GLuint, ResourceAllocation>& E : buffer_allocs_cache) {
#ifdef DEV_ENABLED
			ERR_PRINT(E.value.name + ": leaked " + itos(E.value.size) + " bytes.");
#else
			ERR_PRINT("Buffer with GL ID of " + itos(E.key) + ": leaked " + itos(E.value.size) +
					  " bytes.");
#endif
			leaked_data_size += E.value.size;
		}
		if (leaked_data_size < buffer_mem_cache) {
			ERR_PRINT("Buffer cache is not empty. There may be an additional buffer leak of " +
					  itos(buffer_mem_cache - leaked_data_size) + " bytes.");
		}
	}
}

Vector<uint8_t> Utilities::buffer_get_data(GLenum p_target, GLuint p_buffer, uint32_t p_buffer_size)
{
	Vector<uint8_t> ret;

	if (p_buffer_size == 0) {
		return ret;
	}

	ret.resize(p_buffer_size);
	glBindBuffer(p_target, p_buffer);

#if defined(__EMSCRIPTEN__)
	{
		uint8_t* w = ret.ptrw();
		godot_webgl2_glGetBufferSubData(p_target, 0, p_buffer_size, w);
	}
#else
	void* data = glMapBufferRange(p_target, 0, p_buffer_size, GL_MAP_READ_BIT);
	ERR_FAIL_NULL_V(data, Vector<uint8_t>());
	{
		uint8_t* w = ret.ptrw();
		memcpy(w, data, p_buffer_size);
	}
	glUnmapBuffer(p_target);
#endif
	glBindBuffer(p_target, 0);
	return ret;
}

/* INSTANCES */

RSE::InstanceType Utilities::get_base_type(RID p_rid)
{
	if (GLES3::MeshStorage::owns_mesh(p_rid)) {
		return RSE::INSTANCE_MESH;
	}
	else if (GLES3::MeshStorage::owns_multimesh(p_rid)) {
		return RSE::INSTANCE_MULTIMESH;
	}
	else if (GLES3::LightStorage::owns_light(p_rid)) {
		return RSE::INSTANCE_LIGHT;
	}
	else if (GLES3::LightStorage::owns_lightmap(p_rid)) {
		return RSE::INSTANCE_LIGHTMAP;
	}
	else if (GLES3::ParticlesStorage::owns_particles(p_rid)) {
		return RSE::INSTANCE_PARTICLES;
	}
	else if (GLES3::LightStorage::owns_reflection_probe(p_rid)) {
		return RSE::INSTANCE_REFLECTION_PROBE;
	}
	else if (GLES3::ParticlesStorage::owns_particles_collision(p_rid)) {
		return RSE::INSTANCE_PARTICLES_COLLISION;
	}
	else if (owns_visibility_notifier(p_rid)) {
		return RSE::INSTANCE_VISIBLITY_NOTIFIER;
	}
	return RSE::INSTANCE_NONE;
}

bool Utilities::free(RID p_rid)
{
	if (GLES3::TextureStorage::owns_render_target(p_rid)) {
		GLES3::TextureStorage::render_target_free(p_rid);
		return true;
	}
	else if (GLES3::TextureStorage::owns_texture(p_rid)) {
		GLES3::TextureStorage::texture_free(p_rid);
		return true;
	}
	else if (GLES3::TextureStorage::owns_canvas_texture(p_rid)) {
		GLES3::TextureStorage::canvas_texture_free(p_rid);
		return true;
	}
	else if (GLES3::MaterialStorage::owns_shader(p_rid)) {
		GLES3::MaterialStorage::shader_free(p_rid);
		return true;
	}
	else if (GLES3::MaterialStorage::owns_material(p_rid)) {
		GLES3::MaterialStorage::material_free(p_rid);
		return true;
	}
	else if (GLES3::MeshStorage::owns_mesh(p_rid)) {
		GLES3::MeshStorage::mesh_free(p_rid);
		return true;
	}
	else if (GLES3::MeshStorage::owns_multimesh(p_rid)) {
		GLES3::MeshStorage::multimesh_free(p_rid);
		return true;
	}
	else if (GLES3::MeshStorage::owns_mesh_instance(p_rid)) {
		GLES3::MeshStorage::mesh_instance_free(p_rid);
		return true;
	}
	else if (GLES3::LightStorage::owns_light(p_rid)) {
		GLES3::LightStorage::light_free(p_rid);
		return true;
	}
	else if (GLES3::LightStorage::owns_lightmap(p_rid)) {
		GLES3::LightStorage::lightmap_free(p_rid);
		return true;
	}
	else if (GLES3::LightStorage::owns_reflection_probe(p_rid)) {
		GLES3::LightStorage::reflection_probe_free(p_rid);
		return true;
	}
	else if (GLES3::LightStorage::owns_reflection_atlas(p_rid)) {
		GLES3::LightStorage::reflection_atlas_free(p_rid);
		return true;
	}
	else if (GLES3::LightStorage::owns_reflection_probe_instance(p_rid)) {
		GLES3::LightStorage::reflection_probe_instance_free(p_rid);
		return true;
	}
	else if (GLES3::ParticlesStorage::owns_particles(p_rid)) {
		GLES3::ParticlesStorage::particles_free(p_rid);
		return true;
	}
	else if (GLES3::ParticlesStorage::owns_particles_collision(p_rid)) {
		GLES3::ParticlesStorage::particles_collision_free(p_rid);
		return true;
	}
	else if (GLES3::ParticlesStorage::owns_particles_collision_instance(p_rid)) {
		GLES3::ParticlesStorage::particles_collision_instance_free(p_rid);
		return true;
	}
	else if (GLES3::MeshStorage::owns_skeleton(p_rid)) {
		GLES3::MeshStorage::skeleton_free(p_rid);
		return true;
	}
	else if (owns_visibility_notifier(p_rid)) {
		visibility_notifier_free(p_rid);
		return true;
	}
	else {
		return false;
	}
}

/* DEPENDENCIES */

void Utilities::base_update_dependency(RID p_base, DependencyTracker* p_instance)
{
	if (MeshStorage::owns_mesh(p_base)) {
		Mesh* mesh = MeshStorage::get_mesh(p_base);
		p_instance->update_dependency(&mesh->dependency);
	}
	else if (MeshStorage::owns_multimesh(p_base)) {
		MultiMesh* multimesh = MeshStorage::get_multimesh(p_base);
		p_instance->update_dependency(&multimesh->dependency);
		if (multimesh->mesh.is_valid()) {
			base_update_dependency(multimesh->mesh, p_instance);
		}
	}
	else if (LightStorage::owns_reflection_probe(p_base)) {
		Dependency* dependency =
			LightStorage::reflection_probe_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (LightStorage::owns_light(p_base)) {
		Light* l = LightStorage::get_light(p_base);
		p_instance->update_dependency(&l->dependency);
	}
	else if (ParticlesStorage::owns_particles(p_base)) {
		Dependency* dependency =
			ParticlesStorage::particles_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (ParticlesStorage::owns_particles_collision(p_base)) {
		Dependency* dependency =
			ParticlesStorage::particles_collision_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (owns_visibility_notifier(p_base)) {
		VisibilityNotifier* vn = get_visibility_notifier(p_base);
		p_instance->update_dependency(&vn->dependency);
	}
}

/* VISIBILITY NOTIFIER */

RID Utilities::visibility_notifier_allocate() { return visibility_notifier_owner.allocate_rid(); }

void Utilities::visibility_notifier_initialize(RID p_notifier)
{
	visibility_notifier_owner.initialize_rid(p_notifier, VisibilityNotifier());
}

void Utilities::visibility_notifier_free(RID p_notifier)
{
	VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	vn->dependency.deleted_notify(p_notifier);
	visibility_notifier_owner.free(p_notifier);
}

void Utilities::visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb)
{
	VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	ERR_FAIL_NULL(vn);
	vn->aabb = p_aabb;
	vn->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB Utilities::visibility_notifier_get_aabb(RID p_notifier)
{
	const VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	ERR_FAIL_NULL_V(vn, AABB());
	return vn->aabb;
}

void Utilities::visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred)
{
	VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	ERR_FAIL_NULL(vn);
}

/* TIMING */

void Utilities::capture_timestamps_begin() { capture_timestamp("Frame Begin"); }

void Utilities::capture_timestamp(const String& p_name)
{
	if (frames[frame].timestamp_count >= max_timestamp_query_elements) {
		return;
	}

#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		glQueryCounter(frames[frame].queries[frames[frame].timestamp_count], GL_TIMESTAMP);
	}
#endif // GL_API_ENABLED

	frames[frame].timestamp_names[frames[frame].timestamp_count] = p_name;
	frames[frame].timestamp_cpu_values[frames[frame].timestamp_count] = OS::get_ticks_usec();
	frames[frame].timestamp_count++;
}

void Utilities::_capture_timestamps_begin()
{
	// frame is incremented at the end of the frame so this gives us the queries for frame - 2. By
	// then they should be ready.
	if (frames[frame].timestamp_count) {
#ifdef GL_API_ENABLED
		if (RasterizerUtilGLES3::is_gles_over_gl()) {
			for (uint32_t i = 0; i < frames[frame].timestamp_count; i++) {
				uint64_t temp = 0;
				glGetQueryObjectui64v(frames[frame].queries[i], GL_QUERY_RESULT, &temp);
				frames[frame].timestamp_result_values[i] = temp;
			}
		}
#endif // GL_API_ENABLED
		SWAP(frames[frame].timestamp_names, frames[frame].timestamp_result_names);
		SWAP(frames[frame].timestamp_cpu_values, frames[frame].timestamp_cpu_result_values);
	}

	frames[frame].timestamp_result_count = frames[frame].timestamp_count;
	frames[frame].timestamp_count = 0;
	frames[frame].index = Engine::get_frames_drawn();
	capture_timestamp("Internal Begin");
}

void Utilities::capture_timestamps_end()
{
	capture_timestamp("Internal End");
	frame = (frame + 1) % FRAME_COUNT;
}

uint32_t Utilities::get_captured_timestamps_count() { return frames[frame].timestamp_result_count; }

uint64_t Utilities::get_captured_timestamps_frame() { return frames[frame].index; }

uint64_t Utilities::get_captured_timestamp_gpu_time(uint32_t p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, frames[frame].timestamp_result_count, 0);
	return frames[frame].timestamp_result_values[p_index];
}

uint64_t Utilities::get_captured_timestamp_cpu_time(uint32_t p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, frames[frame].timestamp_result_count, 0);
	return frames[frame].timestamp_cpu_result_values[p_index];
}

String Utilities::get_captured_timestamp_name(uint32_t p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, frames[frame].timestamp_result_count, String());
	return frames[frame].timestamp_result_names[p_index];
}

/* MISC */

void Utilities::update_dirty_resources()
{
	MaterialStorage::_update_global_shader_uniforms();
	MaterialStorage::_update_queued_materials();
	MeshStorage::_update_dirty_skeletons();
	MeshStorage::_update_dirty_multimeshes();
	TextureStorage::update_texture_atlas();
}

void Utilities::set_debug_generate_wireframes(bool p_generate)
{
	Config::generate_wireframes = p_generate;
}

bool Utilities::has_os_feature(const String& p_feature)
{
	if (!Config::is_initialized()) {
		return false;
	}

	if (p_feature == "rgtc") {
		return Config::rgtc_supported;
	}
	if (p_feature == "s3tc") {
		return Config::s3tc_supported;
	}
	if (p_feature == "bptc") {
		return Config::bptc_supported;
	}
	if (p_feature == "astc") {
		return Config::astc_supported;
	}
	if (p_feature == "etc2") {
		return Config::etc2_supported;
	}
	if (p_feature == "astc_hdr") {
		return Config::astc_hdr_supported;
	}

	return false;
}

void Utilities::update_memory_info() {}

uint64_t Utilities::get_rendering_info(RSE::RenderingInfo p_info)
{
	if (p_info == RSE::RENDERING_INFO_TEXTURE_MEM_USED) {
		return texture_mem_cache +
			   render_buffer_mem_cache; // Add render buffer memory to our texture mem.
	}
	else if (p_info == RSE::RENDERING_INFO_BUFFER_MEM_USED) {
		return buffer_mem_cache;
	}
	else if (p_info == RSE::RENDERING_INFO_VIDEO_MEM_USED) {
		return texture_mem_cache + buffer_mem_cache + render_buffer_mem_cache;
	}
	return 0;
}

String Utilities::get_video_adapter_name()
{
	const String rendering_device_name = String::utf8((const char*)glGetString(GL_RENDERER));
	return rendering_device_name.trim_suffix("/PCIe/SSE2");
}

String Utilities::get_video_adapter_vendor()
{
	const String rendering_device_vendor = String::utf8((const char*)glGetString(GL_VENDOR));
	return rendering_device_vendor.trim_suffix(" Corporation");
}

RenderingDeviceEnums::DeviceType Utilities::get_video_adapter_type()
{
	return RenderingDeviceEnums::DeviceType::DEVICE_TYPE_OTHER;
}

String Utilities::get_video_adapter_api_version()
{
	return String::utf8((const char*)glGetString(GL_VERSION));
}

Size2i Utilities::get_maximum_viewport_size()
{
	ERR_FAIL_COND_V(!Config::is_initialized(), Size2i());
	return Size2i(Config::max_viewport_size[0], Config::max_viewport_size[1]);
}

uint32_t Utilities::get_maximum_shader_varyings()
{
	ERR_FAIL_COND_V(!Config::is_initialized(), 31);
	return Config::max_shader_varyings;
}

uint64_t Utilities::get_maximum_uniform_buffer_size()
{
	ERR_FAIL_COND_V(!Config::is_initialized(), 65536);
	return uint64_t(Config::max_uniform_buffer_size);
}

#endif // GLES3_ENABLED
