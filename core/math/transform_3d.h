/**************************************************************************/
/*  transform_3d.h                                                        */
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

#include "core/math/aabb.h"
// --- Inlined/Folded from core/math/basis.h ---
// --- Inlined/Folded from core/math/quaternion.h ---
#include "core/math/math_funcs.h"
#include "core/math/vector3.h"
#include "core/string/ustring.h"

struct [[nodiscard]] Quaternion {
	real_t x = 0.0f;
	real_t y = 0.0f;
	real_t z = 0.0f;
	real_t w = 1.0f;

	constexpr real_t &operator[](int p_idx) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Quaternion, x) == 0 * sizeof(real_t));
		static_assert(offsetof(Quaternion, y) == 1 * sizeof(real_t));
		static_assert(offsetof(Quaternion, z) == 2 * sizeof(real_t));
		static_assert(offsetof(Quaternion, w) == 3 * sizeof(real_t));
		static_assert(sizeof(Quaternion) == 4 * sizeof(real_t));

		DEV_ASSERT((unsigned int)p_idx < 4);
		return (&x)[p_idx];
	}
	constexpr const real_t &operator[](int p_idx) const {
		DEV_ASSERT((unsigned int)p_idx < 4);
		return (&x)[p_idx];
	}

	_FORCE_INLINE_ real_t length_squared() const;
	bool is_equal_approx(const Quaternion &p_quaternion) const;
	bool is_same(const Quaternion &p_quaternion) const;
	bool is_finite() const;
	real_t length() const;
	void normalize();
	Quaternion normalized() const;
	bool is_normalized() const;
	Quaternion inverse() const;
	Quaternion log() const;
	Quaternion exp() const;
	_FORCE_INLINE_ real_t dot(const Quaternion &p_q) const;
	real_t angle_to(const Quaternion &p_to) const;

	Vector3 get_euler(EulerOrder p_order = EulerOrder::YXZ) const;
	static Quaternion from_euler(const Vector3 &p_euler);

	Quaternion slerp(const Quaternion &p_to, real_t p_weight) const;
	Quaternion slerpni(const Quaternion &p_to, real_t p_weight) const;
	Quaternion spherical_cubic_interpolate(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight) const;
	Quaternion spherical_cubic_interpolate_in_time(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const;

	Vector3 get_axis() const;
	real_t get_angle() const;

	_FORCE_INLINE_ void get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
		r_angle = 2 * Math::acos(w);
		real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
		r_axis.x = x * r;
		r_axis.y = y * r;
		r_axis.z = z * r;
	}

	constexpr void operator*=(const Quaternion &p_q);
	constexpr Quaternion operator*(const Quaternion &p_q) const;

	_FORCE_INLINE_ Vector3 xform(const Vector3 &p_v) const {
#ifdef MATH_CHECKS
		ERR_FAIL_COND_V_MSG(!is_normalized(), p_v, "The quaternion " + operator String() + " must be normalized.");
#endif
		Vector3 u(x, y, z);
		Vector3 uv = u.cross(p_v);
		return p_v + ((uv * w) + u.cross(uv)) * ((real_t)2);
	}

	_FORCE_INLINE_ Vector3 xform_inv(const Vector3 &p_v) const {
		return inverse().xform(p_v);
	}

	constexpr void operator+=(const Quaternion &p_q);
	constexpr void operator-=(const Quaternion &p_q);
	constexpr void operator*=(real_t p_s);
	constexpr void operator/=(real_t p_s);
	constexpr Quaternion operator+(const Quaternion &p_q2) const;
	constexpr Quaternion operator-(const Quaternion &p_q2) const;
	constexpr Quaternion operator-() const;
	constexpr Quaternion operator*(real_t p_s) const;
	constexpr Quaternion operator/(real_t p_s) const;

	constexpr bool operator==(const Quaternion &p_quaternion) const;
	constexpr bool operator!=(const Quaternion &p_quaternion) const;

	explicit operator String() const;

	constexpr Quaternion() = default;

	constexpr Quaternion(real_t p_x, real_t p_y, real_t p_z, real_t p_w) :
			x(p_x), y(p_y), z(p_z), w(p_w) {}

	Quaternion(const Vector3 &p_axis, real_t p_angle);

	Quaternion(const Vector3 &p_v0, const Vector3 &p_v1) { // Shortest arc.
#ifdef MATH_CHECKS
		ERR_FAIL_COND_MSG(p_v0.is_zero_approx() || p_v1.is_zero_approx(), "The vectors must not be zero.");
#endif
#ifdef REAL_T_IS_DOUBLE
		constexpr real_t ALMOST_ONE = 0.999999999999999;
#else
		constexpr real_t ALMOST_ONE = 0.99999975f;
#endif
		Vector3 n0 = p_v0.normalized();
		Vector3 n1 = p_v1.normalized();
		real_t d = n0.dot(n1);
		if (Math::abs(d) > ALMOST_ONE) {
			if (d >= 0) {
				return; // Vectors are same.
			}
			Vector3 axis = n0.get_any_perpendicular();
			x = axis.x;
			y = axis.y;
			z = axis.z;
			w = 0;
		} else {
			Vector3 c = n0.cross(n1);
			real_t s = Math::sqrt((1.0f + d) * 2.0f);
			real_t rs = 1.0f / s;

			x = c.x * rs;
			y = c.y * rs;
			z = c.z * rs;
			w = s * 0.5f;
		}
		normalize();
	}
};

