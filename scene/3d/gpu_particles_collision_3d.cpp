/**************************************************************************/
/*  gpu_particles_collision_3d.cpp                                        */
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
