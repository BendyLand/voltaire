/**************************************************************************/
/*  gltf_structures.cpp                                                   */
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

#include "gltf_structures.h"

#include "core/types.h"
#include "gltf_state.h"
#include "scene/3d/camera_3d.h"

// ============================================================================
// GLTFTexture
// ============================================================================

GLTFImageIndex GLTFTexture::get_src_image() const { return src_image; }

void GLTFTexture::set_src_image(GLTFImageIndex val) { src_image = val; }

GLTFTextureSamplerIndex GLTFTexture::get_sampler() const { return sampler; }

void GLTFTexture::set_sampler(GLTFTextureSamplerIndex val) { sampler = val; }

// ============================================================================
// GLTFBufferView
// ============================================================================

GLTFBufferIndex GLTFBufferView::get_buffer() const { return buffer; }

void GLTFBufferView::set_buffer(GLTFBufferIndex p_buffer) { buffer = p_buffer; }

int64_t GLTFBufferView::get_byte_offset() const { return byte_offset; }

void GLTFBufferView::set_byte_offset(int64_t p_byte_offset) { byte_offset = p_byte_offset; }

int64_t GLTFBufferView::get_byte_length() const { return byte_length; }

void GLTFBufferView::set_byte_length(int64_t p_byte_length) { byte_length = p_byte_length; }

int64_t GLTFBufferView::get_byte_stride() const { return byte_stride; }

void GLTFBufferView::set_byte_stride(int64_t p_byte_stride) { byte_stride = p_byte_stride; }

bool GLTFBufferView::get_indices() const { return indices; }

void GLTFBufferView::set_indices(bool p_indices) { indices = p_indices; }

bool GLTFBufferView::get_vertex_attributes() const { return vertex_attributes; }

void GLTFBufferView::set_vertex_attributes(bool p_attributes) { vertex_attributes = p_attributes; }

Vector<uint8_t> GLTFBufferView::load_buffer_view_data(const Ref<GLTFState> p_gltf_state) const
{
	ERR_FAIL_COND_V(p_gltf_state.is_null(), Vector<uint8_t>());
	const Vector<PackedByteArray>& buffers = p_gltf_state->get_buffers();
	ERR_FAIL_INDEX_V(buffer, buffers.size(), Vector<uint8_t>());
	const PackedByteArray& buffer_data = buffers[buffer];
	const int64_t byte_end = byte_offset + byte_length;
	// Note that for buffer views with a byte stride, the parts of this data which get used may
	// only be determined in combination with the accessors that reference this buffer view.
	return buffer_data.slice(byte_offset, byte_end);
}

GLTFBufferViewIndex GLTFBufferView::write_new_buffer_view_into_state(
	const Ref<GLTFState>& p_gltf_state, const PackedByteArray& p_input_data,
	const int64_t p_alignment, const ArrayBufferTarget p_target, const int64_t p_byte_stride,
	const GLTFBufferIndex p_buffer_index, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_buffer_index < 0, -1, "Buffer index must be greater than or equal to zero.");
	const bool target_is_indices = p_target == ArrayBufferTarget::TARGET_ELEMENT_ARRAY_BUFFER;
	const bool target_is_vertex_attributes = p_target == ArrayBufferTarget::TARGET_ARRAY_BUFFER;
	if (target_is_vertex_attributes) {
		ERR_FAIL_COND_V_MSG(p_byte_stride < 4 || p_byte_stride % 4 != 0, -1,
			"glTF export: Vertex attributes using TARGET_ARRAY_BUFFER must have a byte stride that "
			"is a multiple of 4 as required by section 3.6.2.4 of the glTF specification.");
	}
	// Check for duplicate buffer views before adding a new one.
	Vector<Ref<GLTFBufferView>> state_buffer_views = p_gltf_state->get_buffer_views();
	const int buffer_view_index = state_buffer_views.size();
	if (p_deduplicate) {
		for (int i = 0; i < buffer_view_index; i++) {
			const Ref<GLTFBufferView>& existing_buffer_view = state_buffer_views[i];
			if (existing_buffer_view->get_byte_offset() % p_alignment == 0 &&
				existing_buffer_view->get_byte_length() == p_input_data.size() &&
				existing_buffer_view->get_byte_stride() == p_byte_stride &&
				existing_buffer_view->get_indices() == target_is_indices &&
				existing_buffer_view->get_vertex_attributes() == target_is_vertex_attributes) {
				if (existing_buffer_view->load_buffer_view_data(p_gltf_state) == p_input_data) {
					// Duplicate found, return the index of the existing buffer view.
					return i;
				}
			}
		}
	}
	// Write the data into the buffer at the specified index.
	Vector<PackedByteArray> state_buffers = p_gltf_state->get_buffers();
	if (state_buffers.size() <= p_buffer_index) {
		state_buffers.resize(p_buffer_index + 1);
	}
	PackedByteArray state_buffer = state_buffers[p_buffer_index];
	const int64_t input_data_size = p_input_data.size();
	// This is used by accessors. The byte offset of an accessor MUST be a multiple of the
	// accessor's component size.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	int64_t byte_offset = state_buffer.size();
	int64_t padding_bytes = 0;
	if (byte_offset % p_alignment != 0) {
		padding_bytes = p_alignment - (byte_offset % p_alignment);
		byte_offset += padding_bytes;
	}
	state_buffer.resize(byte_offset + input_data_size);
	if (padding_bytes > 0) {
		memset(state_buffer.ptrw() + (byte_offset - padding_bytes), 0, padding_bytes);
	}
	uint8_t* buffer_ptr = state_buffer.ptrw();
	memcpy(buffer_ptr + byte_offset, p_input_data.ptr(), input_data_size);
	state_buffers.set(p_buffer_index, state_buffer);
	p_gltf_state->set_buffers(state_buffers);
	// Create a new GLTFBufferView that references the new buffer.
	Ref<GLTFBufferView> buffer_view;
	buffer_view.instantiate();
	buffer_view->set_buffer(p_buffer_index);
	buffer_view->set_byte_offset(byte_offset);
	buffer_view->set_byte_length(input_data_size);
	buffer_view->set_byte_stride(p_byte_stride);
	buffer_view->set_indices(target_is_indices);
	buffer_view->set_vertex_attributes(target_is_vertex_attributes);
	// Add the new buffer view to the state.
	state_buffer_views.append(buffer_view);
	p_gltf_state->set_buffer_views(state_buffer_views);
	return buffer_view_index;
}

// ============================================================================
// GLTFAccessor
// ============================================================================

GLTFBufferViewIndex GLTFAccessor::get_buffer_view() const { return buffer_view; }

void GLTFAccessor::set_buffer_view(GLTFBufferViewIndex p_buffer_view)
{
	buffer_view = p_buffer_view;
}

int64_t GLTFAccessor::get_byte_offset() const { return byte_offset; }

void GLTFAccessor::set_byte_offset(int64_t p_byte_offset) { byte_offset = p_byte_offset; }

GLTFAccessor::GLTFComponentType GLTFAccessor::get_component_type() const { return component_type; }

void GLTFAccessor::set_component_type(GLTFComponentType p_component_type)
{
	component_type = (GLTFComponentType)p_component_type;
}

bool GLTFAccessor::get_normalized() const { return normalized; }

void GLTFAccessor::set_normalized(bool p_normalized) { normalized = p_normalized; }

int64_t GLTFAccessor::get_count() const { return count; }

void GLTFAccessor::set_count(int64_t p_count) { count = p_count; }

GLTFAccessor::GLTFAccessorType GLTFAccessor::get_accessor_type() const { return accessor_type; }

