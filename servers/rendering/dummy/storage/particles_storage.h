/**************************************************************************/
/*  particles_storage.h                                                   */
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

#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "core/types.h"
#include "servers/rendering/rendering_server_enums.h"

namespace RendererDummy
{

class ParticlesStorage final
{
public:
	ParticlesStorage() = delete;
	~ParticlesStorage() = delete;
	ParticlesStorage(const ParticlesStorage&) = delete;
	ParticlesStorage& operator=(const ParticlesStorage&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* PARTICLES */

	static bool owns_particles(RID p_rid) { return false; }

	static RID particles_allocate() { return RID(); }

	static void particles_initialize(RID p_rid) {}

	static void particles_free(RID p_rid) {}

	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode) {}

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags)
	{
	}

	static void particles_set_emitting(RID p_particles, bool p_emitting) {}

	static void particles_set_amount(RID p_particles, int p_amount) {}

	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio) {}

	static void particles_set_lifetime(RID p_particles, double p_lifetime) {}

	static void particles_set_one_shot(RID p_particles, bool p_one_shot) {}

	static void particles_set_pre_process_time(RID p_particles, double p_time) {}

	static void particles_request_process_time(
		RID p_particles, real_t p_request_process_time, real_t p_particles_request_process_time)
	{
	}

	static void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_randomness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_seed(RID p_particles, uint32_t p_seed) {}

	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb) {}

	static void particles_set_speed_scale(RID p_particles, double p_scale) {}

	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable) {}

	static void particles_set_process_material(RID p_particles, RID p_material) {}

	static RID particles_get_process_material(RID p_particles) { return RID(); }

	static void particles_set_fixed_fps(RID p_particles, int p_fps) {}

	static void particles_set_interpolate(RID p_particles, bool p_enable) {}

	static void particles_set_fractional_delta(RID p_particles, bool p_enable) {}

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles) {}

	static void particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
	{
	}

	static void particles_set_collision_base_size(RID p_particles, real_t p_size) {}

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align)
	{
	}

	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter)
	{
	}

	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis)
	{
	}

	static void particles_set_trails(RID p_particles, bool p_enable, double p_length) {}

	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses)
	{
	}

	static void particles_restart(RID p_particles) {}

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order) {}

	static void particles_set_draw_passes(RID p_particles, int p_count) {}

	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) {}

	static void particles_request_process(RID p_particles) {}

	static AABB particles_get_current_aabb(RID p_particles) { return AABB(); }

	static AABB particles_get_aabb(RID p_particles) { return AABB(); }

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform) {}

	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity) {}

	static void particles_set_interp_to_end(RID p_particles, float p_interp) {}

	static bool particles_get_emitting(RID p_particles) { return false; }

	static int particles_get_draw_passes(RID p_particles) { return 0; }

	static RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) { return RID(); }

	static void particles_add_collision(RID p_particles, RID p_instance) {}

	static void particles_remove_collision(RID p_particles, RID p_instance) {}

	static void update_particles() {}

	/* PARTICLES COLLISION */

	static bool owns_particles_collision(RID p_rid) { return false; }

	static RID particles_collision_allocate() { return RID(); }

	static void particles_collision_initialize(RID p_rid) {}

	static void particles_collision_free(RID p_rid) {}

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type)
	{
	}

	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask)
	{
	}

	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius) {}

	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents)
	{
	}

	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength)
	{
	}

	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality)
	{
	}

	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve)
	{
	}

	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture) {}

	static void particles_collision_height_field_update(RID p_particles_collision) {}

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution)
	{
	}

	static AABB particles_collision_get_aabb(RID p_particles_collision) { return AABB(); }

	static bool particles_collision_is_heightfield(RID p_particles_collision) { return false; }

	static uint32_t particles_collision_get_height_field_mask(RID p_particles_collision)
	{
		return 0;
	}

	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask)
	{
	}

	static uint32_t particles_collision_get_cull_mask(RID p_particles_collision) { return 0; }

	static bool owns_particles_collision_instance(RID p_rid) { return false; }

	static RID particles_collision_instance_create(RID p_collision) { return RID(); }

	static void particles_collision_instance_free(RID p_rid) {}

	static void particles_collision_instance_set_transform(
		RID p_collision_instance, const Transform3D& p_transform)
	{
	}

	static void particles_collision_instance_set_active(RID p_collision_instance, bool p_active) {}

	static bool particles_is_inactive(RID p_particles) { return false; }
};

} // namespace RendererDummy
