/**************************************************************************/
/*  types.h                                                               */
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
#ifndef TYPES_H
#define TYPES_H

// Core definitions & memory management
#include "core/os/memory.h"
#include "core/typedefs.h"
#include "core/os/mutex.h"
// --- Inlined/Folded from core/templates/ref.h ---
#ifndef REF_H
#define REF_H

#include <atomic>
#include <type_traits>
#include <utility>
#include "core/os/memory.h"
#include "core/typedefs.h"

class RefCounted
{
	mutable std::atomic<uint32_t> _ref_count{0};
	mutable std::atomic<uint32_t> _ref_count_init{0};

public:
	_FORCE_INLINE_ bool reference() const
	{
		uint32_t count = _ref_count.fetch_add(1, std::memory_order_relaxed);
		return count > 0 || _ref_count_init.exchange(1, std::memory_order_acq_rel) == 0;
	}

	_FORCE_INLINE_ bool unreference() const
	{
		if (_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			memdelete(const_cast<RefCounted*>(this));
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ uint32_t get_reference_count() const
	{
		return _ref_count.load(std::memory_order_relaxed);
	}

	RefCounted() = default;
	virtual ~RefCounted() = default;

	RefCounted(const RefCounted&) = delete;
	RefCounted& operator=(const RefCounted&) = delete;
};

template <typename T> class Ref
{
	T* data = nullptr;

	_FORCE_INLINE_ void ref(T* p_ptr)
	{
		if (p_ptr == data) {
			return;
		}
		unref();
		if (p_ptr && p_ptr->reference()) {
			data = p_ptr;
		}
	}

public:
	// --- Constructors ---
	_FORCE_INLINE_ Ref() = default;

	_FORCE_INLINE_ Ref(std::nullptr_t) {}

	_FORCE_INLINE_ Ref(T* p_ptr)
	{
		if (p_ptr) {
			ref(p_ptr);
		}
	}

	_FORCE_INLINE_ Ref(const Ref& p_from) { ref(p_from.data); }

	_FORCE_INLINE_ Ref(Ref&& p_from) noexcept : data(p_from.data) { p_from.data = nullptr; }

	// Universal converting copy constructor (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
	}

	// Converting move constructor (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref(Ref<U>&& p_from) noexcept : data(p_from.data)
	{
		p_from.data = nullptr;
	}

	_FORCE_INLINE_ ~Ref() { unref(); }

	// --- Lifecycle & Mutators ---
	_FORCE_INLINE_ void unref()
	{
		if (data) {
			data->unreference();
			data = nullptr;
		}
	}

	template <typename... Args> _FORCE_INLINE_ void instantiate(Args&&... p_args)
	{
		ref(memnew(T(std::forward<Args>(p_args)...)));
	}

	// --- Assignment Operators ---
	_FORCE_INLINE_ Ref& operator=(const Ref& p_from)
	{
		if (this != &p_from) {
			ref(p_from.data);
		}
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(Ref&& p_from) noexcept
	{
		if (this != &p_from) {
			unref();
			data = p_from.data;
			p_from.data = nullptr;
		}
		return *this;
	}

	// Universal converting copy assignment (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref& operator=(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
		return *this;
	}

	// Converting move assignment (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref& operator=(Ref<U>&& p_from) noexcept
	{
		unref();
		data = p_from.data;
		p_from.data = nullptr;
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(T* p_ptr)
	{
		ref(p_ptr);
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(std::nullptr_t)
	{
		unref();
		return *this;
	}

	_FORCE_INLINE_ T* ptr() const { return data; }

	_FORCE_INLINE_ T* operator->() const { return data; }

	_FORCE_INLINE_ T& operator*() const { return *data; }

	_FORCE_INLINE_ explicit operator bool() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_valid() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_null() const { return data == nullptr; }

	_FORCE_INLINE_ bool operator==(const Ref& p_r) const { return data == p_r.data; }

	_FORCE_INLINE_ bool operator!=(const Ref& p_r) const { return data != p_r.data; }

	_FORCE_INLINE_ bool operator==(const T* p_ptr) const { return data == p_ptr; }

	_FORCE_INLINE_ bool operator!=(const T* p_ptr) const { return data != p_ptr; }

	_FORCE_INLINE_ bool operator<(const Ref& p_r) const { return data < p_r.data; }

	template <typename U> friend class Ref;
};

template <typename T, typename... Args> _FORCE_INLINE_ Ref<T> memnew_ref(Args&&... p_args)
{
	return Ref<T>(memnew(T(std::forward<Args>(p_args)...)));
}

#endif // REF_H

// Math primitives
// --- Folded from core/math/transform_3d.h ---
// --- Folded from core/math/aabb.h ---
// --- Folded from core/math/plane.h ---
// --- Folded from core/math/vector3.h ---
#include "core/typedefs.h"
#include "core/string/ustring.h"

struct Basis;
struct Vector2;
struct Vector3i;

struct [[nodiscard]] Vector3 {
	static const Vector3 LEFT;
	static const Vector3 RIGHT;
	static const Vector3 UP;
	static const Vector3 DOWN;
	static const Vector3 FORWARD;
	static const Vector3 BACK;
	static const Vector3 MODEL_LEFT;
	static const Vector3 MODEL_RIGHT;
	static const Vector3 MODEL_TOP;
	static const Vector3 MODEL_BOTTOM;
	static const Vector3 MODEL_FRONT;
	static const Vector3 MODEL_REAR;

	static constexpr int AXIS_COUNT = 3;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
	};

	real_t x = 0.0f;
	real_t y = 0.0f;
	real_t z = 0.0f;

	constexpr real_t &operator[](int p_axis) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Vector3, x) == 0 * sizeof(real_t));
		static_assert(offsetof(Vector3, y) == 1 * sizeof(real_t));
		static_assert(offsetof(Vector3, z) == 2 * sizeof(real_t));
		static_assert(sizeof(Vector3) == 3 * sizeof(real_t));

		DEV_ASSERT((unsigned int)p_axis < 3);
		return (&x)[p_axis];
	}
	constexpr const real_t &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 3);
		return (&x)[p_axis];
	}

	_FORCE_INLINE_ Vector3::Axis min_axis_index() const {
		return x < y ? (x < z ? Vector3::AXIS_X : Vector3::AXIS_Z) : (y < z ? Vector3::AXIS_Y : Vector3::AXIS_Z);
	}

	_FORCE_INLINE_ Vector3::Axis max_axis_index() const {
		return x < y ? (y < z ? Vector3::AXIS_Z : Vector3::AXIS_Y) : (x < z ? Vector3::AXIS_Z : Vector3::AXIS_X);
	}

	Vector3 min(const Vector3 &p_vector3) const {
		return Vector3(MIN(x, p_vector3.x), MIN(y, p_vector3.y), MIN(z, p_vector3.z));
	}

	Vector3 minf(real_t p_scalar) const {
		return Vector3(MIN(x, p_scalar), MIN(y, p_scalar), MIN(z, p_scalar));
	}

	Vector3 max(const Vector3 &p_vector3) const {
		return Vector3(MAX(x, p_vector3.x), MAX(y, p_vector3.y), MAX(z, p_vector3.z));
	}

	Vector3 maxf(real_t p_scalar) const {
		return Vector3(MAX(x, p_scalar), MAX(y, p_scalar), MAX(z, p_scalar));
	}

	Vector3 clamp(const Vector3 &p_min, const Vector3 &p_max) const {
		return Vector3(
				CLAMP(x, p_min.x, p_max.x),
				CLAMP(y, p_min.y, p_max.y),
				CLAMP(z, p_min.z, p_max.z));
	}

	Vector3 clampf(real_t p_min, real_t p_max) const {
		return Vector3(
				CLAMP(x, p_min, p_max),
				CLAMP(y, p_min, p_max),
				CLAMP(z, p_min, p_max));
	}

	_FORCE_INLINE_ real_t length() const;
	_FORCE_INLINE_ real_t length_squared() const;

	_FORCE_INLINE_ void normalize();
	_FORCE_INLINE_ Vector3 normalized() const;
	_FORCE_INLINE_ bool is_normalized() const;
	_FORCE_INLINE_ Vector3 inverse() const;
	Vector3 limit_length(real_t p_len = 1.0) const;

	_FORCE_INLINE_ void zero();

	void snap(const Vector3 &p_step);
	void snapf(real_t p_step);
	Vector3 snapped(const Vector3 &p_step) const;
	Vector3 snappedf(real_t p_step) const;

	void rotate(const Vector3 &p_axis, real_t p_angle);
	Vector3 rotated(const Vector3 &p_axis, real_t p_angle) const;

	/* Static Methods between 2 vector3s */

	_FORCE_INLINE_ Vector3 lerp(const Vector3 &p_to, real_t p_weight) const;
	_FORCE_INLINE_ Vector3 slerp(const Vector3 &p_to, real_t p_weight) const;
	_FORCE_INLINE_ Vector3 cubic_interpolate(const Vector3 &p_b, const Vector3 &p_pre_a, const Vector3 &p_post_b, real_t p_weight) const;
	_FORCE_INLINE_ Vector3 cubic_interpolate_in_time(const Vector3 &p_b, const Vector3 &p_pre_a, const Vector3 &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const;
	_FORCE_INLINE_ Vector3 bezier_interpolate(const Vector3 &p_control_1, const Vector3 &p_control_2, const Vector3 &p_end, real_t p_t) const;
	_FORCE_INLINE_ Vector3 bezier_derivative(const Vector3 &p_control_1, const Vector3 &p_control_2, const Vector3 &p_end, real_t p_t) const;

	Vector3 move_toward(const Vector3 &p_to, real_t p_delta) const;

	Vector2 octahedron_encode() const;
	static Vector3 octahedron_decode(const Vector2 &p_oct);
	Vector2 octahedron_tangent_encode(float p_sign) const;
	static Vector3 octahedron_tangent_decode(const Vector2 &p_oct, float *r_sign);

	_FORCE_INLINE_ Vector3 cross(const Vector3 &p_with) const;
	_FORCE_INLINE_ real_t dot(const Vector3 &p_with) const;
	Basis outer(const Vector3 &p_with) const;
	_FORCE_INLINE_ Vector3 get_any_perpendicular() const;

	_FORCE_INLINE_ Vector3 abs() const;
	_FORCE_INLINE_ Vector3 floor() const;
	_FORCE_INLINE_ Vector3 sign() const;
	_FORCE_INLINE_ Vector3 ceil() const;
	_FORCE_INLINE_ Vector3 round() const;

	_FORCE_INLINE_ real_t distance_to(const Vector3 &p_to) const;
	_FORCE_INLINE_ real_t distance_squared_to(const Vector3 &p_to) const;

	_FORCE_INLINE_ Vector3 posmod(real_t p_mod) const;
	_FORCE_INLINE_ Vector3 posmodv(const Vector3 &p_modv) const;
	_FORCE_INLINE_ Vector3 project(const Vector3 &p_to) const;

	_FORCE_INLINE_ real_t angle_to(const Vector3 &p_to) const;
	_FORCE_INLINE_ real_t signed_angle_to(const Vector3 &p_to, const Vector3 &p_axis) const;
	_FORCE_INLINE_ Vector3 direction_to(const Vector3 &p_to) const;

	_FORCE_INLINE_ Vector3 slide(const Vector3 &p_normal) const;
	_FORCE_INLINE_ Vector3 bounce(const Vector3 &p_normal) const;
	_FORCE_INLINE_ Vector3 reflect(const Vector3 &p_normal) const;

	bool is_equal_approx(const Vector3 &p_v) const;
	bool is_same(const Vector3 &p_v) const;
	bool is_zero_approx() const;
	bool is_finite() const;

	/* Operators */

	constexpr Vector3 &operator+=(const Vector3 &p_v);
	constexpr Vector3 operator+(const Vector3 &p_v) const;
	constexpr Vector3 &operator-=(const Vector3 &p_v);
	constexpr Vector3 operator-(const Vector3 &p_v) const;
	constexpr Vector3 &operator*=(const Vector3 &p_v);
	constexpr Vector3 operator*(const Vector3 &p_v) const;
	constexpr Vector3 &operator/=(const Vector3 &p_v);
	constexpr Vector3 operator/(const Vector3 &p_v) const;

	constexpr Vector3 &operator*=(real_t p_scalar);
	constexpr Vector3 operator*(real_t p_scalar) const;
	constexpr Vector3 &operator/=(real_t p_scalar);
	constexpr Vector3 operator/(real_t p_scalar) const;

	constexpr Vector3 operator-() const;

	constexpr bool operator==(const Vector3 &p_v) const;
	constexpr bool operator!=(const Vector3 &p_v) const;
	constexpr bool operator<(const Vector3 &p_v) const;
	constexpr bool operator<=(const Vector3 &p_v) const;
	constexpr bool operator>(const Vector3 &p_v) const;
	constexpr bool operator>=(const Vector3 &p_v) const;

	explicit operator String() const;
	operator Vector3i() const;

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_real(x);
		h = hash_murmur3_one_real(y, h);
		h = hash_murmur3_one_real(z, h);
		return hash_fmix32(h);
	}

	constexpr Vector3() :
			x(0), y(0), z(0) {}
	constexpr Vector3(real_t p_x, real_t p_y, real_t p_z) :
			x(p_x), y(p_y), z(p_z) {}
};

inline constexpr Vector3 Vector3::LEFT = { -1, 0, 0 };
inline constexpr Vector3 Vector3::RIGHT = { 1, 0, 0 };
inline constexpr Vector3 Vector3::UP = { 0, 1, 0 };
inline constexpr Vector3 Vector3::DOWN = { 0, -1, 0 };
inline constexpr Vector3 Vector3::FORWARD = { 0, 0, -1 };
inline constexpr Vector3 Vector3::BACK = { 0, 0, 1 };
inline constexpr Vector3 Vector3::MODEL_LEFT = { 1, 0, 0 };
inline constexpr Vector3 Vector3::MODEL_RIGHT = { -1, 0, 0 };
inline constexpr Vector3 Vector3::MODEL_TOP = { 0, 1, 0 };
inline constexpr Vector3 Vector3::MODEL_BOTTOM = { 0, -1, 0 };
inline constexpr Vector3 Vector3::MODEL_FRONT = { 0, 0, 1 };
inline constexpr Vector3 Vector3::MODEL_REAR = { 0, 0, -1 };

Vector3 Vector3::cross(const Vector3 &p_with) const {
	Vector3 ret(
			(y * p_with.z) - (z * p_with.y),
			(z * p_with.x) - (x * p_with.z),
			(x * p_with.y) - (y * p_with.x));

	return ret;
}

real_t Vector3::dot(const Vector3 &p_with) const {
	return x * p_with.x + y * p_with.y + z * p_with.z;
}

Vector3 Vector3::abs() const {
	return Vector3(Math::abs(x), Math::abs(y), Math::abs(z));
}

Vector3 Vector3::sign() const {
	return Vector3(SIGN(x), SIGN(y), SIGN(z));
}

Vector3 Vector3::floor() const {
	return Vector3(Math::floor(x), Math::floor(y), Math::floor(z));
}

Vector3 Vector3::ceil() const {
	return Vector3(Math::ceil(x), Math::ceil(y), Math::ceil(z));
}

Vector3 Vector3::round() const {
	return Vector3(Math::round(x), Math::round(y), Math::round(z));
}

Vector3 Vector3::lerp(const Vector3 &p_to, real_t p_weight) const {
	Vector3 res = *this;
	res.x = Math::lerp(res.x, p_to.x, p_weight);
	res.y = Math::lerp(res.y, p_to.y, p_weight);
	res.z = Math::lerp(res.z, p_to.z, p_weight);
	return res;
}

Vector3 Vector3::slerp(const Vector3 &p_to, real_t p_weight) const {
	// This method seems more complicated than it really is, since we write out
	// the internals of some methods for efficiency (mainly, checking length).
	real_t start_length_sq = length_squared();
	real_t end_length_sq = p_to.length_squared();
	if (unlikely(start_length_sq == 0.0f || end_length_sq == 0.0f)) {
		// Zero length vectors have no angle, so the best we can do is either lerp or throw an error.
		return lerp(p_to, p_weight);
	}
	Vector3 axis = cross(p_to);
	real_t axis_length_sq = axis.length_squared();
	if (unlikely(axis_length_sq == 0.0f)) {
		// Colinear vectors have no rotation axis or angle between them, so the best we can do is lerp.
		return lerp(p_to, p_weight);
	}
	axis /= Math::sqrt(axis_length_sq);
	real_t start_length = Math::sqrt(start_length_sq);
	real_t result_length = Math::lerp(start_length, Math::sqrt(end_length_sq), p_weight);
	real_t angle = angle_to(p_to);
	return rotated(axis, angle * p_weight) * (result_length / start_length);
}

Vector3 Vector3::cubic_interpolate(const Vector3 &p_b, const Vector3 &p_pre_a, const Vector3 &p_post_b, real_t p_weight) const {
	Vector3 res = *this;
	res.x = Math::cubic_interpolate(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight);
	res.y = Math::cubic_interpolate(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight);
	res.z = Math::cubic_interpolate(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight);
	return res;
}

Vector3 Vector3::cubic_interpolate_in_time(const Vector3 &p_b, const Vector3 &p_pre_a, const Vector3 &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
	Vector3 res = *this;
	res.x = Math::cubic_interpolate_in_time(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.y = Math::cubic_interpolate_in_time(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.z = Math::cubic_interpolate_in_time(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	return res;
}

Vector3 Vector3::bezier_interpolate(const Vector3 &p_control_1, const Vector3 &p_control_2, const Vector3 &p_end, real_t p_t) const {
	Vector3 res = *this;
	res.x = Math::bezier_interpolate(res.x, p_control_1.x, p_control_2.x, p_end.x, p_t);
	res.y = Math::bezier_interpolate(res.y, p_control_1.y, p_control_2.y, p_end.y, p_t);
	res.z = Math::bezier_interpolate(res.z, p_control_1.z, p_control_2.z, p_end.z, p_t);
	return res;
}

Vector3 Vector3::bezier_derivative(const Vector3 &p_control_1, const Vector3 &p_control_2, const Vector3 &p_end, real_t p_t) const {
	Vector3 res = *this;
	res.x = Math::bezier_derivative(res.x, p_control_1.x, p_control_2.x, p_end.x, p_t);
	res.y = Math::bezier_derivative(res.y, p_control_1.y, p_control_2.y, p_end.y, p_t);
	res.z = Math::bezier_derivative(res.z, p_control_1.z, p_control_2.z, p_end.z, p_t);
	return res;
}

real_t Vector3::distance_to(const Vector3 &p_to) const {
	return (p_to - *this).length();
}

real_t Vector3::distance_squared_to(const Vector3 &p_to) const {
	return (p_to - *this).length_squared();
}

Vector3 Vector3::posmod(real_t p_mod) const {
	return Vector3(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod), Math::fposmod(z, p_mod));
}

Vector3 Vector3::posmodv(const Vector3 &p_modv) const {
	return Vector3(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y), Math::fposmod(z, p_modv.z));
}

Vector3 Vector3::project(const Vector3 &p_to) const {
	return p_to * (dot(p_to) / p_to.length_squared());
}

real_t Vector3::angle_to(const Vector3 &p_to) const {
	return Math::atan2(cross(p_to).length(), dot(p_to));
}

real_t Vector3::signed_angle_to(const Vector3 &p_to, const Vector3 &p_axis) const {
	Vector3 cross_to = cross(p_to);
	real_t unsigned_angle = Math::atan2(cross_to.length(), dot(p_to));
	real_t sign = cross_to.dot(p_axis);
	return (sign < 0) ? -unsigned_angle : unsigned_angle;
}

Vector3 Vector3::direction_to(const Vector3 &p_to) const {
	Vector3 ret(p_to.x - x, p_to.y - y, p_to.z - z);
	ret.normalize();
	return ret;
}

Vector3 Vector3::get_any_perpendicular() const {
	// Return the any perpendicular vector by cross product with the Vector3.RIGHT or Vector3.UP,
	// whichever has the greater angle to the current vector with the sign of each element positive.
	// The only essence is "to avoid being parallel to the current vector", and there is no mathematical basis for using Vector3.RIGHT and Vector3.UP,
	// since it could be a different vector depending on the prior branching code Math::abs(x) <= Math::abs(y) && Math::abs(x) <= Math::abs(z).
	// However, it would be reasonable to use any of the axes of the basis, as it is simpler to calculate.
	ERR_FAIL_COND_V_MSG(is_zero_approx(), Vector3(0, 0, 0), "The Vector3 must not be zero.");
	return cross((Math::abs(x) <= Math::abs(y) && Math::abs(x) <= Math::abs(z)) ? Vector3::RIGHT : Vector3::UP).normalized();
}

/* Operators */

constexpr Vector3 &Vector3::operator+=(const Vector3 &p_v) {
	x += p_v.x;
	y += p_v.y;
	z += p_v.z;
	return *this;
}

constexpr Vector3 Vector3::operator+(const Vector3 &p_v) const {
	return Vector3(x + p_v.x, y + p_v.y, z + p_v.z);
}

constexpr Vector3 &Vector3::operator-=(const Vector3 &p_v) {
	x -= p_v.x;
	y -= p_v.y;
	z -= p_v.z;
	return *this;
}

constexpr Vector3 Vector3::operator-(const Vector3 &p_v) const {
	return Vector3(x - p_v.x, y - p_v.y, z - p_v.z);
}

constexpr Vector3 &Vector3::operator*=(const Vector3 &p_v) {
	x *= p_v.x;
	y *= p_v.y;
	z *= p_v.z;
	return *this;
}

constexpr Vector3 Vector3::operator*(const Vector3 &p_v) const {
	return Vector3(x * p_v.x, y * p_v.y, z * p_v.z);
}

constexpr Vector3 &Vector3::operator/=(const Vector3 &p_v) {
	x /= p_v.x;
	y /= p_v.y;
	z /= p_v.z;
	return *this;
}

constexpr Vector3 Vector3::operator/(const Vector3 &p_v) const {
	return Vector3(x / p_v.x, y / p_v.y, z / p_v.z);
}

constexpr Vector3 &Vector3::operator*=(real_t p_scalar) {
	x *= p_scalar;
	y *= p_scalar;
	z *= p_scalar;
	return *this;
}

// Multiplication operators required to workaround issues with LLVM using implicit conversion
// to Vector3i instead for integers where it should not.

constexpr Vector3 operator*(float p_scalar, const Vector3 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector3 operator*(double p_scalar, const Vector3 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector3 operator*(int32_t p_scalar, const Vector3 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector3 operator*(int64_t p_scalar, const Vector3 &p_vec) {
	return p_vec * p_scalar;
}

constexpr Vector3 Vector3::operator*(real_t p_scalar) const {
	return Vector3(x * p_scalar, y * p_scalar, z * p_scalar);
}

constexpr Vector3 &Vector3::operator/=(real_t p_scalar) {
	x /= p_scalar;
	y /= p_scalar;
	z /= p_scalar;
	return *this;
}

constexpr Vector3 Vector3::operator/(real_t p_scalar) const {
	return Vector3(x / p_scalar, y / p_scalar, z / p_scalar);
}

constexpr Vector3 Vector3::operator-() const {
	return Vector3(-x, -y, -z);
}

constexpr bool Vector3::operator==(const Vector3 &p_v) const {
	return x == p_v.x && y == p_v.y && z == p_v.z;
}

constexpr bool Vector3::operator!=(const Vector3 &p_v) const {
	return x != p_v.x || y != p_v.y || z != p_v.z;
}

constexpr bool Vector3::operator<(const Vector3 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z < p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

constexpr bool Vector3::operator>(const Vector3 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z > p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

constexpr bool Vector3::operator<=(const Vector3 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z <= p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

constexpr bool Vector3::operator>=(const Vector3 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z >= p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

_FORCE_INLINE_ Vector3 vec3_cross(const Vector3 &p_a, const Vector3 &p_b) {
	return p_a.cross(p_b);
}

_FORCE_INLINE_ real_t vec3_dot(const Vector3 &p_a, const Vector3 &p_b) {
	return p_a.dot(p_b);
}

real_t Vector3::length() const {
	real_t x2 = x * x;
	real_t y2 = y * y;
	real_t z2 = z * z;

	return Math::sqrt(x2 + y2 + z2);
}

real_t Vector3::length_squared() const {
	real_t x2 = x * x;
	real_t y2 = y * y;
	real_t z2 = z * z;

	return x2 + y2 + z2;
}

void Vector3::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector3 cannot be normalized, the elements must be finite. Making (0, 0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	real_t l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
		z /= l;
	}
}

Vector3 Vector3::normalized() const {
	Vector3 v = *this;
	v.normalize();
	return v;
}

bool Vector3::is_normalized() const {
	// use length_squared() instead of length() to avoid sqrt(), makes it more stringent.
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON);
}

Vector3 Vector3::inverse() const {
	return Vector3(1.0f / x, 1.0f / y, 1.0f / z);
}

void Vector3::zero() {
	x = y = z = 0;
}

// slide returns the component of the vector along the given plane, specified by its normal vector.
Vector3 Vector3::slide(const Vector3 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector3(), "The normal Vector3 " + p_normal.operator String() + " must be normalized.");
#endif
	return *this - p_normal * dot(p_normal);
}

Vector3 Vector3::bounce(const Vector3 &p_normal) const {
	return -reflect(p_normal);
}

Vector3 Vector3::reflect(const Vector3 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector3(), "The normal Vector3 " + p_normal.operator String() + " must be normalized.");
#endif
	return 2.0f * p_normal * dot(p_normal) - *this;
}

template <>
struct is_zero_constructible<Vector3> : std::true_type {};

// --- Folded from core/math/vector3i.h ---
#include "core/typedefs.h"
#include "core/math/vector2.h"

class String;
struct Vector3;

struct [[nodiscard]] Vector3i
{
	static const Vector3i LEFT;
	static const Vector3i RIGHT;
	static const Vector3i UP;
	static const Vector3i DOWN;
	static const Vector3i FORWARD;
	static const Vector3i BACK;

	static constexpr int AXIS_COUNT = 3;

	enum Axis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
	};

	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;

	constexpr int32_t& operator[](int p_axis)
	{
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Vector3i, x) == 0 * sizeof(int32_t));
		static_assert(offsetof(Vector3i, y) == 1 * sizeof(int32_t));
		static_assert(offsetof(Vector3i, z) == 2 * sizeof(int32_t));
		static_assert(sizeof(Vector3i) == 3 * sizeof(int32_t));

		DEV_ASSERT((unsigned int)p_axis < 3);
		return (&x)[p_axis];
	}

	constexpr const int32_t& operator[](int p_axis) const
	{
		DEV_ASSERT((unsigned int)p_axis < 3);
		return (&x)[p_axis];
	}

	Vector3i::Axis min_axis_index() const;
	Vector3i::Axis max_axis_index() const;

	Vector3i min(const Vector3i& p_vector3i) const
	{
		return Vector3i(MIN(x, p_vector3i.x), MIN(y, p_vector3i.y), MIN(z, p_vector3i.z));
	}

	Vector3i mini(int32_t p_scalar) const
	{
		return Vector3i(MIN(x, p_scalar), MIN(y, p_scalar), MIN(z, p_scalar));
	}

	Vector3i max(const Vector3i& p_vector3i) const
	{
		return Vector3i(MAX(x, p_vector3i.x), MAX(y, p_vector3i.y), MAX(z, p_vector3i.z));
	}

	Vector3i maxi(int32_t p_scalar) const
	{
		return Vector3i(MAX(x, p_scalar), MAX(y, p_scalar), MAX(z, p_scalar));
	}

	_FORCE_INLINE_ int64_t length_squared() const;
	_FORCE_INLINE_ double length() const;

	_FORCE_INLINE_ void zero();

	_FORCE_INLINE_ Vector3i abs() const;
	_FORCE_INLINE_ Vector3i sign() const;
	Vector3i clamp(const Vector3i& p_min, const Vector3i& p_max) const;
	Vector3i clampi(int32_t p_min, int32_t p_max) const;
	Vector3i snapped(const Vector3i& p_step) const;
	Vector3i snappedi(int32_t p_step) const;

	_FORCE_INLINE_ double distance_to(const Vector3i& p_to) const;
	_FORCE_INLINE_ int64_t distance_squared_to(const Vector3i& p_to) const;

	/* Operators */

	constexpr Vector3i& operator+=(const Vector3i& p_v);
	constexpr Vector3i operator+(const Vector3i& p_v) const;
	constexpr Vector3i& operator-=(const Vector3i& p_v);
	constexpr Vector3i operator-(const Vector3i& p_v) const;
	constexpr Vector3i& operator*=(const Vector3i& p_v);
	constexpr Vector3i operator*(const Vector3i& p_v) const;
	constexpr Vector3i& operator/=(const Vector3i& p_v);
	constexpr Vector3i operator/(const Vector3i& p_v) const;
	constexpr Vector3i& operator%=(const Vector3i& p_v);
	constexpr Vector3i operator%(const Vector3i& p_v) const;

	constexpr Vector3i& operator*=(int32_t p_scalar);
	constexpr Vector3i operator*(int32_t p_scalar) const;
	constexpr Vector3i& operator/=(int32_t p_scalar);
	constexpr Vector3i operator/(int32_t p_scalar) const;
	constexpr Vector3i& operator%=(int32_t p_scalar);
	constexpr Vector3i operator%(int32_t p_scalar) const;

	constexpr Vector3i operator-() const;

	constexpr bool operator==(const Vector3i& p_v) const;
	constexpr bool operator!=(const Vector3i& p_v) const;
	constexpr bool operator<(const Vector3i& p_v) const;
	constexpr bool operator<=(const Vector3i& p_v) const;
	constexpr bool operator>(const Vector3i& p_v) const;
	constexpr bool operator>=(const Vector3i& p_v) const;

	explicit operator String() const;
	operator Vector3() const;

	uint32_t hash() const
	{
		uint32_t h = hash_murmur3_one_32(uint32_t(x));
		h = hash_murmur3_one_32(uint32_t(y), h);
		h = hash_murmur3_one_32(uint32_t(z), h);
		return hash_fmix32(h);
	}

	constexpr Vector3i() = default;

	constexpr Vector3i(int32_t p_x, int32_t p_y, int32_t p_z) : x(p_x), y(p_y), z(p_z) {}
};

inline constexpr Vector3i Vector3i::LEFT = {-1, 0, 0};
inline constexpr Vector3i Vector3i::RIGHT = {1, 0, 0};
inline constexpr Vector3i Vector3i::UP = {0, 1, 0};
inline constexpr Vector3i Vector3i::DOWN = {0, -1, 0};
inline constexpr Vector3i Vector3i::FORWARD = {0, 0, -1};
inline constexpr Vector3i Vector3i::BACK = {0, 0, 1};

int64_t Vector3i::length_squared() const
{
	return x * (int64_t)x + y * (int64_t)y + z * (int64_t)z;
}

double Vector3i::length() const { return Math::sqrt((double)length_squared()); }

Vector3i Vector3i::abs() const { return Vector3i(Math::abs(x), Math::abs(y), Math::abs(z)); }

Vector3i Vector3i::sign() const { return Vector3i(SIGN(x), SIGN(y), SIGN(z)); }

double Vector3i::distance_to(const Vector3i& p_to) const { return (p_to - *this).length(); }

int64_t Vector3i::distance_squared_to(const Vector3i& p_to) const
{
	return (p_to - *this).length_squared();
}

/* Operators */

constexpr Vector3i& Vector3i::operator+=(const Vector3i& p_v)
{
	x += p_v.x;
	y += p_v.y;
	z += p_v.z;
	return *this;
}

constexpr Vector3i Vector3i::operator+(const Vector3i& p_v) const
{
	return Vector3i(x + p_v.x, y + p_v.y, z + p_v.z);
}

constexpr Vector3i& Vector3i::operator-=(const Vector3i& p_v)
{
	x -= p_v.x;
	y -= p_v.y;
	z -= p_v.z;
	return *this;
}

constexpr Vector3i Vector3i::operator-(const Vector3i& p_v) const
{
	return Vector3i(x - p_v.x, y - p_v.y, z - p_v.z);
}

constexpr Vector3i& Vector3i::operator*=(const Vector3i& p_v)
{
	x *= p_v.x;
	y *= p_v.y;
	z *= p_v.z;
	return *this;
}

constexpr Vector3i Vector3i::operator*(const Vector3i& p_v) const
{
	return Vector3i(x * p_v.x, y * p_v.y, z * p_v.z);
}

constexpr Vector3i& Vector3i::operator/=(const Vector3i& p_v)
{
	x /= p_v.x;
	y /= p_v.y;
	z /= p_v.z;
	return *this;
}

constexpr Vector3i Vector3i::operator/(const Vector3i& p_v) const
{
	return Vector3i(x / p_v.x, y / p_v.y, z / p_v.z);
}

constexpr Vector3i& Vector3i::operator%=(const Vector3i& p_v)
{
	x %= p_v.x;
	y %= p_v.y;
	z %= p_v.z;

	return *this;
}

constexpr Vector3i Vector3i::operator%(const Vector3i& p_v) const
{
	return Vector3i(x % p_v.x, y % p_v.y, z % p_v.z);
}

constexpr Vector3i& Vector3i::operator*=(int32_t p_scalar)
{
	x *= p_scalar;
	y *= p_scalar;
	z *= p_scalar;
	return *this;
}

constexpr Vector3i Vector3i::operator*(int32_t p_scalar) const
{
	return Vector3i(x * p_scalar, y * p_scalar, z * p_scalar);
}

// Multiplication operators required to workaround issues with LLVM using implicit conversion.

constexpr Vector3i operator*(int32_t p_scalar, const Vector3i& p_vector)
{
	return p_vector * p_scalar;
}

constexpr Vector3i operator*(int64_t p_scalar, const Vector3i& p_vector)
{
	return p_vector * p_scalar;
}

constexpr Vector3i operator*(float p_scalar, const Vector3i& p_vector)
{
	return p_vector * p_scalar;
}

constexpr Vector3i operator*(double p_scalar, const Vector3i& p_vector)
{
	return p_vector * p_scalar;
}

constexpr Vector3i& Vector3i::operator/=(int32_t p_scalar)
{
	x /= p_scalar;
	y /= p_scalar;
	z /= p_scalar;
	return *this;
}

constexpr Vector3i Vector3i::operator/(int32_t p_scalar) const
{
	return Vector3i(x / p_scalar, y / p_scalar, z / p_scalar);
}

constexpr Vector3i& Vector3i::operator%=(int32_t p_scalar)
{
	x %= p_scalar;
	y %= p_scalar;
	z %= p_scalar;
	return *this;
}

constexpr Vector3i Vector3i::operator%(int32_t p_scalar) const
{
	return Vector3i(x % p_scalar, y % p_scalar, z % p_scalar);
}

constexpr Vector3i Vector3i::operator-() const { return Vector3i(-x, -y, -z); }

constexpr bool Vector3i::operator==(const Vector3i& p_v) const
{
	return (x == p_v.x && y == p_v.y && z == p_v.z);
}

constexpr bool Vector3i::operator!=(const Vector3i& p_v) const
{
	return (x != p_v.x || y != p_v.y || z != p_v.z);
}

constexpr bool Vector3i::operator<(const Vector3i& p_v) const
{
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z < p_v.z;
		}
		else {
			return y < p_v.y;
		}
	}
	else {
		return x < p_v.x;
	}
}

