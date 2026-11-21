/**************************************************************************/
/*  rasterizer_gles3.cpp                                                  */
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

#include "drivers/gles3/rasterizer_canvas_gles3.h"
#include "drivers/gles3/storage/light_storage.h"
#include "drivers/gles3/storage/material_storage.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/storage/particles_storage.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "rasterizer_gles3.h"
#include "servers/rendering/storage/utilities.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "drivers/gles3/rasterizer_scene_gles3.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/rendering_server_types.h"

#define _EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB 0x8242
#define _EXT_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB 0x8243
#define _EXT_DEBUG_CALLBACK_FUNCTION_ARB 0x8244
#define _EXT_DEBUG_CALLBACK_USER_PARAM_ARB 0x8245
#define _EXT_DEBUG_SOURCE_API_ARB 0x8246
#define _EXT_DEBUG_SOURCE_WINDOW_SYSTEM_ARB 0x8247
#define _EXT_DEBUG_SOURCE_SHADER_COMPILER_ARB 0x8248
#define _EXT_DEBUG_SOURCE_THIRD_PARTY_ARB 0x8249
#define _EXT_DEBUG_SOURCE_APPLICATION_ARB 0x824A
#define _EXT_DEBUG_SOURCE_OTHER_ARB 0x824B
#define _EXT_DEBUG_TYPE_ERROR_ARB 0x824C
#define _EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB 0x824D
#define _EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB 0x824E
#define _EXT_DEBUG_TYPE_PORTABILITY_ARB 0x824F
#define _EXT_DEBUG_TYPE_PERFORMANCE_ARB 0x8250
#define _EXT_DEBUG_TYPE_OTHER_ARB 0x8251
#define _EXT_DEBUG_TYPE_MARKER_ARB 0x8268
#define _EXT_MAX_DEBUG_MESSAGE_LENGTH_ARB 0x9143
#define _EXT_MAX_DEBUG_LOGGED_MESSAGES_ARB 0x9144
#define _EXT_DEBUG_LOGGED_MESSAGES_ARB 0x9145
#define _EXT_DEBUG_SEVERITY_HIGH_ARB 0x9146
#define _EXT_DEBUG_SEVERITY_MEDIUM_ARB 0x9147
#define _EXT_DEBUG_SEVERITY_LOW_ARB 0x9148
#define _EXT_DEBUG_OUTPUT 0x92E0

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GLAPIENTRY
#if defined(WINDOWS_ENABLED)
#define GLAPIENTRY APIENTRY
#else
#define GLAPIENTRY
#endif
#endif

#include <platform_gl.h>

#if defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
#include <platform_egl.h>
#endif

#if defined(GLAD_ENABLED) || defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
// We can debug GL if we use GLAD or EGL, so not on iOS or Web.
#define GL_DEBUG_CALLBACK

#ifdef _WIN32
#define strcpy strcpy_s
#endif
#endif // GLAD_ENABLED || EGL_ENABLED || ANDROID_ENABLED

#ifdef WINDOWS_ENABLED
bool RasterizerGLES3::screen_flipped_y = false;
#endif

void RasterizerGLES3::begin_frame(double frame_step)
{
	frame++;
	delta = frame_step;

	time_total += frame_step;

	RasterizerCanvasGLES3::set_time(time_total);
	scene->set_time(time_total, frame_step);

	GLES3::Utilities::_capture_timestamps_begin();

	// scene->iteration();
}

