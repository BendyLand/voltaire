/**************************************************************************/
/*  math_transforms.cpp                                                   */
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

#include "core/math/aabb.h"
#include "core/math/basis.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/projection.h"
#include "core/math/quaternion.h"
#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/math/transform_3d.h"
#include "core/math/transform_interpolator.h"
#include "core/string/ustring.h"

// --- quaternion.cpp ---

real_t Quaternion::angle_to(const Quaternion &p_to) const {
	real_t d = dot(p_to);
	// acos does clamping.
	return Math::acos(d * d * 2 - 1);
}

Vector3 Quaternion::get_euler(EulerOrder p_order) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Vector3(0, 0, 0), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Basis(*this).get_euler(p_order);
}

bool Quaternion::is_equal_approx(const Quaternion &p_quaternion) const {
	return Math::is_equal_approx(x, p_quaternion.x) && Math::is_equal_approx(y, p_quaternion.y) && Math::is_equal_approx(z, p_quaternion.z) && Math::is_equal_approx(w, p_quaternion.w);
}

bool Quaternion::is_same(const Quaternion &p_quaternion) const {
	return Math::is_same(x, p_quaternion.x) && Math::is_same(y, p_quaternion.y) && Math::is_same(z, p_quaternion.z) && Math::is_same(w, p_quaternion.w);
}

bool Quaternion::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

real_t Quaternion::length() const {
	return Math::sqrt(length_squared());
}

void Quaternion::normalize() {
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Quaternion cannot be normalized, the elements should be finite.");
	}
#endif // MATH_CHECKS
	*this /= length();
}

Quaternion Quaternion::normalized() const {
	return *this / length();
}

bool Quaternion::is_normalized() const {
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON); //use less epsilon
}

Quaternion Quaternion::inverse() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::log() const {
	Quaternion src = *this;
	Vector3 src_v = src.get_axis() * src.get_angle();
	return Quaternion(src_v.x, src_v.y, src_v.z, 0);
}

Quaternion Quaternion::exp() const {
	Quaternion src = *this;
	Vector3 src_v = Vector3(src.x, src.y, src.z);
	real_t theta = src_v.length();
	src_v = src_v.normalized();
	if (theta < CMP_EPSILON || !src_v.is_normalized()) {
		return Quaternion(0, 0, 0, 1);
	}
	return Quaternion(src_v, theta);
}

Quaternion Quaternion::slerp(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1 = -p_to;
	} else {
		to1 = p_to;
	}

	// calculate coefficients

	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_weight) * omega) / sinom;
		scale1 = Math::sin(p_weight * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_weight;
		scale1 = p_weight;
	}
	// calculate final values
	return Quaternion(
			scale0 * x + scale1 * to1.x,
			scale0 * y + scale1 * to1.y,
			scale0 * z + scale1 * to1.z,
			scale0 * w + scale1 * to1.w);
}

Quaternion Quaternion::slerpni(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	const Quaternion &from = *this;

	real_t dot = from.dot(p_to);

	if (Math::abs(dot) > 0.9999f) {
		return from;
	}

	real_t theta = Math::acos(dot),
		   sinT = 1.0f / Math::sin(theta),
		   newFactor = Math::sin(p_weight * theta) * sinT,
		   invFactor = Math::sin((1.0f - p_weight) * theta) * sinT;

	return Quaternion(invFactor * from.x + newFactor * p_to.x,
			invFactor * from.y + newFactor * p_to.y,
			invFactor * from.z + newFactor * p_to.z,
			invFactor * from.w + newFactor * p_to.w);
}

Quaternion Quaternion::spherical_cubic_interpolate(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion Quaternion::spherical_cubic_interpolate_in_time(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight,
		real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion::operator String() const {
	return "(" + String::num_real(x, false) + ", " + String::num_real(y, false) + ", " + String::num_real(z, false) + ", " + String::num_real(w, false) + ")";
}

Vector3 Quaternion::get_axis() const {
	if (Math::abs(w) > 1 - CMP_EPSILON) {
		return Vector3(x, y, z);
	}
	real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
	return Vector3(x * r, y * r, z * r);
}

real_t Quaternion::get_angle() const {
	return 2 * Math::acos(w);
}

Quaternion::Quaternion(const Vector3 &p_axis, real_t p_angle) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	real_t d = p_axis.length();
	if (d == 0) {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	} else {
		real_t sin_angle = Math::sin(p_angle * 0.5f);
		real_t cos_angle = Math::cos(p_angle * 0.5f);
		real_t s = sin_angle / d;
		x = p_axis.x * s;
		y = p_axis.y * s;
		z = p_axis.z * s;
		w = cos_angle;
	}
}

// Euler constructor expects a vector containing the Euler angles in the format
// (ax, ay, az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Quaternion Quaternion::from_euler(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.y * 0.5f;
	real_t half_a2 = p_euler.x * 0.5f;
	real_t half_a3 = p_euler.z * 0.5f;

	// R = Y(a1).X(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	return Quaternion(
			sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3,
			sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3,
			-sin_a1 * sin_a2 * cos_a3 + cos_a1 * cos_a2 * sin_a3,
			sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3);
}

// --- basis.cpp ---

#define cofac(row1, col1, row2, col2) \
	(rows[row1][col1] * rows[row2][col2] - rows[row1][col2] * rows[row2][col1])

void Basis::invert() {
	real_t co[3] = {
		cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
	};
	real_t det = rows[0][0] * co[0] +
			rows[0][1] * co[1] +
			rows[0][2] * co[2];
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t s = 1.0f / det;

	set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
			co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
			co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

#undef cofac

void Basis::orthonormalize() {
	// Orthonormalizable check is done in Vector3 class below.
	// Gram-Schmidt Process:

	Vector3 x = get_column(0);
	Vector3 y = get_column(1);
	Vector3 z = get_column(2);

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	z.normalize();

	set_column(0, x);
	set_column(1, y);
	set_column(2, z);
}

Basis Basis::orthonormalized() const {
	Basis c = *this;
	c.orthonormalize();
	return c;
}

void Basis::orthogonalize() {
	Vector3 scl = get_scale();
	orthonormalize();
	scale_local(scl);
}

Basis Basis::orthogonalized() const {
	Basis c = *this;
	c.orthogonalize();
	return c;
}

// Returns true if the basis vectors are orthogonal (perpendicular), so it has no skew or shear, and can be decomposed into rotation and scale.
// See https://en.wikipedia.org/wiki/Orthogonal_basis
bool Basis::is_orthogonal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis vectors are orthonormal (orthogonal and normalized), so it has no scale, skew, or shear.
// See https://en.wikipedia.org/wiki/Orthonormal_basis
bool Basis::is_orthonormal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_equal_approx(x.length_squared(), 1) && Math::is_equal_approx(y.length_squared(), 1) && Math::is_equal_approx(z.length_squared(), 1) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis is conformal (orthogonal, uniform scale, preserves angles and distance ratios).
// See https://en.wikipedia.org/wiki/Conformal_linear_transformation
bool Basis::is_conformal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	const real_t x_len_sq = x.length_squared();
	return Math::is_equal_approx(x_len_sq, y.length_squared()) && Math::is_equal_approx(x_len_sq, z.length_squared()) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis only has diagonal elements, so it may only have scale or flip, but no rotation, skew, or shear.
bool Basis::is_diagonal() const {
	return (
			Math::is_zero_approx(rows[0][1]) && Math::is_zero_approx(rows[0][2]) &&
			Math::is_zero_approx(rows[1][0]) && Math::is_zero_approx(rows[1][2]) &&
			Math::is_zero_approx(rows[2][0]) && Math::is_zero_approx(rows[2][1]));
}

// Returns true if the basis is a pure rotation matrix, so it has no scale, skew, shear, or flip.
bool Basis::is_rotation() const {
	return is_conformal() && Math::is_equal_approx(determinant(), 1, (real_t)UNIT_EPSILON);
}

#ifdef MATH_CHECKS
// This method is only used once, in diagonalize. If it's desired elsewhere, feel free to remove the #ifdef.
bool Basis::is_symmetric() const {
	if (!Math::is_equal_approx(rows[0][1], rows[1][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[0][2], rows[2][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[1][2], rows[2][1])) {
		return false;
	}

	return true;
}
#endif

Basis Basis::diagonalize() {
// NOTE: only implemented for symmetric matrices
// with the Jacobi iterative method
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
#endif
	const int ite_max = 1024;

	real_t off_matrix_norm_2 = rows[0][1] * rows[0][1] + rows[0][2] * rows[0][2] + rows[1][2] * rows[1][2];

	int ite = 0;
	Basis acc_rot;
	while (off_matrix_norm_2 > (real_t)CMP_EPSILON2 && ite++ < ite_max) {
		real_t el01_2 = rows[0][1] * rows[0][1];
		real_t el02_2 = rows[0][2] * rows[0][2];
		real_t el12_2 = rows[1][2] * rows[1][2];
		// Find the pivot element
		int i, j;
		if (el01_2 > el02_2) {
			if (el12_2 > el01_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 1;
			}
		} else {
			if (el12_2 > el02_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 2;
			}
		}

		// Compute the rotation angle
		real_t angle;
		if (Math::is_equal_approx(rows[j][j], rows[i][i])) {
			angle = Math::PI / 4;
		} else {
			angle = 0.5f * Math::atan(2 * rows[i][j] / (rows[j][j] - rows[i][i]));
		}

		// Compute the rotation matrix
		Basis rot;
		rot.rows[i][i] = rot.rows[j][j] = Math::cos(angle);
		rot.rows[i][j] = -(rot.rows[j][i] = Math::sin(angle));

		// Update the off matrix norm
		off_matrix_norm_2 -= rows[i][j] * rows[i][j];

		// Apply the rotation
		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
	}

	return acc_rot;
}

Basis Basis::inverse() const {
	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {
	SWAP(rows[0][1], rows[1][0]);
	SWAP(rows[0][2], rows[2][0]);
	SWAP(rows[1][2], rows[2][1]);
}

Basis Basis::transposed() const {
	Basis tr = *this;
	tr.transpose();
	return tr;
}

Basis Basis::from_scale(const Vector3 &p_scale) {
	return Basis(p_scale.x, 0, 0, 0, p_scale.y, 0, 0, 0, p_scale.z);
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3 &p_scale) {
	rows[0] *= p_scale.x;
	rows[1] *= p_scale.y;
	rows[2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale(p_scale);
	return m;
}

void Basis::scale_local(const Vector3 &p_scale) {
	// performs a scaling in object-local coordinate system:
	// M -> (M.S.Minv).M = M.S.
	rows[0] *= p_scale;
	rows[1] *= p_scale;
	rows[2] *= p_scale;
}

void Basis::scale_orthogonal(const Vector3 &p_scale) {
	*this = scaled_orthogonal(p_scale);
}

Basis Basis::scaled_orthogonal(const Vector3 &p_scale) const {
	Basis m = *this;
	Vector3 s = Vector3(-1, -1, -1) + p_scale;
	bool sign = std::signbit(s.x + s.y + s.z);
	Basis b = m.orthonormalized();
	s = b.xform_inv(s);
	Vector3 dots;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			dots[j] += s[i] * Math::abs(m.get_column(i).normalized().dot(b.get_column(j)));
		}
	}
	if (sign != std::signbit(dots.x + dots.y + dots.z)) {
		dots = -dots;
	}
	m.scale_local(Vector3(1, 1, 1) + dots);
	return m;
}

real_t Basis::get_uniform_scale() const {
	return (rows[0].length() + rows[1].length() + rows[2].length()) / 3.0f;
}

Basis Basis::scaled_local(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale_local(p_scale);
	return m;
}

Vector3 Basis::get_scale_abs() const {
	return Vector3(
			Vector3(rows[0][0], rows[1][0], rows[2][0]).length(),
			Vector3(rows[0][1], rows[1][1], rows[2][1]).length(),
			Vector3(rows[0][2], rows[1][2], rows[2][2]).length());
}

Vector3 Basis::get_scale_global() const {
	real_t det_sign = SIGN(determinant());
	return det_sign * Vector3(rows[0].length(), rows[1].length(), rows[2].length());
}

// get_scale works with get_rotation, use get_scale_abs if you need to enforce positive signature.
Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as get_rotation_axis_angle, get_rotation, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	//
	// The rotation part of this decomposition is returned by get_rotation* functions.
	real_t det_sign = SIGN(determinant());
	return det_sign * get_scale_abs();
}

// Decomposes a Basis into a rotation-reflection matrix (an element of the group O(3)) and a positive scaling matrix as B = O.S.
// Returns the rotation-reflection matrix via reference argument, and scaling information is returned as a Vector3.
// This (internal) function is too specific and named too ugly to expose to users, and probably there's no need to do so.
Vector3 Basis::rotref_posscale_decomposition(Basis &r_rotref) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(determinant() == 0, Vector3());

	Basis m = transposed() * (*this);
	ERR_FAIL_COND_V(!m.is_diagonal(), Vector3());
#endif
	Vector3 scale = get_scale();
	Basis inv_scale = Basis().scaled(scale.inverse()); // this will also absorb the sign of scale
	r_rotref = (*this) * inv_scale;

#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!r_rotref.is_orthogonal(), Vector3());
#endif
	return scale.abs();
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used by the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3 &p_axis, real_t p_angle) const {
	return Basis(p_axis, p_angle) * (*this);
}

void Basis::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

void Basis::rotate_local(const Vector3 &p_axis, real_t p_angle) {
	// performs a rotation in object-local coordinate system:
	// M -> (M.R.Minv).M = M.R.
	*this = rotated_local(p_axis, p_angle);
}

Basis Basis::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	return (*this) * Basis(p_axis, p_angle);
}