constexpr bool Vector3i::operator>(const Vector3i& p_v) const
{
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z > p_v.z;
		}
		else {
			return y > p_v.y;
		}
	}
	else {
		return x > p_v.x;
	}
}

constexpr bool Vector3i::operator<=(const Vector3i& p_v) const
{
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z <= p_v.z;
		}
		else {
			return y < p_v.y;
		}
	}
	else {
		return x < p_v.x;
	}
}

constexpr bool Vector3i::operator>=(const Vector3i& p_v) const
{
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z >= p_v.z;
		}
		else {
			return y > p_v.y;
		}
	}
	else {
		return x > p_v.x;
	}
}

void Vector3i::zero() { x = y = z = 0; }

template <> struct is_zero_constructible<Vector3i> : std::true_type
{
};
// --- End core/math/vector3.h ---

struct [[nodiscard]] Plane
{
	static const Plane PLANE_YZ;
	static const Plane PLANE_XZ;
	static const Plane PLANE_XY;

	Vector3 normal;
	real_t d = 0;

	void set_normal(const Vector3& p_normal);

	_FORCE_INLINE_ Vector3 get_normal() const { return normal; }

	void zero()
	{
		normal.zero();
		d = 0;
	}

	void normalize();
	Plane normalized() const;

	/* Plane-Point operations */

	_FORCE_INLINE_ Vector3 get_center() const { return normal * d; }

	Vector3 get_any_perpendicular_normal() const;

	_FORCE_INLINE_ bool is_point_over(const Vector3& p_point) const; ///< Point is over plane
	_FORCE_INLINE_ real_t distance_to(const Vector3& p_point) const;
	_FORCE_INLINE_ bool has_point(const Vector3& p_point, real_t p_tolerance = CMP_EPSILON) const;

	/* intersections */

	bool intersect_3(
		const Plane& p_plane1, const Plane& p_plane2, Vector3* r_result = nullptr) const;
	bool intersects_ray(const Vector3& p_from, const Vector3& p_dir, Vector3* p_intersection) const;
	bool intersects_segment(
		const Vector3& p_begin, const Vector3& p_end, Vector3* p_intersection) const;

	_FORCE_INLINE_ Vector3 project(const Vector3& p_point) const
	{
		return p_point - normal * distance_to(p_point);
	}

	/* misc */

	constexpr Plane operator-() const { return Plane(-normal, -d); }

	bool is_equal_approx(const Plane& p_plane) const;
	bool is_same(const Plane& p_plane) const;
	bool is_equal_approx_any_side(const Plane& p_plane) const;
	bool is_finite() const;

	constexpr bool operator==(const Plane& p_plane) const;
	constexpr bool operator!=(const Plane& p_plane) const;
	explicit operator String() const;

	Plane() = default;

	constexpr Plane(real_t p_a, real_t p_b, real_t p_c, real_t p_d) : normal(p_a, p_b, p_c), d(p_d)
	{
	}

	constexpr Plane(const Vector3& p_normal, real_t p_d = 0.0);
	_FORCE_INLINE_ Plane(const Vector3& p_normal, const Vector3& p_point);
	_FORCE_INLINE_ Plane(const Vector3& p_point1, const Vector3& p_point2, const Vector3& p_point3,
		ClockDirection p_dir = CLOCKWISE);
};

inline constexpr Plane Plane::PLANE_YZ = {1, 0, 0, 0};
inline constexpr Plane Plane::PLANE_XZ = {0, 1, 0, 0};
inline constexpr Plane Plane::PLANE_XY = {0, 0, 1, 0};

bool Plane::is_point_over(const Vector3& p_point) const { return (normal.dot(p_point) > d); }

real_t Plane::distance_to(const Vector3& p_point) const { return (normal.dot(p_point) - d); }

bool Plane::has_point(const Vector3& p_point, real_t p_tolerance) const
{
	real_t dist = normal.dot(p_point) - d;
	dist = Math::abs(dist);
	return (dist <= p_tolerance);
}

constexpr Plane::Plane(const Vector3& p_normal, real_t p_d) : normal(p_normal), d(p_d) {}

Plane::Plane(const Vector3& p_normal, const Vector3& p_point)
	: normal(p_normal), d(p_normal.dot(p_point))
{
}

Plane::Plane(
	const Vector3& p_point1, const Vector3& p_point2, const Vector3& p_point3, ClockDirection p_dir)
{
	if (p_dir == CLOCKWISE) {
		normal = (p_point1 - p_point3).cross(p_point1 - p_point2);
	}
	else {
		normal = (p_point1 - p_point2).cross(p_point1 - p_point3);
	}

	normal.normalize();
	d = normal.dot(p_point1);
}

constexpr bool Plane::operator==(const Plane& p_plane) const
{
	return normal == p_plane.normal && d == p_plane.d;
}

constexpr bool Plane::operator!=(const Plane& p_plane) const
{
	return normal != p_plane.normal || d != p_plane.d;
}

template <> struct is_zero_constructible<Plane> : std::true_type
{
};
// --- End core/math/plane.h ---
#include "core/math/vector2.h"

/**
 * AABB (Axis Aligned Bounding Box)
 * This is implemented by a point (position) and the box size.
 */

struct [[nodiscard]] AABB {
	Vector3 position;
	Vector3 size;

	real_t get_volume() const;
	_FORCE_INLINE_ bool has_volume() const {
		return size.x > 0.0f && size.y > 0.0f && size.z > 0.0f;
	}

	_FORCE_INLINE_ bool has_surface() const {
		return size.x > 0.0f || size.y > 0.0f || size.z > 0.0f;
	}

	const Vector3 &get_position() const { return position; }
	void set_position(const Vector3 &p_pos) { position = p_pos; }
	const Vector3 &get_size() const { return size; }
	void set_size(const Vector3 &p_size) { size = p_size; }

	constexpr bool operator==(const AABB &p_rval) const {
		return position == p_rval.position && size == p_rval.size;
	}
	constexpr bool operator!=(const AABB &p_rval) const {
		return position != p_rval.position || size != p_rval.size;
	}

	bool is_equal_approx(const AABB &p_aabb) const;
	bool is_same(const AABB &p_aabb) const;
	bool is_finite() const;
	_FORCE_INLINE_ bool intersects(const AABB &p_aabb) const; /// Both AABBs overlap
	_FORCE_INLINE_ bool intersects_inclusive(const AABB &p_aabb) const; /// Both AABBs (or their faces) overlap
	_FORCE_INLINE_ bool encloses(const AABB &p_aabb) const; /// p_aabb is completely inside this

	AABB merge(const AABB &p_with) const;
	void merge_with(const AABB &p_aabb); ///merge with another AABB
	AABB intersection(const AABB &p_aabb) const; ///get box where two intersect, empty if no intersection occurs
	_FORCE_INLINE_ bool smits_intersect_ray(const Vector3 &p_from, const Vector3 &p_dir, real_t p_t0, real_t p_t1) const;

	bool intersects_segment(const Vector3 &p_from, const Vector3 &p_to, Vector3 *r_intersection_point = nullptr, Vector3 *r_normal = nullptr) const;
	bool intersects_ray(const Vector3 &p_from, const Vector3 &p_dir) const {
		bool inside;
		return find_intersects_ray(p_from, p_dir, inside);
	}
	bool find_intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, bool &r_inside, Vector3 *r_intersection_point = nullptr, Vector3 *r_normal = nullptr) const;

	_FORCE_INLINE_ bool intersects_convex_shape(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count) const;
	_FORCE_INLINE_ bool inside_convex_shape(const Plane *p_planes, int p_plane_count) const;
	bool intersects_plane(const Plane &p_plane) const;

	_FORCE_INLINE_ bool has_point(const Vector3 &p_point) const;
	_FORCE_INLINE_ Vector3 get_support(const Vector3 &p_direction) const;

	Vector3 get_longest_axis() const;
	int get_longest_axis_index() const;
	_FORCE_INLINE_ real_t get_longest_axis_size() const;

	Vector3 get_shortest_axis() const;
	int get_shortest_axis_index() const;
	_FORCE_INLINE_ real_t get_shortest_axis_size() const;

	AABB grow(real_t p_by) const;
	_FORCE_INLINE_ void grow_by(real_t p_amount);

	void get_edge(int p_edge, Vector3 &r_from, Vector3 &r_to) const;
	_FORCE_INLINE_ Vector3 get_endpoint(int p_point) const;

	AABB expand(const Vector3 &p_vector) const;
	_FORCE_INLINE_ void project_range_in_plane(const Plane &p_plane, real_t &r_min, real_t &r_max) const;
	_FORCE_INLINE_ void expand_to(const Vector3 &p_vector); /** expand to contain a point if necessary */

	_FORCE_INLINE_ AABB abs() const {
		return AABB(position + size.minf(0), size.abs());
	}

	_FORCE_INLINE_ void quantize(real_t p_unit);
	_FORCE_INLINE_ AABB quantized(real_t p_unit) const;

	_FORCE_INLINE_ void set_end(const Vector3 &p_end) {
		size = p_end - position;
	}

	_FORCE_INLINE_ Vector3 get_end() const {
		return position + size;
	}

	_FORCE_INLINE_ Vector3 get_center() const {
		return position + (size * 0.5f);
	}

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_real(position.x);
		h = hash_murmur3_one_real(position.y, h);
		h = hash_murmur3_one_real(position.z, h);
		h = hash_murmur3_one_real(size.x, h);
		h = hash_murmur3_one_real(size.y, h);
		h = hash_murmur3_one_real(size.z, h);
		return hash_fmix32(h);
	}

	explicit operator String() const;

	AABB() = default;
	constexpr AABB(const Vector3 &p_pos, const Vector3 &p_size) :
			position(p_pos),
			size(p_size) {
	}
};

inline bool AABB::intersects(const AABB &p_aabb) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 || p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	if (position.x >= (p_aabb.position.x + p_aabb.size.x)) {
		return false;
	}
	if ((position.x + size.x) <= p_aabb.position.x) {
		return false;
	}
	if (position.y >= (p_aabb.position.y + p_aabb.size.y)) {
		return false;
	}
	if ((position.y + size.y) <= p_aabb.position.y) {
		return false;
	}
	if (position.z >= (p_aabb.position.z + p_aabb.size.z)) {
		return false;
	}
	if ((position.z + size.z) <= p_aabb.position.z) {
		return false;
	}

	return true;
}

inline bool AABB::intersects_inclusive(const AABB &p_aabb) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 || p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	if (position.x > (p_aabb.position.x + p_aabb.size.x)) {
		return false;
	}
	if ((position.x + size.x) < p_aabb.position.x) {
		return false;
	}
	if (position.y > (p_aabb.position.y + p_aabb.size.y)) {
		return false;
	}
	if ((position.y + size.y) < p_aabb.position.y) {
		return false;
	}
	if (position.z > (p_aabb.position.z + p_aabb.size.z)) {
		return false;
	}
	if ((position.z + size.z) < p_aabb.position.z) {
		return false;
	}

	return true;
}

inline bool AABB::encloses(const AABB &p_aabb) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 || p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	Vector3 src_min = position;
	Vector3 src_max = position + size;
	Vector3 dst_min = p_aabb.position;
	Vector3 dst_max = p_aabb.position + p_aabb.size;

	return (
			(src_min.x <= dst_min.x) &&
			(src_max.x >= dst_max.x) &&
			(src_min.y <= dst_min.y) &&
			(src_max.y >= dst_max.y) &&
			(src_min.z <= dst_min.z) &&
			(src_max.z >= dst_max.z));
}

Vector3 AABB::get_support(const Vector3 &p_direction) const {
	Vector3 support = position;
	if (p_direction.x > 0.0f) {
		support.x += size.x;
	}
	if (p_direction.y > 0.0f) {
		support.y += size.y;
	}
	if (p_direction.z > 0.0f) {
		support.z += size.z;
	}
	return support;
}

Vector3 AABB::get_endpoint(int p_point) const {
	switch (p_point) {
		case 0:
			return Vector3(position.x, position.y, position.z);
		case 1:
			return Vector3(position.x, position.y, position.z + size.z);
		case 2:
			return Vector3(position.x, position.y + size.y, position.z);
		case 3:
			return Vector3(position.x, position.y + size.y, position.z + size.z);
		case 4:
			return Vector3(position.x + size.x, position.y, position.z);
		case 5:
			return Vector3(position.x + size.x, position.y, position.z + size.z);
		case 6:
			return Vector3(position.x + size.x, position.y + size.y, position.z);
		case 7:
			return Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
	}

	ERR_FAIL_V(Vector3());
}

bool AABB::intersects_convex_shape(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count) const {
	Vector3 half_extents = size * 0.5f;
	Vector3 ofs = position + half_extents;

	for (int i = 0; i < p_plane_count; i++) {
		const Plane &p = p_planes[i];
		Vector3 point(
				(p.normal.x > 0) ? -half_extents.x : half_extents.x,
				(p.normal.y > 0) ? -half_extents.y : half_extents.y,
				(p.normal.z > 0) ? -half_extents.z : half_extents.z);
		point += ofs;
		if (p.is_point_over(point)) {
			return false;
		}
	}

	// Make sure all points in the shape aren't fully separated from the AABB on
	// each axis.
	int bad_point_counts_positive[3] = { 0 };
	int bad_point_counts_negative[3] = { 0 };

	for (int k = 0; k < 3; k++) {
		for (int i = 0; i < p_point_count; i++) {
			if (p_points[i][k] > ofs[k] + half_extents[k]) {
				bad_point_counts_positive[k]++;
			}
			if (p_points[i][k] < ofs[k] - half_extents[k]) {
				bad_point_counts_negative[k]++;
			}
		}

		if (bad_point_counts_negative[k] == p_point_count) {
			return false;
		}
		if (bad_point_counts_positive[k] == p_point_count) {
			return false;
		}
	}

	return true;
}

bool AABB::inside_convex_shape(const Plane *p_planes, int p_plane_count) const {
	Vector3 half_extents = size * 0.5f;
	Vector3 ofs = position + half_extents;

	for (int i = 0; i < p_plane_count; i++) {
		const Plane &p = p_planes[i];
		Vector3 point(
				(p.normal.x < 0) ? -half_extents.x : half_extents.x,
				(p.normal.y < 0) ? -half_extents.y : half_extents.y,
				(p.normal.z < 0) ? -half_extents.z : half_extents.z);
		point += ofs;
		if (p.is_point_over(point)) {
			return false;
		}
	}

	return true;
}

bool AABB::has_point(const Vector3 &p_point) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	if (p_point.x < position.x) {
		return false;
	}
	if (p_point.y < position.y) {
		return false;
	}
	if (p_point.z < position.z) {
		return false;
	}
	if (p_point.x > position.x + size.x) {
		return false;
	}
	if (p_point.y > position.y + size.y) {
		return false;
	}
	if (p_point.z > position.z + size.z) {
		return false;
	}

	return true;
}

inline void AABB::expand_to(const Vector3 &p_vector) {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	Vector3 begin = position;
	Vector3 end = position + size;

	if (p_vector.x < begin.x) {
		begin.x = p_vector.x;
	}
	if (p_vector.y < begin.y) {
		begin.y = p_vector.y;
	}
	if (p_vector.z < begin.z) {
		begin.z = p_vector.z;
	}

	if (p_vector.x > end.x) {
		end.x = p_vector.x;
	}
	if (p_vector.y > end.y) {
		end.y = p_vector.y;
	}
	if (p_vector.z > end.z) {
		end.z = p_vector.z;
	}

	position = begin;
	size = end - begin;
}

void AABB::project_range_in_plane(const Plane &p_plane, real_t &r_min, real_t &r_max) const {
	Vector3 half_extents(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	Vector3 center(position.x + half_extents.x, position.y + half_extents.y, position.z + half_extents.z);

	real_t length = p_plane.normal.abs().dot(half_extents);
	real_t distance = p_plane.distance_to(center);
	r_min = distance - length;
	r_max = distance + length;
}

inline real_t AABB::get_longest_axis_size() const {
	real_t max_size = size.x;

	if (size.y > max_size) {
		max_size = size.y;
	}

	if (size.z > max_size) {
		max_size = size.z;
	}

	return max_size;
}

inline real_t AABB::get_shortest_axis_size() const {
	real_t max_size = size.x;

	if (size.y < max_size) {
		max_size = size.y;
	}

	if (size.z < max_size) {
		max_size = size.z;
	}

	return max_size;
}

bool AABB::smits_intersect_ray(const Vector3 &p_from, const Vector3 &p_dir, real_t p_t0, real_t p_t1) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB with a positive size.");
	}
#endif
	real_t divx = 1.0f / p_dir.x;
	real_t divy = 1.0f / p_dir.y;
	real_t divz = 1.0f / p_dir.z;

	Vector3 upbound = position + size;
	real_t tmin, tmax, tymin, tymax, tzmin, tzmax;
	if (p_dir.x >= 0) {
		tmin = (position.x - p_from.x) * divx;
		tmax = (upbound.x - p_from.x) * divx;
	} else {
		tmin = (upbound.x - p_from.x) * divx;
		tmax = (position.x - p_from.x) * divx;
	}
	if (p_dir.y >= 0) {
		tymin = (position.y - p_from.y) * divy;
		tymax = (upbound.y - p_from.y) * divy;
	} else {
		tymin = (upbound.y - p_from.y) * divy;
		tymax = (position.y - p_from.y) * divy;
	}
	if ((tmin > tymax) || (tymin > tmax)) {
		return false;
	}
	if (tymin > tmin) {
		tmin = tymin;
	}
	if (tymax < tmax) {
		tmax = tymax;
	}
	if (p_dir.z >= 0) {
		tzmin = (position.z - p_from.z) * divz;
		tzmax = (upbound.z - p_from.z) * divz;
	} else {
		tzmin = (upbound.z - p_from.z) * divz;
		tzmax = (position.z - p_from.z) * divz;
	}
	if ((tmin > tzmax) || (tzmin > tmax)) {
		return false;
	}
	if (tzmin > tmin) {
		tmin = tzmin;
	}
	if (tzmax < tmax) {
		tmax = tzmax;
	}
	return ((tmin < p_t1) && (tmax > p_t0));
}

void AABB::grow_by(real_t p_amount) {
	position.x -= p_amount;
	position.y -= p_amount;
	position.z -= p_amount;
	size.x += 2.0f * p_amount;
	size.y += 2.0f * p_amount;
	size.z += 2.0f * p_amount;
}

void AABB::quantize(real_t p_unit) {
	size += position;

	position.x -= Math::fposmodp(position.x, p_unit);
	position.y -= Math::fposmodp(position.y, p_unit);
	position.z -= Math::fposmodp(position.z, p_unit);

	size.x -= Math::fposmodp(size.x, p_unit);
	size.y -= Math::fposmodp(size.y, p_unit);
	size.z -= Math::fposmodp(size.z, p_unit);

	size.x += p_unit;
	size.y += p_unit;
	size.z += p_unit;

	size -= position;
}

AABB AABB::quantized(real_t p_unit) const {
	AABB ret = *this;
	ret.quantize(p_unit);
	return ret;
}

template <>
struct is_zero_constructible<AABB> : std::true_type {};
// --- End core/math/aabb.h ---
// --- Inlined/Folded from core/math/basis.h ---
// --- Inlined/Folded from core/math/quaternion.h ---
#include "core/typedefs.h"

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

#include "core/string/ustring.h"

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
// --- End core/math/transform_3d.h ---
#include "core/math/vector2.h"
#include "core/typedefs.h"

// --- Folded from core/math/face3.h ---




struct [[nodiscard]] Face3 {
	enum Side {
		SIDE_OVER,
		SIDE_UNDER,
		SIDE_SPANNING,
		SIDE_COPLANAR
	};

	Vector3 vertex[3];

	/**
	 * @param p_plane plane used to split the face
	 * @param p_res array of at least 3 faces, amount used in function return
	 * @param p_is_point_over array of at least 3 booleans, determining which face is over the plane, amount used in function return
	 * @return amount of faces generated by the split, either 0 (means no split possible), 2 or 3
	 */
	int split_by_plane(const Plane &p_plane, Face3 *p_res, bool *p_is_point_over) const;

	Plane get_plane(ClockDirection p_dir = CLOCKWISE) const;
	Vector3 get_random_point_inside() const;

	bool is_degenerate() const;
	real_t get_area() const;

	Vector3 get_closest_point_to(const Vector3 &p_point) const;

	bool intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection = nullptr) const;
	bool intersects_segment(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection = nullptr) const;

	void get_support(const Vector3 &p_normal, const Transform3D &p_transform, Vector3 *p_vertices, int *p_count, int p_max) const;
	void project_range(const Vector3 &p_normal, const Transform3D &p_transform, real_t &r_min, real_t &r_max) const;

	AABB get_aabb() const {
		AABB aabb(vertex[0], Vector3());
		aabb.expand_to(vertex[1]);
		aabb.expand_to(vertex[2]);
		return aabb;
	}

	bool intersects_aabb(const AABB &p_aabb) const;
	_FORCE_INLINE_ bool intersects_aabb2(const AABB &p_aabb) const;
	explicit operator String() const;

	Face3() = default;
	constexpr Face3(const Vector3 &p_v1, const Vector3 &p_v2, const Vector3 &p_v3) :
			vertex{ p_v1, p_v2, p_v3 } {}
};