void RasterizerGLES3::bind_mesh_storage()
{
	RendererMeshStorage::mesh_allocate = &GLES3::MeshStorage::mesh_allocate;
	RendererMeshStorage::mesh_initialize = &GLES3::MeshStorage::mesh_initialize;
	RendererMeshStorage::mesh_set_blend_shape_count =
		&GLES3::MeshStorage::mesh_set_blend_shape_count;
	RendererMeshStorage::mesh_needs_instance = &GLES3::MeshStorage::mesh_needs_instance;
	RendererMeshStorage::mesh_add_surface = &GLES3::MeshStorage::mesh_add_surface;
	RendererMeshStorage::mesh_get_blend_shape_count =
		&GLES3::MeshStorage::mesh_get_blend_shape_count;
	RendererMeshStorage::mesh_set_blend_shape_mode = &GLES3::MeshStorage::mesh_set_blend_shape_mode;
	RendererMeshStorage::mesh_get_blend_shape_mode = &GLES3::MeshStorage::mesh_get_blend_shape_mode;
	RendererMeshStorage::mesh_surface_update_vertex_region =
		&GLES3::MeshStorage::mesh_surface_update_vertex_region;
	RendererMeshStorage::mesh_surface_update_attribute_region =
		&GLES3::MeshStorage::mesh_surface_update_attribute_region;
	RendererMeshStorage::mesh_surface_update_skin_region =
		&GLES3::MeshStorage::mesh_surface_update_skin_region;
	RendererMeshStorage::mesh_surface_update_index_region =
		&GLES3::MeshStorage::mesh_surface_update_index_region;
	RendererMeshStorage::mesh_surface_set_material = &GLES3::MeshStorage::mesh_surface_set_material;
	RendererMeshStorage::mesh_surface_get_material = &GLES3::MeshStorage::mesh_surface_get_material;
	RendererMeshStorage::mesh_get_surface = [](RID p_mesh, int p_surface) {
		return GLES3::MeshStorage::mesh_get_surface(p_mesh, p_surface);
	};
	RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
	RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid =
		&GLES3::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
	RendererMeshStorage::mesh_get_surface_count = &GLES3::MeshStorage::mesh_get_surface_count;
	RendererMeshStorage::mesh_set_custom_aabb = &GLES3::MeshStorage::mesh_set_custom_aabb;
	RendererMeshStorage::mesh_get_custom_aabb = &GLES3::MeshStorage::mesh_get_custom_aabb;
	RendererMeshStorage::mesh_get_aabb = &GLES3::MeshStorage::mesh_get_aabb;
	RendererMeshStorage::mesh_set_path = &GLES3::MeshStorage::mesh_set_path;
	RendererMeshStorage::mesh_get_path = &GLES3::MeshStorage::mesh_get_path;
	RendererMeshStorage::mesh_set_shadow_mesh = &GLES3::MeshStorage::mesh_set_shadow_mesh;
	RendererMeshStorage::mesh_clear = &GLES3::MeshStorage::mesh_clear;
	RendererMeshStorage::mesh_surface_remove = &GLES3::MeshStorage::mesh_surface_remove;
	RendererMeshStorage::mesh_debug_usage = &GLES3::MeshStorage::mesh_debug_usage;

	RendererMeshStorage::mesh_instance_create = &GLES3::MeshStorage::mesh_instance_create;
	RendererMeshStorage::mesh_instance_free = &GLES3::MeshStorage::mesh_instance_free;
	RendererMeshStorage::mesh_instance_set_skeleton =
		&GLES3::MeshStorage::mesh_instance_set_skeleton;
	RendererMeshStorage::mesh_instance_set_blend_shape_weight =
		&GLES3::MeshStorage::mesh_instance_set_blend_shape_weight;
	RendererMeshStorage::mesh_instance_check_for_update =
		&GLES3::MeshStorage::mesh_instance_check_for_update;
	RendererMeshStorage::mesh_instance_set_canvas_item_transform =
		&GLES3::MeshStorage::mesh_instance_set_canvas_item_transform;
	RendererMeshStorage::update_mesh_instances = &GLES3::MeshStorage::update_mesh_instances;

	RendererMeshStorage::_multimesh_allocate = &GLES3::MeshStorage::_multimesh_allocate;
	RendererMeshStorage::_multimesh_initialize = &GLES3::MeshStorage::_multimesh_initialize;
	RendererMeshStorage::_multimesh_free = &GLES3::MeshStorage::_multimesh_free;
	RendererMeshStorage::_multimesh_allocate_data = &GLES3::MeshStorage::_multimesh_allocate_data;
	RendererMeshStorage::_multimesh_get_instance_count =
		&GLES3::MeshStorage::_multimesh_get_instance_count;
	RendererMeshStorage::_multimesh_set_mesh = &GLES3::MeshStorage::_multimesh_set_mesh;
	RendererMeshStorage::_multimesh_instance_set_transform =
		&GLES3::MeshStorage::_multimesh_instance_set_transform;
	RendererMeshStorage::_multimesh_instance_set_transform_2d =
		&GLES3::MeshStorage::_multimesh_instance_set_transform_2d;
	RendererMeshStorage::_multimesh_instance_set_color =
		&GLES3::MeshStorage::_multimesh_instance_set_color;
	RendererMeshStorage::_multimesh_instance_set_custom_data =
		&GLES3::MeshStorage::_multimesh_instance_set_custom_data;
	RendererMeshStorage::_multimesh_set_custom_aabb =
		&GLES3::MeshStorage::_multimesh_set_custom_aabb;
	RendererMeshStorage::_multimesh_get_custom_aabb =
		&GLES3::MeshStorage::_multimesh_get_custom_aabb;
	RendererMeshStorage::_multimesh_get_mesh = &GLES3::MeshStorage::_multimesh_get_mesh;
	RendererMeshStorage::_multimesh_instance_get_transform =
		&GLES3::MeshStorage::_multimesh_instance_get_transform;
	RendererMeshStorage::_multimesh_instance_get_transform_2d =
		&GLES3::MeshStorage::_multimesh_instance_get_transform_2d;
	RendererMeshStorage::_multimesh_instance_get_color =
		&GLES3::MeshStorage::_multimesh_instance_get_color;
	RendererMeshStorage::_multimesh_instance_get_custom_data =
		&GLES3::MeshStorage::_multimesh_instance_get_custom_data;
	RendererMeshStorage::_multimesh_set_buffer = &GLES3::MeshStorage::_multimesh_set_buffer;
	RendererMeshStorage::_multimesh_get_command_buffer_rd_rid =
		&GLES3::MeshStorage::_multimesh_get_command_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer_rd_rid =
		&GLES3::MeshStorage::_multimesh_get_buffer_rd_rid;
	RendererMeshStorage::_multimesh_get_buffer = &GLES3::MeshStorage::_multimesh_get_buffer;
	RendererMeshStorage::_multimesh_set_visible_instances =
		&GLES3::MeshStorage::_multimesh_set_visible_instances;
	RendererMeshStorage::_multimesh_get_visible_instances =
		&GLES3::MeshStorage::_multimesh_get_visible_instances;
	RendererMeshStorage::_multimesh_get_aabb = &GLES3::MeshStorage::_multimesh_get_aabb;
	RendererMeshStorage::_multimesh_get_interpolator =
		&GLES3::MeshStorage::_multimesh_get_interpolator;

	RendererMeshStorage::skeleton_allocate = &GLES3::MeshStorage::skeleton_allocate;
	RendererMeshStorage::skeleton_initialize = &GLES3::MeshStorage::skeleton_initialize;
	RendererMeshStorage::skeleton_free = &GLES3::MeshStorage::skeleton_free;
	RendererMeshStorage::skeleton_allocate_data = &GLES3::MeshStorage::skeleton_allocate_data;
	RendererMeshStorage::skeleton_set_base_transform_2d =
		&GLES3::MeshStorage::skeleton_set_base_transform_2d;
	RendererMeshStorage::skeleton_get_bone_count = &GLES3::MeshStorage::skeleton_get_bone_count;
	RendererMeshStorage::skeleton_bone_set_transform =
		&GLES3::MeshStorage::skeleton_bone_set_transform;
	RendererMeshStorage::skeleton_bone_get_transform =
		&GLES3::MeshStorage::skeleton_bone_get_transform;
	RendererMeshStorage::skeleton_bone_set_transform_2d =
		&GLES3::MeshStorage::skeleton_bone_set_transform_2d;
	RendererMeshStorage::skeleton_bone_get_transform_2d =
		&GLES3::MeshStorage::skeleton_bone_get_transform_2d;
	RendererMeshStorage::skeleton_update_dependency =
		&GLES3::MeshStorage::skeleton_update_dependency;
}

