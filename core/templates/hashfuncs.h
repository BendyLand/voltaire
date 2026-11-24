/**************************************************************************/
/*  hashfuncs.h                                                           */
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

#include "core/typedefs.h"

#ifdef _MSC_VER
#include <intrin.h> // Needed for `__umulh` below.
#endif

template <typename F, typename S> struct Pair;

/**
 * Hashing functions
 */

/**
 * DJB2 Hash function
 * @param C String
 * @return 32-bits hashcode
 */
_FORCE_INLINE_ uint32_t hash_djb2(const char* p_cstr)
{
	const unsigned char* chr = (const unsigned char*)p_cstr;
	uint32_t hash = 5381;
	uint32_t c = *chr++;

	while (c) {
		hash = ((hash << 5) + hash) ^ c; /* hash * 33 ^ c */
		c = *chr++;
	}

	return hash;
}

_FORCE_INLINE_ uint32_t hash_djb2_buffer(const uint8_t* p_buff, int p_len, uint32_t p_prev = 5381)
{
	uint32_t hash = p_prev;

	for (int i = 0; i < p_len; i++) {
		hash = ((hash << 5) + hash) ^ p_buff[i]; /* hash * 33 + c */
	}

	return hash;
}

_FORCE_INLINE_ uint32_t hash_djb2_one_32(uint32_t p_in, uint32_t p_prev = 5381)
{
	return ((p_prev << 5) + p_prev) ^ p_in;
}

/**
 * Thomas Wang's 64-bit to 32-bit Hash function:
 * https://web.archive.org/web/20071223173210/https:/www.concentric.net/~Ttwang/tech/inthash.htm
 *
 * @param p_int - 64-bit unsigned integer key to be hashed
 * @return unsigned 32-bit value representing hashcode
 */
_FORCE_INLINE_ uint32_t hash_one_uint64(const uint64_t p_int)
{
	uint64_t v = p_int;
	v = (~v) + (v << 18); // v = (v << 18) - v - 1;
	v = v ^ (v >> 31);
	v = v * 21; // v = (v + (v << 2)) + (v << 4);
	v = v ^ (v >> 11);
	v = v + (v << 6);
	v = v ^ (v >> 22);
	return uint32_t(v);
}

_FORCE_INLINE_ uint64_t hash64_murmur3_64(uint64_t p_key, uint64_t p_seed)
{
	p_key ^= p_seed;
	p_key ^= p_key >> 33;
	p_key *= 0xff51afd7ed558ccd;
	p_key ^= p_key >> 33;
	p_key *= 0xc4ceb9fe1a85ec53;
	p_key ^= p_key >> 33;
	return p_key;
}

#define HASH_MURMUR3_SEED 0x7F07C65

// Murmurhash3 32-bit version.
// All MurmurHash versions are public domain software, and the author disclaims all copyright to
// their code.

_FORCE_INLINE_ uint32_t hash_murmur3_one_32(uint32_t p_in, uint32_t p_seed = HASH_MURMUR3_SEED)
{
	p_in *= 0xcc9e2d51;
	p_in = (p_in << 15) | (p_in >> 17);
	p_in *= 0x1b873593;

	p_seed ^= p_in;
	p_seed = (p_seed << 13) | (p_seed >> 19);
	p_seed = p_seed * 5 + 0xe6546b64;

	return p_seed;
}

_FORCE_INLINE_ uint32_t hash_murmur3_one_64(uint64_t p_in, uint32_t p_seed = HASH_MURMUR3_SEED)
{
	p_seed = hash_murmur3_one_32(p_in & 0xFFFFFFFF, p_seed);
	return hash_murmur3_one_32(p_in >> 32, p_seed);
}

uint32_t hash_murmur3_one_float(float p_in, uint32_t p_seed = HASH_MURMUR3_SEED);
uint32_t hash_murmur3_one_double(double p_in, uint32_t p_seed = HASH_MURMUR3_SEED);

_FORCE_INLINE_ uint32_t hash_murmur3_one_real(real_t p_in, uint32_t p_seed = HASH_MURMUR3_SEED)
{
#ifdef REAL_T_IS_DOUBLE
	return hash_murmur3_one_double(p_in, p_seed);
#else
	return hash_murmur3_one_float(p_in, p_seed);
#endif
}

_FORCE_INLINE_ uint32_t hash_rotl32(uint32_t p_x, int8_t p_r)
{
	return (p_x << p_r) | (p_x >> (32 - p_r));
}

_FORCE_INLINE_ uint32_t hash_fmix32(uint32_t p_h)
{
	p_h ^= p_h >> 16;
	p_h *= 0x85ebca6b;
	p_h ^= p_h >> 13;
	p_h *= 0xc2b2ae35;
	p_h ^= p_h >> 16;

	return p_h;
}

uint32_t hash_murmur3_buffer(const void* p_key, int p_length, uint32_t p_seed = HASH_MURMUR3_SEED);

uint32_t hash_djb2_one_float(double p_in, uint32_t p_prev = 5381);
uint64_t hash_djb2_one_float_64(double p_in, uint64_t p_prev = 5381);

_FORCE_INLINE_ uint64_t hash_djb2_one_64(uint64_t p_in, uint64_t p_prev = 5381)
{
	return ((p_prev << 5) + p_prev) ^ p_in;
}

template <typename, typename = std::void_t<>> struct has_hash_method : std::false_type
{
};

