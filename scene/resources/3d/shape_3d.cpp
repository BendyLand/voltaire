/**************************************************************************/
/*  shape_3d.cpp                                                          */
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

#include "shape_3d.h"

#include <cfloat> // FLT_MAX

#include "core/io/image.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

RID Shape3D::_create_shape_rid(ShapeType p_type)
{
	switch (p_type) {
	case SHAPE_WORLD_BOUNDARY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_WORLD_BOUNDARY);
	case SHAPE_SEPARATION_RAY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SEPARATION_RAY);
	case SHAPE_SPHERE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SPHERE);
	case SHAPE_BOX:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_BOX);
	case SHAPE_CAPSULE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CAPSULE);
	case SHAPE_CYLINDER:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CYLINDER);
	case SHAPE_CONVEX_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONVEX_POLYGON);
	case SHAPE_CONCAVE_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONCAVE_POLYGON);
	case SHAPE_HEIGHTMAP:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_HEIGHTMAP);
	}
	return RID();
}

void Shape3D::_ensure_heightmap_initialized()
{
	if (shape_type == SHAPE_HEIGHTMAP && map_data.size() != map_width * map_depth) {
		map_data.resize(map_width * map_depth);
		real_t* w = map_data.ptrw();
		for (int i = 0; i < map_width * map_depth; i++) {
			w[i] = 0.0;
		}
	}
}

void Shape3D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	if (shape.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(shape);
	}
	shape_type = p_type;
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	if (custom_bias != 0.0) {
		PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
	}
	if (margin != 0.04) {
		PhysicsServer3D::shape_set_margin(shape, margin);
	}
	_update_shape();
}

// World boundary

void Shape3D::set_plane(const Plane& p_plane)
{
	plane = p_plane;
	_update_shape();
}

const Plane& Shape3D::get_plane() const { return plane; }

// Separation ray

void Shape3D::set_length(float p_length)
{
	length = p_length;
	_update_shape();
}

float Shape3D::get_length() const { return length; }

void Shape3D::set_slide_on_slope(bool p_active)
{
	slide_on_slope = p_active;
	_update_shape();
}

bool Shape3D::get_slide_on_slope() const { return slide_on_slope; }

// Sphere / Capsule / Cylinder

void Shape3D::set_radius(float p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0f, "Shape3D radius cannot be negative.");
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && height < radius * 2.0f) {
		height = radius * 2.0f;
	}
	_update_shape();
}

float Shape3D::get_radius() const { return radius; }

void Shape3D::set_height(float p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0f, "Shape3D height cannot be negative.");
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
	_update_shape();
}

float Shape3D::get_height() const { return height; }

void Shape3D::set_mid_height(real_t p_mid_height)
{
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "Shape3D mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
	_update_shape();
}

real_t Shape3D::get_mid_height() const { return height - radius * 2.0f; }

// Box

void Shape3D::set_size(const Vector3& p_size)
{
	ERR_FAIL_COND_MSG(
		p_size.x < 0 || p_size.y < 0 || p_size.z < 0, "Shape3D size cannot be negative.");
	size = p_size;
	_update_shape();
}

Vector3 Shape3D::get_size() const { return size; }

// Convex polygon

void Shape3D::set_points(const Vector<Vector3>& p_points)
{
	points = p_points;
	_update_shape();
}

Vector<Vector3> Shape3D::get_points() const { return points; }

// Concave polygon

void Shape3D::set_faces(const Vector<Vector3>& p_faces)
{
	faces = p_faces;
	_update_shape();
}

Vector<Vector3> Shape3D::get_faces() const { return faces; }

void Shape3D::set_backface_collision_enabled(bool p_enabled)
{
	backface_collision = p_enabled;
	if (!faces.is_empty()) {
		_update_shape();
	}
}

bool Shape3D::is_backface_collision_enabled() const { return backface_collision; }

// Heightmap

