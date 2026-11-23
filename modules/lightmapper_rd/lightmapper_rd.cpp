#include "modules/modules_enabled.gen.h"

#if defined(MODULE_LIGHTMAPPER_RD_ENABLED)
#include "core/config/project_settings.h"
#include "modules/register_module_types.h"
#include "core/io/dir_access.h"
#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "editor/settings/editor_settings.h"
// --- Inlined from modules/lightmapper_rd/lightmapper_rd.h ---
#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class RenderingDevice;
class RDShaderFile;

struct Lightmapper::RDState
{
	struct BakeParameters
	{
		float world_size[3] = {};
		float bias = 0.0;

		float to_cell_offset[3] = {};
		int32_t grid_size = 0;

		float to_cell_size[3] = {};
		uint32_t light_count = 0;

		float env_transform[12] = {};

		int32_t atlas_size[2] = {};
		float exposure_normalization = 0.0f;
		uint32_t bounces = 0;

		float bounce_indirect_energy = 0.0f;
		uint32_t shadowmask_light_idx = 0;
		uint32_t transparency_rays = 0;
		float supersampling_factor = 0.0f;
	};

	struct MeshInstance
	{
		MeshData data;
		int slice = 0;
		Vector2i offset;
	};

	struct Light
	{
		float position[3] = {};
		uint32_t type = LIGHT_TYPE_DIRECTIONAL;
		float direction[3] = {};
		float energy = 0.0;
		float color[3] = {};
		float size = 0.0;
		float range = 0.0;
		float attenuation = 0.0;
		float cos_spot_angle = 0.0;
		float inv_spot_attenuation = 0.0;
		float indirect_energy = 0.0;
		float shadow_blur = 0.0;
		uint32_t static_bake = 0;
		uint32_t pad = 0;
		float area_width[4] = {};
		float area_height[4] = {};
		float area_texture_rect[4] = {};

		bool operator<(const Light& p_light) const { return type < p_light.type; }
	};

	struct LightMetadata
	{
		String name;
		uint32_t type = LIGHT_TYPE_DIRECTIONAL;

		bool operator<(const LightMetadata& p_light) const { return type < p_light.type; }
	};

	struct Vertex
	{
		float position[3] = {};
		float normal_z = 0.0;
		float uv[2] = {};
		float normal_xy[2] = {};

		bool operator==(const Vertex& p_vtx) const
		{
			return (position[0] == p_vtx.position[0]) && (position[1] == p_vtx.position[1]) &&
				   (position[2] == p_vtx.position[2]) && (uv[0] == p_vtx.uv[0]) &&
				   (uv[1] == p_vtx.uv[1]) && (normal_xy[0] == p_vtx.normal_xy[0]) &&
				   (normal_xy[1] == p_vtx.normal_xy[1]) && (normal_z == p_vtx.normal_z);
		}
	};

	struct Edge
	{
		Vector3 a;
		Vector3 b;
		Vector3 na;
		Vector3 nb;

		bool operator==(const Edge& p_seam) const
		{
			return a == p_seam.a && b == p_seam.b && na == p_seam.na && nb == p_seam.nb;
		}

		Edge() {}

		Edge(const Vector3& p_a, const Vector3& p_b, const Vector3& p_na, const Vector3& p_nb)
		{
			a = p_a;
			b = p_b;
			na = p_na;
			nb = p_nb;
		}
	};

	struct Probe
	{
		float position[4] = {};
	};

	Vector<Probe> probe_positions;

	struct EdgeHash
	{
		_FORCE_INLINE_ static uint32_t hash(const Edge& p_edge)
		{
			uint32_t h = hash_murmur3_one_float(p_edge.a.x);
			h = hash_murmur3_one_float(p_edge.a.y, h);
			h = hash_murmur3_one_float(p_edge.a.z, h);
			h = hash_murmur3_one_float(p_edge.b.x, h);
			h = hash_murmur3_one_float(p_edge.b.y, h);
			h = hash_murmur3_one_float(p_edge.b.z, h);
			return h;
		}
	};

	struct EdgeUV2
	{
		Vector2 a;
		Vector2 b;
		Vector2i indices;

		bool operator==(const EdgeUV2& p_uv2) const { return a == p_uv2.a && b == p_uv2.b; }

		bool seam_found = false;

		EdgeUV2(Vector2 p_a, Vector2 p_b, Vector2i p_indices)
		{
			a = p_a;
			b = p_b;
			indices = p_indices;
		}

		EdgeUV2() {}
	};

	struct Seam
	{
		Vector2i a;
		Vector2i b;
		uint32_t slice;

		bool operator<(const Seam& p_seam) const { return slice < p_seam.slice; }
	};

	struct VertexHash
	{
		_FORCE_INLINE_ static uint32_t hash(const Vertex& p_vtx)
		{
			uint32_t h = hash_murmur3_one_float(p_vtx.position[0]);
			h = hash_murmur3_one_float(p_vtx.position[1], h);
			h = hash_murmur3_one_float(p_vtx.position[2], h);
			h = hash_murmur3_one_float(p_vtx.uv[0], h);
			h = hash_murmur3_one_float(p_vtx.uv[1], h);
			h = hash_murmur3_one_float(p_vtx.normal_xy[0], h);
			h = hash_murmur3_one_float(p_vtx.normal_xy[1], h);
			h = hash_murmur3_one_float(p_vtx.normal_z, h);
			return hash_fmix32(h);
		}
	};

	struct Triangle
	{
		uint32_t indices[3] = {};
		uint32_t slice = 0;
		float min_bounds[3] = {};
		uint32_t cull_mode = 0;
		float max_bounds[3] = {};
		float pad1 = 0.0;

		bool operator<(const Triangle& p_triangle) const { return slice < p_triangle.slice; }
	};

	struct ClusterAABB
	{
		float min_bounds[3];
		float pad0 = 0.0f;
		float max_bounds[3];
		float pad1 = 0.0f;
	};

	Vector<MeshInstance> mesh_instances;

	Vector<Light> lights;
	Vector<LightMetadata> light_metadata;

	struct AreaLightAtlas
	{
		int mipmap_count;
		Vector2i size;
		PackedByteArray atlas_data;
	} area_light_atlas;

	struct TriangleSort
	{
		uint32_t cell_index = 0;
		uint32_t triangle_index = 0;
		AABB triangle_aabb;

		bool operator<(const TriangleSort& p_triangle_sort) const
		{
			return cell_index <
				   p_triangle_sort
					   .cell_index; // sorting by triangle index in this case makes no sense
		}
	};

	template <int T> struct TriangleSortAxis
	{
		bool operator()(const TriangleSort& p_a, const TriangleSort& p_b) const
		{
			return p_a.triangle_aabb.get_center()[T] < p_b.triangle_aabb.get_center()[T];
		}
	};

	void _plot_triangle_into_triangle_index_list(int p_size, const Vector3i& p_ofs,
		const AABB& p_bounds, const Vector3 p_points[3], uint32_t p_triangle_index,
		LocalVector<TriangleSort>& triangles, uint32_t p_grid_size);
	void _sort_triangle_clusters(uint32_t p_cluster_size, uint32_t p_cluster_index,
		uint32_t p_index_start, uint32_t p_count, LocalVector<TriangleSort>& p_triangle_sort,
		LocalVector<ClusterAABB>& p_cluster_aabb);

	struct RasterPushConstant
	{
		float atlas_size[2] = {};
		float uv_offset[2] = {};
		float to_cell_size[3] = {};
		uint32_t base_triangle = 0;
		float to_cell_offset[3] = {};
		float bias = 0.0;
		int32_t grid_size[3] = {};
		uint32_t pad2 = 0;
	};

	struct RasterSeamsPushConstant
	{
		uint32_t base_index = 0;
		uint32_t slice = 0;
		float uv_offset[2] = {};
		uint32_t debug = 0;
		float blend = 0.0;
		uint32_t pad[2] = {};
	};

	struct PushConstant
	{
		uint32_t atlas_slice = 0;
		uint32_t ray_count = 0;
		uint32_t ray_from = 0;
		uint32_t ray_to = 0;
		uint32_t region_ofs[2] = {};
		uint32_t probe_count = 0;
		uint32_t denoiser_range = 0;
	};

	Vector<Ref<Image>> lightmap_textures;
	Vector<Ref<Image>> shadowmask_textures;
	Vector<Color> probe_values;

	struct DilateParams
	{
		uint32_t radius;
		uint32_t pad[3];
	};

	struct DenoiseParams
	{
		float spatial_bandwidth;
		float light_bandwidth;
		float albedo_bandwidth;
		float normal_bandwidth;

		int half_search_window;
		float filter_strength;
		uint32_t slice_count;
		uint32_t pad;
	};

	BakeError _blit_meshes_into_atlas(int p_max_texture_size, int p_denoiser_range,
		Vector<Ref<Image>>& albedo_images, Vector<Ref<Image>>& emission_images, AABB& bounds,
		Size2i& atlas_size, int& atlas_slices, float p_supersampling_factor,
		BakeStepFunc p_step_function, void* p_bake_userdata);
	void _create_acceleration_structures(RenderingDevice* rd, Size2i atlas_size, int atlas_slices,
		AABB& bounds, int grid_size, uint32_t p_cluster_size, Vector<Probe>& probe_positions,
		GenerateProbes p_generate_probes, Vector<int>& slice_triangle_count,
		Vector<int>& slice_seam_count, RID& vertex_buffer, RID& triangle_buffer, RID& lights_buffer,
		RID& r_triangle_indices_buffer, RID& r_cluster_indices_buffer, RID& r_cluster_aabbs_buffer,
		RID& probe_positions_buffer, RID& grid_texture, RID& seams_buffer,
		BakeStepFunc p_step_function, void* p_bake_userdata);
	void _raster_geometry(RenderingDevice* rd, Size2i atlas_size, int atlas_slices, int grid_size,
		AABB bounds, float p_bias, Vector<int> slice_triangle_count, RID position_tex,
		RID unocclude_tex, RID normal_tex, RID raster_depth_buffer, RID rasterize_shader,
		RID raster_base_uniform);

	BakeError _dilate(RenderingDevice* rd, Ref<RDShaderFile>& compute_shader,
		RID& compute_base_uniform_set, PushConstant& push_constant, RID& source_light_tex,
		RID& dest_light_tex, const Size2i& atlas_size, int atlas_slices);
	BakeError _denoise(RenderingDevice* p_rd, Ref<RDShaderFile>& p_compute_shader,
		const RID& p_compute_base_uniform_set, PushConstant& p_push_constant,
		RID p_source_light_tex, RID p_source_normal_tex, RID p_dest_light_tex, RID p_unocclude_tex,
		float p_denoiser_strength, int p_denoiser_range, const Size2i& p_atlas_size,
		int p_atlas_slices, bool p_bake_sh, BakeStepFunc p_step_function, void* p_bake_userdata);
	BakeError _pack_l1(RenderingDevice* rd, Ref<RDShaderFile>& compute_shader,
		RID& compute_base_uniform_set, PushConstant& push_constant, RID& source_light_tex,
		RID& dest_light_tex, const Size2i& atlas_size, int atlas_slices);

	Error _store_pfm(RenderingDevice* p_rd, RID p_atlas_tex, int p_index,
		const Size2i& p_atlas_size, const String& p_name, bool p_shadowmask);
	Ref<Image> _read_pfm(const String& p_name, bool p_shadowmask);
	BakeError _denoise_oidn(RenderingDevice* p_rd, RID p_source_light_tex, RID p_source_normal_tex,
		RID p_dest_light_tex, const Size2i& p_atlas_size, int p_atlas_slices, bool p_bake_sh,
		bool p_shadowmask, const String& p_exe);
};



// --- End modules/lightmapper_rd/lightmapper_rd.h ---
#include "modules/lightmapper_rd/rd_raw_shaders.gen.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

// uncomment this if you want to see textures from all the process saved
// #define DEBUG_TEXTURES

Ref<Lightmapper> Lightmapper::create()
{
	return Ref<Lightmapper>(memnew(Lightmapper));
}

