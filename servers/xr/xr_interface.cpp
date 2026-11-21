/**************************************************************************/
/*  xr_interface.cpp                                                      */
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

#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/xr/xr_server.h"
#include "xr_interface.h"

bool XRInterface::is_primary()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	return XRServer::get_primary_interface() == this;
}

void XRInterface::set_primary(bool p_primary)
{
	ERR_FAIL_COND(!XRServer::is_initialized());

	if (p_primary) {
		ERR_FAIL_COND(!is_initialized());

		XRServer::set_primary_interface(this);
	}
	else if (XRServer::get_primary_interface() == this) {
		XRServer::set_primary_interface(nullptr);
	}
}

// query if this interface supports this play area mode
bool XRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// get the current play area mode
XRInterface::PlayAreaMode XRInterface::get_play_area_mode() const { return XR_PLAY_AREA_UNKNOWN; }

// change the play area mode, note that this should return false if the mode is not available
bool XRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// if available, returns an array of vectors denoting the play area the player can move around in
PackedVector3Array XRInterface::get_play_area() const
{
	// Return an empty array by default.
	// Note implementation is responsible for applying our reference frame and world scale to the
	// raw data. `play_area_changed` should be emitted if play area data is available and either the
	// reference frame or world scale changes.
	return PackedVector3Array();
}

/** these will only be implemented on AR interfaces, so we want dummies for VR **/
bool XRInterface::get_anchor_detection_is_enabled() const { return false; }

void XRInterface::set_anchor_detection_is_enabled(bool p_enable) {}

int XRInterface::get_camera_feed_id() { return 0; }

RID XRInterface::get_vrs_texture() { return RID(); }

/** these are optional, so we want dummies **/

RID XRInterface::get_color_texture() { return RID(); }

RID XRInterface::get_depth_texture() { return RID(); }

RID XRInterface::get_velocity_texture() { return RID(); }

RID XRInterface::get_velocity_depth_texture() { return RID(); }

Size2i XRInterface::get_velocity_target_size() { return Size2i(); }

Rect2i XRInterface::get_render_region() { return Rect2i(); }

PackedStringArray XRInterface::get_suggested_tracker_names() const
{
	PackedStringArray arr;

	return arr;
}

PackedStringArray XRInterface::get_suggested_pose_names(const String& p_tracker_name) const
{
	PackedStringArray arr;

	return arr;
}

XRInterface::TrackingStatus XRInterface::get_tracking_status() const { return XR_UNKNOWN_TRACKING; }

void XRInterface::trigger_haptic_pulse(const String& p_action_name,
	const String& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
}

String XRInterface::get_name() const { return String(); }

uint32_t XRInterface::get_capabilities() const { return 0; }

bool XRInterface::is_initialized() const { return true; }

bool XRInterface::initialize() { return true; }

void XRInterface::uninitialize() {}

Transform3D XRInterface::get_camera_transform() { return Transform3D(); }

void XRInterface::process() {}

Size2 XRInterface::get_render_target_size() { return Size2(); }

uint32_t XRInterface::get_view_count() { return 0; }

Transform3D XRInterface::get_transform_for_view(uint32_t p_view, const Transform3D& p_cam_transform)
{
	return Transform3D();
}

Projection XRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	return Projection();
}

Vector<RenderingServerTypes::BlitToScreen> XRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	return Vector<RenderingServerTypes::BlitToScreen>();
}

// XRVRS

XRVRS::~XRVRS()
{
	if (vrs_texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(vrs_texture);
		vrs_texture = RID();
	}
}

float XRVRS::get_vrs_min_radius() const { return vrs_min_radius; }

void XRVRS::set_vrs_min_radius(float p_vrs_min_radius)
{
	if (p_vrs_min_radius < 1.0) {
		WARN_PRINT_ONCE("VRS minimum radius can not be set below 1.0");
		vrs_min_radius = 1.0;
	}
	else if (p_vrs_min_radius > 100.0) {
		WARN_PRINT_ONCE("VRS minimum radius can not be set above 100.0");
		vrs_min_radius = 100.0;
	}
	else {
		vrs_min_radius = p_vrs_min_radius;
		vrs_dirty = true;
	}
}

float XRVRS::get_vrs_strength() const { return vrs_strength; }