template <typename T>
struct has_hash_method<T,
	std::void_t<std::is_same<decltype(std::declval<const T>().hash()), uint32_t>>> : std::true_type
{
};

template <typename T> constexpr bool has_hash_method_v = has_hash_method<T>::value;

template <typename T, typename = void> struct HashMapHasherDefaultImpl
{
};

struct HashMapHasherDefault
{
	template <typename T> static _FORCE_INLINE_ uint32_t hash(const T& p_type)
	{
		return HashMapHasherDefaultImpl<std::decay_t<T>>::hash(p_type);
	}
};

template <typename T> struct HashMapHasherDefaultImpl<T, std::enable_if_t<has_hash_method_v<T>>>
{
	// For self hashing types.
	static _FORCE_INLINE_ uint32_t hash(const T& p_value) { return p_value.hash(); }
};

template <typename T> struct HashMapHasherDefaultImpl<T*>
{
	// For pointer types.
	static _FORCE_INLINE_ uint32_t hash(const T* p_pointer)
	{
		return hash_one_uint64((uint64_t)p_pointer);
	}
};

template <typename T> struct HashMapHasherDefaultImpl<T, std::enable_if_t<std::is_enum_v<T>>>
{
	// For all enums.
	static _FORCE_INLINE_ uint32_t hash(T p_value)
	{
		return HashMapHasherDefaultImpl<std::underlying_type_t<T>>::hash(
			static_cast<std::underlying_type_t<T>>(p_value));
	}
};

template <> struct HashMapHasherDefaultImpl<char*>
{
	static _FORCE_INLINE_ uint32_t hash(const char* p_cstr) { return hash_djb2(p_cstr); }
};

template <> struct HashMapHasherDefaultImpl<wchar_t>
{
	static _FORCE_INLINE_ uint32_t hash(const wchar_t p_wchar)
	{
		return hash_fmix32(uint32_t(p_wchar));
	}
};

template <> struct HashMapHasherDefaultImpl<char16_t>
{
	static _FORCE_INLINE_ uint32_t hash(const char16_t p_uchar)
	{
		return hash_fmix32(uint32_t(p_uchar));
	}
};

template <> struct HashMapHasherDefaultImpl<char32_t>
{
	static _FORCE_INLINE_ uint32_t hash(const char32_t p_uchar)
	{
		return hash_fmix32(uint32_t(p_uchar));
	}
};

template <> struct HashMapHasherDefaultImpl<uint64_t>
{
	static _FORCE_INLINE_ uint32_t hash(const uint64_t p_int) { return hash_one_uint64(p_int); }
};

template <> struct HashMapHasherDefaultImpl<int64_t>
{
	static _FORCE_INLINE_ uint32_t hash(const int64_t p_int)
	{
		return hash_one_uint64(uint64_t(p_int));
	}
};

template <> struct HashMapHasherDefaultImpl<float>
{
	static _FORCE_INLINE_ uint32_t hash(const float p_float)
	{
		return hash_murmur3_one_float(p_float);
	}
};

template <> struct HashMapHasherDefaultImpl<double>
{
	static _FORCE_INLINE_ uint32_t hash(const double p_double)
	{
		return hash_murmur3_one_double(p_double);
	}
};

template <> struct HashMapHasherDefaultImpl<uint32_t>
{
	static _FORCE_INLINE_ uint32_t hash(const uint32_t p_int) { return hash_fmix32(p_int); }
};

template <> struct HashMapHasherDefaultImpl<int32_t>
{
	static _FORCE_INLINE_ uint32_t hash(const int32_t p_int)
	{
		return hash_fmix32(uint32_t(p_int));
	}
};

template <> struct HashMapHasherDefaultImpl<uint16_t>
{
	static _FORCE_INLINE_ uint32_t hash(const uint16_t p_int)
	{
		return hash_fmix32(uint32_t(p_int));
	}
};

template <> struct HashMapHasherDefaultImpl<int16_t>
{
	static _FORCE_INLINE_ uint32_t hash(const int16_t p_int)
	{
		return hash_fmix32(uint32_t(p_int));
	}
};

template <> struct HashMapHasherDefaultImpl<uint8_t>
{
	static _FORCE_INLINE_ uint32_t hash(const uint8_t p_int)
	{
		return hash_fmix32(uint32_t(p_int));
	}
};

template <> struct HashMapHasherDefaultImpl<int8_t>
{
	static _FORCE_INLINE_ uint32_t hash(const int8_t p_int) { return hash_fmix32(uint32_t(p_int)); }
};

template <typename F, typename S> struct HashMapHasherDefaultImpl<Pair<F, S>>
{
	static _FORCE_INLINE_ uint32_t hash(const Pair<F, S>& p_pair)
	{
		uint64_t h1 = HashMapHasherDefault::hash(p_pair.first);
		uint64_t h2 = HashMapHasherDefault::hash(p_pair.second);
		return hash_one_uint64((h1 << 32) | h2);
	}
};

template <typename, typename = std::void_t<>> struct has_is_same_method : std::false_type
{
};

template <typename T>
struct has_is_same_method<T,
	std::void_t<
		std::is_same<decltype(std::declval<const T>().is_same(std::declval<const T>())), uint32_t>>>
	: std::true_type
{
};

template <typename T> constexpr bool has_is_same_method_v = has_is_same_method<T>::value;

