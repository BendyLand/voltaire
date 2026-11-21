/**************************************************************************/
/*  openxr_interface.cpp                                                  */
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

#include <openxr/openxr.h>
#include "action_map/openxr_action_map.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "extensions/openxr_extensions.h"
#include "extensions/openxr_interaction_profile_extension.h"
#include "openxr_interface.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server_types.h"

String OpenXRInterface::get_name() const { return String("OpenXR"); }

uint32_t OpenXRInterface::get_capabilities() const
{
	return XRInterface::XR_VR + XRInterface::XR_STEREO;
}

XRInterface::TrackingStatus OpenXRInterface::get_tracking_status() const { return tracking_state; }

OpenXRInterface::ActionSet* OpenXRInterface::create_action_set(
	const String& p_action_set_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	// find if it already exists
	for (int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i]->action_set_name == p_action_set_name) {
			// already exists in this set
			return nullptr;
		}
	}

	ActionSet* action_set = memnew(ActionSet);
	action_set->action_set_name = p_action_set_name;
	action_set->is_active = true;
	action_set->action_set_rid =
		OpenXRAPI::action_set_create(p_action_set_name, p_localized_name, p_priority);
	action_sets.push_back(action_set);

	return action_set;
}

void OpenXRInterface::free_action_sets()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < action_sets.size(); i++) {
		ActionSet* action_set = action_sets[i];

		free_actions(action_set);

		OpenXRAPI::action_set_free(action_set->action_set_rid);

		memfree(action_set);
	}
	action_sets.clear();
}

OpenXRInterface::Action* OpenXRInterface::create_action(ActionSet* p_action_set,
	const String& p_action_name, const String& p_localized_name,
	OpenXRAction::ActionType p_action_type, const Vector<Tracker*> p_trackers)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		if (p_action_set->actions[i]->action_name == p_action_name) {
			// already exists in this set
			return nullptr;
		}
	}

	Vector<RID> tracker_rids;
	for (int i = 0; i < p_trackers.size(); i++) {
		tracker_rids.push_back(p_trackers[i]->tracker_rid);
	}

	Action* action = memnew(Action);
	if (p_action_type == OpenXRAction::OPENXR_ACTION_POSE) {
		// We can't have dual action names in OpenXR hence we added _pose,
		// but default, aim and grip and default pose action names in Godot so rename them on the
		// tracker. NOTE need to decide on whether we should keep the naming convention or rename it
		// on Godots side
		if (p_action_name == "default_pose") {
			action->action_name = "default";
		}
		else if (p_action_name == "aim_pose") {
			action->action_name = "aim";
		}
		else if (p_action_name == "grip_pose") {
			action->action_name = "grip";
		}
		else {
			action->action_name = p_action_name;
		}
	}
	else {
		action->action_name = p_action_name;
	}

	action->action_type = p_action_type;
	action->action_rid = OpenXRAPI::action_create(
		p_action_set->action_set_rid, p_action_name, p_localized_name, p_action_type, tracker_rids);
	p_action_set->actions.push_back(action);

	// we link our actions back to our trackers so we know which actions to check when we're
	// processing our trackers
	for (int i = 0; i < p_trackers.size(); i++) {
		if (!p_trackers[i]->actions.has(action)) {
			p_trackers[i]->actions.push_back(action);
		}
	}

	return action;
}

OpenXRInterface::Action* OpenXRInterface::find_action(const String& p_action_name)
{
	// We just find the first action by this name

	for (int i = 0; i < action_sets.size(); i++) {
		for (int j = 0; j < action_sets[i]->actions.size(); j++) {
			if (action_sets[i]->actions[j]->action_name == p_action_name) {
				return action_sets[i]->actions[j];
			}
		}
	}

	// not found
	return nullptr;
}

void OpenXRInterface::free_actions(ActionSet* p_action_set)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		Action* action = p_action_set->actions[i];

		OpenXRAPI::action_free(action->action_rid);

		memdelete(action);
	}
	p_action_set->actions.clear();
}

