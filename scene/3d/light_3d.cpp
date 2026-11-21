/**************************************************************************/
/*  light_3d.cpp                                                          */
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

#include "light_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light3D::_create_light_rid()
{
	if (!Renderer::data) {
		return;
	}
	switch (type) {
	case RSE::LIGHT_DIRECTIONAL:
		light = Renderer::directional_light_create();
		break;
	case RSE::LIGHT_OMNI:
		light = Renderer::omni_light_create();
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(omni_shadow_mode));
		break;
	case RSE::LIGHT_SPOT:
		light = Renderer::spot_light_create();
		break;
	case RSE::LIGHT_AREA:
		light = Renderer::area_light_create();
		break;
	}
	set_base(light);
}

void Light3D::_free_light_rid()
{
	if (light.is_valid() && Renderer::data) {
		set_base(RID());
		Renderer::free_rid(light);
		light = RID();
	}
}

void Light3D::set_light_type(RSE::LightType p_type)
{
	if (type == p_type) {
		return;
	}
	_free_light_rid();
	type = p_type;
	_create_light_rid();
}

void Light3D::set_param(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	param[p_param] = p_value;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_param(light, RSE::LightParam(p_param), p_value);
	}
}

real_t Light3D::get_param(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return param[p_param];
}

void Light3D::set_shadow(bool p_enable)
{
	shadow = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow(light, p_enable);
	}
}

bool Light3D::has_shadow() const { return shadow; }

void Light3D::set_negative(bool p_enable)
{
	negative = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_negative(light, p_enable);
	}
}

bool Light3D::is_negative() const { return negative; }

void Light3D::set_enable_distance_fade(bool p_enable)
{
	distance_fade_enabled = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

bool Light3D::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Light3D::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_begin() const { return distance_fade_begin; }

void Light3D::set_distance_fade_shadow(real_t p_distance)
{
	distance_fade_shadow = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_shadow() const { return distance_fade_shadow; }

void Light3D::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_length() const { return distance_fade_length; }

void Light3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_cull_mask(light, p_cull_mask);
	}
}

uint32_t Light3D::get_cull_mask() const { return cull_mask; }

void Light3D::set_color(const Color& p_color)
{
	color = p_color;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_color(light, color * correlated_color);
	}
}

Color Light3D::get_color() const { return color; }

void Light3D::set_shadow_reverse_cull_face(bool p_enable)
{
	reverse_cull = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_reverse_cull_face_mode(light, reverse_cull);
	}
}

bool Light3D::get_shadow_reverse_cull_face() const { return reverse_cull; }

void Light3D::set_shadow_caster_mask(uint32_t p_caster_mask)
{
	shadow_caster_mask = p_caster_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow_caster_mask(light, shadow_caster_mask);
	}
}

uint32_t Light3D::get_shadow_caster_mask() const { return shadow_caster_mask; }

AABB Light3D::get_aabb() const { return AABB(); }

void Light3D::set_bake_mode(BakeMode p_mode)
{
	bake_mode = p_mode;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_bake_mode(light, RSE::LightBakeMode(p_mode));
	}
}

Light3D::BakeMode Light3D::get_bake_mode() const { return bake_mode; }

void Light3D::set_projector(const Ref<Texture2D>& p_texture)
{
	projector = p_texture;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_projector(light, projector.is_valid() ? projector->get_rid() : RID());
	}
}

Ref<Texture2D> Light3D::get_projector() const { return projector; }

void Light3D::owner_changed_notify()
{
	_update_visibility();
}

Color _color_from_temperature(float p_temperature)
{
	float T2 = p_temperature * p_temperature;
	float u = (0.860117757f + 1.54118254e-4f * p_temperature + 1.28641212e-7f * T2) /
			  (1.0f + 8.42420235e-4f * p_temperature + 7.08145163e-7f * T2);
	float v = (0.317398726f + 4.22806245e-5f * p_temperature + 4.20481691e-8f * T2) /
			  (1.0f - 2.89741816e-5f * p_temperature + 1.61456053e-7f * T2);

	float d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
	float x = 3.0f * u * d;
	float y = 2.0f * v * d;

	const float a = 1.0 / MAX(y, 1e-5f);
	Vector3 xyz = Vector3(x * a, 1.0, (1.0f - x - y) * a);

	Vector3 linear = Vector3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
		-0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
		0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
	linear /= MAX(1e-5f, linear[linear.max_axis_index()]);
	return Color(linear.x, linear.y, linear.z).clamp().linear_to_srgb();
}

Color Light3D::get_correlated_color() const { return correlated_color; }

float Light3D::get_temperature() const { return temperature; }

void Light3D::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif

	if (get_instance().is_valid() && Renderer::data) {
		Renderer::instance_set_visible(get_instance(), is_visible_in_tree() && editor_ok);
	}
}

void Light3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Light3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_ENTER_TREE: {
		_update_visibility();
	} break;
	}
}

void Light3D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_visibility();
}

bool Light3D::is_editor_only() const { return editor_only; }

void Light3D::set_directional_shadow_mode(DirectionalShadowMode p_mode)
{
	directional_shadow_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_shadow_mode(
			light, RSE::LightDirectionalShadowMode(p_mode));
	}
}

void Light3D::set_blend_splits(bool p_enable)
{
	blend_splits = p_enable;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_blend_splits(light, p_enable);
	}
}

void Light3D::set_sky_mode(SkyMode p_mode)
{
	sky_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_sky_mode(light, RSE::LightDirectionalSkyMode(p_mode));
	}
}

void Light3D::set_omni_shadow_mode(OmniShadowMode p_mode)
{
	omni_shadow_mode = p_mode;
	if (type == RSE::LIGHT_OMNI && light.is_valid() && Renderer::data) {
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(p_mode));
	}
}

void Light3D::set_area_size(const Vector2& p_size)
{
	area_size = p_size;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_size(light, area_size);
	}
}

void Light3D::set_area_texture(const Ref<Texture2D>& p_texture)
{
	area_texture = p_texture;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_texture(
			light, area_texture.is_valid() ? area_texture->get_rid() : RID());
	}
}

void Light3D::set_area_normalize_energy(bool p_enabled)
{
	area_normalize_energy = p_enabled;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_normalize_energy(light, p_enabled);
	}
}

PackedStringArray Light3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_scale().is_equal_approx(Vector3(1, 1, 1))) {
		warnings.push_back(RTR("A light's scale does not affect the visual size of the light."));
	}

	switch (type) {
	case RSE::LIGHT_OMNI: {
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_SPOT: {
		if (has_shadow() && get_param(PARAM_SPOT_ANGLE) >= 90.0) {
			warnings.push_back(
				RTR("A SpotLight3D with an angle wider than 90 degrees cannot cast shadows."));
		}
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_AREA: {
		if (get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture is not supported for area lights. Use the "
								   "area_texture field instead."));
		}
		if (area_texture.is_valid() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR("Rendering textured area lights is not implemented in the "
								   "Compatibility rendering mode."));
		}
		if (has_shadow() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR(
				"Rendering area light shadows does not work in the Compatibility rendering mode."));
		}
	} break;

	default:
		break;
	}

	return warnings;
}

Light3D::Light3D(RSE::LightType p_type) : type(p_type)
{
	_create_light_rid();
}

Light3D::~Light3D()
{
	_free_light_rid();
}

// ==========================================================================
// Merged from scene/3d/voxelizer.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/math/geometry_3d.h"
#include "scene/resources/texture.h"
// --- voxelizer.h ---
#include "scene/resources/multimesh.h"

class Image;

class Voxelizer {
public:
	enum BakeResult {
		BAKE_RESULT_OK,
		BAKE_RESULT_INVALID_PARAMETER,
		BAKE_RESULT_CANCELLED,
	};

	typedef bool (*BakeStepFunc)(int, int);

private:
	enum : uint32_t {
		CHILD_EMPTY = 0xFFFFFFFF
	};

	struct Cell {
		uint32_t children[8];
		float albedo[3] = {}; //albedo in RGB24
		float emission[3] = {}; //accumulated light in 16:16 fixed point (needs to be integer for moving lights fast)
		float normal[3] = {};
		float alpha = 0.0; //used for upsampling
		uint16_t x = 0;
		uint16_t y = 0;
		uint16_t z = 0;
		uint16_t level = 0;

		Cell() {
			for (int i = 0; i < 8; i++) {
				children[i] = CHILD_EMPTY;
			}
		}
	};

	Vector<Cell> bake_cells;
	int cell_subdiv = 0;

	struct CellSort {
		union {
			uint64_t key = 0;
			struct {
				uint64_t z : 16;
				uint64_t y : 16;
				uint64_t x : 16;
				uint64_t level : 16;
			};
		};

		int32_t index = 0;

		_FORCE_INLINE_ bool operator<(const CellSort &p_cell_sort) const {
			return key < p_cell_sort.key;
		}
	};

	struct MaterialCache {
		//128x128 textures
		Vector<Color> albedo;
		Vector<Color> emission;
	};