Lightmapper::Lightmapper()
{
	rd_state = memnew(RDState);
}

Lightmapper::~Lightmapper()
{
	memdelete(rd_state);
	rd_state = nullptr;
}

void Lightmapper::add_mesh(const MeshData& p_mesh)
{
	ERR_FAIL_COND(p_mesh.albedo_on_uv2.is_null() || p_mesh.albedo_on_uv2->is_empty());
	ERR_FAIL_COND(p_mesh.emission_on_uv2.is_null() || p_mesh.emission_on_uv2->is_empty());
	ERR_FAIL_COND(p_mesh.albedo_on_uv2->get_width() != p_mesh.emission_on_uv2->get_width());
	ERR_FAIL_COND(p_mesh.albedo_on_uv2->get_height() != p_mesh.emission_on_uv2->get_height());
	ERR_FAIL_COND(p_mesh.points.is_empty());
	RDState::MeshInstance mi;
	mi.data = p_mesh;
	rd_state->mesh_instances.push_back(mi);
}

void Lightmapper::add_directional_light(const String& p_name, bool p_static,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_angular_distance, float p_shadow_blur)
{
	RDState::Light l;
	l.type = LIGHT_TYPE_DIRECTIONAL;
	l.direction[0] = p_direction.x;
	l.direction[1] = p_direction.y;
	l.direction[2] = p_direction.z;
	l.color[0] = p_color.r;
	l.color[1] = p_color.g;
	l.color[2] = p_color.b;
	l.energy = p_energy;
	l.indirect_energy = p_indirect_energy;
	l.static_bake = p_static;
	l.size = Math::tan(Math::deg_to_rad(p_angular_distance));
	l.shadow_blur = p_shadow_blur;
	rd_state->lights.push_back(l);

	RDState::LightMetadata md;
	md.name = p_name;
	md.type = LIGHT_TYPE_DIRECTIONAL;
	rd_state->light_metadata.push_back(md);
}

void Lightmapper::add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
	float p_attenuation, float p_size, float p_shadow_blur)
{
	RDState::Light l;
	l.type = LIGHT_TYPE_OMNI;
	l.position[0] = p_position.x;
	l.position[1] = p_position.y;
	l.position[2] = p_position.z;
	l.range = p_range;
	l.attenuation = p_attenuation;
	l.color[0] = p_color.r;
	l.color[1] = p_color.g;
	l.color[2] = p_color.b;
	l.energy = p_energy;
	l.indirect_energy = p_indirect_energy;
	l.static_bake = p_static;
	l.size = p_size;
	l.shadow_blur = p_shadow_blur;
	rd_state->lights.push_back(l);

	RDState::LightMetadata md;
	md.name = p_name;
	md.type = LIGHT_TYPE_OMNI;
	rd_state->light_metadata.push_back(md);
}

void Lightmapper::add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation, float p_size,
	float p_shadow_blur)
{
	RDState::Light l;
	l.type = LIGHT_TYPE_SPOT;
	l.position[0] = p_position.x;
	l.position[1] = p_position.y;
	l.position[2] = p_position.z;
	l.direction[0] = p_direction.x;
	l.direction[1] = p_direction.y;
	l.direction[2] = p_direction.z;
	l.range = p_range;
	l.attenuation = p_attenuation;
	l.cos_spot_angle = Math::cos(Math::deg_to_rad(p_spot_angle));
	l.inv_spot_attenuation = 1.0f / p_spot_attenuation;
	l.color[0] = p_color.r;
	l.color[1] = p_color.g;
	l.color[2] = p_color.b;
	l.energy = p_energy;
	l.indirect_energy = p_indirect_energy;
	l.static_bake = p_static;
	l.size = p_size;
	l.shadow_blur = p_shadow_blur;
	rd_state->lights.push_back(l);

	RDState::LightMetadata md;
	md.name = p_name;
	md.type = LIGHT_TYPE_SPOT;
	rd_state->light_metadata.push_back(md);
}

void Lightmapper::add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, const Vector3& p_area_width, const Vector3& p_area_height,
	float p_size, float p_shadow_blur, const Rect2& p_texture_rect, float p_max_mipmap)
{
	RDState::Light l;
	l.type = LIGHT_TYPE_AREA;
	l.position[0] = p_position.x;
	l.position[1] = p_position.y;
	l.position[2] = p_position.z;
	l.direction[0] = p_direction.x;
	l.direction[1] = p_direction.y;
	l.direction[2] = p_direction.z;
	l.area_width[0] = p_area_width.x;
	l.area_width[1] = p_area_width.y;
	l.area_width[2] = p_area_width.z;
	l.area_height[0] = p_area_height.x;
	l.area_height[1] = p_area_height.y;
	l.area_height[2] = p_area_height.z;
	l.range = p_range;
	l.attenuation = p_attenuation;
	l.color[0] = p_color.r;
	l.color[1] = p_color.g;
	l.color[2] = p_color.b;
	l.energy = p_energy;
	l.indirect_energy = p_indirect_energy;
	l.static_bake = p_static;
	l.size = p_size;
	l.shadow_blur = p_shadow_blur;

	if (Renderer::get_current_rendering_method() == "gl_compatibility") {
		l.area_texture_rect[0] = 0.0;
		l.area_texture_rect[1] = 0.0;
		l.area_texture_rect[2] = 0.0;
		l.area_texture_rect[3] = 0.0;
	}
	else {
		l.area_texture_rect[0] = p_texture_rect.position.x;
		l.area_texture_rect[1] = p_texture_rect.position.y;
		l.area_texture_rect[2] = p_texture_rect.size.x;
		l.area_texture_rect[3] = p_texture_rect.size.y;
	}
	l.cos_spot_angle = p_max_mipmap;
	rd_state->lights.push_back(l);

	RDState::LightMetadata md;
	md.name = p_name;
	md.type = LIGHT_TYPE_AREA;
	rd_state->light_metadata.push_back(md);
}

void Lightmapper::add_area_light_atlas(
	const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data)
{
	rd_state->area_light_atlas.mipmap_count = p_mipmap_count;
	rd_state->area_light_atlas.size = p_size;
	rd_state->area_light_atlas.atlas_data = p_atlas_data;
}

void Lightmapper::add_probe(const Vector3& p_position)
{
	RDState::Probe probe;
	probe.position[0] = p_position.x;
	probe.position[1] = p_position.y;
	probe.position[2] = p_position.z;
	probe.position[3] = 0;
	rd_state->probe_positions.push_back(probe);
}

void Lightmapper::RDState::_plot_triangle_into_triangle_index_list(int p_size, const Vector3i& p_ofs,
	const AABB& p_bounds, const Vector3 p_points[3], uint32_t p_triangle_index,
	LocalVector<TriangleSort>& p_triangles_sort, uint32_t p_grid_size)
{
	int half_size = p_size / 2;

	for (int i = 0; i < 8; i++) {
		AABB aabb = p_bounds;
		aabb.size *= 0.5;
		Vector3i n = p_ofs;

		if (i & 1) {
			aabb.position.x += aabb.size.x;
			n.x += half_size;
		}
		if (i & 2) {
			aabb.position.y += aabb.size.y;
			n.y += half_size;
		}
		if (i & 4) {
			aabb.position.z += aabb.size.z;
			n.z += half_size;
		}

		{
			Vector3 qsize = aabb.size * 0.5; // quarter size, for fast aabb test

			if (!Geometry3D::triangle_box_overlap(aabb.position + qsize, qsize, p_points)) {
				// does not fit in child, go on
				continue;
			}
		}

		if (half_size == 1) {
			// got to the end
			TriangleSort ts;
			ts.cell_index = n.x + (n.y * p_grid_size) + (n.z * p_grid_size * p_grid_size);
			ts.triangle_index = p_triangle_index;
			ts.triangle_aabb.position = p_points[0];
			ts.triangle_aabb.size = Vector3();
			ts.triangle_aabb.expand_to(p_points[1]);
			ts.triangle_aabb.expand_to(p_points[2]);
			p_triangles_sort.push_back(ts);
		}
		else {
			_plot_triangle_into_triangle_index_list(
				half_size, n, aabb, p_points, p_triangle_index, p_triangles_sort, p_grid_size);
		}
	}
}

void Lightmapper::RDState::_sort_triangle_clusters(uint32_t p_cluster_size, uint32_t p_cluster_index,
	uint32_t p_index_start, uint32_t p_count, LocalVector<TriangleSort>& p_triangle_sort,
	LocalVector<ClusterAABB>& p_cluster_aabb)
{
	if (p_count == 0) {
		return;
	}

	// Compute AABB for all triangles in the range.
	SortArray<TriangleSort, TriangleSortAxis<0>> triangle_sorter_x;
	SortArray<TriangleSort, TriangleSortAxis<1>> triangle_sorter_y;
	SortArray<TriangleSort, TriangleSortAxis<2>> triangle_sorter_z;
	AABB cluster_aabb = p_triangle_sort[p_index_start].triangle_aabb;
	for (uint32_t i = 1; i < p_count; i++) {
		cluster_aabb.merge_with(p_triangle_sort[p_index_start + i].triangle_aabb);
	}

	if (p_count > p_cluster_size) {
		int longest_axis_index = cluster_aabb.get_longest_axis_index();
		switch (longest_axis_index) {
		case 0:
			triangle_sorter_x.sort(&p_triangle_sort[p_index_start], p_count);
			break;
		case 1:
			triangle_sorter_y.sort(&p_triangle_sort[p_index_start], p_count);
			break;
		case 2:
			triangle_sorter_z.sort(&p_triangle_sort[p_index_start], p_count);
			break;
		default:
			DEV_ASSERT(false && "Invalid axis returned by AABB.");
			break;
		}

		uint32_t left_cluster_count = Math::next_power_of_2(p_count / 2);
		left_cluster_count = MAX(left_cluster_count, p_cluster_size);
		left_cluster_count = MIN(left_cluster_count, p_count);
		_sort_triangle_clusters(p_cluster_size, p_cluster_index, p_index_start, left_cluster_count,
			p_triangle_sort, p_cluster_aabb);

		if (left_cluster_count < p_count) {
			uint32_t cluster_index_right = p_cluster_index + (left_cluster_count / p_cluster_size);
			_sort_triangle_clusters(p_cluster_size, cluster_index_right,
				p_index_start + left_cluster_count, p_count - left_cluster_count, p_triangle_sort,
				p_cluster_aabb);
		}
	}
	else {
		ClusterAABB& aabb = p_cluster_aabb[p_cluster_index];
		Vector3 aabb_end = cluster_aabb.get_end();
		aabb.min_bounds[0] = cluster_aabb.position.x;
		aabb.min_bounds[1] = cluster_aabb.position.y;
		aabb.min_bounds[2] = cluster_aabb.position.z;
		aabb.max_bounds[0] = aabb_end.x;
		aabb.max_bounds[1] = aabb_end.y;
		aabb.max_bounds[2] = aabb_end.z;
	}
}

