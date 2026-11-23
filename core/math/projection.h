/**************************************************************************/
/*  projection.h                                                          */
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

#include "core/math/vector3.h"
// --- Inlined/Folded from core/math/vector4.h ---
#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/templates/hashfuncs.h"
#include "core/typedefs.h"

class String;
struct Vector4i;

struct [[nodiscard]] Vector4 {
	static constexpr int AXIS_COUNT = 4;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_W,
	};

	real_t x = 0.0f;
	real_t y = 0.0f;
	real_t z = 0.0f;
	real_t w = 0.0f;

	constexpr real_t &operator[](int p_axis) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Vector4, x) == 0 * sizeof(real_t));
		static_assert(offsetof(Vector4, y) == 1 * sizeof(real_t));
		static_assert(offsetof(Vector4, z) == 2 * sizeof(real_t));
		static_assert(offsetof(Vector4, w) == 3 * sizeof(real_t));
		static_assert(sizeof(Vector4) == 4 * sizeof(real_t));

		DEV_ASSERT((unsigned int)p_axis < 4);
		return (&x)[p_axis];
	}
	constexpr const real_t &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return (&x)[p_axis];
	}

	Vector4::Axis min_axis_index() const;
	Vector4::Axis max_axis_index() const;

	Vector4 min(const Vector4 &p_vector4) const {
		return Vector4(MIN(x, p_vector4.x), MIN(y, p_vector4.y), MIN(z, p_vector4.z), MIN(w, p_vector4.w));
	}

	Vector4 minf(real_t p_scalar) const {
		return Vector4(MIN(x, p_scalar), MIN(y, p_scalar), MIN(z, p_scalar), MIN(w, p_scalar));
	}

	Vector4 max(const Vector4 &p_vector4) const {
		return Vector4(MAX(x, p_vector4.x), MAX(y, p_vector4.y), MAX(z, p_vector4.z), MAX(w, p_vector4.w));
	}

	Vector4 maxf(real_t p_scalar) const {
		return Vector4(MAX(x, p_scalar), MAX(y, p_scalar), MAX(z, p_scalar), MAX(w, p_scalar));
	}

	_FORCE_INLINE_ real_t length_squared() const;
	bool is_equal_approx(const Vector4 &p_vec4) const;
	bool is_zero_approx() const;
	bool is_same(const Vector4 &p_vec4) const;
	bool is_finite() const;
	real_t length() const;
	void normalize();
	Vector4 normalized() const;
	bool is_normalized() const;

	void zero() { x = y = z = w = 0; }

	real_t distance_to(const Vector4 &p_to) const;
	real_t distance_squared_to(const Vector4 &p_to) const;
	Vector4 direction_to(const Vector4 &p_to) const;

	Vector4 abs() const;
	Vector4 sign() const;
	Vector4 floor() const;
	Vector4 ceil() const;
	Vector4 round() const;
	Vector4 lerp(const Vector4 &p_to, real_t p_weight) const;
	Vector4 cubic_interpolate(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight) const;
	Vector4 cubic_interpolate_in_time(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const;

	Vector4 posmod(real_t p_mod) const;
	Vector4 posmodv(const Vector4 &p_modv) const;
	void snap(const Vector4 &p_step);
	void snapf(real_t p_step);
	Vector4 snapped(const Vector4 &p_step) const;
	Vector4 snappedf(real_t p_step) const;
	Vector4 clamp(const Vector4 &p_min, const Vector4 &p_max) const;
	Vector4 clampf(real_t p_min, real_t p_max) const;

	Vector4 inverse() const;
	_FORCE_INLINE_ real_t dot(const Vector4 &p_vec4) const;

	constexpr void operator+=(const Vector4 &p_vec4);
	constexpr void operator-=(const Vector4 &p_vec4);
	constexpr void operator*=(const Vector4 &p_vec4);
	constexpr void operator/=(const Vector4 &p_vec4);
	constexpr void operator*=(real_t p_s);
	constexpr void operator/=(real_t p_s);
	constexpr Vector4 operator+(const Vector4 &p_vec4) const;
	constexpr Vector4 operator-(const Vector4 &p_vec4) const;
	constexpr Vector4 operator*(const Vector4 &p_vec4) const;
	constexpr Vector4 operator/(const Vector4 &p_vec4) const;
	constexpr Vector4 operator-() const;
	constexpr Vector4 operator*(real_t p_s) const;
	constexpr Vector4 operator/(real_t p_s) const;

	constexpr bool operator==(const Vector4 &p_vec4) const;
	constexpr bool operator!=(const Vector4 &p_vec4) const;
	constexpr bool operator>(const Vector4 &p_vec4) const;
	constexpr bool operator<(const Vector4 &p_vec4) const;
	constexpr bool operator>=(const Vector4 &p_vec4) const;
	constexpr bool operator<=(const Vector4 &p_vec4) const;

	explicit operator String() const;
	operator Vector4i() const;

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_real(x);
		h = hash_murmur3_one_real(y, h);
		h = hash_murmur3_one_real(z, h);
		h = hash_murmur3_one_real(w, h);
		return hash_fmix32(h);
	}

	constexpr Vector4() = default;
	constexpr Vector4(real_t p_x, real_t p_y, real_t p_z, real_t p_w) :
			x(p_x), y(p_y), z(p_z), w(p_w) {}
};

