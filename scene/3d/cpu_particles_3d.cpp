/**************************************************************************/
/*  cpu_particles_3d.cpp                                                  */
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
#include "cpu_particles_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/mesh.h"
#include "scene/resources/particle_process_material.h"
#include "servers/rendering/rendering_server.h"

AABB CPUParticles3D::get_aabb() const { return AABB(); }

void CPUParticles3D::set_emitting(bool p_emitting)
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

void CPUParticles3D::_set_emitting()
{
	active = true;
	set_process_internal(true);
	// first update before rendering to avoid one frame delay after emitting starts
	if (time == 0) {
		_update_internal();
	}
}

void CPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles must be greater than 0.");

	particles.resize(p_amount);
	{
		Particle* w = particles.ptrw();

		for (int i = 0; i < p_amount; i++) {
			w[i].active = false;
			w[i].custom[3] = 1.0; // Make sure w component isn't garbage data and doesn't break
								  // shaders with CUSTOM.y/Custom.w
		}
	}

	particle_data.resize((12 + 4 + 4) * p_amount);
	RS::get_singleton()->multimesh_set_visible_instances(multimesh, -1);
	RS::get_singleton()->multimesh_allocate_data(
		multimesh, p_amount, RSE::MULTIMESH_TRANSFORM_3D, true, true);

	particle_order.resize(p_amount);
}

void CPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
}

void CPUParticles3D::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

void CPUParticles3D::set_pre_process_time(double p_time) { pre_process_time = p_time; }

void CPUParticles3D::set_explosiveness_ratio(real_t p_ratio) { explosiveness_ratio = p_ratio; }

void CPUParticles3D::set_randomness_ratio(real_t p_ratio) { randomness_ratio = p_ratio; }

void CPUParticles3D::set_visibility_aabb(const AABB& p_aabb)
{
	RS::get_singleton()->multimesh_set_custom_aabb(multimesh, p_aabb);
	visibility_aabb = p_aabb;
	update_gizmos();
}

void CPUParticles3D::set_lifetime_randomness(double p_random) { lifetime_randomness = p_random; }

void CPUParticles3D::set_use_local_coordinates(bool p_enable) { local_coords = p_enable; }

void CPUParticles3D::set_speed_scale(double p_scale) { speed_scale = p_scale; }

bool CPUParticles3D::is_emitting() const { return emitting; }

int CPUParticles3D::get_amount() const { return particles.size(); }

double CPUParticles3D::get_lifetime() const { return lifetime; }

bool CPUParticles3D::get_one_shot() const { return one_shot; }

double CPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t CPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t CPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB CPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

double CPUParticles3D::get_lifetime_randomness() const { return lifetime_randomness; }

bool CPUParticles3D::get_use_local_coordinates() const { return local_coords; }

double CPUParticles3D::get_speed_scale() const { return speed_scale; }

void CPUParticles3D::set_draw_order(DrawOrder p_order)
{
	ERR_FAIL_INDEX(p_order, DRAW_ORDER_MAX);
	draw_order = p_order;
}

CPUParticles3D::DrawOrder CPUParticles3D::get_draw_order() const { return draw_order; }

void CPUParticles3D::set_mesh(const Ref<Mesh>& p_mesh)
{
	mesh = p_mesh;
	if (mesh.is_valid()) {
		RS::get_singleton()->multimesh_set_mesh(multimesh, mesh->get_rid());
	}
	else {
		RS::get_singleton()->multimesh_set_mesh(multimesh, RID());
	}

	update_configuration_warnings();
}

Ref<Mesh> CPUParticles3D::get_mesh() const { return mesh; }

void CPUParticles3D::set_fixed_fps(int p_count) { fixed_fps = p_count; }

int CPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void CPUParticles3D::set_fractional_delta(bool p_enable) { fractional_delta = p_enable; }

bool CPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void CPUParticles3D::restart(bool p_keep_seed)
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

void CPUParticles3D::set_direction(Vector3 p_direction) { direction = p_direction; }

Vector3 CPUParticles3D::get_direction() const { return direction; }

void CPUParticles3D::set_spread(real_t p_spread) { spread = p_spread; }

real_t CPUParticles3D::get_spread() const { return spread; }

void CPUParticles3D::set_flatness(real_t p_flatness) { flatness = p_flatness; }

real_t CPUParticles3D::get_flatness() const { return flatness; }

void CPUParticles3D::set_param_min(Parameter p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	parameters_min[p_param] = p_value;
	if (parameters_min[p_param] > parameters_max[p_param]) {
		set_param_max(p_param, p_value);
	}

	update_configuration_warnings();
}

real_t CPUParticles3D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
}

void CPUParticles3D::set_param_max(Parameter p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	parameters_max[p_param] = p_value;
	if (parameters_min[p_param] > parameters_max[p_param]) {
		set_param_min(p_param, p_value);
	}

	update_configuration_warnings();
}

real_t CPUParticles3D::get_param_max(Parameter p_param) const
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

void CPUParticles3D::set_param_curve(Parameter p_param, const Ref<Curve>& p_curve)
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

Ref<Curve> CPUParticles3D::get_param_curve(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Curve>());

	return curve_parameters[p_param];
}

