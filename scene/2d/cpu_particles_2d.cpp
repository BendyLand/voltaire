/**************************************************************************/
/*  cpu_particles_2d.cpp                                                  */
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
#include "core/math/random_number_generator.h"
#include "core/math/transform_interpolator.h"
#include "cpu_particles_2d.compat.inc"
#include "cpu_particles_2d.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/particle_process_material.h"
#include "servers/rendering/rendering_server.h"

void CPUParticles2D::set_emitting(bool p_emitting)
{
	if (emitting == p_emitting) {
		return;
	}

	if (p_emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}

	emitting = p_emitting;
	if (emitting) {
		_set_emitting();
	}
}

void CPUParticles2D::_set_emitting()
{
	active = true;
	set_process_internal(true);
	// first update before rendering to avoid one frame delay after emitting starts
	if (time == 0) {
		_update_internal();
	}
}

void CPUParticles2D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles must be greater than 0.");

	particles.resize(p_amount);
	{
		Particle* w = particles.ptrw();

		for (int i = 0; i < p_amount; i++) {
			w[i].active = false;
		}
	}

	particle_data.resize((8 + 4 + 4) * p_amount);
	RS::get_singleton()->multimesh_allocate_data(
		multimesh, p_amount, RSE::MULTIMESH_TRANSFORM_2D, true, true);

	particle_order.resize(p_amount);
}

void CPUParticles2D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
}

void CPUParticles2D::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

void CPUParticles2D::set_pre_process_time(double p_time) { pre_process_time = p_time; }

void CPUParticles2D::set_explosiveness_ratio(real_t p_ratio) { explosiveness_ratio = p_ratio; }

void CPUParticles2D::set_randomness_ratio(real_t p_ratio) { randomness_ratio = p_ratio; }

void CPUParticles2D::set_lifetime_randomness(double p_random) { lifetime_randomness = p_random; }

void CPUParticles2D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;

	// Prevent sending item transforms when using global coords,
	// and inform the RenderingServer to use identity mode.
	set_canvas_item_use_identity_transform(!local_coords);

	// We only need NOTIFICATION_TRANSFORM_CHANGED
	// when following an interpolated target.

#ifdef TOOLS_ENABLED
	set_notify_transform(_interpolation_data.interpolated_follow ||
						 (Engine::get_singleton()->is_editor_hint() && !local_coords));
#else
	set_notify_transform(_interpolation_data.interpolated_follow);
#endif

	queue_redraw();
}

void CPUParticles2D::set_speed_scale(double p_scale) { speed_scale = p_scale; }

bool CPUParticles2D::is_emitting() const { return emitting; }

int CPUParticles2D::get_amount() const { return particles.size(); }

double CPUParticles2D::get_lifetime() const { return lifetime; }

bool CPUParticles2D::get_one_shot() const { return one_shot; }

double CPUParticles2D::get_pre_process_time() const { return pre_process_time; }

real_t CPUParticles2D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t CPUParticles2D::get_randomness_ratio() const { return randomness_ratio; }

double CPUParticles2D::get_lifetime_randomness() const { return lifetime_randomness; }

bool CPUParticles2D::get_use_local_coordinates() const { return local_coords; }

double CPUParticles2D::get_speed_scale() const { return speed_scale; }

void CPUParticles2D::set_draw_order(DrawOrder p_order) { draw_order = p_order; }

CPUParticles2D::DrawOrder CPUParticles2D::get_draw_order() const { return draw_order; }

void CPUParticles2D::_texture_changed()
{
	if (texture.is_valid()) {
		queue_redraw();
		_update_mesh_texture();
	}
}

void CPUParticles2D::_refresh_interpolation_state()
{
	if (!is_inside_tree()) {
		return;
	}

	// The logic for whether to do an interpolated follow.
	// This is rather complex, but basically:
	// If project setting interpolation is ON and this particle system is in global mode,
	// we will follow the INTERPOLATED position rather than the actual position.
	// This is so that particles aren't generated AHEAD of the interpolated parent.
	bool follow = !local_coords && get_tree()->is_physics_interpolation_enabled();

	if (follow == _interpolation_data.interpolated_follow) {
		return;
	}

	_interpolation_data.interpolated_follow = follow;

	set_physics_process_internal(_interpolation_data.interpolated_follow);
}

Ref<Texture2D> CPUParticles2D::get_texture() const { return texture; }

void CPUParticles2D::set_fixed_fps(int p_count) { fixed_fps = p_count; }