Basis Basis::rotated(const Vector3 &p_euler, EulerOrder p_order) const {
	return Basis::from_euler(p_euler, p_order) * (*this);
}

void Basis::rotate(const Vector3 &p_euler, EulerOrder p_order) {
	*this = rotated(p_euler, p_order);
}

Basis Basis::rotated(const Quaternion &p_quaternion) const {
	return Basis(p_quaternion) * (*this);
}

void Basis::rotate(const Quaternion &p_quaternion) {
	*this = rotated(p_quaternion);
}

Vector3 Basis::get_euler_normalized(EulerOrder p_order) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler(p_order);
}

Quaternion Basis::get_rotation_quaternion() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_quaternion();
}

void Basis::rotate_to_align(Vector3 p_start_direction, Vector3 p_end_direction) {
	// Takes two vectors and rotates the basis from the first vector to the second vector.
	// Adopted from: https://gist.github.com/kevinmoran/b45980723e53edeb8a5a43c49f134724
	const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
	if (axis.length_squared() != 0) {
		real_t dot = p_start_direction.dot(p_end_direction);
		dot = CLAMP(dot, -1.0f, 1.0f);
		const real_t angle_rads = Math::acos(dot);
		*this = Basis(axis, angle_rads) * (*this);
	}
}

void Basis::get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

void Basis::get_rotation_axis_angle_local(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = transposed();
	m.orthonormalize();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
	p_angle = -p_angle;
}

Vector3 Basis::get_euler(EulerOrder p_order) const {
	// This epsilon value results in angles within a +/- 0.04 degree range being simplified/truncated.
	// Based on testing, this is the largest the epsilon can be without the angle truncation becoming
	// visually noticeable.
	const real_t epsilon = 0.00000025;

	switch (p_order) {
		case EulerOrder::XYZ: {
			// Euler angles in XYZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz          -cy*sz           sy
			//        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
			//       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

			Vector3 euler;
			real_t sy = rows[0][2];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					// is this a pure Y rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[1][2] == 0 && rows[2][1] == 0 && rows[1][1] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = 0;
						euler.y = std::atan2(rows[0][2], rows[0][0]);
						euler.z = 0;
					} else {
						euler.x = Math::atan2(-rows[1][2], rows[2][2]);
						euler.y = Math::asin(sy);
						euler.z = Math::atan2(-rows[0][1], rows[0][0]);
					}
				} else {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = -Math::PI / 2.0f;
					euler.z = 0.0f;
				}
			} else {
				euler.x = Math::atan2(rows[2][1], rows[1][1]);
				euler.y = Math::PI / 2.0f;
				euler.z = 0.0f;
			}
			return euler;
		}
		case EulerOrder::XZY: {
			// Euler angles in XZY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             -sz             cz*sy
			//        sx*sy+cx*cy*sz    cx*cz           cx*sz*sy-cy*sx
			//        cy*sx*sz          cz*sx           cx*cy+sx*sz*sy

			Vector3 euler;
			real_t sz = rows[0][1];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = Math::asin(-sz);
				} else {
					// It's -1
					euler.x = -Math::atan2(rows[1][2], rows[2][2]);
					euler.y = 0.0f;
					euler.z = Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = -Math::atan2(rows[1][2], rows[2][2]);
				euler.y = 0.0f;
				euler.z = -Math::PI / 2.0f;
			}
			return euler;
		}
		case EulerOrder::YXZ: {
			// Euler angles in YXZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz+sy*sx*sz    cz*sy*sx-cy*sz        cx*sy
			//        cx*sz             cx*cz                 -sx
			//        cy*sx*sz-cz*sy    cy*cz*sx+sy*sz        cy*cx

			Vector3 euler;

			real_t m12 = rows[1][2];

			if (m12 < (1 - epsilon)) {
				if (m12 > -(1 - epsilon)) {
					// is this a pure X rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[0][2] == 0 && rows[2][0] == 0 && rows[0][0] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = std::atan2(-m12, rows[1][1]);
						euler.y = 0;
						euler.z = 0;
					} else {
						euler.x = std::asin(-m12);
						euler.y = std::atan2(rows[0][2], rows[2][2]);
						euler.z = std::atan2(rows[1][0], rows[1][1]);
					}
				} else { // m12 == -1
					euler.x = Math::PI * 0.5f;
					euler.y = std::atan2(rows[0][1], rows[0][0]);
					euler.z = 0;
				}
			} else { // m12 == 1
				euler.x = -Math::PI * 0.5f;
				euler.y = -std::atan2(rows[0][1], rows[0][0]);
				euler.z = 0;
			}

			return euler;
		}
		case EulerOrder::YZX: {
			// Euler angles in YZX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz             sy*sx-cy*cx*sz     cx*sy+cy*sz*sx
			//        sz                cz*cx              -cz*sx
			//        -cz*sy            cy*sx+cx*sy*sz     cy*cx-sy*sz*sx

			Vector3 euler;
			real_t sz = rows[1][0];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(-rows[1][2], rows[1][1]);
					euler.y = Math::atan2(-rows[2][0], rows[0][0]);
					euler.z = Math::asin(sz);
				} else {
					// It's -1
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = 0.0f;
					euler.z = -Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = Math::atan2(rows[2][1], rows[2][2]);
				euler.y = 0.0f;
				euler.z = Math::PI / 2.0f;
			}
			return euler;
		} break;
		case EulerOrder::ZXY: {
			// Euler angles in ZXY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy-sz*sx*sy    -cx*sz                cz*sy+cy*sz*sx
			//        cy*sz+cz*sx*sy    cz*cx                 sz*sy-cz*cy*sx
			//        -cx*sy            sx                    cx*cy
			Vector3 euler;
			real_t sx = rows[2][1];
			if (sx < (1.0f - epsilon)) {
				if (sx > -(1.0f - epsilon)) {
					euler.x = Math::asin(sx);
					euler.y = Math::atan2(-rows[2][0], rows[2][2]);
					euler.z = Math::atan2(-rows[0][1], rows[1][1]);
				} else {
					// It's -1
					euler.x = -Math::PI / 2.0f;
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = 0;
				}
			} else {
				// It's 1
				euler.x = Math::PI / 2.0f;
				euler.y = Math::atan2(rows[0][2], rows[0][0]);
				euler.z = 0;
			}
			return euler;
		} break;
		case EulerOrder::ZYX: {
			// Euler angles in ZYX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             cz*sy*sx-cx*sz        sz*sx+cz*cx*cy
			//        cy*sz             cz*cx+sz*sy*sx        cx*sz*sy-cz*sx
			//        -sy               cy*sx                 cy*cx
			Vector3 euler;
			real_t sy = rows[2][0];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = Math::asin(-sy);
					euler.z = Math::atan2(rows[1][0], rows[0][0]);
				} else {
					// It's -1
					euler.x = 0;
					euler.y = Math::PI / 2.0f;
					euler.z = -Math::atan2(rows[0][1], rows[1][1]);
				}
			} else {
				// It's 1
				euler.x = 0;
				euler.y = -Math::PI / 2.0f;
				euler.z = -Math::atan2(rows[0][1], rows[1][1]);
			}
			return euler;
		}
		default: {
			ERR_FAIL_V_MSG(Vector3(), "Invalid parameter for get_euler(order)");
		}
	}
	return Vector3();
}