	HashMap<Ref<Material>, MaterialCache> material_cache;
	float exposure_normalization = 1.0;
	AABB original_bounds;
	AABB po2_bounds;
	int axis_cell_size[3] = {};

	Transform3D to_cell_space;

	int color_scan_cell_width = 4;
	int bake_texture_size = 128;
	float cell_size = 0.0;

	int max_original_cells = 0;
	int leaf_voxel_count = 0;

	Vector<Color> _get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add);
	MaterialCache _get_material_cache(Ref<Material> p_material);

	void _plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3 *p_vtx, const Vector3 *p_normal, const Vector2 *p_uv, const MaterialCache &p_material, const AABB &p_aabb);
	void _fixup_plot(int p_idx, int p_level);
	void _debug_mesh(int p_idx, int p_level, const AABB &p_aabb, Ref<MultiMesh> &p_multimesh, int &idx);

	bool sorted = false;
	void _sort();

public:
	void begin_bake(int p_subdiv, const AABB &p_bounds, float p_exposure_normalization);
	int get_bake_steps(Ref<Mesh> &p_mesh) const;
	BakeResult plot_mesh(const Transform3D &p_xform, Ref<Mesh> &p_mesh, const Vector<Ref<Material>> &p_materials, const Ref<Material> &p_override_material, BakeStepFunc p_bake_step_function);
	void end_bake();

	int get_voxel_gi_octree_depth() const;
	Vector3i get_voxel_gi_octree_size() const;
	int get_voxel_gi_cell_count() const;
	Vector<uint8_t> get_voxel_gi_octree_cells() const;
	Vector<uint8_t> get_voxel_gi_data_cells() const;
	Vector<int> get_voxel_gi_level_cell_count() const;
	BakeResult get_sdf_3d_image(Vector<uint8_t> &r_image, BakeStepFunc p_bake_step_function) const;

	Ref<MultiMesh> create_debug_multimesh();

	Transform3D get_to_cell_space_xform() const;
	Voxelizer();
};

static _FORCE_INLINE_ void get_uv_and_normal(const Vector3& p_pos, const Vector3* p_vtx,
	const Vector2* p_uv, const Vector3* p_normal, Vector2& r_uv, Vector3& r_normal)
{
	if (p_pos.is_equal_approx(p_vtx[0])) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[1])) {
		r_uv = p_uv[1];
		r_normal = p_normal[1];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[2])) {
		r_uv = p_uv[2];
		r_normal = p_normal[2];
		return;
	}

	Vector3 v0 = p_vtx[1] - p_vtx[0];
	Vector3 v1 = p_vtx[2] - p_vtx[0];
	Vector3 v2 = p_pos - p_vtx[0];

	real_t d00 = v0.dot(v0);
	real_t d01 = v0.dot(v1);
	real_t d11 = v1.dot(v1);
	real_t d20 = v2.dot(v0);
	real_t d21 = v2.dot(v1);
	real_t denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	real_t v = (d11 * d20 - d01 * d21) / denom;
	real_t w = (d00 * d21 - d01 * d20) / denom;
	real_t u = 1.0f - v - w;

	r_uv = p_uv[0] * u + p_uv[1] * v + p_uv[2] * w;
	r_normal = (p_normal[0] * u + p_normal[1] * v + p_normal[2] * w).normalized();
}

void Voxelizer::_plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3* p_vtx,
	const Vector3* p_normal, const Vector2* p_uv, const MaterialCache& p_material,
	const AABB& p_aabb)
{
	if (p_level == cell_subdiv) {
		// plot the face by guessing its albedo and emission value

		// find best axis to map to, for scanning values
		int closest_axis = 0;
		real_t closest_dot = 0;

		Plane plane = Plane(p_vtx[0], p_vtx[1], p_vtx[2]);
		Vector3 normal = plane.normal;

		for (int i = 0; i < 3; i++) {
			Vector3 axis;
			axis[i] = 1.0;
			real_t dot = Math::abs(normal.dot(axis));
			if (i == 0 || dot > closest_dot) {
				closest_axis = i;
				closest_dot = dot;
			}
		}

		Vector3 axis;
		axis[closest_axis] = 1.0;
		Vector3 t1;
		t1[(closest_axis + 1) % 3] = 1.0;
		Vector3 t2;
		t2[(closest_axis + 2) % 3] = 1.0;

		t1 *= p_aabb.size[(closest_axis + 1) % 3] / real_t(color_scan_cell_width);
		t2 *= p_aabb.size[(closest_axis + 2) % 3] / real_t(color_scan_cell_width);

		Color albedo_accum;
		Color emission_accum;
		Vector3 normal_accum;

		float alpha = 0.0;

		// map to a grid average in the best axis for this face
		for (int i = 0; i < color_scan_cell_width; i++) {
			Vector3 ofs_i = real_t(i) * t1;

			for (int j = 0; j < color_scan_cell_width; j++) {
				Vector3 ofs_j = real_t(j) * t2;

				Vector3 from = p_aabb.position + ofs_i + ofs_j;
				Vector3 to = from + t1 + t2 + axis * p_aabb.size[closest_axis];
				Vector3 half = (to - from) * 0.5;

				// is in this cell?
				if (!Geometry3D::triangle_box_overlap(from + half, half, p_vtx)) {
					continue; // face does not span this cell
				}

				// go from -size to +size*2 to avoid skipping collisions
				Vector3 ray_from = from + (t1 + t2) * 0.5 - axis * p_aabb.size[closest_axis];
				Vector3 ray_to = ray_from + axis * p_aabb.size[closest_axis] * 2;

				if (normal.dot(ray_from - ray_to) < 0) {
					SWAP(ray_from, ray_to);
				}

				Vector3 intersection;

				if (!plane.intersects_segment(ray_from, ray_to, &intersection)) {
					if (Math::abs(plane.distance_to(ray_from)) <
						Math::abs(plane.distance_to(ray_to))) {
						intersection = plane.project(ray_from);
					}
					else {
						intersection = plane.project(ray_to);
					}
				}

				intersection =
					Face3(p_vtx[0], p_vtx[1], p_vtx[2]).get_closest_point_to(intersection);

				Vector2 uv;
				Vector3 lnormal;
				get_uv_and_normal(intersection, p_vtx, p_uv, p_normal, uv, lnormal);
				if (lnormal == Vector3()) { // just in case normal is not provided
					lnormal = normal;
				}

				int uv_x = CLAMP(int(Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);
				int uv_y = CLAMP(int(Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);

				int ofs = uv_y * bake_texture_size + uv_x;
				albedo_accum.r += p_material.albedo[ofs].r;
				albedo_accum.g += p_material.albedo[ofs].g;
				albedo_accum.b += p_material.albedo[ofs].b;
				albedo_accum.a += p_material.albedo[ofs].a;

				emission_accum.r += p_material.emission[ofs].r;
				emission_accum.g += p_material.emission[ofs].g;
				emission_accum.b += p_material.emission[ofs].b;

				normal_accum += lnormal;

				alpha += 1.0;
			}
		}

		if (alpha == 0) {
			// could not in any way get texture information.. so use closest point to center

			Face3 f(p_vtx[0], p_vtx[1], p_vtx[2]);
			Vector3 inters = f.get_closest_point_to(p_aabb.get_center());

			Vector3 lnormal;
			Vector2 uv;
			get_uv_and_normal(inters, p_vtx, p_uv, p_normal, uv, normal);
			if (lnormal == Vector3()) { // just in case normal is not provided
				lnormal = normal;
			}

			int uv_x = CLAMP(
				Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);
			int uv_y = CLAMP(
				Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);

			int ofs = uv_y * bake_texture_size + uv_x;

			alpha = 1.0 / (color_scan_cell_width * color_scan_cell_width);

			albedo_accum.r = p_material.albedo[ofs].r * alpha;
			albedo_accum.g = p_material.albedo[ofs].g * alpha;
			albedo_accum.b = p_material.albedo[ofs].b * alpha;
			albedo_accum.a = p_material.albedo[ofs].a * alpha;

			emission_accum.r = p_material.emission[ofs].r * alpha;
			emission_accum.g = p_material.emission[ofs].g * alpha;
			emission_accum.b = p_material.emission[ofs].b * alpha;

			normal_accum = lnormal * alpha;

		}
		else {
			float accdiv = 1.0 / (color_scan_cell_width * color_scan_cell_width);
			alpha *= accdiv;

			albedo_accum.r *= accdiv;
			albedo_accum.g *= accdiv;
			albedo_accum.b *= accdiv;
			albedo_accum.a *= accdiv;

			emission_accum.r *= accdiv;
			emission_accum.g *= accdiv;
			emission_accum.b *= accdiv;

			normal_accum *= accdiv;
		}

		// put this temporarily here, corrected in a later step
		bake_cells.write[p_idx].albedo[0] += albedo_accum.r;
		bake_cells.write[p_idx].albedo[1] += albedo_accum.g;
		bake_cells.write[p_idx].albedo[2] += albedo_accum.b;
		bake_cells.write[p_idx].emission[0] += emission_accum.r;
		bake_cells.write[p_idx].emission[1] += emission_accum.g;
		bake_cells.write[p_idx].emission[2] += emission_accum.b;
		bake_cells.write[p_idx].normal[0] += normal_accum.x;
		bake_cells.write[p_idx].normal[1] += normal_accum.y;
		bake_cells.write[p_idx].normal[2] += normal_accum.z;
		bake_cells.write[p_idx].alpha += alpha;

	}
	else {
		// go down

		int half = (1 << cell_subdiv) >> (p_level + 1);
		for (int i = 0; i < 8; i++) {
			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			int nx = p_x;
			int ny = p_y;
			int nz = p_z;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
				nx += half;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
				ny += half;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
				nz += half;
			}
			// make sure to not plot beyond limits
			if (nx < 0 || nx >= axis_cell_size[0] || ny < 0 || ny >= axis_cell_size[1] || nz < 0 ||
				nz >= axis_cell_size[2]) {
				continue;
			}

			{
				AABB test_aabb = aabb;
				// test_aabb.grow_by(test_aabb.get_longest_axis_size()*0.05); //grow a bit to avoid
				// numerical error in real-time
				Vector3 qsize = test_aabb.size * 0.5; // quarter size, for fast aabb test

				if (!Geometry3D::triangle_box_overlap(test_aabb.position + qsize, qsize, p_vtx)) {
					// if (!Face3(p_vtx[0],p_vtx[1],p_vtx[2]).intersects_aabb2(aabb)) {
					// does not fit in child, go on
					continue;
				}
			}

			if (bake_cells[p_idx].children[i] == CHILD_EMPTY) {
				// sub cell must be created

				uint32_t child_idx = bake_cells.size();
				bake_cells.write[p_idx].children[i] = child_idx;
				bake_cells.resize(bake_cells.size() + 1);
				bake_cells.write[child_idx].level = p_level + 1;
				bake_cells.write[child_idx].x = nx / half;
				bake_cells.write[child_idx].y = ny / half;
				bake_cells.write[child_idx].z = nz / half;
			}

			_plot_face(bake_cells[p_idx].children[i], p_level + 1, nx, ny, nz, p_vtx, p_normal,
				p_uv, p_material, aabb);
		}
	}
}

Vector<Color> Voxelizer::_get_bake_texture(
	Ref<Image> p_image, const Color& p_color_mul, const Color& p_color_add)
{
	Vector<Color> ret;

	if (p_image.is_null() || p_image->is_empty()) {
		ret.resize(bake_texture_size * bake_texture_size);
		for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
			ret.write[i] = p_color_add;
		}

		return ret;
	}
	if (p_image->is_compressed()) {
		p_image->decompress();
	}
	p_image->convert(Image::FORMAT_RGBA8);
	p_image->resize(bake_texture_size, bake_texture_size, Image::INTERPOLATE_CUBIC);

	const uint8_t* r = p_image->get_data().ptr();
	ret.resize(bake_texture_size * bake_texture_size);

	for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
		Color c;
		Color src = Color(
			r[i * 4 + 0] / 255.0, r[i * 4 + 1] / 255.0, r[i * 4 + 2] / 255.0, r[i * 4 + 3] / 255.0);
		src = src.srgb_to_linear();

		c.r = src.r * p_color_mul.r + p_color_add.r;
		c.g = src.g * p_color_mul.g + p_color_add.g;
		c.b = src.b * p_color_mul.b + p_color_add.b;
		c.a = src.a;

		ret.write[i] = c;
	}

	return ret;
}