void Shape3D::set_map_width(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_width != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_width = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_width() const { return map_width; }

void Shape3D::set_map_depth(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_depth != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_depth = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_depth() const { return map_depth; }

void Shape3D::set_map_data(Vector<real_t> p_new)
{
	int expected_size = (map_width * map_depth);
	if (p_new.size() != expected_size) {
		return;
	}

	map_data.resize(expected_size);
	real_t* w = map_data.ptrw();
	const real_t* r = p_new.ptr();
	for (int i = 0; i < expected_size; i++) {
		real_t val = r[i];
		w[i] = val;
		if (i == 0) {
			min_height = val;
			max_height = val;
		}
		else {
			if (min_height > val) {
				min_height = val;
			}
			if (max_height < val) {
				max_height = val;
			}
		}
	}

	_update_shape();
}

Vector<real_t> Shape3D::get_map_data() const { return map_data; }

real_t Shape3D::get_min_height() const { return min_height; }

real_t Shape3D::get_max_height() const { return max_height; }

void Shape3D::update_map_data_from_image(
	const Ref<Image>& p_image, real_t p_height_min, real_t p_height_max)
{
	ERR_FAIL_COND_MSG(
		p_image.is_null(), "Heightmap update image requires a valid Image reference.");
	ERR_FAIL_COND_MSG(p_image->get_format() != Image::FORMAT_RF &&
						  p_image->get_format() != Image::FORMAT_RH &&
						  p_image->get_format() != Image::FORMAT_R8,
		"Heightmap update image requires Image in format FORMAT_RF (32 bit), FORMAT_RH (16 bit), "
		"or FORMAT_R8 (8 bit).");
	ERR_FAIL_COND_MSG(
		p_image->get_width() < 2, "Heightmap update image requires a minimum Image width of 2.");
	ERR_FAIL_COND_MSG(
		p_image->get_height() < 2, "Heightmap update image requires a minimum Image height of 2.");
	ERR_FAIL_COND_MSG(p_height_min > p_height_max,
		"Heightmap update image requires height_max to be greater than height_min.");

	map_width = p_image->get_width();
	map_depth = p_image->get_height();
	map_data.resize(map_width * map_depth);

	real_t new_min_height = FLT_MAX;
	real_t new_max_height = -FLT_MAX;

	float remap_height_min = float(p_height_min);
	float remap_height_max = float(p_height_max);

	real_t* map_data_ptrw = map_data.ptrw();

	switch (p_image->get_format()) {
	case Image::FORMAT_RF: {
		const float* image_data_ptr = (float*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = image_data_ptr[i];

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_RH: {
		const uint16_t* image_data_ptr = (uint16_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = Math::half_to_float(image_data_ptr[i]);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_R8: {
		const uint8_t* image_data_ptr = (uint8_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = float(image_data_ptr[i] / 255.0);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	default: {
		return;
	}
	}

	min_height = new_min_height;
	max_height = new_max_height;

	_update_shape();
}

// Debug & bounds

Vector<Vector3> Shape3D::get_debug_mesh_lines() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Plane p = plane;
		Vector3 n1 = p.get_any_perpendicular_normal();
		Vector3 n2 = p.normal.cross(n1).normalized();

		Vector3 pface[4] = {
			p.normal * p.d + n1 * 10.0 + n2 * 10.0,
			p.normal * p.d + n1 * 10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * 10.0,
		};

		Vector<Vector3> wb_points = {pface[0], pface[1], pface[1], pface[2], pface[2], pface[3],
			pface[3], pface[0], p.normal * p.d, p.normal * p.d + p.normal * 3};
		return wb_points;
	}
	case SHAPE_SEPARATION_RAY: {
		return {Vector3(), Vector3(0, 0, length)};
	}
	case SHAPE_SPHERE: {
		Vector<Vector3> sphere_points;
		for (int i = 0; i <= 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			sphere_points.push_back(Vector3(a.x, 0, a.y));
			sphere_points.push_back(Vector3(b.x, 0, b.y));
			sphere_points.push_back(Vector3(0, a.x, a.y));
			sphere_points.push_back(Vector3(0, b.x, b.y));
			sphere_points.push_back(Vector3(a.x, a.y, 0));
			sphere_points.push_back(Vector3(b.x, b.y, 0));
		}
		return sphere_points;
	}
	case SHAPE_BOX: {
		return Vector<Vector3>();
	}
	case SHAPE_CAPSULE: {
		Vector<Vector3> capsule_points;
		Vector3 d(0, height * 0.5f - radius, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) + d);

			capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
				capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 180 ? d : -d;

			capsule_points.push_back(Vector3(0, a.x, a.y) + dud);
			capsule_points.push_back(Vector3(0, b.x, b.y) + dud);
			capsule_points.push_back(Vector3(a.y, a.x, 0) + dud);
			capsule_points.push_back(Vector3(b.y, b.x, 0) + dud);
		}
		return capsule_points;
	}
	case SHAPE_CYLINDER: {
		Vector<Vector3> cylinder_points;
		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) + d);

			cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
				cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			}
		}
		return cylinder_points;
	}
	case SHAPE_CONVEX_POLYGON: {
		return Vector<Vector3>();
	}
	case SHAPE_CONCAVE_POLYGON: {
		HashSet<DrawEdge, DrawEdge> edges;

		int index_count = faces.size();
		ERR_FAIL_COND_V((index_count % 3) != 0, Vector<Vector3>());

		const Vector3* r = faces.ptr();

		for (int i = 0; i < index_count; i += 3) {
			for (int j = 0; j < 3; j++) {
				DrawEdge de(r[i + j], r[i + ((j + 1) % 3)]);
				edges.insert(de);
			}
		}

		Vector<Vector3> concave_points;
		concave_points.resize(edges.size() * 2);
		int idx = 0;
		for (const DrawEdge& E : edges) {
			concave_points.write[idx + 0] = E.a;
			concave_points.write[idx + 1] = E.b;
			idx += 2;
		}
		return concave_points;
	}
	case SHAPE_HEIGHTMAP: {
		Vector<Vector3> hm_points;
		if ((map_width != 0) && (map_depth != 0) && map_data.size() >= map_width * map_depth) {
			Vector2 hm_size(map_width - 1, map_depth - 1);
			Vector2 start = hm_size * -0.5;

			const real_t* r = map_data.ptr();

			hm_points.resize(((map_width - 1) * map_depth * 2) + (map_width * (map_depth - 1) * 2) +
							 ((map_width - 1) * (map_depth - 1) * 2));

			int r_offset = 0;
			int w_offset = 0;
			for (int d = 0; d < map_depth; d++) {
				Vector3 h_pt(start.x, 0.0, start.y);

				for (int w = 0; w < map_width; w++) {
					h_pt.y = r[r_offset++];

					if (w != map_width - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
					}

					if (d != map_depth - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					if ((w != map_width - 1) && (d != map_depth - 1)) {
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					h_pt.x += 1.0;
				}

				start.y += 1.0;
			}
		}
		return hm_points;
	}
	}
	return Vector<Vector3>();
}

Ref<ArrayMesh> Shape3D::get_debug_arraymesh_faces(const Color& p_modulate) const
{
	(void)p_modulate;
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
	case SHAPE_SEPARATION_RAY:
	case SHAPE_SPHERE:
	case SHAPE_BOX:
	case SHAPE_CAPSULE:
	case SHAPE_CYLINDER:
	case SHAPE_CONVEX_POLYGON:
	case SHAPE_CONCAVE_POLYGON:
	case SHAPE_HEIGHTMAP:
	default:
		return memnew(ArrayMesh);
	}
}