real_t Quaternion::dot(const Quaternion &p_q) const {
	return x * p_q.x + y * p_q.y + z * p_q.z + w * p_q.w;
}

real_t Quaternion::length_squared() const {
	return dot(*this);
}

constexpr void Quaternion::operator+=(const Quaternion &p_q) {
	x += p_q.x;
	y += p_q.y;
	z += p_q.z;
	w += p_q.w;
}

constexpr void Quaternion::operator-=(const Quaternion &p_q) {
	x -= p_q.x;
	y -= p_q.y;
	z -= p_q.z;
	w -= p_q.w;
}

constexpr void Quaternion::operator*=(real_t p_s) {
	x *= p_s;
	y *= p_s;
	z *= p_s;
	w *= p_s;
}

constexpr void Quaternion::operator/=(real_t p_s) {
	*this *= (1 / p_s);
}

constexpr Quaternion Quaternion::operator+(const Quaternion &p_q2) const {
	const Quaternion &q1 = *this;
	return Quaternion(q1.x + p_q2.x, q1.y + p_q2.y, q1.z + p_q2.z, q1.w + p_q2.w);
}

constexpr Quaternion Quaternion::operator-(const Quaternion &p_q2) const {
	const Quaternion &q1 = *this;
	return Quaternion(q1.x - p_q2.x, q1.y - p_q2.y, q1.z - p_q2.z, q1.w - p_q2.w);
}

constexpr Quaternion Quaternion::operator-() const {
	const Quaternion &q2 = *this;
	return Quaternion(-q2.x, -q2.y, -q2.z, -q2.w);
}

constexpr Quaternion Quaternion::operator*(real_t p_s) const {
	return Quaternion(x * p_s, y * p_s, z * p_s, w * p_s);
}

constexpr Quaternion Quaternion::operator/(real_t p_s) const {
	return *this * (1 / p_s);
}

constexpr bool Quaternion::operator==(const Quaternion &p_quaternion) const {
	return x == p_quaternion.x && y == p_quaternion.y && z == p_quaternion.z && w == p_quaternion.w;
}

constexpr bool Quaternion::operator!=(const Quaternion &p_quaternion) const {
	return x != p_quaternion.x || y != p_quaternion.y || z != p_quaternion.z || w != p_quaternion.w;
}

constexpr void Quaternion::operator*=(const Quaternion &p_q) {
	real_t xx = w * p_q.x + x * p_q.w + y * p_q.z - z * p_q.y;
	real_t yy = w * p_q.y + y * p_q.w + z * p_q.x - x * p_q.z;
	real_t zz = w * p_q.z + z * p_q.w + x * p_q.y - y * p_q.x;
	w = w * p_q.w - x * p_q.x - y * p_q.y - z * p_q.z;
	x = xx;
	y = yy;
	z = zz;
}

constexpr Quaternion Quaternion::operator*(const Quaternion &p_q) const {
	Quaternion r = *this;
	r *= p_q;
	return r;
}

constexpr Quaternion operator*(real_t p_real, const Quaternion &p_quaternion) {
	return p_quaternion * p_real;
}
#include "core/math/vector3.h"

