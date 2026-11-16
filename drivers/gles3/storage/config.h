/**************************************************************************/
/*  config.h                                                              */
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

#include <platform_gl.h>
#include "core/string/ustring.h"
#include "core/templates/hash_set.h"
#undef ConnectFlags // Defined by windows.h through egl.h, breaks object.h.

#ifdef ANDROID_ENABLED
typedef void (*PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
	GLenum, GLenum, GLuint, GLint, GLint, GLsizei);
typedef void (*PFNGLTEXSTORAGE3DMULTISAMPLEPROC)(
	GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(
	GLenum, GLenum, GLenum, GLuint, GLint, GLsizei);
typedef void (*PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(
	GLenum, GLenum, GLuint, GLint, GLsizei, GLint, GLsizei);
typedef void (*PFNEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum, void*);
#endif

namespace GLES3
{

class Config final
{
private:
	static inline bool _initialized = false;

public:
	Config() = delete;
	~Config() = delete;
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return _initialized; }

	static inline bool use_nearest_mip_filter = false;
	static inline bool use_depth_prepass = true;

	static inline GLint max_vertex_texture_image_units = 0;
	static inline GLint max_texture_image_units = 0;
	static inline GLint max_texture_size = 0;
	static inline GLint max_viewport_size[2] = {0, 0};
	static inline GLint max_vertex_attribs = 0;
	static inline GLint64 max_uniform_buffer_size = 0;
	static inline uint32_t max_shader_varyings = 0;

	static inline int64_t max_renderable_elements = 0;
	static inline int64_t max_renderable_lights = 0;
	static inline int64_t max_lights_per_object = 0;

	static inline bool generate_wireframes = false;

	static inline HashSet<String> extensions;

	static inline bool float_texture_supported = false;
	static inline bool float_texture_linear_supported = false;
	static inline bool s3tc_supported = false;
	static inline bool rgtc_supported = false;
	static inline bool bptc_supported = false;
	static inline bool etc2_supported = false;
	static inline bool astc_supported = false;
	static inline bool astc_hdr_supported = false;
	static inline bool astc_layered_supported = false;
	static inline bool astc_3d_supported = false;
	static inline bool srgb_framebuffer_supported = false;
	static inline bool unorm16_texture_supported = false;

	static inline bool force_vertex_shading = false;
	static inline bool specular_occlusion = false;

	static inline bool support_anisotropic_filter = false;
	static inline float anisotropic_level = 0.0f;

	static inline GLint msaa_max_samples = 0;
	static inline bool msaa_supported = false;
	static inline bool msaa_multiview_supported = false;
	static inline bool rt_msaa_supported = false;
	static inline bool rt_msaa_multiview_supported = false;
	static inline bool multiview_supported = false;
	static inline bool external_texture_supported = false;

	// Adreno 3XX compatibility.
	static inline bool disable_particles_workaround = false; // Set to 'true' to disable 'GPUParticles'.

	// PowerVR GE 8320 workaround.
	static inline bool disable_transform_feedback_shader_cache = false;

	// ANGLE shader workaround.
	static inline bool polyfill_half2float = true;

#ifdef ANDROID_ENABLED
	static inline PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC eglFramebufferTextureMultiviewOVR =
		nullptr;
	static inline PFNGLTEXSTORAGE3DMULTISAMPLEPROC eglTexStorage3DMultisample = nullptr;
	static inline PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC
		eglFramebufferTexture2DMultisampleEXT = nullptr;
	static inline PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC
		eglFramebufferTextureMultisampleMultiviewOVR = nullptr;
	static inline PFNEGLIMAGETARGETTEXTURE2DOESPROC eglEGLImageTargetTexture2DOES = nullptr;

#define glFramebufferTextureMultiviewOVR GLES3::Config::eglFramebufferTextureMultiviewOVR
#define glTexStorage3DMultisample GLES3::Config::eglTexStorage3DMultisample
#define glFramebufferTexture2DMultisampleEXT GLES3::Config::eglFramebufferTexture2DMultisampleEXT
#define glFramebufferTextureMultisampleMultiviewOVR                                                \
	GLES3::Config::eglFramebufferTextureMultisampleMultiviewOVR
#define glEGLImageTargetTexture2DOES GLES3::Config::eglEGLImageTargetTexture2DOES
#endif // ANDROID_ENABLED
};

} // namespace GLES3

#endif // GLES3_ENABLED