void RasterizerGLES3::end_frame(bool p_swap_buffers) { GLES3::Utilities::capture_timestamps_end(); }

void RasterizerGLES3::gl_end_frame(bool p_swap_buffers)
{
	if (p_swap_buffers) {
		DisplayServer::swap_buffers();
	}
	else {
		glFinish();
	}
}

#ifdef GL_DEBUG_CALLBACK
static void GLAPIENTRY _gl_debug_print(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	// These are ultimately annoying, so removing for now.
	if (type == _EXT_DEBUG_TYPE_OTHER_ARB || type == _EXT_DEBUG_TYPE_PERFORMANCE_ARB ||
		type == _EXT_DEBUG_TYPE_MARKER_ARB) {
		return;
	}

	char debSource[256], debType[256], debSev[256];

	if (source == _EXT_DEBUG_SOURCE_API_ARB) {
		strcpy(debSource, "OpenGL");
	}
	else if (source == _EXT_DEBUG_SOURCE_WINDOW_SYSTEM_ARB) {
		strcpy(debSource, "Windows");
	}
	else if (source == _EXT_DEBUG_SOURCE_SHADER_COMPILER_ARB) {
		strcpy(debSource, "Shader Compiler");
	}
	else if (source == _EXT_DEBUG_SOURCE_THIRD_PARTY_ARB) {
		strcpy(debSource, "Third Party");
	}
	else if (source == _EXT_DEBUG_SOURCE_APPLICATION_ARB) {
		strcpy(debSource, "Application");
	}
	else if (source == _EXT_DEBUG_SOURCE_OTHER_ARB) {
		strcpy(debSource, "Other");
	}
	else {
		ERR_FAIL_MSG(
			vformat("GL ERROR: Invalid or unhandled source '%d' in debug callback.", source));
	}

	if (type == _EXT_DEBUG_TYPE_ERROR_ARB) {
		strcpy(debType, "Error");
	}
	else if (type == _EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB) {
		strcpy(debType, "Deprecated behavior");
	}
	else if (type == _EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) {
		strcpy(debType, "Undefined behavior");
	}
	else if (type == _EXT_DEBUG_TYPE_PORTABILITY_ARB) {
		strcpy(debType, "Portability");
	}
	else {
		ERR_FAIL_MSG(vformat("GL ERROR: Invalid or unhandled type '%d' in debug callback.", type));
	}

	if (severity == _EXT_DEBUG_SEVERITY_HIGH_ARB) {
		strcpy(debSev, "High");
	}
	else if (severity == _EXT_DEBUG_SEVERITY_MEDIUM_ARB) {
		strcpy(debSev, "Medium");
	}
	else if (severity == _EXT_DEBUG_SEVERITY_LOW_ARB) {
		strcpy(debSev, "Low");
	}
	else {
		ERR_FAIL_MSG(
			vformat("GL ERROR: Invalid or unhandled severity '%d' in debug callback.", severity));
	}

	String output = String() + "GL ERROR: Source: " + debSource + "\tType: " + debType +
					"\tID: " + itos(id) + "\tSeverity: " + debSev + "\tMessage: " + message;

	ERR_PRINT(output);
}
#endif // GL_DEBUG_CALLBACK

