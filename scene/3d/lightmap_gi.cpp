/**************************************************************************/
/*  lightmap_gi.cpp                                                       */
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
#include "servers/rendering/rendering_server.h"

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
	RS::get_singleton()->lightmap_set_textures(lightmap,
		combined_light_texture.is_valid() ? combined_light_texture->get_rid() : RID(),
		uses_spherical_harmonics);
}

void LightmapGIData::_reset_shadowmask_textures()
{
	RS::get_singleton()->lightmap_set_shadowmask_textures(lightmap,
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
	RS::get_singleton()->lightmap_set_shadowmask_mode(lightmap, (RSE::ShadowmaskMode)p_mode);
}

LightmapGIData::ShadowmaskMode LightmapGIData::get_shadowmask_mode() const
{
	return (ShadowmaskMode)RS::get_singleton()->lightmap_get_shadowmask_mode(lightmap);
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
		RS::get_singleton()->lightmap_set_probe_capture_data(
			lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
		RS::get_singleton()->lightmap_set_probe_bounds(lightmap, p_bounds);
		RS::get_singleton()->lightmap_set_probe_interior(lightmap, p_interior);
	}
	else {
		RS::get_singleton()->lightmap_set_probe_capture_data(lightmap, PackedVector3Array(),
			PackedColorArray(), PackedInt32Array(), PackedInt32Array());
		RS::get_singleton()->lightmap_set_probe_bounds(lightmap, AABB());
		RS::get_singleton()->lightmap_set_probe_interior(lightmap, false);
	}
	RS::get_singleton()->lightmap_set_baked_exposure_normalization(lightmap, p_baked_exposure);
	baked_exposure = p_baked_exposure;
	lightprobe_hash = p_lightprobe_hash;
	interior = p_interior;
	bounds = p_bounds;
}

PackedVector3Array LightmapGIData::get_capture_points() const
{
	return RS::get_singleton()->lightmap_get_probe_capture_points(lightmap);
}

PackedColorArray LightmapGIData::get_capture_sh() const
{
	return RS::get_singleton()->lightmap_get_probe_capture_sh(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_tetrahedra() const
{
	return RS::get_singleton()->lightmap_get_probe_capture_tetrahedra(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_bsp_tree() const
{
	return RS::get_singleton()->lightmap_get_probe_capture_bsp_tree(lightmap);
}

uint32_t LightmapGIData::get_lightprobe_hash() const { return lightprobe_hash; }

AABB LightmapGIData::get_capture_bounds() const { return bounds; }

bool LightmapGIData::is_interior() const { return interior; }

float LightmapGIData::get_baked_exposure() const { return baked_exposure; }

LightmapGIData::LightmapGIData() { lightmap = RS::get_singleton()->lightmap_create(); }

LightmapGIData::~LightmapGIData()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(lightmap);
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

void LightmapGI::_notification(int p_what)
{
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

void LightmapGI::set_light_data(const Ref<LightmapGIData>& p_data)
{
	if (light_data.is_valid()) {
		if (is_inside_tree()) {
			_clear_lightmaps();
		}
		set_base(RID());
	}
	light_data = p_data;

	if (light_data.is_valid()) {
		set_base(light_data->get_rid());
		if (is_inside_tree()) {
			_assign_lightmaps();
		}
		light_data->update_shadowmask_mode(shadowmask_mode);
	}

	update_gizmos();
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

void LightmapGI::set_shadowmask_mode(LightmapGIData::ShadowmaskMode p_mode)
{
	shadowmask_mode = p_mode;
	if (light_data.is_valid()) {
		light_data->update_shadowmask_mode(p_mode);
	}

	update_configuration_warnings();
}

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
	PackedStringArray warnings = VisualInstance3D::get_configuration_warnings();

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
	if (!DisplayServer::get_singleton()->can_create_rendering_device()) {
		warnings.push_back(vformat(
			RTR("Lightmaps can only be baked from a GPU that supports the RenderingDevice "
				"backends.\nYour GPU (%s) does not support RenderingDevice, as it does not support "
				"Vulkan, Direct3D 12, or Metal.\nLightmap baking will not be available on this "
				"device, although rendering existing baked lightmaps will work."),
			RenderingServer::get_singleton()->get_video_adapter_name()));
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
		OS::get_singleton()->get_name()));
#else
	warnings.push_back(RTR("Lightmaps cannot be baked, as the `lightmapper_rd` module was disabled "
						   "at compile-time. Rendering existing baked lightmaps will still work."));
#endif

	return warnings;
}

LightmapGI::LightmapGI() {}