struct HashHasher
{
	static _FORCE_INLINE_ uint32_t hash(const int32_t p_hash) { return p_hash; }

	static _FORCE_INLINE_ uint32_t hash(const uint32_t p_hash) { return p_hash; }

	static _FORCE_INLINE_ uint64_t hash(const int64_t p_hash) { return p_hash; }

	static _FORCE_INLINE_ uint64_t hash(const uint64_t p_hash) { return p_hash; }
};

template <typename T, typename = void> struct HashMapComparatorDefault
{
	static bool compare(const T& p_lhs, const T& p_rhs) { return p_lhs == p_rhs; }
};

template <> struct HashMapComparatorDefault<float>
{
	static bool compare(const float& p_lhs, const float& p_rhs)
	{
		return Math::is_same(p_lhs, p_rhs);
	}
};

template <> struct HashMapComparatorDefault<double>
{
	static bool compare(const double& p_lhs, const double& p_rhs)
	{
		return Math::is_same(p_lhs, p_rhs);
	}
};

template <typename T> struct HashMapComparatorDefault<T, std::enable_if_t<has_is_same_method_v<T>>>
{
	// For self comparing types.
	static bool compare(const T& p_lhs, const T& p_rhs) { return p_lhs.is_same(p_rhs); }
};

constexpr uint32_t HASH_TABLE_SIZE_MAX = 29;

inline constexpr uint32_t hash_table_size_primes[HASH_TABLE_SIZE_MAX] = {
	5,
	13,
	23,
	47,
	97,
	193,
	389,
	769,
	1543,
	3079,
	6151,
	12289,
	24593,
	49157,
	98317,
	196613,
	393241,
	786433,
	1572869,
	3145739,
	6291469,
	12582917,
	25165843,
	50331653,
	100663319,
	201326611,
	402653189,
	805306457,
	1610612741,
};

// Computed with elem_i = UINT64_C (0 x FFFFFFFF FFFFFFFF ) / d_i + 1, where d_i is the i-th element
// of the above array.
inline constexpr uint64_t hash_table_size_primes_inv[HASH_TABLE_SIZE_MAX] = {3689348814741910324,
	1418980313362273202, 802032351030850071, 392483916461905354, 190172619316593316,
	95578984837873325, 47420935922132524, 23987963684927896, 11955116055547344, 5991147799191151,
	2998982941588287, 1501077717772769, 750081082979285, 375261795343686, 187625172388393,
	93822606204624, 46909513691883, 23456218233098, 11728086747027, 5864041509391, 2932024948977,
	1466014921160, 733007198436, 366503839517, 183251896093, 91625960335, 45812983922, 22906489714,
	11453246088};

/**
 * Fastmod computes ( n mod d ) given the precomputed c much faster than n % d.
 * The implementation of fastmod is based on the following paper by Daniel Lemire et al.
 * Faster Remainder by Direct Computation: Appl
ications to Compilers and Software Libraries
 * https://arxiv.org/abs/1902.01961
 */
static _FORCE_INLINE_ uint32_t fastmod(const uint32_t p_n, const uint64_t p_c, const uint32_t p_d)
{
#if defined(_MSC_VER)
	// Returns the upper 64 bits of the product of two 64-bit unsigned integers.
	// This intrinsic function is required since MSVC does not support unsigned 128-bit integers.
#if defined(_M_X64) || defined(_M_ARM64)
	return __umulh(p_c * p_n, p_d);
#else
	// Fallback to the slower method for 32-bit platforms.
	return p_n % p_d;
#endif // _M_X64 || _M_ARM64
#else
#ifdef __SIZEOF_INT128__
	// Prevent compiler warning, because we know what we are doing.
	uint64_t lowbits = p_c * p_n;
	__extension__ typedef unsigned __int128 uint128;
	return static_cast<uint64_t>(((uint128)lowbits * p_d) >> 64);
#else
	// Fallback to the slower method if no 128-bit unsigned integer type is available.
	return p_n % p_d;
#endif // __SIZEOF_INT128__
#endif // _MSC_VER
}

// --- Folded from core/templates/rid.h ---

#include "core/typedefs.h"

class RID_AllocBase;

class RID
{
	friend class RID_AllocBase;
	uint64_t _id = 0;

public:
	_ALWAYS_INLINE_ bool operator==(const RID& p_rid) const { return _id == p_rid._id; }

	_ALWAYS_INLINE_ bool operator<(const RID& p_rid) const { return _id < p_rid._id; }

	_ALWAYS_INLINE_ bool operator<=(const RID& p_rid) const { return _id <= p_rid._id; }

	_ALWAYS_INLINE_ bool operator>(const RID& p_rid) const { return _id > p_rid._id; }

	_ALWAYS_INLINE_ bool operator>=(const RID& p_rid) const { return _id >= p_rid._id; }

	_ALWAYS_INLINE_ bool operator!=(const RID& p_rid) const { return _id != p_rid._id; }

	_ALWAYS_INLINE_ bool is_valid() const { return _id != 0; }

	_ALWAYS_INLINE_ bool is_null() const { return _id == 0; }

	_ALWAYS_INLINE_ uint32_t get_local_index() const { return _id & 0xFFFFFFFF; }

	static _ALWAYS_INLINE_ RID from_uint64(uint64_t p_id)
	{
		RID _rid;
		_rid._id = p_id;
		return _rid;
	}

