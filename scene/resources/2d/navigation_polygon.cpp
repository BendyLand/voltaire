/**************************************************************************/
/*  navigation_polygon.cpp                                                */
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

#include <thirdparty/misc/polypartition.h>
#include "core/math/geometry_2d.h"
#include "core/os/mutex.h"
#include "navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"

NavigationPolygon::NavigationPolygon() :
		cell_size(NavigationDefaults2D::NAV_MESH_CELL_SIZE) {
}

#ifdef DEBUG_ENABLED
Rect2 NavigationPolygon::_edit_get_rect() const
{
	RWLockRead read_lock(rwlock);
	if (rect_cache_dirty) {
		item_rect = Rect2();
		bool first = true;

		for (int i = 0; i < outlines.size(); i++) {
			const Vector<Vector2>& outline = outlines[i];
			const int outline_size = outline.size();
			if (outline_size < 3) {
				continue;
			}
			const Vector2* p = outline.ptr();
			for (int j = 0; j < outline_size; j++) {
				if (first) {
					item_rect = Rect2(p[j], Vector2(0, 0));
					first = false;
				}
				else {
					item_rect.expand_to(p[j]);
				}
			}
		}

		rect_cache_dirty = false;
	}
	return item_rect;
}

#endif // DEBUG_ENABLED

void NavigationPolygon::set_vertices(const Vector<Vector2>& p_vertices)
{
	RWLockWrite write_lock(rwlock);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	vertices = p_vertices;
	rect_cache_dirty = true;
}

const Vector<Vector2>& NavigationPolygon::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