bool Face3::intersects_aabb2(const AABB &p_aabb) const {
	Vector3 perp = (vertex[0] - vertex[2]).cross(vertex[0] - vertex[1]);

	Vector3 half_extents = p_aabb.size * 0.5f;
	Vector3 ofs = p_aabb.position + half_extents;

	Vector3 sup = Vector3(
			(perp.x > 0) ? -half_extents.x : half_extents.x,
			(perp.y > 0) ? -half_extents.y : half_extents.y,
			(perp.z > 0) ? -half_extents.z : half_extents.z);

	real_t d = perp.dot(vertex[0]);
	real_t dist_a = perp.dot(ofs + sup) - d;
	real_t dist_b = perp.dot(ofs - sup) - d;

	if (dist_a * dist_b > 0) {
		return false; //does not intersect the plane
	}

#define TEST_AXIS(m_ax) \
	{ \
		real_t aabb_min = p_aabb.position.m_ax; \
		real_t aabb_max = p_aabb.position.m_ax + p_aabb.size.m_ax; \
		real_t tri_min, tri_max; \
		for (int i = 0; i < 3; i++) { \
			if (i == 0 || vertex[i].m_ax > tri_max) \
				tri_max = vertex[i].m_ax; \
			if (i == 0 || vertex[i].m_ax < tri_min) \
				tri_min = vertex[i].m_ax; \
		} \
\
		if (tri_max < aabb_min || aabb_max < tri_min) \
			return false; \
	}

	TEST_AXIS(x);
	TEST_AXIS(y);
	TEST_AXIS(z);

#undef TEST_AXIS

	const Vector3 edge_norms[3] = {
		vertex[0] - vertex[1],
		vertex[1] - vertex[2],
		vertex[2] - vertex[0],
	};

	for (int i = 0; i < 12; i++) {
		Vector3 from, to;
		switch (i) {
			case 0: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
			} break;
			case 1: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
			} break;
			case 2: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 3: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 4: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
			} break;
			case 5: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
			} break;
			case 6: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 7: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 8: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 9: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 10: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 11: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
		}

		Vector3 e1 = from - to;
		for (int j = 0; j < 3; j++) {
			Vector3 e2 = edge_norms[j];

			Vector3 axis = vec3_cross(e1, e2);

			if (axis.length_squared() < 0.0001f) {
				continue; // coplanar
			}
			//axis.normalize();

			Vector3 sup2 = Vector3(
					(axis.x > 0) ? -half_extents.x : half_extents.x,
					(axis.y > 0) ? -half_extents.y : half_extents.y,
					(axis.z > 0) ? -half_extents.z : half_extents.z);

			real_t maxB = axis.dot(ofs + sup2);
			real_t minB = axis.dot(ofs - sup2);
			if (minB > maxB) {
				SWAP(maxB, minB);
			}

			real_t minT = 1e20, maxT = -1e20;
			for (int k = 0; k < 3; k++) {
				real_t vert_d = axis.dot(vertex[k]);

				if (vert_d > maxT) {
					maxT = vert_d;
				}

				if (vert_d < minT) {
					minT = vert_d;
				}
			}

			if (maxB < minT || maxT < minB) {
				return false;
			}
		}
	}
	return true;
}

template <>
struct is_zero_constructible<Face3> : std::true_type {};
// --- End core/math/face3.h ---
// --- Folded from core/math/rect2.h ---
#include "core/typedefs.h"
#include "core/math/vector2.h"

class String;
struct Rect2i;
struct Transform2D;

struct [[nodiscard]] Rect2
{
	Point2 position;
	Size2 size;

	const Vector2& get_position() const { return position; }

	void set_position(const Vector2& p_pos) { position = p_pos; }

	const Vector2& get_size() const { return size; }

	void set_size(const Vector2& p_size) { size = p_size; }

	real_t get_area() const { return size.width * size.height; }

	_FORCE_INLINE_ Vector2 get_center() const { return position + (size * 0.5f); }

	inline bool intersects(const Rect2& p_rect, bool p_include_borders = false) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		if (p_include_borders) {
			if (position.x > (p_rect.position.x + p_rect.size.width)) {
				return false;
			}
			if ((position.x + size.width) < p_rect.position.x) {
				return false;
			}
			if (position.y > (p_rect.position.y + p_rect.size.height)) {
				return false;
			}
			if ((position.y + size.height) < p_rect.position.y) {
				return false;
			}
		}
		else {
			if (position.x >= (p_rect.position.x + p_rect.size.width)) {
				return false;
			}
			if ((position.x + size.width) <= p_rect.position.x) {
				return false;
			}
			if (position.y >= (p_rect.position.y + p_rect.size.height)) {
				return false;
			}
			if ((position.y + size.height) <= p_rect.position.y) {
				return false;
			}
		}

		return true;
	}

	inline real_t distance_to(const Vector2& p_point) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		real_t dist = 0.0;
		bool inside = true;

		if (p_point.x < position.x) {
			real_t d = position.x - p_point.x;
			dist = d;
			inside = false;
		}
		if (p_point.y < position.y) {
			real_t d = position.y - p_point.y;
			dist = inside ? d : MIN(dist, d);
			inside = false;
		}
		if (p_point.x >= (position.x + size.x)) {
			real_t d = p_point.x - (position.x + size.x);
			dist = inside ? d : MIN(dist, d);
			inside = false;
		}
		if (p_point.y >= (position.y + size.y)) {
			real_t d = p_point.y - (position.y + size.y);
			dist = inside ? d : MIN(dist, d);
			inside = false;
		}

		if (inside) {
			return 0;
		}
		else {
			return dist;
		}
	}

	bool intersects_transformed(const Transform2D& p_xform, const Rect2& p_rect) const;

	bool intersects_segment(const Point2& p_from, const Point2& p_to, Point2* r_pos = nullptr,
		Point2* r_normal = nullptr) const;

	inline bool encloses(const Rect2& p_rect) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		return (p_rect.position.x >= position.x) && (p_rect.position.y >= position.y) &&
			   ((p_rect.position.x + p_rect.size.x) <= (position.x + size.x)) &&
			   ((p_rect.position.y + p_rect.size.y) <= (position.y + size.y));
	}

	_FORCE_INLINE_ bool has_area() const { return size.x > 0.0f && size.y > 0.0f; }

	Rect2 intersection_transformed(const Transform2D& p_xform, const Rect2& p_rect) const;

	// Returns the intersection between two Rect2s or an empty Rect2 if there is no intersection.
	inline Rect2 intersection(const Rect2& p_rect) const
	{
		Rect2 new_rect = p_rect;

		if (!intersects(new_rect)) {
			return Rect2();
		}

		new_rect.position = p_rect.position.max(position);

		Point2 p_rect_end = p_rect.position + p_rect.size;
		Point2 end = position + size;

		new_rect.size = p_rect_end.min(end) - new_rect.position;

		return new_rect;
	}

	inline Rect2 merge(const Rect2& p_rect) const
	{ ///< return a merged rect
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		Rect2 new_rect;

		new_rect.position = p_rect.position.min(position);

		new_rect.size = (p_rect.position + p_rect.size).max(position + size);

		new_rect.size = new_rect.size - new_rect.position; // Make relative again.

		return new_rect;
	}

	inline bool has_point(const Point2& p_point) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		if (p_point.x < position.x) {
			return false;
		}
		if (p_point.y < position.y) {
			return false;
		}

		if (p_point.x >= (position.x + size.x)) {
			return false;
		}
		if (p_point.y >= (position.y + size.y)) {
			return false;
		}

		return true;
	}

	bool is_equal_approx(const Rect2& p_rect) const;
	bool is_same(const Rect2& p_rect) const;
	bool is_finite() const;

	constexpr bool operator==(const Rect2& p_rect) const
	{
		return position == p_rect.position && size == p_rect.size;
	}

	constexpr bool operator!=(const Rect2& p_rect) const
	{
		return position != p_rect.position || size != p_rect.size;
	}

	inline Rect2 grow(real_t p_amount) const
	{
		Rect2 g = *this;
		g.grow_by(p_amount);
		return g;
	}

	inline void grow_by(real_t p_amount)
	{
		position.x -= p_amount;
		position.y -= p_amount;
		size.width += p_amount * 2;
		size.height += p_amount * 2;
	}

	inline Rect2 grow_side(Side p_side, real_t p_amount) const
	{
		Rect2 g = *this;
		g = g.grow_individual((SIDE_LEFT == p_side) ? p_amount : 0,
			(SIDE_TOP == p_side) ? p_amount : 0, (SIDE_RIGHT == p_side) ? p_amount : 0,
			(SIDE_BOTTOM == p_side) ? p_amount : 0);
		return g;
	}

	inline Rect2 grow_side_bind(uint32_t p_side, real_t p_amount) const
	{
		return grow_side(Side(p_side), p_amount);
	}

	inline Rect2 grow_individual(real_t p_left, real_t p_top, real_t p_right, real_t p_bottom) const
	{
		Rect2 g = *this;
		g.position.x -= p_left;
		g.position.y -= p_top;
		g.size.width += p_left + p_right;
		g.size.height += p_top + p_bottom;

		return g;
	}

	_FORCE_INLINE_ Rect2 expand(const Vector2& p_vector) const
	{
		Rect2 r = *this;
		r.expand_to(p_vector);
		return r;
	}

	inline void expand_to(const Vector2& p_vector)
	{ // In place function for speed.
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0)) {
			ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a "
					  "Rect2 with a positive size.");
		}
#endif
		Vector2 begin = position;
		Vector2 end = position + size;

		if (p_vector.x < begin.x) {
			begin.x = p_vector.x;
		}
		if (p_vector.y < begin.y) {
			begin.y = p_vector.y;
		}

		if (p_vector.x > end.x) {
			end.x = p_vector.x;
		}
		if (p_vector.y > end.y) {
			end.y = p_vector.y;
		}

		position = begin;
		size = end - begin;
	}

	_FORCE_INLINE_ Rect2 abs() const { return Rect2(position + size.minf(0), size.abs()); }

	_FORCE_INLINE_ Rect2 round() const { return Rect2(position.round(), size.round()); }

	Vector2 get_support(const Vector2& p_direction) const
	{
		Vector2 support = position;
		if (p_direction.x > 0.0f) {
			support.x += size.x;
		}
		if (p_direction.y > 0.0f) {
			support.y += size.y;
		}
		return support;
	}

	_FORCE_INLINE_ bool intersects_filled_polygon(const Vector2* p_points, int p_point_count) const
	{
		Vector2 center = get_center();
		int side_plus = 0;
		int side_minus = 0;
		Vector2 end = position + size;

		int i_f = p_point_count - 1;
		for (int i = 0; i < p_point_count; i++) {
			const Vector2& a = p_points[i_f];
			const Vector2& b = p_points[i];
			i_f = i;

			Vector2 r = (b - a);
			const real_t l = r.length();
			if (l == 0.0f) {
				continue;
			}

			// Check inside.
			Vector2 tg = r.orthogonal();
			const real_t s = tg.dot(center) - tg.dot(a);
			if (s < 0.0f) {
				side_plus++;
			}
			else {
				side_minus++;
			}

			// Check ray box.
			r /= l;
			Vector2 ir(1.0f / r.x, 1.0f / r.y);

			// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
			// r.org is origin of ray
			Vector2 t13 = (position - a) * ir;
			Vector2 t24 = (end - a) * ir;

			const real_t tmin = MAX(MIN(t13.x, t24.x), MIN(t13.y, t24.y));
			const real_t tmax = MIN(MAX(t13.x, t24.x), MAX(t13.y, t24.y));

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if (tmax < 0 || tmin > tmax || tmin >= l) {
				continue;
			}

			return true;
		}

		if (side_plus * side_minus == 0) {
			return true; // All inside.
		}
		else {
			return false;
		}
	}

	_FORCE_INLINE_ void set_end(const Vector2& p_end) { size = p_end - position; }

	_FORCE_INLINE_ Vector2 get_end() const { return position + size; }

	explicit operator String() const;
	operator Rect2i() const;

	uint32_t hash() const
	{
		uint32_t h = hash_murmur3_one_real(position.x);
		h = hash_murmur3_one_real(position.y, h);
		h = hash_murmur3_one_real(size.x, h);
		h = hash_murmur3_one_real(size.y, h);
		return hash_fmix32(h);
	}

	static Rect2 from_points(const Vector2* p_points, int p_point_count)
	{
		Rect2 result;
		ERR_FAIL_NULL_V_MSG(p_points, result, "The pointer of points passed in is invalid.");
		ERR_FAIL_COND_V_MSG(
			p_point_count <= 0, result, "The number of points passed in is invalid.");
		result.position = p_points[0];
		Vector2 end = result.position;
		for (int i = 1; i < p_point_count; i++) {
			const Vector2& p = p_points[i];

			if (p.x < result.position.x) {
				result.position.x = p.x;
			}
			else if (p.x > end.x) {
				end.x = p.x;
			}
			if (p.y < result.position.y) {
				result.position.y = p.y;
			}
			else if (p.y > end.y) {
				end.y = p.y;
			}
		}
		result.size = end - result.position;
		return result;
	}

	Rect2() = default;

	constexpr Rect2(real_t p_x, real_t p_y, real_t p_width, real_t p_height)
		: position(Point2(p_x, p_y)), size(Size2(p_width, p_height))
	{
	}

	constexpr Rect2(const Point2& p_pos, const Size2& p_size) : position(p_pos), size(p_size) {}
};

template <> struct is_zero_constructible<Rect2> : std::true_type
{
};

// --- Folded from core/math/rect2i.h ---
#include "core/typedefs.h"
#include "core/math/vector2.h"

class String;
struct Rect2;

struct [[nodiscard]] Rect2i
{
	Point2i position;
	Size2i size;

	const Point2i& get_position() const { return position; }

	void set_position(const Point2i& p_position) { position = p_position; }

	const Size2i& get_size() const { return size; }

	void set_size(const Size2i& p_size) { size = p_size; }

	int get_area() const { return size.width * size.height; }

	_FORCE_INLINE_ Vector2i get_center() const { return position + (size / 2); }

	inline bool intersects(const Rect2i& p_rect) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2i size is negative, this is not supported. Use Rect2i.abs() to get a "
					  "Rect2i with a positive size.");
		}
#endif
		if (position.x >= (p_rect.position.x + p_rect.size.width)) {
			return false;
		}
		if ((position.x + size.width) <= p_rect.position.x) {
			return false;
		}
		if (position.y >= (p_rect.position.y + p_rect.size.height)) {
			return false;
		}
		if ((position.y + size.height) <= p_rect.position.y) {
			return false;
		}

		return true;
	}

	inline bool encloses(const Rect2i& p_rect) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2i size is negative, this is not supported. Use Rect2i.abs() to get a "
					  "Rect2i with a positive size.");
		}
#endif
		return (p_rect.position.x >= position.x) && (p_rect.position.y >= position.y) &&
			   ((p_rect.position.x + p_rect.size.x) <= (position.x + size.x)) &&
			   ((p_rect.position.y + p_rect.size.y) <= (position.y + size.y));
	}

	_FORCE_INLINE_ bool has_area() const { return size.x > 0 && size.y > 0; }

	// Returns the intersection between two Rect2is or an empty Rect2i if there is no intersection.
	inline Rect2i intersection(const Rect2i& p_rect) const
	{
		Rect2i new_rect = p_rect;

		if (!intersects(new_rect)) {
			return Rect2i();
		}

		new_rect.position = p_rect.position.max(position);

		Point2i p_rect_end = p_rect.position + p_rect.size;
		Point2i end = position + size;

		new_rect.size = p_rect_end.min(end) - new_rect.position;

		return new_rect;
	}

	inline Rect2i merge(const Rect2i& p_rect) const
	{ ///< return a merged rect
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
			ERR_PRINT("Rect2i size is negative, this is not supported. Use Rect2i.abs() to get a "
					  "Rect2i with a positive size.");
		}
#endif
		Rect2i new_rect;

		new_rect.position = p_rect.position.min(position);

		new_rect.size = (p_rect.position + p_rect.size).max(position + size);

		new_rect.size = new_rect.size - new_rect.position; // Make relative again.

		return new_rect;
	}

	bool has_point(const Point2i& p_point) const
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0)) {
			ERR_PRINT("Rect2i size is negative, this is not supported. Use Rect2i.abs() to get a "
					  "Rect2i with a positive size.");
		}
#endif
		if (p_point.x < position.x) {
			return false;
		}
		if (p_point.y < position.y) {
			return false;
		}

		if (p_point.x >= (position.x + size.x)) {
			return false;
		}
		if (p_point.y >= (position.y + size.y)) {
			return false;
		}

		return true;
	}

	constexpr bool operator==(const Rect2i& p_rect) const
	{
		return position == p_rect.position && size == p_rect.size;
	}

	constexpr bool operator!=(const Rect2i& p_rect) const
	{
		return position != p_rect.position || size != p_rect.size;
	}

	Rect2i grow(int p_amount) const
	{
		Rect2i g = *this;
		g.position.x -= p_amount;
		g.position.y -= p_amount;
		g.size.width += p_amount * 2;
		g.size.height += p_amount * 2;
		return g;
	}

	inline Rect2i grow_side(Side p_side, int p_amount) const
	{
		Rect2i g = *this;
		g = g.grow_individual((SIDE_LEFT == p_side) ? p_amount : 0,
			(SIDE_TOP == p_side) ? p_amount : 0, (SIDE_RIGHT == p_side) ? p_amount : 0,
			(SIDE_BOTTOM == p_side) ? p_amount : 0);
		return g;
	}

	inline Rect2i grow_side_bind(uint32_t p_side, int p_amount) const
	{
		return grow_side(Side(p_side), p_amount);
	}

	inline Rect2i grow_individual(int p_left, int p_top, int p_right, int p_bottom) const
	{
		Rect2i g = *this;
		g.position.x -= p_left;
		g.position.y -= p_top;
		g.size.width += p_left + p_right;
		g.size.height += p_top + p_bottom;

		return g;
	}

	_FORCE_INLINE_ Rect2i expand(const Vector2i& p_vector) const
	{
		Rect2i r = *this;
		r.expand_to(p_vector);
		return r;
	}

	inline void expand_to(const Point2i& p_vector)
	{
#ifdef MATH_CHECKS
		if (unlikely(size.x < 0 || size.y < 0)) {
			ERR_PRINT("Rect2i size is negative, this is not supported. Use Rect2i.abs() to get a "
					  "Rect2i with a positive size.");
		}
#endif
		Point2i begin = position;
		Point2i end = position + size;

		if (p_vector.x < begin.x) {
			begin.x = p_vector.x;
		}
		if (p_vector.y < begin.y) {
			begin.y = p_vector.y;
		}

		if (p_vector.x > end.x) {
			end.x = p_vector.x;
		}
		if (p_vector.y > end.y) {
			end.y = p_vector.y;
		}

		position = begin;
		size = end - begin;
	}

	_FORCE_INLINE_ Rect2i abs() const { return Rect2i(position + size.mini(0), size.abs()); }

	_FORCE_INLINE_ void set_end(const Vector2i& p_end) { size = p_end - position; }

	_FORCE_INLINE_ Vector2i get_end() const { return position + size; }

	explicit operator String() const;
	operator Rect2() const;

	uint32_t hash() const
	{
		uint32_t h = hash_murmur3_one_32(uint32_t(position.x));
		h = hash_murmur3_one_32(uint32_t(position.y), h);
		h = hash_murmur3_one_32(uint32_t(size.x), h);
		h = hash_murmur3_one_32(uint32_t(size.y), h);
		return hash_fmix32(h);
	}

	Rect2i() = default;

	constexpr Rect2i(int p_x, int p_y, int p_width, int p_height)
		: position(Point2i(p_x, p_y)), size(Size2i(p_width, p_height))
	{
	}

	constexpr Rect2i(const Point2i& p_pos, const Size2i& p_size) : position(p_pos), size(p_size) {}
};

template <> struct is_zero_constructible<Rect2i> : std::true_type
{
};
// --- End core/math/rect2.h ---
// --- Folded from core/math/transform_2d.h ---
#include "core/typedefs.h"

#include "core/math/vector2.h"
#include "core/string/ustring.h"

class String;

struct [[nodiscard]] Transform2D {
	// WARNING: The basis of Transform2D is stored differently from Basis.
	// In terms of columns array, the basis matrix looks like "on paper":
	// M = (columns[0][0] columns[1][0])
	//     (columns[0][1] columns[1][1])
	// This is such that the columns, which can be interpreted as basis vectors
	// of the coordinate system "painted" on the object, can be accessed as columns[i].
	// NOTE: This is the opposite of the indices in mathematical texts,
	// meaning: $M_{12}$ in a math book corresponds to columns[1][0] here.
	// This requires additional care when working with explicit indices.
	// See https://en.wikipedia.org/wiki/Row-_and_column-major_order for further reading.

	// WARNING: Be aware that unlike 3D code, 2D code uses a left-handed coordinate system:
	// Y-axis points down, and angle is measure from +X to +Y in a clockwise-fashion.

	static const Transform2D FLIP_X;
	static const Transform2D FLIP_Y;

	Vector2 columns[3] = {
		{ 1, 0 },
		{ 0, 1 },
		{ 0, 0 },
	};

	_FORCE_INLINE_ real_t tdotx(const Vector2 &p_v) const { return columns[0][0] * p_v.x + columns[1][0] * p_v.y; }
	_FORCE_INLINE_ real_t tdoty(const Vector2 &p_v) const { return columns[0][1] * p_v.x + columns[1][1] * p_v.y; }

	constexpr const Vector2 &operator[](int p_idx) const { return columns[p_idx]; }
	constexpr Vector2 &operator[](int p_idx) { return columns[p_idx]; }

	void invert();
	Transform2D inverse() const;

	void affine_invert();
	Transform2D affine_inverse() const;

	void set_rotation(real_t p_rot);
	real_t get_rotation() const;
	real_t get_skew() const;
	void set_skew(real_t p_angle);
	_FORCE_INLINE_ void set_rotation_and_scale(real_t p_rot, const Size2 &p_scale);
	_FORCE_INLINE_ void set_rotation_scale_and_skew(real_t p_rot, const Size2 &p_scale, real_t p_skew);
	void rotate(real_t p_angle);

	void scale(const Size2 &p_scale);
	void scale_basis(const Size2 &p_scale);
	void translate_local(real_t p_tx, real_t p_ty);
	void translate_local(const Vector2 &p_translation);

	real_t determinant() const;

	Size2 get_scale() const;
	void set_scale(const Size2 &p_scale);

	_FORCE_INLINE_ const Vector2 &get_origin() const { return columns[2]; }
	_FORCE_INLINE_ void set_origin(const Vector2 &p_origin) { columns[2] = p_origin; }

	Transform2D scaled(const Size2 &p_scale) const;
	Transform2D scaled_local(const Size2 &p_scale) const;
	Transform2D translated(const Vector2 &p_offset) const;
	Transform2D translated_local(const Vector2 &p_offset) const;
	Transform2D rotated(real_t p_angle) const;
	Transform2D rotated_local(real_t p_angle) const;

	Transform2D untranslated() const;

	void orthonormalize();
	Transform2D orthonormalized() const;
	bool is_conformal() const;
	bool is_equal_approx(const Transform2D &p_transform) const;
	bool is_same(const Transform2D &p_transform) const;
	bool is_finite() const;

	Transform2D looking_at(const Vector2 &p_target) const;

	constexpr bool operator==(const Transform2D &p_transform) const;
	constexpr bool operator!=(const Transform2D &p_transform) const;

	void operator*=(const Transform2D &p_transform);
	Transform2D operator*(const Transform2D &p_transform) const;
	constexpr void operator*=(real_t p_val);
	constexpr Transform2D operator*(real_t p_val) const;
	constexpr void operator/=(real_t p_val);
	constexpr Transform2D operator/(real_t p_val) const;

	Transform2D interpolate_with(const Transform2D &p_transform, real_t p_c) const;

	_FORCE_INLINE_ Vector2 basis_xform(const Vector2 &p_vec) const;
	_FORCE_INLINE_ Vector2 basis_xform_inv(const Vector2 &p_vec) const;
	_FORCE_INLINE_ Vector2 xform(const Vector2 &p_vec) const;
	_FORCE_INLINE_ Vector2 xform_inv(const Vector2 &p_vec) const;
	_FORCE_INLINE_ Rect2 xform(const Rect2 &p_rect) const;
	_FORCE_INLINE_ Rect2 xform_inv(const Rect2 &p_rect) const;
	_FORCE_INLINE_ Vector<Vector2> xform(const Vector<Vector2> &p_array) const;
	_FORCE_INLINE_ Vector<Vector2> xform_inv(const Vector<Vector2> &p_array) const;

	explicit operator String() const;

	constexpr Transform2D(real_t p_xx, real_t p_xy, real_t p_yx, real_t p_yy, real_t p_ox, real_t p_oy) :
			columns{
				{ p_xx, p_xy },
				{ p_yx, p_yy },
				{ p_ox, p_oy },
			} {}

	constexpr Transform2D(const Vector2 &p_x, const Vector2 &p_y, const Vector2 &p_origin) :
			columns{ p_x, p_y, p_origin } {}

	Transform2D(real_t p_rot, const Vector2 &p_pos);

	Transform2D(real_t p_rot, const Size2 &p_scale, real_t p_skew, const Vector2 &p_pos);

	Transform2D() = default;
};

inline constexpr Transform2D Transform2D::FLIP_X = { { -1, 0 }, { 0, 1 }, { 0, 0 } };
inline constexpr Transform2D Transform2D::FLIP_Y = { { 1, 0 }, { 0, -1 }, { 0, 0 } };

constexpr bool Transform2D::operator==(const Transform2D &p_transform) const {
	for (int i = 0; i < 3; i++) {
		if (columns[i] != p_transform.columns[i]) {
			return false;
		}
	}

	return true;
}

constexpr bool Transform2D::operator!=(const Transform2D &p_transform) const {
	for (int i = 0; i < 3; i++) {
		if (columns[i] != p_transform.columns[i]) {
			return true;
		}
	}

	return false;
}

constexpr void Transform2D::operator*=(real_t p_val) {
	columns[0] *= p_val;
	columns[1] *= p_val;
	columns[2] *= p_val;
}

constexpr Transform2D Transform2D::operator*(real_t p_val) const {
	Transform2D ret(*this);
	ret *= p_val;
	return ret;
}

constexpr void Transform2D::operator/=(real_t p_val) {
	columns[0] /= p_val;
	columns[1] /= p_val;
	columns[2] /= p_val;
}

constexpr Transform2D Transform2D::operator/(real_t p_val) const {
	Transform2D ret(*this);
	ret /= p_val;
	return ret;
}

Vector2 Transform2D::basis_xform(const Vector2 &p_vec) const {
	return Vector2(
			tdotx(p_vec),
			tdoty(p_vec));
}

Vector2 Transform2D::basis_xform_inv(const Vector2 &p_vec) const {
	return Vector2(
			columns[0].dot(p_vec),
			columns[1].dot(p_vec));
}

Vector2 Transform2D::xform(const Vector2 &p_vec) const {
	return Vector2(
				   tdotx(p_vec),
				   tdoty(p_vec)) +
			columns[2];
}

Vector2 Transform2D::xform_inv(const Vector2 &p_vec) const {
	Vector2 v = p_vec - columns[2];

	return Vector2(
			columns[0].dot(v),
			columns[1].dot(v));
}

Rect2 Transform2D::xform(const Rect2 &p_rect) const {
	Vector2 x = columns[0] * p_rect.size.x;
	Vector2 y = columns[1] * p_rect.size.y;
	Vector2 pos = xform(p_rect.position);

	Rect2 new_rect;
	new_rect.position = pos;
	new_rect.expand_to(pos + x);
	new_rect.expand_to(pos + y);
	new_rect.expand_to(pos + x + y);
	return new_rect;
}

void Transform2D::set_rotation_and_scale(real_t p_rot, const Size2 &p_scale) {
	columns[0][0] = Math::cos(p_rot) * p_scale.x;
	columns[1][1] = Math::cos(p_rot) * p_scale.y;
	columns[1][0] = -Math::sin(p_rot) * p_scale.y;
	columns[0][1] = Math::sin(p_rot) * p_scale.x;
}

void Transform2D::set_rotation_scale_and_skew(real_t p_rot, const Size2 &p_scale, real_t p_skew) {
	columns[0][0] = Math::cos(p_rot) * p_scale.x;
	columns[1][1] = Math::cos(p_rot + p_skew) * p_scale.y;
	columns[1][0] = -Math::sin(p_rot + p_skew) * p_scale.y;
	columns[0][1] = Math::sin(p_rot) * p_scale.x;
}

Rect2 Transform2D::xform_inv(const Rect2 &p_rect) const {
	Vector2 ends[4] = {
		xform_inv(p_rect.position),
		xform_inv(Vector2(p_rect.position.x, p_rect.position.y + p_rect.size.y)),
		xform_inv(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y)),
		xform_inv(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y))
	};

	Rect2 new_rect;
	new_rect.position = ends[0];
	new_rect.expand_to(ends[1]);
	new_rect.expand_to(ends[2]);
	new_rect.expand_to(ends[3]);

	return new_rect;
}

