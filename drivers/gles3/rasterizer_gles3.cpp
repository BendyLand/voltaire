/**************************************************************************/
/*  rasterizer_gles3.cpp                                                  */
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

#include "rasterizer_gles3.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/storage/utilities.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "drivers/gles3/rasterizer_scene_gles3.h"
#include "drivers/gles3/rasterizer_util_gles3.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
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

	canvas->set_time(time_total);
	scene->set_time(time_total, frame_step);

	GLES3::Utilities::_capture_timestamps_begin();

	// scene->iteration();
}

void RasterizerGLES3::bind_mesh_storage()
{
    RendererMeshStorage::mesh_allocate = []() { return mesh_storage->mesh_allocate(); };
    RendererMeshStorage::mesh_initialize = [](RID p_rid) { mesh_storage->mesh_initialize(p_rid); };
    RendererMeshStorage::mesh_set_blend_shape_count = [](RID p_mesh, int p_count) { mesh_storage->mesh_set_blend_shape_count(p_mesh, p_count); };
    RendererMeshStorage::mesh_needs_instance = [](RID p_mesh, bool p_has_skeleton) { return mesh_storage->mesh_needs_instance(p_mesh, p_has_skeleton); };
    RendererMeshStorage::mesh_add_surface = [](RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface) { mesh_storage->mesh_add_surface(p_mesh, p_surface); };
    RendererMeshStorage::mesh_get_blend_shape_count = [](RID p_mesh) { return mesh_storage->mesh_get_blend_shape_count(p_mesh); };
    RendererMeshStorage::mesh_set_blend_shape_mode = [](RID p_mesh, RSE::BlendShapeMode p_mode) { mesh_storage->mesh_set_blend_shape_mode(p_mesh, p_mode); };
    RendererMeshStorage::mesh_get_blend_shape_mode = [](RID p_mesh) { return mesh_storage->mesh_get_blend_shape_mode(p_mesh); };
    RendererMeshStorage::mesh_surface_update_vertex_region = [](RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) { mesh_storage->mesh_surface_update_vertex_region(p_mesh, p_surface, p_offset, p_data); };
    RendererMeshStorage::mesh_surface_update_attribute_region = [](RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) { mesh_storage->mesh_surface_update_attribute_region(p_mesh, p_surface, p_offset, p_data); };
    RendererMeshStorage::mesh_surface_update_skin_region = [](RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) { mesh_storage->mesh_surface_update_skin_region(p_mesh, p_surface, p_offset, p_data); };
    RendererMeshStorage::mesh_surface_update_index_region = [](RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) { mesh_storage->mesh_surface_update_index_region(p_mesh, p_surface, p_offset, p_data); };
    RendererMeshStorage::mesh_surface_set_material = [](RID p_mesh, int p_surface, RID p_material) { mesh_storage->mesh_surface_set_material(p_mesh, p_surface, p_material); };
    RendererMeshStorage::mesh_surface_get_material = [](RID p_mesh, int p_surface) { return mesh_storage->mesh_surface_get_material(p_mesh, p_surface); };
    RendererMeshStorage::mesh_get_surface = [](RID p_mesh, int p_surface) {
	    return static_cast<const GLES3::MeshStorage*>(mesh_storage)->mesh_get_surface(p_mesh, p_surface);
	};
    RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid = [](RID p_mesh, int p_surface) { return mesh_storage->mesh_surface_get_vertex_buffer_rd_rid(p_mesh, p_surface); };
    RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid = [](RID p_mesh, int p_surface) { return mesh_storage->mesh_surface_get_attribute_buffer_rd_rid(p_mesh, p_surface); };
    RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid = [](RID p_mesh, int p_surface) { return mesh_storage->mesh_surface_get_skin_buffer_rd_rid(p_mesh, p_surface); };
    RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid = [](RID p_mesh, int p_surface) { return mesh_storage->mesh_surface_get_index_buffer_rd_rid(p_mesh, p_surface); };
    RendererMeshStorage::mesh_get_surface_count = [](RID p_mesh) { return mesh_storage->mesh_get_surface_count(p_mesh); };
    RendererMeshStorage::mesh_set_custom_aabb = [](RID p_mesh, const AABB& p_aabb) { mesh_storage->mesh_set_custom_aabb(p_mesh, p_aabb); };
    RendererMeshStorage::mesh_get_custom_aabb = [](RID p_mesh) { return mesh_storage->mesh_get_custom_aabb(p_mesh); };
    RendererMeshStorage::mesh_get_aabb = [](RID p_mesh, RID p_skeleton) { return mesh_storage->mesh_get_aabb(p_mesh, p_skeleton); };
    RendererMeshStorage::mesh_set_path = [](RID p_mesh, const String& p_path) { mesh_storage->mesh_set_path(p_mesh, p_path); };
    RendererMeshStorage::mesh_get_path = [](RID p_mesh) { return mesh_storage->mesh_get_path(p_mesh); };
    RendererMeshStorage::mesh_set_shadow_mesh = [](RID p_mesh, RID p_shadow_mesh) { mesh_storage->mesh_set_shadow_mesh(p_mesh, p_shadow_mesh); };
    RendererMeshStorage::mesh_clear = [](RID p_mesh) { mesh_storage->mesh_clear(p_mesh); };
    RendererMeshStorage::mesh_surface_remove = [](RID p_mesh, int p_surface) { mesh_storage->mesh_surface_remove(p_mesh, p_surface); };
    RendererMeshStorage::mesh_debug_usage = [](List<RenderingServerTypes::MeshInfo>* r_info) { mesh_storage->mesh_debug_usage(r_info); };

    RendererMeshStorage::mesh_instance_create = [](RID p_base) { return mesh_storage->mesh_instance_create(p_base); };
    RendererMeshStorage::mesh_instance_free = [](RID p_mi) { mesh_storage->mesh_instance_free(p_mi); };
    RendererMeshStorage::mesh_instance_set_skeleton = [](RID p_mi, RID p_sk) { mesh_storage->mesh_instance_set_skeleton(p_mi, p_sk); };
    RendererMeshStorage::mesh_instance_set_blend_shape_weight = [](RID p_mi, int p_shape, float p_w) { mesh_storage->mesh_instance_set_blend_shape_weight(p_mi, p_shape, p_w); };
    RendererMeshStorage::mesh_instance_check_for_update = [](RID p_mi) { mesh_storage->mesh_instance_check_for_update(p_mi); };
    RendererMeshStorage::mesh_instance_set_canvas_item_transform = [](RID p_mi, const Transform2D& p_t) { mesh_storage->mesh_instance_set_canvas_item_transform(p_mi, p_t); };
    RendererMeshStorage::update_mesh_instances = []() { mesh_storage->update_mesh_instances(); };

    RendererMeshStorage::_multimesh_allocate = []() { return mesh_storage->_multimesh_allocate(); };
    RendererMeshStorage::_multimesh_initialize = [](RID p_rid) { mesh_storage->_multimesh_initialize(p_rid); };
    RendererMeshStorage::_multimesh_free = [](RID p_rid) { mesh_storage->_multimesh_free(p_rid); };
    RendererMeshStorage::_multimesh_allocate_data = [](RID p_mm, int p_instances, RSE::MultimeshTransformFormat p_format, bool p_colors, bool p_custom_data, bool p_indirect) {
        mesh_storage->_multimesh_allocate_data(p_mm, p_instances, p_format, p_colors, p_custom_data, p_indirect);
    };
    RendererMeshStorage::_multimesh_get_instance_count = [](RID p_mm) { return mesh_storage->_multimesh_get_instance_count(p_mm); };
    RendererMeshStorage::_multimesh_set_mesh = [](RID p_mm, RID p_mesh) { mesh_storage->_multimesh_set_mesh(p_mm, p_mesh); };
    RendererMeshStorage::_multimesh_instance_set_transform = [](RID p_mm, int p_idx, const Transform3D& p_t) { mesh_storage->_multimesh_instance_set_transform(p_mm, p_idx, p_t); };
    RendererMeshStorage::_multimesh_instance_set_transform_2d = [](RID p_mm, int p_idx, const Transform2D& p_t) { mesh_storage->_multimesh_instance_set_transform_2d(p_mm, p_idx, p_t); };
    RendererMeshStorage::_multimesh_instance_set_color = [](RID p_mm, int p_idx, const Color& p_c) { mesh_storage->_multimesh_instance_set_color(p_mm, p_idx, p_c); };
    RendererMeshStorage::_multimesh_instance_set_custom_data = [](RID p_mm, int p_idx, const Color& p_c) { mesh_storage->_multimesh_instance_set_custom_data(p_mm, p_idx, p_c); };
    RendererMeshStorage::_multimesh_set_custom_aabb = [](RID p_mm, const AABB& p_aabb) { mesh_storage->_multimesh_set_custom_aabb(p_mm, p_aabb); };
    RendererMeshStorage::_multimesh_get_custom_aabb = [](RID p_mm) { return mesh_storage->_multimesh_get_custom_aabb(p_mm); };
    RendererMeshStorage::_multimesh_get_mesh = [](RID p_mm) { return mesh_storage->_multimesh_get_mesh(p_mm); };
    RendererMeshStorage::_multimesh_instance_get_transform = [](RID p_mm, int p_idx) { return mesh_storage->_multimesh_instance_get_transform(p_mm, p_idx); };
    RendererMeshStorage::_multimesh_instance_get_transform_2d = [](RID p_mm, int p_idx) { return mesh_storage->_multimesh_instance_get_transform_2d(p_mm, p_idx); };
    RendererMeshStorage::_multimesh_instance_get_color = [](RID p_mm, int p_idx) { return mesh_storage->_multimesh_instance_get_color(p_mm, p_idx); };
    RendererMeshStorage::_multimesh_instance_get_custom_data = [](RID p_mm, int p_idx) { return mesh_storage->_multimesh_instance_get_custom_data(p_mm, p_idx); };
    RendererMeshStorage::_multimesh_set_buffer = [](RID p_mm, const Vector<float>& p_buf) { mesh_storage->_multimesh_set_buffer(p_mm, p_buf); };
    RendererMeshStorage::_multimesh_get_command_buffer_rd_rid = [](RID p_mm) { return mesh_storage->_multimesh_get_command_buffer_rd_rid(p_mm); };
    RendererMeshStorage::_multimesh_get_buffer_rd_rid = [](RID p_mm) { return mesh_storage->_multimesh_get_buffer_rd_rid(p_mm); };
    RendererMeshStorage::_multimesh_get_buffer = [](RID p_mm) { return mesh_storage->_multimesh_get_buffer(p_mm); };
    RendererMeshStorage::_multimesh_set_visible_instances = [](RID p_mm, int p_v) { mesh_storage->_multimesh_set_visible_instances(p_mm, p_v); };
    RendererMeshStorage::_multimesh_get_visible_instances = [](RID p_mm) { return mesh_storage->_multimesh_get_visible_instances(p_mm); };
    RendererMeshStorage::_multimesh_get_aabb = [](RID p_mm) { return mesh_storage->_multimesh_get_aabb(p_mm); };
    RendererMeshStorage::_multimesh_get_interpolator = [](RID p_mm) { return mesh_storage->_multimesh_get_interpolator(p_mm); };

    RendererMeshStorage::skeleton_allocate = []() { return mesh_storage->skeleton_allocate(); };
    RendererMeshStorage::skeleton_initialize = [](RID p_rid) { mesh_storage->skeleton_initialize(p_rid); };
    RendererMeshStorage::skeleton_free = [](RID p_rid) { mesh_storage->skeleton_free(p_rid); };
    RendererMeshStorage::skeleton_allocate_data = [](RID p_sk, int p_bones, bool p_2d) { mesh_storage->skeleton_allocate_data(p_sk, p_bones, p_2d); };
    RendererMeshStorage::skeleton_set_base_transform_2d = [](RID p_sk, const Transform2D& p_t) { mesh_storage->skeleton_set_base_transform_2d(p_sk, p_t); };
    RendererMeshStorage::skeleton_get_bone_count = [](RID p_sk) { return mesh_storage->skeleton_get_bone_count(p_sk); };
    RendererMeshStorage::skeleton_bone_set_transform = [](RID p_sk, int p_b, const Transform3D& p_t) { mesh_storage->skeleton_bone_set_transform(p_sk, p_b, p_t); };
    RendererMeshStorage::skeleton_bone_get_transform = [](RID p_sk, int p_b) { return mesh_storage->skeleton_bone_get_transform(p_sk, p_b); };
    RendererMeshStorage::skeleton_bone_set_transform_2d = [](RID p_sk, int p_b, const Transform2D& p_t) { mesh_storage->skeleton_bone_set_transform_2d(p_sk, p_b, p_t); };
    RendererMeshStorage::skeleton_bone_get_transform_2d = [](RID p_sk, int p_b) { return mesh_storage->skeleton_bone_get_transform_2d(p_sk, p_b); };
    RendererMeshStorage::skeleton_update_dependency = [](RID p_sk, DependencyTracker* p_dep) { mesh_storage->skeleton_update_dependency(p_sk, p_dep); };
}

