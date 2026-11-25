/**************************************************************************/
/*  openxr_platform_inc.h                                                 */
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

// In various places we need to include platform definitions but we can't
// include these in our normal header files as we'll end up with issues.

// IWYU pragma: begin_keep

#ifdef VULKAN_ENABLED
#include "servers/rendering/renderer.h"
#endif

#ifdef X11_ENABLED
#define Window OpenXRX11Window
#define Font OpenXRX11Font
#define Time OpenXRX11Time
#endif

#ifdef VULKAN_ENABLED
#define XR_USE_GRAPHICS_API_VULKAN
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif // VULKAN_ENABLED

#ifdef METAL_ENABLED
#define XR_USE_GRAPHICS_API_METAL
#include "servers/rendering/renderer.h"
#endif // METAL_ENABLED

#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
#ifdef ANDROID_ENABLED
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#define XR_USE_GRAPHICS_API_OPENGL
#endif // ANDROID_ENABLED
#if defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED)
#ifdef GLAD_ENABLED
#include <thirdparty/glad/glad/egl.h>
#else
#include <EGL/egl.h>
#endif // GLAD_ENABLED
#endif // defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED)
#ifdef X11_ENABLED
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <thirdparty/glad/glad/gl.h>
#include <thirdparty/glad/glad/glx.h>
#endif // X11_ENABLED
#endif // defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)

#ifdef D3D12_ENABLED
#define XR_USE_GRAPHICS_API_D3D12
#include "servers/rendering/renderer.h"
#endif // D3D12_ENABLED

#ifdef X11_ENABLED
#include <X11/Xlib.h>
#endif // X11_ENABLED

#ifdef WINDOWS_ENABLED
#define COM_NO_WINDOWS_H
#include <objbase.h>
#include <unknwn.h> // codespell:ignore unknwn
#endif // WINDOWS_ENABLED

#ifdef ANDROID_ENABLED
// The jobject type from jni.h is used by openxr_platform.h on Android.
#include <jni.h>
#endif // ANDROID_ENABLED

// Include platform dependent structs.
#include <openxr/openxr_platform.h>

#ifdef X11_ENABLED
#undef Time
#undef Font
#undef Window
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Always
#undef Always
#endif
#endif // X11_ENABLED

// IWYU pragma: end_keep

// --- Folded from modules/openxr/extensions/platform/openxr_android_extension.h ---
#if defined(ANDROID_ENABLED)
#include "servers/rendering/renderer.h"

#include <jni.h>

class OpenXRAndroidExtension final
{
public:

	static HashMap<String, bool *> get_requested_extensions(XrVersion p_version);
	static void on_before_instance_created();
	static void *set_instance_create_info_and_get_next_pointer(XrVersion p_xr_version, void *p_next_pointer);

private:

	// Initialize the loader
	EXT_PROTO_XRRESULT_FUNC1(xrInitializeLoaderKHR, (const XrLoaderInitInfoBaseHeaderKHR *), loaderInitInfo)

public:
	struct Data
	{
		JavaVM *vm;
		jobject activity_object;
		bool loader_init_extension_available = false;
		bool loader_init_android_extension_available = false;
		bool create_instance_extension_available = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRAndroidExtension() = delete;
	OpenXRAndroidExtension(const OpenXRAndroidExtension&) = delete;
	OpenXRAndroidExtension& operator=(const OpenXRAndroidExtension&) = delete;
	~OpenXRAndroidExtension() = delete;
};
#endif // defined(ANDROID_ENABLED)
// --- End modules/openxr/extensions/platform/openxr_android_extension.h ---

// --- Folded from modules/openxr/extensions/platform/openxr_d3d12_extension.h ---
#if defined(D3D12_ENABLED)
#ifdef D3D12_ENABLED

#include "servers/rendering/renderer.h"

#include "core/string/ustring.h"
#include "servers/rendering/renderer.h"

// Always include this as late as possible.


class OpenXRD3D12Extension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void* set_session_create_and_get_next_pointer(void* p_next_pointer);