Voxelizer::MaterialCache Voxelizer::_get_material_cache(Ref<Material> p_material)
{
	// This way of obtaining materials is inaccurate and also does not support some compressed
	// formats very well.
	Ref<StandardMaterial3D> mat = p_material;

	Ref<Material> material = mat; // hack for now

	if (material_cache.has(material)) {
		return material_cache[material];
	}

	MaterialCache mc;

	if (mat.is_valid()) {
		Ref<Texture2D> albedo_tex = mat->get_texture(StandardMaterial3D::TEXTURE_ALBEDO);

		Ref<Image> img_albedo;
		if (albedo_tex.is_valid()) {
			img_albedo = albedo_tex->get_image();
			mc.albedo = _get_bake_texture(img_albedo, mat->get_albedo().srgb_to_linear(),
				Color(0, 0, 0)); // albedo texture, color is multiplicative
		}
		else {
			mc.albedo = _get_bake_texture(img_albedo, Color(1, 1, 1),
				mat->get_albedo().srgb_to_linear()); // no albedo texture, color is additive
		}
		if (mat->get_feature(StandardMaterial3D::FEATURE_EMISSION)) {
			Ref<Texture2D> emission_tex = mat->get_texture(StandardMaterial3D::TEXTURE_EMISSION);

			Color emission_col = mat->get_emission().srgb_to_linear();
			float emission_energy = mat->get_emission_energy_multiplier() * exposure_normalization;
			if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
				emission_energy *= mat->get_emission_intensity();
			}

			Ref<Image> img_emission;

			if (emission_tex.is_valid()) {
				img_emission = emission_tex->get_image();
			}

			if (mat->get_emission_operator() == StandardMaterial3D::EMISSION_OP_ADD) {
				mc.emission = _get_bake_texture(
					img_emission, Color(1, 1, 1) * emission_energy, emission_col * emission_energy);
			}
			else {
				mc.emission =
					_get_bake_texture(img_emission, emission_col * emission_energy, Color(0, 0, 0));
			}
		}
		else {
			Ref<Image> empty;
			mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
		}

	}
	else {
		Ref<Image> empty;

		mc.albedo = _get_bake_texture(empty, Color(0, 0, 0), Color(1, 1, 1));
		mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
	}

	material_cache[p_material] = mc;
	return mc;
}

void Voxelizer::_sort()
{
	// cells need to be sorted by level and coordinates
	// it is important that level has more priority (for compute), and that Z has the least,
	// given it may aid older implementations plot using GPU

	Vector<CellSort> sorted_cells;
	uint32_t cell_count = bake_cells.size();
	sorted_cells.resize(cell_count);
	{
		CellSort* sort_cellsp = sorted_cells.ptrw();
		const Cell* bake_cellsp = bake_cells.ptr();

		for (uint32_t i = 0; i < cell_count; i++) {
			sort_cellsp[i].x = bake_cellsp[i].x;
			sort_cellsp[i].y = bake_cellsp[i].y;
			sort_cellsp[i].z = bake_cellsp[i].z;
			sort_cellsp[i].level = bake_cellsp[i].level;
			sort_cellsp[i].index = i;
		}
	}

	sorted_cells.sort();

	// verify just in case, index 0 must be level 0
	ERR_FAIL_COND(sorted_cells[0].level != 0);

	Vector<Cell> new_bake_cells;
	new_bake_cells.resize(cell_count);
	Vector<uint32_t> reverse_map;

	{
		reverse_map.resize(cell_count);
		const CellSort* sort_cellsp = sorted_cells.ptr();
		uint32_t* reverse_mapp = reverse_map.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			reverse_mapp[sort_cellsp[i].index] = i;
		}
	}

	{
		const CellSort* sort_cellsp = sorted_cells.ptr();
		const Cell* bake_cellsp = bake_cells.ptr();
		const uint32_t* reverse_mapp = reverse_map.ptr();
		Cell* new_bake_cellsp = new_bake_cells.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			// copy to new cell
			new_bake_cellsp[i] = bake_cellsp[sort_cellsp[i].index];
			// remap children
			for (uint32_t j = 0; j < 8; j++) {
				if (new_bake_cellsp[i].children[j] != CHILD_EMPTY) {
					new_bake_cellsp[i].children[j] = reverse_mapp[new_bake_cellsp[i].children[j]];
				}
			}
		}
	}

	bake_cells = new_bake_cells;
	sorted = true;
}

void Voxelizer::_fixup_plot(int p_idx, int p_level)
{
	if (p_level == cell_subdiv) {
		leaf_voxel_count++;
		float alpha = bake_cells[p_idx].alpha;

		bake_cells.write[p_idx].albedo[0] /= alpha;
		bake_cells.write[p_idx].albedo[1] /= alpha;
		bake_cells.write[p_idx].albedo[2] /= alpha;

		// transfer emission to light
		bake_cells.write[p_idx].emission[0] /= alpha;
		bake_cells.write[p_idx].emission[1] /= alpha;
		bake_cells.write[p_idx].emission[2] /= alpha;

		bake_cells.write[p_idx].normal[0] /= alpha;
		bake_cells.write[p_idx].normal[1] /= alpha;
		bake_cells.write[p_idx].normal[2] /= alpha;

		Vector3 n(
			bake_cells[p_idx].normal[0], bake_cells[p_idx].normal[1], bake_cells[p_idx].normal[2]);
		if (n.length() < 0.01f) {
			// too much fight over normal, zero it
			bake_cells.write[p_idx].normal[0] = 0;
			bake_cells.write[p_idx].normal[1] = 0;
			bake_cells.write[p_idx].normal[2] = 0;
		}
		else {
			n.normalize();
			bake_cells.write[p_idx].normal[0] = n.x;
			bake_cells.write[p_idx].normal[1] = n.y;
			bake_cells.write[p_idx].normal[2] = n.z;
		}

		bake_cells.write[p_idx].alpha = 1.0;

		/*if (bake_light.size()) {
			for(int i=0;i<6;i++) {
			}
		}*/

	}
	else {
		// go down

		bake_cells.write[p_idx].emission[0] = 0;
		bake_cells.write[p_idx].emission[1] = 0;
		bake_cells.write[p_idx].emission[2] = 0;
		bake_cells.write[p_idx].normal[0] = 0;
		bake_cells.write[p_idx].normal[1] = 0;
		bake_cells.write[p_idx].normal[2] = 0;
		bake_cells.write[p_idx].albedo[0] = 0;
		bake_cells.write[p_idx].albedo[1] = 0;
		bake_cells.write[p_idx].albedo[2] = 0;

		float alpha_average = 0;

		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY) {
				continue;
			}

			_fixup_plot(child, p_level + 1);
			alpha_average += bake_cells[child].alpha;
		}

		bake_cells.write[p_idx].alpha = alpha_average / 8.0;
	}
}

