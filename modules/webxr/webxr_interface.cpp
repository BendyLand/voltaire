/**************************************************************************/
/*  webxr_interface.cpp                                                   */
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
// --- Inlined from modules/webxr/webxr_interface.h ---
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_tracker.h"

#ifdef WEB_ENABLED
#include "core/templates/rb_map.h"

namespace GLES3 {
class TextureStorage;
}
#endif // WEB_ENABLED

/**
	The WebXR interface is a VR/AR interface that can be used on the web.
*/

class WebXRInterface final : public XRInterface {
public:
	enum TargetRayMode {
		TARGET_RAY_MODE_UNKNOWN,
		TARGET_RAY_MODE_GAZE,
		TARGET_RAY_MODE_TRACKED_POINTER,
		TARGET_RAY_MODE_SCREEN,
	};

protected:
#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	Ref<XRTracker> _get_input_source_tracker_bind_compat_90645(int p_input_source_id) const;
#endif

public:
	void is_session_supported(const String &p_session_mode);
	void set_session_mode(const String &p_session_mode);
	String get_session_mode() const;
	void set_required_features(const String &p_required_features);
	String get_required_features() const;
	void set_optional_features(const String &p_optional_features);
	String get_optional_features() const;
	void set_requested_reference_space_types(const String &p_requested_reference_space_types);
	String get_requested_reference_space_types() const;
	String get_reference_space_type() const;
	String get_enabled_features() const;
	bool is_input_source_active(int p_input_source_id) const;
	Ref<XRTracker> get_input_source_tracker(int p_input_source_id) const;
	TargetRayMode get_input_source_target_ray_mode(int p_input_source_id) const;
	String get_visibility_state() const;
	float get_display_refresh_rate() const;
	void set_display_refresh_rate(float p_refresh_rate);
	Vector<float> get_available_display_refresh_rates() const;
	Vector<EnvironmentBlendMode> get_supported_environment_blend_modes();
	HashMap<String, String> get_system_info();

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

#ifdef WEB_ENABLED
private:
	bool initialized;
	Ref<XRTracker> head_tracker;
	Transform3D head_transform;

	String session_mode;
	String required_features;
	String optional_features;
	String requested_reference_space_types;
	String reference_space_type;
	String enabled_features;

	XRInterface::EnvironmentBlendMode environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;

	Size2 render_targetsize;
	RBMap<unsigned int, RID> texture_cache;
	struct Touch {
		bool is_touching = false;
		Vector2 position;
	} touches[5];

	static constexpr uint8_t input_source_count = 16;

	struct InputSource {
		Ref<XRTracker> tracker;
		bool active = false;
		TargetRayMode target_ray_mode;
		int touch_index = -1;
	} input_sources[input_source_count];

	static const int WEBXR_HAND_JOINT_MAX = 25;
	static const int HAND_MAX = 2;
	Ref<XRTracker> hand_trackers[HAND_MAX];

	RID color_texture;
	RID depth_texture;

	RID _get_color_texture();
	RID _get_depth_texture();
	RID _get_texture(unsigned int p_texture_id);
	Transform3D _js_matrix_to_transform(float *p_js_matrix);
	void _update_input_source(int p_input_source_id);

	Vector2 _get_screen_position_from_joy_vector(const Vector2 &p_joy_vector);

public:
	virtual PackedVector3Array get_play_area() const override;
	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) override;
	virtual Projection get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;
	virtual bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) override;
	virtual RID get_color_texture() override;
	virtual RID get_depth_texture() override;
	virtual RID get_velocity_texture() override;

	virtual void process() override;

	void _on_input_event(int p_event_type, int p_input_source_id);

	// Internal setters used by callbacks from Emscripten.
	inline void _set_reference_space_type(const String &p_reference_space_type) { reference_space_type = p_reference_space_type; }
	inline void _set_enabled_features(const String &p_enabled_features) { enabled_features = p_enabled_features; }
	void _set_environment_blend_mode(const String &p_blend_mode_string);

	WebXRInterface();
	~WebXRInterface();

