/**************************************************************************/
/*  godot_constraints_2d.cpp                                              */
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

#include "godot_constraints_2d.h"

#include "godot_collision_solver_2d.h"
#include "godot_space_2d.h"

// ============================================================================
// GodotAreaPair2D / GodotArea2Pair2D
// ============================================================================

void GodotAreaPair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotAreaPair2D::GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape) {
	body = p_body;
	area = p_area;
	body_shape = p_body_shape;
	area_shape = p_area_shape;
	body->add_constraint(this, 0);
	area->add_constraint(this);
	if (p_body->get_mode() == PS2DE::BODY_MODE_KINEMATIC) { // need to be active to process pair
		p_body->set_active(true);
	}
}

bool GodotAreaPair2D::setup(real_t p_step) {
	return false;
}

bool GodotAreaPair2D::pre_solve(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::setup(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::pre_solve(real_t p_step) {
	if (process_collision_a) {
		if (colliding_a) {
			area_a->add_area_to_query(area_b, shape_b, shape_a);
		} else {
			area_a->remove_area_from_query(area_b, shape_b, shape_a);
		}
	}

	if (process_collision_b) {
		if (colliding_b) {
			area_b->add_area_to_query(area_a, shape_a, shape_b);
		} else {
			area_b->remove_area_from_query(area_a, shape_a, shape_b);
		}
	}

	return false; // Never do any post solving.
}

void GodotArea2Pair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotArea2Pair2D::GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b) {
	area_a = p_area_a;
	area_b = p_area_b;
	shape_a = p_shape_a;
	shape_b = p_shape_b;
	area_a_monitorable = area_a->is_monitorable();
	area_b_monitorable = area_b->is_monitorable();
	area_a->add_constraint(this);
	area_b->add_constraint(this);
}

// ============================================================================
// GodotBodyPair2D
// ============================================================================

#define ACCUMULATE_IMPULSES

#define MIN_VELOCITY 0.001
#define MAX_BIAS_ROTATION (Math::PI / 8)

void GodotBodyPair2D::_add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self) {
	GodotBodyPair2D *self = static_cast<GodotBodyPair2D *>(p_self);

	self->_contact_added_callback(p_point_A, p_point_B);
}

void GodotBodyPair2D::_contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B) {
	Vector2 local_A = A->get_inv_transform().basis_xform(p_point_A);
	Vector2 local_B = B->get_inv_transform().basis_xform(p_point_B - offset_B);

	int new_index = contact_count;

	ERR_FAIL_COND(new_index >= (MAX_CONTACTS + 1));

	Contact contact;
	contact.local_A = local_A;
	contact.local_B = local_B;
	contact.normal = (p_point_A - p_point_B).normalized();
	contact.used = true;

	// Attempt to determine if the contact will be reused.
	real_t recycle_radius_2 = space->get_contact_recycle_radius() * space->get_contact_recycle_radius();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		if (c.local_A.distance_squared_to(local_A) < (recycle_radius_2) &&
				c.local_B.distance_squared_to(local_B) < (recycle_radius_2)) {
			contact.acc_normal_impulse = c.acc_normal_impulse;
			contact.acc_tangent_impulse = c.acc_tangent_impulse;
			contact.acc_bias_impulse = c.acc_bias_impulse;
			contact.acc_bias_impulse_center_of_mass = c.acc_bias_impulse_center_of_mass;
			c = contact;
			return;
		}
	}

	// Figure out if the contact amount must be reduced to fit the new contact.
	if (new_index == MAX_CONTACTS) {
		// Remove the contact with the minimum depth.
		const Transform2D &transform_A = A->get_transform();
		const Transform2D &transform_B = B->get_transform();

		int least_deep = -1;
		real_t min_depth;

		// Start with depth for new contact.
		{
			Vector2 global_A = transform_A.basis_xform(contact.local_A);
			Vector2 global_B = transform_B.basis_xform(contact.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			min_depth = axis.dot(contact.normal);
		}

		for (int i = 0; i < contact_count; i++) {
			const Contact &c = contacts[i];
			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < min_depth) {
				min_depth = depth;
				least_deep = i;
			}
		}

		if (least_deep > -1) {
			// Replace the least deep contact by the new one.
			contacts[least_deep] = contact;
		}

		return;
	}

	contacts[new_index] = contact;
	contact_count++;
}

