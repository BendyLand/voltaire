/**************************************************************************/
/*  gpu_particles_collision_3d.cpp                                        */
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

#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "gpu_particles_collision_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/rendering_server.h"

void GPUParticlesCollision3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	RS::get_singleton()->particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesCollision3D::get_cull_mask() const { return cull_mask; }

GPUParticlesCollision3D::GPUParticlesCollision3D(RSE::ParticlesCollisionType p_type)
{
	collision = RS::get_singleton()->particles_collision_create();
	RS::get_singleton()->particles_collision_set_collision_type(collision, p_type);
	set_base(collision);
}

GPUParticlesCollision3D::~GPUParticlesCollision3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(collision);
}

void GPUParticlesCollisionSphere3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	RS::get_singleton()->particles_collision_set_sphere_radius(_get_collision(), radius);
	update_gizmos();
}

real_t GPUParticlesCollisionSphere3D::get_radius() const { return radius; }

AABB GPUParticlesCollisionSphere3D::get_aabb() const
{
	return AABB(Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
}

GPUParticlesCollisionSphere3D::GPUParticlesCollisionSphere3D()
	: GPUParticlesCollision3D(RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE)
{
}

GPUParticlesCollisionSphere3D::~GPUParticlesCollisionSphere3D() {}

void GPUParticlesCollisionBox3D::set_size(const Vector3& p_size)
{
	size = p_size;
	RS::get_singleton()->particles_collision_set_box_extents(_get_collision(), size / 2);
	update_gizmos();
}

Vector3 GPUParticlesCollisionBox3D::get_size() const { return size; }

AABB GPUParticlesCollisionBox3D::get_aabb() const { return AABB(-size / 2, size); }

GPUParticlesCollisionBox3D::GPUParticlesCollisionBox3D()
	: GPUParticlesCollision3D(RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE)
{
}

GPUParticlesCollisionBox3D::~GPUParticlesCollisionBox3D() {}

static _FORCE_INLINE_ real_t Vector3_dot2(const Vector3& p_vec3) { return p_vec3.dot(p_vec3); }

Vector3i GPUParticlesCollisionSDF3D::get_estimated_cell_size() const
{
	static const int subdivs[RESOLUTION_MAX] = {16, 32, 64, 128, 256, 512};
	int subdiv = subdivs[get_resolution()];

	AABB aabb(-size / 2, size);

	float cell_size = aabb.get_longest_axis_size() / float(subdiv);

	Vector3i sdf_size = Vector3i(aabb.size / cell_size);
	sdf_size = sdf_size.maxi(1);
	return sdf_size;
}

PackedStringArray GPUParticlesCollisionSDF3D::get_configuration_warnings() const
{
	PackedStringArray warnings = GPUParticlesCollision3D::get_configuration_warnings();

	if (bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any collision for this GPUParticlesCollisionSDF3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	return warnings;
}

void GPUParticlesCollisionSDF3D::set_thickness(float p_thickness) { thickness = p_thickness; }

float GPUParticlesCollisionSDF3D::get_thickness() const { return thickness; }

void GPUParticlesCollisionSDF3D::set_size(const Vector3& p_size)
{
	size = p_size;
	RS::get_singleton()->particles_collision_set_box_extents(_get_collision(), size / 2);
	update_gizmos();
}

Vector3 GPUParticlesCollisionSDF3D::get_size() const { return size; }

void GPUParticlesCollisionSDF3D::set_resolution(Resolution p_resolution)
{
	resolution = p_resolution;
	update_gizmos();
}

GPUParticlesCollisionSDF3D::Resolution GPUParticlesCollisionSDF3D::get_resolution() const
{
	return resolution;
}

void GPUParticlesCollisionSDF3D::set_bake_mask(uint32_t p_mask)
{
	bake_mask = p_mask;
	update_configuration_warnings();
}

uint32_t GPUParticlesCollisionSDF3D::get_bake_mask() const { return bake_mask; }

void GPUParticlesCollisionSDF3D::set_bake_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 20,
		vformat(
			"The render layer number (%d) must be between 1 and 20 (inclusive).", p_layer_number));
	uint32_t mask = get_bake_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_bake_mask(mask);
}

