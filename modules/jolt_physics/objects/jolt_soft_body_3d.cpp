/**************************************************************************/
/*  jolt_soft_body_3d.cpp                                                 */
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

#include "../misc/jolt_type_conversions.h"
#include "../spaces/jolt_broad_phase_layer.h"
#include "../spaces/jolt_space_3d.h"
#include "core/config/engine.h"
#include "jolt_area_3d.h"
#include "jolt_body_3d.h"
#include "jolt_group_filter.h"
#include "jolt_soft_body_3d.h"
#include "servers/physics_3d/physics_server_3d_rendering_server_handler.h"
#include "servers/rendering/rendering_server.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>

namespace
{

template <typename TJoltVertex>
void pin_vertices(const JoltSoftBody3D& p_body, const HashSet<int>& p_pinned_vertices,
	const LocalVector<int>& p_mesh_to_physics, JPH::Array<TJoltVertex>& r_physics_vertices)
{
	const int mesh_vertex_count = p_mesh_to_physics.size();
	const int physics_vertex_count = (int)r_physics_vertices.size();

	for (int mesh_index : p_pinned_vertices) {
		ERR_CONTINUE_MSG(mesh_index < 0 || mesh_index >= mesh_vertex_count,
			vformat("Index %d of pinned vertex in soft body '%s' is out of bounds. There are only "
					"%d vertices in the current mesh.",
				mesh_index, p_body.to_string(), mesh_vertex_count));

		const int physics_index = p_mesh_to_physics[mesh_index];
		ERR_CONTINUE_MSG(physics_index < 0 || physics_index >= physics_vertex_count,
			vformat("Index %d of pinned vertex in soft body '%s' is out of bounds. There are only "
					"%d vertices in the current mesh. This should not happen. Please report this.",
				physics_index, p_body.to_string(), physics_vertex_count));

		r_physics_vertices[physics_index].mInvMass = 0.0f;
	}
}

} // namespace

JPH::BroadPhaseLayer JoltSoftBody3D::_get_broad_phase_layer() const
{
	return JoltBroadPhaseLayer::BODY_DYNAMIC;
}

