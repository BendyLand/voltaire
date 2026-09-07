/**************************************************************************/
/*  gpu_particles_2d.cpp                                                  */
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
#include "core/os/os.h"
#include "gpu_particles_2d.compat.inc"
#include "gpu_particles_2d.h"
#include "scene/2d/cpu_particles_2d.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/particle_process_material.h"
#include "servers/rendering/rendering_server.h"

void GPUParticles2D::set_emitting(bool p_emitting)
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

	emitting = p_emitting;
	RS::get_singleton()->particles_set_emitting(particles, p_emitting);
}

void GPUParticles2D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	RS::get_singleton()->particles_set_amount(particles, amount);
}

void GPUParticles2D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	RS::get_singleton()->particles_set_lifetime(particles, lifetime);
}

void GPUParticles2D::set_one_shot(bool p_enable)
{
	one_shot = p_enable;
	RS::get_singleton()->particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		set_process_internal(true);
		if (!one_shot) {
			RenderingServer::get_singleton()->particles_restart(particles);
		}
	}

	if (!one_shot) {
		set_process_internal(false);
	}
}

void GPUParticles2D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	RS::get_singleton()->particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles2D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	RS::get_singleton()->particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles2D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	RS::get_singleton()->particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles2D::set_visibility_rect(const Rect2& p_visibility_rect)
{
	visibility_rect = p_visibility_rect;
	AABB aabb;
	aabb.position.x = p_visibility_rect.position.x;
	aabb.position.y = p_visibility_rect.position.y;
	aabb.size.x = p_visibility_rect.size.x;
	aabb.size.y = p_visibility_rect.size.y;

	RS::get_singleton()->particles_set_custom_aabb(particles, aabb);

	queue_redraw();
}

void GPUParticles2D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	RS::get_singleton()->particles_set_use_local_coordinates(particles, local_coords);
	set_notify_transform(!p_enable);
	if (!p_enable && is_inside_tree()) {
		_update_particle_emission_transform();
	}
}

void GPUParticles2D::_update_particle_emission_transform()
{
	Transform2D xf2d = get_global_transform();
	Transform3D xf;
	xf.basis.set_column(0, Vector3(xf2d.columns[0].x, xf2d.columns[0].y, 0));
	xf.basis.set_column(1, Vector3(xf2d.columns[1].x, xf2d.columns[1].y, 0));
	xf.set_origin(Vector3(xf2d.get_origin().x, xf2d.get_origin().y, 0));

	RS::get_singleton()->particles_set_emission_transform(particles, xf);
}

void GPUParticles2D::set_trail_enabled(bool p_enabled)
{
	trail_enabled = p_enabled;
	RS::get_singleton()->particles_set_trails(particles, trail_enabled, trail_lifetime);
	queue_redraw();
	update_configuration_warnings();

	RS::get_singleton()->particles_set_transform_align(
		particles, p_enabled ? RSE::PARTICLES_TRANSFORM_ALIGN_Y_TO_VELOCITY
							 : RSE::PARTICLES_TRANSFORM_ALIGN_DISABLED);
}

void GPUParticles2D::set_trail_lifetime(double p_seconds)
{
	ERR_FAIL_COND(p_seconds < 0.01 - CMP_EPSILON);
	trail_lifetime = p_seconds;
	RS::get_singleton()->particles_set_trails(particles, trail_enabled, trail_lifetime);
	queue_redraw();
}

void GPUParticles2D::set_trail_sections(int p_sections)
{
	ERR_FAIL_COND(p_sections < 2);
	ERR_FAIL_COND(p_sections > 128);

	trail_sections = p_sections;
	queue_redraw();
}

void GPUParticles2D::set_trail_section_subdivisions(int p_subdivisions)
{
	ERR_FAIL_COND(p_subdivisions < 1);
	ERR_FAIL_COND(p_subdivisions > 1024);

	trail_section_subdivisions = p_subdivisions;
	queue_redraw();
}

void GPUParticles2D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	RS::get_singleton()->particles_set_interp_to_end(particles, interp_to_end_factor);
}

#ifdef TOOLS_ENABLED
void GPUParticles2D::set_show_gizmos(bool p_show_gizmos)
{
	if (show_gizmos == p_show_gizmos) {
		return;
	}
	show_gizmos = p_show_gizmos;
	queue_redraw();
}
#endif

bool GPUParticles2D::is_trail_enabled() const { return trail_enabled; }

double GPUParticles2D::get_trail_lifetime() const { return trail_lifetime; }