Lightmapper::BakeError Lightmapper::RDState::_blit_meshes_into_atlas(int p_max_texture_size,
	int p_denoiser_range, Vector<Ref<Image>>& albedo_images, Vector<Ref<Image>>& emission_images,
	AABB& bounds, Size2i& atlas_size, int& atlas_slices, float p_supersampling_factor,
	BakeStepFunc p_step_function, void* p_bake_userdata)
{
	Vector<Size2i> sizes;

	for (int m_i = 0; m_i < mesh_instances.size(); m_i++) {
		MeshInstance& mi = mesh_instances.write[m_i];
		Size2i s = Size2i(mi.data.albedo_on_uv2->get_width(), mi.data.albedo_on_uv2->get_height());
		sizes.push_back(s);
		atlas_size =
			atlas_size.max(s + Size2i(2, 2).maxi(p_denoiser_range) * p_supersampling_factor);
	}

	int max = Math::nearest_power_of_2_templated(atlas_size.width);
	max = MAX(max, Math::nearest_power_of_2_templated(atlas_size.height));

	if (max > p_max_texture_size) {
		return BAKE_ERROR_TEXTURE_EXCEEDS_MAX_SIZE;
	}

	if (p_step_function) {
		if (p_step_function(0.1, RTR("Determining optimal atlas size"), p_bake_userdata, true)) {
			return BAKE_ERROR_USER_ABORTED;
		}
	}

	atlas_size = Size2i(max, max);

	Size2i best_atlas_size;
	int best_atlas_slices = 0;
	int best_atlas_memory = 0x7FFFFFFF;
	Vector<Vector3i> best_atlas_offsets;

	// Determine best texture array atlas size by bruteforce fitting.
	while (atlas_size.x <= p_max_texture_size && atlas_size.y <= p_max_texture_size) {
		Vector<Vector2i> source_sizes;
		Vector<int> source_indices;
		source_sizes.resize(sizes.size());
		source_indices.resize(sizes.size());
		for (int i = 0; i < source_indices.size(); i++) {
			// Add padding between lightmaps.
			// Scale the padding if the lightmap will be downsampled at the end of the baking
			// process Otherwise the padding would be insufficient.
			source_sizes.write[i] =
				sizes[i] + Vector2i(2, 2).maxi(p_denoiser_range) * p_supersampling_factor;
			source_indices.write[i] = i;
		}
		Vector<Vector3i> atlas_offsets;
		atlas_offsets.resize(source_sizes.size());

		// Ensure the sizes can all fit into a single atlas layer.
		// This should always happen, and this check is only in place to prevent an infinite loop.
		for (int i = 0; i < source_sizes.size(); i++) {
			if (source_sizes[i] > atlas_size) {
				return BAKE_ERROR_ATLAS_TOO_SMALL;
			}
		}

		int slices = 0;

		while (source_sizes.size() > 0) {
			Vector<Vector3i> offsets = Geometry2D::partial_pack_rects(source_sizes, atlas_size);
			Vector<int> new_indices;
			Vector<Vector2i> new_sources;
			for (int i = 0; i < offsets.size(); i++) {
				Vector3i ofs = offsets[i];
				int sidx = source_indices[i];
				if (ofs.z > 0) {
					// valid
					ofs.z = slices;
					atlas_offsets.write[sidx] =
						ofs + Vector3i(1, 1, 0); // Center lightmap in the reserved oversized region
				}
				else {
					new_indices.push_back(sidx);
					new_sources.push_back(source_sizes[i]);
				}
			}

			source_sizes = new_sources;
			source_indices = new_indices;
			slices++;
		}

		int mem_used = atlas_size.x * atlas_size.y * slices;
		if (mem_used < best_atlas_memory) {
			best_atlas_size = atlas_size;
			best_atlas_offsets = atlas_offsets;
			best_atlas_slices = slices;
			best_atlas_memory = mem_used;
		}

		if (atlas_size.width == atlas_size.height) {
			atlas_size.width *= 2;
		}
		else {
			atlas_size.height *= 2;
		}
	}
	atlas_size = best_atlas_size;
	atlas_slices = best_atlas_slices;

	// apply the offsets and slice to all images, and also blit albedo and emission
	albedo_images.resize(atlas_slices);
	emission_images.resize(atlas_slices);

	if (p_step_function) {
		if (p_step_function(0.2, RTR("Blitting albedo and emission"), p_bake_userdata, true)) {
			return BAKE_ERROR_USER_ABORTED;
		}
	}

	for (int i = 0; i < atlas_slices; i++) {
		Ref<Image> albedo =
			Image::create_empty(atlas_size.width, atlas_size.height, false, Image::FORMAT_RGBA8);
		albedo->set_as_black();
		albedo_images.write[i] = albedo;

		Ref<Image> emission =
			Image::create_empty(atlas_size.width, atlas_size.height, false, Image::FORMAT_RGBAH);
		emission->set_as_black();
		emission_images.write[i] = emission;
	}

	// assign uv positions

	for (int m_i = 0; m_i < mesh_instances.size(); m_i++) {
		MeshInstance& mi = mesh_instances.write[m_i];
		mi.offset.x = best_atlas_offsets[m_i].x;
		mi.offset.y = best_atlas_offsets[m_i].y;
		mi.slice = best_atlas_offsets[m_i].z;
		albedo_images.write[mi.slice]->blit_rect(mi.data.albedo_on_uv2,
			Rect2i(Vector2i(), mi.data.albedo_on_uv2->get_size()), mi.offset);
		emission_images.write[mi.slice]->blit_rect(mi.data.emission_on_uv2,
			Rect2(Vector2i(), mi.data.emission_on_uv2->get_size()), mi.offset);
	}

	return BAKE_OK;
}

void Lightmapper::RDState::_create_acceleration_structures(RenderingDevice* rd, Size2i atlas_size,
	int atlas_slices, AABB& bounds, int grid_size, uint32_t p_cluster_size,
	Vector<Probe>& p_probe_positions, GenerateProbes p_generate_probes,
	Vector<int>& slice_triangle_count, Vector<int>& slice_seam_count, RID& vertex_buffer,
	RID& triangle_buffer, RID& lights_buffer, RID& r_triangle_indices_buffer,
	RID& r_cluster_indices_buffer, RID& r_cluster_aabbs_buffer, RID& probe_positions_buffer,
	RID& grid_texture, RID& seams_buffer, BakeStepFunc p_step_function, void* p_bake_userdata)
{
	HashMap<Vertex, uint32_t, VertexHash> vertex_map;

	// fill triangles array and vertex array
	LocalVector<Triangle> triangles;
	LocalVector<Vertex> vertex_array;
	LocalVector<Seam> seams;

	slice_triangle_count.resize(atlas_slices);
	slice_seam_count.resize(atlas_slices);

	for (int i = 0; i < atlas_slices; i++) {
		slice_triangle_count.write[i] = 0;
		slice_seam_count.write[i] = 0;
	}

	bounds = AABB();

	for (int m_i = 0; m_i < mesh_instances.size(); m_i++) {
		if (p_step_function) {
			float p = float(m_i + 1) / MAX(1, mesh_instances.size()) * 0.1;
			p_step_function(0.3 + p,
				vformat(RTR("Plotting mesh into acceleration structure %d/%d"), m_i + 1,
					mesh_instances.size()),
				p_bake_userdata, false);
		}

		HashMap<Edge, EdgeUV2, EdgeHash> edges;

		MeshInstance& mi = mesh_instances.write[m_i];

		Vector2 uv_scale =
			Vector2(mi.data.albedo_on_uv2->get_width(), mi.data.albedo_on_uv2->get_height()) /
			Vector2(atlas_size);
		Vector2 uv_offset = Vector2(mi.offset) / Vector2(atlas_size);
		if (m_i == 0) {
			bounds.position = mi.data.points[0];
		}

		for (int i = 0; i < mi.data.points.size(); i += 3) {
			Vector3 vtxs[3] = {mi.data.points[i + 0], mi.data.points[i + 1], mi.data.points[i + 2]};
			Vector2 uvs[3] = {mi.data.uv2[i + 0] * uv_scale + uv_offset,
				mi.data.uv2[i + 1] * uv_scale + uv_offset,
				mi.data.uv2[i + 2] * uv_scale + uv_offset};
			Vector3 normal[3] = {
				mi.data.normal[i + 0], mi.data.normal[i + 1], mi.data.normal[i + 2]};

			AABB taabb;
			Triangle t;
			t.slice = mi.slice;
			for (int k = 0; k < 3; k++) {
				bounds.expand_to(vtxs[k]);

				Vertex v;
				v.position[0] = vtxs[k].x;
				v.position[1] = vtxs[k].y;
				v.position[2] = vtxs[k].z;
				v.uv[0] = uvs[k].x;
				v.uv[1] = uvs[k].y;
				v.normal_xy[0] = normal[k].x;
				v.normal_xy[1] = normal[k].y;
				v.normal_z = normal[k].z;

				uint32_t* indexptr = vertex_map.getptr(v);

				if (indexptr) {
					t.indices[k] = *indexptr;
				}
				else {
					uint32_t new_index = vertex_map.size();
					t.indices[k] = new_index;
					vertex_map[v] = new_index;
					vertex_array.push_back(v);
				}

				if (k == 0) {
					taabb.position = vtxs[k];
				}
				else {
					taabb.expand_to(vtxs[k]);
				}
			}

			// compute seams that will need to be blended later
			for (int k = 0; k < 3; k++) {
				int n = (k + 1) % 3;

				Edge edge(vtxs[k], vtxs[n], normal[k], normal[n]);
				Vector2i edge_indices(t.indices[k], t.indices[n]);
				EdgeUV2 uv2(uvs[k], uvs[n], edge_indices);

				if (edge.b == edge.a) {
					continue; // degenerate, somehow
				}
				if (edge.b < edge.a) {
					SWAP(edge.a, edge.b);
					SWAP(edge.na, edge.nb);
					SWAP(uv2.a, uv2.b);
					SWAP(uv2.indices.x, uv2.indices.y);
					SWAP(edge_indices.x, edge_indices.y);
				}

				EdgeUV2* euv2 = edges.getptr(edge);
				if (!euv2) {
					edges[edge] = uv2;
				}
				else {
					if (*euv2 == uv2) {
						continue; // seam shared UV space, no need to blend
					}
					if (euv2->seam_found) {
						continue; // bad geometry
					}

					Seam seam;
					seam.a = edge_indices;
					seam.b = euv2->indices;
					seam.slice = mi.slice;
					seams.push_back(seam);
					slice_seam_count.write[mi.slice]++;
					euv2->seam_found = true;
				}
			}

			t.min_bounds[0] = taabb.position.x;
			t.min_bounds[1] = taabb.position.y;
			t.min_bounds[2] = taabb.position.z;
			t.max_bounds[0] = taabb.position.x + MAX(taabb.size.x, 0.0001);
			t.max_bounds[1] = taabb.position.y + MAX(taabb.size.y, 0.0001);
			t.max_bounds[2] = taabb.position.z + MAX(taabb.size.z, 0.0001);

			t.cull_mode = RSE::CULL_MODE_BACK;

			RID material = mi.data.material[i];
			if (material.is_valid()) {
				t.cull_mode = RendererRD::MaterialStorage::material_get_cull_mode(material);
			}
			t.pad1 = 0; // make valgrind not complain
			triangles.push_back(t);
			slice_triangle_count.write[t.slice]++;
		}
	}

	// also consider probe positions for bounds
	for (int i = 0; i < p_probe_positions.size(); i++) {
		Vector3 pp(p_probe_positions[i].position[0], p_probe_positions[i].position[1],
			p_probe_positions[i].position[2]);
		bounds.expand_to(pp);
	}
	bounds.grow_by(0.1); // grow a bit to avoid numerical error

	triangles.sort(); // sort by slice
	seams.sort();

	if (p_step_function) {
		p_step_function(0.4, RTR("Optimizing acceleration structure"), p_bake_userdata, true);
	}

	// fill list of triangles in grid
	LocalVector<TriangleSort> triangle_sort;
	for (uint32_t i = 0; i < triangles.size(); i++) {
		const Triangle& t = triangles[i];
		Vector3 face[3] = {
			Vector3(vertex_array[t.indices[0]].position[0], vertex_array[t.indices[0]].position[1],
				vertex_array[t.indices[0]].position[2]),
			Vector3(vertex_array[t.indices[1]].position[0], vertex_array[t.indices[1]].position[1],
				vertex_array[t.indices[1]].position[2]),
			Vector3(vertex_array[t.indices[2]].position[0], vertex_array[t.indices[2]].position[1],
				vertex_array[t.indices[2]].position[2])};
		_plot_triangle_into_triangle_index_list(
			grid_size, Vector3i(), bounds, face, i, triangle_sort, grid_size);
	}
	// sort it
	triangle_sort.sort();

	LocalVector<uint32_t> cluster_indices;
	LocalVector<ClusterAABB> cluster_aabbs;
	Vector<uint32_t> triangle_indices;
	triangle_indices.resize(triangle_sort.size());
	Vector<uint32_t> grid_indices;
	grid_indices.resize(grid_size * grid_size * grid_size * 2);
	memset(grid_indices.ptrw(), 0, grid_indices.size() * sizeof(uint32_t));

	{
		// Fill grid with cell indices.
		uint32_t last_cell = 0xFFFFFFFF;
		uint32_t* giw = grid_indices.ptrw();
		uint32_t cluster_count = 0;
		uint32_t solid_cell_count = 0;
		for (uint32_t i = 0; i < triangle_sort.size(); i++) {
			uint32_t cell = triangle_sort[i].cell_index;
			if (cell != last_cell) {
				giw[cell * 2 + 1] = solid_cell_count;
				solid_cell_count++;
			}

			if ((giw[cell * 2] % p_cluster_size) == 0) {
				// Add an extra cluster every time the triangle counter reaches a multiple of the
				// cluster size.
				cluster_count++;
			}

			giw[cell * 2]++;
			last_cell = cell;
		}

		// Build fixed-size triangle clusters for all the cells to speed up the traversal. A cell
		// can hold multiple clusters that each contain a fixed amount of triangles and an AABB. The
		// tracer will check against the AABBs first to know whether it needs to visit the cell's
		// triangles.
		//
		// The building algorithm will divide the triangles recursively contained inside each cell,
		// sorting by the longest axis of the AABB on each step.
		//
		// - If the amount of triangles is less or equal to the cluster size, the AABB will be
		// stored and the algorithm stops.
		//
		// - The division by two is increased to the next power of two of half the amount of
		// triangles (with cluster size as the minimum value) to
		//   ensure the first half always fills the cluster.

		cluster_indices.resize(solid_cell_count * 2);
		cluster_aabbs.resize(cluster_count);

		uint32_t i = 0;
		uint32_t cluster_index = 0;
		uint32_t solid_cell_index = 0;
		uint32_t* tiw = triangle_indices.ptrw();
		while (i < triangle_sort.size()) {
			cluster_indices[solid_cell_index * 2] = cluster_index;
			cluster_indices[solid_cell_index * 2 + 1] = i;

			uint32_t cell = triangle_sort[i].cell_index;
			uint32_t triangle_count = giw[cell * 2];
			uint32_t cell_cluster_count = (triangle_count + p_cluster_size - 1) / p_cluster_size;
			_sort_triangle_clusters(
				p_cluster_size, cluster_index, i, triangle_count, triangle_sort, cluster_aabbs);

			for (uint32_t j = 0; j < triangle_count; j++) {
				tiw[i + j] = triangle_sort[i + j].triangle_index;
			}

			i += triangle_count;
			cluster_index += cell_cluster_count;
			solid_cell_index++;
		}
	}
#if 0
	for (int i = 0; i < grid_size; i++) {
		for (int j = 0; j < grid_size; j++) {
			for (int k = 0; k < grid_size; k++) {
				uint32_t index = i * (grid_size * grid_size) + j * grid_size + k;
				grid_indices.write[index * 2] = float(i) / grid_size * 255;
				grid_indices.write[index * 2 + 1] = float(j) / grid_size * 255;
			}
		}
	}
#endif

#if 0
	for (int i = 0; i < grid_size; i++) {
		Vector<uint8_t> grid_usage;
		grid_usage.resize(grid_size * grid_size);
		for (int j = 0; j < grid_usage.size(); j++) {
			uint32_t ofs = i * grid_size * grid_size + j;
			uint32_t count = grid_indices[ofs * 2];
			grid_usage.write[j] = count > 0 ? 255 : 0;
		}

		Ref<Image> img = Image::create_from_data(grid_size, grid_size, false, Image::FORMAT_L8, grid_usage);
		img->save_png("res://grid_layer_" + itos(1000 + i).substr(1, 3) + ".png");
	}
#endif

	/*****************************/
	/*** CREATE GPU STRUCTURES ***/
	/*****************************/

	lights.sort();
	light_metadata.sort();

	Vector<Vector2i> seam_buffer_vec;
	seam_buffer_vec.resize(seams.size() * 2);
	for (uint32_t i = 0; i < seams.size(); i++) {
		seam_buffer_vec.write[i * 2 + 0] = seams[i].a;
		seam_buffer_vec.write[i * 2 + 1] = seams[i].b;
	}

	{ // buffers
		vertex_buffer = rd->storage_buffer_create(
			vertex_array.size() * sizeof(Vertex), vertex_array.span().reinterpret<uint8_t>());

		triangle_buffer = rd->storage_buffer_create(
			triangles.size() * sizeof(Triangle), triangles.span().reinterpret<uint8_t>());

		r_triangle_indices_buffer =
			rd->storage_buffer_create(triangle_indices.size() * sizeof(uint32_t),
				triangle_indices.span().reinterpret<uint8_t>());

		r_cluster_indices_buffer =
			rd->storage_buffer_create(cluster_indices.size() * sizeof(uint32_t),
				cluster_indices.span().reinterpret<uint8_t>());

		r_cluster_aabbs_buffer =
			rd->storage_buffer_create(cluster_aabbs.size() * sizeof(ClusterAABB),
				cluster_aabbs.span().reinterpret<uint8_t>());

		// Even when there are no lights, the buffer must exist.
		static const Light empty_lights[1];
		Span<uint8_t> lb =
			(lights.is_empty() ? Span(empty_lights) : lights.span()).reinterpret<uint8_t>();
		lights_buffer = rd->storage_buffer_create(lb.size(), lb);

		// Even when there are no seams, the buffer must exist.
		static const Vector2i empty_seams[2] = {};
		Span<uint8_t> sb = (seam_buffer_vec.is_empty() ? Span(empty_seams) : seam_buffer_vec.span())
							   .reinterpret<uint8_t>();
		seams_buffer = rd->storage_buffer_create(sb.size(), sb);

		// Even when there are no probes, the buffer must exist.
		static const Probe empty_probes[1];
		Span<uint8_t> pb =
			(p_probe_positions.is_empty() ? Span(empty_probes) : p_probe_positions.span())
				.reinterpret<uint8_t>();
		probe_positions_buffer = rd->storage_buffer_create(pb.size(), pb);
	}

	{ // grid

		RDC::TextureFormat tf;
		tf.width = grid_size;
		tf.height = grid_size;
		tf.depth = grid_size;
		tf.texture_type = RDC::TEXTURE_TYPE_3D;
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> texdata;
		texdata.resize(1);
		// grid and indices
		tf.format = RDC::DATA_FORMAT_R32G32_UINT;
		texdata.write[0] = grid_indices.to_byte_array();
		grid_texture = rd->texture_create(tf, RDD::TextureView(), texdata);
	}
}