real_t Vector4::dot(const Vector4 &p_vec4) const {
	return x * p_vec4.x + y * p_vec4.y + z * p_vec4.z + w * p_vec4.w;
}

real_t Vector4::length_squared() const {
	return dot(*this);
}

constexpr void Vector4::operator+=(const Vector4 &p_vec4) {
	x += p_vec4.x;
	y += p_vec4.y;
	z += p_vec4.z;
	w += p_vec4.w;
}

constexpr void Vector4::operator-=(const Vector4 &p_vec4) {
	x -= p_vec4.x;
	y -= p_vec4.y;
	z -= p_vec4.z;
	w -= p_vec4.w;
}

constexpr void Vector4::operator*=(const Vector4 &p_vec4) {
	x *= p_vec4.x;
	y *= p_vec4.y;
	z *= p_vec4.z;
	w *= p_vec4.w;
}

constexpr void Vector4::operator/=(const Vector4 &p_vec4) {
	x /= p_vec4.x;
	y /= p_vec4.y;
	z /= p_vec4.z;
	w /= p_vec4.w;
}
constexpr void Vector4::operator*=(real_t p_s) {
	x *= p_s;
	y *= p_s;
	z *= p_s;
	w *= p_s;
}

constexpr void Vector4::operator/=(real_t p_s) {
	x /= p_s;
	y /= p_s;
	z /= p_s;
	w /= p_s;
}

constexpr Vector4 Vector4::operator+(const Vector4 &p_vec4) const {
	return Vector4(x + p_vec4.x, y + p_vec4.y, z + p_vec4.z, w + p_vec4.w);
}

constexpr Vector4 Vector4::operator-(const Vector4 &p_vec4) const {
	return Vector4(x - p_vec4.x, y - p_vec4.y, z - p_vec4.z, w - p_vec4.w);
}

constexpr Vector4 Vector4::operator*(const Vector4 &p_vec4) const {
	return Vector4(x * p_vec4.x, y * p_vec4.y, z * p_vec4.z, w * p_vec4.w);
}

constexpr Vector4 Vector4::operator/(const Vector4 &p_vec4) const {
	return Vector4(x / p_vec4.x, y / p_vec4.y, z / p_vec4.z, w / p_vec4.w);
}

constexpr Vector4 Vector4::operator-() const {
	return Vector4(-x, -y, -z, -w);
}

constexpr Vector4 Vector4::operator*(real_t p_s) const {
	return Vector4(x * p_s, y * p_s, z * p_s, w * p_s);
}

constexpr Vector4 Vector4::operator/(real_t p_s) const {
	return Vector4(x / p_s, y / p_s, z / p_s, w / p_s);
}

constexpr bool Vector4::operator==(const Vector4 &p_vec4) const {
	return x == p_vec4.x && y == p_vec4.y && z == p_vec4.z && w == p_vec4.w;
}

constexpr bool Vector4::operator!=(const Vector4 &p_vec4) const {
	return x != p_vec4.x || y != p_vec4.y || z != p_vec4.z || w != p_vec4.w;
}

constexpr bool Vector4::operator<(const Vector4 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w < p_v.w;
			}
			return z < p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