void Voxelizer::begin_bake(int p_subdiv, const AABB& p_bounds, float p_exposure_normalization)
{
	sorted = false;
	original_bounds = p_bounds;
	cell_subdiv = p_subdiv;
	exposure_normalization = p_exposure_normalization;
	bake_cells.resize(1);
	material_cache.clear();

	// find out the actual real bounds, power of 2, which gets the highest subdivision
	po2_bounds = p_bounds;
	int longest_axis = po2_bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;
	leaf_voxel_count = 0;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		real_t axis_size = po2_bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= po2_bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}

		po2_bounds.size[i] = po2_bounds.size[longest_axis];
	}

	Transform3D to_bounds;
	to_bounds.basis.scale(Vector3(po2_bounds.size[longest_axis], po2_bounds.size[longest_axis],
		po2_bounds.size[longest_axis]));
	to_bounds.origin = po2_bounds.position;

	Transform3D to_grid;
	to_grid.basis.scale(Vector3(
		axis_cell_size[longest_axis], axis_cell_size[longest_axis], axis_cell_size[longest_axis]));

	to_cell_space = to_grid * to_bounds.affine_inverse();

	cell_size = po2_bounds.size[longest_axis] / axis_cell_size[longest_axis];
}

void Voxelizer::end_bake()
{
	if (!sorted) {
		_sort();
	}
	_fixup_plot(0, 0);
}

// create the data for rendering server

int Voxelizer::get_voxel_gi_octree_depth() const { return cell_subdiv; }

Vector3i Voxelizer::get_voxel_gi_octree_size() const
{
	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

int Voxelizer::get_voxel_gi_cell_count() const { return bake_cells.size(); }

Vector<uint8_t> Voxelizer::get_voxel_gi_octree_cells() const
{
	Vector<uint8_t> data;
	data.resize((8 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* children_cells = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			for (uint32_t j = 0; j < 8; j++) {
				children_cells[i * 8 + j] = cells[i].children[j];
			}
		}
	}

	return data;
}

Vector<uint8_t> Voxelizer::get_voxel_gi_data_cells() const
{
	Vector<uint8_t> data;
	data.resize((4 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* dataptr = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			{ // position

				uint32_t x = cells[i].x;
				uint32_t y = cells[i].y;
				uint32_t z = cells[i].z;

				uint32_t position = x;
				position |= y << 11;
				position |= z << 21;

				dataptr[i * 4 + 0] = position;
			}

			{ // albedo + alpha
				uint32_t rgba = uint32_t(CLAMP(cells[i].alpha * 255.0, 0, 255)) << 24; // a
				rgba |= uint32_t(CLAMP(cells[i].albedo[2] * 255.0, 0, 255)) << 16;	   // b
				rgba |= uint32_t(CLAMP(cells[i].albedo[1] * 255.0, 0, 255)) << 8;	   // g
				rgba |= uint32_t(CLAMP(cells[i].albedo[0] * 255.0, 0, 255));		   // r

				dataptr[i * 4 + 1] = rgba;
			}

			{ // emission, as rgbe9995
				Color emission =
					Color(cells[i].emission[0], cells[i].emission[1], cells[i].emission[2]);
				dataptr[i * 4 + 2] = emission.to_rgbe9995();
			}

			{ // normal

				Vector3 n(
					bake_cells[i].normal[0], bake_cells[i].normal[1], bake_cells[i].normal[2]);
				n.normalize();

				uint32_t normal = uint32_t(uint8_t(int8_t(CLAMP(n.x * 127.0, -128, 127))));
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.y * 127.0, -128, 127)))) << 8;
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.z * 127.0, -128, 127)))) << 16;

				dataptr[i * 4 + 3] = normal;
			}
		}
	}

	return data;
}

Vector<int> Voxelizer::get_voxel_gi_level_cell_count() const
{
	uint32_t cell_count = bake_cells.size();
	const Cell* cells = bake_cells.ptr();
	Vector<int> level_count;
	level_count.resize(cell_subdiv + 1); // remember, always x+1 levels for x subdivisions
	{
		int* w = level_count.ptrw();
		for (int i = 0; i < cell_subdiv + 1; i++) {
			w[i] = 0;
		}

		for (uint32_t i = 0; i < cell_count; i++) {
			w[cells[i].level]++;
		}
	}

	return level_count;
}

// euclidean distance computation based on:
// https://prideout.net/blog/distance_fields/

#define square(m_s) ((m_s) * (m_s))
#define BIG_VAL 1e20

/* dt of 1d function using squared distance */
static void edt(float* f, int stride, int n)
{
	float* d = (float*)alloca(sizeof(float) * n + sizeof(int) * n + sizeof(float) * (n + 1));
	int* v = reinterpret_cast<int*>(&(d[n]));
	float* z = reinterpret_cast<float*>(&v[n]);

	int k = 0;
	v[0] = 0;
	z[0] = -BIG_VAL;
	z[1] = +BIG_VAL;
	for (int q = 1; q <= n - 1; q++) {
		float s =
			((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		while (s <= z[k]) {
			k--;
			s = ((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) /
				(2 * q - 2 * v[k]);
		}
		k++;
		v[k] = q;

		z[k] = s;
		z[k + 1] = +BIG_VAL;
	}

	k = 0;
	for (int q = 0; q <= n - 1; q++) {
		while (z[k + 1] < q) {
			k++;
		}
		d[q] = square(q - v[k]) + f[v[k] * stride];
	}

	for (int i = 0; i < n; i++) {
		f[i * stride] = d[i];
	}
}

#undef square

Voxelizer::BakeResult Voxelizer::get_sdf_3d_image(
	Vector<uint8_t>& r_image, BakeStepFunc p_bake_step_function) const
{
	Vector3i octree_size = get_voxel_gi_octree_size();

	uint32_t float_count = octree_size.x * octree_size.y * octree_size.z;
	float* work_memory = memnew_arr(float, float_count);
	for (uint32_t i = 0; i < float_count; i++) {
		work_memory[i] = BIG_VAL;
	}

	uint32_t y_mult = octree_size.x;
	uint32_t z_mult = y_mult * octree_size.y;

	// plot solid cells
	{
		const Cell* cells = bake_cells.ptr();
		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			if (cells[i].level < cell_subdiv) {
				continue; // do not care about this level
			}

			work_memory[cells[i].x + cells[i].y * y_mult + cells[i].z * z_mult] = 0;
		}
	}

	// process in each direction

	int bake_total = octree_size.x * 2 + octree_size.y, bake_current = 0;

	// xy->z

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.y; j++) {
			edt(&work_memory[i + j * y_mult], z_mult, octree_size.z);
		}
	}

	// xz->y

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i + j * z_mult], y_mult, octree_size.y);
		}
	}

	// yz->x
	for (int i = 0; i < octree_size.y; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i * y_mult + j * z_mult], 1, octree_size.x);
		}
	}

	r_image.resize(float_count);
	{
		uint8_t* w = r_image.ptrw();
		for (uint32_t i = 0; i < float_count; i++) {
			uint32_t d = uint32_t(Math::sqrt(work_memory[i]));
			if (d == 0) {
				w[i] = 0;
			}
			else {
				w[i] = MIN(d, 254u) + 1;
			}
		}
	}

	memdelete_arr(work_memory);

	return BAKE_RESULT_OK;
}

#undef BIG_VAL

void Voxelizer::_debug_mesh(
	int p_idx, int p_level, const AABB& p_aabb, Ref<MultiMesh>& p_multimesh, int& idx)
{
	if (p_level == cell_subdiv - 1) {
		Vector3 center = p_aabb.get_center();
		Transform3D xform;
		xform.origin = center;
		xform.basis.scale(p_aabb.size * 0.5);
		p_multimesh->set_instance_transform(idx, xform);
		Color col;
		col = Color(
			bake_cells[p_idx].albedo[0], bake_cells[p_idx].albedo[1], bake_cells[p_idx].albedo[2]);
		// Color col = Color(bake_cells[p_idx].emission[0], bake_cells[p_idx].emission[1],
		// bake_cells[p_idx].emission[2]);
		p_multimesh->set_instance_color(idx, col);

		idx++;

	}
	else {
		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY || child >= (uint32_t)max_original_cells) {
				continue;
			}

			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
			}

			_debug_mesh(bake_cells[p_idx].children[i], p_level + 1, aabb, p_multimesh, idx);
		}
	}
}