void NavigationPolygon::add_polygon(const Vector<int>& p_polygon)
{
	RWLockWrite write_lock(rwlock);
	polygons.push_back(p_polygon);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::add_outline_at_index(const Vector<Vector2>& p_outline, int p_index)
{
	RWLockWrite write_lock(rwlock);
	outlines.insert(p_index, p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationPolygon::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationPolygon::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(
	const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(const Vector<Vector2>& p_vertices,
	const Vector<Vector<int>>& p_polygons, const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	outlines = p_outlines;
	rect_cache_dirty = true;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons,
	Vector<Vector<Vector2>>& r_outlines)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
	r_outlines = outlines;
}

Ref<NavigationMesh> NavigationPolygon::get_navigation_mesh()
{
	MutexLock lock(navigation_mesh_generation);

	if (navigation_mesh.is_null()) {
		navigation_mesh.instantiate();
		Vector<Vector3> verts;
		Vector<Vector<int>> polys;
		{
			verts.resize(get_vertices().size());
			Vector3* w = verts.ptrw();

			const Vector2* r = get_vertices().ptr();

			for (int i(0); i < get_vertices().size(); i++) {
				w[i] = Vector3(r[i].x, 0.0, r[i].y);
			}
		}

		for (int i(0); i < get_polygon_count(); i++) {
			polys.push_back(get_polygon(i));
		}

		navigation_mesh->set_data(verts, polys);
		navigation_mesh->set_cell_size(
			cell_size); // Needed to not fail the cell size check on the server
	}

	return navigation_mesh;
}

void NavigationPolygon::set_outlines(const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	outlines = p_outlines;
	rect_cache_dirty = true;
}

Vector<Vector<Vector2>> NavigationPolygon::get_outlines() const
{
	RWLockRead read_lock(rwlock);
	return outlines;
}

void NavigationPolygon::set_polygons(const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

Vector<Vector<int>> NavigationPolygon::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

void NavigationPolygon::add_outline(const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	outlines.push_back(p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_outline_count() const
{
	RWLockRead read_lock(rwlock);
	return outlines.size();
}

void NavigationPolygon::set_outline(int p_idx, const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.write[p_idx] = p_outline;
	rect_cache_dirty = true;
}

void NavigationPolygon::remove_outline(int p_idx)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.remove_at(p_idx);
	rect_cache_dirty = true;
}

Vector<Vector2> NavigationPolygon::get_outline(int p_idx) const
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, outlines.size(), Vector<Vector2>());
	return outlines[p_idx];
}

void NavigationPolygon::clear_outlines()
{
	RWLockWrite write_lock(rwlock);
	outlines.clear();
	rect_cache_dirty = true;
}

#ifndef DISABLE_DEPRECATED
void NavigationPolygon::make_polygons_from_outlines()
{
	RWLockWrite write_lock(rwlock);
	WARN_PRINT("Function make_polygons_from_outlines() is deprecated."
			   "\nUse NavigationServer2D.parse_source_geometry_data() and "
			   "NavigationServer2D.bake_from_source_geometry_data() instead.");

	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	List<TPPLPoly> in_poly, out_poly;

	Vector2 outside_point(-1e10, -1e10);

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();
		for (int j = 0; j < olsize; j++) {
			outside_point = outside_point.max(r[j]);
		}
	}

	outside_point += Vector2(0.7239784, 0.819238); // avoid precision issues

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();

		int interscount = 0;
		// test if this is an outer outline
		for (int k = 0; k < outlines.size(); k++) {
			if (i == k) {
				continue; // no self intersect
			}

			Vector<Vector2> ol2 = outlines[k];
			int olsize2 = ol2.size();
			if (olsize2 < 3) {
				continue;
			}
			const Vector2* r2 = ol2.ptr();

			for (int l = 0; l < olsize2; l++) {
				if (Geometry2D::segment_intersects_segment(
						r[0], outside_point, r2[l], r2[(l + 1) % olsize2], nullptr)) {
					interscount++;
				}
			}
		}

		bool outer = (interscount % 2) == 0;

		TPPLPoly tp;
		tp.Init(olsize);
		for (int j = 0; j < olsize; j++) {
			tp[j] = r[j];
		}

		if (outer) {
			tp.SetOrientation(TPPL_ORIENTATION_CCW);
		}
		else {
			tp.SetOrientation(TPPL_ORIENTATION_CW);
			tp.SetHole(true);
		}

		in_poly.push_back(tp);
	}

	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // failed!
		ERR_PRINT("NavigationPolygon: Convex partition failed! Failed to convert outlines to a "
				  "valid NavigationPolygon."
				  "\nNavigationPolygon outlines can not overlap vertices or edges inside same "
				  "outline or with other outlines or have any intersections."
				  "\nAdd the outmost and largest outline first. To add holes inside this outline "
				  "add the smaller outlines with same winding order.");
		return;
	}

	polygons.clear();
	vertices.clear();

	HashMap<Vector2, int> points;
	for (const TPPLPoly& tp : out_poly) {
		Vector<int> p;

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			HashMap<Vector2, int>::Iterator E = points.find(tp[i]);
			if (!E) {
				E = points.insert(tp[i], vertices.size());
				vertices.push_back(tp[i]);
			}
			p.push_back(E->value);
		}

		polygons.push_back(p);
	}

	emit_changed();
}
#endif // DISABLE_DEPRECATED

void NavigationPolygon::set_cell_size(real_t p_cell_size)
{
	cell_size = p_cell_size;
	get_navigation_mesh()->set_cell_size(cell_size);
}

real_t NavigationPolygon::get_cell_size() const { return cell_size; }

void NavigationPolygon::set_border_size(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0.0);
	border_size = p_value;
}

real_t NavigationPolygon::get_border_size() const { return border_size; }

void NavigationPolygon::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationPolygon::SamplePartitionType NavigationPolygon::get_sample_partition_type() const
{
	return partition_type;
}

NavigationPolygon::ParsedGeometryType NavigationPolygon::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationPolygon::set_parsed_collision_mask(uint32_t p_mask)
{
	parsed_collision_mask = p_mask;
}

uint32_t NavigationPolygon::get_parsed_collision_mask() const { return parsed_collision_mask; }

void NavigationPolygon::set_parsed_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_parsed_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_parsed_collision_mask(mask);
}

bool NavigationPolygon::get_parsed_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_parsed_collision_mask() & (1 << (p_layer_number - 1));
}

