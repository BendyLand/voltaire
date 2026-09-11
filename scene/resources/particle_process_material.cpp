/**************************************************************************/
/*  particle_process_material.cpp                                         */
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

#include "core/config/engine.h"
#include "core/version.h"
#include "particle_process_material.h"
#include "servers/rendering/rendering_server.h"

Mutex ParticleProcessMaterial::dirty_materials_mutex;
SelfList<ParticleProcessMaterial>::List ParticleProcessMaterial::dirty_materials;
Mutex ParticleProcessMaterial::shader_map_mutex;
HashMap<ParticleProcessMaterial::MaterialKey, ParticleProcessMaterial::ShaderData,
	ParticleProcessMaterial::MaterialKey>
	ParticleProcessMaterial::shader_map;
RBSet<String> ParticleProcessMaterial::min_max_properties;
ParticleProcessMaterial::ShaderNames* ParticleProcessMaterial::shader_names = nullptr;

void ParticleProcessMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->direction = "direction";
	shader_names->spread = "spread";
	shader_names->flatness = "flatness";
	shader_names->initial_linear_velocity_min = "initial_linear_velocity_min";
	shader_names->initial_angle_min = "initial_angle_min";
	shader_names->angular_velocity_min = "angular_velocity_min";
	shader_names->orbit_velocity_min = "orbit_velocity_min";
	shader_names->radial_velocity_min = "radial_velocity_min";
	shader_names->linear_accel_min = "linear_accel_min";
	shader_names->radial_accel_min = "radial_accel_min";
	shader_names->tangent_accel_min = "tangent_accel_min";
	shader_names->damping_min = "damping_min";
	shader_names->scale_min = "scale_min";
	shader_names->hue_variation_min = "hue_variation_min";
	shader_names->anim_speed_min = "anim_speed_min";
	shader_names->anim_offset_min = "anim_offset_min";
	shader_names->directional_velocity_min = "directional_velocity_min";
	shader_names->scale_over_velocity_min = "scale_over_velocity_min";
	shader_names->scale_3d_min = "scale_3d_min";
	shader_names->rotation_3d_min = "rotation_3d_min";

	shader_names->initial_linear_velocity_max = "initial_linear_velocity_max";
	shader_names->initial_angle_max = "initial_angle_max";
	shader_names->angular_velocity_max = "angular_velocity_max";
	shader_names->orbit_velocity_max = "orbit_velocity_max";
	shader_names->radial_velocity_max = "radial_velocity_max";
	shader_names->linear_accel_max = "linear_accel_max";
	shader_names->radial_accel_max = "radial_accel_max";
	shader_names->tangent_accel_max = "tangent_accel_max";
	shader_names->damping_max = "damping_max";
	shader_names->scale_max = "scale_max";
	shader_names->hue_variation_max = "hue_variation_max";
	shader_names->anim_speed_max = "anim_speed_max";
	shader_names->anim_offset_max = "anim_offset_max";
	shader_names->directional_velocity_max = "directional_velocity_max";
	shader_names->scale_over_velocity_max = "scale_over_velocity_max";
	shader_names->scale_3d_max = "scale_3d_max";
	shader_names->rotation_3d_max = "rotation_3d_max";

	shader_names->angle_texture = "angle_texture";
	shader_names->angular_velocity_texture = "angular_velocity_texture";
	shader_names->orbit_velocity_texture = "orbit_velocity_curve";
	shader_names->radial_velocity_texture = "radial_velocity_curve";
	shader_names->linear_accel_texture = "linear_accel_texture";
	shader_names->radial_accel_texture = "radial_accel_texture";
	shader_names->tangent_accel_texture = "tangent_accel_texture";
	shader_names->damping_texture = "damping_texture";
	shader_names->scale_texture = "scale_curve";
	shader_names->hue_variation_texture = "hue_rot_curve";
	shader_names->anim_speed_texture = "animation_speed_curve";
	shader_names->anim_offset_texture = "animation_offset_curve";
	shader_names->directional_velocity_texture = "directional_velocity_curve";
	shader_names->scale_over_velocity_texture = "scale_over_velocity_curve";

	shader_names->color = "color_value";
	shader_names->color_ramp = "color_ramp";
	shader_names->alpha_ramp = "alpha_curve";
	shader_names->emission_ramp = "emission_curve";
	shader_names->color_initial_ramp = "color_initial_ramp";
	shader_names->velocity_limit_curve = "velocity_limit_curve";
	shader_names->inherit_emitter_velocity_ratio = "inherit_emitter_velocity_ratio";
	shader_names->velocity_pivot = "velocity_pivot";
	shader_names->rotation_velocity_3d_max = "rotation_velocity_3d_max";
	shader_names->rotation_velocity_3d_min = "rotation_velocity_3d_min";
	shader_names->rotation_velocity_3d_curve = "rotation_velocity_3d_curve";

	shader_names->emission_sphere_radius = "emission_sphere_radius";
	shader_names->emission_box_extents = "emission_box_extents";
	shader_names->emission_texture_point_count = "emission_texture_point_count";
	shader_names->emission_texture_points = "emission_texture_points";
	shader_names->emission_texture_normal = "emission_texture_normal";
	shader_names->emission_texture_color = "emission_texture_color";
	shader_names->emission_ring_axis = "emission_ring_axis";
	shader_names->emission_ring_height = "emission_ring_height";
	shader_names->emission_ring_radius = "emission_ring_radius";
	shader_names->emission_ring_inner_radius = "emission_ring_inner_radius";
	shader_names->emission_ring_cone_angle = "emission_ring_cone_angle";
	shader_names->emission_shape_offset = "emission_shape_offset";
	shader_names->emission_shape_scale = "emission_shape_scale";

	shader_names->turbulence_enabled = "turbulence_enabled";
	shader_names->turbulence_noise_strength = "turbulence_noise_strength";
	shader_names->turbulence_noise_scale = "turbulence_noise_scale";
	shader_names->turbulence_noise_speed = "turbulence_noise_speed";
	shader_names->turbulence_noise_speed_random = "turbulence_noise_speed_random";
	shader_names->turbulence_influence_over_life = "turbulence_influence_over_life";
	shader_names->turbulence_influence_min = "turbulence_influence_min";
	shader_names->turbulence_influence_max = "turbulence_influence_max";
	shader_names->turbulence_initial_displacement_min = "turbulence_initial_displacement_min";
	shader_names->turbulence_initial_displacement_max = "turbulence_initial_displacement_max";

	shader_names->gravity = "gravity";

	shader_names->lifetime_randomness = "lifetime_randomness";

	shader_names->sub_emitter_frequency = "sub_emitter_frequency";
	shader_names->sub_emitter_amount_at_end = "sub_emitter_amount_at_end";
	shader_names->sub_emitter_amount_at_collision = "sub_emitter_amount_at_collision";
	shader_names->sub_emitter_amount_at_start = "sub_emitter_amount_at_start";
	shader_names->sub_emitter_keep_velocity = "sub_emitter_keep_velocity";

	shader_names->collision_friction = "collision_friction";
	shader_names->collision_bounce = "collision_bounce";
}