typedef void(GLAPIENTRY* DEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const char* message, const void* userParam);

typedef void(GLAPIENTRY* DebugMessageCallbackARB)(DEBUGPROCARB callback, const void* userParam);

#if defined(GLAD_ENABLED) && defined(EGL_ENABLED)
void* _egl_load_function_wrapper(const char* p_name) { return (void*)eglGetProcAddress(p_name); }
#endif

void RasterizerGLES3::initialize()
{
#ifdef GLAD_ENABLED
	bool glad_loaded = false;

#ifdef EGL_ENABLED
#if defined(EGL_STATIC)
	bool has_egl = true;
#else
	bool has_egl = (eglGetProcAddress != nullptr);
#endif

	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (has_egl && !glad_loaded && gladLoadGL((GLADloadfunc)&_egl_load_function_wrapper)) {
			glad_loaded = true;
		}
	}
	else {
		if (has_egl && !glad_loaded && gladLoadGLES2((GLADloadfunc)&_egl_load_function_wrapper)) {
			glad_loaded = true;
		}
	}
#endif // EGL_ENABLED

	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (!glad_loaded && gladLoaderLoadGL()) {
			glad_loaded = true;
		}
#ifdef GLES_API_ENABLED
	}
	else {
		if (!glad_loaded && gladLoaderLoadGLES2()) {
			glad_loaded = true;
		}
#endif
	}

	ERR_FAIL_COND_MSG(!glad_loaded, "Error initializing GLAD.");