	_ALWAYS_INLINE_ uint64_t get_id() const { return _id; }

	uint32_t hash() const { return HashMapHasherDefault::hash(_id); }

	RID() = default;
	~RID() = default;
};

template <> struct is_zero_constructible<RID> : std::true_type
{
};
// --- End core/templates/rid.h ---

// --- Folded from core/math/color.h ---
#include "core/typedefs.h"


class String;

struct [[nodiscard]] Color {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;

	uint32_t to_rgba32() const;
	uint32_t to_argb32() const;
	uint32_t to_abgr32() const;
	uint64_t to_rgba64() const;
	uint64_t to_argb64() const;
	uint64_t to_abgr64() const;
	String to_html(bool p_alpha = true) const;
	float get_h() const;
	float get_s() const;
	float get_v() const;
	void set_hsv(float p_h, float p_s, float p_v, float p_alpha = 1.0f);
	float get_ok_hsl_h() const;
	float get_ok_hsl_s() const;
	float get_ok_hsl_l() const;
	void set_ok_hsl(float p_h, float p_s, float p_l, float p_alpha = 1.0f);
	void set_ok_hsv(float p_h, float p_s, float p_v, float p_alpha = 1.0f);

	constexpr float &operator[](int p_idx) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Color, r) == 0 * sizeof(float));
		static_assert(offsetof(Color, g) == 1 * sizeof(float));
		static_assert(offsetof(Color, b) == 2 * sizeof(float));
		static_assert(offsetof(Color, a) == 3 * sizeof(float));
		static_assert(sizeof(Color) == 4 * sizeof(float));

		DEV_ASSERT((unsigned int)p_idx < 4);
		return (&r)[p_idx];
	}
	constexpr const float &operator[](int p_idx) const {
		DEV_ASSERT((unsigned int)p_idx < 4);
		return (&r)[p_idx];
	}

	constexpr const float *as_float4_buffer() const { return &r; }

	constexpr bool operator==(const Color &p_color) const {
		return (r == p_color.r && g == p_color.g && b == p_color.b && a == p_color.a);
	}
	constexpr bool operator!=(const Color &p_color) const {
		return (r != p_color.r || g != p_color.g || b != p_color.b || a != p_color.a);
	}

	constexpr Color operator+(const Color &p_color) const;
	constexpr void operator+=(const Color &p_color);

	constexpr Color operator-() const;
	constexpr Color operator-(const Color &p_color) const;
	constexpr void operator-=(const Color &p_color);

	constexpr Color operator*(const Color &p_color) const;
	constexpr Color operator*(float p_scalar) const;
	constexpr void operator*=(const Color &p_color);
	constexpr void operator*=(float p_scalar);

	constexpr Color operator/(const Color &p_color) const;
	constexpr Color operator/(float p_scalar) const;
	constexpr void operator/=(const Color &p_color);
	constexpr void operator/=(float p_scalar);

	bool is_equal_approx(const Color &p_color) const;
	bool is_same(const Color &p_color) const;

	Color clamp(const Color &p_min = Color(0, 0, 0, 0), const Color &p_max = Color(1, 1, 1, 1)) const;
	void invert();
	Color inverted() const;

	_FORCE_INLINE_ float get_luminance() const {
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	_FORCE_INLINE_ Color lerp(const Color &p_to, float p_weight) const {
		Color res = *this;
		res.r = Math::lerp(res.r, p_to.r, p_weight);
		res.g = Math::lerp(res.g, p_to.g, p_weight);
		res.b = Math::lerp(res.b, p_to.b, p_weight);
		res.a = Math::lerp(res.a, p_to.a, p_weight);
		return res;
	}

	_FORCE_INLINE_ Color darkened(float p_amount) const {
		Color res = *this;
		res.r = res.r * (1.0f - p_amount);
		res.g = res.g * (1.0f - p_amount);
		res.b = res.b * (1.0f - p_amount);
		return res;
	}

	_FORCE_INLINE_ Color lightened(float p_amount) const {
		Color res = *this;
		res.r = res.r + (1.0f - res.r) * p_amount;
		res.g = res.g + (1.0f - res.g) * p_amount;
		res.b = res.b + (1.0f - res.b) * p_amount;
		return res;
	}

	_FORCE_INLINE_ uint32_t to_rgbe9995() const {
		// https://github.com/microsoft/DirectX-Graphics-Samples/blob/v10.0.19041.0/MiniEngine/Core/Color.cpp
		static const float kMaxVal = float(0x1FF << 7);
		static const float kMinVal = float(1.f / (1 << 16));

		// Clamp RGB to [0, 1.FF*2^16]
		const float _r = CLAMP(r, 0.0f, kMaxVal);
		const float _g = CLAMP(g, 0.0f, kMaxVal);
		const float _b = CLAMP(b, 0.0f, kMaxVal);

		// Compute the maximum channel, no less than 1.0*2^-15
		const float MaxChannel = MAX(MAX(_r, _g), MAX(_b, kMinVal));

		// Take the exponent of the maximum channel (rounding up the 9th bit) and
		// add 15 to it.  When added to the channels, it causes the implicit '1.0'
		// bit and the first 8 mantissa bits to be shifted down to the low 9 bits
		// of the mantissa, rounding the truncated bits.
		union {
			float f;
			uint32_t i;
		} R, G, B, E;

		E.f = MaxChannel;
		E.i += 0x07804000; // Add 15 to the exponent and 0x4000 to the mantissa
		E.i &= 0x7F800000; // Zero the mantissa

		// This shifts the 9-bit values we need into the lowest bits, rounding as
		// needed. Note that if the channel has a smaller exponent than the max
		// channel, it will shift even more.  This is intentional.
		R.f = _r + E.f;
		G.f = _g + E.f;
		B.f = _b + E.f;

		// Convert the Bias to the correct exponent in the upper 5 bits.
		E.i <<= 4;
		E.i += 0x10000000;

		// Combine the fields. RGB floats have unwanted data in the upper 9
		// bits. Only red needs to mask them off because green and blue shift
		// it out to the left.
		return E.i | (B.i << 18U) | (G.i << 9U) | (R.i & 511U);
	}

	_FORCE_INLINE_ Color blend(const Color &p_over) const {
		Color res;
		float sa = 1.0f - p_over.a;
		res.a = a * sa + p_over.a;
		if (res.a == 0) {
			return Color(0, 0, 0, 0);
		} else {
			res.r = (r * a * sa + p_over.r * p_over.a) / res.a;
			res.g = (g * a * sa + p_over.g * p_over.a) / res.a;
			res.b = (b * a * sa + p_over.b * p_over.a) / res.a;
		}
		return res;
	}

	_FORCE_INLINE_ Color srgb_to_linear() const {
		return Color(
				r < 0.04045f ? r * (1.0f / 12.92f) : Math::pow(float((r + 0.055) * (1.0 / (1.0 + 0.055))), 2.4f),
				g < 0.04045f ? g * (1.0f / 12.92f) : Math::pow(float((g + 0.055) * (1.0 / (1.0 + 0.055))), 2.4f),
				b < 0.04045f ? b * (1.0f / 12.92f) : Math::pow(float((b + 0.055) * (1.0 / (1.0 + 0.055))), 2.4f),
				a);
	}
	_FORCE_INLINE_ Color linear_to_srgb() const {
		return Color(
				r < 0.0031308f ? 12.92f * r : (1.0 + 0.055) * Math::pow(r, 1.0f / 2.4f) - 0.055,
				g < 0.0031308f ? 12.92f * g : (1.0 + 0.055) * Math::pow(g, 1.0f / 2.4f) - 0.055,
				b < 0.0031308f ? 12.92f * b : (1.0 + 0.055) * Math::pow(b, 1.0f / 2.4f) - 0.055, a);
	}

	static Color hex(uint32_t p_hex);
	static Color hex64(uint64_t p_hex);
	static Color html(const String &p_rgba);
	static bool html_is_valid(const String &p_color);
	static Color named(const String &p_name);
	static Color named(const String &p_name, const Color &p_default);
	static int find_named_color(const String &p_name);
	static int get_named_color_count();
	static String get_named_color_name(int p_idx);
	static Color get_named_color(int p_idx);
	static Color from_string(const String &p_string, const Color &p_default);
	static Color from_hsv(float p_h, float p_s, float p_v, float p_alpha = 1.0f);
	static Color from_ok_hsl(float p_h, float p_s, float p_l, float p_alpha = 1.0f);
	static Color from_ok_hsv(float p_h, float p_s, float p_l, float p_alpha = 1.0f);
	static Color from_rgbe9995(uint32_t p_rgbe);
	static Color from_rgba8(int64_t p_r8, int64_t p_g8, int64_t p_b8, int64_t p_a8 = 255);

	constexpr bool operator<(const Color &p_color) const; // Used in set keys.
	explicit operator String() const;

	// For the binder.
	_FORCE_INLINE_ void set_r8(int32_t p_r8) { r = (CLAMP(p_r8, 0, 255) / 255.0f); }
	_FORCE_INLINE_ int32_t get_r8() const { return int32_t(CLAMP(Math::round(r * 255.0f), 0.0f, 255.0f)); }
	_FORCE_INLINE_ void set_g8(int32_t p_g8) { g = (CLAMP(p_g8, 0, 255) / 255.0f); }
	_FORCE_INLINE_ int32_t get_g8() const { return int32_t(CLAMP(Math::round(g * 255.0f), 0.0f, 255.0f)); }
	_FORCE_INLINE_ void set_b8(int32_t p_b8) { b = (CLAMP(p_b8, 0, 255) / 255.0f); }
	_FORCE_INLINE_ int32_t get_b8() const { return int32_t(CLAMP(Math::round(b * 255.0f), 0.0f, 255.0f)); }
	_FORCE_INLINE_ void set_a8(int32_t p_a8) { a = (CLAMP(p_a8, 0, 255) / 255.0f); }
	_FORCE_INLINE_ int32_t get_a8() const { return int32_t(CLAMP(Math::round(a * 255.0f), 0.0f, 255.0f)); }

	_FORCE_INLINE_ void set_h(float p_h) { set_hsv(p_h, get_s(), get_v(), a); }
	_FORCE_INLINE_ void set_s(float p_s) { set_hsv(get_h(), p_s, get_v(), a); }
	_FORCE_INLINE_ void set_v(float p_v) { set_hsv(get_h(), get_s(), p_v, a); }
	_FORCE_INLINE_ void set_ok_hsl_h(float p_h) { set_ok_hsl(p_h, get_ok_hsl_s(), get_ok_hsl_l(), a); }
	_FORCE_INLINE_ void set_ok_hsl_s(float p_s) { set_ok_hsl(get_ok_hsl_h(), p_s, get_ok_hsl_l(), a); }
	_FORCE_INLINE_ void set_ok_hsl_l(float p_l) { set_ok_hsl(get_ok_hsl_h(), get_ok_hsl_s(), p_l, a); }

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_float(r);
		h = hash_murmur3_one_float(g, h);
		h = hash_murmur3_one_float(b, h);
		h = hash_murmur3_one_float(a, h);
		return hash_fmix32(h);
	}

	constexpr Color() = default;

	/**
	 * RGBA construct parameters.
	 * Alpha is not optional as otherwise we can't bind the RGB version for scripting.
	 */
	constexpr Color(float p_r, float p_g, float p_b, float p_a) :
			r(p_r), g(p_g), b(p_b), a(p_a) {}

	/**
	 * RGB construct parameters.
	 */
	constexpr Color(float p_r, float p_g, float p_b) :
			r(p_r), g(p_g), b(p_b), a(1) {}

	/**
	 * Construct a Color from another Color, but with the specified alpha value.
	 */
	constexpr Color(const Color &p_c, float p_a) :
			r(p_c.r), g(p_c.g), b(p_c.b), a(p_a) {}

	Color(const String &p_code) {
		if (html_is_valid(p_code)) {
			*this = html(p_code);
		} else {
			*this = named(p_code);
		}
	}

	Color(const String &p_code, float p_a) {
		*this = Color(p_code);
		a = p_a;
	}
};