void GodotBodyPair2D::_validate_contacts() {
	// Make sure to erase contacts that are no longer valid.
	real_t max_separation = space->get_contact_max_separation();
	real_t max_separation2 = max_separation * max_separation;

	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];

		bool erase = false;
		if (!c.used) {
			// Was left behind in previous frame.
			erase = true;
		} else {
			c.used = false;

			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;
			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < -max_separation || (global_B + c.normal * depth - global_A).length_squared() > max_separation2) {
				erase = true;
			}
		}

		if (erase) {
			// Contact no longer needed, remove.
			if ((i + 1) < contact_count) {
				// Swap with the last one.
				SWAP(contacts[i], contacts[contact_count - 1]);
			}

			i--;
			contact_count--;
		}
	}
}

bool GodotBodyPair2D::_test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B) {
	Vector2 motion = p_A->get_linear_velocity() * p_step;
	real_t mlen = motion.length();
	if (mlen < CMP_EPSILON) {
		return false;
	}

	Vector2 mnormal = motion / mlen;

	real_t min = 0.0, max = 0.0;
	p_A->get_shape(p_shape_A)->project_rangev(mnormal, p_xform_A, min, max);

	bool fast_object = mlen > (max - min) * 0.3;
	if (!fast_object) {
		return false;
	}

	Transform2D predicted_xform_B = p_xform_B.translated(p_B->get_linear_velocity() * p_step);

	int a;
	Vector2 s[2];
	p_A->get_shape(p_shape_A)->get_supports(p_xform_A.basis_xform_inv(mnormal).normalized(), s, a);
	Vector2 from = p_xform_A.xform(s[0]);
	Vector2 to = from + motion;

	Transform2D from_inv = predicted_xform_B.affine_inverse();

	Vector2 local_from = from_inv.xform(from - motion * 0.1);
	Vector2 local_to = from_inv.xform(to);

	Vector2 rpos, rnorm;
	if (!p_B->get_shape(p_shape_B)->intersect_segment(local_from, local_to, rpos, rnorm)) {
		return false;
	}

	// Check one-way collision based on motion direction.
	if (p_A->get_shape(p_shape_A)->allows_one_way_collision() && p_B->is_shape_set_as_one_way_collision(p_shape_B)) {
		Vector2 direction = predicted_xform_B.basis_xform(p_B->get_shape_one_way_collision_direction(p_shape_B)).normalized();

		if (direction.dot(mnormal) < CMP_EPSILON) {
			collided = false;
			oneway_disabled = true;
			return false;
		}
	}

	// Shorten the linear velocity so it does not hit, but gets close enough,
	// next frame will hit softly or soft enough.
	Vector2 hitpos = predicted_xform_B.xform(rpos);

	real_t newlen = hitpos.distance_to(from) + (max - min) * 0.01;
	p_A->set_linear_velocity(mnormal * (newlen / p_step));

	return true;
}

static real_t combine_bounce(GodotBody2D *A, GodotBody2D *B) {
	return CLAMP(A->get_bounce() + B->get_bounce(), 0, 1);
}

static real_t combine_friction(GodotBody2D *A, GodotBody2D *B) {
	return Math::abs(MIN(A->get_friction(), B->get_friction()));
}

