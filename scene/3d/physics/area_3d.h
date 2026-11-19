/**************************************************************************/
/*  area_3d.h                                                             */
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

#include "core/templates/vset.h"
#include "scene/3d/physics/collision_object_3d.h"

class Area3D : public Node3D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState3D::DisableMode;

private:
	CollisionState3D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector3 gravity_vec = Vector3(0, -1, 0);
	real_t gravity = 9.8f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t angular_damp = 0.1f;
	real_t linear_damp = 0.1f;

	int priority = 0;

	real_t wind_force_magnitude = 0.0f;
	real_t wind_attenuation_factor = 0.0f;
	NodePath wind_source_path;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	bool use_reverb_bus = false;
	String reverb_bus;
	float reverb_amount = 0.0f;
	float reverb_uniformity = 0.0f;

	void _clear_monitoring();
	void _initialize_wind();

protected:
	void _notification(int p_what);

public:
	Area3D();
	~Area3D();

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector3 &p_center);
	const Vector3 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector3 &p_direction);
	const Vector3 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_wind_force_magnitude(real_t p_wind_force_magnitude);
	real_t get_wind_force_magnitude() const;

	void set_wind_attenuation_factor(real_t p_wind_attenuation_factor);
	real_t get_wind_attenuation_factor() const;

	void set_wind_source_path(const NodePath &p_wind_source_path);
	const NodePath &get_wind_source_path() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	bool has_overlapping_bodies() const;
	bool has_overlapping_areas() const;

	bool overlaps_area(Node *rp_area) const;
	bool overlaps_body(Node *rp_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const String &p_audio_bus);
	String get_audio_bus_name() const;

	void set_use_reverb_bus(bool p_enable);
	bool is_using_reverb_bus() const;

	void set_reverb_bus_name(const String &p_audio_bus);
	String get_reverb_bus_name() const;

	void set_reverb_amount(float p_amount);
	float get_reverb_amount() const;

	void set_reverb_uniformity(float p_uniformity);
	float get_reverb_uniformity() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};
