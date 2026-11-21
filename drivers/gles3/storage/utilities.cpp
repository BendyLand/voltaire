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

// ============================================================================
// Merged from drivers/gles3/storage/light_storage.cpp
// ============================================================================

#include "light_storage.h"

#ifdef GLES3_ENABLED

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/utilities.h"
#include "drivers/gles3/rasterizer_scene_gles3.h"
#include "drivers/gles3/storage/utilities.h"

using namespace GLES3;

/* Light API */

void GLES3::LightStorage::_light_initialize(RID p_light, RSE::LightType p_type)
{
	Light light;
	light.type = p_type;

	light.param[RSE::LIGHT_PARAM_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_INDIRECT_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_SPECULAR] = 0.5;
	light.param[RSE::LIGHT_PARAM_RANGE] = 1.0;
	light.param[RSE::LIGHT_PARAM_SIZE] = 0.0;
	light.param[RSE::LIGHT_PARAM_ATTENUATION] = 1.0;
	light.param[RSE::LIGHT_PARAM_SPOT_ANGLE] = 45;
	light.param[RSE::LIGHT_PARAM_SPOT_ATTENUATION] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE] = 0;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET] = 0.1;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET] = 0.3;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET] = 0.6;
	light.param[RSE::LIGHT_PARAM_SHADOW_FADE_START] = 0.8;
	light.param[RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_OPACITY] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_BIAS] = 0.02;
	light.param[RSE::LIGHT_PARAM_SHADOW_BLUR] = 0;
	light.param[RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE] = 20.0;
	light.param[RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS] = 0.05;
	light.param[RSE::LIGHT_PARAM_INTENSITY] = p_type == RSE::LIGHT_DIRECTIONAL ? 100000.0 : 1000.0;

	light_owner.initialize_rid(p_light, light);
}

RID GLES3::LightStorage::directional_light_allocate() { return light_owner.allocate_rid(); }

void GLES3::LightStorage::directional_light_initialize(RID p_rid)
{
	_light_initialize(p_rid, RSE::LIGHT_DIRECTIONAL);
}

RID GLES3::LightStorage::omni_light_allocate() { return light_owner.allocate_rid(); }

void GLES3::LightStorage::omni_light_initialize(RID p_rid)
{
	_light_initialize(p_rid, RSE::LIGHT_OMNI);
}

RID GLES3::LightStorage::spot_light_allocate() { return light_owner.allocate_rid(); }

void GLES3::LightStorage::spot_light_initialize(RID p_rid)
{
	_light_initialize(p_rid, RSE::LIGHT_SPOT);
}

RID GLES3::LightStorage::area_light_allocate() { return light_owner.allocate_rid(); }

void GLES3::LightStorage::area_light_initialize(RID p_rid)
{
	_light_initialize(p_rid, RSE::LIGHT_AREA);
}

void GLES3::LightStorage::light_free(RID p_rid)
{
	light_set_projector(p_rid, RID()); // clear projector

	// delete the texture
	Light* light = light_owner.get_or_null(p_rid);
	light->dependency.deleted_notify(p_rid);
	light_owner.free(p_rid);
}

void GLES3::LightStorage::light_set_color(RID p_light, const Color& p_color)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->color = p_color;
}

void GLES3::LightStorage::light_set_param(RID p_light, RSE::LightParam p_param, float p_value)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	ERR_FAIL_INDEX(p_param, RSE::LIGHT_PARAM_MAX);

	if (light->param[p_param] == p_value) {
		return;
	}

	switch (p_param) {
	case RSE::LIGHT_PARAM_RANGE:
	case RSE::LIGHT_PARAM_SPOT_ANGLE:
	case RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS:
	case RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE:
	case RSE::LIGHT_PARAM_SHADOW_BIAS: {
		light->version++;
		light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
	} break;
	case RSE::LIGHT_PARAM_SIZE: {
		if ((light->param[p_param] > CMP_EPSILON) != (p_value > CMP_EPSILON)) {
			// changing from no size to size and the opposite
			light->dependency.changed_notify(
				Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR);
		}
	} break;
	default: {
	}
	}

	light->param[p_param] = p_value;
}

void GLES3::LightStorage::light_set_shadow(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	if (light->type == RSE::LIGHT_AREA) {
		light->shadow = false;
	}
	else {
		light->shadow = p_enabled;
	}

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void GLES3::LightStorage::light_set_projector(RID p_light, RID p_texture)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	if (light->projector == p_texture) {
		return;
	}

	if (light->type != RSE::LIGHT_DIRECTIONAL && light->projector.is_valid()) {
		GLES3::TextureStorage::texture_remove_from_decal_atlas(
			light->projector, light->type == RSE::LIGHT_OMNI);
	}

	light->projector = p_texture;

	if (light->type != RSE::LIGHT_DIRECTIONAL) {
		if (light->projector.is_valid()) {
			GLES3::TextureStorage::texture_add_to_decal_atlas(
				light->projector, light->type == RSE::LIGHT_OMNI);
		}
		light->dependency.changed_notify(
			Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR);
	}
}

void GLES3::LightStorage::light_set_negative(RID p_light, bool p_enable)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->negative = p_enable;
}

void GLES3::LightStorage::light_set_cull_mask(RID p_light, uint32_t p_mask)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->cull_mask = p_mask;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_CULL_MASK);
}

void GLES3::LightStorage::light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->shadow_caster_mask = p_caster_mask;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

uint32_t GLES3::LightStorage::light_get_shadow_caster_mask(RID p_light)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->shadow_caster_mask;
}

void GLES3::LightStorage::light_set_distance_fade(
	RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->distance_fade = p_enabled;
	light->distance_fade_begin = p_begin;
	light->distance_fade_shadow = p_shadow;
	light->distance_fade_length = p_length;
}

void GLES3::LightStorage::light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->reverse_cull = p_enabled;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void GLES3::LightStorage::light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->bake_mode = p_bake_mode;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void GLES3::LightStorage::light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->omni_shadow_mode = p_mode;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

RSE::LightOmniShadowMode GLES3::LightStorage::light_omni_get_shadow_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_OMNI_SHADOW_CUBE);

	return light->omni_shadow_mode;
}

void GLES3::LightStorage::light_directional_set_shadow_mode(
	RID p_light, RSE::LightDirectionalShadowMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_shadow_mode = p_mode;
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void GLES3::LightStorage::light_directional_set_blend_splits(RID p_light, bool p_enable)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_blend_splits = p_enable;
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

bool GLES3::LightStorage::light_directional_get_blend_splits(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, false);

	return light->directional_blend_splits;
}

void GLES3::LightStorage::light_directional_set_sky_mode(
	RID p_light, RSE::LightDirectionalSkyMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_sky_mode = p_mode;
}

RSE::LightDirectionalSkyMode GLES3::LightStorage::light_directional_get_sky_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY);

	return light->directional_sky_mode;
}

RSE::LightDirectionalShadowMode GLES3::LightStorage::light_directional_get_shadow_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL);

	return light->directional_shadow_mode;
}

void GLES3::LightStorage::light_area_set_size(RID p_light, const Vector2& p_size)
{
	Light* light = light_owner.get_or_null(p_light);
	light->area_size = p_size.maxf(0.0f);
	// The range in which objects are illuminated change, so the z-range of the shadow map needs to
	// adjust accordingly.
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

Vector2 GLES3::LightStorage::light_area_get_size(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	return light->area_size;
}

void GLES3::LightStorage::light_area_set_normalize_energy(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	light->area_normalize_energy = p_enabled;
}

bool GLES3::LightStorage::light_area_get_normalize_energy(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	return light->area_normalize_energy;
}

void GLES3::LightStorage::light_area_set_texture(RID p_light, RID p_texture)
{
	// not implemented
}

RID GLES3::LightStorage::light_area_get_texture(RID p_light)
{
	return RID(); // not implemented
}

RSE::LightBakeMode GLES3::LightStorage::light_get_bake_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_BAKE_DISABLED);

	return light->bake_mode;
}

uint64_t GLES3::LightStorage::light_get_version(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->version;
}

uint32_t GLES3::LightStorage::light_get_cull_mask(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->cull_mask;
}

AABB GLES3::LightStorage::light_get_aabb(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, AABB());

	switch (light->type) {
	case RSE::LIGHT_SPOT: {
		float len = light->param[RSE::LIGHT_PARAM_RANGE];
		float angle = Math::deg_to_rad(light->param[RSE::LIGHT_PARAM_SPOT_ANGLE]);

		if (angle > Math::PI * 0.5) {
			// Light casts backwards as well.
			return AABB(Vector3(-1, -1, -1) * len, Vector3(2, 2, 2) * len);
		}

		float size = Math::sin(angle) * len;
		return AABB(Vector3(-size, -size, -len), Vector3(size * 2, size * 2, len));
	};
	case RSE::LIGHT_OMNI: {
		float r = light->param[RSE::LIGHT_PARAM_RANGE];
		return AABB(-Vector3(r, r, r), Vector3(r, r, r) * 2);
	};
	case RSE::LIGHT_AREA: {
		float len = light->param[RSE::LIGHT_PARAM_RANGE];

		float width = light->area_size.x / 2.0 + len;
		float height = light->area_size.y / 2.0 + len;

		return AABB(-Vector3(width, height, 0), Vector3(width * 2, height * 2, -len));
	};
	case RSE::LIGHT_DIRECTIONAL: {
		return AABB();
	};
	}

	ERR_FAIL_V(AABB());
}

/* LIGHT INSTANCE API */

RID GLES3::LightStorage::light_instance_create(RID p_light)
{
	RID li = light_instance_owner.make_rid(LightInstance());

	LightInstance* light_instance = light_instance_owner.get_or_null(li);

	light_instance->self = li;
	light_instance->light = p_light;
	light_instance->light_type = light_get_type(p_light);

	return li;
}

void GLES3::LightStorage::light_instance_free(RID p_light_instance)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	// Remove from shadow atlases.
	for (const RID& E : light_instance->shadow_atlases) {
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(E);
		ERR_CONTINUE(!shadow_atlas->shadow_owners.has(p_light_instance));
		uint32_t key = shadow_atlas->shadow_owners[p_light_instance];
		uint32_t q = (key >> QUADRANT_SHIFT) & 0x3;
		uint32_t s = key & SHADOW_INDEX_MASK;

		shadow_atlas->quadrants[q].shadows.write[s].owner = RID();

		shadow_atlas->shadow_owners.erase(p_light_instance);
	}

	light_instance_owner.free(p_light_instance);
}

void GLES3::LightStorage::light_instance_set_transform(
	RID p_light_instance, const Transform3D& p_transform)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->transform = p_transform;
}

void GLES3::LightStorage::light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->aabb = p_aabb;
}

void GLES3::LightStorage::light_instance_set_shadow_transform(RID p_light_instance,
	const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
	int p_pass, float p_shadow_texel_size, float p_bias_scale, float p_range_begin,
	const Vector2& p_uv_scale)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	ERR_FAIL_INDEX(p_pass, 6);

	light_instance->shadow_transform[p_pass].camera = p_projection;
	light_instance->shadow_transform[p_pass].transform = p_transform;
	light_instance->shadow_transform[p_pass].farplane = p_far;
	light_instance->shadow_transform[p_pass].split = p_split;
	light_instance->shadow_transform[p_pass].bias_scale = p_bias_scale;
	light_instance->shadow_transform[p_pass].range_begin = p_range_begin;
	light_instance->shadow_transform[p_pass].shadow_texel_size = p_shadow_texel_size;
	light_instance->shadow_transform[p_pass].uv_scale = p_uv_scale;
}

void GLES3::LightStorage::light_instance_mark_visible(RID p_light_instance)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->last_scene_pass = RasterizerSceneGLES3::get_scene_pass();
}

/* PROBE API */

RID GLES3::LightStorage::reflection_probe_allocate()
{
	return reflection_probe_owner.allocate_rid();
}

void GLES3::LightStorage::reflection_probe_initialize(RID p_rid)
{
	ReflectionProbe probe;

	reflection_probe_owner.initialize_rid(p_rid, probe);
}

void GLES3::LightStorage::reflection_probe_free(RID p_rid)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_rid);
	reflection_probe->dependency.deleted_notify(p_rid);

	reflection_probe_owner.free(p_rid);
}

void GLES3::LightStorage::reflection_probe_set_update_mode(
	RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->update_mode = p_mode;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_intensity(RID p_probe, float p_intensity)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->intensity = p_intensity;
}

void GLES3::LightStorage::reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->blend_distance = p_blend_distance;
}

void GLES3::LightStorage::reflection_probe_set_ambient_mode(
	RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_mode = p_mode;
}

void GLES3::LightStorage::reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_color = p_color;
}

void GLES3::LightStorage::reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_color_energy = p_energy;
}

void GLES3::LightStorage::reflection_probe_set_max_distance(RID p_probe, float p_distance)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->max_distance = p_distance;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_size(RID p_probe, const Vector3& p_size)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->size = p_size;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->origin_offset = p_offset;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_as_interior(RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->interior = p_enable;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->box_projection = p_enable;
}

void GLES3::LightStorage::reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->enable_shadows = p_enable;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->cull_mask = p_layers;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->reflection_mask = p_layers;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void GLES3::LightStorage::reflection_probe_set_resolution(RID p_probe, int p_resolution)
{
	WARN_PRINT_ONCE("reflection_probe_set_resolution is not available in Godot 4. ReflectionProbe "
					"size is configured in the project settings with the "
					"rendering/reflections/reflection_atlas/reflection_size setting.");
}

AABB GLES3::LightStorage::reflection_probe_get_aabb(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, AABB());

	AABB aabb;
	aabb.position = -reflection_probe->size / 2;
	aabb.size = reflection_probe->size;

	return aabb;
}

RSE::ReflectionProbeUpdateMode GLES3::LightStorage::reflection_probe_get_update_mode(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, RSE::REFLECTION_PROBE_UPDATE_ONCE);

	return reflection_probe->update_mode;
}

uint32_t GLES3::LightStorage::reflection_probe_get_cull_mask(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->cull_mask;
}

uint32_t GLES3::LightStorage::reflection_probe_get_reflection_mask(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->reflection_mask;
}

Vector3 GLES3::LightStorage::reflection_probe_get_size(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, Vector3());

	return reflection_probe->size;
}

Vector3 GLES3::LightStorage::reflection_probe_get_origin_offset(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, Vector3());

	return reflection_probe->origin_offset;
}

float GLES3::LightStorage::reflection_probe_get_origin_max_distance(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0.0);

	return reflection_probe->max_distance;
}

bool GLES3::LightStorage::reflection_probe_renders_shadows(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, false);

	return reflection_probe->enable_shadows;
}

void GLES3::LightStorage::reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->mesh_lod_threshold = p_ratio;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

float GLES3::LightStorage::reflection_probe_get_mesh_lod_threshold(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0.0);

	return reflection_probe->mesh_lod_threshold;
}

Dependency* GLES3::LightStorage::reflection_probe_get_dependency(RID p_probe)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, nullptr);

	return &reflection_probe->dependency;
}

/* REFLECTION ATLAS */

RID GLES3::LightStorage::reflection_atlas_create()
{
	ReflectionAtlas ra = {};
	return reflection_atlas_owner.make_rid(ra);
}

void GLES3::LightStorage::reflection_atlas_free(RID p_ref_atlas)
{
	reflection_atlas_set_size(p_ref_atlas, 0, 0);

	reflection_atlas_owner.free(p_ref_atlas);
}

int GLES3::LightStorage::reflection_atlas_get_size(RID p_ref_atlas)
{
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(p_ref_atlas);
	ERR_FAIL_NULL_V(ra, 0);

	return ra->size;
}

void GLES3::LightStorage::reflection_atlas_set_size(
	RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
{
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(p_ref_atlas);
	ERR_FAIL_NULL(ra);

	if (ra->size == p_reflection_size && ra->count == p_reflection_count) {
		return; // no changes
	}

	ra->size = p_reflection_size;
	ra->count = p_reflection_count;

	if (ra->depth != 0) {
		// clear and invalidate everything
		for (int i = 0; i < ra->reflections.size(); i++) {
			for (int j = 0; j < 7; j++) {
				if (ra->reflections[i].fbos[j] != 0) {
					glDeleteFramebuffers(1, &ra->reflections[i].fbos[j]);
					ra->reflections.write[i].fbos[j] = 0;
				}
			}

			GLES3::Utilities::texture_free_data(ra->reflections[i].color);
			ra->reflections.write[i].color = 0;

			GLES3::Utilities::texture_free_data(ra->reflections[i].radiance);
			ra->reflections.write[i].radiance = 0;

			if (ra->reflections[i].owner.is_null()) {
				continue;
			}
			reflection_probe_release_atlas_index(ra->reflections[i].owner);
			// rp->atlasindex clear
		}

		ra->reflections.clear();

		GLES3::Utilities::texture_free_data(ra->depth);
		ra->depth = 0;
	}

	if (ra->render_buffers.is_valid()) {
		ra->render_buffers->free_render_buffer_data();
	}
}

/* REFLECTION PROBE INSTANCE */

RID GLES3::LightStorage::reflection_probe_instance_create(RID p_probe)
{
	ReflectionProbeInstance rpi;
	rpi.probe = p_probe;

	return reflection_probe_instance_owner.make_rid(rpi);
}

void GLES3::LightStorage::reflection_probe_instance_free(RID p_instance)
{
	reflection_probe_release_atlas_index(p_instance);
	reflection_probe_instance_owner.free(p_instance);
}

void GLES3::LightStorage::reflection_probe_instance_set_transform(
	RID p_instance, const Transform3D& p_transform)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(rpi);

	rpi->transform = p_transform;
	rpi->dirty = true;
}

bool GLES3::LightStorage::reflection_probe_has_atlas_index(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (rpi->atlas.is_null()) {
		return false;
	}

	return rpi->atlas_index >= 0;
}

void GLES3::LightStorage::reflection_probe_release_atlas_index(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(rpi);

	if (rpi->atlas.is_null()) {
		return; // nothing to release
	}
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL(atlas);

	ERR_FAIL_INDEX(rpi->atlas_index, atlas->reflections.size());
	atlas->reflections.write[rpi->atlas_index].owner = RID();

	if (rpi->rendering) {
		// We were cancelled mid rendering, trigger refresh.
		rpi->rendering = false;
		rpi->dirty = true;
		rpi->processing_layer = 0;
	}

	rpi->atlas_index = -1;
	rpi->atlas = RID();
}

bool GLES3::LightStorage::reflection_probe_instance_needs_redraw(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (rpi->rendering) {
		return false;
	}

	if (rpi->dirty) {
		return true;
	}

	if (reflection_probe_get_update_mode(rpi->probe) == RSE::REFLECTION_PROBE_UPDATE_ALWAYS) {
		return true;
	}

	return rpi->atlas_index == -1;
}

bool GLES3::LightStorage::reflection_probe_instance_has_reflection(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	return rpi->atlas.is_valid();
}

bool GLES3::LightStorage::reflection_probe_instance_begin_render(
	RID p_instance, RID p_reflection_atlas)
{
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_reflection_atlas);

	ERR_FAIL_NULL_V(atlas, false);

	ERR_FAIL_COND_V_MSG(
		atlas->size < 4, false, "Attempted to render to a reflection atlas of invalid resolution.");
	ERR_FAIL_COND_V_MSG(
		atlas->count < 1, false, "Attempted to render to a reflection atlas of size < 1.");

	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (atlas->render_buffers.is_null()) {
		atlas->render_buffers.instantiate();
		atlas->render_buffers->configure_for_probe(Size2i(atlas->size, atlas->size));
	}

	// First we check if our atlas is initialized.

	// Not making an exception for update_mode = REFLECTION_PROBE_UPDATE_ALWAYS, we are using
	// the same render techniques regardless of realtime or update once (for now).

	if (atlas->depth == 0) {
		// We need to create our textures
		atlas->mipmap_count =
			Image::get_image_required_mipmaps(atlas->size, atlas->size, Image::FORMAT_RGBAH) - 1;
		atlas->mipmap_count = MIN(atlas->mipmap_count, 8); // No more than 8 please..

		glActiveTexture(GL_TEXTURE0);

		{
			// We create one set of 6 layers for depth, we can reuse this when rendering.
			glGenTextures(1, &atlas->depth);
			glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->depth);

			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, atlas->size, atlas->size, 6,
				0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

			GLES3::Utilities::texture_allocated_data(
				atlas->depth, atlas->size * atlas->size * 6 * 3, "Reflection probe atlas (depth)");
		}

		// Make room for our atlas entries
		atlas->reflections.resize(atlas->count);

		for (int i = 0; i < atlas->count; i++) {
			// Create a cube map for this atlas entry
			GLuint color = 0;
			glGenTextures(1, &color);
			glBindTexture(GL_TEXTURE_CUBE_MAP, color);
			atlas->reflections.write[i].color = color;

#ifdef GL_API_ENABLED
			if (RasterizerUtilGLES3::is_gles_over_gl()) {
				for (int s = 0; s < 6; s++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + s, 0, GL_RGB10_A2, atlas->size,
						atlas->size, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
				}
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
#endif
#ifdef GLES_API_ENABLED
			if (!RasterizerUtilGLES3::is_gles_over_gl()) {
				glTexStorage2D(GL_TEXTURE_CUBE_MAP, atlas->mipmap_count, GL_RGB10_A2, atlas->size,
					atlas->size);
			}
#endif // GLES_API_ENABLED

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, atlas->mipmap_count - 1);

			// Setup sizes and calculate how much memory we're using.
			int mipmap_size = atlas->size;
			uint32_t data_size = 0;
			for (int m = 0; m < atlas->mipmap_count; m++) {
				atlas->mipmap_size[m] = mipmap_size;
				data_size += mipmap_size * mipmap_size * 6 * 4;
				mipmap_size = MAX(mipmap_size >> 1, 1);
			}

			GLES3::Utilities::texture_allocated_data(color, data_size,
				String("Reflection probe atlas (") + String::num_int64(i) + String(", color)"));

			// Create a radiance map for this atlas entry
			GLuint radiance = 0;
			glGenTextures(1, &radiance);
			glBindTexture(GL_TEXTURE_CUBE_MAP, radiance);
			atlas->reflections.write[i].radiance = radiance;

#ifdef GL_API_ENABLED
			if (RasterizerUtilGLES3::is_gles_over_gl()) {
				for (int s = 0; s < 6; s++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + s, 0, GL_RGB10_A2, atlas->size,
						atlas->size, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
				}
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
#endif
#ifdef GLES_API_ENABLED
			if (!RasterizerUtilGLES3::is_gles_over_gl()) {
				glTexStorage2D(GL_TEXTURE_CUBE_MAP, atlas->mipmap_count, GL_RGB10_A2, atlas->size,
					atlas->size);
			}
#endif // GLES_API_ENABLED

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, atlas->mipmap_count - 1);

			// Same data size as our color buffer
			GLES3::Utilities::texture_allocated_data(radiance, data_size,
				String("Reflection probe atlas (") + String::num_int64(i) + String(", radiance)"));

			// Create our framebuffers so we can draw to all sides
			for (int side = 0; side < 6; side++) {
				GLuint fbo = 0;
				glGenFramebuffers(1, &fbo);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);

				// We use glFramebufferTexture2D for the color buffer as glFramebufferTextureLayer
				// doesn't always work with cubemaps.
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, color, 0);
				glFramebufferTextureLayer(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, atlas->depth, 0, side);

				// Validate framebuffer
				GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE) {
					WARN_PRINT("Could not create reflections framebuffer, status: " +
							   GLES3::TextureStorage::get_framebuffer_error(status));
				}

				atlas->reflections.write[i].fbos[side] = fbo;
			}

			// Create an extra framebuffer for building our radiance
			{
				GLuint fbo = 0;
				glGenFramebuffers(1, &fbo);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);

				atlas->reflections.write[i].fbos[6] = fbo;
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	// Then we find a free slot for our reflection probe

	if (rpi->atlas_index == -1) {
		for (int i = 0; i < atlas->reflections.size(); i++) {
			if (atlas->reflections[i].owner.is_null()) {
				rpi->atlas_index = i;
				break;
			}
		}
		// find the one used last
		if (rpi->atlas_index == -1) {
			// everything is in use, find the one least used via LRU
			uint64_t pass_min = 0;

			for (int i = 0; i < atlas->reflections.size(); i++) {
				ReflectionProbeInstance* rpi2 =
					reflection_probe_instance_owner.get_or_null(atlas->reflections[i].owner);
				if (rpi2->last_pass < pass_min) {
					pass_min = rpi2->last_pass;
					rpi->atlas_index = i;
				}
			}
		}
	}

	if (rpi->atlas_index != -1) { // should we fail if this is still -1 ?
		atlas->reflections.write[rpi->atlas_index].owner = p_instance;
	}

	rpi->atlas = p_reflection_atlas;
	rpi->rendering = true;
	rpi->dirty = false;
	rpi->processing_layer = 0;

	return true;
}

bool GLES3::LightStorage::reflection_probe_instance_end_render(
	RID p_instance, RID p_reflection_atlas)
{
	return true;
}

Ref<RenderSceneBuffers> GLES3::LightStorage::reflection_probe_atlas_get_render_buffers(
	RID p_reflection_atlas)
{
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_reflection_atlas);
	ERR_FAIL_NULL_V(atlas, Ref<RenderSceneBuffersGLES3>());

	return atlas->render_buffers;
}

bool GLES3::LightStorage::reflection_probe_instance_postprocess_step(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);
	ERR_FAIL_COND_V(!rpi->rendering, false);
	ERR_FAIL_COND_V(rpi->atlas.is_null(), false);

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	if (!atlas || rpi->atlas_index == -1) {
		// does not belong to an atlas anymore, cancel (was removed from atlas or atlas changed
		// while rendering)
		rpi->rendering = false;
		rpi->processing_layer = 0;
		return false;
	}

	if (GLES3::LightStorage::reflection_probe_get_update_mode(rpi->probe) ==
		RSE::REFLECTION_PROBE_UPDATE_ALWAYS) {
		// Using real time reflections, all roughness is done in one step
		for (int m = 0; m < atlas->mipmap_count; m++) {
			const GLES3::ReflectionAtlas::Reflection& reflection =
				atlas->reflections[rpi->atlas_index];
			GLES3::CubemapFilter::filter_radiance(reflection.color, reflection.radiance,
				reflection.fbos[6], atlas->size, atlas->mipmap_count, m);
		}

		rpi->rendering = false;
		rpi->processing_layer = 0;
		return true;
	}
	else {
		const GLES3::ReflectionAtlas::Reflection& reflection = atlas->reflections[rpi->atlas_index];
		GLES3::CubemapFilter::filter_radiance(reflection.color, reflection.radiance,
			reflection.fbos[6], atlas->size, atlas->mipmap_count, rpi->processing_layer);

		rpi->processing_layer++;
		if (rpi->processing_layer == atlas->mipmap_count) {
			rpi->rendering = false;
			rpi->processing_layer = 0;
			return true;
		}
	}

	return false;
}

GLuint GLES3::LightStorage::reflection_probe_instance_get_texture(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, 0);

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL_V(atlas, 0);
	ERR_FAIL_COND_V(rpi->atlas_index < 0, 0);

	return atlas->reflections[rpi->atlas_index].radiance;
}

GLuint GLES3::LightStorage::reflection_probe_instance_get_framebuffer(RID p_instance, int p_index)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, 0);
	ERR_FAIL_INDEX_V(p_index, 6, 0);

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL_V(atlas, 0);
	ERR_FAIL_COND_V(rpi->atlas_index < 0, 0);

	return atlas->reflections[rpi->atlas_index].fbos[p_index];
}

/* LIGHTMAP CAPTURE */

RID GLES3::LightStorage::lightmap_allocate() { return lightmap_owner.allocate_rid(); }

void GLES3::LightStorage::lightmap_initialize(RID p_rid)
{
	lightmap_owner.initialize_rid(p_rid, Lightmap());
}

void GLES3::LightStorage::lightmap_free(RID p_rid)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(lightmap);
	lightmap->dependency.deleted_notify(p_rid);
	lightmap_owner.free(p_rid);
}

void GLES3::LightStorage::lightmap_set_textures(
	RID p_lightmap, RID p_light, bool p_uses_spherical_haromics)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);
	lightmap->light_texture = p_light;
	lightmap->uses_spherical_harmonics = p_uses_spherical_haromics;

	Vector3i light_texture_size = GLES3::TextureStorage::texture_get_size(lightmap->light_texture);
	lightmap->light_texture_size = Vector2i(light_texture_size.x, light_texture_size.y);

	GLuint tex = GLES3::TextureStorage::texture_get_texid(lightmap->light_texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void GLES3::LightStorage::lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);
	lightmap->bounds = p_bounds;
}

void GLES3::LightStorage::lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);
	lightmap->interior = p_interior;
}

void GLES3::LightStorage::lightmap_set_probe_capture_data(RID p_lightmap,
	const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);

	if (p_points.size()) {
		ERR_FAIL_COND(p_points.size() * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
	}

	lightmap->points = p_points;
	lightmap->point_sh = p_point_sh;
	lightmap->tetrahedra = p_tetrahedra;
	lightmap->bsp_tree = p_bsp_tree;
}

void GLES3::LightStorage::lightmap_set_baked_exposure_normalization(
	RID p_lightmap, float p_exposure)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);

	lightmap->baked_exposure = p_exposure;
}

PackedVector3Array GLES3::LightStorage::lightmap_get_probe_capture_points(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, PackedVector3Array());
	return lightmap->points;
}

PackedColorArray GLES3::LightStorage::lightmap_get_probe_capture_sh(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, PackedColorArray());
	return lightmap->point_sh;
}

PackedInt32Array GLES3::LightStorage::lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, PackedInt32Array());
	return lightmap->tetrahedra;
}

PackedInt32Array GLES3::LightStorage::lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, PackedInt32Array());
	return lightmap->bsp_tree;
}

AABB GLES3::LightStorage::lightmap_get_aabb(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, AABB());
	return lightmap->bounds;
}

void GLES3::LightStorage::lightmap_tap_sh_light(RID p_lightmap, const Vector3& p_point, Color* r_sh)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	for (int i = 0; i < 9; i++) {
		r_sh[i] = Color(0, 0, 0, 0);
	}

	if (!lm->points.size() || !lm->bsp_tree.size() || !lm->tetrahedra.size()) {
		return;
	}

	static_assert(sizeof(Lightmap::BSP) == 24);

	const Lightmap::BSP* bsp = (const Lightmap::BSP*)lm->bsp_tree.ptr();
	int32_t node = 0;
	while (node >= 0) {
		if (Plane(bsp[node].plane[0], bsp[node].plane[1], bsp[node].plane[2], bsp[node].plane[3])
				.is_point_over(p_point)) {
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND(bsp[node].over >= 0 && bsp[node].over < node);
#endif

			node = bsp[node].over;
		}
		else {
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND(bsp[node].under >= 0 && bsp[node].under < node);
#endif
			node = bsp[node].under;
		}
	}

	if (node == Lightmap::BSP::EMPTY_LEAF) {
		return; // Nothing could be done.
	}

	node = Math::abs(node) - 1;

	uint32_t* tetrahedron = (uint32_t*)&lm->tetrahedra[node * 4];
	Vector3 points[4] = {lm->points[tetrahedron[0]], lm->points[tetrahedron[1]],
		lm->points[tetrahedron[2]], lm->points[tetrahedron[3]]};
	const Color* sh_colors[4]{&lm->point_sh[tetrahedron[0] * 9], &lm->point_sh[tetrahedron[1] * 9],
		&lm->point_sh[tetrahedron[2] * 9], &lm->point_sh[tetrahedron[3] * 9]};
	Color barycentric = Geometry3D::tetrahedron_get_barycentric_coords(
		points[0], points[1], points[2], points[3], p_point);

	for (int i = 0; i < 4; i++) {
		float c = CLAMP(barycentric[i], 0.0, 1.0);
		for (int j = 0; j < 9; j++) {
			r_sh[j] += sh_colors[i][j] * c;
		}
	}
}

bool GLES3::LightStorage::lightmap_is_interior(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, false);
	return lightmap->interior;
}

void GLES3::LightStorage::lightmap_set_probe_capture_update_speed(float p_speed)
{
	lightmap_probe_capture_update_speed = p_speed;
}

float GLES3::LightStorage::lightmap_get_probe_capture_update_speed()
{
	return lightmap_probe_capture_update_speed;
}

void GLES3::LightStorage::lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);
	lightmap->shadow_texture = p_shadow;

	GLuint tex = GLES3::TextureStorage::texture_get_texid(lightmap->shadow_texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

RSE::ShadowmaskMode GLES3::LightStorage::lightmap_get_shadowmask_mode(RID p_lightmap)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lightmap, RSE::SHADOWMASK_MODE_NONE);

	return lightmap->shadowmask_mode;
}

void GLES3::LightStorage::lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode)
{
	Lightmap* lightmap = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lightmap);
	lightmap->shadowmask_mode = p_mode;
}

/* LIGHTMAP INSTANCE */

RID GLES3::LightStorage::lightmap_instance_create(RID p_lightmap)
{
	LightmapInstance li;
	li.lightmap = p_lightmap;
	return lightmap_instance_owner.make_rid(li);
}

void GLES3::LightStorage::lightmap_instance_free(RID p_lightmap)
{
	lightmap_instance_owner.free(p_lightmap);
}

void GLES3::LightStorage::lightmap_instance_set_transform(
	RID p_lightmap, const Transform3D& p_transform)
{
	LightmapInstance* li = lightmap_instance_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(li);
	li->transform = p_transform;
}

/* SHADOW ATLAS API */

RID GLES3::LightStorage::shadow_atlas_create()
{
	return shadow_atlas_owner.make_rid(ShadowAtlas());
}

void GLES3::LightStorage::shadow_atlas_free(RID p_atlas)
{
	shadow_atlas_set_size(p_atlas, 0);
	shadow_atlas_owner.free(p_atlas);
}

void GLES3::LightStorage::shadow_atlas_set_size(RID p_atlas, int p_size, bool p_16_bits)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL(shadow_atlas);
	ERR_FAIL_COND(p_size < 0);
	p_size = Math::next_power_of_2((uint32_t)p_size);

	if (p_size == shadow_atlas->size && p_16_bits == shadow_atlas->use_16_bits) {
		return;
	}

	for (uint32_t i = 0; i < 4; i++) {
		// Clear all subdivisions and free shadows.
		for (uint32_t j = 0; j < shadow_atlas->quadrants[i].textures.size(); j++) {
			glDeleteTextures(1, &shadow_atlas->quadrants[i].textures[j]);
			glDeleteFramebuffers(1, &shadow_atlas->quadrants[i].fbos[j]);
		}
		shadow_atlas->quadrants[i].textures.clear();
		shadow_atlas->quadrants[i].fbos.clear();

		shadow_atlas->quadrants[i].shadows.clear();
		shadow_atlas->quadrants[i].shadows.resize(
			shadow_atlas->quadrants[i].subdivision * shadow_atlas->quadrants[i].subdivision);
	}

	// Erase shadow atlas reference from lights.
	for (const KeyValue<RID, uint32_t>& E : shadow_atlas->shadow_owners) {
		LightInstance* li = light_instance_owner.get_or_null(E.key);
		ERR_CONTINUE(!li);
		li->shadow_atlases.erase(p_atlas);
	}

	if (shadow_atlas->debug_texture != 0) {
		glDeleteTextures(1, &shadow_atlas->debug_texture);
	}

	if (shadow_atlas->debug_fbo != 0) {
		glDeleteFramebuffers(1, &shadow_atlas->debug_fbo);
	}

	// Clear owners.
	shadow_atlas->shadow_owners.clear();

	shadow_atlas->size = p_size;
	shadow_atlas->use_16_bits = p_16_bits;
}

void GLES3::LightStorage::shadow_atlas_set_quadrant_subdivision(
	RID p_atlas, int p_quadrant, int p_subdivision)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL(shadow_atlas);
	ERR_FAIL_INDEX(p_quadrant, 4);
	ERR_FAIL_INDEX(p_subdivision, 16384);

	uint32_t subdiv = Math::next_power_of_2((uint32_t)p_subdivision);
	if (subdiv & 0xaaaaaaaa) { // sqrt(subdiv) must be integer.
		subdiv <<= 1;
	}

	subdiv = int(Math::sqrt((float)subdiv));

	if (shadow_atlas->quadrants[p_quadrant].subdivision == subdiv) {
		return;
	}

	// Erase all data from quadrant.
	for (int i = 0; i < shadow_atlas->quadrants[p_quadrant].shadows.size(); i++) {
		if (shadow_atlas->quadrants[p_quadrant].shadows[i].owner.is_valid()) {
			shadow_atlas->shadow_owners.erase(shadow_atlas->quadrants[p_quadrant].shadows[i].owner);
			LightInstance* li = light_instance_owner.get_or_null(
				shadow_atlas->quadrants[p_quadrant].shadows[i].owner);
			ERR_CONTINUE(!li);
			li->shadow_atlases.erase(p_atlas);
		}
	}

	for (uint32_t j = 0; j < shadow_atlas->quadrants[p_quadrant].textures.size(); j++) {
		glDeleteTextures(1, &shadow_atlas->quadrants[p_quadrant].textures[j]);
		glDeleteFramebuffers(1, &shadow_atlas->quadrants[p_quadrant].fbos[j]);
	}

	shadow_atlas->quadrants[p_quadrant].textures.clear();
	shadow_atlas->quadrants[p_quadrant].fbos.clear();

	shadow_atlas->quadrants[p_quadrant].shadows.clear();
	shadow_atlas->quadrants[p_quadrant].shadows.resize(subdiv * subdiv);
	shadow_atlas->quadrants[p_quadrant].subdivision = subdiv;

	// Cache the smallest subdiv (for faster allocation in light update).

	shadow_atlas->smallest_subdiv = 1 << 30;

	for (int i = 0; i < 4; i++) {
		if (shadow_atlas->quadrants[i].subdivision) {
			shadow_atlas->smallest_subdiv =
				MIN(shadow_atlas->smallest_subdiv, shadow_atlas->quadrants[i].subdivision);
		}
	}

	if (shadow_atlas->smallest_subdiv == 1 << 30) {
		shadow_atlas->smallest_subdiv = 0;
	}

	// Re-sort the size orders, simple bubblesort for 4 elements.

	int swaps = 0;
	do {
		swaps = 0;

		for (int i = 0; i < 3; i++) {
			if (shadow_atlas->quadrants[shadow_atlas->size_order[i]].subdivision <
				shadow_atlas->quadrants[shadow_atlas->size_order[i + 1]].subdivision) {
				SWAP(shadow_atlas->size_order[i], shadow_atlas->size_order[i + 1]);
				swaps++;
			}
		}
	} while (swaps > 0);
}

bool GLES3::LightStorage::shadow_atlas_update_light(
	RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL_V(shadow_atlas, false);

	LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL_V(li, false);

	if (shadow_atlas->size == 0 || shadow_atlas->smallest_subdiv == 0) {
		return false;
	}

	uint32_t quad_size = shadow_atlas->size >> 1;
	int desired_fit = MIN(quad_size / shadow_atlas->smallest_subdiv,
		Math::next_power_of_2(uint32_t(quad_size * p_coverage)));

	int valid_quadrants[4];
	int valid_quadrant_count = 0;
	int best_size = -1;	  // Best size found.
	int best_subdiv = -1; // Subdiv for the best size.

	// Find the quadrants this fits into, and the best possible size it can fit into.
	for (int i = 0; i < 4; i++) {
		int q = shadow_atlas->size_order[i];
		int sd = shadow_atlas->quadrants[q].subdivision;
		if (sd == 0) {
			continue; // Unused.
		}

		int max_fit = quad_size / sd;

		if (best_size != -1 && max_fit > best_size) {
			break; // Too large.
		}

		valid_quadrants[valid_quadrant_count++] = q;
		best_subdiv = sd;

		if (max_fit >= desired_fit) {
			best_size = max_fit;
		}
	}

	ERR_FAIL_COND_V(valid_quadrant_count == 0, false);

	uint64_t tick = OS::get_ticks_msec();

	uint32_t old_key = SHADOW_INVALID;
	uint32_t old_quadrant = SHADOW_INVALID;
	uint32_t old_shadow = SHADOW_INVALID;
	int old_subdivision = -1;

	bool should_realloc = false;
	bool should_redraw = false;

	if (shadow_atlas->shadow_owners.has(p_light_instance)) {
		old_key = shadow_atlas->shadow_owners[p_light_instance];
		old_quadrant = (old_key >> QUADRANT_SHIFT) & 0x3;
		old_shadow = old_key & SHADOW_INDEX_MASK;

		// Only re-allocate if a better option is available, and enough time has passed.
		should_realloc =
			shadow_atlas->quadrants[old_quadrant].subdivision != (uint32_t)best_subdiv &&
			(tick - shadow_atlas->quadrants[old_quadrant].shadows[old_shadow].alloc_tick >
				shadow_atlas_realloc_tolerance_msec);
		should_redraw =
			shadow_atlas->quadrants[old_quadrant].shadows[old_shadow].version != p_light_version;

		if (!should_realloc) {
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].version =
				p_light_version;
			// Already existing, see if it should redraw or it's just OK.
			return should_redraw;
		}

		old_subdivision = shadow_atlas->quadrants[old_quadrant].subdivision;
	}

	bool is_omni = li->light_type == RSE::LIGHT_OMNI;
	bool found_shadow = false;
	int new_quadrant = -1;
	int new_shadow = -1;

	found_shadow = _shadow_atlas_find_shadow(shadow_atlas, valid_quadrants, valid_quadrant_count,
		old_subdivision, tick, is_omni, new_quadrant, new_shadow);

	// For new shadows if we found an atlas.
	// Or for existing shadows that found a better atlas.
	if (found_shadow) {
		if (old_quadrant != SHADOW_INVALID) {
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].version = 0;
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].owner = RID();
		}

		uint32_t new_key = new_quadrant << QUADRANT_SHIFT;
		new_key |= new_shadow;

		ShadowAtlas::Quadrant::Shadow* sh =
			&shadow_atlas->quadrants[new_quadrant].shadows.write[new_shadow];
		_shadow_atlas_invalidate_shadow(sh, p_atlas, shadow_atlas, new_quadrant, new_shadow);

		sh->owner = p_light_instance;
		sh->owner_is_omni = is_omni;
		sh->alloc_tick = tick;
		sh->version = p_light_version;

		li->shadow_atlases.insert(p_atlas);

		// Update it in map.
		shadow_atlas->shadow_owners[p_light_instance] = new_key;
		// Make it dirty, as it should redraw anyway.
		return true;
	}

	return should_redraw;
}

bool GLES3::LightStorage::_shadow_atlas_find_shadow(ShadowAtlas* shadow_atlas, int* p_in_quadrants,
	int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, bool is_omni, int& r_quadrant,
	int& r_shadow)
{
	for (int i = p_quadrant_count - 1; i >= 0; i--) {
		int qidx = p_in_quadrants[i];

		if (shadow_atlas->quadrants[qidx].subdivision == (uint32_t)p_current_subdiv) {
			return false;
		}

		// Look for an empty space.
		int sc = shadow_atlas->quadrants[qidx].shadows.size();
		const ShadowAtlas::Quadrant::Shadow* sarr = shadow_atlas->quadrants[qidx].shadows.ptr();

		// We have a free space in this quadrant, allocate a texture and use it.
		if (sc > (int)shadow_atlas->quadrants[qidx].textures.size()) {
			GLuint fbo_id = 0;
			glGenFramebuffers(1, &fbo_id);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

			GLuint texture_id = 0;
			glGenTextures(1, &texture_id);
			glActiveTexture(GL_TEXTURE0);

			int size = (shadow_atlas->size >> 1) / shadow_atlas->quadrants[qidx].subdivision;

			GLenum format = shadow_atlas->use_16_bits ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24;
			GLenum type = shadow_atlas->use_16_bits ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

			if (is_omni) {
				glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
				for (int id = 0; id < 6; id++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + id, 0, format, size / 2, size / 2,
						0, GL_DEPTH_COMPONENT, type, nullptr);
				}

				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				glTexParameteri(
					GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X, texture_id, 0);

#ifdef DEBUG_ENABLED
				GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE) {
					ERR_PRINT("Could not create omni light shadow framebuffer, status: " +
							  GLES3::TextureStorage::get_framebuffer_error(status));
				}
#endif
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, texture_id);

				glTexImage2D(
					GL_TEXTURE_2D, 0, format, size, size, 0, GL_DEPTH_COMPONENT, type, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				glFramebufferTexture2D(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id, 0);

				glBindTexture(GL_TEXTURE_2D, 0);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

			r_quadrant = qidx;
			r_shadow = shadow_atlas->quadrants[qidx].textures.size();

			shadow_atlas->quadrants[qidx].textures.push_back(texture_id);
			shadow_atlas->quadrants[qidx].fbos.push_back(fbo_id);

			return true;
		}

		int found_used_idx = -1; // Found existing one, must steal it.
		uint64_t min_pass =
			0; // Pass of the existing one, try to use the least recently used one (LRU fashion).

		for (int j = 0; j < sc; j++) {
			if (sarr[j].owner_is_omni != is_omni) {
				// Existing light instance type doesn't match new light instance type skip.
				continue;
			}

			LightInstance* sli = light_instance_owner.get_or_null(sarr[j].owner);
			if (!sli) {
				// Found a released light instance.
				found_used_idx = j;
				break;
			}

			if (sli->last_scene_pass != RasterizerSceneGLES3::get_scene_pass()) {
				// Was just allocated, don't kill it so soon, wait a bit.
				if (p_tick - sarr[j].alloc_tick < shadow_atlas_realloc_tolerance_msec) {
					continue;
				}

				if (found_used_idx == -1 || sli->last_scene_pass < min_pass) {
					found_used_idx = j;
					min_pass = sli->last_scene_pass;
				}
			}
		}

		if (found_used_idx != -1) {
			r_quadrant = qidx;
			r_shadow = found_used_idx;

			return true;
		}
	}

	return false;
}

void GLES3::LightStorage::_shadow_atlas_invalidate_shadow(ShadowAtlas::Quadrant::Shadow* p_shadow,
	RID p_atlas, ShadowAtlas* p_shadow_atlas, uint32_t p_quadrant, uint32_t p_shadow_idx)
{
	if (p_shadow->owner.is_valid()) {
		LightInstance* sli = light_instance_owner.get_or_null(p_shadow->owner);

		p_shadow_atlas->shadow_owners.erase(p_shadow->owner);
		p_shadow->version = 0;
		p_shadow->owner = RID();
		sli->shadow_atlases.erase(p_atlas);
	}
}

void GLES3::LightStorage::shadow_atlas_update(RID p_atlas)
{
	// Do nothing as there is no shadow atlas texture.
}

/* DIRECTIONAL SHADOW */

// Create if necessary and clear.
void GLES3::LightStorage::update_directional_shadow_atlas()
{
	if (directional_shadow.depth == 0 && directional_shadow.size > 0) {
		glGenFramebuffers(1, &directional_shadow.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, directional_shadow.fbo);

		glGenTextures(1, &directional_shadow.depth);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, directional_shadow.depth);

		GLenum format =
			directional_shadow.use_16_bits ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24;
		GLenum type = directional_shadow.use_16_bits ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

		glTexImage2D(GL_TEXTURE_2D, 0, format, directional_shadow.size, directional_shadow.size, 0,
			GL_DEPTH_COMPONENT, type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, directional_shadow.depth, 0);
	}
	glUseProgram(0);
	glDepthMask(GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, directional_shadow.fbo);
	RasterizerUtilGLES3::clear_depth(0.0);
	glClear(GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

void GLES3::LightStorage::directional_shadow_atlas_set_size(int p_size, bool p_16_bits)
{
	p_size = Math::nearest_power_of_2_templated(p_size);

	if (directional_shadow.size == p_size && directional_shadow.use_16_bits == p_16_bits) {
		return;
	}

	directional_shadow.size = p_size;
	directional_shadow.use_16_bits = p_16_bits;

	if (directional_shadow.depth != 0) {
		glDeleteTextures(1, &directional_shadow.depth);
		directional_shadow.depth = 0;
		glDeleteFramebuffers(1, &directional_shadow.fbo);
		directional_shadow.fbo = 0;
	}
}

void GLES3::LightStorage::set_directional_shadow_count(int p_count)
{
	directional_shadow.light_count = p_count;
	directional_shadow.current_light = 0;
}

static Rect2i _get_directional_shadow_rect(int p_size, int p_shadow_count, int p_shadow_index)
{
	int split_h = 1;
	int split_v = 1;

	while (split_h * split_v < p_shadow_count) {
		if (split_h == split_v) {
			split_h <<= 1;
		}
		else {
			split_v <<= 1;
		}
	}

	Rect2i rect(0, 0, p_size, p_size);
	rect.size.width /= split_h;
	rect.size.height /= split_v;

	rect.position.x = rect.size.width * (p_shadow_index % split_h);
	rect.position.y = rect.size.height * (p_shadow_index / split_h);

	return rect;
}

Rect2i GLES3::LightStorage::get_directional_shadow_rect()
{
	return _get_directional_shadow_rect(
		directional_shadow.size, directional_shadow.light_count, directional_shadow.current_light);
}

int GLES3::LightStorage::get_directional_light_shadow_size(RID p_light_instance)
{
	ERR_FAIL_COND_V(directional_shadow.light_count == 0, 0);

	Rect2i r =
		_get_directional_shadow_rect(directional_shadow.size, directional_shadow.light_count, 0);

	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL_V(light_instance, 0);

	switch (light_directional_get_shadow_mode(light_instance->light)) {
	case RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
		break; // none
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
		r.size.height /= 2;
		break;
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
		r.size /= 2;
		break;
	}

	return MAX(r.size.width, r.size.height);
}

void GLES3::LightStorage::initialize() {}

#endif // !GLES3_ENABLED



// ============================================================================
// Merged from drivers/gles3/storage/material_storage.cpp
// ============================================================================

#include "material_storage.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "drivers/gles3/rasterizer_canvas_gles3.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/variant_converters.h"

using namespace GLES3;

_FORCE_INLINE_ static void _fill_std140_ubo_value(
	ShaderLanguage::DataType type, const Vector<ShaderLanguage::Scalar>& value, uint8_t* data)
{
	switch (type) {
	case ShaderLanguage::TYPE_BOOL: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
	} break;
	case ShaderLanguage::TYPE_BVEC2: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC3: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC4: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;
		gui[3] = value[3].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_INT: {
		int32_t* gui = (int32_t*)data;
		gui[0] = value[0].sint;

	} break;
	case ShaderLanguage::TYPE_IVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_UINT: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].uint;

	} break;
	case ShaderLanguage::TYPE_UVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_UVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].uint;
		}

	} break;
	case ShaderLanguage::TYPE_UVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_FLOAT: {
		float* gui = (float*)data;
		gui[0] = value[0].real;

	} break;
	case ShaderLanguage::TYPE_VEC2: {
		float* gui = (float*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC3: {
		float* gui = (float*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC4: {
		float* gui = (float*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].real;
		}
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		float* gui = (float*)data;

		// in std140 members of mat2 are treated as vec4s
		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = 0;
		gui[3] = 0;
		gui[4] = value[2].real;
		gui[5] = value[3].real;
		gui[6] = 0;
		gui[7] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		float* gui = (float*)data;

		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = value[2].real;
		gui[3] = 0;
		gui[4] = value[3].real;
		gui[5] = value[4].real;
		gui[6] = value[5].real;
		gui[7] = 0;
		gui[8] = value[6].real;
		gui[9] = value[7].real;
		gui[10] = value[8].real;
		gui[11] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		float* gui = (float*)data;

		for (int i = 0; i < 16; i++) {
			gui[i] = value[i].real;
		}
	} break;
	default: {
	}
	}
}

_FORCE_INLINE_ static void _fill_std140_ubo_empty(
	ShaderLanguage::DataType type, int p_array_size, uint8_t* data)
{
	if (p_array_size <= 0) {
		p_array_size = 1;
	}

	switch (type) {
	case ShaderLanguage::TYPE_BOOL:
	case ShaderLanguage::TYPE_INT:
	case ShaderLanguage::TYPE_UINT:
	case ShaderLanguage::TYPE_FLOAT: {
		memset(data, 0, 4 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC2:
	case ShaderLanguage::TYPE_IVEC2:
	case ShaderLanguage::TYPE_UVEC2:
	case ShaderLanguage::TYPE_VEC2: {
		memset(data, 0, 8 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC3:
	case ShaderLanguage::TYPE_IVEC3:
	case ShaderLanguage::TYPE_UVEC3:
	case ShaderLanguage::TYPE_VEC3: {
		memset(data, 0, 12 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC4:
	case ShaderLanguage::TYPE_IVEC4:
	case ShaderLanguage::TYPE_UVEC4:
	case ShaderLanguage::TYPE_VEC4: {
		memset(data, 0, 16 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		memset(data, 0, 32 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		memset(data, 0, 48 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		memset(data, 0, 64 * p_array_size);
	} break;

	default: {
	}
	}
}

void ShaderData::get_instance_param_list(
	List<InstanceShaderParam>* p_param_list) const
{
}

void ShaderData::set_path_hint(const String& p_hint) { path = p_hint; }

void ShaderData::set_default_texture_parameter(const String& p_name, RID p_texture, int p_index)
{
	if (!p_texture.is_valid()) {
		if (default_texture_params.has(p_name) && default_texture_params[p_name].has(p_index)) {
			default_texture_params[p_name].erase(p_index);

			if (default_texture_params[p_name].is_empty()) {
				default_texture_params.erase(p_name);
			}
		}
	}
	else {
		if (!default_texture_params.has(p_name)) {
			default_texture_params[p_name] = HashMap<int, RID>();
		}
		default_texture_params[p_name][p_index] = p_texture;
	}
}

bool ShaderData::is_parameter_texture(const String& p_param) const
{
	if (!uniforms.has(p_param)) {
		return false;
	}

	return uniforms[p_param].is_texture();
}

// Look up table to translate ShaderLanguage::DataType to GL_TEXTURE_*
static const GLenum target_from_type[ShaderLanguage::TYPE_MAX] = {
	GL_TEXTURE_2D,			  // TYPE_VOID,
	GL_TEXTURE_2D,			  // TYPE_BOOL,
	GL_TEXTURE_2D,			  // TYPE_BVEC2,
	GL_TEXTURE_2D,			  // TYPE_BVEC3,
	GL_TEXTURE_2D,			  // TYPE_BVEC4,
	GL_TEXTURE_2D,			  // TYPE_INT,
	GL_TEXTURE_2D,			  // TYPE_IVEC2,
	GL_TEXTURE_2D,			  // TYPE_IVEC3,
	GL_TEXTURE_2D,			  // TYPE_IVEC4,
	GL_TEXTURE_2D,			  // TYPE_UINT,
	GL_TEXTURE_2D,			  // TYPE_UVEC2,
	GL_TEXTURE_2D,			  // TYPE_UVEC3,
	GL_TEXTURE_2D,			  // TYPE_UVEC4,
	GL_TEXTURE_2D,			  // TYPE_FLOAT,
	GL_TEXTURE_2D,			  // TYPE_VEC2,
	GL_TEXTURE_2D,			  // TYPE_VEC3,
	GL_TEXTURE_2D,			  // TYPE_VEC4,
	GL_TEXTURE_2D,			  // TYPE_MAT2,
	GL_TEXTURE_2D,			  // TYPE_MAT3,
	GL_TEXTURE_2D,			  // TYPE_MAT4,
	GL_TEXTURE_2D,			  // TYPE_SAMPLER2D,
	GL_TEXTURE_2D,			  // TYPE_ISAMPLER2D,
	GL_TEXTURE_2D,			  // TYPE_USAMPLER2D,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_SAMPLER2DARRAY,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_ISAMPLER2DARRAY,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_USAMPLER2DARRAY,
	GL_TEXTURE_3D,			  // TYPE_SAMPLER3D,
	GL_TEXTURE_3D,			  // TYPE_ISAMPLER3D,
	GL_TEXTURE_3D,			  // TYPE_USAMPLER3D,
	GL_TEXTURE_CUBE_MAP,	  // TYPE_SAMPLERCUBE,
	GL_TEXTURE_CUBE_MAP,	  // TYPE_SAMPLERCUBEARRAY,
	_GL_TEXTURE_EXTERNAL_OES, // TYPE_SAMPLEREXT
	GL_TEXTURE_2D,			  // TYPE_STRUCT
};

static const RSE::CanvasItemTextureRepeat repeat_from_uniform[ShaderLanguage::REPEAT_DEFAULT + 1] =
	{
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DISABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_ENABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_DEFAULT,
};

static const RSE::CanvasItemTextureRepeat
	repeat_from_uniform_canvas[ShaderLanguage::REPEAT_DEFAULT + 1] = {
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DISABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_ENABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DEFAULT,
};

static const RSE::CanvasItemTextureFilter filter_from_uniform[ShaderLanguage::FILTER_DEFAULT +
															  1] = {
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST, // ShaderLanguage::TextureFilter::FILTER_NEAREST,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_LINEAR,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_DEFAULT,
};

static const RSE::CanvasItemTextureFilter filter_from_uniform_canvas[ShaderLanguage::
																		 FILTER_DEFAULT +
																	 1] = {
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST, // ShaderLanguage::TextureFilter::FILTER_NEAREST,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_LINEAR,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_DEFAULT,
};

MaterialData::~MaterialData()
{
	if (global_buffer_E) {
		// unregister global buffers
		MaterialStorage::global_shader_uniforms.materials_using_buffer.erase(global_buffer_E);
	}

	if (global_texture_E) {
		// unregister global textures

		for (const KeyValue<String, uint64_t>& E : used_global_textures) {
			GlobalShaderUniforms::Variable* v =
				MaterialStorage::global_shader_uniforms.variables.getptr(E.key);
			if (v) {
				v->texture_materials.erase(self);
			}
		}
		// unregister material from those using global textures
		MaterialStorage::global_shader_uniforms.materials_using_texture.erase(global_texture_E);
	}

	if (uniform_buffer) {
		glDeleteBuffers(1, &uniform_buffer);
		uniform_buffer = 0;
	}
}

RID MaterialData::get_default_texture_id(
	ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint)
{
	RID gl_texture;

	switch (p_type) {
	case ShaderLanguage::TYPE_ISAMPLER2D:
	case ShaderLanguage::TYPE_USAMPLER2D:
	case ShaderLanguage::TYPE_SAMPLER2D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_TRANSPARENT);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ANISOTROPY: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_ANISO);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_NORMAL);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_NORMAL);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_SAMPLERCUBE: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture =
				GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_WHITE);
		} break;
		}
	} break;
	case ShaderLanguage::TYPE_SAMPLERCUBEARRAY: {
		ERR_PRINT_ONCE("Type: SamplerCubeArray is not supported in the Compatibility renderer, "
					   "please use another type.");
	} break;
	case ShaderLanguage::TYPE_SAMPLEREXT: {
		gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_EXT);
	} break;

	case ShaderLanguage::TYPE_ISAMPLER3D:
	case ShaderLanguage::TYPE_USAMPLER3D:
	case ShaderLanguage::TYPE_SAMPLER3D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
	case ShaderLanguage::TYPE_USAMPLER2DARRAY:
	case ShaderLanguage::TYPE_SAMPLER2DARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture =
				GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_WHITE);
		} break;
		}
	} break;

	default: {
	}
	}

	return gl_texture;
}

int32_t GLES3::MaterialStorage::_global_shader_uniform_allocate(uint32_t p_elements)
{
	int32_t idx = 0;
	while (idx + p_elements <= global_shader_uniforms.buffer_size) {
		if (global_shader_uniforms.buffer_usage[idx].elements == 0) {
			bool valid = true;
			for (uint32_t i = 1; i < p_elements; i++) {
				if (global_shader_uniforms.buffer_usage[idx + i].elements > 0) {
					valid = false;
					idx += i + global_shader_uniforms.buffer_usage[idx + i].elements;
					break;
				}
			}

			if (!valid) {
				continue; // if not valid, idx is in new position
			}

			return idx;
		}
		else {
			idx += global_shader_uniforms.buffer_usage[idx].elements;
		}
	}

	return -1;
}

void GLES3::MaterialStorage::_global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements)
{
	int32_t prev_chunk = -1;

	for (int32_t i = 0; i < p_elements; i++) {
		int32_t chunk = (p_index + i) / GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;
		if (chunk != prev_chunk) {
			if (!global_shader_uniforms.buffer_dirty_regions[chunk]) {
				global_shader_uniforms.buffer_dirty_regions[chunk] = true;
				global_shader_uniforms.buffer_dirty_region_count++;
			}
		}

		prev_chunk = chunk;
	}
}

void GLES3::MaterialStorage::global_shader_parameter_remove(const String& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return;
	}
	GlobalShaderUniforms::Variable& gv = global_shader_uniforms.variables[p_name];

	if (gv.buffer_index >= 0) {
		global_shader_uniforms.buffer_usage[gv.buffer_index].elements = 0;
		global_shader_uniforms.must_update_buffer_materials = true;
	}
	else {
		global_shader_uniforms.must_update_texture_materials = true;
	}

	global_shader_uniforms.variables.erase(p_name);
}

Vector<String> GLES3::MaterialStorage::global_shader_parameter_get_list()
{
	if (!Engine::is_editor_hint() &&
		!Engine::is_project_manager_hint()) {
		ERR_FAIL_V_MSG(Vector<String>(), "This function should never be used outside the "
											 "editor, it can severely damage performance.");
	}

	Vector<String> names;
	for (const KeyValue<String, GlobalShaderUniforms::Variable>& E :
		global_shader_uniforms.variables) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

RSE::GlobalShaderParameterType GLES3::MaterialStorage::global_shader_parameter_get_type_internal(
	const String& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return RSE::GLOBAL_VAR_TYPE_MAX;
	}

	return global_shader_uniforms.variables[p_name].type;
}

RSE::GlobalShaderParameterType GLES3::MaterialStorage::global_shader_parameter_get_type(
	const String& p_name)
{
	if (!Engine::is_editor_hint()) {
		ERR_FAIL_V_MSG(RSE::GLOBAL_VAR_TYPE_MAX, "This function should never be used outside the "
												 "editor, it can severely damage performance.");
	}

	return global_shader_parameter_get_type_internal(p_name);
}

void GLES3::MaterialStorage::global_shader_parameters_clear() { global_shader_uniforms.variables.clear(); }

GLuint GLES3::MaterialStorage::global_shader_parameters_get_uniform_buffer()
{
	return global_shader_uniforms.buffer;
}

int32_t GLES3::MaterialStorage::global_shader_parameters_instance_allocate(RID p_instance)
{
	ERR_FAIL_COND_V(global_shader_uniforms.instance_buffer_pos.has(p_instance), -1);
	int32_t pos = _global_shader_uniform_allocate(ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES);
	global_shader_uniforms.instance_buffer_pos[p_instance] = pos; // save anyway
	ERR_FAIL_COND_V_MSG(pos < 0, -1,
		vformat("Too many instances using shader instance variables. Consider increasing "
				"rendering/limits/global_shader_variables/buffer_size in the Project Settings. "
				"Maximum items supported by this hardware is: %d.",
			Config::max_uniform_buffer_size / sizeof(GlobalShaderUniforms::Value)));
	global_shader_uniforms.buffer_usage[pos].elements =
		ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES;
	return pos;
}

void GLES3::MaterialStorage::global_shader_parameters_instance_free(RID p_instance)
{
	ERR_FAIL_COND(!global_shader_uniforms.instance_buffer_pos.has(p_instance));
	int32_t pos = global_shader_uniforms.instance_buffer_pos[p_instance];
	if (pos >= 0) {
		global_shader_uniforms.buffer_usage[pos].elements = 0;
	}
	global_shader_uniforms.instance_buffer_pos.erase(p_instance);
}

void GLES3::MaterialStorage::_update_global_shader_uniforms()
{
	if (global_shader_uniforms.buffer_dirty_region_count > 0) {
		uint32_t total_regions = 1 + (global_shader_uniforms.buffer_size /
										 GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE);
		if (total_regions / global_shader_uniforms.buffer_dirty_region_count <= 4) {
			// 25% of regions dirty, just update all buffer
			glBindBuffer(GL_UNIFORM_BUFFER, global_shader_uniforms.buffer);
			glBufferData(GL_UNIFORM_BUFFER,
				sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size,
				global_shader_uniforms.buffer_values, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			memset(global_shader_uniforms.buffer_dirty_regions, 0, sizeof(bool) * total_regions);
		}
		else {
			uint32_t region_byte_size = sizeof(GlobalShaderUniforms::Value) *
										GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;
			glBindBuffer(GL_UNIFORM_BUFFER, global_shader_uniforms.buffer);
			for (uint32_t i = 0; i < total_regions; i++) {
				if (global_shader_uniforms.buffer_dirty_regions[i]) {
					glBufferSubData(GL_UNIFORM_BUFFER, i * region_byte_size, region_byte_size,
						&global_shader_uniforms
							 .buffer_values[i * GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE]);
					global_shader_uniforms.buffer_dirty_regions[i] = false;
				}
			}
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		global_shader_uniforms.buffer_dirty_region_count = 0;
	}

	if (global_shader_uniforms.must_update_buffer_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_buffer) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, true, false);
		}

		global_shader_uniforms.must_update_buffer_materials = false;
	}

	if (global_shader_uniforms.must_update_texture_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_texture) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, false, true);
		}

		global_shader_uniforms.must_update_texture_materials = false;
	}
}

RID GLES3::MaterialStorage::shader_allocate() { return shader_owner.allocate_rid(); }

void GLES3::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	Shader shader;
	shader.data = nullptr;
	shader.mode = RSE::SHADER_MAX;

	shader_owner.initialize_rid(p_rid, shader);
}

void GLES3::MaterialStorage::shader_free(RID p_rid)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	// make material unreference this
	while (shader->owners.size()) {
		material_set_shader((*shader->owners.begin())->self, RID());
	}

	// clear data if exists
	memdelete(shader->data);
	shader_owner.free(p_rid);
}

void GLES3::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->code = p_code;

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
		/*
		}
		else if (mode_string == "fog") {
			new_mode = RSE::SHADER_FOG;
			*/
	}
	else if (mode_string == "texture_blit") {
		new_mode = RSE::SHADER_TEXTURE_BLIT;
	}
	else {
		new_mode = RSE::SHADER_MAX;
		ERR_PRINT("shader type " + mode_string + " not supported in OpenGL renderer");
	}

	if (new_mode != shader->mode) {
		if (shader->data) {
			memdelete(shader->data);
			shader->data = nullptr;
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			material->shader_mode = new_mode;
			if (material->data) {
				memdelete(material->data);
				material->data = nullptr;
			}
		}

		shader->mode = new_mode;

		if (new_mode < RSE::SHADER_MAX) {
			shader->data = memnew(ShaderData(new_mode));
		}
		else {
			shader->mode = RSE::SHADER_MAX; // invalid
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			if (shader->data) {
				material->data = memnew(MaterialData(shader->data));
				material->data->self = material->self;
				material->data->set_next_pass(material->next_pass);
				material->data->set_render_priority(material->priority);
			}
			material->shader_mode = new_mode;
		}

		if (shader->data) {
			for (const KeyValue<String, HashMap<int, RID>>& E :
				shader->default_texture_parameter) {
				for (const KeyValue<int, RID>& E2 : E.value) {
					shader->data->set_default_texture_parameter(E.key, E2.value, E2.key);
				}
			}
		}
	}

	if (shader->data) {
		shader->data->set_path_hint(shader->path_hint);
		shader->data->set_code(p_code);
	}

	for (Material* E : shader->owners) {
		Material* material = E;
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		_material_queue_update(material, true, true);
	}
}

void GLES3::MaterialStorage::shader_set_path_hint(RID p_shader, const String& p_path)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->path_hint = p_path;
	if (shader->data) {
		shader->data->set_path_hint(p_path);
	}
}

String GLES3::MaterialStorage::shader_get_code(RID p_shader)
{
	const GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, String());
	return shader->code;
}

void GLES3::MaterialStorage::shader_set_default_texture_parameter(
	RID p_shader, const String& p_name, RID p_texture, int p_index)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	if (p_texture.is_valid() && TextureStorage::owns_texture(p_texture)) {
		if (!shader->default_texture_parameter.has(p_name)) {
			shader->default_texture_parameter[p_name] = HashMap<int, RID>();
		}
		shader->default_texture_parameter[p_name][p_index] = p_texture;
	}
	else {
		if (shader->default_texture_parameter.has(p_name) &&
			shader->default_texture_parameter[p_name].has(p_index)) {
			shader->default_texture_parameter[p_name].erase(p_index);

			if (shader->default_texture_parameter[p_name].is_empty()) {
				shader->default_texture_parameter.erase(p_name);
			}
		}
	}
	if (shader->data) {
		shader->data->set_default_texture_parameter(p_name, p_texture, p_index);
	}
	for (Material* E : shader->owners) {
		Material* material = E;
		_material_queue_update(material, false, true);
	}
}

RID GLES3::MaterialStorage::shader_get_default_texture_parameter(
	RID p_shader, const String& p_name, int p_index)
{
	const GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());
	if (shader->default_texture_parameter.has(p_name) &&
		shader->default_texture_parameter[p_name].has(p_index)) {
		return shader->default_texture_parameter[p_name][p_index];
	}

	return RID();
}

RenderingServerTypes::ShaderNativeSourceCode GLES3::MaterialStorage::shader_get_native_source_code(
	RID p_shader)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RenderingServerTypes::ShaderNativeSourceCode());
	if (shader->data) {
		return shader->data->get_native_source_code();
	}
	return RenderingServerTypes::ShaderNativeSourceCode();
}

void GLES3::MaterialStorage::_material_queue_update(
	GLES3::Material* material, bool p_uniform, bool p_texture)
{
	material->uniform_dirty = material->uniform_dirty || p_uniform;
	material->texture_dirty = material->texture_dirty || p_texture;

	if (material->update_element.in_list()) {
		return;
	}

	material_update_list.add(&material->update_element);
}

void GLES3::MaterialStorage::_update_queued_materials()
{
	while (material_update_list.first()) {
		Material* material = material_update_list.first()->self();
		material->texture_dirty = false;
		material->uniform_dirty = false;

		material_update_list.remove(&material->update_element);
	}
}

RID GLES3::MaterialStorage::material_allocate() { return material_owner.allocate_rid(); }

void GLES3::MaterialStorage::material_initialize(RID p_rid)
{
	material_owner.initialize_rid(p_rid);
	Material* material = material_owner.get_or_null(p_rid);
	material->self = p_rid;
}

void GLES3::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->data) {
		memdelete(material->data);
		material->data = nullptr;
	}

	if (material->shader) {
		material->shader->owners.erase(material);
		material->shader = nullptr;
		material->shader_mode = RSE::SHADER_MAX;
	}

	if (p_shader.is_null()) {
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		material->shader_id = 0;
		return;
	}

	Shader* shader = get_shader(p_shader);
	ERR_FAIL_NULL(shader);
	material->shader = shader;
	material->shader_mode = shader->mode;
	material->shader_id = p_shader.get_local_index();
	shader->owners.insert(material);

	if (shader->mode == RSE::SHADER_MAX) {
		return;
	}

	ERR_FAIL_NULL(shader->data);

	material->data = memnew(MaterialData(shader->data));
	material->data->self = p_material;
	material->data->set_next_pass(material->next_pass);
	material->data->set_render_priority(material->priority);
	// updating happens later
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	_material_queue_update(material, true, true);
}

void GLES3::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->next_pass == p_next_material) {
		return;
	}

	material->next_pass = p_next_material;
	if (material->data) {
		material->data->set_next_pass(p_next_material);
	}

	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

void GLES3::MaterialStorage::material_set_render_priority(RID p_material, int priority)
{
	ERR_FAIL_COND(priority < RSE::MATERIAL_RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);

	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	material->priority = priority;
	if (material->data) {
		material->data->set_render_priority(priority);
	}
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

bool GLES3::MaterialStorage::material_is_animated(RID p_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, false);
	if (material->shader && material->shader->data) {
		if (material->shader->data->is_animated()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_is_animated(material->next_pass);
		}
	}
	return false; // by default nothing is animated
}

bool GLES3::MaterialStorage::material_casts_shadows(RID p_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, true);
	if (material->shader && material->shader->data) {
		if (material->shader->data->casts_shadows()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_casts_shadows(material->next_pass);
		}
	}
	return true; // by default everything casts shadows
}

RSE::CullMode GLES3::MaterialStorage::material_get_cull_mode(RID p_material)
{
	const GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, RSE::CULL_MODE_DISABLED);
	ERR_FAIL_NULL_V(material->shader, RSE::CULL_MODE_DISABLED);
	if (material->shader->data && material->shader->data->mode == RSE::SHADER_SPATIAL) {
		return material->shader->data->cull_mode;
	}
	return RSE::CULL_MODE_DISABLED;
}

void GLES3::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	if (material->shader && material->shader->data) {
		material->shader->data->get_instance_param_list(r_parameters);

		if (material->next_pass.is_valid()) {
			material_get_instance_shader_parameters(material->next_pass, r_parameters);
		}
	}
}

void GLES3::MaterialStorage::material_update_dependency(RID p_material, DependencyTracker* p_instance)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	p_instance->update_dependency(&material->dependency);
	if (material->next_pass.is_valid()) {
		material_update_dependency(material->next_pass, p_instance);
	}
}

LocalVector<ShaderGLES3::TextureUniformData> get_texture_uniform_data(
	const Vector<ShaderCompiler::GeneratedCode::Texture>& texture_uniforms)
{
	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data;
	for (int i = 0; i < texture_uniforms.size(); i++) {
		int num_textures = texture_uniforms[i].array_size;
		if (num_textures == 0) {
			num_textures = 1;
		}
		texture_uniform_data.push_back({texture_uniforms[i].name, num_textures});
	}
	return texture_uniform_data;
}

void ShaderData::_set_code_canvas(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_screen_texture = false;
	uses_screen_texture_mipmaps = false;
	uses_sdf = false;
	uses_time = false;
	uses_custom0 = false;
	uses_custom1 = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enum set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["vertex"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["fragment"] = ShaderCompiler::STAGE_FRAGMENT;
	actions.entry_point_stages["light"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_premul_alpha"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_PMALPHA);
	actions.render_mode_values["blend_disabled"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_DISABLED);

	actions.usage_flag_pointers["texture_sdf"] = &uses_sdf;
	actions.usage_flag_pointers["texture_sdf_normal"] = &uses_sdf;
	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["CUSTOM0"] = &uses_custom0;
	actions.usage_flag_pointers["CUSTOM1"] = &uses_custom1;

	actions.uniforms = &uniforms;
	Error err = MaterialStorage::shaders->compiler_canvas.compile(
		RSE::SHADER_CANVAS_ITEM, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->canvas_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);
	uses_screen_texture = gen_code.uses_screen_texture;
	uses_screen_texture_mipmaps = gen_code.uses_screen_texture_mipmaps;

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->canvas_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->canvas_shader.version_is_valid(version));

	vertex_input_mask =
		RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_COLOR | RSE::ARRAY_FORMAT_TEX_UV;
	vertex_input_mask |= uses_custom0 << RSE::ARRAY_CUSTOM0;
	vertex_input_mask |= uses_custom1 << RSE::ARRAY_CUSTOM1;

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

static void bind_uniforms_generic(const Vector<RID>& p_textures,
	const Vector<ShaderCompiler::GeneratedCode::Texture>& p_texture_uniforms,
	int texture_offset = 0,
	const RSE::CanvasItemTextureFilter* filter_mapping = filter_from_uniform,
	const RSE::CanvasItemTextureRepeat* repeat_mapping = repeat_from_uniform)
{
	const RID* textures = p_textures.ptr();
	const ShaderCompiler::GeneratedCode::Texture* texture_uniforms = p_texture_uniforms.ptr();
	int texture_uniform_index = 0;
	int texture_uniform_count = 0;
	for (int ti = 0; ti < p_textures.size(); ti++) {
		ERR_FAIL_COND_MSG(texture_uniform_index >= p_texture_uniforms.size(),
			"texture_uniform_index out of bounds");
		GLES3::Texture* texture = GLES3::TextureStorage::get_texture(textures[ti]);
		const ShaderCompiler::GeneratedCode::Texture& texture_uniform =
			texture_uniforms[texture_uniform_index];
		if (texture) {
			glActiveTexture(GL_TEXTURE0 + texture_offset + ti);
			GLenum target = target_from_type[texture_uniform.type];
			if (target == _GL_TEXTURE_EXTERNAL_OES && !GLES3::Config::external_texture_supported) {
				target = GL_TEXTURE_2D;
			}
			glBindTexture(target, texture->tex_id);
			if (texture->render_target) {
				texture->render_target->used_in_frame = true;
			}

			texture->gl_set_filter(filter_mapping[int(texture_uniform.filter)]);
			texture->gl_set_repeat(repeat_mapping[int(texture_uniform.repeat)]);
		}
		texture_uniform_count++;
		if (texture_uniform_count >= texture_uniform.array_size) {
			texture_uniform_index++;
			texture_uniform_count = 0;
		}
	}
}

void ShaderData::_set_code_sky(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_time = false;
	uses_position = false;
	uses_half_res = false;
	uses_quarter_res = false;
	uses_light = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["sky"
] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_flags["use_half_res_pass"] = &uses_half_res;
	actions.render_mode_flags["use_quarter_res_pass"] = &uses_quarter_res;

	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["POSITION"] = &uses_position;
	actions.usage_flag_pointers["LIGHT0_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_SIZE"] = &uses_light;

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_sky.compile(
		RSE::SHADER_SKY, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->sky_shader.version_create();
	}

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->sky_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(!MaterialStorage::shaders->sky_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

void ShaderData::_set_code_scene(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_point_size = false;
	uses_alpha = false;
	uses_alpha = false;
	uses_blend_alpha = false;
	uses_depth_prepass_alpha = false;
	uses_discard = false;
	uses_roughness = false;
	uses_normal = false;
	uses_particle_trails = false;
	wireframe = false;

	unshaded = false;
	uses_vertex = false;
	uses_position = false;
	uses_sss = false;
	uses_transmittance = false;
	uses_screen_texture = false;
	uses_screen_texture_mipmaps = false;
	uses_depth_texture = false;
	uses_normal_texture = false;
	uses_bent_normal_texture = false;
	uses_time = false;
	uses_vertex_time = false;
	uses_fragment_time = false;
	writes_modelview_or_projection = false;
	uses_world_coordinates = false;
	uses_tangent = false;
	writes_tangent = false;
	uses_color = false;
	uses_uv = false;
	uses_uv2 = false;
	uses_custom0 = false;
	uses_custom1 = false;
	uses_custom2 = false;
	uses_custom3 = false;
	uses_bones = false;
	uses_weights = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enums set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;
	int depth_test_disabledi = 0;
	int depth_test_invertedi = 0;
	int alpha_antialiasing_modei = ALPHA_ANTIALIASING_OFF;
	int cull_modei = RSE::CULL_MODE_BACK;
	int depth_drawi = DEPTH_DRAW_OPAQUE;

	int stencil_readi = 0;
	int stencil_writei = 0;
	int stencil_write_depth_faili = 0;
	int stencil_comparei = STENCIL_COMPARE_ALWAYS;
	int stencil_referencei = -1;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["vertex"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["fragment"] = ShaderCompiler::STAGE_FRAGMENT;
	actions.entry_point_stages["light"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_premul_alpha"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_PREMULT_ALPHA);

	actions.render_mode_values["alpha_to_coverage"] =
		Pair<int*, int>(&alpha_antialiasing_modei, ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE);
	actions.render_mode_values["alpha_to_coverage_and_one"] =
		Pair<int*, int>(&alpha_antialiasing_modei, ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE);

	actions.render_mode_values["depth_draw_never"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_DISABLED);
	actions.render_mode_values["depth_draw_opaque"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_OPAQUE);
	actions.render_mode_values["depth_draw_always"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_ALWAYS);

	actions.render_mode_values["depth_test_disabled"] = Pair<int*, int>(&depth_test_disabledi, 1);
	actions.render_mode_values["depth_test_inverted"] = Pair<int*, int>(&depth_test_invertedi, 1);

	actions.render_mode_values["cull_disabled"] =
		Pair<int*, int>(&cull_modei, RSE::CULL_MODE_DISABLED);
	actions.render_mode_values["cull_front"] = Pair<int*, int>(&cull_modei, RSE::CULL_MODE_FRONT);
	actions.render_mode_values["cull_back"] = Pair<int*, int>(&cull_modei, RSE::CULL_MODE_BACK);

	actions.render_mode_flags["unshaded"] = &unshaded;
	actions.render_mode_flags["wireframe"] = &wireframe;
	actions.render_mode_flags["particle_trails"] = &uses_particle_trails;
	actions.render_mode_flags["world_vertex_coords"] = &uses_world_coordinates;

	actions.usage_flag_pointers["ALPHA"] = &uses_alpha;
	actions.usage_flag_pointers["ALPHA_SCISSOR_THRESHOLD"] = &uses_alpha;
	// Use alpha clip pipeline for alpha hash/dither.
	// This prevents sorting issues inherent to alpha blending and allows such materials to cast
	// shadows.
	actions.usage_flag_pointers["ALPHA_HASH_SCALE"] = &uses_alpha;
	actions.render_mode_flags["depth_prepass_alpha"] = &uses_depth_prepass_alpha;

	actions.usage_flag_pointers["SSS_STRENGTH"] = &uses_sss;
	actions.usage_flag_pointers["SSS_TRANSMITTANCE_DEPTH"] = &uses_transmittance;

	actions.usage_flag_pointers["DISCARD"] = &uses_discard;
	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["ROUGHNESS"] = &uses_roughness;
	actions.usage_flag_pointers["NORMAL"] = &uses_normal;
	actions.usage_flag_pointers["NORMAL_MAP"] = &uses_normal;
	actions.usage_flag_pointers["BENT_NORMAL_MAP"] = &uses_bent_normal_texture;

	actions.usage_flag_pointers["POINT_SIZE"] = &uses_point_size;
	actions.usage_flag_pointers["POINT_COORD"] = &uses_point_size;

	actions.write_flag_pointers["MODELVIEW_MATRIX"] = &writes_modelview_or_projection;
	actions.write_flag_pointers["PROJECTION_MATRIX"] = &writes_modelview_or_projection;
	actions.write_flag_pointers["VERTEX"] = &uses_vertex;
	actions.write_flag_pointers["POSITION"] = &uses_position;
	actions.write_flag_pointers["TANGENT"] = &writes_tangent;
	actions.write_flag_pointers["BINORMAL"] = &writes_tangent;

	actions.usage_flag_pointers["TANGENT"] = &uses_tangent;
	actions.usage_flag_pointers["BINORMAL"] = &uses_tangent;
	actions.usage_flag_pointers["ANISOTROPY"] = &uses_tangent;
	actions.usage_flag_pointers["ANISOTROPY_FLOW"] = &uses_tangent;
	actions.usage_flag_pointers["COLOR"] = &uses_color;
	actions.usage_flag_pointers["UV"] = &uses_uv;
	actions.usage_flag_pointers["UV2"] = &uses_uv2;
	actions.usage_flag_pointers["CUSTOM0"] = &uses_custom0;
	actions.usage_flag_pointers["CUSTOM1"] = &uses_custom1;
	actions.usage_flag_pointers["CUSTOM2"] = &uses_custom2;
	actions.usage_flag_pointers["CUSTOM3"] = &uses_custom3;
	actions.usage_flag_pointers["BONE_INDICES"] = &uses_bones;
	actions.usage_flag_pointers["BONE_WEIGHTS"] = &uses_weights;

	actions.stencil_mode_values["read"] = Pair<int*, int>(&stencil_readi, STENCIL_FLAG_READ);
	actions.stencil_mode_values["write"] = Pair<int*, int>(&stencil_writei, STENCIL_FLAG_WRITE);
	actions.stencil_mode_values["write_depth_fail"] =
		Pair<int*, int>(&stencil_write_depth_faili, STENCIL_FLAG_WRITE_DEPTH_FAIL);

	actions.stencil_mode_values["compare_less"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_LESS);
	actions.stencil_mode_values["compare_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_EQUAL);
	actions.stencil_mode_values["compare_less_or_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_LESS_OR_EQUAL);
	actions.stencil_mode_values["compare_greater"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_GREATER);
	actions.stencil_mode_values["compare_not_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_NOT_EQUAL);
	actions.stencil_mode_values["compare_greater_or_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_GREATER_OR_EQUAL);
	actions.stencil_mode_values["compare_always"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_ALWAYS);

	actions.stencil_reference = &stencil_referencei;

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_scene.compile(
		RSE::SHADER_SPATIAL, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->scene_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);
	alpha_antialiasing_mode = AlphaAntiAliasing(alpha_antialiasing_modei);
	depth_draw = DepthDraw(depth_drawi);
	if (depth_test_disabledi) {
		depth_test = DEPTH_TEST_DISABLED;
	}
	else if (depth_test_invertedi) {
		depth_test = DEPTH_TEST_ENABLED_INVERTED;
	}
	else {
		depth_test = DEPTH_TEST_ENABLED;
	}
	cull_mode = RSE::CullMode(cull_modei);

	vertex_input_mask = RSE::ARRAY_FORMAT_VERTEX |
						RSE::ARRAY_FORMAT_NORMAL; // We can always read vertices and normals.
	vertex_input_mask |= uses_tangent << RSE::ARRAY_TANGENT;
	vertex_input_mask |= uses_color << RSE::ARRAY_COLOR;
	vertex_input_mask |= uses_uv << RSE::ARRAY_TEX_UV;
	vertex_input_mask |= uses_uv2 << RSE::ARRAY_TEX_UV2;
	vertex_input_mask |= uses_custom0 << RSE::ARRAY_CUSTOM0;
	vertex_input_mask |= uses_custom1 << RSE::ARRAY_CUSTOM1;
	vertex_input_mask |= uses_custom2 << RSE::ARRAY_CUSTOM2;
	vertex_input_mask |= uses_custom3 << RSE::ARRAY_CUSTOM3;
	vertex_input_mask |= uses_bones << RSE::ARRAY_BONES;
	vertex_input_mask |= uses_weights << RSE::ARRAY_WEIGHTS;

	uses_screen_texture = gen_code.uses_screen_texture;
	uses_screen_texture_mipmaps = gen_code.uses_screen_texture_mipmaps;
	uses_depth_texture = gen_code.uses_depth_texture;
	uses_normal_texture = gen_code.uses_normal_roughness_texture;
	uses_vertex_time = gen_code.uses_vertex_time;
	uses_fragment_time = gen_code.uses_fragment_time;

	stencil_enabled = stencil_referencei != -1;
	stencil_flags = stencil_readi | stencil_writei | stencil_write_depth_faili;
	stencil_compare = StencilCompare(stencil_comparei);
	stencil_reference = stencil_referencei;

#ifdef DEBUG_ENABLED
	if (uses_particle_trails) {
		WARN_PRINT_ONCE_ED(
			"Particle trails are only available when using the Forward+ or Mobile renderer.");
	}

	if (uses_sss) {
		WARN_PRINT_ONCE_ED(
			"Subsurface scattering is only available when using the Forward+ renderer.");
	}

	if (uses_transmittance) {
		WARN_PRINT_ONCE_ED("Transmittance is only available when using the Forward+ renderer.");
	}

	if (uses_normal_texture) {
		WARN_PRINT_ONCE_ED("Reading from the normal-roughness texture is only available when using "
						   "the Forward+ or Mobile renderer.");
	}
#endif

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->scene_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->scene_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	// If any form of Alpha Antialiasing is enabled, set the blend mode to alpha to coverage.
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF) {
		blend_mode = BLEND_MODE_ALPHA_TO_COVERAGE;
	}

	if (blend_mode == BLEND_MODE_ADD || blend_mode == BLEND_MODE_SUB ||
		blend_mode == BLEND_MODE_MUL) {
		uses_blend_alpha = true; // Force alpha used because of blend.
	}

	valid = true;
}

void ShaderData::_set_code_particles(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_collision = false;
	uses_time = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["start"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["process"] = ShaderCompiler::STAGE_VERTEX;

	actions.usage_flag_pointers["COLLIDED"] = &uses_collision;

	userdata_count = 0;
	for (uint32_t i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
		userdatas_used[i] = false;
		actions.usage_flag_pointers["USERDATA" + itos(i + 1)] = &userdatas_used[i];
	}

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_particles.compile(
		RSE::SHADER_PARTICLES, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version =
			MaterialStorage::shaders->particles_process_shader.version_create();
	}

	for (uint32_t i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
		if (userdatas_used[i]) {
			userdata_count++;
		}
	}

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->particles_process_shader.version_set_code(version,
		gen_code.code, gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->particles_process_shader.version_is_valid(
			version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

void ShaderData::_set_code_tex_blit(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enum set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["blit"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_disabled"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_DISABLED);

	actions.uniforms = &uniforms;
	Error err = MaterialStorage::shaders->compiler_tex_blit.compile(
		RSE::SHADER_TEXTURE_BLIT, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->tex_blit_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->tex_blit_shader.version_set_code(version,
		gen_code.code, gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->tex_blit_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;
	valid = true;
}

void ShaderData::set_code(const String& p_code)
{
	switch (mode) {
	case RSE::SHADER_CANVAS_ITEM:
		_set_code_canvas(p_code);
		break;
	case RSE::SHADER_SKY:
		_set_code_sky(p_code);
		break;
	case RSE::SHADER_SPATIAL:
		_set_code_scene(p_code);
		break;
	case RSE::SHADER_PARTICLES:
		_set_code_particles(p_code);
		break;
	case RSE::SHADER_TEXTURE_BLIT:
		_set_code_tex_blit(p_code);
		break;
	default:
		break;
	}
}

bool ShaderData::is_animated() const
{
	if (mode == RSE::SHADER_SPATIAL) {
		return (uses_fragment_time && uses_discard) || (uses_vertex_time && uses_vertex);
	}
	return false;
}

bool ShaderData::casts_shadows() const
{
	if (mode != RSE::SHADER_SPATIAL) {
		return false;
	}
	bool has_read_screen_alpha = uses_screen_texture || uses_depth_texture || uses_normal_texture;
	bool has_base_alpha = (uses_alpha && !uses_alpha) || has_read_screen_alpha;
	bool has_alpha = has_base_alpha || uses_blend_alpha;

	return !has_alpha || (uses_depth_prepass_alpha && !(depth_draw == DEPTH_DRAW_DISABLED ||
														  depth_test != DEPTH_TEST_ENABLED));
}

RenderingServerTypes::ShaderNativeSourceCode ShaderData::get_native_source_code() const
{
	switch (mode) {
	case RSE::SHADER_CANVAS_ITEM:
		return MaterialStorage::shaders->canvas_shader.version_get_native_source_code(version);
	case RSE::SHADER_SKY:
		return MaterialStorage::shaders->sky_shader.version_get_native_source_code(version);
	case RSE::SHADER_SPATIAL:
		return MaterialStorage::shaders->scene_shader.version_get_native_source_code(version);
	case RSE::SHADER_PARTICLES:
		return MaterialStorage::shaders->particles_process_shader.version_get_native_source_code(version);
	case RSE::SHADER_TEXTURE_BLIT:
		return MaterialStorage::shaders->tex_blit_shader.version_get_native_source_code(version);
	default:
		return RenderingServerTypes::ShaderNativeSourceCode();
	}
}

ShaderData::~ShaderData()
{
	if (!version.is_valid()) {
		return;
	}
	switch (mode) {
	case RSE::SHADER_CANVAS_ITEM:
		MaterialStorage::shaders->canvas_shader.version_free(version);
		break;
	case RSE::SHADER_SKY:
		MaterialStorage::shaders->sky_shader.version_free(version);
		break;
	case RSE::SHADER_SPATIAL:
		MaterialStorage::shaders->scene_shader.version_free(version);
		break;
	case RSE::SHADER_PARTICLES:
		MaterialStorage::shaders->particles_process_shader.version_free(version);
		break;
	case RSE::SHADER_TEXTURE_BLIT:
		MaterialStorage::shaders->tex_blit_shader.version_free(version);
		break;
	default:
		break;
	}
}

void MaterialData::set_render_priority(int p_priority)
{
	if (shader_data && shader_data->mode == RSE::SHADER_SPATIAL) {
		priority = p_priority - RSE::MATERIAL_RENDER_PRIORITY_MIN; // 8 bits
	}
}

void MaterialData::set_next_pass(RID p_pass)
{
	if (shader_data && shader_data->mode == RSE::SHADER_SPATIAL) {
		next_pass = p_pass;
	}
}

void MaterialData::bind_uniforms()
{
	if (!shader_data) {
		return;
	}
	switch (shader_data->mode) {
	case RSE::SHADER_CANVAS_ITEM:
		glBindBufferBase(
			GL_UNIFORM_BUFFER, RasterizerCanvasGLES3::MATERIAL_UNIFORM_LOCATION, uniform_buffer);
		bind_uniforms_generic(texture_cache, shader_data->texture_uniforms, 1,
			filter_from_uniform_canvas, repeat_from_uniform_canvas);
		break;
	case RSE::SHADER_SKY:
		glBindBufferBase(GL_UNIFORM_BUFFER, SKY_MATERIAL_UNIFORM_LOCATION, uniform_buffer);
		bind_uniforms_generic(texture_cache, shader_data->texture_uniforms);
		break;
	case RSE::SHADER_SPATIAL:
		glBindBufferBase(GL_UNIFORM_BUFFER, SCENE_MATERIAL_UNIFORM_LOCATION, uniform_buffer);
		bind_uniforms_generic(texture_cache, shader_data->texture_uniforms);
		break;
	case RSE::SHADER_PARTICLES:
		glBindBufferBase(
			GL_UNIFORM_BUFFER, GLES3::PARTICLES_MATERIAL_UNIFORM_LOCATION, uniform_buffer);
		bind_uniforms_generic(texture_cache, shader_data->texture_uniforms, 1);
		break;
	case RSE::SHADER_TEXTURE_BLIT:
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_buffer);
		bind_uniforms_generic(texture_cache, shader_data->texture_uniforms, 1);
		break;
	default:
		break;
	}
}

void GLES3::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void GLES3::MaterialStorage::material_free(RID p_rid)
{
	GLES3::Material* material = material_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(material);

	material_set_shader(p_rid, RID());
	material_set_next_pass(p_rid, RID());

	if (material->update_element.in_list()) {
		material_update_list.remove(&material->update_element);
	}

	material->dependency.deleted_notify(p_rid);
	material_owner.free(p_rid);
}

void GLES3::MaterialStorage::initialize()
{
	shaders = memnew(Shaders);

	static_assert(sizeof(GlobalShaderUniforms::Value) == 16);

	global_shader_uniforms.buffer_size = 256;
	if (global_shader_uniforms.buffer_size * sizeof(GlobalShaderUniforms::Value) >
		uint32_t(Config::max_uniform_buffer_size)) {
		global_shader_uniforms.buffer_size =
			uint32_t(Config::max_uniform_buffer_size) / sizeof(GlobalShaderUniforms::Value);
	}

	global_shader_uniforms.buffer_values =
		memnew_arr(GlobalShaderUniforms::Value, global_shader_uniforms.buffer_size);
	memset(global_shader_uniforms.buffer_values, 0,
		sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size);
	global_shader_uniforms.buffer_usage =
		memnew_arr(GlobalShaderUniforms::ValueUsage, global_shader_uniforms.buffer_size);
	global_shader_uniforms.buffer_dirty_regions = memnew_arr(bool,
		1 + (global_shader_uniforms.buffer_size / GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE));
	memset(global_shader_uniforms.buffer_dirty_regions, 0,
		sizeof(bool) * (1 + (global_shader_uniforms.buffer_size /
								GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE)));
	glGenBuffers(1, &global_shader_uniforms.buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, global_shader_uniforms.buffer);
	glBufferData(GL_UNIFORM_BUFFER,
		sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size,
		global_shader_uniforms.buffer_values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	{
		// Setup CanvasItem compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["VERTEX"] = "vertex";
		actions.renames["LIGHT_VERTEX"] = "light_vertex";
		actions.renames["SHADOW_VERTEX"] = "shadow_vertex";
		actions.renames["UV"] = "uv";
		actions.renames["POINT_SIZE"] = "point_size";

		actions.renames["MODEL_MATRIX"] = "model_matrix";
		actions.renames["CANVAS_MATRIX"] = "canvas_transform";
		actions.renames["SCREEN_MATRIX"] = "screen_transform";
		actions.renames["TIME"] = "time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.renames["AT_LIGHT_PASS"] = "false";
		actions.renames["INSTANCE_CUSTOM"] = "instance_custom";

		actions.renames["COLOR"] = "color";
		actions.renames["NORMAL"] = "normal";
		actions.renames["NORMAL_MAP"] = "normal_map";
		actions.renames["NORMAL_MAP_DEPTH"] = "normal_map_depth";
		actions.renames["TEXTURE"] = "color_texture";
		actions.renames["TEXTURE_PIXEL_SIZE"] = "color_texture_pixel_size";
		actions.renames["NORMAL_TEXTURE"] = "normal_texture";
		actions.renames["SPECULAR_SHININESS_TEXTURE"] = "specular_texture";
		actions.renames["SPECULAR_SHININESS"] = "specular_shininess";
		actions.renames["SCREEN_UV"] = "screen_uv";
		actions.renames["REGION_RECT"] = "region_rect";
		actions.renames["SCREEN_PIXEL_SIZE"] = "screen_pixel_size";
		actions.renames["FRAGCOORD"] = "gl_FragCoord";
		actions.renames["POINT_COORD"] = "gl_PointCoord";
		actions.renames["INSTANCE_ID"] = "gl_InstanceID";
		actions.renames["VERTEX_ID"] = "gl_VertexID";

		actions.renames["CUSTOM0"] = "custom0";
		actions.renames["CUSTOM1"] = "custom1";

		actions.renames["LIGHT_POSITION"] = "light_position";
		actions.renames["LIGHT_DIRECTION"] = "light_direction";
		actions.renames["LIGHT_IS_DIRECTIONAL"] = "is_directional";
		actions.renames["LIGHT_COLOR"] = "light_color";
		actions.renames["LIGHT_ENERGY"] = "light_energy";
		actions.renames["LIGHT"] = "light";
		actions.renames["SHADOW_MODULATE"] = "shadow_modulate";

		actions.renames["texture_sdf"] = "texture_sdf";
		actions.renames["texture_sdf_normal"] = "texture_sdf_normal";
		actions.renames["sdf_to_screen_uv"] = "sdf_to_screen_uv";
		actions.renames["screen_uv_to_sdf"] = "screen_uv_to_sdf";

		actions.usage_defines["COLOR"] = "#define COLOR_USED\n";
		actions.usage_defines["SCREEN_UV"] = "#define SCREEN_UV_USED\n";
		actions.usage_defines["SCREEN_PIXEL_SIZE"] = "@SCREEN_UV";
		actions.usage_defines["NORMAL"] = "#define NORMAL_USED\n";
		actions.usage_defines["NORMAL_MAP"] = "#define NORMAL_MAP_USED\n";
		actions.usage_defines["SPECULAR_SHININESS"] = "#define SPECULAR_SHININESS_USED\n";
		actions.usage_defines["CUSTOM0"] = "#define CUSTOM0_USED\n";
		actions.usage_defines["CUSTOM1"] = "#define CUSTOM1_USED\n";

		actions.render_mode_defines["skip_vertex_transform"] = "#define SKIP_TRANSFORM_USED\n";
		actions.render_mode_defines["unshaded"] = "#define MODE_UNSHADED\n";
		actions.render_mode_defines["light_only"] = "#define MODE_LIGHT_ONLY\n";
		actions.render_mode_defines["world_vertex_coords"] = "#define USE_WORLD_VERTEX_COORDS\n";

		actions.global_buffer_array_variable = "global_shader_uniforms";
		actions.instance_uniform_index_variable = "read_draw_data_instance_offset";

		shaders->compiler_canvas.initialize(actions);
	}

	{
		// Setup Scene compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["MODEL_MATRIX"] = "model_matrix";
		actions.renames["MODEL_NORMAL_MATRIX"] = "model_normal_matrix";
		actions.renames["VIEW_MATRIX"] = "scene_data_block.data.view_matrix";
		actions.renames["INV_VIEW_MATRIX"] = "scene_data_block.data.inv_view_matrix";
		actions.renames["PROJECTION_MATRIX"] = "projection_matrix";
		actions.renames["INV_PROJECTION_MATRIX"] = "inv_projection_matrix";
		actions.renames["MODELVIEW_MATRIX"] = "modelview";
		actions.renames["MODELVIEW_NORMAL_MATRIX"] = "modelview_normal";
		actions.renames["MAIN_CAM_INV_VIEW_MATRIX"] =
			"scene_data_block.data.main_cam_inv_view_matrix";

		actions.renames["VERTEX"] = "vertex";
		actions.renames["NORMAL"] = "normal";
		actions.renames["TANGENT"] = "tangent";
		actions.renames["BINORMAL"] = "binormal";
		actions.renames["POSITION"] = "position";
		actions.renames["UV"] = "uv_interp";
		actions.renames["UV2"] = "uv2_interp";
		actions.renames["COLOR"] = "color_interp";
		actions.renames["POINT_SIZE"] = "point_size";
		actions.renames["INSTANCE_ID"] = "gl_InstanceID";
		actions.renames["VERTEX_ID"] = "gl_VertexID";
		actions.renames["Z_CLIP_SCALE"] = "z_clip_scale";

		actions.renames["ALPHA_SCISSOR_THRESHOLD"] = "alpha_scissor_threshold";
		actions.renames["ALPHA_HASH_SCALE"] = "alpha_hash_scale";
		actions.renames["ALPHA_ANTIALIASING_EDGE"] = "alpha_antialiasing_edge";
		actions.renames["ALPHA_TEXTURE_COORDINATE"] = "alpha_texture_coordinate";

		actions.renames["TIME"] = "scene_data_block.data.time";
		actions.renames["EXPOSURE"] =
			"(1.0 / scene_data_block.data.emissive_exposure_normalization)";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.renames["OUTPUT_IS_SRGB"] = "SHADER_IS_SRGB";
		actions.renames["CLIP_SPACE_FAR"] = "SHADER_SPACE_FAR";
		actions.renames["IN_SHADOW_PASS"] = "IN_SHADOW_PASS";
		actions.renames["VIEWPORT_SIZE"] = "scene_data_block.data.viewport_size";

		actions.renames["FRAGCOORD"] = "gl_FragCoord";
		actions.renames["FRONT_FACING"] = "gl_FrontFacing";
		actions.renames["NORMAL_MAP"] = "normal_map";
		actions.renames["NORMAL_MAP_DEPTH"] = "normal_map_depth";
		actions.renames["BENT_NORMAL_MAP"] = "bent_normal_map";
		actions.renames["ALBEDO"] = "albedo";
		actions.renames["ALPHA"] = "alpha";
		actions.renames["PREMUL_ALPHA_FACTOR"] = "premul_alpha";
		actions.renames["METALLIC"] = "metallic";
		actions.renames["SPECULAR"] = "specular";
		actions.renames["ROUGHNESS"] = "roughness";
		actions.renames["RIM"] = "rim";
		actions.renames["RIM_TINT"] = "rim_tint";
		actions.renames["CLEARCOAT"] = "clearcoat";
		actions.renames["CLEARCOAT_ROUGHNESS"] = "clearcoat_roughness";
		actions.renames["ANISOTROPY"] = "anisotropy";
		actions.renames["ANISOTROPY_FLOW"] = "anisotropy_flow";
		actions.renames["SSS_STRENGTH"] = "sss_strength";
		actions.renames["SSS_TRANSMITTANCE_COLOR"] = "transmittance_color";
		actions.renames["SSS_TRANSMITTANCE_DEPTH"] = "transmittance_depth";
		actions.renames["SSS_TRANSMITTANCE_BOOST"] = "transmittance_boost";
		actions.renames["BACKLIGHT"] = "backlight";
		actions.renames["AO"] = "ao";
		actions.renames["AO_LIGHT_AFFECT"] = "ao_light_affect";
		actions.renames["EMISSION"] = "emission";
		actions.renames["POINT_COORD"] = "gl_PointCoord";
		actions.renames["INSTANCE_CUSTOM"] = "instance_custom";
		actions.renames["SCREEN_UV"] = "screen_uv";
		actions.renames["DEPTH"] = "gl_FragDepth";
		actions.renames["FOG"] = "fog";
		actions.renames["RADIANCE"] = "custom_radiance";
		actions.renames["IRRADIANCE"] = "custom_irradiance";
		actions.renames["BONE_INDICES"] = "bone_attrib";
		actions.renames["BONE_WEIGHTS"] = "weight_attrib";
		actions.renames["CUSTOM0"] = "custom0_attrib";
		actions.renames["CUSTOM1"] = "custom1_attrib";
		actions.renames["CUSTOM2"] = "custom2_attrib";
		actions.renames["CUSTOM3"] = "custom3_attrib";
		actions.renames["LIGHT_VERTEX"] = "light_vertex";

		actions.renames["NODE_POSITION_WORLD"] = "model_matrix[3].xyz";
		actions.renames["CAMERA_POSITION_WORLD"] = "scene_data_block.data.inv_view_matrix[3].xyz";
		actions.renames["CAMERA_DIRECTION_WORLD"] = "scene_data_block.data.inv_view_matrix[2].xyz";
		actions.renames["CAMERA_VISIBLE_LAYERS"] = "scene_data_block.data.camera_visible_layers";
		actions.renames["NODE_POSITION_VIEW"] =
			"(scene_data_block.data.view_matrix * model_matrix)[3].xyz";

		actions.renames["IS_MULTIVIEW"] = "OUTPUT_IS_MULTIVIEW";
		actions.renames["VIEW_INDEX"] = "ViewIndex";
		actions.renames["VIEW_MONO_LEFT"] = "uint(0)";
		actions.renames["VIEW_RIGHT"] = "uint(1)";
		actions.renames["EYE_OFFSET"] = "eye_offset";

		actions.renames["VIEW"] = "view";
		actions.renames["SPECULAR_AMOUNT"] = "specular_amount";
		actions.renames["LIGHT_COLOR"] = "light_color";
		actions.renames["LIGHT_IS_DIRECTIONAL"] = "is_directional";
		actions.renames["LIGHT_IS_AREA"] = "is_area";
		actions.renames["LIGHT_AREA_DIFFUSE_MULTIPLIER"] = "area_diffuse";
		actions.renames["LIGHT_AREA_SPECULAR_MULTIPLIER"] = "area_specular";
		actions.renames["LIGHT"] = "light";
		actions.renames["ATTENUATION"] = "attenuation";
		actions.renames["DIFFUSE_LIGHT"] = "diffuse_light";
		actions.renames["SPECULAR_LIGHT"] = "specular_light";

		actions.usage_defines["NORMAL"] = "#define NORMAL_USED\n";
		actions.usage_defines["TANGENT"] = "#define TANGENT_USED\n";
		actions.usage_defines["BINORMAL"] = "@TANGENT";
		actions.usage_defines["RIM"] = "#define LIGHT_RIM_USED\n";
		actions.usage_defines["RIM_TINT"] = "@RIM";
		actions.usage_defines["CLEARCOAT"] = "#define LIGHT_CLEARCOAT_USED\n";
		actions.usage_defines["CLEARCOAT_ROUGHNESS"] = "@CLEARCOAT";
		actions.usage_defines["ANISOTROPY"] = "#define LIGHT_ANISOTROPY_USED\n";
		actions.usage_defines["ANISOTROPY_FLOW"] = "@ANISOTROPY";
		actions.usage_defines["AO"] = "#define AO_USED\n";
		actions.usage_defines["AO_LIGHT_AFFECT"] = "#define AO_USED\n";
		actions.usage_defines["UV"] = "#define UV_USED\n";
		actions.usage_defines["UV2"] = "#define UV2_USED\n";
		actions.usage_defines["BONE_INDICES"] = "#define BONES_USED\n";
		actions.usage_defines["BONE_WEIGHTS"] = "#define WEIGHTS_USED\n";
		actions.usage_defines["CUSTOM0"] = "#define CUSTOM0_USED\n";
		actions.usage_defines["CUSTOM1"] = "#define CUSTOM1_USED\n";
		actions.usage_defines["CUSTOM2"] = "#define CUSTOM2_USED\n";
		actions.usage_defines["CUSTOM3"] = "#define CUSTOM3_USED\n";
		actions.usage_defines["NORMAL_MAP"] = "#define NORMAL_MAP_USED\n";
		actions.usage_defines["NORMAL_MAP_DEPTH"] = "@NORMAL_MAP";
		actions.usage_defines["BENT_NORMAL_MAP"] = "#define BENT_NORMAL_MAP_USED\n";
		actions.usage_defines["COLOR"] = "#define COLOR_USED\n";
		actions.usage_defines["INSTANCE_CUSTOM"] = "#define ENABLE_INSTANCE_CUSTOM\n";
		actions.usage_defines["POSITION"] = "#define OVERRIDE_POSITION\n";
		actions.usage_defines["LIGHT_VERTEX"] = "#define LIGHT_VERTEX_USED\n";
		actions.usage_defines["Z_CLIP_SCALE"] = "#define Z_CLIP_SCALE_USED\n";
		actions.usage_defines["LIGHT_AREA_DIFFUSE_MULTIPLIER"] = "#define AREA_LIGHT_CODE_USED\n";
		actions.usage_defines["LIGHT_AREA_SPECULAR_MULTIPLIER"] = "@LIGHT_AREA_DIFFUSE_MULTIPLIER";
		actions.usage_defines["LIGHT_IS_AREA"] = "@LIGHT_AREA_DIFFUSE_MULTIPLIER";

		actions.usage_defines["ALPHA_SCISSOR_THRESHOLD"] = "#define ALPHA_SCISSOR_USED\n";
		actions.usage_defines["ALPHA_HASH_SCALE"] = "#define ALPHA_HASH_USED\n";
		actions.usage_defines["ALPHA_ANTIALIASING_EDGE"] = "#define ALPHA_ANTIALIASING_EDGE_USED\n";
		actions.usage_defines["ALPHA_TEXTURE_COORDINATE"] = "@ALPHA_ANTIALIASING_EDGE";
		actions.usage_defines["PREMULT_ALPHA_FACTOR"] = "#define PREMULT_ALPHA_USED";

		actions.usage_defines["SSS_STRENGTH"] = "#define ENABLE_SSS\n";
		actions.usage_defines["SSS_TRANSMITTANCE_DEPTH"] = "#define ENABLE_TRANSMITTANCE\n";
		actions.usage_defines["BACKLIGHT"] = "#define LIGHT_BACKLIGHT_USED\n";
		actions.usage_defines["SCREEN_UV"] = "#define SCREEN_UV_USED\n";

		actions.usage_defines["FOG"] = "#define CUSTOM_FOG_USED\n";
		actions.usage_defines["RADIANCE"] = "#define CUSTOM_RADIANCE_USED\n";
		actions.usage_defines["IRRADIANCE"] = "#define CUSTOM_IRRADIANCE_USED\n";

		actions.render_mode_defines["skip_vertex_transform"] = "#define SKIP_TRANSFORM_USED\n";
		actions.render_mode_defines["world_vertex_coords"] = "#define VERTEX_WORLD_COORDS_USED\n";
		actions.render_mode_defines["ensure_correct_normals"] = "#define ENSURE_CORRECT_NORMALS\n";
		actions.render_mode_defines["cull_front"] = "#define DO_SIDE_CHECK\n";
		actions.render_mode_defines["cull_disabled"] = "#define DO_SIDE_CHECK\n";
		actions.render_mode_defines["particle_trails"] = "#define USE_PARTICLE_TRAILS\n";
		actions.render_mode_defines["depth_prepass_alpha"] = "#define USE_OPAQUE_PREPASS\n";

		actions.render_mode_defines["diffuse_lambert_wrap"] = "#define DIFFUSE_LAMBERT_WRAP\n";
		actions.render_mode_defines["diffuse_toon"] = "#define DIFFUSE_TOON\n";

		actions.render_mode_defines["sss_mode_skin"] = "#define SSS_MODE_SKIN\n";

		actions.render_mode_defines["specular_schlick_ggx"] = "#define SPECULAR_SCHLICK_GGX\n";
		actions.render_mode_defines["specular_toon"] = "#define SPECULAR_TOON\n";
		actions.render_mode_defines["specular_disabled"] = "#define SPECULAR_DISABLED\n";
		actions.render_mode_defines["shadows_disabled"] = "#define SHADOWS_DISABLED\n";
		actions.render_mode_defines["ambient_light_disabled"] = "#define AMBIENT_LIGHT_DISABLED\n";
		actions.render_mode_defines["shadow_to_opacity"] = "#define USE_SHADOW_TO_OPACITY\n";
		actions.render_mode_defines["unshaded"] = "#define MODE_UNSHADED\n";
		if (!GLES3::Config::force_vertex_shading) {
			actions.render_mode_defines["vertex_lighting"] = "#define USE_VERTEX_LIGHTING\n";
		}
		actions.render_mode_defines["fog_disabled"] = "#define FOG_DISABLED\n";
		actions.render_mode_defines["specular_occlusion_disabled"] =
			"#define SPECULAR_OCCLUSION_DISABLED\n";

		actions.default_filter = ShaderLanguage::FILTER_LINEAR_MIPMAP;
		actions.default_repeat = ShaderLanguage::REPEAT_ENABLE;

		actions.apply_luminance_multiplier = true;
		actions.check_multiview_samplers = RendererCompositor::is_xr_enabled();
		actions.global_buffer_array_variable = "global_shader_uniforms";
		actions.instance_uniform_index_variable = "instance_offset";

		shaders->compiler_scene.initialize(actions);
	}

	{
		// Setup Particles compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["COLOR"] = "out_color";
		actions.renames["VELOCITY"] = "out_velocity_flags.xyz";
		actions.renames["MASS"] = "mass";
		actions.renames["ACTIVE"] = "particle_active";
		actions.renames["RESTART"] = "restart";
		actions.renames["CUSTOM"] = "out_custom";
		for (int i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
			String udname = "USERDATA" + itos(i + 1);
			actions.renames[udname] = "out_userdata" + itos(i + 1);
			actions.usage_defines[udname] = "#define USERDATA" + itos(i + 1) + "_USED\n";
		}
		actions.renames["TRANSFORM"] = "xform";
		actions.renames["TIME"] = "time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.renames["LIFETIME"] = "lifetime";
		actions.renames["DELTA"] = "local_delta";
		actions.renames["NUMBER"] = "particle_number";
		actions.renames["INDEX"] = "index";
		actions.renames["AMOUNT_RATIO"] = "amount_ratio";
		actions.renames["EMISSION_TRANSFORM"] = "emission_transform";
		actions.renames["RANDOM_SEED"] = "random_seed";
		actions.renames["RESTART_POSITION"] = "restart_position";
		actions.renames["RESTART_ROT_SCALE"] = "restart_rotation_scale";
		actions.renames["RESTART_VELOCITY"] = "restart_velocity";
		actions.renames["RESTART_COLOR"] = "restart_color";
		actions.renames["RESTART_CUSTOM"] = "restart_custom";
		actions.renames["COLLIDED"] = "collided";
		actions.renames["COLLISION_NORMAL"] = "collision_normal";
		actions.renames["COLLISION_DEPTH"] = "collision_depth";
		actions.renames["ATTRACTOR_FORCE"] = "attractor_force";
		actions.renames["EMITTER_VELOCITY"] = "emitter_velocity";
		actions.renames["INTERPOLATE_TO_END"] = "interp_to_end";

		actions.renames["FLAG_EMIT_POSITION"] = "uint(1)";
		actions.renames["FLAG_EMIT_ROT_SCALE"] = "uint(2)";
		actions.renames["FLAG_EMIT_VELOCITY"] = "uint(4)";
		actions.renames["FLAG_EMIT_COLOR"] = "uint(8)";
		actions.renames["FLAG_EMIT_CUSTOM"] = "uint(16)";
		actions.renames["emit_subparticle"] = "emit_subparticle";
		actions.usage_defines["emit_subparticle"] =
			"\nbool emit_subparticle(mat4 p_xform, vec3 p_velocity, vec4 p_color, vec4 p_custom, "
			"uint p_flags) {\n\treturn false;\n}\n";

		actions.render_mode_defines["disable_force"] = "#define DISABLE_FORCE\n";
		actions.render_mode_defines["disable_velocity"] = "#define DISABLE_VELOCITY\n";
		actions.render_mode_defines["keep_data"] = "#define ENABLE_KEEP_DATA\n";
		actions.render_mode_defines["collision_use_scale"] = "#define USE_COLLISION_SCALE\n";

		actions.default_filter = ShaderLanguage::FILTER_LINEAR_MIPMAP;
		actions.default_repeat = ShaderLanguage::REPEAT_ENABLE;

		actions.global_buffer_array_variable = "global_shader_uniforms";

		shaders->compiler_particles.initialize(actions);
	}

	{
		// Setup Sky compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["COLOR"] = "color";
		actions.renames["ALPHA"] = "alpha";
		actions.renames["EYEDIR"] = "cube_normal";
		actions.renames["POSITION"] = "position";
		actions.renames["SKY_COORDS"] = "panorama_coords";
		actions.renames["SCREEN_UV"] = "uv";
		actions.renames["TIME"] = "time";
		actions.renames["FRAGCOORD"] = "gl_FragCoord";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.renames["HALF_RES_COLOR"] = "half_res_color";
		actions.renames["QUARTER_RES_COLOR"] = "quarter_res_color";
		actions.renames["RADIANCE"] = "radiance";
		actions.renames["FOG"] = "custom_fog";
		actions.renames["LIGHT0_ENABLED"] =
			"bool(directional_lights.data[0].enabled_bake_mode & DIRECTIONAL_LIGHT_ENABLED)";
		actions.renames["LIGHT0_DIRECTION"] = "directional_lights.data[0].direction_energy.xyz";
		actions.renames["LIGHT0_ENERGY"] = "directional_lights.data[0].direction_energy.w";
		actions.renames["LIGHT0_COLOR"] = "directional_lights.data[0].color_size.xyz";
		actions.renames["LIGHT0_SIZE"] = "directional_lights.data[0].color_size.w";
		actions.renames["LIGHT1_ENABLED"] =
			"bool(directional_lights.data[1].enabled_bake_mode & DIRECTIONAL_LIGHT_ENABLED)";
		actions.renames["LIGHT1_DIRECTION"] = "directional_lights.data[1].direction_energy.xyz";
		actions.renames["LIGHT1_ENERGY"] = "directional_lights.data[1].direction_energy.w";
		actions.renames["LIGHT1_COLOR"] = "directional_lights.data[1].color_size.xyz";
		actions.renames["LIGHT1_SIZE"] = "directional_lights.data[1].color_size.w";
		actions.renames["LIGHT2_ENABLED"] =
			"bool(directional_lights.data[2].enabled_bake_mode & DIRECTIONAL_LIGHT_ENABLED)";
		actions.renames["LIGHT2_DIRECTION"] = "directional_lights.data[2].direction_energy.xyz";
		actions.renames["LIGHT2_ENERGY"] = "directional_lights.data[2].direction_energy.w";
		actions.renames["LIGHT2_COLOR"] = "directional_lights.data[2].color_size.xyz";
		actions.renames["LIGHT2_SIZE"] = "directional_lights.data[2].color_size.w";
		actions.renames["LIGHT3_ENABLED"] =
			"bool(directional_lights.data[3].enabled_bake_mode & DIRECTIONAL_LIGHT_ENABLED)";
		actions.renames["LIGHT3_DIRECTION"] = "directional_lights.data[3].direction_energy.xyz";
		actions.renames["LIGHT3_ENERGY"] = "directional_lights.data[3].direction_energy.w";
		actions.renames["LIGHT3_COLOR"] = "directional_lights.data[3].color_size.xyz";
		actions.renames["LIGHT3_SIZE"] = "directional_lights.data[3].color_size.w";
		actions.renames["AT_CUBEMAP_PASS"] = "AT_CUBEMAP_PASS";
		actions.renames["AT_HALF_RES_PASS"] = "AT_HALF_RES_PASS";
		actions.renames["AT_QUARTER_RES_PASS"] = "AT_QUARTER_RES_PASS";
		actions.usage_defines["HALF_RES_COLOR"] = "\n#define USES_HALF_RES_COLOR\n";
		actions.usage_defines["QUARTER_RES_COLOR"] = "\n#define USES_QUARTER_RES_COLOR\n";
		actions.render_mode_defines["disable_fog"] = "#define DISABLE_FOG\n";
		actions.render_mode_defines["use_debanding"] = "#define USE_DEBANDING\n";

		actions.default_filter = ShaderLanguage::FILTER_LINEAR_MIPMAP;
		actions.default_repeat = ShaderLanguage::REPEAT_ENABLE;

		actions.global_buffer_array_variable = "global_shader_uniforms";

		shaders->compiler_sky.initialize(actions);
	}

	{
		// Setup TextureBlit compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["TIME"] = "time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);

		actions.renames["FRAGCOORD"] = "gl_FragCoord";

		actions.renames["UV"] = "uv";
		actions.renames["MODULATE"] = "modulate";

		actions.renames["COLOR0"] = "color0";
		actions.renames["COLOR1"] = "color1";
		actions.renames["COLOR2"] = "color2";
		actions.renames["COLOR3"] = "color3";

		shaders->compiler_tex_blit.initialize(actions);
	}
}

void GLES3::MaterialStorage::finalize()
{
	LocalVector<RID> owned_materials = material_owner.get_owned_list();
	for (const RID& E : owned_materials) {
		material_free(E);
	}
	LocalVector<RID> owned_shaders = shader_owner.get_owned_list();
	for (const RID& E : owned_shaders) {
		shader_free(E);
	}
	if (global_shader_uniforms.buffer != 0) {
		glDeleteBuffers(1, &global_shader_uniforms.buffer);
		global_shader_uniforms.buffer = 0;
	}
	if (global_shader_uniforms.buffer_values) {
		memdelete_arr(global_shader_uniforms.buffer_values);
		global_shader_uniforms.buffer_values = nullptr;
	}
	if (global_shader_uniforms.buffer_usage) {
		memdelete_arr(global_shader_uniforms.buffer_usage);
		global_shader_uniforms.buffer_usage = nullptr;
	}
	if (global_shader_uniforms.buffer_dirty_regions) {
		memdelete_arr(global_shader_uniforms.buffer_dirty_regions);
		global_shader_uniforms.buffer_dirty_regions = nullptr;
	}
	if (shaders) {
		memdelete(shaders);
		shaders = nullptr;
	}
}

#endif // !GLES3_ENABLED



// ============================================================================
// Merged from drivers/gles3/storage/mesh_storage.cpp
// ============================================================================

#include "mesh_storage.h"

#ifdef GLES3_ENABLED

#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_viewport.h"

using namespace GLES3;

/* MESH API */

RID MeshStorage::mesh_allocate() { return mesh_owner.allocate_rid(); }

void MeshStorage::mesh_initialize(RID p_rid) { mesh_owner.initialize_rid(p_rid, Mesh()); }

void MeshStorage::multimesh_free(RID p_rid)
{
	mesh_clear(p_rid);
	mesh_set_shadow_mesh(p_rid, RID());
	Mesh* mesh = mesh_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(mesh);

	mesh->dependency.deleted_notify(p_rid);
	if (mesh->instances.size()) {
		ERR_PRINT("deleting mesh with active instances");
	}
	if (mesh->shadow_owners.size()) {
		for (Mesh* E : mesh->shadow_owners) {
			Mesh* shadow_owner = E;
			shadow_owner->shadow_mesh = RID();
			shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
		}
	}
	mesh_owner.free(p_rid);
}

void MeshStorage::mesh_free(RID p_rid)
{
	mesh_clear(p_rid);
	mesh_set_shadow_mesh(p_rid, RID());
	Mesh* mesh = mesh_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(mesh);

	mesh->dependency.deleted_notify(p_rid);
	if (mesh->instances.size()) {
		ERR_PRINT("deleting mesh with active instances");
	}
	if (mesh->shadow_owners.size()) {
		for (Mesh* E : mesh->shadow_owners) {
			Mesh* shadow_owner = E;
			shadow_owner->shadow_mesh = RID();
			shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
		}
	}
	mesh_owner.free(p_rid);
}

RID MeshStorage::multimesh_get_mesh(RID p_multimesh) { return RID(); }

void MeshStorage::mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
{
	ERR_FAIL_COND(p_blend_shape_count < 0);

	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	ERR_FAIL_COND(mesh->surface_count > 0); // surfaces already exist
	mesh->blend_shape_count = p_blend_shape_count;
}

bool MeshStorage::mesh_needs_instance(RID p_mesh, bool p_has_skeleton)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, false);

	return mesh->blend_shape_count > 0 || (mesh->has_bone_weights && p_has_skeleton);
}

void MeshStorage::mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	ERR_FAIL_COND(mesh->surface_count == RSE::MAX_MESH_SURFACES);

#ifdef DEBUG_ENABLED
	// do a validation, to catch errors first
	{
		uint32_t stride = 0;
		uint32_t attrib_stride = 0;
		uint32_t skin_stride = 0;

		for (int i = 0; i < RSE::ARRAY_WEIGHTS; i++) {
			if ((p_surface.format & (1ULL << i))) {
				switch (i) {
				case RSE::ARRAY_VERTEX: {
					if ((p_surface.format & RSE::ARRAY_FLAG_USE_2D_VERTICES) ||
						(p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
						stride += sizeof(float) * 2;
					}
					else {
						stride += sizeof(float) * 3;
					}
				} break;
				case RSE::ARRAY_NORMAL: {
					stride += sizeof(uint16_t) * 2;

				} break;
				case RSE::ARRAY_TANGENT: {
					if (!(p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
						stride += sizeof(uint16_t) * 2;
					}
				} break;
				case RSE::ARRAY_COLOR: {
					attrib_stride += sizeof(uint32_t);
				} break;
				case RSE::ARRAY_TEX_UV: {
					if (p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
						attrib_stride += sizeof(uint16_t) * 2;
					}
					else {
						attrib_stride += sizeof(float) * 2;
					}
				} break;
				case RSE::ARRAY_TEX_UV2: {
					if (p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
						attrib_stride += sizeof(uint16_t) * 2;
					}
					else {
						attrib_stride += sizeof(float) * 2;
					}
				} break;
				case RSE::ARRAY_CUSTOM0:
				case RSE::ARRAY_CUSTOM1:
				case RSE::ARRAY_CUSTOM2:
				case RSE::ARRAY_CUSTOM3: {
					int idx = i - RSE::ARRAY_CUSTOM0;
					uint32_t fmt_shift[RSE::ARRAY_CUSTOM_COUNT] = {RSE::ARRAY_FORMAT_CUSTOM0_SHIFT,
						RSE::ARRAY_FORMAT_CUSTOM1_SHIFT, RSE::ARRAY_FORMAT_CUSTOM2_SHIFT,
						RSE::ARRAY_FORMAT_CUSTOM3_SHIFT};
					uint32_t fmt =
						(p_surface.format >> fmt_shift[idx]) & RSE::ARRAY_FORMAT_CUSTOM_MASK;
					uint32_t fmtsize[RSE::ARRAY_CUSTOM_MAX] = {4, 4, 4, 8, 4, 8, 12, 16};
					attrib_stride += fmtsize[fmt];

				} break;
				case RSE::ARRAY_WEIGHTS:
				case RSE::ARRAY_BONES: {
					// uses a separate array
					bool use_8 = p_surface.format & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
					skin_stride += sizeof(int16_t) * (use_8 ? 16 : 8);
				} break;
				}
			}
		}

		int expected_size = stride * p_surface.vertex_count;
		ERR_FAIL_COND_MSG(expected_size != p_surface.vertex_data.size(),
			"Size of vertex data provided (" + itos(p_surface.vertex_data.size()) +
				") does not match expected (" + itos(expected_size) + ")");

		int bs_expected_size = expected_size * mesh->blend_shape_count;

		ERR_FAIL_COND_MSG(bs_expected_size != p_surface.blend_shape_data.size(),
			"Size of blend shape data provided (" + itos(p_surface.blend_shape_data.size()) +
				") does not match expected (" + itos(bs_expected_size) + ")");

		int expected_attrib_size = attrib_stride * p_surface.vertex_count;
		ERR_FAIL_COND_MSG(expected_attrib_size != p_surface.attribute_data.size(),
			"Size of attribute data provided (" + itos(p_surface.attribute_data.size()) +
				") does not match expected (" + itos(expected_attrib_size) + ")");

		if ((p_surface.format & RSE::ARRAY_FORMAT_WEIGHTS) &&
			(p_surface.format & RSE::ARRAY_FORMAT_BONES)) {
			expected_size = skin_stride * p_surface.vertex_count;
			ERR_FAIL_COND_MSG(expected_size != p_surface.skin_data.size(),
				"Size of skin data provided (" + itos(p_surface.skin_data.size()) +
					") does not match expected (" + itos(expected_size) + ")");
		}
	}

#endif

	uint64_t surface_version = p_surface.format & (uint64_t(RSE::ARRAY_FLAG_FORMAT_VERSION_MASK)
													  << RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
	RenderingServerTypes::SurfaceData new_surface = p_surface;
#ifdef DISABLE_DEPRECATED

	ERR_FAIL_COND_MSG(surface_version != RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION,
		"Surface version provided (" +
			itos(int(surface_version >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT)) +
			") does not match current version (" +
			itos(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) +
			")");

#else

	if (surface_version != uint64_t(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION)) {
		Renderer::fix_surface_compatibility(new_surface);
		surface_version = new_surface.format & (uint64_t(RSE::ARRAY_FLAG_FORMAT_VERSION_MASK)
												   << RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
		ERR_FAIL_COND_MSG(surface_version != RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION,
			vformat("Surface version provided (%d) does not match current version (%d).",
				(surface_version >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) &
					RSE::ARRAY_FLAG_FORMAT_VERSION_MASK,
				(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) &
					RSE::ARRAY_FLAG_FORMAT_VERSION_MASK));
	}
#endif

	Mesh::Surface* s = memnew(Mesh::Surface);

	s->format = new_surface.format;
	s->primitive = new_surface.primitive;

	if (new_surface.vertex_data.size()) {
		glGenBuffers(1, &s->vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
		// If we have an uncompressed surface that contains normals, but not tangents, we need to
		// differentiate the array from a compressed array in the shader. To do so, we allow the
		// normal to read 4 components out of the buffer But only give it 2 components per normal.
		// So essentially, each vertex reads the next normal in normal.zw. This allows us to avoid
		// adding a shader permutation, and avoid passing dummy tangents. Since the stride is kept
		// small this should still be a net win for bandwidth. If we do this, then the last normal
		// will read past the end of the array. So we need to pad the array with dummy data.
		if (!(new_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) &&
			(new_surface.format & RSE::ARRAY_FORMAT_NORMAL) &&
			!(new_surface.format & RSE::ARRAY_FORMAT_TANGENT)) {
			// Unfortunately, we need to copy the buffer, which is fine as doing a resize triggers a
			// CoW anyway.
			Vector<uint8_t> new_vertex_data;
			new_vertex_data.resize_initialized(
				new_surface.vertex_data.size() + sizeof(uint16_t) * 2);
			memcpy(new_vertex_data.ptrw(), new_surface.vertex_data.ptr(),
				new_surface.vertex_data.size());
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, s->vertex_buffer,
				new_vertex_data.size(), new_vertex_data.ptr(),
				(s->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW,
				"Mesh vertex buffer");
			s->vertex_buffer_size = new_vertex_data.size();
		}
		else {
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, s->vertex_buffer,
				new_surface.vertex_data.size(), new_surface.vertex_data.ptr(),
				(s->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW,
				"Mesh vertex buffer");
			s->vertex_buffer_size = new_surface.vertex_data.size();
		}
	}

	if (new_surface.attribute_data.size()) {
		glGenBuffers(1, &s->attribute_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, s->attribute_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, s->attribute_buffer,
			new_surface.attribute_data.size(), new_surface.attribute_data.ptr(),
			(s->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW,
			"Mesh attribute buffer");
		s->attribute_buffer_size = new_surface.attribute_data.size();
	}

	if (new_surface.skin_data.size()) {
		glGenBuffers(1, &s->skin_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, s->skin_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, s->skin_buffer,
			new_surface.skin_data.size(), new_surface.skin_data.ptr(),
			(s->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW,
			"Mesh skin buffer");
		s->skin_buffer_size = new_surface.skin_data.size();
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	s->vertex_count = new_surface.vertex_count;

	if (new_surface.format & RSE::ARRAY_FORMAT_BONES) {
		mesh->has_bone_weights = true;
	}

	if (new_surface.index_count) {
		bool is_index_16 = new_surface.vertex_count <= 65536 && new_surface.vertex_count > 0;
		glGenBuffers(1, &s->index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->index_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER, s->index_buffer,
			new_surface.index_data.size(), new_surface.index_data.ptr(), GL_STATIC_DRAW,
			"Mesh index buffer");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // unbind
		s->index_count = new_surface.index_count;
		s->index_buffer_size = new_surface.index_data.size();

		if (new_surface.lods.size()) {
			s->lods = memnew_arr(Mesh::Surface::LOD, new_surface.lods.size());
			s->lod_count = new_surface.lods.size();

			for (int i = 0; i < new_surface.lods.size(); i++) {
				glGenBuffers(1, &s->lods[i].index_buffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->lods[i].index_buffer);
				GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER,
					s->lods[i].index_buffer, new_surface.lods[i].index_data.size(),
					new_surface.lods[i].index_data.ptr(), GL_STATIC_DRAW,
					"Mesh index buffer LOD[" + itos(i) + "]");
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // unbind
				s->lods[i].edge_length = new_surface.lods[i].edge_length;
				s->lods[i].index_count =
					new_surface.lods[i].index_data.size() / (is_index_16 ? 2 : 4);
				s->lods[i].index_buffer_size = new_surface.lods[i].index_data.size();
			}
		}
	}

	ERR_FAIL_COND_MSG(!new_surface.index_count && !new_surface.vertex_count,
		"Meshes must contain a vertex array, an index array, or both");

	if (GLES3::Config::generate_wireframes && s->primitive == RSE::PRIMITIVE_TRIANGLES) {
		// Generate wireframes. This is mostly used by the editor.
		s->wireframe = memnew(Mesh::Surface::Wireframe);
		Vector<uint32_t> wf_indices;
		uint32_t& wf_index_count = s->wireframe->index_count;
		uint32_t* wr = nullptr;

		if (new_surface.format & RSE::ARRAY_FORMAT_INDEX) {
			wf_index_count = s->index_count * 2;
			wf_indices.resize(wf_index_count);

			Vector<uint8_t> ir = new_surface.index_data;
			wr = wf_indices.ptrw();

			if (new_surface.vertex_count <= 65536) {
				// Read 16 bit indices.
				const uint16_t* src_idx = (const uint16_t*)ir.ptr();
				for (uint32_t i = 0; i + 5 < wf_index_count; i += 6) {
					// We use GL_LINES instead of GL_TRIANGLES for drawing these primitives later,
					// so we need double the indices for each triangle.
					wr[i + 0] = src_idx[i / 2];
					wr[i + 1] = src_idx[i / 2 + 1];
					wr[i + 2] = src_idx[i / 2 + 1];
					wr[i + 3] = src_idx[i / 2 + 2];
					wr[i + 4] = src_idx[i / 2 + 2];
					wr[i + 5] = src_idx[i / 2];
				}

			}
			else {
				// Read 32 bit indices.
				const uint32_t* src_idx = (const uint32_t*)ir.ptr();
				for (uint32_t i = 0; i + 5 < wf_index_count; i += 6) {
					wr[i + 0] = src_idx[i / 2];
					wr[i + 1] = src_idx[i / 2 + 1];
					wr[i + 2] = src_idx[i / 2 + 1];
					wr[i + 3] = src_idx[i / 2 + 2];
					wr[i + 4] = src_idx[i / 2 + 2];
					wr[i + 5] = src_idx[i / 2];
				}
			}
		}
		else {
			// Not using indices.
			wf_index_count = s->vertex_count * 2;
			wf_indices.resize(wf_index_count);
			wr = wf_indices.ptrw();

			for (uint32_t i = 0; i + 5 < wf_index_count; i += 6) {
				wr[i + 0] = i / 2;
				wr[i + 1] = i / 2 + 1;
				wr[i + 2] = i / 2 + 1;
				wr[i + 3] = i / 2 + 2;
				wr[i + 4] = i / 2 + 2;
				wr[i + 5] = i / 2;
			}
		}

		s->wireframe->index_buffer_size = wf_index_count * sizeof(uint32_t);
		glGenBuffers(1, &s->wireframe->index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->wireframe->index_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ELEMENT_ARRAY_BUFFER, s->wireframe->index_buffer,
			s->wireframe->index_buffer_size, wr, GL_STATIC_DRAW, "Mesh wireframe index buffer");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // unbind
	}

	s->aabb = new_surface.aabb;
	s->bone_aabbs = new_surface.bone_aabbs; // only really useful for returning them.
	s->mesh_to_skeleton_xform = p_surface.mesh_to_skeleton_xform;

	s->uv_scale = new_surface.uv_scale;

	if (new_surface.skin_data.size() || mesh->blend_shape_count > 0) {
		// Size must match the size of the vertex array.
		int size = new_surface.vertex_data.size();
		int vertex_size = 0;
		int position_stride = 0;
		int normal_tangent_stride = 0;
		int normal_offset = 0;
		int tangent_offset = 0;
		if ((new_surface.format & (1ULL << RSE::ARRAY_VERTEX))) {
			if (new_surface.format & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
				vertex_size = 2;
				position_stride = sizeof(float) * vertex_size;
			}
			else {
				if (new_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
					vertex_size = 4;
					position_stride = sizeof(uint16_t) * vertex_size;
				}
				else {
					vertex_size = 3;
					position_stride = sizeof(float) * vertex_size;
				}
			}
		}
		if ((new_surface.format & (1ULL << RSE::ARRAY_NORMAL))) {
			normal_offset = position_stride * s->vertex_count;
			normal_tangent_stride += sizeof(uint16_t) * 2;
		}
		if ((new_surface.format & (1ULL << RSE::ARRAY_TANGENT))) {
			tangent_offset = normal_offset + normal_tangent_stride;
			normal_tangent_stride += sizeof(uint16_t) * 2;
		}

		if (mesh->blend_shape_count > 0) {
			// Blend shapes are passed as one large array, for OpenGL, we need to split each of them
			// into their own buffer
			s->blend_shapes = memnew_arr(Mesh::Surface::BlendShape, mesh->blend_shape_count);

			for (uint32_t i = 0; i < mesh->blend_shape_count; i++) {
				glGenVertexArrays(1, &s->blend_shapes[i].vertex_array);
				glBindVertexArray(s->blend_shapes[i].vertex_array);
				glGenBuffers(1, &s->blend_shapes[i].vertex_buffer);
				glBindBuffer(GL_ARRAY_BUFFER, s->blend_shapes[i].vertex_buffer);
				GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
					s->blend_shapes[i].vertex_buffer, size,
					new_surface.blend_shape_data.ptr() + i * size,
					(s->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW
																	 : GL_STATIC_DRAW,
					"Mesh blend shape buffer");

				if ((new_surface.format & (1ULL << RSE::ARRAY_VERTEX))) {
					glEnableVertexAttribArray(RSE::ARRAY_VERTEX + 3);
					glVertexAttribPointer(RSE::ARRAY_VERTEX + 3, vertex_size, GL_FLOAT, GL_FALSE,
						position_stride, CAST_INT_TO_UCHAR_PTR(0));
				}
				if ((new_surface.format & (1ULL << RSE::ARRAY_NORMAL))) {
					// Normal and tangent are packed into the same attribute.
					glEnableVertexAttribArray(RSE::ARRAY_NORMAL + 3);
					glVertexAttribPointer(RSE::ARRAY_NORMAL + 3, 2, GL_UNSIGNED_SHORT, GL_TRUE,
						normal_tangent_stride, CAST_INT_TO_UCHAR_PTR(normal_offset));
				}
				if ((p_surface.format & (1ULL << RSE::ARRAY_TANGENT))) {
					glEnableVertexAttribArray(RSE::ARRAY_TANGENT + 3);
					glVertexAttribPointer(RSE::ARRAY_TANGENT + 3, 2, GL_UNSIGNED_SHORT, GL_TRUE,
						normal_tangent_stride, CAST_INT_TO_UCHAR_PTR(tangent_offset));
				}
			}
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if (mesh->surface_count == 0) {
		mesh->aabb = new_surface.aabb;
	}
	else {
		mesh->aabb.merge_with(new_surface.aabb);
	}
	mesh->skeleton_aabb_version = 0;

	s->material = new_surface.material;

	mesh->surfaces = (Mesh::Surface**)memrealloc(
		mesh->surfaces, sizeof(Mesh::Surface*) * (mesh->surface_count + 1));
	mesh->surfaces[mesh->surface_count] = s;
	mesh->surface_count++;

	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_add_surface(mi, mesh, mesh->surface_count - 1);
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}

	mesh->material_cache.clear();
}

void MeshStorage::_mesh_surface_clear(Mesh* mesh, int p_surface)
{
	Mesh::Surface& s = *mesh->surfaces[p_surface];

	if (s.vertex_buffer != 0) {
		GLES3::Utilities::buffer_free_data(s.vertex_buffer);
		s.vertex_buffer = 0;
	}

	if (s.version_count != 0) {
		for (uint32_t j = 0; j < s.version_count; j++) {
			glDeleteVertexArrays(1, &s.versions[j].vertex_array);
			s.versions[j].vertex_array = 0;
		}
	}

	if (s.attribute_buffer != 0) {
		GLES3::Utilities::buffer_free_data(s.attribute_buffer);
		s.attribute_buffer = 0;
	}

	if (s.skin_buffer != 0) {
		GLES3::Utilities::buffer_free_data(s.skin_buffer);
		s.skin_buffer = 0;
	}

	if (s.index_buffer != 0) {
		GLES3::Utilities::buffer_free_data(s.index_buffer);
		s.index_buffer = 0;
	}

	if (s.versions) {
		memfree(s.versions); // reallocs, so free with memfree.
	}

	if (s.wireframe) {
		GLES3::Utilities::buffer_free_data(s.wireframe->index_buffer);
		memdelete(s.wireframe);
	}

	if (s.lod_count) {
		for (uint32_t j = 0; j < s.lod_count; j++) {
			if (s.lods[j].index_buffer != 0) {
				GLES3::Utilities::buffer_free_data(s.lods[j].index_buffer);
				s.lods[j].index_buffer = 0;
			}
		}
		memdelete_arr(s.lods);
	}

	if (mesh->blend_shape_count) {
		for (uint32_t j = 0; j < mesh->blend_shape_count; j++) {
			if (s.blend_shapes[j].vertex_buffer != 0) {
				GLES3::Utilities::buffer_free_data(s.blend_shapes[j].vertex_buffer);
				s.blend_shapes[j].vertex_buffer = 0;
			}
			if (s.blend_shapes[j].vertex_array != 0) {
				glDeleteVertexArrays(1, &s.blend_shapes[j].vertex_array);
				s.blend_shapes[j].vertex_array = 0;
			}
		}
		memdelete_arr(s.blend_shapes);
	}

	memdelete(mesh->surfaces[p_surface]);
}

int MeshStorage::mesh_get_blend_shape_count(RID p_mesh)
{
	const Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, -1);
	return mesh->blend_shape_count;
}

void MeshStorage::mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_INDEX((int)p_mode, 2);

	mesh->blend_shape_mode = p_mode;
}

RSE::BlendShapeMode MeshStorage::mesh_get_blend_shape_mode(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RSE::BLEND_SHAPE_MODE_NORMALIZED);
	return mesh->blend_shape_mode;
}

void MeshStorage::mesh_surface_update_vertex_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND(p_data.is_empty());
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	uint64_t data_size = p_data.size();
	ERR_FAIL_COND(p_offset + data_size > mesh->surfaces[p_surface]->vertex_buffer_size);
	const uint8_t* r = p_data.ptr();

	glBindBuffer(GL_ARRAY_BUFFER, mesh->surfaces[p_surface]->vertex_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, p_offset, data_size, r);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MeshStorage::mesh_surface_update_attribute_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND(p_data.is_empty());
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	uint64_t data_size = p_data.size();
	ERR_FAIL_COND(p_offset + data_size > mesh->surfaces[p_surface]->attribute_buffer_size);
	const uint8_t* r = p_data.ptr();

	glBindBuffer(GL_ARRAY_BUFFER, mesh->surfaces[p_surface]->attribute_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, p_offset, data_size, r);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MeshStorage::mesh_surface_update_skin_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND(p_data.is_empty());
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	uint64_t data_size = p_data.size();
	ERR_FAIL_COND(p_offset + data_size > mesh->surfaces[p_surface]->skin_buffer_size);
	const uint8_t* r = p_data.ptr();

	glBindBuffer(GL_ARRAY_BUFFER, mesh->surfaces[p_surface]->skin_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, p_offset, data_size, r);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MeshStorage::mesh_surface_update_index_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND(p_data.is_empty());
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	uint64_t data_size = p_data.size();
	ERR_FAIL_COND(p_offset + data_size > mesh->surfaces[p_surface]->index_buffer_size);
	const uint8_t* r = p_data.ptr();

	glBindBuffer(GL_ARRAY_BUFFER, mesh->surfaces[p_surface]->index_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, p_offset, data_size, r);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MeshStorage::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);
	mesh->surfaces[p_surface]->material = p_material;

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	mesh->material_cache.clear();
}

RID MeshStorage::mesh_surface_get_material(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());

	return mesh->surfaces[p_surface]->material;
}

RenderingServerTypes::SurfaceData MeshStorage::mesh_get_surface(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RenderingServerTypes::SurfaceData());
	ERR_FAIL_UNSIGNED_INDEX_V(
		(uint32_t)p_surface, mesh->surface_count, RenderingServerTypes::SurfaceData());

	Mesh::Surface& s = *mesh->surfaces[p_surface];

	RenderingServerTypes::SurfaceData sd;
	sd.format = s.format;
	if (s.vertex_buffer != 0) {
		sd.vertex_data =
			Utilities::buffer_get_data(GL_ARRAY_BUFFER, s.vertex_buffer, s.vertex_buffer_size);

		// When using an uncompressed buffer with normals, but without tangents, we have to trim the
		// padding.
		if (!(s.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) &&
			(s.format & RSE::ARRAY_FORMAT_NORMAL) && !(s.format & RSE::ARRAY_FORMAT_TANGENT)) {
			sd.vertex_data.resize(sd.vertex_data.size() - sizeof(uint16_t) * 2);
		}
	}

	if (s.attribute_buffer != 0) {
		sd.attribute_data = Utilities::buffer_get_data(
			GL_ARRAY_BUFFER, s.attribute_buffer, s.attribute_buffer_size);
	}

	if (s.skin_buffer != 0) {
		sd.skin_data =
			Utilities::buffer_get_data(GL_ARRAY_BUFFER, s.skin_buffer, s.skin_buffer_size);
	}

	sd.vertex_count = s.vertex_count;
	sd.index_count = s.index_count;
	sd.primitive = s.primitive;

	if (sd.index_count) {
		sd.index_data = Utilities::buffer_get_data(
			GL_ELEMENT_ARRAY_BUFFER, s.index_buffer, s.index_buffer_size);
	}

	sd.aabb = s.aabb;
	for (uint32_t i = 0; i < s.lod_count; i++) {
		RenderingServerTypes::SurfaceData::LOD lod;
		lod.edge_length = s.lods[i].edge_length;
		lod.index_data = Utilities::buffer_get_data(
			GL_ELEMENT_ARRAY_BUFFER, s.lods[i].index_buffer, s.lods[i].index_buffer_size);
		sd.lods.push_back(lod);
	}

	sd.bone_aabbs = s.bone_aabbs;
	sd.mesh_to_skeleton_xform = s.mesh_to_skeleton_xform;

	if (mesh->blend_shape_count) {
		sd.blend_shape_data = Vector<uint8_t>();
		for (uint32_t i = 0; i < mesh->blend_shape_count; i++) {
			sd.blend_shape_data.append_array(Utilities::buffer_get_data(
				GL_ARRAY_BUFFER, s.blend_shapes[i].vertex_buffer, s.vertex_buffer_size));
		}
	}

	sd.uv_scale = s.uv_scale;

	return sd;
}

int MeshStorage::mesh_get_surface_count(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, 0);
	return mesh->surface_count;
}

void MeshStorage::mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	mesh->custom_aabb = p_aabb;

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB MeshStorage::mesh_get_custom_aabb(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());
	return mesh->custom_aabb;
}

AABB MeshStorage::mesh_get_aabb(RID p_mesh, RID p_skeleton)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());

	if (mesh->custom_aabb != AABB()) {
		return mesh->custom_aabb;
	}

	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	if (!skeleton || skeleton->size == 0 || mesh->skeleton_aabb_version == skeleton->version) {
		return mesh->aabb;
	}

	// Calculate AABB based on Skeleton

	AABB aabb;

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		AABB laabb;
		const Mesh::Surface& surface = *mesh->surfaces[i];
		if ((surface.format & RSE::ARRAY_FORMAT_BONES) && surface.bone_aabbs.size()) {
			int bs = surface.bone_aabbs.size();
			const AABB* skbones = surface.bone_aabbs.ptr();

			int sbs = skeleton->size;
			ERR_CONTINUE(bs > sbs);
			const float* baseptr = skeleton->data.ptr();

			bool found_bone_aabb = false;

			if (skeleton->use_2d) {
				for (int j = 0; j < bs; j++) {
					if (skbones[j].size == Vector3(-1, -1, -1)) {
						continue; // bone is unused
					}

					const float* dataptr = baseptr + j * 8;

					Transform3D mtx;

					mtx.basis.rows[0][0] = dataptr[0];
					mtx.basis.rows[0][1] = dataptr[1];
					mtx.origin.x = dataptr[3];

					mtx.basis.rows[1][0] = dataptr[4];
					mtx.basis.rows[1][1] = dataptr[5];
					mtx.origin.y = dataptr[7];

					// Transform bounds to skeleton's space before applying animation data.
					AABB baabb = surface.mesh_to_skeleton_xform.xform(skbones[j]);
					baabb = mtx.xform(baabb);

					if (!found_bone_aabb) {
						laabb = baabb;
						found_bone_aabb = true;
					}
					else {
						laabb.merge_with(baabb);
					}
				}
			}
			else {
				for (int j = 0; j < bs; j++) {
					if (skbones[j].size == Vector3(-1, -1, -1)) {
						continue; // bone is unused
					}

					const float* dataptr = baseptr + j * 12;

					Transform3D mtx;

					mtx.basis.rows[0][0] = dataptr[0];
					mtx.basis.rows[0][1] = dataptr[1];
					mtx.basis.rows[0][2] = dataptr[2];
					mtx.origin.x = dataptr[3];
					mtx.basis.rows[1][0] = dataptr[4];
					mtx.basis.rows[1][1] = dataptr[5];
					mtx.basis.rows[1][2] = dataptr[6];
					mtx.origin.y = dataptr[7];
					mtx.basis.rows[2][0] = dataptr[8];
					mtx.basis.rows[2][1] = dataptr[9];
					mtx.basis.rows[2][2] = dataptr[10];
					mtx.origin.z = dataptr[11];

					// Transform bounds to skeleton's space before applying animation data.
					AABB baabb = surface.mesh_to_skeleton_xform.xform(skbones[j]);
					baabb = mtx.xform(baabb);

					if (!found_bone_aabb) {
						laabb = baabb;
						found_bone_aabb = true;
					}
					else {
						laabb.merge_with(baabb);
					}
				}
			}

			if (found_bone_aabb) {
				// Transform skeleton bounds back to mesh's space if any animated AABB applied.
				laabb = surface.mesh_to_skeleton_xform.affine_inverse().xform(laabb);
			}

			if (laabb.size == Vector3()) {
				laabb = surface.aabb;
			}
		}
		else {
			laabb = surface.aabb;
		}

		if (i == 0) {
			aabb = laabb;
		}
		else {
			aabb.merge_with(laabb);
		}
	}

	mesh->aabb = aabb;
	mesh->skeleton_aabb_version = skeleton->version;
	return aabb;
}

void MeshStorage::mesh_set_path(RID p_mesh, const String& p_path)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	mesh->path = p_path;
}

String MeshStorage::mesh_get_path(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, String());

	return mesh->path;
}

void MeshStorage::mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh)
{
	ERR_FAIL_COND_MSG(p_mesh == p_shadow_mesh, "Cannot set a mesh as its own shadow mesh.");
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	Mesh* shadow_mesh = mesh_owner.get_or_null(mesh->shadow_mesh);
	if (shadow_mesh) {
		shadow_mesh->shadow_owners.erase(mesh);
	}
	mesh->shadow_mesh = p_shadow_mesh;

	shadow_mesh = mesh_owner.get_or_null(mesh->shadow_mesh);

	if (shadow_mesh) {
		shadow_mesh->shadow_owners.insert(mesh);
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

void MeshStorage::mesh_clear(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	// Clear instance data before mesh data.
	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_clear(mi);
	}

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		_mesh_surface_clear(mesh, i);
	}
	if (mesh->surfaces) {
		memfree(mesh->surfaces);
	}

	mesh->surfaces = nullptr;
	mesh->surface_count = 0;
	mesh->material_cache.clear();
	mesh->has_bone_weights = false;
	mesh->aabb = AABB();
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}
}

void MeshStorage::_mesh_surface_generate_version_for_input_mask(Mesh::Surface::Version& v,
	Mesh::Surface* s, uint64_t p_input_mask, bool p_uses_motion_vectors, MeshInstance::Surface* mis,
	int p_current_vertex_buffer, int p_prev_vertex_buffer)
{
	Mesh::Surface::Attrib attribs[RSE::ARRAY_MAX];

	int position_stride = 0; // Vertex position only.
	int normal_tangent_stride = 0;
	int attributes_stride = 0;
	int skin_stride = 0;

	for (int i = 0; i < RSE::ARRAY_INDEX; i++) {
		attribs[i].enabled = false;
		attribs[i].integer = false;
		if (!(s->format & (1ULL << i))) {
			continue;
		}

		if ((p_input_mask & (1ULL << i))) {
			// Only enable if it matches input mask.
			// Iterate over all anyway, so we can calculate stride.
			attribs[i].enabled = true;
		}

		switch (i) {
		case RSE::ARRAY_VERTEX: {
			attribs[i].offset = 0;
			attribs[i].type = GL_FLOAT;
			attribs[i].normalized = GL_FALSE;
			if (s->format & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
				attribs[i].size = 2;
				position_stride = attribs[i].size * sizeof(float);
			}
			else {
				if (!mis && (s->format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
					attribs[i].size = 4;
					position_stride = attribs[i].size * sizeof(uint16_t);
					attribs[i].type = GL_UNSIGNED_SHORT;
					attribs[i].normalized = GL_TRUE;
				}
				else {
					attribs[i].size = 3;
					position_stride = attribs[i].size * sizeof(float);
				}
			}
		} break;
		case RSE::ARRAY_NORMAL: {
			if (!mis && (s->format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
				attribs[i].size = 2;
				normal_tangent_stride += 2 * attribs[i].size;
			}
			else {
				attribs[i].size = 4;
				// A small trick here: if we are uncompressed and we have normals, but no tangents.
				// We need the shader to think there are 4 components to "axis_tangent_attrib". So
				// we give a size of 4, but a stride based on only having 2 elements.
				if (!(s->format & RSE::ARRAY_FORMAT_TANGENT)) {
					normal_tangent_stride += (mis ? sizeof(float) : sizeof(uint16_t)) * 2;
				}
				else {
					normal_tangent_stride += (mis ? sizeof(float) : sizeof(uint16_t)) * 4;
				}
			}

			if (mis) {
				// Transform feedback has interleave all or no attributes. It can't mix
				// interleaving.
				attribs[i].offset = position_stride;
				normal_tangent_stride += position_stride;
				position_stride = normal_tangent_stride;
			}
			else {
				attribs[i].offset = position_stride * s->vertex_count;
			}
			attribs[i].type = (mis ? GL_FLOAT : GL_UNSIGNED_SHORT);
			attribs[i].normalized = GL_TRUE;
		} break;
		case RSE::ARRAY_TANGENT: {
			// We never use the tangent attribute. It is always packed in ARRAY_NORMAL, or
			// ARRAY_VERTEX.
			attribs[i].enabled = false;
			attribs[i].integer = false;
		} break;
		case RSE::ARRAY_COLOR: {
			attribs[i].offset = attributes_stride;
			attribs[i].size = 4;
			attribs[i].type = GL_UNSIGNED_BYTE;
			attributes_stride += 4;
			attribs[i].normalized = GL_TRUE;
		} break;
		case RSE::ARRAY_TEX_UV: {
			attribs[i].offset = attributes_stride;
			attribs[i].size = 2;
			if (s->format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
				attribs[i].type = GL_UNSIGNED_SHORT;
				attributes_stride += 2 * sizeof(uint16_t);
				attribs[i].normalized = GL_TRUE;
			}
			else {
				attribs[i].type = GL_FLOAT;
				attributes_stride += 2 * sizeof(float);
				attribs[i].normalized = GL_FALSE;
			}
		} break;
		case RSE::ARRAY_TEX_UV2: {
			attribs[i].offset = attributes_stride;
			attribs[i].size = 2;
			if (s->format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
				attribs[i].type = GL_UNSIGNED_SHORT;
				attributes_stride += 2 * sizeof(uint16_t);
				attribs[i].normalized = GL_TRUE;
			}
			else {
				attribs[i].type = GL_FLOAT;
				attributes_stride += 2 * sizeof(float);
				attribs[i].normalized = GL_FALSE;
			}
		} break;
		case RSE::ARRAY_CUSTOM0:
		case RSE::ARRAY_CUSTOM1:
		case RSE::ARRAY_CUSTOM2:
		case RSE::ARRAY_CUSTOM3: {
			attribs[i].offset = attributes_stride;

			int idx = i - RSE::ARRAY_CUSTOM0;
			uint32_t fmt_shift[RSE::ARRAY_CUSTOM_COUNT] = {RSE::ARRAY_FORMAT_CUSTOM0_SHIFT,
				RSE::ARRAY_FORMAT_CUSTOM1_SHIFT, RSE::ARRAY_FORMAT_CUSTOM2_SHIFT,
				RSE::ARRAY_FORMAT_CUSTOM3_SHIFT};
			uint32_t fmt = (s->format >> fmt_shift[idx]) & RSE::ARRAY_FORMAT_CUSTOM_MASK;
			uint32_t fmtsize[RSE::ARRAY_CUSTOM_MAX] = {4, 4, 4, 8, 4, 8, 12, 16};
			GLenum gl_type[RSE::ARRAY_CUSTOM_MAX] = {GL_UNSIGNED_BYTE, GL_BYTE, GL_HALF_FLOAT,
				GL_HALF_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT};
			GLboolean norm[RSE::ARRAY_CUSTOM_MAX] = {
				GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
			attribs[i].type = gl_type[fmt];
			attributes_stride += fmtsize[fmt];
			attribs[i].size = fmtsize[fmt] / sizeof(float);
			attribs[i].normalized = norm[fmt];
		} break;
		case RSE::ARRAY_BONES: {
			attribs[i].offset = skin_stride;
			attribs[i].size = 4;
			attribs[i].type = GL_UNSIGNED_SHORT;
			skin_stride += 4 * sizeof(uint16_t);
			attribs[i].normalized = GL_FALSE;
			attribs[i].integer = true;
		} break;
		case RSE::ARRAY_WEIGHTS: {
			attribs[i].offset = skin_stride;
			attribs[i].size = 4;
			attribs[i].type = GL_UNSIGNED_SHORT;
			skin_stride += 4 * sizeof(uint16_t);
			attribs[i].normalized = GL_TRUE;
		} break;
		}
	}

	glGenVertexArrays(1, &v.vertex_array);
	glBindVertexArray(v.vertex_array);

	for (int i = 0; i < RSE::ARRAY_INDEX; i++) {
		if (!attribs[i].enabled) {
			glDisableVertexAttribArray(i);
			continue;
		}
		if (i <= RSE::ARRAY_TANGENT) {
			attribs[i].stride = (i == RSE::ARRAY_VERTEX) ? position_stride : normal_tangent_stride;
			if (mis) {
				glBindBuffer(GL_ARRAY_BUFFER, mis->vertex_buffers[p_current_vertex_buffer]);
			}
			else {
				glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
			}
		}
		else if (i <= RSE::ARRAY_CUSTOM3) {
			attribs[i].stride = attributes_stride;
			glBindBuffer(GL_ARRAY_BUFFER, s->attribute_buffer);
		}
		else {
			attribs[i].stride = skin_stride;
			glBindBuffer(GL_ARRAY_BUFFER, s->skin_buffer);
		}

		if (attribs[i].integer) {
			glVertexAttribIPointer(i, attribs[i].size, attribs[i].type, attribs[i].stride,
				CAST_INT_TO_UCHAR_PTR(attribs[i].offset));
		}
		else {
			glVertexAttribPointer(i, attribs[i].size, attribs[i].type, attribs[i].normalized,
				attribs[i].stride, CAST_INT_TO_UCHAR_PTR(attribs[i].offset));
		}
		glEnableVertexAttribArray(i);
	}

	if (p_uses_motion_vectors) {
		for (int i = 0; i < RSE::ARRAY_TANGENT; i++) {
			if (mis) {
				glBindBuffer(GL_ARRAY_BUFFER, mis->vertex_buffers[mis->prev_vertex_buffer]);
			}
			else {
				glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
			}

			glVertexAttribPointer(i + 16, attribs[i].size, attribs[i].type, attribs[i].normalized,
				attribs[i].stride, CAST_INT_TO_UCHAR_PTR(attribs[i].offset));
			glEnableVertexAttribArray(i + 16);
		}
	}

	// Do not bind index here as we want to switch between index buffers for LOD

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	v.input_mask = p_input_mask;
	v.uses_motion_vectors = p_uses_motion_vectors;
	v.current_vertex_buffer = p_current_vertex_buffer;
	v.prev_vertex_buffer = p_prev_vertex_buffer;
}

void MeshStorage::mesh_surface_remove(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	// Clear instance data before mesh data.
	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_remove_surface(mi, p_surface);
	}

	_mesh_surface_clear(mesh, p_surface);

	if ((uint32_t)p_surface < mesh->surface_count - 1) {
		memmove(mesh->surfaces + p_surface, mesh->surfaces + p_surface + 1,
			sizeof(Mesh::Surface*) * (mesh->surface_count - (p_surface + 1)));
	}
	mesh->surfaces = (Mesh::Surface**)memrealloc(
		mesh->surfaces, sizeof(Mesh::Surface*) * (mesh->surface_count - 1));
	--mesh->surface_count;

	mesh->material_cache.clear();

	mesh->skeleton_aabb_version = 0;

	if (mesh->has_bone_weights) {
		mesh->has_bone_weights = false;
		for (uint32_t i = 0; i < mesh->surface_count; i++) {
			if (mesh->surfaces[i]->format & RSE::ARRAY_FORMAT_BONES) {
				mesh->has_bone_weights = true;
				break;
			}
		}
	}

	if (mesh->surface_count == 0) {
		mesh->aabb = AABB();
	}
	else {
		mesh->aabb = mesh->surfaces[0]->aabb;
		for (uint32_t i = 1; i < mesh->surface_count; i++) {
			mesh->aabb.merge_with(mesh->surfaces[i]->aabb);
		}
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}
}

/* MESH INSTANCE API */

RID MeshStorage::mesh_instance_create(RID p_base)
{
	Mesh* mesh = mesh_owner.get_or_null(p_base);
	ERR_FAIL_NULL_V(mesh, RID());

	RID rid = mesh_instance_owner.make_rid();
	MeshInstance* mi = mesh_instance_owner.get_or_null(rid);

	mi->mesh = mesh;

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		_mesh_instance_add_surface(mi, mesh, i);
	}

	mi->I = mesh->instances.push_back(mi);

	mi->dirty = true;

	return rid;
}

void MeshStorage::mesh_instance_free(RID p_rid)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_rid);
	_mesh_instance_clear(mi);
	mi->mesh->instances.erase(mi->I);
	mi->I = nullptr;

	mesh_instance_owner.free(p_rid);
}

void MeshStorage::mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	if (mi->skeleton == p_skeleton) {
		return;
	}
	mi->skeleton = p_skeleton;
	mi->skeleton_version = 0;
	mi->dirty = true;
}

void MeshStorage::mesh_instance_set_blend_shape_weight(
	RID p_mesh_instance, int p_shape, float p_weight)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	ERR_FAIL_NULL(mi);
	ERR_FAIL_INDEX(p_shape, (int)mi->blend_weights.size());
	mi->blend_weights[p_shape] = p_weight;
	mi->dirty = true;
}

void MeshStorage::_mesh_instance_clear(MeshInstance* mi)
{
	while (mi->surfaces.size()) {
		_mesh_instance_remove_surface(mi, mi->surfaces.size() - 1);
	}
	mi->dirty = false;
}

void MeshStorage::_mesh_instance_add_surface(MeshInstance* mi, Mesh* mesh, uint32_t p_surface)
{
	if (mesh->blend_shape_count > 0) {
		mi->blend_weights.resize(mesh->blend_shape_count);
		for (uint32_t i = 0; i < mi->blend_weights.size(); i++) {
			mi->blend_weights[i] = 0.0;
		}
	}

	MeshInstance::Surface s;
	if ((mesh->blend_shape_count > 0 ||
			(mesh->surfaces[p_surface]->format & RSE::ARRAY_FORMAT_BONES)) &&
		mesh->surfaces[p_surface]->vertex_buffer_size > 0) {
		// Cache surface properties
		s.format_cache = mesh->surfaces[p_surface]->format;
		if ((s.format_cache & (1ULL << RSE::ARRAY_VERTEX))) {
			if (s.format_cache & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
				s.vertex_size_cache = 2;
			}
			else {
				s.vertex_size_cache = 3;
			}
			s.vertex_stride_cache = sizeof(float) * s.vertex_size_cache;
		}
		if ((s.format_cache & (1ULL << RSE::ARRAY_NORMAL))) {
			s.vertex_normal_offset_cache = s.vertex_stride_cache;
			s.vertex_stride_cache += sizeof(uint32_t) * 2;
		}
		if ((s.format_cache & (1ULL << RSE::ARRAY_TANGENT))) {
			s.vertex_tangent_offset_cache = s.vertex_stride_cache;
			s.vertex_stride_cache += sizeof(uint32_t) * 2;
		}

		int buffer_size = s.vertex_stride_cache * mesh->surfaces[p_surface]->vertex_count;

		// First buffer to be used for rendering. Final output of skeleton and blend shapes.
		// If motion vectors are enabled, a second buffer will be created on demand, and they'll be
		// swapped every frame.
		glGenBuffers(1, &s.vertex_buffers[0]);
		glBindBuffer(GL_ARRAY_BUFFER, s.vertex_buffers[0]);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, s.vertex_buffers[0], buffer_size,
			nullptr, GL_DYNAMIC_DRAW, "MeshInstance vertex buffer");
		if (mesh->blend_shape_count > 0) {
			// Ping-Pong buffers for processing blendshapes.
			glGenBuffers(2, s.blend_shape_vertex_buffers);
			for (uint32_t i = 0; i < 2; i++) {
				glBindBuffer(GL_ARRAY_BUFFER, s.blend_shape_vertex_buffers[i]);
				GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
					s.blend_shape_vertex_buffers[i], buffer_size, nullptr, GL_DYNAMIC_DRAW,
					"MeshInstance process buffer[" + itos(i) + "]");
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}

	mi->surfaces.push_back(s);
	mi->dirty = true;
}

void MeshStorage::_mesh_instance_remove_surface(MeshInstance* mi, int p_surface)
{
	MeshInstance::Surface& surface = mi->surfaces[p_surface];

	if (surface.version_count != 0) {
		for (uint32_t j = 0; j < surface.version_count; j++) {
			glDeleteVertexArrays(1, &surface.versions[j].vertex_array);
			surface.versions[j].vertex_array = 0;
		}
		memfree(surface.versions);
	}

	if (surface.blend_shape_vertex_buffers[0] != 0) {
		GLES3::Utilities::buffer_free_data(surface.blend_shape_vertex_buffers[0]);
		GLES3::Utilities::buffer_free_data(surface.blend_shape_vertex_buffers[1]);
		surface.blend_shape_vertex_buffers[0] = 0;
		surface.blend_shape_vertex_buffers[1] = 0;
	}

	for (int i = 0; i < 2; i++) {
		if (surface.vertex_buffers[i] != 0) {
			GLES3::Utilities::buffer_free_data(surface.vertex_buffers[i]);
			surface.vertex_buffers[i] = 0;
		}
	}

	mi->surfaces.remove_at(p_surface);

	if (mi->surfaces.is_empty()) {
		mi->blend_weights.clear();
		mi->weights_dirty = false;
		mi->skeleton_version = 0;
	}
	mi->dirty = true;
}

void MeshStorage::mesh_instance_check_for_update(RID p_mesh_instance)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);

	bool needs_update = mi->dirty;

	if (mi->array_update_list.in_list()) {
		return;
	}

	if (!needs_update && mi->skeleton.is_valid()) {
		Skeleton* sk = skeleton_owner.get_or_null(mi->skeleton);
		if (sk && sk->version != mi->skeleton_version) {
			needs_update = true;
		}
	}

	if (needs_update) {
		dirty_mesh_instance_arrays.add(&mi->array_update_list);
	}
}

void MeshStorage::mesh_instance_set_canvas_item_transform(
	RID p_mesh_instance, const Transform2D& p_transform)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	mi->canvas_item_transform_2d = p_transform;
}

void MeshStorage::_blend_shape_bind_mesh_instance_buffer(MeshInstance* p_mi, uint32_t p_surface)
{
	glBindBuffer(GL_ARRAY_BUFFER, p_mi->surfaces[p_surface].blend_shape_vertex_buffers[0]);

	if ((p_mi->surfaces[p_surface].format_cache & (1ULL << RSE::ARRAY_VERTEX))) {
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, p_mi->surfaces[p_surface].vertex_size_cache,
			GL_FLOAT, GL_FALSE, p_mi->surfaces[p_surface].vertex_stride_cache,
			CAST_INT_TO_UCHAR_PTR(0));
	}
	else {
		glDisableVertexAttribArray(RSE::ARRAY_VERTEX);
	}
	if ((p_mi->surfaces[p_surface].format_cache & (1ULL << RSE::ARRAY_NORMAL))) {
		glEnableVertexAttribArray(RSE::ARRAY_NORMAL);
		glVertexAttribIPointer(RSE::ARRAY_NORMAL, 2, GL_UNSIGNED_INT,
			p_mi->surfaces[p_surface].vertex_stride_cache,
			CAST_INT_TO_UCHAR_PTR(p_mi->surfaces[p_surface].vertex_normal_offset_cache));
	}
	else {
		glDisableVertexAttribArray(RSE::ARRAY_NORMAL);
	}
	if ((p_mi->surfaces[p_surface].format_cache & (1ULL << RSE::ARRAY_TANGENT))) {
		glEnableVertexAttribArray(RSE::ARRAY_TANGENT);
		glVertexAttribIPointer(RSE::ARRAY_TANGENT, 2, GL_UNSIGNED_INT,
			p_mi->surfaces[p_surface].vertex_stride_cache,
			CAST_INT_TO_UCHAR_PTR(p_mi->surfaces[p_surface].vertex_tangent_offset_cache));
	}
	else {
		glDisableVertexAttribArray(RSE::ARRAY_TANGENT);
	}
}

void MeshStorage::_compute_skeleton(MeshInstance* p_mi, Skeleton* p_sk, uint32_t p_surface)
{
	// Add in the bones and weights.
	glBindBuffer(GL_ARRAY_BUFFER, p_mi->mesh->surfaces[p_surface]->skin_buffer);

	bool use_8_weights =
		p_mi->surfaces[p_surface].format_cache & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	int skin_stride = sizeof(int16_t) * (use_8_weights ? 16 : 8);
	glEnableVertexAttribArray(RSE::ARRAY_BONES);
	glVertexAttribIPointer(
		RSE::ARRAY_BONES, 4, GL_UNSIGNED_SHORT, skin_stride, CAST_INT_TO_UCHAR_PTR(0));
	if (use_8_weights) {
		glEnableVertexAttribArray(11);
		glVertexAttribIPointer(
			11, 4, GL_UNSIGNED_SHORT, skin_stride, CAST_INT_TO_UCHAR_PTR(4 * sizeof(uint16_t)));
		glEnableVertexAttribArray(12);
		glVertexAttribPointer(12, 4, GL_UNSIGNED_SHORT, GL_TRUE, skin_stride,
			CAST_INT_TO_UCHAR_PTR(8 * sizeof(uint16_t)));
		glEnableVertexAttribArray(13);
		glVertexAttribPointer(13, 4, GL_UNSIGNED_SHORT, GL_TRUE, skin_stride,
			CAST_INT_TO_UCHAR_PTR(12 * sizeof(uint16_t)));
	}
	else {
		glEnableVertexAttribArray(RSE::ARRAY_WEIGHTS);
		glVertexAttribPointer(RSE::ARRAY_WEIGHTS, 4, GL_UNSIGNED_SHORT, GL_TRUE, skin_stride,
			CAST_INT_TO_UCHAR_PTR(4 * sizeof(uint16_t)));
	}

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
		p_mi->surfaces[p_surface].vertex_buffers[p_mi->surfaces[p_surface].current_vertex_buffer]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_sk->transforms_texture);

	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, p_mi->mesh->surfaces[p_surface]->vertex_count);
	glEndTransformFeedback();

	glDisableVertexAttribArray(RSE::ARRAY_BONES);
	glDisableVertexAttribArray(RSE::ARRAY_WEIGHTS);
	glDisableVertexAttribArray(RSE::ARRAY_BONES + 2);
	glDisableVertexAttribArray(RSE::ARRAY_WEIGHTS + 2);
	glBindVertexArray(0);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
}

void MeshStorage::update_mesh_instances()
{
	if (dirty_mesh_instance_arrays.first() == nullptr) {
		return; // nothing to do
	}

	glEnable(GL_RASTERIZER_DISCARD);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	// Process skeletons and blend shapes using transform feedback
	while (dirty_mesh_instance_arrays.first()) {
		MeshInstance* mi = dirty_mesh_instance_arrays.first()->self();

		bool uses_motion_vectors = Renderer::viewport->get_num_viewports_with_motion_vectors() > 0;
		int frame = RendererCompositor::get_frame_number();
		if (uses_motion_vectors) {
			for (uint32_t i = 0; i < mi->surfaces.size(); i++) {
				mi->surfaces[i].prev_vertex_buffer = mi->surfaces[i].current_vertex_buffer;

				if (frame - mi->surfaces[i].last_change == 1) {
					// Previous buffer's data can only be one frame old to be able to use motion
					// vectors.
					uint32_t new_buffer_index = mi->surfaces[i].current_vertex_buffer ^ 1;

					if (mi->surfaces[i].vertex_buffers[new_buffer_index] == 0) {
						// Create the new vertex buffer on demand where the result for the current
						// frame will be stored.
						GLuint new_vertex_buffer = 0;
						GLES3::Mesh::Surface* surface = mi->mesh->surfaces[i];
						int buffer_size =
							mi->surfaces[i].vertex_stride_cache * surface->vertex_count;
						glGenBuffers(1, &new_vertex_buffer);
						glBindBuffer(GL_ARRAY_BUFFER, new_vertex_buffer);
						GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, new_vertex_buffer,
							buffer_size, nullptr,
							(surface->format & RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW
																				   : GL_STATIC_DRAW,
							"Secondary mesh vertex buffer");
						glBindBuffer(GL_ARRAY_BUFFER, 0);

						mi->surfaces[i].vertex_buffers[new_buffer_index] = new_vertex_buffer;
					}

					mi->surfaces[i].current_vertex_buffer = new_buffer_index;
				}

				mi->surfaces[i].last_change = frame;
			}
		}

		Skeleton* sk = skeleton_owner.get_or_null(mi->skeleton);

		// Precompute base weight if using blend shapes.
		float base_weight = 1.0;
		if (mi->surfaces.size() && mi->mesh->blend_shape_count &&
			mi->mesh->blend_shape_mode == RSE::BLEND_SHAPE_MODE_NORMALIZED) {
			for (uint32_t i = 0; i < mi->mesh->blend_shape_count; i++) {
				base_weight -= mi->blend_weights[i];
			}
		}

		for (uint32_t i = 0; i < mi->surfaces.size(); i++) {
			if (mi->surfaces[i].vertex_buffers[mi->surfaces[i].current_vertex_buffer] == 0) {
				continue;
			}

			bool array_is_2d = mi->surfaces[i].format_cache & RSE::ARRAY_FLAG_USE_2D_VERTICES;
			bool can_use_skeleton = sk != nullptr && sk->use_2d == array_is_2d &&
									(mi->surfaces[i].format_cache & RSE::ARRAY_FORMAT_BONES);
			bool use_8_weights = mi->surfaces[i].format_cache & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS;

			// Always process blend shapes first.
			if (mi->mesh->blend_shape_count) {
				SkeletonShaderGLES3::ShaderVariant variant = SkeletonShaderGLES3::MODE_BASE_PASS;
				uint64_t specialization = 0;
				specialization |= array_is_2d ? SkeletonShaderGLES3::MODE_2D : 0;
				specialization |= SkeletonShaderGLES3::USE_BLEND_SHAPES;
				if (!array_is_2d) {
					if ((mi->surfaces[i].format_cache & (1ULL << RSE::ARRAY_NORMAL))) {
						specialization |= SkeletonShaderGLES3::USE_NORMAL;
					}
					if ((mi->surfaces[i].format_cache & (1ULL << RSE::ARRAY_TANGENT))) {
						specialization |= SkeletonShaderGLES3::USE_TANGENT;
					}
				}

				bool success = skeleton_shader->shader.version_bind_shader(
					skeleton_shader->shader_version, variant, specialization);
				if (!success) {
					continue;
				}

				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::BLEND_WEIGHT,
					base_weight, skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::BLEND_SHAPE_COUNT,
					float(mi->mesh->blend_shape_count), skeleton_shader->shader_version, variant,
					specialization);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				GLuint vertex_array_gl = 0;
				uint64_t mask =
					RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_NORMAL | RSE::ARRAY_FORMAT_VERTEX;
				uint64_t format =
					mi->mesh->surfaces[i]->format &
					mask; // Format should only have vertex, normal, tangent (as necessary).
				mesh_surface_get_vertex_arrays_and_format(
					mi->mesh->surfaces[i], format, false, vertex_array_gl);
				glBindVertexArray(vertex_array_gl);
				glBindBufferBase(
					GL_TRANSFORM_FEEDBACK_BUFFER, 0, mi->surfaces[i].blend_shape_vertex_buffers[0]);
				glBeginTransformFeedback(GL_POINTS);
				glDrawArrays(GL_POINTS, 0, mi->mesh->surfaces[i]->vertex_count);
				glEndTransformFeedback();

				variant = SkeletonShaderGLES3::MODE_BLEND_PASS;
				success = skeleton_shader->shader.version_bind_shader(
					skeleton_shader->shader_version, variant, specialization);
				if (!success) {
					continue;
				}

				// Do the last blend shape separately, as it can be combined with the skeleton pass.
				for (uint32_t bs = 0; bs < mi->mesh->blend_shape_count - 1; bs++) {
					float weight = mi->blend_weights[bs];

					if (Math::is_zero_approx(weight)) {
						// not bother with this one
						continue;
					}
					skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::BLEND_WEIGHT,
						weight, skeleton_shader->shader_version, variant, specialization);
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::BLEND_SHAPE_COUNT, float(mi->mesh->blend_shape_count),
						skeleton_shader->shader_version, variant, specialization);

					// Ensure the skeleton shader outputs to the correct (current) VBO.

					glBindVertexArray(mi->mesh->surfaces[i]->blend_shapes[bs].vertex_array);
					_blend_shape_bind_mesh_instance_buffer(mi, i);
					glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
						mi->surfaces[i].blend_shape_vertex_buffers[1]);

					glBeginTransformFeedback(GL_POINTS);
					glDrawArrays(GL_POINTS, 0, mi->mesh->surfaces[i]->vertex_count);
					glEndTransformFeedback();

					SWAP(mi->surfaces[i].blend_shape_vertex_buffers[0],
						mi->surfaces[i].blend_shape_vertex_buffers[1]);
				}
				uint32_t bs = mi->mesh->blend_shape_count - 1;

				float weight = mi->blend_weights[bs];

				glBindVertexArray(mi->mesh->surfaces[i]->blend_shapes[bs].vertex_array);
				_blend_shape_bind_mesh_instance_buffer(mi, i);

				specialization |= can_use_skeleton ? SkeletonShaderGLES3::USE_SKELETON : 0;
				specialization |= (can_use_skeleton && use_8_weights)
									  ? SkeletonShaderGLES3::USE_EIGHT_WEIGHTS
									  : 0;
				specialization |= SkeletonShaderGLES3::FINAL_PASS;
				success = skeleton_shader->shader.version_bind_shader(
					skeleton_shader->shader_version, variant, specialization);
				if (!success) {
					continue;
				}

				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::BLEND_WEIGHT,
					weight, skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::BLEND_SHAPE_COUNT,
					float(mi->mesh->blend_shape_count), skeleton_shader->shader_version, variant,
					specialization);

				if (can_use_skeleton) {
					Transform2D transform =
						mi->canvas_item_transform_2d.affine_inverse() * sk->base_transform_2d;
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::SKELETON_TRANSFORM_X, transform[0],
						skeleton_shader->shader_version, variant, specialization);
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::SKELETON_TRANSFORM_Y, transform[1],
						skeleton_shader->shader_version, variant, specialization);
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::SKELETON_TRANSFORM_OFFSET, transform[2],
						skeleton_shader->shader_version, variant, specialization);

					Transform2D inverse_transform = transform.affine_inverse();
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::INVERSE_TRANSFORM_X, inverse_transform[0],
						skeleton_shader->shader_version, variant, specialization);
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::INVERSE_TRANSFORM_Y, inverse_transform[1],
						skeleton_shader->shader_version, variant, specialization);
					skeleton_shader->shader.version_set_uniform(
						SkeletonShaderGLES3::INVERSE_TRANSFORM_OFFSET, inverse_transform[2],
						skeleton_shader->shader_version, variant, specialization);

					// Do last blendshape in the same pass as the Skeleton.
					_compute_skeleton(mi, sk, i);
					can_use_skeleton = false;
				}
				else {
					// Do last blendshape by itself and prepare vertex data for use by the renderer.
					glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
						mi->surfaces[i].vertex_buffers[mi->surfaces[i].current_vertex_buffer]);

					glBeginTransformFeedback(GL_POINTS);
					glDrawArrays(GL_POINTS, 0, mi->mesh->surfaces[i]->vertex_count);
					glEndTransformFeedback();
				}

				glBindVertexArray(0);
				glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
			}

			// This branch should only execute when Skeleton is run by itself.
			if (can_use_skeleton) {
				SkeletonShaderGLES3::ShaderVariant variant = SkeletonShaderGLES3::MODE_BASE_PASS;
				uint64_t specialization = 0;
				specialization |= array_is_2d ? SkeletonShaderGLES3::MODE_2D : 0;
				specialization |= SkeletonShaderGLES3::USE_SKELETON;
				specialization |= SkeletonShaderGLES3::FINAL_PASS;
				specialization |= use_8_weights ? SkeletonShaderGLES3::USE_EIGHT_WEIGHTS : 0;
				if (!array_is_2d) {
					if ((mi->surfaces[i].format_cache & (1ULL << RSE::ARRAY_NORMAL))) {
						specialization |= SkeletonShaderGLES3::USE_NORMAL;
					}
					if ((mi->surfaces[i].format_cache & (1ULL << RSE::ARRAY_TANGENT))) {
						specialization |= SkeletonShaderGLES3::USE_TANGENT;
					}
				}

				bool success = skeleton_shader->shader.version_bind_shader(
					skeleton_shader->shader_version, variant, specialization);
				if (!success) {
					continue;
				}

				Transform2D transform =
					mi->canvas_item_transform_2d.affine_inverse() * sk->base_transform_2d;
				skeleton_shader->shader.version_set_uniform(
					SkeletonShaderGLES3::SKELETON_TRANSFORM_X, transform[0],
					skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(
					SkeletonShaderGLES3::SKELETON_TRANSFORM_Y, transform[1],
					skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(
					SkeletonShaderGLES3::SKELETON_TRANSFORM_OFFSET, transform[2],
					skeleton_shader->shader_version, variant, specialization);

				Transform2D inverse_transform = transform.affine_inverse();
				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::INVERSE_TRANSFORM_X,
					inverse_transform[0], skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(SkeletonShaderGLES3::INVERSE_TRANSFORM_Y,
					inverse_transform[1], skeleton_shader->shader_version, variant, specialization);
				skeleton_shader->shader.version_set_uniform(
					SkeletonShaderGLES3::INVERSE_TRANSFORM_OFFSET, inverse_transform[2],
					skeleton_shader->shader_version, variant, specialization);

				GLuint vertex_array_gl = 0;
				uint64_t mask =
					RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_NORMAL | RSE::ARRAY_FORMAT_VERTEX;
				uint64_t format =
					mi->mesh->surfaces[i]->format &
					mask; // Format should only have vertex, normal, tangent (as necessary).
				mesh_surface_get_vertex_arrays_and_format(
					mi->mesh->surfaces[i], format, false, vertex_array_gl);
				glBindVertexArray(vertex_array_gl);
				_compute_skeleton(mi, sk, i);
			}
		}
		mi->dirty = false;
		if (sk) {
			mi->skeleton_version = sk->version;
		}
		dirty_mesh_instance_arrays.remove(&mi->array_update_list);
	}
	glDisable(GL_RASTERIZER_DISCARD);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

/* MULTIMESH API */

RID MeshStorage::_multimesh_allocate() { return multimesh_owner.allocate_rid(); }

void MeshStorage::_multimesh_initialize(RID p_rid)
{
	multimesh_owner.initialize_rid(p_rid, MultiMesh());
}

void MeshStorage::multimesh_allocate_data(
	RID p_rid, int p_instances, RSE::MultimeshTransformFormat p_transform_format)
{
	_multimesh_allocate_data(p_rid, p_instances, p_transform_format, true, false, false);
}

void MeshStorage::_multimesh_free(RID p_rid)
{
	// Remove from interpolator.
	_interpolation_data.notify_free_multimesh(p_rid);
	_update_dirty_multimeshes();
	multimesh_allocate_data(p_rid, 0, RSE::MULTIMESH_TRANSFORM_2D);
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_rid);
	multimesh->dependency.deleted_notify(p_rid);
	multimesh_owner.free(p_rid);
}

void MeshStorage::_multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);

	if (multimesh->instances == p_instances && multimesh->xform_format == p_transform_format &&
		multimesh->uses_colors == p_use_colors &&
		multimesh->uses_custom_data == p_use_custom_data) {
		return;
	}

	for (int i = 0; i < 2; i++) {
		if (multimesh->buffer[i] != 0) {
			GLES3::Utilities::buffer_free_data(multimesh->buffer[i]);
			multimesh->buffer[i] = 0;
		}
	}

	if (multimesh->data_cache_dirty_regions) {
		memdelete_arr(multimesh->data_cache_dirty_regions);
		multimesh->data_cache_dirty_regions = nullptr;
		multimesh->data_cache_used_dirty_regions = 0;
	}

	// If we have either color or custom data, reserve space for both to make data handling logic
	// simpler. This way we can always treat them both as a single, compressed uvec4.
	int color_and_custom_strides = (p_use_colors || p_use_custom_data) ? 2 : 0;

	multimesh->instances = p_instances;
	multimesh->xform_format = p_transform_format;
	multimesh->uses_colors = p_use_colors;
	multimesh->color_offset_cache = p_transform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
	multimesh->uses_custom_data = p_use_custom_data;
	multimesh->custom_data_offset_cache = multimesh->color_offset_cache + color_and_custom_strides;
	multimesh->stride_cache = multimesh->custom_data_offset_cache + color_and_custom_strides;
	multimesh->buffer_set = false;

	multimesh->data_cache = Vector<float>();
	multimesh->aabb = AABB();
	multimesh->aabb_dirty = false;
	multimesh->visible_instances = MIN(multimesh->visible_instances, multimesh->instances);

	if (multimesh->instances) {
		uint32_t buffer_size = multimesh->instances * multimesh->stride_cache * sizeof(float);

		LocalVector<uint8_t> zeros;
		zeros.resize_initialized(buffer_size);

		glGenBuffers(1, &multimesh->buffer[0]);
		glBindBuffer(GL_ARRAY_BUFFER, multimesh->buffer[0]);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, multimesh->buffer[0], buffer_size,
			zeros.ptr(), GL_STATIC_DRAW, "MultiMesh buffer");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MULTIMESH);
}

int MeshStorage::_multimesh_get_instance_count(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, 0);
	return multimesh->instances;
}

void MeshStorage::_multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	if (multimesh->mesh == p_mesh || p_mesh.is_null()) {
		return;
	}
	multimesh->mesh = p_mesh;

	if (multimesh->instances == 0) {
		return;
	}

	if (multimesh->data_cache.size()) {
		// we have a data cache, just mark it dirty
		_multimesh_mark_all_dirty(multimesh, false, true);
	}
	else if (multimesh->instances) {
		// Need to re-create AABB. Unfortunately, calling this has a penalty.
		if (multimesh->buffer_set) {
			Vector<uint8_t> buffer = Utilities::buffer_get_data(GL_ARRAY_BUFFER,
				multimesh->buffer[multimesh->current_buffer],
				multimesh->instances * multimesh->stride_cache * sizeof(float));
			const uint8_t* r = buffer.ptr();
			const float* data = (const float*)r;
			_multimesh_re_create_aabb(multimesh, data, multimesh->instances);
		}
	}

	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

#define MULTIMESH_DIRTY_REGION_SIZE 512

void MeshStorage::_multimesh_make_local(MultiMesh* multimesh)
{
	if (multimesh->data_cache.size() > 0 || multimesh->instances == 0) {
		return; // already local
	}
	ERR_FAIL_COND(multimesh->data_cache.size() > 0);
	// this means that the user wants to load/save individual elements,
	// for this, the data must reside on CPU, so just copy it there.
	multimesh->data_cache.resize(multimesh->instances * multimesh->stride_cache);
	{
		float* w = multimesh->data_cache.ptrw();

		if (multimesh->buffer_set) {
			Vector<uint8_t> buffer = Utilities::buffer_get_data(GL_ARRAY_BUFFER,
				multimesh->buffer[multimesh->current_buffer],
				multimesh->instances * multimesh->stride_cache * sizeof(float));

			{
				const uint8_t* r = buffer.ptr();
				memcpy(w, r, buffer.size());
			}
		}
		else {
			memset(w, 0, (size_t)multimesh->instances * multimesh->stride_cache * sizeof(float));
		}
	}
	uint32_t data_cache_dirty_region_count =
		Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);
	multimesh->data_cache_dirty_regions = memnew_arr(bool, data_cache_dirty_region_count);
	for (uint32_t i = 0; i < data_cache_dirty_region_count; i++) {
		multimesh->data_cache_dirty_regions[i] = false;
	}
	multimesh->data_cache_used_dirty_regions = 0;
}

void MeshStorage::_multimesh_mark_dirty(MultiMesh* multimesh, int p_index, bool p_aabb)
{
	uint32_t region_index = p_index / MULTIMESH_DIRTY_REGION_SIZE;
#ifdef DEBUG_ENABLED
	uint32_t data_cache_dirty_region_count =
		Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);
	ERR_FAIL_UNSIGNED_INDEX(region_index, data_cache_dirty_region_count); // bug
#endif
	if (!multimesh->data_cache_dirty_regions[region_index]) {
		multimesh->data_cache_dirty_regions[region_index] = true;
		multimesh->data_cache_used_dirty_regions++;
	}

	if (p_aabb) {
		multimesh->aabb_dirty = true;
	}

	if (!multimesh->dirty) {
		multimesh->dirty_list = multimesh_dirty_list;
		multimesh_dirty_list = multimesh;
		multimesh->dirty = true;
	}
}

void MeshStorage::_multimesh_mark_all_dirty(MultiMesh* multimesh, bool p_data, bool p_aabb)
{
	if (p_data) {
		uint32_t data_cache_dirty_region_count =
			Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);

		for (uint32_t i = 0; i < data_cache_dirty_region_count; i++) {
			if (!multimesh->data_cache_dirty_regions[i]) {
				multimesh->data_cache_dirty_regions[i] = true;
				multimesh->data_cache_used_dirty_regions++;
			}
		}
	}

	if (p_aabb) {
		multimesh->aabb_dirty = true;
	}

	if (!multimesh->dirty) {
		multimesh->dirty_list = multimesh_dirty_list;
		multimesh_dirty_list = multimesh;
		multimesh->dirty = true;
	}
}

void MeshStorage::_multimesh_re_create_aabb(
	MultiMesh* multimesh, const float* p_data, int p_instances)
{
	ERR_FAIL_COND(multimesh->mesh.is_null());
	if (multimesh->custom_aabb != AABB()) {
		return;
	}
	AABB aabb;
	AABB mesh_aabb = mesh_get_aabb(multimesh->mesh);
	for (int i = 0; i < p_instances; i++) {
		const float* data = p_data + multimesh->stride_cache * i;
		Transform3D t;

		if (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_3D) {
			t.basis.rows[0][0] = data[0];
			t.basis.rows[0][1] = data[1];
			t.basis.rows[0][2] = data[2];
			t.origin.x = data[3];
			t.basis.rows[1][0] = data[4];
			t.basis.rows[1][1] = data[5];
			t.basis.rows[1][2] = data[6];
			t.origin.y = data[7];
			t.basis.rows[2][0] = data[8];
			t.basis.rows[2][1] = data[9];
			t.basis.rows[2][2] = data[10];
			t.origin.z = data[11];

		}
		else {
			t.basis.rows[0][0] = data[0];
			t.basis.rows[0][1] = data[1];
			t.origin.x = data[3];

			t.basis.rows[1][0] = data[4];
			t.basis.rows[1][1] = data[5];
			t.origin.y = data[7];
		}

		if (i == 0) {
			aabb = t.xform(mesh_aabb);
		}
		else {
			aabb.merge_with(t.xform(mesh_aabb));
		}
	}

	multimesh->aabb = aabb;
}

void MeshStorage::_multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_3D);

	_multimesh_make_local(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr = w + p_index * multimesh->stride_cache;

		dataptr[0] = p_transform.basis.rows[0][0];
		dataptr[1] = p_transform.basis.rows[0][1];
		dataptr[2] = p_transform.basis.rows[0][2];
		dataptr[3] = p_transform.origin.x;
		dataptr[4] = p_transform.basis.rows[1][0];
		dataptr[5] = p_transform.basis.rows[1][1];
		dataptr[6] = p_transform.basis.rows[1][2];
		dataptr[7] = p_transform.origin.y;
		dataptr[8] = p_transform.basis.rows[2][0];
		dataptr[9] = p_transform.basis.rows[2][1];
		dataptr[10] = p_transform.basis.rows[2][2];
		dataptr[11] = p_transform.origin.z;
	}

	_multimesh_mark_dirty(multimesh, p_index, true);
}

void MeshStorage::_multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_2D);

	_multimesh_make_local(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr = w + p_index * multimesh->stride_cache;

		dataptr[0] = p_transform.columns[0][0];
		dataptr[1] = p_transform.columns[1][0];
		dataptr[2] = 0;
		dataptr[3] = p_transform.columns[2][0];
		dataptr[4] = p_transform.columns[0][1];
		dataptr[5] = p_transform.columns[1][1];
		dataptr[6] = 0;
		dataptr[7] = p_transform.columns[2][1];
	}

	_multimesh_mark_dirty(multimesh, p_index, true);
}

void MeshStorage::_multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(!multimesh->uses_colors);

	_multimesh_make_local(multimesh);

	{
		// Colors are packed into 2 floats.
		float* w = multimesh->data_cache.ptrw();

		float* dataptr = w + p_index * multimesh->stride_cache + multimesh->color_offset_cache;
		uint16_t val[4] = {Math::make_half_float(p_color.r), Math::make_half_float(p_color.g),
			Math::make_half_float(p_color.b), Math::make_half_float(p_color.a)};
		memcpy(dataptr, val, 2 * 4);
	}

	_multimesh_mark_dirty(multimesh, p_index, false);
}

void MeshStorage::_multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(!multimesh->uses_custom_data);

	_multimesh_make_local(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr =
			w + p_index * multimesh->stride_cache + multimesh->custom_data_offset_cache;
		uint16_t val[4] = {Math::make_half_float(p_color.r), Math::make_half_float(p_color.g),
			Math::make_half_float(p_color.b), Math::make_half_float(p_color.a)};
		memcpy(dataptr, val, 2 * 4);
	}

	_multimesh_mark_dirty(multimesh, p_index, false);
}

RID MeshStorage::_multimesh_get_mesh(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, RID());

	return multimesh->mesh;
}

void MeshStorage::_multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	multimesh->custom_aabb = p_aabb;
	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB MeshStorage::_multimesh_get_custom_aabb(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, AABB());
	return multimesh->custom_aabb;
}

AABB MeshStorage::_multimesh_get_aabb(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, AABB());
	if (multimesh->custom_aabb != AABB()) {
		return multimesh->custom_aabb;
	}
	if (multimesh->aabb_dirty) {
		_update_dirty_multimeshes();
	}
	return multimesh->aabb;
}

Transform3D MeshStorage::_multimesh_instance_get_transform(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Transform3D());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Transform3D());
	ERR_FAIL_COND_V(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_3D, Transform3D());

	_multimesh_make_local(multimesh);

	Transform3D t;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr = r + p_index * multimesh->stride_cache;

		t.basis.rows[0][0] = dataptr[0];
		t.basis.rows[0][1] = dataptr[1];
		t.basis.rows[0][2] = dataptr[2];
		t.origin.x = dataptr[3];
		t.basis.rows[1][0] = dataptr[4];
		t.basis.rows[1][1] = dataptr[5];
		t.basis.rows[1][2] = dataptr[6];
		t.origin.y = dataptr[7];
		t.basis.rows[2][0] = dataptr[8];
		t.basis.rows[2][1] = dataptr[9];
		t.basis.rows[2][2] = dataptr[10];
		t.origin.z = dataptr[11];
	}

	return t;
}

Transform2D MeshStorage::_multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Transform2D());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Transform2D());
	ERR_FAIL_COND_V(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_2D, Transform2D());

	_multimesh_make_local(multimesh);

	Transform2D t;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr = r + p_index * multimesh->stride_cache;

		t.columns[0][0] = dataptr[0];
		t.columns[1][0] = dataptr[1];
		t.columns[2][0] = dataptr[3];
		t.columns[0][1] = dataptr[4];
		t.columns[1][1] = dataptr[5];
		t.columns[2][1] = dataptr[7];
	}

	return t;
}

Color MeshStorage::_multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Color());
	ERR_FAIL_COND_V(!multimesh->uses_colors, Color());

	_multimesh_make_local(multimesh);

	Color c;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + p_index * multimesh->stride_cache + multimesh->color_offset_cache;
		uint16_t raw_data[4];
		memcpy(raw_data, dataptr, 2 * 4);
		c.r = Math::half_to_float(raw_data[0]);
		c.g = Math::half_to_float(raw_data[1]);
		c.b = Math::half_to_float(raw_data[2]);
		c.a = Math::half_to_float(raw_data[3]);
	}

	return c;
}

Color MeshStorage::_multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Color());
	ERR_FAIL_COND_V(!multimesh->uses_custom_data, Color());

	_multimesh_make_local(multimesh);

	Color c;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + p_index * multimesh->stride_cache + multimesh->custom_data_offset_cache;
		uint16_t raw_data[4];
		memcpy(raw_data, dataptr, 2 * 4);
		c.r = Math::half_to_float(raw_data[0]);
		c.g = Math::half_to_float(raw_data[1]);
		c.b = Math::half_to_float(raw_data[2]);
		c.a = Math::half_to_float(raw_data[3]);
	}

	return c;
}

void MeshStorage::_multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);

	// Assign data to previous buffer if motion vectors are used, that data will be made current in
	// _update_dirty_multimeshes().
	bool uses_motion_vectors = Renderer::viewport->get_num_viewports_with_motion_vectors() > 0;
	int buffer_index = uses_motion_vectors ? multimesh->prev_buffer : multimesh->current_buffer;

	if (multimesh->uses_colors || multimesh->uses_custom_data) {
		// Color and custom need to be packed so copy buffer to data_cache and pack.

		_multimesh_make_local(multimesh);

		uint32_t old_stride = multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
		old_stride += multimesh->uses_colors ? 4 : 0;
		old_stride += multimesh->uses_custom_data ? 4 : 0;
		ERR_FAIL_COND(p_buffer.size() != (multimesh->instances * (int)old_stride));

		multimesh->data_cache = p_buffer;

		float* w = multimesh->data_cache.ptrw();

		for (int i = 0; i < multimesh->instances; i++) {
			{
				float* dataptr = w + i * old_stride;
				float* newptr = w + i * multimesh->stride_cache;
				float vals[8] = {dataptr[0], dataptr[1], dataptr[2], dataptr[3], dataptr[4],
					dataptr[5], dataptr[6], dataptr[7]};
				memcpy(newptr, vals, 8 * 4);
			}

			if (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_3D) {
				float* dataptr = w + i * old_stride + 8;
				float* newptr = w + i * multimesh->stride_cache + 8;
				float vals[8] = {dataptr[0], dataptr[1], dataptr[2], dataptr[3]};
				memcpy(newptr, vals, 4 * 4);
			}

			if (multimesh->uses_colors) {
				float* dataptr = w + i * old_stride +
								 (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12);
				float* newptr = w + i * multimesh->stride_cache + multimesh->color_offset_cache;
				uint16_t val[4] = {Math::make_half_float(dataptr[0]),
					Math::make_half_float(dataptr[1]), Math::make_half_float(dataptr[2]),
					Math::make_half_float(dataptr[3])};
				memcpy(newptr, val, 2 * 4);
			}
			if (multimesh->uses_custom_data) {
				float* dataptr = w + i * old_stride +
								 (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12) +
								 (multimesh->uses_colors ? 4 : 0);
				float* newptr =
					w + i * multimesh->stride_cache + multimesh->custom_data_offset_cache;
				uint16_t val[4] = {Math::make_half_float(dataptr[0]),
					Math::make_half_float(dataptr[1]), Math::make_half_float(dataptr[2]),
					Math::make_half_float(dataptr[3])};
				memcpy(newptr, val, 2 * 4);
			}
		}

		multimesh->data_cache.resize(multimesh->instances * (int)multimesh->stride_cache);
		const float* r = multimesh->data_cache.ptr();
		glBindBuffer(GL_ARRAY_BUFFER, multimesh->buffer[buffer_index]);
		glBufferData(
			GL_ARRAY_BUFFER, multimesh->data_cache.size() * sizeof(float), r, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}
	else {
		// If we have a data cache, just update it.
		if (multimesh->data_cache.size()) {
			multimesh->data_cache = p_buffer;
		}

		// Only Transform is being used, so we can upload directly.
		ERR_FAIL_COND(p_buffer.size() != (multimesh->instances * (int)multimesh->stride_cache));
		const float* r = p_buffer.ptr();
		glBindBuffer(GL_ARRAY_BUFFER, multimesh->buffer[buffer_index]);
		glBufferData(GL_ARRAY_BUFFER, p_buffer.size() * sizeof(float), r, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	multimesh->buffer_set = true;

	if (multimesh->data_cache.size() || multimesh->uses_colors || multimesh->uses_custom_data) {
		// Clear dirty since nothing will be dirty anymore.
		uint32_t data_cache_dirty_region_count =
			Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);
		for (uint32_t i = 0; i < data_cache_dirty_region_count; i++) {
			multimesh->data_cache_dirty_regions[i] = false;
		}
		multimesh->data_cache_used_dirty_regions = 0;

		_multimesh_mark_all_dirty(multimesh, false, true); // update AABB
	}
	else if (multimesh->mesh.is_valid()) {
		// if we have a mesh set, we need to re-generate the AABB from the new data
		const float* data = p_buffer.ptr();

		if (multimesh->custom_aabb == AABB()) {
			_multimesh_re_create_aabb(multimesh, data, multimesh->instances);
			multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
		}
	}
}

RID MeshStorage::_multimesh_get_command_buffer_rd_rid(RID p_multimesh)
{
	ERR_FAIL_V_MSG(RID(), "GLES3 does not implement indirect multimeshes.");
}

RID MeshStorage::_multimesh_get_buffer_rd_rid(RID p_multimesh)
{
	ERR_FAIL_V_MSG(RID(), "GLES3 does not contain a Rid for the multimesh buffer.");
}

Vector<float> MeshStorage::_multimesh_get_buffer(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Vector<float>());
	Vector<float> ret;
	if (multimesh->buffer[multimesh->current_buffer] == 0 || multimesh->instances == 0) {
		return Vector<float>();
	}
	else if (multimesh->data_cache.size()) {
		ret = multimesh->data_cache;
	}
	else {
		// Buffer not cached, so fetch from GPU memory. This can be a stalling operation, avoid
		// whenever possible.

		Vector<uint8_t> buffer = Utilities::buffer_get_data(GL_ARRAY_BUFFER,
			multimesh->buffer[multimesh->current_buffer],
			multimesh->instances * multimesh->stride_cache * sizeof(float));
		ret.resize(multimesh->instances * multimesh->stride_cache);
		{
			float* w = ret.ptrw();
			const uint8_t* r = buffer.ptr();
			memcpy(w, r, buffer.size());
		}
	}
	if (multimesh->uses_colors || multimesh->uses_custom_data) {
		// Need to decompress buffer.
		uint32_t new_stride = multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
		new_stride += multimesh->uses_colors ? 4 : 0;
		new_stride += multimesh->uses_custom_data ? 4 : 0;

		Vector<float> decompressed;
		decompressed.resize(multimesh->instances * (int)new_stride);
		float* w = decompressed.ptrw();
		const float* r = ret.ptr();

		for (int i = 0; i < multimesh->instances; i++) {
			{
				float* newptr = w + i * new_stride;
				const float* oldptr = r + i * multimesh->stride_cache;
				float vals[8] = {oldptr[0], oldptr[1], oldptr[2], oldptr[3], oldptr[4], oldptr[5],
					oldptr[6], oldptr[7]};
				memcpy(newptr, vals, 8 * 4);
			}

			if (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_3D) {
				float* newptr = w + i * new_stride + 8;
				const float* oldptr = r + i * multimesh->stride_cache + 8;
				float vals[8] = {oldptr[0], oldptr[1], oldptr[2], oldptr[3]};
				memcpy(newptr, vals, 4 * 4);
			}

			if (multimesh->uses_colors) {
				float* newptr = w + i * new_stride +
								(multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12);
				const float* oldptr =
					r + i * multimesh->stride_cache + multimesh->color_offset_cache;
				uint16_t raw_data[4];
				memcpy(raw_data, oldptr, 2 * 4);
				newptr[0] = Math::half_to_float(raw_data[0]);
				newptr[1] = Math::half_to_float(raw_data[1]);
				newptr[2] = Math::half_to_float(raw_data[2]);
				newptr[3] = Math::half_to_float(raw_data[3]);
			}
			if (multimesh->uses_custom_data) {
				float* newptr = w + i * new_stride +
								(multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12) +
								(multimesh->uses_colors ? 4 : 0);
				const float* oldptr =
					r + i * multimesh->stride_cache + multimesh->custom_data_offset_cache;
				uint16_t raw_data[4];
				memcpy(raw_data, oldptr, 2 * 4);
				newptr[0] = Math::half_to_float(raw_data[0]);
				newptr[1] = Math::half_to_float(raw_data[1]);
				newptr[2] = Math::half_to_float(raw_data[2]);
				newptr[3] = Math::half_to_float(raw_data[3]);
			}
		}
		return decompressed;
	}
	else {
		return ret;
	}
}

void MeshStorage::_multimesh_set_visible_instances(RID p_multimesh, int p_visible)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_COND(p_visible < -1 || p_visible > multimesh->instances);
	if (multimesh->visible_instances == p_visible) {
		return;
	}

	if (multimesh->data_cache.size()) {
		// There is a data cache, but we may need to update some sections.
		_multimesh_mark_all_dirty(multimesh, false, true);
		int start =
			multimesh->visible_instances >= 0 ? multimesh->visible_instances : multimesh->instances;
		for (int i = start; i < p_visible; i++) {
			_multimesh_mark_dirty(multimesh, i, true);
		}
	}

	multimesh->visible_instances = p_visible;

	multimesh->dependency.changed_notify(
		Dependency::DEPENDENCY_CHANGED_MULTIMESH_VISIBLE_INSTANCES);
}

int MeshStorage::_multimesh_get_visible_instances(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, 0);
	return multimesh->visible_instances;
}

RendererMeshStorage::MultiMeshInterpolator* MeshStorage::_multimesh_get_interpolator(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V_MSG(multimesh, nullptr, "Multimesh not found: " + itos(p_multimesh.get_id()));

	return &multimesh->interpolator;
}

void MeshStorage::_update_dirty_multimeshes()
{
	while (multimesh_dirty_list) {
		MultiMesh* multimesh = multimesh_dirty_list;

		bool uses_motion_vectors = Renderer::viewport->get_num_viewports_with_motion_vectors() > 0;
		if (uses_motion_vectors) {
			multimesh->prev_buffer = multimesh->current_buffer;
			uint32_t new_buffer_index = multimesh->current_buffer ^ 1;

			// Generate secondary buffer if it doesn't exist.
			if (multimesh->buffer[new_buffer_index] == 0 && multimesh->instances) {
				GLuint new_buffer = 0;
				glGenBuffers(1, &new_buffer);
				glBindBuffer(GL_ARRAY_BUFFER, new_buffer);
				GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER, new_buffer,
					multimesh->instances * multimesh->stride_cache * sizeof(float), nullptr,
					GL_STATIC_DRAW, "MultiMesh secondary buffer");
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				multimesh->buffer[new_buffer_index] = new_buffer;
			}

			multimesh->current_buffer = new_buffer_index;
			multimesh->last_change = RendererCompositor::get_frame_number();
		}

		_update_dirty_multimesh(multimesh, uses_motion_vectors);

		multimesh_dirty_list = multimesh->dirty_list;

		multimesh->dirty_list = nullptr;
		multimesh->dirty = false;
	}

	multimesh_dirty_list = nullptr;
}

void MeshStorage::_update_dirty_multimesh(MultiMesh* p_multimesh, bool p_uses_motion_vectors)
{
	if (p_multimesh->data_cache.size()) { // May have been cleared, so only process if it exists.
		const float* data = p_multimesh->data_cache.ptr();

		uint32_t visible_instances = p_multimesh->visible_instances >= 0
										 ? p_multimesh->visible_instances
										 : p_multimesh->instances;

		if (p_multimesh->data_cache_used_dirty_regions) {
			uint32_t data_cache_dirty_region_count =
				Math::division_round_up(p_multimesh->instances, (int)MULTIMESH_DIRTY_REGION_SIZE);
			uint32_t visible_region_count = visible_instances == 0
												? 0
												: Math::division_round_up(visible_instances,
													  (uint32_t)MULTIMESH_DIRTY_REGION_SIZE);

			GLint region_size =
				p_multimesh->stride_cache * MULTIMESH_DIRTY_REGION_SIZE * sizeof(float);

			if (p_multimesh->data_cache_used_dirty_regions > 32 ||
				p_multimesh->data_cache_used_dirty_regions > visible_region_count / 2 ||
				p_uses_motion_vectors) {
				// If there are too many dirty regions, the dirty regions represent the majority of
				// visible regions, or motion vectors are used: Just copy all, else transfer cost
				// piles up too much.
				glBindBuffer(GL_ARRAY_BUFFER, p_multimesh->buffer[p_multimesh->current_buffer]);
				glBufferSubData(GL_ARRAY_BUFFER, 0,
					MIN(visible_region_count * region_size,
						p_multimesh->instances * p_multimesh->stride_cache * sizeof(float)),
					data);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else {
				// Not that many regions? Update them all.
				// TODO: profile the performance cost on low end
				glBindBuffer(GL_ARRAY_BUFFER, p_multimesh->buffer[p_multimesh->current_buffer]);
				for (uint32_t i = 0; i < visible_region_count; i++) {
					if (p_multimesh->data_cache_dirty_regions[i]) {
						GLint offset = i * region_size;
						GLint size = p_multimesh->stride_cache * (uint32_t)p_multimesh->instances *
									 (uint32_t)sizeof(float);
						uint32_t region_start_index =
							p_multimesh->stride_cache * MULTIMESH_DIRTY_REGION_SIZE * i;
						glBufferSubData(GL_ARRAY_BUFFER, offset, MIN(region_size, size - offset),
							&data[region_start_index]);
					}
				}
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			for (uint32_t i = 0; i < data_cache_dirty_region_count; i++) {
				p_multimesh->data_cache_dirty_regions[i] = false;
			}

			p_multimesh->data_cache_used_dirty_regions = 0;
		}

		if (p_multimesh->aabb_dirty && p_multimesh->mesh.is_valid()) {
			p_multimesh->aabb_dirty = false;
			if (p_multimesh->custom_aabb == AABB()) {
				_multimesh_re_create_aabb(p_multimesh, data, visible_instances);
				p_multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
			}
		}
	}
}

void GLES3::MeshStorage::multimesh_vertex_attrib_setup(GLuint p_instance_buffer, uint32_t p_stride,
	bool p_uses_format_2d, bool p_has_color_or_custom_data, int p_attrib_base_index)
{
	glBindBuffer(GL_ARRAY_BUFFER, p_instance_buffer);

	glEnableVertexAttribArray(p_attrib_base_index + 0);
	glVertexAttribPointer(p_attrib_base_index + 0, 4, GL_FLOAT, GL_FALSE, p_stride * sizeof(float),
		CAST_INT_TO_UCHAR_PTR(0));
	glVertexAttribDivisor(p_attrib_base_index + 0, 1);
	glEnableVertexAttribArray(p_attrib_base_index + 1);
	glVertexAttribPointer(p_attrib_base_index + 1, 4, GL_FLOAT, GL_FALSE, p_stride * sizeof(float),
		CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4));
	glVertexAttribDivisor(p_attrib_base_index + 1, 1);
	if (!p_uses_format_2d) {
		glEnableVertexAttribArray(p_attrib_base_index + 2);
		glVertexAttribPointer(p_attrib_base_index + 2, 4, GL_FLOAT, GL_FALSE,
			p_stride * sizeof(float), CAST_INT_TO_UCHAR_PTR(sizeof(float) * 8));
		glVertexAttribDivisor(p_attrib_base_index + 2, 1);
	}

	if (p_has_color_or_custom_data) {
		uint32_t color_custom_offset = p_uses_format_2d ? 8 : 12;
		glEnableVertexAttribArray(p_attrib_base_index + 3);
		glVertexAttribIPointer(p_attrib_base_index + 3, 4, GL_UNSIGNED_INT,
			p_stride * sizeof(float), CAST_INT_TO_UCHAR_PTR(color_custom_offset * sizeof(float)));
		glVertexAttribDivisor(p_attrib_base_index + 3, 1);
	}
	else {
		// Set all default instance color and custom data values to 1.0 or 0.0 using a compressed
		// format.
		uint16_t zero = Math::make_half_float(0.0f);
		uint16_t one = Math::make_half_float(1.0f);
		GLuint default_color = (uint32_t(one) << 16) | one;
		GLuint default_custom = (uint32_t(zero) << 16) | zero;
		glVertexAttribI4ui(
			p_attrib_base_index + 3, default_color, default_color, default_custom, default_custom);
	}
}

/* SKELETON API */

RID MeshStorage::skeleton_allocate() { return skeleton_owner.allocate_rid(); }

void MeshStorage::skeleton_initialize(RID p_rid)
{
	skeleton_owner.initialize_rid(p_rid, Skeleton());
}

void MeshStorage::skeleton_free(RID p_rid)
{
	_update_dirty_skeletons();
	skeleton_allocate_data(p_rid, 0);
	Skeleton* skeleton = skeleton_owner.get_or_null(p_rid);
	skeleton->dependency.deleted_notify(p_rid);
	skeleton_owner.free(p_rid);
}

void MeshStorage::_skeleton_make_dirty(Skeleton* skeleton)
{
	if (!skeleton->dirty) {
		skeleton->dirty = true;
		skeleton->dirty_list = skeleton_dirty_list;
		skeleton_dirty_list = skeleton;
	}
}

void MeshStorage::skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_COND(p_bones < 0);

	if (skeleton->size == p_bones && skeleton->use_2d == p_2d_skeleton) {
		return;
	}

	skeleton->size = p_bones;
	skeleton->use_2d = p_2d_skeleton;
	skeleton->height = (p_bones * (p_2d_skeleton ? 2 : 3)) / 256;
	if ((p_bones * (p_2d_skeleton ? 2 : 3)) % 256) {
		skeleton->height++;
	}

	if (skeleton->transforms_texture != 0) {
		GLES3::Utilities::texture_free_data(skeleton->transforms_texture);
		skeleton->transforms_texture = 0;
		skeleton->data.clear();
	}

	if (skeleton->size) {
		skeleton->data.resize(256 * skeleton->height * 4);
		glGenTextures(1, &skeleton->transforms_texture);
		glBindTexture(GL_TEXTURE_2D, skeleton->transforms_texture);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA32F, 256, skeleton->height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLES3::Utilities::texture_allocated_data(skeleton->transforms_texture,
			skeleton->data.size() * sizeof(float), "Skeleton transforms texture");

		memset(skeleton->data.ptr(), 0, skeleton->data.size() * sizeof(float));

		_skeleton_make_dirty(skeleton);
	}

	skeleton->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_SKELETON_DATA);
}

void MeshStorage::skeleton_set_base_transform_2d(
	RID p_skeleton, const Transform2D& p_base_transform)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_COND(!skeleton->use_2d);

	skeleton->base_transform_2d = p_base_transform;
}

int MeshStorage::skeleton_get_bone_count(RID p_skeleton)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL_V(skeleton, 0);

	return skeleton->size;
}

void MeshStorage::skeleton_bone_set_transform(
	RID p_skeleton, int p_bone, const Transform3D& p_transform)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_INDEX(p_bone, skeleton->size);
	ERR_FAIL_COND(skeleton->use_2d);

	float* dataptr = skeleton->data.ptr() + p_bone * 12;

	dataptr[0] = p_transform.basis.rows[0][0];
	dataptr[1] = p_transform.basis.rows[0][1];
	dataptr[2] = p_transform.basis.rows[0][2];
	dataptr[3] = p_transform.origin.x;
	dataptr[4] = p_transform.basis.rows[1][0];
	dataptr[5] = p_transform.basis.rows[1][1];
	dataptr[6] = p_transform.basis.rows[1][2];
	dataptr[7] = p_transform.origin.y;
	dataptr[8] = p_transform.basis.rows[2][0];
	dataptr[9] = p_transform.basis.rows[2][1];
	dataptr[10] = p_transform.basis.rows[2][2];
	dataptr[11] = p_transform.origin.z;

	_skeleton_make_dirty(skeleton);
}

Transform3D MeshStorage::skeleton_bone_get_transform(RID p_skeleton, int p_bone)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL_V(skeleton, Transform3D());
	ERR_FAIL_INDEX_V(p_bone, skeleton->size, Transform3D());
	ERR_FAIL_COND_V(skeleton->use_2d, Transform3D());

	const float* dataptr = skeleton->data.ptr() + p_bone * 12;

	Transform3D t;

	t.basis.rows[0][0] = dataptr[0];
	t.basis.rows[0][1] = dataptr[1];
	t.basis.rows[0][2] = dataptr[2];
	t.origin.x = dataptr[3];
	t.basis.rows[1][0] = dataptr[4];
	t.basis.rows[1][1] = dataptr[5];
	t.basis.rows[1][2] = dataptr[6];
	t.origin.y = dataptr[7];
	t.basis.rows[2][0] = dataptr[8];
	t.basis.rows[2][1] = dataptr[9];
	t.basis.rows[2][2] = dataptr[10];
	t.origin.z = dataptr[11];

	return t;
}

void MeshStorage::skeleton_bone_set_transform_2d(
	RID p_skeleton, int p_bone, const Transform2D& p_transform)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_INDEX(p_bone, skeleton->size);
	ERR_FAIL_COND(!skeleton->use_2d);

	float* dataptr = skeleton->data.ptr() + p_bone * 8;

	dataptr[0] = p_transform.columns[0][0];
	dataptr[1] = p_transform.columns[1][0];
	dataptr[2] = 0;
	dataptr[3] = p_transform.columns[2][0];
	dataptr[4] = p_transform.columns[0][1];
	dataptr[5] = p_transform.columns[1][1];
	dataptr[6] = 0;
	dataptr[7] = p_transform.columns[2][1];

	_skeleton_make_dirty(skeleton);
}

Transform2D MeshStorage::skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL_V(skeleton, Transform2D());
	ERR_FAIL_INDEX_V(p_bone, skeleton->size, Transform2D());
	ERR_FAIL_COND_V(!skeleton->use_2d, Transform2D());

	const float* dataptr = skeleton->data.ptr() + p_bone * 8;

	Transform2D t;
	t.columns[0][0] = dataptr[0];
	t.columns[1][0] = dataptr[1];
	t.columns[2][0] = dataptr[3];
	t.columns[0][1] = dataptr[4];
	t.columns[1][1] = dataptr[5];
	t.columns[2][1] = dataptr[7];

	return t;
}

void MeshStorage::_update_dirty_skeletons()
{
	while (skeleton_dirty_list) {
		Skeleton* skeleton = skeleton_dirty_list;

		if (skeleton->size) {
			glBindTexture(GL_TEXTURE_2D, skeleton->transforms_texture);
			glTexImage2D
(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, skeleton->height, 0, GL_RGBA, GL_FLOAT,
				skeleton->data.ptr());
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		skeleton_dirty_list = skeleton->dirty_list;

		skeleton->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_SKELETON_BONES);

		skeleton->version++;

		skeleton->dirty = false;
		skeleton->dirty_list = nullptr;
	}

	skeleton_dirty_list = nullptr;
}

void MeshStorage::skeleton_update_dependency(RID p_skeleton, DependencyTracker* p_instance)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL(skeleton);

	p_instance->update_dependency(&skeleton->dependency);
}

#endif // GLES3_ENABLED



// ============================================================================
// Merged from drivers/gles3/storage/particles_storage.cpp
// ============================================================================

#include "particles_storage.h"
#include "servers/rendering/renderer_compositor.h"

#ifdef GLES3_ENABLED

#include "drivers/gles3/storage/material_storage.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer.h"

using namespace GLES3;


void ParticlesStorage::initialize()
{
	{
		String global_defines;
		global_defines +=
			"#define MAX_GLOBAL_SHADER_UNIFORMS 256\n"; // TODO: this is arbitrary for now
		MaterialStorage::shaders->particles_process_shader.initialize(global_defines, 1);
	}
	{
		// default material and shader for particles shader
		particles_shader.default_shader = MaterialStorage::shader_allocate();
		MaterialStorage::shader_initialize(particles_shader.default_shader);
		MaterialStorage::shader_set_code(particles_shader.default_shader, R"(
// Default particles shader.

shader_type particles;

void process() {
	COLOR = vec4(1.0);
}
)");
		particles_shader.default_material = MaterialStorage::material_allocate();
		MaterialStorage::material_initialize(particles_shader.default_material);
		MaterialStorage::material_set_shader(
			particles_shader.default_material, particles_shader.default_shader);
	}
	{
		particles_shader.copy_shader.initialize();
		particles_shader.copy_shader_version = particles_shader.copy_shader.version_create();
	}
}

void ParticlesStorage::finalize()
{
	MaterialStorage::material_free(particles_shader.default_material);
	MaterialStorage::shader_free(particles_shader.default_shader);
	particles_shader.copy_shader.version_free(particles_shader.copy_shader_version);
}

/* PARTICLES */

RID ParticlesStorage::particles_allocate() { return particles_owner.allocate_rid(); }

void ParticlesStorage::particles_initialize(RID p_rid) { particles_owner.initialize_rid(p_rid); }

void ParticlesStorage::particles_free(RID p_rid)
{
	Particles* particles = particles_owner.get_or_null(p_rid);

	particles->dependency.deleted_notify(p_rid);
	particles->update_list.remove_from_list();

	_particles_free_data(particles);
	particles_owner.free(p_rid);
}

void ParticlesStorage::particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	if (particles->mode == p_mode) {
		return;
	}

	_particles_free_data(particles);

	particles->mode = p_mode;
}

void ParticlesStorage::particles_set_emitting(RID p_particles, bool p_emitting)
{
	ERR_FAIL_COND_MSG(GLES3::Config::disable_particles_workaround,
		"Due to driver bugs, GPUParticles are not supported on Adreno 3XX devices. Please use "
		"CPUParticles instead.");

	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emitting = p_emitting;
}

bool ParticlesStorage::particles_get_emitting(RID p_particles)
{
	if (GLES3::Config::disable_particles_workaround) {
		return false;
	}

	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, false);

	return particles->emitting;
}

void ParticlesStorage::_particles_free_data(Particles* particles)
{
	particles->userdata_count = 0;
	particles->instance_buffer_size_cache = 0;
	particles->instance_buffer_stride_cache = 0;
	particles->num_attrib_arrays_cache = 0;
	particles->process_buffer_stride_cache = 0;

	if (particles->front_process_buffer != 0) {
		glDeleteVertexArrays(1, &particles->front_vertex_array);
		GLES3::Utilities::buffer_free_data(particles->front_process_buffer);
		GLES3::Utilities::buffer_free_data(particles->front_instance_buffer);
		particles->front_vertex_array = 0;
		particles->front_process_buffer = 0;
		particles->front_instance_buffer = 0;

		glDeleteVertexArrays(1, &particles->back_vertex_array);
		GLES3::Utilities::buffer_free_data(particles->back_process_buffer);
		GLES3::Utilities::buffer_free_data(particles->back_instance_buffer);
		particles->back_vertex_array = 0;
		particles->back_process_buffer = 0;
		particles->back_instance_buffer = 0;
	}

	if (particles->sort_buffer != 0) {
		GLES3::Utilities::buffer_free_data(particles->last_frame_buffer);
		GLES3::Utilities::buffer_free_data(particles->sort_buffer);
		particles->last_frame_buffer = 0;
		particles->sort_buffer = 0;
		particles->sort_buffer_filled = false;
		particles->last_frame_buffer_filled = false;
	}

	if (particles->frame_params_ubo != 0) {
		GLES3::Utilities::buffer_free_data(particles->frame_params_ubo);
		particles->frame_params_ubo = 0;
	}
}

void ParticlesStorage::particles_set_amount(RID p_particles, int p_amount)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	if (particles->amount == p_amount) {
		return;
	}

	_particles_free_data(particles);

	particles->amount = p_amount;

	particles->prev_ticks = 0;
	particles->phase = 0;
	particles->prev_phase = 0;
	particles->clear = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_amount_ratio(RID p_particles, float p_amount_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->amount_ratio = p_amount_ratio;
}

void ParticlesStorage::particles_set_lifetime(RID p_particles, double p_lifetime)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->lifetime = p_lifetime;
}

void ParticlesStorage::particles_set_one_shot(RID p_particles, bool p_one_shot)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->one_shot = p_one_shot;
}

void ParticlesStorage::particles_set_pre_process_time(RID p_particles, double p_time)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->pre_process_time = p_time;
}

void ParticlesStorage::particles_request_process_time(
	RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->request_process_time = p_request_process_time;
	particles->request_process_time_residual = p_request_process_time_residual;
}

void ParticlesStorage::particles_set_seed(RID p_particles, uint32_t p_seed)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->random_seed = p_seed;
}

void ParticlesStorage::particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->explosiveness = p_ratio;
}

void ParticlesStorage::particles_set_randomness_ratio(RID p_particles, real_t p_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->randomness = p_ratio;
}

void ParticlesStorage::particles_set_custom_aabb(RID p_particles, const AABB& p_aabb)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->custom_aabb = p_aabb;
	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_set_speed_scale(RID p_particles, double p_scale)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->speed_scale = p_scale;
}

void ParticlesStorage::particles_set_use_local_coordinates(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->use_local_coords = p_enable;
	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_fixed_fps(RID p_particles, int p_fps)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->fixed_fps = p_fps;

	_particles_free_data(particles);

	particles->prev_ticks = 0;
	particles->phase = 0;
	particles->prev_phase = 0;
	particles->clear = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_interpolate(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->interpolate = p_enable;
}

void ParticlesStorage::particles_set_fractional_delta(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->fractional_delta = p_enable;
}

void ParticlesStorage::particles_set_trails(RID p_particles, bool p_enable, double p_length)
{
	if (p_enable) {
		WARN_PRINT_ONCE_ED("The Compatibility renderer does not support particle trails.");
	}
}

void ParticlesStorage::particles_set_trail_bind_poses(
	RID p_particles, const Vector<Transform3D>& p_bind_poses)
{
	if (p_bind_poses.size() != 0) {
		WARN_PRINT_ONCE_ED("The Compatibility renderer does not support particle trails.");
	}
}

void ParticlesStorage::particles_set_collision_base_size(RID p_particles, real_t p_size)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->collision_base_size = p_size;
}

void ParticlesStorage::particles_set_transform_align(
	RID p_particles, RSE::ParticlesTransformAlign p_transform_align)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align = p_transform_align;
}

void ParticlesStorage::particles_set_transform_align_channel_filter(
	RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel_filter)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align_channel_filter = p_channel_filter;
}

void ParticlesStorage::particles_set_transform_align_axis(
	RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align_axis = p_rotation_axis;
}

void ParticlesStorage::particles_set_process_material(RID p_particles, RID p_material)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->process_material = p_material;
	particles->dependency.changed_notify(
		Dependency::DEPENDENCY_CHANGED_PARTICLES); // the instance buffer may have changed
}

RID ParticlesStorage::particles_get_process_material(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, RID());

	return particles->process_material;
}

void ParticlesStorage::particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->draw_order = p_order;
}

void ParticlesStorage::particles_set_draw_passes(RID p_particles, int p_passes)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->draw_passes.resize(p_passes);
}

void ParticlesStorage::particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	ERR_FAIL_INDEX(p_pass, particles->draw_passes.size());
	particles->draw_passes.write[p_pass] = p_mesh;
	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_restart(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->restart_request = true;
}

void ParticlesStorage::particles_set_subemitter(RID p_particles, RID p_subemitter_particles)
{
	if (p_subemitter_particles.is_valid()) {
		WARN_PRINT_ONCE_ED("The Compatibility renderer does not support particle sub-emitters.");
	}
}

void ParticlesStorage::particles_emit(RID p_particles, const Transform3D& p_transform,
	const Vector3& p_velocity, const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	WARN_PRINT_ONCE_ED("The Compatibility renderer does not support manually emitting particles.");
}

void ParticlesStorage::particles_request_process(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	if (!particles->dirty) {
		particles->dirty = true;

		if (!particles->update_list.in_list()) {
			particle_update_list.add(&particles->update_list);
		}
	}
}

AABB ParticlesStorage::particles_get_current_aabb(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, AABB());

	int total_amount = particles->amount;

	// If available, read from the sort buffer which should be 2 frames out of date.
	// This will help alleviate GPU stalls.
	GLuint read_buffer =
		particles->sort_buffer_filled ? particles->sort_buffer : particles->back_instance_buffer;

	Vector<uint8_t> buffer = Utilities::buffer_get_data(
		GL_ARRAY_BUFFER, read_buffer, total_amount * sizeof(ParticleInstanceData3D));
	ERR_FAIL_COND_V(buffer.size() != (int)(total_amount * sizeof(ParticleInstanceData3D)), AABB());

	Transform3D inv = particles->emission_transform.affine_inverse();

	AABB aabb;
	if (buffer.size()) {
		bool first = true;

		const uint8_t* data_ptr = (const uint8_t*)buffer.ptr();
		uint32_t particle_data_size = sizeof(ParticleInstanceData3D);

		for (int i = 0; i < total_amount; i++) {
			const ParticleInstanceData3D& particle_data =
				*(const ParticleInstanceData3D*)&data_ptr[particle_data_size * i];
			// If scale is 0.0, we assume the particle is inactive.
			if (particle_data.xform[0] > 0.0) {
				Vector3 pos = Vector3(
					particle_data.xform[3], particle_data.xform[7], particle_data.xform[11]);
				if (!particles->use_local_coords) {
					pos = inv.xform(pos);
				}
				if (first) {
					aabb.position = pos;
					first = false;
				}
				else {
					aabb.expand_to(pos);
				}
			}
		}
	}

	float longest_axis_size = 0;
	for (int i = 0; i < particles->draw_passes.size(); i++) {
		if (particles->draw_passes[i].is_valid()) {
			AABB maabb =
				MeshStorage::mesh_get_aabb(particles->draw_passes[i], RID());
			longest_axis_size = MAX(maabb.get_longest_axis_size(), longest_axis_size);
		}
	}

	aabb.grow_by(longest_axis_size);

	return aabb;
}

AABB ParticlesStorage::particles_get_aabb(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, AABB());

	return particles->custom_aabb;
}

void ParticlesStorage::particles_set_emission_transform(
	RID p_particles, const Transform3D& p_transform)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emission_transform = p_transform;
}

void ParticlesStorage::particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emitter_velocity = p_velocity;
}

void ParticlesStorage::particles_set_interp_to_end(RID p_particles, float p_interp)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->interp_to_end = p_interp;
}

int ParticlesStorage::particles_get_draw_passes(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, 0);

	return particles->draw_passes.size();
}

RID ParticlesStorage::particles_get_draw_pass_mesh(RID p_particles, int p_pass)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, RID());
	ERR_FAIL_INDEX_V(p_pass, particles->draw_passes.size(), RID());

	return particles->draw_passes[p_pass];
}

void ParticlesStorage::particles_add_collision(RID p_particles, RID p_particles_collision_instance)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->collisions.insert(p_particles_collision_instance);
}

void ParticlesStorage::particles_remove_collision(
	RID p_particles, RID p_particles_collision_instance)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->collisions.erase(p_particles_collision_instance);
}

void ParticlesStorage::particles_set_canvas_sdf_collision(RID p_particles, bool p_enable,
	const Transform2D& p_xform, const Rect2& p_to_screen, GLuint p_texture)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->has_sdf_collision = p_enable;
	particles->sdf_collision_transform = p_xform;
	particles->sdf_collision_to_screen = p_to_screen;
	particles->sdf_collision_texture = p_texture;
}

// Does one step of processing particles by reading from back_process_buffer and writing to
// front_process_buffer.
void ParticlesStorage::_particles_process(Particles* p_particles, double p_delta)
{
	double new_phase = Math::fmod(p_particles->phase + (p_delta / p_particles->lifetime), 1.0);

	// update current frame
	ParticlesFrameParams frame_params;

	if (p_particles->clear) {
		p_particles->cycle_number = 0;
	}
	else if (new_phase < p_particles->phase) {
		if (p_particles->one_shot) {
			p_particles->emitting = false;
		}
		p_particles->cycle_number++;
	}

	frame_params.emitting = p_particles->emitting;
	frame_params.system_phase = new_phase;
	frame_params.prev_system_phase = p_particles->phase;

	p_particles->phase = new_phase;

	frame_params.time = RendererCompositor::get_total_time();
	frame_params.delta = p_delta;
	frame_params.random_seed = p_particles->random_seed;
	frame_params.explosiveness = p_particles->explosiveness;
	frame_params.randomness = p_particles->randomness;

	if (p_particles->use_local_coords) {
		GLES3::MaterialStorage::store_transform(Transform3D(), frame_params.emission_transform);
	}
	else {
		GLES3::MaterialStorage::store_transform(
			p_particles->emission_transform, frame_params.emission_transform);
	}

	frame_params.cycle = p_particles->cycle_number;
	frame_params.frame = p_particles->frame_counter++;
	frame_params.amount_ratio = p_particles->amount_ratio;
	frame_params.pad1 = 0;
	frame_params.pad2 = 0;
	frame_params.interp_to_end = p_particles->interp_to_end;
	frame_params.emitter_velocity[0] = p_particles->emitter_velocity.x;
	frame_params.emitter_velocity[1] = p_particles->emitter_velocity.y;
	frame_params.emitter_velocity[2] = p_particles->emitter_velocity.z;

	{ // collision and attractors

		frame_params.collider_count = 0;
		frame_params.attractor_count = 0;
		frame_params.particle_size = p_particles->collision_base_size;

		GLuint collision_heightmap_texture = 0;

		Transform3D to_particles;
		if (p_particles->use_local_coords) {
			to_particles = p_particles->emission_transform.affine_inverse();
		}

		if (p_particles->has_sdf_collision && p_particles->sdf_collision_texture != 0) {
			// 2D collision

			Transform2D xform =
				p_particles
					->sdf_collision_transform; // will use dotproduct manually so invert beforehand

			if (!p_particles->use_local_coords) {
				Transform2D emission;
				emission.columns[0] = Vector2(p_particles->emission_transform.basis.get_column(0).x,
					p_particles->emission_transform.basis.get_column(0).y);
				emission.columns[1] = Vector2(p_particles->emission_transform.basis.get_column(1).x,
					p_particles->emission_transform.basis.get_column(1).y);
				emission.set_origin(Vector2(p_particles->emission_transform.origin.x,
					p_particles->emission_transform.origin.y));
				xform = xform * emission.affine_inverse();
			}

			Transform2D revert = xform.affine_inverse();
			frame_params.collider_count = 1;
			frame_params.colliders[0].transform[0] = xform.columns[0][0];
			frame_params.colliders[0].transform[1] = xform.columns[0][1];
			frame_params.colliders[0].transform[2] = 0;
			frame_params.colliders[0].transform[3] = xform.columns[2][0];

			frame_params.colliders[0].transform[4] = xform.columns[1][0];
			frame_params.colliders[0].transform[5] = xform.columns[1][1];
			frame_params.colliders[0].transform[6] = 0;
			frame_params.colliders[0].transform[7] = xform.columns[2][1];

			frame_params.colliders[0].transform[8] = revert.columns[0][0];
			frame_params.colliders[0].transform[9] = revert.columns[0][1];
			frame_params.colliders[0].transform[10] = 0;
			frame_params.colliders[0].transform[11] = revert.columns[2][0];

			frame_params.colliders[0].transform[12] = revert.columns[1][0];
			frame_params.colliders[0].transform[13] = revert.columns[1][1];
			frame_params.colliders[0].transform[14] = 0;
			frame_params.colliders[0].transform[15] = revert.columns[2][1];

			frame_params.colliders[0].extents[0] = p_particles->sdf_collision_to_screen.size.x;
			frame_params.colliders[0].extents[1] = p_particles->sdf_collision_to_screen.size.y;
			frame_params.colliders[0].extents[2] = p_particles->sdf_collision_to_screen.position.x;
			frame_params.colliders[0].scale = p_particles->sdf_collision_to_screen.position.y;
			frame_params.colliders[0].type = ParticlesFrameParams::COLLISION_TYPE_2D_SDF;

			collision_heightmap_texture = p_particles->sdf_collision_texture;
		}

		for (const RID& E : p_particles->collisions) {
			ParticlesCollisionInstance* pci = particles_collision_instance_owner.get_or_null(E);
			if (!pci || !pci->active) {
				continue;
			}
			ParticlesCollision* pc = particles_collision_owner.get_or_null(pci->collision);
			ERR_CONTINUE(!pc);

			Transform3D to_collider = pci->transform;
			if (p_particles->use_local_coords) {
				to_collider = to_particles * to_collider;
			}
			Vector3 scale = to_collider.basis.get_scale();
			to_collider.basis.orthonormalize();

			if (pc->type <= RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT) {
				// attractor
				if (frame_params.attractor_count >= ParticlesFrameParams::MAX_ATTRACTORS) {
					continue;
				}

				ParticlesFrameParams::Attractor& attr =
					frame_params.attractors[frame_params.attractor_count];

				GLES3::MaterialStorage::store_transform(to_collider, attr.transform);
				attr.strength = pc->attractor_strength;
				attr.attenuation = pc->attractor_attenuation;
				attr.directionality = pc->attractor_directionality;

				switch (pc->type) {
				case RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT: {
					attr.type = ParticlesFrameParams::ATTRACTOR_TYPE_SPHERE;
					float radius = pc->radius;
					radius *= (scale.x + scale.y + scale.z) / 3.0;
					attr.extents[0] = radius;
					attr.extents[1] = radius;
					attr.extents[2] = radius;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT: {
					attr.type = ParticlesFrameParams::ATTRACTOR_TYPE_BOX;
					Vector3 extents = pc->extents * scale;
					attr.extents[0] = extents.x;
					attr.extents[1] = extents.y;
					attr.extents[2] = extents.z;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT: {
					WARN_PRINT_ONCE_ED("Vector field particle attractors are not available in the "
									   "Compatibility renderer.");
				} break;
				default: {
				}
				}

				frame_params.attractor_count++;
			}
			else {
				// collider
				if (frame_params.collider_count >= ParticlesFrameParams::MAX_COLLIDERS) {
					continue;
				}

				ParticlesFrameParams::Collider& col =
					frame_params.colliders[frame_params.collider_count];

				GLES3::MaterialStorage::store_transform(to_collider, col.transform);
				switch (pc->type) {
				case RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE: {
					col.type = ParticlesFrameParams::COLLISION_TYPE_SPHERE;
					float radius = pc->radius;
					radius *= (scale.x + scale.y + scale.z) / 3.0;
					col.extents[0] = radius;
					col.extents[1] = radius;
					col.extents[2] = radius;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE: {
					col.type = ParticlesFrameParams::COLLISION_TYPE_BOX;
					Vector3 extents = pc->extents * scale;
					col.extents[0] = extents.x;
					col.extents[1] = extents.y;
					col.extents[2] = extents.z;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE: {
					WARN_PRINT_ONCE_ED(
						"SDF Particle Colliders are not available in the Compatibility renderer.");
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE: {
					if (collision_heightmap_texture != 0) { // already taken
						continue;
					}

					col.type = ParticlesFrameParams::COLLISION_TYPE_HEIGHT_FIELD;
					Vector3 extents = pc->extents * scale;
					col.extents[0] = extents.x;
					col.extents[1] = extents.y;
					col.extents[2] = extents.z;
					collision_heightmap_texture = pc->heightfield_texture;
				} break;
				default: {
				}
				}

				frame_params.collider_count++;
			}
		}

		// Bind heightmap or SDF texture.
		GLuint heightmap = collision_heightmap_texture;
		if (heightmap == 0) {
			GLES3::Texture* tex = GLES3::TextureStorage::get_texture(
				GLES3::TextureStorage::texture_gl_get_default(GLES3::DEFAULT_GL_TEXTURE_BLACK));
			heightmap = tex->tex_id;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, heightmap);
	}

	if (p_particles->frame_params_ubo == 0) {
		glGenBuffers(1, &p_particles->frame_params_ubo);
		glBindBufferBase(
			GL_UNIFORM_BUFFER, PARTICLES_FRAME_UNIFORM_LOCATION, p_particles->frame_params_ubo);
		GLES3::Utilities::buffer_allocate_data(GL_UNIFORM_BUFFER,
			p_particles->frame_params_ubo, sizeof(ParticlesFrameParams), &frame_params,
			GL_STREAM_DRAW, "Particle Frame UBO");
	}
	else {
		// Update per-frame UBO.
		glBindBufferBase(
			GL_UNIFORM_BUFFER, PARTICLES_FRAME_UNIFORM_LOCATION, p_particles->frame_params_ubo);
		glBufferData(
			GL_UNIFORM_BUFFER, sizeof(ParticlesFrameParams), &frame_params, GL_STREAM_DRAW);
	}

	// Get shader and set shader uniforms;
	MaterialData* m =
		MaterialStorage::material_get_data(p_particles->process_material, RSE::SHADER_PARTICLES);
	if (!m) {
		m = MaterialStorage::material_get_data(
			particles_shader.default_material, RSE::SHADER_PARTICLES);
	}

	ERR_FAIL_NULL(m);

	ParticlesShaderGLES3::ShaderVariant variant = ParticlesShaderGLES3::MODE_DEFAULT;

	uint32_t specialization = 0;
	for (uint32_t i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
		if (m->shader_data->userdatas_used[i]) {
			specialization |= ParticlesShaderGLES3::USERDATA1_USED << i;
		}
	}

	if (p_particles->mode == RSE::ParticlesMode::PARTICLES_MODE_3D) {
		specialization |= ParticlesShaderGLES3::MODE_3D;
	}

	RID version = particles_shader.default_shader_version;
	if (m->shader_data->version.is_valid() && m->shader_data->valid) {
		// Bind material uniform buffer and textures.
		m->bind_uniforms();
		version = m->shader_data->version;
	}

	bool success = MaterialStorage::shaders->particles_process_shader.version_bind_shader(
		version, variant, specialization);
	if (!success) {
		return;
	}

	MaterialStorage::shaders->particles_process_shader.version_set_uniform(
		ParticlesShaderGLES3::LIFETIME, p_particles->lifetime, version, variant, specialization);
	MaterialStorage::shaders->particles_process_shader.version_set_uniform(
		ParticlesShaderGLES3::CLEAR, p_particles->clear, version, variant, specialization);
	MaterialStorage::shaders->particles_process_shader.version_set_uniform(
		ParticlesShaderGLES3::TOTAL_PARTICLES, uint32_t(p_particles->amount), version, variant,
		specialization);
	MaterialStorage::shaders->particles_process_shader.version_set_uniform(
		ParticlesShaderGLES3::USE_FRACTIONAL_DELTA, p_particles->fractional_delta, version, variant,
		specialization);

	p_particles->clear = false;

	p_particles->has_collision_cache = m->shader_data->uses_collision;

	glBindVertexArray(p_particles->back_vertex_array);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, p_particles->front_process_buffer);

	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, p_particles->amount);
	glEndTransformFeedback();

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	glBindVertexArray(0);

	SWAP(p_particles->front_process_buffer, p_particles->back_process_buffer);
	SWAP(p_particles->front_vertex_array, p_particles->back_vertex_array);
}

void ParticlesStorage::particles_set_view_axis(
	RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	if (particles->draw_order != RSE::PARTICLES_DRAW_ORDER_VIEW_DEPTH &&
		particles->transform_align != RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD &&
		particles->transform_align != RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY &&
		particles->transform_align != RSE::PARTICLES_TRANSFORM_ALIGN_LOCAL_BILLBOARD) {
		return;
	}

	if (particles->front_process_buffer == 0) {
		return; // particles have not processed yet
	}

	Vector3 axis = -p_axis; // cameras look to z negative

	if (particles->use_local_coords) {
		axis = particles->emission_transform.basis.xform_inv(axis).normalized();
	}

	// Sort will be done on CPU since we don't have compute shaders.
	// If the sort_buffer has valid data
	// Use a buffer that is 2 frames out of date to avoid stalls.
	if (particles->draw_order == RSE::PARTICLES_DRAW_ORDER_VIEW_DEPTH &&
		particles->sort_buffer_filled) {
		glBindBuffer(GL_ARRAY_BUFFER, particles->sort_buffer);

		ParticleInstanceData3D* particle_array;
#ifndef __EMSCRIPTEN__
		particle_array = static_cast<ParticleInstanceData3D*>(
			glMapBufferRange(GL_ARRAY_BUFFER, 0, particles->amount * sizeof(ParticleInstanceData3D),
				GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
		ERR_FAIL_NULL(particle_array);
#else
		LocalVector<ParticleInstanceData3D> particle_vector;
		particle_vector.resize(particles->amount);
		particle_array = particle_vector.ptr();
		godot_webgl2_glGetBufferSubData(
			GL_ARRAY_BUFFER, 0, particles->amount * sizeof(ParticleInstanceData3D), particle_array);
#endif
		SortArray<ParticleInstanceData3D, ParticlesViewSort> sorter;
		sorter.compare.z_dir = axis;
		sorter.sort(particle_array, particles->amount);

#ifndef __EMSCRIPTEN__
		glUnmapBuffer(GL_ARRAY_BUFFER);
#else
		glBufferSubData(GL_ARRAY_BUFFER, 0, particles->amount * sizeof(ParticleInstanceData3D),
			particle_vector.ptr());
#endif
	}

	glEnable(GL_RASTERIZER_DISCARD);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	_particles_update_instance_buffer(particles, axis, p_up_axis);
	glDisable(GL_RASTERIZER_DISCARD);
}

void ParticlesStorage::_particles_update_buffers(Particles* particles)
{
	uint32_t userdata_count = 0;

	if (particles->process_material.is_valid()) {
		GLES3::MaterialData* material_data = MaterialStorage::material_get_data(
			particles->process_material, RSE::SHADER_PARTICLES);
		if (material_data && material_data->shader_data->version.is_valid() &&
			material_data->shader_data->valid) {
			userdata_count = material_data->shader_data->userdata_count;
		}
	}

	if (userdata_count != particles->userdata_count) {
		// Mismatch userdata, re-create buffers.
		_particles_free_data(particles);
		particles->restart_request = true;
	}

	if (particles->amount > 0 && particles->front_process_buffer == 0) {
		int total_amount = particles->amount;

		particles->userdata_count = userdata_count;

		uint32_t xform_size = particles->mode == RSE::PARTICLES_MODE_2D ? 2 : 3;
		particles->instance_buffer_stride_cache = sizeof(float) * 4 * (xform_size + 1);
		particles->instance_buffer_size_cache =
			particles->instance_buffer_stride_cache * total_amount;
		particles->num_attrib_arrays_cache = 5 + userdata_count + (xform_size - 2);
		particles->process_buffer_stride_cache =
			sizeof(float) * 4 * particles->num_attrib_arrays_cache;

		PackedByteArray data;
		data.resize_initialized(particles->process_buffer_stride_cache * total_amount);

		PackedByteArray instance_data;
		instance_data.resize_initialized(particles->instance_buffer_size_cache);

		{
			glGenVertexArrays(1, &particles->front_vertex_array);
			glBindVertexArray(particles->front_vertex_array);
			glGenBuffers(1, &particles->front_process_buffer);
			glGenBuffers(1, &particles->front_instance_buffer);

			glBindBuffer(GL_ARRAY_BUFFER, particles->front_process_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
				particles->front_process_buffer,
				particles->process_buffer_stride_cache * total_amount, data.ptr(), GL_DYNAMIC_COPY,
				"Particles front process buffer");

			for (uint32_t j = 0; j < particles->num_attrib_arrays_cache; j++) {
				glEnableVertexAttribArray(j);
				glVertexAttribPointer(j, 4, GL_FLOAT, GL_FALSE,
					particles->process_buffer_stride_cache,
					CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * j));
			}
			glBindVertexArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, particles->front_instance_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
				particles->front_instance_buffer, particles->instance_buffer_size_cache,
				instance_data.ptr(), GL_DYNAMIC_COPY, "Particles front instance buffer");
		}

		{
			glGenVertexArrays(1, &particles->back_vertex_array);
			glBindVertexArray(particles->back_vertex_array);
			glGenBuffers(1, &particles->back_process_buffer);
			glGenBuffers(1, &particles->back_instance_buffer);

			glBindBuffer(GL_ARRAY_BUFFER, particles->back_process_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
				particles->back_process_buffer,
				particles->process_buffer_stride_cache * total_amount, data.ptr(), GL_DYNAMIC_COPY,
				"Particles back process buffer");

			for (uint32_t j = 0; j < particles->num_attrib_arrays_cache; j++) {
				glEnableVertexAttribArray(j);
				glVertexAttribPointer(j, 4, GL_FLOAT, GL_FALSE,
					particles->process_buffer_stride_cache,
					CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * j));
			}
			glBindVertexArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, particles->back_instance_buffer);
			GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
				particles->back_instance_buffer, particles->instance_buffer_size_cache,
				instance_data.ptr(), GL_DYNAMIC_COPY, "Particles back instance buffer");
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void ParticlesStorage::_particles_allocate_history_buffers(Particles* particles)
{
	if (particles->sort_buffer == 0) {
		glGenBuffers(1, &particles->last_frame_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particles->last_frame_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
			particles->last_frame_buffer, particles->instance_buffer_size_cache, nullptr,
			GL_DYNAMIC_READ, "Particles last frame buffer");

		glGenBuffers(1, &particles->sort_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particles->sort_buffer);
		GLES3::Utilities::buffer_allocate_data(GL_ARRAY_BUFFER,
			particles->sort_buffer, particles->instance_buffer_size_cache, nullptr, GL_DYNAMIC_READ,
			"Particles sort buffer");

		particles->sort_buffer_filled = false;
		particles->last_frame_buffer_filled = false;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void ParticlesStorage::_particles_update_instance_buffer(
	Particles* particles, const Vector3& p_axis, const Vector3& p_up_axis)
{
	ParticlesCopyShaderGLES3::ShaderVariant variant = ParticlesCopyShaderGLES3::MODE_DEFAULT;

	uint64_t specialization = 0;
	if (particles->mode == RSE::ParticlesMode::PARTICLES_MODE_3D) {
		specialization |= ParticlesCopyShaderGLES3::MODE_3D;
	}

	bool success = particles_shader.copy_shader.version_bind_shader(
		particles_shader.copy_shader_version, variant, specialization);
	if (!success) {
		return;
	}

	// Affect 2D only.
	if (particles->use_local_coords) {
		// In local mode, particle positions are calculated locally (relative to the node position)
		// and they're also drawn locally.
		// It works as expected, so we just pass an identity transform.
		particles_shader.copy_shader.version_set_uniform(
			ParticlesCopyShaderGLES3::INV_EMISSION_TRANSFORM, Transform3D(),
			particles_shader.copy_shader_version, variant, specialization);
	}
	else {
		// In global mode, particle positions are calculated globally (relative to the canvas
		// origin) but they're drawn locally. So, we need to pass the inverse of the emission
		// transform to bring the particles to local coordinates before drawing.
		Transform3D inv = particles->emission_transform.affine_inverse();
		particles_shader.copy_shader.version_set_uniform(
			ParticlesCopyShaderGLES3::INV_EMISSION_TRANSFORM, inv,
			particles_shader.copy_shader_version, variant, specialization);
	}

	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::FRAME_REMAINDER,
		particles->interpolate ? particles->frame_remainder : 0.0,
		particles_shader.copy_shader_version, variant, specialization);
	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::ALIGN_MODE,
		uint32_t(particles->transform_align), particles_shader.copy_shader_version, variant,
		specialization);
	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::ALIGN_UP, p_up_axis,
		particles_shader.copy_shader_version, variant, specialization);
	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::SORT_DIRECTION,
		p_axis, particles_shader.copy_shader_version, variant, specialization);
	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::ALIGN_AXIS,
		uint32_t(particles->transform_align_axis), particles_shader.copy_shader_version, variant,
		specialization);
	particles_shader.copy_shader.version_set_uniform(ParticlesCopyShaderGLES3::ALIGN_CHANNEL_FILTER,
		uint32_t(particles->transform_align_channel_filter), particles_shader.copy_shader_version,
		variant, specialization);

	glBindVertexArray(particles->back_vertex_array);
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particles->front_instance_buffer, 0,
		particles->instance_buffer_size_cache);
	glBeginTransformFeedback(GL_POINTS);

	if (particles->draw_order == RSE::PARTICLES_DRAW_ORDER_LIFETIME) {
		uint32_t lifetime_split =
			(MIN(int(particles->amount * particles->phase), particles->amount - 1) + 1) %
			particles->amount;
		uint32_t stride = particles->process_buffer_stride_cache;

		glBindBuffer(GL_ARRAY_BUFFER, particles->back_process_buffer);

		// Offset VBO so you render starting at the newest particle.
		if (particles->amount - lifetime_split > 0) {
			glEnableVertexAttribArray(0); // Color.
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride,
				CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 0));
			glEnableVertexAttribArray(1); // .xyz: velocity. .z: flags.
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
				CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 1));
			glEnableVertexAttribArray(2); // Custom.
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
				CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 2));
			glEnableVertexAttribArray(3); // Xform1.
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
				CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 3));
			glEnableVertexAttribArray(4); // Xform2.
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride,
				CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 4));
			if (particles->mode == RSE::PARTICLES_MODE_3D) {
				glEnableVertexAttribArray(5); // Xform3.
				glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride,
					CAST_INT_TO_UCHAR_PTR(stride * lifetime_split + sizeof(float) * 4 * 5));
			}

			uint32_t to_draw = particles->amount - lifetime_split;
			glDrawArrays(GL_POINTS, 0, to_draw);
		}

		// Then render from index 0 up intil the newest particle.
		if (lifetime_split > 0) {
			glEndTransformFeedback();
			// Now output to the second portion of the instance buffer.
			glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particles->front_instance_buffer,
				particles->instance_buffer_stride_cache * (particles->amount - lifetime_split),
				particles->instance_buffer_stride_cache * (lifetime_split));
			glBeginTransformFeedback(GL_POINTS);
			// Reset back to normal.
			for (uint32_t j = 0; j < particles->num_attrib_arrays_cache; j++) {
				glEnableVertexAttribArray(j);
				glVertexAttribPointer(
					j, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * j));
			}

			glDrawArrays(GL_POINTS, 0, lifetime_split);
		}
	}
	else {
		glDrawArrays(GL_POINTS, 0, particles->amount);
	}

	glEndTransformFeedback();
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, 0, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticlesStorage::update_particles()
{
	if (!particle_update_list.first()) {
		// Return early to avoid unnecessary state changes.
		return;
	}

	RENDER_TIMESTAMP("Update GPUParticles");
	glEnable(GL_RASTERIZER_DISCARD);
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

	GLuint global_buffer =
		GLES3::MaterialStorage::global_shader_parameters_get_uniform_buffer();

	glBindBufferBase(GL_UNIFORM_BUFFER, PARTICLES_GLOBALS_UNIFORM_LOCATION, global_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	while (particle_update_list.first()) {
		// Use transform feedback to process particles.

		Particles* particles = particle_update_list.first()->self();

		particles->update_list.remove_from_list();
		particles->dirty = false;

		_particles_update_buffers(particles);

		if (particles->restart_request) {
			particles->prev_ticks = 0;
			particles->phase = 0;
			particles->prev_phase = 0;
			particles->clear = true;
			particles->restart_request = false;
			particles->frame_remainder = 0.0;
		}

		if (particles->inactive && !particles->emitting) {
			// go next
			continue;
		}

		if (particles->emitting) {
			if (particles->inactive) {
				// restart system from scratch
				particles->prev_ticks = 0;
				particles->phase = 0;
				particles->prev_phase = 0;
				particles->clear = true;
			}
			particles->inactive = false;
			particles->inactive_time = 0;
		}
		else {
			particles->inactive_time +=
				particles->speed_scale * RendererCompositor::get_frame_delta_time();
			if (particles->inactive_time > particles->lifetime * 1.2) {
				particles->inactive = true;
				continue;
			}
		}

		// Copy the instance buffer that was last used into the last_frame buffer.
		// sort_buffer should now be 2 frames out of date.
		if (particles->draw_order == RSE::PARTICLES_DRAW_ORDER_VIEW_DEPTH ||
			particles->draw_order == RSE::PARTICLES_DRAW_ORDER_REVERSE_LIFETIME) {
			_particles_allocate_history_buffers(particles);
			SWAP(particles->last_frame_buffer, particles->sort_buffer);

			glBindBuffer(GL_COPY_READ_BUFFER, particles->back_instance_buffer);
			glBindBuffer(GL_COPY_WRITE_BUFFER, particles->last_frame_buffer);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
				particles->instance_buffer_size_cache);

			// Last frame's last_frame turned into this frame's sort buffer.
			particles->sort_buffer_filled = particles->last_frame_buffer_filled;
			particles->sort_buffer_phase = particles->last_frame_phase;
			particles->last_frame_buffer_filled = true;
			particles->last_frame_phase = particles->phase;
			glBindBuffer(GL_COPY_READ_BUFFER, 0);
			glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
		}

		int fixed_fps = 0;
		if (particles->fixed_fps > 0) {
			fixed_fps = particles->fixed_fps;
		}

		// Request process and pre-process block
		{
			float todo = particles->clear ? particles->pre_process_time : 0;
			todo = todo > particles->request_process_time ? todo : particles->request_process_time;
			todo = todo > particles->request_process_time_residual
					   ? todo
					   : particles->request_process_time_residual;

			if (todo > 0.0) {
				real_t frame_time;
				if (fixed_fps > 0) {
					frame_time = 1.0 / fixed_fps;
				}
				else {
					frame_time = 1.0 / 30.0;
				}

				float tmp_scale = particles->speed_scale;
				// We need this otherwise the speed scale of the particle system influences the
				// `todo`.
				particles->speed_scale = 1.0;
				if (particles->clear) {
					todo = particles->pre_process_time;
					while (todo > 0.00001) {
						_particles_process(particles, frame_time > todo ? todo : frame_time);
						todo -= frame_time;
					}
				}
				if (particles->request_process_time > 0.0) {
					todo = particles->request_process_time;
					while (todo > 0.0) {
						_particles_process(particles, frame_time > todo ? todo : frame_time);
						todo -= frame_time;
					}
				}
				if (particles->request_process_time_residual > 0.0) {
					particles->emitting = false;
					todo = particles->request_process_time_residual;
					while (todo > 0.0) {
						_particles_process(particles, frame_time > todo ? todo : frame_time);
						todo -= frame_time;
					}
				}
				particles->speed_scale = tmp_scale;
			}

			particles->request_process_time = 0.0;
			particles->request_process_time_residual = 0.0;
		}

		double time_scale = MAX(particles->speed_scale, 0.0);

		if (fixed_fps > 0) {
			double frame_time = 1.0 / fixed_fps;
			double delta = RendererCompositor::get_frame_delta_time();
			if (delta > 0.1) { // avoid recursive stalls if fps goes below 10
				delta = 0.1;
			}
			else if (delta < 0.0) {
				delta = 0.0;
			}
			double todo = particles->frame_remainder + delta * time_scale;

			while (todo >= frame_time) {
				_particles_process(particles, frame_time);
				todo -= frame_time;
			}

			particles->frame_remainder = todo;

		}
		else {
			_particles_process(particles, RendererCompositor::get_frame_delta_time() * time_scale);
		}

		if (particles->request_process_time > 0.0) {
			double frame_time;
			if (fixed_fps > 0) {
				frame_time = 1.0 / fixed_fps;
			}
			else {
				frame_time = 1.0 / 30.0;
			}
			float tmp_scale = particles->speed_scale;
			particles->speed_scale = 1.0;
			double todo = particles->request_process_time;
			while (todo >= 0) {
				_particles_process(particles, frame_time);
				todo -= frame_time;
			}
			particles->speed_scale = tmp_scale;
			particles->request_process_time = 0.0;
		}

		// Copy particles to instance buffer and pack Color/Custom.
		// We don't have camera information here, so don't copy here if we need camera information
		// for view depth or align mode.

		if (particles->draw_order != RSE::PARTICLES_DRAW_ORDER_VIEW_DEPTH &&
			particles->transform_align != RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD &&
			particles->transform_align !=
				RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY &&
			particles->transform_align != RSE::PARTICLES_TRANSFORM_ALIGN_LOCAL_BILLBOARD) {
			_particles_update_instance_buffer(
				particles, Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0));

			if (particles->draw_order == RSE::PARTICLES_DRAW_ORDER_REVERSE_LIFETIME &&
				particles->sort_buffer_filled) {
				if (particles->mode == RSE::ParticlesMode::PARTICLES_MODE_2D) {
					_particles_reverse_lifetime_sort<ParticleInstanceData2D>(particles);
				}
				else {
					_particles_reverse_lifetime_sort<ParticleInstanceData3D>(particles);
				}
			}
		}

		SWAP(particles->front_instance_buffer, particles->back_instance_buffer);
		particles->last_change = RendererCompositor::get_frame_number();

		// At the end of update, the back_buffer contains the most up-to-date-information to read
		// from.

		particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
	}

	glDisable(GL_RASTERIZER_DISCARD);
}

template <typename ParticleInstanceData>
void ParticlesStorage::_particles_reverse_lifetime_sort(Particles* particles)
{
	glBindBuffer(GL_ARRAY_BUFFER, particles->sort_buffer);

	ParticleInstanceData* particle_array;
	uint32_t buffer_size = particles->amount * sizeof(ParticleInstanceData);
#ifndef __EMSCRIPTEN__
	particle_array = static_cast<ParticleInstanceData*>(
		glMapBufferRange(GL_ARRAY_BUFFER, 0, buffer_size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

	ERR_FAIL_NULL(particle_array);
#else
	LocalVector<ParticleInstanceData> particle_vector;
	particle_vector.resize(particles->amount);
	particle_array = particle_vector.ptr();
	godot_webgl2_glGetBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, particle_array);
#endif

	uint32_t lifetime_split =
		(MIN(int(particles->amount * particles->sort_buffer_phase), particles->amount - 1) + 1) %
		particles->amount;
	for (uint32_t i = 0; i < lifetime_split / 2; i++) {
		SWAP(particle_array[i], particle_array[lifetime_split - i - 1]);
	}

	for (uint32_t i = 0; i < (particles->amount - lifetime_split) / 2; i++) {
		SWAP(particle_array[lifetime_split + i], particle_array[particles->amount - 1 - i]);
	}

#ifndef __EMSCRIPTEN__
	glUnmapBuffer(GL_ARRAY_BUFFER);
#else
	glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, particle_vector.ptr());
#endif
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Dependency* ParticlesStorage::particles_get_dependency(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, nullptr);

	return &particles->dependency;
}

bool ParticlesStorage::particles_is_inactive(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, false);
	return !particles->emitting && particles->inactive;
}

/* PARTICLES COLLISION API */

RID ParticlesStorage::particles_collision_allocate()
{
	return particles_collision_owner.allocate_rid();
}

void ParticlesStorage::particles_collision_initialize(RID p_rid)
{
	particles_collision_owner.initialize_rid(p_rid, ParticlesCollision());
}

void ParticlesStorage::particles_collision_free(RID p_rid)
{
	ParticlesCollision* particles_collision = particles_collision_owner.get_or_null(p_rid);

	if (particles_collision->heightfield_texture != 0) {
		GLES3::Utilities::texture_free_data(
			particles_collision->heightfield_texture);
		particles_collision->heightfield_texture = 0;
		glDeleteFramebuffers(1, &particles_collision->heightfield_fb);
		particles_collision->heightfield_fb = 0;
	}
	particles_collision->dependency.deleted_notify(p_rid);
	particles_collision_owner.free(p_rid);
}

GLuint ParticlesStorage::particles_collision_get_heightfield_framebuffer(
	RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, 0);
	ERR_FAIL_COND_V(
		particles_collision->type != RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE, 0);

	if (particles_collision->heightfield_texture == 0) {
		// create
		const int resolutions[RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_MAX] = {
			256, 512, 1024, 2048, 4096, 8192};
		Size2i size;
		if (particles_collision->extents.x > particles_collision->extents.z) {
			size.x = resolutions[particles_collision->heightfield_resolution];
			size.y =
				int32_t(particles_collision->extents.z / particles_collision->extents.x * size.x);
		}
		else {
			size.y = resolutions[particles_collision->heightfield_resolution];
			size.x =
				int32_t(particles_collision->extents.x / particles_collision->extents.z * size.y);
		}

		glGenTextures(1, &particles_collision->heightfield_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, particles_collision->heightfield_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.x, size.y, 0, GL_DEPTH_COMPONENT,
			GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		glGenFramebuffers(1, &particles_collision->heightfield_fb);
		glBindFramebuffer(GL_FRAMEBUFFER, particles_collision->heightfield_fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
			particles_collision->heightfield_texture, 0);
#ifdef DEBUG_ENABLED
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			WARN_PRINT("Could not create heightmap texture, status: " +
					   GLES3::TextureStorage::get_framebuffer_error(status));
		}
#endif
		GLES3::Utilities::texture_allocated_data(
			particles_collision->heightfield_texture, size.x * size.y * 4,
			"Particles collision heightfield texture");

		particles_collision->heightfield_fb_size = size;

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	}

	return particles_collision->heightfield_fb;
}

void ParticlesStorage::particles_collision_set_collision_type(
	RID p_particles_collision, RSE::ParticlesCollisionType p_type)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	if (p_type == particles_collision->type) {
		return;
	}

	if (particles_collision->heightfield_texture != 0) {
		GLES3::Utilities::texture_free_data(
			particles_collision->heightfield_texture);
		particles_collision->heightfield_texture = 0;
		glDeleteFramebuffers(1, &particles_collision->heightfield_fb);
		particles_collision->heightfield_fb = 0;
	}

	particles_collision->type = p_type;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_cull_mask(
	RID p_particles_collision, uint32_t p_cull_mask)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->cull_mask = p_cull_mask;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_CULL_MASK);
}

uint32_t ParticlesStorage::particles_collision_get_cull_mask(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, 0);
	return particles_collision->cull_mask;
}

void ParticlesStorage::particles_collision_set_sphere_radius(
	RID p_particles_collision, real_t p_radius)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->radius = p_radius;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_box_extents(
	RID p_particles_collision, const Vector3& p_extents)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->extents = p_extents;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_attractor_strength(
	RID p_particles_collision, real_t p_strength)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_strength = p_strength;
}

void ParticlesStorage::particles_collision_set_attractor_directionality(
	RID p_particles_collision, real_t p_directionality)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_directionality = p_directionality;
}

void ParticlesStorage::particles_collision_set_attractor_attenuation(
	RID p_particles_collision, real_t p_curve)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_attenuation = p_curve;
}

void ParticlesStorage::particles_collision_set_field_texture(
	RID p_particles_collision, RID p_texture)
{
	WARN_PRINT_ONCE_ED(
		"The Compatibility renderer does not support SDF collisions in 3D particle shaders");
}

void ParticlesStorage::particles_collision_height_field_update(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_height_field_resolution(
	RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	ERR_FAIL_INDEX(p_resolution, RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_MAX);

	if (particles_collision->heightfield_resolution == p_resolution) {
		return;
	}

	particles_collision->heightfield_resolution = p_resolution;

	if (particles_collision->heightfield_texture != 0) {
		GLES3::Utilities::texture_free_data(
			particles_collision->heightfield_texture);
		particles_collision->heightfield_texture = 0;
		glDeleteFramebuffers(1, &particles_collision->heightfield_fb);
		particles_collision->heightfield_fb = 0;
	}
}

AABB ParticlesStorage::particles_collision_get_aabb(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, AABB());

	switch (particles_collision->type) {
	case RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT:
	case RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE: {
		AABB aabb;
		aabb.position = -Vector3(1, 1, 1) * particles_collision->radius;
		aabb.size = Vector3(2, 2, 2) * particles_collision->radius;
		return aabb;
	}
	default: {
		AABB aabb;
		aabb.position = -particles_collision->extents;
		aabb.size = particles_collision->extents * 2;
		return aabb;
	}
	}
}

Vector3 ParticlesStorage::particles_collision_get_extents(RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, Vector3());
	return particles_collision->extents;
}

bool ParticlesStorage::particles_collision_is_heightfield(RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, false);
	return particles_collision->type == RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE;
}

uint32_t ParticlesStorage::particles_collision_get_height_field_mask(
	RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, false);
	return particles_collision->heightfield_mask;
}

void ParticlesStorage::particles_collision_set_height_field_mask(
	RID p_particles_collision, uint32_t p_heightfield_mask)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->heightfield_mask = p_heightfield_mask;
}

Dependency* ParticlesStorage::particles_collision_get_dependency(RID p_particles_collision)
{
	ParticlesCollision* pc = particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(pc, nullptr);

	return &pc->dependency;
}

/* Particles collision instance */

RID ParticlesStorage::particles_collision_instance_create(RID p_collision)
{
	ParticlesCollisionInstance pci;
	pci.collision = p_collision;
	return particles_collision_instance_owner.make_rid(pci);
}

void ParticlesStorage::particles_collision_instance_free(RID p_rid)
{
	particles_collision_instance_owner.free(p_rid);
}

void ParticlesStorage::particles_collision_instance_set_transform(
	RID p_collision_instance, const Transform3D& p_transform)
{
	ParticlesCollisionInstance* pci =
		particles_collision_instance_owner.get_or_null(p_collision_instance);
	ERR_FAIL_NULL(pci);
	pci->transform = p_transform;
}

void ParticlesStorage::particles_collision_instance_set_active(
	RID p_collision_instance, bool p_active)
{
	ParticlesCollisionInstance* pci =
		particles_collision_instance_owner.get_or_null(p_collision_instance);
	ERR_FAIL_NULL(pci);
	pci->active = p_active;
}

#endif // GLES3_ENABLED



// ============================================================================
// Merged from drivers/gles3/storage/texture_storage.cpp
// ============================================================================

#include "texture_storage.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "drivers/gles3/storage/utilities.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/material_storage.h"
#include "drivers/gles3/storage/utilities.h"

using namespace GLES3;

static const GLenum _cube_side_enum[6] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

// Has to be a separate call from TextureStorage initialization due to interacting with
// MaterialStorage
void GLES3::TextureStorage::_tex_blit_shader_initialize()
{
	{
		String global_defines;
		global_defines +=
			"#define MAX_GLOBAL_SHADER_UNIFORMS 256\n"; // TODO: this is arbitrary for now
		MaterialStorage::shaders->tex_blit_shader.initialize(global_defines, 1);
	}

	{
		// default material and shader for Texture Blit shader
		tex_blit_shader.default_shader = MaterialStorage::shader_allocate();
		MaterialStorage::shader_initialize(tex_blit_shader.default_shader);
		MaterialStorage::shader_set_code(tex_blit_shader.default_shader, R"(
// Default Texture Blit shader.

shader_type texture_blit;
render_mode blend_mix;

uniform sampler2D source_texture0 : hint_blit_source0;
uniform sampler2D source_texture1 : hint_blit_source1;
uniform sampler2D source_texture2 : hint_blit_source2;
uniform sampler2D source_texture3 : hint_blit_source3;

void blit() {
	// Copies from each whole source texture to a rect on each output texture.
	COLOR0 = texture(source_texture0, UV) * MODULATE;
	COLOR1 = texture(source_texture1, UV) * MODULATE;
	COLOR2 = texture(source_texture2, UV) * MODULATE;
	COLOR3 = texture(source_texture3, UV) * MODULATE;
}
)");
		tex_blit_shader.default_material = MaterialStorage::material_allocate();
		MaterialStorage::material_initialize(tex_blit_shader.default_material);
		MaterialStorage::material_set_shader(
			tex_blit_shader.default_material, tex_blit_shader.default_shader);
	}

	{
		// Set up Frame & Vertex Buffers for TextureBlit Shaders
		// Just a 1x1 Quad to draw
		glGenFramebuffers(1, &tex_blit_fbo);

		glGenBuffers(1, &tex_blit_quad);
		glBindBuffer(GL_ARRAY_BUFFER, tex_blit_quad);

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

		glGenVertexArrays(1, &tex_blit_quad_array);
		glBindVertexArray(tex_blit_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, tex_blit_quad);
		glVertexAttribPointer(RSE::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RSE::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind
	}

	tex_blit_shader.initialized = true;
}

// Has to be a separate call from GLES3::TextureStorage destruction due to interacting with Material
// Storage
void GLES3::TextureStorage::_tex_blit_shader_free()
{
	if (tex_blit_shader.initialized) {
		glDeleteFramebuffers(1, &tex_blit_fbo);
		glDeleteBuffers(1, &tex_blit_quad);
		glDeleteVertexArrays(1, &tex_blit_quad_array);
		MaterialStorage::material_free(tex_blit_shader.default_material);
		MaterialStorage::shader_free(tex_blit_shader.default_shader);
	}
}

/* Canvas Texture API */

RID GLES3::TextureStorage::canvas_texture_allocate() { return canvas_texture_owner.allocate_rid(); }

void GLES3::TextureStorage::canvas_texture_initialize(RID p_rid)
{
	canvas_texture_owner.initialize_rid(p_rid);
}

void GLES3::TextureStorage::canvas_texture_free(RID p_rid) { canvas_texture_owner.free(p_rid); }

void GLES3::TextureStorage::canvas_texture_set_channel(
	RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	switch (p_channel) {
	case RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE: {
		ct->diffuse = p_texture;
	} break;
	case RSE::CANVAS_TEXTURE_CHANNEL_NORMAL: {
		ct->normal_map = p_texture;
	} break;
	case RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR: {
		ct->specular = p_texture;
	} break;
	}
}

void GLES3::TextureStorage::canvas_texture_set_shading_parameters(
	RID p_canvas_texture, const Color& p_specular_color, float p_shininess)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->specular_color.r = p_specular_color.r;
	ct->specular_color.g = p_specular_color.g;
	ct->specular_color.b = p_specular_color.b;
	ct->specular_color.a = p_shininess;
}

void GLES3::TextureStorage::canvas_texture_set_texture_filter(
	RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->texture_filter = p_filter;
}

void GLES3::TextureStorage::canvas_texture_set_texture_repeat(
	RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->texture_repeat = p_repeat;
}

/* Texture API */

static inline Error _get_gl_uncompressed_format(const Ref<Image>& p_image, Image::Format p_format,
	Image::Format& r_real_format, GLenum& r_gl_format, GLenum& r_gl_internal_format,
	GLenum& r_gl_type)
{
	switch (p_format) {
	case Image::FORMAT_L8: {
		if (RasterizerUtilGLES3::is_gles_over_gl()) {
			r_gl_internal_format = GL_R8;
			r_gl_format = GL_RED;
			r_gl_type = GL_UNSIGNED_BYTE;
		}
		else {
			r_gl_internal_format = GL_LUMINANCE;
			r_gl_format = GL_LUMINANCE;
			r_gl_type = GL_UNSIGNED_BYTE;
		}
	} break;
	case Image::FORMAT_LA8: {
		if (RasterizerUtilGLES3::is_gles_over_gl()) {
			r_gl_internal_format = GL_RG8;
			r_gl_format = GL_RG;
			r_gl_type = GL_UNSIGNED_BYTE;
		}
		else {
			r_gl_internal_format = GL_LUMINANCE_ALPHA;
			r_gl_format = GL_LUMINANCE_ALPHA;
			r_gl_type = GL_UNSIGNED_BYTE;
		}
	} break;
	case Image::FORMAT_R8: {
		r_gl_internal_format = GL_R8;
		r_gl_format = GL_RED;
		r_gl_type = GL_UNSIGNED_BYTE;
	} break;
	case Image::FORMAT_RG8: {
		r_gl_internal_format = GL_RG8;
		r_gl_format = GL_RG;
		r_gl_type = GL_UNSIGNED_BYTE;
	} break;
	case Image::FORMAT_RGB8: {
		r_gl_internal_format = GL_RGB8;
		r_gl_format = GL_RGB;
		r_gl_type = GL_UNSIGNED_BYTE;
	} break;
	case Image::FORMAT_RGBA8: {
		r_gl_internal_format = GL_RGBA8;
		r_gl_format = GL_RGBA;
		r_gl_type = GL_UNSIGNED_BYTE;
	} break;
	case Image::FORMAT_RGBA4444: {
		r_gl_internal_format = GL_RGBA4;
		r_gl_format = GL_RGBA;
		r_gl_type = GL_UNSIGNED_SHORT_4_4_4_4;
	} break;
	case Image::FORMAT_RGB565: {
		r_gl_internal_format = GL_RGB565;
		r_gl_format = GL_RGB;
		r_gl_type = GL_UNSIGNED_SHORT_5_6_5;
	} break;
	case Image::FORMAT_RF: {
		if (Config::float_texture_linear_supported) {
			r_gl_internal_format = GL_R32F;
			r_gl_format = GL_RED;
			r_gl_type = GL_FLOAT;
		}
		else {
			if (p_image.is_valid()) {
				p_image->convert(Image::FORMAT_RH);
			}
			r_real_format = Image::FORMAT_RH;
			r_gl_internal_format = GL_R16F;
			r_gl_format = GL_RED;
			r_gl_type = GL_HALF_FLOAT;
		}
	} break;
	case Image::FORMAT_RGF: {
		if (Config::float_texture_linear_supported) {
			r_gl_internal_format = GL_RG32F;
			r_gl_format = GL_RG;
			r_gl_type = GL_FLOAT;
		}
		else {
			if (p_image.is_valid()) {
				p_image->convert(Image::FORMAT_RGH);
			}
			r_real_format = Image::FORMAT_RGH;
			r_gl_internal_format = GL_RG16F;
			r_gl_format = GL_RG;
			r_gl_type = GL_HALF_FLOAT;
		}
	} break;
	case Image::FORMAT_RGBF: {
		if (Config::float_texture_linear_supported) {
			r_gl_internal_format = GL_RGB32F;
			r_gl_format = GL_RGB;
			r_gl_type = GL_FLOAT;
		}
		else {
			if (p_image.is_valid()) {
				p_image->convert(Image::FORMAT_RGBH);
			}
			r_real_format = Image::FORMAT_RGBH;
			r_gl_internal_format = GL_RGB16F;
			r_gl_format = GL_RGB;
			r_gl_type = GL_HALF_FLOAT;
		}
	} break;
	case Image::FORMAT_RGBAF: {
		if (Config::float_texture_linear_supported) {
			r_gl_internal_format = GL_RGBA32F;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_FLOAT;
		}
		else {
			if (p_image.is_valid()) {
				p_image->convert(Image::FORMAT_RGBAH);
			}
			r_real_format = Image::FORMAT_RGBAH;
			r_gl_internal_format = GL_RGBA16F;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_HALF_FLOAT;
		}
	} break;
	case Image::FORMAT_RH: {
		r_gl_internal_format = GL_R16F;
		r_gl_format = GL_RED;
		r_gl_type = GL_HALF_FLOAT;
	} break;
	case Image::FORMAT_RGH: {
		r_gl_internal_format = GL_RG16F;
		r_gl_format = GL_RG;
		r_gl_type = GL_HALF_FLOAT;
	} break;
	case Image::FORMAT_RGBH: {
		r_gl_internal_format = GL_RGB16F;
		r_gl_format = GL_RGB;
		r_gl_type = GL_HALF_FLOAT;
	} break;
	case Image::FORMAT_RGBAH: {
		r_gl_internal_format = GL_RGBA16F;
		r_gl_format = GL_RGBA;
		r_gl_type = GL_HALF_FLOAT;
	} break;
	case Image::FORMAT_RGBE9995: {
		r_gl_internal_format = GL_RGB9_E5;
		r_gl_format = GL_RGB;
		r_gl_type = GL_UNSIGNED_INT_5_9_9_9_REV;
	} break;
	case Image::FORMAT_R16: {
		if (Config::unorm16_texture_supported) {
			r_gl_internal_format = _EXT_R16;
			r_gl_format = GL_RED;
			r_gl_type = GL_UNSIGNED_SHORT;
		}
		else {
			if (Config::float_texture_linear_supported) {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RF);
				}
				r_real_format = Image::FORMAT_RF;
				r_gl_internal_format = GL_R32F;
				r_gl_format = GL_RED;
				r_gl_type = GL_FLOAT;
			}
			else {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RH);
				}
				r_real_format = Image::FORMAT_RH;
				r_gl_internal_format = GL_R16F;
				r_gl_format = GL_RED;
				r_gl_type = GL_HALF_FLOAT;
			}
		}
	} break;
	case Image::FORMAT_RG16: {
		if (Config::unorm16_texture_supported) {
			r_gl_internal_format = _EXT_RG16;
			r_gl_format = GL_RG;
			r_gl_type = GL_UNSIGNED_SHORT;
		}
		else {
			if (Config::float_texture_linear_supported) {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGF);
				}
				r_real_format = Image::FORMAT_RGF;
				r_gl_internal_format = GL_RG32F;
				r_gl_format = GL_RG;
				r_gl_type = GL_FLOAT;
			}
			else {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGH);
				}
				r_real_format = Image::FORMAT_RGH;
				r_gl_internal_format = GL_RG16F;
				r_gl_format = GL_RG;
				r_gl_type = GL_HALF_FLOAT;
			}
		}
	} break;
	case Image::FORMAT_RGB16: {
		if (Config::unorm16_texture_supported) {
			r_gl_internal_format = _EXT_RGB16;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_SHORT;
		}
		else {
			if (Config::float_texture_linear_supported) {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGBF);
				}
				r_real_format = Image::FORMAT_RGBF;
				r_gl_internal_format = GL_RGB32F;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
			}
			else {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGBH);
				}
				r_real_format = Image::FORMAT_RGBH;
				r_gl_internal_format = GL_RGB16F;
				r_gl_format = GL_RGB;
				r_gl_type = GL_HALF_FLOAT;
			}
		}
	} break;
	case Image::FORMAT_RGBA16: {
		if (Config::unorm16_texture_supported) {
			r_gl_internal_format = _EXT_RGBA16;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_SHORT;
		}
		else {
			if (Config::float_texture_linear_supported) {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGBAF);
				}
				r_real_format = Image::FORMAT_RGBAF;
				r_gl_internal_format = GL_RGBA32F;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_FLOAT;
			}
			else {
				if (p_image.is_valid()) {
					p_image->convert(Image::FORMAT_RGH);
				}
				r_real_format = Image::FORMAT_RGH;
				r_gl_internal_format = GL_RGBA16F;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_HALF_FLOAT;
			}
		}
	} break;
	case Image::FORMAT_R16I: {
		r_gl_internal_format = GL_R16UI;
		r_gl_format = GL_RED_INTEGER;
		r_gl_type = GL_UNSIGNED_SHORT;
	} break;
	case Image::FORMAT_RG16I: {
		r_gl_internal_format = GL_RG16UI;
		r_gl_format = GL_RG_INTEGER;
		r_gl_type = GL_UNSIGNED_SHORT;
	} break;
	case Image::FORMAT_RGB16I: {
		r_gl_internal_format = GL_RGB16UI;
		r_gl_format = GL_RGB_INTEGER;
		r_gl_type = GL_UNSIGNED_SHORT;
	} break;
	case Image::FORMAT_RGBA16I: {
		r_gl_internal_format = GL_RGBA16UI;
		r_gl_format = GL_RGBA_INTEGER;
		r_gl_type = GL_UNSIGNED_SHORT;
	} break;
	default: {
		return ERR_UNAVAILABLE;
	}
	}

	return OK;
}

Ref<Image> GLES3::TextureStorage::_get_gl_image_and_format(const Ref<Image>& p_image,
	Image::Format p_format, Image::Format& r_real_format, GLenum& r_gl_format,
	GLenum& r_gl_internal_format, GLenum& r_gl_type, bool& r_compressed, bool p_force_decompress)
{
	r_gl_format = 0;
	Ref<Image> image = p_image;
	r_compressed = false;
	r_real_format = p_format;

	if (!Image::is_format_compressed(p_format)) {
		Error err = _get_gl_uncompressed_format(
			p_image, p_format, r_real_format, r_gl_format, r_gl_internal_format, r_gl_type);
		ERR_FAIL_COND_V_MSG(err != OK, Ref<Image>(),
			vformat(
				"The image format %d is not supported by the Compatibility renderer.", p_format));

		if (p_format != r_real_format) {
			WARN_PRINT(vformat("Image format %s not supported by hardware, converting to %s.",
				Image::get_format_name(p_format), Image::get_format_name(r_real_format)));
		}

		return p_image;
	}

	// For compressed images, some formats may not be supported by the current device and will
	// require decompression.
	bool need_decompress = false;
	bool decompress_ra_to_rg = false;

	switch (p_format) {
	case Image::FORMAT_DXT1: {
		if (Config::s3tc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGB_S3TC_DXT1_EXT;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_DXT3: {
		if (Config::s3tc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_DXT5: {
		if (Config::s3tc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_RGTC_R: {
		if (Config::rgtc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RED_RGTC1_EXT;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_RGTC_RG: {
		if (Config::rgtc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_BPTC_RGBA: {
		if (Config::bptc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_BPTC_UNORM;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_BPTC_RGBF: {
		if (Config::bptc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
			r_gl_format = GL_RGB;
			r_gl_type = GL_FLOAT;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_BPTC_RGBFU: {
		if (Config::bptc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
			r_gl_format = GL_RGB;
			r_gl_type = GL_FLOAT;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_R11: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_R11_EAC;
			r_gl_format = GL_RED;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_R11S: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_SIGNED_R11_EAC;
			r_gl_format = GL_RED;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_RG11: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RG11_EAC;
			r_gl_format = GL_RG;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_RG11S: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_SIGNED_RG11_EAC;
			r_gl_format = GL_RG;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC:
	case Image::FORMAT_ETC2_RGB8: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGB8_ETC2;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_RGBA8: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA8_ETC2_EAC;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_RGB8A1: {
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ETC2_RA_AS_RG: {
#ifndef WEB_ENABLED
		if (Config::etc2_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA8_ETC2_EAC;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else
#endif
		{
			need_decompress = true;
		}
		decompress_ra_to_rg = true;
	} break;
	case Image::FORMAT_DXT5_RA_AS_RG: {
#ifndef WEB_ENABLED
		if (Config::s3tc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else
#endif
		{
			need_decompress = true;
		}
		decompress_ra_to_rg = true;
	} break;
	case Image::FORMAT_ASTC_4x4: {
		if (Config::astc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_4x4_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ASTC_4x4_HDR: {
		if (Config::astc_hdr_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_4x4_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ASTC_6x6: {
		if (Config::astc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_6x6_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ASTC_6x6_HDR: {
		if (Config::astc_hdr_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_6x6_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ASTC_8x8: {
		if (Config::astc_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_8x8_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	case Image::FORMAT_ASTC_8x8_HDR: {
		if (Config::astc_hdr_supported) {
			r_gl_internal_format = _EXT_COMPRESSED_RGBA_ASTC_8x8_KHR;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;
			r_compressed = true;
		}
		else {
			need_decompress = true;
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(Ref<Image>(),
			vformat(
				"The image format %d is not supported by the Compatibility renderer.", p_format));
	}
	}

	if (need_decompress || p_force_decompress) {
		return image;
	}

	return p_image;
}

RID GLES3::TextureStorage::texture_allocate() { return texture_owner.allocate_rid(); }

void GLES3::TextureStorage::texture_free(RID p_texture)
{
	Texture* t = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(t);
	ERR_FAIL_COND(t->is_render_target);

	memdelete(t->canvas_texture);

	bool must_free_data = false;
	if (t->is_proxy) {
		if (t->proxy_to.is_valid()) {
			Texture* proxy_to = texture_owner.get_or_null(t->proxy_to);
			if (proxy_to) {
				proxy_to->proxies.erase(p_texture);
			}
		}
	}
	else {
		must_free_data = t->tex_id != 0 && !t->is_from_native_handle;
	}
	if (must_free_data) {
		GLES3::Utilities::texture_free_data(t->tex_id);
		t->tex_id = 0;
	}

	texture_atlas_remove_texture(p_texture);

	for (uint32_t i = 0; i < t->proxies.size(); i++) {
		Texture* p = texture_owner.get_or_null(t->proxies[i]);
		ERR_CONTINUE(!p);
		p->proxy_to = RID();
		p->tex_id = 0;
	}

	texture_owner.free(p_texture);
}

void GLES3::TextureStorage::texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
{
	ERR_FAIL_COND(p_image.is_null());

	Texture texture;
	texture.width = p_image->get_width();
	texture.height = p_image->get_height();
	texture.alloc_width = texture.width;
	texture.alloc_height = texture.height;
	texture.mipmaps = p_image->get_mipmap_count() + 1;
	texture.format = p_image->get_format();
	texture.type = Texture::TYPE_2D;
	texture.target = GL_TEXTURE_2D;
	_get_gl_image_and_format(Ref<Image>(), texture.format, texture.real_format,
		texture.gl_format_cache, texture.gl_internal_format_cache, texture.gl_type_cache,
		texture.compressed, false);
	texture.total_data_size = p_image->get_image_data_size(
		texture.width, texture.height, texture.format, texture.mipmaps);
	texture.active = true;
	glGenTextures(1, &texture.tex_id);
	GLES3::Utilities::texture_allocated_data(texture.tex_id, texture.total_data_size, "Texture 2D");
	texture_owner.initialize_rid(p_texture, texture);
	texture_set_data(p_texture, p_image);
}

void GLES3::TextureStorage::texture_external_initialize(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
	Texture texture;
	texture.active = true;
	texture.alloc_width = texture.width = p_width;
	texture.alloc_height = texture.height = p_height;
	texture.real_format = texture.format = Image::FORMAT_RGB8;
	texture.type = Texture::TYPE_2D;

	if (GLES3::Config::external_texture_supported) {
		texture.target = _GL_TEXTURE_EXTERNAL_OES;
	}
	else {
		texture.target = GL_TEXTURE_2D;
	}

	glGenTextures(1, &texture.tex_id);
	glBindTexture(texture.target, texture.tex_id);

#ifdef ANDROID_ENABLED
	if (texture.target == _GL_TEXTURE_EXTERNAL_OES) {
		if (p_external_buffer) {
			GLES3::Config::eglEGLImageTargetTexture2DOES(
				_GL_TEXTURE_EXTERNAL_OES, reinterpret_cast<void*>(p_external_buffer));
		}
		texture.total_data_size = 0;
	}
	else
#endif
	{
		// If external textures aren't supported, allocate an empty 1x1 texture.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		texture.total_data_size = 3;
	}

	glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLES3::Utilities::texture_allocated_data(
		texture.tex_id, texture.total_data_size, "Texture External");
	texture_owner.initialize_rid(p_texture, texture);

	glBindTexture(texture.target, 0);
}

void GLES3::TextureStorage::texture_2d_layered_initialize(
	RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
{
	ERR_FAIL_COND(p_layers.is_empty());

	ERR_FAIL_COND(p_layered_type == RSE::TEXTURE_LAYERED_CUBEMAP && p_layers.size() != 6);
	ERR_FAIL_COND_MSG(p_layered_type == RSE::TEXTURE_LAYERED_CUBEMAP_ARRAY,
		"Cubemap Arrays are not supported in the Compatibility renderer.");

	const Ref<Image>& image = p_layers[0];
	{
		int valid_width = 0;
		int valid_height = 0;
		bool valid_mipmaps = false;
		Image::Format valid_format = Image::FORMAT_MAX;

		for (int i = 0; i < p_layers.size(); i++) {
			ERR_FAIL_COND(p_layers[i]->is_empty());

			if (i == 0) {
				valid_width = p_layers[i]->get_width();
				valid_height = p_layers[i]->get_height();
				valid_format = p_layers[i]->get_format();
				valid_mipmaps = p_layers[i]->has_mipmaps();
			}
			else {
				ERR_FAIL_COND(p_layers[i]->get_width() != valid_width);
				ERR_FAIL_COND(p_layers[i]->get_height() != valid_height);
				ERR_FAIL_COND(p_layers[i]->get_format() != valid_format);
				ERR_FAIL_COND(p_layers[i]->has_mipmaps() != valid_mipmaps);
			}
		}
	}

	Texture texture;
	texture.width = image->get_width();
	texture.height = image->get_height();
	texture.alloc_width = texture.width;
	texture.alloc_height = texture.height;
	texture.mipmaps = image->get_mipmap_count() + 1;
	texture.format = image->get_format();
	texture.type = Texture::TYPE_LAYERED;
	texture.layered_type = p_layered_type;
	texture.target =
		p_layered_type == RSE::TEXTURE_LAYERED_CUBEMAP ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D_ARRAY;
	texture.layers = p_layers.size();
	_get_gl_image_and_format(Ref<Image>(), texture.format, texture.real_format,
		texture.gl_format_cache, texture.gl_internal_format_cache, texture.gl_type_cache,
		texture.compressed, false);
	texture.total_data_size = p_layers[0]->get_image_data_size(
								  texture.width, texture.height, texture.format, texture.mipmaps) *
							  texture.layers;
	texture.active = true;
	glGenTextures(1, &texture.tex_id);
	GLES3::Utilities::texture_allocated_data(
		texture.tex_id, texture.total_data_size, "Texture Layered");
	texture_owner.initialize_rid(p_texture, texture);
	for (int i = 0; i < p_layers.size(); i++) {
		_texture_set_data(p_texture, p_layers[i], i, i == 0);
	}
}

void GLES3::TextureStorage::texture_3d_initialize(RID p_texture, Image::Format p_format,
	int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	ERR_FAIL_COND(p_data.is_empty());

	Image::Image3DValidateError verr =
		Image::validate_3d_image(p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	ERR_FAIL_COND_MSG(
		verr != Image::VALIDATE_3D_OK, Image::get_3d_image_validation_error_text(verr));

	Ref<Image> image = p_data[0];
	int mipmap_count = 0;
	{
		Size2i prev_size;
		for (int i = 0; i < p_data.size(); i++) {
			Size2i img_size(p_data[i]->get_width(), p_data[i]->get_height());
			if (img_size != prev_size) {
				mipmap_count++;
			}
			prev_size = img_size;
		}
	}

	Texture texture;
	texture.width = p_width;
	texture.height = p_height;
	texture.depth = p_depth;
	texture.alloc_width = texture.width;
	texture.alloc_height = texture.height;
	texture.mipmaps = mipmap_count;
	texture.format = image->get_format();
	texture.type = Texture::TYPE_3D;
	texture.target = GL_TEXTURE_3D;
	_get_gl_image_and_format(Ref<Image>(), texture.format, texture.real_format,
		texture.gl_format_cache, texture.gl_internal_format_cache, texture.gl_type_cache,
		texture.compressed, false);
	texture.total_data_size = p_data[0]->get_image_data_size(
								  texture.width, texture.height, texture.format, texture.mipmaps) *
							  texture.depth;
	texture.active = true;
	glGenTextures(1, &texture.tex_id);
	GLES3::Utilities::texture_allocated_data(texture.tex_id, texture.total_data_size, "Texture 3D");
	texture_owner.initialize_rid(p_texture, texture);
	_texture_set_3d_data(p_texture, p_data, true);
}

// Called internally when texture_proxy_create(p_base) is called.
// Note: p_base is the root and p_texture is the proxy.
void GLES3::TextureStorage::texture_proxy_initialize(RID p_texture, RID p_base)
{
	Texture* texture = texture_owner.get_or_null(p_base);
	ERR_FAIL_NULL(texture);
	Texture proxy_tex;
	proxy_tex.copy_from(*texture);
	proxy_tex.proxy_to = p_base;
	proxy_tex.is_render_target = false;
	proxy_tex.is_proxy = true;
	proxy_tex.proxies.clear();
	texture->proxies.push_back(p_texture);
	texture_owner.initialize_rid(p_texture, proxy_tex);
}

void GLES3::TextureStorage::texture_drawable_initialize(RID p_texture, int p_width, int p_height,
	RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_with_mipmaps)
{
	// Behaves identically to Texture_2D_Initialize by generating a white image based on parameters.

	// GUARDRAIL: Bad Widths/Heights
	ERR_FAIL_COND_MSG(
		p_width <= 0 || p_height <= 0, "Drawable Texture Width or Height cannot be less than 1.");
	ERR_FAIL_COND_MSG(p_width >= 16384 || p_height >= 16384,
		"Drawable Texture Width or Height cannot be greater than 16383.");

	Image::Format format;
	switch (p_format) {
	case RSE::TEXTURE_DRAWABLE_FORMAT_RGBA8:
		format = Image::FORMAT_RGBA8;
		break;
	case RSE::TEXTURE_DRAWABLE_FORMAT_RGBA8_SRGB:
		format = Image::FORMAT_RGBA8;
		break;
	case RSE::TEXTURE_DRAWABLE_FORMAT_RGBAH:
		format = Image::FORMAT_RGBAH;
		break;
	case RSE::TEXTURE_DRAWABLE_FORMAT_RGBAF:
		format = Image::FORMAT_RGBAF;
		break;
	default:
		format = Image::FORMAT_RGBA8;
	}

	Ref<Image> image = Image::create_empty(p_width, p_height, p_with_mipmaps, format);
	image->fill(p_color);
	Texture texture;
	texture.width = image->get_width();
	texture.height = image->get_height();
	texture.alloc_width = texture.width;
	texture.alloc_height = texture.height;
	texture.mipmaps = image->get_mipmap_count() + 1;
	texture.format = image->get_format();
	texture.drawable_type = p_format;
	texture.type = Texture::TYPE_2D;
	texture.target = GL_TEXTURE_2D;
	_get_gl_image_and_format(Ref<Image>(), texture.format, texture.real_format,
		texture.gl_format_cache, texture.gl_internal_format_cache, texture.gl_type_cache,
		texture.compressed, false);
	texture.total_data_size =
		image->get_image_data_size(texture.width, texture.height, texture.format, texture.mipmaps);
	texture.active = true;
	glGenTextures(1, &texture.tex_id);
	GLES3::Utilities::texture_allocated_data(texture.tex_id, texture.total_data_size, "Texture 2D");
	texture_owner.initialize_rid(p_texture, texture);
	texture_set_data(p_texture, image);
}

RID GLES3::TextureStorage::texture_create_from_native_handle(RSE::TextureType p_type,
	Image::Format p_format, uint64_t p_native_handle, int p_width, int p_height, int p_depth,
	int p_layers, RSE::TextureLayeredType p_layered_type)
{
	Texture texture;
	texture.active = true;
	texture.is_from_native_handle = true;

	switch (p_type) {
	case RSE::TEXTURE_TYPE_2D: {
		texture.type = Texture::TYPE_2D;
		texture.target = GL_TEXTURE_2D;
	} break;
	case RSE::TEXTURE_TYPE_3D: {
		texture.type = Texture::TYPE_3D;
		texture.target = GL_TEXTURE_3D;
	} break;
	case RSE::TEXTURE_TYPE_LAYERED: {
		texture.type = Texture::TYPE_LAYERED;
		texture.target = GL_TEXTURE_2D_ARRAY;
	} break;
	}

	texture.real_format = texture.format = p_format;
	texture.tex_id = p_native_handle;
	texture.alloc_width = texture.width = p_width;
	texture.alloc_height = texture.height = p_height;
	texture.depth = p_depth;
	texture.layers = p_layers;
	texture.layered_type = p_layered_type;

	return texture_owner.make_rid(texture);
}

void GLES3::TextureStorage::texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
{
	texture_set_data(p_texture, p_image, p_layer);

	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	GLES3::Utilities::texture_resize_data(tex->tex_id, tex->total_data_size);

#ifdef TOOLS_ENABLED
	tex->image_cache_2d.unref();
#endif
}

void GLES3::TextureStorage::texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(tex->type != Texture::TYPE_3D);

	Image::Image3DValidateError verr = Image::validate_3d_image(
		tex->format, tex->width, tex->height, tex->depth, tex->mipmaps > 1, p_data);
	ERR_FAIL_COND_MSG(
		verr != Image::VALIDATE_3D_OK, Image::get_3d_image_validation_error_text(verr));

	_texture_set_3d_data(p_texture, p_data, false);

	GLES3::Utilities::texture_resize_data(tex->tex_id, tex->total_data_size);
}

void GLES3::TextureStorage::texture_external_update(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);

	tex->alloc_width = tex->width = p_width;
	tex->alloc_height = tex->height = p_height;

#ifdef ANDROID_ENABLED
	if (tex->target == _GL_TEXTURE_EXTERNAL_OES && p_external_buffer) {
		glBindTexture(_GL_TEXTURE_EXTERNAL_OES, tex->tex_id);
		GLES3::Config::eglEGLImageTargetTexture2DOES(
			_GL_TEXTURE_EXTERNAL_OES, reinterpret_cast<void*>(p_external_buffer));
		glBindTexture(_GL_TEXTURE_EXTERNAL_OES, 0);
	}
#endif
}

void GLES3::TextureStorage::texture_proxy_update(RID p_texture, RID p_proxy_to)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(!tex->is_proxy);
	Texture* proxy_to = texture_owner.get_or_null(p_proxy_to);
	ERR_FAIL_NULL(proxy_to);
	ERR_FAIL_COND(proxy_to->is_proxy);

	if (tex->proxy_to.is_valid()) {
		Texture* prev_tex = texture_owner.get_or_null(tex->proxy_to);
		ERR_FAIL_NULL(prev_tex);
		prev_tex->proxies.erase(p_texture);
	}

	*tex = *proxy_to;

	tex->proxy_to = p_proxy_to;
	tex->is_render_target = false;
	tex->is_proxy = true;
	tex->proxies.clear();
	tex->canvas_texture = nullptr;
	tex->tex_id = 0;
	proxy_to->proxies.push_back(p_texture);
}

void GLES3::TextureStorage::texture_remap_proxies(RID p_from_texture, RID p_to_texture)
{
	Texture* from_tex = texture_owner.get_or_null(p_from_texture);
	ERR_FAIL_NULL(from_tex);
	ERR_FAIL_COND(from_tex->is_proxy);
	Texture* to_tex = texture_owner.get_or_null(p_to_texture);
	ERR_FAIL_NULL(to_tex);
	ERR_FAIL_COND(to_tex->is_proxy);

	if (from_tex == to_tex) {
		return;
	}

	// Make a local copy, we're about to change the content of the original.
	thread_local LocalVector<RID> proxies;
	proxies.clear();
	for (int i = 0; i < from_tex->proxies.size(); i++) {
		proxies.push_back(from_tex->proxies[i]);
	}

	// Now change them to our new texture.
	for (RID& proxy : proxies) {
		texture_proxy_update(proxy, p_to_texture);
	}
}

void GLES3::TextureStorage::texture_2d_placeholder_initialize(RID p_texture)
{
	texture_2d_initialize(p_texture, texture_2d_placeholder);
}

void GLES3::TextureStorage::texture_2d_layered_placeholder_initialize(
	RID p_texture, RSE::TextureLayeredType p_layered_type)
{
	if (p_layered_type == RSE::TEXTURE_LAYERED_2D_ARRAY) {
		texture_2d_layered_initialize(p_texture, texture_2d_array_placeholder, p_layered_type);
	}
	else {
		texture_2d_layered_initialize(p_texture, cubemap_placeholder, p_layered_type);
	}
}

void GLES3::TextureStorage::texture_3d_placeholder_initialize(RID p_texture)
{
	texture_3d_initialize(p_texture, Image::FORMAT_RGBA8, 4, 4, 4, false, texture_3d_placeholder);
}

Ref<Image> GLES3::TextureStorage::texture_2d_get(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, Ref<Image>());

#ifdef TOOLS_ENABLED
	if (texture->image_cache_2d.is_valid() && !texture->is_render_target) {
		return texture->image_cache_2d;
	}
#endif

	Ref<Image> image;
#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		// OpenGL 3.3 supports glGetTexImage which is faster and simpler than glReadPixels.
		// It also allows for reading compressed textures, mipmaps, and more formats.
		Vector<uint8_t> data;

		int64_t data_size = Image::get_image_data_size(texture->alloc_width, texture->alloc_height,
			texture->real_format, texture->mipmaps > 1);

		data.resize(data_size * 2); // Add some memory at the end, just in case for buggy drivers.
		uint8_t* w = data.ptrw();

		glActiveTexture(GL_TEXTURE0);

		glBindTexture(texture->target, texture->tex_id);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		for (int i = 0; i < texture->mipmaps; i++) {
			int64_t ofs = Image::get_image_mipmap_offset(
				texture->alloc_width, texture->alloc_height, texture->real_format, i);

			if (texture->compressed) {
				glPixelStorei(GL_PACK_ALIGNMENT, 4);
				glGetCompressedTexImage(texture->target, i, &w[ofs]);
			}
			else {
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glGetTexImage(
					texture->target, i, texture->gl_format_cache, texture->gl_type_cache, &w[ofs]);
			}
		}

		data.resize(data_size);

		ERR_FAIL_COND_V(data.is_empty(), Ref<Image>());
		image = Image::create_from_data(texture->alloc_width, texture->alloc_height,
			texture->mipmaps > 1, texture->real_format, data);
		if (image->is_empty()) {
			const String& path_str = texture->path.is_empty()
										 ? "with no path"
										 : vformat("with path '%s'", texture->path);
			ERR_FAIL_V_MSG(Ref<Image>(), vformat("Texture %s has no data.", path_str));
		}

		if (texture->format != texture->real_format &&
			!Image::is_format_compressed(texture->real_format)) {
			image->convert(texture->format);
		}
	}
#endif // GL_API_ENABLED
#ifdef GLES_API_ENABLED
	if (!RasterizerUtilGLES3::is_gles_over_gl()) {
		Vector<uint8_t> data;

		// On web and mobile we always read an RGBA8 image with no mipmaps.
		int64_t data_size = Image::get_image_data_size(
			texture->alloc_width, texture->alloc_height, Image::FORMAT_RGBA8, false);

		data.resize(data_size * 2); // Add some memory at the end, just in case for buggy drivers.
		uint8_t* w = data.ptrw();

		GLuint temp_framebuffer;
		glGenFramebuffers(1, &temp_framebuffer);

		GLuint temp_color_texture;
		glGenTextures(1, &temp_color_texture);

		glBindFramebuffer(GL_FRAMEBUFFER, temp_framebuffer);

		glBindTexture(GL_TEXTURE_2D, temp_color_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->alloc_width, texture->alloc_height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_color_texture, 0);

		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthFunc(GL_GEQUAL);
		glColorMask(1, 1, 1, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->tex_id);

		glViewport(0, 0, texture->alloc_width, texture->alloc_height);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		CopyEffects::copy_to_rect(Rect2i(0, 0, 1.0, 1.0));

		glReadPixels(
			0, 0, texture->alloc_width, texture->alloc_height, GL_RGBA, GL_UNSIGNED_BYTE, &w[0]);

		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
		glDeleteTextures(1, &temp_color_texture);
		glDeleteFramebuffers(1, &temp_framebuffer);

		data.resize(data_size);

		ERR_FAIL_COND_V(data.is_empty(), Ref<Image>());
		image = Image::create_from_data(
			texture->alloc_width, texture->alloc_height, false, Image::FORMAT_RGBA8, data);
		if (image->is_empty()) {
			const String& path_str = texture->path.is_empty()
										 ? "with no path"
										 : vformat("with path '%s'", texture->path);
			ERR_FAIL_V_MSG(Ref<Image>(), vformat("Texture %s has no data.", path_str));
		}

		if (texture->format != Image::FORMAT_RGBA8 &&
			!Image::is_format_compressed(texture->format)) {
			image->convert(texture->format);
		}

		if (texture->mipmaps > 1) {
			image->generate_mipmaps();
		}
	}
#endif // GLES_API_ENABLED

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() && !texture->is_render_target) {
		texture->image_cache_2d = image;
	}
#endif

	return image;
}

Ref<Image> GLES3::TextureStorage::texture_2d_layer_get(RID p_texture, int p_layer)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, Ref<Image>());

	Vector<uint8_t> data;

	int64_t data_size = Image::get_image_data_size(
		texture->alloc_width, texture->alloc_height, Image::FORMAT_RGBA8, false);

	data.resize(data_size * 2); // add some memory at the end, just in case for buggy drivers
	uint8_t* w = data.ptrw();

	GLuint temp_framebuffer;
	glGenFramebuffers(1, &temp_framebuffer);

	GLuint temp_color_texture;
	glGenTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, temp_framebuffer);

	glBindTexture(GL_TEXTURE_2D, temp_color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->alloc_width, texture->alloc_height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_color_texture, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->tex_id);

	glViewport(0, 0, texture->alloc_width, texture->alloc_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	CopyEffects::copy_to_rect_3d(Rect2i(0, 0, 1, 1), p_layer, Texture::TYPE_LAYERED);

	glReadPixels(
		0, 0, texture->alloc_width, texture->alloc_height, GL_RGBA, GL_UNSIGNED_BYTE, &w[0]);

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDeleteTextures(1, &temp_color_texture);
	glDeleteFramebuffers(1, &temp_framebuffer);

	data.resize(data_size);

	ERR_FAIL_COND_V(data.is_empty(), Ref<Image>());
	Ref<Image> image =
		Image::create_from_data(texture->width, texture->height, false, Image::FORMAT_RGBA8, data);
	if (image->is_empty()) {
		const String& path_str =
			texture->path.is_empty() ? "with no path" : vformat("with path '%s'", texture->path);
		ERR_FAIL_V_MSG(Ref<Image>(), vformat("Texture %s has no data.", path_str));
	}

	if (texture->format != Image::FORMAT_RGBA8 && !Image::is_format_compressed(texture->format)) {
		image->convert(texture->format);
	}

	if (texture->mipmaps > 1) {
		image->generate_mipmaps();
	}

	return image;
}

Vector<Ref<Image>> GLES3::TextureStorage::_texture_3d_read_framebuffer(GLES3::Texture* p_texture)
{
	ERR_FAIL_NULL_V(p_texture, Vector<Ref<Image>>());

	Vector<Ref<Image>> ret;
	Vector<uint8_t> data;

	int width = p_texture->width;
	int height = p_texture->height;
	int depth = p_texture->depth;

	for (int mipmap_level = 0; mipmap_level < p_texture->mipmaps; mipmap_level++) {
		int64_t data_size = Image::get_image_data_size(width, height, Image::FORMAT_RGBA8, false);
		glViewport(0, 0, width, height);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		for (int layer = 0; layer < depth; layer++) {
			data.resize(data_size * 2); // add some memory at the end, just in case for buggy
										// drivers
			uint8_t* w = data.ptrw();

			float layer_f = layer / float(depth);
			CopyEffects::copy_to_rect_3d(Rect2i(0, 0, 1, 1), layer_f, Texture::TYPE_3D, mipmap_level);
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &w[0]);

			data.resize(data_size);
			ERR_FAIL_COND_V(data.is_empty(), Vector<Ref<Image>>());

			Ref<Image> img =
				Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, data);
			ERR_FAIL_COND_V(img->is_empty(), Vector<Ref<Image>>());

			if (p_texture->format != Image::FORMAT_RGBA8 &&
				!Image::is_format_compressed(p_texture->format)) {
				img->convert(p_texture->format);
			}

			ret.push_back(img);
		}

		width = MAX(1, width >> 1);
		height = MAX(1, height >> 1);
		depth = MAX(1, depth >> 1);
	}

	return ret;
}

Vector<Ref<Image>> GLES3::TextureStorage::texture_3d_get(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, Vector<Ref<Image>>());
	ERR_FAIL_COND_V(texture->type != Texture::TYPE_3D, Vector<Ref<Image>>());

#ifdef TOOLS_ENABLED
	if (!texture->image_cache_3d.is_empty() && !texture->is_render_target) {
		return texture->image_cache_3d;
	}
#endif

	GLuint temp_framebuffer;
	glGenFramebuffers(1, &temp_framebuffer);

	GLuint temp_color_texture;
	glGenTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, temp_framebuffer);

	glBindTexture(GL_TEXTURE_2D, temp_color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->alloc_width, texture->alloc_height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_color_texture, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture->tex_id);

	Vector<Ref<Image>> ret = _texture_3d_read_framebuffer(texture);

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDeleteTextures(1, &temp_color_texture);
	glDeleteFramebuffers(1, &temp_framebuffer);

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() && !texture->is_render_target) {
		texture->image_cache_3d = ret;
	}
#endif

	return ret;
}

void GLES3::TextureStorage::texture_drawable_generate_mipmaps(RID p_texture)
{
	Texture* texture = get_texture(p_texture);
	Vector3i size = texture_get_size(p_texture);
	CopyEffects::bilinear_blur(texture->tex_id, texture->mipmaps, Rect2i(0, 0, size.x, size.y));
}

RID GLES3::TextureStorage::texture_drawable_get_default_material()
{
	// This should never be called before DrawableTexture stuff is initialized.
	return tex_blit_shader.default_material;
}

void GLES3::TextureStorage::texture_replace(RID p_texture, RID p_by_texture)
{
	Texture* tex_to = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex_to);
	ERR_FAIL_COND(tex_to->is_proxy); // can't replace proxy
	Texture* tex_from = texture_owner.get_or_null(p_by_texture);
	ERR_FAIL_NULL(tex_from);
	ERR_FAIL_COND(tex_from->is_proxy); // can't replace proxy

	if (tex_to == tex_from) {
		return;
	}

	if (tex_to->canvas_texture) {
		memdelete(tex_to->canvas_texture);
		tex_to->canvas_texture = nullptr;
	}

	if (tex_to->tex_id) {
		GLES3::Utilities::texture_free_data(tex_to->tex_id);
		tex_to->tex_id = 0;
	}

	Vector<RID> proxies_to_update = Vector<RID>(tex_to->proxies);
	Vector<RID> proxies_to_redirect = Vector<RID>(tex_from->proxies);

	*tex_to = *tex_from;

	tex_to->proxies = proxies_to_update; // restore proxies, so they can be updated

	if (tex_to->canvas_texture) {
		tex_to->canvas_texture->diffuse = p_texture; // update
	}

	for (int i = 0; i < proxies_to_update.size(); i++) {
		texture_proxy_update(proxies_to_update[i], p_texture);
	}
	for (int i = 0; i < proxies_to_redirect.size(); i++) {
		texture_proxy_update(proxies_to_redirect[i], p_texture);
	}
	// delete last, so proxies can be updated
	texture_owner.free(p_by_texture);

	texture_atlas_mark_dirty_on_texture(p_texture);
}

void GLES3::TextureStorage::texture_set_size_override(RID p_texture, int p_width, int p_height)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL(texture);
	ERR_FAIL_COND(texture->is_render_target);

	ERR_FAIL_COND(p_width <= 0 || p_width > 16384);
	ERR_FAIL_COND(p_height <= 0 || p_height > 16384);
	// real texture size is in alloc width and height
	texture->width = p_width;
	texture->height = p_height;
}

void GLES3::TextureStorage::texture_set_path(RID p_texture, const String& p_path)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->path = p_path;
}

String GLES3::TextureStorage::texture_get_path(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, "");

	return texture->path;
}

void GLES3::TextureStorage::texture_set_detect_3d_callback(
	RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->detect_3d_callback = p_callback;
	texture->detect_3d_callback_ud = p_userdata;
}

void GLES3::TextureStorage::texture_set_detect_srgb_callback(
	RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata)
{
}

void GLES3::TextureStorage::texture_set_detect_normal_callback(
	RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->detect_normal_callback = p_callback;
	texture->detect_normal_callback_ud = p_userdata;
}

void GLES3::TextureStorage::texture_set_detect_roughness_callback(RID p_texture,
	RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->detect_roughness_callback = p_callback;
	texture->detect_roughness_callback_ud = p_userdata;
}

void GLES3::TextureStorage::texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info)
{
	for (const RID& rid : texture_owner.get_owned_list()) {
		Texture* t = texture_owner.get_or_null(rid);
		if (!t) {
			continue;
		}
		RenderingServerTypes::TextureInfo tinfo;
		tinfo.path = t->path;
		tinfo.format = t->format;
		tinfo.width = t->alloc_width;
		tinfo.height = t->alloc_height;
		tinfo.bytes = t->total_data_size;
		tinfo.type = static_cast<RSE::TextureType>(t->type);

		switch (t->type) {
		case Texture::TYPE_3D:
			tinfo.depth = t->depth;
			break;

		case Texture::TYPE_LAYERED:
			tinfo.depth = t->layers;
			break;

		default:
			tinfo.depth = 0;
			break;
		}

		r_info->push_back(tinfo);
	}
}

void GLES3::TextureStorage::texture_set_force_redraw_if_visible(RID p_texture, bool p_enable)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->redraw_if_visible = p_enable;
}

Size2 GLES3::TextureStorage::texture_size_with_proxy(RID p_texture)
{
	const Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, Size2());
	if (texture->is_proxy) {
		const Texture* proxy = texture_owner.get_or_null(texture->proxy_to);
		return Size2(proxy->width, proxy->height);
	}
	else {
		return Size2(texture->width, texture->height);
	}
}

void GLES3::TextureStorage::texture_rd_initialize(
	RID p_texture, const RID& p_rd_texture, const RSE::TextureLayeredType p_layer_type)
{
}

RID GLES3::TextureStorage::texture_get_rd_texture(RID p_texture, bool p_srgb) { return RID(); }

uint64_t GLES3::TextureStorage::texture_get_native_handle(RID p_texture, bool p_srgb)
{
	const Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, 0);

	return texture->tex_id;
}

void GLES3::TextureStorage::texture_set_data(RID p_texture, const Ref<Image>& p_image, int p_layer)
{
	_texture_set_data(p_texture, p_image, p_layer, false);
}

void GLES3::TextureStorage::_texture_set_data(
	RID p_texture, const Ref<Image>& p_image, int p_layer, bool p_initialize)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL(texture);
	if (texture->target == GL_TEXTURE_3D) {
		// Target is set to a 3D texture or array texture, exit early to avoid spamming errors
		return;
	}
	ERR_FAIL_COND(!texture->active);
	ERR_FAIL_COND(texture->is_render_target);
	ERR_FAIL_COND(p_image.is_null());
	ERR_FAIL_COND(texture->format != p_image->get_format());

	ERR_FAIL_COND(!p_image->get_width());
	ERR_FAIL_COND(!p_image->get_height());

	GLenum type;
	GLenum format;
	GLenum internal_format;
	bool compressed = false;

	bool needs_decompress = texture->resize_to_po2;

	// Support for RGTC-compressed Texture Arrays isn't mandated by GLES3/WebGL.
	if (!RasterizerUtilGLES3::is_gles_over_gl() && texture->target == GL_TEXTURE_2D_ARRAY) {
		if (p_image->get_format() == Image::FORMAT_RGTC_R ||
			p_image->get_format() == Image::FORMAT_RGTC_RG) {
			needs_decompress = true;
		}
	}

	Image::Format real_format;
	Ref<Image> img = _get_gl_image_and_format(p_image, p_image->get_format(), real_format, format,
		internal_format, type, compressed, needs_decompress);
	ERR_FAIL_COND(img.is_null());
	if (texture->resize_to_po2) {
		if (p_image->is_compressed()) {
			ERR_PRINT("Texture '" + texture->path +
					  "' is required to be a power of 2 because it uses either mipmaps or repeat, "
					  "so it was decompressed. This will hurt performance and memory usage.");
		}
		img->resize_to_po2(false);
	}

	GLenum blit_target =
		(texture->target == GL_TEXTURE_CUBE_MAP) ? _cube_side_enum[p_layer] : texture->target;

	Vector<uint8_t> read = img->get_data();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);
	_texture_set_swizzle(texture, real_format);

	int mipmaps = img->has_mipmaps() ? img->get_mipmap_count() + 1 : 1;

	// Set filtering and repeat state to default.
	if (mipmaps > 1) {
		texture->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS);
	}
	else {
		texture->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	}

	texture->gl_set_repeat(RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED);

	int w = img->get_width();
	int h = img->get_height();

	int tsize = 0;

	for (int i = 0; i < mipmaps; i++) {
		int64_t size, ofs;
		img->get_mipmap_offset_and_size(i, ofs, size);
		if (compressed) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			if (texture->target == GL_TEXTURE_2D_ARRAY) {
				if (p_initialize) {
					glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, w, h,
						texture->layers, 0, size * texture->layers, nullptr);
				}
				glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, p_layer, w, h, 1,
					internal_format, size, &read[ofs]);
			}
			else {
				glCompressedTexImage2D(blit_target, i, internal_format, w, h, 0, size, &read[ofs]);
			}
		}
		else {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (texture->target == GL_TEXTURE_2D_ARRAY) {
				if (p_initialize) {
					glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, w, h, texture->layers, 0,
						format, type, nullptr);
				}
				glTexSubImage3D(
					GL_TEXTURE_2D_ARRAY, i, 0, 0, p_layer, w, h, 1, format, type, &read[ofs]);
			}
			else {
				glTexImage2D(blit_target, i, internal_format, w, h, 0, format, type, &read[ofs]);
			}
		}

		tsize += size;

		w = MAX(1, w >> 1);
		h = MAX(1, h >> 1);
	}

	if (texture->target == GL_TEXTURE_CUBE_MAP || texture->target == GL_TEXTURE_2D_ARRAY) {
		texture->total_data_size = tsize * texture->layers;
	}
	else {
		texture->total_data_size = tsize;
	}

	texture->stored_cube_sides |= (1 << p_layer);

	texture->mipmaps = mipmaps;
}

void GLES3::TextureStorage::_texture_set_3d_data(
	RID p_texture, const Vector<Ref<Image>>& p_data, bool p_initialize)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL(texture);
	ERR_FAIL_COND(!texture->active);
	ERR_FAIL_COND(texture->is_render_target);
	ERR_FAIL_COND(texture->target != GL_TEXTURE_3D);
	ERR_FAIL_COND(p_data.is_empty());

	GLenum type;
	GLenum format;
	GLenum internal_format;
	bool compressed = false;

	Image::Format real_format;
	Ref<Image> img = _get_gl_image_and_format(p_data[0], p_data[0]->get_format(), real_format,
		format, internal_format, type, compressed, texture->resize_to_po2);
	ERR_FAIL_COND(img.is_null());

	ERR_FAIL_COND_MSG(
		compressed, "Compressed 3D textures are not supported in the Compatibility renderer.");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);
	_texture_set_swizzle(texture, texture->real_format);

	// Set filtering and repeat state to default.
	if (texture->mipmaps > 1) {
		texture->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS);
	}
	else {
		texture->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	}

	texture->gl_set_repeat(RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED);

	Vector<Ref<Image>> images;
	images.resize(p_data.size());
	for (int i = 0; i < p_data.size(); i++) {
		Ref<Image> image = p_data[i];
		if (image->get_format() != texture->format) {
			image->convert(texture->format);
		}
		images.write[i] = image;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	int all_data_size = 0;
	int mipmap_level = 0;
	int layer = 0;
	int depth = texture->depth;
	Size2i prev_size(images[0]->get_width(), images[0]->get_height());
	for (int i = 0; i < images.size(); i++) {
		Ref<Image> image = images[i];
		Size2i img_size(image->get_width(), image->get_height());

		if (img_size != prev_size) {
			mipmap_level++;
			depth = MAX(1, depth >> 1);
			layer = 0;
		}
		prev_size = img_size;
		all_data_size += image->get_data().size();

		if (layer == 0 && p_initialize) {
			glTexImage3D(GL_TEXTURE_3D, mipmap_level, internal_format, img_size.width,
				img_size.height, depth, 0, format, type, nullptr);
		}

		glTexSubImage3D(GL_TEXTURE_3D, mipmap_level, 0, 0, layer, img_size.width, img_size.height,
			1, format, type, image->get_data().ptr());

		layer++;
	}

	texture->total_data_size = all_data_size;
	texture->mipmaps = mipmap_level + 1;

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() && !texture->is_render_target) {
		texture->image_cache_3d = images;
	}
#endif
}

void GLES3::TextureStorage::_texture_set_swizzle(
	GLES3::Texture* p_texture, Image::Format p_real_format)
{
#ifndef WEB_ENABLED
	switch (p_texture->format) {
	case Image::FORMAT_L8: {
		if (RasterizerUtilGLES3::is_gles_over_gl()) {
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
		}
		else {
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
		}
	} break;
	case Image::FORMAT_LA8: {
		if (RasterizerUtilGLES3::is_gles_over_gl()) {
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
		}
		else {
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
		}
	} break;
	case Image::FORMAT_ETC2_RA_AS_RG:
	case Image::FORMAT_DXT5_RA_AS_RG: {
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		if (p_texture->format == p_real_format) {
			// Swizzle RA from compressed texture into RG.
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_ALPHA);
		}
		else {
			// Converted textures are already in RG, leave as-is.
			glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		}
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
	} break;
	default: {
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		glTexParameteri(p_texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
	} break;
	}
#endif // WEB_ENABLED
}

Image::Format GLES3::TextureStorage::texture_get_format(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, Image::FORMAT_L8);

	return texture->format;
}

uint32_t GLES3::TextureStorage::texture_get_texid(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, 0);

	return texture->tex_id;
}

Vector3i GLES3::TextureStorage::texture_get_size(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, Vector3i(0, 0, 0));

	return Vector3i(texture->width, texture->height, texture->depth);
}

int GLES3::TextureStorage::texture_get_width(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, 0);

	return texture->width;
}

int GLES3::TextureStorage::texture_get_height(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, 0);

	return texture->height;
}

int GLES3::TextureStorage::texture_get_depth(RID p_texture)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL_V(texture, 0);

	return texture->depth;
}

void GLES3::TextureStorage::texture_bind(RID p_texture, uint32_t p_texture_no)
{
	Texture* texture = texture_owner.get_or_null(p_texture);

	ERR_FAIL_NULL(texture);

	glActiveTexture(GL_TEXTURE0 + p_texture_no);
	glBindTexture(texture->target, texture->tex_id);
}

/* TEXTURE ATLAS API */

void GLES3::TextureStorage::texture_add_to_texture_atlas(RID p_texture)
{
	if (!texture_atlas.textures.has(p_texture)) {
		TextureAtlas::Texture t;
		t.users = 1;
		texture_atlas.textures[p_texture] = t;
		texture_atlas.dirty = true;
	}
	else {
		TextureAtlas::Texture* t = texture_atlas.textures.getptr(p_texture);
		t->users++;
	}
}

void GLES3::TextureStorage::texture_remove_from_texture_atlas(RID p_texture)
{
	TextureAtlas::Texture* t = texture_atlas.textures.getptr(p_texture);
	ERR_FAIL_NULL(t);
	t->users--;
	if (t->users == 0) {
		texture_atlas.textures.erase(p_texture);
		// Do not mark it dirty, there is no need to since it remains working.
	}
}

void GLES3::TextureStorage::texture_atlas_mark_dirty_on_texture(RID p_texture)
{
	if (texture_atlas.textures.has(p_texture)) {
		texture_atlas.dirty = true; // Mark it dirty since it was most likely modified.
	}
}

void GLES3::TextureStorage::texture_atlas_remove_texture(RID p_texture)
{
	if (texture_atlas.textures.has(p_texture)) {
		texture_atlas.textures.erase(p_texture);
		// There is not much a point of making it dirty, texture can be removed next time the atlas
		// is updated.
	}
}

GLuint GLES3::TextureStorage::texture_atlas_get_texture() { return texture_atlas.texture; }

void GLES3::TextureStorage::update_texture_atlas()
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	if (!texture_atlas.dirty) {
		return; // nothing to do
	}

	texture_atlas.dirty = false;

	if (texture_atlas.texture != 0) {
		GLES3::Utilities::texture_free_data(texture_atlas.texture);
		texture_atlas.texture = 0;
		glDeleteFramebuffers(1, &texture_atlas.framebuffer);
		texture_atlas.framebuffer = 0;
	}

	const int border = 2;

	if (texture_atlas.textures.size()) {
		// generate atlas
		Vector<TextureAtlas::SortItem> itemsv;
		itemsv.resize(texture_atlas.textures.size());
		uint32_t base_size = 8;

		int idx = 0;

		for (const KeyValue<RID, TextureAtlas::Texture>& E : texture_atlas.textures) {
			TextureAtlas::SortItem& si = itemsv.write[idx];

			Texture* src_tex = get_texture(E.key);

			si.size.width = (src_tex->width / border) + 1;
			si.size.height = (src_tex->height / border) + 1;
			si.pixel_size = Size2i(src_tex->width, src_tex->height);

			if (base_size < (uint32_t)si.size.width) {
				base_size = Math::nearest_power_of_2_templated(si.size.width);
			}

			si.texture = E.key;
			idx++;
		}

		// sort items by size
		itemsv.sort();

		// attempt to create atlas
		int item_count = itemsv.size();
		TextureAtlas::SortItem* items = itemsv.ptrw();


	int atlas_height = 0;

		while (true) {
			Vector<int> v_offsetsv;
			v_offsetsv.resize(base_size);

			int* v_offsets = v_offsetsv.ptrw();
			memset(v_offsets, 0, sizeof(int) * base_size);

			int max_height = 0;

			for (int i = 0; i < item_count; i++) {
				// best fit
				TextureAtlas::SortItem& si = items[i];
				int best_idx = -1;
				int best_height = 0x7FFFFFFF;
				for (uint32_t j = 0; j <= base_size - si.size.width; j++) {
					int height = 0;
					for (int k = 0; k < si.size.width; k++) {
						int h = v_offsets[k + j];
						if (h > height) {
							height = h;
							if (height > best_height) {
								break; // already bad
							}
						}
					}

					if (height < best_height) {
						best_height = height;
						best_idx = j;
					}
				}

				// update
				for (int k = 0; k < si.size.width; k++) {
					v_offsets[k + best_idx] = best_height + si.size.height;
				}

				si.pos.x = best_idx;
				si.pos.y = best_height;

				if (si.pos.y + si.size.height > max_height) {
					max_height = si.pos.y + si.size.height;
				}
			}

			if ((uint32_t)max_height <= base_size * 2) {
				atlas_height = max_height;
				break; // good ratio, break;
			}

			base_size *= 2;
		}

		texture_atlas.size.width = base_size * border;
		texture_atlas.size.height = Math::nearest_power_of_2_templated(atlas_height * border);

		for (int i = 0; i < item_count; i++) {
			TextureAtlas::Texture* t = texture_atlas.textures.getptr(items[i].texture);
			t->uv_rect.position = items[i].pos * border + Vector2i(border / 2, border / 2);
			t->uv_rect.size = items[i].pixel_size;

			t->uv_rect.position /= Size2(texture_atlas.size);
			t->uv_rect.size /= Size2(texture_atlas.size);
		}
	}
	else {
		texture_atlas.size.width = 4;
		texture_atlas.size.height = 4;
	}

	{ // Atlas Texture initialize.
		// TODO validate texture atlas size with maximum texture size
		glGenTextures(1, &texture_atlas.texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_atlas.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_atlas.size.width,
			texture_atlas.size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		GLES3::Utilities::texture_allocated_data(texture_atlas.texture,
			texture_atlas.size.width * texture_atlas.size.height * 4, "Texture atlas");

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);

		glGenFramebuffers(1, &texture_atlas.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, texture_atlas.framebuffer);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_atlas.texture, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteFramebuffers(1, &texture_atlas.framebuffer);
			texture_atlas.framebuffer = 0;
			GLES3::Utilities::texture_free_data(texture_atlas.texture);
			texture_atlas.texture = 0;
			WARN_PRINT("Could not create texture atlas, status: " + get_framebuffer_error(status));
			return;
		}
		glViewport(0, 0, texture_atlas.size.width, texture_atlas.size.height);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDisable(GL_BLEND);

	if (texture_atlas.textures.size()) {
		for (const KeyValue<RID, TextureAtlas::Texture>& E : texture_atlas.textures) {
			TextureAtlas::Texture* t = texture_atlas.textures.getptr(E.key);
			Texture* src_tex = get_texture(E.key);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, src_tex->tex_id);
			CopyEffects::copy_to_rect(t->uv_rect);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
}

/* DECAL API */

RID GLES3::TextureStorage::decal_allocate() { return RID(); }

void GLES3::TextureStorage::decal_initialize(RID p_rid) {}

void GLES3::TextureStorage::decal_set_size(RID p_decal, const Vector3& p_size) {}

void GLES3::TextureStorage::decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture)
{
}

void GLES3::TextureStorage::decal_set_emission_energy(RID p_decal, float p_energy) {}

void GLES3::TextureStorage::decal_set_albedo_mix(RID p_decal, float p_mix) {}

void GLES3::TextureStorage::decal_set_modulate(RID p_decal, const Color& p_modulate) {}

void GLES3::TextureStorage::decal_set_cull_mask(RID p_decal, uint32_t p_layers) {}

void GLES3::TextureStorage::decal_set_distance_fade(
	RID p_decal, bool p_enabled, float p_begin, float p_length)
{
}

void GLES3::TextureStorage::decal_set_fade(RID p_decal, float p_above, float p_below) {}

void GLES3::TextureStorage::decal_set_normal_fade(RID p_decal, float p_fade) {}

AABB GLES3::TextureStorage::decal_get_aabb(RID p_decal) { return AABB(); }

/* RENDER TARGET API */

void GLES3::TextureStorage::_update_render_target_color(RenderTarget* rt)
{
	// do not allocate a render target with no size
	if (rt->size.x <= 0 || rt->size.y <= 0) {
		return;
	}

	// do not allocate a render target that is attached to the screen
	if (rt->direct_to_screen) {
		rt->fbo = system_fbo;
		return;
	}

	if (rt->hdr) {
		rt->color_internal_format = GL_RGBA16F;
		rt->color_format = GL_RGBA;
		rt->color_type = GL_FLOAT;
		rt->color_format_size = 8;
		rt->image_format = Image::FORMAT_RGBAF;
	}
	else if (rt->is_transparent) {
		rt->color_internal_format = GL_RGBA8;
		rt->color_format = GL_RGBA;
		rt->color_type = GL_UNSIGNED_BYTE;
		rt->color_format_size = 4;
		rt->image_format = Image::FORMAT_RGBA8;
	}
	else {
		rt->color_internal_format = GL_RGB10_A2;
		rt->color_format = GL_RGBA;
		rt->color_type = GL_UNSIGNED_INT_2_10_10_10_REV;
		rt->color_format_size = 4;
		rt->image_format = Image::FORMAT_RGBA8;
	}

	glDisable(GL_SCISSOR_TEST);
	glColorMask(1, 1, 1, 1);
	glDepthMask(GL_FALSE);

	{
		Texture* texture;
		bool use_multiview = rt->view_count > 1 && Config::multiview_supported;
		GLenum texture_target = use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

		/* Front FBO */

		glGenFramebuffers(1, &rt->fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);

		// color
		if (rt->overridden.color.is_valid()) {
			texture = get_texture(rt->overridden.color);
			ERR_FAIL_NULL(texture);

			rt->color = texture->tex_id;
			rt->size = Size2i(texture->width, texture->height);
		}
		else {
			texture = get_texture(rt->texture);
			ERR_FAIL_NULL(texture);

			glGenTextures(1, &rt->color);
			glBindTexture(texture_target, rt->color);

			if (use_multiview) {
				glTexImage3D(texture_target, 0, rt->color_internal_format, rt->size.x, rt->size.y,
					rt->view_count, 0, rt->color_format, rt->color_type, nullptr);
			}
			else {
				glTexImage2D(texture_target, 0, rt->color_internal_format, rt->size.x, rt->size.y,
					0, rt->color_format, rt->color_type, nullptr);
			}

			texture->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
			texture->gl_set_repeat(RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);

			GLES3::Utilities::texture_allocated_data(rt->color,
				rt->size.x * rt->size.y * rt->view_count * rt->color_format_size,
				"Render target color texture");
		}
#ifndef IOS_ENABLED
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rt->color, 0, 0, rt->view_count);
		}
		else {
#else
		{
#endif
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, rt->color, 0);
		}

		// depth
		if (rt->overridden.depth.is_valid()) {
			texture = get_texture(rt->overridden.depth);
			ERR_FAIL_NULL(texture);

			rt->depth = texture->tex_id;
			rt->depth_has_stencil = rt->overridden.depth_has_stencil;
		}
		else {
			glGenTextures(1, &rt->depth);
			glBindTexture(texture_target, rt->depth);

			if (use_multiview) {
				glTexImage3D(texture_target, 0, GL_DEPTH24_STENCIL8, rt->size.x, rt->size.y,
					rt->view_count, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
			}
			else {
				glTexImage2D(texture_target, 0, GL_DEPTH24_STENCIL8, rt->size.x, rt->size.y, 0,
					GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
			}

			glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			rt->depth_has_stencil = true;

			GLES3::Utilities::texture_allocated_data(rt->depth,
				rt->size.x * rt->size.y * rt->view_count * 4, "Render target depth texture");
		}

#ifndef IOS_ENABLED
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER,
				rt->depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
				rt->depth, 0, 0, rt->view_count);
		}
		else {
#else
		{
#endif
			glFramebufferTexture2D(GL_FRAMEBUFFER,
				rt->depth_has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
				texture_target, rt->depth, 0);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteFramebuffers(1, &rt->fbo);
			if (rt->overridden.color.is_null()) {
				GLES3::Utilities::texture_free_data(rt->color);
			}
			if (rt->overridden.depth.is_null()) {
				GLES3::Utilities::texture_free_data(rt->depth);
			}
			rt->fbo = 0;
			rt->size.x = 0;
			rt->size.y = 0;
			rt->color = 0;
			rt->depth = 0;
			if (rt->overridden.color.is_null()) {
				texture->tex_id = 0;
				texture->active = false;
			}
			WARN_PRINT("Could not create render target, status: " + get_framebuffer_error(status));
			return;
		}

		texture->is_render_target = true;
		texture->render_target = rt;
		if (rt->overridden.color.is_null()) {
			texture->format = rt->image_format;
			texture->real_format = rt->image_format;
			texture->target = texture_target;
			if (rt->view_count > 1 && Config::multiview_supported) {
				texture->type = Texture::TYPE_LAYERED;
				texture->layers = rt->view_count;
			}
			else {
				texture->type = Texture::TYPE_2D;
				texture->layers = 1;
			}
			texture->gl_format_cache = rt->color_format;
			texture->gl_type_cache =
				!rt->hdr ? GL_UNSIGNED_BYTE
						 : GL_FLOAT; // to set HDR format size to 8 and keep 4 for LDR format
			texture->gl_internal_format_cache = rt->color_internal_format;
			texture->tex_id = rt->color;
			texture->width = rt->size.x;
			texture->alloc_width = rt->size.x;
			texture->height = rt->size.y;
			texture->alloc_height = rt->size.y;
			texture->active = true;
		}
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
}

void GLES3::TextureStorage::_update_render_target_velocity(RenderTarget* rt)
{
	GLuint new_velocity_fbo;
	glGenFramebuffers(1, &new_velocity_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, new_velocity_fbo);

	uint32_t view_count = rt->view_count;
	GLuint texture_target = view_count > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	GLuint velocity_texture_id = texture_get_texid(rt->overridden.velocity);
	glBindTexture(texture_target, velocity_texture_id);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifndef IOS_ENABLED
	if (view_count > 1) {
		glFramebufferTextureMultiviewOVR(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, velocity_texture_id, 0, 0, view_count);
	}
	else {
#else
	{
#endif
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocity_texture_id, 0);
	}

	GLuint velocity_depth_texture_id = texture_get_texid(rt->overridden.velocity_depth);
	glBindTexture(texture_target, velocity_depth_texture_id);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifndef IOS_ENABLED
	if (view_count > 1) {
		glFramebufferTextureMultiviewOVR(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, velocity_depth_texture_id, 0, 0, view_count);
	}
	else {
#else
	{
#endif
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, velocity_depth_texture_id, 0);
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		glDeleteFramebuffers(1, &new_velocity_fbo);
		WARN_PRINT(vformat("Could not create motion vector render target, status: %s.",
			GLES3::TextureStorage::get_framebuffer_error(status)));
	}
	else {
		rt->overridden.velocity_fbo = new_velocity_fbo;
	}

	glBindTexture(texture_target, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLES3::TextureStorage::_create_render_target_backbuffer(RenderTarget* rt)
{
	ERR_FAIL_COND_MSG(
		rt->backbuffer_fbo != 0, "Cannot allocate RenderTarget backbuffer: already initialized.");
	ERR_FAIL_COND_MSG(rt->size.x <= 0 || rt->size.y <= 0,
		"Cannot allocate RenderTarget backbuffer: invalid size.");
	ERR_FAIL_COND(rt->direct_to_screen);

	// Allocate mipmap chains for full screen blur
	// Limit mipmaps so smallest is 32x32 to avoid unnecessary framebuffer switches
	int count =
		MAX(1, Image::get_image_required_mipmaps(rt->size.x, rt->size.y, Image::FORMAT_RGBA8) - 4);
	GLsizei width = rt->size.x;
	GLsizei height = rt->size.y;

	rt->mipmap_count = count;

	glGenTextures(1, &rt->backbuffer);
	glBindTexture(GL_TEXTURE_2D, rt->backbuffer);
	uint32_t texture_size_bytes = 0;

	for (int l = 0; l < count; l++) {
		texture_size_bytes += width * height * 4;
		glTexImage2D(GL_TEXTURE_2D, l, rt->color_internal_format, width, height, 0,
			rt->color_format, rt->color_type, nullptr);
		width = MAX(1, (width / 2));
		height = MAX(1, (height / 2));
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, count - 1);

	glGenFramebuffers(1, &rt->backbuffer_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->backbuffer, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		WARN_PRINT_ONCE(
			"Cannot allocate RenderTarget backbuffer. Status: " + get_framebuffer_error(status));
		glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
		return;
	}
	GLES3::Utilities::texture_allocated_data(
		rt->backbuffer, texture_size_bytes, "Render target backbuffer color texture");

	// Initialize all levels to clear black.
	for (int j = 0; j < count; j++) {
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->backbuffer, j);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->backbuffer, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void GLES3::TextureStorage::_clear_render_target(RenderTarget* rt)
{
	// there is nothing else to clear when DIRECT_TO_SCREEN is used
	if (rt->direct_to_screen) {
		return;
	}

	for (KeyValue<uint32_t, GLuint>& E : rt->overridden.velocity_fbo_cache) {
		glDeleteFramebuffers(1, &E.value);
	}
	rt->overridden.velocity_fbo_cache.clear();
	rt->overridden.velocity_fbo = 0;

	// Dispose of the cached fbo's and the allocated textures
	for (KeyValue<uint32_t, RenderTarget::RTOverridden::FBOCacheEntry>& E :
		rt->overridden.fbo_cache) {
		glDeleteTextures(E.value.allocated_textures.size(), E.value.allocated_textures.ptr());
		// Don't delete the current FBO, we'll do that a couple lines down.
		if (E.value.fbo != rt->fbo) {
			glDeleteFramebuffers(1, &E.value.fbo);
		}
	}
	rt->overridden.fbo_cache.clear();

	if (rt->fbo) {
		glDeleteFramebuffers(1, &rt->fbo);
		rt->fbo = 0;
	}

	if (rt->overridden.color.is_null()) {
		if (rt->texture.is_valid()) {
			Texture* tex = get_texture(rt->texture);
			tex->alloc_height = 0;
			tex->alloc_width = 0;
			tex->width = 0;
			tex->height = 0;
			tex->active = false;
			tex->render_target = nullptr;
			tex->is_render_target = false;
			tex->gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX);
			tex->gl_set_repeat(RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX);
		}
	}
	else {
		Texture* tex = get_texture(rt->overridden.color);
		tex->render_target = nullptr;
		tex->is_render_target = false;
	}

	if (rt->overridden.color.is_valid()) {
		rt->overridden.color = RID();
	}
	else if (rt->color) {
		GLES3::Utilities::texture_free_data(rt->color);
		if (rt->texture.is_valid()) {
			Texture* tex = get_texture(rt->texture);
			tex->tex_id = 0;
		}
	}
	rt->color = 0;

	if (rt->overridden.depth.is_valid()) {
		rt->overridden.depth = RID();
	}
	else if (rt->depth) {
		GLES3::Utilities::texture_free_data(rt->depth);
	}
	rt->depth = 0;

	rt->overridden.is_overridden = false;

	if (rt->backbuffer_fbo != 0) {
		glDeleteFramebuffers(1, &rt->backbuffer_fbo);
		rt->backbuffer_fbo = 0;
	}
	if (rt->backbuffer != 0) {
		GLES3::Utilities::texture_free_data(rt->backbuffer);
		rt->backbuffer = 0;
	}
	if (rt->backbuffer_depth != 0) {
		GLES3::Utilities::texture_free_data(rt->backbuffer_depth);
		rt->backbuffer_depth = 0;
	}
	_render_target_clear_sdf(rt);
}

RID GLES3::TextureStorage::render_target_create()
{
	RenderTarget render_target;
	render_target.used_in_frame = false;
	render_target.clear_requested = false;

	Texture t;
	t.active = true;
	t.is_render_target = true;

	render_target.texture = texture_owner.make_rid(t);
	RID rt_rid = render_target_owner.make_rid(render_target);
	RenderTarget* rt = render_target_owner.get_or_null(rt_rid);
	Texture* tex = get_texture(rt->texture);
	if (tex) {
		tex->render_target = rt;
	}
	_update_render_target_color(rt);
	return rt_rid;
}

void GLES3::TextureStorage::render_target_free(RID p_rid)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_rid);
	_clear_render_target(rt);

	Texture* t = get_texture(rt->texture);
	if (t) {
		t->is_render_target = false;
		if (rt->overridden.color.is_null()) {
			texture_free(rt->texture);
		}
		// memdelete(t);
	}
	render_target_owner.free(p_rid);
}

void GLES3::TextureStorage::render_target_set_position(RID p_render_target, int p_x, int p_y)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->position = Point2i(p_x, p_y);
}

Point2i GLES3::TextureStorage::render_target_get_position(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Point2i());

	return rt->position;
}

void GLES3::TextureStorage::render_target_set_size(
	RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	if (p_width == rt->size.x && p_height == rt->size.y && p_view_count == rt->view_count) {
		return;
	}
	if (rt->overridden.color.is_valid()) {
		return;
	}

	_clear_render_target(rt);

	rt->size = Size2i(p_width, p_height);
	rt->view_count = p_view_count;

	_update_render_target_color(rt);
}

// TODO: convert to Size2i internally
Size2i GLES3::TextureStorage::render_target_get_size(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Size2i());

	return rt->size;
}

void GLES3::TextureStorage::render_target_set_override(RID p_render_target, RID p_color_texture,
	RID p_depth_texture, RID p_velocity_texture, RID p_velocity_depth_texture)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->direct_to_screen);

	// Remember what our current color output is.
	RID was_color_texture = render_target_get_texture(p_render_target);

	bool create_new_color_fbo = true;
	bool create_new_velocity_fbo = true;

	if (rt->overridden.color == p_color_texture && rt->overridden.depth == p_depth_texture &&
		rt->overridden.velocity == p_velocity_texture &&
		rt->overridden.velocity_depth == p_velocity_depth_texture) {
		return;
	}

	if (p_color_texture.is_null() && p_depth_texture.is_null()) {
		// Set this back to our default textures.
		if (was_color_texture.is_valid()) {
			texture_remap_proxies(was_color_texture, rt->texture);
		}

		_clear_render_target(rt);
		_update_render_target_color(rt);
		create_new_color_fbo = false;
	}

	if (!rt->overridden.is_overridden) {
		_clear_render_target(rt);
	}

	rt->overridden.color = p_color_texture;
	rt->overridden.depth = p_depth_texture;
	rt->overridden.depth_has_stencil = p_depth_texture.is_null();
	rt->overridden.velocity = p_velocity_texture;
	rt->overridden.velocity_depth = p_velocity_depth_texture;
	rt->overridden.is_overridden = true;

	// Update to our new color output.
	RID new_color_texture = render_target_get_texture(p_render_target);
	if (was_color_texture.is_valid() && new_color_texture.is_valid()) {
		texture_remap_proxies(was_color_texture, new_color_texture);
	}

	uint32_t hash_key = hash_murmur3_one_64(p_color_texture.get_id());
	hash_key = hash_murmur3_one_64(p_depth_texture.get_id(), hash_key);
	hash_key = hash_fmix32(hash_key);

	RBMap<uint32_t, RenderTarget::RTOverridden::FBOCacheEntry>::Element* cache;
	if ((cache = rt->overridden.fbo_cache.find(hash_key)) != nullptr) {
		rt->fbo = cache->get().fbo;
		rt->color = cache->get().color;
		rt->depth = cache->get().depth;
		rt->depth_has_stencil = cache->get().depth_has_stencil;
		rt->size = cache->get().size;
		rt->texture = p_color_texture;
		create_new_color_fbo = false;
	}

	uint32_t velocity_hash_key = hash_murmur3_one_64(p_velocity_texture.get_id());
	velocity_hash_key = hash_murmur3_one_64(p_velocity_depth_texture.get_id(), velocity_hash_key);
	velocity_hash_key = hash_fmix32(velocity_hash_key);

	RBMap<uint32_t, GLuint>::Element* fbo =
		rt->overridden.velocity_fbo_cache.find(velocity_hash_key);
	if (fbo != nullptr) {
		rt->overridden.velocity_fbo = fbo->get();
		create_new_velocity_fbo = false;
	}

	if (p_velocity_texture.is_null()) {
		for (KeyValue<uint32_t, GLuint>& E : rt->overridden.velocity_fbo_cache) {
			glDeleteFramebuffers(1, &E.value);
		}

		rt->overridden.velocity_fbo_cache.clear();
		rt->overridden.velocity_fbo = 0;
		create_new_velocity_fbo = false;
	}

	if (create_new_color_fbo) {
		_update_render_target_color(rt);

		RenderTarget::RTOverridden::FBOCacheEntry new_entry;
		new_entry.fbo = rt->fbo;
		new_entry.color = rt->color;
		new_entry.depth = rt->depth;
		new_entry.size = rt->size;
		// Keep track of any textures we had to allocate because they weren't overridden.
		if (p_color_texture.is_null()) {
			new_entry.allocated_textures.push_back(rt->color);
		}
		if (p_depth_texture.is_null()) {
			new_entry.allocated_textures.push_back(rt->depth);
		}
		rt->overridden.fbo_cache.insert(hash_key, new_entry);
	}

	if (create_new_velocity_fbo) {
		_update_render_target_velocity(rt);
		rt->overridden.velocity_fbo_cache.insert(velocity_hash_key, rt->overridden.velocity_fbo);
	}
}

RID GLES3::TextureStorage::render_target_get_override_color(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.color;
}

RID GLES3::TextureStorage::render_target_get_override_depth(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.depth;
}

RID GLES3::TextureStorage::render_target_get_override_velocity(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.velocity;
}

RID GLES3::TextureStorage::render_target_get_override_velocity_depth(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.velocity_depth;
}

void GLES3::TextureStorage::render_target_set_render_region(
	RID p_render_target, const Rect2i& p_render_region)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->render_region = p_render_region;
}

Rect2i GLES3::TextureStorage::render_target_get_render_region(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Rect2i());

	return rt->render_region;
}

RID GLES3::TextureStorage::render_target_get_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->overridden.color.is_valid()) {
		return rt->overridden.color;
	}

	return rt->texture;
}

void GLES3::TextureStorage::render_target_set_velocity_target_size(
	RID p_render_target, const Size2i& p_target_size)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->velocity_target_size = p_target_size;
}

Size2i GLES3::TextureStorage::render_target_get_velocity_target_size(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Size2i(0, 0));

	return rt->velocity_target_size;
}

void GLES3::TextureStorage::render_target_set_transparent(RID p_render_target, bool p_transparent)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->is_transparent = p_transparent;

	if (rt->overridden.color.is_null()) {
		_clear_render_target(rt);
		_update_render_target_color(rt);
	}
}

bool GLES3::TextureStorage::render_target_get_transparent(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->is_transparent;
}

void GLES3::TextureStorage::render_target_set_direct_to_screen(
	RID p_render_target, bool p_direct_to_screen)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	if (p_direct_to_screen == rt->direct_to_screen) {
		return;
	}
	// When setting DIRECT_TO_SCREEN, you need to clear before the value is set, but allocate after
	// as those functions change how they operate depending on the value of DIRECT_TO_SCREEN
	_clear_render_target(rt);
	rt->direct_to_screen = p_direct_to_screen;
	if (rt->direct_to_screen) {
		rt->overridden.color = RID();
		rt->overridden.depth = RID();
		rt->overridden.velocity = RID();
		rt->overridden.velocity_depth = RID();
	}
	_update_render_target_color(rt);
}

bool GLES3::TextureStorage::render_target_get_direct_to_screen(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->direct_to_screen;
}

bool GLES3::TextureStorage::render_target_was_used(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->used_in_frame;
}

void GLES3::TextureStorage::render_target_clear_used(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->used_in_frame = false;
}

void GLES3::TextureStorage::render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->direct_to_screen);
	if (p_msaa == rt->msaa) {
		return;
	}

	WARN_PRINT("2D MSAA is not yet supported for GLES3.");

	if (rt->overridden.color.is_null()) {
		_clear_render_target(rt);
		rt->msaa = p_msaa;
		_update_render_target_color(rt);
	}
}

RSE::ViewportMSAA GLES3::TextureStorage::render_target_get_msaa(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_MSAA_DISABLED);

	return rt->msaa;
}

void GLES3::TextureStorage::render_target_set_use_hdr(RID p_render_target, bool p_use_hdr_2d)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->direct_to_screen);
	if (p_use_hdr_2d == rt->hdr) {
		return;
	}

	if (rt->overridden.color.is_null()) {
		_clear_render_target(rt);
		rt->hdr = p_use_hdr_2d;
		_update_render_target_color(rt);
	}
}

bool GLES3::TextureStorage::render_target_is_using_hdr(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->hdr;
}

GLuint GLES3::TextureStorage::render_target_get_color_internal_format(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, GL_RGBA8);

	return rt->color_internal_format;
}

GLuint GLES3::TextureStorage::render_target_get_color_format(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, GL_RGBA);

	return rt->color_format;
}

GLuint GLES3::TextureStorage::render_target_get_color_type(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, GL_UNSIGNED_BYTE);

	return rt->color_type;
}

uint32_t GLES3::TextureStorage::render_target_get_color_format_size(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 4);

	return rt->color_format_size;
}

void GLES3::TextureStorage::render_target_request_clear(
	RID p_render_target, const Color& p_clear_color)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = true;
	rt->clear_color = p_clear_color;
}

bool GLES3::TextureStorage::render_target_is_clear_requested(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);
	return rt->clear_requested;
}

Color GLES3::TextureStorage::render_target_get_clear_request_color(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Color());
	return rt->clear_color;
}

void GLES3::TextureStorage::render_target_disable_clear_request(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = false;
}

void GLES3::TextureStorage::render_target_do_clear_request(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (!rt->clear_requested) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);

	glClearBufferfv(GL_COLOR, 0, rt->clear_color.as_float4_buffer());
	rt->clear_requested = false;
	glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
}

GLuint GLES3::TextureStorage::render_target_get_fbo(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 0);

	return rt->fbo;
}

GLuint GLES3::TextureStorage::render_target_get_color(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 0);

	if (rt->overridden.color.is_valid()) {
		Texture* texture = get_texture(rt->overridden.color);
		ERR_FAIL_NULL_V(texture, 0);

		return texture->tex_id;
	}
	else {
		return rt->color;
	}
}

GLuint GLES3::TextureStorage::render_target_get_depth(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 0);

	if (rt->overridden.depth.is_valid()) {
		Texture* texture = get_texture(rt->overridden.depth);
		ERR_FAIL_NULL_V(texture, 0);

		return texture->tex_id;
	}
	else {
		return rt->depth;
	}
}

bool GLES3::TextureStorage::render_target_get_depth_has_stencil(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->depth_has_stencil;
}

void GLES3::TextureStorage::render_target_set_reattach_textures(
	RID p_render_target, bool p_reattach_textures)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->reattach_textures = p_reattach_textures;
}

bool GLES3::TextureStorage::render_target_is_reattach_textures(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->reattach_textures;
}

void GLES3::TextureStorage::render_target_set_sdf_size_and_scale(
	RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (rt->sdf_oversize == p_size && rt->sdf_scale == p_scale) {
		return;
	}

	rt->sdf_oversize = p_size;
	rt->sdf_scale = p_scale;

	_render_target_clear_sdf(rt);
}

Rect2i GLES3::TextureStorage::_render_target_get_sdf_rect(const RenderTarget* rt)
{
	Size2i margin;
	int scale;
	switch (rt->sdf_oversize) {
	case RSE::VIEWPORT_SDF_OVERSIZE_100_PERCENT: {
		scale = 100;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_120_PERCENT: {
		scale = 120;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_150_PERCENT: {
		scale = 150;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_200_PERCENT: {
		scale = 200;
	} break;
	default: {
		ERR_PRINT("Invalid viewport SDF oversize, defaulting to 100%.");
		scale = 100;
	} break;
	}

	margin = (rt->size * scale / 100) - rt->size;

	Rect2i r(Vector2i(), rt->size);
	r.position -= margin;
	r.size += margin * 2;

	return r;
}

Rect2i GLES3::TextureStorage::render_target_get_sdf_rect(RID p_render_target)
{
	const RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Rect2i());

	return _render_target_get_sdf_rect(rt);
}

void GLES3::TextureStorage::render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->sdf_enabled = p_enabled;
}

bool GLES3::TextureStorage::render_target_is_sdf_enabled(RID p_render_target)
{
	const RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->sdf_enabled;
}

GLuint GLES3::TextureStorage::render_target_get_sdf_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 0);
	if (rt->sdf_texture_read == 0) {
		Texture* texture = texture_owner.get_or_null(default_gl_textures[DEFAULT_GL_TEXTURE_BLACK]);
		return texture ? texture->tex_id : 0;
	}

	return rt->sdf_texture_read;
}

void GLES3::TextureStorage::_render_target_allocate_sdf(RenderTarget* rt)
{
	ERR_FAIL_COND(rt->sdf_texture_write_fb != 0);

	Size2i size = _render_target_get_sdf_rect(rt).size;

	glGenTextures(1, &rt->sdf_texture_write);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_write);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_R8, size.width, size.height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	GLES3::Utilities::texture_allocated_data(
		rt->sdf_texture_write, size.width * size.height, "SDF texture");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &rt->sdf_texture_write_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, rt->sdf_texture_write_fb);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->sdf_texture_write, 0);

	int scale;
	switch (rt->sdf_scale) {
	case RSE::VIEWPORT_SDF_SCALE_100_PERCENT: {
		scale = 100;
	} break;
	case RSE::VIEWPORT_SDF_SCALE_50_PERCENT: {
		scale = 50;
	} break;
	case RSE::VIEWPORT_SDF_SCALE_25_PERCENT: {
		scale = 25;
	} break;
	default: {
		ERR_PRINT("Invalid viewport SDF scale, defaulting to 100%.");
		scale = 100;
	} break;
	}

	rt->process_size = size * scale / 100;
	rt->process_size = rt->process_size.maxi(1);

	glGenTextures(2, rt->sdf_texture_process);
	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_process[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16I, rt->process_size.width, rt->process_size.height, 0,
		GL_RG_INTEGER, GL_SHORT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLES3::Utilities::texture_allocated_data(rt->sdf_texture_process[0],
		rt->process_size.width * rt->process_size.height * 4, "SDF process texture[0]");

	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_process[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16I, rt->process_size.width, rt->process_size.height, 0,
		GL_RG_INTEGER, GL_SHORT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLES3::Utilities::texture_allocated_data(rt->sdf_texture_process[1],
		rt->process_size.width * rt->process_size.height * 4, "SDF process texture[1]");

	glGenTextures(1, &rt->sdf_texture_read);
	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_read);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt->process_size.width, rt->process_size.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLES3::Utilities::texture_allocated_data(rt->sdf_texture_read,
		rt->process_size.width * rt->process_size.height * 4, "SDF texture (read)");
}

void GLES3::TextureStorage::_render_target_clear_sdf(RenderTarget* rt)
{
	if (rt->sdf_texture_write_fb != 0) {
		GLES3::Utilities::texture_free_data(rt->sdf_texture_read);
		GLES3::Utilities::texture_free_data(rt->sdf_texture_write);
		GLES3::Utilities::texture_free_data(rt->sdf_texture_process[0]);
		GLES3::Utilities::texture_free_data(rt->sdf_texture_process[1]);

		glDeleteFramebuffers(1, &rt->sdf_texture_write_fb);
		rt->sdf_texture_read = 0;
		rt->sdf_texture_write = 0;
		rt->sdf_texture_process[0] = 0;
		rt->sdf_texture_process[1] = 0;
		rt->sdf_texture_write_fb = 0;
	}
}

GLuint GLES3::TextureStorage::render_target_get_sdf_framebuffer(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, 0);

	if (rt->sdf_texture_write_fb == 0) {
		_render_target_allocate_sdf(rt);
	}

	return rt->sdf_texture_write_fb;
}

void GLES3::TextureStorage::render_target_sdf_process(RID p_render_target)
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->sdf_texture_write_fb == 0);

	Rect2i r = _render_target_get_sdf_rect(rt);

	Size2i size = r.size;
	int32_t shift = 0;

	bool shrink = false;

	switch (rt->sdf_scale) {
	case RSE::VIEWPORT_SDF_SCALE_50_PERCENT: {
		size[0] >>= 1;
		size[1] >>= 1;
		shift = 1;
		shrink = true;
	} break;
	case RSE::VIEWPORT_SDF_SCALE_25_PERCENT: {
		size[0] >>= 2;
		size[1] >>= 2;
		shift = 2;
		shrink = true;
	} break;
	default: {
	};
	}

	GLuint temp_fb;
	glGenFramebuffers(1, &temp_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, temp_fb);

	// Load
	CanvasSdfShaderGLES3::ShaderVariant variant =
		shrink ? CanvasSdfShaderGLES3::MODE_LOAD_SHRINK : CanvasSdfShaderGLES3::MODE_LOAD;
	bool success = sdf_shader.shader.version_bind_shader(sdf_shader.shader_version, variant);
	if (!success) {
		return;
	}

	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::BASE_SIZE, r.size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SIZE, size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::STRIDE, 0, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SHIFT, shift, sdf_shader.shader_version, variant);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_write);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->sdf_texture_process[0], 0);
	glViewport(0, 0, size.width, size.height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, 0, size.width, size.height);

	CopyEffects::draw_screen_triangle();

	// Process

	int stride = Math::nearest_power_of_2_templated(MAX(size.width, size.height) / 2);

	variant = CanvasSdfShaderGLES3::MODE_PROCESS;
	success = sdf_shader.shader.version_bind_shader(sdf_shader.shader_version, variant);
	if (!success) {
		return;
	}

	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::BASE_SIZE, r.size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SIZE, size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::STRIDE, stride, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SHIFT, shift, sdf_shader.shader_version, variant);

	bool swap = false;

	// jumpflood
	while (stride > 0) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			rt->sdf_texture_process[swap ? 0 : 1], 0);
		glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_process[swap ? 1 : 0]);

		sdf_shader.shader.version_set_uniform(
			CanvasSdfShaderGLES3::STRIDE, stride, sdf_shader.shader_version, variant);

		CopyEffects::draw_screen_triangle();

		stride /= 2;
		swap = !swap;
	}

	// Store
	variant = shrink ? CanvasSdfShaderGLES3::MODE_STORE_SHRINK : CanvasSdfShaderGLES3::MODE_STORE;
	success = sdf_shader.shader.version_bind_shader(sdf_shader.shader_version, variant);
	if (!success) {
		return;
	}

	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::BASE_SIZE, r.size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SIZE, size, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::STRIDE, stride, sdf_shader.shader_version, variant);
	sdf_shader.shader.version_set_uniform(
		CanvasSdfShaderGLES3::SHIFT, shift, sdf_shader.shader_version, variant);

	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(

		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->sdf_texture_read, 0);
	glBindTexture(GL_TEXTURE_2D, rt->sdf_texture_process[swap ? 1 : 0]);

	CopyEffects::draw_screen_triangle();

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
	glDeleteFramebuffers(1, &temp_fb);
	glDisable(GL_SCISSOR_TEST);
}

void GLES3::TextureStorage::render_target_copy_to_back_buffer(
	RID p_render_target, const Rect2i& p_region, bool p_gen_mipmaps)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->direct_to_screen);

	if (rt->backbuffer_fbo == 0) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {
		region.size = rt->size;
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
	}

	glDisable(GL_BLEND);
	// Single texture copy for backbuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rt->color);
	Rect2 normalized_region = region;
	normalized_region.position = normalized_region.position / Size2(rt->size);
	normalized_region.size = normalized_region.size / Size2(rt->size);
	GLES3::CopyEffects::copy_to_and_from_rect(normalized_region);

	if (p_gen_mipmaps) {
		GLES3::CopyEffects::gaussian_blur(rt->backbuffer, rt->mipmap_count, region, rt->size);
		glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);
	}

	glEnable(GL_BLEND); // 2D starts with blend enabled.
}

void GLES3::TextureStorage::render_target_clear_back_buffer(
	RID p_render_target, const Rect2i& p_region, const Color& p_color)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->direct_to_screen);

	if (rt->backbuffer_fbo == 0) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {

		// Just do a full screen clear;
		glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);
		glClearColor(p_color.r, p_color.g, p_color.b, p_color.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
		glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);
		GLES3::CopyEffects::set_color(p_color, region);
	}
}

void GLES3::TextureStorage::render_target_gen_back_buffer_mipmaps(
	RID p_render_target, const Rect2i& p_region)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	if (rt->backbuffer_fbo == 0) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {
		region.size = rt->size;
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
	}
	glDisable(GL_BLEND);
	GLES3::CopyEffects::gaussian_blur(rt->backbuffer, rt->mipmap_count, region, rt->size);
	glEnable(GL_BLEND); // 2D starts with blend enabled.

	glBindFramebuffer(GL_FRAMEBUFFER, rt->backbuffer_fbo);
}

String GLES3::TextureStorage::get_framebuffer_error(GLenum p_status)
{
	switch (p_status) {
	case GL_FRAMEBUFFER_COMPLETE:
		return "GL_FRAMEBUFFER_COMPLETE";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	default:
		return "Unknown Framebuffer Error: " + String::num_int64(p_status);
	}
}

/* Lifecycle & Defaults */

void GLES3::TextureStorage::initialize()
{
	{
		// White textures
		{
			Ref<Image> image = Image::create_empty(4, 4, true, Image::FORMAT_RGBA8);
			image->fill(Color(1, 1, 1, 1));
			image->generate_mipmaps();

			default_gl_textures[DEFAULT_GL_TEXTURE_WHITE] = texture_allocate();
			texture_2d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_WHITE], image);

			Vector<Ref<Image>> images;
			images.push_back(image);

			default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_WHITE] = texture_allocate();
			texture_2d_layered_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_WHITE],
				images, RSE::TEXTURE_LAYERED_2D_ARRAY);

			for (int i = 0; i < 5; i++) {
				images.push_back(image);
			}

			default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_WHITE] = texture_allocate();
			texture_2d_layered_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_WHITE],
				images, RSE::TEXTURE_LAYERED_CUBEMAP);
		}

		{
			Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
			image->fill(Color(1, 1, 1, 1));

			Vector<Ref<Image>> images;
			for (int i = 0; i < 4; i++) {
				images.push_back(image);
			}
			default_gl_textures[DEFAULT_GL_TEXTURE_3D_WHITE] = texture_allocate();
			texture_3d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_3D_WHITE],
				image->get_format(), 4, 4, 4, false, images);
		}

		// Black textures
		{
			Ref<Image> image = Image::create_empty(4, 4, true, Image::FORMAT_RGBA8);
			image->fill(Color(0, 0, 0, 1));
			image->generate_mipmaps();

			default_gl_textures[DEFAULT_GL_TEXTURE_BLACK] = texture_allocate();
			texture_2d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_BLACK], image);

			Vector<Ref<Image>> images;
			images.push_back(image);

			default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_BLACK] = texture_allocate();
			texture_2d_layered_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_BLACK],
				images, RSE::TEXTURE_LAYERED_2D_ARRAY);

			for (int i = 0; i < 5; i++) {
				images.push_back(image);
			}
			default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_BLACK] = texture_allocate();
			texture_2d_layered_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_BLACK],
				images, RSE::TEXTURE_LAYERED_CUBEMAP);
		}

		{
			Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
			image->fill(Color());

			Vector<Ref<Image>> images;
			for (int i = 0; i < 4; i++) {
				images.push_back(image);
			}
			default_gl_textures[DEFAULT_GL_TEXTURE_3D_BLACK] = texture_allocate();
			texture_3d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_3D_BLACK],
				image->get_format(), 4, 4, 4, false, images);
		}

		// Transparent black textures
		{
			Ref<Image> image = Image::create_empty(4, 4, true, Image::FORMAT_RGBA8);
			image->fill(Color(0, 0, 0, 0));
			image->generate_mipmaps();

			default_gl_textures[DEFAULT_GL_TEXTURE_TRANSPARENT] = texture_allocate();
			texture_2d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_TRANSPARENT], image);

			Vector<Ref<Image>> images;
			images.push_back(image);

			default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_TRANSPARENT] = texture_allocate();
			texture_2d_layered_initialize(
				default_gl_textures[DEFAULT_GL_TEXTURE_2D_ARRAY_TRANSPARENT], images,
				RSE::TEXTURE_LAYERED_2D_ARRAY);

			for (int i = 0; i < 5; i++) {
				images.push_back(image);
			}

			default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_TRANSPARENT] = texture_allocate();
			texture_2d_layered_initialize(
				default_gl_textures[DEFAULT_GL_TEXTURE_CUBEMAP_TRANSPARENT], images,
				RSE::TEXTURE_LAYERED_CUBEMAP);
		}

		{
			Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
			image->fill(Color(0, 0, 0, 0));

			Vector<Ref<Image>> images;
			for (int i = 0; i < 4; i++) {
				images.push_back(image);
			}
			default_gl_textures[DEFAULT_GL_TEXTURE_3D_TRANSPARENT] = texture_allocate();
			texture_3d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_3D_TRANSPARENT],
				image->get_format(), 4, 4, 4, false, images);
		}

		{
			Ref<Image> image = Image::create_empty(4, 4, true, Image::FORMAT_RGBA8);
			image->fill(Color(0.5, 0.5, 1, 1));
			image->generate_mipmaps();

			default_gl_textures[DEFAULT_GL_TEXTURE_NORMAL] = texture_allocate();
			texture_2d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_NORMAL], image);
		}

		{
			Ref<Image> image = Image::create_empty(4, 4, true, Image::FORMAT_RGBA8);
			image->fill(Color(1.0, 0.5, 1, 1));
			image->generate_mipmaps();

			default_gl_textures[DEFAULT_GL_TEXTURE_ANISO] = texture_allocate();
			texture_2d_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_ANISO], image);
		}

		{
			default_gl_textures[DEFAULT_GL_TEXTURE_EXT] = texture_allocate();
			texture_external_initialize(default_gl_textures[DEFAULT_GL_TEXTURE_EXT], 1, 1, 0);
		}

		{
			uint16_t pixel_data[4 * 4];
			for (int i = 0; i < 16; i++) {
				pixel_data[i] = Math::make_half_float(1.0f);
			}

			default_gl_textures[DEFAULT_GL_TEXTURE_DEPTH] = texture_allocate();
			Texture texture;
			texture.width = 4;
			texture.height = 4;
			texture.format = Image::FORMAT_RGBA8;
			texture.type = Texture::TYPE_2D;
			texture.target = GL_TEXTURE_2D;
			texture.active = true;
			glGenTextures(1, &texture.tex_id);
			texture_owner.initialize_rid(default_gl_textures[DEFAULT_GL_TEXTURE_DEPTH], texture);

			glBindTexture(GL_TEXTURE_2D, texture.tex_id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 4, 4, 0, GL_DEPTH_COMPONENT,
				GL_UNSIGNED_SHORT, pixel_data);
			GLES3::Utilities::texture_allocated_data(
				texture.tex_id, 4 * 4 * 2, "Default depth texture");
			texture.gl_set_filter(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	{ // Atlas Texture initialize.
		uint8_t pixel_data[4 * 4 * 4];
		for (int i = 0; i < 16; i++) {
			pixel_data[i * 4 + 0] = 0;
			pixel_data[i * 4 + 1] = 0;
			pixel_data[i * 4 + 2] = 0;
			pixel_data[i * 4 + 3] = 255;
		}

		glGenTextures(1, &texture_atlas.texture);
		glBindTexture(GL_TEXTURE_2D, texture_atlas.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
		GLES3::Utilities::texture_allocated_data(
			texture_atlas.texture, 4 * 4 * 4, "Texture atlas (Default)");
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	{
		sdf_shader.shader.initialize();
		sdf_shader.shader_version = sdf_shader.shader.version_create();
	}

	// Initialize texture placeholder data for the `texture_*_placeholder_initialize()` methods.
	constexpr int placeholder_size = 4;
	texture_2d_placeholder =
		Image::create_empty(placeholder_size, placeholder_size, false, Image::FORMAT_RGBA8);
	for (int i = 0; i < placeholder_size * placeholder_size; i++) {
		const int x = i % placeholder_size;
		const int y = i / placeholder_size;
		texture_2d_placeholder->set_pixel(x, y, (x + y) % 2 == 0 ? Color(1, 0, 1) : Color(0, 0, 0));
	}

	texture_2d_array_placeholder.push_back(texture_2d_placeholder);

	for (int i = 0; i < 6; i++) {
		cubemap_placeholder.push_back(texture_2d_placeholder);
	}

	Ref<Image> texture_2d_placeholder_rotated;
	texture_2d_placeholder_rotated.instantiate();
	texture_2d_placeholder_rotated->copy_internals_from(texture_2d_placeholder);
	texture_2d_placeholder_rotated->rotate_90(CLOCKWISE);
	for (int i = 0; i < 4; i++) {
		texture_3d_placeholder.push_back(
			i % 2 == 0 ? texture_2d_placeholder : texture_2d_placeholder_rotated);
	}

#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		glEnable(GL_PROGRAM_POINT_SIZE);
	}
#endif // GL_API_ENABLED
}

void GLES3::TextureStorage::finalize()
{
	for (int i = 0; i < DEFAULT_GL_TEXTURE_MAX; i++) {
		if (default_gl_textures[i].is_valid()) {
			if (texture_owner.get_or_null(default_gl_textures[i]) != nullptr) {
				texture_free(default_gl_textures[i]);
			}
			default_gl_textures[i] = RID();
		}
	}
	if (texture_atlas.texture != 0) {
		GLES3::Utilities::texture_free_data(texture_atlas.texture);
		texture_atlas.texture = 0;
	}
	if (texture_atlas.framebuffer != 0) {
		glDeleteFramebuffers(1, &texture_atlas.framebuffer);
		texture_atlas.framebuffer = 0;
	}
	if (sdf_shader.shader_version.is_valid()) {
		sdf_shader.shader.version_free(sdf_shader.shader_version);
		sdf_shader.shader_version = RID();
	}
	texture_2d_placeholder.unref();
	texture_2d_array_placeholder.clear();
	cubemap_placeholder.clear();
	texture_3d_placeholder.clear();
}

RID GLES3::TextureStorage::texture_gl_get_default(DefaultGLTexture p_texture)
{
	ERR_FAIL_INDEX_V(p_texture, DEFAULT_GL_TEXTURE_MAX, RID());
	return default_gl_textures[p_texture];
}

void GLES3::TextureStorage::render_target_set_as_unused(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->used_in_frame = false;
}

void GLES3::TextureStorage::texture_initialize(RID p_texture)
{
	texture_owner.initialize_rid(p_texture, Texture());
}

RID GLES3::TextureStorage::texture_2d_create(const Ref<Image>& p_image)
{
	RID rid = texture_allocate();
	texture_2d_initialize(rid, p_image);
	return rid;
}

void GLES3::TextureStorage::texture_2d_initialize_from_texture(
	RID p_texture, const Texture& p_from_texture)
{
	Texture texture = p_from_texture;
	texture_owner.initialize_rid(p_texture, texture);
}

/* Texture & Decal Atlas Helpers */

Rect2 GLES3::TextureStorage::texture_atlas_get_texture_rect(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Rect2());
	return Rect2(0, 0, 1, 1);
}

void GLES3::TextureStorage::texture_add_to_decal_atlas(RID p_texture, bool p_panorama_to_dp)
{
	// Stub for GLES3 decal atlas tracking
}

void GLES3::TextureStorage::texture_remove_from_decal_atlas(RID p_texture, bool p_panorama_to_dp)
{
	// Stub for GLES3 decal atlas tracking
}

void GLES3::TextureStorage::bind_dispatch()
{
	RendererTextureStorage::bind<GLES3::TextureStorage>();
}

void GLES3::TextureStorage::texture_set_format(RID p_texture, Image::Format p_format) {}

#endif // GLES3_ENABLED