void Basis::set_euler(const Vector3 &p_euler, EulerOrder p_order) {
	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1, 0, 0, 0, c, -s, 0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0, s, 0, 1, 0, -s, 0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0, s, c, 0, 0, 0, 1);

	switch (p_order) {
		case EulerOrder::XYZ: {
			*this = xmat * (ymat * zmat);
		} break;
		case EulerOrder::XZY: {
			*this = xmat * zmat * ymat;
		} break;
		case EulerOrder::YXZ: {
			*this = ymat * xmat * zmat;
		} break;
		case EulerOrder::YZX: {
			*this = ymat * zmat * xmat;
		} break;
		case EulerOrder::ZXY: {
			*this = zmat * xmat * ymat;
		} break;
		case EulerOrder::ZYX: {
			*this = zmat * ymat * xmat;
		} break;
		default: {
			ERR_FAIL_MSG("Invalid Euler order parameter.");
		}
	}
}

bool Basis::is_equal_approx(const Basis &p_basis) const {
	return rows[0].is_equal_approx(p_basis.rows[0]) && rows[1].is_equal_approx(p_basis.rows[1]) && rows[2].is_equal_approx(p_basis.rows[2]);
}

bool Basis::is_same(const Basis &p_basis) const {
	return rows[0].is_same(p_basis.rows[0]) && rows[1].is_same(p_basis.rows[1]) && rows[2].is_same(p_basis.rows[2]);
}

bool Basis::is_finite() const {
	return rows[0].is_finite() && rows[1].is_finite() && rows[2].is_finite();
}

Basis::operator String() const {
	return "[X: " + get_column(0).operator String() +
			", Y: " + get_column(1).operator String() +
			", Z: " + get_column(2).operator String() + "]";
}

Quaternion Basis::get_quaternion() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_rotation(), Quaternion(), "Basis " + operator String() + " must be normalized in order to be casted to a Quaternion. Use get_rotation_quaternion() or call orthonormalized() if the Basis contains linearly independent vectors.");
#endif
	/* Allow getting a quaternion from an unnormalized transform */
	Basis m = *this;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0f) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