constexpr bool Vector4::operator>(const Vector4 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w > p_v.w;
			}
			return z > p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

constexpr bool Vector4::operator<=(const Vector4 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w <= p_v.w;
			}
			return z < p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

constexpr bool Vector4::operator>=(const Vector4 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w >= p_v.w;
			}
			return z > p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

constexpr Vector4 operator*(float p_scalar, const Vector4 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector4 operator*(double p_scalar, const Vector4 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector4 operator*(int32_t p_scalar, const Vector4 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector4 operator*(int64_t p_scalar, const Vector4 &p_vec) {
	return p_vec * p_scalar;
}

template <>
struct is_zero_constructible<Vector4> : std::true_type {};

// --- Folded from core/math/vector4i.h ---
#include "core/error/error_macros.h"
#include "core/math/math_funcs.h"
#include "core/templates/hashfuncs.h"

class String;
struct Vector4;

struct [[nodiscard]] Vector4i {
	static constexpr int AXIS_COUNT = 4;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_W,
	};

	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;
	int32_t w = 0;

	constexpr int32_t &operator[](int p_axis) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Vector4i, x) == 0 * sizeof(int32_t));
		static_assert(offsetof(Vector4i, y) == 1 * sizeof(int32_t));
		static_assert(offsetof(Vector4i, z) == 2 * sizeof(int32_t));
		static_assert(offsetof(Vector4i, w) == 3 * sizeof(int32_t));
		static_assert(sizeof(Vector4i) == 4 * sizeof(int32_t));

		DEV_ASSERT((unsigned int)p_axis < 4);
		return (&x)[p_axis];
	}
	constexpr const int32_t &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return (&x)[p_axis];
	}

	Vector4i::Axis min_axis_index() const;
	Vector4i::Axis max_axis_index() const;

	Vector4i min(const Vector4i &p_vector4i) const {
		return Vector4i(MIN(x, p_vector4i.x), MIN(y, p_vector4i.y), MIN(z, p_vector4i.z), MIN(w, p_vector4i.w));
	}

	Vector4i mini(int32_t p_scalar) const {
		return Vector4i(MIN(x, p_scalar), MIN(y, p_scalar), MIN(z, p_scalar), MIN(w, p_scalar));
	}

	Vector4i max(const Vector4i &p_vector4i) const {
		return Vector4i(MAX(x, p_vector4i.x), MAX(y, p_vector4i.y), MAX(z, p_vector4i.z), MAX(w, p_vector4i.w));
	}

	Vector4i maxi(int32_t p_scalar) const {
		return Vector4i(MAX(x, p_scalar), MAX(y, p_scalar), MAX(z, p_scalar), MAX(w, p_scalar));
	}

	_FORCE_INLINE_ int64_t length_squared() const;
	_FORCE_INLINE_ double length() const;

	_FORCE_INLINE_ void zero();

	_FORCE_INLINE_ double distance_to(const Vector4i &p_to) const;
	_FORCE_INLINE_ int64_t distance_squared_to(const Vector4i &p_to) const;

	_FORCE_INLINE_ Vector4i abs() const;
	_FORCE_INLINE_ Vector4i sign() const;
	Vector4i clamp(const Vector4i &p_min, const Vector4i &p_max) const;
	Vector4i clampi(int32_t p_min, int32_t p_max) const;
	Vector4i snapped(const Vector4i &p_step) const;
	Vector4i snappedi(int32_t p_step) const;

	/* Operators */

	constexpr Vector4i &operator+=(const Vector4i &p_v);
	constexpr Vector4i operator+(const Vector4i &p_v) const;
	constexpr Vector4i &operator-=(const Vector4i &p_v);
	constexpr Vector4i operator-(const Vector4i &p_v) const;
	constexpr Vector4i &operator*=(const Vector4i &p_v);
	constexpr Vector4i operator*(const Vector4i &p_v) const;
	constexpr Vector4i &operator/=(const Vector4i &p_v);
	constexpr Vector4i operator/(const Vector4i &p_v) const;
	constexpr Vector4i &operator%=(const Vector4i &p_v);
	constexpr Vector4i operator%(const Vector4i &p_v) const;

	constexpr Vector4i &operator*=(int32_t p_scalar);
	constexpr Vector4i operator*(int32_t p_scalar) const;
	constexpr Vector4i &operator/=(int32_t p_scalar);
	constexpr Vector4i operator/(int32_t p_scalar) const;
	constexpr Vector4i &operator%=(int32_t p_scalar);
	constexpr Vector4i operator%(int32_t p_scalar) const;

	constexpr Vector4i operator-() const;

	constexpr bool operator==(const Vector4i &p_v) const;
	constexpr bool operator!=(const Vector4i &p_v) const;
	constexpr bool operator<(const Vector4i &p_v) const;
	constexpr bool operator<=(const Vector4i &p_v) const;
	constexpr bool operator>(const Vector4i &p_v) const;
	constexpr bool operator>=(const Vector4i &p_v) const;

	explicit operator String() const;
	operator Vector4() const;

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_32(uint32_t(x));
		h = hash_murmur3_one_32(uint32_t(y), h);
		h = hash_murmur3_one_32(uint32_t(z), h);
		h = hash_murmur3_one_32(uint32_t(w), h);
		return hash_fmix32(h);
	}

	constexpr Vector4i() = default;
	constexpr Vector4i(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_w) :
			x(p_x), y(p_y), z(p_z), w(p_w) {}
};

