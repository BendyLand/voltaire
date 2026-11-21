/**************************************************************************/
/*  utilities.h                                                           */
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

#include <cstdint>

class RasterizerUtilGLES3 final
{
private:
	static bool gles_over_gl;

public:
	static void set_gles_over_gl(bool p_gles_over_gl) { gles_over_gl = p_gles_over_gl; }
	static bool is_gles_over_gl() { return gles_over_gl; }

	static void clear_depth(float p_depth);
	static void clear_stencil(int32_t p_stencil);
};

#ifdef GLES3_ENABLED

#include <platform_gl.h>
#include "core/string/ustring.h"
#include "core/templates/hash_set.h"
#include "core/templates/rid_owner.h"
// --- Folded from drivers/gles3/effects/gles3_effects.h ---
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

// --- End drivers/gles3/effects/gles3_effects.h ---
#include "servers/rendering/rendering_device_enums.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/rendering/storage/utilities.h"
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

class RenderSceneBuffersGLES3 final : public RenderSceneBuffers
{
public:
	Size2i internal_size; // Size of the buffer we render 3D content to.
	Size2i target_size;	  // Size of our output buffer (render target).
	RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	uint32_t view_count = 1;
	bool apply_environment_effects_in_post = false;
	bool apply_canvas_bg_exposure = false;

	RID render_target;

	// Color format details from our render target
	GLuint color_internal_format = GL_RGBA8;
	GLuint color_format = GL_RGBA;
	GLuint color_type = GL_UNSIGNED_BYTE;
	uint32_t color_format_size = 4;

	struct FBDEF
	{
		GLuint color = 0;
		GLuint depth = 0;
		GLuint fbo = 0;
	};

	struct RTMSAA3D
	{
		RSE::ViewportMSAA mode = RSE::VIEWPORT_MSAA_DISABLED;
		bool needs_resolve = false;
		GLsizei samples = 1;
		GLuint color = 0;
		GLuint depth = 0;
		GLuint fbo = 0;

		bool check_fbo_cache = false;
		Vector<FBDEF> cached_fbos;
	} msaa3d; // MSAA buffers used to render 3D

	FBDEF internal3d; // buffers used to either render 3D (scaled/post) or to resolve MSAA into

	FBDEF backbuffer3d; // our back buffer

	// Buffers for our glow implementation
	struct GLOW
	{
		GLES3::Glow::Level levels[4];
	} glow;

private:
	void _check_render_buffers();
	void _clear_msaa3d_buffers();
	void _clear_intermediate_buffers();
	void _clear_back_buffers();
	void _clear_glow_buffers();

	void _rt_attach_textures(GLuint p_color, GLuint p_depth, GLsizei p_samples,
		uint32_t p_view_count, bool p_depth_has_stencil);
	GLuint _rt_get_cached_fbo(
		GLuint p_color, GLuint p_depth, GLsizei p_samples, uint32_t p_view_count);

public:
	RenderSceneBuffersGLES3();
	virtual ~RenderSceneBuffersGLES3();
	virtual void configure(const RenderSceneBuffersConfiguration* p_config) override;
	void configure_for_probe(Size2i p_size);

	virtual void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) override
	{
	}

	virtual void set_fsr_sharpness(float p_fsr_sharpness) override {}

	virtual void set_texture_mipmap_bias(float p_texture_mipmap_bias) override {}

	virtual void set_use_debanding(bool p_use_debanding) override {}

	void set_apply_environment_effects_in_post(bool p_apply_environment_effects_in_post);
	void set_apply_canvas_bg_exposure(bool p_apply_canvas_bg_exposure);

	void free_render_buffer_data();

	void check_backbuffer(
		bool p_need_color, bool p_need_depth); // Check if we need to initialize our backbuffer.
	void check_glow_buffers();				   // Check if we need to initialize our glow buffers.

	GLuint get_render_fbo();

	GLuint get_msaa3d_fbo()
	{
		_check_render_buffers();
		return msaa3d.fbo;
	}

	GLuint get_msaa3d_color()
	{
		_check_render_buffers();
		return msaa3d.color;
	}

	GLuint get_msaa3d_depth()
	{
		_check_render_buffers();
		return msaa3d.depth;
	}

	bool get_msaa_needs_resolve()
	{
		_check_render_buffers();
		return msaa3d.needs_resolve;
	}

	GLuint get_internal_fbo()
	{
		_check_render_buffers();
		return internal3d.fbo;
	}

	GLuint get_internal_color()
	{
		_check_render_buffers();
		return internal3d.color;
	}

	GLuint get_internal_depth()
	{
		_check_render_buffers();
		return internal3d.depth;
	}

	GLuint get_backbuffer_fbo() const { return backbuffer3d.fbo; }

	GLuint get_backbuffer() const { return backbuffer3d.color; }

	GLuint get_backbuffer_depth() const { return backbuffer3d.depth; }

	const GLES3::Glow::Level* get_glow_buffers() const { return &glow.levels[0]; }

	// Getters

	_FORCE_INLINE_ RID get_render_target() const { return render_target; }

	_FORCE_INLINE_ uint32_t get_view_count() const { return view_count; }

	_FORCE_INLINE_ Size2i get_internal_size() const { return internal_size; }

	_FORCE_INLINE_ Size2i get_target_size() const { return target_size; }

	_FORCE_INLINE_ RSE::ViewportScaling3DMode get_scaling_3d_mode() const
	{
		return scaling_3d_mode;
	}

	_FORCE_INLINE_ RSE::ViewportMSAA get_msaa_3d() const { return msaa3d.mode; }
};