constexpr Color Color::operator+(const Color &p_color) const {
	return Color(
			r + p_color.r,
			g + p_color.g,
			b + p_color.b,
			a + p_color.a);
}

constexpr void Color::operator+=(const Color &p_color) {
	r = r + p_color.r;
	g = g + p_color.g;
	b = b + p_color.b;
	a = a + p_color.a;
}

constexpr Color Color::operator-(const Color &p_color) const {
	return Color(
			r - p_color.r,
			g - p_color.g,
			b - p_color.b,
			a - p_color.a);
}

constexpr void Color::operator-=(const Color &p_color) {
	r = r - p_color.r;
	g = g - p_color.g;
	b = b - p_color.b;
	a = a - p_color.a;
}

constexpr Color Color::operator*(const Color &p_color) const {
	return Color(
			r * p_color.r,
			g * p_color.g,
			b * p_color.b,
			a * p_color.a);
}

constexpr Color Color::operator*(float p_scalar) const {
	return Color(
			r * p_scalar,
			g * p_scalar,
			b * p_scalar,
			a * p_scalar);
}

constexpr void Color::operator*=(const Color &p_color) {
	r = r * p_color.r;
	g = g * p_color.g;
	b = b * p_color.b;
	a = a * p_color.a;
}

constexpr void Color::operator*=(float p_scalar) {
	r = r * p_scalar;
	g = g * p_scalar;
	b = b * p_scalar;
	a = a * p_scalar;
}