int64_t Vector4i::length_squared() const {
	return x * (int64_t)x + y * (int64_t)y + z * (int64_t)z + w * (int64_t)w;
}

double Vector4i::length() const {
	return Math::sqrt((double)length_squared());
}

double Vector4i::distance_to(const Vector4i &p_to) const {
	return (p_to - *this).length();
}

int64_t Vector4i::distance_squared_to(const Vector4i &p_to) const {
	return (p_to - *this).length_squared();
}

Vector4i Vector4i::abs() const {
	return Vector4i(Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w));
}

Vector4i Vector4i::sign() const {
	return Vector4i(SIGN(x), SIGN(y), SIGN(z), SIGN(w));
}

/* Operators */

constexpr Vector4i &Vector4i::operator+=(const Vector4i &p_v) {
	x += p_v.x;
	y += p_v.y;
	z += p_v.z;
	w += p_v.w;
	return *this;
}

constexpr Vector4i Vector4i::operator+(const Vector4i &p_v) const {
	return Vector4i(x + p_v.x, y + p_v.y, z + p_v.z, w + p_v.w);
}

constexpr Vector4i &Vector4i::operator-=(const Vector4i &p_v) {
	x -= p_v.x;
	y -= p_v.y;
	z -= p_v.z;
	w -= p_v.w;
	return *this;
}

constexpr Vector4i Vector4i::operator-(const Vector4i &p_v) const {
	return Vector4i(x - p_v.x, y - p_v.y, z - p_v.z, w - p_v.w);
}

constexpr Vector4i &Vector4i::operator*=(const Vector4i &p_v) {
	x *= p_v.x;
	y *= p_v.y;
	z *= p_v.z;
	w *= p_v.w;
	return *this;
}

constexpr Vector4i Vector4i::operator*(const Vector4i &p_v) const {
	return Vector4i(x * p_v.x, y * p_v.y, z * p_v.z, w * p_v.w);
}

constexpr Vector4i &Vector4i::operator/=(const Vector4i &p_v) {
	x /= p_v.x;
	y /= p_v.y;
	z /= p_v.z;
	w /= p_v.w;
	return *this;
}

constexpr Vector4i Vector4i::operator/(const Vector4i &p_v) const {
	return Vector4i(x / p_v.x, y / p_v.y, z / p_v.z, w / p_v.w);
}

constexpr Vector4i &Vector4i::operator%=(const Vector4i &p_v) {
	x %= p_v.x;
	y %= p_v.y;
	z %= p_v.z;
	w %= p_v.w;
	return *this;
}

constexpr Vector4i Vector4i::operator%(const Vector4i &p_v) const {
	return Vector4i(x % p_v.x, y % p_v.y, z % p_v.z, w % p_v.w);
}

constexpr Vector4i &Vector4i::operator*=(int32_t p_scalar) {
	x *= p_scalar;
	y *= p_scalar;
	z *= p_scalar;
	w *= p_scalar;
	return *this;
}