struct [[nodiscard]] Basis {
	static const Basis FLIP_X;
	static const Basis FLIP_Y;
	static const Basis FLIP_Z;

	Vector3 rows[3] = {
		Vector3(1, 0, 0),
		Vector3(0, 1, 0),
		Vector3(0, 0, 1)
	};

	constexpr const Vector3 &operator[](int p_row) const {
		return rows[p_row];
	}
	constexpr Vector3 &operator[](int p_row) {
		return rows[p_row];
	}

	void invert();
	void transpose();

	Basis inverse() const;
	Basis transposed() const;

	_FORCE_INLINE_ real_t determinant() const;

	void rotate(const Vector3 &p_axis, real_t p_angle);
	Basis rotated(const Vector3 &p_axis, real_t p_angle) const;

	void rotate_local(const Vector3 &p_axis, real_t p_angle);
	Basis rotated_local(const Vector3 &p_axis, real_t p_angle) const;

	void rotate(const Vector3 &p_euler, EulerOrder p_order = EulerOrder::YXZ);
	Basis rotated(const Vector3 &p_euler, EulerOrder p_order = EulerOrder::YXZ) const;

	void rotate(const Quaternion &p_quaternion);
	Basis rotated(const Quaternion &p_quaternion) const;

	Vector3 get_euler_normalized(EulerOrder p_order = EulerOrder::YXZ) const;
	void get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const;
	void get_rotation_axis_angle_local(Vector3 &p_axis, real_t &p_angle) const;
	Quaternion get_rotation_quaternion() const;

	void rotate_to_align(Vector3 p_start_direction, Vector3 p_end_direction);

	Vector3 rotref_posscale_decomposition(Basis &r_rotref) const;

	Vector3 get_euler(EulerOrder p_order = EulerOrder::YXZ) const;
	void set_euler(const Vector3 &p_euler, EulerOrder p_order = EulerOrder::YXZ);
	static Basis from_euler(const Vector3 &p_euler, EulerOrder p_order = EulerOrder::YXZ) {
		Basis b;
		b.set_euler(p_euler, p_order);
		return b;
	}

	Quaternion get_quaternion() const;
	void set_quaternion(const Quaternion &p_quaternion);

	void get_axis_angle(Vector3 &r_axis, real_t &r_angle) const;
	void set_axis_angle(const Vector3 &p_axis, real_t p_angle);

	void scale(const Vector3 &p_scale);
	Basis scaled(const Vector3 &p_scale) const;

	void scale_local(const Vector3 &p_scale);
	Basis scaled_local(const Vector3 &p_scale) const;

	void scale_orthogonal(const Vector3 &p_scale);
	Basis scaled_orthogonal(const Vector3 &p_scale) const;
	real_t get_uniform_scale() const;

	Vector3 get_scale() const;
	Vector3 get_scale_abs() const;
	Vector3 get_scale_global() const;

	void set_axis_angle_scale(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale);
	void set_euler_scale(const Vector3 &p_euler, const Vector3 &p_scale, EulerOrder p_order = EulerOrder::YXZ);
	void set_quaternion_scale(const Quaternion &p_quaternion, const Vector3 &p_scale);

	// transposed dot products
	_FORCE_INLINE_ real_t tdotx(const Vector3 &p_v) const {
		return rows[0][0] * p_v[0] + rows[1][0] * p_v[1] + rows[2][0] * p_v[2];
	}
	_FORCE_INLINE_ real_t tdoty(const Vector3 &p_v) const {
		return rows[0][1] * p_v[0] + rows[1][1] * p_v[1] + rows[2][1] * p_v[2];
	}
	_FORCE_INLINE_ real_t tdotz(const Vector3 &p_v) const {
		return rows[0][2] * p_v[0] + rows[1][2] * p_v[1] + rows[2][2] * p_v[2];
	}

	bool is_equal_approx(const Basis &p_basis) const;
	bool is_same(const Basis &p_basis) const;
	bool is_finite() const;

	constexpr bool operator==(const Basis &p_matrix) const;
	constexpr bool operator!=(const Basis &p_matrix) const;