void ParticleProcessMaterial::finish_shaders()
{
	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void ParticleProcessMaterial::flush_changes()
{
	MutexLock lock(dirty_materials_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void ParticleProcessMaterial::_queue_shader_change()
{
	if (!_is_initialized()) {
		return;
	}

	MutexLock lock(dirty_materials_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

bool ParticleProcessMaterial::has_min_max_property(const String& p_name)
{
	return min_max_properties.has(p_name);
}

Vector3 ParticleProcessMaterial::get_direction() const { return direction; }

float ParticleProcessMaterial::get_spread() const { return spread; }

float ParticleProcessMaterial::get_flatness() const { return flatness; }

Vector3 ParticleProcessMaterial::get_velocity_pivot() { return velocity_pivot; }

void ParticleProcessMaterial::set_param(Parameter p_param, const Vector2& p_value)
{
	set_param_min(p_param, p_value.x);
	set_param_max(p_param, p_value.y);
}

Vector2 ParticleProcessMaterial::get_param(Parameter p_param) const
{
	return Vector2(get_param_min(p_param), get_param_max(p_param));
}

float ParticleProcessMaterial::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_min[p_param];
}

float ParticleProcessMaterial::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_max[p_param];
}

static void _adjust_curve_range(const Ref<Texture2D>& p_texture, float p_min, float p_max)
{
	Ref<CurveTexture> curve_tex = p_texture;
	if (curve_tex.is_valid()) {
		curve_tex->ensure_default_setup(p_min, p_max);
		return;
	}
	Ref<CurveXYZTexture> curve_xyz_tex = p_texture;
	if (curve_xyz_tex.is_valid()) {
		curve_xyz_tex->ensure_default_setup(p_min, p_max);
		return;
	}
}

Ref<Texture2D> ParticleProcessMaterial::get_param_texture(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Texture2D>());

	return tex_parameters[p_param];
}

Color ParticleProcessMaterial::get_color() const { return color; }

Ref<Texture2D> ParticleProcessMaterial::get_color_ramp() const { return color_ramp; }

Ref<Texture2D> ParticleProcessMaterial::get_color_initial_ramp() const
{
	return color_initial_ramp;
}

Ref<Texture2D> ParticleProcessMaterial::get_alpha_curve() const { return alpha_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_curve() const { return emission_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_velocity_limit_curve() const
{
	return velocity_limit_curve;
}

bool ParticleProcessMaterial::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

ParticleProcessMaterial::EmissionShape ParticleProcessMaterial::get_emission_shape() const
{
	return emission_shape;
}

real_t ParticleProcessMaterial::get_emission_sphere_radius() const
{
	return emission_sphere_radius;
}

Vector3 ParticleProcessMaterial::get_emission_box_extents() const { return emission_box_extents; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_point_texture() const
{
	return emission_point_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_normal_texture() const
{
	return emission_normal_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_color_texture() const
{
	return emission_color_texture;
}

int ParticleProcessMaterial::get_emission_point_count() const { return emission_point_count; }

Vector3 ParticleProcessMaterial::get_emission_ring_axis() const { return emission_ring_axis; }

real_t ParticleProcessMaterial::get_emission_ring_height() const { return emission_ring_height; }

real_t ParticleProcessMaterial::get_emission_ring_radius() const { return emission_ring_radius; }

real_t ParticleProcessMaterial::get_emission_ring_inner_radius() const
{
	return emission_ring_inner_radius;
}

real_t ParticleProcessMaterial::get_emission_ring_cone_angle() const
{
	return emission_ring_cone_angle;
}

Vector3 ParticleProcessMaterial::get_emission_shape_offset() const { return emission_shape_offset; }

Vector3 ParticleProcessMaterial::get_emission_shape_scale() const { return emission_shape_scale; }

double ParticleProcessMaterial::get_inherit_velocity_ratio()
{
	return inherit_emitter_velocity_ratio;
}

bool ParticleProcessMaterial::is_using_scale_3d() const { return use_scale_3d; }

Vector3 ParticleProcessMaterial::get_scale_3d_min() const { return scale_3d_min; }

Vector3 ParticleProcessMaterial::get_scale_3d_max() const { return scale_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_3d() const { return use_rotation_3d; }

Vector3 ParticleProcessMaterial::get_rotation_3d_min() const { return rotation_3d_min; }

Vector3 ParticleProcessMaterial::get_rotation_3d_max() const { return rotation_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_velocity_3d() const
{
	return using_rotation_velocity_3d;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_min() const
{
	return rotation_velocity_3d_min;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_max() const
{
	return rotation_velocity_3d_max;
}

Ref<Texture2D> ParticleProcessMaterial::get_rotation_velocity_3d_curve() const
{
	return rotation_velocity_3d_curve;
}

bool ParticleProcessMaterial::get_turbulence_enabled() const { return turbulence_enabled; }

float ParticleProcessMaterial::get_turbulence_noise_strength() const
{
	return turbulence_noise_strength;
}

float ParticleProcessMaterial::get_turbulence_noise_scale() const { return turbulence_noise_scale; }

float ParticleProcessMaterial::get_turbulence_noise_speed_random() const
{
	return turbulence_noise_speed_random;
}

Vector3 ParticleProcessMaterial::get_turbulence_noise_speed() const
{
	return turbulence_noise_speed;
}

Vector3 ParticleProcessMaterial::get_gravity() const { return gravity; }

double ParticleProcessMaterial::get_lifetime_randomness() const { return lifetime_randomness; }

RID ParticleProcessMaterial::get_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return Material::get_rid();
}

RID ParticleProcessMaterial::get_shader_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return shader_rid;
}

ParticleProcessMaterial::SubEmitterMode ParticleProcessMaterial::get_sub_emitter_mode() const
{
	return sub_emitter_mode;
}

double ParticleProcessMaterial::get_sub_emitter_frequency() const { return sub_emitter_frequency; }

int ParticleProcessMaterial::get_sub_emitter_amount_at_end() const
{
	return sub_emitter_amount_at_end;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_collision() const
{
	return sub_emitter_amount_at_collision;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_start() const
{
	return sub_emitter_amount_at_start;
}

bool ParticleProcessMaterial::get_sub_emitter_keep_velocity() const
{
	return sub_emitter_keep_velocity;
}

void ParticleProcessMaterial::set_attractor_interaction_enabled(bool p_enable)
{
	attractor_interaction_enabled = p_enable;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_attractor_interaction_enabled() const
{
	return attractor_interaction_enabled;
}

ParticleProcessMaterial::CollisionMode ParticleProcessMaterial::get_collision_mode() const
{
	return collision_mode;
}

void ParticleProcessMaterial::set_collision_use_scale(bool p_scale)
{
	collision_scale = p_scale;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_collision_using_scale() const { return collision_scale; }

float ParticleProcessMaterial::get_collision_friction() const { return collision_friction; }

float ParticleProcessMaterial::get_collision_bounce() const { return collision_bounce; }

Shader::Mode ParticleProcessMaterial::get_shader_mode() const { return Shader::MODE_PARTICLES; }

ParticleProcessMaterial::ParticleProcessMaterial() : element(this)
{
	_set_material(RS::get_singleton()->material_create());

	set_direction(Vector3(1, 0, 0));
	set_spread(45);
	set_flatness(0);
	set_param_min(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_min(PARAM_ANGULAR_VELOCITY, 0);
	set_param_min(PARAM_ORBIT_VELOCITY, 0);
	set_param_min(PARAM_LINEAR_ACCEL, 0);
	set_param_min(PARAM_RADIAL_ACCEL, 0);
	set_param_min(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_min(PARAM_DAMPING, 0);
	set_param_min(PARAM_ANGLE, 0);
	set_param_min(PARAM_SCALE, 1);
	set_param_min(PARAM_HUE_VARIATION, 0);
	set_param_min(PARAM_ANIM_SPEED, 0);
	set_param_min(PARAM_ANIM_OFFSET, 0);
	set_param_max(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_max(PARAM_ANGULAR_VELOCITY, 0);
	set_param_max(PARAM_ORBIT_VELOCITY, 0);
	set_param_max(PARAM_LINEAR_ACCEL, 0);
	set_param_max(PARAM_RADIAL_ACCEL, 0);
	set_param_max(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_max(PARAM_DAMPING, 0);
	set_param_max(PARAM_ANGLE, 0);
	set_param_max(PARAM_SCALE, 1);
	set_param_max(PARAM_HUE_VARIATION, 0);
	set_param_max(PARAM_ANIM_SPEED, 0);
	set_param_max(PARAM_ANIM_OFFSET, 0);
	set_param_min(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_param_max(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_use_scale_3d(false);
	set_scale_3d_min(Vector3(1.0, 1.0, 1.0));
	set_scale_3d_max(Vector3(1.0, 1.0, 1.0));
	set_use_rotation_3d(false);
	set_rotation_3d_min(Vector3(0.0, 0.0, 0.0));
	set_rotation_3d_max(Vector3(0.0, 0.0, 0.0));
	set_emission_shape(EMISSION_SHAPE_POINT);
	set_emission_sphere_radius(1);
	set_emission_box_extents(Vector3(1, 1, 1));
	set_emission_ring_axis(Vector3(0, 0, 1.0));
	set_emission_ring_height(1);
	set_emission_ring_radius(1);
	set_emission_ring_inner_radius(0);
	set_emission_ring_cone_angle(90);
	set_emission_shape_offset(Vector3(0.0, 0.0, 0.0));
	set_emission_shape_scale(Vector3(1.0, 1.0, 1.0));

	set_turbulence_enabled(false);
	set_turbulence_noise_speed(Vector3(0.0, 0.0, 0.0));
	set_turbulence_noise_strength(1);
	set_turbulence_noise_scale(9);
	set_turbulence_noise_speed_random(0.2);
	set_param_min(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_max(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_min(PARAM_TURB_INIT_DISPLACEMENT, 0.0);
	set_param_max(PARAM_TURB_INIT_DISPLACEMENT, 0.0);

	set_gravity(Vector3(0, -9.8, 0));
	set_lifetime_randomness(0);

	set_sub_emitter_mode(SUB_EMITTER_DISABLED);
	set_sub_emitter_frequency(4);
	set_sub_emitter_amount_at_end(1);
	set_sub_emitter_amount_at_collision(1);
	set_sub_emitter_amount_at_start(1);
	set_sub_emitter_keep_velocity(false);

	set_attractor_interaction_enabled(true);
	set_collision_mode(COLLISION_DISABLED);
	set_collision_bounce(0.0);
	set_collision_friction(0.0);
	set_collision_use_scale(false);

	for (int i = 0; i < PARTICLE_FLAG_MAX; i++) {
		particle_flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));

	current_key.invalid_key = 1;
}

ParticleProcessMaterial::~ParticleProcessMaterial()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	MutexLock lock(shader_map_mutex);

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			RS::get_singleton()->free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		RS::get_singleton()->material_set_shader(_get_material(), RID());
	}
}