Transform3D Voxelizer::get_to_cell_space_xform() const { return to_cell_space; }

Voxelizer::Voxelizer() {}

// ==========================================================================
// Merged from scene/3d/lightmap_gi.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/delaunay_3d.h"
#include "core/math/geometry_3d.h"
#include "lightmap_gi.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For lightmapper_rd.
#include "scene/3d/light_3d.h"
#include "scene/3d/lightmap_probe.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/camera_attributes.h"
#include "scene/resources/environment.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/sky.h"
#include "servers/rendering/renderer.h"

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
#include "servers/display/display_server.h"
#endif

#if defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
#include "core/os/os.h"
#endif

void LightmapGIData::add_user(
	const NodePath& p_path, const Rect2& p_uv_scale, int p_slice_index, int32_t p_sub_instance)
{
	User user;
	user.path = p_path;
	user.uv_scale = p_uv_scale;
	user.slice_index = p_slice_index;
	user.sub_instance = p_sub_instance;
	users.push_back(user);
}

int LightmapGIData::get_user_count() const { return users.size(); }

NodePath LightmapGIData::get_user_path(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), NodePath());
	return users[p_user].path;
}

int32_t LightmapGIData::get_user_sub_instance(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].sub_instance;
}

Rect2 LightmapGIData::get_user_lightmap_uv_scale(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), Rect2());
	return users[p_user].uv_scale;
}

int LightmapGIData::get_user_lightmap_slice_index(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].slice_index;
}

void LightmapGIData::clear_users() { users.clear(); }

RID LightmapGIData::get_rid() const { return lightmap; }

void LightmapGIData::clear() { users.clear(); }

void LightmapGIData::_reset_lightmap_textures()
{
	Renderer::lightmap_set_textures(lightmap,
		combined_light_texture.is_valid() ? combined_light_texture->get_rid() : RID(),
		uses_spherical_harmonics);
}

void LightmapGIData::_reset_shadowmask_textures()
{
	Renderer::lightmap_set_shadowmask_textures(lightmap,
		combined_shadowmask_texture.is_valid() ? combined_shadowmask_texture->get_rid() : RID());
}

void LightmapGIData::set_uses_spherical_harmonics(bool p_enable)
{
	uses_spherical_harmonics = p_enable;
	_reset_lightmap_textures();
}

bool LightmapGIData::is_using_spherical_harmonics() const { return uses_spherical_harmonics; }

void LightmapGIData::_set_uses_packed_directional(bool p_enable)
{
	_uses_packed_directional = p_enable;
}

bool LightmapGIData::_is_using_packed_directional() const { return _uses_packed_directional; }

void LightmapGIData::update_shadowmask_mode(ShadowmaskMode p_mode)
{
	Renderer::lightmap_set_shadowmask_mode(lightmap, (RSE::ShadowmaskMode)p_mode);
}

LightmapGIData::ShadowmaskMode LightmapGIData::get_shadowmask_mode() const
{
	return (ShadowmaskMode)Renderer::lightmap_get_shadowmask_mode(lightmap);
}

void LightmapGIData::set_capture_data(const AABB& p_bounds, bool p_interior,
	const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree,
	float p_baked_exposure, uint32_t p_lightprobe_hash)
{
	if (p_points.size()) {
		int pc = p_points.size();
		ERR_FAIL_COND(pc * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
		Renderer::lightmap_set_probe_capture_data(
			lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
		Renderer::lightmap_set_probe_bounds(lightmap, p_bounds);
		Renderer::lightmap_set_probe_interior(lightmap, p_interior);
	}
	else {
		Renderer::lightmap_set_probe_capture_data(lightmap, PackedVector3Array(),
			PackedColorArray(), PackedInt32Array(), PackedInt32Array());
		Renderer::lightmap_set_probe_bounds(lightmap, AABB());
		Renderer::lightmap_set_probe_interior(lightmap, false);
	}
	Renderer::lightmap_set_baked_exposure_normalization(lightmap, p_baked_exposure);
	baked_exposure = p_baked_exposure;
	lightprobe_hash = p_lightprobe_hash;
	interior = p_interior;
	bounds = p_bounds;
}

PackedVector3Array LightmapGIData::get_capture_points() const
{
	return Renderer::lightmap_get_probe_capture_points(lightmap);
}

PackedColorArray LightmapGIData::get_capture_sh() const
{
	return Renderer::lightmap_get_probe_capture_sh(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_tetrahedra() const
{
	return Renderer::lightmap_get_probe_capture_tetrahedra(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_bsp_tree() const
{
	return Renderer::lightmap_get_probe_capture_bsp_tree(lightmap);
}

uint32_t LightmapGIData::get_lightprobe_hash() const { return lightprobe_hash; }

AABB LightmapGIData::get_capture_bounds() const { return bounds; }

bool LightmapGIData::is_interior() const { return interior; }

float LightmapGIData::get_baked_exposure() const { return baked_exposure; }

LightmapGIData::LightmapGIData() { lightmap = Renderer::lightmap_create(); }

LightmapGIData::~LightmapGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(lightmap);
}

///////////////////////////

int LightmapGI::_bsp_get_simplex_side(const LocalVector<Vector3>& p_points,
	const LocalVector<BSPSimplex>& p_simplices, const Plane& p_plane, uint32_t p_simplex) const
{
	int over = 0;
	int under = 0;
	const BSPSimplex& s = p_simplices[p_simplex];
	for (int i = 0; i < 4; i++) {
		const Vector3 v = p_points[s.vertices[i]];
		// The tolerance used here comes from experiments on scenes up to
		// 1000x1000x100 meters. If it's any smaller, some simplices will
		// appear to self-intersect due to a lack of precision in Plane.
		if (p_plane.has_point(v, 1.0 / (1 << 13))) {
			// Coplanar.
		}
		else if (p_plane.is_point_over(v)) {
			over++;
		}
		else {
			under++;
		}
	}

	ERR_FAIL_COND_V(under == 0 && over == 0,
		-2); // should never happen, we discarded flat simplices before, but in any case drop it
			 // from the bsp tree and throw an error
	if (under == 0) {
		return 1; // all over
	}
	else if (over == 0) {
		return -1; // all under
	}
	else {
		return 0; // crossing
	}
}

// #define DEBUG_BSP

int32_t LightmapGI::_compute_bsp_tree(const LocalVector<Vector3>& p_points,
	const LocalVector<Plane>& p_planes, LocalVector<int32_t>& planes_tested,
	const LocalVector<BSPSimplex>& p_simplices, const LocalVector<int32_t>& p_simplex_indices,
	LocalVector<BSPNode>& bsp_nodes)
{
	ERR_FAIL_COND_V(p_simplex_indices.size() < 2, -1);

	int32_t node_index = (int32_t)bsp_nodes.size();
	bsp_nodes.push_back(BSPNode());

	// test with all the simplex planes
	Plane best_plane;
	float best_plane_score = -1.0;

	for (const int idx : p_simplex_indices) {
		const BSPSimplex& s = p_simplices[idx];
		for (int j = 0; j < 4; j++) {
			uint32_t plane_index = s.planes[j];
			if (planes_tested[plane_index] == node_index) {
				continue; // tested this plane already
			}

			planes_tested[plane_index] = node_index;

			static const int face_order[4][3] = {{0, 1, 2}, {0, 2, 3}, {0, 1, 3}, {1, 2, 3}};

			// despite getting rid of plane duplicates, we should still use here the actual plane to
			// avoid numerical error from thinking this same simplex is intersecting rather than on
			// a side
			Vector3 v0 = p_points[s.vertices[face_order[j][0]]];
			Vector3 v1 = p_points[s.vertices[face_order[j][1]]];
			Vector3 v2 = p_points[s.vertices[face_order[j][2]]];

			Plane plane(v0, v1, v2);

			// test with all the simplices
			int over_count = 0;
			int under_count = 0;

			for (const int& index : p_simplex_indices) {
				int side = _bsp_get_simplex_side(p_points, p_simplices, plane, index);
				if (side == -2) {
					continue; // this simplex is invalid, skip for now
				}
				else if (side < 0) {
					under_count++;
				}
				else if (side > 0) {
					over_count++;
				}
			}

			if (under_count == 0 && over_count == 0) {
				continue; // most likely precision issue with a flat simplex, do not try this plane
			}

			if (under_count > over_count) { // make sure under is always less than over, so we can
											// compute the same ratio
				SWAP(under_count, over_count);
			}

			float score = 0; // by default, score is 0 (worst)
			if (over_count > 0) {
				// Simplices that are intersected by the plane are moved into both the over
				// and under subtrees which makes the entire tree deeper, so the best plane
				// will have the least intersections while separating the simplices evenly.
				float balance = float(under_count) / over_count;
				float separation = float(over_count + under_count) / p_simplex_indices.size();
				score = balance * separation * separation;
			}

			if (score > best_plane_score) {
				best_plane = plane;
				best_plane_score = score;
			}
		}
	}

	// We often end up with two (or on rare occasions, three) simplices that are
	// either disjoint or share one vertex and don't have a separating plane
	// among their faces. The fallback is to loop through new planes created
	// with one vertex of the first simplex and two vertices of the second until
	// we find a winner.
	if (best_plane_score == 0) {
		const BSPSimplex& simplex0 = p_simplices[p_simplex_indices[0]];
		const BSPSimplex& simplex1 = p_simplices[p_simplex_indices[1]];

		for (uint32_t i = 0; i < 4 && !best_plane_score; i++) {
			Vector3 v0 = p_points[simplex0.vertices[i]];
			for (uint32_t j = 0; j < 3 && !best_plane_score; j++) {
				if (simplex0.vertices[i] == simplex1.vertices[j]) {
					break;
				}
				Vector3 v1 = p_points[simplex1.vertices[j]];
				for (uint32_t k = j + 1; k < 4; k++) {
					if (simplex0.vertices[i] == simplex1.vertices[k]) {
						break;
					}
					Vector3 v2 = p_points[simplex1.vertices[k]];

					Plane plane = Plane(v0, v1, v2);
					if (plane ==
						Plane()) { // When v0, v1, and v2 are collinear, they can't form a plane.
						continue;
					}
					int32_t side0 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[0]);
					int32_t side1 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[1]);
					if ((side0 == 1 && side1 == -1) || (side0 == -1 && side1 == 1)) {
						best_plane = plane;
						best_plane_score = 1.0;
						break;
					}
				}
			}
		}
	}

	LocalVector<int32_t> indices_over;
	LocalVector<int32_t> indices_under;

	// split again, but add to list
	for (const uint32_t index : p_simplex_indices) {
		int side = _bsp_get_simplex_side(p_points, p_simplices, best_plane, index);

		if (side == -2) {
			continue; // simplex sits on the plane, does not make sense to use it
		}
		if (side <= 0) {
			indices_under.push_back(index);
		}

		if (side >= 0) {
			indices_over.push_back(index);
		}
	}

#ifdef DEBUG_BSP
	print_line("node " + itos(node_index) + " found plane: " + best_plane +
			   " score:" + rtos(best_plane_score) + " - over " + itos(indices_over.size()) +
			   " under " + itos(indices_under.size()) + " intersecting " + itos(intersecting));
#endif

	if (best_plane_score < 0.0 || indices_over.size() == p_simplex_indices.size() ||
		indices_under.size() == p_simplex_indices.size()) {
		// Failed to separate the tetrahedrons using planes
		// this means Delaunay broke at some point.
		// Luckily, because we are using tetrahedrons, we can resort to
		// less precise but still working ways to generate the separating plane
		// this will most likely look bad when interpolating, but at least it will not crash.
		// and the artifact will most likely also be very small, so too difficult to notice.

		// find the longest axis

		WARN_PRINT("Inconsistency found in triangulation while building BSP, probe interpolation "
				   "quality may degrade a bit.");

		LocalVector<Vector3> centers;
		AABB bounds_all;
		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			AABB bounds;
			for (uint32_t j = 0; j < 4; j++) {
				Vector3 p = p_points[p_simplices[p_simplex_indices[i]].vertices[j]];
				if (j == 0) {
					bounds.position = p;
				}
				else {
					bounds.expand_to(p);
				}
			}
			if (i == 0) {
				centers.push_back(bounds.get_center());
			}
			else {
				bounds_all.merge_with(bounds);
			}
		}
		Vector3::Axis longest_axis = Vector3::Axis(bounds_all.get_longest_axis_index());

		// find the simplex that will go under
		uint32_t min_d_idx = 0xFFFFFFFF;
		float min_d_dist = 1e20;

		for (uint32_t i = 0; i < centers.size(); i++) {
			if (centers[i][longest_axis] < min_d_dist) {
				min_d_idx = i;
				min_d_dist = centers[i][longest_axis];
			}
		}
		// rebuild best_plane and over/under arrays
		best_plane = Plane();
		best_plane.normal[longest_axis] = 1.0;
		best_plane.d = min_d_dist;

		indices_under.clear();
		indices_under.push_back(min_d_idx);

		indices_over.clear();

		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			if (i == min_d_idx) {
				continue;
			}
			indices_over.push_back(p_simplex_indices[i]);
		}
	}

	BSPNode node;
	node.plane = best_plane;

	if (indices_under.is_empty()) {
		// nothing to do here
		node.under = BSPNode::EMPTY_LEAF;
	}
	else if (indices_under.size() == 1) {
		node.under = -(indices_under[0] + 1);
	}
	else {
		node.under = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_under, bsp_nodes);
	}

	if (indices_over.is_empty()) {
		// nothing to do here
		node.over = BSPNode::EMPTY_LEAF;
	}
	else if (indices_over.size() == 1) {
		node.over = -(indices_over[0] + 1);
	}
	else {
		node.over = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_over, bsp_nodes);
	}

	bsp_nodes[node_index] = node;

	return node_index;
}