	_FORCE_INLINE_ Vector3 xform(const Vector3 &p_vector) const;
	_FORCE_INLINE_ Vector3 xform_inv(const Vector3 &p_vector) const;
	_FORCE_INLINE_ void operator*=(const Basis &p_matrix);
	_FORCE_INLINE_ Basis operator*(const Basis &p_matrix) const;
	constexpr void operator+=(const Basis &p_matrix);
	constexpr Basis operator+(const Basis &p_matrix) const;
	constexpr void operator-=(const Basis &p_matrix);
	constexpr Basis operator-(const Basis &p_matrix) const;
	constexpr void operator*=(real_t p_val);
	constexpr Basis operator*(real_t p_val) const;
	constexpr void operator/=(real_t p_val);
	constexpr Basis operator/(real_t p_val) const;

	bool is_orthogonal() const;
	bool is_orthonormal() const;
	bool is_conformal() const;
	bool is_diagonal() const;
	bool is_rotation() const;

	Basis lerp(const Basis &p_to, real_t p_weight) const;
	Basis slerp(const Basis &p_to, real_t p_weight) const;
	void rotate_sh(real_t *p_values);

	explicit operator String() const;

	/* create / set */

	_FORCE_INLINE_ void set(real_t p_xx, real_t p_xy, real_t p_xz, real_t p_yx, real_t p_yy, real_t p_yz, real_t p_zx, real_t p_zy, real_t p_zz) {
		rows[0][0] = p_xx;
		rows[0][1] = p_xy;
		rows[0][2] = p_xz;
		rows[1][0] = p_yx;
		rows[1][1] = p_yy;
		rows[1][2] = p_yz;
		rows[2][0] = p_zx;
		rows[2][1] = p_zy;
		rows[2][2] = p_zz;
	}
	_FORCE_INLINE_ void set_columns(const Vector3 &p_x, const Vector3 &p_y, const Vector3 &p_z) {
		set_column(0, p_x);
		set_column(1, p_y);
		set_column(2, p_z);
	}

	_FORCE_INLINE_ Vector3 get_column(int p_index) const {
		// Get actual basis axis column (we store transposed as rows for performance).
		return Vector3(rows[0][p_index], rows[1][p_index], rows[2][p_index]);
	}

	_FORCE_INLINE_ void set_column(int p_index, const Vector3 &p_value) {
		// Set actual basis axis column (we store transposed as rows for performance).
		rows[0][p_index] = p_value.x;
		rows[1][p_index] = p_value.y;
		rows[2][p_index] = p_value.z;
	}

	_FORCE_INLINE_ Vector3 get_main_diagonal() const {
		return Vector3(rows[0][0], rows[1][1], rows[2][2]);
	}

	_FORCE_INLINE_ void set_zero() {
		rows[0].zero();
		rows[1].zero();
		rows[2].zero();
	}

	_FORCE_INLINE_ Basis transpose_xform(const Basis &p_m) const {
		return Basis(
				rows[0].x * p_m[0].x + rows[1].x * p_m[1].x + rows[2].x * p_m[2].x,
				rows[0].x * p_m[0].y + rows[1].x * p_m[1].y + rows[2].x * p_m[2].y,
				rows[0].x * p_m[0].z + rows[1].x * p_m[1].z + rows[2].x * p_m[2].z,
				rows[0].y * p_m[0].x + rows[1].y * p_m[1].x + rows[2].y * p_m[2].x,
				rows[0].y * p_m[0].y + rows[1].y * p_m[1].y + rows[2].y * p_m[2].y,
				rows[0].y * p_m[0].z + rows[1].y * p_m[1].z + rows[2].y * p_m[2].z,
				rows[0].z * p_m[0].x + rows[1].z * p_m[1].x + rows[2].z * p_m[2].x,
				rows[0].z * p_m[0].y + rows[1].z * p_m[1].y + rows[2].z * p_m[2].y,
				rows[0].z * p_m[0].z + rows[1].z * p_m[1].z + rows[2].z * p_m[2].z);
	}
	constexpr Basis(real_t p_xx, real_t p_xy, real_t p_xz, real_t p_yx, real_t p_yy, real_t p_yz, real_t p_zx, real_t p_zy, real_t p_zz) :
			rows{
				{ p_xx, p_xy, p_xz },
				{ p_yx, p_yy, p_yz },
				{ p_zx, p_zy, p_zz },
			} {}

	void orthonormalize();
	Basis orthonormalized() const;