void GLTFAccessor::set_accessor_type(GLTFAccessorType p_accessor_type)
{
	accessor_type = p_accessor_type;
}

int GLTFAccessor::get_type() const { return (int)accessor_type; }

void GLTFAccessor::set_type(int p_accessor_type)
{
	accessor_type = (GLTFAccessorType)p_accessor_type;
}

Vector<double> GLTFAccessor::get_min() const { return Vector<double>(min); }

void GLTFAccessor::set_min(const Vector<double>& p_min) { min = Vector<double>(p_min); }

Vector<double> GLTFAccessor::get_max() const { return Vector<double>(max); }

void GLTFAccessor::set_max(const Vector<double>& p_max) { max = Vector<double>(p_max); }

int64_t GLTFAccessor::get_sparse_count() const { return sparse_count; }

void GLTFAccessor::set_sparse_count(int64_t p_sparse_count) { sparse_count = p_sparse_count; }

GLTFBufferViewIndex GLTFAccessor::get_sparse_indices_buffer_view() const
{
	return sparse_indices_buffer_view;
}

void GLTFAccessor::set_sparse_indices_buffer_view(GLTFBufferViewIndex p_sparse_indices_buffer_view)
{
	sparse_indices_buffer_view = p_sparse_indices_buffer_view;
}

int64_t GLTFAccessor::get_sparse_indices_byte_offset() const { return sparse_indices_byte_offset; }

void GLTFAccessor::set_sparse_indices_byte_offset(int64_t p_sparse_indices_byte_offset)
{
	sparse_indices_byte_offset = p_sparse_indices_byte_offset;
}

GLTFAccessor::GLTFComponentType GLTFAccessor::get_sparse_indices_component_type() const
{
	return sparse_indices_component_type;
}

void GLTFAccessor::set_sparse_indices_component_type(
	GLTFComponentType p_sparse_indices_component_type)
{
	sparse_indices_component_type = (GLTFComponentType)p_sparse_indices_component_type;
}

GLTFBufferViewIndex GLTFAccessor::get_sparse_values_buffer_view() const
{
	return sparse_values_buffer_view;
}

void GLTFAccessor::set_sparse_values_buffer_view(GLTFBufferViewIndex p_sparse_values_buffer_view)
{
	sparse_values_buffer_view = p_sparse_values_buffer_view;
}

int64_t GLTFAccessor::get_sparse_values_byte_offset() const { return sparse_values_byte_offset; }

void GLTFAccessor::set_sparse_values_byte_offset(int64_t p_sparse_values_byte_offset)
{
	sparse_values_byte_offset = p_sparse_values_byte_offset;
}

void GLTFAccessor::_calculate_min_and_max(const PackedFloat64Array& p_numbers)
{
	const int64_t vector_size = _get_vector_size();
	ERR_FAIL_COND(vector_size <= 0 || p_numbers.size() % vector_size != 0);
	min.resize(vector_size);
	max.resize(vector_size);
	// Initialize min and max with the first vector element values.
	for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
		min.write[in_vec] = p_numbers[in_vec];
		max.write[in_vec] = p_numbers[in_vec];
	}
	// Iterate over the rest of the vectors.
	for (int64_t which_vec = vector_size; which_vec < p_numbers.size(); which_vec += vector_size) {
		for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
			min.write[in_vec] = MIN(p_numbers[which_vec + in_vec], min[in_vec]);
			max.write[in_vec] = MAX(p_numbers[which_vec + in_vec], max[in_vec]);
		}
	}
	// 3.6.2.5: For floating-point components, JSON-stored minimum and maximum values represent
	// single precision floats and SHOULD be rounded to single precision before usage to avoid any
	// potential boundary mismatches.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#accessors-bounds
	if (component_type == GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT) {
		for (int64_t i = 0; i < min.size(); i++) {
			min.write[i] = (double)(float)min[i];
			max.write[i] = (double)(float)max[i];
		}
	}
}

void GLTFAccessor::_determine_pad_skip(int64_t& r_skip_every, int64_t& r_skip_bytes) const
{
	// 3.6.2.4. Accessors of matrix type have data stored in column-major order. The start of each
	// column MUST be aligned to 4-byte boundaries.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	switch (component_type) {
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
		if (accessor_type == GLTFAccessor::TYPE_MAT2) {
			r_skip_every = 2;
			r_skip_bytes = 2;
		}
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			r_skip_every = 3;
			r_skip_bytes = 1;
		}
	} break;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			r_skip_every = 6;
			r_skip_bytes = 2;
		}
	} break;
	default: {
	} break;
	}
}

int64_t GLTFAccessor::_determine_padded_byte_count(int64_t p_raw_byte_size) const
{
	// 3.6.2.4. Accessors of matrix type have data stored in column-major order. The start of each
	// column MUST be aligned to 4-byte boundaries.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	switch (component_type) {
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
		if (accessor_type == GLTFAccessor::TYPE_MAT2) {
			return p_raw_byte_size * 2;
		}
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			return p_raw_byte_size * 4 / 3;
		}
	} break;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			return p_raw_byte_size * 4 / 3;
		}
	} break;
	default: {
	} break;
	}
	return p_raw_byte_size;
}

PackedFloat64Array GLTFAccessor::_filter_numbers(const PackedFloat64Array& p_numbers) const
{
	PackedFloat64Array filtered_numbers = p_numbers;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		const double num = p_numbers[i];
		if (!Math::is_finite(num)) {
			// 3.6.2.2. "Values of NaN, +Infinity, and -Infinity MUST NOT be present."
			// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#accessor-data-types
			filtered_numbers.set(i, 0.0);
		}
		else if (component_type == GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT) {
			filtered_numbers.set(i, (double)(float)num);
		}
	}
	return filtered_numbers;
}

String GLTFAccessor::_get_component_type_name(const GLTFComponentType p_component)
{
	// These names are only for debugging and printing error messages, glTF uses the numeric values.
	switch (p_component) {
	case GLTFAccessor::COMPONENT_TYPE_NONE:
		return "None";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
		return "Byte";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE:
		return "UByte";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
		return "Short";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT:
		return "UShort";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT:
		return "Int";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT:
		return "UInt";
	case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT:
		return "Float";
	case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT:
		return "Double";
	case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT:
		return "Half";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG:
		return "Long";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG:
		return "ULong";
	}

	return "<Error>";
}

GLTFAccessor::GLTFComponentType GLTFAccessor::_get_indices_component_type_for_size(
	const int64_t p_size)
{
	ERR_FAIL_COND_V(p_size < 0, GLTFAccessor::COMPONENT_TYPE_NONE);
	// 3.7.2.1. indices accessor MUST NOT contain the maximum possible value for the component type
	// used (i.e., 255 for unsigned bytes, 65535 for unsigned shorts, 4294967295 for unsigned ints).
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
	if (unlikely(p_size > 4294967294LL)) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG;
	}
	if (p_size > 65534LL) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT;
	}
	if (p_size > 254LL) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT;
	}
	return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE;
}

GLTFAccessor::GLTFAccessorType GLTFAccessor::_get_accessor_type_from_str(const String& p_string)
{
	if (p_string == "SCALAR") {
		return GLTFAccessor::TYPE_SCALAR;
	}
	if (p_string == "VEC2") {
		return GLTFAccessor::TYPE_VEC2;
	}
	if (p_string == "VEC3") {
		return GLTFAccessor::TYPE_VEC3;
	}
	if (p_string == "VEC4") {
		return GLTFAccessor::TYPE_VEC4;
	}
	if (p_string == "MAT2") {
		return GLTFAccessor::TYPE_MAT2;
	}
	if (p_string == "MAT3") {
		return GLTFAccessor::TYPE_MAT3;
	}
	if (p_string == "MAT4") {
		return GLTFAccessor::TYPE_MAT4;
	}
	ERR_FAIL_V(GLTFAccessor::TYPE_SCALAR);
}

