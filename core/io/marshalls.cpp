/**************************************************************************/
/*  marshalls.cpp                                                         */
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

#include <climits>
#include <cstdio>
#include "core/io/resource_loader.h"
#include "marshalls.h"
#include "core/types.h"

void EncodedObjectAsID::_bind_methods() {}

#define ERR_FAIL_ADD_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(b)) < 0 || ((int32_t)(a)) < 0 || ((int32_t)(a)) > INT_MAX - ((int32_t)(b)),     \
		err)
#define ERR_FAIL_MUL_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(a)) < 0 || ((int32_t)(b)) <= 0 || ((int32_t)(a)) > INT_MAX / ((int32_t)(b)),    \
		err)

// Byte 0: `Variant::Type`, byte 1: unused, bytes 2 and 3: additional data.
#define HEADER_TYPE_MASK 0xFF

// For `Variant::INT`, `Variant::FLOAT` and other math types.
#define HEADER_DATA_FLAG_64 (1 << 16)

// For `Variant::OBJECT`.
#define HEADER_DATA_FLAG_OBJECT_AS_ID (1 << 16)

// For `Variant::ARRAY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_ARRAY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT 16

// For `Variant::DICTIONARY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT 16
// Occupies bits 18 and 19.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK (0b11 << 18)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT 18

enum ContainerTypeKind
{
	CONTAINER_TYPE_KIND_NONE = 0b00,
	CONTAINER_TYPE_KIND_BUILTIN = 0b01,
	CONTAINER_TYPE_KIND_CLASS_NAME = 0b10,
	CONTAINER_TYPE_KIND_SCRIPT = 0b11,
};

#define GET_CONTAINER_TYPE_KIND(m_header, m_field)                                                 \
	((ContainerTypeKind)(((m_header)&HEADER_DATA_FIELD_##m_field##_MASK) >>                        \
						 HEADER_DATA_FIELD_##m_field##_SHIFT))

static Error _decode_string(const uint8_t*& p_buffer, int& r_left, int* r_len, String& r_string)
{
	ERR_FAIL_COND_V(r_left < 4, ERR_INVALID_DATA);

	int32_t strlen = decode_uint32(p_buffer);
	int32_t pad = 0;

	// Handle padding.
	if (strlen % 4) {
		pad = 4 - strlen % 4;
	}

	p_buffer += 4;
	r_left -= 4;

	// Ensure buffer is big enough.
	ERR_FAIL_ADD_OF(strlen, pad, ERR_FILE_EOF);
	ERR_FAIL_COND_V(strlen < 0 || strlen + pad > r_left, ERR_FILE_EOF);

	String str;
	ERR_FAIL_COND_V(str.append_utf8((const char*)p_buffer, strlen) != OK, ERR_INVALID_DATA);
	r_string = str;

	// Add padding.
	strlen += pad;

	// Update buffer pos, left data count, and return size.
	p_buffer += strlen;
	r_left -= strlen;
	if (r_len) {
		(*r_len) += 4 + strlen;
	}

	return OK;
}

static void _encode_string(const String& p_string, uint8_t*& p_buffer, int& r_len)
{
	CharString utf8 = p_string.utf8();

	if (p_buffer) {
		encode_uint32(utf8.length(), p_buffer);
		p_buffer += 4;
		memcpy(p_buffer, utf8.get_data(), utf8.length());
		p_buffer += utf8.length();
	}

	r_len += 4 + utf8.length();
	while (r_len % 4) {
		r_len++; // Pad.
		if (p_buffer) {
			*(p_buffer++) = 0;
		}
	}
}

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count)
{
	// We always allocate a new array, and we don't `memcpy()`.
	// We also don't consider returning a pointer to the passed vectors when `sizeof(real_t) == 4`.
	// One reason is that we could decide to put a 4th component in `Vector3` for SIMD/mobile
	// performance, which would cause trouble with these optimizations.
	Vector<float> floats;
	if (p_count == 0) {
		return floats;
	}
	floats.resize(p_count * 3);
	float* floats_w = floats.ptrw();
	for (size_t i = 0; i < p_count; ++i) {
		const Vector3 v = p_vecs[i];
		floats_w[0] = v.x;
		floats_w[1] = v.y;
		floats_w[2] = v.z;
		floats_w += 3;
	}
	return floats;
}