bool LightmapGI::_lightmap_bake_step_function(
	float p_completion, const String& p_text, void* ud, bool p_refresh)
{
	BakeStepUD* bsud = (BakeStepUD*)ud;
	bool ret = false;
	if (bsud->func) {
		ret =
			bsud->func(bsud->from_percent + p_completion * (bsud->to_percent - bsud->from_percent),
				p_text, bsud->ud, p_refresh);
	}
	return ret;
}

void LightmapGI::_plot_triangle_into_octree(
	GenProbesOctree* p_cell, float p_cell_size, const Vector3* p_triangle)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		uint32_t half_size = p_cell->size / 2;
		if (i & 1) {
			pos.x += half_size;
		}
		if (i & 2) {
			pos.y += half_size;
		}
		if (i & 4) {
			pos.z += half_size;
		}

		AABB subcell;
		subcell.position = Vector3(pos) * p_cell_size;
		subcell.size = Vector3(half_size, half_size, half_size) * p_cell_size;

		if (!Geometry3D::triangle_box_overlap(
				subcell.get_center(), subcell.size * 0.5, p_triangle)) {
			continue;
		}

		if (p_cell->children[i] == nullptr) {
			GenProbesOctree* child = memnew(GenProbesOctree);
			child->offset = pos;
			child->size = half_size;
			p_cell->children[i] = child;
		}

		if (half_size > 1) {
			// still levels missing
			_plot_triangle_into_octree(p_cell->children[i], p_cell_size, p_triangle);
		}
	}
}

void LightmapGI::_gen_new_positions_from_octree(const GenProbesOctree* p_cell, float p_cell_size,
	const Vector<Vector3>& probe_positions, LocalVector<Vector3>& new_probe_positions,
	HashMap<Vector3i, bool>& positions_used, const AABB& p_bounds)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		if (i & 1) {
			pos.x += p_cell->size;
		}
		if (i & 2) {
			pos.y += p_cell->size;
		}
		if (i & 4) {
			pos.z += p_cell->size;
		}

		if (p_cell->size == 1 && !positions_used.has(pos)) {
			// new position to insert!
			Vector3 real_pos = p_bounds.position + Vector3(pos) * p_cell_size;
			// see if a user submitted probe is too close
			int ppcount = probe_positions.size();
			const Vector3* pp = probe_positions.ptr();
			bool exists = false;
			for (int j = 0; j < ppcount; j++) {
				if (pp[j].distance_to(real_pos) < (p_cell_size * 0.5f)) {
					exists = true;
					break;
				}
			}

			if (!exists) {
				new_probe_positions.push_back(real_pos);
			}

			positions_used[pos] = true;
		}

		if (p_cell->children[i] != nullptr) {
			_gen_new_positions_from_octree(p_cell->children[i], p_cell_size, probe_positions,
				new_probe_positions, positions_used, p_bounds);
		}
	}
}

void LightmapGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void LightmapGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (light_data.is_valid()) {
			ERR_FAIL_COND_MSG(light_data->is_using_spherical_harmonics() &&
								  !light_data->_is_using_packed_directional(),
				vformat("%s (%s): The directional lightmap textures are stored in a format that "
						"isn't supported anymore. Please bake lightmaps again to make lightmaps "
						"display from this node again.",
					get_light_data()->get_path(), get_name()));

			if (last_owner && last_owner != get_owner()) {
				light_data->clear_users();
			}

			_assign_lightmaps();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		last_owner = get_owner();

		if (light_data.is_valid()) {
			_clear_lightmaps();
		}
	} break;
	}
}

Ref<LightmapGIData> LightmapGI::get_light_data() const { return light_data; }

void LightmapGI::set_bake_quality(BakeQuality p_quality) { bake_quality = p_quality; }

LightmapGI::BakeQuality LightmapGI::get_bake_quality() const { return bake_quality; }

AABB LightmapGI::get_aabb() const { return AABB(); }