int CPUParticles2D::get_fixed_fps() const { return fixed_fps; }

void CPUParticles2D::set_fractional_delta(bool p_enable) { fractional_delta = p_enable; }

bool CPUParticles2D::get_fractional_delta() const { return fractional_delta; }

void CPUParticles2D::restart(bool p_keep_seed)
{
	time = 0;
	frame_remainder = 0;
	cycle = 0;
	emitting = false;

	{
		int pc = particles.size();
		Particle* w = particles.ptrw();

		for (int i = 0; i < pc; i++) {
			w[i].active = false;
		}
	}
	if (!p_keep_seed && !use_fixed_seed) {
		seed = Math::rand();
	}

	emitting = true;
	_set_emitting();
}

void CPUParticles2D::set_direction(Vector2 p_direction) { direction = p_direction; }

Vector2 CPUParticles2D::get_direction() const { return direction; }

void CPUParticles2D::set_spread(real_t p_spread) { spread = p_spread; }

real_t CPUParticles2D::get_spread() const { return spread; }

void CPUParticles2D::set_param_min(Parameter p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	parameters_min[p_param] = p_value;
	if (parameters_min[p_param] > parameters_max[p_param]) {
		set_param_max(p_param, p_value);
	}
}

real_t CPUParticles2D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
}

void CPUParticles2D::set_param_max(Parameter p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	parameters_max[p_param] = p_value;
	if (parameters_min[p_param] > parameters_max[p_param]) {
		set_param_min(p_param, p_value);
	}

	update_configuration_warnings();
}

real_t CPUParticles2D::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_max[p_param];
}

static void _adjust_curve_range(const Ref<Curve>& p_curve, real_t p_min, real_t p_max)
{
	Ref<Curve> curve = p_curve;
	if (curve.is_null()) {
		return;
	}

	curve->ensure_default_setup(p_min, p_max);
}

void CPUParticles2D::set_param_curve(Parameter p_param, const Ref<Curve>& p_curve)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	curve_parameters[p_param] = p_curve;

	switch (p_param) {
	case PARAM_INITIAL_LINEAR_VELOCITY: {
		// do none for this one
	} break;
	case PARAM_ANGULAR_VELOCITY: {
		_adjust_curve_range(p_curve, -360, 360);
	} break;
	case PARAM_ORBIT_VELOCITY: {
		_adjust_curve_range(p_curve, -500, 500);
	} break;
	case PARAM_LINEAR_ACCEL: {
		_adjust_curve_range(p_curve, -200, 200);
	} break;
	case PARAM_RADIAL_ACCEL: {
		_adjust_curve_range(p_curve, -200, 200);
	} break;
	case PARAM_TANGENTIAL_ACCEL: {
		_adjust_curve_range(p_curve, -200, 200);
	} break;
	case PARAM_DAMPING: {
		_adjust_curve_range(p_curve, 0, 100);
	} break;
	case PARAM_ANGLE: {
		_adjust_curve_range(p_curve, -360, 360);
	} break;
	case PARAM_SCALE: {
	} break;
	case PARAM_HUE_VARIATION: {
		_adjust_curve_range(p_curve, -1, 1);
	} break;
	case PARAM_ANIM_SPEED: {
		_adjust_curve_range(p_curve, 0, 200);
	} break;
	case PARAM_ANIM_OFFSET: {
	} break;
	default: {
	}
	}

	update_configuration_warnings();
}

Ref<Curve> CPUParticles2D::get_param_curve(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Curve>());

	return curve_parameters[p_param];
}

void CPUParticles2D::set_color(const Color& p_color) { color = p_color; }

Color CPUParticles2D::get_color() const { return color; }

void CPUParticles2D::set_color_ramp(const Ref<Gradient>& p_ramp) { color_ramp = p_ramp; }

Ref<Gradient> CPUParticles2D::get_color_ramp() const { return color_ramp; }

void CPUParticles2D::set_color_initial_ramp(const Ref<Gradient>& p_ramp)
{
	color_initial_ramp = p_ramp;
}

Ref<Gradient> CPUParticles2D::get_color_initial_ramp() const { return color_initial_ramp; }

void CPUParticles2D::set_particle_flag(ParticleFlags p_particle_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_particle_flag, PARTICLE_FLAG_MAX);
	particle_flags[p_particle_flag] = p_enable;
}

bool CPUParticles2D::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

