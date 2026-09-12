/**************************************************************************/
/*  openxr_frame_synthesis_extension.cpp                                  */
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

#include "core/config/project_settings.h"
#include "openxr_frame_synthesis_extension.h"
#include "servers/rendering/rendering_server.h"
#include "servers/xr/xr_server.h"

#define GL_RGBA16F 0x881A
#define GL_DEPTH24_STENCIL8 0x88F0

#define VK_FORMAT_R16G16B16A16_SFLOAT 97
#define VK_FORMAT_D24_UNORM_S8_UINT 129

OpenXRFrameSynthesisExtension* OpenXRFrameSynthesisExtension::singleton = nullptr;

OpenXRFrameSynthesisExtension* OpenXRFrameSynthesisExtension::get_singleton() { return singleton; }


OpenXRFrameSynthesisExtension::OpenXRFrameSynthesisExtension() { singleton = this; }

OpenXRFrameSynthesisExtension::~OpenXRFrameSynthesisExtension() { singleton = nullptr; }

void OpenXRFrameSynthesisExtension::prepare_view_configuration(uint32_t p_view_count)
{
	if (!frame_synthesis_ext) {
		return;
	}

	// Called during initialization, we can safely change this.
	render_state.config_views.resize(p_view_count);

	for (XrFrameSynthesisConfigViewEXT& config_view : render_state.config_views) {
		config_view.type = XR_TYPE_FRAME_SYNTHESIS_CONFIG_VIEW_EXT;
		config_view.next = nullptr;

		// These will be set by xrEnumerateViewConfigurationViews.
		config_view.recommendedMotionVectorImageRectWidth = 0;
		config_view.recommendedMotionVectorImageRectHeight = 0;
	}
}

void* OpenXRFrameSynthesisExtension::set_view_configuration_and_get_next_pointer(
	uint32_t p_view, void* p_next_pointer)
{
	if (!frame_synthesis_ext) {
		return nullptr;
	}

	// Called during initialization, we can safely access this.
	ERR_FAIL_UNSIGNED_INDEX_V(p_view, render_state.config_views.size(), nullptr);

	XrFrameSynthesisConfigViewEXT& config_view = render_state.config_views[p_view];
	config_view.next = p_next_pointer;

	return &config_view;
}

void OpenXRFrameSynthesisExtension::on_session_destroyed()
{
	if (!frame_synthesis_ext) {
		return;
	}

	// Free our swapchains.
	free_swapchains();
}

void OpenXRFrameSynthesisExtension::on_pre_draw_viewport(RID p_render_target)
{
	if (!frame_synthesis_ext) {
		return;
	}

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	if (!enabled || render_state.config_views.size() != 2 ||
		render_state.frame_synthesis_info.size() != 2 || render_state.skip_next_frame) {
		// Unset these just in case.
		openxr_api->set_velocity_texture(RID());
		openxr_api->set_velocity_depth_texture(RID());

		// Remember our transform just in case we (re)start frame synthesis later on.
		render_state.previous_transform = XRServer::get_singleton()->get_world_origin();

		return;
	}

	// Acquire our swapchains.
	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		bool should_render = true;
		render_state.swapchains[i].acquire(should_render);
	}

	// Set our images.
	openxr_api->set_velocity_texture(render_state.swapchains[SWAPCHAIN_MOTION_VECTOR].get_image());
	openxr_api->set_velocity_depth_texture(render_state.swapchains[SWAPCHAIN_DEPTH].get_image());

	// Set our size.
	uint32_t width = render_state.config_views[0].recommendedMotionVectorImageRectWidth;
	uint32_t height = render_state.config_views[0].recommendedMotionVectorImageRectHeight;
	openxr_api->set_velocity_target_size(Size2i(width, height));

	// Get our head motion
	Transform3D world_transform = XRServer::get_singleton()->get_world_origin();
	Transform3D delta_transform =
		render_state.previous_transform.affine_inverse() * world_transform;
	Quaternion delta_quat = delta_transform.basis.get_quaternion();
	Vector3 delta_origin = delta_transform.origin;

	float z_far = openxr_api->get_render_state_z_far();
	float z_near = openxr_api->get_render_state_z_near();

	// Z near/far can change per frame, so make sure we update this.
	for (XrFrameSynthesisInfoEXT& frame_synthesis_info : render_state.frame_synthesis_info) {
		frame_synthesis_info.layerFlags =
			render_state.relax_frame_interval
				? XR_FRAME_SYNTHESIS_INFO_REQUEST_RELAXED_FRAME_INTERVAL_BIT_EXT
				: 0;

		frame_synthesis_info.appSpaceDeltaPose = {
			{(float)delta_quat.x, (float)delta_quat.y, (float)delta_quat.z, (float)delta_quat.w},
			{(float)delta_origin.x, (float)delta_origin.y, (float)delta_origin.z}};

		// Ensure we only use valid near/far Z values.
		if (z_far != z_near) {
			// Note: reverse-Z.
			frame_synthesis_info.nearZ = z_far;
			frame_synthesis_info.farZ = z_near;
		}
	}

	// Remember our transform.
	render_state.previous_transform = world_transform;
}

void OpenXRFrameSynthesisExtension::on_post_draw_viewport(RID p_render_target)
{
	// Check if our extension is supported and enabled.
	if (!frame_synthesis_ext || !enabled || render_state.config_views.size() != 2 ||
		render_state.frame_synthesis_info.size() != 2 || render_state.skip_next_frame) {
		return;
	}

	// Release our swapchains.
	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		render_state.swapchains[i].release();
	}
}

void* OpenXRFrameSynthesisExtension::set_projection_views_and_get_next_pointer(
	int p_view_index, void* p_next_pointer)
{
	// Check if our extension is supported and enabled.
	if (!frame_synthesis_ext || !enabled || render_state.config_views.size() != 2 ||
		render_state.frame_synthesis_info.size() != 2) {
		return nullptr;
	}

	// Did we skip this frame?
	if (render_state.skip_next_frame) {
		// Only unset when we've handled both eyes.
		if (p_view_index == 1) {
			render_state.skip_next_frame = false;
		}
		return nullptr;
	}

	render_state.frame_synthesis_info[p_view_index].next = p_next_pointer;
	return &render_state.frame_synthesis_info[p_view_index];
}

bool OpenXRFrameSynthesisExtension::is_available() const { return frame_synthesis_ext; }

bool OpenXRFrameSynthesisExtension::is_enabled() const { return frame_synthesis_ext && enabled; }

bool OpenXRFrameSynthesisExtension::get_relax_frame_interval() const
{
	return relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::_set_render_state_enabled_rt(bool p_enabled)
{
	render_state.enabled = p_enabled;
}

void OpenXRFrameSynthesisExtension::_set_relax_frame_interval_rt(bool p_relax_frame_interval)
{
	render_state.relax_frame_interval = p_relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::free_swapchains()
{
	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		render_state.swapchains[i].queue_free();
	}
}

void OpenXRFrameSynthesisExtension::_set_skip_next_frame_rt()
{
	render_state.skip_next_frame = true;
}