namespace GLES3
{

/* VISIBILITY NOTIFIER */

struct VisibilityNotifier
{
	AABB aabb;
	Dependency dependency;
};

#define MAX_QUERIES 256

struct Frame
{
	GLuint queries[MAX_QUERIES];
	TightLocalVector<String> timestamp_names;
	TightLocalVector<uint64_t> timestamp_cpu_values;
	uint32_t timestamp_count = 0;
	TightLocalVector<String> timestamp_result_names;
	TightLocalVector<uint64_t> timestamp_cpu_result_values;
	TightLocalVector<uint64_t> timestamp_result_values;
	uint32_t timestamp_result_count = 0;
	uint64_t index = 0;
};

class Utilities final
{
private:
	/* VISIBILITY NOTIFIER */

	static inline RID_Owner<VisibilityNotifier> visibility_notifier_owner;

	/* MISC */

	struct ResourceAllocation
	{
#ifdef DEV_ENABLED
		String name;
#endif
		uint32_t size = 0;
	};

	static inline HashMap<GLuint, ResourceAllocation> buffer_allocs_cache;
	static inline HashMap<GLuint, ResourceAllocation> render_buffer_allocs_cache;
	static inline HashMap<GLuint, ResourceAllocation> texture_allocs_cache;

	static inline uint64_t buffer_mem_cache = 0;
	static inline uint64_t render_buffer_mem_cache = 0;
	static inline uint64_t texture_mem_cache = 0;

public:
	Utilities() = delete;
	Utilities(const Utilities&) = delete;
	Utilities& operator=(const Utilities&) = delete;
	~Utilities() = delete;

	static void initialize();
	static void finalize();

	// Buffer size is specified in bytes
	static Vector<uint8_t> buffer_get_data(
		GLenum p_target, GLuint p_buffer, uint32_t p_buffer_size);

	// Allocate memory with glBufferData. Does not handle resizing.
	_FORCE_INLINE_ static void buffer_allocate_data(GLenum p_target, GLuint p_id, uint32_t p_size,
		const void* p_data, GLenum p_usage, String p_name = "")
	{
		glBufferData(p_target, p_size, p_data, p_usage);
		buffer_mem_cache += p_size;

#ifdef DEV_ENABLED
		ERR_FAIL_COND_MSG(buffer_allocs_cache.has(p_id), "trying to allocate buffer with name " +
															 p_name + " but ID already used by " +
															 buffer_allocs_cache[p_id].name);
#endif

		ResourceAllocation resource_allocation;
		resource_allocation.size = p_size;
#ifdef DEV_ENABLED
		resource_allocation.name = p_name + ": " + itos((uint64_t)p_id);
#endif
		buffer_allocs_cache[p_id] = resource_allocation;
	}

	_FORCE_INLINE_ static void buffer_free_data(GLuint p_id)
	{
		ERR_FAIL_COND(!buffer_allocs_cache.has(p_id));
		glDeleteBuffers(1, &p_id);
		buffer_mem_cache -= buffer_allocs_cache[p_id].size;
		buffer_allocs_cache.erase(p_id);
	}