void CPUParticles2D::set_emission_sphere_radius(real_t p_radius)
{
	if (p_radius == emission_sphere_radius) {
		return;
	}
	emission_sphere_radius = p_radius;
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
#endif
}

void CPUParticles2D::set_emission_rect_extents(Vector2 p_extents)
{
	if (p_extents == emission_rect_extents) {
		return;
	}
	emission_rect_extents = p_extents;
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
#endif
}

void CPUParticles2D::set_emission_points(const Vector<Vector2>& p_points)
{
	emission_points = p_points;
}

void CPUParticles2D::set_emission_normals(const Vector<Vector2>& p_normals)
{
	emission_normals = p_normals;
}

void CPUParticles2D::set_emission_colors(const Vector<Color>& p_colors)
{
	emission_colors = p_colors;
}

void CPUParticles2D::set_emission_ring_inner_radius(real_t p_inner_radius)
{
	emission_ring_inner_radius = p_inner_radius;
}

void CPUParticles2D::set_emission_ring_radius(real_t p_ring_radius)
{
	emission_ring_radius = p_ring_radius;
}

real_t CPUParticles2D::get_emission_sphere_radius() const { return emission_sphere_radius; }

Vector2 CPUParticles2D::get_emission_rect_extents() const { return emission_rect_extents; }

Vector<Vector2> CPUParticles2D::get_emission_points() const { return emission_points; }

Vector<Vector2> CPUParticles2D::get_emission_normals() const { return emission_normals; }

Vector<Color> CPUParticles2D::get_emission_colors() const { return emission_colors; }

real_t CPUParticles2D::get_emission_ring_inner_radius() const { return emission_ring_inner_radius; }

real_t CPUParticles2D::get_emission_ring_radius() const { return emission_ring_radius; }

CPUParticles2D::EmissionShape CPUParticles2D::get_emission_shape() const { return emission_shape; }

void CPUParticles2D::set_gravity(const Vector2& p_gravity) { gravity = p_gravity; }

Vector2 CPUParticles2D::get_gravity() const { return gravity; }

void CPUParticles2D::set_scale_curve_x(Ref<Curve> p_scale_curve) { scale_curve_x = p_scale_curve; }

void CPUParticles2D::set_scale_curve_y(Ref<Curve> p_scale_curve) { scale_curve_y = p_scale_curve; }

Ref<Curve> CPUParticles2D::get_scale_curve_x() const { return scale_curve_x; }

Ref<Curve> CPUParticles2D::get_scale_curve_y() const { return scale_curve_y; }

bool CPUParticles2D::get_split_scale() { return split_scale; }

void CPUParticles2D::set_use_fixed_seed(bool p_use_fixed_seed)
{
	if (p_use_fixed_seed == use_fixed_seed) {
		return;
	}
	use_fixed_seed = p_use_fixed_seed;
}

bool CPUParticles2D::get_use_fixed_seed() const { return use_fixed_seed; }

void CPUParticles2D::set_seed(uint32_t p_seed) { seed = p_seed; }

#ifdef TOOLS_ENABLED
void CPUParticles2D::set_show_gizmos(bool p_show_gizmos)
{
	if (show_gizmos == p_show_gizmos) {
		return;
	}
	show_gizmos = p_show_gizmos;
	queue_redraw();
}
#endif

uint32_t CPUParticles2D::get_seed() const { return seed; }

void CPUParticles2D::request_particles_process(
	real_t p_request_process_time, real_t p_request_process_time_residual)
{
	_request_process_time = p_request_process_time;
	_request_process_time_residual = p_request_process_time_residual;
	_update_internal();
}

static uint32_t idhash(uint32_t x)
{
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = (x >> uint32_t(16)) ^ x;
	return x;
}

static real_t rand_from_seed(uint32_t& seed)
{
	int k;
	int s = int(seed);
	if (s == 0) {
		s = 305420679;
	}
	k = s / 127773;
	s = 16807 * (s - k * 127773) - 2836 * k;
	if (s < 0) {
		s += 2147483647;
	}
	seed = uint32_t(s);
	return (seed % uint32_t(65536)) / 65535.0;
}