String GLTFAccessor::_get_accessor_type_name() const
{
	switch (accessor_type) {
	case GLTFAccessor::TYPE_SCALAR:
		return "SCALAR";
	case GLTFAccessor::TYPE_VEC2:
		return "VEC2";
	case GLTFAccessor::TYPE_VEC3:
		return "VEC3";
	case GLTFAccessor::TYPE_VEC4:
		return "VEC4";
	case GLTFAccessor::TYPE_MAT2:
		return "MAT2";
	case GLTFAccessor::TYPE_MAT3:
		return "MAT3";
	case GLTFAccessor::TYPE_MAT4:
		return "MAT4";
	default:
		break;
	}
	ERR_FAIL_V("SCALAR");
}

int64_t GLTFAccessor::_get_vector_size() const
{
	switch (accessor_type) {
	case GLTFAccessor::TYPE_SCALAR:
		return 1;
	case GLTFAccessor::TYPE_VEC2:
		return 2;
	case GLTFAccessor::TYPE_VEC3:
		return 3;
	case GLTFAccessor::TYPE_VEC4:
		return 4;
	case GLTFAccessor::TYPE_MAT2:
		return 4;
	case GLTFAccessor::TYPE_MAT3:
		return 9;
	case GLTFAccessor::TYPE_MAT4:
		return 16;
	default:
		break;
	}
	ERR_FAIL_V(0);
}

int64_t GLTFAccessor::_get_bytes_per_component(const GLTFComponentType p_component_type)
{
	switch (p_component_type) {
	case GLTFAccessor::COMPONENT_TYPE_NONE:
		ERR_FAIL_V(0);
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE:
		return 1;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT:
		return 2;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT:
	case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT:
		return 4;
	case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT:
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG:
		return 8;
	}
	ERR_FAIL_V(0);
}

int64_t GLTFAccessor::_get_bytes_per_vector() const
{
	const int64_t raw_byte_size = _get_bytes_per_component(component_type) * _get_vector_size();
	return _determine_padded_byte_count(raw_byte_size);
}

bool GLTFAccessor::is_equal_exact(const Ref<GLTFAccessor>& p_other) const
{
	if (p_other.is_null()) {
		return false;
	}
	return (buffer_view == p_other->buffer_view && byte_offset == p_other->byte_offset &&
			component_type == p_other->component_type && normalized == p_other->normalized &&
			count == p_other->count && accessor_type == p_other->accessor_type &&
			min == p_other->min && max == p_other->max && sparse_count == p_other->sparse_count &&
			sparse_indices_buffer_view == p_other->sparse_indices_buffer_view &&
			sparse_indices_byte_offset == p_other->sparse_indices_byte_offset &&
			sparse_indices_component_type == p_other->sparse_indices_component_type &&
			sparse_values_buffer_view == p_other->sparse_values_buffer_view &&
			sparse_values_byte_offset == p_other->sparse_values_byte_offset);
}

