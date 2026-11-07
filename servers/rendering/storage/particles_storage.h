/**************************************************************************/
/*  particles_storage.h                                                   */
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
/* permit persons to whom the Software is furnished to do so, subject to */
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

#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererParticlesStorage
{
public:
	/* PARTICLES FUNCTION POINTERS */
	inline static RID (*particles_allocate)() = nullptr;
	inline static void (*particles_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_free)(RID p_rid) = nullptr;

	inline static void (*particles_set_mode)(RID p_particles, RSE::ParticlesMode p_mode) = nullptr;

	inline static void (*particles_set_emitting)(RID p_particles, bool p_emitting) = nullptr;
	inline static bool (*particles_get_emitting)(RID p_particles) = nullptr;

	inline static void (*particles_set_amount)(RID p_particles, int p_amount) = nullptr;
	inline static void (*particles_set_amount_ratio)(RID p_particles, float p_amount_ratio) = nullptr;
	inline static void (*particles_set_lifetime)(RID p_particles, double p_lifetime) = nullptr;
	inline static void (*particles_set_one_shot)(RID p_particles, bool p_one_shot) = nullptr;
	inline static void (*particles_set_pre_process_time)(RID p_particles, double p_time) = nullptr;
	inline static void (*particles_request_process_time)(
		RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual) = nullptr;
	inline static void (*particles_set_explosiveness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_randomness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_custom_aabb)(RID p_particles, const AABB &p_aabb) = nullptr;
	inline static void (*particles_set_speed_scale)(RID p_particles, double p_scale) = nullptr;
	inline static void (*particles_set_use_local_coordinates)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_process_material)(RID p_particles, RID p_material) = nullptr;
	inline static RID (*particles_get_process_material)(RID p_particles) = nullptr;
	inline static void (*particles_set_fixed_fps)(RID p_particles, int p_fps) = nullptr;
	inline static void (*particles_set_interpolate)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_fractional_delta)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_collision_base_size)(RID p_particles, real_t p_size) = nullptr;

	inline static void (*particles_set_transform_align)(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align) = nullptr;
	inline static void (*particles_set_transform_align_channel_filter)(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel_filter) = nullptr;
	inline static void (*particles_set_transform_align_axis)(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis) = nullptr;

	inline static void (*particles_set_seed)(RID p_particles, uint32_t p_seed) = nullptr;

	inline static void (*particles_set_trails)(RID p_particles, bool p_enable, double p_length) = nullptr;
	inline static void (*particles_set_trail_bind_poses)(
		RID p_particles, const Vector<Transform3D> &p_bind_poses) = nullptr;

	inline static void (*particles_restart)(RID p_particles) = nullptr;
	inline static void (*particles_emit)(
		RID p_particles, const Transform3D &p_transform, const Vector3 &p_velocity,
		const Color &p_color, const Color &p_custom, uint32_t p_emit_flags) = nullptr;
	inline static void (*particles_set_subemitter)(RID p_particles, RID p_subemitter_particles) = nullptr;

	inline static bool (*particles_is_inactive)(RID p_particles) = nullptr;

	inline static void (*particles_set_draw_order)(RID p_particles, RSE::ParticlesDrawOrder p_order) = nullptr;

	inline static void (*particles_set_draw_passes)(RID p_particles, int p_count) = nullptr;
	inline static void (*particles_set_draw_pass_mesh)(RID p_particles, int p_pass, RID p_mesh) = nullptr;

	inline static void (*particles_request_process)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_current_aabb)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_aabb)(RID p_particles) = nullptr;

	inline static void (*particles_set_emission_transform)(
		RID p_particles, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_set_emitter_velocity)(RID p_particles, const Vector3 &p_velocity) = nullptr;
	inline static void (*particles_set_interp_to_end)(RID p_particles, float p_interp_to_end) = nullptr;

	inline static int (*particles_get_draw_passes)(RID p_particles) = nullptr;
	inline static RID (*particles_get_draw_pass_mesh)(RID p_particles, int p_pass) = nullptr;

	inline static void (*particles_set_view_axis)(
		RID p_particles, const Vector3 &p_axis, const Vector3 &p_up_axis) = nullptr;

	inline static void (*particles_add_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;
	inline static void (*particles_remove_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;

	inline static void (*update_particles)() = nullptr;

	/* PARTICLES COLLISION FUNCTION POINTERS */
	inline static RID (*particles_collision_allocate)() = nullptr;
	inline static void (*particles_collision_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_collision_free)(RID p_rid) = nullptr;

	inline static void (*particles_collision_set_collision_type)(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type) = nullptr;
	inline static void (*particles_collision_set_cull_mask)(
		RID p_particles_collision, uint32_t p_cull_mask) = nullptr;
	inline static void (*particles_collision_set_sphere_radius)(
		RID p_particles_collision, real_t p_radius) = nullptr;
	inline static void (*particles_collision_set_box_extents)(
		RID p_particles_collision, const Vector3 &p_extents) = nullptr;
	inline static void (*particles_collision_set_attractor_strength)(
		RID p_particles_collision, real_t p_strength) = nullptr;
	inline static void (*particles_collision_set_attractor_directionality)(
		RID p_particles_collision, real_t p_directionality) = nullptr;
	inline static void (*particles_collision_set_attractor_attenuation)(
		RID p_particles_collision, real_t p_curve) = nullptr;
	inline static void (*particles_collision_set_field_texture)(
		RID p_particles_collision, RID p_texture) = nullptr;
	inline static void (*particles_collision_height_field_update)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_resolution)(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution) = nullptr;
	inline static AABB (*particles_collision_get_aabb)(RID p_particles_collision) = nullptr;
	inline static bool (*particles_collision_is_heightfield)(RID p_particles_collision) = nullptr;
	inline static uint32_t (*particles_collision_get_height_field_mask)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_mask)(
		RID p_particles_collision, uint32_t p_heightfield_mask) = nullptr;
	inline static uint32_t (*particles_collision_get_cull_mask)(RID p_particles_collision) = nullptr;

	inline static RID (*particles_collision_instance_create)(RID p_collision) = nullptr;
	inline static void (*particles_collision_instance_free)(RID p_rid) = nullptr;
	inline static void (*particles_collision_instance_set_transform)(
		RID p_collision_instance, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_collision_instance_set_active)(
		RID p_collision_instance, bool p_active) = nullptr;

	/* BIND TEMPLATE */
	template <typename T>
	static void bind()
	{
		particles_allocate = T::particles_allocate;
		particles_initialize = T::particles_initialize;
		particles_free = T::particles_free;

		particles_set_mode = T::particles_set_mode;

		particles_set_emitting = T::particles_set_emitting;
		particles_get_emitting = T::particles_get_emitting;

		particles_set_amount = T::particles_set_amount;
		particles_set_amount_ratio = T::particles_set_amount_ratio;
		particles_set_lifetime = T::particles_set_lifetime;
		particles_set_one_shot = T::particles_set_one_shot;
		particles_set_pre_process_time = T::particles_set_pre_process_time;
		particles_request_process_time = T::particles_request_process_time;
		particles_set_explosiveness_ratio = T::particles_set_explosiveness_ratio;
		particles_set_randomness_ratio = T::particles_set_randomness_ratio;
		particles_set_custom_aabb = T::particles_set_custom_aabb;
		particles_set_speed_scale = T::particles_set_speed_scale;
		particles_set_use_local_coordinates = T::particles_set_use_local_coordinates;
		particles_set_process_material = T::particles_set_process_material;
		particles_get_process_material = T::particles_get_process_material;
		particles_set_fixed_fps = T::particles_set_fixed_fps;
		particles_set_interpolate = T::particles_set_interpolate;
		particles_set_fractional_delta = T::particles_set_fractional_delta;
		particles_set_collision_base_size = T::particles_set_collision_base_size;

		particles_set_transform_align = T::particles_set_transform_align;
		particles_set_transform_align_channel_filter = T::particles_set_transform_align_channel_filter;
		particles_set_transform_align_axis = T::particles_set_transform_align_axis;

		particles_set_seed = T::particles_set_seed;

		particles_set_trails = T::particles_set_trails;
		particles_set_trail_bind_poses = T::particles_set_trail_bind_poses;

		particles_restart = T::particles_restart;
		particles_emit = T::particles_emit;
		particles_set_subemitter = T::particles_set_subemitter;

		particles_is_inactive = T::particles_is_inactive;

		particles_set_draw_order = T::particles_set_draw_order;

		particles_set_draw_passes = T::particles_set_draw_passes;
		particles_set_draw_pass_mesh = T::particles_set_draw_pass_mesh;

		particles_request_process = T::particles_request_process;
		particles_get_current_aabb = T::particles_get_current_aabb;
		particles_get_aabb = T::particles_get_aabb;

		particles_set_emission_transform = T::particles_set_emission_transform;
		particles_set_emitter_velocity = T::particles_set_emitter_velocity;
		particles_set_interp_to_end = T::particles_set_interp_to_end;

		particles_get_draw_passes = T::particles_get_draw_passes;
		particles_get_draw_pass_mesh = T::particles_get_draw_pass_mesh;

		particles_set_view_axis = T::particles_set_view_axis;

		particles_add_collision = T::particles_add_collision;
		particles_remove_collision = T::particles_remove_collision;

		update_particles = T::update_particles;

		particles_collision_allocate = T::particles_collision_allocate;
		particles_collision_initialize = T::particles_collision_initialize;
		particles_collision_free = T::particles_collision_free;

		particles_collision_set_collision_type = T::particles_collision_set_collision_type;
		particles_collision_set_cull_mask = T::particles_collision_set_cull_mask;
		particles_collision_set_sphere_radius = T::particles_collision_set_sphere_radius;
		particles_collision_set_box_extents = T::particles_collision_set_box_extents;
		particles_collision_set_attractor_strength = T::particles_collision_set_attractor_strength;
		particles_collision_set_attractor_directionality = T::particles_collision_set_attractor_directionality;
		particles_collision_set_attractor_attenuation = T::particles_collision_set_attractor_attenuation;
		particles_collision_set_field_texture = T::particles_collision_set_field_texture;
		particles_collision_height_field_update = T::particles_collision_height_field_update;
		particles_collision_set_height_field_resolution = T::particles_collision_set_height_field_resolution;
		particles_collision_get_aabb = T::particles_collision_get_aabb;
		particles_collision_is_heightfield = T::particles_collision_is_heightfield;
		particles_collision_get_height_field_mask = T::particles_collision_get_height_field_mask;
		particles_collision_set_height_field_mask = T::particles_collision_set_height_field_mask;
		particles_collision_get_cull_mask = T::particles_collision_get_cull_mask;

		particles_collision_instance_create = T::particles_collision_instance_create;
		particles_collision_instance_free = T::particles_collision_instance_free;
		particles_collision_instance_set_transform = T::particles_collision_instance_set_transform;
		particles_collision_instance_set_active = T::particles_collision_instance_set_active;
	}
};