void XRVRS::set_vrs_strength(float p_vrs_strength)
{
	if (p_vrs_strength < 0.1) {
		WARN_PRINT_ONCE("VRS strength can not be set below 0.1");
		vrs_strength = 0.1;
	}
	else if (p_vrs_strength > 10.0) {
		WARN_PRINT_ONCE("VRS strength can not be set above 10.0");
		vrs_strength = 10.0;
	}
	else {
		vrs_strength = p_vrs_strength;
		vrs_dirty = true;
	}
}

Rect2i XRVRS::get_vrs_render_region() const { return vrs_render_region; }

void XRVRS::set_vrs_render_region(const Rect2i& p_vrs_render_region)
{
	vrs_render_region = p_vrs_render_region;
	vrs_dirty = true;
}

RID XRVRS::make_vrs_texture(const Size2& p_target_size, const Vector<Vector2>& p_eye_foci)
{
	ERR_FAIL_COND_V(p_eye_foci.is_empty(), RID());

	Size2i texel_size = RD::vrs_get_texel_size();

	// Should return sensible data or graphics API does not support VRS.
	ERR_FAIL_COND_V(texel_size.x < 1 || texel_size.y < 1, RID());

	Size2 vrs_size =
		Size2(0.5 + p_target_size.x / texel_size.x, 0.5 + p_target_size.y / texel_size.y).floor();

	// Make sure we have at least one pixel.
	vrs_size = vrs_size.maxf(1.0);

	float max_radius =
		0.5 * MIN(vrs_size.x, vrs_size.y); // Maximum radius that fits inside of our image
	float min_radius =
		vrs_min_radius * max_radius / 100.0; // Minimum radius as a percentage of our size
	real_t outer_radius = MAX(1.0, (max_radius - min_radius) / vrs_strength);
	Size2 vrs_sizei = vrs_size;

	// Our density map is now unified, with a value of (0.0, 0.0) meaning a 1x1 texel size and
	// (1.0, 1.0) an max texel size. For our standard VRS extension on Vulkan this means a maximum
	// of 8x8. For the density map extension this scales depending on the max texel size.

	if (target_size != vrs_sizei || eye_foci != p_eye_foci || vrs_dirty) {
		// Out with the old.
		if (vrs_texture.is_valid()) {
			Renderer::free_rid(vrs_texture);
			vrs_texture = RID();
		}

		// In with the new.
		Vector<Ref<Image>> images;
		target_size = vrs_sizei;
		eye_foci = p_eye_foci;

		Size2 region_ratio = Size2(1.0, 1.0);
		Point2i region_offset;
		if (vrs_render_region != Rect2i()) {
			region_ratio = (Size2)vrs_render_region.size / p_target_size;
			region_offset = (Point2)vrs_render_region.position / p_target_size * vrs_sizei;
		}

		for (int i = 0; i < eye_foci.size() && i < RendererSceneRender::MAX_RENDER_VIEWS; i++) {
			PackedByteArray data;
			data.resize(vrs_sizei.x * vrs_sizei.y * 2);
			uint8_t* data_ptr = data.ptrw();

			Vector2i view_center;
			view_center.x =
				int(vrs_size.x * (eye_foci[i].x + 1.0) * region_ratio.x * 0.5) + region_offset.x;
			view_center.y =
				int(vrs_size.y * (-eye_foci[i].y + 1.0) * region_ratio.y * 0.5) + region_offset.y;

			int d = 0;
			for (int y = 0; y < vrs_sizei.y; y++) {
				for (int x = 0; x < vrs_sizei.x; x++) {
					Vector2 offset = Vector2(x - view_center.x, y - view_center.y) / region_ratio;
					real_t density = MAX(offset.length() - min_radius, 0.0) / outer_radius;
					data_ptr[d++] = CLAMP(255.0 * density, 0, 255);
					data_ptr[d++] = CLAMP(255.0 * density, 0, 255);
				}
			}
			images.push_back(
				Image::create_from_data(vrs_sizei.x, vrs_sizei.y, false, Image::FORMAT_RG8, data));
		}

		if (images.size() == 1) {
			vrs_texture = Renderer::texture_2d_create(images[0]);
		}
		else {
			vrs_texture = Renderer::texture_2d_layered_create(
				images, RSE::TEXTURE_LAYERED_2D_ARRAY);
		}

		vrs_dirty = false;
	}

	return vrs_texture;
}