void Basis::get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
	/* checking this is a bad idea, because obtaining from scaled transform is a valid use case
#ifdef MATH_CHECKS
	ERR_FAIL_COND(!is_rotation());
#endif
	*/

	// https://www.euclideanspace.com/math/geometry/rotations/conversions/matrixToAngle/index.htm
	real_t x, y, z; // Variables for result.
	if (Math::is_zero_approx(rows[0][1] - rows[1][0]) && Math::is_zero_approx(rows[0][2] - rows[2][0]) && Math::is_zero_approx(rows[1][2] - rows[2][1])) {
		// Singularity found.
		// First check for identity matrix which must have +1 for all terms in leading diagonal and zero in other terms.
		if (is_diagonal() && (Math::abs(rows[0][0] + rows[1][1] + rows[2][2] - 3) < 3 * CMP_EPSILON)) {
			// This singularity is identity matrix so angle = 0.
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// Otherwise this singularity is angle = 180.
		real_t xx = (rows[0][0] + 1) / 2;
		real_t yy = (rows[1][1] + 1) / 2;
		real_t zz = (rows[2][2] + 1) / 2;
		real_t xy = (rows[0][1] + rows[1][0]) / 4;
		real_t xz = (rows[0][2] + rows[2][0]) / 4;
		real_t yz = (rows[1][2] + rows[2][1]) / 4;

		if ((xx > yy) && (xx > zz)) { // rows[0][0] is the largest diagonal term.
			if (xx < CMP_EPSILON) {
				x = 0;
				y = Math::SQRT12;
				z = Math::SQRT12;
			} else {
				x = Math::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		} else if (yy > zz) { // rows[1][1] is the largest diagonal term.
			if (yy < CMP_EPSILON) {
				x = Math::SQRT12;
				y = 0;
				z = Math::SQRT12;
			} else {
				y = Math::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		} else { // rows[2][2] is the largest diagonal term so base result on this.
			if (zz < CMP_EPSILON) {
				x = Math::SQRT12;
				y = Math::SQRT12;
				z = 0;
			} else {
				z = Math::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = Math::PI;
		return;
	}
	// As we have reached here there are no singularities so we can handle normally.
	double s = Math::sqrt((rows[2][1] - rows[1][2]) * (rows[2][1] - rows[1][2]) + (rows[0][2] - rows[2][0]) * (rows[0][2] - rows[2][0]) + (rows[1][0] - rows[0][1]) * (rows[1][0] - rows[0][1])); // Used to normalize.

	if (Math::abs(s) < CMP_EPSILON) {
		// Prevent divide by zero, should not happen if matrix is orthogonal and should be caught by singularity test above.
		s = 1;
	}

	x = (rows[2][1] - rows[1][2]) / s;
	y = (rows[0][2] - rows[2][0]) / s;
	z = (rows[1][0] - rows[0][1]) / s;

	r_axis = Vector3(x, y, z);
	// acos does clamping.
	r_angle = Math::acos((rows[0][0] + rows[1][1] + rows[2][2] - 1) / 2);
}

void Basis::set_quaternion(const Quaternion &p_quaternion) {
	real_t d = p_quaternion.length_squared();
	real_t s = 2.0f / d;
	real_t xs = p_quaternion.x * s, ys = p_quaternion.y * s, zs = p_quaternion.z * s;
	real_t wx = p_quaternion.w * xs, wy = p_quaternion.w * ys, wz = p_quaternion.w * zs;
	real_t xx = p_quaternion.x * xs, xy = p_quaternion.x * ys, xz = p_quaternion.x * zs;
	real_t yy = p_quaternion.y * ys, yz = p_quaternion.y * zs, zz = p_quaternion.z * zs;
	set(1.0f - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1.0f - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1.0f - (xx + yy));
}

void Basis::set_axis_angle(const Vector3 &p_axis, real_t p_angle) {
// Rotation matrix from axis and angle, see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	Vector3 axis_sq(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);
	real_t cosine = Math::cos(p_angle);
	rows[0][0] = axis_sq.x + cosine * (1.0f - axis_sq.x);
	rows[1][1] = axis_sq.y + cosine * (1.0f - axis_sq.y);
	rows[2][2] = axis_sq.z + cosine * (1.0f - axis_sq.z);

	real_t sine = Math::sin(p_angle);
	real_t t = 1 - cosine;

	real_t xyzt = p_axis.x * p_axis.y * t;
	real_t zyxs = p_axis.z * sine;
	rows[0][1] = xyzt - zyxs;
	rows[1][0] = xyzt + zyxs;

	xyzt = p_axis.x * p_axis.z * t;
	zyxs = p_axis.y * sine;
	rows[0][2] = xyzt + zyxs;
	rows[2][0] = xyzt - zyxs;

	xyzt = p_axis.y * p_axis.z * t;
	zyxs = p_axis.x * sine;
	rows[1][2] = xyzt - zyxs;
	rows[2][1] = xyzt + zyxs;
}

void Basis::set_axis_angle_scale(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_axis, p_angle);
}

void Basis::set_euler_scale(const Vector3 &p_euler, const Vector3 &p_scale, EulerOrder p_order) {
	_set_diagonal(p_scale);
	rotate(p_euler, p_order);
}

void Basis::set_quaternion_scale(const Quaternion &p_quaternion, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_quaternion);
}

// This also sets the non-diagonal elements to 0, which is misleading from the
// name, so we want this method to be private. Use `from_scale` externally.
void Basis::_set_diagonal(const Vector3 &p_diag) {
	rows[0][0] = p_diag.x;
	rows[0][1] = 0;
	rows[0][2] = 0;

	rows[1][0] = 0;
	rows[1][1] = p_diag.y;
	rows[1][2] = 0;

	rows[2][0] = 0;
	rows[2][1] = 0;
	rows[2][2] = p_diag.z;
}

Basis Basis::lerp(const Basis &p_to, real_t p_weight) const {
	Basis b;
	b.rows[0] = rows[0].lerp(p_to.rows[0], p_weight);
	b.rows[1] = rows[1].lerp(p_to.rows[1], p_weight);
	b.rows[2] = rows[2].lerp(p_to.rows[2], p_weight);

	return b;
}

Basis Basis::slerp(const Basis &p_to, real_t p_weight) const {
	//consider scale
	Quaternion from(*this);
	Quaternion to(p_to);

	Basis b(from.slerp(to, p_weight));
	b.rows[0] *= Math::lerp(rows[0].length(), p_to.rows[0].length(), p_weight);
	b.rows[1] *= Math::lerp(rows[1].length(), p_to.rows[1].length(), p_weight);
	b.rows[2] *= Math::lerp(rows[2].length(), p_to.rows[2].length(), p_weight);

	return b;
}

void Basis::rotate_sh(real_t *p_values) {
	// code by John Hable
	// http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
	// this code is Public Domain

	const static real_t s_c3 = 0.94617469575; // (3*sqrt(5))/(4*sqrt(pi))
	const static real_t s_c4 = -0.31539156525; // (-sqrt(5))/(4*sqrt(pi))
	const static real_t s_c5 = 0.54627421529; // (sqrt(15))/(4*sqrt(pi))

	const static real_t s_c_scale = 1.0 / 0.91529123286551084;
	const static real_t s_c_scale_inv = 0.91529123286551084;

	const static real_t s_rc2 = 1.5853309190550713 * s_c_scale;
	const static real_t s_c4_div_c3 = s_c4 / s_c3;
	const static real_t s_c4_div_c3_x2 = (s_c4 / s_c3) * 2.0;

	const static real_t s_scale_dst2 = s_c3 * s_c_scale_inv;
	const static real_t s_scale_dst4 = s_c5 * s_c_scale_inv;

	const real_t src[9] = { p_values[0], p_values[1], p_values[2], p_values[3], p_values[4], p_values[5], p_values[6], p_values[7], p_values[8] };

	real_t m00 = rows[0][0];
	real_t m01 = rows[0][1];
	real_t m02 = rows[0][2];
	real_t m10 = rows[1][0];
	real_t m11 = rows[1][1];
	real_t m12 = rows[1][2];
	real_t m20 = rows[2][0];
	real_t m21 = rows[2][1];
	real_t m22 = rows[2][2];

	p_values[0] = src[0];
	p_values[1] = m11 * src[1] - m12 * src[2] + m10 * src[3];
	p_values[2] = -m21 * src[1] + m22 * src[2] - m20 * src[3];
	p_values[3] = m01 * src[1] - m02 * src[2] + m00 * src[3];

	real_t sh0 = src[7] + src[8] + src[8] - src[5];
	real_t sh1 = src[4] + s_rc2 * src[6] + src[7] + src[8];
	real_t sh2 = src[4];
	real_t sh3 = -src[7];
	real_t sh4 = -src[5];

	// Rotations.  R0 and R1 just use the raw matrix columns
	real_t r2x = m00 + m01;
	real_t r2y = m10 + m11;
	real_t r2z = m20 + m21;

	real_t r3x = m00 + m02;
	real_t r3y = m10 + m12;
	real_t r3z = m20 + m22;

	real_t r4x = m01 + m02;
	real_t r4y = m11 + m12;
	real_t r4z = m21 + m22;

	// dense matrix multiplication one column at a time

	// column 0
	real_t sh0_x = sh0 * m00;
	real_t sh0_y = sh0 * m10;
	real_t d0 = sh0_x * m10;
	real_t d1 = sh0_y * m20;
	real_t d2 = sh0 * (m20 * m20 + s_c4_div_c3);
	real_t d3 = sh0_x * m20;
	real_t d4 = sh0_x * m00 - sh0_y * m10;

	// column 1
	real_t sh1_x = sh1 * m02;
	real_t sh1_y = sh1 * m12;
	d0 += sh1_x * m12;
	d1 += sh1_y * m22;
	d2 += sh1 * (m22 * m22 + s_c4_div_c3);
	d3 += sh1_x * m22;
	d4 += sh1_x * m02 - sh1_y * m12;

	// column 2
	real_t sh2_x = sh2 * r2x;
	real_t sh2_y = sh2 * r2y;
	d0 += sh2_x * r2y;
	d1 += sh2_y * r2z;
	d2 += sh2 * (r2z * r2z + s_c4_div_c3_x2);
	d3 += sh2_x * r2z;
	d4 += sh2_x * r2x - sh2_y * r2y;

	// column 3
	real_t sh3_x = sh3 * r3x;
	real_t sh3_y = sh3 * r3y;
	d0 += sh3_x * r3y;
	d1 += sh3_y * r3z;
	d2 += sh3 * (r3z * r3z + s_c4_div_c3_x2);
	d3 += sh3_x * r3z;
	d4 += sh3_x * r3x - sh3_y * r3y;

	// column 4
	real_t sh4_x = sh4 * r4x;
	real_t sh4_y = sh4 * r4y;
	d0 += sh4_x * r4y;
	d1 += sh4_y * r4z;
	d2 += sh4 * (r4z * r4z + s_c4_div_c3_x2);
	d3 += sh4_x * r4z;
	d4 += sh4_x * r4x - sh4_y * r4y;

	// extra multipliers
	p_values[4] = d0;
	p_values[5] = -d1;
	p_values[6] = d2 * s_scale_dst2;
	p_values[7] = -d3;
	p_values[8] = d4 * s_scale_dst4;
}

Basis Basis::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(p_target.is_zero_approx(), Basis(), "The target vector can't be zero.");
	ERR_FAIL_COND_V_MSG(p_up.is_zero_approx(), Basis(), "The up vector can't be zero.");
#endif
	Vector3 v_z = p_target.normalized();
	if (!p_use_model_front) {
		v_z = -v_z;
	}
	Vector3 v_x = p_up.cross(v_z);
	if (v_x.is_zero_approx()) {
		WARN_PRINT("Target and up vectors are colinear. This is not advised as it may cause unwanted rotation around local Z axis.");
		v_x = p_up.get_any_perpendicular(); // Vectors are almost parallel.
	}
	v_x.normalize();
	Vector3 v_y = v_z.cross(v_x);

	Basis basis;
	basis.set_columns(v_x, v_y, v_z);
	return basis;
}

// --- transform_2d.cpp ---

void Transform2D::invert()
{
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform2D::affine_inverse can handle matrices with scaling, so GDScript should eventually
	// use that.
	SWAP(columns[0][1], columns[1][0]);
	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::inverse() const
{
	Transform2D inv = *this;
	inv.invert();
	return inv;
}

void Transform2D::affine_invert()
{
	real_t det = determinant();
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t idet = 1.0f / det;

	SWAP(columns[0][0], columns[1][1]);
	columns[0] *= Vector2(idet, -idet);
	columns[1] *= Vector2(-idet, idet);

	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::affine_inverse() const
{
	Transform2D inv = *this;
	inv.affine_invert();
	return inv;
}

void Transform2D::rotate(real_t p_angle) { *this = Transform2D(p_angle, Vector2()) * (*this); }

real_t Transform2D::get_skew() const
{
	real_t det = determinant();
	return Math::acos(columns[0].normalized().dot(SIGN(det) * columns[1].normalized())) -
		   (real_t)Math::PI * 0.5f;
}

void Transform2D::set_skew(real_t p_angle)
{
	real_t det = determinant();
	columns[1] = SIGN(det) * columns[0].rotated(((real_t)Math::PI * 0.5f + p_angle)).normalized() *
				 columns[1].length();
}

real_t Transform2D::get_rotation() const { return Math::atan2(columns[0].y, columns[0].x); }

void Transform2D::set_rotation(real_t p_rot)
{
	Size2 scale = get_scale();
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	set_scale(scale);
}

Transform2D::Transform2D(real_t p_rot, const Vector2& p_pos)
{
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	columns[2] = p_pos;
}

Transform2D::Transform2D(real_t p_rot, const Size2& p_scale, real_t p_skew, const Vector2& p_pos)
{
	columns[0][0] = Math::cos(p_rot) * p_scale.x;
	columns[1][1] = Math::cos(p_rot + p_skew) * p_scale.y;
	columns[1][0] = -Math::sin(p_rot + p_skew) * p_scale.y;
	columns[0][1] = Math::sin(p_rot) * p_scale.x;
	columns[2] = p_pos;
}

Size2 Transform2D::get_scale() const
{
	real_t det_sign = SIGN(determinant());
	return Size2(columns[0].length(), det_sign * columns[1].length());
}

void Transform2D::set_scale(const Size2& p_scale)
{
	columns[0].normalize();
	columns[1].normalize();
	columns[0] *= p_scale.x;
	columns[1] *= p_scale.y;
}

void Transform2D::scale(const Size2& p_scale)
{
	scale_basis(p_scale);
	columns[2] *= p_scale;
}

void Transform2D::scale_basis(const Size2& p_scale)
{
	columns[0][0] *= p_scale.x;
	columns[0][1] *= p_scale.y;
	columns[1][0] *= p_scale.x;
	columns[1][1] *= p_scale.y;
}

void Transform2D::translate_local(real_t p_tx, real_t p_ty)
{
	translate_local(Vector2(p_tx, p_ty));
}

void Transform2D::translate_local(const Vector2& p_translation)
{
	columns[2] += basis_xform(p_translation);
}

void Transform2D::orthonormalize()
{
	// Orthonormalizable check is done in Vector2 class below.
	// Gram-Schmidt Process:

	Vector2 x = columns[0];
	Vector2 y = columns[1];

	x.normalize();
	y = y - x * x.dot(y);
	y.normalize();

	columns[0] = x;
	columns[1] = y;
}

Transform2D Transform2D::orthonormalized() const
{
	Transform2D ortho = *this;
	ortho.orthonormalize();
	return ortho;
}

bool Transform2D::is_conformal() const
{
	// Non-flipped case.
	if (Math::is_equal_approx(columns[0][0], columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], -columns[1][0])) {
		return true;
	}
	// Flipped case.
	if (Math::is_equal_approx(columns[0][0], -columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], columns[1][0])) {
		return true;
	}
	return false;
}

bool Transform2D::is_equal_approx(const Transform2D& p_transform) const
{
	return columns[0].is_equal_approx(p_transform.columns[0]) &&
		   columns[1].is_equal_approx(p_transform.columns[1]) &&
		   columns[2].is_equal_approx(p_transform.columns[2]);
}

bool Transform2D::is_same(const Transform2D& p_transform) const
{
	return columns[0].is_same(p_transform.columns[0]) &&
		   columns[1].is_same(p_transform.columns[1]) && columns[2].is_same(p_transform.columns[2]);
}

bool Transform2D::is_finite() const
{
	return columns[0].is_finite() && columns[1].is_finite() && columns[2].is_finite();
}

Transform2D Transform2D::looking_at(const Vector2& p_target) const
{
	Transform2D return_trans = Transform2D(get_rotation(), get_origin());
	Vector2 target_position = affine_inverse().xform(p_target);
	return_trans.set_rotation(
		return_trans.get_rotation() + (target_position * get_scale()).angle());
	return return_trans;
}

void Transform2D::operator*=(const Transform2D& p_transform)
{
	columns[2] = xform(p_transform.columns[2]);

	real_t x0, x1, y0, y1;

	x0 = tdotx(p_transform.columns[0]);
	x1 = tdoty(p_transform.columns[0]);
	y0 = tdotx(p_transform.columns[1]);
	y1 = tdoty(p_transform.columns[1]);

	columns[0][0] = x0;
	columns[0][1] = x1;
	columns[1][0] = y0;
	columns[1][1] = y1;
}

Transform2D Transform2D::operator*(const Transform2D& p_transform) const
{
	Transform2D t = *this;
	t *= p_transform;
	return t;
}

Transform2D Transform2D::scaled(const Size2& p_scale) const
{
	// Equivalent to left multiplication
	Transform2D copy = *this;
	copy.scale(p_scale);
	return copy;
}

Transform2D Transform2D::scaled_local(const Size2& p_scale) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0] * p_scale.x, columns[1] * p_scale.y, columns[2]);
}

Transform2D Transform2D::untranslated() const
{
	Transform2D copy = *this;
	copy.columns[2] = Vector2();
	return copy;
}

Transform2D Transform2D::translated(const Vector2& p_offset) const
{
	// Equivalent to left multiplication
	return Transform2D(columns[0], columns[1], columns[2] + p_offset);
}

Transform2D Transform2D::translated_local(const Vector2& p_offset) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0], columns[1], columns[2] + basis_xform(p_offset));
}

Transform2D Transform2D::rotated(real_t p_angle) const
{
	// Equivalent to left multiplication
	return Transform2D(p_angle, Vector2()) * (*this);
}

Transform2D Transform2D::rotated_local(real_t p_angle) const
{
	// Equivalent to right multiplication
	return (*this) * Transform2D(p_angle,
						 Vector2()); // Could be optimized, because origin transform can be skipped.
}

