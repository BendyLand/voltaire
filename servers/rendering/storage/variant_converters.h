/**************************************************************************/
/*  variant_converters.h                                                  */
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

#pragma once

#include <initializer_list>
#include <type_traits>
#include "core/error/error_macros.h"
#include "core/types.h"

template <typename T> struct VariantConverterStd140
{
	// Generic base template for all Vector2/3/4(i) classes.
	static constexpr int Elements = T::AXIS_COUNT;

	template <typename P> static void convert(const T& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < Elements; i++) {
			p_write[i] = p_v[i];
		}
	}
};

template <> struct VariantConverterStd140<float>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(float p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<int32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(int32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<uint32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(uint32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<Basis>
{
	static constexpr int Elements = 9;

	template <typename P> static void convert(const Basis& p_v, P* p_write, bool p_compact)
	{
		// Basis can have compact 9 floats or std140 layout 12 floats.
		int i = 0;

		p_write[i++] = p_v.rows[0][0];
		p_write[i++] = p_v.rows[1][0];
		p_write[i++] = p_v.rows[2][0];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][1];
		p_write[i++] = p_v.rows[1][1];
		p_write[i++] = p_v.rows[2][1];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][2];
		p_write[i++] = p_v.rows[1][2];
		p_write[i++] = p_v.rows[2][2];
		if (!p_compact) {
			p_write[i++] = 0;
		}
	}
};

template <> struct VariantConverterStd140<Transform2D>
{
	static constexpr int Elements = 12;

	template <typename P> static void convert(const Transform2D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.columns[0][0];
		p_write[1] = p_v.columns[0][1];
		p_write[2] = 0;
		p_write[3] = 0;

		p_write[4] = p_v.columns[1][0];
		p_write[5] = p_v.columns[1][1];
		p_write[6] = 0;
		p_write[7] = 0;

		p_write[8] = p_v.columns[2][0];
		p_write[9] = p_v.columns[2][1];
		p_write[10] = 1;
		p_write[11] = 0;
	}
};

template <> struct VariantConverterStd140<Transform3D>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Transform3D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.basis.rows[0][0];
		p_write[1] = p_v.basis.rows[1][0];
		p_write[2] = p_v.basis.rows[2][0];
		p_write[3] = 0;

		p_write[4] = p_v.basis.rows[0][1];
		p_write[5] = p_v.basis.rows[1][1];
		p_write[6] = p_v.basis.rows[2][1];
		p_write[7] = 0;

		p_write[8] = p_v.basis.rows[0][2];
		p_write[9] = p_v.basis.rows[1][2];
		p_write[10] = p_v.basis.rows[2][2];
		p_write[11] = 0;

		p_write[12] = p_v.origin.x;
		p_write[13] = p_v.origin.y;
		p_write[14] = p_v.origin.z;
		p_write[15] = 1;
	}
};

template <> struct VariantConverterStd140<Projection>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Projection& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_write[i * 4 + j] = p_v.columns[i][j];
			}
		}
	}
};

template <typename T, typename P> T construct_vector(const std::initializer_list<P>& values)
{
	T vector{};
	int index = 0;
	for (P v : values) {
		vector[index++] = v;
		if (index >= T::AXIS_COUNT) {
			break;
		}
	}
	return vector;
}

template <typename, typename = void> inline constexpr bool is_vector_type_v = false;

template <typename T>
inline constexpr bool is_vector_type_v<T, std::void_t<decltype(T::AXIS_COUNT)>> = true;

template <typename T, typename P>
void convert_item_std140(const T& p_item, P* p_write, bool p_compact = false)
{
	VariantConverterStd140<T>::template convert<P>(p_item, p_write, p_compact);
}

template <typename T, typename From, typename To>
void write_array_std140(const Vector<From>& p_values, To* p_write, int p_array_size, int p_stride)
{
	constexpr int elements = VariantConverterStd140<T>::Elements;
	const int src_count = p_values.size();
	const int dst_count = elements * p_array_size;
	const int stride_count = p_stride * p_array_size;
	const From* read = p_values.ptr();
	const T default_value{};

	memset(p_write, 0, sizeof(To) * stride_count);

	for (int i = 0, j = 0; i < dst_count; i += elements, j += p_stride) {
		if (i + elements - 1 < src_count) {
			// Only copy full items with all elements, no partial or missing data.
			for (int e = 0; e < elements; e++) {
				DEV_ASSERT(j + e < stride_count && i + e < src_count);
				p_write[j + e] = read[i + e];
			}
		}
		else {
			// If not enough source data was passed in, write default values.
			convert_item_std140(default_value, p_write + j);
		}
	}
}