void CPUParticles3D::set_color(const Color& p_color) { color = p_color; }

Color CPUParticles3D::get_color() const { return color; }

void CPUParticles3D::set_color_ramp(const Ref<Gradient>& p_ramp) { color_ramp = p_ramp; }

Ref<Gradient> CPUParticles3D::get_color_ramp() const { return color_ramp; }

void CPUParticles3D::set_color_initial_ramp(const Ref<Gradient>& p_ramp)
{
	color_initial_ramp = p_ramp;
}

Ref<Gradient> CPUParticles3D::get_color_initial_ramp() const { return color_initial_ramp; }

bool CPUParticles3D::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

void CPUParticles3D::set_emission_shape(EmissionShape p_shape)
{
	ERR_FAIL_INDEX(p_shape, EMISSION_SHAPE_MAX);
	emission_shape = p_shape;
	update_gizmos();
}

void CPUParticles3D::set_emission_sphere_radius(real_t p_radius)
{
	emission_sphere_radius = p_radius;
	update_gizmos();
}

void CPUParticles3D::set_emission_box_extents(Vector3 p_extents)
{
	emission_box_extents = p_extents;
	update_gizmos();
}

void CPUParticles3D::set_emission_points(const Vector<Vector3>& p_points)
{
	emission_points = p_points;
}

void CPUParticles3D::set_emission_normals(const Vector<Vector3>& p_normals)
{
	emission_normals = p_normals;
}

void CPUParticles3D::set_emission_colors(const Vector<Color>& p_colors)
{
	emission_colors = p_colors;
}

void CPUParticles3D::set_emission_ring_axis(Vector3 p_axis)
{
	emission_ring_axis = p_axis;
	update_gizmos();
}

void CPUParticles3D::set_emission_ring_height(real_t p_height)
{
	emission_ring_height = p_height;
	update_gizmos();
}

void CPUParticles3D::set_emission_ring_radius(real_t p_radius)
{
	emission_ring_radius = p_radius;
	update_gizmos();
}

void CPUParticles3D::set_emission_ring_inner_radius(real_t p_radius)
{
	emission_ring_inner_radius = p_radius;
	update_gizmos();
}

void CPUParticles3D::set_emission_ring_cone_angle(real_t p_angle)
{
	emission_ring_cone_angle = p_angle;
	update_gizmos();
}

void CPUParticles3D::set_scale_curve_x(Ref<Curve> p_scale_curve) { scale_curve_x = p_scale_curve; }

void CPUParticles3D::set_scale_curve_y(Ref<Curve> p_scale_curve) { scale_curve_y = p_scale_curve; }

void CPUParticles3D::set_scale_curve_z(Ref<Curve> p_scale_curve) { scale_curve_z = p_scale_curve; }

real_t CPUParticles3D::get_emission_sphere_radius() const { return emission_sphere_radius; }

Vector3 CPUParticles3D::get_emission_box_extents() const { return emission_box_extents; }

Vector<Vector3> CPUParticles3D::get_emission_points() const { return emission_points; }

Vector<Vector3> CPUParticles3D::get_emission_normals() const { return emission_normals; }

Vector<Color> CPUParticles3D::get_emission_colors() const { return emission_colors; }

Vector3 CPUParticles3D::get_emission_ring_axis() const { return emission_ring_axis; }

real_t CPUParticles3D::get_emission_ring_height() const { return emission_ring_height; }

real_t CPUParticles3D::get_emission_ring_radius() const { return emission_ring_radius; }

real_t CPUParticles3D::get_emission_ring_inner_radius() const { return emission_ring_inner_radius; }

real_t CPUParticles3D::get_emission_ring_cone_angle() const { return emission_ring_cone_angle; }

CPUParticles3D::EmissionShape CPUParticles3D::get_emission_shape() const { return emission_shape; }

void CPUParticles3D::set_gravity(const Vector3& p_gravity) { gravity = p_gravity; }

Vector3 CPUParticles3D::get_gravity() const { return gravity; }

Ref<Curve> CPUParticles3D::get_scale_curve_x() const { return scale_curve_x; }

Ref<Curve> CPUParticles3D::get_scale_curve_y() const { return scale_curve_y; }

Ref<Curve> CPUParticles3D::get_scale_curve_z() const { return scale_curve_z; }

bool CPUParticles3D::get_split_scale() { return split_scale; }

AABB CPUParticles3D::capture_aabb() const
{
	RS::get_singleton()->multimesh_set_custom_aabb(multimesh, AABB());
	return RS::get_singleton()->multimesh_get_aabb(multimesh);
}

bool CPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void CPUParticles3D::set_seed(uint32_t p_seed) { seed = p_seed; }

uint32_t CPUParticles3D::get_seed() const { return seed; }

