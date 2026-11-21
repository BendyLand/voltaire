/**************************************************************************/
/*  xr_server.cpp                                                         */
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

#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_tracker.h"
// Inlined servers/xr/xr_server.compat.inc
#ifndef DISABLE_DEPRECATED

#include "xr_server.h"

void XRServer::_add_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker)
{
	add_tracker(p_tracker);
}

void XRServer::_remove_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker)
{
	remove_tracker(p_tracker);
}

Ref<XRTracker> XRServer::_get_tracker_bind_compat_90645(const String& p_name)
{
	return get_tracker(p_name);
}

void XRServer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED



#include "xr_server.h"

XRServer::XRMode XRServer::xr_mode = XRMODE_DEFAULT;

void XRServer::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void XRServer::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

XRServer::XRMode XRServer::get_xr_mode() { return xr_mode; }

void XRServer::set_xr_mode(XRServer::XRMode p_mode) { xr_mode = p_mode; }

double XRServer::get_world_scale()
{
	ERR_FAIL_NULL_V(data, 1.0);
	if (Renderer::data && Renderer::is_on_render_thread()) {
		// Return the value with which we're currently rendering,
		// if we're on the render thread
		return data->render_state.world_scale;
	}
	else {
		// Return our current value
		return data->world_scale;
	}
}

void XRServer::set_world_scale(double p_world_scale)
{
	ERR_FAIL_NULL(data);
	if (p_world_scale < 0.01) {
		p_world_scale = 0.01;
	}
	else if (p_world_scale > 1000.0) {
		p_world_scale = 1000.0;
	}

	data->world_scale = p_world_scale;
	set_render_world_scale(data->world_scale);
}

void XRServer::_set_render_world_scale(double p_world_scale)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_scale = p_world_scale;
}

void XRServer::_set_render_world_origin(const Transform3D& p_world_origin)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_origin = p_world_origin;
}

void XRServer::_set_render_reference_frame(const Transform3D& p_reference_frame)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.reference_frame = p_reference_frame;
}

Transform3D XRServer::get_reference_frame() { return Transform3D(); }

void XRServer::set_camera_locked_to_origin(bool p_enable)
{
	ERR_FAIL_NULL(data);
	data->camera_locked_to_origin = p_enable;
}

Ref<XRInterface> XRServer::get_primary_interface()
{
	Ref<XRInterface> xri = memnew(XRInterface);
	return xri;
}

void XRServer::remove_tracker(const Ref<XRTracker>& p_tracker) {}

void XRServer::set_primary_interface(const Ref<XRInterface>& p_primary_interface) {}

void XRServer::add_tracker(const Ref<XRTracker>& p_tracker) {}

Ref<XRTracker> XRServer::get_tracker(const String& p_name)
{
	Ref<XRTracker> xpt = memnew(XRTracker);
	return xpt;
}

void XRServer::set_render_world_scale(double p_scale) {}

void XRServer::remove_interface(const Ref<XRInterface>& p_interface) {}

Transform3D XRServer::get_world_origin() { return Transform3D(); }

void XRServer::set_world_origin(const Transform3D& p_world_origin) {}

void XRServer::_process() {}

void XRServer::clear_reference_frame() {}

void XRServer::add_interface(const Ref<XRInterface>& p_interface) {}

// ============================================================================
// Merged from servers/xr/xr_interface.cpp
// ============================================================================

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



// ============================================================================
// Merged from servers/xr/xr_tracker.cpp
// ============================================================================

#include "xr_tracker.h"

// XRPose

void XRPose::set_has_tracking_data(const bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRPose::get_has_tracking_data() const { return has_tracking_data; }

void XRPose::set_name(const String& p_name) { name = p_name; }

String XRPose::get_name() const { return name; }

void XRPose::set_transform(const Transform3D p_transform) { transform = p_transform; }

Transform3D XRPose::get_transform() const { return transform; }

Transform3D XRPose::get_adjusted_transform() const
{
	Transform3D adjusted_transform = transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform);

	// apply world scale
	adjusted_transform.origin *= XRServer::get_world_scale();

	// apply reference frame
	adjusted_transform = XRServer::get_reference_frame() * adjusted_transform;

	return adjusted_transform;
}