#endif // GLAD_ENABLED

	// Print adapter details only AFTER GLAD has populated glGetString
	Engine::print_header(vformat("OpenGL API %s - Compatibility - Using Device: %s - %s",
		Renderer::get_video_adapter_api_version(), Renderer::get_video_adapter_vendor(),
		Renderer::get_video_adapter_name()));
	if (Engine::get_gpu_index() >= 0) {
		WARN_PRINT(
			"The Compatibility renderer does not support overriding the GPU with the --gpu-index "
			"command line argument. Falling back to the default GPU for OpenGL applications.");
	}

#ifdef GL_DEBUG_CALLBACK
	// Setup OpenGL debug callback, via DebugMessageCallbackARB (GLAD or EGL).

#ifdef GLAD_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose()) {
			if (GLAD_GL_ARB_debug_output) {
				glEnable(_EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB((GLDEBUGPROCARB)_gl_debug_print, nullptr);
				glEnable(_EXT_DEBUG_OUTPUT);
			}
			else {
				__print_line("OpenGL debugging not supported!");
			}
		}
	}

#ifdef GL_API_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose() && GLAD_GL_ARB_debug_output) {
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_ERROR_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB,
				_EXT_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB, _EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr,
				GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB,
				_EXT_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB, _EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr,
				GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_PORTABILITY_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_PERFORMANCE_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
			glDebugMessageControlARB(_EXT_DEBUG_SOURCE_API_ARB, _EXT_DEBUG_TYPE_OTHER_ARB,
				_EXT_DEBUG_SEVERITY_HIGH_ARB, 0, nullptr, GL_TRUE);
		}
	}
#endif // GL_API_ENABLED
#endif // GLAD_ENABLED

#if defined(EGL_ENABLED) || defined(ANDROID_ENABLED)
#ifdef GLES_API_ENABLED
	if (!RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::is_stdout_verbose()) {
			DebugMessageCallbackARB callback =
				(DebugMessageCallbackARB)eglGetProcAddress("glDebugMessageCallback");
			if (!callback) {
				callback = (DebugMessageCallbackARB)eglGetProcAddress("glDebugMessageCallbackKHR");
			}

			if (callback) {
				__print_line("godot: ENABLING GL DEBUG");
				glEnable(_EXT_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				callback((DEBUGPROCARB)_gl_debug_print, nullptr);
				glEnable(_EXT_DEBUG_OUTPUT);
			}
		}
	}
#endif // GLES_API_ENABLED
#endif // EGL_ENABLED || ANDROID_ENABLED