OpenXRInterface::Tracker* OpenXRInterface::find_tracker(const String& p_tracker_name, bool p_create)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), nullptr);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size(); i++) {
		tracker = trackers[i];
		if (tracker->tracker_name == p_tracker_name) {
			return tracker;
		}
	}

	if (!p_create) {
		return nullptr;
	}

	ERR_FAIL_COND_V(!OpenXRAPI::is_top_level_path_supported(p_tracker_name), nullptr);

	// Create our RID
	RID tracker_rid = OpenXRAPI::tracker_create(p_tracker_name);
	ERR_FAIL_COND_V(tracker_rid.is_null(), nullptr);

	// Create our controller tracker.
	Ref<XRControllerTracker> controller_tracker;
	controller_tracker.instantiate();

	// We have standardized some names to make things nicer to the user so lets recognize the
	// toplevel paths related to these.
	if (p_tracker_name == "/user/hand/left") {
		controller_tracker->set_tracker_name("left_hand");
		controller_tracker->set_tracker_desc("Left hand controller");
		controller_tracker->set_tracker_hand(XRPositionalTracker::TRACKER_HAND_LEFT);
	}
	else if (p_tracker_name == "/user/hand/right") {
		controller_tracker->set_tracker_name("right_hand");
		controller_tracker->set_tracker_desc("Right hand controller");
		controller_tracker->set_tracker_hand(XRPositionalTracker::TRACKER_HAND_RIGHT);
	}
	else {
		controller_tracker->set_tracker_name(p_tracker_name);
		controller_tracker->set_tracker_desc(p_tracker_name);
	}
	controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	XRServer::add_tracker(controller_tracker);

	// create a new entry
	tracker = memnew(Tracker);
	tracker->tracker_name = p_tracker_name;
	tracker->tracker_rid = tracker_rid;
	tracker->controller_tracker = controller_tracker;
	tracker->interaction_profile = RID();
	trackers.push_back(tracker);

	return tracker;
}

void OpenXRInterface::tracker_profile_changed(RID p_tracker, RID p_interaction_profile)
{
	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size() && tracker == nullptr; i++) {
		if (trackers[i]->tracker_rid == p_tracker) {
			tracker = trackers[i];
		}
	}
	ERR_FAIL_NULL(tracker);

	tracker->interaction_profile = p_interaction_profile;

	if (p_interaction_profile.is_null()) {
		print_verbose("OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " +
					  INTERACTION_PROFILE_NONE);
		tracker->controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	}
	else {
		String name = OpenXRAPI::interaction_profile_get_name(p_interaction_profile);
		print_verbose(
			"OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " + name);
		tracker->controller_tracker->set_tracker_profile(name);
	}
}

void OpenXRInterface::trigger_haptic_pulse(const String& p_action_name,
	const String& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	Action* action = find_action(p_action_name);
	ERR_FAIL_NULL(action);

	// We need to map our tracker name to our OpenXR name for our inbuild names.
	String tracker_name = p_tracker_name;
	if (tracker_name == "left_hand") {
		tracker_name = "/user/hand/left";
	}
	else if (tracker_name == "right_hand") {
		tracker_name = "/user/hand/right";
	}
	Tracker* tracker = find_tracker(tracker_name);
	ERR_FAIL_NULL(tracker);

	// TODO OpenXR does not support delay, so we may need to add support for that somehow...

	XrDuration duration = XrDuration(p_duration_sec * 1000000000.0); // seconds -> nanoseconds

	OpenXRAPI::trigger_haptic_pulse(
		action->action_rid, tracker->tracker_rid, p_frequency, p_amplitude, duration);
}

void OpenXRInterface::free_trackers()
{
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < trackers.size(); i++) {
		Tracker* tracker = trackers[i];

		OpenXRAPI::tracker_free(tracker->tracker_rid);
		XRServer::remove_tracker(tracker->controller_tracker);
		tracker->controller_tracker.unref();

		memdelete(tracker);
	}
	trackers.clear();
}

void OpenXRInterface::free_interaction_profiles()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (const RID& interaction_profile : interaction_profiles) {
		OpenXRAPI::interaction_profile_free(interaction_profile);
	}
	interaction_profiles.clear();
}

bool OpenXRInterface::initialize_on_startup() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return true;
	}
}

bool OpenXRInterface::is_initialized() const { return initialized; }

bool OpenXRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	if (p_mode == XRInterface::XR_PLAY_AREA_3DOF) {
		return false;
	}
	return true;
}

XRInterface::PlayAreaMode OpenXRInterface::get_play_area_mode() const
{
	if (!OpenXRAPI::is_allocated() || !initialized) {
		return XRInterface::XR_PLAY_AREA_UNKNOWN;
	}

	XrReferenceSpaceType reference_space = OpenXRAPI::get_reference_space();

	if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL) {
		return XRInterface::XR_PLAY_AREA_SITTING;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT) {
		return XRInterface::XR_PLAY_AREA_ROOMSCALE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_STAGE) {
		return XRInterface::XR_PLAY_AREA_STAGE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_MAX_ENUM) {
		return XRInterface::XR_PLAY_AREA_CUSTOM;
	}

	return XRInterface::XR_PLAY_AREA_UNKNOWN;
}

