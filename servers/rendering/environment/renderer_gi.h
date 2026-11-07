/**************************************************************************/
/*  renderer_gi.h                                                         */
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

#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererGI
{
public:
	/* VOXEL GI FUNCTION POINTERS */
	inline static bool (*owns_voxel_gi)(RID p_rid) = nullptr;
	inline static RID (*voxel_gi_allocate)() = nullptr;
	inline static void (*voxel_gi_free)(RID p_voxel_gi) = nullptr;
	inline static void (*voxel_gi_initialize)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_allocate_data)(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts) = nullptr;

	inline static AABB (*voxel_gi_get_bounds)(RID p_voxel_gi) = nullptr;
	inline static Vector3i (*voxel_gi_get_octree_size)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_octree_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_data_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_distance_field)(RID p_voxel_gi) = nullptr;
	inline static Vector<int> (*voxel_gi_get_level_counts)(RID p_voxel_gi) = nullptr;
	inline static Transform3D (*voxel_gi_get_to_cell_xform)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_dynamic_range)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_dynamic_range)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_propagation)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_propagation)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_energy)(RID p_voxel_gi, float p_energy) = nullptr;
	inline static float (*voxel_gi_get_energy)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_baked_exposure_normalization)(
		RID p_voxel_gi, float p_baked_exposure) = nullptr;
	inline static float (*voxel_gi_get_baked_exposure_normalization)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_bias)(RID p_voxel_gi, float p_bias) = nullptr;
	inline static float (*voxel_gi_get_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_normal_bias)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_normal_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_interior)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_interior)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_use_two_bounces)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_using_two_bounces)(RID p_voxel_gi) = nullptr;

	inline static uint32_t (*voxel_gi_get_version)(RID p_probe) = nullptr;

	/* SDFGI FUNCTION POINTERS */
	inline static void (*sdfgi_reset)() = nullptr;

	/* BIND TEMPLATE */
	template <typename T> static void bind()
	{
		owns_voxel_gi = T::owns_voxel_gi;
		voxel_gi_allocate = T::voxel_gi_allocate;
		voxel_gi_free = T::voxel_gi_free;
		voxel_gi_initialize = T::voxel_gi_initialize;
		voxel_gi_allocate_data = T::voxel_gi_allocate_data;
		voxel_gi_get_bounds = T::voxel_gi_get_bounds;
		voxel_gi_get_octree_size = T::voxel_gi_get_octree_size;
		voxel_gi_get_octree_cells = T::voxel_gi_get_octree_cells;
		voxel_gi_get_data_cells = T::voxel_gi_get_data_cells;
		voxel_gi_get_distance_field = T::voxel_gi_get_distance_field;
		voxel_gi_get_level_counts = T::voxel_gi_get_level_counts;
		voxel_gi_get_to_cell_xform = T::voxel_gi_get_to_cell_xform;
		voxel_gi_set_dynamic_range = T::voxel_gi_set_dynamic_range;
		voxel_gi_get_dynamic_range = T::voxel_gi_get_dynamic_range;
		voxel_gi_set_propagation = T::voxel_gi_set_propagation;
		voxel_gi_get_propagation = T::voxel_gi_get_propagation;
		voxel_gi_set_energy = T::voxel_gi_set_energy;
		voxel_gi_get_energy = T::voxel_gi_get_energy;
		voxel_gi_set_baked_exposure_normalization = T::voxel_gi_set_baked_exposure_normalization;
		voxel_gi_get_baked_exposure_normalization = T::voxel_gi_get_baked_exposure_normalization;
		voxel_gi_set_bias = T::voxel_gi_set_bias;
		voxel_gi_get_bias = T::voxel_gi_get_bias;
		voxel_gi_set_normal_bias = T::voxel_gi_set_normal_bias;
		voxel_gi_get_normal_bias = T::voxel_gi_get_normal_bias;
		voxel_gi_set_interior = T::voxel_gi_set_interior;
		voxel_gi_is_interior = T::voxel_gi_is_interior;
		voxel_gi_set_use_two_bounces = T::voxel_gi_set_use_two_bounces;
		voxel_gi_is_using_two_bounces = T::voxel_gi_is_using_two_bounces;
		voxel_gi_get_version = T::voxel_gi_get_version;
		sdfgi_reset = T::sdfgi_reset;
	}
};
