/**************************************************************************/
/*  renderer_geometry_instance.cpp                                        */
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

#include "servers/rendering/renderer_geometry_instance.h"

void RenderGeometryInstance::set_skeleton(RID p_skeleton) {
	if (!data) {
		return;
	}
	data->skeleton = p_skeleton;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_override(RID p_override) {
	if (!data) {
		return;
	}
	data->material_override = p_override;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_overlay(RID p_overlay) {
	if (!data) {
		return;
	}
	data->material_overlay = p_overlay;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_surface_materials(const Vector<RID> &p_materials) {
	if (!data) {
		return;
	}
	data->surface_materials = p_materials;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_mesh_instance(RID p_mesh_instance) {
	mesh_instance = p_mesh_instance;

	_mark_dirty();
}

void RenderGeometryInstance::set_transform(const Transform3D &p_transform, const AABB &p_aabb, const AABB &p_transformed_aabb) {
	transform = p_transform;
	mirror = p_transform.basis.determinant() < 0;
	if (data) {
		data->aabb = p_aabb;
	}
	transformed_aabb = p_transformed_aabb;

	Vector3 model_scale_vec = p_transform.basis.get_scale_abs();
	// handle non uniform scale here

	float max_scale = MAX(model_scale_vec.x, MAX(model_scale_vec.y, model_scale_vec.z));
	float min_scale = MIN(model_scale_vec.x, MIN(model_scale_vec.y, model_scale_vec.z));
	non_uniform_scale = max_scale >= 0.0 && (min_scale / max_scale) < 0.999;

	lod_model_scale = max_scale;
}

void RenderGeometryInstance::set_pivot_data(float p_sorting_offset, bool p_use_aabb_center) {
	sorting_offset = p_sorting_offset;
	use_aabb_center = p_use_aabb_center;
}

void RenderGeometryInstance::set_lod_bias(float p_lod_bias) {
	lod_bias = p_lod_bias;
}

void RenderGeometryInstance::set_layer_mask(uint32_t p_layer_mask) {
	layer_mask = p_layer_mask;
}

void RenderGeometryInstance::set_fade_range(bool p_enable_near, float p_near_begin, float p_near_end, bool p_enable_far, float p_far_begin, float p_far_end) {
	fade_near = p_enable_near;
	fade_near_begin = p_near_begin;
	fade_near_end = p_near_end;
	fade_far = p_enable_far;
	fade_far_begin = p_far_begin;
	fade_far_end = p_far_end;
}

void RenderGeometryInstance::set_parent_fade_alpha(float p_alpha) {
	parent_fade_alpha = p_alpha;
}

void RenderGeometryInstance::set_transparency(float p_transparency) {
	force_alpha = CLAMP(1.0 - p_transparency, 0, 1);
}

void RenderGeometryInstance::set_use_baked_light(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_baked_light = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_use_dynamic_gi(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_dynamic_gi = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_instance_shader_uniforms_offset(int32_t p_offset) {
	shader_uniforms_offset = p_offset;

	_mark_dirty();
}

void RenderGeometryInstance::set_cast_double_sided_shadows(bool p_enable) {
	if (!data) {
		return;
	}
	data->cast_double_sided_shadows = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::reset_motion_vectors() {
}

Transform3D RenderGeometryInstance::get_transform() {
	return transform;
}

AABB RenderGeometryInstance::get_aabb() {
	return data ? data->aabb : AABB();
}