real_t Shape3D::get_enclosing_radius() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
		return 0;
	case SHAPE_SEPARATION_RAY:
		return length;
	case SHAPE_SPHERE:
		return radius;
	case SHAPE_BOX:
		return size.length() / 2;
	case SHAPE_CAPSULE:
		return height * 0.5f;
	case SHAPE_CYLINDER:
		return Vector2(radius, height * 0.5).length();
	case SHAPE_CONVEX_POLYGON: {
		const Vector3* read = points.ptr();
		real_t r = 0.0;
		for (int i = 0; i < points.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_CONCAVE_POLYGON: {
		const Vector3* read = faces.ptr();
		real_t r = 0.0;
		for (int i = 0; i < faces.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_HEIGHTMAP:
		return Vector3(real_t(map_width), max_height - min_height, real_t(map_depth)).length();
	}
	return 0;
}

void Shape3D::add_vertices_to_array(Vector<Vector3>& array, const Transform3D& p_xform)
{
	Vector<Vector3> toadd = get_debug_mesh_lines();

	if (toadd.size()) {
		int base = array.size();
		array.resize(base + toadd.size());
		Vector3* w = array.ptrw();
		for (int i = 0; i < toadd.size(); i++) {
			w[i + base] = p_xform.xform(toadd[i]);
		}
	}
}

void Shape3D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape3D::get_custom_solver_bias() const { return custom_bias; }

real_t Shape3D::get_margin() const { return margin; }

void Shape3D::set_margin(real_t p_margin)
{
	margin = p_margin;
	PhysicsServer3D::shape_set_margin(shape, margin);
}

void Shape3D::set_debug_color(const Color& p_color)
{
	if (p_color == debug_color) {
		return;
	}

	debug_color = p_color;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

Color Shape3D::get_debug_color() const { return debug_color; }

void Shape3D::set_debug_fill(bool p_fill)
{
	if (p_fill == debug_fill) {
		return;
	}

	debug_fill = p_fill;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

bool Shape3D::get_debug_fill() const { return debug_fill; }

void Shape3D::_update_shape()
{
	emit_changed();
	debug_mesh_cache.unref();
}

Shape3D::Shape3D(ShapeType p_type) : shape_type(p_type)
{
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	_update_shape();
}

Shape3D::~Shape3D()
{
	ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
	PhysicsServer3D::free_rid(shape);
}

Ref<ArrayMesh> Shape3D::get_debug_mesh()
{
	if (debug_mesh_cache.is_null()) {
		debug_mesh_cache.instantiate();
	}
	return debug_mesh_cache;
}