	_FORCE_INLINE_ static void render_buffer_allocated_data(
		GLuint p_id, uint32_t p_size, String p_name = "")
	{
		render_buffer_mem_cache += p_size;
#ifdef DEV_ENABLED
		ERR_FAIL_COND_MSG(render_buffer_allocs_cache.has(p_id),
			"trying to allocate render buffer with name " + p_name + " but ID already used by " +
				render_buffer_allocs_cache[p_id].name);
#endif
		ResourceAllocation resource_allocation;
		resource_allocation.size = p_size;
#ifdef DEV_ENABLED
		resource_allocation.name = p_name + ": " + itos((uint64_t)p_id);
#endif
		render_buffer_allocs_cache[p_id] = resource_allocation;
	}

	_FORCE_INLINE_ static void render_buffer_free_data(GLuint p_id)
	{
		ERR_FAIL_COND(!render_buffer_allocs_cache.has(p_id));
		glDeleteRenderbuffers(1, &p_id);
		render_buffer_mem_cache -= render_buffer_allocs_cache[p_id].size;
		render_buffer_allocs_cache.erase(p_id);
	}

	// Records that data was allocated for state tracking purposes.
	// Size is measured in bytes.
	_FORCE_INLINE_ static void texture_allocated_data(
		GLuint p_id, uint32_t p_size, String p_name = "")
	{
		texture_mem_cache += p_size;
#ifdef DEV_ENABLED
		ERR_FAIL_COND_MSG(texture_allocs_cache.has(p_id), "trying to allocate texture with name " +
															  p_name + " but ID already used by " +
															  texture_allocs_cache[p_id].name);
#endif
		ResourceAllocation resource_allocation;
		resource_allocation.size = p_size;
#ifdef DEV_ENABLED
		resource_allocation.name = p_name + ": " + itos((uint64_t)p_id);
#endif
		texture_allocs_cache[p_id] = resource_allocation;
	}

	_FORCE_INLINE_ static void texture_free_data(GLuint p_id)
	{
		ERR_FAIL_COND(!texture_allocs_cache.has(p_id));
		glDeleteTextures(1, &p_id);
		texture_mem_cache -= texture_allocs_cache[p_id].size;
		texture_allocs_cache.erase(p_id);
	}

	_FORCE_INLINE_ static void texture_resize_data(GLuint p_id, uint32_t p_size)
	{
		ERR_FAIL_COND(!texture_allocs_cache.has(p_id));
		texture_mem_cache -= texture_allocs_cache[p_id].size;
		texture_mem_cache += p_size;
		texture_allocs_cache[p_id].size = p_size;
	}

	/* INSTANCES */

	static RSE::InstanceType get_base_type(RID p_rid);
	static bool free(RID p_rid);

	/* DEPENDENCIES */

	static void base_update_dependency(RID p_base, DependencyTracker* p_instance);

	/* VISIBILITY NOTIFIER */

	_FORCE_INLINE_ static VisibilityNotifier* get_visibility_notifier(RID p_rid)
	{
		return visibility_notifier_owner.get_or_null(p_rid);
	}

	_FORCE_INLINE_ static bool owns_visibility_notifier(RID p_rid)
	{
		return visibility_notifier_owner.owns(p_rid);
	}

	static RID visibility_notifier_allocate();
	static void visibility_notifier_initialize(RID p_notifier);
	static void visibility_notifier_free(RID p_notifier);

	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb);

	static AABB visibility_notifier_get_aabb(RID p_notifier);
	static void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred);

	/* TIMING */

#define FRAME_COUNT 3

	static inline const uint32_t max_timestamp_query_elements = MAX_QUERIES;

	static inline Frame frames[FRAME_COUNT];
	static inline uint32_t frame = 0;

	static void capture_timestamps_begin();
	static void capture_timestamp(const String& p_name);
	static uint32_t get_captured_timestamps_count();
	static uint64_t get_captured_timestamps_frame();
	static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index);
	static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index);
	static String get_captured_timestamp_name(uint32_t p_index);
	static void _capture_timestamps_begin();
	static void capture_timestamps_end();

	/* MISC */

	static void update_dirty_resources();
	static void set_debug_generate_wireframes(bool p_generate);

	static bool has_os_feature(const String& p_feature);

	static void update_memory_info();

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info);
	static String get_video_adapter_name();
	static String get_video_adapter_vendor();
	static RenderingDeviceEnums::DeviceType get_video_adapter_type();
	static String get_video_adapter_api_version();

	static Size2i get_maximum_viewport_size();
	static uint32_t get_maximum_shader_varyings();
	static uint64_t get_maximum_uniform_buffer_size();
};

} // namespace GLES3

#endif // GLES3_ENABLED