bool GodotBodyPair2D::setup(real_t p_step) {
	check_ccd = false;

	if (!A->interacts_with(B) || A->has_exception(B->get_self()) || B->has_exception(A->get_self())) {
		collided = false;
		return false;
	}

	collide_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && A->collides_with(B);
	collide_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && B->collides_with(A);

	report_contacts_only = false;
	if (!collide_A && !collide_B) {
		if ((A->get_max_contacts_reported() > 0) || (B->get_max_contacts_reported() > 0)) {
			report_contacts_only = true;
		} else {
			collided = false;
			return false;
		}
	}

	// use local A coordinates to avoid numerical issues on collision detection
	offset_B = B->get_transform().get_origin() - A->get_transform().get_origin();

	_validate_contacts();

	const Vector2 &offset_A = A->get_transform().get_origin();
	Transform2D xform_Au = A->get_transform().untranslated();
	Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

	Transform2D xform_Bu = B->get_transform();
	xform_Bu.columns[2] -= offset_A;
	Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	Vector2 motion_A, motion_B;

	if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_A = A->get_motion();
	}
	if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_B = B->get_motion();
	}

	bool prev_collided = collided;

	collided = GodotCollisionSolver2D::solve(shape_A_ptr, xform_A, motion_A, shape_B_ptr, xform_B, motion_B, _add_contact, this, &sep_axis);
	if (!collided) {
		oneway_disabled = false;

		if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
			check_ccd = true;
			return true;
		}

		if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
			check_ccd = true;
			return true;
		}

		return false;
	}

	if (oneway_disabled) {
		return false;
	}

	if (!prev_collided) {
		if (shape_B_ptr->allows_one_way_collision() && A->is_shape_set_as_one_way_collision(shape_A)) {
			Vector2 direction = xform_A.basis_xform(A->get_shape_one_way_collision_direction(shape_A)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) > -CMP_EPSILON) { // Greater (normal inverted).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}

		if (shape_A_ptr->allows_one_way_collision() && B->is_shape_set_as_one_way_collision(shape_B)) {
			Vector2 direction = xform_B.basis_xform(B->get_shape_one_way_collision_direction(shape_B)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) < CMP_EPSILON) { // Less (normal ok).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}
	}

	return true;
}

bool GodotBodyPair2D::pre_solve(real_t p_step) {
	if (oneway_disabled) {
		return false;
	}

	if (!collided) {
		if (check_ccd) {
			const Vector2 &offset_A = A->get_transform().get_origin();
			Transform2D xform_Au = A->get_transform().untranslated();
			Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

			Transform2D xform_Bu = B->get_transform();
			xform_Bu.columns[2] -= offset_A;
			Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

			if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
				_test_ccd(p_step, A, shape_A, xform_A, B, shape_B, xform_B);
			}

			if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
				_test_ccd(p_step, B, shape_B, xform_B, A, shape_A, xform_A);
			}
		}

		return false;
	}

	real_t max_penetration = space->get_contact_max_allowed_penetration();
	real_t bias = space->get_contact_bias();

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	if (shape_A_ptr->get_custom_bias() || shape_B_ptr->get_custom_bias()) {
		if (shape_A_ptr->get_custom_bias() == 0) {
			bias = shape_B_ptr->get_custom_bias();
		} else if (shape_B_ptr->get_custom_bias() == 0) {
			bias = shape_A_ptr->get_custom_bias();
		} else {
			bias = (shape_B_ptr->get_custom_bias() + shape_A_ptr->get_custom_bias()) * 0.5;
		}
	}

	real_t inv_dt = 1.0 / p_step;

	bool do_process = false;

	const Vector2 &offset_A = A->get_transform().get_origin();
	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	real_t inv_inertia_A = collide_A ? A->get_inv_inertia() : 0.0;
	real_t inv_inertia_B = collide_B ? B->get_inv_inertia() : 0.0;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		c.active = false;

		Vector2 global_A = transform_A.basis_xform(c.local_A);
		Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

		Vector2 axis = global_A - global_B;
		real_t depth = axis.dot(c.normal);

		if (depth <= 0.0) {
			continue;
		}

#ifdef DEBUG_ENABLED
		if (space->is_debugging_contacts()) {
			space->add_debug_contact(global_A + offset_A);
			space->add_debug_contact(global_B + offset_A);
		}
