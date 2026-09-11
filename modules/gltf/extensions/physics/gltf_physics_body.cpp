/**************************************************************************/
/*  gltf_physics_body.cpp                                                 */
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

#include "gltf_physics_body.h"
#include "scene/3d/physics/animatable_body_3d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/physics/vehicle_body_3d.h"

void GLTFPhysicsBody::_bind_methods() {}

String GLTFPhysicsBody::get_body_type() const
{
	switch (body_type) {
	case PhysicsBodyType::STATIC:
		return "static";
	case PhysicsBodyType::ANIMATABLE:
		return "animatable";
	case PhysicsBodyType::CHARACTER:
		return "character";
	case PhysicsBodyType::RIGID:
		return "rigid";
	case PhysicsBodyType::VEHICLE:
		return "vehicle";
	case PhysicsBodyType::TRIGGER:
		return "trigger";
	}
	// Unreachable, the switch cases handle all values the enum can take.
	// Omitting this works on Clang but not GCC or MSVC. If reached, it's UB.
	return "rigid";
}

void GLTFPhysicsBody::set_body_type(const String& p_body_type)
{
	if (p_body_type == "static") {
		body_type = PhysicsBodyType::STATIC;
	}
	else if (p_body_type == "animatable") {
		body_type = PhysicsBodyType::ANIMATABLE;
	}
	else if (p_body_type == "character") {
		body_type = PhysicsBodyType::CHARACTER;
	}
	else if (p_body_type == "rigid") {
		body_type = PhysicsBodyType::RIGID;
	}
	else if (p_body_type == "vehicle") {
		body_type = PhysicsBodyType::VEHICLE;
	}
	else if (p_body_type == "trigger") {
		body_type = PhysicsBodyType::TRIGGER;
	}
	else {
		ERR_PRINT("Error setting glTF physics body type: The body type must be one of \"static\", "
				  "\"animatable\", \"character\", \"rigid\", \"vehicle\", or \"trigger\".");
	}
}

GLTFPhysicsBody::PhysicsBodyType GLTFPhysicsBody::get_physics_body_type() const
{
	return body_type;
}

void GLTFPhysicsBody::set_physics_body_type(PhysicsBodyType p_body_type)
{
	body_type = p_body_type;
}

real_t GLTFPhysicsBody::get_mass() const { return mass; }

void GLTFPhysicsBody::set_mass(real_t p_mass) { mass = p_mass; }

Vector3 GLTFPhysicsBody::get_linear_velocity() const { return linear_velocity; }

void GLTFPhysicsBody::set_linear_velocity(const Vector3& p_linear_velocity)
{
	linear_velocity = p_linear_velocity;
}

Vector3 GLTFPhysicsBody::get_angular_velocity() const { return angular_velocity; }

void GLTFPhysicsBody::set_angular_velocity(const Vector3& p_angular_velocity)
{
	angular_velocity = p_angular_velocity;
}

Vector3 GLTFPhysicsBody::get_center_of_mass() const { return center_of_mass; }

void GLTFPhysicsBody::set_center_of_mass(const Vector3& p_center_of_mass)
{
	center_of_mass = p_center_of_mass;
}

Vector3 GLTFPhysicsBody::get_inertia_diagonal() const { return inertia_diagonal; }

void GLTFPhysicsBody::set_inertia_diagonal(const Vector3& p_inertia_diagonal)
{
	inertia_diagonal = p_inertia_diagonal;
}

Quaternion GLTFPhysicsBody::get_inertia_orientation() const { return inertia_orientation; }

void GLTFPhysicsBody::set_inertia_orientation(const Quaternion& p_inertia_orientation)
{
	inertia_orientation = p_inertia_orientation;
}

#ifndef DISABLE_DEPRECATED
Basis GLTFPhysicsBody::get_inertia_tensor() const { return Basis::from_scale(inertia_diagonal); }

void GLTFPhysicsBody::set_inertia_tensor(const Basis& p_inertia_tensor)
{
	inertia_diagonal = p_inertia_tensor.get_main_diagonal();
}
#endif // DISABLE_DEPRECATED

CollisionObject3D* GLTFPhysicsBody::to_node() const
{
	switch (body_type) {
	case PhysicsBodyType::CHARACTER: {
		CharacterBody3D* body = memnew(CharacterBody3D);
		return body;
	}
	case PhysicsBodyType::ANIMATABLE: {
		AnimatableBody3D* body = memnew(AnimatableBody3D);
		return body;
	}
	case PhysicsBodyType::VEHICLE: {
		VehicleBody3D* body = memnew(VehicleBody3D);
		body->set_mass(mass);
		body->set_linear_velocity(linear_velocity);
		body->set_angular_velocity(angular_velocity);
		body->set_inertia(inertia_diagonal);
		body->set_center_of_mass_mode(RigidBody3D::CENTER_OF_MASS_MODE_CUSTOM);
		body->set_center_of_mass(center_of_mass);
		return body;
	}
	case PhysicsBodyType::RIGID: {
		RigidBody3D* body = memnew(RigidBody3D);
		body->set_mass(mass);
		body->set_linear_velocity(linear_velocity);
		body->set_angular_velocity(angular_velocity);
		body->set_inertia(inertia_diagonal);
		body->set_center_of_mass_mode(RigidBody3D::CENTER_OF_MASS_MODE_CUSTOM);
		body->set_center_of_mass(center_of_mass);
		return body;
	}
	case PhysicsBodyType::STATIC: {
		StaticBody3D* body = memnew(StaticBody3D);
		return body;
	}
	case PhysicsBodyType::TRIGGER: {
		Area3D* body = memnew(Area3D);
		return body;
	}
	}
	// Unreachable, the switch cases handle all values the enum can take.
	// Omitting this works on Clang but not GCC or MSVC. If reached, it's UB.
	return nullptr;
}