void CPUParticles2D::_update_internal()
{
	if (particles.is_empty() || !is_visible_in_tree()) {
		_set_do_redraw(false);
		return;
	}

	// Change update mode?
	_refresh_interpolation_state();

	double delta = get_process_delta_time();
	if (!active && !emitting) {
		set_process_internal(false);
		_set_do_redraw(false);

		// reset variables
		time = 0;
		frame_remainder = 0;
		cycle = 0;
		return;
	}

	_set_do_redraw(true);
	{
		float todo = time == 0 ? pre_process_time : 0;
		todo = todo > _request_process_time ? todo : _request_process_time;
		todo = todo > _request_process_time_residual ? todo : _request_process_time_residual;

		if (todo > 0.0) {
			real_t frame_time;
			if (fixed_fps > 0) {
				frame_time = 1.0 / fixed_fps;
			}
			else {
				frame_time = 1.0 / 30.0;
			}

			float tmp_scale = speed_scale;
			// We need this otherwise the speed scale of the particle system influences the `todo`.
			speed_scale = 1.0;
			if (time == 0) {
				todo = pre_process_time;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time > 0.0) {
				todo = _request_process_time;
				emitting = true;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time_residual > 0.0) {
				emitting = false;
				todo = _request_process_time_residual;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			speed_scale = tmp_scale;
		}
		_request_process_time = 0;
		_request_process_time_residual = 0;
	}
	double frame_time;
	if (fixed_fps > 0) {
		frame_time = 1.0 / fixed_fps;
	}
	else {
		frame_time = 1.0 / 30.0;
	}
	if (fixed_fps > 0) {
		double decr = frame_time;

		double ldelta = delta;
		if (ldelta > 0.1) { // avoid recursive stalls if fps goes below 10
			ldelta = 0.1;
		}
		else if (ldelta < 0.0) {
			ldelta = 0.0;
		}
		double todo = frame_remainder + ldelta;

		while (todo >= frame_time) {
			_particles_process(frame_time);
			todo -= decr;
		}
		frame_remainder = todo;

	}
	else {
		_particles_process(delta);
	}

	_update_particle_data_buffer();
}

void CPUParticles2D::_particles_process(double p_delta)
{
	p_delta *= speed_scale;

	int pcount = particles.size();
	Particle* w = particles.ptrw();

	Particle* parray = w;

	double prev_time = time;
	time += p_delta;
	if (time > lifetime) {
		time = Math::fmod(time, lifetime);
		cycle++;
		if (one_shot && cycle > 0) {
			set_emitting(false);
		}
	}

	Transform2D emission_xform;
	Transform2D velocity_xform;
	if (!local_coords) {
		if (!_interpolation_data.interpolated_follow) {
			emission_xform = get_global_transform();
		}
		else {
			TransformInterpolator::interpolate_transform_2d(_interpolation_data.global_xform_prev,
				_interpolation_data.global_xform_curr, emission_xform,
				Engine::get_singleton()->get_physics_interpolation_fraction());
		}
		velocity_xform = emission_xform;
		velocity_xform[2] = Vector2();
	}

	double system_phase = time / lifetime;

	bool should_be_active = false;
	for (int i = 0; i < pcount; i++) {
		Particle& p = parray[i];

		if (!emitting && !p.active) {
			continue;
		}

		double local_delta = p_delta;

		// The phase is a ratio between 0 (birth) and 1 (end of life) for each particle.
		// While we use time in tests later on, for randomness we use the phase as done in the
		// original shader code, and we later multiply by lifetime to get the time.
		double restart_phase = double(i) / double(pcount);

		if (randomness_ratio > 0.0) {
			uint32_t _seed = cycle;
			if (restart_phase >= system_phase) {
				_seed -= uint32_t(1);
			}
			_seed *= uint32_t(pcount);
			_seed += uint32_t(i);
			double random = double(idhash(_seed) % uint32_t(65536)) / 65536.0;
			restart_phase += randomness_ratio * random * 1.0 / double(pcount);
		}

		restart_phase *= (1.0 - explosiveness_ratio);
		double restart_time = restart_phase * lifetime;
		bool restart = false;

		if (time > prev_time) {
			// restart_time >= prev_time is used so particles emit in the first frame they are
			// processed

			if (restart_time >= prev_time && restart_time < time) {
				restart = true;
				if (fractional_delta) {
					local_delta = time - restart_time;
				}
			}

		}
		else if (local_delta > 0.0) {
			if (restart_time >= prev_time) {
				restart = true;
				if (fractional_delta) {
					local_delta = lifetime - restart_time + time;
				}

			}
			else if (restart_time < time) {
				restart = true;
				if (fractional_delta) {
					local_delta = time - restart_time;
				}
			}
		}

		if (p.time * (1.0 - explosiveness_ratio) > p.lifetime) {
			restart = true;
		}

		float tv = 0.0;

		if (restart) {
			if (!emitting) {
				p.active = false;
				continue;
			}
			p.active = true;

			/*real_t tex_linear_velocity = 0;
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				tex_linear_velocity = curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY]->sample(0);
			}*/

			real_t tex_angle = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_angle = curve_parameters[PARAM_ANGLE]->sample(tv);
			}

			real_t tex_anim_offset = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_anim_offset = curve_parameters[PARAM_ANGLE]->sample(tv);
			}

			p.seed = seed + uint32_t(i) + i + cycle;
			rng->set_seed(p.seed);

			p.angle_rand = rng->randf();
			p.scale_rand = rng->randf();
			p.hue_rot_rand = rng->randf();
			p.anim_offset_rand = rng->randf();

			if (color_initial_ramp.is_valid()) {
				p.start_color_rand = color_initial_ramp->get_color_at_offset(rng->randf());
			}
			else {
				p.start_color_rand = Color(1, 1, 1, 1);
			}

			real_t angle1_rad =
				direction.angle() + Math::deg_to_rad((rng->randf() * 2.0 - 1.0) * spread);
			Vector2 rot = Vector2(Math::cos(angle1_rad), Math::sin(angle1_rad));
			p.velocity = rot * Math::lerp(parameters_min[PARAM_INITIAL_LINEAR_VELOCITY],
								   parameters_max[PARAM_INITIAL_LINEAR_VELOCITY], rng->randf());

			real_t base_angle = tex_angle * Math::lerp(parameters_min[PARAM_ANGLE],
												parameters_max[PARAM_ANGLE], p.angle_rand);
			p.rotation = Math::deg_to_rad(base_angle);

			p.custom[0] = 0.0; // unused
			p.custom[1] = 0.0; // phase [0..1]
			p.custom[2] =
				tex_anim_offset * Math::lerp(parameters_min[PARAM_ANIM_OFFSET],
									  parameters_max[PARAM_ANIM_OFFSET], p.anim_offset_rand);
			p.custom[3] = (1.0 - rng->randf() * lifetime_randomness);
			p.transform = Transform2D();
			p.time = 0;
			p.lifetime = lifetime * p.custom[3];
			p.base_color = Color(1, 1, 1, 1);

			switch (emission_shape) {
			case EMISSION_SHAPE_POINT: {
				// do none
			} break;
			case EMISSION_SHAPE_SPHERE: {
				real_t t = Math::TAU * rng->randf();
				real_t radius = emission_sphere_radius * rng->randf();
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_SPHERE_SURFACE: {
				real_t s = rng->randf(), t = Math::TAU * rng->randf();
				real_t radius = emission_sphere_radius * Math::sqrt(1.0 - s * s);
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_RECTANGLE: {
				p.transform[2] = Vector2(rng->randf() * 2.0 - 1.0, rng->randf() * 2.0 - 1.0) *
								 emission_rect_extents;
			} break;
			case EMISSION_SHAPE_POINTS:
			case EMISSION_SHAPE_DIRECTED_POINTS: {
				int pc = emission_points.size();
				if (pc == 0) {
					break;
				}

				int random_idx = Math::rand() % pc;

				p.transform[2] = emission_points.get(random_idx);

				if (emission_shape == EMISSION_SHAPE_DIRECTED_POINTS &&
					emission_normals.size() == pc) {
					Vector2 normal = emission_normals.get(random_idx);
					Transform2D m2;
					m2.columns[0] = normal;
					m2.columns[1] = normal.orthogonal();
					p.velocity = m2.basis_xform(p.velocity);
				}

				if (emission_colors.size() == pc) {
					p.base_color = emission_colors.get(random_idx);
				}
			} break;
			case EMISSION_SHAPE_RING: {
				real_t t = Math::TAU * Math::randf();
				real_t outer_sq = emission_ring_radius * emission_ring_radius;
				real_t inner_sq = emission_ring_inner_radius * emission_ring_inner_radius;
				real_t radius = Math::sqrt(Math::randf() * (outer_sq - inner_sq) + inner_sq);
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_MAX: { // Max value for validity check.
				break;
			}
			}

			if (!local_coords) {
				p.velocity = velocity_xform.xform(p.velocity);
				p.transform = emission_xform * p.transform;
			}

		}
		else if (!p.active) {
			continue;
		}
		else if (p.time >= p.lifetime) {
			p.active = false;
			tv = 1.0;
		}
		else {
			uint32_t _seed = p.seed;
			p.time += local_delta;
			p.custom[1] = p.time / lifetime;
			tv = p.time / p.lifetime;

			real_t tex_linear_velocity = 1.0;
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				tex_linear_velocity = curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY]->sample(tv);
			}

			real_t tex_orbit_velocity = 1.0;
			if (curve_parameters[PARAM_ORBIT_VELOCITY].is_valid()) {
				tex_orbit_velocity = curve_parameters[PARAM_ORBIT_VELOCITY]->sample(tv);
			}

			real_t tex_angular_velocity = 1.0;
			if (curve_parameters[PARAM_ANGULAR_VELOCITY].is_valid()) {
				tex_angular_velocity = curve_parameters[PARAM_ANGULAR_VELOCITY]->sample(tv);
			}

			real_t tex_linear_accel = 1.0;
			if (curve_parameters[PARAM_LINEAR_ACCEL].is_valid()) {
				tex_linear_accel = curve_parameters[PARAM_LINEAR_ACCEL]->sample(tv);
			}

			real_t tex_tangential_accel = 1.0;
			if (curve_parameters[PARAM_TANGENTIAL_ACCEL].is_valid()) {
				tex_tangential_accel = curve_parameters[PARAM_TANGENTIAL_ACCEL]->sample(tv);
			}

			real_t tex_radial_accel = 1.0;
			if (curve_parameters[PARAM_RADIAL_ACCEL].is_valid()) {
				tex_radial_accel = curve_parameters[PARAM_RADIAL_ACCEL]->sample(tv);
			}

			real_t tex_damping = 1.0;
			if (curve_parameters[PARAM_DAMPING].is_valid()) {
				tex_damping = curve_parameters[PARAM_DAMPING]->sample(tv);
			}

			real_t tex_angle = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_angle = curve_parameters[PARAM_ANGLE]->sample(tv);
			}
			real_t tex_anim_speed = 1.0;
			if (curve_parameters[PARAM_ANIM_SPEED].is_valid()) {
				tex_anim_speed = curve_parameters[PARAM_ANIM_SPEED]->sample(tv);
			}

			real_t tex_anim_offset = 1.0;
			if (curve_parameters[PARAM_ANIM_OFFSET].is_valid()) {
				tex_anim_offset = curve_parameters[PARAM_ANIM_OFFSET]->sample(tv);
			}

			Vector2 force = gravity;
			Vector2 pos = p.transform[2];

			// apply linear acceleration
			force += p.velocity.length() > 0.0f
						 ? p.velocity.normalized() * tex_linear_accel *
							   Math::lerp(parameters_min[PARAM_LINEAR_ACCEL],
								   parameters_max[PARAM_LINEAR_ACCEL], rand_from_seed(_seed))
						 : Vector2();
			// apply radial acceleration
			Vector2 org = emission_xform[2];
			Vector2 diff = pos - org;
			force += diff.length() > 0.0f
						 ? diff.normalized() *
							   (tex_radial_accel)*Math::lerp(parameters_min[PARAM_RADIAL_ACCEL],
								   parameters_max[PARAM_RADIAL_ACCEL], rand_from_seed(_seed))
						 : Vector2();
			// apply tangential acceleration;
			Vector2 yx = Vector2(diff.y, diff.x);
			force +=
				yx.length() > 0.0f
					? (yx * Vector2(-1.0f, 1.0f)).normalized() *
						  (tex_tangential_accel * Math::lerp(parameters_min[PARAM_TANGENTIAL_ACCEL],
													  parameters_max[PARAM_TANGENTIAL_ACCEL],
													  rand_from_seed(_seed)))
					: Vector2();
			// apply attractor forces
			p.velocity += force * local_delta;
			// orbit velocity
			real_t orbit_amount = tex_orbit_velocity *
								  Math::lerp(parameters_min[PARAM_ORBIT_VELOCITY],
									  parameters_max[PARAM_ORBIT_VELOCITY], rand_from_seed(_seed));
			if (orbit_amount != 0.0) {
				real_t ang = orbit_amount * local_delta * Math::TAU;
				// Not sure why the ParticleProcessMaterial code uses a clockwise rotation matrix,
				// but we use -ang here to reproduce its behavior.
				Transform2D rot = Transform2D(-ang, Vector2());
				p.transform[2] -= diff;
				p.transform[2] += rot.basis_xform(diff);
			}
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				p.velocity = p.velocity.normalized() * tex_linear_velocity;
			}

			if (parameters_max[PARAM_DAMPING] + tex_damping > 0.0) {
				real_t v = p.velocity.length();
				real_t damp =
					tex_damping * Math::lerp(parameters_min[PARAM_DAMPING],
									  parameters_max[PARAM_DAMPING], rand_from_seed(_seed));
				v -= damp * local_delta;
				if (v < 0.0) {
					p.velocity = Vector2();
				}
				else {
					p.velocity = p.velocity.normalized() * v;
				}
			}
			real_t base_angle = (tex_angle)*Math::lerp(
				parameters_min[PARAM_ANGLE], parameters_max[PARAM_ANGLE], p.angle_rand);
			base_angle += p.custom[1] * lifetime * tex_angular_velocity *
						  Math::lerp(parameters_min[PARAM_ANGULAR_VELOCITY],
							  parameters_max[PARAM_ANGULAR_VELOCITY], rand_from_seed(_seed));
			p.rotation = Math::deg_to_rad(base_angle); // angle
			p.custom[2] =
				tex_anim_offset * Math::lerp(parameters_min[PARAM_ANIM_OFFSET],
									  parameters_max[PARAM_ANIM_OFFSET], p.anim_offset_rand) +
				tv * tex_anim_speed *
					Math::lerp(parameters_min[PARAM_ANIM_SPEED], parameters_max[PARAM_ANIM_SPEED],
						rand_from_seed(_seed));
		}
		// apply color
		// apply hue rotation

		Vector2 tex_scale = Vector2(1.0, 1.0);
		if (split_scale) {
			if (scale_curve_x.is_valid()) {
				tex_scale.x = scale_curve_x->sample(tv);
			}
			else {
				tex_scale.x = 1.0;
			}
			if (scale_curve_y.is_valid()) {
				tex_scale.y = scale_curve_y->sample(tv);
			}
			else {
				tex_scale.y = 1.0;
			}
		}
		else {
			if (curve_parameters[PARAM_SCALE].is_valid()) {
				real_t tmp_scale = curve_parameters[PARAM_SCALE]->sample(tv);
				tex_scale.x = tmp_scale;
				tex_scale.y = tmp_scale;
			}
		}

		real_t tex_hue_variation = 0.0;
		if (curve_parameters[PARAM_HUE_VARIATION].is_valid()) {
			tex_hue_variation = curve_parameters[PARAM_HUE_VARIATION]->sample(tv);
		}

		real_t hue_rot_angle = (tex_hue_variation)*Math::TAU *
							   Math::lerp(parameters_min[PARAM_HUE_VARIATION],
								   parameters_max[PARAM_HUE_VARIATION], p.hue_rot_rand);
		real_t hue_rot_c = Math::cos(hue_rot_angle);
		real_t hue_rot_s = Math::sin(hue_rot_angle);

		Basis hue_rot_mat;
		{
			Basis mat1(0.299, 0.587, 0.114, 0.299, 0.587, 0.114, 0.299, 0.587, 0.114);
			Basis mat2(0.701, -0.587, -0.114, -0.299, 0.413, -0.114, -0.300, -0.588, 0.886);
			Basis mat3(0.168, 0.330, -0.497, -0.328, 0.035, 0.292, 1.250, -1.050, -0.203);

			for (int j = 0; j < 3; j++) {
				hue_rot_mat[j] = mat1[j] + mat2[j] * hue_rot_c + mat3[j] * hue_rot_s;
			}
		}

		if (color_ramp.is_valid()) {
			p.color = color_ramp->get_color_at_offset(tv) * color;
		}
		else {
			p.color = color;
		}

		Vector3 color_rgb = hue_rot_mat.xform_inv(Vector3(p.color.r, p.color.g, p.color.b));
		p.color.r = color_rgb.x;
		p.color.g = color_rgb.y;
		p.color.b = color_rgb.z;

		p.color *= p.base_color * p.start_color_rand;

		if (particle_flags[PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY]) {
			if (p.velocity.length() > 0.0f) {
				p.transform.columns[1] = p.velocity;
			}

			p.transform.columns[1] = p.transform.columns[1].normalized();
			p.transform.columns[0] = p.transform.columns[1].orthogonal();
		}
		else {
			p.transform.columns[0] = Vector2(Math::cos(p.rotation), -Math::sin(p.rotation));
			p.transform.columns[1] = Vector2(Math::sin(p.rotation), Math::cos(p.rotation));
		}

		// scale by scale
		Vector2 base_scale = tex_scale * Math::lerp(parameters_min[PARAM_SCALE],
											 parameters_max[PARAM_SCALE], p.scale_rand);
		if (base_scale.x < 0.00001) {
			base_scale.x = 0.00001;
		}
		if (base_scale.y < 0.00001) {
			base_scale.y = 0.00001;
		}
		p.transform.columns[0] *= base_scale.x;
		p.transform.columns[1] *= base_scale.y;

		p.transform[2] += p.velocity * local_delta;

		should_be_active = true;
	}
	if (!Math::is_equal_approx(time, 0.0) && active && !should_be_active) {
		active = false;
	}
}