#endif

		c.rA = global_A - A->get_center_of_mass();
		c.rB = global_B - B->get_center_of_mass() - offset_B;

		// Precompute normal mass, tangent mass, and bias.
		real_t rnA = c.rA.dot(c.normal);
		real_t rnB = c.rB.dot(c.normal);
		real_t kNormal = inv_mass_A + inv_mass_B;
		kNormal += inv_inertia_A * (c.rA.dot(c.rA) - rnA * rnA) + inv_inertia_B * (c.rB.dot(c.rB) - rnB * rnB);
		c.mass_normal = 1.0f / kNormal;

		Vector2 tangent = c.normal.orthogonal();
		real_t rtA = c.rA.dot(tangent);
		real_t rtB = c.rB.dot(tangent);
		real_t kTangent = inv_mass_A + inv_mass_B;
		kTangent += inv_inertia_A * (c.rA.dot(c.rA) - rtA * rtA) + inv_inertia_B * (c.rB.dot(c.rB) - rtB * rtB);
		c.mass_tangent = 1.0f / kTangent;

		c.bias = -bias * inv_dt * MIN(0.0f, -depth + max_penetration);
		c.depth = depth;

		Vector2 P = c.acc_normal_impulse * c.normal + c.acc_tangent_impulse * tangent;

		c.acc_impulse -= P;

		if (report_contacts_only) {
			collided = false;
			continue;
		}

#ifdef ACCUMULATE_IMPULSES
		{
			// Apply normal + friction impulse
			if (collide_A) {
				A->apply_impulse(-P, c.rA + A->get_center_of_mass());
			}
			if (collide_B) {
				B->apply_impulse(P, c.rB + B->get_center_of_mass());
			}
		}
#endif

		c.bounce = combine_bounce(A, B);
		if (c.bounce) {
			Vector2 crA(-A->get_prev_angular_velocity() * c.rA.y, A->get_prev_angular_velocity() * c.rA.x);
			Vector2 crB(-B->get_prev_angular_velocity() * c.rB.y, B->get_prev_angular_velocity() * c.rB.x);
			Vector2 dv = B->get_prev_linear_velocity() + crB - A->get_prev_linear_velocity() - crA;
			c.bounce = c.bounce * dv.dot(c.normal);
		}

		c.active = true;
		do_process = true;
	}

	return do_process;
}

