/**************************************************************************/
/*  rendering_server.h                                                    */
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

#include "core/io/image.h"
#include "core/os/thread_safe.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "servers/display/display_server_enums.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_device_enums.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

namespace Geometry3D
{
struct MeshData;
}

#ifdef DEBUG_ENABLED
#define ERR_NOT_ON_RENDER_THREAD ERR_FAIL_COND(!RenderingServer::is_on_render_thread());
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret)                                                          \
	ERR_FAIL_COND_V(!RenderingServer::is_on_render_thread(), m_ret);
#else
#define ERR_NOT_ON_RENDER_THREAD
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret)
#endif

class RenderingDevice;

class RenderingServer final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	static inline const Vector2 SMALL_VEC2 = Vector2(CMP_EPSILON, CMP_EPSILON);
	static inline const Vector3 SMALL_VEC3 = Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON);

private:
	struct Data
	{
		struct Backend
		{
			enum
			{
				MAX_INSTANCE_CULL = 8192,
				MAX_INSTANCE_LIGHTS = 4,
				LIGHT_CACHE_DIRTY = -1,
				MAX_LIGHTS_CULLED = 256,
				MAX_ROOM_CULL = 32,
				MAX_EXTERIOR_PORTALS = 128,
				MAX_LIGHT_SAMPLERS = 256,
				INSTANCE_ROOMLESS_MASK = (1 << 20)
			};

			// Active subsystem driver
			RendererCompositor* compositor = nullptr;

			// Frame profiling & debug tracking
			uint64_t frame_profile_frame = 0;
			Vector<RenderingServerTypes::FrameProfileArea> frame_profile;
			double frame_setup_time = 0.0;
			bool print_gpu_profile = false;
			HashMap<String, float> print_gpu_profile_task_time;
			uint64_t print_frame_profile_ticks_from = 0;
			uint32_t print_frame_profile_frame_count = 0;

			int changes = 0;
			RID test_cube;
		};

		int mm_policy = 0;
		bool render_loop_enabled = true;

		RID test_texture;
		RID white_texture;
		RID test_material;

		Backend backend;
	};

	static RID _make_test_cube();
	static void _free_internal_rids();