void XRPose::set_linear_velocity(const Vector3 p_velocity) { linear_velocity = p_velocity; }

Vector3 XRPose::get_linear_velocity() const { return linear_velocity; }

void XRPose::set_angular_velocity(const Vector3 p_velocity) { angular_velocity = p_velocity; }

Vector3 XRPose::get_angular_velocity() const { return angular_velocity; }

void XRPose::set_tracking_confidence(const XRPose::TrackingConfidence p_tracking_confidence)
{
	tracking_confidence = p_tracking_confidence;
}

XRPose::TrackingConfidence XRPose::get_tracking_confidence() const { return tracking_confidence; }

// XRTracker

XRTracker::BodyData* XRTracker::_ensure_body_data()
{
	if (!body_data) {
		body_data = memnew(BodyData);
	}
	return body_data;
}

XRTracker::HandData* XRTracker::_ensure_hand_data()
{
	if (!hand_data) {
		hand_data = memnew(HandData);
	}
	return hand_data;
}

void XRTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	type = p_type;
	if (type == XRServer::TRACKER_BODY) {
		_ensure_body_data();
	}
	else if (type == XRServer::TRACKER_HAND) {
		_ensure_hand_data();
		if (tracker_hand == TRACKER_HAND_UNKNOWN) {
			tracker_hand = TRACKER_HAND_LEFT;
		}
	}
}

XRServer::TrackerType XRTracker::get_tracker_type() const { return type; }

void XRTracker::set_tracker_name(const String& p_name)
{
	// Note: this should not be changed after the tracker is registered with the XRServer!
	name = p_name;
}

String XRTracker::get_tracker_name() const { return name; }

void XRTracker::set_tracker_desc(const String& p_desc) { description = p_desc; }

String XRTracker::get_tracker_desc() const { return description; }

void XRTracker::set_tracker_profile(const String& p_profile) { profile = p_profile; }

String XRTracker::get_tracker_profile() const { return profile; }

XRTracker::TrackerHand XRTracker::get_tracker_hand() const { return tracker_hand; }

void XRTracker::set_tracker_hand(const XRTracker::TrackerHand p_hand)
{
	ERR_FAIL_INDEX(p_hand, TRACKER_HAND_MAX);
	if (type == XRServer::TRACKER_BODY) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_UNKNOWN, "Body tracker cannot specify hand.");
	}
	else if (type == XRServer::TRACKER_HAND) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_LEFT && p_hand != TRACKER_HAND_RIGHT,
			"Hand tracker must specify hand.");
	}
	tracker_hand = p_hand;
}

void XRTracker::set_has_tracking_data(bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRTracker::get_has_tracking_data() const { return has_tracking_data; }

bool XRTracker::has_pose(const String& p_action_name) const
{
	return poses.has(p_action_name);
}

Ref<XRPose> XRTracker::get_pose(const String& p_action_name) const
{
	Ref<XRPose> pose;
	if (poses.has(p_action_name)) {
		pose = poses[p_action_name];
	}
	return pose;
}

void XRTracker::invalidate_pose(const String& p_action_name)
{
	if (poses.has(p_action_name)) {
		poses[p_action_name]->set_has_tracking_data(false);
	}
}

void XRTracker::set_pose(const String& p_action_name, const Transform3D& p_transform,
	const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
	const XRPose::TrackingConfidence p_tracking_confidence)
{
	Ref<XRPose> new_pose;
	if (poses.has(p_action_name)) {
		new_pose = poses[p_action_name];
	}
	else {
		new_pose.instantiate();
		new_pose->set_name(p_action_name);
		poses[p_action_name] = new_pose;
	}

	new_pose->set_has_tracking_data(p_tracking_confidence != XRPose::XR_TRACKING_CONFIDENCE_NONE);
	new_pose->set_transform(p_transform);
	new_pose->set_linear_velocity(p_linear_velocity);
	new_pose->set_angular_velocity(p_angular_velocity);
	new_pose->set_tracking_confidence(p_tracking_confidence);
}

void XRTracker::set_input(const String& p_action_name, bool p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_BOOL;
	val.bool_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, float p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_FLOAT;
	val.float_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, const Vector2& p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_VECTOR2;
	val.vec2_val.x = p_value.x;
	val.vec2_val.y = p_value.y;
	inputs[p_action_name] = val;
}

XRTracker::InputValue XRTracker::get_input(const String& p_action_name) const
{
	if (inputs.has(p_action_name)) {
		return inputs[p_action_name];
	}
	return InputValue();
}

bool XRTracker::is_button_pressed(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return false;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val > 0.0f;
	}
	return false;
}