Vector<Vector2> Transform2D::xform(const Vector<Vector2> &p_array) const {
	Vector<Vector2> array;
	array.resize(p_array.size());

	const Vector2 *r = p_array.ptr();
	Vector2 *w = array.ptrw();

	for (int i = 0; i < p_array.size(); ++i) {
		w[i] = xform(r[i]);
	}
	return array;
}

Vector<Vector2> Transform2D::xform_inv(const Vector<Vector2> &p_array) const {
	Vector<Vector2> array;
	array.resize(p_array.size());

	const Vector2 *r = p_array.ptr();
	Vector2 *w = array.ptrw();

	for (int i = 0; i < p_array.size(); ++i) {
		w[i] = xform_inv(r[i]);
	}
	return array;
}
// --- End core/math/transform_2d.h ---

#include "core/math/vector2.h"

// --- Folded from core/math/projection.h ---

// --- Inlined/Folded from core/math/vector4.h ---
#include "core/typedefs.h"
#include "core/math/vector2.h"
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
#include "core/typedefs.h"
#include "core/math/vector2.h"

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
// --- End core/math/projection.h ---

// Strings & Paths
// --- Inlined/Folded from core/string/node_path.h ---
#include <climits>
#include "core/string/ustring.h"

// Represents a path to a node or property in a hierarchy of nodes
// Note that NodePath is (effectively) const: If you hold a NodePath,
// you can expect it to remain unchanged, even if you make copies of
// it. This is achieved through copy-on-write (CoW).
class [[nodiscard]] _WARN_UNUSED_ NodePath
{
	struct Data
	{
		SafeRefCount refcount;
		Vector<String> path;
		Vector<String> subpath;
		String concatenated_path;
		String concatenated_subpath;
		bool absolute;
		mutable bool hash_cache_valid;
		mutable uint32_t hash_cache;
	};

	mutable Data* data = nullptr;
	void unref();

	void _update_hash_cache() const;

	// Copies the underlying data.
	// Every non-const function must call this before starting to mutate data.
	void _copy_on_write();

public:
	bool is_absolute() const;
	int get_name_count() const;
	String get_name(int p_idx) const;
	int get_subname_count() const;
	String get_subname(int p_idx) const;
	int get_total_name_count() const;
	Vector<String> get_names() const;
	Vector<String> get_subnames() const;
	String get_concatenated_names() const;
	String get_concatenated_subnames() const;
	NodePath slice(int p_begin, int p_end = INT_MAX) const;

	NodePath rel_path_to(const NodePath& p_np) const;
	NodePath get_as_property_path() const;

	void prepend_period();

	_FORCE_INLINE_ uint32_t hash() const
	{
		if (!data) {
			return 0;
		}
		if (!data->hash_cache_valid) {
			_update_hash_cache();
		}
		return data->hash_cache;
	}

	explicit operator String() const;
	bool is_empty() const;

	bool operator==(const NodePath& p_path) const;
	bool operator!=(const NodePath& p_path) const;
	void operator=(const NodePath& p_path);

	void simplify();
	NodePath simplified() const;

	NodePath(const Vector<String>& p_path, bool p_absolute);
	NodePath(
		const Vector<String>& p_path, const Vector<String>& p_subpath, bool p_absolute);
	NodePath(const NodePath& p_path);
	NodePath(const String& p_path);

	NodePath() {}

	~NodePath();
};

// Zero-constructing NodePath initializes data to nullptr (and thus empty).
template <> struct is_zero_constructible<NodePath> : std::true_type
{
};
#include "core/string/ustring.h"

// Core Identifiers & Handles
#include "core/math/vector2.h"

// Native Container Templates
#include "core/string/ustring.h"
// --- Folded from core/templates/hash_map.h ---
#include <initializer_list>
#include "core/os/memory.h"
#include "core/string/ustring.h" // IWYU pragma: keep. `WARN_VERBOSE` macro.
#include "core/math/vector2.h"
#include "core/templates/list.h"

/**
 * A HashMap implementation that uses open addressing with Robin Hood hashing.
 * Robin Hood hashing swaps out entries that have a smaller probing distance
 * than the to-be-inserted entry, that evens out the average probing distance
 * and enables faster lookups. Backward shift deletion is employed to further
 * improve the performance and to avoid infinite loops in rare cases.
 *
 * Keys and values are stored in a double linked list by insertion order. This
 * has a slight performance overhead on lookup, which can be mostly compensated
 * using a paged allocator if required.
 *
 * The assignment operator copy the pairs from one map to the other.
 */

template <typename TKey, typename TValue> struct HashMapElement
{
	HashMapElement* next = nullptr;
	HashMapElement* prev = nullptr;
	KeyValue<TKey, TValue> data;

	HashMapElement() {}

	HashMapElement(const TKey& p_key, const TValue& p_value) : data(p_key, p_value) {}
};

template <typename TKey, typename TValue, typename Hasher = HashMapHasherDefault,
	typename Comparator = HashMapComparatorDefault<TKey>,
	typename Allocator = DefaultTypedAllocator<HashMapElement<TKey, TValue>>>
class _WARN_UNUSED_ HashMap : private Allocator
{
public:
	static constexpr uint32_t MIN_CAPACITY_INDEX = 2; // Use a prime.
	static constexpr float MAX_OCCUPANCY = 0.75;
	static constexpr uint32_t EMPTY_HASH = 0;
	using KV = KeyValue<TKey, TValue>; // Type alias for easier access to KeyValue.

private:
	HashMapElement<TKey, TValue>** _elements = nullptr;
	uint32_t* _hashes = nullptr;
	HashMapElement<TKey, TValue>* _head_element = nullptr;
	HashMapElement<TKey, TValue>* _tail_element = nullptr;

	uint32_t _capacity_idx = 0;
	uint32_t _size = 0;

	_FORCE_INLINE_ static uint32_t _hash(const TKey& p_key)
	{
		uint32_t hash = Hasher::hash(p_key);

		if (unlikely(hash == EMPTY_HASH)) {
			hash = EMPTY_HASH + 1;
		}

		return hash;
	}

	_FORCE_INLINE_ static constexpr void _increment_mod(uint32_t& r_idx, const uint32_t p_capacity)
	{
		r_idx++;
		// `if` is faster than both fastmod and mod.
		if (unlikely(r_idx == p_capacity)) {
			r_idx = 0;
		}
	}

	static _FORCE_INLINE_ uint32_t _get_probe_length(const uint32_t p_idx, const uint32_t p_hash,
		const uint32_t p_capacity, const uint64_t p_capacity_inv)
	{
		const uint32_t original_idx = fastmod(p_hash, p_capacity_inv, p_capacity);
		const uint32_t distance_idx = p_idx - original_idx + p_capacity;
		// At most p_capacity over 0, so we can use an if (faster than fastmod).
		return distance_idx >= p_capacity ? distance_idx - p_capacity : distance_idx;
	}

	bool _lookup_idx(const TKey& p_key, uint32_t& r_idx) const
	{
		return _elements != nullptr && _size > 0 &&
			   _lookup_idx_unchecked(p_key, _hash(p_key), r_idx);
	}

	/// Note: Assumes that _elements != nullptr
	bool _lookup_idx_unchecked(const TKey& p_key, uint32_t p_hash, uint32_t& r_idx) const
	{
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t idx = fastmod(p_hash, capacity_inv, capacity);
		uint32_t distance = 0;

		while (true) {
			if (_hashes[idx] == EMPTY_HASH) {
				return false;
			}

			if (distance > _get_probe_length(idx, _hashes[idx], capacity, capacity_inv)) {
				return false;
			}

			if (_hashes[idx] == p_hash && Comparator::compare(_elements[idx]->data.key, p_key)) {
				r_idx = idx;
				return true;
			}

			_increment_mod(idx, capacity);
			distance++;
		}
	}

	void _insert_element(uint32_t p_hash, HashMapElement<TKey, TValue>* p_value)
	{
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t hash = p_hash;
		HashMapElement<TKey, TValue>* value = p_value;
		uint32_t distance = 0;
		uint32_t idx = fastmod(hash, capacity_inv, capacity);

		while (true) {
			if (_hashes[idx] == EMPTY_HASH) {
				_elements[idx] = value;
				_hashes[idx] = hash;

				_size++;

				return;
			}

			// Not an empty slot, let's check the probing length of the existing one.
			uint32_t existing_probe_len =
				_get_probe_length(idx, _hashes[idx], capacity, capacity_inv);
			if (existing_probe_len < distance) {
				SWAP(hash, _hashes[idx]);
				SWAP(value, _elements[idx]);
				distance = existing_probe_len;
			}

			_increment_mod(idx, capacity);
			distance++;
		}
	}

	void _resize_and_rehash(uint32_t p_new_capacity_idx)
	{
		uint32_t old_capacity = hash_table_size_primes[_capacity_idx];

		// Capacity can't be 0.
		_capacity_idx = MAX((uint32_t)MIN_CAPACITY_INDEX, p_new_capacity_idx);

		uint32_t capacity = hash_table_size_primes[_capacity_idx];

		HashMapElement<TKey, TValue>** old_elements = _elements;
		uint32_t* old_hashes = _hashes;

		_size = 0;
		static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");

		_hashes =
			reinterpret_cast<uint32_t*>(Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
		_elements = reinterpret_cast<HashMapElement<TKey, TValue>**>(
			Memory::alloc_static(sizeof(HashMapElement<TKey, TValue>*) * capacity));

		if (old_capacity == 0) {
			// Nothing to do.
			return;
		}

		for (uint32_t i = 0; i < old_capacity; i++) {
			if (old_hashes[i] == EMPTY_HASH) {
				continue;
			}

			_insert_element(old_hashes[i], old_elements[i]);
		}

		Memory::free_static(old_elements);
		Memory::free_static(old_hashes);
	}

	_FORCE_INLINE_ HashMapElement<TKey, TValue>* _insert(
		const TKey& p_key, const TValue& p_value, uint32_t p_hash, bool p_front_insert = false)
	{
		uint32_t capacity = hash_table_size_primes[_capacity_idx];
		if (unlikely(_elements == nullptr)) {
			// Allocate on demand to save memory.

			static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");
			_hashes = reinterpret_cast<uint32_t*>(
				Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
			_elements = reinterpret_cast<HashMapElement<TKey, TValue>**>(
				Memory::alloc_static(sizeof(HashMapElement<TKey, TValue>*) * capacity));
		}

		if (_size + 1 > MAX_OCCUPANCY * capacity) {
			ERR_FAIL_COND_V_MSG(_capacity_idx + 1 == HASH_TABLE_SIZE_MAX, nullptr,
				"Hash table maximum capacity reached, aborting insertion.");
			_resize_and_rehash(_capacity_idx + 1);
		}

		HashMapElement<TKey, TValue>* elem =
			Allocator::new_allocation(HashMapElement<TKey, TValue>(p_key, p_value));

		if (_tail_element == nullptr) {
			_head_element = elem;
			_tail_element = elem;
		}
		else if (p_front_insert) {
			_head_element->prev = elem;
			elem->next = _head_element;
			_head_element = elem;
		}
		else {
			_tail_element->next = elem;
			elem->prev = _tail_element;
			_tail_element = elem;
		}

		_insert_element(p_hash, elem);
		return elem;
	}

	void _clear_data()
	{
		HashMapElement<TKey, TValue>* current = _tail_element;
		while (current != nullptr) {
			HashMapElement<TKey, TValue>* prev = current->prev;
			Allocator::delete_allocation(current);
			current = prev;
		}
	}

public:
	_FORCE_INLINE_ uint32_t get_capacity() const { return hash_table_size_primes[_capacity_idx]; }

	_FORCE_INLINE_ uint32_t size() const { return _size; }

	/* Standard Godot Container API */

	bool is_empty() const { return _size == 0; }

	void clear()
	{
		if (_elements == nullptr || _size == 0) {
			return;
		}

		_clear_data();
		memset(_hashes, EMPTY_HASH, get_capacity() * sizeof(uint32_t));

		_tail_element = nullptr;
		_head_element = nullptr;
		_size = 0;
	}

	void sort() { sort_custom<KeyValueSort<TKey, TValue>>(); }

	template <typename C> void sort_custom()
	{
		if (size() < 2) {
			return;
		}

		using E = HashMapElement<TKey, TValue>;
		SortList<E, KeyValue<TKey, TValue>, &E::data, &E::prev, &E::next, C> sorter;
		sorter.sort(_head_element, _tail_element);
	}

	TValue& get(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND_MSG(!exists, "HashMap key not found.");
		return _elements[idx]->data.value;
	}

	const TValue& get(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND_MSG(!exists, "HashMap key not found.");
		return _elements[idx]->data.value;
	}

	const TValue* getptr(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (exists) {
			return &_elements[idx]->data.value;
		}
		return nullptr;
	}

	TValue* getptr(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (exists) {
			return &_elements[idx]->data.value;
		}
		return nullptr;
	}

	_FORCE_INLINE_ bool has(const TKey& p_key) const
	{
		uint32_t _idx = 0;
		return _lookup_idx(p_key, _idx);
	}

	bool erase(const TKey& p_key)
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (!exists) {
			return false;
		}

		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t next_idx = fastmod((idx + 1), capacity_inv, capacity);
		while (_hashes[next_idx] != EMPTY_HASH &&
			   _get_probe_length(next_idx, _hashes[next_idx], capacity, capacity_inv) != 0) {
			SWAP(_hashes[next_idx], _hashes[idx]);
			SWAP(_elements[next_idx], _elements[idx]);
			idx = next_idx;
			_increment_mod(next_idx, capacity);
		}

		_hashes[idx] = EMPTY_HASH;

		if (_head_element == _elements[idx]) {
			_head_element = _elements[idx]->next;
		}

		if (_tail_element == _elements[idx]) {
			_tail_element = _elements[idx]->prev;
		}

		if (_elements[idx]->prev) {
			_elements[idx]->prev->next = _elements[idx]->next;
		}

		if (_elements[idx]->next) {
			_elements[idx]->next->prev = _elements[idx]->prev;
		}

		Allocator::delete_allocation(_elements[idx]);

		_size--;
		return true;
	}

	// Replace the key of an entry in-place, without invalidating iterators or changing the entries
	// position during iteration. p_old_key must exist in the map and p_new_key must not, unless it
	// is equal to p_old_key.
	bool replace_key(const TKey& p_old_key, const TKey& p_new_key)
	{
		ERR_FAIL_COND_V(_elements == nullptr || _size == 0, false);
		if (p_old_key == p_new_key) {
			return true;
		}
		const uint32_t new_hash = _hash(p_new_key);
		uint32_t idx = 0;
		ERR_FAIL_COND_V(_lookup_idx_unchecked(p_new_key, new_hash, idx), false);
		ERR_FAIL_COND_V(!_lookup_idx(p_old_key, idx), false);
		HashMapElement<TKey, TValue>* element = _elements[idx];

		// Delete the old entries in _hashes and _elements.
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t next_idx = fastmod((idx + 1), capacity_inv, capacity);
		while (_hashes[next_idx] != EMPTY_HASH &&
			   _get_probe_length(next_idx, _hashes[next_idx], capacity, capacity_inv) != 0) {
			SWAP(_hashes[next_idx], _hashes[idx]);
			SWAP(_elements[next_idx], _elements[idx]);
			idx = next_idx;
			_increment_mod(next_idx, capacity);
		}

		_hashes[idx] = EMPTY_HASH;

		// _insert_element will increment this again.
		_size--;

		// Update the HashMapElement with the new key and reinsert it.
		const_cast<TKey&>(element->data.key) = p_new_key;
		_insert_element(new_hash, element);

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old
	// capacity.
	void reserve(uint32_t p_new_capacity)
	{
		uint32_t new_idx = _capacity_idx;

		while (hash_table_size_primes[new_idx] < p_new_capacity) {
			ERR_FAIL_COND_MSG(new_idx + 1 == (uint32_t)HASH_TABLE_SIZE_MAX, nullptr);
			new_idx++;
		}

		if (new_idx == _capacity_idx) {
			if (p_new_capacity < _size) {
				WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This "
							 "is likely a mistake.");
			}
			return;
		}

		if (_elements == nullptr) {
			_capacity_idx = new_idx;
			return; // Unallocated yet.
		}
		_resize_and_rehash(new_idx);
	}

	/** Iterator API **/

	struct ConstIterator
	{
		_FORCE_INLINE_ const KeyValue<TKey, TValue>& operator*() const { return E->data; }

		_FORCE_INLINE_ const KeyValue<TKey, TValue>* operator->() const { return &E->data; }

		_FORCE_INLINE_ ConstIterator& operator++()
		{
			if (E) {
				E = E->next;
			}
			return *this;
		}

		_FORCE_INLINE_ ConstIterator& operator--()
		{
			if (E) {
				E = E->prev;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator& p_other) const
		{
			return E == p_other.E;
		}

		_FORCE_INLINE_ bool operator!=(const ConstIterator& p_other) const
		{
			return E != p_other.E;
		}

		_FORCE_INLINE_ explicit operator bool() const { return E != nullptr; }

		_FORCE_INLINE_ ConstIterator(const HashMapElement<TKey, TValue>* p_element)
		{
			E = p_element;
		}

		_FORCE_INLINE_ ConstIterator() {}

		_FORCE_INLINE_ ConstIterator(const ConstIterator& p_it) { E = p_it.E; }

		_FORCE_INLINE_ void operator=(const ConstIterator& p_it) { E = p_it.E; }

	private:
		const HashMapElement<TKey, TValue>* E = nullptr;
	};

	struct Iterator
	{
		_FORCE_INLINE_ KeyValue<TKey, TValue>& operator*() const { return E->data; }

		_FORCE_INLINE_ KeyValue<TKey, TValue>* operator->() const { return &E->data; }

		_FORCE_INLINE_ Iterator& operator++()
		{
			if (E) {
				E = E->next;
			}
			return *this;
		}

		_FORCE_INLINE_ Iterator& operator--()
		{
			if (E) {
				E = E->prev;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator& p_other) const { return E == p_other.E; }

		_FORCE_INLINE_ bool operator!=(const Iterator& p_other) const { return E != p_other.E; }

		_FORCE_INLINE_ explicit operator bool() const { return E != nullptr; }

		_FORCE_INLINE_ Iterator(HashMapElement<TKey, TValue>* p_element) { E = p_element; }

		_FORCE_INLINE_ Iterator() {}

		_FORCE_INLINE_ Iterator(const Iterator& p_it) { E = p_it.E; }

		_FORCE_INLINE_ void operator=(const Iterator& p_it) { E = p_it.E; }

		operator ConstIterator() const { return ConstIterator(E); }

	private:
		HashMapElement<TKey, TValue>* E = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() _LIFETIME_BOUND_ { return Iterator(_head_element); }

	_FORCE_INLINE_ Iterator end() _LIFETIME_BOUND_ { return Iterator(nullptr); }

	_FORCE_INLINE_ Iterator last() _LIFETIME_BOUND_ { return Iterator(_tail_element); }

	_FORCE_INLINE_ Iterator find(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		if (!exists) {
			return end();
		}
		return Iterator(_elements[idx]);
	}

	_FORCE_INLINE_ void remove(const Iterator& p_iter)
	{
		if (p_iter) {
			erase(p_iter->key);
		}
	}

	_FORCE_INLINE_ ConstIterator begin() const _LIFETIME_BOUND_
	{
		return ConstIterator(_head_element);
	}

	_FORCE_INLINE_ ConstIterator end() const _LIFETIME_BOUND_ { return ConstIterator(nullptr); }

	_FORCE_INLINE_ ConstIterator last() const _LIFETIME_BOUND_
	{
		return ConstIterator(_tail_element);
	}

	_FORCE_INLINE_ ConstIterator find(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		if (!exists) {
			return end();
		}
		return ConstIterator(_elements[idx]);
	}

	/* Indexing */

	const TValue& operator[](const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND(!exists);
		return _elements[idx]->data.value;
	}

	TValue& operator[](const TKey& p_key) _LIFETIME_BOUND_
	{
		const uint32_t hash = _hash(p_key);
		uint32_t idx = 0;
		bool exists = _elements && _size > 0 && _lookup_idx_unchecked(p_key, hash, idx);
		if (!exists) {
			return _insert(p_key, TValue(), hash)->data.value;
		}
		else {
			return _elements[idx]->data.value;
		}
	}

	/* Insert */

	Iterator insert(const TKey& p_key, const TValue& p_value, bool p_front_insert = false)
	{
		const uint32_t hash = _hash(p_key);
		uint32_t idx = 0;
		bool exists = _elements && _size > 0 && _lookup_idx_unchecked(p_key, hash, idx);
		if (!exists) {
			return Iterator(_insert(p_key, p_value, hash, p_front_insert));
		}
		else {
			_elements[idx]->data.value = p_value;
			return Iterator(_elements[idx]);
		}
	}

	/* Constructors */

	explicit HashMap(const HashMap& p_other)
	{
		reserve(hash_table_size_primes[p_other._capacity_idx]);

		if (p_other._size == 0) {
			return;
		}

		for (const KeyValue<TKey, TValue>& E : p_other) {
			insert(E.key, E.value);
		}
	}

	HashMap(HashMap&& p_other)
	{
		_elements = p_other._elements;
		_hashes = p_other._hashes;
		_head_element = p_other._head_element;
		_tail_element = p_other._tail_element;
		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		p_other._elements = nullptr;
		p_other._hashes = nullptr;
		p_other._head_element = nullptr;
		p_other._tail_element = nullptr;
		p_other._capacity_idx = MIN_CAPACITY_INDEX;
		p_other._size = 0;
	}

	void operator=(const HashMap& p_other)
	{
		if (this == &p_other) {
			return; // Ignore self assignment.
		}
		if (_size != 0) {
			clear();
		}

		reserve(hash_table_size_primes[p_other._capacity_idx]);

		if (p_other._elements == nullptr) {
			return; // Nothing to copy.
		}

		for (const KeyValue<TKey, TValue>& E : p_other) {
			insert(E.key, E.value);
		}
	}

	HashMap& operator=(HashMap&& p_other)
	{
		if (this == &p_other) {
			return *this;
		}

		if (_size != 0) {
			clear();
		}
		if (_elements != nullptr) {
			Memory::free_static(_elements);
			Memory::free_static(_hashes);
		}

		_elements = p_other._elements;
		_hashes = p_other._hashes;
		_head_element = p_other._head_element;
		_tail_element = p_other._tail_element;
		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		p_other._elements = nullptr;
		p_other._hashes = nullptr;
		p_other._head_element = nullptr;
		p_other._tail_element = nullptr;
		p_other._capacity_idx = MIN_CAPACITY_INDEX;
		p_other._size = 0;

		return *this;
	}

	HashMap(uint32_t p_initial_capacity)
	{
		// Capacity can't be 0.
		_capacity_idx = 0;
		reserve(p_initial_capacity);
	}

	HashMap() { _capacity_idx = MIN_CAPACITY_INDEX; }

	HashMap(std::initializer_list<KeyValue<TKey, TValue>> p_init)
	{
		reserve(p_init.size());
		for (const KeyValue<TKey, TValue>& E : p_init) {
			insert(E.key, E.value);
		}
	}

	uint32_t debug_get_hash(uint32_t p_idx)
	{
		if (_size == 0) {
			return 0;
		}
		ERR_FAIL_INDEX_V(p_idx, get_capacity(), 0);
		return _hashes[p_idx];
	}

	Iterator debug_get_element(uint32_t p_idx)
	{
		if (_size == 0) {
			return Iterator();
		}
		ERR_FAIL_INDEX_V(p_idx, get_capacity(), Iterator());
		return Iterator(_elements[p_idx]);
	}

	~HashMap()
	{
		_clear_data();

		if (_elements != nullptr) {
			Memory::free_static(_elements);
			Memory::free_static(_hashes);
		}
	}
};

// --- Folded from core/templates/lru.h ---
#include "core/templates/list.h"

template <typename TKey, typename TData, typename Hasher = HashMapHasherDefault, typename Comparator = HashMapComparatorDefault<TKey>, void (*BeforeEvict)(TKey &, TData &) = nullptr>
class _WARN_UNUSED_ LRUCache {
public:
	struct Pair {
		TKey key;
		TData data;

		Pair() {}
		Pair(const TKey &p_key, const TData &p_data) :
				key(p_key),
				data(p_data) {
		}
	};

	typedef typename List<Pair>::Element *Element;

private:
	List<Pair> _list;
	HashMap<TKey, Element, Hasher, Comparator> _map;
	size_t capacity;

public:
	const Pair *insert(const TKey &p_key, const TData &p_value) {
		Element *e = _map.getptr(p_key);
		Element n = _list.push_front(Pair(p_key, p_value));

		if (e) {
			VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
			if constexpr (BeforeEvict != nullptr) {
				BeforeEvict((*e)->get().key, (*e)->get().data);
			}
			VLTR_GCC_WARNING_POP
			_list.erase(*e);
			_map.erase(p_key);
		}
		_map[p_key] = _list.front();

		while (_map.size() > capacity) {
			Element d = _list.back();
			VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
			if constexpr (BeforeEvict != nullptr) {
				BeforeEvict(d->get().key, d->get().data);
			}
			VLTR_GCC_WARNING_POP
			_map.erase(d->get().key);
			_list.pop_back();
		}

		return &n->get();
	}

	void clear() {
		_map.clear();
		_list.clear();
	}

	bool has(const TKey &p_key) const {
		return _map.getptr(p_key);
	}

	bool erase(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		if (!e) {
			return false;
		}
		_list.move_to_front(*e);
		_map.erase(p_key);
		_list.pop_front();
		return true;
	}

	const TData &get(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		CRASH_COND(!e);
		_list.move_to_front(*e);
		return (*e)->get().data;
	}

	const TData *getptr(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		if (!e) {
			return nullptr;
		} else {
			_list.move_to_front(*e);
			return &(*e)->get().data;
		}
	}

	_FORCE_INLINE_ size_t get_capacity() const { return capacity; }
	_FORCE_INLINE_ size_t get_size() const { return _map.size(); }

	void set_capacity(size_t p_capacity) {
		if (capacity > 0) {
			capacity = p_capacity;
			while (_map.size() > capacity) {
				Element d = _list.back();
				VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
				if constexpr (BeforeEvict != nullptr) {
					BeforeEvict(d->get().key, d->get().data);
				}
				VLTR_GCC_WARNING_POP
				_map.erase(d->get().key);
				_list.pop_back();
			}
		}
	}

	LRUCache() {
		capacity = 64;
	}

	LRUCache(int p_capacity) {
		capacity = p_capacity;
	}
};

// --- Folded from core/templates/rb_map.h ---
#include "core/typedefs.h"
#include "core/os/memory.h"
#include "core/math/vector2.h"

#include <initializer_list>

// based on the very nice implementation of rb-trees by:
// https://web.archive.org/web/20120507164830/https://web.mit.edu/~emin/www/source_code/red_black_tree/index.html

template <typename K, typename V, typename C = Comparator<K>, typename A = DefaultAllocator>
class _WARN_UNUSED_ RBMap {
	enum Color {
		RED,
		BLACK
	};
	struct _Data;

public:
	class Element {
	private:
		friend class RBMap<K, V, C, A>;
		int color = RED;
		Element *right = nullptr;
		Element *left = nullptr;
		Element *parent = nullptr;
		Element *_next = nullptr;
		Element *_prev = nullptr;
		KeyValue<K, V> _data;

	public:
		KeyValue<K, V> &key_value() { return _data; }
		const KeyValue<K, V> &key_value() const { return _data; }

		const Element *next() const {
			return _next;
		}
		Element *next() {
			return _next;
		}
		const Element *prev() const {
			return _prev;
		}
		Element *prev() {
			return _prev;
		}
		const K &key() const {
			return _data.key;
		}
		V &value() {
			return _data.value;
		}
		const V &value() const {
			return _data.value;
		}
		V &get() {
			return _data.value;
		}
		const V &get() const {
			return _data.value;
		}
		Element(const KeyValue<K, V> &p_data) :
				_data(p_data) {}
	};

	typedef KeyValue<K, V> ValueType;

	struct Iterator {
		friend class RBMap<K, V, C, A>;

		_FORCE_INLINE_ KeyValue<K, V> &operator*() const {
			return E->key_value();
		}
		_FORCE_INLINE_ KeyValue<K, V> *operator->() const { return &E->key_value(); }
		_FORCE_INLINE_ Iterator &operator++() {
			E = E->next();
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			E = E->prev();
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &p_it) const { return E == p_it.E; }
		_FORCE_INLINE_ bool operator!=(const Iterator &p_it) const { return E != p_it.E; }
		explicit operator bool() const {
			return E != nullptr;
		}

		Iterator &operator=(const Iterator &p_it) {
			E = p_it.E;
			return *this;
		}
		Iterator(Element *p_element) { E = p_element; }
		Iterator() {}
		Iterator(const Iterator &p_it) { E = p_it.E; }

	private:
		Element *E = nullptr;
	};

	struct ConstIterator {
		_FORCE_INLINE_ const KeyValue<K, V> &operator*() const {
			return E->key_value();
		}
		_FORCE_INLINE_ const KeyValue<K, V> *operator->() const { return &E->key_value(); }
		_FORCE_INLINE_ ConstIterator &operator++() {
			E = E->next();
			return *this;
		}
		_FORCE_INLINE_ ConstIterator &operator--() {
			E = E->prev();
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator &p_it) const { return E == p_it.E; }
		_FORCE_INLINE_ bool operator!=(const ConstIterator &p_it) const { return E != p_it.E; }
		explicit operator bool() const {
			return E != nullptr;
		}

		ConstIterator &operator=(const ConstIterator &p_it) {
			E = p_it.E;
			return *this;
		}
		ConstIterator(const Element *p_element) { E = p_element; }
		ConstIterator() {}
		ConstIterator(const ConstIterator &p_it) { E = p_it.E; }

	private:
		const Element *E = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() {
		return Iterator(front());
	}
	_FORCE_INLINE_ Iterator end() {
		return Iterator(nullptr);
	}

#if 0
	//to use when replacing find()
	_FORCE_INLINE_ Iterator find(const K &p_key) {
		return Iterator(find(p_key));
	}
#endif
	_FORCE_INLINE_ void remove(const Iterator &p_iter) {
		return erase(p_iter.E);
	}

	_FORCE_INLINE_ ConstIterator begin() const {
		return ConstIterator(front());
	}
	_FORCE_INLINE_ ConstIterator end() const {
		return ConstIterator(nullptr);
	}

#if 0
	//to use when replacing find()
	_FORCE_INLINE_ ConstIterator find(const K &p_key) const {
		return ConstIterator(find(p_key));
	}
#endif
private:
	struct _Data {
		Element *_root = nullptr;
		Element *_nil = nullptr;
		int size_cache = 0;

		_FORCE_INLINE_ _Data() {
#ifdef GLOBALNIL_DISABLED
			_nil = memnew_allocator(Element, A);
			_nil->parent = _nil->left = _nil->right = _nil;
			_nil->color = BLACK;
#else
			_nil = (Element *)&_GlobalNilClass::_nil;
#endif
		}

		void _create_root() {
			_root = memnew_allocator(Element(KeyValue<K, V>(K(), V())), A);
			_root->parent = _root->left = _root->right = _nil;
			_root->color = BLACK;
		}

		void _free_root() {
			if (_root) {
				memdelete_allocator<Element, A>(_root);
				_root = nullptr;
			}
		}

		~_Data() {
			_free_root();

#ifdef GLOBALNIL_DISABLED
			memdelete_allocator<Element, A>(_nil);
#endif
		}
	};

	_Data _data;

	inline void _set_color(Element *p_node, int p_color) {
		ERR_FAIL_COND(p_node == _data._nil && p_color == RED);
		p_node->color = p_color;
	}

	inline void _rotate_left(Element *p_node) {
		Element *r = p_node->right;
		p_node->right = r->left;
		if (r->left != _data._nil) {
			r->left->parent = p_node;
		}
		r->parent = p_node->parent;
		if (p_node == p_node->parent->left) {
			p_node->parent->left = r;
		} else {
			p_node->parent->right = r;
		}

		r->left = p_node;
		p_node->parent = r;
	}

	inline void _rotate_right(Element *p_node) {
		Element *l = p_node->left;
		p_node->left = l->right;
		if (l->right != _data._nil) {
			l->right->parent = p_node;
		}
		l->parent = p_node->parent;
		if (p_node == p_node->parent->right) {
			p_node->parent->right = l;
		} else {
			p_node->parent->left = l;
		}

		l->right = p_node;
		p_node->parent = l;
	}

	inline Element *_successor(Element *p_node) const {
		Element *node = p_node;

		if (node->right != _data._nil) {
			node = node->right;
			while (node->left != _data._nil) { /* returns the minimum of the right subtree of node */
				node = node->left;
			}
			return node;
		} else {
			while (node == node->parent->right) {
				node = node->parent;
			}

			if (node->parent == _data._root) {
				return nullptr; // No successor, as p_node = last node
			}
			return node->parent;
		}
	}

	inline Element *_predecessor(Element *p_node) const {
		Element *node = p_node;

		if (node->left != _data._nil) {
			node = node->left;
			while (node->right != _data._nil) { /* returns the minimum of the left subtree of node */
				node = node->right;
			}
			return node;
		} else {
			while (node == node->parent->left) {
				node = node->parent;
			}

			if (node == _data._root) {
				return nullptr; // No predecessor, as p_node = first node
			}
			return node->parent;
		}
	}

	Element *_find(const K &p_key) const {
		Element *node = _data._root->left;
		C less;

		while (node != _data._nil) {
			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				return node; // found
			}
		}

		return nullptr;
	}

	Element *_find_closest(const K &p_key) const {
		Element *node = _data._root->left;
		Element *prev = nullptr;
		C less;

		while (node != _data._nil) {
			prev = node;

			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				return node; // found
			}
		}

		if (prev == nullptr) {
			return nullptr; // tree empty
		}

		if (less(p_key, prev->_data.key)) {
			prev = prev->_prev;
		}

		return prev;
	}

	Element *_lower_bound(const K &p_key) const {
		Element *node = _data._root->left;
		Element *result = nullptr;
		C less;

		while (node != _data._nil) {
			if (!less(node->_data.key, p_key)) {
				result = node;
				node = node->left;
			} else {
				node = node->right;
			}
		}

		return result;
	}

	void _insert_rb_fix(Element *p_new_node) {
		Element *node = p_new_node;
		Element *nparent = node->parent;
		Element *ngrand_parent = nullptr;

		while (nparent->color == RED) {
			ngrand_parent = nparent->parent;

			if (nparent == ngrand_parent->left) {
				if (ngrand_parent->right->color == RED) {
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent->right, BLACK);
					_set_color(ngrand_parent, RED);
					node = ngrand_parent;
					nparent = node->parent;
				} else {
					if (node == nparent->right) {
						_rotate_left(nparent);
						node = nparent;
						nparent = node->parent;
					}
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent, RED);
					_rotate_right(ngrand_parent);
				}
			} else {
				if (ngrand_parent->left->color == RED) {
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent->left, BLACK);
					_set_color(ngrand_parent, RED);
					node = ngrand_parent;
					nparent = node->parent;
				} else {
					if (node == nparent->left) {
						_rotate_right(nparent);
						node = nparent;
						nparent = node->parent;
					}
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent, RED);
					_rotate_left(ngrand_parent);
				}
			}
		}