constexpr Color Color::operator/(const Color &p_color) const {
	return Color(
			r / p_color.r,
			g / p_color.g,
			b / p_color.b,
			a / p_color.a);
}

constexpr Color Color::operator/(float p_scalar) const {
	return Color(
			r / p_scalar,
			g / p_scalar,
			b / p_scalar,
			a / p_scalar);
}

constexpr void Color::operator/=(const Color &p_color) {
	r = r / p_color.r;
	g = g / p_color.g;
	b = b / p_color.b;
	a = a / p_color.a;
}

constexpr void Color::operator/=(float p_scalar) {
	r = r / p_scalar;
	g = g / p_scalar;
	b = b / p_scalar;
	a = a / p_scalar;
}

constexpr Color Color::operator-() const {
	return Color(
			1.0f - r,
			1.0f - g,
			1.0f - b,
			1.0f - a);
}

constexpr bool Color::operator<(const Color &p_color) const {
	if (r == p_color.r) {
		if (g == p_color.g) {
			if (b == p_color.b) {
				return (a < p_color.a);
			} else {
				return (b < p_color.b);
			}
		} else {
			return g < p_color.g;
		}
	} else {
		return r < p_color.r;
	}
}

constexpr Color operator*(float p_scalar, const Color &p_color) {
	return p_color * p_scalar;
}
// --- End core/math/color.h ---

// --- Folded from core/math/vector2i.h ---
#include "core/typedefs.h"


class String;
struct Vector2;

struct [[nodiscard]] Vector2i {
	static const Vector2i LEFT;
	static const Vector2i RIGHT;
	static const Vector2i UP;
	static const Vector2i DOWN;

	static constexpr int AXIS_COUNT = 2;

	enum Axis {
		AXIS_X,
		AXIS_Y,
	};

	union {
		int32_t x = 0;
		int32_t width;
	};
	union {
		int32_t y = 0;
		int32_t height;
	};

