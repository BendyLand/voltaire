/**************************************************************************/
/*  openxr_fb_foveation_extension.cpp                                     */
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

#include "../openxr_platform_inc.h"
#include "core/config/project_settings.h"
#include "openxr_eye_gaze_interaction.h"
#include "openxr_fb_foveation_extension.h"
#include "servers/rendering/rendering_server.h"

OpenXRFBFoveationExtension* OpenXRFBFoveationExtension::singleton = nullptr;

OpenXRFBFoveationExtension* OpenXRFBFoveationExtension::get_singleton() { return singleton; }

OpenXRFBFoveationExtension::~OpenXRFBFoveationExtension()
{
	singleton = nullptr;
	swapchain_update_state_ext = nullptr;
}

void OpenXRFBFoveationExtension::on_instance_created(const XrInstance p_instance)
{
	if (fb_foveation_ext) {
		EXT_INIT_XR_FUNC(xrCreateFoveationProfileFB);
		EXT_INIT_XR_FUNC(xrDestroyFoveationProfileFB);
	}

	if (fb_foveation_configuration_ext) {
		// nothing to register here...
	}

	if (meta_foveation_eye_tracked_ext) {
		EXT_INIT_XR_FUNC(xrGetFoveationEyeTrackedStateMETA);
	}
}

void OpenXRFBFoveationExtension::on_instance_destroyed()
{
	fb_foveation_ext = false;
	fb_foveation_configuration_ext = false;
	meta_foveation_eye_tracked_ext = false;
}

bool OpenXRFBFoveationExtension::is_enabled() const
{
	bool enabled = swapchain_update_state_ext != nullptr &&
				   swapchain_update_state_ext->is_enabled() && fb_foveation_ext &&
				   fb_foveation_configuration_ext;
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (rendering_driver == "vulkan") {
		enabled = enabled && fb_foveation_vulkan_ext;
	}
#endif // XR_USE_GRAPHICS_API_VULKAN
	return enabled;
}

void* OpenXRFBFoveationExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (rendering_driver == "vulkan") {
		meta_foveation_eye_tracked_properties.next = p_next_pointer;
		return &meta_foveation_eye_tracked_properties;
	}
#endif
	return p_next_pointer;
}

void* OpenXRFBFoveationExtension::set_swapchain_create_info_and_get_next_pointer(
	void* p_next_pointer)
{
	void* next = p_next_pointer;
	if (is_enabled() && foveation_level > 0) {
		swapchain_create_info_foveation_fb.next = next;
		next = &swapchain_create_info_foveation_fb;

#ifdef VULKAN_ENABLED
		if (meta_vulkan_swapchain_create_info_ext) {
			meta_vulkan_swapchain_create_info.additionalCreateFlags = 0;
			if (meta_foveation_eye_tracked_ext &&
				meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked) {
				meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
			}
			if (foveation_with_subsampled_images_enabled &&
				foveation_with_subsampled_images_active) {
				meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
			}

			meta_vulkan_swapchain_create_info.next = next;
			next = &meta_vulkan_swapchain_create_info;
		}
#endif
	}

	return next;
}

void OpenXRFBFoveationExtension::on_main_swapchains_created() { update_profile(); }

XrFoveationLevelFB OpenXRFBFoveationExtension::get_foveation_level() const
{
	return foveation_level;
}

void OpenXRFBFoveationExtension::set_foveation_level(XrFoveationLevelFB p_foveation_level)
{
	foveation_level = p_foveation_level;

	// Update profile will do nothing if we're not yet initialized.
	update_profile();
}

XrFoveationDynamicFB OpenXRFBFoveationExtension::get_foveation_dynamic() const
{
	return foveation_dynamic;
}

void OpenXRFBFoveationExtension::set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic)
{
	foveation_dynamic = p_foveation_dynamic;

	// Update profile will do nothing if we're not yet initialized.
	update_profile();
}

bool OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled() const
{
	return is_enabled() && meta_foveation_eye_tracked_ext &&
		   meta_vulkan_swapchain_create_info_ext &&
		   meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(bool p_enabled)
{
	foveation_with_subsampled_images_enabled = p_enabled;
}

bool OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled() const
{
	return is_enabled() && foveation_level > 0 && meta_vulkan_swapchain_create_info_ext &&
		   foveation_with_subsampled_images_enabled;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_active(bool p_active)
{
	foveation_with_subsampled_images_active = p_active;
}