		_set_color(_data._root->left, BLACK);
	}

	Element *_insert(const K &p_key, const V &p_value) {
		Element *new_parent = _data._root;
		Element *node = _data._root->left;
		C less;

		while (node != _data._nil) {
			new_parent = node;

			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				node->_data.value = p_value;
				return node; // Return existing node with new value
			}
		}

		typedef KeyValue<K, V> KV;
		Element *new_node = memnew_allocator(Element(KV(p_key, p_value)), A);
		new_node->parent = new_parent;
		new_node->right = _data._nil;
		new_node->left = _data._nil;

		//new_node->data=_data;

		if (new_parent == _data._root || less(p_key, new_parent->_data.key)) {
			new_parent->left = new_node;
		} else {
			new_parent->right = new_node;
		}

		new_node->_next = _successor(new_node);
		new_node->_prev = _predecessor(new_node);
		if (new_node->_next) {
			new_node->_next->_prev = new_node;
		}
		if (new_node->_prev) {
			new_node->_prev->_next = new_node;
		}

		_data.size_cache++;
		_insert_rb_fix(new_node);
		return new_node;
	}

	void _erase_fix_rb(Element *p_node) {
		Element *root = _data._root->left;
		Element *node = _data._nil;
		Element *sibling = p_node;
		Element *parent = sibling->parent;

		while (node != root) { // If red node found, will exit at a break
			if (sibling->color == RED) {
				_set_color(sibling, BLACK);
				_set_color(parent, RED);
				if (sibling == parent->right) {
					sibling = sibling->left;
					_rotate_left(parent);
				} else {
					sibling = sibling->right;
					_rotate_right(parent);
				}
			}
			if ((sibling->left->color == BLACK) && (sibling->right->color == BLACK)) {
				_set_color(sibling, RED);
				if (parent->color == RED) {
					_set_color(parent, BLACK);
					break;
				} else { // loop: haven't found any red nodes yet
					node = parent;
					parent = node->parent;
					sibling = (node == parent->left) ? parent->right : parent->left;
				}
			} else {
				if (sibling == parent->right) {
					if (sibling->right->color == BLACK) {
						_set_color(sibling->left, BLACK);
						_set_color(sibling, RED);
						_rotate_right(sibling);
						sibling = sibling->parent;
					}
					_set_color(sibling, parent->color);
					_set_color(parent, BLACK);
					_set_color(sibling->right, BLACK);
					_rotate_left(parent);
					break;
				} else {
					if (sibling->left->color == BLACK) {
						_set_color(sibling->right, BLACK);
						_set_color(sibling, RED);
						_rotate_left(sibling);
						sibling = sibling->parent;
					}

					_set_color(sibling, parent->color);
					_set_color(parent, BLACK);
					_set_color(sibling->left, BLACK);
					_rotate_right(parent);
					break;
				}
			}
		}

		ERR_FAIL_COND(_data._nil->color != BLACK);
	}

	void _erase(Element *p_node) {
		Element *rp = ((p_node->left == _data._nil) || (p_node->right == _data._nil)) ? p_node : p_node->_next;
		Element *node = (rp->left == _data._nil) ? rp->right : rp->left;

		Element *sibling = nullptr;
		if (rp == rp->parent->left) {
			rp->parent->left = node;
			sibling = rp->parent->right;
		} else {
			rp->parent->right = node;
			sibling = rp->parent->left;
		}

		if (node->color == RED) {
			node->parent = rp->parent;
			_set_color(node, BLACK);
		} else if (rp->color == BLACK && rp->parent != _data._root) {
			_erase_fix_rb(sibling);
		}

		if (rp != p_node) {
			ERR_FAIL_COND(rp == _data._nil);

			rp->left = p_node->left;
			rp->right = p_node->right;
			rp->parent = p_node->parent;
			rp->color = p_node->color;
			if (p_node->left != _data._nil) {
				p_node->left->parent = rp;
			}
			if (p_node->right != _data._nil) {
				p_node->right->parent = rp;
			}

			if (p_node == p_node->parent->left) {
				p_node->parent->left = rp;
			} else {
				p_node->parent->right = rp;
			}
		}

		if (p_node->_next) {
			p_node->_next->_prev = p_node->_prev;
		}
		if (p_node->_prev) {
			p_node->_prev->_next = p_node->_next;
		}

		memdelete_allocator<Element, A>(p_node);
		_data.size_cache--;
		ERR_FAIL_COND(_data._nil->color == RED);
	}

	void _calculate_depth(Element *p_element, int &p_max_d, int p_d) const {
		if (p_element == _data._nil) {
			return;
		}

		_calculate_depth(p_element->left, p_max_d, p_d + 1);
		_calculate_depth(p_element->right, p_max_d, p_d + 1);

		if (p_d > p_max_d) {
			p_max_d = p_d;
		}
	}

	void _cleanup_tree(Element *p_element) {
		if (p_element == _data._nil) {
			return;
		}

		_cleanup_tree(p_element->left);
		_cleanup_tree(p_element->right);
		memdelete_allocator<Element, A>(p_element);
	}

	void _copy_from(const RBMap &p_map) {
		clear();
		// not the fastest way, but safeset to write.
		for (Element *I = p_map.front(); I; I = I->next()) {
			insert(I->key(), I->value());
		}
	}

public:
	const Element *find(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}

		const Element *res = _find(p_key);
		return res;
	}

	Element *find(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}

		Element *res = _find(p_key);
		return res;
	}

	const Element *find_closest(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}

		const Element *res = _find_closest(p_key);
		return res;
	}

	Element *find_closest(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}

		Element *res = _find_closest(p_key);
		return res;
	}

	Element *lower_bound(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}
		return _lower_bound(p_key);
	}

	const Element *lower_bound(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}
		return _lower_bound(p_key);
	}

	bool has(const K &p_key) const {
		return find(p_key) != nullptr;
	}

	Element *insert(const K &p_key, const V &p_value) {
		if (!_data._root) {
			_data._create_root();
		}
		return _insert(p_key, p_value);
	}

	void erase(Element *p_element) {
		if (!_data._root || !p_element) {
			return;
		}

		_erase(p_element);
		if (_data.size_cache == 0 && _data._root) {
			_data._free_root();
		}
	}

	bool erase(const K &p_key) {
		if (!_data._root) {
			return false;
		}

		Element *e = find(p_key);
		if (!e) {
			return false;
		}

		_erase(e);
		if (_data.size_cache == 0 && _data._root) {
			_data._free_root();
		}
		return true;
	}

	const V &operator[](const K &p_key) const {
		CRASH_COND(!_data._root);
		const Element *e = find(p_key);
		CRASH_COND(!e);
		return e->_data.value;
	}

	V &operator[](const K &p_key) {
		if (!_data._root) {
			_data._create_root();
		}

		Element *e = find(p_key);
		if (!e) {
			e = insert(p_key, V());
		}

		return e->_data.value;
	}

	Element *front() const {
		if (!_data._root) {
			return nullptr;
		}

		Element *e = _data._root->left;
		if (e == _data._nil) {
			return nullptr;
		}

		while (e->left != _data._nil) {
			e = e->left;
		}

		return e;
	}

	Element *back() const {
		if (!_data._root) {
			return nullptr;
		}

		Element *e = _data._root->left;
		if (e == _data._nil) {
			return nullptr;
		}

		while (e->right != _data._nil) {
			e = e->right;
		}

		return e;
	}

	inline bool is_empty() const {
		return _data.size_cache == 0;
	}
	inline int size() const {
		return _data.size_cache;
	}

	int calculate_depth() const {
		// used for debug mostly
		if (!_data._root) {
			return 0;
		}

		int max_d = 0;
		_calculate_depth(_data._root->left, max_d, 0);
		return max_d;
	}

	void clear() {
		if (!_data._root) {
			return;
		}

		_cleanup_tree(_data._root->left);
		_data._root->left = _data._nil;
		_data.size_cache = 0;
		_data._free_root();
	}

	void operator=(const RBMap &p_map) {
		if (this == &p_map) {
			return;
		}

		_copy_from(p_map);
	}

	void operator=(RBMap &&p_map) {
		if (this == &p_map) {
			return;
		}

		SWAP(_data._root, p_map._data._root);
		SWAP(_data.size_cache, p_map._data.size_cache);
	}

	explicit RBMap(const RBMap &p_map) {
		_copy_from(p_map);
	}

	RBMap(RBMap &&p_map) {
		_data._root = p_map._data._root;
		_data.size_cache = p_map._data.size_cache;

		p_map._data._root = nullptr;
		p_map._data.size_cache = 0;
	}

	RBMap(std::initializer_list<KeyValue<K, V>> p_init) {
		for (const KeyValue<K, V> &E : p_init) {
			insert(E.key, E.value);
		}
	}

	_FORCE_INLINE_ RBMap() {}

	~RBMap() {
		clear();
	}
};
// --- End core/templates/rb_map.h ---
// --- End core/templates/hash_map.h ---
// --- Folded from core/templates/hash_set.h ---
#include "core/os/memory.h"
#include "core/string/ustring.h" // IWYU pragma: keep. `WARN_VERBOSE` macro.
#include "core/math/vector2.h"

/**
 * Implementation of Set using a bidi indexed hash map.
 * Use RBSet instead of this only if the following conditions are met:
 *
 * - You need to keep an iterator or const pointer to Key and you intend to add/remove elements in the meantime.
 * - Iteration order does matter (via operator<)
 *
 */

template <typename TKey,
		typename Hasher = HashMapHasherDefault,
		typename Comparator = HashMapComparatorDefault<TKey>>
class _WARN_UNUSED_ HashSet {
public:
	static constexpr uint32_t MIN_CAPACITY_INDEX = 2; // Use a prime.
	static constexpr float MAX_OCCUPANCY = 0.75;
	static constexpr uint32_t EMPTY_HASH = 0;

private:
	TKey *_keys = nullptr;
	uint32_t *_hash_idx_to_key_idx = nullptr;
	uint32_t *_key_idx_to_hash_idx = nullptr;
	uint32_t *_hashes = nullptr;

	uint32_t _capacity_idx = 0;
	uint32_t _size = 0;

	_FORCE_INLINE_ uint32_t _hash(const TKey &p_key) const {
		uint32_t hash = Hasher::hash(p_key);

		if (unlikely(hash == EMPTY_HASH)) {
			hash = EMPTY_HASH + 1;
		}

		return hash;
	}

	_FORCE_INLINE_ static constexpr void _increment_mod(uint32_t &r_idx, const uint32_t p_capacity) {
		r_idx++;
		// `if` is faster than both fastmod and mod.
		if (unlikely(r_idx == p_capacity)) {
			r_idx = 0;
		}
	}

	static _FORCE_INLINE_ uint32_t _get_probe_length(const uint32_t p_hash_idx, const uint32_t p_hash, const uint32_t p_capacity, const uint64_t p_capacity_inv) {
		const uint32_t original_idx = fastmod(p_hash, p_capacity_inv, p_capacity);
		const uint32_t distance_idx = p_hash_idx - original_idx + p_capacity;
		// At most p_capacity over 0, so we can use an if (faster than fastmod).
		return distance_idx >= p_capacity ? distance_idx - p_capacity : distance_idx;
	}

	bool _lookup_key_idx(const TKey &p_key, uint32_t &r_key_idx) const {
		if (_keys == nullptr || _size == 0) {
			return false; // Failed lookups, no elements
		}

		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t hash = _hash(p_key);
		uint32_t hash_idx = fastmod(hash, capacity_inv, capacity);
		uint32_t distance = 0;

		while (true) {
			if (_hashes[hash_idx] == EMPTY_HASH) {
				return false;
			}

			if (_hashes[hash_idx] == hash && Comparator::compare(_keys[_hash_idx_to_key_idx[hash_idx]], p_key)) {
				r_key_idx = _hash_idx_to_key_idx[hash_idx];
				return true;
			}

			if (distance > _get_probe_length(hash_idx, _hashes[hash_idx], capacity, capacity_inv)) {
				return false;
			}

			_increment_mod(hash_idx, capacity);
			distance++;
		}
	}

	uint32_t _insert_with_hash(uint32_t p_hash, uint32_t p_key_idx) {
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t hash = p_hash;
		uint32_t key_idx = p_key_idx;
		uint32_t distance = 0;
		uint32_t hash_idx = fastmod(hash, capacity_inv, capacity);

		while (true) {
			if (_hashes[hash_idx] == EMPTY_HASH) {
				_hashes[hash_idx] = hash;
				_key_idx_to_hash_idx[key_idx] = hash_idx;
				_hash_idx_to_key_idx[hash_idx] = key_idx;
				return hash_idx;
			}

			// Not an empty slot, let's check the probing length of the existing one.
			uint32_t existing_probe_len = _get_probe_length(hash_idx, _hashes[hash_idx], capacity, capacity_inv);
			if (existing_probe_len < distance) {
				_key_idx_to_hash_idx[key_idx] = hash_idx;
				SWAP(hash, _hashes[hash_idx]);
				SWAP(key_idx, _hash_idx_to_key_idx[hash_idx]);
				distance = existing_probe_len;
			}

			_increment_mod(hash_idx, capacity);
			distance++;
		}
	}

	void _resize_and_rehash(uint32_t p_new_capacity_idx) {
		// Capacity can't be 0.
		_capacity_idx = MAX((uint32_t)MIN_CAPACITY_INDEX, p_new_capacity_idx);

		uint32_t capacity = hash_table_size_primes[_capacity_idx];

		uint32_t *old_hashes = _hashes;
		uint32_t *old_key_to_hash = _key_idx_to_hash_idx;

		static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");
		_hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
		_keys = reinterpret_cast<TKey *>(Memory::realloc_static(_keys, sizeof(TKey) * capacity));
		_key_idx_to_hash_idx = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));
		_hash_idx_to_key_idx = reinterpret_cast<uint32_t *>(Memory::realloc_static(_hash_idx_to_key_idx, sizeof(uint32_t) * capacity));

		for (uint32_t i = 0; i < _size; i++) {
			uint32_t h = old_hashes[old_key_to_hash[i]];
			_insert_with_hash(h, i);
		}

		Memory::free_static(old_hashes);
		Memory::free_static(old_key_to_hash);
	}

	// Returns key index.
	_FORCE_INLINE_ int32_t _insert(const TKey &p_key) {
		uint32_t capacity = hash_table_size_primes[_capacity_idx];
		if (unlikely(_keys == nullptr)) {
			// Allocate on demand to save memory.

			static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");
			_hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
			_keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * capacity));
			_key_idx_to_hash_idx = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));
			_hash_idx_to_key_idx = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));
		}

		uint32_t key_idx = 0;
		bool exists = _lookup_key_idx(p_key, key_idx);

		if (exists) {
			return key_idx;
		} else {
			if (_size + 1 > MAX_OCCUPANCY * capacity) {
				ERR_FAIL_COND_V_MSG(_capacity_idx + 1 == HASH_TABLE_SIZE_MAX, -1, "Hash table maximum capacity reached, aborting insertion.");
				_resize_and_rehash(_capacity_idx + 1);
			}

			uint32_t hash = _hash(p_key);
			memnew_placement(&_keys[_size], TKey(p_key));
			_insert_with_hash(hash, _size);
			_size++;
			return _size - 1;
		}
	}

	void _init_from(const HashSet &p_other) {
		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		if (p_other._size == 0) {
			return;
		}

		uint32_t capacity = hash_table_size_primes[_capacity_idx];

		_hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));
		_keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * capacity));
		_key_idx_to_hash_idx = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));
		_hash_idx_to_key_idx = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * capacity));

		for (uint32_t i = 0; i < _size; i++) {
			memnew_placement(&_keys[i], TKey(p_other._keys[i]));
			_key_idx_to_hash_idx[i] = p_other._key_idx_to_hash_idx[i];
		}

		for (uint32_t i = 0; i < capacity; i++) {
			_hashes[i] = p_other._hashes[i];
			_hash_idx_to_key_idx[i] = p_other._hash_idx_to_key_idx[i];
		}
	}