void RasterizerGLES3::end_frame(bool p_swap_buffers)
{
	GLES3::Utilities::capture_timestamps_end();
}

void RasterizerGLES3::gl_end_frame(bool p_swap_buffers)
{
	if (p_swap_buffers) {
		DisplayServer::get_singleton()->swap_buffers();
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
	Engine::get_singleton()->print_header(
		vformat("OpenGL API %s - Compatibility - Using Device: %s - %s",
			RS::get_video_adapter_api_version(), RS::get_video_adapter_vendor(),
			RS::get_video_adapter_name()));
	if (Engine::get_singleton()->get_gpu_index() >= 0) {
		WARN_PRINT(
			"The Compatibility renderer does not support overriding the GPU with the --gpu-index "
			"command line argument. Falling back to the default GPU for OpenGL applications.");
	}

#ifdef GLAD_ENABLED
	bool glad_loaded = false;

#ifdef EGL_ENABLED
	// There should be a more flexible system for getting the GL pointer, as
	// different DisplayServers can have different ways. We can just use the GLAD
	// version global to see if it loaded for now though, otherwise we fall back to
	// the generic loader below.
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

	// FIXME this is an early return from a constructor.  Any other code using this instance will
	// crash or the finalizer will crash, because none of the members of this instance are
	// initialized, so this just makes debugging harder.  It should either crash here intentionally,
	// or we need to actually test for this situation before constructing this.
	ERR_FAIL_COND_MSG(!glad_loaded, "Error initializing GLAD.");
#endif // GLAD_ENABLED

#ifdef GL_DEBUG_CALLBACK
	// Setup OpenGL debug callback, via DebugMessageCallbackARB (GLAD or EGL).

#ifdef GLAD_ENABLED
	if (RasterizerUtilGLES3::is_gles_over_gl()) {
		if (OS::get_singleton()->is_stdout_verbose()) {
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
		if (OS::get_singleton()->is_stdout_verbose() && GLAD_GL_ARB_debug_output) {
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
		if (OS::get_singleton()->is_stdout_verbose()) {
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

		String shader_cache_dir = Engine::get_singleton()->get_shader_cache_path();
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
				if (!Engine::get_singleton()->is_editor_hint()) {
					shader_cache_dir = String(); // disable only if not editor
				}
				if (!shader_cache_dir.is_empty()) {
					ShaderGLES3::set_shader_cache_dir(shader_cache_dir);
				}
			}
		}
	}

	// OpenGL needs to be initialized before initializing the Rasterizers
	config = memnew(GLES3::Config);
	GLES3::Utilities::initialize();
	texture_storage = memnew(GLES3::TextureStorage);
	material_storage = memnew(GLES3::MaterialStorage);
	mesh_storage = memnew(GLES3::MeshStorage);
	particles_storage = memnew(GLES3::ParticlesStorage);
	light_storage = memnew(GLES3::LightStorage);
	copy_effects = memnew(GLES3::CopyEffects);
	cubemap_filter = memnew(GLES3::CubemapFilter);
	glow = memnew(GLES3::Glow);
	post_effects = memnew(GLES3::PostEffects);
	feed_effects = memnew(GLES3::FeedEffects);
	gi = memnew(GLES3::GI);
	fog = memnew(GLES3::Fog);

    canvas = memnew(RasterizerCanvasGLES3(mesh_storage, texture_storage, material_storage));
    scene = memnew(RasterizerSceneGLES3(mesh_storage, texture_storage, material_storage, light_storage));
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	texture_storage->_tex_blit_shader_initialize();

	// Disable OpenGL linear to sRGB conversion, because Godot will always do this conversion
	// itself.
	if (config->srgb_framebuffer_supported) {
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}

void RasterizerGLES3::finalize()
{
	// Has to be a separate call due to TextureStorage & MaterialStorage needing to interact for
	// TexBlit Shaders
	texture_storage->_tex_blit_shader_free();
	memdelete(scene);
	memdelete(canvas);
	memdelete(gi);
	memdelete(fog);
	memdelete(post_effects);
	memdelete(glow);
	memdelete(cubemap_filter);
	memdelete(copy_effects);
	memdelete(feed_effects);
	memdelete(light_storage);
	memdelete(particles_storage);
	memdelete(mesh_storage);
	memdelete(material_storage);
	memdelete(texture_storage);
	memdelete(config);
}

void RasterizerGLES3::make_current(bool p_gles_over_gl)
{
	RasterizerUtilGLES3::set_gles_over_gl(p_gles_over_gl);
	OS::get_singleton()->set_gles_over_gl(p_gles_over_gl);

	RendererCompositor::_create_func = _create_current;
	RendererCompositor::low_end = true;
	RendererCompositor::bind_compositor<RasterizerGLES3>();
}

void RasterizerGLES3::_blit_render_target_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen& p_blit, bool p_first)
{
	GLES3::RenderTarget* rt =
		GLES3::TextureStorage::get_singleton()->get_render_target(p_blit.render_target);

	ERR_FAIL_NULL(rt);

	// We normally render to the render target upside down, so flip Y when blitting to the screen.
	bool flip_y = true;
	bool linear_to_srgb = false;
	if (rt->overridden.color.is_valid()) {
		// If we've overridden the render target's color texture, that means we
		// didn't render upside down, so we don't need to flip it.
		// We're probably rendering directly to an XR device.
		flip_y = false;

		// It is 99% likely our texture uses the GL_SRGB8_ALPHA8 texture format in
		// which case we have a GPU sRGB to Linear conversion on texture read.
		// We need to counter this.
		// Unfortunately we do not have an API to check this as Godot does not
		// track this.
		linear_to_srgb = true;
	}

#ifdef WINDOWS_ENABLED
	if (screen_flipped_y) {
		flip_y = !flip_y;
	}
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

	if (p_first) {
		if (p_blit.dst_rect.position != Vector2() || p_blit.dst_rect.size != rt->size) {
			// Viewport doesn't cover entire window so clear window to black before blitting.
			// Querying the actual window size from the DisplayServer would deadlock in separate
			// render thread mode, so let's set the biggest viewport the implementation supports, to
			// be sure the window is fully covered.
			Size2i max_vp = GLES3::Utilities::get_maximum_viewport_size();
			glViewport(0, 0, max_vp[0], max_vp[1]);
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	Vector2 screen_rect_end = p_blit.dst_rect.get_end();

	Vector2 p1 = Vector2(
		p_blit.dst_rect.position.x, flip_y ? screen_rect_end.y : p_blit.dst_rect.position.y);
	Vector2 p2 =
		Vector2(screen_rect_end.x, flip_y ? p_blit.dst_rect.position.y : screen_rect_end.y);
	Vector2 size = p2 - p1;

	Rect2 screenrect = Rect2(Vector2(0.0, flip_y ? 1.0 : 0.0), Vector2(1.0, flip_y ? -1.0 : 1.0));

	glViewport(int(MIN(p1.x, p2.x)), int(MIN(p1.y, p2.y)), Math::abs(size.x), Math::abs(size.y));

	glActiveTexture(GL_TEXTURE0);
	GLenum target = rt->view_count > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	glBindTexture(target, rt->color);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	if (p_blit.lens_distortion.apply &&
		(p_blit.lens_distortion.k1 != 0.0 || p_blit.lens_distortion.k2)) {
		copy_effects->copy_with_lens_distortion(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0,
			p_blit.lens_distortion.eye_center, p_blit.lens_distortion.k1, p_blit.lens_distortion.k2,
			p_blit.lens_distortion.upscale, p_blit.lens_distortion.aspect_ratio, linear_to_srgb);
	}
	else if (rt->view_count > 1) {
		copy_effects->copy_to_rect_3d(screenrect,
			p_blit.multi_view.use_layer ? p_blit.multi_view.layer : 0, GLES3::Texture::TYPE_LAYERED,
			0.0, linear_to_srgb);
	}
	else {
		copy_effects->copy_to_rect(screenrect, linear_to_srgb);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

// is this p_screen useless in a multi window environment?
void RasterizerGLES3::blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
	const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
{
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

	Size2i win_size = DisplayServer::get_singleton()->window_get_size();

	glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);
	glViewport(0, 0, win_size.width, win_size.height);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glClearColor(p_color.r, p_color.g, p_color.b,
		OS::get_singleton()->is_layered_allowed() ? p_color.a : 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	RID texture = texture_storage->texture_allocate();
	texture_storage->texture_2d_initialize(texture, p_image);

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

	GLES3::Texture* t = texture_storage->get_texture(texture);
	t->gl_set_filter(p_use_filter ? RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR
								  : RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t->tex_id);
	copy_effects->copy_to_rect(screenrect);
	glBindTexture(GL_TEXTURE_2D, 0);

	gl_end_frame(true);

	texture_storage->texture_free(texture);
}

void RasterizerGLES3::bind_utilities()
{
    RendererUtilities::visibility_notifier_allocate = &GLES3::Utilities::visibility_notifier_allocate;
    RendererUtilities::visibility_notifier_initialize = &GLES3::Utilities::visibility_notifier_initialize;
    RendererUtilities::visibility_notifier_free = &GLES3::Utilities::visibility_notifier_free;
    RendererUtilities::visibility_notifier_set_aabb = &GLES3::Utilities::visibility_notifier_set_aabb;
    RendererUtilities::visibility_notifier_get_aabb = &GLES3::Utilities::visibility_notifier_get_aabb;
    RendererUtilities::visibility_notifier_call = &GLES3::Utilities::visibility_notifier_call;

    RendererUtilities::free = &GLES3::Utilities::free;
    RendererUtilities::get_base_type = &GLES3::Utilities::get_base_type;
    RendererUtilities::base_update_dependency = &GLES3::Utilities::base_update_dependency;

    RendererUtilities::capture_timestamps_begin = &GLES3::Utilities::capture_timestamps_begin;
    RendererUtilities::capture_timestamp = &GLES3::Utilities::capture_timestamp;
    RendererUtilities::get_captured_timestamps_count = &GLES3::Utilities::get_captured_timestamps_count;
    RendererUtilities::get_captured_timestamps_frame = &GLES3::Utilities::get_captured_timestamps_frame;
    RendererUtilities::get_captured_timestamp_gpu_time = &GLES3::Utilities::get_captured_timestamp_gpu_time;
    RendererUtilities::get_captured_timestamp_cpu_time = &GLES3::Utilities::get_captured_timestamp_cpu_time;
    RendererUtilities::get_captured_timestamp_name = &GLES3::Utilities::get_captured_timestamp_name;

    RendererUtilities::update_dirty_resources = &GLES3::Utilities::update_dirty_resources;
    RendererUtilities::set_debug_generate_wireframes = &GLES3::Utilities::set_debug_generate_wireframes;
    RendererUtilities::has_os_feature = &GLES3::Utilities::has_os_feature;
    RendererUtilities::update_memory_info = &GLES3::Utilities::update_memory_info;

    RendererUtilities::get_rendering_info = &GLES3::Utilities::get_rendering_info;
    RendererUtilities::get_video_adapter_name = &GLES3::Utilities::get_video_adapter_name;
    RendererUtilities::get_video_adapter_vendor = &GLES3::Utilities::get_video_adapter_vendor;
    RendererUtilities::get_video_adapter_type = &GLES3::Utilities::get_video_adapter_type;
    RendererUtilities::get_video_adapter_api_version = &GLES3::Utilities::get_video_adapter_api_version;

    RendererUtilities::get_maximum_viewport_size = &GLES3::Utilities::get_maximum_viewport_size;
    RendererUtilities::get_maximum_shader_varyings = &GLES3::Utilities::get_maximum_shader_varyings;
    RendererUtilities::get_maximum_uniform_buffer_size = &GLES3::Utilities::get_maximum_uniform_buffer_size;
}

#endif // GLES3_ENABLED