private:
	String tracker_names[16] = {
		String("left_hand"),
		String("right_hand"),
		String("tracker_2"),
		String("tracker_3"),
		String("tracker_4"),
		String("tracker_5"),
		String("tracker_6"),
		String("tracker_7"),
		String("tracker_8"),
		String("tracker_9"),
		String("tracker_10"),
		String("tracker_11"),
		String("tracker_12"),
		String("tracker_13"),
		String("tracker_14"),
		String("tracker_15"),
	};

	String touch_names[5] = {
		String("touch_0"),
		String("touch_1"),
		String("touch_2"),
		String("touch_3"),
		String("touch_4"),
	};

	String standard_axis_names[10] = {
		String("touchpad_x"),
		String("touchpad_y"),
		String("thumbstick_x"),
		String("thumbstick_y"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String standard_vector_names[2] = {
		String("touchpad"),
		String("thumbstick"),
	};

	String standard_button_names[10] = {
		String("trigger_click"),
		String("grip_click"),
		String("touchpad_click"),
		String("thumbstick_click"),
		String("ax_button"),
		String("by_button"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String standard_button_pressure_names[10] = {
		String("trigger"),
		String("grip"),
		String("touchpad_click_pressure"),
		String("thumbstick_click_pressure"),
		String("ax_button_pressure"),
		String("by_button_pressure"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};

	String unknown_button_names[10] = {
		String("button_0"),
		String("button_1"),
		String("button_2"),
		String("button_3"),
		String("button_4"),
		String("button_5"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String unknown_axis_names[10] = {
		String("axis_0"),
		String("axis_1"),
		String("axis_2"),
		String("axis_3"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String unknown_button_pressure_names[10] = {
		String("button_pressure_0"),
		String("button_pressure_1"),
		String("button_pressure_2"),
		String("button_pressure_3"),
		String("button_pressure_4"),
		String("button_pressure_5"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};
#else // !WEB_ENABLED
public:
	WebXRInterface() = default;
	~WebXRInterface() = default;
#endif // WEB_ENABLED
};

// --- End modules/webxr/webxr_interface.h ---
#include "register_types.h"

#ifndef DISABLE_DEPRECATED


Ref<XRTracker> WebXRInterface::_get_input_source_tracker_bind_compat_90645(
	int p_input_source_id) const
{
	return get_input_source_tracker(p_input_source_id);
}

void WebXRInterface::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#ifdef WEB_ENABLED
// --- Inlined from modules/webxr/godot_webxr.h ---
#ifdef __cplusplus
extern "C" {
#endif

enum WebXRInputEvent {
	WEBXR_INPUT_EVENT_SELECTSTART,
	WEBXR_INPUT_EVENT_SELECTEND,
	WEBXR_INPUT_EVENT_SQUEEZESTART,
	WEBXR_INPUT_EVENT_SQUEEZEEND,
};

typedef void (*GodotWebXRSupportedCallback)(char *p_session_mode, int p_supported);
typedef void (*GodotWebXRStartedCallback)(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode);
typedef void (*GodotWebXREndedCallback)();
typedef void (*GodotWebXRFailedCallback)(char *p_message);
typedef void (*GodotWebXRInputEventCallback)(int p_event_type, int p_input_source_id);
typedef void (*GodotWebXRSimpleEventCallback)(char *p_signal_name);

extern int godot_webxr_is_supported();
extern void godot_webxr_is_session_supported(const char *p_session_mode, GodotWebXRSupportedCallback p_callback);

extern void godot_webxr_initialize(
		const char *p_session_mode,
		const char *p_required_features,
		const char *p_optional_features,
		const char *p_requested_reference_space_types,
		GodotWebXRStartedCallback p_on_session_started,
		GodotWebXREndedCallback p_on_session_ended,
		GodotWebXRFailedCallback p_on_session_failed,
		GodotWebXRInputEventCallback p_on_input_event,
		GodotWebXRSimpleEventCallback p_on_simple_event);
extern void godot_webxr_uninitialize();

extern int godot_webxr_get_view_count();
extern bool godot_webxr_get_render_target_size(int *r_size);
extern bool godot_webxr_get_transform_for_view(int p_view, float *r_transform);
extern bool godot_webxr_get_projection_for_view(int p_view, float *r_transform);
extern unsigned int godot_webxr_get_color_texture();
extern unsigned int godot_webxr_get_depth_texture();
extern unsigned int godot_webxr_get_velocity_texture();

extern bool godot_webxr_update_input_source(
		int p_input_source_id,
		float *r_target_pose,
		int *r_target_ray_mode,
		int *r_touch_index,
		int *r_has_grip_pose,
		float *r_grip_pose,
		int *r_has_standard_mapping,
		int *r_button_count,
		float *r_buttons,
		int *r_axes_count,
		float *r_axes,
		int *r_has_hand_data,
		float *r_hand_joints,
		float *r_hand_radii);

extern char *godot_webxr_get_visibility_state();
extern int godot_webxr_get_bounds_geometry(float **r_points);

extern float godot_webxr_get_frame_rate();
extern void godot_webxr_update_target_frame_rate(float p_frame_rate);
extern int godot_webxr_get_supported_frame_rates(float **r_frame_rates);

#ifdef __cplusplus
}
#endif

// --- End modules/webxr/godot_webxr.h ---

#include "core/input/input.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "servers/xr/xr_tracker.h"

#include <emscripten.h>

#include <cstdlib>

void _emwebxr_on_session_supported(char *p_session_mode, int p_supported) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	(void)p_session_mode;
	(void)p_supported;
}

void _emwebxr_on_session_started(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	interface->_set_reference_space_type(reference_space_type);
	interface->_set_enabled_features(p_enabled_features);
	interface->_set_environment_blend_mode(p_environment_blend_mode);
}

void _emwebxr_on_session_ended() {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
}

void _emwebxr_on_session_failed(char *p_message) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();

	ERR_PRINT(String(p_message));
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_input_event(int p_event_type, int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->_on_input_event(p_event_type, p_input_source_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_simple_event(char *p_signal_name) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String signal_name = String(p_signal_name);
	(void)signal_name;
}

void WebXRInterface::is_session_supported(const String &p_session_mode) {
	godot_webxr_is_session_supported(p_session_mode.utf8().get_data(), &_emwebxr_on_session_supported);
}

void WebXRInterface::set_session_mode(const String &p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterface::get_session_mode() const {
	return session_mode;
}

void WebXRInterface::set_required_features(const String &p_required_features) {
	required_features = p_required_features;
}

String WebXRInterface::get_required_features() const {
	return required_features;
}

void WebXRInterface::set_optional_features(const String &p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterface::get_optional_features() const {
	return optional_features;
}

void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterface::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

String WebXRInterface::get_reference_space_type() const {
	return reference_space_type;
}

String WebXRInterface::get_enabled_features() const {
	return enabled_features;
}

bool WebXRInterface::is_input_source_active(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, false);
	return input_sources[p_input_source_id].active;
}

Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, Ref<XRTracker>());
	return input_sources[p_input_source_id].tracker;
}

WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, WebXRInterface::TARGET_RAY_MODE_UNKNOWN);
	if (!input_sources[p_input_source_id].active) {
		return WebXRInterface::TARGET_RAY_MODE_UNKNOWN;
	}
	return input_sources[p_input_source_id].target_ray_mode;
}

String WebXRInterface::get_visibility_state() const {
	char *c_str = godot_webxr_get_visibility_state();
	if (c_str) {
		String visibility_state = String(c_str);
		free(c_str);

		return visibility_state;
	}
	return String();
}

PackedVector3Array WebXRInterface::get_play_area() const {
	PackedVector3Array ret;

	float *points;
	int point_count = godot_webxr_get_bounds_geometry(&points);
	if (point_count > 0) {
		ret.resize(point_count);
		for (int i = 0; i < point_count; i++) {
			float *js_vector3 = points + (i * 3);
			ret.set(i, Vector3(js_vector3[0], js_vector3[1], js_vector3[2]));
		}
		free(points);
	}

	return ret;
}

float WebXRInterface::get_display_refresh_rate() const {
	return godot_webxr_get_frame_rate();
}

void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {
	godot_webxr_update_target_frame_rate(p_refresh_rate);
}

Vector<float> WebXRInterface::get_available_display_refresh_rates() const {
	Vector<float> ret;

	float *rates;
	int rate_count = godot_webxr_get_supported_frame_rates(&rates);
	if (rate_count > 0) {
		ret.resize(rate_count);
		for (int i = 0; i < rate_count; i++) {
			ret.write[i] = rates[i];
		}
		free(rates);
	}

	return ret;
}

Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() {
	Vector<XRInterface::EnvironmentBlendMode> blend_modes;
	// The blend mode can't be changed, so return the current blend mode as the only supported one.
	blend_modes.push_back(environment_blend_mode);
	return blend_modes;
}

XRInterface::EnvironmentBlendMode WebXRInterface::get_environment_blend_mode() const {
	return environment_blend_mode;
}

bool WebXRInterface::set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) {
	if (environment_blend_mode == p_new_environment_blend_mode) {
		// Environment blend mode can't be changed, but we'll consider it a success to set it
		// to what it already is.
		return true;
	}
	return false;
}

void WebXRInterface::_set_environment_blend_mode(const String &p_blend_mode_string) {
	if (p_blend_mode_string == "opaque") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
	} else if (p_blend_mode_string == "additive") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ADDITIVE;
	} else if (p_blend_mode_string == "alpha-blend") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
	} else {
		// Not all browsers can give us this information, so as a fallback,
		// we'll make some guesses about the blend mode.
		if (session_mode == "immersive-ar") {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} else {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		}
	}
}

String WebXRInterface::get_name() const {
	return "WebXR";
}

uint32_t WebXRInterface::get_capabilities() const {
	return XRInterface::XR_STEREO | XRInterface::XR_MONO | XRInterface::XR_VR | XRInterface::XR_AR;
}

uint32_t WebXRInterface::get_view_count() {
	return godot_webxr_get_view_count();
}

bool WebXRInterface::is_initialized() const {
	return (initialized);
}

bool WebXRInterface::initialize() {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	if (!initialized) {
		if (!godot_webxr_is_supported()) {
			ERR_PRINT("WebXR is unsupported by this web browser.");
			return false;
		}

		if (session_mode == "immersive-vr" && !GLES3::Config::multiview_supported) {
			ERR_PRINT("Stereo rendering in Godot requires multiview, but this web browser doesn't support it.");
			return false;
		}

		if (requested_reference_space_types.is_empty()) {
			ERR_PRINT("No reference spaces were requested.");
			return false;
		}

		enabled_features.clear();

		// We must create a tracker for our head.
		head_transform.basis = Basis();
		head_transform.origin = Vector3();
		head_tracker.instantiate();
		head_tracker->set_tracker_type(XRServer::TRACKER_HEAD);
		head_tracker->set_tracker_name("head");
		head_tracker->set_tracker_desc("Players head");
		XRServer::add_tracker(head_tracker);

		// Make this our primary interface.
		XRServer::set_primary_interface(this);

		// Clear render_targetsize to make sure it gets reset to the new size.
		// Clearing in uninitialize() doesn't work because a frame can still be
		// rendered after it's called, which will fill render_targetsize again.
		render_targetsize.width = 0;
		render_targetsize.height = 0;

		initialized = true;

		godot_webxr_initialize(
				session_mode.utf8().get_data(),
				required_features.utf8().get_data(),
				optional_features.utf8().get_data(),
				requested_reference_space_types.utf8().get_data(),
				&_emwebxr_on_session_started,
				&_emwebxr_on_session_ended,
				&_emwebxr_on_session_failed,
				&_emwebxr_on_input_event,
				&_emwebxr_on_simple_event);
	};

	return true;
}

void WebXRInterface::uninitialize() {
	if (initialized) {
		if (XRServer::is_initialized()) {
			if (head_tracker.is_valid()) {
				XRServer::remove_tracker(head_tracker);

				head_tracker.unref();
			}

			for (int i = 0; i < HAND_MAX; i++) {
				if (hand_trackers[i].is_valid()) {
					XRServer::remove_tracker(hand_trackers[i]);

					hand_trackers[i].unref();
				}
			}

			if (XRServer::get_primary_interface() == this) {
				// no longer our primary interface
				XRServer::set_primary_interface(nullptr);
			}
		}

		godot_webxr_uninitialize();

		if (GLES3::TextureStorage::is_initialized()) {
			for (KeyValue<unsigned int, RID> &E : texture_cache) {
				// Forcibly mark as not part of a render target so we can free it.
				GLES3::Texture *texture = GLES3::TextureStorage::get_texture(E.value);
				texture->is_render_target = false;

				GLES3::TextureStorage::texture_free(E.value);
			}
		}

		texture_cache.clear();
		reference_space_type.clear();
		enabled_features.clear();
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		initialized = false;
	};
}

HashMap<String, String> WebXRInterface::get_system_info() {
	HashMap<String, String> dict;

	// TODO get actual information from WebXR to return here
	dict[String("XRRuntimeName")] = String("WebXR");
	dict[String("XRRuntimeVersion")] = String("");

	return dict;
}

Transform3D WebXRInterface::_js_matrix_to_transform(float *p_js_matrix) {
	Transform3D transform;

	transform.basis.rows[0].x = p_js_matrix[0];
	transform.basis.rows[1].x = p_js_matrix[1];
	transform.basis.rows[2].x = p_js_matrix[2];
	transform.basis.rows[0].y = p_js_matrix[4];
	transform.basis.rows[1].y = p_js_matrix[5];
	transform.basis.rows[2].y = p_js_matrix[6];
	transform.basis.rows[0].z = p_js_matrix[8];
	transform.basis.rows[1].z = p_js_matrix[9];
	transform.basis.rows[2].z = p_js_matrix[10];
	transform.origin.x = p_js_matrix[12];
	transform.origin.y = p_js_matrix[13];
	transform.origin.z = p_js_matrix[14];

	return transform;
}

Size2 WebXRInterface::get_render_target_size() {
	if (render_targetsize.width != 0 && render_targetsize.height != 0) {
		return render_targetsize;
	}

	int js_size[2];
	bool has_size = godot_webxr_get_render_target_size(js_size);

	if (!initialized || !has_size) {
		// As a temporary default (until WebXR is fully initialized), use the
		// window size.
		return DisplayServer::window_get_size();
	}

	render_targetsize.width = (float)js_size[0];
	render_targetsize.height = (float)js_size[1];

	return render_targetsize;
}

Transform3D WebXRInterface::get_camera_transform() {
	Transform3D camera_transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), camera_transform);

	if (initialized) {
		double world_scale = XRServer::get_world_scale();

		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		camera_transform = (XRServer::get_reference_frame()) * _head_transform;
	}

	return camera_transform;
}