static Vector<RD::Uniform> dilate_or_denoise_common_uniforms(
	RID& p_source_light_tex, RID& p_dest_light_tex)
{
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_IMAGE;
		u.binding = 0;
		u.append_id(p_dest_light_tex);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 1;
		u.append_id(p_source_light_tex);
		uniforms.push_back(u);
	}

	return uniforms;
}

Error Lightmapper::RDState::_store_pfm(RenderingDevice* p_rd, RID p_atlas_tex, int p_index,
	const Size2i& p_atlas_size, const String& p_name, bool p_shadowmask)
{
	Vector<uint8_t> data = p_rd->texture_get_data(p_atlas_tex, p_index);
	Ref<Image> img = Image::create_from_data(p_atlas_size.width, p_atlas_size.height, false,
		p_shadowmask ? Image::FORMAT_RGBA8 : Image::FORMAT_RGBAH, data);
	img->convert(Image::FORMAT_RGBF);
	Vector<uint8_t> data_float = img->get_data();

	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(p_name, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save PFN at path: '%s'.", p_name));
	file->store_line("PF");
	file->store_line(vformat("%d %d", img->get_width(), img->get_height()));
	file->store_line("-1.0");
	file->store_buffer(data_float);
	file->close();

	return OK;
}

Ref<Image> Lightmapper::RDState::_read_pfm(const String& p_name, bool p_shadowmask)
{
	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(p_name, FileAccess::READ, &err);
	ERR_FAIL_COND_V_MSG(err, Ref<Image>(), vformat("Can't load PFM at path: '%s'.", p_name));
	ERR_FAIL_COND_V(file->get_line() != "PF", Ref<Image>());

	Vector<String> new_size = file->get_line().split(" ");
	ERR_FAIL_COND_V(new_size.size() != 2, Ref<Image>());
	int new_width = new_size[0].to_int();
	int new_height = new_size[1].to_int();

	float endian = file->get_line().to_float();
	Vector<uint8_t> new_data = file->get_buffer(file->get_length() - file->get_position());
	file->close();

	if (unlikely(endian > 0.0)) {
		uint32_t count = new_data.size() / 4;
		uint16_t* dst = (uint16_t*)new_data.ptrw();
		for (uint32_t j = 0; j < count; j++) {
			dst[j * 4] = BSWAP32(dst[j * 4]);
		}
	}
	Ref<Image> img =
		Image::create_from_data(new_width, new_height, false, Image::FORMAT_RGBF, new_data);
	img->convert(p_shadowmask ? Image::FORMAT_RGBA8 : Image::FORMAT_RGBAH);
	return img;
}

Lightmapper::BakeError Lightmapper::RDState::_denoise_oidn(RenderingDevice* p_rd,
	RID p_source_light_tex, RID p_source_normal_tex, RID p_dest_light_tex,
	const Size2i& p_atlas_size, int p_atlas_slices, bool p_bake_sh, bool p_shadowmask,
	const String& p_exe)
{
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	for (int i = 0; i < p_atlas_slices; i++) {
		String fname_norm_in =
			EditorPaths::get_cache_dir().path_join(vformat("temp_norm_%d.pfm", i));
		_store_pfm(p_rd, p_source_normal_tex, i, p_atlas_size, fname_norm_in, false);

		for (int j = 0; j < (p_bake_sh ? 4 : 1); j++) {
			int index = i * (p_bake_sh ? 4 : 1) + j;
			String fname_light_in = EditorPaths::get_cache_dir().path_join(
				vformat("temp_light_%d.pfm", index));
			String fname_out = EditorPaths::get_cache_dir().path_join(
				vformat("temp_denoised_%d.pfm", index));

			_store_pfm(p_rd, p_source_light_tex, index, p_atlas_size, fname_light_in, p_shadowmask);

			List<String> args;
			args.push_back("--device");
			args.push_back("default");

			args.push_back("--filter");
			args.push_back("RTLightmap");

			args.push_back(p_shadowmask ? "--ldr" : "--hdr");
			args.push_back(fname_light_in);

			args.push_back("--nrm");
			args.push_back(fname_norm_in);

			args.push_back("--output");
			args.push_back(fname_out);

			String str;
			int exitcode = 0;

			Error err = OS::execute(p_exe, args, &str, &exitcode, true);

			da->remove(fname_light_in);

			if (err != OK || exitcode != 0) {
				da->remove(fname_out);
				print_verbose(str);
				ERR_FAIL_V_MSG(BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES,
					vformat("OIDN denoiser failed, return code: %d", exitcode));
			}

			Ref<Image> img = _read_pfm(fname_out, p_shadowmask);
			da->remove(fname_out);

			ERR_FAIL_COND_V(img.is_null(), BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES);

			Vector<uint8_t> old_data = p_rd->texture_get_data(p_source_light_tex, index);
			Vector<uint8_t> new_data = img->get_data();
			img.unref(); // Avoid copy on write.

			uint32_t count = old_data.size() / 2;
			const uint16_t* src = (const uint16_t*)old_data.ptr();
			uint16_t* dst = (uint16_t*)new_data.ptrw();
			for (uint32_t k = 0; k < count; k += 4) {
				dst[k + 3] = src[k + 3];
			}

			p_rd->texture_update(p_dest_light_tex, index, new_data);
		}
		da->remove(fname_norm_in);
	}
	return BAKE_OK;
}

int Lightmapper::get_bake_texture_count() const { return rd_state->lightmap_textures.size(); }

Ref<Image> Lightmapper::get_bake_texture(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, rd_state->lightmap_textures.size(), Ref<Image>());
	return rd_state->lightmap_textures[p_index];
}