constexpr Vector4i Vector4i::operator*(int32_t p_scalar) const {
	return Vector4i(x * p_scalar, y * p_scalar, z * p_scalar, w * p_scalar);
}

// Multiplication operators required to workaround issues with LLVM using implicit conversion.

constexpr Vector4i operator*(int32_t p_scalar, const Vector4i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector4i operator*(int64_t p_scalar, const Vector4i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector4i operator*(float p_scalar, const Vector4i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector4i operator*(double p_scalar, const Vector4i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector4i &Vector4i::operator/=(int32_t p_scalar) {
	x /= p_scalar;
	y /= p_scalar;
	z /= p_scalar;
	w /= p_scalar;
	return *this;
}

constexpr Vector4i Vector4i::operator/(int32_t p_scalar) const {
	return Vector4i(x / p_scalar, y / p_scalar, z / p_scalar, w / p_scalar);
}

constexpr Vector4i &Vector4i::operator%=(int32_t p_scalar) {
	x %= p_scalar;
	y %= p_scalar;
	z %= p_scalar;
	w %= p_scalar;
	return *this;
}

constexpr Vector4i Vector4i::operator%(int32_t p_scalar) const {
	return Vector4i(x % p_scalar, y % p_scalar, z % p_scalar, w % p_scalar);
}

constexpr Vector4i Vector4i::operator-() const {
	return Vector4i(-x, -y, -z, -w);
}

constexpr bool Vector4i::operator==(const Vector4i &p_v) const {
	return (x == p_v.x && y == p_v.y && z == p_v.z && w == p_v.w);
}

constexpr bool Vector4i::operator!=(const Vector4i &p_v) const {
	return (x != p_v.x || y != p_v.y || z != p_v.z || w != p_v.w);
}

constexpr bool Vector4i::operator<(const Vector4i &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w < p_v.w;
			} else {
				return z < p_v.z;
			}
		} else {
			return y < p_v.y;
		}
	} else {
		return x < p_v.x;
	}
}

constexpr bool Vector4i::operator>(const Vector4i &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w > p_v.w;
			} else {
				return z > p_v.z;
			}
		} else {
			return y > p_v.y;
		}
	} else {
		return x > p_v.x;
	}
}

constexpr bool Vector4i::operator<=(const Vector4i &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w <= p_v.w;
			} else {
				return z < p_v.z;
			}
		} else {
			return y < p_v.y;
		}
	} else {
		return x < p_v.x;
	}
}

constexpr bool Vector4i::operator>=(const Vector4i &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w >= p_v.w;
			} else {
				return z > p_v.z;
			}
		} else {
			return y > p_v.y;
		}
	} else {
		return x > p_v.x;
	}
}

void Vector4i::zero() {
	x = y = z = w = 0;
}

template <>
struct is_zero_constructible<Vector4i> : std::true_type {};

// --- End core/math/vector4i.h ---

template <typename T>
class Vector;

struct AABB;
struct Plane;
struct Rect2;
struct Transform3D;
struct Vector2;

struct [[nodiscard]] Projection {
	enum Planes {
		PLANE_NEAR,
		PLANE_FAR,
		PLANE_LEFT,
		PLANE_TOP,
		PLANE_RIGHT,
		PLANE_BOTTOM
	};