Transform3D WebXRInterface::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), p_cam_transform);
	ERR_FAIL_COND_V(!initialized, p_cam_transform);

	float js_matrix[16];
	bool has_transform = godot_webxr_get_transform_for_view(p_view, js_matrix);
	if (!has_transform) {
		return p_cam_transform;
	}

	Transform3D transform_for_view = _js_matrix_to_transform(js_matrix);

	double world_scale = XRServer::get_world_scale();
	transform_for_view.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * transform_for_view;
}

Projection WebXRInterface::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	Projection view;

	ERR_FAIL_COND_V(!initialized, view);

	float js_matrix[16];
	bool has_projection = godot_webxr_get_projection_for_view(p_view, js_matrix);
	if (!has_projection) {
		return view;
	}

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			view.columns[i][j] = js_matrix[k++];
		}
	}

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	view.columns[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	view.columns[3][2] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return view;
}

bool WebXRInterface::pre_draw_viewport(RID p_render_target) {
	if (!GLES3::TextureStorage::is_initialized()) {
		return false;
	}

	// Cache the resources so we don't have to get them from JS twice.
	color_texture = _get_color_texture();
	depth_texture = _get_depth_texture();

	// Per the WebXR spec, it returns "opaque textures" to us, which may be the
	// same WebGLTexture object (which would be the same GLuint in C++) but
	// represent a different underlying resource (probably the next texture in
	// the XR device's swap chain). In order to render to this texture, we need
	// to re-attach it to the FBO, otherwise we get an "incomplete FBO" error.
	//
	// See: https://immersive-web.github.io/layers/#xropaquetextures
	//
	// So, even if the color and depth textures have the same GLuint as the last
	// frame, we need to re-attach them again.
	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, true);

	return true;
}