real_t Transform2D::determinant() const
{
	return columns[0].x * columns[1].y - columns[0].y * columns[1].x;
}

Transform2D Transform2D::interpolate_with(const Transform2D& p_transform, real_t p_weight) const
{
	return Transform2D(Math::lerp_angle(get_rotation(), p_transform.get_rotation(), p_weight),
		get_scale().lerp(p_transform.get_scale(), p_weight),
		Math::lerp_angle(get_skew(), p_transform.get_skew(), p_weight),
		get_origin().lerp(p_transform.get_origin(), p_weight));
}

Transform2D::operator String() const
{
	return "[X: " + columns[0].operator String() + ", Y: " + columns[1].operator String() +
		   ", O: " + columns[2].operator String() + "]";
}

// --- transform_3d.cpp ---

void Transform3D::affine_invert() {
	basis.invert();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::affine_inverse() const {
	Transform3D ret = *this;
	ret.affine_invert();
	return ret;
}

void Transform3D::invert() {
	basis.transpose();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::inverse() const {
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform3D::affine_inverse can handle matrices with scaling, so GDScript should eventually use that.
	Transform3D ret = *this;
	ret.invert();
	return ret;
}

void Transform3D::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

Transform3D Transform3D::rotated(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to left multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(p_basis * basis, p_basis.xform(origin));
}

Transform3D Transform3D::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to right multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(basis * p_basis, origin);
}

void Transform3D::rotate_basis(const Vector3 &p_axis, real_t p_angle) {
	basis.rotate(p_axis, p_angle);
}

Transform3D Transform3D::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(origin.is_equal_approx(p_target), Transform3D(), "The transform's origin and target can't be equal.");
#endif
	Transform3D t = *this;
	t.basis = Basis::looking_at(p_target - origin, p_up, p_use_model_front);
	return t;
}

void Transform3D::set_look_at(const Vector3 &p_eye, const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(p_eye.is_equal_approx(p_target), "The eye and target vectors can't be equal.");
#endif
	basis = Basis::looking_at(p_target - p_eye, p_up, p_use_model_front);
	origin = p_eye;
}

Transform3D Transform3D::interpolate_with(const Transform3D &p_transform, real_t p_c) const {
	Transform3D interp;

	Vector3 src_scale = basis.get_scale();
	Quaternion src_rot = basis.get_rotation_quaternion();
	Vector3 src_loc = origin;

	Vector3 dst_scale = p_transform.basis.get_scale();
	Quaternion dst_rot = p_transform.basis.get_rotation_quaternion();
	Vector3 dst_loc = p_transform.origin;

	interp.basis.set_quaternion_scale(src_rot.slerp(dst_rot, p_c).normalized(), src_scale.lerp(dst_scale, p_c));
	interp.origin = src_loc.lerp(dst_loc, p_c);

	return interp;
}

void Transform3D::scale(const Vector3 &p_scale) {
	basis.scale(p_scale);
	origin *= p_scale;
}

Transform3D Transform3D::scaled(const Vector3 &p_scale) const {
	// Equivalent to left multiplication
	return Transform3D(basis.scaled(p_scale), origin * p_scale);
}

Transform3D Transform3D::scaled_local(const Vector3 &p_scale) const {
	// Equivalent to right multiplication
	return Transform3D(basis.scaled_local(p_scale), origin);
}

void Transform3D::scale_basis(const Vector3 &p_scale) {
	basis.scale(p_scale);
}

void Transform3D::translate_local(real_t p_tx, real_t p_ty, real_t p_tz) {
	translate_local(Vector3(p_tx, p_ty, p_tz));
}

void Transform3D::translate_local(const Vector3 &p_translation) {
	for (int i = 0; i < 3; i++) {
		origin[i] += basis[i].dot(p_translation);
	}
}

Transform3D Transform3D::translated(const Vector3 &p_translation) const {
	// Equivalent to left multiplication
	return Transform3D(basis, origin + p_translation);
}

Transform3D Transform3D::translated_local(const Vector3 &p_translation) const {
	// Equivalent to right multiplication
	return Transform3D(basis, origin + basis.xform(p_translation));
}

void Transform3D::orthonormalize() {
	// Orthonormalizable check is done in Basis class below.
	basis.orthonormalize();
}

Transform3D Transform3D::orthonormalized() const {
	Transform3D _copy = *this;
	_copy.orthonormalize();
	return _copy;
}

void Transform3D::orthogonalize() {
	basis.orthogonalize();
}

Transform3D Transform3D::orthogonalized() const {
	Transform3D _copy = *this;
	_copy.orthogonalize();
	return _copy;
}

bool Transform3D::is_equal_approx(const Transform3D &p_transform) const {
	return basis.is_equal_approx(p_transform.basis) && origin.is_equal_approx(p_transform.origin);
}

bool Transform3D::is_same(const Transform3D &p_transform) const {
	return basis.is_same(p_transform.basis) && origin.is_same(p_transform.origin);
}

bool Transform3D::is_finite() const {
	return basis.is_finite() && origin.is_finite();
}

void Transform3D::operator*=(const Transform3D &p_transform) {
	origin = xform(p_transform.origin);
	basis *= p_transform.basis;
}

Transform3D Transform3D::operator*(const Transform3D &p_transform) const {
	Transform3D t = *this;
	t *= p_transform;
	return t;
}

Transform3D::operator String() const {
	return "[X: " + basis.get_column(0).operator String() +
			", Y: " + basis.get_column(1).operator String() +
			", Z: " + basis.get_column(2).operator String() +
			", O: " + origin.operator String() + "]";
}

// --- projection.cpp ---

real_t Projection::determinant() const {
	return columns[0][3] * columns[1][2] * columns[2][1] * columns[3][0] - columns[0][2] * columns[1][3] * columns[2][1] * columns[3][0] -
			columns[0][3] * columns[1][1] * columns[2][2] * columns[3][0] + columns[0][1] * columns[1][3] * columns[2][2] * columns[3][0] +
			columns[0][2] * columns[1][1] * columns[2][3] * columns[3][0] - columns[0][1] * columns[1][2] * columns[2][3] * columns[3][0] -
			columns[0][3] * columns[1][2] * columns[2][0] * columns[3][1] + columns[0][2] * columns[1][3] * columns[2][0] * columns[3][1] +
			columns[0][3] * columns[1][0] * columns[2][2] * columns[3][1] - columns[0][0] * columns[1][3] * columns[2][2] * columns[3][1] -
			columns[0][2] * columns[1][0] * columns[2][3] * columns[3][1] + columns[0][0] * columns[1][2] * columns[2][3] * columns[3][1] +
			columns[0][3] * columns[1][1] * columns[2][0] * columns[3][2] - columns[0][1] * columns[1][3] * columns[2][0] * columns[3][2] -
			columns[0][3] * columns[1][0] * columns[2][1] * columns[3][2] + columns[0][0] * columns[1][3] * columns[2][1] * columns[3][2] +
			columns[0][1] * columns[1][0] * columns[2][3] * columns[3][2] - columns[0][0] * columns[1][1] * columns[2][3] * columns[3][2] -
			columns[0][2] * columns[1][1] * columns[2][0] * columns[3][3] + columns[0][1] * columns[1][2] * columns[2][0] * columns[3][3] +
			columns[0][2] * columns[1][0] * columns[2][1] * columns[3][3] - columns[0][0] * columns[1][2] * columns[2][1] * columns[3][3] -
			columns[0][1] * columns[1][0] * columns[2][2] * columns[3][3] + columns[0][0] * columns[1][1] * columns[2][2] * columns[3][3];
}

void Projection::set_identity() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = (i == j) ? 1 : 0;
		}
	}
}

void Projection::set_zero() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = 0;
		}
	}
}

Plane Projection::xform4(const Plane &p_vec4) const {
	Plane ret;

	ret.normal.x = columns[0][0] * p_vec4.normal.x + columns[1][0] * p_vec4.normal.y + columns[2][0] * p_vec4.normal.z + columns[3][0] * p_vec4.d;
	ret.normal.y = columns[0][1] * p_vec4.normal.x + columns[1][1] * p_vec4.normal.y + columns[2][1] * p_vec4.normal.z + columns[3][1] * p_vec4.d;
	ret.normal.z = columns[0][2] * p_vec4.normal.x + columns[1][2] * p_vec4.normal.y + columns[2][2] * p_vec4.normal.z + columns[3][2] * p_vec4.d;
	ret.d = columns[0][3] * p_vec4.normal.x + columns[1][3] * p_vec4.normal.y + columns[2][3] * p_vec4.normal.z + columns[3][3] * p_vec4.d;
	return ret;
}

Vector4 Projection::xform(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[1][0] * p_vec4.y + columns[2][0] * p_vec4.z + columns[3][0] * p_vec4.w,
			columns[0][1] * p_vec4.x + columns[1][1] * p_vec4.y + columns[2][1] * p_vec4.z + columns[3][1] * p_vec4.w,
			columns[0][2] * p_vec4.x + columns[1][2] * p_vec4.y + columns[2][2] * p_vec4.z + columns[3][2] * p_vec4.w,
			columns[0][3] * p_vec4.x + columns[1][3] * p_vec4.y + columns[2][3] * p_vec4.z + columns[3][3] * p_vec4.w);
}
Vector4 Projection::xform_inv(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[0][1] * p_vec4.y + columns[0][2] * p_vec4.z + columns[0][3] * p_vec4.w,
			columns[1][0] * p_vec4.x + columns[1][1] * p_vec4.y + columns[1][2] * p_vec4.z + columns[1][3] * p_vec4.w,
			columns[2][0] * p_vec4.x + columns[2][1] * p_vec4.y + columns[2][2] * p_vec4.z + columns[2][3] * p_vec4.w,
			columns[3][0] * p_vec4.x + columns[3][1] * p_vec4.y + columns[3][2] * p_vec4.z + columns[3][3] * p_vec4.w);
}

void Projection::adjust_perspective_znear(real_t p_new_znear) {
	real_t zfar = get_z_far();
	real_t znear = p_new_znear;

	real_t deltaZ = zfar - znear;
	columns[2][2] = -(zfar + znear) / deltaZ;
	columns[3][2] = -2 * znear * zfar / deltaZ;
}

Projection Projection::create_depth_correction(bool p_flip_y) {
	Projection proj;
	proj.set_depth_correction(p_flip_y);
	return proj;
}

Projection Projection::create_light_atlas_rect(const Rect2 &p_rect) {
	Projection proj;
	proj.set_light_atlas_rect(p_rect);
	return proj;
}