public:
	static inline Data* data = nullptr;

	static void fix_surface_compatibility(
		RenderingServerTypes::SurfaceData& p_surface, const String& p_path = String());
	static void set_boot_image(
		const Ref<Image>& p_image, const Color& p_color, bool p_scale, bool p_use_filter);

	/* SERVER LIFECYCLE */

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	/* TEXTURE API */

	static RID texture_2d_create(const Ref<Image>& p_image);
	static RID texture_2d_layered_create(
		const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_create(Image::Format p_format, int p_width, int p_height, int p_depth,
		bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static RID texture_external_create(int p_width, int p_height, uint64_t p_external_buffer = 0);
	static RID texture_proxy_create(RID p_base);
	static RID texture_drawable_create(int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color = Color(1, 1, 1, 1),
		bool p_with_mipmaps = false);

	static RID texture_create_from_native_handle(RSE::TextureType p_type, Image::Format p_format,
		uint64_t p_native_handle, int p_width, int p_height, int p_depth, int p_layers = 1,
		RSE::TextureLayeredType p_layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY);

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer = 0);
	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data);
	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer = 0);
	static void texture_proxy_update(RID p_texture, RID p_proxy_to);

	static RID texture_2d_placeholder_create();
	static RID texture_2d_layered_placeholder_create(RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_placeholder_create();

	static Ref<Image> texture_2d_get(RID p_texture);
	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer);
	static Vector<Ref<Image>> texture_3d_get(RID p_texture);

	static void texture_replace(RID p_texture, RID p_by_texture);
	static void texture_set_size_override(RID p_texture, int p_width, int p_height);

	static void texture_set_path(RID p_texture, const String& p_path);
	static String texture_get_path(RID p_texture);

	static void texture_drawable_generate_mipmaps(RID p_texture);
	static RID texture_drawable_get_default_material();

	static Image::Format texture_get_format(RID p_texture);

	static void texture_set_detect_3d_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_normal_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_roughness_callback(RID p_texture,
		RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata);

	static void texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info);

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	static RID texture_rd_create(const RID& p_rd_texture,
		const RSE::TextureLayeredType p_layer_type = RSE::TEXTURE_LAYERED_2D_ARRAY);
	static RID texture_get_rd_texture(RID p_texture, bool p_srgb = false);
	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb = false);

	/* SHADER API */

	static RID shader_create();
	static RID shader_create_from_code(const String& p_code, const String& p_path_hint = String());

	static void shader_set_code(RID p_shader, const String& p_code);
	static void shader_set_path_hint(RID p_shader, const String& p_path);
	static String shader_get_code(RID p_shader);

	static void shader_set_default_texture_parameter(
		RID p_shader, const StringName& p_name, RID p_texture, int p_index = 0);
	static RID shader_get_default_texture_parameter(
		RID p_shader, const StringName& p_name, int p_index = 0);

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader);

	/* COMMON MATERIAL API */

	static RID material_create();
	static RID material_create_from_shader(RID p_next_pass, int p_render_priority, RID p_shader);

	static void material_set_shader(RID p_shader_material, RID p_shader);
	static void material_set_render_priority(RID p_material, int priority);
	static void material_set_next_pass(RID p_material, RID p_next_material);
	static void material_set_use_debanding(bool p_enable);

	/* MESH API */

	static RID mesh_create_from_surfaces(
		const Vector<RenderingServerTypes::SurfaceData>& p_surfaces, int p_blend_shape_count = 0);
	static RID mesh_create();

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count);

	static uint32_t mesh_surface_get_format_offset(
		uint32_t p_format, int p_vertex_len, int p_array_index);
	static uint32_t mesh_surface_get_format_vertex_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_normal_tangent_stride(
		uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_attribute_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_skin_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_index_stride(uint32_t p_format, int p_vertex_len);

	static void mesh_surface_make_offsets_from_format(uint64_t p_format, int p_vertex_len,
		int p_index_len, uint32_t* r_offsets, uint32_t& r_vertex_element_size,
		uint32_t& r_normal_element_size, uint32_t& r_attrib_element_size,
		uint32_t& r_skin_element_size);

	static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface);

	static int mesh_get_blend_shape_count(RID p_mesh);

	static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode);
	static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh);

	static void mesh_surface_update_vertex_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_attribute_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_skin_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_index_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);

	static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	static RID mesh_surface_get_material(RID p_mesh, int p_surface);

	static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface);

	static int mesh_get_surface_count(RID p_mesh);

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB mesh_get_custom_aabb(RID p_mesh);

	static void mesh_set_path(RID p_mesh, const String& p_path);
	static String mesh_get_path(RID p_mesh);

	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh);

	static void mesh_surface_remove(RID p_mesh, int p_surface);
	static void mesh_clear(RID p_mesh);

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface);

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info);

	/* MULTIMESH API */

	static RID multimesh_create();

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

	static RID multimesh_get_mesh(RID p_multimesh);
	static AABB multimesh_get_aabb(RID p_multimesh);

	static void multimesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB multimesh_get_custom_aabb(RID p_mesh);

	static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
	static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

	static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
	static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
	static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
	static Vector<float> multimesh_get_buffer(RID p_multimesh);

	static void multimesh_set_buffer_interpolated(
		RID p_multimesh, const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);
	static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
	static void multimesh_set_physics_interpolation_quality(
		RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
	static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
	static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

	static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	static int multimesh_get_visible_instances(RID p_multimesh);

	/* SKELETON API */

	static RID skeleton_create();
	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false);
	static int skeleton_get_bone_count(RID p_skeleton);
	static void skeleton_bone_set_transform(
		RID p_skeleton, int p_bone, const Transform3D& p_transform);
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone);
	static void skeleton_bone_set_transform_2d(
		RID p_skeleton, int p_bone, const Transform2D& p_transform);
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone);
	static void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform);

	/* LIGHT API */

	static RID directional_light_create();
	static RID omni_light_create();
	static RID spot_light_create();
	static RID area_light_create();

	static void light_set_color(RID p_light, const Color& p_color);
	static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value);
	static void light_set_shadow(RID p_light, bool p_enabled);
	static void light_set_projector(RID p_light, RID p_texture);
	static void light_set_negative(RID p_light, bool p_enable);
	static void light_set_cull_mask(RID p_light, uint32_t p_mask);
	static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length);
	static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled);
	static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask);

	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode);
	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade);

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode);

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode);
	static void light_directional_set_blend_splits(RID p_light, bool p_enable);
	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode);

	static void light_area_set_size(RID p_light, const Vector2& p_size);
	static void light_area_set_normalize_energy(RID p_light, bool p_enabled);
	static void light_area_set_texture(RID p_light, RID p_texture);

	static RID shadow_atlas_create();
	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_use_16_bits = true);
	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision);

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true);

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	static void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);

	/* REFLECTION PROBE API */

	static RID reflection_probe_create();

	static void reflection_probe_set_update_mode(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode);
	static void reflection_probe_set_intensity(RID p_probe, float p_intensity);
	static void reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance);

	static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode);
	static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color);
	static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy);
	static void reflection_probe_set_max_distance(RID p_probe, float p_distance);
	static void reflection_probe_set_size(RID p_probe, const Vector3& p_size);
	static void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset);
	static void reflection_probe_set_as_interior(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable);
	static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_resolution(RID p_probe, int p_resolution);
	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_pixels);

	/* DECAL API */

	static RID decal_create();
	static void decal_set_size(RID p_decal, const Vector3& p_size);
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture);
	static void decal_set_emission_energy(RID p_decal, float p_energy);
	static void decal_set_albedo_mix(RID p_decal, float p_mix);
	static void decal_set_modulate(RID p_decal, const Color& p_modulate);
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers);
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length);
	static void decal_set_fade(RID p_decal, float p_above, float p_below);
	static void decal_set_normal_fade(RID p_decal, float p_fade);

	static void decals_set_filter(RSE::DecalFilter p_quality);

	/* VOXEL GI API */

	static RID voxel_gi_create();

	static void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts);

	static AABB voxel_gi_get_bounds(RID p_voxel_gi);
	static Vector3i voxel_gi_get_octree_size(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi);
	static Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi);
	static Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi);

	static void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_energy(RID p_voxel_gi, float p_energy);
	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure);
	static void voxel_gi_set_bias(RID p_voxel_gi, float p_bias);
	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable);
	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable);

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_quality);

	static void sdfgi_reset();

	/* LIGHTMAP API */

	static RID lightmap_create();

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_haromics);
	static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds);
	static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior);
	static void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree);
	static void lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_exposure);
	static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap);
	static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap);

	static void lightmap_set_probe_capture_update_speed(float p_speed);
	static void lightmaps_set_bicubic_filter(bool p_enable);

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow);
	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap);
	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode);

	/* PARTICLES API */

	static RID particles_create();
	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode);

	static void particles_set_emitting(RID p_particles, bool p_enable);
	static bool particles_get_emitting(RID p_particles);
	static void particles_set_amount(RID p_particles, int p_amount);
	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio);
	static void particles_set_lifetime(RID p_particles, double p_lifetime);
	static void particles_set_one_shot(RID p_particles, bool p_one_shot);
	static void particles_set_pre_process_time(RID p_particles, double p_time);
	static void particles_request_process_time(RID p_particles, real_t p_request_process_time,
		real_t p_request_process_time_residual = 0.0);
	static void particles_set_explosiveness_ratio(RID p_particles, float p_ratio);
	static void particles_set_randomness_ratio(RID p_particles, float p_ratio);
	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb);
	static void particles_set_speed_scale(RID p_particles, double p_scale);
	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	static void particles_set_process_material(RID p_particles, RID p_material);
	static void particles_set_fixed_fps(RID p_particles, int p_fps);
	static void particles_set_interpolate(RID p_particles, bool p_enable);
	static void particles_set_fractional_delta(RID p_particles, bool p_enable);
	static void particles_set_collision_base_size(RID p_particles, float p_size);
	static void particles_set_seed(RID p_particles, uint32_t p_seed);

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align);
	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter);
	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis);

	static void particles_set_trails(RID p_particles, bool p_enable, float p_length_sec);
	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses);

	static bool particles_is_inactive(RID p_particles);
	static void particles_request_process(RID p_particles);
	static void particles_restart(RID p_particles);

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles);

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags);

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order);

	static void particles_set_draw_passes(RID p_particles, int p_count);
	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	static AABB particles_get_current_aabb(RID p_particles);

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform);
	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity);
	static void particles_set_interp_to_end(RID p_particles, float p_interp);

	/* PARTICLES COLLISION API */

	static RID particles_collision_create();

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type);
	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask);
	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius);
	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents);
	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength);
	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality);
	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve);
	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture);

	static void particles_collision_height_field_update(RID p_particles_collision);

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution);
	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask);

	/* FOG VOLUME API */

	static RID fog_volume_create();

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape);
	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size);
	static void fog_volume_set_material(RID p_fog_volume, RID p_material);

	/* VISIBILITY NOTIFIER API */

	static RID visibility_notifier_create();
	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb);

	/* OCCLUDER API */

	static RID occluder_create();
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);

	/* CAMERA API */

	static RID camera_create();
	static void camera_set_perspective(
		RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	static void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	static void camera_set_frustum(
		RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	static void camera_set_transform(RID p_camera, const Transform3D& p_transform);
	static void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	static void camera_set_environment(RID p_camera, RID p_env);
	static void camera_set_camera_attributes(RID p_camera, RID p_camera_attributes);
	static void camera_set_compositor(RID p_camera, RID p_compositor);
	static void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	/* VIEWPORT API */

	static RID viewport_create();

#ifndef XR_DISABLED
	static void viewport_set_use_xr(RID p_viewport, bool p_use_xr);
#endif // !XR_DISABLED

	static void viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count = 1);
	static void viewport_set_active(RID p_viewport, bool p_active);
	static void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);
	static void viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask);

	static void viewport_attach_to_screen(RID p_viewport, const Rect2& p_rect = Rect2(),
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);

	static void viewport_set_scaling_3d_mode(
		RID p_viewport, RSE::ViewportScaling3DMode p_scaling_3d_mode);
	static void viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale);
	static void viewport_set_fsr_sharpness(RID p_viewport, float p_fsr_sharpness);
	static void viewport_set_texture_mipmap_bias(RID p_viewport, float p_texture_mipmap_bias);
	static void viewport_set_anisotropic_filtering_level(
		RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level);

	static void viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode);
	static RSE::ViewportUpdateMode viewport_get_update_mode(RID p_viewport);

	static void viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode);

	static RID viewport_get_render_target(RID p_viewport);
	static RID viewport_get_texture(RID p_viewport);

	static void viewport_set_environment_mode(RID p_viewport, RSE::ViewportEnvironmentMode p_mode);
	static void viewport_set_disable_3d(RID p_viewport, bool p_disable);
	static void viewport_set_disable_2d(RID p_viewport, bool p_disable);

	static void viewport_attach_camera(RID p_viewport, RID p_camera);
	static void viewport_set_scenario(RID p_viewport, RID p_scenario);
	static void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	static void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	static void viewport_set_canvas_transform(
		RID p_viewport, RID p_canvas, const Transform2D& p_offset);
	static void viewport_set_transparent_background(RID p_viewport, bool p_enabled);
	static void viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr);
	static bool viewport_is_using_hdr_2d(RID p_viewport);
	static void viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled);
	static void viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled);

	static void viewport_set_default_canvas_item_texture_filter(
		RID p_viewport, RSE::CanvasItemTextureFilter p_filter);
	static void viewport_set_default_canvas_item_texture_repeat(
		RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat);

	static void viewport_set_global_canvas_transform(
		RID p_viewport, const Transform2D& p_transform);
	static void viewport_set_canvas_stacking(
		RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	static void viewport_set_sdf_oversize_and_scale(
		RID p_viewport, RSE::ViewportSDFOversize p_oversize, RSE::ViewportSDFScale p_scale);

	static void viewport_set_positional_shadow_atlas_size(
		RID p_viewport, int p_size, bool p_16_bits = true);
	static void viewport_set_positional_shadow_atlas_quadrant_subdivision(
		RID p_viewport, int p_quadrant, int p_subdiv);

	static void viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	static void viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa);

	static void viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode);

	static void viewport_set_use_taa(RID p_viewport, bool p_use_taa);
	static void viewport_set_use_debanding(RID p_viewport, bool p_use_debanding);
	static void viewport_set_force_motion_vectors(RID p_viewport, bool p_force_motion_vectors);

	static void viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels);

	static void viewport_set_use_occlusion_culling(RID p_viewport, bool p_use_occlusion_culling);
	static void viewport_set_occlusion_rays_per_thread(int p_rays_per_thread);

	static void viewport_set_occlusion_culling_build_quality(
		RSE::ViewportOcclusionCullingBuildQuality p_quality);

	static int viewport_get_render_info(
		RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info);

	static void viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw);

	static void viewport_set_measure_render_time(RID p_viewport, bool p_enable);
	static double viewport_get_measured_render_time_cpu(RID p_viewport);
	static double viewport_get_measured_render_time_gpu(RID p_viewport);

	static RID viewport_find_from_screen_attachment(
		DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID);

	static void viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode);
	static void viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode);
	static void viewport_set_vrs_texture(RID p_viewport, RID p_texture);

	/* SKY API */

	static RID sky_create();
	static void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	static void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	static void sky_set_material(RID p_sky, RID p_material);
	static Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* COMPOSITOR EFFECTS API */

	static RID compositor_effect_create();
	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);

	/* COMPOSITOR API */

	static RID compositor_create();

	/* ENVIRONMENT API */

	static RID environment_create();

	static void environment_set_background(RID p_env, RSE::EnvironmentBG p_bg);
	static void environment_set_sky(RID p_env, RID p_sky);
	static void environment_set_sky_custom_fov(RID p_env, float p_scale);
	static void environment_set_sky_orientation(RID p_env, const Basis& p_orientation);
	static void environment_set_bg_color(RID p_env, const Color& p_color);
	static void environment_set_bg_energy(RID p_env, float p_multiplier, float p_exposure_value);
	static void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	static void environment_set_ambient_light(RID p_env, const Color& p_color,
		RSE::EnvironmentAmbientSource p_ambient = RSE::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0,
		float p_sky_contribution = 0.0,
		RSE::EnvironmentReflectionSource p_reflection_source = RSE::ENV_REFLECTION_SOURCE_BG);
	static void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);

	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
		float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);

	static void environment_glow_set_use_bicubic_upscale(bool p_enable);

	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
		float p_contrast, float p_saturation, bool p_use_1d_color_correction,
		RID p_color_correction);

	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_in,
		float p_fade_out, float p_depth_tolerance);

	static void environment_set_ssr_half_size(bool p_half_size);

	static void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
		float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
		float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
		float p_probe_bias);

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);

	static void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);

	static void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode = RSE::EnvironmentFogMode::ENV_FOG_MODE_EXPONENTIAL);
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);

	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	static void environment_set_volumetric_fog_filter_active(bool p_enable);

	static Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	static void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);

	static void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	static void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	/* CAMERA ATTRIBUTES API */

	static RID camera_attributes_create();

	static void camera_attributes_set_dof_blur_quality(
		RSE::DOFBlurQuality p_quality, bool p_use_jitter);

	static void camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape);

	static void camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
		float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
		float p_near_transition, float p_amount);
	static void camera_attributes_set_exposure(
		RID p_camera_attributes, float p_multiplier, float p_exposure_normalization);
	static void camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
		float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale);

	/* SCENARIO API */

	static RID scenario_create();

	static void scenario_set_environment(RID p_scenario, RID p_environment);
	static void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	static void scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes);
	static void scenario_set_compositor(RID p_scenario, RID p_compositor);

	/* INSTANCING API */

	static RID instance_create2(RID p_base, RID p_scenario);
	static RID instance_create();

	static void instance_set_base(RID p_instance, RID p_base);
	static void instance_set_scenario(RID p_instance, RID p_scenario);
	static void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	static void instance_set_pivot_data(
		RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	static void instance_set_transform(RID p_instance, const Transform3D& p_transform);
	static void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	static void instance_set_surface_override_material(
		RID p_instance, int p_surface, RID p_material);
	static void instance_set_visible(RID p_instance, bool p_visible);

	static void instance_teleport(RID p_instance);
	static void instance_set_custom_aabb(RID p_instance, AABB p_aabb);
	static void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	static void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);
	static void instance_set_visibility_parent(RID p_instance, RID p_parent_instance);
	static void instance_set_ignore_culling(RID p_instance, bool p_enabled);

	static PackedInt64Array _instances_cull_aabb_bind(const AABB& p_aabb, RID p_scenario = RID());
	static PackedInt64Array _instances_cull_ray_bind(
		const Vector3& p_from, const Vector3& p_to, RID p_scenario = RID());

	static void instance_geometry_set_flag(
		RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled);
	static void instance_geometry_set_cast_shadows_setting(
		RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting);
	static void instance_geometry_set_material_override(RID p_instance, RID p_material);
	static void instance_geometry_set_material_overlay(RID p_instance, RID p_material);
	static void instance_geometry_set_visibility_range(RID p_instance, float p_min, float p_max,
		float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode);
	static void instance_geometry_set_lightmap(
		RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_lightmap_slice);
	static void instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias);
	static void instance_geometry_set_transparency(RID p_instance, float p_transparency);

	/* CANVAS API (2D) */

	static RID canvas_create();
	static void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2& p_mirroring);
	static void canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times);
	static void canvas_set_modulate(RID p_canvas, const Color& p_color);
	static void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);
	static void canvas_set_disable_scale(bool p_disable);

	/* CANVAS TEXTURE API */

	static RID canvas_texture_create();
	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat);

	/* CANVAS ITEM API */

	static RID canvas_item_create();
	static void canvas_item_set_parent(RID p_item, RID p_parent);

	static void canvas_item_set_default_texture_filter(
		RID p_item, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_item_set_default_texture_repeat(
		RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	static void canvas_item_set_visible(RID p_item, bool p_visible);
	static void canvas_item_set_light_mask(RID p_item, int p_mask);

	static void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	static void canvas_item_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_set_clip(RID p_item, bool p_clip);
	static void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	static void canvas_item_set_custom_rect(
		RID p_item, bool p_custom_rect, const Rect2& p_rect = Rect2());
	static void canvas_item_set_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_self_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer);

	static void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);
	static void canvas_item_set_use_identity_transform(RID p_item, bool p_enabled);

	static void canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
		const Color& p_color, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_rect(
		RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
		float p_minor, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_circle(RID p_item, const Point2& p_pos, float p_radius,
		const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
		bool p_tile = false, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	static void canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	static void canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1),
		int p_outline_size = 0, float p_px_range = 1.0, float p_scale = 1.0);
	static void canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
		RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
		RSE::NinePatchAxisMode p_x_axis_mode = RSE::NINE_PATCH_STRETCH,
		RSE::NinePatchAxisMode p_y_axis_mode = RSE::NINE_PATCH_STRETCH, bool p_draw_center = true,
		const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture);
	static void canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		RID p_texture = RID());
	static void canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1);
	static void canvas_item_add_mesh(RID p_item, const RID& p_mesh,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1),
		RID p_texture = RID());
	static void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID());
	static void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture);
	static void canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	static void canvas_item_add_animation_slice(RID p_item, double p_animation_length,
		double p_slice_begin, double p_slice_end, double p_offset);

	static void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	static void canvas_item_set_z_index(RID p_item, int p_z);
	static void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	static void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect);

	static void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	static void canvas_item_clear(RID p_item);
	static void canvas_item_set_draw_index(RID p_item, int p_index);

	static void canvas_item_set_material(RID p_item, RID p_material);
	static void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	static void canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
		float p_clear_margin = 5.0, bool p_fit_empty = false, float p_fit_margin = 0.0,
		bool p_blur_mipmaps = false);

	static void canvas_item_set_debug_redraw(bool p_enabled);
	static bool canvas_item_get_debug_redraw();

	static void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
	static void canvas_item_reset_physics_interpolation(RID p_item);
	static void canvas_item_transform_physics_interpolation(
		RID p_item, const Transform2D& p_transform);

	/* CANVAS LIGHT */

	static RID canvas_light_create();
	static void canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode);
	static void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	static void canvas_light_set_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_transform(RID p_light, const Transform2D& p_transform);
	static void canvas_light_set_color(RID p_light, const Color& p_color);
	static void canvas_light_set_height(RID p_light, float p_height);
	static void canvas_light_set_energy(RID p_light, float p_energy);
	static void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	static void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	static void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	static void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);

	static void canvas_light_set_directional_distance(RID p_light, float p_distance);

	static void canvas_light_set_texture_scale(RID p_light, float p_scale);
	static void canvas_light_set_texture(RID p_light, RID p_texture);
	static void canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset);

	static void canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode);

	static void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter);
	static void canvas_light_set_shadow_color(RID p_light, const Color& p_color);
	static void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	static void canvas_light_set_interpolated(RID p_light, bool p_interpolated);
	static void canvas_light_reset_physics_interpolation(RID p_light);
	static void canvas_light_transform_physics_interpolation(
		RID p_light, const Transform2D& p_transform);

	/* CANVAS LIGHT OCCLUDER API */

	static RID canvas_light_occluder_create();
	static void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	static void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	static void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	static void canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable);
	static void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform);
	static void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	static void canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated);
	static void canvas_light_occluder_reset_physics_interpolation(RID p_occluder);
	static void canvas_light_occluder_transform_physics_interpolation(
		RID p_occluder, const Transform2D& p_transform);

	/* CANVAS OCCLUDER POLYGON API */

	static RID canvas_occluder_polygon_create();
	static void canvas_occluder_polygon_set_shape(
		RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed);

	static void canvas_occluder_polygon_set_cull_mode(
		RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode);

	static void canvas_set_shadow_texture_size(int p_size);

	static Rect2 debug_canvas_item_get_rect(RID p_item);
	static Rect2 _debug_canvas_item_get_rect(RID p_item);

	/* GLOBAL SHADER PARAMETERS API */

	static void global_shader_parameter_remove(const StringName& p_name);
	static Vector<StringName> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const StringName& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures);
	static void global_shader_parameters_clear();

	static int global_shader_uniform_type_get_shader_datatype(
		RSE::GlobalShaderParameterType p_type);

	/* FREE */

	static void free_rid(RID p_rid);

	/* INTERPOLATION */

	static void set_physics_interpolation_enabled(bool p_enabled);

	/* EVENT QUEUING */

	static void draw(bool p_swap_buffers = true, double p_frame_step = 0.0);
	static void sync();
	static bool has_changed();
	static void init();
	static void finish();
	static void tick();
	static void pre_draw(bool p_will_draw);

	/* STATUS INFORMATION */

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info);
	static String get_video_adapter_name();
	static String get_video_adapter_vendor();
	static RenderingDeviceEnums::DeviceType get_video_adapter_type();
	static String get_video_adapter_api_version();

	static void set_frame_profiling_enabled(bool p_enable);
	static Vector<RenderingServerTypes::FrameProfileArea> get_frame_profile();
	static uint64_t get_frame_profile_frame();

	static double get_frame_setup_time_cpu();

	static void gi_set_use_half_resolution(bool p_enable);

	/* TESTING */

	static RID get_test_cube();
	static RID get_test_texture();
	static RID get_white_texture();

	static void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	static RID make_sphere_mesh(int p_lats, int p_lons, real_t p_radius);

	static void mesh_add_surface_from_mesh_data(
		RID p_mesh, const Geometry3D::MeshData& p_mesh_data);
	static void mesh_add_surface_from_planes(RID p_mesh, const Vector<Plane>& p_planes);

	/* BACKGROUND */

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true);

	static Color get_default_clear_color();
	static void set_default_clear_color(const Color& p_color);

	/* MISC */

	static bool has_os_feature(const String& p_feature);

	static void set_debug_generate_wireframes(bool p_generate);

	static void call_set_vsync_mode(
		DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window);

	static bool is_low_end();

	static void set_print_gpu_profile(bool p_enable);

	static Size2i get_maximum_viewport_size();

	static RenderingDevice* get_rendering_device();
	static RenderingDevice* create_local_rendering_device();

	static bool is_render_loop_enabled();
	static void set_render_loop_enabled(bool p_enabled);

	static bool is_on_render_thread();

	static String get_current_rendering_driver_name();
	static String get_current_rendering_method();

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const StringName& p_function, int p_idx, List<String>* r_options);

	typedef void (*SurfaceUpgradeCallback)();
	static void set_surface_upgrade_callback(SurfaceUpgradeCallback p_callback);
	static void set_warn_on_surface_upgrade(bool p_warn);
#endif

	static void redraw_request()
	{
		if (data) {
			data->backend.changes++;
		}
	}

	/* LIFECYCLE DISALLOWANCE */

	RenderingServer() = delete;
	RenderingServer(const RenderingServer&) = delete;
	RenderingServer& operator=(const RenderingServer&) = delete;
	~RenderingServer() = delete;
};

#define RS RenderingServer


