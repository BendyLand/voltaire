/**************************************************************************/
/*  webxr_interface_js.h                                                  */
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

#pragma once

#ifdef WEB_ENABLED

#include "webxr_interface.h"

#include "core/templates/rb_map.h"

/**
	The WebXR interface is a VR/AR interface that can be used on the web.
*/

namespace GLES3 {
class TextureStorage;
}

class XRHandTracker;

class WebXRInterfaceJS : public WebXRInterface {

private:
	bool initialized;
	Ref<XRPositionalTracker> head_tracker;
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
		Ref<XRControllerTracker> tracker;
		bool active = false;
		TargetRayMode target_ray_mode;
		int touch_index = -1;
	} input_sources[input_source_count];

	static const int WEBXR_HAND_JOINT_MAX = 25;
	static const int HAND_MAX = 2;
	Ref<XRHandTracker> hand_trackers[HAND_MAX];

	RID color_texture;
	RID depth_texture;

	RID _get_color_texture();
	RID _get_depth_texture();
	RID _get_texture(unsigned int p_texture_id);
	Transform3D _js_matrix_to_transform(float *p_js_matrix);
	void _update_input_source(int p_input_source_id);

	Vector2 _get_screen_position_from_joy_vector(const Vector2 &p_joy_vector);

public:
	virtual void is_session_supported(const String &p_session_mode) override;
	virtual void set_session_mode(const String &p_session_mode) override;
	virtual String get_session_mode() const override;
	virtual void set_required_features(const String &p_required_features) override;
	virtual String get_required_features() const override;
	virtual void set_optional_features(const String &p_optional_features) override;
	virtual String get_optional_features() const override;
	virtual void set_requested_reference_space_types(const String &p_requested_reference_space_types) override;
	virtual String get_requested_reference_space_types() const override;
	virtual String get_reference_space_type() const override;
	virtual String get_enabled_features() const override;
	virtual bool is_input_source_active(int p_input_source_id) const override;
	virtual Ref<XRControllerTracker> get_input_source_tracker(int p_input_source_id) const override;
	virtual TargetRayMode get_input_source_target_ray_mode(int p_input_source_id) const override;
	virtual String get_visibility_state() const override;
	virtual PackedVector3Array get_play_area() const override;

	virtual float get_display_refresh_rate() const override;
	virtual void set_display_refresh_rate(float p_refresh_rate) override;
	virtual Array get_available_display_refresh_rates() const override;

	virtual Array get_supported_environment_blend_modes() override;
	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) override;

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;
	virtual Dictionary get_system_info() override;

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

	WebXRInterfaceJS();
	~WebXRInterfaceJS();

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
};

#endif // WEB_ENABLED