	constexpr int32_t &operator[](int p_axis) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(Vector2i, x) == 0 * sizeof(int32_t));
		static_assert(offsetof(Vector2i, width) == 0 * sizeof(int32_t));
		static_assert(offsetof(Vector2i, y) == 1 * sizeof(int32_t));
		static_assert(offsetof(Vector2i, height) == 1 * sizeof(int32_t));
		static_assert(sizeof(Vector2i) == 2 * sizeof(int32_t));

		DEV_ASSERT((unsigned int)p_axis < 2);
		return (&x)[p_axis];
	}
	constexpr const int32_t &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 2);
		return (&x)[p_axis];
	}

	_FORCE_INLINE_ Vector2i::Axis min_axis_index() const {
		return x < y ? Vector2i::AXIS_X : Vector2i::AXIS_Y;
	}

	_FORCE_INLINE_ Vector2i::Axis max_axis_index() const {
		return x < y ? Vector2i::AXIS_Y : Vector2i::AXIS_X;
	}

	Vector2i min(const Vector2i &p_vector2i) const {
		return Vector2i(MIN(x, p_vector2i.x), MIN(y, p_vector2i.y));
	}

	Vector2i mini(int32_t p_scalar) const {
		return Vector2i(MIN(x, p_scalar), MIN(y, p_scalar));
	}

	Vector2i max(const Vector2i &p_vector2i) const {
		return Vector2i(MAX(x, p_vector2i.x), MAX(y, p_vector2i.y));
	}

	Vector2i maxi(int32_t p_scalar) const {
		return Vector2i(MAX(x, p_scalar), MAX(y, p_scalar));
	}

	double distance_to(const Vector2i &p_to) const {
		return (p_to - *this).length();
	}

	int64_t distance_squared_to(const Vector2i &p_to) const {
		return (p_to - *this).length_squared();
	}

	constexpr Vector2i operator+(const Vector2i &p_v) const;
	constexpr void operator+=(const Vector2i &p_v);
	constexpr Vector2i operator-(const Vector2i &p_v) const;
	constexpr void operator-=(const Vector2i &p_v);
	constexpr Vector2i operator*(const Vector2i &p_v1) const;

	constexpr Vector2i operator*(int32_t p_rvalue) const;
	constexpr void operator*=(int32_t p_rvalue);

	constexpr Vector2i operator/(const Vector2i &p_v1) const;
	constexpr Vector2i operator/(int32_t p_rvalue) const;
	constexpr void operator/=(int32_t p_rvalue);

	constexpr Vector2i operator%(const Vector2i &p_v1) const;
	constexpr Vector2i operator%(int32_t p_rvalue) const;
	constexpr void operator%=(int32_t p_rvalue);

	constexpr Vector2i operator-() const;
	constexpr bool operator<(const Vector2i &p_vec2) const { return (x == p_vec2.x) ? (y < p_vec2.y) : (x < p_vec2.x); }
	constexpr bool operator>(const Vector2i &p_vec2) const { return (x == p_vec2.x) ? (y > p_vec2.y) : (x > p_vec2.x); }

	constexpr bool operator<=(const Vector2i &p_vec2) const { return x == p_vec2.x ? (y <= p_vec2.y) : (x < p_vec2.x); }
	constexpr bool operator>=(const Vector2i &p_vec2) const { return x == p_vec2.x ? (y >= p_vec2.y) : (x > p_vec2.x); }

	constexpr bool operator==(const Vector2i &p_vec2) const;
	constexpr bool operator!=(const Vector2i &p_vec2) const;

	int64_t length_squared() const;
	double length() const;

	real_t aspect() const { return width / (real_t)height; }
	Vector2i sign() const { return Vector2i(SIGN(x), SIGN(y)); }
	Vector2i abs() const { return Vector2i(Math::abs(x), Math::abs(y)); }
	Vector2i clamp(const Vector2i &p_min, const Vector2i &p_max) const;
	Vector2i clampi(int32_t p_min, int32_t p_max) const;
	Vector2i snapped(const Vector2i &p_step) const;
	Vector2i snappedi(int32_t p_step) const;

	explicit operator String() const;
	operator Vector2() const;

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_32(uint32_t(x));
		h = hash_murmur3_one_32(uint32_t(y), h);
		return hash_fmix32(h);
	}

	constexpr Vector2i() = default;
	constexpr Vector2i(int32_t p_x, int32_t p_y) :
			x(p_x), y(p_y) {}
};

inline constexpr Vector2i Vector2i::LEFT = { -1, 0 };
inline constexpr Vector2i Vector2i::RIGHT = { 1, 0 };
inline constexpr Vector2i Vector2i::UP = { 0, -1 };
inline constexpr Vector2i Vector2i::DOWN = { 0, 1 };

constexpr Vector2i Vector2i::operator+(const Vector2i &p_v) const {
	return Vector2i(x + p_v.x, y + p_v.y);
}

constexpr void Vector2i::operator+=(const Vector2i &p_v) {
	x += p_v.x;
	y += p_v.y;
}

constexpr Vector2i Vector2i::operator-(const Vector2i &p_v) const {
	return Vector2i(x - p_v.x, y - p_v.y);
}

constexpr void Vector2i::operator-=(const Vector2i &p_v) {
	x -= p_v.x;
	y -= p_v.y;
}

constexpr Vector2i Vector2i::operator*(const Vector2i &p_v1) const {
	return Vector2i(x * p_v1.x, y * p_v1.y);
}

constexpr Vector2i Vector2i::operator*(int32_t p_rvalue) const {
	return Vector2i(x * p_rvalue, y * p_rvalue);
}

constexpr void Vector2i::operator*=(int32_t p_rvalue) {
	x *= p_rvalue;
	y *= p_rvalue;
}

constexpr Vector2i Vector2i::operator/(const Vector2i &p_v1) const {
	return Vector2i(x / p_v1.x, y / p_v1.y);
}