Vector<RenderingServerTypes::BlitToScreen> WebXRInterface::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

	if (!GLES3::TextureStorage::is_initialized()) {
		return blit_to_screen;
	}

	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, false);

	return blit_to_screen;
}

RID WebXRInterface::_get_color_texture() {
	unsigned int texture_id = godot_webxr_get_color_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_depth_texture() {
	unsigned int texture_id = godot_webxr_get_depth_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_texture(unsigned int p_texture_id) {
	RBMap<unsigned int, RID>::Element *cache = texture_cache.find(p_texture_id);
	if (cache != nullptr) {
		return cache->get();
	}

	if (!GLES3::TextureStorage::is_initialized()) {
		return RID();
	}

	uint32_t view_count = godot_webxr_get_view_count();
	Size2 texture_size = get_render_target_size();

	RID texture = GLES3::TextureStorage::texture_create_from_native_handle(
			view_count == 1 ? RSE::TEXTURE_TYPE_2D : RSE::TEXTURE_TYPE_LAYERED,
			Image::FORMAT_RGBA8,
			p_texture_id,
			(int)texture_size.width,
			(int)texture_size.height,
			1,
			view_count);

	texture_cache.insert(p_texture_id, texture);

	return texture;
}

RID WebXRInterface::get_color_texture() {
	return color_texture;
}

RID WebXRInterface::get_depth_texture() {
	return depth_texture;
}

RID WebXRInterface::get_velocity_texture() {
	unsigned int texture_id = godot_webxr_get_velocity_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

void WebXRInterface::process() {
	if (initialized) {
		// Get the "head" position.
		float js_matrix[16];
		if (godot_webxr_get_transform_for_view(-1, js_matrix)) {
			head_transform = _js_matrix_to_transform(js_matrix);
		}
		if (head_tracker.is_valid()) {
			head_tracker->set_pose("default", head_transform, Vector3(), Vector3());
		}

		// Update all input sources.
		for (int i = 0; i < input_source_count; i++) {
			_update_input_source(i);
		}
	};
}

void WebXRInterface::_update_input_source(int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	InputSource &input_source = input_sources[p_input_source_id];

	float target_pose[16];
	int tmp_target_ray_mode;
	int touch_index;
	int has_grip_pose;
	float grip_pose[16];
	int has_standard_mapping;
	int button_count;
	float buttons[10];
	int axes_count;
	float axes[10];
	int has_hand_data;
	float hand_joints[WEBXR_HAND_JOINT_MAX * 16];
	float hand_radii[WEBXR_HAND_JOINT_MAX];

	input_source.active = godot_webxr_update_input_source(
			p_input_source_id,
			target_pose,
			&tmp_target_ray_mode,
			&touch_index,
			&has_grip_pose,
			grip_pose,
			&has_standard_mapping,
			&button_count,
			buttons,
			&axes_count,
			axes,
			&has_hand_data,
			hand_joints,
			hand_radii);

	if (!input_source.active) {
		if (input_source.tracker.is_valid()) {
			XRServer::remove_tracker(input_source.tracker);
			input_source.tracker.unref();
		}
		return;
	}

	input_source.target_ray_mode = (WebXRInterface::TargetRayMode)tmp_target_ray_mode;
	input_source.touch_index = touch_index;

	Ref<XRTracker> &tracker = input_source.tracker;

	if (tracker.is_null()) {
		tracker.instantiate();
		tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);

		String tracker_name;
		if (input_source.target_ray_mode == WebXRInterface::TargetRayMode::TARGET_RAY_MODE_SCREEN) {
			tracker_name = touch_names[touch_index];
		} else {
			tracker_name = tracker_names[p_input_source_id];
		}

		// Input source id's 0 and 1 are always the left and right hands.
		if (p_input_source_id < 2) {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(p_input_source_id == 0 ? "Left hand controller" : "Right hand controller");
			tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
		} else {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(tracker_name);
		}
		XRServer::add_tracker(tracker);
	}

	Transform3D aim_transform = _js_matrix_to_transform(target_pose);
	tracker->set_pose(SceneString(default_), aim_transform, Vector3(), Vector3());
	tracker->set_pose(String("aim"), aim_transform, Vector3(), Vector3());
	if (has_grip_pose) {
		tracker->set_pose(String("grip"), _js_matrix_to_transform(grip_pose), Vector3(), Vector3());
	}

	for (int i = 0; i < button_count; i++) {
		String button_name = has_standard_mapping ? standard_button_names[i] : unknown_button_names[i];
		String button_pressure_name = has_standard_mapping ? standard_button_pressure_names[i] : unknown_button_pressure_names[i];
		float value = buttons[i];
		bool state = value > 0.0;
		tracker->set_input(button_name, state);
		tracker->set_input(button_pressure_name, value);
	}

	for (int i = 0; i < axes_count; i++) {
		String axis_name = has_standard_mapping ? standard_axis_names[i] : unknown_axis_names[i];
		float value = axes[i];
		if (has_standard_mapping && (i == 1 || i == 3)) {
			// Invert the Y-axis on thumbsticks and trackpads, in order to
			// match OpenXR and other XR platform SDKs.
			value = -value;
		}
		tracker->set_input(axis_name, value);
	}

	// Also create Vector2's for the thumbstick and trackpad when we have the
	// standard mapping.
	if (has_standard_mapping) {
		if (axes_count >= 2) {
			tracker->set_input(standard_vector_names[0], Vector2(axes[0], -axes[1]));
		}
		if (axes_count >= 4) {
			tracker->set_input(standard_vector_names[1], Vector2(axes[2], -axes[3]));
		}
	}

	if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
		if (touch_index < 5 && axes_count >= 2) {
			Vector2 joy_vector = Vector2(axes[0], axes[1]);
			Vector2 position = _get_screen_position_from_joy_vector(joy_vector);

			if (touches[touch_index].is_touching) {
				Vector2 delta = position - touches[touch_index].position;

				// If position has changed by at least 1 pixel, generate a drag event.
				if (std::abs(delta.x) >= 1.0 || std::abs(delta.y) >= 1.0) {
					InputEvent event = InputEvent::make_screen_drag();
					event.screen_drag.index = touch_index;
					event.screen_drag.position = position;
					event.screen_drag.relative = delta;
					event.screen_drag.screen_relative = delta;
					Input::parse_input_event(event);
				}
			}

			touches[touch_index].position = position;
		}
	}

	if (p_input_source_id < 2) {
		Ref<XRTracker> hand_tracker = hand_trackers[p_input_source_id];
		if (has_hand_data) {
			// Transform orientations to match Godot Humanoid skeleton.
			const Basis bone_adjustment(
					Vector3(-1.0, 0.0, 0.0),
					Vector3(0.0, 0.0, -1.0),
					Vector3(0.0, -1.0, 0.0));

			if (unlikely(hand_tracker.is_null())) {
				hand_tracker.instantiate();
				hand_tracker->set_tracker_type(XRServer::TRACKER_HAND);
				hand_tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
				hand_tracker->set_tracker_name(p_input_source_id == 0 ? "/user/hand_tracker/left" : "/user/hand_tracker/right");

				// These flags always apply, since WebXR doesn't give us enough insight to be more fine grained.
				uint32_t joint_flags(XRTracker::HAND_JOINT_FLAG_POSITION_VALID | XRTracker::HAND_JOINT_FLAG_ORIENTATION_VALID | XRTracker::HAND_JOINT_FLAG_POSITION_TRACKED | XRTracker::HAND_JOINT_FLAG_ORIENTATION_TRACKED);
				for (int godot_joint = 0; godot_joint < XRTracker::HAND_JOINT_MAX; godot_joint++) {
					hand_tracker->set_hand_joint_flags((XRTracker::HandJoint)godot_joint, joint_flags);
				}

				hand_trackers[p_input_source_id] = hand_tracker;
				XRServer::add_tracker(hand_tracker);
			}

			hand_tracker->set_has_tracking_data(true);
			for (int webxr_joint = 0; webxr_joint < WEBXR_HAND_JOINT_MAX; webxr_joint++) {
				XRTracker::HandJoint godot_joint = (XRTracker::HandJoint)(webxr_joint + 1);

				Transform3D joint_transform = _js_matrix_to_transform(hand_joints + (16 * webxr_joint));
				joint_transform.basis *= bone_adjustment;
				hand_tracker->set_hand_joint_transform(godot_joint, joint_transform);

				hand_tracker->set_hand_joint_radius(godot_joint, hand_radii[webxr_joint]);
			}

			// WebXR doesn't have a palm joint, so we calculate it by finding the middle of the middle finger metacarpal bone.
			{
				// Start by getting the middle finger metacarpal joint.
				// Note: 10 is the WebXR middle finger metacarpal joint.
				Transform3D palm_transform = _js_matrix_to_transform(hand_joints + (10 * 16));
				palm_transform.basis *= bone_adjustment;

				// Get the middle finger phalanx position.
				// Note: 11 is the WebXR middle finger phalanx proximal joint and 12 is the origin offset.
				const float *phalanx_pos = hand_joints + (11 * 16) + 12;
				Vector3 phalanx(phalanx_pos[0], phalanx_pos[1], phalanx_pos[2]);

				// Offset the palm half-way towards the phalanx joint.
				palm_transform.origin = (palm_transform.origin + phalanx) / 2.0;

				// Set the palm joint and the pose.
				hand_tracker->set_hand_joint_transform(XRTracker::HAND_JOINT_PALM, palm_transform);
				hand_tracker->set_pose("default", palm_transform, Vector3(), Vector3());
			}

		} else if (hand_tracker.is_valid()) {
			hand_tracker->set_has_tracking_data(false);
			hand_tracker->invalidate_pose("default");
		}
	}
}

void WebXRInterface::_on_input_event(int p_event_type, int p_input_source_id) {
	// Get the latest data for this input source. For transient input sources,
	// we may not have any data at all yet!
	_update_input_source(p_input_source_id);

	if (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART || p_event_type == WEBXR_INPUT_EVENT_SELECTEND) {
		const InputSource &input_source = input_sources[p_input_source_id];
		if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
			int touch_index = input_source.touch_index;
			if (touch_index >= 0 && touch_index < 5) {
				touches[touch_index].is_touching = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				InputEvent event = InputEvent::make_screen_touch();
				event.touch.index = touch_index;
				event.touch.position = touches[touch_index].position;
				event.pressed = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				Input::parse_input_event(event);
			}
		}
	}

	(void)p_event_type;
	(void)p_input_source_id;
}