Projection Projection::create_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov);
	return proj;
}

Projection Projection::create_perspective_hmd(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov, p_eye, p_intraocular_dist, p_convergence_dist);
	return proj;
}

Projection Projection::create_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	Projection proj;
	proj.set_for_hmd(p_eye, p_aspect, p_intraocular_dist, p_display_width, p_display_to_lens, p_oversample, p_z_near, p_z_far);
	return proj;
}

Projection Projection::create_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	Projection proj;
	proj.set_orthogonal(p_left, p_right, p_bottom, p_top, p_znear, p_zfar);
	return proj;
}

Projection Projection::create_orthogonal_aspect(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	Projection proj;
	proj.set_orthogonal(p_size, p_aspect, p_znear, p_zfar, p_flip_fov);
	return proj;
}

Projection Projection::create_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	Projection proj;
	proj.set_frustum(p_left, p_right, p_bottom, p_top, p_near, p_far);
	return proj;
}

Projection Projection::create_frustum_aspect(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	Projection proj;
	proj.set_frustum(p_size, p_aspect, p_offset, p_near, p_far, p_flip_fov);
	return proj;
}

Projection Projection::create_fit_aabb(const AABB &p_aabb) {
	Projection proj;
	proj.scale_translate_to_fit(p_aabb);
	return proj;
}

Projection Projection::perspective_znear_adjusted(real_t p_new_znear) const {
	Projection proj = *this;
	proj.adjust_perspective_znear(p_new_znear);
	return proj;
}

Plane Projection::get_projection_plane(Planes p_plane) const {
	const real_t *matrix = (const real_t *)columns;

	switch (p_plane) {
		case PLANE_NEAR: {
			Plane new_plane = Plane(matrix[3] + matrix[2],
					matrix[7] + matrix[6],
					matrix[11] + matrix[10],
					matrix[15] + matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_FAR: {
			Plane new_plane = Plane(matrix[3] - matrix[2],
					matrix[7] - matrix[6],
					matrix[11] - matrix[10],
					matrix[15] - matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_LEFT: {
			Plane new_plane = Plane(matrix[3] + matrix[0],
					matrix[7] + matrix[4],
					matrix[11] + matrix[8],
					matrix[15] + matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_TOP: {
			Plane new_plane = Plane(matrix[3] - matrix[1],
					matrix[7] - matrix[5],
					matrix[11] - matrix[9],
					matrix[15] - matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_RIGHT: {
			Plane new_plane = Plane(matrix[3] - matrix[0],
					matrix[7] - matrix[4],
					matrix[11] - matrix[8],
					matrix[15] - matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_BOTTOM: {
			Plane new_plane = Plane(matrix[3] + matrix[1],
					matrix[7] + matrix[5],
					matrix[11] + matrix[9],
					matrix[15] + matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
	}

	return Plane();
}

Projection Projection::flipped_y() const {
	Projection proj = *this;
	proj.flip_y();
	return proj;
}

Projection Projection ::jitter_offseted(const Vector2 &p_offset) const {
	Projection proj = *this;
	proj.add_jitter_offset(p_offset);
	return proj;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t sine, cotangent, deltaZ;
	real_t radians = Math::deg_to_rad(p_fovy_degrees / 2.0);

	deltaZ = p_z_far - p_z_near;
	sine = Math::sin(radians);

	if ((deltaZ == 0) || (sine == 0) || (p_aspect == 0)) {
		return;
	}
	cotangent = Math::cos(radians) / sine;

	set_identity();

	columns[0][0] = cotangent / p_aspect;
	columns[1][1] = cotangent;
	columns[2][2] = -(p_z_far + p_z_near) / deltaZ;
	columns[2][3] = -1;
	columns[3][2] = -2 * p_z_near * p_z_far / deltaZ;
	columns[3][3] = 0;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t left, right, modeltranslation, ymax, xmax, frustumshift;

	ymax = p_z_near * std::tan(Math::deg_to_rad(p_fovy_degrees / 2.0));
	xmax = ymax * p_aspect;
	frustumshift = (p_intraocular_dist / 2.0) * p_z_near / p_convergence_dist;

	switch (p_eye) {
		case 1: { // left eye
			left = -xmax + frustumshift;
			right = xmax + frustumshift;
			modeltranslation = p_intraocular_dist / 2.0;
		} break;
		case 2: { // right eye
			left = -xmax - frustumshift;
			right = xmax - frustumshift;
			modeltranslation = -p_intraocular_dist / 2.0;
		} break;
		default: { // mono, should give the same result as set_perspective(p_fovy_degrees,p_aspect,p_z_near,p_z_far,p_flip_fov)
			left = -xmax;
			right = xmax;
			modeltranslation = 0.0;
		} break;
	}

	set_frustum(left, right, -ymax, ymax, p_z_near, p_z_far);

	// translate matrix by (modeltranslation, 0.0, 0.0)
	Projection cm;
	cm.set_identity();
	cm.columns[3][0] = modeltranslation;
	*this = *this * cm;
}

void Projection::set_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	// we first calculate our base frustum on our values without taking our lens magnification into account.
	real_t f1 = (p_intraocular_dist * 0.5) / p_display_to_lens;
	real_t f2 = ((p_display_width - p_intraocular_dist) * 0.5) / p_display_to_lens;
	real_t f3 = (p_display_width / 4.0) / p_display_to_lens;

	// now we apply our oversample factor to increase our FOV. how much we oversample is always a balance we strike between performance and how much
	// we're willing to sacrifice in FOV.
	real_t add = ((f1 + f2) * (p_oversample - 1.0)) / 2.0;
	f1 += add;
	f2 += add;
	f3 *= p_oversample;

	// always apply KEEP_WIDTH aspect ratio
	f3 /= p_aspect;

	switch (p_eye) {
		case 1: { // left eye
			set_frustum(-f2 * p_z_near, f1 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		case 2: { // right eye
			set_frustum(-f1 * p_z_near, f2 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		default: { // mono, does not apply here!
		} break;
	}
}

void Projection::set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	set_identity();

	columns[0][0] = 2.0 / (p_right - p_left);
	columns[3][0] = -((p_right + p_left) / (p_right - p_left));
	columns[1][1] = 2.0 / (p_top - p_bottom);
	columns[3][1] = -((p_top + p_bottom) / (p_top - p_bottom));
	columns[2][2] = -2.0 / (p_zfar - p_znear);
	columns[3][2] = -((p_zfar + p_znear) / (p_zfar - p_znear));
	columns[3][3] = 1.0;
}

void Projection::set_orthogonal(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_orthogonal(-p_size / 2, +p_size / 2, -p_size / p_aspect / 2, +p_size / p_aspect / 2, p_znear, p_zfar);
}

void Projection::set_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	ERR_FAIL_COND(p_right <= p_left);
	ERR_FAIL_COND(p_top <= p_bottom);
	ERR_FAIL_COND(p_far <= p_near);

	real_t *te = &columns[0][0];
	real_t x = 2 * p_near / (p_right - p_left);
	real_t y = 2 * p_near / (p_top - p_bottom);

	real_t a = (p_right + p_left) / (p_right - p_left);
	real_t b = (p_top + p_bottom) / (p_top - p_bottom);
	real_t c = -(p_far + p_near) / (p_far - p_near);
	real_t d = -2 * p_far * p_near / (p_far - p_near);

	te[0] = x;
	te[1] = 0;
	te[2] = 0;
	te[3] = 0;
	te[4] = 0;
	te[5] = y;
	te[6] = 0;
	te[7] = 0;
	te[8] = a;
	te[9] = b;
	te[10] = c;
	te[11] = -1;
	te[12] = 0;
	te[13] = 0;
	te[14] = d;
	te[15] = 0;
}

void Projection::set_frustum(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_frustum(-p_size / 2 + p_offset.x, +p_size / 2 + p_offset.x, -p_size / p_aspect / 2 + p_offset.y, +p_size / p_aspect / 2 + p_offset.y, p_near, p_far);
}

real_t Projection::get_z_far() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] - columns[3][2]) / (columns[2][3] - columns[2][2]);
}

real_t Projection::get_z_near() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] + columns[3][2]) / (columns[2][3] + columns[2][2]);
}

Vector2 Projection::get_viewport_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_near() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

Vector2 Projection::get_far_plane_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_far() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

bool Projection::get_endpoints(const Transform3D &p_transform, Vector3 *p_8points) const {
	Vector<Plane> planes = get_projection_planes(Transform3D());
	const Planes intersections[8][3] = {
		{ PLANE_FAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_FAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_BOTTOM },
	};

	for (int i = 0; i < 8; i++) {
		Vector3 point;
		Plane a = planes[intersections[i][0]];
		Plane b = planes[intersections[i][1]];
		Plane c = planes[intersections[i][2]];
		bool res = a.intersect_3(b, c, &point);
		ERR_FAIL_COND_V(!res, false);
		p_8points[i] = p_transform.xform(point);
	}

	return true;
}

Vector<Plane> Projection::get_projection_planes(const Transform3D &p_transform) const {
	/** Fast Plane Extraction from combined modelview/projection matrices.
	 * References:
	 * https://web.archive.org/web/20011221205252/https://www.markmorley.com/opengl/frustumculling.html
	 * https://web.archive.org/web/20061020020112/https://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
	 */

	Vector<Plane> planes;
	planes.resize(6);

	const real_t *matrix = (const real_t *)columns;

	Plane new_plane;

	///////--- Near Plane ---///////
	new_plane = Plane(matrix[3] + matrix[2],
			matrix[7] + matrix[6],
			matrix[11] + matrix[10],
			matrix[15] + matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[0] = p_transform.xform(new_plane);

	///////--- Far Plane ---///////
	new_plane = Plane(matrix[3] - matrix[2],
			matrix[7] - matrix[6],
			matrix[11] - matrix[10],
			matrix[15] - matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[1] = p_transform.xform(new_plane);

	///////--- Left Plane ---///////
	new_plane = Plane(matrix[3] + matrix[0],
			matrix[7] + matrix[4],
			matrix[11] + matrix[8],
			matrix[15] + matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[2] = p_transform.xform(new_plane);

	///////--- Top Plane ---///////
	new_plane = Plane(matrix[3] - matrix[1],
			matrix[7] - matrix[5],
			matrix[11] - matrix[9],
			matrix[15] - matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[3] = p_transform.xform(new_plane);

	///////--- Right Plane ---///////
	new_plane = Plane(matrix[3] - matrix[0],
			matrix[7] - matrix[4],
			matrix[11] - matrix[8],
			matrix[15] - matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[4] = p_transform.xform(new_plane);

	///////--- Bottom Plane ---///////
	new_plane = Plane(matrix[3] + matrix[1],
			matrix[7] + matrix[5],
			matrix[11] + matrix[9],
			matrix[15] + matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[5] = p_transform.xform(new_plane);

	return planes;
}

Projection Projection::inverse() const {
	Projection cm = *this;
	cm.invert();
	return cm;
}

void Projection::invert() {
	// Adapted from Mesa's `src/util/u_math.c` `util_invert_mat4x4`.
	// MIT licensed. Copyright 2008 VMware, Inc. Authored by Jacques Leroy.
	Projection temp;
	real_t *out = (real_t *)temp.columns;
	real_t *m = (real_t *)columns;

	real_t wtmp[4][8];
	real_t m0, m1, m2, m3, s;
	real_t *r0, *r1, *r2, *r3;

#define MAT(m, r, c) (m)[(c) * 4 + (r)]

	r0 = wtmp[0];
	r1 = wtmp[1];
	r2 = wtmp[2];
	r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0);
	r0[1] = MAT(m, 0, 1);
	r0[2] = MAT(m, 0, 2);
	r0[3] = MAT(m, 0, 3);
	r0[4] = 1.0;
	r0[5] = 0.0;
	r0[6] = 0.0;
	r0[7] = 0.0;

	r1[0] = MAT(m, 1, 0);
	r1[1] = MAT(m, 1, 1);
	r1[2] = MAT(m, 1, 2);
	r1[3] = MAT(m, 1, 3);
	r1[5] = 1.0;
	r1[4] = 0.0;
	r1[6] = 0.0;
	r1[7] = 0.0;

	r2[0] = MAT(m, 2, 0);
	r2[1] = MAT(m, 2, 1);
	r2[2] = MAT(m, 2, 2);
	r2[3] = MAT(m, 2, 3);
	r2[6] = 1.0;
	r2[4] = 0.0;
	r2[5] = 0.0;
	r2[7] = 0.0;

	r3[0] = MAT(m, 3, 0);
	r3[1] = MAT(m, 3, 1);
	r3[2] = MAT(m, 3, 2);
	r3[3] = MAT(m, 3, 3);

	r3[7] = 1.0;
	r3[4] = 0.0;
	r3[5] = 0.0;
	r3[6] = 0.0;

	/* choose pivot - or die */
	if (Math::abs(r3[0]) > Math::abs(r2[0])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[0]) > Math::abs(r1[0])) {
		SWAP(r2, r1);
	}
	if (Math::abs(r1[0]) > Math::abs(r0[0])) {
		SWAP(r1, r0);
	}
	ERR_FAIL_COND(0.0 == r0[0]);

	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[1]) > Math::abs(r2[1])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[1]) > Math::abs(r1[1])) {
		SWAP(r2, r1);
	}
	ERR_FAIL_COND(0.0 == r1[1]);

	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[2]) > Math::abs(r2[2])) {
		SWAP(r3, r2);
	}
	ERR_FAIL_COND(0.0 == r2[2]);

	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3];
	r3[4] -= m3 * r2[4];
	r3[5] -= m3 * r2[5];
	r3[6] -= m3 * r2[6];
	r3[7] -= m3 * r2[7];

	/* last check */
	ERR_FAIL_COND(0.0 == r3[3]);

	s = 1.0 / r3[3]; /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;

	m2 = r2[3]; /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2);
	r2[5] = s * (r2[5] - r3[5] * m2);
	r2[6] = s * (r2[6] - r3[6] * m2);
	r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1;
	r1[5] -= r3[5] * m1;
	r1[6] -= r3[6] * m1;
	r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0;
	r0[5] -= r3[5] * m0;
	r0[6] -= r3[6] * m0;
	r0[7] -= r3[7] * m0;

	m1 = r1[2]; /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1);
	r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1);
	r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0;
	r0[5] -= r2[5] * m0;
	r0[6] -= r2[6] * m0;
	r0[7] -= r2[7] * m0;

	m0 = r0[1]; /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0);
	r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0);
	r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5];
	MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7];
	MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5];
	MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7];
	MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5];
	MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7];
	MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5];
	MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