bool GPUParticlesCollisionSDF3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 20, false,
		vformat(
			"The render layer number (%d) must be between 1 and 20 (inclusive).", p_layer_number));
	return bake_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollisionSDF3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	RS::get_singleton()->particles_collision_set_field_texture(_get_collision(), tex);
}

Ref<Texture3D> GPUParticlesCollisionSDF3D::get_texture() const { return texture; }

AABB GPUParticlesCollisionSDF3D::get_aabb() const { return AABB(-size / 2, size); }

GPUParticlesCollisionSDF3D::BakeBeginFunc GPUParticlesCollisionSDF3D::bake_begin_function = nullptr;
GPUParticlesCollisionSDF3D::BakeStepFunc GPUParticlesCollisionSDF3D::bake_step_function = nullptr;
GPUParticlesCollisionSDF3D::BakeEndFunc GPUParticlesCollisionSDF3D::bake_end_function = nullptr;

GPUParticlesCollisionSDF3D::GPUParticlesCollisionSDF3D()
	: GPUParticlesCollision3D(RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE)
{
}

GPUParticlesCollisionSDF3D::~GPUParticlesCollisionSDF3D() {}

void GPUParticlesCollisionHeightField3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (update_mode == UPDATE_MODE_ALWAYS) {
			RS::get_singleton()->particles_collision_height_field_update(_get_collision());
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
					RS::get_singleton()->particles_collision_height_field_update(_get_collision());
				}
			}
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		RS::get_singleton()->particles_collision_height_field_update(_get_collision());
	} break;
	}
}

void GPUParticlesCollisionHeightField3D::set_size(const Vector3& p_size)
{
	size = p_size;
	RS::get_singleton()->particles_collision_set_box_extents(_get_collision(), size / 2);
	update_gizmos();
	RS::get_singleton()->particles_collision_height_field_update(_get_collision());
}

Vector3 GPUParticlesCollisionHeightField3D::get_size() const { return size; }

void GPUParticlesCollisionHeightField3D::set_resolution(Resolution p_resolution)
{
	resolution = p_resolution;
	RS::get_singleton()->particles_collision_set_height_field_resolution(
		_get_collision(), RSE::ParticlesCollisionHeightfieldResolution(resolution));
	update_gizmos();
	RS::get_singleton()->particles_collision_height_field_update(_get_collision());
}

GPUParticlesCollisionHeightField3D::Resolution
GPUParticlesCollisionHeightField3D::get_resolution() const
{
	return resolution;
}

void GPUParticlesCollisionHeightField3D::set_update_mode(UpdateMode p_update_mode)
{
	update_mode = p_update_mode;
	set_process_internal(follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS);
}

GPUParticlesCollisionHeightField3D::UpdateMode
GPUParticlesCollisionHeightField3D::get_update_mode() const
{
	return update_mode;
}

void GPUParticlesCollisionHeightField3D::set_heightfield_mask(uint32_t p_heightfield_mask)
{
	heightfield_mask = p_heightfield_mask;
	RS::get_singleton()->particles_collision_set_height_field_mask(
		_get_collision(), p_heightfield_mask);
}

uint32_t GPUParticlesCollisionHeightField3D::get_heightfield_mask() const
{
	return heightfield_mask;
}

void GPUParticlesCollisionHeightField3D::set_heightfield_mask_value(
	int p_layer_number, bool p_value)
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

bool GPUParticlesCollisionHeightField3D::get_heightfield_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return heightfield_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollisionHeightField3D::set_follow_camera_enabled(bool p_enabled)
{
	follow_camera_mode = p_enabled;
	set_process_internal(follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS);
}

bool GPUParticlesCollisionHeightField3D::is_follow_camera_enabled() const
{
	return follow_camera_mode;
}

