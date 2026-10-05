/**************************************************************************/
/*  mesh_storage.h                                                        */
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

#include "core/math/color.h"
#include "core/math/transform_2d.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"

struct InterpolationData
{
    void notify_free_multimesh(RID p_rid);
    LocalVector<RID> multimesh_interpolate_update_list;
    LocalVector<RID> multimesh_transform_update_lists[2];
    LocalVector<RID>* multimesh_transform_update_list_curr =
        &multimesh_transform_update_lists[0];
    LocalVector<RID>* multimesh_transform_update_list_prev =
        &multimesh_transform_update_lists[1];
};

class RendererMeshStorage final
{
public:
    RendererMeshStorage() = delete;
    RendererMeshStorage(const RendererMeshStorage&) = delete;
    RendererMeshStorage& operator=(const RendererMeshStorage&) = delete;
    ~RendererMeshStorage() = delete;

    /* MESH API */

    inline static RID (*mesh_allocate)() = nullptr;
    inline static void (*mesh_initialize)(RID p_rid) = nullptr;
    inline static void (*mesh_free)(RID p_rid) = nullptr;

    inline static void (*mesh_set_blend_shape_count)(RID p_mesh, int p_blend_shape_count) = nullptr;
    inline static bool (*mesh_needs_instance)(RID p_mesh, bool p_has_skeleton) = nullptr;

    inline static void (*mesh_add_surface)(
        RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface) = nullptr;

    inline static int (*mesh_get_blend_shape_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_blend_shape_mode)(RID p_mesh, RSE::BlendShapeMode p_mode) = nullptr;
    inline static RSE::BlendShapeMode (*mesh_get_blend_shape_mode)(RID p_mesh) = nullptr;

    inline static void (*mesh_surface_update_vertex_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_attribute_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_skin_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_index_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;

    inline static void (*mesh_surface_set_material)(RID p_mesh, int p_surface, RID p_material) = nullptr;
    inline static RID (*mesh_surface_get_material)(RID p_mesh, int p_surface) = nullptr;

    inline static RenderingServerTypes::SurfaceData (*mesh_get_surface)(RID p_mesh, int p_surface) = nullptr;

    inline static RID (*mesh_surface_get_vertex_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_attribute_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_skin_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_index_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;

    inline static int (*mesh_get_surface_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_custom_aabb)(RID p_mesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*mesh_get_custom_aabb)(RID p_mesh) = nullptr;
    inline static AABB (*mesh_get_aabb)(RID p_mesh, RID p_skeleton) = nullptr;

    inline static void (*mesh_set_path)(RID p_mesh, const String& p_path) = nullptr;
    inline static String (*mesh_get_path)(RID p_mesh) = nullptr;
    inline static void (*mesh_set_shadow_mesh)(RID p_mesh, RID p_shadow_mesh) = nullptr;

    inline static void (*mesh_clear)(RID p_mesh) = nullptr;
    inline static void (*mesh_surface_remove)(RID p_mesh, int p_surface) = nullptr;
    inline static void (*mesh_debug_usage)(List<RenderingServerTypes::MeshInfo>* r_info) = nullptr;

    /* MESH INSTANCE API */

    inline static RID (*mesh_instance_create)(RID p_base) = nullptr;
    inline static void (*mesh_instance_free)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_skeleton)(RID p_mesh_instance, RID p_skeleton) = nullptr;
    inline static void (*mesh_instance_set_blend_shape_weight)(
        RID p_mesh_instance, int p_shape, float p_weight) = nullptr;
    inline static void (*mesh_instance_check_for_update)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_canvas_item_transform)(
        RID p_mesh_instance, const Transform2D& p_transform) = nullptr;
    inline static void (*update_mesh_instances)() = nullptr;

    /* MULTIMESH STRUCTS & BACKEND DISPATCH HOOKS */

    struct MultiMeshInterpolator
    {
        RSE::MultimeshTransformFormat _transform_format = RSE::MULTIMESH_TRANSFORM_3D;
        bool _use_colors = false;
        bool _use_custom_data = false;

        int _stride = 0;
        int _vf_size_xform = 0;
        int _vf_size_color = 0;
        int _vf_size_data = 0;
        int _num_instances = 0;

        int quality = 0;
        bool interpolated = false;
        bool on_interpolate_update_list = false;
        bool on_transform_update_list = false;

        Vector<float> _data_prev;
        Vector<float> _data_curr;
        Vector<float> _data_interpolated;
    };

    inline static RID (*_multimesh_allocate)() = nullptr;
    inline static void (*_multimesh_initialize)(RID p_rid) = nullptr;
    inline static void (*_multimesh_free)(RID p_rid) = nullptr;