public:
	_FORCE_INLINE_ uint32_t get_capacity() const { return hash_table_size_primes[_capacity_idx]; }
	_FORCE_INLINE_ uint32_t size() const { return _size; }

	/* Standard Godot Container API */

	bool is_empty() const {
		return _size == 0;
	}

	void clear() {
		if (_keys == nullptr || _size == 0) {
			return;
		}

		uint32_t capacity = hash_table_size_primes[_capacity_idx];
		memset(_hashes, EMPTY_HASH, sizeof(EMPTY_HASH) * capacity);

		if constexpr (!std::is_trivially_destructible_v<TKey>) {
			for (uint32_t i = 0; i < _size; i++) {
				_keys[i].~TKey();
			}
		}

		_size = 0;
	}

	_FORCE_INLINE_ bool has(const TKey &p_key) const {
		uint32_t _idx = 0;
		return _lookup_key_idx(p_key, _idx);
	}

	bool erase(const TKey &p_key) {
		uint32_t key_idx = 0;
		bool exists = _lookup_key_idx(p_key, key_idx);

		if (!exists) {
			return false;
		}

		uint32_t hash_idx = _key_idx_to_hash_idx[key_idx];

		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t next_hash_idx = fastmod(hash_idx + 1, capacity_inv, capacity);
		while (_hashes[next_hash_idx] != EMPTY_HASH && _get_probe_length(next_hash_idx, _hashes[next_hash_idx], capacity, capacity_inv) != 0) {
			uint32_t cur_key_idx = _hash_idx_to_key_idx[hash_idx];
			uint32_t next_key_idx = _hash_idx_to_key_idx[next_hash_idx];
			SWAP(_key_idx_to_hash_idx[cur_key_idx], _key_idx_to_hash_idx[next_key_idx]);
			SWAP(_hashes[next_hash_idx], _hashes[hash_idx]);
			SWAP(_hash_idx_to_key_idx[next_hash_idx], _hash_idx_to_key_idx[hash_idx]);

			hash_idx = next_hash_idx;
			_increment_mod(next_hash_idx, capacity);
		}

		_hashes[hash_idx] = EMPTY_HASH;
		_keys[key_idx].~TKey();
		_size--;
		if (key_idx < _size) {
			// Not the last key, move the last one here to keep keys contiguous.
			memnew_placement(&_keys[key_idx], TKey(_keys[_size]));
			_keys[_size].~TKey();
			_key_idx_to_hash_idx[key_idx] = _key_idx_to_hash_idx[_size];
			_hash_idx_to_key_idx[_key_idx_to_hash_idx[_size]] = key_idx;
		}

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old capacity.
	void reserve(uint32_t p_new_capacity) {
		uint32_t new_capacity_idx = _capacity_idx;

		while (hash_table_size_primes[new_capacity_idx] < p_new_capacity) {
			ERR_FAIL_COND_MSG(new_capacity_idx + 1 == (uint32_t)HASH_TABLE_SIZE_MAX, nullptr);
			new_capacity_idx++;
		}

		if (new_capacity_idx == _capacity_idx) {
			if (p_new_capacity < _size) {
				WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This is likely a mistake.");
			}
			return;
		}

		if (_keys == nullptr) {
			_capacity_idx = new_capacity_idx;
			return; // Unallocated yet.
		}
		_resize_and_rehash(new_capacity_idx);
	}

	/** Iterator API **/

	struct Iterator {
		_FORCE_INLINE_ const TKey &operator*() const {
			return _keys[_key_idx];
		}
		_FORCE_INLINE_ const TKey *operator->() const {
			return &_keys[_key_idx];
		}
		_FORCE_INLINE_ Iterator &operator++() {
			_key_idx++;
			if (_key_idx >= (int32_t)_num_keys) {
				_key_idx = -1;
				_keys = nullptr;
				_num_keys = 0;
			}
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			_key_idx--;
			if (_key_idx < 0) {
				_key_idx = -1;
				_keys = nullptr;
				_num_keys = 0;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &p_other) const { return _keys == p_other._keys && _key_idx == p_other._key_idx; }
		_FORCE_INLINE_ bool operator!=(const Iterator &p_other) const { return _keys != p_other._keys || _key_idx != p_other._key_idx; }

		_FORCE_INLINE_ explicit operator bool() const {
			return _keys != nullptr;
		}

		_FORCE_INLINE_ Iterator(const TKey *p_keys, uint32_t p_num_keys, int32_t p_key_idx = -1) {
			_keys = p_keys;
			_num_keys = p_num_keys;
			_key_idx = p_key_idx;
		}
		_FORCE_INLINE_ Iterator() {}
		_FORCE_INLINE_ Iterator(const Iterator &p_it) {
			_keys = p_it._keys;
			_num_keys = p_it._num_keys;
			_key_idx = p_it._key_idx;
		}
		_FORCE_INLINE_ void operator=(const Iterator &p_it) {
			_keys = p_it._keys;
			_num_keys = p_it._num_keys;
			_key_idx = p_it._key_idx;
		}

	private:
		const TKey *_keys = nullptr;
		uint32_t _num_keys = 0;
		int32_t _key_idx = -1;
	};

	_FORCE_INLINE_ Iterator begin() const _LIFETIME_BOUND_ {
		return _size ? Iterator(_keys, _size, 0) : Iterator();
	}
	_FORCE_INLINE_ Iterator end() const _LIFETIME_BOUND_ {
		return Iterator();
	}
	_FORCE_INLINE_ Iterator last() const _LIFETIME_BOUND_ {
		if (_size == 0) {
			return Iterator();
		}
		return Iterator(_keys, _size, _size - 1);
	}

	_FORCE_INLINE_ Iterator find(const TKey &p_key) const _LIFETIME_BOUND_ {
		uint32_t key_idx = 0;
		bool exists = _lookup_key_idx(p_key, key_idx);
		if (!exists) {
			return end();
		}
		return Iterator(_keys, _size, key_idx);
	}

	_FORCE_INLINE_ void remove(const Iterator &p_iter) {
		if (p_iter) {
			erase(*p_iter);
		}
	}

	/* Insert */

	Iterator insert(const TKey &p_key) _LIFETIME_BOUND_ {
		uint32_t key_idx = _insert(p_key);
		return Iterator(_keys, _size, key_idx);
	}

	/* Constructors */

	explicit HashSet(const HashSet &p_other) {
		_init_from(p_other);
	}

	HashSet(HashSet &&p_other) {
		_keys = p_other._keys;
		_hash_idx_to_key_idx = p_other._hash_idx_to_key_idx;
		_key_idx_to_hash_idx = p_other._key_idx_to_hash_idx;
		_hashes = p_other._hashes;

		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		p_other._keys = nullptr;
		p_other._hash_idx_to_key_idx = nullptr;
		p_other._hashes = nullptr;
		p_other._key_idx_to_hash_idx = nullptr;

		p_other._capacity_idx = 0;
		p_other._size = 0;
	}

	void operator=(const HashSet &p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		clear();

		if (_keys != nullptr) {
			Memory::free_static(_keys);
			Memory::free_static(_key_idx_to_hash_idx);
			Memory::free_static(_hash_idx_to_key_idx);
			Memory::free_static(_hashes);
			_keys = nullptr;
			_hashes = nullptr;
			_hash_idx_to_key_idx = nullptr;
			_key_idx_to_hash_idx = nullptr;
		}

		_init_from(p_other);
	}

	void operator=(HashSet &&p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		SWAP(_keys, p_other._keys);
		SWAP(_hash_idx_to_key_idx, p_other._hash_idx_to_key_idx);
		SWAP(_key_idx_to_hash_idx, p_other._key_idx_to_hash_idx);
		SWAP(_hashes, p_other._hashes);

		SWAP(_capacity_idx, p_other._capacity_idx);
		SWAP(_size, p_other._size);
	}

	bool operator==(const HashSet &p_other) const {
		if (_size != p_other._size) {
			return false;
		}
		for (uint32_t i = 0; i < _size; i++) {
			if (!p_other.has(_keys[i])) {
				return false;
			}
		}
		return true;
	}
	bool operator!=(const HashSet &p_other) const {
		return !(*this == p_other);
	}

	HashSet(uint32_t p_initial_capacity) {
		// Capacity can't be 0.
		_capacity_idx = 0;
		reserve(p_initial_capacity);
	}
	HashSet() {
		_capacity_idx = MIN_CAPACITY_INDEX;
	}
	HashSet(std::initializer_list<TKey> p_init) {
		reserve(p_init.size());
		for (const TKey &E : p_init) {
			insert(E);
		}
	}

	void reset() {
		clear();

		if (_keys != nullptr) {
			Memory::free_static(_keys);
			Memory::free_static(_key_idx_to_hash_idx);
			Memory::free_static(_hash_idx_to_key_idx);
			Memory::free_static(_hashes);
			_keys = nullptr;
			_hashes = nullptr;
			_hash_idx_to_key_idx = nullptr;
			_key_idx_to_hash_idx = nullptr;
		}
		_capacity_idx = MIN_CAPACITY_INDEX;
	}

	~HashSet() {
		clear();

		if (_keys != nullptr) {
			Memory::free_static(_keys);
			Memory::free_static(_key_idx_to_hash_idx);
			Memory::free_static(_hash_idx_to_key_idx);
			Memory::free_static(_hashes);
		}
	}
};
// --- End core/templates/hash_set.h ---
#include "core/templates/list.h"
#include "core/string/ustring.h"
// --- Folded from core/templates/paged_allocator.h ---
#include <type_traits>
#include <typeinfo> // IWYU pragma: keep // Used in macro.
// --- Folded from core/core_globals.h ---
// Home for state needed from global functions
// that cannot be stored in Engine or OS due to e.g. circular includes

class CoreGlobals
{
public:
	static inline bool leak_reporting_enabled = true;
	static inline bool print_line_enabled = true;
	static inline bool print_error_enabled = true;
	static inline bool print_ready = false;
};
// --- End core/core_globals.h ---
// --- Folded from core/math/math_funcs_binary.h ---
#include "core/typedefs.h"

namespace Math {

/* Functions to handle powers of 2 and shifting. */

// Returns `true` if a positive integer is a power of 2, `false` otherwise.
template <typename T>
constexpr bool is_power_of_2(const T p_x) {
	return p_x && ((p_x & (p_x - 1)) == 0);
}

// Function to find the next power of 2 to an integer.
constexpr uint64_t next_power_of_2(uint64_t p_number) {
	if (p_number == 0) {
		return 0;
	}

	--p_number;
	p_number |= p_number >> 1;
	p_number |= p_number >> 2;
	p_number |= p_number >> 4;
	p_number |= p_number >> 8;
	p_number |= p_number >> 16;
	p_number |= p_number >> 32;

	return ++p_number;
}

constexpr uint32_t next_power_of_2(uint32_t p_number) {
	if (p_number == 0) {
		return 0;
	}

	--p_number;
	p_number |= p_number >> 1;
	p_number |= p_number >> 2;
	p_number |= p_number >> 4;
	p_number |= p_number >> 8;
	p_number |= p_number >> 16;

	return ++p_number;
}

// Function to find the previous power of 2 to an integer.
constexpr uint64_t previous_power_of_2(uint64_t p_number) {
	p_number |= p_number >> 1;
	p_number |= p_number >> 2;
	p_number |= p_number >> 4;
	p_number |= p_number >> 8;
	p_number |= p_number >> 16;
	p_number |= p_number >> 32;
	return p_number - (p_number >> 1);
}

constexpr uint32_t previous_power_of_2(uint32_t p_number) {
	p_number |= p_number >> 1;
	p_number |= p_number >> 2;
	p_number |= p_number >> 4;
	p_number |= p_number >> 8;
	p_number |= p_number >> 16;
	return p_number - (p_number >> 1);
}

// Function to find the closest power of 2 to an integer.
constexpr uint64_t closest_power_of_2(uint64_t p_number) {
	uint64_t nx = next_power_of_2(p_number);
	uint64_t px = previous_power_of_2(p_number);
	return (nx - p_number) > (p_number - px) ? px : nx;
}

constexpr uint32_t closest_power_of_2(uint32_t p_number) {
	uint32_t nx = next_power_of_2(p_number);
	uint32_t px = previous_power_of_2(p_number);
	return (nx - p_number) > (p_number - px) ? px : nx;
}

// Get a shift value from a power of 2.
constexpr int32_t get_shift_from_power_of_2(uint64_t p_bits) {
	for (uint64_t i = 0; i < (uint64_t)64; i++) {
		if (p_bits == (uint64_t)((uint64_t)1 << i)) {
			return i;
		}
	}

	return -1;
}

constexpr int32_t get_shift_from_power_of_2(uint32_t p_bits) {
	for (uint32_t i = 0; i < (uint32_t)32; i++) {
		if (p_bits == (uint32_t)((uint32_t)1 << i)) {
			return i;
		}
	}

	return -1;
}

template <typename T>
constexpr T nearest_power_of_2_templated(T p_number) {
	--p_number;

	// The number of operations on x is the base two logarithm
	// of the number of bits in the type. Add three to account
	// for sizeof(T) being in bytes.
	constexpr size_t shift_steps = get_shift_from_power_of_2((uint64_t)sizeof(T)) + 3;

	// If the compiler is smart, it unrolls this loop.
	// If it's dumb, this is a bit slow.
	for (size_t i = 0; i < shift_steps; i++) {
		p_number |= p_number >> (1 << i);
	}

	return ++p_number;
}

// Function to find the nearest (bigger) power of 2 to an integer.
constexpr uint64_t nearest_shift(uint64_t p_number) {
	uint64_t i = 63;
	do {
		i--;
		if (p_number & ((uint64_t)1 << i)) {
			return i + (uint64_t)1;
		}
	} while (i != 0);

	return 0;
}

constexpr uint32_t nearest_shift(uint32_t p_number) {
	uint32_t i = 31;
	do {
		i--;
		if (p_number & ((uint32_t)1 << i)) {
			return i + (uint32_t)1;
		}
	} while (i != 0);

	return 0;
}

// constexpr function to find the floored log2 of a number
template <typename T>
constexpr T floor_log2(T p_x) {
	return p_x < 2 ? p_x : 1 + floor_log2(p_x >> 1);
}

// Get the number of bits needed to represent the number.
// IE, if you pass in 8, you will get 4.
// If you want to know how many bits are needed to store 8 values however, pass in (8 - 1).
template <typename T>
constexpr T get_num_bits(T p_x) {
	return floor_log2(p_x);
}

} //namespace Math
// --- End core/math/math_funcs_binary.h ---
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"

template <typename T, bool thread_safe = false, uint32_t DEFAULT_PAGE_SIZE = 4096>
class PagedAllocator
{
	T** page_pool = nullptr;
	T*** available_pool = nullptr;
	uint32_t pages_allocated = 0;
	uint32_t allocs_available = 0;

	uint32_t page_shift = 0;
	uint32_t page_mask = 0;
	uint32_t page_size = 0;
	SpinLock spin_lock;

public:
	template <typename... Args> T* alloc(Args&&... p_args)
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		if (unlikely(allocs_available == 0)) {
			uint32_t pages_used = pages_allocated;

			pages_allocated++;
			page_pool = (T**)memrealloc(page_pool, sizeof(T*) * pages_allocated);
			available_pool = (T***)memrealloc(available_pool, sizeof(T**) * pages_allocated);

			page_pool[pages_used] = (T*)memalloc(sizeof(T) * page_size);
			available_pool[pages_used] = (T**)memalloc(sizeof(T*) * page_size);

			for (uint32_t i = 0; i < page_size; i++) {
				available_pool[0][i] = &page_pool[pages_used][i];
			}
			allocs_available += page_size;
		}

		allocs_available--;
		T* alloc = available_pool[allocs_available >> page_shift][allocs_available & page_mask];
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
		memnew_placement(alloc, T(p_args...));
		return alloc;
	}

	void free(T* p_mem)
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		p_mem->~T();
		available_pool[allocs_available >> page_shift][allocs_available & page_mask] = p_mem;
		allocs_available++;
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
	}

	template <typename... Args> T* new_allocation(Args&&... p_args) { return alloc(p_args...); }

	void delete_allocation(T* p_mem) { free(p_mem); }

private:
	void _reset(bool p_allow_unfreed)
	{
		if (!p_allow_unfreed || !std::is_trivially_destructible_v<T>) {
			ERR_FAIL_COND(allocs_available < pages_allocated * page_size);
		}
		if (pages_allocated) {
			for (uint32_t i = 0; i < pages_allocated; i++) {
				memfree(page_pool[i]);
				memfree(available_pool[i]);
			}
			memfree(page_pool);
			memfree(available_pool);
			page_pool = nullptr;
			available_pool = nullptr;
			pages_allocated = 0;
			allocs_available = 0;
		}
	}

public:
	void reset(bool p_allow_unfreed = false)
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		_reset(p_allow_unfreed);
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
	}

	bool is_configured() const
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		bool result = page_size > 0;
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
		return result;
	}

	void configure(uint32_t p_page_size)
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		ERR_FAIL_COND(page_pool != nullptr); // Safety check.
		ERR_FAIL_COND(p_page_size == 0);
		page_size = Math::nearest_power_of_2_templated(p_page_size);
		page_mask = page_size - 1;
		page_shift = Math::get_shift_from_power_of_2(page_size);
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
	}

	// Power of 2 recommended because of alignment with OS page sizes.
	// Even if element is bigger, it's still a multiple and gets rounded to amount of pages.
	PagedAllocator(uint32_t p_page_size = DEFAULT_PAGE_SIZE) { configure(p_page_size); }

	~PagedAllocator()
	{
		if constexpr (thread_safe) {
			spin_lock.lock();
		}
		bool leaked = allocs_available < pages_allocated * page_size;
		if (leaked) {
			if (CoreGlobals::leak_reporting_enabled) {
				ERR_PRINT(String("Pages in use exist at exit in PagedAllocator: ") +
						  String(typeid(T).name()));
			}
		}
		else {
			_reset(false);
		}
		if constexpr (thread_safe) {
			spin_lock.unlock();
		}
	}
};

// --- Folded from core/templates/paged_array.h ---

#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/typedefs.h"

#include <type_traits>

// PagedArray is used mainly for filling a very large array from multiple threads efficiently and without causing major fragmentation

// PageArrayPool manages central page allocation in a thread safe matter

template <typename T>
class PagedArrayPool {
	T **page_pool = nullptr;
	uint32_t pages_allocated = 0;

	uint32_t *available_page_pool = nullptr;
	uint32_t pages_available = 0;

	uint32_t page_size = 0;
	SpinLock spin_lock;

public:
	struct PageInfo {
		T *page = nullptr;
		uint32_t page_id = 0;
	};

	PageInfo alloc_page() {
		spin_lock.lock();
		if (unlikely(pages_available == 0)) {
			uint32_t pages_used = pages_allocated;

			pages_allocated++;
			page_pool = (T **)memrealloc(page_pool, sizeof(T *) * pages_allocated);
			available_page_pool = (uint32_t *)memrealloc(available_page_pool, sizeof(uint32_t) * pages_allocated);

			page_pool[pages_used] = (T *)memalloc(sizeof(T) * page_size);
			available_page_pool[0] = pages_used;

			pages_available++;
		}

		pages_available--;
		uint32_t page_id = available_page_pool[pages_available];
		T *page = page_pool[page_id];
		spin_lock.unlock();

		return PageInfo{ page, page_id };
	}

	void free_page(uint32_t p_page_id) {
		spin_lock.lock();
		available_page_pool[pages_available] = p_page_id;
		pages_available++;
		spin_lock.unlock();
	}

	uint32_t get_page_size_shift() const {
		return Math::get_shift_from_power_of_2(page_size);
	}

	uint32_t get_page_size_mask() const {
		return page_size - 1;
	}

	void reset() {
		ERR_FAIL_COND(pages_available < pages_allocated);
		if (pages_allocated) {
			for (uint32_t i = 0; i < pages_allocated; i++) {
				memfree(page_pool[i]);
			}
			memfree(page_pool);
			memfree(available_page_pool);
			page_pool = nullptr;
			available_page_pool = nullptr;
			pages_allocated = 0;
			pages_available = 0;
		}
	}
	bool is_configured() const {
		return page_size > 0;
	}

	void configure(uint32_t p_page_size) {
		ERR_FAIL_COND(page_pool != nullptr); // Safety check.
		ERR_FAIL_COND(p_page_size == 0);
		page_size = Math::nearest_power_of_2_templated(p_page_size);
	}

	PagedArrayPool(uint32_t p_page_size = 4096) { // power of 2 recommended because of alignment with OS page sizes. Even if element is bigger, its still a multiple and get rounded amount of pages
		configure(p_page_size);
	}

	~PagedArrayPool() {
		ERR_FAIL_COND_MSG(pages_available < pages_allocated, "Pages in use exist at exit in PagedArrayPool");
		reset();
	}
};

// PageArray is a local array that is optimized to grow in place, then be cleared often.
// It does so by allocating pages from a PagedArrayPool.
// It is safe to use multiple PagedArrays from different threads, sharing a single PagedArrayPool

template <typename T>
class _WARN_UNUSED_ PagedArray {
	PagedArrayPool<T> *page_pool = nullptr;

	T **page_data = nullptr;
	uint32_t *page_ids = nullptr;
	uint32_t max_pages_used = 0;
	uint32_t page_size_shift = 0;
	uint32_t page_size_mask = 0;
	uint64_t count = 0;

	_FORCE_INLINE_ uint32_t _get_pages_in_use() const {
		if (count == 0) {
			return 0;
		} else {
			return ((count - 1) >> page_size_shift) + 1;
		}
	}

	void _grow_page_array() {
		//no more room in the page array to put the new page, make room
		if (max_pages_used == 0) {
			max_pages_used = 1;
		} else {
			max_pages_used *= 2; // increase in powers of 2 to keep allocations to minimum
		}
		page_data = (T **)memrealloc(page_data, sizeof(T *) * max_pages_used);
		page_ids = (uint32_t *)memrealloc(page_ids, sizeof(uint32_t) * max_pages_used);
	}

public:
	_FORCE_INLINE_ const T &operator[](uint64_t p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page = p_index >> page_size_shift;
		uint32_t offset = p_index & page_size_mask;

		return page_data[page][offset];
	}
	_FORCE_INLINE_ T &operator[](uint64_t p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page = p_index >> page_size_shift;
		uint32_t offset = p_index & page_size_mask;

		return page_data[page][offset];
	}

	_FORCE_INLINE_ void push_back(const T &p_value) {
		uint32_t remainder = count & page_size_mask;
		if (unlikely(remainder == 0)) {
			// at 0, so time to request a new page
			uint32_t page_count = _get_pages_in_use();
			uint32_t new_page_count = page_count + 1;

			if (unlikely(new_page_count > max_pages_used)) {
				ERR_FAIL_NULL(page_pool); // Safety check.

				_grow_page_array(); //keep out of inline
			}

			typename PagedArrayPool<T>::PageInfo page_info = page_pool->alloc_page();
			page_data[page_count] = page_info.page;
			page_ids[page_count] = page_info.page_id;
		}

		// place the new value
		uint32_t page = count >> page_size_shift;
		uint32_t offset = count & page_size_mask;

		if constexpr (!std::is_trivially_constructible_v<T>) {
			memnew_placement(&page_data[page][offset], T(p_value));
		} else {
			page_data[page][offset] = p_value;
		}

		count++;
	}

	_FORCE_INLINE_ void pop_back() {
		ERR_FAIL_COND(count == 0);

		if constexpr (!std::is_trivially_destructible_v<T>) {
			uint32_t page = (count - 1) >> page_size_shift;
			uint32_t offset = (count - 1) & page_size_mask;
			page_data[page][offset].~T();
		}

		uint32_t remainder = count & page_size_mask;
		if (unlikely(remainder == 1)) {
			// one element remained, so page must be freed.
			uint32_t last_page = _get_pages_in_use() - 1;
			page_pool->free_page(page_ids[last_page]);
		}
		count--;
	}

	void remove_at_unordered(uint64_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		(*this)[p_index] = (*this)[count - 1];
		pop_back();
	}

	void clear() {
		//destruct if needed
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (uint64_t i = 0; i < count; i++) {
				uint32_t page = i >> page_size_shift;
				uint32_t offset = i & page_size_mask;
				page_data[page][offset].~T();
			}
		}

		//return the pages to the pagepool, so they can be used by another array eventually
		uint32_t pages_used = _get_pages_in_use();
		for (uint32_t i = 0; i < pages_used; i++) {
			page_pool->free_page(page_ids[i]);
		}

		count = 0;

		//note we leave page_data and page_indices intact for next use. If you really want to clear them call reset()
	}

	void reset() {
		clear();
		if (page_data) {
			memfree(page_data);
			memfree(page_ids);
			page_data = nullptr;
			page_ids = nullptr;
			max_pages_used = 0;
		}
	}

	// This takes the pages from a source array and merges them to this one
	// resulting order is undefined, but content is merged very efficiently,
	// making it ideal to fill content on several threads to later join it.

	void merge_unordered(PagedArray<T> &p_array) {
		ERR_FAIL_COND(page_pool != p_array.page_pool);

		uint32_t remainder = count & page_size_mask;

		T *remainder_page = nullptr;
		uint32_t remainder_page_id = 0;

		if (remainder > 0) {
			uint32_t last_page = _get_pages_in_use() - 1;
			remainder_page = page_data[last_page];
			remainder_page_id = page_ids[last_page];
		}

		count -= remainder;

		uint32_t src_page_index = 0;
		uint32_t page_size = page_size_mask + 1;

		while (p_array.count > 0) {
			uint32_t page_count = _get_pages_in_use();
			uint32_t new_page_count = page_count + 1;

			if (unlikely(new_page_count > max_pages_used)) {
				_grow_page_array(); //keep out of inline
			}

			page_data[page_count] = p_array.page_data[src_page_index];
			page_ids[page_count] = p_array.page_ids[src_page_index];

			uint32_t take = MIN(p_array.count, page_size); //pages to take away
			p_array.count -= take;
			count += take;
			src_page_index++;
		}

		//handle the remainder page if exists
		if (remainder_page) {
			uint32_t new_remainder = count & page_size_mask;

			if (new_remainder > 0) {
				//must merge old remainder with new remainder

				T *dst_page = page_data[_get_pages_in_use() - 1];
				uint32_t to_copy = MIN(page_size - new_remainder, remainder);

				for (uint32_t i = 0; i < to_copy; i++) {
					if constexpr (!std::is_trivially_constructible_v<T>) {
						memnew_placement(&dst_page[i + new_remainder], T(remainder_page[i + remainder - to_copy]));
					} else {
						dst_page[i + new_remainder] = remainder_page[i + remainder - to_copy];
					}

					if constexpr (!std::is_trivially_destructible_v<T>) {
						remainder_page[i + remainder - to_copy].~T();
					}
				}

				remainder -= to_copy; //subtract what was copied from remainder
				count += to_copy; //add what was copied to the count

				if (remainder == 0) {
					//entire remainder copied, let go of remainder page
					page_pool->free_page(remainder_page_id);
					remainder_page = nullptr;
				}
			}

			if (remainder > 0) {
				//there is still remainder, append it
				uint32_t page_count = _get_pages_in_use();
				uint32_t new_page_count = page_count + 1;

				if (unlikely(new_page_count > max_pages_used)) {
					_grow_page_array(); //keep out of inline
				}

				page_data[page_count] = remainder_page;
				page_ids[page_count] = remainder_page_id;

				count += remainder;
			}
		}
	}

	_FORCE_INLINE_ uint64_t size() const {
		return count;
	}

	void set_page_pool(PagedArrayPool<T> *p_page_pool) {
		ERR_FAIL_COND(max_pages_used > 0); // Safety check.

		page_pool = p_page_pool;
		page_size_mask = page_pool->get_page_size_mask();
		page_size_shift = page_pool->get_page_size_shift();
	}

	~PagedArray() {
		reset();
	}
};
// --- End core/templates/paged_allocator.h ---
// --- Inlined/Folded from core/templates/ring_buffer.h ---
#include "core/string/ustring.h"