AABB GPUParticlesCollisionHeightField3D::get_aabb() const { return AABB(-size / 2, size); }

GPUParticlesCollisionHeightField3D::GPUParticlesCollisionHeightField3D()
	: GPUParticlesCollision3D(RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE)
{
}

GPUParticlesCollisionHeightField3D::~GPUParticlesCollisionHeightField3D() {}

void GPUParticlesAttractor3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	RS::get_singleton()->particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesAttractor3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesAttractor3D::set_strength(real_t p_strength)
{
	strength = p_strength;
	RS::get_singleton()->particles_collision_set_attractor_strength(collision, p_strength);
}

real_t GPUParticlesAttractor3D::get_strength() const { return strength; }

void GPUParticlesAttractor3D::set_attenuation(real_t p_attenuation)
{
	attenuation = p_attenuation;
	RS::get_singleton()->particles_collision_set_attractor_attenuation(collision, p_attenuation);
}

real_t GPUParticlesAttractor3D::get_attenuation() const { return attenuation; }

void GPUParticlesAttractor3D::set_directionality(real_t p_directionality)
{
	directionality = p_directionality;
	RS::get_singleton()->particles_collision_set_attractor_directionality(
		collision, p_directionality);
	update_gizmos();
}

real_t GPUParticlesAttractor3D::get_directionality() const { return directionality; }

GPUParticlesAttractor3D::GPUParticlesAttractor3D(RSE::ParticlesCollisionType p_type)
{
	collision = RS::get_singleton()->particles_collision_create();
	RS::get_singleton()->particles_collision_set_collision_type(collision, p_type);
	set_base(collision);
}

GPUParticlesAttractor3D::~GPUParticlesAttractor3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(collision);
}

void GPUParticlesAttractorSphere3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	RS::get_singleton()->particles_collision_set_sphere_radius(_get_collision(), radius);
	update_gizmos();
}

real_t GPUParticlesAttractorSphere3D::get_radius() const { return radius; }

AABB GPUParticlesAttractorSphere3D::get_aabb() const
{
	return AABB(Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
}

GPUParticlesAttractorSphere3D::GPUParticlesAttractorSphere3D()
	: GPUParticlesAttractor3D(RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT)
{
}

GPUParticlesAttractorSphere3D::~GPUParticlesAttractorSphere3D() {}

void GPUParticlesAttractorBox3D::set_size(const Vector3& p_size)
{
	size = p_size;
	RS::get_singleton()->particles_collision_set_box_extents(_get_collision(), size / 2);
	update_gizmos();
}

Vector3 GPUParticlesAttractorBox3D::get_size() const { return size; }

AABB GPUParticlesAttractorBox3D::get_aabb() const { return AABB(-size / 2, size); }

GPUParticlesAttractorBox3D::GPUParticlesAttractorBox3D()
	: GPUParticlesAttractor3D(RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT)
{
}

GPUParticlesAttractorBox3D::~GPUParticlesAttractorBox3D() {}

void GPUParticlesAttractorVectorField3D::set_size(const Vector3& p_size)
{
	size = p_size;
	RS::get_singleton()->particles_collision_set_box_extents(_get_collision(), size / 2);
	update_gizmos();
}

Vector3 GPUParticlesAttractorVectorField3D::get_size() const { return size; }

void GPUParticlesAttractorVectorField3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	RS::get_singleton()->particles_collision_set_field_texture(_get_collision(), tex);
}

Ref<Texture3D> GPUParticlesAttractorVectorField3D::get_texture() const { return texture; }

AABB GPUParticlesAttractorVectorField3D::get_aabb() const { return AABB(-size / 2, size); }

GPUParticlesAttractorVectorField3D::GPUParticlesAttractorVectorField3D()
	: GPUParticlesAttractor3D(RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT)
{
}

GPUParticlesAttractorVectorField3D::~GPUParticlesAttractorVectorField3D() {}