JPH::ObjectLayer JoltSoftBody3D::_get_object_layer() const
{
	ERR_FAIL_NULL_V(space, 0);

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

void JoltSoftBody3D::_space_changed()
{
	JoltObject3D::_space_changed();

	_update_mass();
	_update_pressure();
	_update_damping();
	_update_simulation_precision();
	_update_group_filter();
}

void JoltSoftBody3D::_update_pressure()
{
	if (!in_space()) {
		jolt_settings->mPressure = pressure;
		return;
	}

	JPH::SoftBodyMotionProperties& motion_properties =
		static_cast<JPH::SoftBodyMotionProperties&>(*jolt_body->GetMotionPropertiesUnchecked());
	motion_properties.SetPressure(pressure);
}

void JoltSoftBody3D::_update_simulation_precision()
{
	if (!in_space()) {
		jolt_settings->mNumIterations = (JPH::uint32)simulation_precision;
		return;
	}

	JPH::SoftBodyMotionProperties& motion_properties =
		static_cast<JPH::SoftBodyMotionProperties&>(*jolt_body->GetMotionPropertiesUnchecked());
	motion_properties.SetNumIterations((JPH::uint32)simulation_precision);
}

void JoltSoftBody3D::_try_rebuild()
{
	if (space != nullptr) {
		_reset_space();
	}
}

void JoltSoftBody3D::_mesh_changed() { _try_rebuild(); }

void JoltSoftBody3D::_simulation_precision_changed() { wake_up(); }

void JoltSoftBody3D::_mass_changed()
{
	_update_mass();
	wake_up();
}

void JoltSoftBody3D::_pressure_changed()
{
	_update_pressure();
	wake_up();
}

void JoltSoftBody3D::_damping_changed()
{
	_update_damping();
	wake_up();
}

void JoltSoftBody3D::_pins_changed()
{
	_update_mass();
	wake_up();
}

void JoltSoftBody3D::_vertices_changed() { wake_up(); }

void JoltSoftBody3D::_exceptions_changed() { _update_group_filter(); }

void JoltSoftBody3D::_motion_changed() { wake_up(); }

void JoltSoftBody3D::_transform_changed() { wake_up(); }

void JoltSoftBody3D::_areas_changed() { wake_up(); }

JoltSoftBody3D::JoltSoftBody3D() : JoltObject3D(OBJECT_TYPE_SOFT_BODY)
{
	jolt_settings->mRestitution = 0.0f;
	jolt_settings->mFriction = 1.0f;
	jolt_settings->mUpdatePosition = true;
	jolt_settings->mMakeRotationIdentity = false;
}

JoltSoftBody3D::~JoltSoftBody3D()
{
	if (jolt_settings != nullptr) {
		delete jolt_settings;
		jolt_settings = nullptr;
	}
}



bool JoltSoftBody3D::can_interact_with(const JoltSoftBody3D& p_other) const
{
	return (can_collide_with(p_other) || p_other.can_collide_with(*this)) &&
		   !has_collision_exception(p_other.get_rid()) && !p_other.has_collision_exception(rid);
}

bool JoltSoftBody3D::can_interact_with(const JoltArea3D& p_other) const
{
	return p_other.can_interact_with(*this);
}

Vector3 JoltSoftBody3D::get_velocity_at_position(const Vector3& p_position) const
{
	return Vector3();
}

void JoltSoftBody3D::pre_step(float p_step) { _apply_environmental_forces(p_step); }

void JoltSoftBody3D::set_mesh(const RID& p_mesh)
{
	if (unlikely(mesh == p_mesh)) {
		return;
	}

	mesh = p_mesh;
	_mesh_changed();
}

bool JoltSoftBody3D::is_sleeping() const
{
	if (!in_space()) {
		return false;
	}
	else {
		return !jolt_body->IsActive();
	}
}

void JoltSoftBody3D::apply_vertex_force(int p_index, const Vector3& p_force)
{
	ERR_FAIL_COND_MSG(
		!in_space(), vformat("Failed to apply force to '%s'. Doing so without a physics space is "
							 "not supported when using Jolt Physics. If this relates to a node, "
							 "try adding the node to a scene tree first.",
						 to_string()));

	apply_vertex_impulse(p_index, p_force * space->get_last_step());
}

void JoltSoftBody3D::apply_central_impulse(const Vector3& p_impulse)
{
	ERR_FAIL_COND_MSG(
		!in_space(), vformat("Failed to apply central impulse to '%s'. Doing so without a physics "
							 "space is not supported when using Jolt Physics. If this relates to a "
							 "node, try adding the node to a scene tree first.",
						 to_string()));

	JPH::SoftBodyMotionProperties& motion_properties =
		static_cast<JPH::SoftBodyMotionProperties&>(*jolt_body->GetMotionPropertiesUnchecked());
	JPH::Array<JPH::SoftBodyVertex>& physics_vertices = motion_properties.GetVertices();

	const JPH::Vec3 impulse = to_jolt(p_impulse) / physics_vertices.size();

	for (JPH::SoftBodyVertex& physics_vertex : physics_vertices) {
		if (physics_vertex.mInvMass > 0.0f) {
			physics_vertex.mVelocity += impulse * physics_vertex.mInvMass;
		}
	}

	_motion_changed();
}

void JoltSoftBody3D::apply_central_force(const Vector3& p_force)
{
	ERR_FAIL_COND_MSG(
		!in_space(), vformat("Failed to apply central force to '%s'. Doing so without a physics "
							 "space is not supported when using Jolt Physics. If this relates to a "
							 "node, try adding the node to a scene tree first.",
						 to_string()));

	jolt_body->AddForce(to_jolt(p_force));

	_motion_changed();
}

void JoltSoftBody3D::set_is_sleeping(bool p_enabled)
{
	if (!in_space()) {
		return;
	}

	space->set_is_object_sleeping(jolt_body->GetID(), p_enabled);
}

bool JoltSoftBody3D::is_sleep_allowed() const
{
	if (!in_space()) {
		return jolt_settings->mAllowSleeping;
	}
	else {
		return jolt_body->GetAllowSleeping();
	}
}

void JoltSoftBody3D::set_is_sleep_allowed(bool p_enabled)
{
	if (!in_space()) {
		jolt_settings->mAllowSleeping = p_enabled;
	}
	else {
		jolt_body->SetAllowSleeping(p_enabled);
	}
}

void JoltSoftBody3D::set_simulation_precision(int p_precision)
{
	if (unlikely(simulation_precision == p_precision)) {
		return;
	}

	simulation_precision = MAX(p_precision, 0);

	_simulation_precision_changed();
}

void JoltSoftBody3D::set_mass(float p_mass)
{
	ERR_FAIL_COND(p_mass <= 0.0); // A mass of zero would result in infinite inverse mass.

	if (unlikely(mass == p_mass)) {
		return;
	}

	mass = p_mass;

	_mass_changed();
}

float JoltSoftBody3D::get_stiffness_coefficient() const { return stiffness_coefficient; }

void JoltSoftBody3D::set_stiffness_coefficient(float p_coefficient)
{
	stiffness_coefficient = CLAMP(p_coefficient, 0.0f, 1.0f);
}

float JoltSoftBody3D::get_shrinking_factor() const { return shrinking_factor; }

void JoltSoftBody3D::set_shrinking_factor(float p_shrinking_factor)
{
	shrinking_factor = p_shrinking_factor;
}

void JoltSoftBody3D::set_pressure(float p_pressure)
{
	if (unlikely(pressure == p_pressure)) {
		return;
	}

	pressure = MAX(p_pressure, 0.0f);

	_pressure_changed();
}

void JoltSoftBody3D::set_linear_damping(float p_damping)
{
	if (unlikely(linear_damping == p_damping)) {
		return;
	}

	linear_damping = MAX(p_damping, 0.0f);

	_damping_changed();
}

float JoltSoftBody3D::get_drag() const
{
	// Drag is not a thing in Jolt, and not supported by Godot Physics either.
	return 0.0f;
}

void JoltSoftBody3D::set_drag(float p_drag)
{
	// Drag is not a thing in Jolt, and not supported by Godot Physics either.
}

Transform3D JoltSoftBody3D::get_transform() const
{
	// Since any transform gets baked into the vertices anyway we can just return identity here.
	return Transform3D();
}

void JoltSoftBody3D::set_transform(const Transform3D& p_transform)
{
	ERR_FAIL_COND_MSG(
		!in_space(), vformat("Failed to set transform for '%s'. Doing so without a physics space "
							 "is not supported when using Jolt Physics. If this relates to a node, "
							 "try adding the node to a scene tree first.",
						 to_string()));

	// For whatever reason this has to be interpreted as a relative global-space transform rather
	// than an absolute one, because `SoftBody3D` will immediately upon entering the scene tree set
	// itself to be top-level and also set its transform to be identity, while still expecting to
	// stay in its original position.
	//
	// We also discard any scaling, since we have no way of scaling the actual edge lengths.
	const JPH::Mat44 relative_transform = to_jolt(p_transform.orthonormalized());

	// The translation delta goes to the body's position to avoid vertices getting too far away from
	// it.
	JPH::BodyInterface& body_iface = space->get_body_iface();
	body_iface.SetPosition(jolt_body->GetID(),
		jolt_body->GetPosition() + relative_transform.GetTranslation(),
		JPH::EActivation::DontActivate);

	// The rotation difference goes to the vertices. We also reset the velocity of these vertices.
	JPH::SoftBodyMotionProperties& motion_properties =
		static_cast<JPH::SoftBodyMotionProperties&>(*jolt_body->GetMotionPropertiesUnchecked());
	JPH::Array<JPH::SoftBodyVertex>& physics_vertices = motion_properties.GetVertices();

	for (JPH::SoftBodyVertex& vertex : physics_vertices) {
		vertex.mPosition = vertex.mPreviousPosition =
			relative_transform.Multiply3x3(vertex.mPosition);
		vertex.mVelocity = JPH::Vec3::sZero();
	}

	_transform_changed();
}

AABB JoltSoftBody3D::get_bounds() const
{
	ERR_FAIL_COND_V_MSG(!in_space(), AABB(),
		vformat("Failed to retrieve world bounds of '%s'. Doing so without a physics space is not "
				"supported when using Jolt Physics. If this relates to a node, try adding the node "
				"to a scene tree first.",
			to_string()));
	return to_godot(jolt_body->GetWorldSpaceBounds());
}


