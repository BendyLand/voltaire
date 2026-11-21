/**************************************************************************/
/*  vector4.h                                                             */
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