NavigationPolygon::SourceGeometryMode NavigationPolygon::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationPolygon::set_source_geometry_group_name(const String& p_group_name)
{
	source_geometry_group_name = p_group_name;
}

String NavigationPolygon::get_source_geometry_group_name() const
{
	return source_geometry_group_name;
}

void NavigationPolygon::set_agent_radius(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

real_t NavigationPolygon::get_agent_radius() const { return agent_radius; }

void NavigationPolygon::set_baking_rect(const Rect2& p_rect)
{
	baking_rect = p_rect;
	emit_changed();
}

Rect2 NavigationPolygon::get_baking_rect() const { return baking_rect; }

void NavigationPolygon::set_baking_rect_offset(const Vector2& p_rect_offset)
{
	baking_rect_offset = p_rect_offset;
	emit_changed();
}

Vector2 NavigationPolygon::get_baking_rect_offset() const { return baking_rect_offset; }



bool NavigationPolygon::_edit_is_selected_on_click(Vector2 const&, double) const { return false; }

// --- navigation_mesh_source_geometry_data_2d.cpp ---

#include "navigation_mesh_source_geometry_data_2d.h"

void NavigationMeshSourceGeometryData2D::clear()
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.clear();
	obstruction_outlines.clear();
	_projected_obstructions.clear();
	bounds_dirty = true;
}

bool NavigationMeshSourceGeometryData2D::has_data()
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines.size();
}