	void orthogonalize();
	Basis orthogonalized() const;

#ifdef MATH_CHECKS
	bool is_symmetric() const;
#endif
	Basis diagonalize();

	operator Quaternion() const { return get_quaternion(); }

	static Basis looking_at(const Vector3 &p_target, const Vector3 &p_up = Vector3::UP, bool p_use_model_front = false);

	Basis(const Quaternion &p_quaternion) { set_quaternion(p_quaternion); }
	Basis(const Quaternion &p_quaternion, const Vector3 &p_scale) { set_quaternion_scale(p_quaternion, p_scale); }

	Basis(const Vector3 &p_axis, real_t p_angle) { set_axis_angle(p_axis, p_angle); }
	Basis(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale) { set_axis_angle_scale(p_axis, p_angle, p_scale); }
	static Basis from_scale(const Vector3 &p_scale);

	constexpr Basis(const Vector3 &p_x_axis, const Vector3 &p_y_axis, const Vector3 &p_z_axis) :
			rows{
				{ p_x_axis.x, p_y_axis.x, p_z_axis.x },
				{ p_x_axis.y, p_y_axis.y, p_z_axis.y },
				{ p_x_axis.z, p_y_axis.z, p_z_axis.z },
			} {}

	Basis() = default;

private:
	// Helper method.
	void _set_diagonal(const Vector3 &p_diag);
};

inline constexpr Basis Basis::FLIP_X = { { -1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
inline constexpr Basis Basis::FLIP_Y = { { 1, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 } };
inline constexpr Basis Basis::FLIP_Z = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, -1 } };

constexpr bool Basis::operator==(const Basis &p_matrix) const {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (rows[i][j] != p_matrix.rows[i][j]) {
				return false;
			}
		}
	}

	return true;
}

constexpr bool Basis::operator!=(const Basis &p_matrix) const {
	return (!(*this == p_matrix));
}

_FORCE_INLINE_ void Basis::operator*=(const Basis &p_matrix) {
	set(
			p_matrix.tdotx(rows[0]), p_matrix.tdoty(rows[0]), p_matrix.tdotz(rows[0]),
			p_matrix.tdotx(rows[1]), p_matrix.tdoty(rows[1]), p_matrix.tdotz(rows[1]),
			p_matrix.tdotx(rows[2]), p_matrix.tdoty(rows[2]), p_matrix.tdotz(rows[2]));
}

_FORCE_INLINE_ Basis Basis::operator*(const Basis &p_matrix) const {
	return Basis(
			p_matrix.tdotx(rows[0]), p_matrix.tdoty(rows[0]), p_matrix.tdotz(rows[0]),
			p_matrix.tdotx(rows[1]), p_matrix.tdoty(rows[1]), p_matrix.tdotz(rows[1]),
			p_matrix.tdotx(rows[2]), p_matrix.tdoty(rows[2]), p_matrix.tdotz(rows[2]));
}

constexpr void Basis::operator+=(const Basis &p_matrix) {
	rows[0] += p_matrix.rows[0];
	rows[1] += p_matrix.rows[1];
	rows[2] += p_matrix.rows[2];
}

constexpr Basis Basis::operator+(const Basis &p_matrix) const {
	Basis ret(*this);
	ret += p_matrix;
	return ret;
}

constexpr void Basis::operator-=(const Basis &p_matrix) {
	rows[0] -= p_matrix.rows[0];
	rows[1] -= p_matrix.rows[1];
	rows[2] -= p_matrix.rows[2];
}

constexpr Basis Basis::operator-(const Basis &p_matrix) const {
	Basis ret(*this);
	ret -= p_matrix;
	return ret;
}

constexpr void Basis::operator*=(real_t p_val) {
	rows[0] *= p_val;
	rows[1] *= p_val;
	rows[2] *= p_val;
}

constexpr Basis Basis::operator*(real_t p_val) const {
	Basis ret(*this);
	ret *= p_val;
	return ret;
}

constexpr void Basis::operator/=(real_t p_val) {
	rows[0] /= p_val;
	rows[1] /= p_val;
	rows[2] /= p_val;
}

constexpr Basis Basis::operator/(real_t p_val) const {
	Basis ret(*this);
	ret /= p_val;
	return ret;
}