#endif // GL_DEBUG_CALLBACK

	{
		// Setup shader cache.

		String shader_cache_dir = Engine::get_shader_cache_path();
		if (shader_cache_dir.is_empty()) {
			shader_cache_dir = "user://";
		}
		Ref<DirAccess> da = DirAccess::open(shader_cache_dir);
		if (da.is_null()) {
			ERR_PRINT("Can't create shader cache folder, no shader caching will happen: " +
					  shader_cache_dir);
		}
		else {
			Error err = da->change_dir("shader_cache");
			if (err != OK) {
				err = da->make_dir("shader_cache");
			}
			if (err != OK) {
				ERR_PRINT("Can't create shader cache folder, no shader caching will happen: " +
						  shader_cache_dir);
			}
			else {
				shader_cache_dir = shader_cache_dir.path_join("shader_cache");
				if (!Engine::is_editor_hint()) {
					shader_cache_dir = String(); // disable only if not editor
				}
				if (!shader_cache_dir.is_empty()) {
					ShaderGLES3::set_shader_cache_dir(shader_cache_dir);
				}
			}
		}
	}

	// OpenGL needs to be initialized before initializing the Rasterizers
	GLES3::Config::initialize();
	GLES3::Utilities::initialize();
	GLES3::TextureStorage::initialize();
	GLES3::MaterialStorage::initialize();
	GLES3::MeshStorage::initialize();
	GLES3::LightStorage::initialize();
	GLES3::ParticlesStorage::initialize();
	GLES3::CopyEffects::initialize();
	GLES3::CubemapFilter::initialize();
	GLES3::Glow::initialize();
	GLES3::PostEffects::initialize();
	GLES3::FeedEffects::initialize();
	GLES3::GI::initialize();
	GLES3::Fog::initialize();

	RasterizerCanvasGLES3::initialize();
	scene = memnew(RasterizerSceneGLES3(texture_storage, material_storage, light_storage));
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::set_scene_render(scene);
	}
	printf("[LIFECYCLE 1] Scene allocated: scene=%p, vptr=%p\n", (void*)scene, *(void**)scene);
	fflush(stdout);
	RendererTextureStorage::bind<GLES3::TextureStorage>();
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	GLES3::TextureStorage::_tex_blit_shader_initialize();

	// Disable OpenGL linear to sRGB conversion, because Godot will always do this conversion
	// itself.
	if (GLES3::Config::srgb_framebuffer_supported) {
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}

void RasterizerGLES3::finalize()
{
	static bool finalized = false;
	if (finalized) {
		return;
	}
	finalized = true;
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	GLES3::TextureStorage::_tex_blit_shader_free();
	printf("[LIFECYCLE 4] About to memdelete(scene): scene=%p, vptr=%p\n", (void*)scene,
		scene ? *(void**)scene : nullptr);
	fflush(stdout);
	if (RendererSceneCull::is_initialized() && RendererSceneCull::get_scene_render() == scene) {
		RendererSceneCull::set_scene_render(nullptr);
	}
	if (scene) {
		memdelete(scene);
		scene = nullptr;
	}
	RasterizerCanvasGLES3::finalize();
	GLES3::GI::finalize();
	GLES3::Fog::finalize();
	GLES3::PostEffects::finalize();
	GLES3::Glow::finalize();
	GLES3::CubemapFilter::finalize();
	GLES3::CopyEffects::finalize();
	GLES3::FeedEffects::finalize();
	GLES3::ParticlesStorage::finalize();
	GLES3::MeshStorage::finalize();
	GLES3::MaterialStorage::finalize();
	GLES3::TextureStorage::finalize();
	GLES3::Utilities::finalize();
	GLES3::Config::finalize();
}

void RasterizerGLES3::make_current(bool p_gles_over_gl)
{
	RasterizerUtilGLES3::set_gles_over_gl(p_gles_over_gl);
	OS::set_gles_over_gl(p_gles_over_gl);

	RendererCompositor::low_end = true;
	RendererCompositor::bind_compositor<RasterizerGLES3>();
}

