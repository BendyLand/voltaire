/**************************************************************************/
/*  cpu_particles_3d.cpp                                                  */
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
#include "servers/rendering/renderer.h"

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
	Renderer::multimesh_set_visible_instances(multimesh, -1);
	Renderer::multimesh_allocate_data(
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

real_t CPUParticles3D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
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
	Renderer::multimesh_set_custom_aabb(multimesh, AABB());
	return Renderer::multimesh_get_aabb(multimesh);
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
		Renderer::multimesh_set_buffer(multimesh, particle_data);
		can_update.clear(); // wait for next time
	}
}

void CPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void CPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

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
					ptr[9] = t.basis.rows[2][1];
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

CPUParticles3D::~CPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(multimesh);
}

void CPUParticles3D::_set_redraw(bool p_redraw) {}

void CPUParticles3D::_particles_process(double p_delta) {}

PackedStringArray CPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

// ==========================================================================
// Merged from scene/3d/gpu_particles_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/os/os.h"
#include "gpu_particles_3d.h"
#include "scene/3d/cpu_particles_3d.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/mesh.h"
#include "scene/resources/particle_process_material.h"
#include "servers/rendering/renderer.h"

AABB GPUParticles3D::get_aabb() const { return AABB(); }

void GPUParticles3D::set_emitting(bool p_emitting)
{
	// Do not return even if `p_emitting == emitting` because `emitting` is just an approximation.
	if (p_emitting && p_emitting != emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}
	if (p_emitting && one_shot) {
		if (!active && !emitting) {
			// Last cycle ended.
			active = true;
			time = 0;
			signal_canceled = false;
			emission_time = lifetime;
			active_time = lifetime * (2 - explosiveness_ratio);
		}
		else {
			signal_canceled = true;
		}
		set_process_internal(true);
	}
	else if (!p_emitting) {
		if (one_shot) {
			set_process_internal(true);
		}
		else {
			set_process_internal(false);
		}
	}
	else {
		set_process_internal(true);
	}

	emitting = p_emitting;
	Renderer::particles_set_emitting(particles, p_emitting);
}

void GPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	Renderer::particles_set_amount(particles, amount);
}

void GPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	Renderer::particles_set_lifetime(particles, lifetime);
}

void GPUParticles3D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	Renderer::particles_set_interp_to_end(particles, interp_to_end_factor);
}

void GPUParticles3D::set_one_shot(bool p_one_shot)
{
	one_shot = p_one_shot;
	Renderer::particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		if (!one_shot) {
			Renderer::particles_restart(particles);
		}
	}
}

bool GPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles3D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	Renderer::particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles3D::get_seed() const { return seed; }

void GPUParticles3D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	Renderer::particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles3D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	Renderer::particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles3D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	Renderer::particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles3D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	Renderer::particles_set_use_local_coordinates(particles, local_coords);
}

void GPUParticles3D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	Renderer::particles_set_speed_scale(particles, p_scale);
}

void GPUParticles3D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	Renderer::particles_set_collision_base_size(particles, p_size);
}

bool GPUParticles3D::is_emitting() const { return emitting; }

int GPUParticles3D::get_amount() const { return amount; }

double GPUParticles3D::get_lifetime() const { return lifetime; }

float GPUParticles3D::get_interp_to_end() const { return interp_to_end_factor; }

bool GPUParticles3D::get_one_shot() const { return one_shot; }

double GPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t GPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t GPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB GPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

bool GPUParticles3D::get_use_local_coordinates() const { return local_coords; }

Ref<Material> GPUParticles3D::get_process_material() const { return process_material; }

double GPUParticles3D::get_speed_scale() const { return speed_scale; }

real_t GPUParticles3D::get_collision_base_size() const { return collision_base_size; }

void GPUParticles3D::set_draw_order(DrawOrder p_order)
{
	draw_order = p_order;
	Renderer::particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

void GPUParticles3D::set_trail_lifetime(double p_seconds)
{
	ERR_FAIL_COND(p_seconds < 0.01 - CMP_EPSILON);
	trail_lifetime = p_seconds;
	Renderer::particles_set_trails(particles, trail_enabled, trail_lifetime);
}

bool GPUParticles3D::is_trail_enabled() const { return trail_enabled; }

double GPUParticles3D::get_trail_lifetime() const { return trail_lifetime; }

GPUParticles3D::DrawOrder GPUParticles3D::get_draw_order() const { return draw_order; }

int GPUParticles3D::get_draw_passes() const { return draw_passes.size(); }

Ref<Mesh> GPUParticles3D::get_draw_pass_mesh(int p_pass) const
{
	ERR_FAIL_INDEX_V(p_pass, draw_passes.size(), Ref<Mesh>());

	return draw_passes[p_pass];
}

void GPUParticles3D::set_fixed_fps(int p_count)
{
	fixed_fps = p_count;
	Renderer::particles_set_fixed_fps(particles, p_count);
}

int GPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles3D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	Renderer::particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles3D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	Renderer::particles_set_interpolate(particles, p_enable);
}