bool OpenXRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrReferenceSpaceType reference_space;

	if (p_mode == XRInterface::XR_PLAY_AREA_SITTING) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_ROOMSCALE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_STAGE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_STAGE;
	}
	else {
		return false;
	}

	if (OpenXRAPI::set_requested_reference_space(reference_space)) {
		if (XRServer::is_initialized()) {
			XRServer::clear_reference_frame();
		}
		return true;
	}

	return false;
}

PackedVector3Array OpenXRInterface::get_play_area() const
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), PackedVector3Array());
	PackedVector3Array arr;

	Vector3 sides[4] = {
		Vector3(-0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, 0.5f),
		Vector3(-0.5f, 0.0f, 0.5f),
	};

	if (OpenXRAPI::is_allocated() && OpenXRAPI::is_initialized()) {
		Size2 extents = OpenXRAPI::get_play_space_bounds();
		if (extents.width != 0.0 && extents.height != 0.0) {
			Transform3D reference_frame = XRServer::get_reference_frame();

			for (int i = 0; i < 4; i++) {
				Vector3 coord = sides[i];

				// Scale it up.
				coord.x *= extents.width;
				coord.z *= extents.height;

				// Now apply our reference.
				Vector3 out = reference_frame.xform(coord);
				arr.push_back(out);
			}
		}
		else {
			WARN_PRINT_ONCE("OpenXR: No extents available.");
		}
	}

	return arr;
}

float OpenXRInterface::get_display_refresh_rate() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0.0;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return 0.0;
	}
	else {
		return OpenXRAPI::get_display_refresh_rate();
	}
}

void OpenXRInterface::set_display_refresh_rate(float p_refresh_rate)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return;
	}
	else {
		OpenXRAPI::set_display_refresh_rate(p_refresh_rate);
	}
}

bool OpenXRInterface::is_hand_tracking_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRHandTrackingExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRHandTrackingExtension::get_active();
		}
	}
}

bool OpenXRInterface::is_hand_interaction_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRInteractionProfileExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRInteractionProfileExtension::is_profile_available(
				OpenXRInteractionProfileExtension::PROFILE_HAND_INTERACTION);
		}
	}
}

bool OpenXRInterface::is_eye_gaze_interaction_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXREyeGazeInteractionExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction();
		}
	}
}

bool OpenXRInterface::is_action_set_active(const String& p_action_set) const
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			return action_set->is_active;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
	return false;
}

void OpenXRInterface::set_action_set_active(const String& p_action_set, bool p_active)
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			action_set->is_active = p_active;
			return;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
}

float OpenXRInterface::get_vrs_min_radius() const { return xr_vrs.get_vrs_min_radius(); }

void OpenXRInterface::set_vrs_min_radius(float p_vrs_min_radius)
{
	xr_vrs.set_vrs_min_radius(p_vrs_min_radius);
}

float OpenXRInterface::get_vrs_strength() const { return xr_vrs.get_vrs_strength(); }

void OpenXRInterface::set_vrs_strength(float p_vrs_strength)
{
	xr_vrs.set_vrs_strength(p_vrs_strength);
}

double OpenXRInterface::get_render_target_size_multiplier() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 1.0;
	}
	else {
		return OpenXRAPI::get_render_target_size_multiplier();
	}
}

void OpenXRInterface::set_render_target_size_multiplier(double multiplier)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_render_target_size_multiplier(multiplier);
	}
}

bool OpenXRInterface::is_foveation_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::is_foveation_supported();
	}
}

int OpenXRInterface::get_foveation_level() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0;
	}
	else {
		return OpenXRAPI::get_foveation_level();
	}
}

void OpenXRInterface::set_foveation_level(int p_foveation_level)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_level(p_foveation_level);
	}
}

bool OpenXRInterface::get_foveation_dynamic() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_dynamic();
	}
}

void OpenXRInterface::set_foveation_dynamic(bool p_foveation_dynamic)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_dynamic(p_foveation_dynamic);
	}
}

bool OpenXRInterface::get_foveation_with_subsampled_images() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_with_subsampled_images();
	}
}

void OpenXRInterface::set_foveation_with_subsampled_images(bool p_enabled)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_with_subsampled_images(p_enabled);
	}
}

Size2 OpenXRInterface::get_render_target_size()
{
	if (!OpenXRAPI::is_allocated()) {
		return Size2();
	}
	else {
		return OpenXRAPI::get_recommended_target_size();
	}
}