    inline static void (*_multimesh_allocate_data)(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors,
        bool p_use_custom_data, bool p_use_indirect) = nullptr;

    inline static int (*_multimesh_get_instance_count)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_mesh)(RID p_multimesh, RID p_mesh) = nullptr;
    inline static void (*_multimesh_instance_set_transform)(
        RID p_multimesh, int p_index, const Transform3D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_transform_2d)(
        RID p_multimesh, int p_index, const Transform2D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_color)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;
    inline static void (*_multimesh_instance_set_custom_data)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;

    inline static void (*_multimesh_set_custom_aabb)(RID p_multimesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*_multimesh_get_custom_aabb)(RID p_multimesh) = nullptr;

    inline static RID (*_multimesh_get_mesh)(RID p_multimesh) = nullptr;

    inline static Transform3D (*_multimesh_instance_get_transform)(RID p_multimesh, int p_index) = nullptr;
    inline static Transform2D (*_multimesh_instance_get_transform_2d)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_color)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_custom_data)(RID p_multimesh, int p_index) = nullptr;

    inline static void (*_multimesh_set_buffer)(RID p_multimesh, const Vector<float>& p_buffer) = nullptr;
    inline static RID (*_multimesh_get_command_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static RID (*_multimesh_get_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static Vector<float> (*_multimesh_get_buffer)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_visible_instances)(RID p_multimesh, int p_visible) = nullptr;
    inline static int (*_multimesh_get_visible_instances)(RID p_multimesh) = nullptr;

    inline static AABB (*_multimesh_get_aabb)(RID p_multimesh) = nullptr;

    inline static MultiMeshInterpolator* (*_multimesh_get_interpolator)(RID p_multimesh) = nullptr;

    /* MULTIMESH PUBLIC FRONTEND API */

    static _FORCE_INLINE_ RID multimesh_allocate() { return _multimesh_allocate(); }
    static _FORCE_INLINE_ void multimesh_initialize(RID p_rid) { _multimesh_initialize(p_rid); }
    static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }

    static void multimesh_allocate_data(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
        bool p_use_custom_data = false, bool p_use_indirect = false);

    static int multimesh_get_instance_count(RID p_multimesh);

    static void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
    static void multimesh_instance_set_transform(
        RID p_multimesh, int p_index, const Transform3D& p_transform);
    static void multimesh_instance_set_transform_2d(
        RID p_multimesh, int p_index, const Transform2D& p_transform);
    static void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
    static void multimesh_instance_set_custom_data(
        RID p_multimesh, int p_index, const Color& p_color);

    static void multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb);
    static AABB multimesh_get_custom_aabb(RID p_multimesh);

    static RID multimesh_get_mesh(RID p_multimesh);

    static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
    static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

    static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
    static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
    static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
    static Vector<float> multimesh_get_buffer(RID p_multimesh);

    static void multimesh_set_buffer_interpolated(
        RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev);
    static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
    static void multimesh_set_physics_interpolation_quality(
        RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
    static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
    static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

    static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
    static int multimesh_get_visible_instances(RID p_multimesh);

    static AABB multimesh_get_aabb(RID p_multimesh);

private:
    static void _multimesh_add_to_interpolation_lists(RID p_multimesh, MultiMeshInterpolator& r_mmi);

public:
    /* SKELETON API */

    inline static RID (*skeleton_allocate)() = nullptr;
    inline static void (*skeleton_initialize)(RID p_rid) = nullptr;
    inline static void (*skeleton_free)(RID p_rid) = nullptr;

    inline static void (*skeleton_allocate_data)(
        RID p_skeleton, int p_bones, bool p_2d_skeleton) = nullptr;
    inline static void (*skeleton_set_base_transform_2d)(
        RID p_skeleton, const Transform2D& p_base_transform) = nullptr;
    inline static int (*skeleton_get_bone_count)(RID p_skeleton) = nullptr;
    inline static void (*skeleton_bone_set_transform)(
        RID p_skeleton, int p_bone, const Transform3D& p_transform) = nullptr;
    inline static Transform3D (*skeleton_bone_get_transform)(RID p_skeleton, int p_bone) = nullptr;
    inline static void (*skeleton_bone_set_transform_2d)(
        RID p_skeleton, int p_bone, const Transform2D& p_transform) = nullptr;
    inline static Transform2D (*skeleton_bone_get_transform_2d)(RID p_skeleton, int p_bone) = nullptr;

    inline static void (*skeleton_update_dependency)(RID p_base, DependencyTracker* p_instance) = nullptr;

    /* INTERPOLATION */

    inline static InterpolationData _interpolation_data;

    static void update_interpolation_tick(bool p_process = true);
    static void update_interpolation_frame(bool p_process = true);
};