int Lightmapper::get_shadowmask_texture_count() const
{
	return rd_state->shadowmask_textures.size();
}

Ref<Image> Lightmapper::get_shadowmask_texture(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, rd_state->shadowmask_textures.size(), Ref<Image>());
	return rd_state->shadowmask_textures[p_index];
}

int Lightmapper::get_bake_mesh_count() const { return rd_state->mesh_instances.size(); }

Rect2 Lightmapper::get_bake_mesh_uv_scale(int p_index) const
{
	ERR_FAIL_COND_V(rd_state->lightmap_textures.is_empty(), Rect2());
	Rect2 uv_ofs;
	Vector2 atlas_size = Vector2(
		rd_state->lightmap_textures[0]->get_width(), rd_state->lightmap_textures[0]->get_height());
	uv_ofs.position = Vector2(rd_state->mesh_instances[p_index].offset) / atlas_size;
	uv_ofs.size = Vector2(rd_state->mesh_instances[p_index].data.albedo_on_uv2->get_width(),
					  rd_state->mesh_instances[p_index].data.albedo_on_uv2->get_height()) /
				  atlas_size;
	return uv_ofs;
}

int Lightmapper::get_bake_probe_count() const { return rd_state->probe_positions.size(); }

Vector<Color> Lightmapper::get_bake_probe_sh(int p_probe) const
{
	ERR_FAIL_INDEX_V(p_probe, rd_state->probe_positions.size(), Vector<Color>());
	Vector<Color> ret;
	ret.resize(9);
	memcpy(ret.ptrw(), &rd_state->probe_values[p_probe * 9], sizeof(Color) * 9);
	return ret;
}

Lightmapper::BakeError Lightmapper::bake(Lightmapper::BakeQuality p_quality, bool p_use_denoiser,
	float p_denoiser_strength, int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy,
	float p_bias, int p_max_texture_size, bool p_bake_sh, bool p_texture_based,
	bool p_generate_atlas, Lightmapper::GenerateProbes p_generate_probes,
	const Ref<Image>& p_environment_panorama, const Basis& p_environment_transform,
	BakeStepFunc p_step_function, void* p_step_userdata, float p_exposure_normalization,
	float p_supersampling_factor)
{
	return Lightmapper::BAKE_OK;
}

int Lightmapper::get_bake_mesh_texture_slice(int p_slice) const { return 0; }

Vector3 Lightmapper::get_bake_probe_point(int p_probe) const { return Vector3(); }

void uninitialize_lightmapper_rd_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void initialize_lightmapper_rd_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_LIGHTMAPPER_RD_ENABLED)


// --- Merged from modules/xatlas_unwrap/register_types.cpp ---
#if defined(MODULE_XATLAS_UNWRAP_ENABLED)
#include "modules/register_module_types.h"

#include "core/crypto/crypto_core.h"
#include "core/error/error_macros.h"
#include "core/os/memory.h"

#include <xatlas.h>

#include <cstring>

extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, const uint8_t *p_cache_data, bool *r_use_cache, uint8_t **r_mesh_cache, int *r_mesh_cache_size, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y);

bool xatlas_mesh_lightmap_unwrap_callback(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, const uint8_t *p_cache_data, bool *r_use_cache, uint8_t **r_mesh_cache, int *r_mesh_cache_size, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y) {
	CryptoCore::MD5Context ctx;
	ctx.start();

	ctx.update((unsigned char *)&p_texel_size, sizeof(float));
	ctx.update((unsigned char *)p_indices, sizeof(int) * p_index_count);
	ctx.update((unsigned char *)p_vertices, sizeof(float) * p_vertex_count * 3);
	ctx.update((unsigned char *)p_normals, sizeof(float) * p_vertex_count * 3);

	unsigned char hash[16];
	ctx.finish(hash);

	bool cached = false;
	unsigned int cache_idx = 0;

	*r_mesh_cache = nullptr;
	*r_mesh_cache_size = 0;

	if (p_cache_data) {
		//Check if hash is in cache data
		int *cache_data = (int *)p_cache_data;
		int n_entries = cache_data[0];
		unsigned int read_idx = 1;
		for (int i = 0; i < n_entries; ++i) {
			if (memcmp(&cache_data[read_idx], hash, 16) == 0) {
				cached = true;
				cache_idx = read_idx;
				break;
			}

			read_idx += 4; // hash
			read_idx += 2; // size hint

			int vertex_count = cache_data[read_idx];
			read_idx += 1; // vertex count
			read_idx += vertex_count; // vertex
			read_idx += vertex_count * 2; // uvs

			int index_count = cache_data[read_idx];
			read_idx += 1; // index count
			read_idx += index_count; // indices
		}
	}

	if (cached) {
		int *cache_data = (int *)p_cache_data;

		cache_idx += 4;

		// Load size
		*r_size_hint_x = cache_data[cache_idx];
		*r_size_hint_y = cache_data[cache_idx + 1];
		cache_idx += 2;

		// Load vertices
		*r_vertex_count = cache_data[cache_idx];
		cache_idx++;
		*r_vertex = &cache_data[cache_idx];
		cache_idx += *r_vertex_count;

		// Load UVs
		*r_uv = (float *)&cache_data[cache_idx];
		cache_idx += *r_vertex_count * 2;

		// Load indices
		*r_index_count = cache_data[cache_idx];
		cache_idx++;
		*r_index = &cache_data[cache_idx];
	} else {
		// set up input mesh
		xatlas::MeshDecl input_mesh;
		input_mesh.indexData = p_indices;
		input_mesh.indexCount = p_index_count;
		input_mesh.indexFormat = xatlas::IndexFormat::UInt32;

		input_mesh.vertexCount = p_vertex_count;
		input_mesh.vertexPositionData = p_vertices;
		input_mesh.vertexPositionStride = sizeof(float) * 3;
		input_mesh.vertexNormalData = p_normals;
		input_mesh.vertexNormalStride = sizeof(uint32_t) * 3;
		input_mesh.vertexUvData = nullptr;
		input_mesh.vertexUvStride = 0;

		xatlas::ChartOptions chart_options;
		chart_options.fixWinding = true;

		ERR_FAIL_COND_V_MSG(p_texel_size <= 0.0f, false, "Texel size must be greater than 0.");

		xatlas::PackOptions pack_options;
		pack_options.padding = 1;
		pack_options.maxChartSize = 4094; // Lightmap atlassing needs 2 for padding between meshes, so 4096-2
		pack_options.blockAlign = true;
		pack_options.texelsPerUnit = 1.0 / p_texel_size;

		xatlas::Atlas *atlas = xatlas::Create();

		xatlas::AddMeshError err = xatlas::AddMesh(atlas, input_mesh, 1);
		ERR_FAIL_COND_V_MSG(err != xatlas::AddMeshError::Success, false, xatlas::StringForEnum(err));

		xatlas::Generate(atlas, chart_options, pack_options);

		*r_size_hint_x = atlas->width;
		*r_size_hint_y = atlas->height;

		float w = *r_size_hint_x;
		float h = *r_size_hint_y;

		if (w == 0 || h == 0) {
			xatlas::Destroy(atlas);
			return false; //could not bake because there is no area
		}

		const xatlas::Mesh &output = atlas->meshes[0];

		*r_vertex = (int *)memalloc(sizeof(int) * output.vertexCount);
		ERR_FAIL_NULL_V_MSG(*r_vertex, false, "Out of memory.");
		*r_uv = (float *)memalloc(sizeof(float) * output.vertexCount * 2);
		ERR_FAIL_NULL_V_MSG(*r_uv, false, "Out of memory.");
		*r_index = (int *)memalloc(sizeof(int) * output.indexCount);
		ERR_FAIL_NULL_V_MSG(*r_index, false, "Out of memory.");

		float max_x = 0;
		float max_y = 0;
		for (uint32_t i = 0; i < output.vertexCount; i++) {
			(*r_vertex)[i] = output.vertexArray[i].xref;
			(*r_uv)[i * 2 + 0] = output.vertexArray[i].uv[0] / w;
			(*r_uv)[i * 2 + 1] = output.vertexArray[i].uv[1] / h;
			max_x = MAX(max_x, output.vertexArray[i].uv[0]);
			max_y = MAX(max_y, output.vertexArray[i].uv[1]);
		}

		*r_vertex_count = output.vertexCount;

		for (uint32_t i = 0; i < output.indexCount; i++) {
			(*r_index)[i] = output.indexArray[i];
		}

		*r_index_count = output.indexCount;

		xatlas::Destroy(atlas);
	}

	if (*r_use_cache) {
		// Build cache data for current mesh

		unsigned int new_cache_size = 4 + 2 + 1 + *r_vertex_count + (*r_vertex_count * 2) + 1 + *r_index_count; // hash + size hint + vertex_count + vertices + uvs + index_count + indices
		new_cache_size *= sizeof(int);
		int *new_cache_data = (int *)memalloc(new_cache_size);
		unsigned int new_cache_idx = 0;

		// hash
		memcpy(&new_cache_data[new_cache_idx], hash, 16);
		new_cache_idx += 4;

		// size hint
		new_cache_data[new_cache_idx] = *r_size_hint_x;
		new_cache_data[new_cache_idx + 1] = *r_size_hint_y;
		new_cache_idx += 2;

		// vertex count
		new_cache_data[new_cache_idx] = *r_vertex_count;
		new_cache_idx++;

		// vertices
		memcpy(&new_cache_data[new_cache_idx], *r_vertex, sizeof(int) * (*r_vertex_count));
		new_cache_idx += *r_vertex_count;

		// uvs
		memcpy(&new_cache_data[new_cache_idx], *r_uv, sizeof(float) * (*r_vertex_count) * 2);
		new_cache_idx += *r_vertex_count * 2;

		// index count
		new_cache_data[new_cache_idx] = *r_index_count;
		new_cache_idx++;

		// indices
		memcpy(&new_cache_data[new_cache_idx], *r_index, sizeof(int) * (*r_index_count));

		// Return cache data to the caller
		*r_mesh_cache = (uint8_t *)new_cache_data;
		*r_mesh_cache_size = new_cache_size;
	}

	*r_use_cache = cached; // Return whether cache was used.

	return true;
}

void initialize_xatlas_unwrap_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	array_mesh_lightmap_unwrap_callback = xatlas_mesh_lightmap_unwrap_callback;
}

void uninitialize_xatlas_unwrap_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_XATLAS_UNWRAP_ENABLED)