uint32_t OpenXRInterface::get_view_count()
{
	// TODO set this based on our configuration
	return 2;
}

void OpenXRInterface::_set_default_pos(
	Transform3D& r_transform, double p_world_scale, uint64_t p_eye)
{
	r_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	r_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		r_transform.origin.x = 0.03 * p_world_scale;
	}
	else if (p_eye == 2) {
		r_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D OpenXRInterface::get_camera_transform()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());

	Transform3D hmd_transform;
	double world_scale = XRServer::get_world_scale();

	// head_transform should be updated in process

	hmd_transform.basis = head_transform.basis;
	hmd_transform.origin = head_transform.origin * world_scale;

	return hmd_transform;
}

Transform3D OpenXRInterface::get_transform_for_view(
	uint32_t p_view, const Transform3D& p_cam_transform)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(
		p_view, get_view_count(), Transform3D(), "View index outside bounds.");

	Transform3D t;
	if (OpenXRAPI::is_allocated() && OpenXRAPI::get_view_transform(p_view, t)) {
		// update our cached value if we have a valid transform
		transform_for_view[p_view] = t;
	}
	else {
		// reuse cached value
		t = transform_for_view[p_view];
	}

	// Apply our world scale
	double world_scale = XRServer::get_world_scale();
	t.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * t;
}

Projection OpenXRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	Projection cm;
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(p_view, get_view_count(), cm, "View index outside bounds.");

	if (OpenXRAPI::is_allocated()) {
		if (OpenXRAPI::get_view_projection(p_view, p_z_near, p_z_far, cm)) {
			return cm;
		}
	}

	// Failed to get from our OpenXR device? Default to some sort of sensible camera matrix..
	cm.set_for_hmd(p_view + 1, 1.0, 6.0, 14.5, 4.0, 1.5, p_z_near, p_z_far);

	return cm;
}

Rect2i OpenXRInterface::get_render_region()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_render_region();
	}
	else {
		return Rect2i();
	}
}

RID OpenXRInterface::get_color_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_color_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_depth_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_depth_texture();
	}
	else {
		return RID();
	}
}

Size2i OpenXRInterface::get_velocity_target_size()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_target_size();
	}
	else {
		return Size2i();
	}
}

void OpenXRInterface::pre_render()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::pre_render();
	}
}

bool OpenXRInterface::pre_draw_viewport(RID p_render_target)
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::pre_draw_viewport(p_render_target);
	}
	else {
		// don't render
		return false;
	}
}

Vector<RenderingServerTypes::BlitToScreen> OpenXRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

#ifndef ANDROID_ENABLED
	// If separate HMD we should output one eye to screen
	if (p_screen_rect != Rect2()) {
		RenderingServerTypes::BlitToScreen blit;

		blit.render_target = p_render_target;
		blit.multi_view.use_layer = true;
		blit.multi_view.layer = 0;
		blit.lens_distortion.apply = false;

		Size2 render_size = get_render_target_size();
		Rect2 dst_rect = p_screen_rect;
		float new_height = dst_rect.size.x * (render_size.y / render_size.x);
		if (new_height > dst_rect.size.y) {
			dst_rect.position.y = (0.5 * dst_rect.size.y) - (0.5 * new_height);
			dst_rect.size.y = new_height;
		}
		else {
			float new_width = dst_rect.size.y * (render_size.x / render_size.y);

			dst_rect.position.x = (0.5 * dst_rect.size.x) - (0.5 * new_width);
			dst_rect.size.x = new_width;
		}

		blit.dst_rect = dst_rect;
		blit_to_screen.push_back(blit);
	}
#endif

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::post_draw_viewport(p_render_target);
	}

	return blit_to_screen;
}

void OpenXRInterface::end_frame()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::end_frame();
	}
}

bool OpenXRInterface::is_passthrough_enabled()
{
	return get_environment_blend_mode() == XR_ENV_BLEND_MODE_ALPHA_BLEND;
}

bool OpenXRInterface::start_passthrough()
{
	return set_environment_blend_mode(XR_ENV_BLEND_MODE_ALPHA_BLEND);
}

void OpenXRInterface::stop_passthrough() { set_environment_blend_mode(XR_ENV_BLEND_MODE_OPAQUE); }

XRInterface::EnvironmentBlendMode OpenXRInterface::get_environment_blend_mode() const
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode = OpenXRAPI::get_environment_blend_mode();
		switch (oxr_blend_mode) {
		case XR_ENVIRONMENT_BLEND_MODE_OPAQUE: {
			return XR_ENV_BLEND_MODE_OPAQUE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE: {
			return XR_ENV_BLEND_MODE_ADDITIVE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND: {
			return XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} break;
		default:
			break;
		}
	}

	return XR_ENV_BLEND_MODE_OPAQUE;
}