bool LightmapGI::is_using_denoiser() const { return use_denoiser; }

void LightmapGI::set_denoiser_strength(float p_denoiser_strength)
{
	denoiser_strength = p_denoiser_strength;
}

float LightmapGI::get_denoiser_strength() const { return denoiser_strength; }

void LightmapGI::set_denoiser_range(int p_denoiser_range) { denoiser_range = p_denoiser_range; }

int LightmapGI::get_denoiser_range() const { return denoiser_range; }

void LightmapGI::set_directional(bool p_enable) { directional = p_enable; }

bool LightmapGI::is_directional() const { return directional; }

LightmapGIData::ShadowmaskMode LightmapGI::get_shadowmask_mode() const { return shadowmask_mode; }

void LightmapGI::set_use_texture_for_bounces(bool p_enable) { use_texture_for_bounces = p_enable; }

bool LightmapGI::is_using_texture_for_bounces() const { return use_texture_for_bounces; }

void LightmapGI::set_interior(bool p_enable) { interior = p_enable; }

bool LightmapGI::is_interior() const { return interior; }

LightmapGI::EnvironmentMode LightmapGI::get_environment_mode() const { return environment_mode; }

void LightmapGI::set_environment_custom_sky(const Ref<Sky>& p_sky)
{
	environment_custom_sky = p_sky;
}

Ref<Sky> LightmapGI::get_environment_custom_sky() const { return environment_custom_sky; }

void LightmapGI::set_environment_custom_color(const Color& p_color)
{
	environment_custom_color = p_color;
}

Color LightmapGI::get_environment_custom_color() const { return environment_custom_color; }

void LightmapGI::set_environment_custom_energy(float p_energy)
{
	environment_custom_energy = p_energy;
}

float LightmapGI::get_environment_custom_energy() const { return environment_custom_energy; }

void LightmapGI::set_bounces(int p_bounces)
{
	ERR_FAIL_COND(p_bounces < 0 || p_bounces > 16);
	bounces = p_bounces;
}

int LightmapGI::get_bounces() const { return bounces; }

void LightmapGI::set_bounce_indirect_energy(float p_indirect_energy)
{
	ERR_FAIL_COND(p_indirect_energy < 0.0);
	bounce_indirect_energy = p_indirect_energy;
}

float LightmapGI::get_bounce_indirect_energy() const { return bounce_indirect_energy; }

void LightmapGI::set_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.00001);
	bias = p_bias;
}

float LightmapGI::get_bias() const { return bias; }

void LightmapGI::set_texel_scale(float p_multiplier)
{
	ERR_FAIL_COND(p_multiplier < (0.01 - CMP_EPSILON));
	texel_scale = p_multiplier;
}

float LightmapGI::get_texel_scale() const { return texel_scale; }

void LightmapGI::set_max_texture_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 2048, vformat("The LightmapGI maximum texture size supplied (%d) is "
											 "too small. The minimum allowed value is 2048.",
										 p_size));
	ERR_FAIL_COND_MSG(p_size > 16384, vformat("The LightmapGI maximum texture size supplied (%d) "
											  "is too large. The maximum allowed value is 16384.",
										  p_size));
	max_texture_size = p_size;
}

int LightmapGI::get_max_texture_size() const { return max_texture_size; }

bool LightmapGI::is_supersampling_enabled() const { return supersampling_enabled; }

void LightmapGI::set_supersampling_factor(float p_factor)
{
	ERR_FAIL_COND(p_factor < 1);

	supersampling_factor = p_factor;
}

float LightmapGI::get_supersampling_factor() const { return supersampling_factor; }

void LightmapGI::set_generate_probes(GenerateProbes p_generate_probes)
{
	gen_probes = p_generate_probes;
}

LightmapGI::GenerateProbes LightmapGI::get_generate_probes() const { return gen_probes; }

void LightmapGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;
}

Ref<CameraAttributes> LightmapGI::get_camera_attributes() const { return camera_attributes; }

PackedStringArray LightmapGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
	if (!DisplayServer::can_create_rendering_device()) {
		warnings.push_back(vformat(
			RTR("Lightmaps can only be baked from a GPU that supports the RenderingDevice "
				"backends.\nYour GPU (%s) does not support RenderingDevice, as it does not support "
				"Vulkan, Direct3D 12, or Metal.\nLightmap baking will not be available on this "
				"device, although rendering existing baked lightmaps will work."),
			Renderer::get_video_adapter_name()));
		return warnings;
	}

	if (shadowmask_mode != LightmapGIData::SHADOWMASK_MODE_NONE && light_data.is_valid() &&
		!light_data->has_shadowmask_textures()) {
		warnings.push_back(RTR("The lightmap has no baked shadowmask textures. Please rebake with "
							   "the Shadowmask Mode set to anything other than None."));
	}

#elif defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	warnings.push_back(vformat(
		RTR("Lightmaps cannot be baked on %s. Rendering existing baked lightmaps will still work."),
		OS::get_name()));
#else
	warnings.push_back(RTR("Lightmaps cannot be baked, as the `lightmapper_rd` module was disabled "
						   "at compile-time. Rendering existing baked lightmaps will still work."));
#endif

	return warnings;
}

LightmapGI::LightmapGI() {}



bool LightmapGIData::has_shadowmask_textures() { return false; }

void LightmapGI::_clear_lightmaps() {}

void LightmapGI::_assign_lightmaps() {}

// ==========================================================================
// Merged from scene/3d/lightmap_probe.cpp
// ==========================================================================

#include "lightmap_probe.h"

LightmapProbe::LightmapProbe() {
}

// ==========================================================================
// Merged from scene/3d/lightmapper.cpp
// ==========================================================================

#include "lightmapper.h"

#include "modules/modules_enabled.gen.h"

Ref<LightmapDenoiser> (*LightmapDenoiser::create_function)() = nullptr;

Ref<LightmapDenoiser> LightmapDenoiser::create()
{
	if (create_function) {
		return Ref<LightmapDenoiser>(create_function());
	}
	return Ref<LightmapDenoiser>();
}

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<LightmapRaycaster> LightmapRaycaster::create() { return Ref<LightmapRaycaster>(); }

bool LightmapRaycaster::intersect(Ray& p_ray) { return false; }

void LightmapRaycaster::intersect(Vector<Ray>& r_rays) {}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id) {}

void LightmapRaycaster::commit() {}

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void LightmapRaycaster::clear_mesh_filter() {}

LightmapRaycaster::LightmapRaycaster() {}

LightmapRaycaster::~LightmapRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

#ifndef MODULE_LIGHTMAPPER_RD_ENABLED

Ref<Lightmapper> Lightmapper::create() { return Ref<Lightmapper>(); }

Lightmapper::Lightmapper() {}

Lightmapper::~Lightmapper() {}

void Lightmapper::add_mesh(const MeshData& p_mesh) {}

void Lightmapper::add_directional_light(const String& p_name, bool p_static,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_angular_distance, float p_shadow_blur)
{
}

void Lightmapper::add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
	float p_attenuation, float p_size, float p_shadow_blur)
{
}

void Lightmapper::add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation, float p_size,
	float p_shadow_blur)
{
}

void Lightmapper::add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, const Vector3& p_area_width, const Vector3& p_area_height,
	float p_size, float p_shadow_blur, const Rect2& p_texture_rect, float p_max_mipmap)
{
}

void Lightmapper::add_area_light_atlas(
	const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data)
{
}

void Lightmapper::add_probe(const Vector3& p_position) {}

Lightmapper::BakeError Lightmapper::bake(BakeQuality p_quality, bool p_use_denoiser,
	float p_denoiser_strength, int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy,
	float p_bias, int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask,
	bool p_texture_for_bounces, GenerateProbes p_generate_probes,
	const Ref<Image>& p_environment_panorama, const Basis& p_environment_transform,
	BakeStepFunc p_step_function, void* p_step_userdata, float p_exposure_normalization,
	float p_supersampling_factor)
{
	return BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES;
}

int Lightmapper::get_bake_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_bake_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_shadowmask_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_shadowmask_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_bake_mesh_count() const { return 0; }

Rect2 Lightmapper::get_bake_mesh_uv_scale(int p_index) const { return Rect2(); }

int Lightmapper::get_bake_mesh_texture_slice(int p_index) const { return 0; }

int Lightmapper::get_bake_probe_count() const { return 0; }

Vector3 Lightmapper::get_bake_probe_point(int p_probe) const { return Vector3(); }

Vector<Color> Lightmapper::get_bake_probe_sh(int p_probe) const { return Vector<Color>(); }

#endif // MODULE_LIGHTMAPPER_RD_ENABLED

// ==========================================================================
// Merged from scene/3d/reflection_probe.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "reflection_probe.h"
#include "servers/rendering/renderer.h"

void ReflectionProbe::set_intensity(float p_intensity)
{
	intensity = p_intensity;
	Renderer::reflection_probe_set_intensity(probe, p_intensity);
}

float ReflectionProbe::get_intensity() const { return intensity; }

float ReflectionProbe::get_blend_distance() const { return blend_distance; }