void GPUParticles2D::_update_collision_size()
{
	real_t csize = collision_base_size;

	if (texture.is_valid()) {
		csize *=
			(texture->get_width() + texture->get_height()) / 4.0; // half size since its a radius
	}

	RS::get_singleton()->particles_set_collision_base_size(particles, csize);
}

void GPUParticles2D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	_update_collision_size();
}

real_t GPUParticles2D::get_collision_base_size() const { return collision_base_size; }

void GPUParticles2D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	RS::get_singleton()->particles_set_speed_scale(particles, p_scale);
}

bool GPUParticles2D::is_emitting() const { return emitting; }

int GPUParticles2D::get_amount() const { return amount; }

double GPUParticles2D::get_lifetime() const { return lifetime; }

int GPUParticles2D::get_trail_sections() const { return trail_sections; }

int GPUParticles2D::get_trail_section_subdivisions() const { return trail_section_subdivisions; }

bool GPUParticles2D::get_one_shot() const { return one_shot; }

double GPUParticles2D::get_pre_process_time() const { return pre_process_time; }

real_t GPUParticles2D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t GPUParticles2D::get_randomness_ratio() const { return randomness_ratio; }

Rect2 GPUParticles2D::get_visibility_rect() const { return visibility_rect; }

bool GPUParticles2D::get_use_local_coordinates() const { return local_coords; }

Ref<Material> GPUParticles2D::get_process_material() const { return process_material; }

double GPUParticles2D::get_speed_scale() const { return speed_scale; }

void GPUParticles2D::set_draw_order(DrawOrder p_order)
{
	draw_order = p_order;
	RS::get_singleton()->particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

GPUParticles2D::DrawOrder GPUParticles2D::get_draw_order() const { return draw_order; }

void GPUParticles2D::set_fixed_fps(int p_count)
{
	fixed_fps = p_count;
	RS::get_singleton()->particles_set_fixed_fps(particles, p_count);
}

int GPUParticles2D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles2D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	RS::get_singleton()->particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles2D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles2D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	RS::get_singleton()->particles_set_interpolate(particles, p_enable);
}

bool GPUParticles2D::get_interpolate() const { return interpolate; }

float GPUParticles2D::get_interp_to_end() const { return interp_to_end_factor; }

void GPUParticles2D::set_use_fixed_seed(bool p_use_fixed_seed)
{
	if (p_use_fixed_seed == use_fixed_seed) {
		return;
	}
	use_fixed_seed = p_use_fixed_seed;
}

bool GPUParticles2D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles2D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	RS::get_singleton()->particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles2D::get_seed() const { return seed; }

void GPUParticles2D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	RS::get_singleton()->particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	if (p_requested_process_time > 0.0) {
		emitting = true;
		RS::get_singleton()->particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

Rect2 GPUParticles2D::capture_rect() const
{
	AABB aabb = RS::get_singleton()->particles_get_current_aabb(particles);
	Rect2 r;
	r.position.x = aabb.position.x;
	r.position.y = aabb.position.y;
	r.size.x = aabb.size.x;
	r.size.y = aabb.size.y;
	return r;
}

Ref<Texture2D> GPUParticles2D::get_texture() const { return texture; }

void GPUParticles2D::emit_particle(const Transform2D& p_transform2d, const Vector2& p_velocity2d,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Transform3D emit_transform;
	emit_transform.basis.set_column(
		0, Vector3(p_transform2d.columns[0].x, p_transform2d.columns[0].y, 0));
	emit_transform.basis.set_column(
		1, Vector3(p_transform2d.columns[1].x, p_transform2d.columns[1].y, 0));
	emit_transform.set_origin(
		Vector3(p_transform2d.get_origin().x, p_transform2d.get_origin().y, 0));
	Vector3 velocity = Vector3(p_velocity2d.x, p_velocity2d.y, 0);

	RS::get_singleton()->particles_emit(
		particles, emit_transform, velocity, p_color, p_custom, p_emit_flags);
}

void GPUParticles2D::_texture_changed()
{
	// Changes to the texture need to trigger an update to make
	// the editor redraw the sprite with the updated texture.
	if (texture.is_valid()) {
		queue_redraw();
	}
}

void GPUParticles2D::set_sub_emitter(const NodePath& p_path)
{
	if (is_inside_tree()) {
		RS::get_singleton()->particles_set_subemitter(particles, RID());
	}

	sub_emitter = p_path;

	if (is_inside_tree() && sub_emitter != NodePath()) {
		_attach_sub_emitter();
	}
	update_configuration_warnings();
}

NodePath GPUParticles2D::get_sub_emitter() const { return sub_emitter; }

void GPUParticles2D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	RenderingServer::get_singleton()->particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles2D::get_amount_ratio() const { return amount_ratio; }

void GPUParticles2D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	RS::get_singleton()->particles_restart(particles);
	RS::get_singleton()->particles_set_emitting(particles, true);

	emitting = true;
	active = true;
	signal_canceled = false;
	time = 0;
	emission_time = lifetime;
	active_time = lifetime * (2 - explosiveness_ratio);
	if (one_shot) {
		set_process_internal(true);
	}
}