#undef MAT

	*this = temp;
}

void Projection::flip_y() {
	for (int i = 0; i < 4; i++) {
		columns[1][i] = -columns[1][i];
	}
}

bool Projection::is_same(const Projection &p_cam) const {
	return columns[0].is_same(p_cam.columns[0]) && columns[1].is_same(p_cam.columns[1]) && columns[2].is_same(p_cam.columns[2]) && columns[3].is_same(p_cam.columns[3]);
}

void Projection::set_depth_correction(bool p_flip_y, bool p_reverse_z, bool p_remap_z) {
	// p_remap_z is used to convert from OpenGL-style clip space (-1 - 1) to Vulkan style (0 - 1).
	real_t *m = &columns[0][0];

	m[0] = 1;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_flip_y ? -1 : 1;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = p_remap_z ? (p_reverse_z ? -0.5 : 0.5) : (p_reverse_z ? -1.0 : 1.0);
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = p_remap_z ? 0.5 : 0.0;
	m[15] = 1.0;
}

void Projection::set_light_bias() {
	real_t *m = &columns[0][0];

	m[0] = 0.5;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = 0.5;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 0.5;
	m[11] = 0.0;
	m[12] = 0.5;
	m[13] = 0.5;
	m[14] = 0.5;
	m[15] = 1.0;
}

void Projection::set_light_atlas_rect(const Rect2 &p_rect) {
	real_t *m = &columns[0][0];

	m[0] = p_rect.size.width;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_rect.size.height;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 1.0;
	m[11] = 0.0;
	m[12] = p_rect.position.x;
	m[13] = p_rect.position.y;
	m[14] = 0.0;
	m[15] = 1.0;
}

Projection::operator String() const {
	return "[X: " + columns[0].operator String() +
			", Y: " + columns[1].operator String() +
			", Z: " + columns[2].operator String() +
			", W: " + columns[3].operator String() + "]";
}

real_t Projection::get_aspect() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return columns[1][1] / columns[0][0];
}

int Projection::get_pixels_per_meter(int p_for_pixel_width) const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	real_t width = 2 * (-get_z_near() * columns[2][3] + columns[3][3]) / columns[0][0];
	return p_for_pixel_width / width; // Note : return type should be real_t (kept as int for compatibility for now).
}

bool Projection::is_orthogonal() const {
	// NOTE: This assumes that the matrix is a projection across z-axis
	// i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0
	return columns[2][3] == 0.0;
}

real_t Projection::get_fov() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	if (columns[2][0] == 0) {
		return Math::rad_to_deg(2 * Math::atan2(1, columns[0][0]));
	} else {
		// The frustum is asymmetrical so we need to calculate the left and right angles separately.
		real_t right = Math::atan2(columns[2][0] + 1, columns[0][0]);
		real_t left = Math::atan2(columns[2][0] - 1, columns[0][0]);
		return Math::rad_to_deg(right - left);
	}
}

real_t Projection::get_lod_multiplier() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return 2 / columns[0][0];
}

void Projection::make_scale(const Vector3 &p_scale) {
	set_identity();
	columns[0][0] = p_scale.x;
	columns[1][1] = p_scale.y;
	columns[2][2] = p_scale.z;
}

void Projection::scale_translate_to_fit(const AABB &p_aabb) {
	Vector3 min = p_aabb.position;
	Vector3 max = p_aabb.position + p_aabb.size;

	columns[0][0] = 2 / (max.x - min.x);
	columns[1][0] = 0;
	columns[2][0] = 0;
	columns[3][0] = -(max.x + min.x) / (max.x - min.x);

	columns[0][1] = 0;
	columns[1][1] = 2 / (max.y - min.y);
	columns[2][1] = 0;
	columns[3][1] = -(max.y + min.y) / (max.y - min.y);

	columns[0][2] = 0;
	columns[1][2] = 0;
	columns[2][2] = 2 / (max.z - min.z);
	columns[3][2] = -(max.z + min.z) / (max.z - min.z);

	columns[0][3] = 0;
	columns[1][3] = 0;
	columns[2][3] = 0;
	columns[3][3] = 1;
}

void Projection::add_jitter_offset(const Vector2 &p_offset) {
	columns[3][0] += p_offset.x;
	columns[3][1] += p_offset.y;
}

Projection::operator Transform3D() const {
	Transform3D tr;
	const real_t *m = &columns[0][0];

	tr.basis.rows[0][0] = m[0];
	tr.basis.rows[1][0] = m[1];
	tr.basis.rows[2][0] = m[2];

	tr.basis.rows[0][1] = m[4];
	tr.basis.rows[1][1] = m[5];
	tr.basis.rows[2][1] = m[6];

	tr.basis.rows[0][2] = m[8];
	tr.basis.rows[1][2] = m[9];
	tr.basis.rows[2][2] = m[10];

	tr.origin.x = m[12];
	tr.origin.y = m[13];
	tr.origin.z = m[14];

	return tr;
}

Projection::Projection(const Transform3D &p_transform) {
	const Transform3D &tr = p_transform;
	real_t *m = &columns[0][0];

	m[0] = tr.basis.rows[0][0];
	m[1] = tr.basis.rows[1][0];
	m[2] = tr.basis.rows[2][0];
	m[3] = 0.0;
	m[4] = tr.basis.rows[0][1];
	m[5] = tr.basis.rows[1][1];
	m[6] = tr.basis.rows[2][1];
	m[7] = 0.0;
	m[8] = tr.basis.rows[0][2];
	m[9] = tr.basis.rows[1][2];
	m[10] = tr.basis.rows[2][2];
	m[11] = 0.0;
	m[12] = tr.origin.x;
	m[13] = tr.origin.y;
	m[14] = tr.origin.z;
	m[15] = 1.0;
}

// --- transform_interpolator.cpp ---

void TransformInterpolator::interpolate_transform_2d(const Transform2D &p_prev, const Transform2D &p_curr, Transform2D &r_result, real_t p_fraction) {
	// Special case for physics interpolation, if flipping, don't interpolate basis.
	// If the determinant polarity changes, the handedness of the coordinate system changes.
	if (_sign(p_prev.determinant()) != _sign(p_curr.determinant())) {
		r_result.columns[0] = p_curr.columns[0];
		r_result.columns[1] = p_curr.columns[1];
		r_result.set_origin(p_prev.get_origin().lerp(p_curr.get_origin(), p_fraction));
		return;
	}

	r_result = p_prev.interpolate_with(p_curr, p_fraction);
}