bool GPUParticles3D::get_interpolate() const { return interpolate; }

void GPUParticles3D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	Renderer::particles_restart(particles);
	Renderer::particles_set_emitting(particles, true);

	emitting = true;
	active = true;
	signal_canceled = false;
	time = 0;
	emission_time = lifetime * (1 - explosiveness_ratio);
	active_time = lifetime * (2 - explosiveness_ratio);
	set_process_internal(true);
}

AABB GPUParticles3D::capture_aabb() const
{
	return Renderer::particles_get_current_aabb(particles);
}

void GPUParticles3D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	Renderer::particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	// Setting emitting independently from set_emitting is important here
	// we assume to be in a controlled process situation.
	// setting RS emission to true is necessary to ensure particles will emit when
	// regular process time is > than zero but the particles are already in trailing mode.
	// this could also be done on the GScript/tool side.
	if (p_requested_process_time > 0.0) {
		emitting = true;
		Renderer::particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

void GPUParticles3D::emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Renderer::particles_emit(
		particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}

NodePath GPUParticles3D::get_sub_emitter() const { return sub_emitter; }

Ref<Skin> GPUParticles3D::get_skin() const { return skin; }

GPUParticles3D::TransformAlign GPUParticles3D::get_transform_align() const
{
	return transform_align;
}

void GPUParticles3D::set_transform_align_channel_filter(
	RSE::ParticlesTransformAlignCustomSrc p_align_channel_filter)
{
	ERR_FAIL_INDEX(uint32_t(p_align_channel_filter),
		uint32_t(RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_MAX));
	transform_align_channel_filter = p_align_channel_filter;
	Renderer::particles_set_transform_align_channel_filter(
		particles, transform_align_channel_filter);
}

RSE::ParticlesTransformAlignCustomSrc GPUParticles3D::get_transform_align_channel_filter() const
{
	return transform_align_channel_filter;
}

void GPUParticles3D::set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis)
{
	transform_align_axis = p_axis;
	Renderer::particles_set_transform_align_axis(particles, p_axis);
}

RSE::ParticlesTransformAlignAxis GPUParticles3D::get_transform_align_axis() const
{
	return transform_align_axis;
}

void GPUParticles3D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	Renderer::particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles3D::get_amount_ratio() const { return amount_ratio; }

GPUParticles3D::GPUParticles3D()
{
	particles = Renderer::particles_create();
	Renderer::particles_set_mode(particles, RSE::PARTICLES_MODE_3D);
	set_base(particles);
	one_shot = false; // Needed so that set_emitting doesn't access uninitialized values
	set_emitting(true);
	set_one_shot(false);
	set_seed(Math::rand());
	set_amount_ratio(1.0);
	set_amount(8);
	set_lifetime(1);
	set_fixed_fps(30);
	set_fractional_delta(true);
	set_interpolate(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_trail_lifetime(0.3);
	set_use_local_coordinates(false);
	set_draw_passes(1);
	set_draw_order(DRAW_ORDER_INDEX);
	set_speed_scale(1);
	set_collision_base_size(collision_base_size);
	set_transform_align(TRANSFORM_ALIGN_DISABLED);
	set_transform_align_channel_filter(
		RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_X);
	set_transform_align_axis(RSE::ParticlesTransformAlignAxis::PARTICLES_ALIGN_AXIS_Y);
	set_use_fixed_seed(false);
}

void GPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticles3D::~GPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(particles);
}

PackedStringArray GPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

void GPUParticles3D::set_use_fixed_seed(bool) {}

void GPUParticles3D::set_transform_align(GPUParticles3D::TransformAlign) {}

void GPUParticles3D::set_draw_passes(int) {}

// ==========================================================================
// Merged from scene/3d/gpu_particles_collision_3d.cpp
// ==========================================================================

#include "gpu_particles_collision_3d.h"

#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"

GPUParticlesCollision3D::BakeBeginFunc GPUParticlesCollision3D::bake_begin_function = nullptr;
GPUParticlesCollision3D::BakeStepFunc GPUParticlesCollision3D::bake_step_function = nullptr;
GPUParticlesCollision3D::BakeEndFunc GPUParticlesCollision3D::bake_end_function = nullptr;