void CPUParticles3D::request_particles_process(
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

void CPUParticles3D::_update_internal()
{
	if (particles.is_empty() || !is_visible_in_tree()) {
		_set_redraw(false);
		return;
	}

	double delta = get_process_delta_time();
	if (!active && !emitting) {
		set_process_internal(false);
		_set_redraw(false);

		// reset variables
		time = 0;
		frame_remainder = 0;
		cycle = 0;
		return;
	}
	_set_redraw(true);

	bool processed = false;

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
			// We need this otherwise the speed scale of the particle system influences the TODO.
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
			processed = true;
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
			processed = true;
			todo -= decr;
		}

		frame_remainder = todo;

	}
	else {
		_particles_process(delta);
		processed = true;
	}

	if (processed) {
		_update_particle_data_buffer();
	}
}

void CPUParticles3D::_update_particle_data_buffer()
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
		else if (draw_order == DRAW_ORDER_VIEW_DEPTH) {
			ERR_FAIL_NULL(get_viewport());
			Camera3D* c = get_viewport()->get_camera_3d();
			if (c) {
				Vector3 dir = c->get_global_transform().basis.get_column(2); // far away to close

				if (local_coords) {
					// will look different from Particles in editor as this is based on the camera
					// in the scenetree and not the editor camera
					dir = inv_emission_transform.xform(dir).normalized();
				}
				else {
					dir = dir.normalized();
				}

				SortArray<int, SortAxis> sorter;
				sorter.compare.particles = r;
				sorter.compare.axis = dir;
				sorter.sort(order, pc);
			}
		}
	}

	for (int i = 0; i < pc; i++) {
		int idx = order ? order[i] : i;

		Transform3D t = r[idx].transform;

		if (!local_coords) {
			t = inv_emission_transform * t;
		}

		if (r[idx].active) {
			ptr[0] = t.basis.rows[0][0];
			ptr[1] = t.basis.rows[0][1];
			ptr[2] = t.basis.rows[0][2];
			ptr[3] = t.origin.x;
			ptr[4] = t.basis.rows[1][0];
			ptr[5] = t.basis.rows[1][1];
			ptr[6] = t.basis.rows[1][2];
			ptr[7] = t.origin.y;
			ptr[8] = t.basis.rows[2][0];
			ptr[9] = t.basis.rows[2][1];
			ptr[10] = t.basis.rows[2][2];
			ptr[11] = t.origin.z;
		}
		else {
			memset(ptr, 0, sizeof(float) * 12);
		}

		Color c = r[idx].color;

		ptr[12] = c.r;
		ptr[13] = c.g;
		ptr[14] = c.b;
		ptr[15] = c.a;

		ptr[16] = r[idx].custom[0];
		ptr[17] = r[idx].custom[1];
		ptr[18] = r[idx].custom[2];
		ptr[19] = r[idx].custom[3];

		ptr += 20;
	}

	can_update.set();
}

void CPUParticles3D::_update_render_thread()
{
	MutexLock lock(update_mutex);

	if (can_update.is_set()) {
		RS::get_singleton()->multimesh_set_buffer(multimesh, particle_data);
		can_update.clear(); // wait for next time
	}
}

void CPUParticles3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		set_process_internal(emitting);

		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_set_redraw(false);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_internal();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Transform3D global_transform = get_global_transform();
		if (unlikely(global_transform.basis.determinant() == 0)) {
			return;
		}
		inv_emission_transform = global_transform.affine_inverse();

		if (!local_coords) {
			int pc = particles.size();

			float* w = particle_data.ptrw();
			const Particle* r = particles.ptr();
			float* ptr = w;

			for (int i = 0; i < pc; i++) {
				Transform3D t = inv_emission_transform * r[i].transform;

				if (r[i].active) {
					ptr[0] = t.basis.rows[0][0];
					ptr[1] = t.basis.rows[0][1];
					ptr[2] = t.basis.rows[0][2];
					ptr[3] = t.origin.x;
					ptr[4] = t.basis.rows[1][0];
					ptr[5] = t.basis.rows[1][1];
					ptr[6] = t.basis.rows[1][2];
					ptr[7] = t.origin.y;
					ptr[8] = t.basis.rows[2][0];
					ptr[9]
 = t.basis.rows[2][1];
					ptr[10] = t.basis.rows[2][2];
					ptr[11] = t.origin.z;
				}
				else {
					memset(ptr, 0, sizeof(float) * 12);
				}

				ptr += 20;
			}

			can_update.set();
		}
	} break;
	}
}

CPUParticles3D::CPUParticles3D()
{
	set_notify_transform(true);

	multimesh = RenderingServer::get_singleton()->multimesh_create();
	RenderingServer::get_singleton()->multimesh_set_visible_instances(multimesh, 0);
	set_base(multimesh);

	set_emitting(true);
	set_amount(8);
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
	set_emission_shape(EMISSION_SHAPE_POINT);
	set_emission_sphere_radius(1);
	set_emission_box_extents(Vector3(1, 1, 1));
	set_emission_ring_axis(Vector3(0, 0, 1.0));
	set_emission_ring_height(1);
	set_emission_ring_radius(1);
	set_emission_ring_inner_radius(0);
	set_emission_ring_cone_angle(90);

	set_gravity(Vector3(0, -9.8, 0));

	for (int i = 0; i < PARTICLE_FLAG_MAX; i++) {
		particle_flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));
}

CPUParticles3D::~CPUParticles3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(multimesh);
}