void GodotBodyPair2D::solve(real_t p_step) {
	if (!collided || oneway_disabled) {
		return;
	}

	const real_t max_bias_av = MAX_BIAS_ROTATION / p_step;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; ++i) {
		Contact &c = contacts[i];

		if (!c.active) {
			continue;
		}

		// Relative velocity at contact
		Vector2 crA(-A->get_angular_velocity() * c.rA.y, A->get_angular_velocity() * c.rA.x);
		Vector2 crB(-B->get_angular_velocity() * c.rB.y, B->get_angular_velocity() * c.rB.x);
		Vector2 dv = B->get_linear_velocity() + crB - A->get_linear_velocity() - crA;

		Vector2 crbA(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		Vector2 crbB(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		Vector2 dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		real_t vn = dv.dot(c.normal);
		real_t vbn = dbv.dot(c.normal);

		Vector2 tangent = c.normal.orthogonal();
		real_t vt = dv.dot(tangent);

		real_t jbn = (c.bias - vbn) * c.mass_normal;
		real_t jbnOld = c.acc_bias_impulse;
		c.acc_bias_impulse = MAX(jbnOld + jbn, 0.0f);

		Vector2 jb = c.normal * (c.acc_bias_impulse - jbnOld);

		if (collide_A) {
			A->apply_bias_impulse(-jb, c.rA + A->get_center_of_mass(), max_bias_av);
		}
		if (collide_B) {
			B->apply_bias_impulse(jb, c.rB + B->get_center_of_mass(), max_bias_av);
		}

		crbA = Vector2(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		crbB = Vector2(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		vbn = dbv.dot(c.normal);

		if (Math::abs(-vbn + c.bias) > MIN_VELOCITY) {
			real_t jbn_com = (-vbn + c.bias) / (inv_mass_A + inv_mass_B);
			real_t jbnOld_com = c.acc_bias_impulse_center_of_mass;
			c.acc_bias_impulse_center_of_mass = MAX(jbnOld_com + jbn_com, 0.0f);

			Vector2 jb_com = c.normal * (c.acc_bias_impulse_center_of_mass - jbnOld_com);

			if (collide_A) {
				A->apply_bias_impulse(-jb_com, A->get_center_of_mass(), 0.0f);
			}
			if (collide_B) {
				B->apply_bias_impulse(jb_com, B->get_center_of_mass(), 0.0f);
			}
		}

		real_t jn = -(c.bounce + vn) * c.mass_normal;
		real_t jnOld = c.acc_normal_impulse;
		c.acc_normal_impulse = MAX(jnOld + jn, 0.0f);

		real_t friction = combine_friction(A, B);

		real_t jtMax = friction * c.acc_normal_impulse;
		real_t jt = -vt * c.mass_tangent;
		real_t jtOld = c.acc_tangent_impulse;
		c.acc_tangent_impulse = CLAMP(jtOld + jt, -jtMax, jtMax);

		Vector2 j = c.normal * (c.acc_normal_impulse - jnOld) + tangent * (c.acc_tangent_impulse - jtOld);

		if (collide_A) {
			A->apply_impulse(-j, c.rA + A->get_center_of_mass());
		}
		if (collide_B) {
			B->apply_impulse(j, c.rB + B->get_center_of_mass());
		}
		c.acc_impulse -= j;
	}
}

GodotBodyPair2D::GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B) :
		GodotConstraint2D(_arr, 2) {
	A = p_A;
	B = p_B;
	shape_A = p_shape_A;
	shape_B = p_shape_B;
	space = A->get_space();
	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

GodotBodyPair2D::~GodotBodyPair2D() {
	A->remove_constraint(this, 0);
	B->remove_constraint(this, 1);
}

// ============================================================================
// GodotJoint2D (based on Chipmunk joint constraints)
// ============================================================================

/* Copyright (c) 2007 Scott Lembcke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

void GodotJoint2D::copy_settings_from(GodotJoint2D *p_joint) {
	set_self(p_joint->get_self());
	set_max_force(p_joint->get_max_force());
	set_bias(p_joint->get_bias());
	set_max_bias(p_joint->get_max_bias());
	disable_collisions_between_bodies(p_joint->is_disabled_collisions_between_bodies());
}

static inline real_t k_scalar(GodotBody2D *a, GodotBody2D *b, const Vector2 &rA, const Vector2 &rB, const Vector2 &n) {
	real_t value = 0.0;

	{
		value += a->get_inv_mass();
		real_t rcn = (rA - a->get_center_of_mass()).cross(n);
		value += a->get_inv_inertia() * rcn * rcn;
	}

	if (b) {
		value += b->get_inv_mass();
		real_t rcn = (rB - b->get_center_of_mass()).cross(n);
		value += b->get_inv_inertia() * rcn * rcn;
	}

	return value;
}

static inline Vector2 relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB) {
	Vector2 sum = a->get_linear_velocity() - (rA - a->get_center_of_mass()).orthogonal() * a->get_angular_velocity();
	if (b) {
		return (b->get_linear_velocity() - (rB - b->get_center_of_mass()).orthogonal() * b->get_angular_velocity()) - sum;
	} else {
		return -sum;
	}
}

static inline real_t normal_relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB, Vector2 n) {
	return relative_velocity(a, b, rA, rB).dot(n);
}

static inline Vector2 custom_cross(const Vector2 &p_vec, real_t p_other) {
	return Vector2(p_other * p_vec.y, -p_other * p_vec.x);
}

static inline void k_tensor(GodotBody2D *a, GodotBody2D *b, Vector2 r1, Vector2 r2, Vector2 *k1, Vector2 *k2) {
	real_t k11, k12, k21, k22;
	real_t m_sum = a->get_inv_mass() + b->get_inv_mass();

	// start with I*m_sum
	k11 = m_sum;
	k12 = 0.0f;
	k21 = 0.0f;
	k22 = m_sum;

	r1 -= a->get_center_of_mass();
	r2 -= b->get_center_of_mass();

	// add the influence from r1
	real_t a_i_inv = a->get_inv_inertia();
	real_t r1xsq = r1.x * r1.x * a_i_inv;
	real_t r1ysq = r1.y * r1.y * a_i_inv;
	real_t r1nxy = -r1.x * r1.y * a_i_inv;
	k11 += r1ysq;
	k12 += r1nxy;
	k21 += r1nxy;
	k22 += r1xsq;

	// add the influence from r2
	real_t b_i_inv = b->get_inv_inertia();
	real_t r2xsq = r2.x * r2.x * b_i_inv;
	real_t r2ysq = r2.y * r2.y * b_i_inv;
	real_t r2nxy = -r2.x * r2.y * b_i_inv;
	k11 += r2ysq;
	k12 += r2nxy;
	k21 += r2nxy;
	k22 += r2xsq;

	// invert
	real_t determinant = k11 * k22 - k12 * k21;
	ERR_FAIL_COND(determinant == 0.0);

	real_t det_inv = 1.0f / determinant;
	*k1 = Vector2(k22 * det_inv, -k12 * det_inv);
	*k2 = Vector2(-k21 * det_inv, k11 * det_inv);
}

static _FORCE_INLINE_ Vector2 mult_k(const Vector2 &vr, const Vector2 &k1, const Vector2 &k2) {
	return Vector2(vr.dot(k1), vr.dot(k2));
}

bool GodotJoint2D::setup(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _setup_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _setup_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _setup_damped_spring(p_step);
		default:
			return false;
	}
}

bool GodotJoint2D::pre_solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _pre_solve_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _pre_solve_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _pre_solve_damped_spring(p_step);
		default:
			return false;
	}
}

void GodotJoint2D::solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			_solve_pin(p_step);
			break;
		case PS2DE::JOINT_TYPE_GROOVE:
			_solve_groove(p_step);
			break;
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			_solve_damped_spring(p_step);
			break;
		default:
			break;
	}
}

// --- Pin Joint ---

bool GodotJoint2D::_setup_pin(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B && B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B ? B->get_transform().basis_xform(anchor_B) : anchor_B;

	real_t B_inv_mass = B ? B->get_inv_mass() : 0.0;

	Transform2D K1;
	K1[0].x = A->get_inv_mass() + B_inv_mass;
	K1[1].x = 0.0f;
	K1[0].y = 0.0f;
	K1[1].y = A->get_inv_mass() + B_inv_mass;

	Vector2 r1 = rA - A->get_center_of_mass();

	Transform2D K2;
	K2[0].x = A->get_inv_inertia() * r1.y * r1.y;
	K2[1].x = -A->get_inv_inertia() * r1.x * r1.y;
	K2[0].y = -A->get_inv_inertia() * r1.x * r1.y;
	K2[1].y = A->get_inv_inertia() * r1.x * r1.x;

	Transform2D K;
	K[0] = K1[0] + K2[0];
	K[1] = K1[1] + K2[1];

	if (B) {
		Vector2 r2 = rB - B->get_center_of_mass();

		Transform2D K3;
		K3[0].x = B->get_inv_inertia() * r2.y * r2.y;
		K3[1].x = -B->get_inv_inertia() * r2.x * r2.y;
		K3[0].y = -B->get_inv_inertia() * r2.x * r2.y;
		K3[1].y = B->get_inv_inertia() * r2.x * r2.x;

		K[0] += K3[0];
		K[1] += K3[1];
	}

	K[0].x += softness;
	K[1].y += softness;

	M = K.affine_inverse();

	Vector2 gA = rA + A->get_transform().get_origin();
	Vector2 gB = B ? rB + B->get_transform().get_origin() : rB;

	Vector2 delta = gB - gA;

	pin_bias = delta * -(get_bias() == 0 ? space->get_constraint_bias() : get_bias()) * (1.0 / p_step);

	// Compute max impulse.
	jn_max = get_max_force() * p_step;

	return true;
}

bool GodotJoint2D::_pre_solve_pin(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-P, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(P, rB);
	}
	real_t i_sum_local = A->get_inv_inertia();
	if (B) {
		i_sum_local += B->get_inv_inertia();
	}
	i_sum = 1.0 / (i_sum_local);
	if (angular_limit_enabled && B) {
		Vector2 diff_vector = B->get_transform().get_origin() - A->get_transform().get_origin();
		diff_vector = diff_vector.rotated(-initial_angle);
		real_t dist = diff_vector.angle();
		real_t pdist = 0.0;
		if (dist > angular_limit_upper) {
			pdist = dist - angular_limit_upper;
		} else if (dist < angular_limit_lower) {
			pdist = dist - angular_limit_lower;
		}
		real_t error_bias = Math::pow(1.0 - 0.15, 60.0);
		// Calculate bias velocity.
		bias_velocity = -CLAMP((-1.0 - Math::pow(error_bias, p_step)) * pdist / p_step, -get_max_bias(), get_max_bias());
		// If the bias velocity is 0, the joint is not at a limit.
		if (bias_velocity >= -CMP_EPSILON && bias_velocity <= CMP_EPSILON) {
			j_acc = 0;
			is_joint_at_limit = false;
		} else {
			is_joint_at_limit = true;
		}
	} else {
		bias_velocity = 0.0;
	}

	return true;
}

void GodotJoint2D::_solve_pin(real_t p_step) {
	// Compute relative velocity.
	Vector2 vA = A->get_linear_velocity() - custom_cross(rA - A->get_center_of_mass(), A->get_angular_velocity());

	Vector2 rel_vel;
	if (B) {
		rel_vel = B->get_linear_velocity() - custom_cross(rB - B->get_center_of_mass(), B->get_angular_velocity()) - vA;
	} else {
		rel_vel = -vA;
	}
	if ((angular_limit_enabled || motor_enabled) && B) {
		// Compute relative rotational velocity.
		real_t wr = B->get_angular_velocity() - A->get_angular_velocity();
		if (motor_enabled) {
			wr -= motor_target_velocity;
		}
		real_t j_max = jn_max;

		// Compute normal impulse.
		real_t j_pin = -(bias_velocity + wr) * i_sum;
		real_t j_old = j_acc;
		// Only enable the limits if we have to.
		if (angular_limit_enabled && is_joint_at_limit) {
			if (bias_velocity < 0.0) {
				j_acc = CLAMP(j_old + j_pin, 0.0, j_max);
			} else {
				j_acc = CLAMP(j_old + j_pin, -j_max, 0.0);
			}
		} else {
			j_acc = CLAMP(j_old + j_pin, -j_max, j_max);
		}
		j_pin = j_acc - j_old;
		A->apply_torque_impulse(-j_pin * A->get_inv_inertia());
		B->apply_torque_impulse(j_pin * B->get_inv_inertia());
	}

	Vector2 impulse = M.basis_xform(pin_bias - rel_vel - Vector2(softness, softness) * P);

	if (dynamic_A) {
		A->apply_impulse(-impulse, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(impulse, rB);
	}

	P += impulse;
}

void GodotJoint2D::set_param(PS2DE::PinJointParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			softness = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			angular_limit_upper = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			angular_limit_lower = p_value;
		} break;
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			motor_target_velocity = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::PinJointParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, 0);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			return softness;
		}
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			return angular_limit_upper;
		}
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			return angular_limit_lower;
		}
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			return motor_target_velocity;
		}
	}
	ERR_FAIL_V(0);
}

void GodotJoint2D::set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			angular_limit_enabled = p_enabled;
		} break;
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			motor_enabled = p_enabled;
		} break;
	}
}

bool GodotJoint2D::get_flag(PS2DE::PinJointFlag p_flag) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, false);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			return angular_limit_enabled;
		}
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			return motor_enabled;
		}
	}
	ERR_FAIL_V(false);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, p_body_b ? 2 : 1),
		type(PS2DE::JOINT_TYPE_PIN) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = p_body_a->get_inv_transform().xform(p_pos);
	anchor_B = p_body_b ? p_body_b->get_inv_transform().xform(p_pos) : p_pos;

	p_body_a->add_constraint(this, 0);
	if (p_body_b) {
		p_body_b->add_constraint(this, 1);
		initial_angle = A->get_transform().get_origin().angle_to_point(B->get_transform().get_origin());
	}
}

// --- Groove Joint ---

bool GodotJoint2D::_setup_groove(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	// calculate endpoints in worldspace
	Vector2 ta = A->get_transform().xform(A_groove_1);
	Vector2 tb = A->get_transform().xform(A_groove_2);

	// calculate axis
	Vector2 axis_n = -(tb - ta).orthogonal().normalized();
	real_t d = ta.dot(axis_n);

	xf_normal = axis_n;
	rB = B->get_transform().basis_xform(B_anchor);

	// calculate tangential distance along the axis of rB
	real_t td = (B->get_transform().get_origin() + rB).cross(axis_n);
	// calculate clamping factor and rB
	if (td <= ta.cross(axis_n)) {
		clamp = 1.0f;
		rA = ta - A->get_transform().get_origin();
	} else if (td >= tb.cross(axis_n)) {
		clamp = -1.0f;
		rA = tb - A->get_transform().get_origin();
	} else {
		clamp = 0.0f;
		rA = ((-axis_n.orthogonal() * -td) + axis_n * d) - A->get_transform().get_origin();
	}

	// Calculate mass tensor
	k_tensor(A, B, rA, rB, &k1, &k2);

	// compute max impulse
	jn_max = get_max_force() * p_step;

	// calculate bias velocity
	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);

	real_t _b = get_bias();
	gbias = (delta * -(_b == 0 ? space->get_constraint_bias() : _b) * (1.0 / p_step)).limit_length(get_max_bias());

	correct = true;
	return true;
}

bool GodotJoint2D::_pre_solve_groove(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-jn_acc, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(jn_acc, rB);
	}

	return true;
}

void GodotJoint2D::_solve_groove(real_t p_step) {
	// compute impulse
	Vector2 vr = relative_velocity(A, B, rA, rB);

	Vector2 j_groove = mult_k(gbias - vr, k1, k2);
	Vector2 jOld = jn_acc;
	j_groove += jOld;

	jn_acc = (((clamp * j_groove.cross(xf_normal)) > 0) ? j_groove : j_groove.project(xf_normal)).limit_length(jn_max);

	j_groove = jn_acc - jOld;

	if (dynamic_A) {
		A->apply_impulse(-j_groove, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_groove, rB);
	}
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_GROOVE) {
	A = p_body_a;
	B = p_body_b;

	A_groove_1 = A->get_inv_transform().xform(p_a_groove1);
	A_groove_2 = A->get_inv_transform().xform(p_a_groove2);
	B_anchor = B->get_inv_transform().xform(p_b_anchor);
	A_groove_normal = -(A_groove_2 - A_groove_1).normalized().orthogonal();

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

// --- Damped Spring Joint ---

bool GodotJoint2D::_setup_damped_spring(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B->get_transform().basis_xform(anchor_B);

	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);
	real_t dist = delta.length();

	if (dist) {
		n = delta / dist;
	} else {
		n = Vector2();
	}

	real_t k = k_scalar(A, B, rA, rB, n);
	n_mass = 1.0f / k;

	target_vrn = 0.0f;
	v_coef = 1.0f - Math::exp(-damping * (p_step)*k);

	// Calculate spring force.
	real_t f_spring = (rest_length - dist) * stiffness;
	j = n * f_spring * (p_step);

	return true;
}

bool GodotJoint2D::_pre_solve_damped_spring(real_t p_step) {
	// Apply spring force.
	if (dynamic_A) {
		A->apply_impulse(-j, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j, rB);
	}

	return true;
}

void GodotJoint2D::_solve_damped_spring(real_t p_step) {
	// compute relative velocity
	real_t vrn = normal_relative_velocity(A, B, rA, rB, n) - target_vrn;

	// compute velocity loss from drag
	real_t v_damp = -vrn * v_coef;
	target_vrn = vrn + v_damp;
	Vector2 j_new = n * v_damp * n_mass;

	if (dynamic_A) {
		A->apply_impulse(-j_new, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_new, rB);
	}
}

void GodotJoint2D::set_param(PS2DE::DampedSpringParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_DAMPED_SPRING);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			rest_length = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			damping = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			stiffness = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::DampedSpringParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			return rest_length;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			return damping;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			return stiffness;
		} break;
	}

	ERR_FAIL_V(0);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_DAMPED_SPRING) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = A->get_inv_transform().xform(p_anchor_a);
	anchor_B = B->get_inv_transform().xform(p_anchor_b);

	rest_length = p_anchor_a.distance_to(p_anchor_b);

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}