// --- Merged from modules/betsy/image_compress_betsy.cpp ---
#if defined(MODULE_BETSY_ENABLED)
#include "modules/lightmapper_rd/rd_raw_shaders.gen.h"
#include "modules/register_module_types.h"
#include "modules/lightmapper_rd/rd_raw_shaders.gen.h"
// --- Inlined from modules/betsy/betsy_bc1.h ---
static unsigned char stb__OMatch5[256][2] = {
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 1 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 1, 1 },
	{ 2, 0 },
	{ 2, 0 },
	{ 0, 4 },
	{ 2, 1 },
	{ 2, 1 },
	{ 2, 1 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 1 },
	{ 1, 5 },
	{ 3, 2 },
	{ 3, 2 },
	{ 4, 0 },
	{ 4, 0 },
	{ 4, 1 },
	{ 4, 1 },
	{ 4, 2 },
	{ 4, 2 },
	{ 4, 2 },
	{ 3, 5 },
	{ 5, 1 },
	{ 5, 1 },
	{ 5, 2 },
	{ 4, 4 },
	{ 5, 3 },
	{ 5, 3 },
	{ 5, 3 },
	{ 6, 2 },
	{ 6, 2 },
	{ 6, 2 },
	{ 6, 3 },
	{ 5, 5 },
	{ 6, 4 },
	{ 6, 4 },
	{ 4, 8 },
	{ 7, 3 },
	{ 7, 3 },
	{ 7, 3 },
	{ 7, 4 },
	{ 7, 4 },
	{ 7, 4 },
	{ 7, 5 },
	{ 5, 9 },
	{ 7, 6 },
	{ 7, 6 },
	{ 8, 4 },
	{ 8, 4 },
	{ 8, 5 },
	{ 8, 5 },
	{ 8, 6 },
	{ 8, 6 },
	{ 8, 6 },
	{ 7, 9 },
	{ 9, 5 },
	{ 9, 5 },
	{ 9, 6 },
	{ 8, 8 },
	{ 9, 7 },
	{ 9, 7 },
	{ 9, 7 },
	{ 10, 6 },
	{ 10, 6 },
	{ 10, 6 },
	{ 10, 7 },
	{ 9, 9 },
	{ 10, 8 },
	{ 10, 8 },
	{ 8, 12 },
	{ 11, 7 },
	{ 11, 7 },
	{ 11, 7 },
	{ 11, 8 },
	{ 11, 8 },
	{ 11, 8 },
	{ 11, 9 },
	{ 9, 13 },
	{ 11, 10 },
	{ 11, 10 },
	{ 12, 8 },
	{ 12, 8 },
	{ 12, 9 },
	{ 12, 9 },
	{ 12, 10 },
	{ 12, 10 },
	{ 12, 10 },
	{ 11, 13 },
	{ 13, 9 },
	{ 13, 9 },
	{ 13, 10 },
	{ 12, 12 },
	{ 13, 11 },
	{ 13, 11 },
	{ 13, 11 },
	{ 14, 10 },
	{ 14, 10 },
	{ 14, 10 },
	{ 14, 11 },
	{ 13, 13 },
	{ 14, 12 },
	{ 14, 12 },
	{ 12, 16 },
	{ 15, 11 },
	{ 15, 11 },
	{ 15, 11 },
	{ 15, 12 },
	{ 15, 12 },
	{ 15, 12 },
	{ 15, 13 },
	{ 13, 17 },
	{ 15, 14 },
	{ 15, 14 },
	{ 16, 12 },
	{ 16, 12 },
	{ 16, 13 },
	{ 16, 13 },
	{ 16, 14 },
	{ 16, 14 },
	{ 16, 14 },
	{ 15, 17 },
	{ 17, 13 },
	{ 17, 13 },
	{ 17, 14 },
	{ 16, 16 },
	{ 17, 15 },
	{ 17, 15 },
	{ 17, 15 },
	{ 18, 14 },
	{ 18, 14 },
	{ 18, 14 },
	{ 18, 15 },
	{ 17, 17 },
	{ 18, 16 },
	{ 18, 16 },
	{ 16, 20 },
	{ 19, 15 },
	{ 19, 15 },
	{ 19, 15 },
	{ 19, 16 },
	{ 19, 16 },
	{ 19, 16 },
	{ 19, 17 },
	{ 17, 21 },
	{ 19, 18 },
	{ 19, 18 },
	{ 20, 16 },
	{ 20, 16 },
	{ 20, 17 },
	{ 20, 17 },
	{ 20, 18 },
	{ 20, 18 },
	{ 20, 18 },
	{ 19, 21 },
	{ 21, 17 },
	{ 21, 17 },
	{ 21, 18 },
	{ 20, 20 },
	{ 21, 19 },
	{ 21, 19 },
	{ 21, 19 },
	{ 22, 18 },
	{ 22, 18 },
	{ 22, 18 },
	{ 22, 19 },
	{ 21, 21 },
	{ 22, 20 },
	{ 22, 20 },
	{ 20, 24 },
	{ 23, 19 },
	{ 23, 19 },
	{ 23, 19 },
	{ 23, 20 },
	{ 23, 20 },
	{ 23, 20 },
	{ 23, 21 },
	{ 21, 25 },
	{ 23, 22 },
	{ 23, 22 },
	{ 24, 20 },
	{ 24, 20 },
	{ 24, 21 },
	{ 24, 21 },
	{ 24, 22 },
	{ 24, 22 },
	{ 24, 22 },
	{ 23, 25 },
	{ 25, 21 },
	{ 25, 21 },
	{ 25, 22 },
	{ 24, 24 },
	{ 25, 23 },
	{ 25, 23 },
	{ 25, 23 },
	{ 26, 22 },
	{ 26, 22 },
	{ 26, 22 },
	{ 26, 23 },
	{ 25, 25 },
	{ 26, 24 },
	{ 26, 24 },
	{ 24, 28 },
	{ 27, 23 },
	{ 27, 23 },
	{ 27, 23 },
	{ 27, 24 },
	{ 27, 24 },
	{ 27, 24 },
	{ 27, 25 },
	{ 25, 29 },
	{ 27, 26 },
	{ 27, 26 },
	{ 28, 24 },
	{ 28, 24 },
	{ 28, 25 },
	{ 28, 25 },
	{ 28, 26 },
	{ 28, 26 },
	{ 28, 26 },
	{ 27, 29 },
	{ 29, 25 },
	{ 29, 25 },
	{ 29, 26 },
	{ 28, 28 },
	{ 29, 27 },
	{ 29, 27 },
	{ 29, 27 },
	{ 30, 26 },
	{ 30, 26 },
	{ 30, 26 },
	{ 30, 27 },
	{ 29, 29 },
	{ 30, 28 },
	{ 30, 28 },
	{ 30, 28 },
	{ 31, 27 },
	{ 31, 27 },
	{ 31, 27 },
	{ 31, 28 },
	{ 31, 28 },
	{ 31, 28 },
	{ 31, 29 },
	{ 31, 29 },
	{ 31, 30 },
	{ 31, 30 },
	{ 31, 30 },
	{ 31, 31 },
	{ 31, 31 },
};

static unsigned char stb__OMatch6[256][2] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 2, 0 },
	{ 2, 1 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 1 },
	{ 4, 0 },
	{ 4, 0 },
	{ 4, 1 },
	{ 5, 0 },
	{ 5, 1 },
	{ 6, 0 },
	{ 6, 0 },
	{ 6, 1 },
	{ 7, 0 },
	{ 7, 0 },
	{ 7, 1 },
	{ 8, 0 },
	{ 8, 1 },
	{ 8, 1 },
	{ 8, 2 },
	{ 9, 1 },
	{ 9, 2 },
	{ 9, 2 },
	{ 9, 3 },
	{ 10, 2 },
	{ 10, 3 },
	{ 10, 3 },
	{ 10, 4 },
	{ 11, 3 },
	{ 11, 4 },
	{ 11, 4 },
	{ 11, 5 },
	{ 12, 4 },
	{ 12, 5 },
	{ 12, 5 },
	{ 12, 6 },
	{ 13, 5 },
	{ 13, 6 },
	{ 8, 16 },
	{ 13, 7 },
	{ 14, 6 },
	{ 14, 7 },
	{ 9, 17 },
	{ 14, 8 },
	{ 15, 7 },
	{ 15, 8 },
	{ 11, 16 },
	{ 15, 9 },
	{ 15, 10 },
	{ 16, 8 },
	{ 16, 9 },
	{ 16, 10 },
	{ 15, 13 },
	{ 17, 9 },
	{ 17, 10 },
	{ 17, 11 },
	{ 15, 16 },
	{ 18, 10 },
	{ 18, 11 },
	{ 18, 12 },
	{ 16, 16 },
	{ 19, 11 },
	{ 19, 12 },
	{ 19, 13 },
	{ 17, 17 },
	{ 20, 12 },
	{ 20, 13 },
	{ 20, 14 },
	{ 19, 16 },
	{ 21, 13 },
	{ 21, 14 },
	{ 21, 15 },
	{ 20, 17 },
	{ 22, 14 },
	{ 22, 15 },
	{ 25, 10 },
	{ 22, 16 },
	{ 23, 15 },
	{ 23, 16 },
	{ 26, 11 },
	{ 23, 17 },
	{ 24, 16 },
	{ 24, 17 },
	{ 27, 12 },
	{ 24, 18 },
	{ 25, 17 },
	{ 25, 18 },
	{ 28, 13 },
	{ 25, 19 },
	{ 26, 18 },
	{ 26, 19 },
	{ 29, 14 },
	{ 26, 20 },
	{ 27, 19 },
	{ 27, 20 },
	{ 30, 15 },
	{ 27, 21 },
	{ 28, 20 },
	{ 28, 21 },
	{ 28, 21 },
	{ 28, 22 },
	{ 29, 21 },
	{ 29, 22 },
	{ 24, 32 },
	{ 29, 23 },
	{ 30, 22 },
	{ 30, 23 },
	{ 25, 33 },
	{ 30, 24 },
	{ 31, 23 },
	{ 31, 24 },
	{ 27, 32 },
	{ 31, 25 },
	{ 31, 26 },
	{ 32, 24 },
	{ 32, 25 },
	{ 32, 26 },
	{ 31, 29 },
	{ 33, 25 },
	{ 33, 26 },
	{ 33, 27 },
	{ 31, 32 },
	{ 34, 26 },
	{ 34, 27 },
	{ 34, 28 },
	{ 32, 32 },
	{ 35, 27 },
	{ 35, 28 },
	{ 35, 29 },
	{ 33, 33 },
	{ 36, 28 },
	{ 36, 29 },
	{ 36, 30 },
	{ 35, 32 },
	{ 37, 29 },
	{ 37, 30 },
	{ 37, 31 },
	{ 36, 33 },
	{ 38, 30 },
	{ 38, 31 },
	{ 41, 26 },
	{ 38, 32 },
	{ 39, 31 },
	{ 39, 32 },
	{ 42, 27 },
	{ 39, 33 },
	{ 40, 32 },
	{ 40, 33 },
	{ 43, 28 },
	{ 40, 34 },
	{ 41, 33 },
	{ 41, 34 },
	{ 44, 29 },
	{ 41, 35 },
	{ 42, 34 },
	{ 42, 35 },
	{ 45, 30 },
	{ 42, 36 },
	{ 43, 35 },
	{ 43, 36 },
	{ 46, 31 },
	{ 43, 37 },
	{ 44, 36 },
	{ 44, 37 },
	{ 44, 37 },
	{ 44, 38 },
	{ 45, 37 },
	{ 45, 38 },
	{ 40, 48 },
	{ 45, 39 },
	{ 46, 38 },
	{ 46, 39 },
	{ 41, 49 },
	{ 46, 40 },
	{ 47, 39 },
	{ 47, 40 },
	{ 43, 48 },
	{ 47, 41 },
	{ 47, 42 },
	{ 48, 40 },
	{ 48, 41 },
	{ 48, 42 },
	{ 47, 45 },
	{ 49, 41 },
	{ 49, 42 },
	{ 49, 43 },
	{ 47, 48 },
	{ 50, 42 },
	{ 50, 43 },
	{ 50, 44 },
	{ 48, 48 },
	{ 51, 43 },
	{ 51, 44 },
	{ 51, 45 },
	{ 49, 49 },
	{ 52, 44 },
	{ 52, 45 },
	{ 52, 46 },
	{ 51, 48 },
	{ 53, 45 },
	{ 53, 46 },
	{ 53, 47 },
	{ 52, 49 },
	{ 54, 46 },
	{ 54, 47 },
	{ 57, 42 },
	{ 54, 48 },
	{ 55, 47 },
	{ 55, 48 },
	{ 58, 43 },
	{ 55, 49 },
	{ 56, 48 },
	{ 56, 49 },
	{ 59, 44 },
	{ 56, 50 },
	{ 57, 49 },
	{ 57, 50 },
	{ 60, 45 },
	{ 57, 51 },
	{ 58, 50 },
	{ 58, 51 },
	{ 61, 46 },
	{ 58, 52 },
	{ 59, 51 },
	{ 59, 52 },
	{ 62, 47 },
	{ 59, 53 },
	{ 60, 52 },
	{ 60, 53 },
	{ 60, 53 },
	{ 60, 54 },
	{ 61, 53 },
	{ 61, 54 },
	{ 61, 54 },
	{ 61, 55 },
	{ 62, 54 },
	{ 62, 55 },
	{ 62, 55 },
	{ 62, 56 },
	{ 63, 55 },
	{ 63, 56 },
	{ 63, 56 },
	{ 63, 57 },
	{ 63, 58 },
	{ 63, 59 },
	{ 63, 59 },
	{ 63, 60 },
	{ 63, 61 },
	{ 63, 62 },
	{ 63, 62 },
	{ 63, 63 },
};

