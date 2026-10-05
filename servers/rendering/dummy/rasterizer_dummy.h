/**************************************************************************/
/*  rasterizer_dummy.h                                                    */
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
/* without limitation the rights to use, copy, modify, merge, publish,   */
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

#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/dummy/storage/mesh_storage.h"

class RasterizerCanvasDummy;
class RasterizerSceneDummy;

namespace RendererDummy
{
class Fog;
class GI;
class LightStorage;
class MaterialStorage;
class MeshStorage;
class ParticlesStorage;
class TextureStorage;
class Utilities;
} // namespace RendererDummy

class RasterizerDummy final
{
private:
    static inline uint64_t frame = 1;
    static inline double delta = 0.0;
    static inline double time = 0.0;

    static inline RasterizerCanvasDummy* canvas = nullptr;
    static inline RasterizerSceneDummy* scene = nullptr;

    static inline RendererDummy::Fog* fog = nullptr;
    static inline RendererDummy::GI* gi = nullptr;
    static inline RendererDummy::LightStorage* light_storage = nullptr;
    static inline RendererDummy::MaterialStorage* material_storage = nullptr;
    static inline RendererDummy::MeshStorage* mesh_storage = nullptr;
    static inline RendererDummy::ParticlesStorage* particles_storage = nullptr;
    static inline RendererDummy::TextureStorage* texture_storage = nullptr;
    static inline RendererDummy::Utilities* utilities = nullptr;

public:
	static void bind_utilities() {}
    static RendererCanvasRender* get_canvas();
    static RendererSceneRender* get_scene();

    static RendererFog* get_fog();
    static RendererGI* get_gi();
    static RendererLightStorage* get_light_storage();
    static RendererMaterialStorage* get_material_storage();
    static RendererDummy::MeshStorage* get_mesh_storage();
    static RendererParticlesStorage* get_particles_storage();
    static RendererTextureStorage* get_texture_storage();
    static RendererDummy::Utilities* get_utilities();

    static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
        RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true)
    {
    }

    static void initialize() {}

    static void begin_frame(double frame_step)
    {
        frame++;
        delta = frame_step;
        time += frame_step;
    }

    static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
        const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
    {
    }

    _ALWAYS_INLINE_ static bool is_opengl() { return false; }
    _ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}

    static void end_frame(bool p_present);
    static void finalize() {}

    static Error _create_current()
    {
        RasterizerDummy::initialize();
        return OK;
    }

    static void make_current()
    {
        RendererCompositor::_create_func = _create_current;
        RendererCompositor::low_end = false;
        RendererCompositor::bind_compositor<RasterizerDummy>();
    }

    _ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }
    _ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }
    _ALWAYS_INLINE_ static double get_total_time() { return time; }
    _ALWAYS_INLINE_ static bool can_create_resources_async() { return false; }