void TransformInterpolator::interpolate_transform_3d(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis(p_prev.basis, p_curr.basis, r_result.basis, p_fraction);
}

void TransformInterpolator::interpolate_basis(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	Method method = find_method(p_prev, p_curr);
	interpolate_basis_via_method(p_prev, p_curr, r_result, p_fraction, method);
}

void TransformInterpolator::interpolate_transform_3d_via_method(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction, Method p_method) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis_via_method(p_prev.basis, p_curr.basis, r_result.basis, p_fraction, p_method);
}

void TransformInterpolator::interpolate_basis_via_method(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction, Method p_method) {
	switch (p_method) {
		default: {
			interpolate_basis_linear(p_prev, p_curr, r_result, p_fraction);
		} break;
		case INTERP_SLERP: {
			r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);
		} break;
		case INTERP_SCALED_SLERP: {
			interpolate_basis_scaled_slerp(p_prev, p_curr, r_result, p_fraction);
		} break;
	}
}

Quaternion TransformInterpolator::_basis_to_quat_unchecked(const Basis &p_basis) {
	Basis m = p_basis;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

Quaternion TransformInterpolator::_quat_slerp_unchecked(const Quaternion &p_from, const Quaternion &p_to, real_t p_fraction) {
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// Calculate cosine.
	cosom = p_from.dot(p_to);

	// Adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1.x = -p_to.x;
		to1.y = -p_to.y;
		to1.z = -p_to.z;
		to1.w = -p_to.w;
	} else {
		to1.x = p_to.x;
		to1.y = p_to.y;
		to1.z = p_to.z;
		to1.w = p_to.w;
	}

	// Calculate coefficients.

	// This check could possibly be removed as we dealt with this
	// case in the find_method() function, but is left for safety, it probably
	// isn't a bottleneck.
	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0f - p_fraction) * omega) / sinom;
		scale1 = Math::sin(p_fraction * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_fraction;
		scale1 = p_fraction;
	}
	// Calculate final values.
	return Quaternion(
			scale0 * p_from.x + scale1 * to1.x,
			scale0 * p_from.y + scale1 * to1.y,
			scale0 * p_from.z + scale1 * to1.z,
			scale0 * p_from.w + scale1 * to1.w);
}

Basis TransformInterpolator::_basis_slerp_unchecked(Basis p_from, Basis p_to, real_t p_fraction) {
	Quaternion from = _basis_to_quat_unchecked(p_from);
	Quaternion to = _basis_to_quat_unchecked(p_to);

	Basis b(_quat_slerp_unchecked(from, to, p_fraction));
	return b;
}

void TransformInterpolator::interpolate_basis_scaled_slerp(Basis p_prev, Basis p_curr, Basis &r_result, real_t p_fraction) {
	// Normalize both and find lengths.
	Vector3 lengths_prev = _basis_orthonormalize(p_prev);
	Vector3 lengths_curr = _basis_orthonormalize(p_curr);

	r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);

	// Now the result is unit length basis, we need to scale.
	Vector3 lengths_lerped = lengths_prev + ((lengths_curr - lengths_prev) * p_fraction);

	// Keep a note that the column / row order of the basis is weird,
	// so keep an eye for bugs with this.
	r_result[0] *= lengths_lerped;
	r_result[1] *= lengths_lerped;
	r_result[2] *= lengths_lerped;
}

void TransformInterpolator::interpolate_basis_linear(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	// Interpolate basis.
	r_result = p_prev.lerp(p_curr, p_fraction);

	// It turns out we need to guard against zero scale basis.
	// This is kind of silly, as we should probably fix the bugs elsewhere in Godot that can't deal with
	// zero scale, but until that time...
	for (int n = 0; n < 3; n++) {
		Vector3 &axis = r_result[n];

		// Not ok, this could cause errors due to bugs elsewhere,
		// so we will bodge set this to a small value.
		const real_t smallest = 0.0001f;
		const real_t smallest_squared = smallest * smallest;
		if (axis.length_squared() < smallest_squared) {
			// Setting a different component to the smallest
			// helps prevent the situation where all the axes are pointing in the same direction,
			// which could be a problem for e.g. cross products...
			axis[n] = smallest;
		}
	}
}

// Returns length.
real_t TransformInterpolator::_vec3_normalize(Vector3 &p_vec) {
	real_t lengthsq = p_vec.length_squared();
	if (lengthsq == 0.0f) {
		p_vec.x = p_vec.y = p_vec.z = 0.0f;
		return 0.0f;
	}
	real_t length = Math::sqrt(lengthsq);
	p_vec.x /= length;
	p_vec.y /= length;
	p_vec.z /= length;
	return length;
}

// Returns lengths.
Vector3 TransformInterpolator::_basis_orthonormalize(Basis &r_basis) {
	// Gram-Schmidt Process.

	Vector3 x = r_basis.get_column(0);
	Vector3 y = r_basis.get_column(1);
	Vector3 z = r_basis.get_column(2);

	Vector3 lengths;

	lengths.x = _vec3_normalize(x);
	y = (y - x * (x.dot(y)));
	lengths.y = _vec3_normalize(y);
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	lengths.z = _vec3_normalize(z);

	r_basis.set_column(0, x);
	r_basis.set_column(1, y);
	r_basis.set_column(2, z);

	return lengths;
}

TransformInterpolator::Method TransformInterpolator::_test_basis(Basis p_basis, bool r_needed_normalize, Quaternion &r_quat) {
	// Axis lengths.
	Vector3 al = Vector3(p_basis.get_column(0).length_squared(),
			p_basis.get_column(1).length_squared(),
			p_basis.get_column(2).length_squared());

	// Non unit scale?
	if (r_needed_normalize || !_vec3_is_equal_approx(al, Vector3(1.0, 1.0, 1.0), (real_t)0.001f)) {
		// If the basis is not normalized (at least approximately), it will fail the checks needed for slerp.
		// So we try to detect a scaled (but not sheared) basis, which we *can* slerp by normalizing first,
		// and lerping the scales separately.

		// If any of the axes are really small, it is unlikely to be a valid rotation, or is scaled too small to deal with float error.
		const real_t sl_epsilon = 0.00001f;
		if ((al.x < sl_epsilon) ||
				(al.y < sl_epsilon) ||
				(al.z < sl_epsilon)) {
			return INTERP_LERP;
		}

		// Normalize the basis.
		Basis norm_basis = p_basis;

		al.x = Math::sqrt(al.x);
		al.y = Math::sqrt(al.y);
		al.z = Math::sqrt(al.z);

		norm_basis.set_column(0, norm_basis.get_column(0) / al.x);
		norm_basis.set_column(1, norm_basis.get_column(1) / al.y);
		norm_basis.set_column(2, norm_basis.get_column(2) / al.z);

		// This doesn't appear necessary, as the later checks will catch it.
		// if (!_basis_is_orthogonal_any_scale(norm_basis)) {
		// return INTERP_LERP;
		// }

		p_basis = norm_basis;

		// Orthonormalize not necessary as normal normalization(!) works if the
		// axes are orthonormal.
		// p_basis.orthonormalize();

		// If we needed to normalize one of the two bases, we will need to normalize both,
		// regardless of whether the 2nd needs it, just to make sure it takes the path to return
		// INTERP_SCALED_LERP on the 2nd call of _test_basis.
		r_needed_normalize = true;
	}

	// Apply less stringent tests than the built in slerp, the standard Godot slerp
	// is too susceptible to float error to be useful.
	real_t det = p_basis.determinant();
	if (!Math::is_equal_approx(det, 1, (real_t)0.01f)) {
		return INTERP_LERP;
	}

	if (!_basis_is_orthogonal(p_basis)) {
		return INTERP_LERP;
	}

	// TODO: This could possibly be less stringent too, check this.
	r_quat = _basis_to_quat_unchecked(p_basis);
	if (!r_quat.is_normalized()) {
		return INTERP_LERP;
	}

	return r_needed_normalize ? INTERP_SCALED_SLERP : INTERP_SLERP;
}

// This check doesn't seem to be needed but is preserved in case of bugs.
bool TransformInterpolator::_basis_is_orthogonal_any_scale(const Basis &p_basis) {
	Vector3 cross = p_basis.get_column(0).cross(p_basis.get_column(1));
	real_t l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	const real_t epsilon = 0.9995f;

	real_t dot = cross.dot(p_basis.get_column(2));
	if (dot < epsilon) {
		return false;
	}

	cross = p_basis.get_column(1).cross(p_basis.get_column(2));
	l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	dot = cross.dot(p_basis.get_column(0));
	if (dot < epsilon) {
		return false;
	}

	return true;
}

bool TransformInterpolator::_basis_is_orthogonal(const Basis &p_basis, real_t p_epsilon) {
	Basis identity;
	Basis m = p_basis * p_basis.transposed();

	// Less stringent tests than the standard Godot slerp.
	if (!_vec3_is_equal_approx(m[0], identity[0], p_epsilon) || !_vec3_is_equal_approx(m[1], identity[1], p_epsilon) || !_vec3_is_equal_approx(m[2], identity[2], p_epsilon)) {
		return false;
	}
	return true;
}

real_t TransformInterpolator::checksum_transform_3d(const Transform3D &p_transform) {
	// just a really basic checksum, this can probably be improved
	real_t sum = _vec3_sum(p_transform.origin);
	sum -= _vec3_sum(p_transform.basis.rows[0]);
	sum += _vec3_sum(p_transform.basis.rows[1]);
	sum -= _vec3_sum(p_transform.basis.rows[2]);
	return sum;
}

TransformInterpolator::Method TransformInterpolator::find_method(const Basis &p_a, const Basis &p_b) {
	bool needed_normalize = false;

	Quaternion q0;
	Method method = _test_basis(p_a, needed_normalize, q0);
	if (method == INTERP_LERP) {
		return method;
	}

	Quaternion q1;
	method = _test_basis(p_b, needed_normalize, q1);
	if (method == INTERP_LERP) {
		return method;
	}

	// Are they close together?
	// Apply the same test that will revert to lerp as is present in the slerp routine.
	// Calculate cosine.
	real_t cosom = Math::abs(q0.dot(q1));
	if ((1.0f - cosom) <= (real_t)CMP_EPSILON) {
		return INTERP_LERP;
	}

	return method;
}