void CPUParticles2D::_update_particle_data_buffer()
{
	MutexLock lock(update_mutex);

	int pc = particles.size();

	int* ow;
	int* order = nullptr;

	float* w = particle_data.ptrw();
	const Particle* r = particles.ptr();
	float* ptr = w;

	if (draw_order != DRAW_ORDER_INDEX) {
		ow = particle_order.ptrw();
		order = ow;

		for (int i = 0; i < pc; i++) {
			order[i] = i;
		}
		if (draw_order == DRAW_ORDER_LIFETIME) {
			SortArray<int, SortLifetime> sorter;
			sorter.compare.particles = r;
			sorter.sort(order, pc);
		}
	}

	for (int i = 0; i < pc; i++) {
		int idx = order ? order[i] : i;

		Transform2D t = r[idx].transform;

		if (!local_coords) {
			t = inv_emission_transform * t;
		}

		if (r[idx].active) {
			ptr[0] = t.columns[0][0];
			ptr[1] = t.columns[1][0];
			ptr[2] = 0;
			ptr[3] = t.columns[2][0];
			ptr[4] = t.columns[0][1];
			ptr[5] = t.columns[1][1];
			ptr[6] = 0;
			ptr[7] = t.columns[2][1];

		}
		else {
			memset(ptr, 0, sizeof(float) * 8);
		}

		Color c = r[idx].color;

		ptr[8] = c.r;
		ptr[9] = c.g;
		ptr[10] = c.b;
		ptr[11] = c.a;

		ptr[12] = r[idx].custom[0];
		ptr[13] = r[idx].custom[1];
		ptr[14] = r[idx].custom[2];
		ptr[15] = r[idx].custom[3];

		ptr += 16;
	}
}