float XRTracker::get_input_float(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return 0.0f;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val ? 1.0f : 0.0f;
	}
	return 0.0f;
}

Vector2 XRTracker::get_input_vector2(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (val && val->type == InputValue::INPUT_VECTOR2) {
		return Vector2(val->vec2_val.x, val->vec2_val.y);
	}
	return Vector2();
}

void XRTracker::set_body_flags(uint32_t p_body_flags)
{
	_ensure_body_data()->body_flags = p_body_flags;
}

uint32_t XRTracker::get_body_flags() const
{
	return body_data ? body_data->body_flags : 0;
}

void XRTracker::set_joint_flags(Joint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_joint_flags(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, uint32_t());
	return body_data ? body_data->joint_flags[p_joint] : 0;
}

void XRTracker::set_joint_transform(Joint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_joint_transform(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, Transform3D());
	return body_data ? body_data->joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_tracking_source(HandTrackingSource p_source)
{
	_ensure_hand_data()->hand_tracking_source = p_source;
}

XRTracker::HandTrackingSource XRTracker::get_hand_tracking_source() const
{
	return hand_data ? hand_data->hand_tracking_source : HAND_TRACKING_SOURCE_UNKNOWN;
}

void XRTracker::set_hand_joint_flags(HandJoint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_hand_joint_flags(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, uint32_t());
	return hand_data ? hand_data->hand_joint_flags[p_joint] : 0;
}

void XRTracker::set_hand_joint_transform(HandJoint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_hand_joint_transform(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Transform3D());
	return hand_data ? hand_data->hand_joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_joint_radius(HandJoint p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_radii[p_joint] = p_radius;
}

float XRTracker::get_hand_joint_radius(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, 0.0f);
	return hand_data ? hand_data->hand_joint_radii[p_joint] : 0.0f;
}

void XRTracker::set_hand_joint_linear_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_linear_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_linear_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_linear_velocities[p_joint] : Vector3();
}

void XRTracker::set_hand_joint_angular_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_angular_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_angular_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_angular_velocities[p_joint] : Vector3();
}

float XRTracker::get_blend_shape(BlendShapeEntry p_blend_shape) const
{
	ERR_FAIL_INDEX_V(p_blend_shape, FT_MAX, 0.0f);
	if (blend_shapes.size() != FT_MAX) {
		return 0.0f;
	}
	return blend_shapes[p_blend_shape];
}

void XRTracker::set_blend_shape(BlendShapeEntry p_blend_shape, float p_value)
{
	ERR_FAIL_INDEX(p_blend_shape, FT_MAX);
	if (blend_shapes.size() != FT_MAX) {
		blend_shapes.resize_initialized(FT_MAX);
	}
	blend_shapes.set(p_blend_shape, p_value);
}

PackedFloat32Array XRTracker::get_blend_shapes() const
{
	if (blend_shapes.size() == FT_MAX) {
		return blend_shapes;
	}
	PackedFloat32Array data;
	data.resize_initialized(FT_MAX);
	return data;
}

void XRTracker::set_blend_shapes(const PackedFloat32Array& p_blend_shapes)
{
	ERR_FAIL_COND(p_blend_shapes.size() != FT_MAX);
	blend_shapes = p_blend_shapes;
}

XRTracker::XRTracker(XRServer::TrackerType p_type)
{
	if (p_type != XRServer::TRACKER_UNKNOWN) {
		set_tracker_type(p_type);
	}
}

XRTracker::~XRTracker()
{
	if (body_data) {
		memdelete(body_data);
		body_data = nullptr;
	}
	if (hand_data) {
		memdelete(hand_data);
		hand_data = nullptr;
	}
}
