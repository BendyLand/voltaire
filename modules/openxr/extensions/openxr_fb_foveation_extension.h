/**************************************************************************/
/*  openxr_fb_foveation_extension.h                                       */
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

// This extension implements the FB Foveation extension.
// This is an extension Meta added due to VRS being unavailable on Android.
// Other Android based devices are implementing this as well, see:
// https://github.khronos.org/OpenXR-Inventory/extension_support.html#XR_FB_foveation

#include "../util.h"
#include "core/types.h"
#include "openxr_fb_update_swapchain_extension.h"

// Always include this as late as possible.
#include "../openxr_platform_inc.h" // IWYU pragma: keep.

class OpenXRFBFoveationExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	static void* set_swapchain_create_info_and_get_next_pointer(void* p_next_pointer);

	static void on_main_swapchains_created();

	static bool is_enabled();

	static XrFoveationLevelFB get_foveation_level();
	static void set_foveation_level(XrFoveationLevelFB p_foveation_level);

	static XrFoveationDynamicFB get_foveation_dynamic();
	static void set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic);

	static bool is_foveation_eye_tracked_enabled();
	static void get_fragment_density_offsets(LocalVector<Vector2i>& r_offsets);

	static void set_foveation_with_subsampled_images_enabled(bool p_enabled);
	static bool is_foveation_with_subsampled_images_enabled();

	static void set_foveation_with_subsampled_images_active(bool p_active);

private:

	static void _update_profile_rt();

	static void update_profile();


	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC3(xrCreateFoveationProfileFB, (XrSession), session,
		(const XrFoveationProfileCreateInfoFB*), create_info, (XrFoveationProfileFB*), profile);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyFoveationProfileFB, (XrFoveationProfileFB), profile);
	EXT_PROTO_XRRESULT_FUNC2(xrGetFoveationEyeTrackedStateMETA, (XrSession), session,
		(XrFoveationEyeTrackedStateMETA*), foveationState);

public:
	struct Data
	{
#ifdef VULKAN_ENABLED
		XrVulkanSwapchainCreateInfoMETA meta_vulkan_swapchain_create_info;
#endif
		String rendering_driver;
		bool fb_foveation_ext = false;
		bool fb_foveation_configuration_ext = false;
		bool fb_foveation_vulkan_ext = false;
		bool meta_foveation_eye_tracked_ext = false;
		bool meta_vulkan_swapchain_create_info_ext = false;
		XrFoveationLevelFB foveation_level = XR_FOVEATION_LEVEL_NONE_FB;
		XrFoveationDynamicFB foveation_dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
		bool foveation_with_subsampled_images_enabled = false;
		bool foveation_with_subsampled_images_active = false;
		XrSwapchainCreateInfoFoveationFB swapchain_create_info_foveation_fb;
		XrSystemFoveationEyeTrackedPropertiesMETA meta_foveation_eye_tracked_properties;
		XrFoveationEyeTrackedProfileCreateInfoMETA meta_foveation_eye_tracked_create_info;
	};

	static inline Data* data = nullptr;

	static void initialize(const String& p_rendering_driver = String());
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFBFoveationExtension() = delete;
	OpenXRFBFoveationExtension(const OpenXRFBFoveationExtension&) = delete;
	OpenXRFBFoveationExtension& operator=(const OpenXRFBFoveationExtension&) = delete;
	~OpenXRFBFoveationExtension() = delete;
};