void NavigationMeshSourceGeometryData2D::clear_projected_obstructions()
{
	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_traversable_outlines(
	const Vector<Vector<Vector2>>& p_traversable_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_obstruction_outlines(
	const Vector<Vector<Vector2>>& p_obstruction_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	obstruction_outlines = p_obstruction_outlines;
	bounds_dirty = true;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_traversable_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_obstruction_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return obstruction_outlines;
}

void NavigationMeshSourceGeometryData2D::_add_traversable_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		traversable_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::_add_obstruction_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		obstruction_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_traversable_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> traversable_outline;
		traversable_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			traversable_outline.write[i] = p_shape_outline[i];
		}
		traversable_outlines.push_back(traversable_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_obstruction_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> obstruction_outline;
		obstruction_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			obstruction_outline.write[i] = p_shape_outline[i];
		}
		obstruction_outlines.push_back(obstruction_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::merge(
	const Ref<NavigationMeshSourceGeometryData2D>& p_other_geometry)
{
	ERR_FAIL_COND(p_other_geometry.is_null());

	Vector<Vector<Vector2>> other_traversable_outlines;
	Vector<Vector<Vector2>> other_obstruction_outlines;
	Vector<ProjectedObstruction> other_projected_obstructions;

	p_other_geometry->get_data(
		other_traversable_outlines, other_obstruction_outlines, other_projected_obstructions);

	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.append_array(other_traversable_outlines);
	obstruction_outlines.append_array(other_obstruction_outlines);
	_projected_obstructions.append_array(other_projected_obstructions);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::add_projected_obstruction(
	const Vector<Vector2>& p_vertices, bool p_carve)
{
	ERR_FAIL_COND(p_vertices.size() < 2);

	ProjectedObstruction projected_obstruction;
	projected_obstruction.vertices.resize(p_vertices.size() * 2);
	projected_obstruction.carve = p_carve;

	float* obstruction_vertices_ptrw = projected_obstruction.vertices.ptrw();

	int vertex_index = 0;
	for (const Vector2& vertex : p_vertices) {
		obstruction_vertices_ptrw[vertex_index++] = vertex.x;
		obstruction_vertices_ptrw[vertex_index++] = vertex.y;
	}

	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.push_back(projected_obstruction);
	bounds_dirty = true;
}

Vector<NavigationMeshSourceGeometryData2D::ProjectedObstruction>
NavigationMeshSourceGeometryData2D::_get_projected_obstructions() const
{
	RWLockRead read_lock(geometry_rwlock);
	return _projected_obstructions;
}

void NavigationMeshSourceGeometryData2D::set_data(
	const Vector<Vector<Vector2>>& p_traversable_outlines,
	const Vector<Vector<Vector2>>& p_obstruction_outlines,
	Vector<ProjectedObstruction>& p_projected_obstructions)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	obstruction_outlines = p_obstruction_outlines;
	_projected_obstructions = p_projected_obstructions;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::get_data(Vector<Vector<Vector2>>& r_traversable_outlines,
	Vector<Vector<Vector2>>& r_obstruction_outlines,
	Vector<ProjectedObstruction>& r_projected_obstructions)
{
	RWLockRead read_lock(geometry_rwlock);
	r_traversable_outlines = traversable_outlines;
	r_obstruction_outlines = obstruction_outlines;
	r_projected_obstructions = _projected_obstructions;
}

Rect2 NavigationMeshSourceGeometryData2D::get_bounds()
{
	geometry_rwlock.read_lock();

	if (bounds_dirty) {
		geometry_rwlock.read_unlock();
		RWLockWrite write_lock(geometry_rwlock);

		bounds_dirty = false;
		bounds = Rect2();
		bool first_vertex = true;

		for (const Vector<Vector2>& traversable_outline : traversable_outlines) {
			for (const Vector2& traversable_point : traversable_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = traversable_point;
				}
				else {
					bounds.expand_to(traversable_point);
				}
			}
		}

		for (const Vector<Vector2>& obstruction_outline : obstruction_outlines) {
			for (const Vector2& obstruction_point : obstruction_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = obstruction_point;
				}
				else {
					bounds.expand_to(obstruction_point);
				}
			}
		}

		for (const ProjectedObstruction& projected_obstruction : _projected_obstructions) {
			for (int i = 0; i < projected_obstruction.vertices.size() / 2; i++) {
				const Vector2 vertex = Vector2(projected_obstruction.vertices[i * 2],
					projected_obstruction.vertices[i * 2 + 1]);
				if (first_vertex) {
					first_vertex = false;
					bounds.position = vertex;
				}
				else {
					bounds.expand_to(vertex);
				}
			}
		}
	}
	else {
		geometry_rwlock.read_unlock();
	}

	RWLockRead read_lock(geometry_rwlock);
	return bounds;
}

// --- polygon_path_finder.cpp ---

#include "core/math/geometry_2d.h"
#include "polygon_path_finder.h"

bool PolygonPathFinder::_is_point_inside(const Vector2& p_point) const
{
	int crosses = 0;

	for (const Edge& E : edges) {
		const Edge& e = E;

		Vector2 a = points[e.points[0]].pos;
		Vector2 b = points[e.points[1]].pos;

		if (Geometry2D::segment_intersects_segment(a, b, p_point, outside_point, nullptr)) {
			crosses++;
		}
	}

	return crosses & 1;
}

void PolygonPathFinder::setup(const Vector<Vector2>& p_points, const Vector<int>& p_connections)
{
	ERR_FAIL_COND(p_connections.size() & 1);

	points.clear();
	edges.clear();

	// insert points

	int point_count = p_points.size();
	points.resize(point_count + 2);
	bounds = Rect2();

	for (int i = 0; i < p_points.size(); i++) {
		points.write[i].pos = p_points[i];
		points.write[i].penalty = 0;

		outside_point = i == 0 ? p_points[0] : p_points[i].max(outside_point);

		if (i == 0) {
			bounds.position = points[i].pos;
		}
		else {
			bounds.expand_to(points[i].pos);
		}
	}

	outside_point.x += 20.451 + Math::randf() * 10.2039;
	outside_point.y += 21.193 + Math::randf() * 12.5412;

	// insert edges (which are also connections)

	for (int i = 0; i < p_connections.size(); i += 2) {
		Edge e(p_connections[i], p_connections[i + 1]);
		ERR_FAIL_INDEX(e.points[0], point_count);
		ERR_FAIL_INDEX(e.points[1], point_count);
		points.write[p_connections[i]].connections.insert(p_connections[i + 1]);
		points.write[p_connections[i + 1]].connections.insert(p_connections[i]);
		edges.insert(e);
	}

	// fill the remaining connections based on visibility

	for (int i = 0; i < point_count; i++) {
		for (int j = i + 1; j < point_count; j++) {
			if (edges.has(Edge(i, j))) {
				continue; // if in edge ignore
			}

			Vector2 from = points[i].pos;
			Vector2 to = points[j].pos;

			if (!_is_point_inside(
					from * 0.5 + to * 0.5)) { // connection between points in inside space
				continue;
			}

			bool valid = true;

			for (const Edge& E : edges) {
				const Edge& e = E;
				if (e.points[0] == i || e.points[1] == i || e.points[0] == j || e.points[1] == j) {
					continue;
				}

				Vector2 a = points[e.points[0]].pos;
				Vector2 b = points[e.points[1]].pos;

				if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				points.write[i].connections.insert(j);
				points.write[j].connections.insert(i);
			}
		}
	}
}

Vector<Vector2> PolygonPathFinder::find_path(const Vector2& p_from, const Vector2& p_to)
{
	Vector<Vector2> path;

	Vector2 from = p_from;
	Vector2 to = p_to;
	Edge ignore_from_edge(-1, -1);
	Edge ignore_to_edge(-1, -1);

	if (!_is_point_inside(from)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(from, segment_a, segment_b);
			float d = from.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_from_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		from = closest_point;
	};

	if (!_is_point_inside(to)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(to, segment_a, segment_b);
			float d = to.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_to_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		to = closest_point;
	};

	// test direct connection
	{
		bool can_see_eachother = true;

		for (const Edge& E : edges) {
			const Edge& e = E;
			if (e.points[0] == ignore_from_edge.points[0] &&
				e.points[1] == ignore_from_edge.points[1]) {
				continue;
			}
			if (e.points[0] == ignore_to_edge.points[0] &&
				e.points[1] == ignore_to_edge.points[1]) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
				can_see_eachother = false;
				break;
			}
		}

		if (can_see_eachother) {
			path.push_back(from);
			path.push_back(to);
			return path;
		}
	}

	// add to graph

	int aidx = points.size() - 2;
	int bidx = points.size() - 1;
	points.write[aidx].pos = from;
	points.write[bidx].pos = to;
	points.write[aidx].distance = 0;
	points.write[bidx].distance = 0;
	points.write[aidx].prev = -1;
	points.write[bidx].prev = -1;
	points.write[aidx].penalty = 0;
	points.write[bidx].penalty = 0;

	for (int i = 0; i < points.size() - 2; i++) {
		bool valid_a = true;
		bool valid_b = true;
		points.write[i].prev = -1;
		points.write[i].distance = 0;

		if (!_is_point_inside(from * 0.5 + points[i].pos * 0.5)) {
			valid_a = false;
		}

		if (!_is_point_inside(to * 0.5 + points[i].pos * 0.5)) {
			valid_b = false;
		}

		for (const Edge& E : edges) {
			const Edge& e = E;

			if (e.points[0] == i || e.points[1] == i) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (valid_a) {
				if (e.points[0] != ignore_from_edge.points[1] &&
					e.points[1] != ignore_from_edge.points[1] &&
					e.points[0] != ignore_from_edge.points[0] &&
					e.points[1] != ignore_from_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(
							a, b, from, points[i].pos, nullptr)) {
						valid_a = false;
					}
				}
			}

			if (valid_b) {
				if (e.points[0] != ignore_to_edge.points[1] &&
					e.points[1] != ignore_to_edge.points[1] &&
					e.points[0] != ignore_to_edge.points[0] &&
					e.points[1] != ignore_to_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(a, b, to, points[i].pos, nullptr)) {
						valid_b = false;
					}
				}
			}

			if (!valid_a && !valid_b) {
				break;
			}
		}

		if (valid_a) {
			points.write[i].connections.insert(aidx);
			points.write[aidx].connections.insert(i);
		}

		if (valid_b) {
			points.write[i].connections.insert(bidx);
			points.write[bidx].connections.insert(i);
		}
	}
	// solve graph

	HashSet<int> open_list;

	points.write[aidx].distance = 0;
	points.write[aidx].prev = aidx;
	for (const int& E : points[aidx].connections) {
		open_list.insert(E);
		points.write[E].distance = from.distance_to(points[E].pos);
		points.write[E].prev = aidx;
	}

	bool found_route = false;

	while (true) {
		if (open_list.is_empty()) {
			print_verbose("Open list empty.");
			break;
		}
		// check open list

		int least_cost_point = -1;
		float least_cost = 1e30;

		// this could be faster (cache previous results)
		for (const int& E : open_list) {
			const Point& p = points[E];
			float cost = p.distance;
			cost += p.pos.distance_to(to);
			cost += p.penalty;

			if (cost < least_cost) {
				least_cost_point = E;
				least_cost = cost;
			}
		}

		const Point& np = points[least_cost_point];
		// open the neighbors for search

		for (const int& E : np.connections) {
			Point& p = points.write[E];
			float distance = np.pos.distance_to(p.pos) + np.distance;

			if (p.prev != -1) {
				// oh this was visited already, can we win the cost?

				if (p.distance > distance) {
					p.prev = least_cost_point; // reassign previous
					p.distance = distance;
				}
			}
			else {
				// add to open neighbors

				p.prev = least_cost_point;
				p.distance = distance;
				open_list.insert(E);

				if (E == bidx) {
					// oh my reached end! stop algorithm
					found_route = true;
					break;
				}
			}
		}

		if (found_route) {
			break;
		}

		open_list.erase(least_cost_point);
	}

	if (found_route) {
		int at = bidx;
		path.push_back(points[at].pos);
		do {
			at = points[at].prev;
			path.push_back(points[at].pos);
		} while (at != aidx);

		path.reverse();
	}

	for (int i = 0; i < points.size() - 2; i++) {
		points.write[i].connections.erase(aidx);
		points.write[i].connections.erase(bidx);
		points.write[i].prev = -1;
		points.write[i].distance = 0;
	}

	points.write[aidx].connections.clear();
	points.write[aidx].prev = -1;
	points.write[aidx].distance = 0;
	points.write[bidx].connections.clear();
	points.write[bidx].prev = -1;
	points.write[bidx].distance = 0;

	return path;
}

