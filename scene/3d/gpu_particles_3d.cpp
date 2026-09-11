/**************************************************************************/
/*  gpu_particles_3d.cpp                                                  */
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
#include "gpu_particles_3d.h"
#include "scene/3d/cpu_particles_3d.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/mesh.h"
#include "scene/resources/particle_process_material.h"
#include "servers/rendering/rendering_server.h"

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
	RS::get_singleton()->particles_set_emitting(particles, p_emitting);
}

void GPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	RS::get_singleton()->particles_set_amount(particles, amount);
}

void GPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	RS::get_singleton()->particles_set_lifetime(particles, lifetime);
}

void GPUParticles3D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	RS::get_singleton()->particles_set_interp_to_end(particles, interp_to_end_factor);
}

void GPUParticles3D::set_one_shot(bool p_one_shot)
{
	one_shot = p_one_shot;
	RS::get_singleton()->particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		if (!one_shot) {
			RenderingServer::get_singleton()->particles_restart(particles);
		}
	}
}



bool GPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles3D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	RS::get_singleton()->particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles3D::get_seed() const { return seed; }

void GPUParticles3D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	RS::get_singleton()->particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles3D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	RS::get_singleton()->particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles3D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	RS::get_singleton()->particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles3D::set_visibility_aabb(const AABB& p_aabb)
{
	visibility_aabb = p_aabb;
	RS::get_singleton()->particles_set_custom_aabb(particles, visibility_aabb);
	update_gizmos();
}

void GPUParticles3D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	RS::get_singleton()->particles_set_use_local_coordinates(particles, local_coords);
}



void GPUParticles3D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	RS::get_singleton()->particles_set_speed_scale(particles, p_scale);
}

void GPUParticles3D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	RS::get_singleton()->particles_set_collision_base_size(particles, p_size);
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
	RS::get_singleton()->particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

void GPUParticles3D::set_trail_enabled(bool p_enabled)
{
	trail_enabled = p_enabled;
	RS::get_singleton()->particles_set_trails(particles, trail_enabled, trail_lifetime);
	update_configuration_warnings();
}

void GPUParticles3D::set_trail_lifetime(double p_seconds)
{
	ERR_FAIL_COND(p_seconds < 0.01 - CMP_EPSILON);
	trail_lifetime = p_seconds;
	RS::get_singleton()->particles_set_trails(particles, trail_enabled, trail_lifetime);
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
	RS::get_singleton()->particles_set_fixed_fps(particles, p_count);
}

int GPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles3D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	RS::get_singleton()->particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles3D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	RS::get_singleton()->particles_set_interpolate(particles, p_enable);
}

bool GPUParticles3D::get_interpolate() const { return interpolate; }



void GPUParticles3D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	RenderingServer::get_singleton()->particles_restart(particles);
	RenderingServer::get_singleton()->particles_set_emitting(particles, true);

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
	return RS::get_singleton()->particles_get_current_aabb(particles);
}



void GPUParticles3D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	RS::get_singleton()->particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	// Setting emitting independently from set_emitting is important here
	// we assume to be in a controlled process situation.
	// setting RS emission to true is necessary to ensure particles will emit when
	// regular process time is > than zero but the particles are already in trailing mode.
	// this could also be done on the GScript/tool side.
	if (p_requested_process_time > 0.0) {
		emitting = true;
		RS::get_singleton()->particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

void GPUParticles3D::emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	RS::get_singleton()->particles_emit(
		particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}



void GPUParticles3D::set_sub_emitter(const NodePath& p_path)
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

NodePath GPUParticles3D::get_sub_emitter() const { return sub_emitter; }

void GPUParticles3D::_skinning_changed()
{
	Vector<Transform3D> xforms;
	if (skin.is_valid()) {
		xforms.resize(skin->get_bind_count());
		for (int i = 0; i < skin->get_bind_count(); i++) {
			xforms.write[i] = skin->get_bind_pose(i);
		}
	}
	else {
		for (int i = 0; i < draw_passes.size(); i++) {
			Ref<Mesh> draw_pass = draw_passes[i];
			if (draw_pass.is_valid() && draw_pass->get_builtin_bind_pose_count() > 0) {
				xforms.resize(draw_pass->get_builtin_bind_pose_count());
				for (int j = 0; j < draw_pass->get_builtin_bind_pose_count(); j++) {
					xforms.write[j] = draw_pass->get_builtin_bind_pose(j);
				}
				break;
			}
		}
	}

	RS::get_singleton()->particles_set_trail_bind_poses(particles, xforms);
	update_configuration_warnings();
}

void GPUParticles3D::set_skin(const Ref<Skin>& p_skin)
{
	skin = p_skin;
	_skinning_changed();
}

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
	RS::get_singleton()->particles_set_transform_align_channel_filter(
		particles, transform_align_channel_filter);
}

RSE::ParticlesTransformAlignCustomSrc GPUParticles3D::get_transform_align_channel_filter() const
{
	return transform_align_channel_filter;
}

void GPUParticles3D::set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis)
{
	transform_align_axis = p_axis;
	RS::get_singleton()->particles_set_transform_align_axis(particles, p_axis);
}

RSE::ParticlesTransformAlignAxis GPUParticles3D::get_transform_align_axis() const
{
	return transform_align_axis;
}



void GPUParticles3D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	RS::get_singleton()->particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles3D::get_amount_ratio() const { return amount_ratio; }



GPUParticles3D::GPUParticles3D()
{
	particles = RS::get_singleton()->particles_create();
	RS::get_singleton()->particles_set_mode(particles, RSE::PARTICLES_MODE_3D);
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
	set_visibility_aabb(AABB(Vector3(-4, -4, -4), Vector3(8, 8, 8)));
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

GPUParticles3D::~GPUParticles3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(particles);
}


