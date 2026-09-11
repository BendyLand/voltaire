/**************************************************************************/
/*  jolt_height_map_shape_3d.cpp                                          */
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

#include "../misc/jolt_type_conversions.h"
#include "jolt_height_map_shape_3d.h"
// must stay last
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace
{

bool _is_vertex_hole(const JPH::VertexList& p_vertices, int p_index)
{
	const float height = p_vertices[(size_t)p_index].y;
	return height == FLT_MAX || Math::is_nan(height);
}

bool _is_triangle_hole(const JPH::VertexList& p_vertices, int p_index0, int p_index1, int p_index2)
{
	return _is_vertex_hole(p_vertices, p_index0) || _is_vertex_hole(p_vertices, p_index1) ||
		   _is_vertex_hole(p_vertices, p_index2);
}

} // namespace

JPH::ShapeRefC JoltHeightMapShape3D::_build() const
{
	const int height_count = (int)heights.size();
	if (unlikely(height_count == 0)) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(height_count != width * depth, nullptr,
		vformat("Failed to build Jolt Physics height map shape with %s. Height count must be the "
				"product of width and depth. This shape belongs to %s.",
			to_string(), _owners_to_string()));
	ERR_FAIL_COND_V_MSG(width < 2 || depth < 2, nullptr,
		vformat("Failed to build Jolt Physics height map shape with %s. The height map must be at "
				"least 2x2. This shape belongs to %s.",
			to_string(), _owners_to_string()));

	if (width != depth) {
		return JoltShape3D::with_double_sided(_build_mesh(), true);
	}

	const int block_size = 2; // Default of JPH::HeightFieldShapeSettings::mBlockSize
	const int block_count = width / block_size;

	if (block_count < 2) {
		return JoltShape3D::with_double_sided(_build_mesh(), true);
	}

	return JoltShape3D::with_double_sided(_build_height_field(), true);
}

AABB JoltHeightMapShape3D::_calculate_aabb() const
{
	AABB result;

	const int quad_count_x = width - 1;
	const int quad_count_z = depth - 1;

	const float offset_x = (float)-quad_count_x / 2.0f;
	const float offset_z = (float)-quad_count_z / 2.0f;

	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const Vector3 vertex(
				offset_x + (float)x, (float)heights[z * width + x], offset_z + (float)z);

			if (x == 0 && z == 0) {
				result.position = vertex;
			}
			else {
				result.expand_to(vertex);
			}
		}
	}

	return result;
}

String JoltHeightMapShape3D::to_string() const
{
	return vformat("{height_count=%d width=%d depth=%d}", heights.size(), width, depth);
}