bool PolygonPathFinder::is_point_inside(const Vector2& p_point) const
{
	return _is_point_inside(p_point);
}

Vector2 PolygonPathFinder::get_closest_point(const Vector2& p_point) const
{
	float closest_dist = 1e20f;
	Vector2 closest_point;

	for (const Edge& E : edges) {
		const Edge& e = E;
		const Vector2 segment_a = points[e.points[0]].pos;
		const Vector2 segment_b = points[e.points[1]].pos;
		Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
		float d = p_point.distance_squared_to(closest);

		if (d < closest_dist) {
			closest_dist = d;
			closest_point = closest;
		}
	}

	ERR_FAIL_COND_V(Math::is_equal_approx(closest_dist, 1e20f), Vector2());

	return closest_point;
}

Vector<Vector2> PolygonPathFinder::get_intersections(
	const Vector2& p_from, const Vector2& p_to) const
{
	Vector<Vector2> inters;

	for (const Edge& E : edges) {
		Vector2 a = points[E.points[0]].pos;
		Vector2 b = points[E.points[1]].pos;

		Vector2 res;
		if (Geometry2D::segment_intersects_segment(a, b, p_from, p_to, &res)) {
			inters.push_back(res);
		}
	}

	return inters;
}

Rect2 PolygonPathFinder::get_bounds() const { return bounds; }

void PolygonPathFinder::set_point_penalty(int p_point, float p_penalty)
{
	ERR_FAIL_INDEX(p_point, points.size() - 2);
	points.write[p_point].penalty = p_penalty;
}

float PolygonPathFinder::get_point_penalty(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, points.size() - 2, 0);
	return points[p_point].penalty;
}

PolygonPathFinder::PolygonPathFinder() {}
