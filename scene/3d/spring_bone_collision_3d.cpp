/**************************************************************************/
/*  spring_bone_collision_3d.cpp                                          */
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

#include "spring_bone_collision_3d.h"

#include "core/config/engine.h"
#include "scene/3d/spring_bone_simulator_3d.h"

SpringBoneCollision3D::SpringBoneCollision3D(ShapeType p_shape_type) :
		shape_type(p_shape_type) {
}

void SpringBoneCollision3D::set_shape_type(ShapeType p_shape_type) {
	shape_type = p_shape_type;
}

void SpringBoneCollision3D::set_radius(float p_radius) {
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		height = radius * 2.0f;
	}
}

void SpringBoneCollision3D::set_height(float p_height) {
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
}

void SpringBoneCollision3D::set_mid_height(real_t p_mid_height) {
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "SpringBoneCollision3D capsule mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
}

void SpringBoneCollision3D::set_inside(bool p_enabled) {
	inside = p_enabled;
}

Pair<Vector3, Vector3> SpringBoneCollision3D::get_head_and_tail(const Transform3D &p_center) const {
	Transform3D tr = get_transform_from_skeleton(p_center);
	return Pair<Vector3, Vector3>(
			tr.origin + tr.basis.xform(Vector3::UP * (height * 0.5f - radius)),
			tr.origin + tr.basis.xform(Vector3::DOWN * (height * 0.5f - radius)));
}

void SpringBoneCollision3D::_validate_bone_name() {
	if (!bone_name.is_empty()) {
		set_bone_name(bone_name);
	} else if (bone != -1) {
		set_bone(bone);
	}
}

Skeleton3D *SpringBoneCollision3D::get_skeleton() const {
	SpringBoneSimulator3D *sim = dynamic_cast<SpringBoneSimulator3D *>(get_parent());
	if (sim) {
		return sim->get_skeleton();
	}
	return nullptr;
}

void SpringBoneCollision3D::set_bone_name(const String &p_name) {
	bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_bone(sk->find_bone(bone_name));
	}
}

String SpringBoneCollision3D::get_bone_name() const {
	return bone_name;
}

void SpringBoneCollision3D::set_bone(int p_bone) {
	bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && bone >= 0 && bone < sk->get_bone_count()) {
		bone_name = sk->get_bone_name(bone);
	}
	sync_pose();
}

int SpringBoneCollision3D::get_bone() const {
	return bone;
}

void SpringBoneCollision3D::set_position_offset(const Vector3 &p_offset) {
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	sync_pose();
}

Vector3 SpringBoneCollision3D::get_position_offset() const {
	return position_offset;
}

void SpringBoneCollision3D::set_rotation_offset(const Quaternion &p_offset) {
	if (rotation_offset == p_offset) {
		return;
	}
	rotation_offset = p_offset;
	sync_pose();
}

Quaternion SpringBoneCollision3D::get_rotation_offset() const {
	return rotation_offset;
}

void SpringBoneCollision3D::sync_pose() {
	if (bone >= 0) {
		Skeleton3D *sk = get_skeleton();
		if (sk) {
			Transform3D tr = sk->get_global_transform() * sk->get_bone_global_pose(bone);
			tr.origin += tr.basis.get_rotation_quaternion().xform(position_offset);
			tr.basis *= Basis(rotation_offset);
			set_global_transform(tr);
		}
	}
}

Transform3D SpringBoneCollision3D::get_transform_from_skeleton(const Transform3D &p_center) const {
	Transform3D gtr = get_global_transform();
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		Transform3D tr = sk->get_global_transform();
		gtr = tr.affine_inverse() * p_center * gtr;
	}
	return gtr;
}

void SpringBoneCollision3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_PARENTED: {
			_validate_bone_name();
		} break;
	}
}

Vector3 SpringBoneCollision3D::_collide_sphere(const Vector3 &p_origin, float p_radius, bool p_inside, float p_bone_radius, float p_bone_length, const Vector3 &p_current) {
	Vector3 diff = p_current - p_origin;
	float length = diff.length();
	float r = p_inside ? p_radius - p_bone_radius : p_bone_radius + p_radius;
	float distance = p_inside ? r - length : length - r;
	if (distance > 0) {
		return p_current;
	}
	return p_origin + diff.normalized() * r;
}

Vector3 SpringBoneCollision3D::collide(const Transform3D &p_center, float p_bone_radius, float p_bone_length, const Vector3 &p_current) const {
	switch (shape_type) {
		case SHAPE_SPHERE: {
			return _collide_sphere(get_transform_from_skeleton(p_center).origin, radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_CAPSULE: {
			Pair<Vector3, Vector3> head_tail = get_head_and_tail(p_center);
			Vector3 head = head_tail.first;
			Vector3 tail = head_tail.second;
			Vector3 p = tail - head;
			Vector3 q = p_current - head;
			float dot = p.dot(q);
			if (dot <= 0) {
				return _collide_sphere(head, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			float pls = p.length_squared();
			if (Math::is_zero_approx(pls)) {
				return p_current;
			}
			if (pls <= dot) {
				return _collide_sphere(head + p, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			return _collide_sphere(head + p * (dot / pls), radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_PLANE: {
			Transform3D tr = get_transform_from_skeleton(p_center);
			Vector3 pos = tr.origin;
			Vector3 normal = tr.basis.get_rotation_quaternion().xform(Vector3::UP);
			Vector3 to_vec = p_current - pos;
			float distance = to_vec.dot(normal) - p_bone_radius;
			if (distance > 0) {
				return p_current;
			}
			return p_current + normal * -distance;
		}
	}
	return p_current;
}

PackedStringArray SpringBoneCollision3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!dynamic_cast<SpringBoneSimulator3D *>(get_parent())) {
		warnings.push_back(RTR("SpringBoneCollision3D must be a child of SpringBoneSimulator3D."));
	}
	return warnings;
}