bool OpenXRInterface::set_environment_blend_mode(XRInterface::EnvironmentBlendMode mode)
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode;
		switch (mode) {
		case XR_ENV_BLEND_MODE_OPAQUE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
			break;
		case XR_ENV_BLEND_MODE_ADDITIVE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
			break;
		case XR_ENV_BLEND_MODE_ALPHA_BLEND:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
			break;
		default:
			WARN_PRINT("Unknown blend mode requested: " + String::num_int64(int64_t(mode)));
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		}
		return OpenXRAPI::set_environment_blend_mode(oxr_blend_mode);
	}
	return false;
}

void OpenXRInterface::on_reference_space_change_pending(XrReferenceSpaceType p_type)
{
	reference_stage_changing = true;

	// Emit play area bounds changed signal when the reference space changes.
	PlayAreaMode mode = XR_PLAY_AREA_UNKNOWN;

	switch (p_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW:
		mode = XR_PLAY_AREA_3DOF;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL:
		mode = XR_PLAY_AREA_SITTING;
		break;
	case XR_REFERENCE_SPACE_TYPE_STAGE:
		mode = XR_PLAY_AREA_STAGE;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR:
		mode = XR_PLAY_AREA_ROOMSCALE;
		break;
	default:
		mode = XR_PLAY_AREA_UNKNOWN;
		break;
	}

	print_verbose("OpenXR Interface: Play area changed, emitting signal.");
}

OpenXRInterface::SessionState OpenXRInterface::get_session_state()
{
	if (OpenXRAPI::is_allocated()) {
		return (SessionState)OpenXRAPI::get_session_state();
	}

	return SESSION_STATE_UNKNOWN;
}

/** User Presence. */
bool OpenXRInterface::is_user_presence_supported() const
{
	if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return OpenXRUserPresenceExtension::is_initialized() && OpenXRUserPresenceExtension::is_active();
	}
}

bool OpenXRInterface::is_user_present() const
{
	// If extension is unavailable or unsupported, we default to user is present.
	if (!is_user_presence_supported()) {
		return true;
	}
	else {
		return OpenXRUserPresenceExtension::is_user_present();
	}
}

RID OpenXRInterface::get_vrs_texture()
{
	if (!OpenXRAPI::is_allocated()) {
		return RID();
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return density_map;
	}

	PackedVector2Array eye_foci;

	Size2 target_size = get_render_target_size();
	real_t aspect_ratio = target_size.x / target_size.y;
	uint32_t view_count = get_view_count();

	for (uint32_t v = 0; v < view_count; v++) {
		eye_foci.push_back(OpenXRAPI::get_eye_focus(v, aspect_ratio));
	}

	xr_vrs.set_vrs_render_region(get_render_region());

	return xr_vrs.make_vrs_texture(target_size, eye_foci);
}

XRInterface::VRSTextureFormat OpenXRInterface::get_vrs_texture_format()
{
	if (!OpenXRAPI::is_allocated()) {
		return XR_VRS_TEXTURE_FORMAT_UNIFIED;
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return XR_VRS_TEXTURE_FORMAT_FRAGMENT_DENSITY_MAP;
	}

	return XR_VRS_TEXTURE_FORMAT_UNIFIED;
}

void OpenXRInterface::set_cpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_cpu_level(p_level);
	}
}

void OpenXRInterface::set_gpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_gpu_level(p_level);
	}
}

OpenXRInterface::OpenXRInterface()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(this);
	}

	// while we don't have head tracking, don't put the headset on the floor...
	_set_default_pos(head_transform, 1.0, 0);
	_set_default_pos(transform_for_view[0], 1.0, 1);
	_set_default_pos(transform_for_view[1], 1.0, 2);
}

OpenXRInterface::~OpenXRInterface()
{
	if (is_initialized()) {
		uninitialize();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(nullptr);
	}
}

void OpenXRInterface::uninitialize() {}

void OpenXRInterface::on_refresh_rate_changes(float) {}

void OpenXRInterface::on_gpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

void OpenXRInterface::on_cpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

PackedStringArray OpenXRInterface::get_suggested_tracker_names() const
{
	return PackedStringArray();
}

bool OpenXRInterface::initialize() { return false; }

void OpenXRInterface::process() {}

bool OpenXRInterface::is_passthrough_supported() { return false; }