// --- End modules/betsy/betsy_bc1.h ---
#include "core/config/project_settings.h"
#include "core/os/os.h"
// --- Inlined from modules/betsy/image_compress_betsy.h ---
#include "core/io/image.h"
#include "core/os/condition_variable.h"

class RDShaderFile;
class RenderingDevice;
class RenderingContextDriver;

enum BetsyFormat
{
	BETSY_FORMAT_BC1,
	BETSY_FORMAT_BC1_DITHER,
	BETSY_FORMAT_BC3,
	BETSY_FORMAT_BC4_SIGNED,
	BETSY_FORMAT_BC4_UNSIGNED,
	BETSY_FORMAT_BC5_SIGNED,
	BETSY_FORMAT_BC5_UNSIGNED,
	BETSY_FORMAT_BC6_SIGNED,
	BETSY_FORMAT_BC6_UNSIGNED,
	BETSY_FORMAT_MAX,
};

enum BetsyShaderType
{
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC1_DITHER,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC6_SIGNED,
	BETSY_SHADER_BC6_UNSIGNED,
	BETSY_SHADER_ALPHA_STITCH,
	BETSY_SHADER_RGB_TO_RGBA_FLOAT,
	BETSY_SHADER_RGB_TO_RGBA_HALF,
	BETSY_SHADER_RGB_TO_RGBA_UNORM8,
	BETSY_SHADER_RGB_TO_RGBA_UNORM16,
	BETSY_SHADER_MAX,
};

struct BC6PushConstant
{
	float sizeX;
	float sizeY;
	uint32_t padding[2] = {0};
};

struct BC1PushConstant
{
	uint32_t num_refines;
	uint32_t padding[3] = {0};
};

struct BC4PushConstant
{
	uint32_t channel_idx;
	uint32_t padding[3] = {0};
};

struct RGBToRGBAPushConstant
{
	uint32_t width;
	uint32_t height;
	uint32_t padding[2];
};

void free_device();

Error _betsy_compress_bptc(
	Image* r_img, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format);
Error _betsy_compress_s3tc(Image* r_img, Image::UsedChannels p_channels);

class BetsyCompressor
{
	mutable CommandQueueMT command_queue;
	bool exit = false;

	struct BetsyShader
	{
		RID compiled;
		RID pipeline;
	};

	// Resources shared by all compression formats.
	RenderingContextDriver* compress_rcd = nullptr;
	BetsyShader cached_shaders[BETSY_SHADER_MAX];
	RID src_sampler;

	// Format-specific resources.
	RID dxt1_encoding_table_buffer;

	void _init();
	void _thread_loop();
	void _thread_exit();

	Error _get_shader(BetsyFormat p_format, const String& p_version, BetsyShader& r_shader);
	Error _compress(BetsyFormat p_format, Image* r_img);

public:
	void init();
	void finish();

	Error compress(BetsyFormat p_format, Image* r_img)
	{
		Error err;
		command_queue.push_and_ret(this, &BetsyCompressor::_compress, &err, p_format, r_img);
		return err;
	}
};



// --- End modules/betsy/image_compress_betsy.h ---
#include "modules/lightmapper_rd/rd_raw_shaders.gen.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_device_driver.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

#if defined(VULKAN_ENABLED)
#define Window VkXlibWindow
#define Font VkXlibFont
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#undef Font
#undef Window
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Always
#undef Always
#endif
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif

static Mutex betsy_mutex;
static BetsyCompressor* betsy = nullptr;

static const BetsyShaderType FORMAT_TO_TYPE[BETSY_FORMAT_MAX] = {
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC1_DITHER,
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC6_SIGNED,
	BETSY_SHADER_BC6_UNSIGNED,
};

static const RDC::DataFormat BETSY_TO_RD_FORMAT[BETSY_FORMAT_MAX] = {
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32B32A32_UINT,
	RDC::DATA_FORMAT_R32G32B32A32_UINT,
};

static const Image::Format BETSY_TO_IMAGE_FORMAT[BETSY_FORMAT_MAX] = {
	Image::FORMAT_DXT1,
	Image::FORMAT_DXT1,
	Image::FORMAT_DXT5,
	Image::FORMAT_RGTC_R,
	Image::FORMAT_RGTC_R,
	Image::FORMAT_RGTC_RG,
	Image::FORMAT_RGTC_RG,
	Image::FORMAT_BPTC_RGBF,
	Image::FORMAT_BPTC_RGBFU,
};

void BetsyCompressor::init() {}

void BetsyCompressor::finish() {}

Error BetsyCompressor::_compress(BetsyFormat p_format, Image* r_img) { return OK; }

void BetsyCompressor::_thread_exit()
{
	exit = true;

	if (RD::data != nullptr) {
		if (dxt1_encoding_table_buffer.is_valid()) {
			RD::free_rid(dxt1_encoding_table_buffer);
		}

		RD::free_rid(src_sampler);

		// Clear the shader cache, pipelines will be unreferenced automatically.
		for (int i = 0; i < BETSY_SHADER_MAX; i++) {
			if (cached_shaders[i].compiled.is_valid()) {
				RD::free_rid(cached_shaders[i].compiled);
			}
		}

		// Free the RD (and RCD if necessary).
		memdelete(RD::data);
		RD::data = nullptr;
		if (compress_rcd != nullptr) {
			memdelete(compress_rcd);
			compress_rcd = nullptr;
		}
	}
}

// Helper functions.

static int get_next_multiple(int n, int m) { return n + (m - (n % m)); }

static Error get_src_texture_format(Image* r_img, RDC::DataFormat& r_format, bool& r_is_rgb)
{
	r_is_rgb = false;

	switch (r_img->get_format()) {
	case Image::FORMAT_L8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_LA8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_R8:
		r_format = RDC::DATA_FORMAT_R8_UNORM;
		break;
	case Image::FORMAT_RG8:
		r_format = RDC::DATA_FORMAT_R8G8_UNORM;
		break;
	case Image::FORMAT_RGB8:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_RGBA8:
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_RH:
		r_format = RDC::DATA_FORMAT_R16_SFLOAT;
		break;
	case Image::FORMAT_RGH:
		r_format = RDC::DATA_FORMAT_R16G16_SFLOAT;
		break;
	case Image::FORMAT_RGBH:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case Image::FORMAT_RGBAH:
		r_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case Image::FORMAT_RF:
		r_format = RDC::DATA_FORMAT_R32_SFLOAT;
		break;
	case Image::FORMAT_RGF:
		r_format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		break;
	case Image::FORMAT_RGBF:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case Image::FORMAT_RGBAF:
		r_format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case Image::FORMAT_RGBE9995:
		r_format = RDC::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		break;
	case Image::FORMAT_R16:
		r_format = RDC::DATA_FORMAT_R16_UNORM;
		break;
	case Image::FORMAT_RG16:
		r_format = RDC::DATA_FORMAT_R16G16_UNORM;
		break;
	case Image::FORMAT_RGB16:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
		break;
	case Image::FORMAT_RGBA16:
		r_format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
		break;
	default: {
		return ERR_UNAVAILABLE;
	}
	}

	return OK;
}

void ensure_betsy_exists()
{
	betsy_mutex.lock();
	if (betsy == nullptr) {
		betsy = memnew(BetsyCompressor);
		betsy->init();
	}
	betsy_mutex.unlock();
}

Error _betsy_compress_bptc(
	Image* r_img, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format)
{
	ensure_betsy_exists();
	Image::Format format = r_img->get_format();
	Error result = ERR_UNAVAILABLE;

	if (format >= Image::FORMAT_RF && format <= Image::FORMAT_RGBE9995) {
		if ((p_bptc_format == Image::BPTC_DETECT && r_img->detect_signed()) ||
			p_bptc_format == Image::BPTC_FORCE_SIGNED) {
			result = betsy->compress(BETSY_FORMAT_BC6_SIGNED, r_img);
		}
		else {
			result = betsy->compress(BETSY_FORMAT_BC6_UNSIGNED, r_img);
		}
	}

	return result;
}

Error _betsy_compress_s3tc(Image* r_img, Image::UsedChannels p_channels)
{
	ensure_betsy_exists();
	Error result = ERR_UNAVAILABLE;

	switch (p_channels) {
	case Image::USED_CHANNELS_RGB:
	case Image::USED_CHANNELS_L:
		result = betsy->compress(BETSY_FORMAT_BC1, r_img);
		break;
	case Image::USED_CHANNELS_RGBA:
	case Image::USED_CHANNELS_LA:
		result = betsy->compress(BETSY_FORMAT_BC3, r_img);
		break;
	case Image::USED_CHANNELS_R:
		result = betsy->compress(BETSY_FORMAT_BC4_UNSIGNED, r_img);
		break;
	case Image::USED_CHANNELS_RG:
		result = betsy->compress(BETSY_FORMAT_BC5_UNSIGNED, r_img);
		break;
	default:
		break;
	}

	return result;
}

void free_device()
{
	if (betsy != nullptr) {
		betsy->finish();
		memdelete(betsy);
	}
}

void initialize_betsy_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Image::_image_compress_bptc_rd_func = _betsy_compress_bptc;
	Image::_image_compress_bc_rd_func = _betsy_compress_s3tc;
}

void uninitialize_betsy_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	free_device();
}
#endif // defined(MODULE_BETSY_ENABLED)


// --- Merged from modules/cvtt/image_compress_cvtt.cpp ---
#if defined(MODULE_CVTT_ENABLED)
#include <ConvectionKernels.h>
#include "modules/register_module_types.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/templates/safe_refcount.h"
// --- Inlined from modules/cvtt/image_compress_cvtt.h ---
#include "core/io/image.h"

void image_compress_cvtt(Image *p_image, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format);
void image_decompress_cvtt(Image *p_image);

// --- End modules/cvtt/image_compress_cvtt.h ---

struct CVTTCompressionJobParams
{
	bool is_hdr = false;
	bool is_signed = false;
	int bytes_per_pixel = 0;
	cvtt::BC7EncodingPlan bc7_plan;
	cvtt::Options options;
};

struct CVTTCompressionRowTask
{
	Vector<uint8_t> in_mm;
	uint8_t* out_mm_bytes = nullptr;
	int y_start = 0;
	int width = 0;
	int height = 0;
};

struct CVTTCompressionJobQueue
{
	CVTTCompressionJobParams job_params;
	const CVTTCompressionRowTask* job_tasks = nullptr;
	uint32_t num_tasks = 0;
	SafeNumeric<uint32_t> current_task;
};