Vector2 WebXRInterface::_get_screen_position_from_joy_vector(const Vector2 &p_joy_vector) {
	if (!SceneTree::is_initialized()) {
		return Vector2();
	}

	Window *viewport = SceneTree::get_root();
	if (!viewport) {
		return Vector2();
	}

	Vector2 position_percentage((p_joy_vector.x + 1.0f) / 2.0f, ((p_joy_vector.y) + 1.0f) / 2.0f);
	Vector2 position = (Size2)viewport->get_size() * position_percentage;

	return position;
}

WebXRInterface::WebXRInterface() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
}

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (initialized) {
		uninitialize();
	};
}

#else // !WEB_ENABLED

void WebXRInterface::is_session_supported(const String &p_session_mode) {}
void WebXRInterface::set_session_mode(const String &p_session_mode) {}
String WebXRInterface::get_session_mode() const { return String(); }
void WebXRInterface::set_required_features(const String &p_required_features) {}
String WebXRInterface::get_required_features() const { return String(); }
void WebXRInterface::set_optional_features(const String &p_optional_features) {}
String WebXRInterface::get_optional_features() const { return String(); }
void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {}
String WebXRInterface::get_requested_reference_space_types() const { return String(); }
String WebXRInterface::get_reference_space_type() const { return String(); }
String WebXRInterface::get_enabled_features() const { return String(); }
bool WebXRInterface::is_input_source_active(int p_input_source_id) const { return false; }
Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const { return Ref<XRTracker>(); }
WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const { return TARGET_RAY_MODE_UNKNOWN; }
String WebXRInterface::get_visibility_state() const { return String(); }
float WebXRInterface::get_display_refresh_rate() const { return 0.0f; }
void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {}
Vector<float> WebXRInterface::get_available_display_refresh_rates() const { return Vector<float>(); }
Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() { return Vector<XRInterface::EnvironmentBlendMode>(); }
HashMap<String, String> WebXRInterface::get_system_info() { return HashMap<String, String>(); }

String WebXRInterface::get_name() const { return "WebXR"; }
uint32_t WebXRInterface::get_capabilities() const { return 0; }
bool WebXRInterface::is_initialized() const { return false; }
bool WebXRInterface::initialize() { return false; }
void WebXRInterface::uninitialize() {}

#endif // WEB_ENABLED

#ifdef WEB_ENABLED
#endif


#ifdef WEB_ENABLED
static Ref<WebXRInterface> webxr;
#endif

void initialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
#ifdef WEB_ENABLED
	if (XRServer::is_initialized()) {
		webxr.instantiate();
		XRServer::add_interface(webxr);
	}
#endif
}

void uninitialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef WEB_ENABLED
	if (webxr.is_valid()) {
		// uninitialize our interface if it is initialized
		if (webxr->is_initialized()) {
			webxr->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			XRServer::remove_interface(webxr);
		}

		// and release
		webxr.unref();
	}
#endif
}
