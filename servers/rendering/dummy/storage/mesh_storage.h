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

#include "core/templates/rid_owner.h"
#include "servers/rendering/storage/mesh_storage.h"

namespace RendererDummy
{

struct DummyMesh
{
    Vector<RenderingServerTypes::SurfaceData> surfaces;
    int blend_shape_count = 0;
    RSE::BlendShapeMode blend_shape_mode = RSE::BLEND_SHAPE_MODE_NORMALIZED;
    PackedFloat32Array blend_shape_values;
    Dependency dependency;
};

class MeshStorage final
{
private:
    static inline RID_Owner<DummyMesh> mesh_owner;

    struct DummyMultiMesh
    {
        PackedFloat32Array buffer;
    };

    static inline RID_Owner<DummyMultiMesh> multimesh_owner;

public:
    MeshStorage() = delete;
    MeshStorage(const MeshStorage&) = delete;
    MeshStorage& operator=(const MeshStorage&) = delete;
    ~MeshStorage() = delete;

    /* MESH API */
    static _FORCE_INLINE_ DummyMesh* get_mesh(RID p_rid) { return mesh_owner.get_or_null(p_rid); }
    static bool owns_mesh(RID p_rid) { return mesh_owner.owns(p_rid); }

    static RID mesh_allocate() { return mesh_owner.allocate_rid(); }
    static void mesh_initialize(RID p_rid) { mesh_owner.initialize_rid(p_rid, DummyMesh()); }
    static void mesh_free(RID p_rid) { mesh_owner.free(p_rid); }

    static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        m->blend_shape_count = p_blend_shape_count;
    }

    static bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton) { return false; }

    static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        m->surfaces.push_back(p_surface);
    }

    static int mesh_get_blend_shape_count(RID p_mesh)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL_V(m, 0);
        return m->blend_shape_count;
    }

    static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        m->blend_shape_mode = p_mode;
    }

    static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL_V(m, RSE::BLEND_SHAPE_MODE_NORMALIZED);
        return m->blend_shape_mode;
    }

    static void mesh_surface_update_vertex_region(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) {}
    static void mesh_surface_update_attribute_region(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) {}
    static void mesh_surface_update_skin_region(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) {}
    static void mesh_surface_update_index_region(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) {}

    static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        ERR_FAIL_INDEX(p_surface, m->surfaces.size());
        m->surfaces.write[p_surface].material = p_material;
    }

    static RID mesh_surface_get_material(RID p_mesh, int p_surface)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL_V(m, RID());
        ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RID());
        return m->surfaces[p_surface].material;
    }

    static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL_V(m, RenderingServerTypes::SurfaceData());
        ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RenderingServerTypes::SurfaceData());
        return m->surfaces[p_surface];
    }

    static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
    static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
    static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
    static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }

    static int mesh_get_surface_count(RID p_mesh)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL_V(m, 0);
        return m->surfaces.size();
    }

    static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb) {}
    static AABB mesh_get_custom_aabb(RID p_mesh) { return AABB(); }
    static AABB mesh_get_aabb(RID p_mesh, RID p_skeleton) { return AABB(); }

    static void mesh_set_path(RID p_mesh, const String& p_path) {}
    static String mesh_get_path(RID p_mesh) { return String(); }
    static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh) {}

    static void mesh_clear(RID p_mesh)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        m->surfaces.clear();
    }

    static void mesh_surface_remove(RID p_mesh, int p_surface)
    {
        DummyMesh* m = mesh_owner.get_or_null(p_mesh);
        ERR_FAIL_NULL(m);
        ERR_FAIL_INDEX(p_surface, m->surfaces.size());
        m->surfaces.remove_at(p_surface);
    }

    static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info) {}

    /* MESH INSTANCE API */

    static RID mesh_instance_create(RID p_base) { return RID(); }
    static void mesh_instance_free(RID p_rid) {}
    static void mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton) {}
    static void mesh_instance_set_blend_shape_weight(RID p_mesh_instance, int p_shape, float p_weight) {}
    static void mesh_instance_check_for_update(RID p_mesh_instance) {}
    static void mesh_instance_set_canvas_item_transform(
        RID p_mesh_instance, const Transform2D& p_transform) {}
    static void update_mesh_instances() {}

    /* MULTIMESH API */
    static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }
    static bool owns_multimesh(RID p_rid) { return multimesh_owner.owns(p_rid); }

    static RID _multimesh_allocate() { return multimesh_owner.allocate_rid(); }
    static void _multimesh_initialize(RID p_rid) { multimesh_owner.initialize_rid(p_rid, DummyMultiMesh()); }
    static void _multimesh_free(RID p_rid) { multimesh_owner.free(p_rid); }

    static void _multimesh_allocate_data(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
        bool p_use_custom_data = false, bool p_use_indirect = false) {}
    static int _multimesh_get_instance_count(RID p_multimesh) { return 0; }

    static void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) {}
    static void _multimesh_instance_set_transform(
        RID p_multimesh, int p_index, const Transform3D& p_transform) {}
    static void _multimesh_instance_set_transform_2d(
        RID p_multimesh, int p_index, const Transform2D& p_transform) {}
    static void _multimesh_instance_set_color(
        RID p_multimesh, int p_index, const Color& p_color) {}
    static void _multimesh_instance_set_custom_data(
        RID p_multimesh, int p_index, const Color& p_color) {}

    static void _multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb) {}
    static AABB _multimesh_get_custom_aabb(RID p_multimesh) { return AABB(); }
    static RID _multimesh_get_mesh(RID p_multimesh) { return RID(); }

    static Transform3D _multimesh_instance_get_transform(RID p_multimesh, int p_index) { return Transform3D(); }
    static Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) { return Transform2D(); }
    static Color _multimesh_instance_get_color(RID p_multimesh, int p_index) { return Color(); }
    static Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index) { return Color(); }

    static void _multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer) {}
    static RID _multimesh_get_command_buffer_rd_rid(RID p_multimesh) { return RID(); }
    static RID _multimesh_get_buffer_rd_rid(RID p_multimesh) { return RID(); }
    static Vector<float> _multimesh_get_buffer(RID p_multimesh) { return Vector<float>(); }

    static void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}
    static int _multimesh_get_visible_instances(RID p_multimesh) { return 0; }

    static AABB _multimesh_get_aabb(RID p_multimesh) { return AABB(); }

    static RendererMeshStorage::MultiMeshInterpolator* _multimesh_get_interpolator(RID p_multimesh) { return nullptr; }

    /* SKELETON API */

    static RID skeleton_allocate() { return RID(); }
    static void skeleton_initialize(RID p_rid) {}
    static void skeleton_free(RID p_rid) {}

    static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) {}
    static void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform) {}
    static int skeleton_get_bone_count(RID p_skeleton) { return 0; }
    static void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform3D& p_transform) {}
    static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone) { return Transform3D(); }
    static void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D& p_transform) {}
    static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone) { return Transform2D(); }

    static void skeleton_update_dependency(RID p_base, DependencyTracker* p_instance) {}
};

} // namespace RendererDummy