#ifdef TOOLS_ENABLED
void GPUParticles2D::_draw_emission_gizmo()
{
	Ref<ParticleProcessMaterial> pm = process_material;
	Color emission_ring_color = Color(0.8, 0.7, 0.4, 0.4);
	if (pm.is_null()) {
		return;
	}
	draw_set_transform(
		Vector2(pm->get_emission_shape_offset().x, pm->get_emission_shape_offset().y), 0.0,
		Vector2(pm->get_emission_shape_scale().x, pm->get_emission_shape_scale().y));

	switch (pm->get_emission_shape()) {
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_BOX: {
		Vector2 extents2d =
			Vector2(pm->get_emission_box_extents().x, pm->get_emission_box_extents().y);
		draw_rect(Rect2(-extents2d, extents2d * 2.0), emission_ring_color, false);
		break;
	}
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_SPHERE:
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_SPHERE_SURFACE: {
		draw_circle(Vector2(), pm->get_emission_sphere_radius(), emission_ring_color, false);
		break;
	}
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_RING: {
		Vector3 ring_axis = pm->get_emission_ring_axis();
		if (ring_axis.is_equal_approx(Vector3(0.0, 0.0, 1.0)) || ring_axis.is_zero_approx()) {
			draw_circle(
				Vector2(), pm->get_emission_ring_inner_radius(), emission_ring_color, false);
			draw_circle(Vector2(), pm->get_emission_ring_radius(), emission_ring_color, false);
		}
		else {
			Vector2 a = Vector2(
				pm->get_emission_ring_height() / -2.0, pm->get_emission_ring_radius() / -1.0);
			Vector2 b = Vector2(
				-a.x, MIN(a.y + std::tan((90.0 - pm->get_emission_ring_cone_angle()) * 0.01745329) *
									pm->get_emission_ring_height(),
						  0.0));
			Vector2 c = Vector2(b.x, -b.y);
			Vector2 d = Vector2(a.x, -a.y);
			if (ring_axis.is_equal_approx(Vector3(1.0, 0.0, 0.0))) {
				Vector<Vector2> pos = {a, b, b, c, c, d, d, a};
				draw_multiline(pos, emission_ring_color);
			}
			else if (ring_axis.is_equal_approx(Vector3(0.0, 1.0, 0.0))) {
				a = Vector2(a.y, a.x);
				b = Vector2(b.y, b.x);
				c = Vector2(c.y, c.x);
				d = Vector2(d.y, d.x);
				Vector<Vector2> pos = {a, b, b, c, c, d, d, a};
				draw_multiline(pos, emission_ring_color);
			}
		}
		break;
	}
	default: {
		break;
	}
	}
}
#endif

void GPUParticles2D::_bind_methods() {}

GPUParticles2D::GPUParticles2D()
{
	particles = RS::get_singleton()->particles_create();
	RS::get_singleton()->particles_set_mode(particles, RSE::PARTICLES_MODE_2D);

	mesh = RS::get_singleton()->mesh_create();
	RS::get_singleton()->particles_set_draw_passes(particles, 1);
	RS::get_singleton()->particles_set_draw_pass_mesh(particles, 0, mesh);

	one_shot = false; // Needed so that set_emitting doesn't access uninitialized values
	set_emitting(true);
	set_one_shot(false);
	set_seed(Math::rand());
	set_use_fixed_seed(false);
	set_amount(8);
	set_amount_ratio(1.0);
	set_lifetime(1);
	set_fixed_fps(0);
	set_fractional_delta(true);
	set_interpolate(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_visibility_rect(Rect2(Vector2(-100, -100), Vector2(200, 200)));
	set_use_local_coordinates(false);
	set_draw_order(DRAW_ORDER_LIFETIME);
	set_speed_scale(1);
	set_fixed_fps(30);
	set_collision_base_size(collision_base_size);
}

GPUParticles2D::~GPUParticles2D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(particles);
	RS::get_singleton()->free_rid(mesh);
}