void CPUParticles2D::_update_render_thread()
{
	MutexLock lock(update_mutex);

	RS::get_singleton()->multimesh_set_buffer(multimesh, particle_data);
}

#ifdef TOOLS_ENABLED
void CPUParticles2D::_draw_emission_gizmo()
{
	Color emission_ring_color = Color(0.8, 0.7, 0.4, 0.4);
	Transform2D gizmo_transform;
	if (!local_coords) {
		gizmo_transform = get_global_transform();
	}

	draw_set_transform_matrix(gizmo_transform);

	switch (emission_shape) {
	case CPUParticles2D::EMISSION_SHAPE_RECTANGLE:
		draw_rect(
			Rect2(-emission_rect_extents, emission_rect_extents * 2.0), emission_ring_color, false);
		break;
	case CPUParticles2D::EMISSION_SHAPE_SPHERE:
	case CPUParticles2D::EMISSION_SHAPE_SPHERE_SURFACE:
		draw_circle(Vector2(), emission_sphere_radius, emission_ring_color, false);
		break;
	default:

	break;
	}
}
#endif

CPUParticles2D::CPUParticles2D()
{
	mesh = RenderingServer::get_singleton()->mesh_create();
	multimesh = RenderingServer::get_singleton()->multimesh_create();
	RenderingServer::get_singleton()->multimesh_set_mesh(multimesh, mesh);

	set_emitting(true);
	set_amount(8);
	set_use_local_coordinates(false);
	set_seed(Math::rand());

	rng.instantiate();

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

	for (int i = 0; i < PARTICLE_FLAG_MAX; i++) {
		particle_flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));

	_update_mesh_texture();

	// CPUParticles2D defaults to interpolation off.
	// This is because the result often looks better when the particles are updated every frame.
	// Note that children will need to explicitly turn back on interpolation if they want to use it,
	// rather than relying on inherit mode.
	set_physics_interpolation_mode(Node::PHYSICS_INTERPOLATION_MODE_OFF);
}

CPUParticles2D::~CPUParticles2D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(multimesh);
	RS::get_singleton()->free_rid(mesh);
}