static void _digest_row_task(
	const CVTTCompressionJobParams& p_job_params, const CVTTCompressionRowTask& p_row_task)
{
	const uint8_t* in_bytes = p_row_task.in_mm.ptr();
	uint8_t* out_bytes = p_row_task.out_mm_bytes;
	int w = p_row_task.width;
	int h = p_row_task.height;

	int y_start = p_row_task.y_start;
	int y_end = y_start + 4;

	int bytes_per_pixel = p_job_params.bytes_per_pixel;
	bool is_hdr = p_job_params.is_hdr;
	bool is_signed = p_job_params.is_signed;

	cvtt::PixelBlockU8 input_blocks_ldr[cvtt::NumParallelBlocks];
	cvtt::PixelBlockF16 input_blocks_hdr[cvtt::NumParallelBlocks];

	for (int x_start = 0; x_start < w; x_start += 4 * cvtt::NumParallelBlocks) {
		int x_end = x_start + 4 * cvtt::NumParallelBlocks;

		for (int y = y_start; y < y_end; y++) {
			int first_input_element = (y - y_start) * 4;
			const uint8_t* row_start;
			if (y >= h) {
				row_start = in_bytes + (h - 1) * (w * bytes_per_pixel);
			}
			else {
				row_start = in_bytes + y * (w * bytes_per_pixel);
			}

			for (int x = x_start; x < x_end; x++) {
				const uint8_t* pixel_start;
				if (x >= w) {
					pixel_start = row_start + (w - 1) * bytes_per_pixel;
				}
				else {
					pixel_start = row_start + x * bytes_per_pixel;
				}

				int block_index = (x - x_start) / 4;
				int block_element = (x - x_start) % 4 + first_input_element;
				if (is_hdr) {
					memcpy(input_blocks_hdr[block_index].m_pixels[block_element], pixel_start,
						bytes_per_pixel);
					input_blocks_hdr[block_index].m_pixels[block_element][3] =
						0x3c00; // 1.0 (unused)
				}
				else {
					memcpy(input_blocks_ldr[block_index].m_pixels[block_element], pixel_start,
						bytes_per_pixel);
				}
			}
		}

		uint8_t output_blocks[16 * cvtt::NumParallelBlocks];

		if (is_hdr) {
			if (is_signed) {
				cvtt::Kernels::EncodeBC6HS(output_blocks, input_blocks_hdr, p_job_params.options);
			}
			else {
				cvtt::Kernels::EncodeBC6HU(output_blocks, input_blocks_hdr, p_job_params.options);
			}
		}
		else {
			cvtt::Kernels::EncodeBC7(
				output_blocks, input_blocks_ldr, p_job_params.options, p_job_params.bc7_plan);
		}

		unsigned int num_real_blocks = ((w - x_start) + 3) / 4;
		if (num_real_blocks > cvtt::NumParallelBlocks) {
			num_real_blocks = cvtt::NumParallelBlocks;
		}

		memcpy(out_bytes, output_blocks, 16 * num_real_blocks);
		out_bytes += 16 * num_real_blocks;
	}
}

void image_compress_cvtt(
	Image* p_image, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format)
{
	uint64_t start_time = OS::get_ticks_msec();

	if (p_image->is_compressed()) {
		return; // do not compress, already compressed
	}

	int w = p_image->get_width();
	int h = p_image->get_height();

	if (w % 4 != 0 || h % 4 != 0) {
		w = w <= 2 ? w : (w + 3) & ~3;
		h = h <= 2 ? h : (h + 3) & ~3;
	}

	bool is_ldr = (p_image->get_format() <= Image::FORMAT_RGBA8);
	bool is_hdr = (p_image->get_format() >= Image::FORMAT_RF) &&
				  (p_image->get_format() <= Image::FORMAT_RGBE9995);

	if (!is_ldr && !is_hdr) {
		return; // Not a usable source format
	}

	cvtt::Options options;
	uint32_t flags = cvtt::Flags::Default;
	flags |= cvtt::Flags::BC7_RespectPunchThrough;
	if (p_channels == Image::USED_CHANNELS_RG) { // guessing this is a normal map
		flags |= cvtt::Flags::Uniform;
	}
	options.flags = flags;

	Image::Format target_format = Image::FORMAT_BPTC_RGBA;

	bool is_signed = false;
	if (is_hdr) {
		if (p_image->get_format() != Image::FORMAT_RGBH) {
			p_image->convert(Image::FORMAT_RGBH);
		}

		switch (p_bptc_format) {
		case Image::BPTC_DETECT:
			is_signed = p_image->detect_signed();
			break;
		case Image::BPTC_FORCE_SIGNED:
			is_signed = true;
			break;
		case Image::BPTC_FORCE_UNSIGNED:
			is_signed = false;
			break;
		}
		target_format = is_signed ? Image::FORMAT_BPTC_RGBF : Image::FORMAT_BPTC_RGBFU;
	}
	else {
		p_image->convert(Image::FORMAT_RGBA8); // still uses RGBA to convert
	}

	Vector<uint8_t> data;
	int64_t target_size = Image::get_image_data_size(w, h, target_format, p_image->has_mipmaps());
	int mm_count =
		p_image->has_mipmaps() ? Image::get_image_required_mipmaps(w, h, target_format) : 0;
	data.resize(target_size);

	uint8_t* wb = data.ptrw();

	int64_t dst_ofs = 0;

	CVTTCompressionJobQueue job_queue;
	job_queue.job_params.is_hdr = is_hdr;
	job_queue.job_params.is_signed = is_signed;
	job_queue.job_params.options = options;
	job_queue.job_params.bytes_per_pixel = is_hdr ? 6 : 4;
	cvtt::Kernels::ConfigureBC7EncodingPlanFromQuality(job_queue.job_params.bc7_plan, 5);

	// Amdahl's law (Wikipedia)
	// If a program needs 20 hours to complete using a single thread, but a one-hour portion of the
	// program cannot be parallelized, therefore only the remaining 19 hours (p = 0.95) of execution
	// time can be parallelized, then regardless of how many threads are devoted to a parallelized
	// execution of this program, the minimum execution time cannot be less than one hour.
	//
	// The number of executions with different inputs can be increased while the latency is the
	// same.

	Vector<CVTTCompressionRowTask> tasks;

	for (int i = 0; i <= mm_count; i++) {
		Vector<uint8_t> in_data;
		int width, height;
		Image::get_image_mipmap_offset_and_dimensions(w, h, target_format, i, width, height);

		int bw = width % 4 != 0 ? width + (4 - width % 4) : width;
		int bh = height % 4 != 0 ? height + (4 - height % 4) : height;

		int64_t src_mip_ofs, src_mip_size;
		int src_mip_w, src_mip_h;
		p_image->get_mipmap_offset_size_and_dimensions(
			i, src_mip_ofs, src_mip_size, src_mip_w, src_mip_h);

		// Pad textures to nearest block by smearing.
		if (width != src_mip_w || height != src_mip_h) {
			const uint8_t* src_mip_read = p_image->ptr() + src_mip_ofs;

			// Reserve the buffer for padded image data.
			int px_size = Image::get_format_pixel_size(p_image->get_format());
			in_data.resize(width * height * px_size);
			uint8_t* ptrw = in_data.ptrw();

			int x = 0, y = 0;
			for (y = 0; y < src_mip_h; y++) {
				for (x = 0; x < src_mip_w; x++) {
					memcpy(ptrw + (width * y + x) * px_size,
						src_mip_read + (src_mip_w * y + x) * px_size, px_size);
				}

				// First, smear in x.
				for (; x < width; x++) {
					memcpy(ptrw + (width * y + x) * px_size, ptrw + (width * y + x - 1) * px_size,
						px_size);
				}
			}

			// Then, smear in y.
			for (; y < height; y++) {
				for (x = 0; x < width; x++) {
					memcpy(ptrw + (width * y + x) * px_size,
						ptrw + (width * y + x - width) * px_size, px_size);
				}
			}
		}
		else {
			// Create a buffer filled with the source mip layer data.
			in_data.resize(src_mip_size);
			memcpy(in_data.ptrw(), p_image->ptr() + src_mip_ofs, src_mip_size);
		}

		// const uint8_t *in_bytes = &rb[src_ofs];
		uint8_t* out_bytes = &wb[dst_ofs];

		for (int y_start = 0; y_start < height; y_start += 4) {
			CVTTCompressionRowTask row_task;
			row_task.width = width;
			row_task.height = height;
			row_task.y_start = y_start;
			row_task.in_mm = in_data;
			row_task.out_mm_bytes = out_bytes;

			tasks.push_back(row_task);

			out_bytes += 16 * (bw / 4);
		}

		dst_ofs += Image::get_format_pixels_shifted(target_format, MAX(4, bw) * MAX(4, bh));
	}

	const CVTTCompressionRowTask* tasks_rb = tasks.ptr();

	job_queue.job_tasks = &tasks_rb[0];
	job_queue.num_tasks = static_cast<uint32_t>(tasks.size());

	p_image->set_data(w, h, p_image->has_mipmaps(), target_format, data);

	print_verbose(
		vformat("CVTT: Encoding took %d ms.", OS::get_ticks_msec() - start_time));
}

void image_decompress_cvtt(Image* p_image)
{
	Image::Format target_format;
	bool is_signed = false;
	bool is_hdr = false;

	Image::Format input_format = p_image->get_format();

	switch (input_format) {
	case Image::FORMAT_BPTC_RGBA:
		target_format = Image::FORMAT_RGBA8;
		break;
	case Image::FORMAT_BPTC_RGBF:
	case Image::FORMAT_BPTC_RGBFU:
		target_format = Image::FORMAT_RGBH;
		is_signed = (input_format == Image::FORMAT_BPTC_RGBF);
		is_hdr = true;
		break;
	default:
		return; // Invalid input format
	};

	int w = p_image->get_width();
	int h = p_image->get_height();

	const uint8_t* rb = p_image->get_data().ptr();

	Vector<uint8_t> data;
	int64_t target_size = Image::get_image_data_size(w, h, target_format, p_image->has_mipmaps());
	int mm_count = p_image->get_mipmap_count();
	data.resize(target_size);

	uint8_t* wb = data.ptrw();

	int bytes_per_pixel = is_hdr ? 6 : 4;

	int64_t dst_ofs = 0;

	for (int i = 0; i <= mm_count; i++) {
		int64_t src_ofs = p_image->get_mipmap_offset(i);

		const uint8_t* in_bytes = &rb[src_ofs];
		uint8_t* out_bytes = &wb[dst_ofs];

		cvtt::PixelBlockU8 output_blocks_ldr[cvtt::NumParallelBlocks];
		cvtt::PixelBlockF16 output_blocks_hdr[cvtt::NumParallelBlocks];

		for (int y_start = 0; y_start < h; y_start += 4) {
			int y_end = y_start + 4;

			for (int x_start = 0; x_start < w; x_start += 4 * cvtt::NumParallelBlocks) {
				uint8_t input_blocks[16 * cvtt::NumParallelBlocks];
				memset(input_blocks, 0, sizeof(input_blocks));

				unsigned int num_real_blocks = ((w - x_start) + 3) / 4;
				if (num_real_blocks > cvtt::NumParallelBlocks) {
					num_real_blocks = cvtt::NumParallelBlocks;
				}

				memcpy(input_blocks, in_bytes, 16 * num_real_blocks);
				in_bytes += 16 * num_real_blocks;

				int x_end = x_start + 4 * num_real_blocks;

				if (is_hdr) {
					if (is_signed) {
						cvtt::Kernels::DecodeBC6HS(output_blocks_hdr, input_blocks);
					}
					else {
						cvtt::Kernels::DecodeBC6HU(output_blocks_hdr, input_blocks);
					}
				}
				else {
					cvtt::Kernels::DecodeBC7(output_blocks_ldr, input_blocks);
				}

				for (int y = y_start; y < y_end; y++) {
					int first_input_element = (y - y_start) * 4;
					uint8_t* row_start;
					if (y >= h) {
						row_start = out_bytes + (h - 1) * (w * bytes_per_pixel);
					}
					else {
						row_start = out_bytes + y * (w * bytes_per_pixel);
					}

					for (int x = x_start; x < x_end; x++) {
						uint8_t* pixel_start;
						if (x >= w) {
							pixel_start = row_start + (w - 1) * bytes_per_pixel;
						}
						else {
							pixel_start = row_start + x * bytes_per_pixel;
						}

						int block_index = (x - x_start) / 4;
						int block_element = (x - x_start) % 4 + first_input_element;
						if (is_hdr) {
							memcpy(pixel_start,
								output_blocks_hdr[block_index].m_pixels[block_element],
								bytes_per_pixel);
						}
						else {
							memcpy(pixel_start,
								output_blocks_ldr[block_index].m_pixels[block_element],
								bytes_per_pixel);
						}
					}
				}
			}
		}

		dst_ofs += w * h * bytes_per_pixel;
		w >>= 1;
		h >>= 1;
	}
	p_image->set_data(
		p_image->get_width(), p_image->get_height(), p_image->has_mipmaps(), target_format, data);
}

void initialize_cvtt_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Image::_image_compress_bptc_func = image_compress_cvtt;
}

void uninitialize_cvtt_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_CVTT_ENABLED)