Vector3 Basis::xform(const Vector3 &p_vector) const {
	return Vector3(
			rows[0].dot(p_vector),
			rows[1].dot(p_vector),
			rows[2].dot(p_vector));
}

Vector3 Basis::xform_inv(const Vector3 &p_vector) const {
	return Vector3(
			(rows[0][0] * p_vector.x) + (rows[1][0] * p_vector.y) + (rows[2][0] * p_vector.z),
			(rows[0][1] * p_vector.x) + (rows[1][1] * p_vector.y) + (rows[2][1] * p_vector.z),
			(rows[0][2] * p_vector.x) + (rows[1][2] * p_vector.y) + (rows[2][2] * p_vector.z));
}

real_t Basis::determinant() const {
	return rows[0][0] * (rows[1][1] * rows[2][2] - rows[2][1] * rows[1][2]) -
			rows[1][0] * (rows[0][1] * rows[2][2] - rows[2][1] * rows[0][2]) +
			rows[2][0] * (rows[0][1] * rows[1][2] - rows[1][1] * rows[0][2]);
}
#include "core/math/plane.h"
#include "core/templates/vector.h"

struct [[nodiscard]] Transform3D {
	static const Transform3D FLIP_X;
	static const Transform3D FLIP_Y;
	static const Transform3D FLIP_Z;

	Basis basis;
	Vector3 origin;

	void invert();
	Transform3D inverse() const;

	void affine_invert();
	Transform3D affine_inverse() const;

	Transform3D rotated(const Vector3 &p_axis, real_t p_angle) const;
	Transform3D rotated_local(const Vector3 &p_axis, real_t p_angle) const;

	void rotate(const Vector3 &p_axis, real_t p_angle);
	void rotate_basis(const Vector3 &p_axis, real_t p_angle);

	void set_look_at(const Vector3 &p_eye, const Vector3 &p_target, const Vector3 &p_up = Vector3::UP, bool p_use_model_front = false);
	Transform3D looking_at(const Vector3 &p_target, const Vector3 &p_up = Vector3::UP, bool p_use_model_front = false) const;

	void scale(const Vector3 &p_scale);
	Transform3D scaled(const Vector3 &p_scale) const;
	Transform3D scaled_local(const Vector3 &p_scale) const;
	void scale_basis(const Vector3 &p_scale);
	void translate_local(real_t p_tx, real_t p_ty, real_t p_tz);
	void translate_local(const Vector3 &p_translation);
	Transform3D translated(const Vector3 &p_translation) const;
	Transform3D translated_local(const Vector3 &p_translation) const;

	const Basis &get_basis() const { return basis; }
	void set_basis(const Basis &p_basis) { basis = p_basis; }

	const Vector3 &get_origin() const { return origin; }
	void set_origin(const Vector3 &p_origin) { origin = p_origin; }

	void orthonormalize();
	Transform3D orthonormalized() const;
	void orthogonalize();
	Transform3D orthogonalized() const;
	bool is_equal_approx(const Transform3D &p_transform) const;
	bool is_same(const Transform3D &p_transform) const;
	bool is_finite() const;

	constexpr bool operator==(const Transform3D &p_transform) const;
	constexpr bool operator!=(const Transform3D &p_transform) const;

	_FORCE_INLINE_ Vector3 xform(const Vector3 &p_vector) const;
	_FORCE_INLINE_ AABB xform(const AABB &p_aabb) const;
	_FORCE_INLINE_ Vector<Vector3> xform(const Vector<Vector3> &p_array) const;

	// NOTE: These are UNSAFE with non-uniform scaling, and will produce incorrect results.
	// They use the transpose.
	// For safe inverse transforms, xform by the affine_inverse.
	_FORCE_INLINE_ Vector3 xform_inv(const Vector3 &p_vector) const;
	_FORCE_INLINE_ AABB xform_inv(const AABB &p_aabb) const;
	_FORCE_INLINE_ Vector<Vector3> xform_inv(const Vector<Vector3> &p_array) const;

	// Safe with non-uniform scaling (uses affine_inverse).
	_FORCE_INLINE_ Plane xform(const Plane &p_plane) const;
	_FORCE_INLINE_ Plane xform_inv(const Plane &p_plane) const;