void RasterizerGLES3::_blit_render_target_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen& p_blit, bool p_first)
{
	GLES3::RenderTarget* rt = GLES3::TextureStorage::get_render_target(p_blit.render_target);
	ERR_FAIL_NULL(rt);

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

	if (p_first) {
		if (p_blit.dst_rect.position != Vector2() || p_blit.dst_rect.size != rt->size) {
			Size2i max_vp = GLES3::Utilities::get_maximum_viewport_size();
			glViewport(0, 0, max_vp[0], max_vp[1]);
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	Vector2 screen_rect_end = p_blit.dst_rect.get_end();
	Vector2 p1 = Vector2(p_blit.dst_rect.position.x, screen_rect_end.y);
	Vector2 p2 = Vector2(screen_rect_end.x, p_blit.dst_rect.position.y);
	Vector2 size = p2 - p1;

	bool flip_y = !rt->overridden.is_overridden;
	bool linear_to_srgb = false;

	Rect2 screenrect = Rect2(Vector2(0.0, flip_y ? 1.0 : 0.0), Vector2(1.0, flip_y ? -1.0 : 1.0));
	glViewport(int(p1.x), int(DisplayServer::window_get_size(p_screen).y - p1.y),
		int(size.x), int(-size.y));

	GLenum target = (rt->view_count > 1) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(target, rt->color);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	if (p_blit.lens_distortion.apply &&
		(p_blit.lens_distortion.k1 != 0.0 || p_blit.lens_distortion.k2)) {
		GLES3::CopyEffects::copy_with_lens_distortion(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0,
			p_blit.lens_distortion.eye_center, p_blit.lens_distortion.k1, p_blit.lens_distortion.k2,
			p_blit.lens_distortion.upscale, p_blit.lens_distortion.aspect_ratio, linear_to_srgb);
	}
	else if (rt->view_count > 1) {
		GLES3::CopyEffects::copy_to_rect_3d(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0, GLES3::Texture::TYPE_LAYERED,
			0.0, linear_to_srgb);
	}
	else {
		GLES3::CopyEffects::copy_to_rect(screenrect, linear_to_srgb);
	}

	glBindTexture(target, 0);
}

// is this p_screen useless in a multi window environment?
void RasterizerGLES3::blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	// Querying the actual window size from the DisplayServer would deadlock in separate
	// render thread mode, so set the biggest viewport the implementation supports to
	// ensure the entire window is covered (including across resizes).
	Size2i max_vp = GLES3::Utilities::get_maximum_viewport_size();
	glViewport(0, 0, max_vp[0], max_vp[1]);
	// glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < p_amount; i++) {
		_blit_render_target_to_screen(p_screen, p_render_targets[i], i == 0);
	}
}

void RasterizerGLES3::set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
	RSE::SplashStretchMode p_stretch_mode, bool p_use_filter)
{
	if (p_image.is_null() || p_image->is_empty()) {
		return;
	}

	Size2i win_size = DisplayServer::window_get_size();

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glViewport(0, 0, win_size.width, win_size.height);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glClearColor(p_color.r, p_color.g, p_color.b, OS::is_layered_allowed() ? p_color.a : 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	RID texture = GLES3::TextureStorage::texture_allocate();
	GLES3::TextureStorage::texture_2d_initialize(texture, p_image);

	Rect2 screenrect = RenderingServerTypes::get_splash_stretched_screen_rect(
		p_image->get_size(), win_size, p_stretch_mode);

#ifdef WINDOWS_ENABLED
	if (!screen_flipped_y)
#endif
	{
		// Flip Y.
		screenrect.position.y = win_size.y - screenrect.position.y;
		screenrect.size.y = -screenrect.size.y;
	}

	// Normalize texture coordinates to window size.
	screenrect.position /= win_size;
	screenrect.size /= win_size;

	GLES3::Texture* t = GLES3::TextureStorage::get_texture(texture);
	t->gl_set_filter(p_use_filter ? RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR
								  : RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t->tex_id);
	GLES3::CopyEffects::copy_to_rect(screenrect);
	glBindTexture(GL_TEXTURE_2D, 0);

	gl_end_frame(true);

	GLES3::TextureStorage::texture_free(texture);
}

void RasterizerGLES3::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&GLES3::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&GLES3::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free = &GLES3::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&GLES3::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&GLES3::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call = &GLES3::Utilities::visibility_notifier_call;

	RendererUtilities::free = &GLES3::Utilities::free;
	RendererUtilities::get_base_type = &GLES3::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &GLES3::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin = &GLES3::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &GLES3::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&GLES3::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&GLES3::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&GLES3::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&GLES3::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
&GLES3::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &GLES3::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&GLES3::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &GLES3::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &GLES3::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &GLES3::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &GLES3::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor = &GLES3::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &GLES3::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&GLES3::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size = &GLES3::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings = &GLES3::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&GLES3::Utilities::get_maximum_uniform_buffer_size;
}

#endif // GLES3_ENABLED

// ============================================================================
// Merged from drivers/gles3/effects/gles3_effects.cpp
// ============================================================================

#include "drivers/gles3/storage/utilities.h"

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
