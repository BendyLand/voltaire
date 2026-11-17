/**************************************************************************/
/*  openxr_fb_foveation_extension.cpp                                     */
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

#include "../openxr_platform_inc.h"
#include "core/config/project_settings.h"
#include "openxr_eye_gaze_interaction.h"
#include "openxr_fb_foveation_extension.h"
#include "servers/rendering/renderer.h"

void OpenXRFBFoveationExtension::initialize(const String& p_rendering_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->rendering_driver = p_rendering_driver;
}

void OpenXRFBFoveationExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


void OpenXRFBFoveationExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->fb_foveation_ext) {
		EXT_INIT_XR_FUNC(xrCreateFoveationProfileFB);
		EXT_INIT_XR_FUNC(xrDestroyFoveationProfileFB);
	}

	if (data->fb_foveation_configuration_ext) {
		// nothing to register here...
	}

	if (data->meta_foveation_eye_tracked_ext) {
		EXT_INIT_XR_FUNC(xrGetFoveationEyeTrackedStateMETA);
	}
}

void OpenXRFBFoveationExtension::on_instance_destroyed()
{
	data->fb_foveation_ext = false;
	data->fb_foveation_configuration_ext = false;
	data->meta_foveation_eye_tracked_ext = false;
}

bool OpenXRFBFoveationExtension::is_enabled()
{
	bool enabled = OpenXRFBUpdateSwapchainExtension::is_initialized() &&
				   OpenXRFBUpdateSwapchainExtension::is_enabled() && data->fb_foveation_ext &&
				   data->fb_foveation_configuration_ext;
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		enabled = enabled && data->fb_foveation_vulkan_ext;
	}
#endif // XR_USE_GRAPHICS_API_VULKAN
	return enabled;
}

void* OpenXRFBFoveationExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		data->meta_foveation_eye_tracked_properties.next = p_next_pointer;
		return &data->meta_foveation_eye_tracked_properties;
	}
#endif
	return p_next_pointer;
}

void* OpenXRFBFoveationExtension::set_swapchain_create_info_and_get_next_pointer(
	void* p_next_pointer)
{
	void* next = p_next_pointer;
	if (is_enabled() && data->foveation_level > 0) {
		data->swapchain_create_info_foveation_fb.next = next;
		next = &data->swapchain_create_info_foveation_fb;

#ifdef VULKAN_ENABLED
		if (data->meta_vulkan_swapchain_create_info_ext) {
			data->meta_vulkan_swapchain_create_info.additionalCreateFlags = 0;
			if (data->meta_foveation_eye_tracked_ext &&
				data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
			}
			if (data->foveation_with_subsampled_images_enabled &&
				data->foveation_with_subsampled_images_active) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
			}

			data->meta_vulkan_swapchain_create_info.next = next;
			next = &data->meta_vulkan_swapchain_create_info;
		}
#endif
	}

	return next;
}

void OpenXRFBFoveationExtension::on_main_swapchains_created() { update_profile(); }

XrFoveationLevelFB OpenXRFBFoveationExtension::get_foveation_level()
{
	return data ? data->foveation_level : XR_FOVEATION_LEVEL_NONE_FB;
}

void OpenXRFBFoveationExtension::set_foveation_level(XrFoveationLevelFB p_foveation_level)
{
	data->foveation_level = p_foveation_level;

	// Update profile will do nothing if we're not yet initialized.
	update_profile();
}

XrFoveationDynamicFB OpenXRFBFoveationExtension::get_foveation_dynamic()
{
	return data ? data->foveation_dynamic : XR_FOVEATION_DYNAMIC_DISABLED_FB;
}

void OpenXRFBFoveationExtension::set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic)
{
	data->foveation_dynamic = p_foveation_dynamic;

	// Update profile will do nothing if we're not yet initialized.
	update_profile();
}

bool OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled()
{
	return is_enabled() && data->meta_foveation_eye_tracked_ext &&
		   data->meta_vulkan_swapchain_create_info_ext &&
		   data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(bool p_enabled)
{
	data->foveation_with_subsampled_images_enabled = p_enabled;
}

bool OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled()
{
	return is_enabled() && data->foveation_level > 0 && data->meta_vulkan_swapchain_create_info_ext &&
		   data->foveation_with_subsampled_images_enabled;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_active(bool p_active)
{
	data->foveation_with_subsampled_images_active = p_active;
}

void OpenXRFBFoveationExtension::update_profile() {}

void OpenXRFBFoveationExtension::get_fragment_density_offsets(
	LocalVector<Vector2i, unsigned int, false, false>& r_offsets)
{
}

HashMap<String, bool*> OpenXRFBFoveationExtension::get_requested_extensions(
	unsigned long p_extension_flags)
{
	return HashMap<String, bool*>();
}