ReflectionProbe::AmbientMode ReflectionProbe::get_ambient_mode() const { return ambient_mode; }

void ReflectionProbe::set_ambient_color(Color p_ambient)
{
	ambient_color = p_ambient;
	Renderer::reflection_probe_set_ambient_color(probe, p_ambient);
}

void ReflectionProbe::set_ambient_color_energy(float p_energy)
{
	ambient_color_energy = p_energy;
	Renderer::reflection_probe_set_ambient_energy(probe, p_energy);
}

float ReflectionProbe::get_ambient_color_energy() const { return ambient_color_energy; }

Color ReflectionProbe::get_ambient_color() const { return ambient_color; }

void ReflectionProbe::set_max_distance(float p_distance)
{
	max_distance = CLAMP(p_distance, 0.0, 262'144.0);
	// Reflection rendering breaks if distance exceeds 262,144 units (due to floating-point
	// precision with the near plane being 0.01).
	Renderer::reflection_probe_set_max_distance(probe, max_distance);
}

float ReflectionProbe::get_max_distance() const { return max_distance; }

void ReflectionProbe::set_mesh_lod_threshold(float p_pixels)
{
	mesh_lod_threshold = p_pixels;
	Renderer::reflection_probe_set_mesh_lod_threshold(probe, p_pixels);
}

float ReflectionProbe::get_mesh_lod_threshold() const { return mesh_lod_threshold; }

Vector3 ReflectionProbe::get_size() const { return size; }

Vector3 ReflectionProbe::get_origin_offset() const { return origin_offset; }

void ReflectionProbe::set_enable_box_projection(bool p_enable)
{
	box_projection = p_enable;
	Renderer::reflection_probe_set_enable_box_projection(probe, p_enable);
}

bool ReflectionProbe::is_box_projection_enabled() const { return box_projection; }

void ReflectionProbe::set_as_interior(bool p_enable)
{
	interior = p_enable;
	Renderer::reflection_probe_set_as_interior(probe, interior);
}

bool ReflectionProbe::is_set_as_interior() const { return interior; }

void ReflectionProbe::set_enable_shadows(bool p_enable)
{
	enable_shadows = p_enable;
	Renderer::reflection_probe_set_enable_shadows(probe, p_enable);
}

bool ReflectionProbe::are_shadows_enabled() const { return enable_shadows; }

void ReflectionProbe::set_cull_mask(uint32_t p_layers)
{
	cull_mask = p_layers;
	Renderer::reflection_probe_set_cull_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_cull_mask() const { return cull_mask; }

void ReflectionProbe::set_reflection_mask(uint32_t p_layers)
{
	reflection_mask = p_layers;
	Renderer::reflection_probe_set_reflection_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_reflection_mask() const { return reflection_mask; }

void ReflectionProbe::set_update_mode(UpdateMode p_mode)
{
	update_mode = p_mode;
	Renderer::reflection_probe_set_update_mode(
		probe, RSE::ReflectionProbeUpdateMode(p_mode));
}

ReflectionProbe::UpdateMode ReflectionProbe::get_update_mode() const { return update_mode; }

AABB ReflectionProbe::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

void ReflectionProbe::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void ReflectionProbe::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

ReflectionProbe::ReflectionProbe()
{
	probe = Renderer::reflection_probe_create();
	set_base(probe);
	set_disable_scale(true);
}

ReflectionProbe::~ReflectionProbe()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

// ==========================================================================
// Merged from scene/3d/voxel_gi.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/camera_attributes.h"
#include "servers/rendering/renderer.h"
#include "voxel_gi.h"

void VoxelGIData::allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
	const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	Renderer::voxel_gi_allocate_data(probe, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
	bounds = p_aabb;
	to_cell_xform = p_to_cell_xform;
	octree_size = p_octree_size;
}

AABB VoxelGIData::get_bounds() const { return bounds; }

Vector3 VoxelGIData::get_octree_size() const { return octree_size; }

Vector<uint8_t> VoxelGIData::get_octree_cells() const
{
	return Renderer::voxel_gi_get_octree_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_data_cells() const
{
	return Renderer::voxel_gi_get_data_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_distance_field() const
{
	return Renderer::voxel_gi_get_distance_field(probe);
}

Vector<int> VoxelGIData::get_level_counts() const
{
	return Renderer::voxel_gi_get_level_counts(probe);
}

Transform3D VoxelGIData::get_to_cell_xform() const { return to_cell_xform; }

void VoxelGIData::set_dynamic_range(float p_range)
{
	Renderer::voxel_gi_set_dynamic_range(probe, p_range);
	dynamic_range = p_range;
}

float VoxelGIData::get_dynamic_range() const { return dynamic_range; }

void VoxelGIData::set_propagation(float p_propagation)
{
	Renderer::voxel_gi_set_propagation(probe, p_propagation);
	propagation = p_propagation;
}

float VoxelGIData::get_propagation() const { return propagation; }

void VoxelGIData::set_energy(float p_energy)
{
	Renderer::voxel_gi_set_energy(probe, p_energy);
	energy = p_energy;
}

float VoxelGIData::get_energy() const { return energy; }

void VoxelGIData::set_bias(float p_bias)
{
	Renderer::voxel_gi_set_bias(probe, p_bias);
	bias = p_bias;
}

float VoxelGIData::get_bias() const { return bias; }

void VoxelGIData::set_normal_bias(float p_normal_bias)
{
	Renderer::voxel_gi_set_normal_bias(probe, p_normal_bias);
	normal_bias = p_normal_bias;
}

float VoxelGIData::get_normal_bias() const { return normal_bias; }

void VoxelGIData::set_interior(bool p_enable)
{
	Renderer::voxel_gi_set_interior(probe, p_enable);
	interior = p_enable;
}

bool VoxelGIData::is_interior() const { return interior; }

void VoxelGIData::set_use_two_bounces(bool p_enable)
{
	Renderer::voxel_gi_set_use_two_bounces(probe, p_enable);
	use_two_bounces = p_enable;
}

bool VoxelGIData::is_using_two_bounces() const { return use_two_bounces; }

RID VoxelGIData::get_rid() const { return probe; }

VoxelGIData::VoxelGIData() { probe = Renderer::voxel_gi_create(); }

VoxelGIData::~VoxelGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

Ref<VoxelGIData> VoxelGI::get_probe_data() const { return probe_data; }

VoxelGI::Subdiv VoxelGI::get_subdiv() const { return subdiv; }

Vector3 VoxelGI::get_size() const { return size; }

void VoxelGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;

	if (probe_data.is_valid()) {
		Renderer::voxel_gi_set_baked_exposure_normalization(
			probe_data->get_rid(), _get_camera_exposure_normalization());
	}
}

Ref<CameraAttributes> VoxelGI::get_camera_attributes() const { return camera_attributes; }

VoxelGI::BakeBeginFunc VoxelGI::bake_begin_function = nullptr;
VoxelGI::BakeStepFunc VoxelGI::bake_step_function = nullptr;
VoxelGI::BakeEndFunc VoxelGI::bake_end_function = nullptr;

static int voxelizer_plot_bake_base = 0;
static int voxelizer_plot_bake_total = 0;

static bool voxelizer_plot_bake_step_function(int current, int)
{
	return VoxelGI::bake_step_function(
		(voxelizer_plot_bake_base + current) * 500 / voxelizer_plot_bake_total,
		RTR("Plotting Meshes"));
}

static bool voxelizer_sdf_bake_step_function(int current, int total)
{
	return VoxelGI::bake_step_function(
		500 + current * 500 / total, RTR("Generating Distance Field"));
}

Vector3i VoxelGI::get_estimated_cell_size() const
{
	static const int subdiv_value[SUBDIV_MAX] = {6, 7, 8, 9};
	int cell_subdiv = subdiv_value[subdiv];
	int axis_cell_size[3];
	AABB bounds = AABB(-size / 2, size);
	int longest_axis = bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		float axis_size = bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}
	}

	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

void VoxelGI::_debug_bake() { bake(nullptr, true); }

float VoxelGI::_get_camera_exposure_normalization()
{
	float exposure_normalization = 1.0;
	if (camera_attributes.is_valid()) {
		exposure_normalization = camera_attributes->get_exposure_multiplier();
		if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
			exposure_normalization = camera_attributes->calculate_exposure_normalization();
		}
	}
	return exposure_normalization;
}

AABB VoxelGI::get_aabb() const { return AABB(-size / 2, size); }

PackedStringArray VoxelGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (OS::get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}
	else if (OS::get_current_rendering_method() == "dummy") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Dummy renderer."));
	}
	else if (probe_data.is_null()) {
		warnings.push_back(RTR(
			"No VoxelGI data set, so this node is disabled. Bake static objects to enable GI."));
	}
	return warnings;
}

void VoxelGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void VoxelGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

VoxelGI::VoxelGI()
{
	voxel_gi = Renderer::voxel_gi_create();
	set_disable_scale(true);
}

VoxelGI::~VoxelGI()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(voxel_gi);
}

void VoxelGI::bake(Node*, bool) {}