constexpr Vector2i Vector2i::operator/(int32_t p_rvalue) const {
	return Vector2i(x / p_rvalue, y / p_rvalue);
}

constexpr void Vector2i::operator/=(int32_t p_rvalue) {
	x /= p_rvalue;
	y /= p_rvalue;
}

constexpr Vector2i Vector2i::operator%(const Vector2i &p_v1) const {
	return Vector2i(x % p_v1.x, y % p_v1.y);
}

constexpr Vector2i Vector2i::operator%(int32_t p_rvalue) const {
	return Vector2i(x % p_rvalue, y % p_rvalue);
}

constexpr void Vector2i::operator%=(int32_t p_rvalue) {
	x %= p_rvalue;
	y %= p_rvalue;
}

constexpr Vector2i Vector2i::operator-() const {
	return Vector2i(-x, -y);
}

constexpr bool Vector2i::operator==(const Vector2i &p_vec2) const {
	return x == p_vec2.x && y == p_vec2.y;
}

constexpr bool Vector2i::operator!=(const Vector2i &p_vec2) const {
	return x != p_vec2.x || y != p_vec2.y;
}

// Multiplication operators required to workaround issues with LLVM using implicit conversion.

constexpr Vector2i operator*(int32_t p_scalar, const Vector2i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector2i operator*(int64_t p_scalar, const Vector2i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector2i operator*(float p_scalar, const Vector2i &p_vector) {
	return p_vector * p_scalar;
}

constexpr Vector2i operator*(double p_scalar, const Vector2i &p_vector) {
	return p_vector * p_scalar;
}

typedef Vector2i Size2i;
typedef Vector2i Point2i;

template <>
struct is_zero_constructible<Vector2i> : std::true_type {};
// --- End core/math/vector2i.h ---

// --- Folded from core/templates/pair.h ---
#include "core/typedefs.h"

template <typename F, typename S> struct Pair
{
	F first{};
	S second{};

	constexpr Pair() = default;

	constexpr Pair(const F& p_first, const S& p_second) : first(p_first), second(p_second) {}

	constexpr bool operator==(const Pair& p_other) const
	{
		return first == p_other.first && second == p_other.second;
	}

	constexpr bool operator!=(const Pair& p_other) const
	{
		return first != p_other.first || second != p_other.second;
	}

	constexpr bool operator<(const Pair& p_other) const
	{
		return first == p_other.first ? (second < p_other.second) : (first < p_other.first);
	}

	constexpr bool operator<=(const Pair& p_other) const
	{
		return first == p_other.first ? (second <= p_other.second) : (first < p_other.first);
	}

	constexpr bool operator>(const Pair& p_other) const
	{
		return first == p_other.first ? (second > p_other.second) : (first > p_other.first);
	}

	constexpr bool operator>=(const Pair& p_other) const
	{
		return first == p_other.first ? (second >= p_other.second) : (first > p_other.first);
	}
};

template <typename F, typename S> struct PairSort
{
	constexpr bool operator()(const Pair<F, S>& p_lhs, const Pair<F, S>& p_rhs) const
	{
		return p_lhs < p_rhs;
	}
};

// Pair is zero-constructible if and only if both constrained types are zero-constructible.
template <typename F, typename S>
struct is_zero_constructible<Pair<F, S>>
	: std::conjunction<is_zero_constructible<F>, is_zero_constructible<S>>
{
};

template <typename K, typename V> struct KeyValue
{
	const K key{};
	V value{};

	KeyValue& operator=(const KeyValue& p_kv) = delete;
	KeyValue& operator=(KeyValue&& p_kv) = delete;

	constexpr KeyValue(const KeyValue& p_kv) = default;
	constexpr KeyValue(KeyValue&& p_kv) = default;

	constexpr KeyValue(const K& p_key, const V& p_value) : key(p_key), value(p_value) {}


	constexpr KeyValue(const Pair<K, V>& p_pair) : key(p_pair.first), value(p_pair.second) {}

	constexpr bool operator==(const KeyValue& p_other) const
	{
		return key == p_other.key && value == p_other.value;
	}

	constexpr bool operator!=(const KeyValue& p_other) const
	{
		return key != p_other.key || value != p_other.value;
	}

	constexpr bool operator<(const KeyValue& p_other) const
	{
		return key == p_other.key ? (value < p_other.value) : (key < p_other.key);
	}

	constexpr bool operator<=(const KeyValue& p_other) const
	{
		return key == p_other.key ? (value <= p_other.value) : (key < p_other.key);
	}

	constexpr bool operator>(const KeyValue& p_other) const
	{
		return key == p_other.key ? (value > p_other.value) : (key > p_other.key);
	}

	constexpr bool operator>=(const KeyValue& p_other) const
	{
		return key == p_other.key ? (value >= p_other.value) : (key > p_other.key);
	}
};

template <typename K, typename V> struct KeyValueSort
{
	constexpr bool operator()(const KeyValue<K, V>& p_lhs, const KeyValue<K, V>& p_rhs) const
	{
		return p_lhs.key < p_rhs.key;
	}
};

// KeyValue is zero-constructible if and only if both constrained types are zero-constructible.
template <typename K, typename V>
struct is_zero_constructible<KeyValue<K, V>>
	: std::conjunction<is_zero_constructible<K>, is_zero_constructible<V>>
{
};
// --- End core/templates/pair.h ---