template <typename T> class _WARN_UNUSED_ RingBuffer
{
	LocalVector<T> data;
	int read_pos = 0;
	int write_pos = 0;
	int size_mask;

	inline int inc(int& p_var, int p_size) const
	{
		int ret = p_var;
		p_var += p_size;
		p_var = p_var & size_mask;
		return ret;
	}

public:
	T read()
	{
		ERR_FAIL_COND_V(data_left() < 1, T());
		return data.ptr()[inc(read_pos, 1)];
	}

	int read(T* p_buf, int p_size, bool p_advance = true)
	{
		int left = data_left();
		p_size = MIN(left, p_size);
		int pos = read_pos;
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			const T* read = data.ptr();
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = read[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		if (p_advance) {
			inc(read_pos, p_size);
		}
		return p_size;
	}

	int copy(T* p_buf, int p_offset, int p_size) const
	{
		int left = data_left();
		if ((p_offset + p_size) > left) {
			p_size = left - p_offset;
			if (p_size <= 0) {
				return 0;
			}
		}
		p_size = MIN(left, p_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = data[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		return p_size;
	}

	int find(const T& p_value, int p_offset, int p_max_size) const
	{
		int left = data_left();
		if ((p_offset + p_max_size) > left) {
			p_max_size = left - p_offset;
			if (p_max_size <= 0) {
				return 0;
			}
		}
		p_max_size = MIN(left, p_max_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_max_size;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				if (data[pos + i] == p_value) {
					return i + (p_max_size - to_read);
				}
			}
			to_read -= total;
			pos = 0;
		}
		return -1;
	}

	inline int advance_read(int p_n)
	{
		p_n = MIN(p_n, data_left());
		inc(read_pos, p_n);
		return p_n;
	}

	inline int decrease_write(int p_n)
	{
		p_n = MIN(p_n, data_left());
		inc(write_pos, size_mask + 1 - p_n);
		return p_n;
	}

	Error write(const T& p_v)
	{
		ERR_FAIL_COND_V(space_left() < 1, FAILED);
		data[inc(write_pos, 1)] = p_v;
		return OK;
	}

	int write(const T* p_buf, int p_size)
	{
		int left = space_left();
		p_size = MIN(left, p_size);

		int pos = write_pos;
		int to_write = p_size;
		int src = 0;
		while (to_write) {
			int end = pos + to_write;
			end = MIN(end, size());
			int total = end - pos;

			for (int i = 0; i < total; i++) {
				data[pos + i] = p_buf[src++];
			}
			to_write -= total;
			pos = 0;
		}

		inc(write_pos, p_size);
		return p_size;
	}

	inline int space_left() const
	{
		int left = read_pos - write_pos;
		if (left < 0) {
			return size() + left - 1;
		}
		if (left == 0) {
			return size() - 1;
		}
		return left - 1;
	}

	inline int data_left() const { return size() - space_left() - 1; }

	inline int size() const { return data.size(); }

	inline void clear()
	{
		read_pos = 0;
		write_pos = 0;
	}

	void resize(int p_power)
	{
		int old_size = size();
		int new_size = 1 << p_power;
		int mask = new_size - 1;
		data.resize(int64_t(1) << int64_t(p_power));
		if (old_size < new_size && read_pos > write_pos) {
			for (int i = 0; i < write_pos; i++) {
				data[(old_size + i) & mask] = data[i];
			}
			write_pos = (old_size + write_pos) & mask;
		}
		else {
			read_pos = read_pos & mask;
			write_pos = write_pos & mask;
		}

		size_mask = mask;
	}

	RingBuffer(int p_power = 0) { resize(p_power); }
};
#include "core/string/ustring.h"

// Packed Arrays
typedef Vector<uint8_t> PackedByteArray;
typedef Vector<int32_t> PackedInt32Array;
typedef Vector<int64_t> PackedInt64Array;
typedef Vector<float> PackedFloat32Array;
typedef Vector<double> PackedFloat64Array;
typedef Vector<String> PackedStringArray;
typedef Vector<Vector2> PackedVector2Array;
typedef Vector<Vector3> PackedVector3Array;
typedef Vector<Color> PackedColorArray;
typedef Vector<Vector4> PackedVector4Array;

template <typename T>
struct HashMapHasherDefaultImpl<Ref<T>> {
	static _FORCE_INLINE_ uint32_t hash(const Ref<T> &p_ref) {
		return hash_one_uint64(reinterpret_cast<uint64_t>(p_ref.ptr()));
	}
};

// --- Folded from core/math/expression.h ---


class Expression : public RefCounted
{
protected:
	String expression;

	int str_ofs = 0;
	bool expression_dirty = false;

	bool _compile_expression();

	enum TokenType
	{
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_PARENTHESIS_OPEN,
		TK_PARENTHESIS_CLOSE,
		TK_IDENTIFIER,
		TK_BUILTIN_FUNC,
		TK_SELF,
		TK_CONSTANT,
		TK_BASIC_TYPE,
		TK_COLON,
		TK_COMMA,
		TK_PERIOD,
		TK_OP_IN,
		TK_OP_EQUAL,
		TK_OP_NOT_EQUAL,
		TK_OP_LESS,
		TK_OP_LESS_EQUAL,
		TK_OP_GREATER,
		TK_OP_GREATER_EQUAL,
		TK_OP_AND,
		TK_OP_OR,
		TK_OP_NOT,
		TK_OP_ADD,
		TK_OP_SUB,
		TK_OP_MUL,
		TK_OP_DIV,
		TK_OP_MOD,
		TK_OP_POW,
		TK_OP_SHIFT_LEFT,
		TK_OP_SHIFT_RIGHT,
		TK_OP_BIT_AND,
		TK_OP_BIT_OR,
		TK_OP_BIT_XOR,
		TK_OP_BIT_INVERT,
		TK_INPUT,
		TK_EOF,
		TK_ERROR,
		TK_MAX
	};

	static const char* token_name[TK_MAX];

	struct Token
	{
		TokenType type;
	};

	void _set_error(const String& p_err)
	{
		if (error_set) {
			return;
		}
		error_str = p_err;
		error_set = true;
	}

	Error _get_token(Token& r_token);

	String error_str;
	bool error_set = true;

	struct ENode
	{
		enum Type
		{
			TYPE_INPUT,
			TYPE_CONSTANT,
			TYPE_SELF,
			TYPE_OPERATOR,
			TYPE_INDEX,
			TYPE_NAMED_INDEX,
			TYPE_ARRAY,
			TYPE_DICTIONARY,
			TYPE_CONSTRUCTOR,
			TYPE_BUILTIN_FUNC,
			TYPE_CALL
		};

		ENode* next = nullptr;

		Type type = TYPE_INPUT;

		int input_index = 0;
		ENode* nodes[2] = {nullptr, nullptr};
		ENode* base = nullptr;
		ENode* index = nullptr;
		String name;
		Vector<ENode*> arguments;

		~ENode()
		{
			if (next) {
				memdelete(next);
			}
		}
	};

	struct ExpressionNode
	{
		bool is_op = false;

		union
		{
			ENode* node = nullptr;
		};
	};

	ENode* _parse_expression();

	ENode* alloc_node(ENode::Type p_type = ENode::TYPE_INPUT)
	{
		ENode* node = memnew(ENode);
		node->type = p_type;
		node->next = nodes;
		nodes = node;
		return node;
	}

	ENode* root = nullptr;
	ENode* nodes = nullptr;

	Vector<String> input_names;

	bool execution_error = false;

protected:

public:
	Error parse(const String& p_expression, const Vector<String>& p_input_names = Vector<String>());
	bool has_execute_failed() const;
	String get_error_text() const;

	~Expression();
};
// --- End core/math/expression.h ---
#include "thirdparty/openxr/include/openxr/openxr.h"


// --- Folded from core/math/random_pcg.h ---
#include "core/typedefs.h"

#include <thirdparty/misc/pcg.h>

#include <cmath> // ldexp

#if defined(__GNUC__)
#define CLZ32(m_x) __builtin_clz(m_x)
#elif defined(_MSC_VER)
#include <intrin.h>
static int __bsr_clz32(uint32_t p_x) {
	unsigned long index;
	_BitScanReverse(&index, p_x);
	return 31 - index;
}
#define CLZ32(m_x) __bsr_clz32(m_x)
#endif

template <typename T>
class Vector;

class RandomPCG {
	pcg32_random_t pcg;
	uint64_t current_seed = 0; // The seed the current generator state started from.
	uint64_t current_inc = 0;

public:
	static const uint64_t DEFAULT_SEED = 12047754176567800795U;
	static const uint64_t DEFAULT_INC = PCG_DEFAULT_INC_64;

	RandomPCG(uint64_t p_seed = DEFAULT_SEED, uint64_t p_inc = DEFAULT_INC);

	_FORCE_INLINE_ void seed(uint64_t p_seed) {
		current_seed = p_seed;
		pcg32_srandom_r(&pcg, current_seed, current_inc);
	}
	_FORCE_INLINE_ uint64_t get_seed() { return current_seed; }

	_FORCE_INLINE_ void set_state(uint64_t p_state) { pcg.state = p_state; }
	_FORCE_INLINE_ uint64_t get_state() const { return pcg.state; }

	void randomize();
	_FORCE_INLINE_ uint32_t rand() {
		return pcg32_random_r(&pcg);
	}
	_FORCE_INLINE_ uint32_t rand(uint32_t p_bounds) {
		return pcg32_boundedrand_r(&pcg, p_bounds);
	}

	int64_t rand_weighted(const Vector<float> &p_weights);

	// Obtaining floating point numbers in [0, 1] range with "good enough" uniformity.
	// These functions sample the output of rand() as the fraction part of an infinite binary number,
	// with some tricks applied to reduce ops and branching:
	// 1. Instead of shifting to the first 1 and connecting random bits, we simply set the MSB and LSB to 1.
	//    Provided that the RNG is actually uniform bit by bit, this should have the exact same effect.
	// 2. In order to compensate for exponent info loss, we count zeros from another random number,
	//    and just add that to the initial offset.
	//    This has the same probability as counting and shifting an actual bit stream: 2^-n for n zeroes.
	// For all numbers above 2^-96 (2^-64 for floats), the functions should be uniform.
	// However, all numbers below that threshold are floored to 0.
	// The thresholds are chosen to minimize rand() calls while keeping the numbers within a totally subjective quality standard.
	// If clz or ldexp isn't available, fall back to bit truncation for performance, sacrificing uniformity.
	_FORCE_INLINE_ double randd() {
#if defined(CLZ32)
		uint32_t proto_exp_offset = rand();
		if (unlikely(proto_exp_offset == 0)) {
			return 0;
		}
		uint64_t significand = (((uint64_t)rand()) << 32) | rand() | 0x8000000000000001U;
		return std::ldexp((double)significand, -64 - CLZ32(proto_exp_offset));
#else
#pragma message("RandomPCG::randd - intrinsic clz is not available, falling back to bit truncation")
		return (double)(((((uint64_t)rand()) << 32) | rand()) & 0x1FFFFFFFFFFFFFU) / (double)0x1FFFFFFFFFFFFFU;
#endif
	}
	_FORCE_INLINE_ float randf() {
#if defined(CLZ32)
		uint32_t proto_exp_offset = rand();
		if (unlikely(proto_exp_offset == 0)) {
			return 0;
		}
		return std::ldexp((float)(rand() | 0x80000001), -32 - CLZ32(proto_exp_offset));
#else
#pragma message("RandomPCG::randf - intrinsic clz is not available, falling back to bit truncation")
		return (float)(rand() & 0xFFFFFF) / (float)0xFFFFFF;
#endif
	}

	_FORCE_INLINE_ double randfn(double p_mean, double p_deviation) {
		double temp = randd();
		if (temp < CMP_EPSILON) {
			temp += CMP_EPSILON; // To prevent generating of INF value in log function, resulting to return NaN value from this function.
		}
		return p_mean + p_deviation * (Math::cos(Math::TAU * randd()) * Math::sqrt(-2.0 * Math::log(temp))); // Box-Muller transform.
	}
	_FORCE_INLINE_ float randfn(float p_mean, float p_deviation) {
		float temp = randf();
		if (temp < CMP_EPSILON) {
			temp += CMP_EPSILON; // To prevent generating of INF value in log function, resulting to return NaN value from this function.
		}
		return p_mean + p_deviation * (Math::cos((float)Math::TAU * randf()) * Math::sqrt(-2.0 * Math::log(temp))); // Box-Muller transform.
	}

	double random(double p_from, double p_to);
	float random(float p_from, float p_to);
	int random(int p_from, int p_to);
};

// --- Folded from core/math/random_number_generator.h ---


class RandomNumberGenerator : public RefCounted
{
protected:
	RandomPCG randbase;


public:
	_FORCE_INLINE_ void set_seed(uint64_t p_seed) { randbase.seed(p_seed); }

	_FORCE_INLINE_ uint64_t get_seed() { return randbase.get_seed(); }

	_FORCE_INLINE_ void set_state(uint64_t p_state) { randbase.set_state(p_state); }

	_FORCE_INLINE_ uint64_t get_state() const { return randbase.get_state(); }

	_FORCE_INLINE_ void randomize() { randbase.randomize(); }

	_FORCE_INLINE_ uint32_t randi() { return randbase.rand(); }

	_FORCE_INLINE_ real_t randf() { return randbase.randf(); }

	_FORCE_INLINE_ real_t randf_range(real_t p_from, real_t p_to)
	{
		return randbase.random(p_from, p_to);
	}

	_FORCE_INLINE_ real_t randfn(real_t p_mean = 0.0, real_t p_deviation = 1.0)
	{
		return randbase.randfn(p_mean, p_deviation);
	}

	_FORCE_INLINE_ int randi_range(int p_from, int p_to) { return randbase.random(p_from, p_to); }

	_FORCE_INLINE_ int64_t rand_weighted(const Vector<float>& p_weights)
	{
		return randbase.rand_weighted(p_weights);
	}

	RandomNumberGenerator() { randbase.randomize(); }
};
// --- End core/math/random_pcg.h ---

#endif // TYPES_H

// --- Folded from core/templates/self_list.h ---
#include "core/typedefs.h"
#include "core/templates/list.h"
#include "core/typedefs.h"

template <typename T> class _WARN_UNUSED_ SelfList
{
public:
	class List
	{
		SelfList<T>* _first = nullptr;
		SelfList<T>* _last = nullptr;

	public:
		void add(SelfList<T>* p_elem)
		{
			ERR_FAIL_COND(p_elem->_root);

			p_elem->_root = this;
			p_elem->_next = _first;
			p_elem->_prev = nullptr;

			if (_first) {
				_first->_prev = p_elem;

			}
			else {
				_last = p_elem;
			}

			_first = p_elem;
		}

		void add_last(SelfList<T>* p_elem)
		{
			ERR_FAIL_COND(p_elem->_root);

			p_elem->_root = this;
			p_elem->_next = nullptr;
			p_elem->_prev = _last;

			if (_last) {
				_last->_next = p_elem;

			}
			else {
				_first = p_elem;
			}

			_last = p_elem;
		}

		void remove(SelfList<T>* p_elem)
		{
			ERR_FAIL_COND(p_elem->_root != this);
			if (p_elem->_next) {
				p_elem->_next->_prev = p_elem->_prev;
			}

			if (p_elem->_prev) {
				p_elem->_prev->_next = p_elem->_next;
			}

			if (_first == p_elem) {
				_first = p_elem->_next;
			}

			if (_last == p_elem) {
				_last = p_elem->_prev;
			}

			p_elem->_next = nullptr;
			p_elem->_prev = nullptr;
			p_elem->_root = nullptr;
		}

		void clear()
		{
			while (_first) {
				remove(_first);
			}
		}

		void sort() { sort_custom<Comparator<T>>(); }

		template <typename C> void sort_custom()
		{
			if (_first == _last) {
				return;
			}

			struct PtrComparator
			{
				C compare;

				_FORCE_INLINE_ bool operator()(const T* p_a, const T* p_b) const
				{
					return compare(*p_a, *p_b);
				}
			};

			using Element = SelfList<T>;
			SortList<Element, T*, _self_ref, _prev_ref, _next_ref, PtrComparator> sorter;
			sorter.sort(_first, _last);
		}

		_FORCE_INLINE_ SelfList<T>* first() { return _first; }

		_FORCE_INLINE_ const SelfList<T>* first() const { return _first; }

		// Forbid copying, which has broken behavior.
		void operator=(const List&) = delete;

		_FORCE_INLINE_ ~List()
		{
			// A self list must be empty on destruction.
			DEV_ASSERT(_first == nullptr);
		}
	};

private:
	List* _root = nullptr;
	T* _self = nullptr;
	SelfList<T>* _next = nullptr;
	SelfList<T>* _prev = nullptr;
	// Specify pointers in variables to work around a VS 2022 bug (GH-121326).
	static constexpr T* SelfList<T>::*_self_ref = &SelfList<T>::_self;
	static constexpr SelfList<T>* SelfList<T>::*_prev_ref = &SelfList<T>::_prev;
	static constexpr SelfList<T>* SelfList<T>::*_next_ref = &SelfList<T>::_next;

public:
	_FORCE_INLINE_ bool in_list() const { return _root; }

	_FORCE_INLINE_ void remove_from_list()
	{
		if (_root) {
			_root->remove(this);
		}
	}

	_FORCE_INLINE_ SelfList<T>* next() { return _next; }

	_FORCE_INLINE_ SelfList<T>* prev() { return _prev; }

	_FORCE_INLINE_ const SelfList<T>* next() const { return _next; }

	_FORCE_INLINE_ const SelfList<T>* prev() const { return _prev; }

	_FORCE_INLINE_ T* self() const { return _self; }

	// Forbid copying, which has broken behavior.
	void operator=(const SelfList<T>&) = delete;

	_FORCE_INLINE_ SelfList() { this; }

	_FORCE_INLINE_ SelfList(T* p_self) { _self = p_self; }

	_FORCE_INLINE_ ~SelfList()
	{
		if (_root) {
			_root->remove(this);
		}
	}
};
// --- End core/templates/self_list.h ---

// --- Folded from core/templates/a_hash_map.h ---
#include <initializer_list>

#include "core/os/memory.h"
#include "core/string/ustring.h"
#include "core/math/vector2.h"

class String;
class String;

/**
 * An array-based implementation of a hash map. It is very efficient in terms of performance and
 * memory usage. Works like a dynamic array, adding elements to the end of the array, and
 * allows you to access array elements by their index by using `get_by_index` method.
 * Example:
 * ```
 *  AHashMap<int, Object *> map;
 *
 *  int get_object_id_by_number(int p_number) {
 *		int id = map.get_index(p_number);
 *		return id;
 *  }
 *
 *  Object *get_object_by_id(int p_id) {
 *		map.get_by_index(p_id).value;
 *  }
 * ```
 * Still, don`t erase the elements because ID can break.
 *
 * When an element erase, its place is taken by the element from the end.
 *
 *        <-------------
 *      |               |
 *  6 8 X 9 32 -1 5 -10 7 X X X
 *  6 8 7 9 32 -1 5 -10 X X X X
 *
 *
 * Use RBMap if you need to iterate over sorted elements.
 *
 * Use HashMap if:
 *   - You need to keep an iterator or const pointer to Key and you intend to add/remove elements in
 *the meantime.
 *   - You need to preserve the insertion order when using erase.
 *
 * It is recommended to use `HashMap` if `KeyValue` size is very large.
 */
template <typename TKey, typename TValue, typename Hasher = HashMapHasherDefault,
	typename Comparator = HashMapComparatorDefault<TKey>>
class _WARN_UNUSED_ AHashMap
{
public:
	// Must be a power of two.
	static constexpr uint32_t INITIAL_CAPACITY = 16;
	static constexpr uint32_t EMPTY_HASH = 0;
	static_assert(EMPTY_HASH == 0, "EMPTY_HASH must always be 0 for the memcpy() optimization.");

private:
	struct Metadata
	{
		uint32_t hash;
		uint32_t element_idx;
	};

	static_assert(sizeof(Metadata) == 8);

	typedef KeyValue<TKey, TValue> MapKeyValue;
	MapKeyValue* _elements = nullptr;
	Metadata* _metadata = nullptr;

	// Due to optimization, this is `capacity - 1`. Use + 1 to get normal capacity.
	uint32_t _capacity_mask = 0;
	uint32_t _size = 0;

	uint32_t _hash(const TKey& p_key) const
	{
		uint32_t hash = Hasher::hash(p_key);

		if (unlikely(hash == EMPTY_HASH)) {
			hash = EMPTY_HASH + 1;
		}

		return hash;
	}

	static _FORCE_INLINE_ uint32_t _get_resize_count(uint32_t p_capacity_mask)
	{
		return p_capacity_mask ^
			   (p_capacity_mask + 1) >>
				   2; // = get_capacity() * 0.75 - 1; Works only if p_capacity_mask = 2^n - 1.
	}

	static _FORCE_INLINE_ uint32_t _get_probe_length(
		uint32_t p_meta_idx, uint32_t p_hash, uint32_t p_capacity)
	{
		const uint32_t original_idx = p_hash & p_capacity;
		return (p_meta_idx - original_idx + p_capacity + 1) & p_capacity;
	}

	bool _lookup_idx(const TKey& p_key, uint32_t& r_element_idx, uint32_t& r_meta_idx) const
	{
		if (unlikely(_elements == nullptr)) {
			return false; // Failed lookups, no _elements.
		}
		return _lookup_idx_with_hash(p_key, r_element_idx, r_meta_idx, _hash(p_key));
	}

	bool _lookup_idx_with_hash(
		const TKey& p_key, uint32_t& r_element_idx, uint32_t& r_meta_idx, uint32_t p_hash) const
	{
		if (unlikely(_elements == nullptr)) {
			return false; // Failed lookups, no _elements.
		}

		uint32_t meta_idx = p_hash & _capacity_mask;
		Metadata metadata = _metadata[meta_idx];
		if (metadata.hash == p_hash &&
			Comparator::compare(_elements[metadata.element_idx].key, p_key)) {
			r_element_idx = metadata.element_idx;
			r_meta_idx = meta_idx;
			return true;
		}

		if (metadata.hash == EMPTY_HASH) {
			return false;
		}

		// A collision occurred.
		meta_idx = (meta_idx + 1) & _capacity_mask;
		uint32_t distance = 1;
		while (true) {
			metadata = _metadata[meta_idx];
			if (metadata.hash == p_hash &&
				Comparator::compare(_elements[metadata.element_idx].key, p_key)) {
				r_element_idx = metadata.element_idx;
				r_meta_idx = meta_idx;
				return true;
			}

			if (metadata.hash == EMPTY_HASH) {
				return false;
			}

			if (distance > _get_probe_length(meta_idx, metadata.hash, _capacity_mask)) {
				return false;
			}

			meta_idx = (meta_idx + 1) & _capacity_mask;
			distance++;
		}
	}

	uint32_t _insert_metadata(uint32_t p_hash, uint32_t p_element_idx)
	{
		uint32_t meta_idx = p_hash & _capacity_mask;

		if (_metadata[meta_idx].hash == EMPTY_HASH) {
			_metadata[meta_idx] = Metadata{p_hash, p_element_idx};
			return meta_idx;
		}

		uint32_t distance = 1;
		meta_idx = (meta_idx + 1) & _capacity_mask;
		Metadata metadata;
		metadata.hash = p_hash;
		metadata.element_idx = p_element_idx;

		while (true) {
			if (_metadata[meta_idx].hash == EMPTY_HASH) {
#ifdef DEV_ENABLED
				if (unlikely(distance > 12)) {
					WARN_PRINT("Excessive collision count, is the right hash function being used?");
				}
#endif
				_metadata[meta_idx] = metadata;
				return meta_idx;
			}

			// Not an empty slot, let's check the probing length of the existing one.
			uint32_t existing_probe_len =
				_get_probe_length(meta_idx, _metadata[meta_idx].hash, _capacity_mask);
			if (existing_probe_len < distance) {
				SWAP(metadata, _metadata[meta_idx]);
				distance = existing_probe_len;
			}

			meta_idx = (meta_idx + 1) & _capacity_mask;
			distance++;
		}
	}

	void _resize_and_rehash(uint32_t p_new_capacity)
	{
		uint32_t real_old_capacity = _capacity_mask + 1;
		// Capacity can't be 0 and must be 2^n - 1.
		_capacity_mask = MAX(4u, p_new_capacity);
		uint32_t real_capacity = Math::next_power_of_2(_capacity_mask);
		_capacity_mask = real_capacity - 1;

		Metadata* old_map_data = _metadata;

		_metadata = reinterpret_cast<Metadata*>(
			Memory::alloc_static_zeroed(sizeof(Metadata) * real_capacity));
		_elements = reinterpret_cast<MapKeyValue*>(Memory::realloc_static(
			_elements, sizeof(MapKeyValue) * (_get_resize_count(_capacity_mask) + 1)));

		if (_size != 0) {
			for (uint32_t i = 0; i < real_old_capacity; i++) {
				Metadata metadata = old_map_data[i];
				if (metadata.hash != EMPTY_HASH) {
					_insert_metadata(metadata.hash, metadata.element_idx);
				}
			}
		}

		Memory::free_static(old_map_data);
	}

	int32_t _insert_element(const TKey& p_key, const TValue& p_value, uint32_t p_hash)
	{
		if (unlikely(_elements == nullptr)) {
			// Allocate on demand to save memory.

			uint32_t real_capacity = _capacity_mask + 1;
			_metadata = reinterpret_cast<Metadata*>(
				Memory::alloc_static_zeroed(sizeof(Metadata) * real_capacity));
			_elements = reinterpret_cast<MapKeyValue*>(Memory::alloc_static(
				sizeof(MapKeyValue) * (_get_resize_count(_capacity_mask) + 1)));
		}

		if (unlikely(_size > _get_resize_count(_capacity_mask))) {
			_resize_and_rehash(_capacity_mask * 2);
		}

		memnew_placement(&_elements[_size], MapKeyValue(p_key, p_value));

		_insert_metadata(p_hash, _size);
		_size++;
		return _size - 1;
	}

	void _init_from(const AHashMap& p_other)
	{
		_capacity_mask = p_other._capacity_mask;
		uint32_t real_capacity = _capacity_mask + 1;
		_size = p_other._size;

		if (p_other._size == 0) {
			return;
		}

		_metadata =
			reinterpret_cast<Metadata*>(Memory::alloc_static(sizeof(Metadata) * real_capacity));
		_elements = reinterpret_cast<MapKeyValue*>(
			Memory::alloc_static(sizeof(MapKeyValue) * (_get_resize_count(_capacity_mask) + 1)));

		if constexpr (std::is_trivially_copyable_v<TKey> && std::is_trivially_copyable_v<TValue>) {
			void* destination = _elements;
			const void* source = p_other._elements;
			memcpy(destination, source, sizeof(MapKeyValue) * _size);
		}
		else {
			for (uint32_t i = 0; i < _size; i++) {
				memnew_placement(&_elements[i], MapKeyValue(p_other._elements[i]));
			}
		}

		memcpy(_metadata, p_other._metadata, sizeof(Metadata) * real_capacity);
	}

public:
	/* Standard Godot Container API */

	_FORCE_INLINE_ uint32_t get_capacity() const { return _capacity_mask + 1; }

	_FORCE_INLINE_ uint32_t size() const { return _size; }

	_FORCE_INLINE_ bool is_empty() const { return _size == 0; }

	void clear()
	{
		if (_elements == nullptr || _size == 0) {
			return;
		}

		memset(_metadata, EMPTY_HASH, (_capacity_mask + 1) * sizeof(Metadata));
		if constexpr (!(std::is_trivially_destructible_v<TKey> &&
						  std::is_trivially_destructible_v<TValue>)) {
			for (uint32_t i = 0; i < _size; i++) {
				_elements[i].key.~TKey();
				_elements[i].value.~TValue();
			}
		}

		_size = 0;
	}

	TValue& get(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND_MSG(!exists, "AHashMap key not found.");
		return _elements[element_idx].value;
	}

	const TValue& get(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND_MSG(!exists, "AHashMap key not found.");
		return _elements[element_idx].value;
	}

	const TValue* getptr(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (exists) {
			return &_elements[element_idx].value;
		}
		return nullptr;
	}

	TValue* getptr(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (exists) {
			return &_elements[element_idx].value;
		}
		return nullptr;
	}

	bool has(const TKey& p_key) const
	{
		uint32_t _idx = 0;
		uint32_t meta_idx = 0;
		return _lookup_idx(p_key, _idx, meta_idx);
	}

	bool erase(const TKey& p_key)
	{
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);

		if (!exists) {
			return false;
		}

		uint32_t next_meta_idx = (meta_idx + 1) & _capacity_mask;
		while (
			_metadata[next_meta_idx].hash != EMPTY_HASH &&
			_get_probe_length(next_meta_idx, _metadata[next_meta_idx].hash, _capacity_mask) != 0) {
			SWAP(_metadata[next_meta_idx], _metadata[meta_idx]);

			meta_idx = next_meta_idx;
			next_meta_idx = (next_meta_idx + 1) & _capacity_mask;
		}

		_metadata[meta_idx].hash = EMPTY_HASH;
		_elements[element_idx].key.~TKey();
		_elements[element_idx].value.~TValue();
		_size--;

		if (element_idx < _size) {
			memcpy((void*)&_elements[element_idx], (const void*)&_elements[_size],
				sizeof(MapKeyValue));
			uint32_t moved_element_idx = 0;
			uint32_t moved_meta_idx = 0;
			_lookup_idx(_elements[_size].key, moved_element_idx, moved_meta_idx);
			_metadata[moved_meta_idx].element_idx = element_idx;
		}

		return true;
	}

	// Replace the key of an entry in-place, without invalidating iterators or changing the entries
	// position during iteration. p_old_key must exist in the map and p_new_key must not, unless it
	// is equal to p_old_key.
	bool replace_key(const TKey& p_old_key, const TKey& p_new_key)
	{
		if (p_old_key == p_new_key) {
			return true;
		}
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		ERR_FAIL_COND_V(_lookup_idx(p_new_key, element_idx, meta_idx), false);
		ERR_FAIL_COND_V(!_lookup_idx(p_old_key, element_idx, meta_idx), false);
		MapKeyValue& element = _elements[element_idx];
		const_cast<TKey&>(element.key) = p_new_key;

		uint32_t next_meta_idx = (meta_idx + 1) & _capacity_mask;
		while (
			_metadata[next_meta_idx].hash != EMPTY_HASH &&
			_get_probe_length(next_meta_idx, _metadata[next_meta_idx].hash, _capacity_mask) != 0) {
			SWAP(_metadata[next_meta_idx], _metadata[meta_idx]);

			meta_idx = next_meta_idx;
			next_meta_idx = (next_meta_idx + 1) & _capacity_mask;
		}

		_metadata[meta_idx].hash = EMPTY_HASH;

		uint32_t hash = _hash(p_new_key);
		_insert_metadata(hash, element_idx);

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old
	// capacity.
	void reserve(uint32_t p_new_capacity)
	{
		if (_elements == nullptr) {
			_capacity_mask = MAX(4u, p_new_capacity);
			_capacity_mask = Math::next_power_of_2(_capacity_mask) - 1;
			return; // Unallocated yet.
		}
		if (p_new_capacity <= get_capacity()) {
			if (p_new_capacity < size()) {
				WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This "
							 "is likely a mistake.");
			}
			return;
		}
		_resize_and_rehash(p_new_capacity);
	}

	/** Iterator API **/

	struct ConstIterator
	{
		_FORCE_INLINE_ const MapKeyValue& operator*() const { return *pair; }

		_FORCE_INLINE_ const MapKeyValue* operator->() const { return pair; }

		_FORCE_INLINE_ ConstIterator& operator++()
		{
			pair++;
			return *this;
		}

		_FORCE_INLINE_ ConstIterator& operator--()
		{
			pair--;
			if (pair < begin) {
				pair = end;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator& p_other) const
		{
			return pair == p_other.pair;
		}

		_FORCE_INLINE_ bool operator!=(const ConstIterator& p_other) const
		{
			return pair != p_other.pair;
		}

		_FORCE_INLINE_ explicit operator bool() const { return pair != end; }

		_FORCE_INLINE_ ConstIterator(MapKeyValue* p_key, MapKeyValue* p_begin, MapKeyValue* p_end)
		{
			pair = p_key;
			begin = p_begin;
			end = p_end;
		}

		_FORCE_INLINE_ ConstIterator() {}

		_FORCE_INLINE_ ConstIterator(const ConstIterator& p_it)
		{
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

		_FORCE_INLINE_ void operator=(const ConstIterator& p_it)
		{
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

	private:
		MapKeyValue* pair = nullptr;
		MapKeyValue* begin = nullptr;
		MapKeyValue* end = nullptr;
	};

	struct Iterator
	{
		_FORCE_INLINE_ MapKeyValue& operator*() const { return *pair; }

		_FORCE_INLINE_ MapKeyValue* operator->() const { return pair; }

		_FORCE_INLINE_ Iterator& operator++()
		{
			pair++;
			return *this;
		}

		_FORCE_INLINE_ Iterator& operator--()
		{
			pair--;
			if (pair < begin) {
				pair = end;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator& p_other) const
		{
			return pair == p_other.pair;
		}

		_FORCE_INLINE_ bool operator!=(const Iterator& p_other) const
		{
			return pair != p_other.pair;
		}

		_FORCE_INLINE_ explicit operator bool() const { return pair != end; }

		_FORCE_INLINE_ Iterator(MapKeyValue* p_key, MapKeyValue* p_begin, MapKeyValue* p_end)
		{
			pair = p_key;
			begin = p_begin;
			end = p_end;
		}

		_FORCE_INLINE_ Iterator() {}

		_FORCE_INLINE_ Iterator(const Iterator& p_it)
		{
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

		_FORCE_INLINE_ void operator=(const Iterator& p_it)
		{
			pair = p_it.pair;
			begin = p_it.begin;
			end = p_it.end;
		}

		operator ConstIterator() const { return ConstIterator(pair, begin, end); }

	private:
		MapKeyValue* pair = nullptr;
		MapKeyValue* begin = nullptr;
		MapKeyValue* end = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() _LIFETIME_BOUND_
	{
		return Iterator(_elements, _elements, _elements + _size);
	}

	_FORCE_INLINE_ Iterator end() _LIFETIME_BOUND_
	{
		return Iterator(_elements + _size, _elements, _elements + _size);
	}

	_FORCE_INLINE_ Iterator last() _LIFETIME_BOUND_
	{
		if (unlikely(_size == 0)) {
			return Iterator(nullptr, nullptr, nullptr);
		}
		return Iterator(_elements + _size - 1, _elements, _elements + _size);
	}

	Iterator find(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t meta_idx = 0;
		uint32_t element_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return end();
		}
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	void remove(const Iterator& p_iter)
	{
		if (p_iter) {
			erase(p_iter->key);
		}
	}

	_FORCE_INLINE_ ConstIterator begin() const _LIFETIME_BOUND_
	{
		return ConstIterator(_elements, _elements, _elements + _size);
	}

	_FORCE_INLINE_ ConstIterator end() const _LIFETIME_BOUND_
	{
		return ConstIterator(_elements + _size, _elements, _elements + _size);
	}

	_FORCE_INLINE_ ConstIterator last() const _LIFETIME_BOUND_
	{
		if (unlikely(_size == 0)) {
			return ConstIterator(nullptr, nullptr, nullptr);
		}
		return ConstIterator(_elements + _size - 1, _elements, _elements + _size);
	}

	ConstIterator find(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return end();
		}
		return ConstIterator(_elements + element_idx, _elements, _elements + _size);
	}

	/* Indexing */

	const TValue& operator[](const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		CRASH_COND(!exists);
		return _elements[element_idx].value;
	}

	TValue& operator[](const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		uint32_t hash = _hash(p_key);
		bool exists = _lookup_idx_with_hash(p_key, element_idx, meta_idx, hash);

		if (exists) {
			return _elements[element_idx].value;
		}
		else {
			element_idx = _insert_element(p_key, TValue(), hash);
			return _elements[element_idx].value;
		}
	}

	/* Insert */

	Iterator insert(const TKey& p_key, const TValue& p_value) _LIFETIME_BOUND_
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		uint32_t hash = _hash(p_key);
		bool exists = _lookup_idx_with_hash(p_key, element_idx, meta_idx, hash);

		if (!exists) {
			element_idx = _insert_element(p_key, p_value, hash);
		}
		else {
			_elements[element_idx].value = p_value;
		}
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	// Inserts an element without checking if it already exists.
	Iterator insert_new(const TKey& p_key, const TValue& p_value) _LIFETIME_BOUND_
	{
		DEV_ASSERT(!has(p_key));
		uint32_t hash = _hash(p_key);
		uint32_t element_idx = _insert_element(p_key, p_value, hash);
		return Iterator(_elements + element_idx, _elements, _elements + _size);
	}

	/* Array methods. */

	// Unsafe. Changing keys and going outside the bounds of an array can lead to undefined
	// behavior.
	KeyValue<TKey, TValue>* get_elements_ptr() _LIFETIME_BOUND_ { return _elements; }

	// Returns the element index. If not found, returns -1.
	int get_index(const TKey& p_key)
	{
		uint32_t element_idx = 0;
		uint32_t meta_idx = 0;
		bool exists = _lookup_idx(p_key, element_idx, meta_idx);
		if (!exists) {
			return -1;
		}
		return element_idx;
	}

	KeyValue<TKey, TValue>& get_by_index(uint32_t p_index) _LIFETIME_BOUND_
	{
		CRASH_BAD_UNSIGNED_INDEX(p_index, _size);
		return _elements[p_index];
	}

	bool erase_by_index(uint32_t p_index)
	{
		if (p_index >= size()) {
			return false;
		}
		return erase(_elements[p_index].key);
	}

	/* Constructors */

	AHashMap(AHashMap&& p_other)
	{
		_elements = p_other._elements;
		_metadata = p_other._metadata;
		_capacity_mask = p_other._capacity_mask;
		_size = p_other._size;

		p_other._elements = nullptr;
		p_other._metadata = nullptr;
		p_other._capacity_mask = 0;
		p_other._size = 0;
	}

	explicit AHashMap(const AHashMap& p_other) { _init_from(p_other); }

	void operator=(const AHashMap& p_other)
	{
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		reset();

		_init_from(p_other);
	}

	AHashMap(uint32_t p_initial_capacity)
	{
		// Capacity can't be 0 and must be 2^n - 1.
		_capacity_mask = MAX(4u, p_initial_capacity);
		_capacity_mask = Math::next_power_of_2(_capacity_mask) - 1;
	}

	AHashMap() : _capacity_mask(INITIAL_CAPACITY - 1) {}

	AHashMap(std::initializer_list<KeyValue<TKey, TValue>> p_init)
	{
		reserve(p_init.size());
		for (const KeyValue<TKey, TValue>& E : p_init) {
			insert(E.key, E.value);
		}
	}

	void reset()
	{
		if (_elements != nullptr) {
			if constexpr (!(std::is_trivially_destructible_v<TKey> &&
							  std::is_trivially_destructible_v<TValue>)) {
				for (uint32_t i = 0; i < _size; i++) {
					_elements[i].key.~TKey();
					_elements[i].value.~TValue();
				}
			}
			Memory::free_static(_elements);
			Memory::free_static(_metadata);
			_elements = nullptr;
		}
		_capacity_mask = INITIAL_CAPACITY - 1;
		_size = 0;
	}

	~AHashMap() { reset(); }
};

extern template class AHashMap<int, int>;
extern template class AHashMap<String, int>;
extern template class AHashMap<String, String>;
extern template class AHashMap<String, int>;
// --- End core/templates/a_hash_map.h ---

// --- Folded from core/templates/rid_owner.h ---
#include <cstdio>
#include <typeinfo> // IWYU pragma: keep // Used in macro.
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/os/memory.h"


#ifdef TSAN_ENABLED
#include <sanitizer/tsan_interface.h>
#endif

// The following macros would need to be implemented somehow
// for purely weakly ordered architectures. There's a test case
// ("[RID_Owner] Thread safety") with potential to catch issues
// on such architectures if these primitives fail to be implemented.
// For now, they will be just markers about needs that may arise.
#define WEAK_MEMORY_ORDER 0
#if WEAK_MEMORY_ORDER
// Ideally, we'd have implementations that collaborate with the
// sync mechanism used (e.g., the mutex) so instead of some full
// memory barriers being issued, some acquire-release on the
// primitive itself. However, these implementations will at least
// provide correctness.
#define SYNC_ACQUIRE std::atomic_thread_fence(std::memory_order_acquire);
#define SYNC_RELEASE std::atomic_thread_fence(std::memory_order_release);
#else
// Compiler barriers are enough in this case.
#define SYNC_ACQUIRE std::atomic_signal_fence(std::memory_order_acquire);
#define SYNC_RELEASE std::atomic_signal_fence(std::memory_order_release);
#endif

class RID_AllocBase
{
	static inline SafeNumeric<uint64_t> base_id{1};

protected:
	static RID _make_from_id(uint64_t p_id)
	{
		RID rid;
		rid._id = p_id;
		return rid;
	}

	static RID _gen_rid() { return _make_from_id(_gen_id()); }

	friend struct VariantUtilityFunctions;

	static uint64_t _gen_id() { return base_id.increment(); }

public:
	virtual ~RID_AllocBase() {}
};

template <typename T, bool THREAD_SAFE = false> class RID_Alloc : public RID_AllocBase
{
	struct Chunk
	{
		T data;
		uint32_t validator;
	};

	Chunk** chunks = nullptr;
	uint32_t** free_list_chunks = nullptr;

	uint32_t elements_in_chunk;
	uint32_t max_alloc = 0;
	uint32_t alloc_count = 0;
	uint32_t chunk_limit = 0;

	const char* description = nullptr;

	mutable Mutex mutex;

	_FORCE_INLINE_ RID _allocate_rid()
	{
		if constexpr (THREAD_SAFE) {
			mutex.lock();
		}

		if (alloc_count == max_alloc) {
			// allocate a new chunk
			uint32_t chunk_count = alloc_count == 0 ? 0 : (max_alloc / elements_in_chunk);
			if (THREAD_SAFE && chunk_count == chunk_limit) {
				mutex.unlock();
			}

			// grow chunks
			if constexpr (!THREAD_SAFE) {
				chunks = (Chunk**)memrealloc(chunks, sizeof(Chunk*) * (chunk_count + 1));
			}
			chunks[chunk_count] =
				(Chunk*)memalloc(sizeof(Chunk) * elements_in_chunk); // but don't initialize
			// grow free lists
			if constexpr (!THREAD_SAFE) {
				free_list_chunks =
					(uint32_t**)memrealloc(free_list_chunks, sizeof(uint32_t*) * (chunk_count + 1));
			}
			free_list_chunks[chunk_count] =
				(uint32_t*)memalloc(sizeof(uint32_t) * elements_in_chunk);

			// initialize
			for (uint32_t i = 0; i < elements_in_chunk; i++) {
				// Don't initialize chunk.
				chunks[chunk_count][i].validator = 0xFFFFFFFF;
				free_list_chunks[chunk_count][i] = alloc_count + i;
			}

			if constexpr (THREAD_SAFE) {
				// Store atomically to avoid data race with the load in get_or_null().
				((std::atomic<uint32_t>*)&max_alloc)
					->store(max_alloc + elements_in_chunk, std::memory_order_relaxed);
			}
			else {
				max_alloc += elements_in_chunk;
			}
		}

		uint32_t free_index =
			free_list_chunks[alloc_count / elements_in_chunk][alloc_count % elements_in_chunk];

		uint32_t free_chunk = free_index / elements_in_chunk;
		uint32_t free_element = free_index % elements_in_chunk;

		uint32_t validator = 1 + (uint32_t)(_gen_id() % 0x7FFFFFFF);
		uint64_t id = validator;
		id <<= 32;
		id |= free_index;

		chunks[free_chunk][free_element].validator = validator;
		chunks[free_chunk][free_element].validator |= 0x80000000; // mark uninitialized bit

		alloc_count++;

		if constexpr (THREAD_SAFE) {
			mutex.unlock();
		}

		return _make_from_id(id);
	}

public:
	RID make_rid()
	{
		RID rid = _allocate_rid();
		initialize_rid(rid);
		return rid;
	}

	RID make_rid(const T& p_value)
	{
		RID rid = _allocate_rid();
		initialize_rid(rid, p_value);
		return rid;
	}

	// allocate but don't initialize, use initialize_rid afterwards
	RID allocate_rid() { return _allocate_rid(); }

	_FORCE_INLINE_ T* get_or_null(const RID& p_rid, bool p_initialize = false)
	{
		if (p_rid == RID()) {
			return nullptr;
		}

		if constexpr (THREAD_SAFE) {
			SYNC_ACQUIRE;
		}

		uint64_t id = p_rid.get_id();
		uint32_t idx = uint32_t(id & 0xFFFFFFFF);
		uint32_t ma;
		if constexpr (THREAD_SAFE) { // Read atomically to avoid data race with the store in
									 // _allocate_rid().
			ma = ((std::atomic<uint32_t>*)&max_alloc)->load(std::memory_order_relaxed);
		}
		else {
			ma = max_alloc;
		}
		if (unlikely(idx >= ma)) {
			return nullptr;
		}

		uint32_t idx_chunk = idx / elements_in_chunk;
		uint32_t idx_element = idx % elements_in_chunk;

		uint32_t validator = uint32_t(id >> 32);

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_acquire(&chunks[idx_chunk]);				 // We know not a race in practice.
			__tsan_acquire(&chunks[idx_chunk][idx_element]); // We know not a race in practice.
#endif
		}

		Chunk& c = chunks[idx_chunk][idx_element];

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_release(&chunks[idx_chunk]);
			__tsan_release(&chunks[idx_chunk][idx_element]);
			__tsan_acquire(&c.validator); // We know not a race in practice.
#endif
		}

		if (unlikely(p_initialize)) {
			if (unlikely(!(c.validator & 0x80000000))) {
				ERR_FAIL_V_MSG(nullptr, "Initializing already initialized RID");
			}

			if (unlikely((c.validator & 0x7FFFFFFF) != validator)) {
				ERR_FAIL_V_MSG(nullptr, "Attempting to initialize the wrong RID");
			}

			c.validator &= 0x7FFFFFFF; // initialized

		}
		else if (unlikely(c.validator != validator)) {
			if ((c.validator & 0x80000000) && c.validator != 0xFFFFFFFF) {
				ERR_FAIL_V_MSG(nullptr, "Attempting to use an uninitialized RID");
			}
			return nullptr;
		}

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_release(&c.validator);
#endif
		}

		T* ptr = &c.data;

		return ptr;
	}

	void initialize_rid(RID p_rid)
	{
		T* mem = get_or_null(p_rid, true);
		ERR_FAIL_NULL(mem);

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_acquire(mem); // We know not a race in practice.
#endif
		}

		memnew_placement(mem, T);

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_release(mem);
#endif
			SYNC_RELEASE;
		}
	}

	void initialize_rid(RID p_rid, const T& p_value)
	{
		T* mem = get_or_null(p_rid, true);
		ERR_FAIL_NULL(mem);

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_acquire(mem); // We know not a race in practice.
#endif
		}

		memnew_placement(mem, T(p_value));

		if constexpr (THREAD_SAFE) {
#ifdef TSAN_ENABLED
			__tsan_release(mem);
#endif
			SYNC_RELEASE;
		}
	}

	_FORCE_INLINE_ bool owns(const RID& p_rid) const
	{
		if constexpr (THREAD_SAFE) {
			mutex.lock();
		}

		uint64_t id = p_rid.get_id();
		uint32_t idx = uint32_t(id & 0xFFFFFFFF);
		if (unlikely(idx >= max_alloc)) {
			if constexpr (THREAD_SAFE) {
				mutex.unlock();
			}
			return false;
		}

		uint32_t idx_chunk = idx / elements_in_chunk;
		uint32_t idx_element = idx % elements_in_chunk;

		uint32_t validator = uint32_t(id >> 32);

		bool owned = (chunks[idx_chunk][idx_element].validator & 0x7FFFFFFF) == validator;

		if constexpr (THREAD_SAFE) {
			mutex.unlock();
		}

		return owned;
	}

	_FORCE_INLINE_ void free(const RID& p_rid)
	{
		if constexpr (THREAD_SAFE) {
			mutex.lock();
		}

		uint64_t id = p_rid.get_id();
		uint32_t idx = uint32_t(id & 0xFFFFFFFF);
		if (unlikely(idx >= max_alloc)) {
			if constexpr (THREAD_SAFE) {
				mutex.unlock();
			}
			ERR_FAIL();
		}

		uint32_t idx_chunk = idx / elements_in_chunk;
		uint32_t idx_element = idx % elements_in_chunk;

		uint32_t validator = uint32_t(id >> 32);
		if (unlikely(chunks[idx_chunk][idx_element].validator & 0x80000000)) {
			if constexpr (THREAD_SAFE) {
				mutex.unlock();
			}
			ERR_FAIL_MSG("Attempted to free an uninitialized or invalid RID");
		}
		else if (unlikely(chunks[idx_chunk][idx_element].validator != validator)) {
			if constexpr (THREAD_SAFE) {
				mutex.unlock();
			}
			ERR_FAIL();
		}

		chunks[idx_chunk][idx_element].data.~T();
		chunks[idx_chunk][idx_element].validator = 0xFFFFFFFF; // go invalid

		alloc_count--;
		free_list_chunks[alloc_count / elements_in_chunk][alloc_count % elements_in_chunk] = idx;

		if constexpr (THREAD_SAFE) {
			mutex.unlock();
		}
	}

	_FORCE_INLINE_ uint32_t get_rid_count() const { return alloc_count; }

	LocalVector<RID> get_owned_list() const
	{
		LocalVector<RID> owned;
		if constexpr (THREAD_SAFE) {
			mutex.lock();
		}
		for (size_t i = 0; i < max_alloc; i++) {
			uint64_t validator = chunks[i / elements_in_chunk][i % elements_in_chunk].validator;
			if (validator != 0xFFFFFFFF) {
				owned.push_back(_make_from_id((validator << 32) | i));
			}
		}
		if constexpr (THREAD_SAFE) {
			mutex.unlock();
		}
		return owned;
	}

	// used for fast iteration in the elements or RIDs
	void fill_owned_buffer(RID* p_rid_buffer) const
	{
		if constexpr (THREAD_SAFE) {
			mutex.lock();
		}
		uint32_t idx = 0;
		for (size_t i = 0; i < max_alloc; i++) {
			uint64_t validator = chunks[i / elements_in_chunk][i % elements_in_chunk].validator;
			if (validator != 0xFFFFFFFF) {
				p_rid_buffer[idx] = _make_from_id((validator << 32) | i);
				idx++;
			}
		}

		if constexpr (THREAD_SAFE) {
			mutex.unlock();
		}
	}

	void set_description(const char* p_description) { description = p_description; }

	RID_Alloc(
		uint32_t p_target_chunk_byte_size = 65536, uint32_t p_maximum_number_of_elements = 262144)
	{
		elements_in_chunk =
			sizeof(T) > p_target_chunk_byte_size ? 1 : (p_target_chunk_byte_size / sizeof(T));
		if constexpr (THREAD_SAFE) {
			chunk_limit = (p_maximum_number_of_elements / elements_in_chunk) + 1;
			chunks = (Chunk**)memalloc(sizeof(Chunk*) * chunk_limit);
			free_list_chunks = (uint32_t**)memalloc(sizeof(uint32_t*) * chunk_limit);
			SYNC_RELEASE;
		}
	}

	~RID_Alloc()
	{
		if constexpr (THREAD_SAFE) {
			SYNC_ACQUIRE;
		}

		if (alloc_count) {
			for (size_t i = 0; i < max_alloc; i++) {
				uint32_t validator = chunks[i / elements_in_chunk][i % elements_in_chunk].validator;
				if (validator & 0x80000000) {
					continue; // uninitialized
				}
				if (validator != 0xFFFFFFFF) {
					chunks[i / elements_in_chunk][i % elements_in_chunk].data.~T();
				}
			}
		}

		uint32_t chunk_count = max_alloc / elements_in_chunk;
		for (uint32_t i = 0; i < chunk_count; i++) {
			memfree(chunks[i]);
			memfree(free_list_chunks[i]);
		}

		if (chunks) {
			memfree(chunks);
			memfree(free_list_chunks);
		}
	}
};