	Vector4 columns[4] = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 },
	};

	constexpr const Vector4 &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return columns[p_axis];
	}

	constexpr Vector4 &operator[](int p_axis) {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return columns[p_axis];
	}

	real_t determinant() const;
	void set_identity();
	void set_zero();
	void set_light_bias();
	void set_depth_correction(bool p_flip_y = true, bool p_reverse_z = true, bool p_remap_z = true);

	void set_light_atlas_rect(const Rect2 &p_rect);
	void set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov = false);
	void set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist);
	void set_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far);
	void set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar);
	void set_orthogonal(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov = false);
	void set_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far);
	void set_frustum(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov = false);
	void adjust_perspective_znear(real_t p_new_znear);

	static Projection create_depth_correction(bool p_flip_y);
	static Projection create_light_atlas_rect(const Rect2 &p_rect);
	static Projection create_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov = false);
	static Projection create_perspective_hmd(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist);
	static Projection create_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far);
	static Projection create_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar);
	static Projection create_orthogonal_aspect(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov = false);
	static Projection create_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far);
	static Projection create_frustum_aspect(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov = false);
	static Projection create_fit_aabb(const AABB &p_aabb);
	Projection perspective_znear_adjusted(real_t p_new_znear) const;
	Plane get_projection_plane(Planes p_plane) const;
	Projection flipped_y() const;
	Projection jitter_offseted(const Vector2 &p_offset) const;

	static real_t get_fovy(real_t p_fovx, real_t p_aspect) {
		return Math::rad_to_deg(Math::atan(p_aspect * Math::tan(Math::deg_to_rad(p_fovx) * 0.5)) * 2.0);
	}

	real_t get_z_far() const;
	real_t get_z_near() const;
	real_t get_aspect() const;
	real_t get_fov() const;
	bool is_orthogonal() const;

	Vector<Plane> get_projection_planes(const Transform3D &p_transform) const;

	bool get_endpoints(const Transform3D &p_transform, Vector3 *p_8points) const;
	Vector2 get_viewport_half_extents() const;
	Vector2 get_far_plane_half_extents() const;

	void invert();
	Projection inverse() const;

	constexpr Projection operator*(const Projection &p_matrix) const;

	Plane xform4(const Plane &p_vec4) const;
	_FORCE_INLINE_ Vector3 xform(const Vector3 &p_vec3) const;

	Vector4 xform(const Vector4 &p_vec4) const;
	Vector4 xform_inv(const Vector4 &p_vec4) const;

	explicit operator String() const;

	void scale_translate_to_fit(const AABB &p_aabb);
	void add_jitter_offset(const Vector2 &p_offset);
	void make_scale(const Vector3 &p_scale);
	int get_pixels_per_meter(int p_for_pixel_width) const;
	operator Transform3D() const;

	void flip_y();

	bool is_same(const Projection &p_cam) const;

	constexpr bool operator==(const Projection &p_cam) const {
		for (uint32_t i = 0; i < 4; i++) {
			for (uint32_t j = 0; j < 4; j++) {
				if (columns[i][j] != p_cam.columns[i][j]) {
					return false;
				}
			}
		}
		return true;
	}

	constexpr bool operator!=(const Projection &p_cam) const {
		return !(*this == p_cam);
	}

	real_t get_lod_multiplier() const;

	Projection() = default;
	constexpr Projection(const Vector4 &p_x, const Vector4 &p_y, const Vector4 &p_z, const Vector4 &p_w) :
			columns{ p_x, p_y, p_z, p_w } {}
	constexpr Projection(real_t p_xx, real_t p_xy, real_t p_xz, real_t p_xw, real_t p_yx, real_t p_yy, real_t p_yz, real_t p_yw, real_t p_zx, real_t p_zy, real_t p_zz, real_t p_zw, real_t p_wx, real_t p_wy, real_t p_wz, real_t p_ww) :
			columns{
				{ p_xx, p_xy, p_xz, p_xw },
				{ p_yx, p_yy, p_yz, p_yw },
				{ p_zx, p_zy, p_zz, p_zw },
				{ p_wx, p_wy, p_wz, p_ww },
			} {}
	Projection(const Transform3D &p_transform);
};

constexpr Projection Projection::operator*(const Projection &p_matrix) const {
	Projection new_matrix;

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			real_t ab = 0;
			for (int k = 0; k < 4; k++) {
				ab += columns[k][i] * p_matrix.columns[j][k];
			}
			new_matrix.columns[j][i] = ab;
		}
	}

	return new_matrix;
}

Vector3 Projection::xform(const Vector3 &p_vec3) const {
	Vector3 ret;
	ret.x = columns[0][0] * p_vec3.x + columns[1][0] * p_vec3.y + columns[2][0] * p_vec3.z + columns[3][0];
	ret.y = columns[0][1] * p_vec3.x + columns[1][1] * p_vec3.y + columns[2][1] * p_vec3.z + columns[3][1];
	ret.z = columns[0][2] * p_vec3.x + columns[1][2] * p_vec3.y + columns[2][2] * p_vec3.z + columns[3][2];
	real_t w = columns[0][3] * p_vec3.x + columns[1][3] * p_vec3.y + columns[2][3] * p_vec3.z + columns[3][3];
	return ret / w;
}