PackedInt64Array GLTFAccessor::_decode_sparse_indices(
	const Ref<GLTFState>& p_gltf_state, const Vector<Ref<GLTFBufferView>>& p_buffer_views) const
{
	const int64_t bytes_per_component = _get_bytes_per_component(sparse_indices_component_type);
	PackedInt64Array numbers;
	ERR_FAIL_INDEX_V(sparse_indices_buffer_view, p_buffer_views.size(), numbers);
	const Ref<GLTFBufferView> actual_buffer_view = p_buffer_views[sparse_indices_buffer_view];
	const PackedByteArray raw_bytes = actual_buffer_view->load_buffer_view_data(p_gltf_state);
	const int64_t min_raw_byte_size =
		bytes_per_component * sparse_count + sparse_indices_byte_offset;
	ERR_FAIL_COND_V_MSG(raw_bytes.size() < min_raw_byte_size, numbers,
		"glTF import: Sparse indices buffer view did not have enough bytes to read the expected "
		"number of indices. Returning an empty array.");
	numbers.resize(sparse_count);
	const uint8_t* raw_pointer = raw_bytes.ptr();
	int64_t raw_read_offset = sparse_indices_byte_offset;
	for (int64_t i = 0; i < sparse_count; i++) {
		const uint8_t* raw_source = &raw_pointer[raw_read_offset];
		int64_t number = 0;
		switch (sparse_indices_component_type) {
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			number = *(uint8_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			number = *(uint16_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			number = *(uint32_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			number = *(uint64_t*)raw_source;
		} break;
		default: {
			ERR_FAIL_V_MSG(PackedInt64Array(),
				"glTF import: Sparse indices must have an unsigned integer "
				"component type. Failed to decode, returning an empty array.");
		}
		}
		numbers.set(i, number);
		raw_read_offset += bytes_per_component;
	}
	ERR_FAIL_COND_V_MSG(raw_read_offset != raw_bytes.size(), numbers,
		"glTF import: Sparse indices buffer view size did not exactly match the expected size.");
	return numbers;
}

template <typename T>
Vector<T> GLTFAccessor::_decode_raw_numbers(const Ref<GLTFState>& p_gltf_state,
	const Vector<Ref<GLTFBufferView>>& p_buffer_views, bool p_sparse_values) const
{
	const int64_t bytes_per_component = _get_bytes_per_component(component_type);
	const int64_t bytes_per_vector = _get_bytes_per_vector();
	const int64_t vector_size = _get_vector_size();
	int64_t pad_skip_every = 0;
	int64_t pad_skip_bytes = 0;
	_determine_pad_skip(pad_skip_every, pad_skip_bytes);
	int64_t raw_vector_count;
	int64_t raw_buffer_view_index;
	int64_t raw_read_offset_start;
	if (p_sparse_values) {
		raw_vector_count = sparse_count;
		raw_buffer_view_index = sparse_values_buffer_view;
		raw_read_offset_start = sparse_values_byte_offset;
	}
	else {
		raw_vector_count = count;
		raw_buffer_view_index = buffer_view;
		raw_read_offset_start = byte_offset;
	}
	const int64_t raw_number_count = raw_vector_count * vector_size;
	Vector<T> ret_numbers;
	if (raw_buffer_view_index == -1) {
		ret_numbers.resize(raw_number_count);
		// No buffer view, so fill with zeros.
		for (int64_t i = 0; i < raw_number_count; i++) {
			ret_numbers.set(i, T(0));
		}
		return ret_numbers;
	}
	ERR_FAIL_INDEX_V(raw_buffer_view_index, p_buffer_views.size(), ret_numbers);
	const Ref<GLTFBufferView> raw_buffer_view = p_buffer_views[raw_buffer_view_index];
	if (raw_buffer_view->get_byte_offset() % bytes_per_component != 0) {
		WARN_PRINT("glTF import: Buffer view byte offset is not a multiple of accessor component "
				   "size. This file is invalid per the glTF specification and will not load "
				   "correctly in some glTF viewers, but Godot will try to load it anyway.");
	}
	if (byte_offset % bytes_per_component != 0) {
		WARN_PRINT("glTF import: Accessor byte offset is not a multiple of accessor component "
				   "size. This file is invalid per the glTF specification and will not load "
				   "correctly in some glTF viewers, but Godot will try to load it anyway.");
	}
	int64_t declared_byte_stride = raw_buffer_view->get_byte_stride();
	int64_t actual_byte_stride = bytes_per_vector;
	int64_t stride_skip_every = 0;
	int64_t stride_skip_bytes = 0;
	if (declared_byte_stride != -1) {
		ERR_FAIL_COND_V_MSG(declared_byte_stride % 4 != 0, ret_numbers,
			"glTF import: The declared buffer view byte stride " + itos(declared_byte_stride) +
				" was not a multiple of 4 as required by glTF. Returning an empty array.");
		if (declared_byte_stride > bytes_per_vector) {
			actual_byte_stride = declared_byte_stride;
			stride_skip_every = vector_size;
			stride_skip_bytes = declared_byte_stride - bytes_per_vector;
		}
	}
	else if (raw_buffer_view->get_vertex_attributes()) {
		print_verbose("WARNING: glTF import: Buffer view byte stride should be declared for vertex "
					  "attributes. Assuming packed data and reading anyway.");
	}
	const int64_t min_raw_byte_size =
		actual_byte_stride * (raw_vector_count - 1) + bytes_per_vector + raw_read_offset_start;
	const PackedByteArray raw_bytes = raw_buffer_view->load_buffer_view_data(p_gltf_state);
	ERR_FAIL_COND_V_MSG(raw_bytes.size() < min_raw_byte_size, ret_numbers,
		"glTF import: The buffer view size was smaller than the minimum required size for the "
		"accessor. Returning an empty array.");
	ret_numbers.resize(raw_number_count);
	const uint8_t* raw_pointer = raw_bytes.ptr();
	int64_t raw_read_offset = raw_read_offset_start;
	for (int64_t i = 0; i < raw_number_count; i++) {
		const uint8_t* raw_source = &raw_pointer[raw_read_offset];
		T number = 0;
		// 3.11. Implementations MUST use following equations to decode real floating-point value f
		// from a normalized integer c and vice-versa.
		// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#animations
		switch (component_type) {
		case GLTFAccessor::COMPONENT_TYPE_NONE: {
			ERR_FAIL_V_MSG(Vector<T>(), "glTF import: Failed to decode buffer view, component type "
										"not set. Returning an empty array.");
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE: {
			int8_t prim = *(int8_t*)raw_source;
			if (normalized) {
				number = T(MAX(double(prim) / 127.0, -1.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			uint8_t prim = *(uint8_t*)raw_source;
			if (normalized) {
				number = T((double(prim) / 255.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT: {
			int16_t prim = *(int16_t*)raw_source;
			if (normalized) {
				number = T(MAX(double(prim) / 32767.0, -1.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			uint16_t prim = *(uint16_t*)raw_source;
			if (normalized) {
				number = T(double(prim) / 65535.0);
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT: {
			number = T(*(int32_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			number = T(*(uint32_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT: {
			number = T(*(float*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT: {
			number = T(*(double*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT: {
			number = Math::half_to_float(*(uint16_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG: {
			number = T(*(int64_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			number = T(*(uint64_t*)raw_source);
		} break;
		}
		ret_numbers.set(i, number);
		raw_read_offset += bytes_per_component;
		// Padding and stride skipping are distinct concepts that both need to be handled.
		// For example, a 2-in-1 interleaved MAT3 bytes accessor has both, and would look like:
		// AAA0 AAA0 AAA0 BBB0 BBB0 BBB0 AAA0 AAA0 AAA0 BBB0 BBB0 BBB0
		// The "0" is skipped by the padding, and the "BBB0" is skipped by the stride.
		// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
		if (unlikely(pad_skip_every > 0)) {
			if ((i + 1) % pad_skip_every == 0) {
				raw_read_offset += pad_skip_bytes;
			}
		}
		if (unlikely(stride_skip_every > 0)) {
			if ((i + 1) % stride_skip_every == 0) {
				raw_read_offset += stride_skip_bytes;
			}
		}
	}
	return ret_numbers;
}

template <typename T>
Vector<T> GLTFAccessor::_decode_as_numbers(const Ref<GLTFState>& p_gltf_state) const
{
	const Vector<Ref<GLTFBufferView>>& p_buffer_views = p_gltf_state->get_buffer_views();
	Vector<T> ret_numbers = _decode_raw_numbers<T>(p_gltf_state, p_buffer_views, false);
	if (sparse_count == 0) {
		return ret_numbers;
	}
	// Handle sparse accessors.
	PackedInt64Array sparse_indices = _decode_sparse_indices(p_gltf_state, p_buffer_views);
	ERR_FAIL_COND_V_MSG(sparse_indices.size() != sparse_count, ret_numbers,
		"glTF import: Sparse indices size does not match the sparse count.");
	const int64_t vector_size = _get_vector_size();
	Vector<T> sparse_values = _decode_raw_numbers<T>(p_gltf_state, p_buffer_views, true);
	ERR_FAIL_COND_V_MSG(sparse_values.size() != sparse_count * vector_size, ret_numbers,
		"glTF import: Sparse values size does not match the sparse count.");
	for (int64_t in_sparse = 0; in_sparse < sparse_count; in_sparse++) {
		const int64_t sparse_index = sparse_indices[in_sparse];
		const int64_t array_offset = sparse_index * vector_size;
		ERR_FAIL_INDEX_V_MSG(array_offset, ret_numbers.size(), ret_numbers,
			"glTF import: Sparse indices were out of bounds for the accessor.");
		for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
			ret_numbers.set(array_offset + in_vec, sparse_values[in_sparse * vector_size + in_vec]);
		}
	}
	return ret_numbers;
}

PackedColorArray GLTFAccessor::decode_as_colors(const Ref<GLTFState>& p_gltf_state) const
{
	PackedColorArray ret;
	PackedFloat32Array numbers = _decode_as_numbers<float>(p_gltf_state);
	if (accessor_type == TYPE_VEC3) {
		ERR_FAIL_COND_V_MSG(numbers.size() != count * 3, ret,
			"glTF import: The accessor does not have the expected amount of numbers for the given "
			"count and vector size.");
		ret.resize(count);
		for (int64_t i = 0; i < count; i++) {
			const int64_t number_index = i * 3;
			ret.set(i, Color(numbers[number_index], numbers[number_index + 1],
						   numbers[number_index + 2], 1.0f));
		}
	}
	else if (accessor_type == TYPE_VEC4) {
		ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
			"glTF import: The accessor does not have the expected amount of numbers for the given "
			"count and vector size.");
		ret.resize(count);
		for (int64_t i = 0; i < count; i++) {
			const int64_t number_index = i * 4;
			ret.set(i, Color(numbers[number_index], numbers[number_index + 1],
						   numbers[number_index + 2], numbers[number_index + 3]));
		}
	}
	else {
		ERR_FAIL_V_MSG(ret,
			"glTF import: The `decode_as_colors` function is designed to be fast and can only be "
			"used with accessors of type \"VEC3\" or \"VEC4\", but was called with type \"" +
				_get_accessor_type_name() + "\".");
	}
	return ret;
}

PackedFloat32Array GLTFAccessor::decode_as_float32s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<float>(p_gltf_state);
}

PackedFloat64Array GLTFAccessor::decode_as_float64s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<double>(p_gltf_state);
}

PackedInt32Array GLTFAccessor::decode_as_int32s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<int32_t>(p_gltf_state);
}

PackedInt64Array GLTFAccessor::decode_as_int64s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<int64_t>(p_gltf_state);
}

Vector<Quaternion> GLTFAccessor::decode_as_quaternions(const Ref<GLTFState>& p_gltf_state) const
{
	Vector<Quaternion> ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC4, ret,
		"glTF import: The `decode_as_quaternions` function is designed to be fast and can only be "
		"used with accessors of type \"VEC4\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 4;
		ret.set(i, Quaternion(numbers[number_index], numbers[number_index + 1],
					   numbers[number_index + 2], numbers[number_index + 3])
					   .normalized());
	}
	return ret;
}

PackedVector2Array GLTFAccessor::decode_as_vector2s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector2Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC2, ret,
		"glTF import: The `decode_as_vector2s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC2\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 2, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 2;
		ret.set(i, Vector2(numbers[number_index], numbers[number_index + 1]));
	}
	return ret;
}

PackedVector3Array GLTFAccessor::decode_as_vector3s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector3Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC3, ret,
		"glTF import: The `decode_as_vector3s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC3\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 3, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 3;
		ret.set(i,
			Vector3(numbers[number_index], numbers[number_index + 1], numbers[number_index + 2]));
	}
	return ret;
}

PackedVector4Array GLTFAccessor::decode_as_vector4s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector4Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC4, ret,
		"glTF import: The `decode_as_vector4s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC4\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 4;
		ret.set(i, Vector4(numbers[number_index], numbers[number_index + 1],
					   numbers[number_index + 2], numbers[number_index + 3]));
	}
	return ret;
}

void GLTFAccessor::_store_sparse_indices_into_state(const Ref<GLTFState>& p_gltf_state,
	const PackedInt64Array& p_sparse_indices, const bool p_deduplicate)
{
	// The byte offset of a sparse accessor's indices buffer view MUST be a multiple of the indices
	// primitive componentType.
	// https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/accessor.sparse.indices.schema.json
	const int64_t bytes_per_index = _get_bytes_per_component(sparse_indices_component_type);
	PackedByteArray indices_bytes;
	indices_bytes.resize(bytes_per_index * p_sparse_indices.size());
	uint8_t* ret_write = indices_bytes.ptrw();
	int64_t ret_byte_offset = 0;
	for (int64_t i = 0; i < p_sparse_indices.size(); i++) {
		switch (sparse_indices_component_type) {
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			*(uint8_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			*(uint32_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			*(uint64_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		default: {
			ERR_FAIL_MSG("glTF export: Invalid sparse indices component type '" +
						 _get_component_type_name(sparse_indices_component_type) +
						 "' for sparse accessor indices.");
		} break;
		}
		ret_byte_offset += bytes_per_index;
	}
	const GLTFBufferViewIndex buffer_view_index =
		GLTFBufferView::write_new_buffer_view_into_state(p_gltf_state, indices_bytes,
			bytes_per_index, GLTFBufferView::TARGET_NONE, -1, 0, p_deduplicate);
	ERR_FAIL_COND_MSG(
		buffer_view_index == -1, "glTF export: Failed to write sparse indices into glTF state.");
	set_sparse_indices_buffer_view(buffer_view_index);
}

GLTFAccessor::GLTFComponentType GLTFAccessor::get_minimal_integer_component_type_from_ints(
	const PackedInt64Array& p_numbers)
{
	bool has_negative = false;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		if (p_numbers[i] < 0) {
			has_negative = true;
			break;
		}
	}
	if (has_negative) {
		GLTFComponentType ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE;
		for (int64_t i = 0; i < p_numbers.size(); i++) {
			const int64_t num = p_numbers[i];
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE && (num < -128LL || num > 127LL)) {
				ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT;
			}
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT &&
				(num < -32768LL || num > 32767LL)) {
				ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_INT;
			}
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_INT &&
				(num < -2147483648LL || num > 2147483647LL)) {
				return GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG;
			}
		}
		return ret;
	}
	GLTFComponentType ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		const int64_t num = p_numbers[i];
		// 3.7.2.1. indices accessor MUST NOT contain the maximum possible value for the component
		// type used (i.e., 255 for unsigned bytes, 65535 for unsigned shorts, 4294967295 for
		// unsigned ints). https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE && num > 254LL) {
			ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT;
		}
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT && num > 65534LL) {
			ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT;
		}
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT && num > 4294967294LL) {
			return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG;
		}
	}
	return ret;
}

PackedByteArray GLTFAccessor::encode_floats_as_bytes(const PackedFloat64Array& p_input_numbers)
{
	// Filter and update `count`, `min`, and `max` based on the given data.
	PackedFloat64Array filtered_numbers = _filter_numbers(p_input_numbers);
	count = filtered_numbers.size() / _get_vector_size();
	_calculate_min_and_max(filtered_numbers);
	// Actually encode the data.
	const int64_t input_size = filtered_numbers.size();
	const int64_t bytes_per_component = _get_bytes_per_component(component_type);
	int64_t raw_byte_size = _determine_padded_byte_count(bytes_per_component * input_size);
	int64_t skip_every = 0;
	int64_t skip_bytes = 0;
	_determine_pad_skip(skip_every, skip_bytes);
	PackedByteArray ret;
	ret.resize(raw_byte_size);
	uint8_t* ret_write = ret.ptrw();
	int64_t ret_byte_offset = 0;
	for (int64_t i = 0; i < input_size; i++) {
		switch (component_type) {
		case COMPONENT_TYPE_NONE: {
			ERR_FAIL_V_MSG(ret, "glTF export: Invalid component type 'NONE' for glTF accessor.");
		} break;
		case COMPONENT_TYPE_SIGNED_BYTE: {
			*(int8_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_BYTE: {
			*(uint8_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SIGNED_SHORT: {
			*(int16_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_SHORT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SIGNED_INT: {
			*(int32_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_INT: {
			*(uint32_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SINGLE_FLOAT: {
			*(float*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_DOUBLE_FLOAT: {
			*(double*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_HALF_FLOAT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = Math::make_half_float(filtered_numbers[i]);
		} break;
		case COMPONENT_TYPE_SIGNED_LONG: {
			*(int64_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_LONG: {
			*(uint64_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		default: {
			ERR_FAIL_V_MSG(ret,
				"glTF export: Godot does not support writing glTF accessor components of type '" +
					itos(component_type) + "'.");
		} break;
		}
		ret_byte_offset += bytes_per_component;
		if (unlikely(skip_every > 0)) {
			if ((i + 1) % skip_every == 0) {
				ret_byte_offset += skip_bytes;
			}
		}
	}
	ERR_FAIL_COND_V_MSG(ret_byte_offset != raw_byte_size, ret,
		"glTF export: Accessor encoded data did not write exactly the expected number of bytes.");
	return ret;
}

GLTFAccessorIndex GLTFAccessor::store_accessor_data_into_state(const Ref<GLTFState>& p_gltf_state,
	const PackedByteArray& p_data_bytes,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target,
	const GLTFBufferIndex p_buffer_index, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(p_data_bytes.is_empty(), -1, "glTF export: Cannot store nothing.");
	const int64_t bytes_per_vec = _get_bytes_per_vector();
	ERR_FAIL_COND_V_MSG(bytes_per_vec == 0 || p_data_bytes.size() % bytes_per_vec != 0, -1,
		"glTF export: Tried to store an accessor with data that is not a multiple of the "
		"accessor's bytes per vector.");
	count = p_data_bytes.size() / bytes_per_vec;
	const int64_t alignment = _get_bytes_per_component(component_type);
	int64_t byte_stride = -1;
	if (p_buffer_view_target == GLTFBufferView::TARGET_ARRAY_BUFFER) {
		byte_stride = bytes_per_vec;
		ERR_FAIL_COND_V_MSG(byte_stride < 4 || byte_stride % 4 != 0, -1,
			"glTF export: Vertex attributes using TARGET_ARRAY_BUFFER must have a byte stride that "
			"is a multiple of 4 as required by section 3.6.2.4 of the glTF specification.");
	}
	const GLTFBufferViewIndex buffer_view_index = GLTFBufferView::write_new_buffer_view_into_state(
		p_gltf_state, p_data_bytes, alignment, p_buffer_view_target, byte_stride, 0, p_deduplicate);
	ERR_FAIL_COND_V_MSG(buffer_view_index == -1, -1,
		"glTF export: Accessor failed to write new buffer view into glTF state.");
	set_buffer_view(buffer_view_index);
	Vector<Ref<GLTFAccessor>> state_accessors = p_gltf_state->get_accessors();
	const GLTFAccessorIndex accessor_count = state_accessors.size();
	for (GLTFAccessorIndex i = 0; i < accessor_count; i++) {
		const Ref<GLTFAccessor>& existing_accessor = state_accessors[i];
		if (is_equal_exact(existing_accessor)) {
			return i;
		}
	}
	Ref<GLTFAccessor> self = this;
	state_accessors.append(self);
	p_gltf_state->set_accessors(state_accessors);
	return accessor_count;
}

Ref<GLTFAccessor> GLTFAccessor::make_new_accessor_without_data(
	GLTFAccessorType p_accessor_type, GLTFComponentType p_component_type)
{
	Ref<GLTFAccessor> accessor;
	accessor.instantiate();
	accessor->set_accessor_type(p_accessor_type);
	accessor->set_component_type(p_component_type);
	return accessor;
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_colors(const Ref<GLTFState>& p_gltf_state,
	const PackedColorArray& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Color& color = p_input_data[i];
		numbers.set(i * 4, color.r);
		numbers.set(i * 4 + 1, color.g);
		numbers.set(i * 4 + 2, color.b);
		numbers.set(i * 4 + 3, color.a);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_float64s(
	const Ref<GLTFState>& p_gltf_state, const PackedFloat64Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_SCALAR, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(p_input_data);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_int32s(const Ref<GLTFState>& p_gltf_state,
	const PackedInt32Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedInt64Array numbers;
	numbers.resize(p_input_data.size());
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		numbers.set(i, p_input_data[i]);
	}
	const GLTFComponentType component_type = get_minimal_integer_component_type_from_ints(numbers);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_SCALAR, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_int64s(const Ref<GLTFState>& p_gltf_state,
	const PackedInt64Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	const GLTFComponentType component_type =
		get_minimal_integer_component_type_from_ints(p_input_data);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_SCALAR, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(p_input_data);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_quaternions(
	const Ref<GLTFState>& p_gltf_state, const Vector<Quaternion>& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Quaternion& quat = p_input_data[i];
		numbers.set(i * 4, quat.x);
		numbers.set(i * 4 + 1, quat.y);
		numbers.set(i * 4 + 2, quat.z);
		numbers.set(i * 4 + 3, quat.w);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector2s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector2Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 2);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector2& vec = p_input_data[i];
		numbers.set(i * 2, vec.x);
		numbers.set(i * 2 + 1, vec.y);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC2, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector3s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector3Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 3);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector3& vec = p_input_data[i];
		numbers.set(i * 3, vec.x);
		numbers.set(i * 3 + 1, vec.y);
		numbers.set(i * 3 + 2, vec.z);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC3, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector4s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector4Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector4& vec = p_input_data[i];
		numbers.set(i * 4, vec.x);
		numbers.set(i * 4 + 1, vec.y);
		numbers.set(i * 4 + 2, vec.z);
		numbers.set(i * 4 + 3, vec.w);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector4is(
	const Ref<GLTFState>& p_gltf_state, const Vector<Vector4i>& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedInt64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector4i& vec = p_input_data[i];
		numbers.set(i * 4, vec.x);
		numbers.set(i * 4 + 1, vec.y);
		numbers.set(i * 4 + 2, vec.z);
		numbers.set(i * 4 + 3, vec.w);
	}
	const GLTFComponentType component_type = get_minimal_integer_component_type_from_ints(numbers);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_VEC4, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_sparse_accessor_from_vec3s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector3Array& p_input_data,
	const PackedVector3Array& p_base_reference_data, const double p_tolerance_multiplier,
	const GLTFBufferView::ArrayBufferTarget p_main_buffer_view_target, const bool p_deduplicate)
{
	const int64_t input_size = p_input_data.size();
	ERR_FAIL_COND_V_MSG(
		input_size == 0, -1, "glTF export: Cannot encode an accessor from an empty array.");
	const bool is_base_empty = p_base_reference_data.is_empty();
	ERR_FAIL_COND_V_MSG(!is_base_empty && p_base_reference_data.size() != input_size, -1,
		"glTF export: Base reference data must either be empty, or have the same size as the main "
		"input data.");
	PackedInt64Array sparse_indices;
	PackedFloat64Array sparse_values;
	PackedFloat64Array dense_values;
	int64_t highest_index = 0;
	dense_values.resize(input_size * 3);
	for (int64_t i = 0; i < input_size; i++) {
		Vector3 vec = p_input_data[i];
		Vector3 base_ref_vec;
		Vector3 displacement;
		if (is_base_empty) {
			base_ref_vec = Vector3();
			displacement = vec;
		}
		else {
			base_ref_vec = p_base_reference_data[i];
			displacement = vec - base_ref_vec;
		}
		if ((displacement * p_tolerance_multiplier).is_zero_approx()) {
			vec = base_ref_vec;
		}
		else {
			highest_index = i;
			sparse_indices.append(i);
			sparse_values.append(vec.x);
			sparse_values.append(vec.y);
			sparse_values.append(vec.z);
		}
		dense_values.set(i * 3, vec.x);
		dense_values.set(i * 3 + 1, vec.y);
		dense_values.set(i * 3 + 2, vec.z);
	}
	const int64_t sparse_count = sparse_indices.size();
	const int64_t bytes_per_value_component = _get_bytes_per_component(COMPONENT_TYPE_SINGLE_FLOAT);
	const GLTFComponentType indices_component_type =
		_get_indices_component_type_for_size(highest_index);
	const int64_t sparse_data_bytes =
		_get_bytes_per_component(indices_component_type) * sparse_count +
		bytes_per_value_component * sparse_values.size();
	const int64_t dense_data_bytes = bytes_per_value_component * 3 * input_size;
	constexpr int64_t sparse_json_fluff = 200;
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC3, COMPONENT_TYPE_SINGLE_FLOAT);
	if (sparse_data_bytes + sparse_json_fluff >= dense_data_bytes) {
		PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(dense_values);
		ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
			"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
		return accessor->store_accessor_data_into_state(
			p_gltf_state, encoded_bytes, p_main_buffer_view_target, 0, p_deduplicate);
	}
	if (sparse_count > 0) {
		accessor->set_sparse_count(sparse_count);
		accessor->set_sparse_indices_component_type(indices_component_type);
		accessor->_store_sparse_indices_into_state(p_gltf_state, sparse_indices, p_deduplicate);
		const PackedByteArray sparse_values_encoded_bytes =
			accessor->encode_floats_as_bytes(sparse_values);
		ERR_FAIL_COND_V_MSG(sparse_values_encoded_bytes.is_empty(), -1,
			"glTF export: Accessor failed to encode sparse values as bytes.");
		const GLTFBufferViewIndex sparse_values_buffer_view_index =
			GLTFBufferView::write_new_buffer_view_into_state(p_gltf_state,
				sparse_values_encoded_bytes, bytes_per_value_component, GLTFBufferView::TARGET_NONE,
				-1, 0, p_deduplicate);
		accessor->set_sparse_values_buffer_view(sparse_values_buffer_view_index);
	}
	if (is_base_empty) {
		accessor->set_count(input_size);
		const PackedFloat64Array filtered_numbers = accessor->_filter_numbers(dense_values);
		accessor->_calculate_min_and_max(filtered_numbers);
		Vector<Ref<GLTFAccessor>> state_accessors = p_gltf_state->get_accessors();
		const GLTFAccessorIndex accessor_count = state_accessors.size();
		for (GLTFAccessorIndex i = 0; i < accessor_count; i++) {
			const Ref<GLTFAccessor>& existing_accessor = state_accessors[i];
			if (accessor->is_equal_exact(existing_accessor)) {
				return i;
			}
		}
		state_accessors.append(accessor);
		p_gltf_state->set_accessors(state_accessors);
		return accessor_count;
	}
	PackedFloat64Array base_reference_values;
	base_reference_values.resize(input_size * 3);
	for (int64_t i = 0; i < input_size; i++) {
		const Vector3& base_ref_vec = p_base_reference_data[i];
		base_reference_values.set(i * 3, base_ref_vec.x);
		base_reference_values.set(i * 3 + 1, base_ref_vec.y);
		base_reference_values.set(i * 3 + 2, base_ref_vec.z);
	}
	const PackedByteArray base_reference_encoded_bytes =
		accessor->encode_floats_as_bytes(base_reference_values);
	ERR_FAIL_COND_V_MSG(base_reference_encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, base_reference_encoded_bytes, p_main_buffer_view_target, 0, p_deduplicate);
}

PackedByteArray GLTFAccessor::encode_ints_as_bytes(const PackedInt64Array&)
{
	return PackedByteArray();
}

// ============================================================================
// GLTFNode
// ============================================================================

String GLTFNode::get_original_name() { return original_name; }

void GLTFNode::set_original_name(const String& p_name) { original_name = p_name; }

GLTFNodeIndex GLTFNode::get_parent() { return parent; }

void GLTFNode::set_parent(GLTFNodeIndex p_parent) { parent = p_parent; }

int GLTFNode::get_height() { return height; }

void GLTFNode::set_height(int p_height) { height = p_height; }

Transform3D GLTFNode::get_xform() { return transform; }

void GLTFNode::set_xform(const Transform3D& p_xform) { transform = p_xform; }

GLTFMeshIndex GLTFNode::get_mesh() { return mesh; }

void GLTFNode::set_mesh(GLTFMeshIndex p_mesh) { mesh = p_mesh; }

GLTFCameraIndex GLTFNode::get_camera() { return camera; }

void GLTFNode::set_camera(GLTFCameraIndex p_camera) { camera = p_camera; }

GLTFSkinIndex GLTFNode::get_skin() { return skin; }

void GLTFNode::set_skin(GLTFSkinIndex p_skin) { skin = p_skin; }

GLTFSkeletonIndex GLTFNode::get_skeleton() { return skeleton; }

void GLTFNode::set_skeleton(GLTFSkeletonIndex p_skeleton) { skeleton = p_skeleton; }

Vector3 GLTFNode::get_position() { return transform.origin; }

void GLTFNode::set_position(const Vector3& p_position) { transform.origin = p_position; }

Quaternion GLTFNode::get_rotation() { return transform.basis.get_rotation_quaternion(); }

void GLTFNode::set_rotation(const Quaternion& p_rotation)
{
	transform.basis.set_quaternion_scale(p_rotation, transform.basis.get_scale());
}

Vector3 GLTFNode::get_scale() { return transform.basis.get_scale(); }

void GLTFNode::set_scale(const Vector3& p_scale)
{
	transform.basis = transform.basis.orthonormalized() * Basis::from_scale(p_scale);
}

Vector<int> GLTFNode::get_children() { return Vector<int>(children); }

void GLTFNode::set_children(const Vector<int>& p_children) { children = Vector<int>(p_children); }

void GLTFNode::append_child_index(int p_child_index) { children.append(p_child_index); }

GLTFLightIndex GLTFNode::get_light() { return light; }

void GLTFNode::set_light(GLTFLightIndex p_light) { light = p_light; }

bool GLTFNode::get_visible() { return visible; }

void GLTFNode::set_visible(bool p_visible) { visible = p_visible; }

NodePath GLTFNode::get_scene_node_path(Ref<GLTFState> p_state, bool p_handle_skeletons)
{
	ERR_FAIL_COND_V_MSG(
		p_state.is_null(), NodePath(), "Cannot get scene node path because GLTFState is null.");
	Vector<String> path;
	Vector<String> subpath;
	Ref<GLTFNode> current_gltf_node = this;
	const int gltf_node_count = p_state->nodes.size();
	if (p_handle_skeletons && skeleton != -1) {
		subpath.append(get_name());
		path.append("Skeleton3D");
		do {
			const int parent_index = current_gltf_node->get_parent();
			ERR_FAIL_INDEX_V(parent_index, gltf_node_count, NodePath());
			current_gltf_node = p_state->nodes[parent_index];
		} while (current_gltf_node->skeleton != -1);
	}
	const bool is_godot_single_root = p_state->extensions_used.has("GODOT_single_root");
	while (true) {
		const int parent_index = current_gltf_node->get_parent();
		if (is_godot_single_root && parent_index == -1) {
			break;
		}
		path.insert(0, current_gltf_node->get_name());
		if (!is_godot_single_root && parent_index == -1) {
			break;
		}
		ERR_FAIL_INDEX_V(parent_index, gltf_node_count, NodePath());
		current_gltf_node = p_state->nodes[parent_index];
	}
	if (unlikely(path.is_empty())) {
		path.append(".");
	}
	return NodePath(path, subpath, false);
}

// ============================================================================
// GLTFSkeleton
// ============================================================================

Vector<GLTFNodeIndex> GLTFSkeleton::get_joints() { return Vector<GLTFNodeIndex>(joints); }

void GLTFSkeleton::set_joints(const Vector<GLTFNodeIndex>& p_joints)
{
	joints = Vector<GLTFNodeIndex>(p_joints);
}

Vector<GLTFNodeIndex> GLTFSkeleton::get_roots() { return Vector<GLTFNodeIndex>(roots); }

void GLTFSkeleton::set_roots(const Vector<GLTFNodeIndex>& p_roots)
{
	roots = Vector<GLTFNodeIndex>(p_roots);
}

Skeleton3D* GLTFSkeleton::get_godot_skeleton() { return godot_skeleton; }

BoneAttachment3D* GLTFSkeleton::get_bone_attachment(int idx)
{
	ERR_FAIL_INDEX_V(idx, bone_attachments.size(), nullptr);
	return bone_attachments[idx];
}

int32_t GLTFSkeleton::get_bone_attachment_count() { return bone_attachments.size(); }

// ============================================================================
// GLTFSkin
// ============================================================================

GLTFNodeIndex GLTFSkin::get_skin_root() { return skin_root; }

void GLTFSkin::set_skin_root(GLTFNodeIndex p_skin_root) { skin_root = p_skin_root; }

Vector<GLTFNodeIndex> GLTFSkin::get_joints_original() { return joints_original; }

void GLTFSkin::set_joints_original(const Vector<GLTFNodeIndex>& p_joints_original)
{
	joints_original = Vector<GLTFNodeIndex>(p_joints_original);
}

Vector<GLTFNodeIndex> GLTFSkin::get_joints() { return joints; }

void GLTFSkin::set_joints(const Vector<GLTFNodeIndex>& p_joints)
{
	joints = Vector<GLTFNodeIndex>(p_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_non_joints() { return non_joints; }

void GLTFSkin::set_non_joints(const Vector<GLTFNodeIndex>& p_non_joints)
{
	non_joints = Vector<GLTFNodeIndex>(p_non_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_roots() { return roots; }

void GLTFSkin::set_roots(const Vector<GLTFNodeIndex>& p_roots)
{
	roots = Vector<GLTFNodeIndex>(p_roots);
}

int GLTFSkin::get_skeleton() { return skeleton; }

void GLTFSkin::set_skeleton(int p_skeleton) { skeleton = p_skeleton; }

Ref<Skin> GLTFSkin::get_godot_skin() { return godot_skin; }

void GLTFSkin::set_godot_skin(const Ref<Skin>& p_godot_skin) { godot_skin = p_godot_skin; }

// ============================================================================
// GLTFMesh
// ============================================================================

String GLTFMesh::get_original_name() { return original_name; }

void GLTFMesh::set_original_name(const String& p_name) { original_name = p_name; }

Ref<ImporterMesh> GLTFMesh::get_mesh() { return mesh; }

void GLTFMesh::set_mesh(const Ref<ImporterMesh>& p_mesh) { mesh = p_mesh; }

Vector<float> GLTFMesh::get_blend_weights() { return Vector<float>(blend_weights); }

void GLTFMesh::set_blend_weights(const Vector<float>& p_blend_weights)
{
	blend_weights = Vector<float>(p_blend_weights);
}

// ============================================================================
// GLTFCamera
// ============================================================================

void GLTFCamera::set_fov_conversion_expressions(Ref<GLTFObjectModelProperty>& r_obj_model_prop)
{
	// Expression to convert glTF yfov in radians to Godot fov in degrees.
	Ref<Expression> gltf_to_godot_expr;
	gltf_to_godot_expr.instantiate();
	PackedStringArray gltf_to_godot_args = {"yfov_rad"};
	gltf_to_godot_expr->parse("rad_to_deg(yfov_rad)", gltf_to_godot_args);
	r_obj_model_prop->set_gltf_to_godot_expression(gltf_to_godot_expr);
	// Expression to convert Godot fov in degrees to glTF yfov in radians.
	Ref<Expression> godot_to_gltf_expr;
	godot_to_gltf_expr.instantiate();
	PackedStringArray godot_to_gltf_args = {"fov_deg"};
	godot_to_gltf_expr->parse("deg_to_rad(fov_deg)", godot_to_gltf_args);
	r_obj_model_prop->set_godot_to_gltf_expression(godot_to_gltf_expr);
}

Ref<GLTFCamera> GLTFCamera::from_node(const Camera3D* p_camera)
{
	Ref<GLTFCamera> c;
	c.instantiate();
	ERR_FAIL_NULL_V_MSG(p_camera, c,
		"Tried to create a GLTFCamera from a Camera3D node, but the given node was null.");
	c->set_perspective(
		p_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
	// glTF spec (yfov) is in radians, Godot's camera (fov) is in degrees.
	c->set_fov(Math::deg_to_rad(p_camera->get_fov()));
	// glTF spec (xmag and ymag) is a radius in meters, Godot's camera (size) is a diameter in
	// meters.
	c->set_size_mag(p_camera->get_size() * 0.5f);
	c->set_depth_far(p_camera->get_far());
	c->set_depth_near(p_camera->get_near());
	return c;
}

// ============================================================================
// GLTFAnimation
// ============================================================================

GLTFAnimation::Interpolation GLTFAnimation::godot_to_gltf_interpolation(
	const Ref<Animation>& p_godot_animation, int32_t p_godot_anim_track_index)
{
	Animation::InterpolationType interpolation =
		p_godot_animation->track_get_interpolation_type(p_godot_anim_track_index);
	switch (interpolation) {
	case Animation::INTERPOLATION_LINEAR:
	case Animation::INTERPOLATION_LINEAR_ANGLE:
		return INTERP_LINEAR;
	case Animation::INTERPOLATION_NEAREST:
		return INTERP_STEP;
	case Animation::INTERPOLATION_CUBIC:
	case Animation::INTERPOLATION_CUBIC_ANGLE:
		return INTERP_CUBIC_SPLINE;
	}
	return INTERP_LINEAR;
}

Animation::InterpolationType GLTFAnimation::gltf_to_godot_interpolation(
	Interpolation p_gltf_interpolation)
{
	switch (p_gltf_interpolation) {
	case INTERP_LINEAR:
		return Animation::INTERPOLATION_LINEAR;
	case INTERP_STEP:
		return Animation::INTERPOLATION_NEAREST;
	case INTERP_CATMULLROMSPLINE:
	case INTERP_CUBIC_SPLINE:
		return Animation::INTERPOLATION_CUBIC;
	}
	return Animation::INTERPOLATION_LINEAR;
}

String GLTFAnimation::get_original_name() { return original_name; }

void GLTFAnimation::set_original_name(const String& p_name) { original_name = p_name; }

bool GLTFAnimation::get_loop() const { return loop; }

void GLTFAnimation::set_loop(bool p_val) { loop = p_val; }

HashMap<int, GLTFAnimation::NodeTrack>& GLTFAnimation::get_node_tracks() { return node_tracks; }

GLTFAnimation::GLTFAnimation() {}

// ============================================================================
// GLTFObjectModelProperty
// ============================================================================

GLTFAccessor::GLTFAccessorType GLTFObjectModelProperty::get_accessor_type() const
{
	switch (object_model_type) {
	case GLTF_OBJECT_MODEL_TYPE_FLOAT2:
		return GLTFAccessor::TYPE_VEC2;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT3:
		return GLTFAccessor::TYPE_VEC3;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT4:
		return GLTFAccessor::TYPE_VEC4;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT2X2:
		return GLTFAccessor::TYPE_MAT2;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT3X3:
		return GLTFAccessor::TYPE_MAT3;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT4X4:
		return GLTFAccessor::TYPE_MAT4;
	default:
		return GLTFAccessor::TYPE_SCALAR;
	}
}

Ref<Expression> GLTFObjectModelProperty::get_gltf_to_godot_expression() const
{
	return gltf_to_godot_expr;
}

void GLTFObjectModelProperty::set_gltf_to_godot_expression(
	const Ref<Expression>& p_gltf_to_godot_expr)
{
	gltf_to_godot_expr = p_gltf_to_godot_expr;
}

Ref<Expression> GLTFObjectModelProperty::get_godot_to_gltf_expression() const
{
	return godot_to_gltf_expr;
}

void GLTFObjectModelProperty::set_godot_to_gltf_expression(
	const Ref<Expression>& p_godot_to_gltf_expr)
{
	godot_to_gltf_expr = p_godot_to_gltf_expr;
}

GLTFObjectModelProperty::GLTFObjectModelType GLTFObjectModelProperty::get_object_model_type() const
{
	return object_model_type;
}

void GLTFObjectModelProperty::set_object_model_type(GLTFObjectModelType p_type)
{
	object_model_type = p_type;
}

Vector<PackedStringArray> GLTFObjectModelProperty::get_json_pointers() const
{
	return json_pointers;
}

bool GLTFObjectModelProperty::has_json_pointers() const { return !json_pointers.is_empty(); }

void GLTFObjectModelProperty::set_json_pointers(const Vector<PackedStringArray>& p_json_pointers)
{
	json_pointers = p_json_pointers;
}