template <typename T, bool THREAD_SAFE = false> class RID_PtrOwner
{
	RID_Alloc<T*, THREAD_SAFE> alloc;

public:
	_FORCE_INLINE_ RID make_rid(T* p_ptr) { return alloc.make_rid(p_ptr); }

	_FORCE_INLINE_ RID allocate_rid() { return alloc.allocate_rid(); }

	_FORCE_INLINE_ void initialize_rid(RID p_rid, T* p_ptr) { alloc.initialize_rid(p_rid, p_ptr); }

	_FORCE_INLINE_ T* get_or_null(const RID& p_rid)
	{
		T** ptr = alloc.get_or_null(p_rid);
		if (unlikely(!ptr)) {
			return nullptr;
		}
		return *ptr;
	}

	_FORCE_INLINE_ void replace(const RID& p_rid, T* p_new_ptr)
	{
		T** ptr = alloc.get_or_null(p_rid);
		ERR_FAIL_NULL(ptr);
		*ptr = p_new_ptr;
	}

	_FORCE_INLINE_ bool owns(const RID& p_rid) const { return alloc.owns(p_rid); }

	_FORCE_INLINE_ void free(const RID& p_rid) { alloc.free(p_rid); }

	_FORCE_INLINE_ uint32_t get_rid_count() const { return alloc.get_rid_count(); }

	_FORCE_INLINE_ LocalVector<RID> get_owned_list() const { return alloc.get_owned_list(); }

	void fill_owned_buffer(RID* p_rid_buffer) const { alloc.fill_owned_buffer(p_rid_buffer); }

	void set_description(const char* p_description) { alloc.set_description(p_description); }

	RID_PtrOwner(
		uint32_t p_target_chunk_byte_size = 65536, uint32_t p_maximum_number_of_elements = 262144)
		: alloc(p_target_chunk_byte_size, p_maximum_number_of_elements)
	{
	}
};

template <typename T, bool THREAD_SAFE = false> class RID_Owner
{
	RID_Alloc<T, THREAD_SAFE> alloc;

public:
	_FORCE_INLINE_ RID make_rid() { return alloc.make_rid(); }

	_FORCE_INLINE_ RID make_rid(const T& p_ptr) { return alloc.make_rid(p_ptr); }

	_FORCE_INLINE_ RID allocate_rid() { return alloc.allocate_rid(); }

	_FORCE_INLINE_ void initialize_rid(RID p_rid) { alloc.initialize_rid(p_rid); }

	_FORCE_INLINE_ void initialize_rid(RID p_rid, const T& p_ptr)
	{
		alloc.initialize_rid(p_rid, p_ptr);
	}

	_FORCE_INLINE_ T* get_or_null(const RID& p_rid) { return alloc.get_or_null(p_rid); }

	_FORCE_INLINE_ bool owns(const RID& p_rid) const { return alloc.owns(p_rid); }

	_FORCE_INLINE_ void free(const RID& p_rid) { alloc.free(p_rid); }

	_FORCE_INLINE_ uint32_t get_rid_count() const { return alloc.get_rid_count(); }

	_FORCE_INLINE_ LocalVector<RID> get_owned_list() const { return alloc.get_owned_list(); }

	void fill_owned_buffer(RID* p_rid_buffer) const { alloc.fill_owned_buffer(p_rid_buffer); }

	void set_description(const char* p_description) { alloc.set_description(p_description); }

	RID_Owner(
		uint32_t p_target_chunk_byte_size = 65536, uint32_t p_maximum_number_of_elements = 262144)
		: alloc(p_target_chunk_byte_size, p_maximum_number_of_elements)
	{
	}
};
// --- End core/templates/rid_owner.h ---

// --- Folded from core/io/resource_uid.h ---

#include "core/string/ustring.h"


class FileAccess;

typedef void (*ResourceUIDScanForUIDOnStartup)();

class ResourceUID final
{
public:
	typedef int64_t ID;
	constexpr const static ID INVALID_ID = -1;

	static String get_cache_file();

private:
	struct Cache
	{
		CharString cs;
		bool saved_to_cache = false;
	};

	struct Data
	{
		Mutex mutex;
		HashMap<ID, Cache> unique_ids; // Unique IDs and utf8 paths (less memory used).
		bool use_reverse_cache = false;
		HashMap<CharString, ID> reverse_cache; // Used at runtime.
		uint32_t cache_entries = 0;
		bool changed = false;
		bool cache_initialized = false;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	inline static ResourceUIDScanForUIDOnStartup scan_for_uid_on_startup = nullptr;

	static String id_to_text(ID p_id);
	static ID text_to_id(const String& p_text);

	static ID create_id();
	static ID create_id_for_path(const String& p_path);
	static bool has_id(ID p_id);
	static void add_id(ID p_id, const String& p_path);
	static void set_id(ID p_id, const String& p_path);
	static String get_id_path(ID p_id);
	static ID get_path_id(const String& p_path);
	static void remove_id(ID p_id);

	static String uid_to_path(const String& p_uid);
	static String path_to_uid(const String& p_path);
	static String ensure_path(const String& p_uid_or_path);

	static Error load_from_cache(bool p_reset);
	static Error save_to_cache();
	static Error update_cache();
	static String get_path_from_cache(Ref<FileAccess>& p_cache_file, const String& p_uid_string);
	static Vector<uint8_t> encode_binary_cache(const Vector<Pair<ID, String>>& p_entries);

	static void enable_reverse_cache()
	{
		if (data) {
			data->use_reverse_cache = true;
		}
	}

	static void clear();

	ResourceUID() = delete;
	ResourceUID(const ResourceUID&) = delete;
	ResourceUID& operator=(const ResourceUID&) = delete;
	~ResourceUID() = delete;
};
// --- End core/io/resource_uid.h ---

// --- Folded from core/io/resource.h ---



class Node;
class RWLock;

#define RES_BASE_EXTENSION(m_ext)                                                                  \
public:                                                                                            \
	static void register_custom_data_to_otdb()                                                     \
	{                                                                                              \
		Resource::_add_resource_base_extension_to_classdb(m_ext, get_class_static());              \
	}                                                                                              \
                                                                                                   \
	virtual String get_base_extension() const override                                             \
	{                                                                                              \
		return m_ext;                                                                              \
	}                                                                                              \
                                                                                                   \
private:

class Resource : public RefCounted
{
public:
	static void register_custom_data_to_otdb();

	virtual String get_base_extension() const { return "res"; }

protected:
	static void _add_resource_base_extension_to_classdb(
		const String& p_extension, const String& p_class);

private:
	friend class ResourceCache;

	String name;
	String path_cache;
	String scene_unique_id;

#ifdef TOOLS_ENABLED
	uint64_t last_modified_time = 0;
	uint64_t import_last_modified_time = 0;
	String import_path;
#endif

	enum EmitChangedState
	{
		EMIT_CHANGED_UNBLOCKED,
		EMIT_CHANGED_BLOCKED,
		EMIT_CHANGED_BLOCKED_PENDING_EMIT,
	};

	EmitChangedState emit_changed_state = EMIT_CHANGED_UNBLOCKED;
	bool local_to_scene = false;
	friend class SceneState;
	Node* local_scene = nullptr;

	SelfList<Resource> remapped_list = { this };

	using DuplicateRemapCacheT = HashMap<Ref<Resource>, Ref<Resource>>;
	static thread_local inline DuplicateRemapCacheT* thread_duplicate_remap_cache = nullptr;
	static thread_local inline bool thread_duplicate_remap_cache_needs_deallocation = true;

protected:
	virtual void _resource_path_changed();

	void _block_emit_changed();
	void _unblock_emit_changed();

	void _set_path(const String& p_path);
	void _take_over_path(const String& p_path);

	virtual void reset_local_to_scene();

	virtual Ref<Resource> _duplicate(bool p_subresources = false) const;
	virtual String _to_string();

public:
	Ref<Resource> duplicate(bool p_deep) const;
	static Node* (*_get_local_scene_func)();		// Used by the editor.
	static void (*_update_configuration_warning)(); // Used by the editor.

	void update_configuration_warning();
	virtual bool editor_can_reload_from_file();
	virtual void reset_state(); // For resources that store state in non-exposed properties, such as
								// via _validate_property or _get_property_list, this function must
								// be implemented to clear them.
	virtual Error copy_from(const Ref<Resource>& p_resource);
	virtual void reload_from_file();

	void emit_changed();
	void set_name(const String& p_name);
	String get_name() const;

	virtual void set_path(const String& p_path, bool p_take_over = false);
	String get_path() const;
	virtual void set_path_cache(
		const String& p_path); // Set raw path without involving resource cache.

	_FORCE_INLINE_ bool is_built_in() const
	{
		return path_cache.is_empty() || path_cache.contains("::") ||
			   path_cache.begins_with("local://");
	}

	static void seed_scene_unique_id(uint32_t p_seed);
	static String generate_scene_unique_id();
	void set_scene_unique_id(const String& p_id);
	String get_scene_unique_id() const;

	void configure_for_local_scene(
		Node* p_for_scene, HashMap<Ref<Resource>, Ref<Resource>>& p_remap_cache);

	void set_local_to_scene(bool p_enable);
	bool is_local_to_scene() const;
	virtual void setup_local_to_scene();

	Node* get_local_scene() const;

	void set_as_translation_remapped(bool p_remapped);

	virtual RID get_rid() const; // Some resources may offer conversion to RID.

	// Helps keep IDs the same when loading/saving scenes. An empty ID clears the entry, and an
	// empty ID is returned when not found.
	static void set_resource_id_for_path(
		const String& p_referrer_path, const String& p_resource_path, const String& p_id);

	void set_id_for_path(const String& p_referrer_path, const String& p_id)
	{
		set_resource_id_for_path(p_referrer_path, get_path(), p_id);
	}

	String get_id_for_path(const String& p_referrer_path) const;

	Resource() = default;
	~Resource();
};

class ResourceCache
{
	friend class Resource;
	friend class ResourceLoader; // Need the lock.
	static Mutex lock;
	static HashMap<String, Resource*> resources;
#ifdef TOOLS_ENABLED
	static HashMap<String, HashMap<String, String>>
		resource_path_cache; // Each tscn has a set of resource paths and IDs.
	static RWLock path_cache_lock;
#endif // TOOLS_ENABLED
	friend void unregister_core_types();
	static void clear();
	friend void register_core_types();

public:
	static bool has(const String& p_path);
	static Ref<Resource> get_ref(const String& p_path);
	static void get_cached_resources(List<Ref<Resource>>* p_resources);
	static int get_cached_resource_count();
};

// --- Folded from scene/property_list_helper.h ---
class PropertyListHelper
{
	static HashMap<String, Vector<PropertyListHelper*>> base_helpers;

	String prefix;

	bool allow_oob_assign = false;

	int _call_array_length_getter() const;

public:
	static void clear_base_helpers();
	static void register_base_helper(const String& p_class_name, PropertyListHelper* p_helper);

	static Vector<PropertyListHelper*> get_helpers_for_class(const String& p_class_name);

	void set_prefix(const String& p_prefix);

	bool is_initialized() const;
	bool is_property_valid(const String& p_property, int* r_index = nullptr) const;

	bool property_can_revert(const String& p_property) const;

	void enable_out_of_bounds_assign() { allow_oob_assign = true; }

	void clear();
};
// --- End scene/property_list_helper.h ---
// --- End core/io/resource.h ---