void GPUParticlesCollision3D::_apply_shape_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
	switch (shape_type) {
	case SHAPE_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE;
		break;
	case SHAPE_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
		break;
	case SHAPE_SDF:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE;
		break;
	case SHAPE_HEIGHT_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesCollision3D::set_shape_type(ShapeType p_type)
{
	shape_type = p_type;
	_apply_shape_type();
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesCollision3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesCollision3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesCollision3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesCollision3D::set_thickness(float p_thickness) { thickness = p_thickness; }

bool GPUParticlesCollision3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 20, false,
		vformat(
			"The render layer number (%d) must be between 1 and 20 (inclusive).", p_layer_number));
	return bake_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

Vector3i GPUParticlesCollision3D::get_estimated_cell_size() const
{
	static const int subdivs[SDF_RESOLUTION_MAX] = {16, 32, 64, 128, 256, 512};
	int subdiv = subdivs[sdf_resolution];
	AABB aabb(-size / 2, size);
	float cell_size = aabb.get_longest_axis_size() / float(subdiv);
	Vector3i sdf_size = Vector3i(aabb.size / cell_size);
	return sdf_size.maxi(1);
}

void GPUParticlesCollision3D::set_update_mode(UpdateMode p_update_mode)
{
	update_mode = p_update_mode;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_heightfield_mask(uint32_t p_heightfield_mask)
{
	heightfield_mask = p_heightfield_mask;
	Renderer::particles_collision_set_height_field_mask(collision, p_heightfield_mask);
}

void GPUParticlesCollision3D::set_heightfield_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_heightfield_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_heightfield_mask(mask);
}

bool GPUParticlesCollision3D::get_heightfield_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return heightfield_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_follow_camera_enabled(bool p_enabled)
{
	follow_camera_mode = p_enabled;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

AABB GPUParticlesCollision3D::get_aabb() const
{
	if (shape_type == SHAPE_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

PackedStringArray GPUParticlesCollision3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (shape_type == SHAPE_SDF && bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any collision for this GPUParticlesCollision3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	return warnings;
}

void GPUParticlesCollision3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void GPUParticlesCollision3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	if (shape_type != SHAPE_HEIGHT_FIELD) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (update_mode == UPDATE_MODE_ALWAYS) {
			Renderer::particles_collision_height_field_update(collision);
		}

		if (follow_camera_mode && get_viewport()) {
			Camera3D* cam = get_viewport()->get_camera_3d();
			if (cam) {
				Transform3D xform = get_global_transform();
				Vector3 x_axis = xform.basis.get_column(Vector3::AXIS_X).normalized();
				Vector3 z_axis = xform.basis.get_column(Vector3::AXIS_Z).normalized();
				float x_len = xform.basis.get_scale().x;
				float z_len = xform.basis.get_scale().z;

				Vector3 cam_pos = cam->get_global_transform().origin;
				Transform3D new_xform = xform;

				while (x_axis.dot(cam_pos - new_xform.origin) > x_len) {
					new_xform.origin += x_axis * x_len;
				}
				while (x_axis.dot(cam_pos - new_xform.origin) < -x_len) {
					new_xform.origin -= x_axis * x_len;
				}

				while (z_axis.dot(cam_pos - new_xform.origin) > z_len) {
					new_xform.origin += z_axis * z_len;
				}
				while (z_axis.dot(cam_pos - new_xform.origin) < -z_len) {
					new_xform.origin -= z_axis * z_len;
				}

				if (new_xform != xform) {
					set_global_transform(new_xform);
					Renderer::particles_collision_height_field_update(collision);
				}
			}
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Renderer::particles_collision_height_field_update(collision);
	} break;
	}
}

GPUParticlesCollision3D::GPUParticlesCollision3D(ShapeType p_type) : shape_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_shape_type();
		set_base(collision);
	}
}

GPUParticlesCollision3D::~GPUParticlesCollision3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}

void GPUParticlesAttractor3D::_apply_attractor_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
	switch (attractor_type) {
	case ATTRACTOR_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		break;
	case ATTRACTOR_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT;
		break;
	case ATTRACTOR_VECTOR_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesAttractor3D::set_attractor_type(AttractorType p_type)
{
	attractor_type = p_type;
	_apply_attractor_type();
}

void GPUParticlesAttractor3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesAttractor3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesAttractor3D::set_strength(real_t p_strength)
{
	strength = p_strength;
	Renderer::particles_collision_set_attractor_strength(collision, p_strength);
}

real_t GPUParticlesAttractor3D::get_strength() const { return strength; }

void GPUParticlesAttractor3D::set_attenuation(real_t p_attenuation)
{
	attenuation = p_attenuation;
	Renderer::particles_collision_set_attractor_attenuation(collision, p_attenuation);
}

real_t GPUParticlesAttractor3D::get_attenuation() const { return attenuation; }

real_t GPUParticlesAttractor3D::get_directionality() const { return directionality; }

void GPUParticlesAttractor3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesAttractor3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesAttractor3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

AABB GPUParticlesAttractor3D::get_aabb() const
{
	if (attractor_type == ATTRACTOR_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

void GPUParticlesAttractor3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticlesAttractor3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticlesAttractor3D::GPUParticlesAttractor3D(AttractorType p_type) : attractor_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_attractor_type();
		set_base(collision);
	}
}

GPUParticlesAttractor3D::~GPUParticlesAttractor3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}