public:
static void bind_mesh_storage()
    {
        RendererMeshStorage::mesh_allocate = &RendererDummy::MeshStorage::mesh_allocate;
        RendererMeshStorage::mesh_initialize = &RendererDummy::MeshStorage::mesh_initialize;
        RendererMeshStorage::mesh_free = &RendererDummy::MeshStorage::mesh_free;
        RendererMeshStorage::mesh_set_blend_shape_count = &RendererDummy::MeshStorage::mesh_set_blend_shape_count;
        RendererMeshStorage::mesh_needs_instance = &RendererDummy::MeshStorage::mesh_needs_instance;
        RendererMeshStorage::mesh_add_surface = &RendererDummy::MeshStorage::mesh_add_surface;
        RendererMeshStorage::mesh_get_blend_shape_count = &RendererDummy::MeshStorage::mesh_get_blend_shape_count;
        RendererMeshStorage::mesh_set_blend_shape_mode = &RendererDummy::MeshStorage::mesh_set_blend_shape_mode;
        RendererMeshStorage::mesh_get_blend_shape_mode = &RendererDummy::MeshStorage::mesh_get_blend_shape_mode;
        RendererMeshStorage::mesh_surface_update_vertex_region = &RendererDummy::MeshStorage::mesh_surface_update_vertex_region;
        RendererMeshStorage::mesh_surface_update_attribute_region = &RendererDummy::MeshStorage::mesh_surface_update_attribute_region;
        RendererMeshStorage::mesh_surface_update_skin_region = &RendererDummy::MeshStorage::mesh_surface_update_skin_region;
        RendererMeshStorage::mesh_surface_update_index_region = &RendererDummy::MeshStorage::mesh_surface_update_index_region;
        RendererMeshStorage::mesh_surface_set_material = &RendererDummy::MeshStorage::mesh_surface_set_material;
        RendererMeshStorage::mesh_surface_get_material = &RendererDummy::MeshStorage::mesh_surface_get_material;
        RendererMeshStorage::mesh_get_surface = &RendererDummy::MeshStorage::mesh_get_surface;
        RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid = &RendererDummy::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
        RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid = &RendererDummy::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
        RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid = &RendererDummy::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
        RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid = &RendererDummy::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
        RendererMeshStorage::mesh_get_surface_count = &RendererDummy::MeshStorage::mesh_get_surface_count;
        RendererMeshStorage::mesh_set_custom_aabb = &RendererDummy::MeshStorage::mesh_set_custom_aabb;
        RendererMeshStorage::mesh_get_custom_aabb = &RendererDummy::MeshStorage::mesh_get_custom_aabb;
        RendererMeshStorage::mesh_get_aabb = &RendererDummy::MeshStorage::mesh_get_aabb;
        RendererMeshStorage::mesh_set_path = &RendererDummy::MeshStorage::mesh_set_path;
        RendererMeshStorage::mesh_get_path = &RendererDummy::MeshStorage::mesh_get_path;
        RendererMeshStorage::mesh_set_shadow_mesh = &RendererDummy::MeshStorage::mesh_set_shadow_mesh;
        RendererMeshStorage::mesh_clear = &RendererDummy::MeshStorage::mesh_clear;
        RendererMeshStorage::mesh_surface_remove = &RendererDummy::MeshStorage::mesh_surface_remove;
        RendererMeshStorage::mesh_debug_usage = &RendererDummy::MeshStorage::mesh_debug_usage;

        RendererMeshStorage::mesh_instance_create = &RendererDummy::MeshStorage::mesh_instance_create;
        RendererMeshStorage::mesh_instance_free = &RendererDummy::MeshStorage::mesh_instance_free;
        RendererMeshStorage::mesh_instance_set_skeleton = &RendererDummy::MeshStorage::mesh_instance_set_skeleton;
        RendererMeshStorage::mesh_instance_set_blend_shape_weight = &RendererDummy::MeshStorage::mesh_instance_set_blend_shape_weight;
        RendererMeshStorage::mesh_instance_check_for_update = &RendererDummy::MeshStorage::mesh_instance_check_for_update;
        RendererMeshStorage::mesh_instance_set_canvas_item_transform = &RendererDummy::MeshStorage::mesh_instance_set_canvas_item_transform;
        RendererMeshStorage::update_mesh_instances = &RendererDummy::MeshStorage::update_mesh_instances;

        RendererMeshStorage::_multimesh_allocate = &RendererDummy::MeshStorage::_multimesh_allocate;
        RendererMeshStorage::_multimesh_initialize = &RendererDummy::MeshStorage::_multimesh_initialize;
        RendererMeshStorage::_multimesh_free = &RendererDummy::MeshStorage::_multimesh_free;
        RendererMeshStorage::_multimesh_allocate_data = &RendererDummy::MeshStorage::_multimesh_allocate_data;
        RendererMeshStorage::_multimesh_get_instance_count = &RendererDummy::MeshStorage::_multimesh_get_instance_count;
        RendererMeshStorage::_multimesh_set_mesh = &RendererDummy::MeshStorage::_multimesh_set_mesh;
        RendererMeshStorage::_multimesh_instance_set_transform = &RendererDummy::MeshStorage::_multimesh_instance_set_transform;
        RendererMeshStorage::_multimesh_instance_set_transform_2d = &RendererDummy::MeshStorage::_multimesh_instance_set_transform_2d;
        RendererMeshStorage::_multimesh_instance_set_color = &RendererDummy::MeshStorage::_multimesh_instance_set_color;
        RendererMeshStorage::_multimesh_instance_set_custom_data = &RendererDummy::MeshStorage::_multimesh_instance_set_custom_data;
        RendererMeshStorage::_multimesh_set_custom_aabb = &RendererDummy::MeshStorage::_multimesh_set_custom_aabb;
        RendererMeshStorage::_multimesh_get_custom_aabb = &RendererDummy::MeshStorage::_multimesh_get_custom_aabb;
        RendererMeshStorage::_multimesh_get_mesh = &RendererDummy::MeshStorage::_multimesh_get_mesh;
        RendererMeshStorage::_multimesh_instance_get_transform = &RendererDummy::MeshStorage::_multimesh_instance_get_transform;
        RendererMeshStorage::_multimesh_instance_get_transform_2d = &RendererDummy::MeshStorage::_multimesh_instance_get_transform_2d;
        RendererMeshStorage::_multimesh_instance_get_color = &RendererDummy::MeshStorage::_multimesh_instance_get_color;
        RendererMeshStorage::_multimesh_instance_get_custom_data = &RendererDummy::MeshStorage::_multimesh_instance_get_custom_data;
        RendererMeshStorage::_multimesh_set_buffer = &RendererDummy::MeshStorage::_multimesh_set_buffer;
        RendererMeshStorage::_multimesh_get_command_buffer_rd_rid = &RendererDummy::MeshStorage::_multimesh_get_command_buffer_rd_rid;
        RendererMeshStorage::_multimesh_get_buffer_rd_rid = &RendererDummy::MeshStorage::_multimesh_get_buffer_rd_rid;
        RendererMeshStorage::_multimesh_get_buffer = &RendererDummy::MeshStorage::_multimesh_get_buffer;
        RendererMeshStorage::_multimesh_set_visible_instances = &RendererDummy::MeshStorage::_multimesh_set_visible_instances;
        RendererMeshStorage::_multimesh_get_visible_instances = &RendererDummy::MeshStorage::_multimesh_get_visible_instances;
        RendererMeshStorage::_multimesh_get_aabb = &RendererDummy::MeshStorage::_multimesh_get_aabb;
        RendererMeshStorage::_multimesh_get_interpolator = &RendererDummy::MeshStorage::_multimesh_get_interpolator;

        RendererMeshStorage::skeleton_allocate = &RendererDummy::MeshStorage::skeleton_allocate;
        RendererMeshStorage::skeleton_initialize = &RendererDummy::MeshStorage::skeleton_initialize;
        RendererMeshStorage::skeleton_free = &RendererDummy::MeshStorage::skeleton_free;
        RendererMeshStorage::skeleton_allocate_data = &RendererDummy::MeshStorage::skeleton_allocate_data;
        RendererMeshStorage::skeleton_set_base_transform_2d = &RendererDummy::MeshStorage::skeleton_set_base_transform_2d;
        RendererMeshStorage::skeleton_get_bone_count = &RendererDummy::MeshStorage::skeleton_get_bone_count;
        RendererMeshStorage::skeleton_bone_set_transform = &RendererDummy::MeshStorage::skeleton_bone_set_transform;
        RendererMeshStorage::skeleton_bone_get_transform = &RendererDummy::MeshStorage::skeleton_bone_get_transform;
        RendererMeshStorage::skeleton_bone_set_transform_2d = &RendererDummy::MeshStorage::skeleton_bone_set_transform_2d;
        RendererMeshStorage::skeleton_bone_get_transform_2d = &RendererDummy::MeshStorage::skeleton_bone_get_transform_2d;
        RendererMeshStorage::skeleton_update_dependency = &RendererDummy::MeshStorage::skeleton_update_dependency;
    }

    RasterizerDummy() = delete;
    RasterizerDummy(const RasterizerDummy&) = delete;
    ~RasterizerDummy() = delete;
};

using RDummy = RasterizerDummy;