	static D3D_FEATURE_LEVEL get_feature_level();
	static LUID get_adapter_luid();
	static void set_device(ID3D12Device* p_device);
	static void set_command_queue(ID3D12CommandQueue* p_queue);
	static void cleanup_device();

	static void get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains);
	static void get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format,
		uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size,
		void** r_swapchain_graphics_data);
	static void cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data);
	static bool create_projection_fov(
		const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix);
	static RID get_texture(void* p_swapchain_graphics_data, int p_image_index);
	static RID get_density_map(void* p_swapchain_graphics_data, int p_image_index)
	{
		return RID();
	}

private:

	static XrGraphicsBindingD3D12KHR graphics_binding_d3d12;

	struct SwapchainGraphicsData {
		bool is_multiview;
		Vector<RID> texture_rids;
	};

	EXT_PROTO_XRRESULT_FUNC3(xrGetD3D12GraphicsRequirementsKHR, (XrInstance), p_instance, (XrSystemId), p_system_id, (XrGraphicsRequirementsD3D12KHR *), p_graphics_requirements)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t), p_image_capacity_input, (uint32_t *), p_image_count_output, (XrSwapchainImageBaseHeader *), p_images)

public:
	struct Data
	{
		Microsoft::WRL::ComPtr<ID3D12Device> graphics_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRD3D12Extension() = delete;
	OpenXRD3D12Extension(const OpenXRD3D12Extension&) = delete;
	OpenXRD3D12Extension& operator=(const OpenXRD3D12Extension&) = delete;
	~OpenXRD3D12Extension() = delete;
};

#endif // D3D12_ENABLED
#endif // defined(D3D12_ENABLED)
// --- End modules/openxr/extensions/platform/openxr_d3d12_extension.h ---

// --- Folded from modules/openxr/extensions/platform/openxr_metal_extension.h ---
#if defined(METAL_ENABLED)
#include "servers/rendering/renderer.h"

#include "core/string/ustring.h"

// Always include this as late as possible.


class OpenXRMetalExtension : public OpenXRGraphicsExtensionWrapper {
public:
	virtual HashMap<String, bool *> get_requested_extensions(XrVersion p_version) override;

	virtual void on_instance_created(const XrInstance p_instance) override;
	virtual void *set_session_create_and_get_next_pointer(void *p_next_pointer) override;

	virtual void get_usable_swapchain_formats(Vector<int64_t> &p_usable_swap_chains) override;
	virtual void get_usable_depth_formats(Vector<int64_t> &p_usable_swap_chains) override;
	virtual String get_swapchain_format_name(int64_t p_swapchain_format) const override;
	virtual bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, void **r_swapchain_graphics_data) override;
	virtual void cleanup_swapchain_graphics_data(void **p_swapchain_graphics_data) override;
	virtual bool create_projection_fov(const XrFovf p_fov, double p_z_near, double p_z_far, Projection &r_camera_matrix) override;
	virtual RID get_texture(void *p_swapchain_graphics_data, int p_image_index) override;
	virtual RID get_density_map(void *p_swapchain_graphics_data, int p_image_index) override { return RID(); }

private:
	static XrGraphicsBindingMetalKHR graphics_binding_metal;

	struct SwapchainGraphicsData {
		bool is_multiview;
		Vector<RID> texture_rids;
	};

	bool check_graphics_api_support();

	EXT_PROTO_XRRESULT_FUNC3(xrGetMetalGraphicsRequirementsKHR, (XrInstance), p_instance, (XrSystemId), p_system_id, (XrGraphicsRequirementsMetalKHR *), p_graphics_requirements)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t), p_image_capacity_input, (uint32_t *), p_image_count_output, (XrSwapchainImageBaseHeader *), p_images)
};
#endif // defined(METAL_ENABLED)
// --- End modules/openxr/extensions/platform/openxr_metal_extension.h ---