	// These fast versions use precomputed affine inverse, and should be used in bottleneck areas where
	// multiple planes are to be transformed.
	_FORCE_INLINE_ Plane xform_fast(const Plane &p_plane, const Basis &p_basis_inverse_transpose) const;
	static _FORCE_INLINE_ Plane xform_inv_fast(const Plane &p_plane, const Transform3D &p_inverse, const Basis &p_basis_transpose);

	void operator*=(const Transform3D &p_transform);
	Transform3D operator*(const Transform3D &p_transform) const;
	constexpr void operator*=(real_t p_val);
	constexpr Transform3D operator*(real_t p_val) const;
	constexpr void operator/=(real_t p_val);
	constexpr Transform3D operator/(real_t p_val) const;

	Transform3D interpolate_with(const Transform3D &p_transform, real_t p_c) const;

	_FORCE_INLINE_ Transform3D inverse_xform(const Transform3D &p_transform) const {
		Vector3 v = p_transform.origin - origin;
		return Transform3D(basis.transpose_xform(p_transform.basis),
				basis.xform(v));
	}

	void set(real_t p_xx, real_t p_xy, real_t p_xz, real_t p_yx, real_t p_yy, real_t p_yz, real_t p_zx, real_t p_zy, real_t p_zz, real_t p_tx, real_t p_ty, real_t p_tz) {
		basis.set(p_xx, p_xy, p_xz, p_yx, p_yy, p_yz, p_zx, p_zy, p_zz);
		origin.x = p_tx;
		origin.y = p_ty;
		origin.z = p_tz;
	}

	explicit operator String() const;

	Transform3D() = default;
	constexpr Transform3D(const Basis &p_basis, const Vector3 &p_origin = Vector3()) :
			basis(p_basis),
			origin(p_origin) {}
	constexpr Transform3D(const Vector3 &p_x, const Vector3 &p_y, const Vector3 &p_z, const Vector3 &p_origin) :
			basis(p_x, p_y, p_z),
			origin(p_origin) {}
	constexpr Transform3D(real_t p_xx, real_t p_xy, real_t p_xz, real_t p_yx, real_t p_yy, real_t p_yz, real_t p_zx, real_t p_zy, real_t p_zz, real_t p_ox, real_t p_oy, real_t p_oz) :
			basis(p_xx, p_xy, p_xz, p_yx, p_yy, p_yz, p_zx, p_zy, p_zz),
			origin(p_ox, p_oy, p_oz) {}
};

inline constexpr Transform3D Transform3D::FLIP_X = { Basis::FLIP_X };
inline constexpr Transform3D Transform3D::FLIP_Y = { Basis::FLIP_Y };
inline constexpr Transform3D Transform3D::FLIP_Z = { Basis::FLIP_Z };

constexpr bool Transform3D::operator==(const Transform3D &p_transform) const {
	return (basis == p_transform.basis && origin == p_transform.origin);
}

constexpr bool Transform3D::operator!=(const Transform3D &p_transform) const {
	return (basis != p_transform.basis || origin != p_transform.origin);
}

constexpr void Transform3D::operator*=(real_t p_val) {
	origin *= p_val;
	basis *= p_val;
}

constexpr Transform3D Transform3D::operator*(real_t p_val) const {
	Transform3D ret(*this);
	ret *= p_val;
	return ret;
}

constexpr void Transform3D::operator/=(real_t p_val) {
	basis /= p_val;
	origin /= p_val;
}

constexpr Transform3D Transform3D::operator/(real_t p_val) const {
	Transform3D ret(*this);
	ret /= p_val;
	return ret;
}

_FORCE_INLINE_ Vector3 Transform3D::xform(const Vector3 &p_vector) const {
	return Vector3(
			basis[0].dot(p_vector) + origin.x,
			basis[1].dot(p_vector) + origin.y,
			basis[2].dot(p_vector) + origin.z);
}

_FORCE_INLINE_ Vector3 Transform3D::xform_inv(const Vector3 &p_vector) const {
	Vector3 v = p_vector - origin;

	return Vector3(
			(basis.rows[0][0] * v.x) + (basis.rows[1][0] * v.y) + (basis.rows[2][0] * v.z),
			(basis.rows[0][1] * v.x) + (basis.rows[1][1] * v.y) + (basis.rows[2][1] * v.z),
			(basis.rows[0][2] * v.x) + (basis.rows[1][2] * v.y) + (basis.rows[2][2] * v.z));
}

// Neither the plane regular xform or xform_inv are particularly efficient,
// as they do a basis inverse. For xforming a large number
// of planes it is better to pre-calculate the inverse transpose basis once
// and reuse it for each plane, by using the 'fast' version of the functions.
_FORCE_INLINE_ Plane Transform3D::xform(const Plane &p_plane) const {
	Basis b = basis.inverse();
	b.transpose();
	return xform_fast(p_plane, b);
}

_FORCE_INLINE_ Plane Transform3D::xform_inv(const Plane &p_plane) const {
	Transform3D inv = affine_inverse();
	Basis basis_transpose = basis.transposed();
	return xform_inv_fast(p_plane, inv, basis_transpose);
}

_FORCE_INLINE_ AABB Transform3D::xform(const AABB &p_aabb) const {
	/* https://dev.theomader.com/transform-bounding-boxes/ */
	Vector3 min = p_aabb.position;
	Vector3 max = p_aabb.position + p_aabb.size;
	Vector3 tmin, tmax;
	for (int i = 0; i < 3; i++) {
		tmin[i] = tmax[i] = origin[i];
		for (int j = 0; j < 3; j++) {
			real_t e = basis[i][j] * min[j];
			real_t f = basis[i][j] * max[j];
			if (e < f) {
				tmin[i] += e;
				tmax[i] += f;
			} else {
				tmin[i] += f;
				tmax[i] += e;
			}
		}
	}
	AABB r_aabb;
	r_aabb.position = tmin;
	r_aabb.size = tmax - tmin;
	return r_aabb;
}

_FORCE_INLINE_ AABB Transform3D::xform_inv(const AABB &p_aabb) const {
	/* define vertices */
	Vector3 vertices[8] = {
		Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z),
		Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z),
		Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z),
		Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z),
		Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z),
		Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z),
		Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z),
		Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z)
	};

	AABB ret;

	ret.position = xform_inv(vertices[0]);

	for (int i = 1; i < 8; i++) {
		ret.expand_to(xform_inv(vertices[i]));
	}

	return ret;
}

Vector<Vector3> Transform3D::xform(const Vector<Vector3> &p_array) const {
	Vector<Vector3> array;
	array.resize(p_array.size());

	const Vector3 *r = p_array.ptr();
	Vector3 *w = array.ptrw();

	for (int i = 0; i < p_array.size(); ++i) {
		w[i] = xform(r[i]);
	}
	return array;
}

Vector<Vector3> Transform3D::xform_inv(const Vector<Vector3> &p_array) const {
	Vector<Vector3> array;
	array.resize(p_array.size());

	const Vector3 *r = p_array.ptr();
	Vector3 *w = array.ptrw();

	for (int i = 0; i < p_array.size(); ++i) {
		w[i] = xform_inv(r[i]);
	}
	return array;
}

_FORCE_INLINE_ Plane Transform3D::xform_fast(const Plane &p_plane, const Basis &p_basis_inverse_transpose) const {
	// Transform a single point on the plane.
	Vector3 point = p_plane.normal * p_plane.d;
	point = xform(point);

	// Use inverse transpose for correct normals with non-uniform scaling.
	Vector3 normal = p_basis_inverse_transpose.xform(p_plane.normal);
	normal.normalize();

	real_t d = normal.dot(point);
	return Plane(normal, d);
}

_FORCE_INLINE_ Plane Transform3D::xform_inv_fast(const Plane &p_plane, const Transform3D &p_inverse, const Basis &p_basis_transpose) {
	// Transform a single point on the plane.
	Vector3 point = p_plane.normal * p_plane.d;
	point = p_inverse.xform(point);

	// Note that instead of precalculating the transpose, an alternative
	// would be to use the transpose for the basis transform.
	// However that would be less SIMD friendly (requiring a swizzle).
	// So the cost is one extra precalced value in the calling code.
	// This is probably worth it, as this could be used in bottleneck areas. And
	// where it is not a bottleneck, the non-fast method is fine.

	// Use transpose for correct normals with non-uniform scaling.
	Vector3 normal = p_basis_transpose.xform(p_plane.normal);
	normal.normalize();

	real_t d = normal.dot(point);
	return Plane(normal, d);
}
