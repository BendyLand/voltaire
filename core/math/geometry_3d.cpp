/**************************************************************************/
/*  geometry_3d.cpp                                                       */
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

#include "geometry_3d.h"

#include "core/templates/hash_map.h"

void Geometry3D::get_closest_points_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1, Vector3 &r_ps, Vector3 &r_qt) {
	// Based on David Eberly's Computation of Distance Between Line Segments algorithm.

	Vector3 p = p_p1 - p_p0;
	Vector3 q = p_q1 - p_q0;
	Vector3 r = p_p0 - p_q0;

	real_t a = p.dot(p);
	real_t b = p.dot(q);
	real_t c = q.dot(q);
	real_t d = p.dot(r);
	real_t e = q.dot(r);

	real_t s = 0.0f;
	real_t t = 0.0f;

	real_t det = a * c - b * b;
	if (det > CMP_EPSILON) {
		// Non-parallel segments
		real_t bte = b * e;
		real_t ctd = c * d;

		if (bte <= ctd) {
			// s <= 0.0f
			if (e <= 0.0f) {
				// t <= 0.0f
				s = (-d >= a ? 1 : (-d > 0.0f ? -d / a : 0.0f));
				t = 0.0f;
			} else if (e < c) {
				// 0.0f < t < 1
				s = 0.0f;
				t = e / c;
			} else {
				// t >= 1
				s = (b - d >= a ? 1 : (b - d > 0.0f ? (b - d) / a : 0.0f));
				t = 1;
			}
		} else {
			// s > 0.0f
			s = bte - ctd;
			if (s >= det) {
				// s >= 1
				if (b + e <= 0.0f) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d < a ? -d / a : 1));
					t = 0.0f;
				} else if (b + e < c) {
					// 0.0f < t < 1
					s = 1;
					t = (b + e) / c;
				} else {
					// t >= 1
					s = (b - d <= 0.0f ? 0.0f : (b - d < a ? (b - d) / a : 1));
					t = 1;
				}
			} else {
				// 0.0f < s < 1
				real_t ate = a * e;
				real_t btd = b * d;

				if (ate <= btd) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
					t = 0.0f;
				} else {
					// t > 0.0f
					t = ate - btd;
					if (t >= det) {
						// t >= 1
						s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
						t = 1;
					} else {
						// 0.0f < t < 1
						s /= det;
						t /= det;
					}
				}
			}
		}
	} else {
		// Parallel segments
		if (e <= 0.0f) {
			s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
			t = 0.0f;
		} else if (e >= c) {
			s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
			t = 1;
		} else {
			s = 0.0f;
			t = e / c;
		}
	}

	r_ps = (1 - s) * p_p0 + s * p_p1;
	r_qt = (1 - t) * p_q0 + t * p_q1;
}

real_t Geometry3D::get_closest_distance_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1) {
	Vector3 ps;
	Vector3 qt;
	get_closest_points_between_segments(p_p0, p_p1, p_q0, p_q1, ps, qt);
	Vector3 st = qt - ps;
	return st.length();
}

void Geometry3D::MeshData::optimize_vertices() {
	HashMap<int, int> vtx_remap;

	for (MeshData::Face &face : faces) {
		for (int &index : face.indices) {
			if (!vtx_remap.has(index)) {
				int ni = vtx_remap.size();
				vtx_remap[index] = ni;
			}
			index = vtx_remap[index];
		}
	}

	for (MeshData::Edge &edge : edges) {
		int a = edge.vertex_a;
		int b = edge.vertex_b;

		if (!vtx_remap.has(a)) {
			int ni = vtx_remap.size();
			vtx_remap[a] = ni;
		}
		if (!vtx_remap.has(b)) {
			int ni = vtx_remap.size();
			vtx_remap[b] = ni;
		}

		edge.vertex_a = vtx_remap[a];
		edge.vertex_b = vtx_remap[b];
	}

	LocalVector<Vector3> new_vertices;
	new_vertices.resize(vtx_remap.size());

	for (uint32_t i = 0; i < vertices.size(); i++) {
		if (vtx_remap.has(i)) {
			new_vertices[vtx_remap[i]] = vertices[i];
		}
	}
	vertices = new_vertices;
}

struct _FaceClassify {
	struct _Link {
		int face = -1;
		int edge = -1;
		void clear() {
			face = -1;
			edge = -1;
		}
	};
	bool valid = false;
	int group = -1;
	_Link links[3];
	Face3 face;
};

/*** GEOMETRY WRAPPER ***/

enum _CellFlags {
	_CELL_SOLID = 1,
	_CELL_EXTERIOR = 2,
	_CELL_STEP_MASK = 0x1C,
	_CELL_STEP_NONE = 0 << 2,
	_CELL_STEP_Y_POS = 1 << 2,
	_CELL_STEP_Y_NEG = 2 << 2,
	_CELL_STEP_X_POS = 3 << 2,
	_CELL_STEP_X_NEG = 4 << 2,
	_CELL_STEP_Z_POS = 5 << 2,
	_CELL_STEP_Z_NEG = 6 << 2,
	_CELL_STEP_DONE = 7 << 2,
	_CELL_PREV_MASK = 0xE0,
	_CELL_PREV_NONE = 0 << 5,
	_CELL_PREV_Y_POS = 1 << 5,
	_CELL_PREV_Y_NEG = 2 << 5,
	_CELL_PREV_X_POS = 3 << 5,
	_CELL_PREV_X_NEG = 4 << 5,
	_CELL_PREV_Z_POS = 5 << 5,
	_CELL_PREV_Z_NEG = 6 << 5,
	_CELL_PREV_FIRST = 7 << 5,
};

static inline void _plot_face(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z, const Vector3 &p_voxelsize, const Face3 &p_face) {
	AABB aabb(Vector3(p_x, p_y, p_z), Vector3(p_len_x, p_len_y, p_len_z));
	aabb.position = aabb.position * p_voxelsize;
	aabb.size = aabb.size * p_voxelsize;

	if (!p_face.intersects_aabb(aabb)) {
		return;
	}

	if (p_len_x == 1 && p_len_y == 1 && p_len_z == 1) {
		p_cell_status[p_x][p_y][p_z] = _CELL_SOLID;
		return;
	}

	int div_x = p_len_x > 1 ? 2 : 1;
	int div_y = p_len_y > 1 ? 2 : 1;
	int div_z = p_len_z > 1 ? 2 : 1;

#define SPLIT_DIV(m_i, m_div, m_v, m_len_v, m_new_v, m_new_len_v) \
	if (m_div == 1) { \
		m_new_v = m_v; \
		m_new_len_v = 1; \
	} else if (m_i == 0) { \
		m_new_v = m_v; \
		m_new_len_v = m_len_v / 2; \
	} else { \
		m_new_v = m_v + m_len_v / 2; \
		m_new_len_v = m_len_v - m_len_v / 2; \
	}

	int new_x;
	int new_len_x;
	int new_y;
	int new_len_y;
	int new_z;
	int new_len_z;

	for (int i = 0; i < div_x; i++) {
		SPLIT_DIV(i, div_x, p_x, p_len_x, new_x, new_len_x);

		for (int j = 0; j < div_y; j++) {
			SPLIT_DIV(j, div_y, p_y, p_len_y, new_y, new_len_y);

			for (int k = 0; k < div_z; k++) {
				SPLIT_DIV(k, div_z, p_z, p_len_z, new_z, new_len_z);

				_plot_face(p_cell_status, new_x, new_y, new_z, new_len_x, new_len_y, new_len_z, p_voxelsize, p_face);
			}
		}
	}

#undef SPLIT_DIV
}

static inline void _mark_outside(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z) {
	if (p_cell_status[p_x][p_y][p_z] & 3) {
		return; // Nothing to do, already used and/or visited.
	}

	p_cell_status[p_x][p_y][p_z] = _CELL_PREV_FIRST;

	while (true) {
		uint8_t &c = p_cell_status[p_x][p_y][p_z];

		if ((c & _CELL_STEP_MASK) == _CELL_STEP_NONE) {
			// Haven't been in here, mark as outside.
			p_cell_status[p_x][p_y][p_z] |= _CELL_EXTERIOR;
		}

		if ((c & _CELL_STEP_MASK) != _CELL_STEP_DONE) {
			// If not done, increase step.
			c += 1 << 2;
		}

		if ((c & _CELL_STEP_MASK) == _CELL_STEP_DONE) {
			// Go back.
			switch (c & _CELL_PREV_MASK) {
				case _CELL_PREV_FIRST: {
					return;
				} break;
				case _CELL_PREV_Y_POS: {
					p_y++;
					ERR_FAIL_COND(p_y >= p_len_y);
				} break;
				case _CELL_PREV_Y_NEG: {
					p_y--;
					ERR_FAIL_COND(p_y < 0);
				} break;
				case _CELL_PREV_X_POS: {
					p_x++;
					ERR_FAIL_COND(p_x >= p_len_x);
				} break;
				case _CELL_PREV_X_NEG: {
					p_x--;
					ERR_FAIL_COND(p_x < 0);
				} break;
				case _CELL_PREV_Z_POS: {
					p_z++;
					ERR_FAIL_COND(p_z >= p_len_z);
				} break;
				case _CELL_PREV_Z_NEG: {
					p_z--;
					ERR_FAIL_COND(p_z < 0);
				} break;
				default: {
					ERR_FAIL();
				}
			}
			continue;
		}

		int next_x = p_x, next_y = p_y, next_z = p_z;
		uint8_t prev = 0;

		switch (c & _CELL_STEP_MASK) {
			case _CELL_STEP_Y_POS: {
				next_y++;
				prev = _CELL_PREV_Y_NEG;
			} break;
			case _CELL_STEP_Y_NEG: {
				next_y--;
				prev = _CELL_PREV_Y_POS;
			} break;
			case _CELL_STEP_X_POS: {
				next_x++;
				prev = _CELL_PREV_X_NEG;
			} break;
			case _CELL_STEP_X_NEG: {
				next_x--;
				prev = _CELL_PREV_X_POS;
			} break;
			case _CELL_STEP_Z_POS: {
				next_z++;
				prev = _CELL_PREV_Z_NEG;
			} break;
			case _CELL_STEP_Z_NEG: {
				next_z--;
				prev = _CELL_PREV_Z_POS;
			} break;
			default:
				ERR_FAIL();
		}

		if (next_x < 0 || next_x >= p_len_x) {
			continue;
		}
		if (next_y < 0 || next_y >= p_len_y) {
			continue;
		}
		if (next_z < 0 || next_z >= p_len_z) {
			continue;
		}

		if (p_cell_status[next_x][next_y][next_z] & 3) {
			continue;
		}

		p_x = next_x;
		p_y = next_y;
		p_z = next_z;
		p_cell_status[p_x][p_y][p_z] |= prev;
	}
}

static inline void _build_faces(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z, Vector<Face3> &r_faces) {
	ERR_FAIL_INDEX(p_x, p_len_x);
	ERR_FAIL_INDEX(p_y, p_len_y);
	ERR_FAIL_INDEX(p_z, p_len_z);

	if (p_cell_status[p_x][p_y][p_z] & _CELL_EXTERIOR) {
		return;
	}

#define vert(m_idx) Vector3(((m_idx) & 4) >> 2, ((m_idx) & 2) >> 1, (m_idx) & 1)

	static const uint8_t indices[6][4] = {
		{ 7, 6, 4, 5 },
		{ 7, 3, 2, 6 },
		{ 7, 5, 1, 3 },
		{ 0, 2, 3, 1 },
		{ 0, 1, 5, 4 },
		{ 0, 4, 6, 2 },

	};

	for (int i = 0; i < 6; i++) {
		Vector3 face_points[4];
		int disp_x = p_x + ((i % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);
		int disp_y = p_y + (((i - 1) % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);
		int disp_z = p_z + (((i - 2) % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);

		bool plot = false;

		if (disp_x < 0 || disp_x >= p_len_x) {
			plot = true;
		}
		if (disp_y < 0 || disp_y >= p_len_y) {
			plot = true;
		}
		if (disp_z < 0 || disp_z >= p_len_z) {
			plot = true;
		}

		if (!plot && (p_cell_status[disp_x][disp_y][disp_z] & _CELL_EXTERIOR)) {
			plot = true;
		}

		if (!plot) {
			continue;
		}

		for (int j = 0; j < 4; j++) {
			face_points[j] = vert(indices[i][j]) + Vector3(p_x, p_y, p_z);
		}

		r_faces.push_back(
				Face3(
						face_points[0],
						face_points[1],
						face_points[2]));

		r_faces.push_back(
				Face3(
						face_points[2],
						face_points[3],
						face_points[0]));
	}
}

Vector<Face3> Geometry3D::wrap_geometry(const Vector<Face3> &p_array, real_t *r_error) {
	int face_count = p_array.size();
	const Face3 *faces = p_array.ptr();
	constexpr double min_size = 1.0;
	constexpr int max_length = 20;

	AABB global_aabb;

	for (int i = 0; i < face_count; i++) {
		if (i == 0) {
			global_aabb = faces[i].get_aabb();
		} else {
			global_aabb.merge_with(faces[i].get_aabb());
		}
	}

	global_aabb.grow_by(0.01f); // Avoid numerical error.

	// Determine amount of cells in grid axis.
	int div_x, div_y, div_z;

	if (global_aabb.size.x / min_size < max_length) {
		div_x = (int)(global_aabb.size.x / min_size) + 1;
	} else {
		div_x = max_length;
	}

	if (global_aabb.size.y / min_size < max_length) {
		div_y = (int)(global_aabb.size.y / min_size) + 1;
	} else {
		div_y = max_length;
	}

	if (global_aabb.size.z / min_size < max_length) {
		div_z = (int)(global_aabb.size.z / min_size) + 1;
	} else {
		div_z = max_length;
	}

	Vector3 voxelsize = global_aabb.size;
	voxelsize.x /= div_x;
	voxelsize.y /= div_y;
	voxelsize.z /= div_z;

	// Create and initialize cells to zero.

	uint8_t ***cell_status = memnew_arr(uint8_t **, div_x);
	for (int i = 0; i < div_x; i++) {
		cell_status[i] = memnew_arr(uint8_t *, div_y);

		for (int j = 0; j < div_y; j++) {
			cell_status[i][j] = memnew_arr(uint8_t, div_z);

			for (int k = 0; k < div_z; k++) {
				cell_status[i][j][k] = 0;
			}
		}
	}

	// Plot faces into cells.

	for (int i = 0; i < face_count; i++) {
		Face3 f = faces[i];
		for (int j = 0; j < 3; j++) {
			f.vertex[j] -= global_aabb.position;
		}
		_plot_face(cell_status, 0, 0, 0, div_x, div_y, div_z, voxelsize, f);
	}

	// Determine which cells connect to the outside by traversing the outside and recursively flood-fill marking.

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			_mark_outside(cell_status, i, j, 0, div_x, div_y, div_z);
			_mark_outside(cell_status, i, j, div_z - 1, div_x, div_y, div_z);
		}
	}

	for (int i = 0; i < div_z; i++) {
		for (int j = 0; j < div_y; j++) {
			_mark_outside(cell_status, 0, j, i, div_x, div_y, div_z);
			_mark_outside(cell_status, div_x - 1, j, i, div_x, div_y, div_z);
		}
	}

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_z; j++) {
			_mark_outside(cell_status, i, 0, j, div_x, div_y, div_z);
			_mark_outside(cell_status, i, div_y - 1, j, div_x, div_y, div_z);
		}
	}

	// Build faces for the inside-outside cell divisors.

	Vector<Face3> wrapped_faces;

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			for (int k = 0; k < div_z; k++) {
				_build_faces(cell_status, i, j, k, div_x, div_y, div_z, wrapped_faces);
			}
		}
	}

	// Transform face vertices to global coords.

	int wrapped_faces_count = wrapped_faces.size();
	Face3 *wrapped_faces_ptr = wrapped_faces.ptrw();

	for (int i = 0; i < wrapped_faces_count; i++) {
		for (int j = 0; j < 3; j++) {
			Vector3 &v = wrapped_faces_ptr[i].vertex[j];
			v = v * voxelsize;
			v += global_aabb.position;
		}
	}

	// clean up grid

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			memdelete_arr(cell_status[i][j]);
		}

		memdelete_arr(cell_status[i]);
	}

	memdelete_arr(cell_status);
	if (r_error) {
		*r_error = voxelsize.length();
	}

	return wrapped_faces;
}

Geometry3D::MeshData Geometry3D::build_convex_mesh(const Vector<Plane> &p_planes) {
	MeshData mesh;

#define SUBPLANE_SIZE 1024.0

	real_t subplane_size = 1024.0; // Should compute this from the actual plane.
	for (int i = 0; i < p_planes.size(); i++) {
		Plane p = p_planes[i];

		Vector3 ref = Vector3(0.0, 1.0, 0.0);

		if (Math::abs(p.normal.dot(ref)) > 0.95f) {
			ref = Vector3(0.0, 0.0, 1.0); // Change axis.
		}

		Vector3 right = p.normal.cross(ref).normalized();
		Vector3 up = p.normal.cross(right).normalized();

		Vector3 center = p.get_center();

		// make a quad clockwise
		LocalVector<Vector3> vertices = {
			center - up * subplane_size + right * subplane_size,
			center - up * subplane_size - right * subplane_size,
			center + up * subplane_size - right * subplane_size,
			center + up * subplane_size + right * subplane_size
		};

		for (int j = 0; j < p_planes.size(); j++) {
			if (j == i) {
				continue;
			}

			LocalVector<Vector3> new_vertices;
			Plane clip = p_planes[j];

			if (clip.normal.dot(p.normal) > 0.95f) {
				continue;
			}

			if (vertices.size() < 3) {
				break;
			}

			for (uint32_t k = 0; k < vertices.size(); k++) {
				int k_n = (k + 1) % vertices.size();

				Vector3 edge0_A = vertices[k];
				Vector3 edge1_A = vertices[k_n];

				real_t dist0 = clip.distance_to(edge0_A);
				real_t dist1 = clip.distance_to(edge1_A);

				if (dist0 <= 0) { // Behind plane.

					new_vertices.push_back(vertices[k]);
				}

				// Check for different sides and non coplanar.
				if ((dist0 * dist1) < 0) {
					// Calculate intersection.
					Vector3 rel = edge1_A - edge0_A;

					real_t den = clip.normal.dot(rel);
					if (Math::is_zero_approx(den)) {
						continue; // Point too short.
					}

					real_t dist = -(clip.normal.dot(edge0_A) - clip.d) / den;
					Vector3 inters = edge0_A + rel * dist;
					new_vertices.push_back(inters);
				}
			}

			vertices = new_vertices;
		}

		if (vertices.size() < 3) {
			continue;
		}

		// Result is a clockwise face.

		MeshData::Face face;

		// Add face indices.
		for (const Vector3 &vertex : vertices) {
			int idx = -1;
			for (uint32_t k = 0; k < mesh.vertices.size(); k++) {
				if (mesh.vertices[k].distance_to(vertex) < 0.001f) {
					idx = k;
					break;
				}
			}

			if (idx == -1) {
				idx = mesh.vertices.size();
				mesh.vertices.push_back(vertex);
			}

			face.indices.push_back(idx);
		}
		face.plane = p;
		mesh.faces.push_back(face);

		// Add edge.

		for (uint32_t j = 0; j < face.indices.size(); j++) {
			int a = face.indices[j];
			int b = face.indices[(j + 1) % face.indices.size()];

			bool found = false;
			int found_idx = -1;
			for (uint32_t k = 0; k < mesh.edges.size(); k++) {
				if (mesh.edges[k].vertex_a == a && mesh.edges[k].vertex_b == b) {
					found = true;
					found_idx = k;
					break;
				}
				if (mesh.edges[k].vertex_b == a && mesh.edges[k].vertex_a == b) {
					found = true;
					found_idx = k;
					break;
				}
			}

			if (found) {
				mesh.edges[found_idx].face_b = j;
				continue;
			}
			MeshData::Edge edge;
			edge.vertex_a = a;
			edge.vertex_b = b;
			edge.face_a = j;
			edge.face_b = -1;
			mesh.edges.push_back(edge);
		}
	}

	return mesh;
}

Vector<Plane> Geometry3D::build_box_planes(const Vector3 &p_extents) {
	Vector<Plane> planes = {
		Plane(Vector3(1, 0, 0), p_extents.x),
		Plane(Vector3(-1, 0, 0), p_extents.x),
		Plane(Vector3(0, 1, 0), p_extents.y),
		Plane(Vector3(0, -1, 0), p_extents.y),
		Plane(Vector3(0, 0, 1), p_extents.z),
		Plane(Vector3(0, 0, -1), p_extents.z)
	};

	return planes;
}

Vector<Plane> Geometry3D::build_cylinder_planes(real_t p_radius, real_t p_height, int p_sides, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	const double sides_step = Math::TAU / p_sides;
	for (int i = 0; i < p_sides; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * sides_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * sides_step);

		planes.push_back(Plane(normal, p_radius));
	}

	Vector3 axis;
	axis[p_axis] = 1.0;

	planes.push_back(Plane(axis, p_height * 0.5f));
	planes.push_back(Plane(-axis, p_height * 0.5f));

	return planes;
}

Vector<Plane> Geometry3D::build_sphere_planes(real_t p_radius, int p_lats, int p_lons, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	Vector3 axis;
	axis[p_axis] = 1.0;

	Vector3 axis_neg;
	axis_neg[(p_axis + 1) % 3] = 1.0;
	axis_neg[(p_axis + 2) % 3] = 1.0;
	axis_neg[p_axis] = -1.0;

	const double lon_step = Math::TAU / p_lons;
	for (int i = 0; i < p_lons; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * lon_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * lon_step);

		planes.push_back(Plane(normal, p_radius));

		for (int j = 1; j <= p_lats; j++) {
			Vector3 plane_normal = normal.lerp(axis, j / (real_t)p_lats).normalized();
			planes.push_back(Plane(plane_normal, p_radius));
			planes.push_back(Plane(plane_normal * axis_neg, p_radius));
		}
	}

	return planes;
}

Vector<Plane> Geometry3D::build_capsule_planes(real_t p_radius, real_t p_height, int p_sides, int p_lats, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	Vector3 axis;
	axis[p_axis] = 1.0;

	Vector3 axis_neg;
	axis_neg[(p_axis + 1) % 3] = 1.0;
	axis_neg[(p_axis + 2) % 3] = 1.0;
	axis_neg[p_axis] = -1.0;

	const double sides_step = Math::TAU / p_sides;
	for (int i = 0; i < p_sides; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * sides_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * sides_step);

		planes.push_back(Plane(normal, p_radius));

		for (int j = 1; j <= p_lats; j++) {
			Vector3 plane_normal = normal.lerp(axis, j / (real_t)p_lats).normalized();
			Vector3 position = axis * p_height * 0.5f + plane_normal * p_radius;
			planes.push_back(Plane(plane_normal, position));
			planes.push_back(Plane(plane_normal * axis_neg, position * axis_neg));
		}
	}

	return planes;
}

Vector<Vector3> Geometry3D::compute_convex_mesh_points(const Plane *p_planes, int p_plane_count) {
	Vector<Vector3> points;

	// Iterate through every unique combination of any three planes.
	for (int i = p_plane_count - 1; i >= 0; i--) {
		for (int j = i - 1; j >= 0; j--) {
			for (int k = j - 1; k >= 0; k--) {
				// Find the point where these planes all cross over (if they
				// do at all).
				Vector3 convex_shape_point;
				if (p_planes[i].intersect_3(p_planes[j], p_planes[k], &convex_shape_point)) {
					// See if any *other* plane excludes this point because it's
					// on the wrong side.
					bool excluded = false;
					for (int n = 0; n < p_plane_count; n++) {
						if (n != i && n != j && n != k) {
							real_t dp = p_planes[n].normal.dot(convex_shape_point);
							if (dp - p_planes[n].d > (real_t)CMP_EPSILON) {
								excluded = true;
								break;
							}
						}
					}

					// Only add the point if it passed all tests.
					if (!excluded) {
						points.push_back(convex_shape_point);
					}
				}
			}
		}
	}

	return points;
}

#define square(m_s) ((m_s) * (m_s))
#define BIG_VAL 1e20

/* dt of 1d function using squared distance */
static void edt(float *p_f, int p_stride, int p_n) {
	float *d = (float *)alloca(sizeof(float) * p_n + sizeof(int) * p_n + sizeof(float) * (p_n + 1));
	int *v = reinterpret_cast<int *>(&(d[p_n]));
	float *z = reinterpret_cast<float *>(&v[p_n]);

	int k = 0;
	v[0] = 0;
	z[0] = -BIG_VAL;
	z[1] = +BIG_VAL;
	for (int q = 1; q <= p_n - 1; q++) {
		float s = ((p_f[q * p_stride] + square(q)) - (p_f[v[k] * p_stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		while (s <= z[k]) {
			k--;
			s = ((p_f[q * p_stride] + square(q)) - (p_f[v[k] * p_stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		}
		k++;
		v[k] = q;

		z[k] = s;
		z[k + 1] = +BIG_VAL;
	}

	k = 0;
	for (int q = 0; q <= p_n - 1; q++) {
		while (z[k + 1] < q) {
			k++;
		}
		d[q] = square(q - v[k]) + p_f[v[k] * p_stride];
	}

	for (int i = 0; i < p_n; i++) {
		p_f[i * p_stride] = d[i];
	}
}

#undef square

Vector<uint32_t> Geometry3D::generate_edf(const Vector<bool> &p_voxels, const Vector3i &p_size, bool p_negative) {
	uint32_t float_count = p_size.x * p_size.y * p_size.z;

	ERR_FAIL_COND_V((uint32_t)p_voxels.size() != float_count, Vector<uint32_t>());

	float *work_memory = memnew_arr(float, float_count);
	for (uint32_t i = 0; i < float_count; i++) {
		work_memory[i] = BIG_VAL;
	}

	uint32_t y_mult = p_size.x;
	uint32_t z_mult = y_mult * p_size.y;

	//plot solid cells
	{
		const bool *voxr = p_voxels.ptr();
		for (uint32_t i = 0; i < float_count; i++) {
			bool plot = voxr[i];
			if (p_negative) {
				plot = !plot;
			}
			if (plot) {
				work_memory[i] = 0;
			}
		}
	}

	//process in each direction

	//xy->z

	for (int i = 0; i < p_size.x; i++) {
		for (int j = 0; j < p_size.y; j++) {
			edt(&work_memory[i + j * y_mult], z_mult, p_size.z);
		}
	}

	//xz->y

	for (int i = 0; i < p_size.x; i++) {
		for (int j = 0; j < p_size.z; j++) {
			edt(&work_memory[i + j * z_mult], y_mult, p_size.y);
		}
	}

	//yz->x
	for (int i = 0; i < p_size.y; i++) {
		for (int j = 0; j < p_size.z; j++) {
			edt(&work_memory[i * y_mult + j * z_mult], 1, p_size.x);
		}
	}

	Vector<uint32_t> ret;
	ret.resize(float_count);
	{
		uint32_t *w = ret.ptrw();
		for (uint32_t i = 0; i < float_count; i++) {
			w[i] = uint32_t(Math::sqrt(work_memory[i]));
		}
	}

	memdelete_arr(work_memory);

	return ret;
}

#undef BIG_VAL

Vector<int8_t> Geometry3D::generate_sdf8(const Vector<uint32_t> &p_positive, const Vector<uint32_t> &p_negative) {
	ERR_FAIL_COND_V(p_positive.size() != p_negative.size(), Vector<int8_t>());
	Vector<int8_t> sdf8;
	int s = p_positive.size();
	sdf8.resize(s);

	const uint32_t *rpos = p_positive.ptr();
	const uint32_t *rneg = p_negative.ptr();
	int8_t *wsdf = sdf8.ptrw();
	for (int i = 0; i < s; i++) {
		int32_t diff = int32_t(rpos[i]) - int32_t(rneg[i]);
		wsdf[i] = CLAMP(diff, -128, 127);
	}
	return sdf8;
}

// ============================================================================
// Merged from core/math/dynamic_bvh.cpp
// ============================================================================

#include "servers/rendering/renderer_viewport.h"

void DynamicBVH::_delete_node(Node *p_node) {
	node_allocator.free(p_node);
}

void DynamicBVH::_recurse_delete_node(Node *p_node) {
	if (!p_node->is_leaf()) {
		_recurse_delete_node(p_node->children[0]);
		_recurse_delete_node(p_node->children[1]);
	}
	if (p_node == bvh_root) {
		bvh_root = nullptr;
	}
	_delete_node(p_node);
}

DynamicBVH::Node *DynamicBVH::_create_node(Node *p_parent, void *p_data) {
	Node *node = node_allocator.alloc();
	node->parent = p_parent;
	node->data = p_data;
	return (node);
}

DynamicBVH::Node *DynamicBVH::_create_node_with_volume(Node *p_parent, const Volume &p_volume, void *p_data) {
	Node *node = _create_node(p_parent, p_data);
	node->volume = p_volume;
	return node;
}

void DynamicBVH::_insert_leaf(Node *p_root, Node *p_leaf) {
	if (!bvh_root) {
		bvh_root = p_leaf;
		p_leaf->parent = nullptr;
	} else {
		if (!p_root->is_leaf()) {
			do {
				p_root = p_root->children[p_leaf->volume.select_by_proximity(
						p_root->children[0]->volume,
						p_root->children[1]->volume)];
			} while (!p_root->is_leaf());
		}
		Node *prev = p_root->parent;
		Node *node = _create_node_with_volume(prev, p_leaf->volume.merge(p_root->volume), nullptr);
		if (prev) {
			prev->children[p_root->get_index_in_parent()] = node;
			node->children[0] = p_root;
			p_root->parent = node;
			node->children[1] = p_leaf;
			p_leaf->parent = node;
			do {
				if (!prev->volume.contains(node->volume)) {
					prev->volume = prev->children[0]->volume.merge(prev->children[1]->volume);
				} else {
					break;
				}
				node = prev;
			} while (nullptr != (prev = node->parent));
		} else {
			node->children[0] = p_root;
			p_root->parent = node;
			node->children[1] = p_leaf;
			p_leaf->parent = node;
			bvh_root = node;
		}
	}
}

DynamicBVH::Node *DynamicBVH::_remove_leaf(Node *r_leaf) {
	if (r_leaf == bvh_root) {
		bvh_root = nullptr;
		return (nullptr);
	} else {
		Node *parent = r_leaf->parent;
		Node *prev = parent->parent;
		Node *sibling = parent->children[1 - r_leaf->get_index_in_parent()];
		if (prev) {
			prev->children[parent->get_index_in_parent()] = sibling;
			sibling->parent = prev;
			_delete_node(parent);
			while (prev) {
				const Volume pb = prev->volume;
				prev->volume = prev->children[0]->volume.merge(prev->children[1]->volume);
				if (pb.is_not_equal_to(prev->volume)) {
					prev = prev->parent;
				} else {
					break;
				}
			}
			return (prev ? prev : bvh_root);
		} else {
			bvh_root = sibling;
			sibling->parent = nullptr;
			_delete_node(parent);
			return (bvh_root);
		}
	}
}

void DynamicBVH::_fetch_leaves(Node *p_root, LocalVector<Node *> &r_leaves, int p_depth) {
	if (p_root->is_internal() && p_depth) {
		_fetch_leaves(p_root->children[0], r_leaves, p_depth - 1);
		_fetch_leaves(p_root->children[1], r_leaves, p_depth - 1);
		_delete_node(p_root);
	} else {
		r_leaves.push_back(p_root);
	}
}

// Partitions leaves such that leaves[0, n) are on the
// left of axis, and leaves[n, count) are on the right
// of axis. returns N.
int DynamicBVH::_split(Node **r_leaves, int p_count, const Vector3 &p_org, const Vector3 &p_axis) {
	int begin = 0;
	int end = p_count;
	for (;;) {
		while (begin != end && r_leaves[begin]->is_left_of_axis(p_org, p_axis)) {
			++begin;
		}

		if (begin == end) {
			break;
		}

		while (begin != end && !r_leaves[end - 1]->is_left_of_axis(p_org, p_axis)) {
			--end;
		}

		if (begin == end) {
			break;
		}

		// swap out of place nodes
		--end;
		Node *temp = r_leaves[begin];
		r_leaves[begin] = r_leaves[end];
		r_leaves[end] = temp;
		++begin;
	}

	return begin;
}

DynamicBVH::Volume DynamicBVH::_bounds(Node **r_leaves, int p_count) {
	Volume volume = r_leaves[0]->volume;
	for (int i = 1, ni = p_count; i < ni; ++i) {
		volume = volume.merge(r_leaves[i]->volume);
	}
	return (volume);
}

void DynamicBVH::_bottom_up(Node **r_leaves, int p_count) {
	while (p_count > 1) {
		real_t minsize = Math::INF;
		int minidx[2] = { -1, -1 };
		for (int i = 0; i < p_count; ++i) {
			for (int j = i + 1; j < p_count; ++j) {
				const real_t sz = r_leaves[i]->volume.merge(r_leaves[j]->volume).get_size();
				if (sz < minsize) {
					minsize = sz;
					minidx[0] = i;
					minidx[1] = j;
				}
			}
		}
		Node *n[] = { r_leaves[minidx[0]], r_leaves[minidx[1]] };
		Node *p = _create_node_with_volume(nullptr, n[0]->volume.merge(n[1]->volume), nullptr);
		p->children[0] = n[0];
		p->children[1] = n[1];
		n[0]->parent = p;
		n[1]->parent = p;
		r_leaves[minidx[0]] = p;
		r_leaves[minidx[1]] = r_leaves[p_count - 1];
		--p_count;
	}
}

DynamicBVH::Node *DynamicBVH::_top_down(Node **r_leaves, int p_count, int p_bu_threshold) {
	static const Vector3 axis[] = { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1) };

	ERR_FAIL_COND_V(p_bu_threshold <= 1, nullptr);
	if (p_count > 1) {
		if (p_count > p_bu_threshold) {
			const Volume vol = _bounds(r_leaves, p_count);
			const Vector3 org = vol.get_center();
			int partition;
			int bestaxis = -1;
			int bestmidp = p_count;
			int splitcount[3][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
			int i;
			for (i = 0; i < p_count; ++i) {
				const Vector3 x = r_leaves[i]->volume.get_center() - org;
				for (int j = 0; j < 3; ++j) {
					++splitcount[j][x.dot(axis[j]) > 0 ? 1 : 0];
				}
			}
			for (i = 0; i < 3; ++i) {
				if ((splitcount[i][0] > 0) && (splitcount[i][1] > 0)) {
					const int midp = (int)Math::abs(real_t(splitcount[i][0] - splitcount[i][1]));
					if (midp < bestmidp) {
						bestaxis = i;
						bestmidp = midp;
					}
				}
			}
			if (bestaxis >= 0) {
				partition = _split(r_leaves, p_count, org, axis[bestaxis]);
				ERR_FAIL_COND_V(partition == 0 || partition == p_count, nullptr);
			} else {
				partition = p_count / 2 + 1;
			}

			Node *node = _create_node_with_volume(nullptr, vol, nullptr);
			node->children[0] = _top_down(&r_leaves[0], partition, p_bu_threshold);
			node->children[1] = _top_down(&r_leaves[partition], p_count - partition, p_bu_threshold);
			node->children[0]->parent = node;
			node->children[1]->parent = node;
			return (node);
		} else {
			_bottom_up(r_leaves, p_count);
			return (r_leaves[0]);
		}
	}
	return (r_leaves[0]);
}

DynamicBVH::Node *DynamicBVH::_node_sort(Node *r_node, Node *&r_root) {
	Node *p = r_node->parent;
	ERR_FAIL_COND_V(!r_node->is_internal(), nullptr);
	if (p > r_node) {
		const int i = r_node->get_index_in_parent();
		const int j = 1 - i;
		Node *s = p->children[j];
		Node *q = p->parent;
		ERR_FAIL_COND_V(r_node != p->children[i], nullptr);
		if (q) {
			q->children[p->get_index_in_parent()] = r_node;
		} else {
			r_root = r_node;
		}
		s->parent = r_node;
		p->parent = r_node;
		r_node->parent = q;
		p->children[0] = r_node->children[0];
		p->children[1] = r_node->children[1];
		r_node->children[0]->parent = p;
		r_node->children[1]->parent = p;
		r_node->children[i] = p;
		r_node->children[j] = s;
		SWAP(p->volume, r_node->volume);
		return (p);
	}
	return (r_node);
}

void DynamicBVH::clear() {
	if (bvh_root) {
		_recurse_delete_node(bvh_root);
	}
	lkhd = -1;
	opath = 0;
}

void DynamicBVH::optimize_bottom_up() {
	if (bvh_root) {
		LocalVector<Node *> leaves;
		_fetch_leaves(bvh_root, leaves);
		_bottom_up(&leaves[0], leaves.size());
		bvh_root = leaves[0];
	}
}

void DynamicBVH::optimize_top_down(int p_bu_threshold) {
	if (bvh_root) {
		LocalVector<Node *> leaves;
		_fetch_leaves(bvh_root, leaves);
		bvh_root = _top_down(&leaves[0], leaves.size(), p_bu_threshold);
	}
}

void DynamicBVH::optimize_incremental(int p_passes) {
	if (p_passes < 0) {
		p_passes = total_leaves;
	}
	if (p_passes > 0) {
		do {
			if (!bvh_root) {
				break;
			}
			Node *node = bvh_root;
			unsigned bit = 0;
			while (node->is_internal()) {
				node = _node_sort(node, bvh_root)->children[(opath >> bit) & 1];
				bit = (bit + 1) & (sizeof(unsigned) * 8 - 1);
			}
			_update(node);
			++opath;
		} while (--p_passes);
	}
}

DynamicBVH::ID DynamicBVH::insert(const AABB &p_box, void *p_userdata) {
	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	Node *leaf = _create_node_with_volume(nullptr, volume, p_userdata);
	_insert_leaf(bvh_root, leaf);
	++total_leaves;

	ID id;
	id.node = leaf;

	return id;
}

void DynamicBVH::_update(Node *r_leaf, int p_lookahead) {
	Node *root = _remove_leaf(r_leaf);
	if (root) {
		if (p_lookahead >= 0) {
			for (int i = 0; (i < p_lookahead) && root->parent; ++i) {
				root = root->parent;
			}
		} else {
			root = bvh_root;
		}
	}
	_insert_leaf(root, r_leaf);
}

bool DynamicBVH::update(const ID &p_id, const AABB &p_box) {
	ERR_FAIL_COND_V(!p_id.is_valid(), false);
	Node *leaf = p_id.node;

	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	if (leaf->volume.min.is_equal_approx(volume.min) && leaf->volume.max.is_equal_approx(volume.max)) {
		// noop
		return false;
	}

	Node *base = _remove_leaf(leaf);
	if (base) {
		if (lkhd >= 0) {
			for (int i = 0; (i < lkhd) && base->parent; ++i) {
				base = base->parent;
			}
		} else {
			base = bvh_root;
		}
	}
	leaf->volume = volume;
	_insert_leaf(base, leaf);
	return true;
}

void DynamicBVH::remove(const ID &p_id) {
	ERR_FAIL_COND(!p_id.is_valid());
	Node *leaf = p_id.node;
	_remove_leaf(leaf);
	_delete_node(leaf);
	--total_leaves;
}

void DynamicBVH::_extract_leaves(Node *p_node, List<ID> *r_elements) {
	if (p_node->is_internal()) {
		_extract_leaves(p_node->children[0], r_elements);
		_extract_leaves(p_node->children[1], r_elements);
	} else {
		ID id;
		id.node = p_node;
		r_elements->push_back(id);
	}
}

void DynamicBVH::set_index(uint32_t p_index) {
	ERR_FAIL_COND(bvh_root != nullptr);
	index = p_index;
}

uint32_t DynamicBVH::get_index() const {
	return index;
}

void DynamicBVH::get_elements(List<ID> *r_elements) {
	if (bvh_root) {
		_extract_leaves(bvh_root, r_elements);
	}
}

int DynamicBVH::get_leaf_count() const {
	return total_leaves;
}
int DynamicBVH::get_max_depth() const {
	if (bvh_root) {
		int depth = 1;
		int max_depth = 0;
		bvh_root->get_max_depth(depth, max_depth);
		return max_depth;
	} else {
		return 0;
	}
}

DynamicBVH::~DynamicBVH() {
	clear();
}

// ============================================================================
// Merged from core/math/triangle_mesh.cpp
// ============================================================================

#include "core/templates/sort_array.h"
#include "core/math/geometry_3d.h"

int TriangleMesh::_create_bvh(
	BVH* p_bvh, BVH** p_bb, int p_from, int p_size, int p_depth, int& r_max_depth, int& r_max_alloc)
{
	if (p_depth > r_max_depth) {
		r_max_depth = p_depth;
	}

	if (p_size == 1) {
		return p_bb[p_from] - p_bvh;
	}
	else if (p_size == 0) {
		return -1;
	}

	AABB aabb;
	aabb = p_bb[p_from]->aabb;
	for (int i = 1; i < p_size; i++) {
		aabb.merge_with(p_bb[p_from + i]->aabb);
	}

	int li = aabb.get_longest_axis_index();

	switch (li) {
	case Vector3::AXIS_X: {
		SortArray<BVH*, BVHCmpX> sort_x;
		sort_x.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_x.sort(&p_bb[p_from],p_size);
	} break;
	case Vector3::AXIS_Y: {
		SortArray<BVH*, BVHCmpY> sort_y;
		sort_y.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_y.sort(&p_bb[p_from],p_size);
	} break;
	case Vector3::AXIS_Z: {
		SortArray<BVH*, BVHCmpZ> sort_z;
		sort_z.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_z.sort(&p_bb[p_from],p_size);

	} break;
	}

	int left = _create_bvh(p_bvh, p_bb, p_from, p_size / 2, p_depth + 1, r_max_depth, r_max_alloc);
	int right = _create_bvh(p_bvh, p_bb, p_from + p_size / 2, p_size - p_size / 2, p_depth + 1,
		r_max_depth, r_max_alloc);

	int index = r_max_alloc++;
	BVH* _new = &p_bvh[index];
	_new->aabb = aabb;
	_new->center = aabb.get_center();
	_new->face_index = -1;
	_new->left = left;
	_new->right = right;

	return index;
}

void TriangleMesh::get_indices(Vector<int>* r_triangles_indices) const
{
	if (!valid) {
		return;
	}

	const int triangles_num = triangles.size();

	// Parse vertices indices
	const Triangle* triangles_read = triangles.ptr();

	r_triangles_indices->resize(triangles_num * 3);
	int* r_indices_write = r_triangles_indices->ptrw();

	for (int i = 0; i < triangles_num; ++i) {
		r_indices_write[3 * i + 0] = triangles_read[i].indices[0];
		r_indices_write[3 * i + 1] = triangles_read[i].indices[1];
		r_indices_write[3 * i + 2] = triangles_read[i].indices[2];
	}
}

void TriangleMesh::create(const Vector<Vector3>& p_faces, const Vector<int32_t>& p_surface_indices)
{
	valid = false;

	ERR_FAIL_COND(p_surface_indices.size() && p_surface_indices.size() != p_faces.size());

	int fc = p_faces.size();
	ERR_FAIL_COND(!fc || ((fc % 3) != 0));
	fc /= 3;
	triangles.resize(fc);

	bvh.resize(fc * 3); // will never be larger than this (todo make better)
	BVH* bw = bvh.ptrw();

	{
		// create faces and indices and base bvh
		// except for the Set for repeated triangles, everything
		// goes in-place.

		const Vector3* r = p_faces.ptr();
		const int32_t* si = p_surface_indices.ptr();
		Triangle* w = triangles.ptrw();
		HashMap<Vector3, int> db;

		for (int i = 0; i < fc; i++) {
			Triangle& f = w[i];
			const Vector3* v = &r[i * 3];

			for (int j = 0; j < 3; j++) {
				int vidx = -1;
				Vector3 vs = v[j].snappedf(0.0001);
				HashMap<Vector3, int>::Iterator E = db.find(vs);
				if (E) {
					vidx = E->value;
				}
				else {
					vidx = db.size();
					db[vs] = vidx;
				}

				f.indices[j] = vidx;
				if (j == 0) {
					bw[i].aabb.position = vs;
				}
				else {
					bw[i].aabb.expand_to(vs);
				}
			}

			f.normal = Face3(r[i * 3 + 0], r[i * 3 + 1], r[i * 3 + 2]).get_plane().get_normal();
			f.surface_index = si ? si[i] : 0;

			bw[i].left = -1;
			bw[i].right = -1;
			bw[i].face_index = i;
			bw[i].center = bw[i].aabb.get_center();
		}

		vertices.resize(db.size());
		Vector3* vw = vertices.ptrw();
		for (const KeyValue<Vector3, int>& E : db) {
			vw[E.value] = E.key;
		}
	}

	Vector<BVH*> bwptrs;
	bwptrs.resize(fc);
	BVH** bwp = bwptrs.ptrw();
	for (int i = 0; i < fc; i++) {
		bwp[i] = &bw[i];
	}

	max_depth = 0;
	int max_alloc = fc;
	_create_bvh(bw, bwp, 0, fc, 1, max_depth, max_alloc);

	bvh.resize(max_alloc); // resize back

	valid = true;
}

bool TriangleMesh::intersect_segment(const Vector3& p_begin, const Vector3& p_end, Vector3& r_point,
	Vector3& r_normal, int32_t* r_surf_index, int32_t* r_face_index) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	Vector3 n = (p_end - p_begin).normalized();
	real_t d = 1e10;
	bool inters = false;

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			if (!b.aabb.intersects_segment(p_begin, p_end)) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					Face3 f3(
						vertexptr[s.indices[0]], vertexptr[s.indices[1]], vertexptr[s.indices[2]]);

					Vector3 res;

					if (f3.intersects_segment(p_begin, p_end, &res)) {
						real_t nd = n.dot(res);
						if (nd < d) {
							d = nd;
							r_point = res;
							r_normal = f3.get_plane().get_normal();
							if (r_surf_index) {
								*r_surf_index = s.surface_index;
							}
							if (r_face_index) {
								*r_face_index = b.face_index;
							}
							inters = true;
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	if (inters) {
		if (n.dot(r_normal) > 0) {
			r_normal = -r_normal;
		}
	}

	return inters;
}

bool TriangleMesh::intersect_ray(const Vector3& p_begin, const Vector3& p_dir, Vector3& r_point,
	Vector3& r_normal, int32_t* r_surf_index, int32_t* r_face_index) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	Vector3 n = p_dir;
	real_t d = 1e20;
	bool inters = false;

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			if (!b.aabb.intersects_ray(p_begin, p_dir)) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					Face3 f3(
						vertexptr[s.indices[0]], vertexptr[s.indices[1]], vertexptr[s.indices[2]]);

					Vector3 res;

					if (f3.intersects_ray(p_begin, p_dir, &res)) {
						real_t nd = n.dot(res);
						if (nd < d) {
							d = nd;
							r_point = res;
							r_normal = f3.get_plane().get_normal();
							if (r_surf_index) {
								*r_surf_index = s.surface_index;
							}
							if (r_face_index) {
								*r_face_index = b.face_index;
							}
							inters = true;
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	if (inters) {
		if (n.dot(r_normal) > 0) {
			r_normal = -r_normal;
		}
	}

	return inters;
}

bool TriangleMesh::inside_convex_shape(const Plane* p_planes, int p_plane_count,
	const Vector3* p_points, int p_point_count, Vector3 p_scale) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	Transform3D scale(Basis().scaled(p_scale));

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			bool intersects = scale.xform(b.aabb).intersects_convex_shape(
				p_planes, p_plane_count, p_points, p_point_count);
			if (!intersects) {
				return false;
			}

			bool inside = scale.xform(b.aabb).inside_convex_shape(p_planes, p_plane_count);
			if (inside) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					for (int j = 0; j < 3; ++j) {
						Vector3 point = scale.xform(vertexptr[s.indices[j]]);
						for (int i = 0; i < p_plane_count; i++) {
							const Plane& p = p_planes[i];
							if (p.is_point_over(point)) {
								return false;
							}
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	return true;
}

bool TriangleMesh::is_valid() const { return valid; }

Vector<Face3> TriangleMesh::get_faces() const
{
	if (!valid) {
		return Vector<Face3>();
	}

	Vector<Face3> faces;
	int ts = triangles.size();
	faces.resize(triangles.size());

	Face3* w = faces.ptrw();
	const Triangle* r = triangles.ptr();
	const Vector3* rv = vertices.ptr();

	for (int i = 0; i < ts; i++) {
		for (int j = 0; j < 3; j++) {
			w[i].vertex[j] = rv[r[i].indices[j]];
		}
	}

	return faces;
}

bool TriangleMesh::create_from_faces(const Vector<Vector3>& p_faces)
{
	create(p_faces);
	return is_valid();
}

Vector<Vector3> TriangleMesh::get_faces_scriptwrap() const
{
	if (!valid) {
		return Vector<Vector3>();
	}

	Vector<Vector3> faces;
	int ts = triangles.size();
	faces.resize(triangles.size() * 3);

	Vector3* w = faces.ptrw();
	const Triangle* r = triangles.ptr();
	const Vector3* rv = vertices.ptr();

	for (int i = 0; i < ts; i++) {
		for (int j = 0; j < 3; j++) {
			w[i * 3 + j] = rv[r[i].indices[j]];
		}
	}

	return faces;
}

TriangleMesh::TriangleMesh()
{
	valid = false;
	max_depth = 0;
}


// ============================================================================
// Merged from core/math/expression.cpp
// ============================================================================

#include "expression.h"

Error Expression::_get_token(Token& r_token)
{
	while (true) {
#define GET_CHAR() (str_ofs >= expression.length() ? 0 : expression[str_ofs++])

		char32_t cchar = GET_CHAR();

		switch (cchar) {
		case 0: {
			r_token.type = TK_EOF;
			return OK;
		}
		case '{': {
			r_token.type = TK_CURLY_BRACKET_OPEN;
			return OK;
		}
		case '}': {
			r_token.type = TK_CURLY_BRACKET_CLOSE;
			return OK;
		}
		case '[': {
			r_token.type = TK_BRACKET_OPEN;
			return OK;
		}
		case ']': {
			r_token.type = TK_BRACKET_CLOSE;
			return OK;
		}
		case '(': {
			r_token.type = TK_PARENTHESIS_OPEN;
			return OK;
		}
		case ')': {
			r_token.type = TK_PARENTHESIS_CLOSE;
			return OK;
		}
		case ',': {
			r_token.type = TK_COMMA;
			return OK;
		}
		case ':': {
			r_token.type = TK_COLON;
			return OK;
		}
		case '$': {
			r_token.type = TK_INPUT;
			int index = 0;
			do {
				if (!is_digit(expression[str_ofs])) {
					_set_error("Expected number after '$'");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				index *= 10;
				index += expression[str_ofs] - '0';
				str_ofs++;

			} while (is_digit(expression[str_ofs]));
			return OK;
		}
		case '=': {
			cchar = GET_CHAR();
			if (cchar == '=') {
				r_token.type = TK_OP_EQUAL;
			}
			else {
				_set_error("Expected '='");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
			return OK;
		}
		case '!': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_NOT_EQUAL;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_NOT;
			}
			return OK;
		}
		case '>': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_GREATER_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '>') {
				r_token.type = TK_OP_SHIFT_RIGHT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_GREATER;
			}
			return OK;
		}
		case '<': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_LESS_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '<') {
				r_token.type = TK_OP_SHIFT_LEFT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_LESS;
			}
			return OK;
		}
		case '+': {
			r_token.type = TK_OP_ADD;
			return OK;
		}
		case '-': {
			r_token.type = TK_OP_SUB;
			return OK;
		}
		case '/': {
			r_token.type = TK_OP_DIV;
			return OK;
		}
		case '*': {
			if (expression[str_ofs] == '*') {
				r_token.type = TK_OP_POW;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_MUL;
			}
			return OK;
		}
		case '%': {
			r_token.type = TK_OP_MOD;
			return OK;
		}
		case '&': {
			if (expression[str_ofs] == '&') {
				r_token.type = TK_OP_AND;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_AND;
			}
			return OK;
		}
		case '|': {
			if (expression[str_ofs] == '|') {
				r_token.type = TK_OP_OR;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_OR;
			}
			return OK;
		}
		case '^': {
			r_token.type = TK_OP_BIT_XOR;

			return OK;
		}
		case '~': {
			r_token.type = TK_OP_BIT_INVERT;

			return OK;
		}
		case '\'':
		case '"': {
			String str;
			char32_t prev = 0;
			while (true) {
				char32_t ch = GET_CHAR();

				if (ch == 0) {
					_set_error("Unterminated String");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				else if (ch == cchar) {
					// cchar contain a corresponding quote symbol
					break;
				}
				else if (ch == '\\') {
					// escaped characters...

					char32_t next = GET_CHAR();
					if (next == 0) {
						_set_error("Unterminated String");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					char32_t res = 0;

					switch (next) {
					case 'b':
						res = 8;
						break;
					case 't':
						res = 9;
						break;
					case 'n':
						res = 10;
						break;
					case 'f':
						res = 12;
						break;
					case 'r':
						res = 13;
						break;
					case 'U':
					case 'u': {
						// Hexadecimal sequence.
						int hex_len = (next == 'U') ? 6 : 4;
						for (int j = 0; j < hex_len; j++) {
							char32_t c = GET_CHAR();

							if (c == 0) {
								_set_error("Unterminated String");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							if (!is_hex_digit(c)) {
								_set_error("Malformed hex constant in string");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							char32_t v;
							if (is_digit(c)) {
								v = c - '0';
							}
							else if (c >= 'a' && c <= 'f') {
								v = c - 'a';
								v += 10;
							}
							else if (c >= 'A' && c <= 'F') {
								v = c - 'A';
								v += 10;
							}
							else {
								ERR_PRINT("Bug parsing hex constant.");
								v = 0;
							}

							res <<= 4;
							res |= v;
						}

					} break;
					default: {
						res = next;
					} break;
					}

					// Parse UTF-16 pair.
					if ((res & 0xfffffc00) == 0xd800) {
						if (prev == 0) {
							prev = res;
							continue;
						}
						else {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired lead surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
					}
					else if ((res & 0xfffffc00) == 0xdc00) {
						if (prev == 0) {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired trail surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
						else {
							res = (prev << 10UL) + res - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
							prev = 0;
						}
					}
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += res;
				}
				else {
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += ch;
				}
			}
			if (prev != 0) {
				_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}

			r_token.type = TK_CONSTANT;
			return OK;

		} break;
		default: {
			if (cchar <= 32) {
				break;
			}

			char32_t next_char = (str_ofs >= expression.length()) ? 0 : expression[str_ofs];
			if (is_digit(cchar) || (cchar == '.' && is_digit(next_char))) {
				// a number

				String num;
#define READING_SIGN 0
#define READING_INT 1
#define READING_HEX 2
#define READING_BIN 3
#define READING_DEC 4
#define READING_EXP 5
#define READING_DONE 6
				int reading = READING_INT;

				char32_t c = cchar;
				bool exp_sign = false;
				bool exp_beg = false;
				bool bin_beg = false;
				bool hex_beg = false;
				bool is_float = false;
				bool is_first_char = true;

				while (true) {
					switch (reading) {
					case READING_INT: {
						if (is_digit(c)) {
							if (is_first_char && c == '0') {
								if (next_char == 'b' || next_char == 'B') {
									reading = READING_BIN;
								}
								else if (next_char == 'x' || next_char == 'X') {
									reading = READING_HEX;
								}
							}
						}
						else if (c == '.') {
							reading = READING_DEC;
							is_float = true;
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
							is_float = true;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_BIN: {
						if (bin_beg && !is_binary_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'b' || c == 'B') {
							bin_beg = true;
						}

					} break;
					case READING_HEX: {
						if (hex_beg && !is_hex_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'x' || c == 'X') {
							hex_beg = true;
						}

					} break;
					case READING_DEC: {
						if (is_digit(c)) {
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_EXP: {
						if (is_digit(c)) {
							exp_beg = true;

						}
						else if ((c == '-' || c == '+') && !exp_sign && !exp_beg) {
							exp_sign = true;

						}
						else {
							reading = READING_DONE;
						}
					} break;
					}

					if (reading == READING_DONE) {
						break;
					}
					num += c;
					c = GET_CHAR();
					is_first_char = false;
				}

				if (c != 0) {
					str_ofs--;
				}

				r_token.type = TK_CONSTANT;

				return OK;

			}
			else if (is_unicode_identifier_start(cchar)) {
				String id = String::chr(cchar);
				cchar = GET_CHAR();

				while (is_unicode_identifier_continue(cchar)) {
					id += cchar;
					cchar = GET_CHAR();
				}

				str_ofs--; // go back one

				if (id == "in") {
					r_token.type = TK_OP_IN;
				}
				else if (id == "null") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "true") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "false") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "PI") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "TAU") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "INF") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "NAN") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "not") {
					r_token.type = TK_OP_NOT;
				}
				else if (id == "or") {
					r_token.type = TK_OP_OR;
				}
				else if (id == "and") {
					r_token.type = TK_OP_AND;
				}
				else if (id == "self") {
					r_token.type = TK_SELF;
				}
				else {
					r_token.type = TK_IDENTIFIER;
				}
				return OK;

			}
			else if (cchar == '.') {
				// Handled down there as we support '.[0-9]' as numbers above
				r_token.type = TK_PERIOD;
				return OK;

			}
			else {
				_set_error("Unexpected character.");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
		}
		}
#undef GET_CHAR
	}

	r_token.type = TK_ERROR;
	return ERR_PARSE_ERROR;
}

const char* Expression::token_name[TK_MAX] = {"CURLY BRACKET OPEN", "CURLY BRACKET CLOSE",
	"BRACKET OPEN", "BRACKET CLOSE", "PARENTHESIS OPEN", "PARENTHESIS CLOSE", "IDENTIFIER",
	"BUILTIN FUNC", "SELF", "CONSTANT", "BASIC TYPE", "COLON", "COMMA", "PERIOD", "OP IN",
	"OP EQUAL", "OP NOT EQUAL", "OP LESS", "OP LESS EQUAL", "OP GREATER", "OP GREATER EQUAL",
	"OP AND", "OP OR", "OP NOT", "OP ADD", "OP SUB", "OP MUL", "OP DIV", "OP MOD", "OP POW",
	"OP SHIFT LEFT", "OP SHIFT RIGHT", "OP BIT AND", "OP BIT OR", "OP BIT XOR", "OP BIT INVERT",
	"OP INPUT", "EOF", "ERROR"};

bool Expression::_compile_expression()
{
	if (!expression_dirty) {
		return error_set;
	}

	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;

	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return true;
	}

	expression_dirty = false;
	return false;
}

Error Expression::parse(const String& p_expression, const Vector<String>& p_input_names)
{
	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;
	input_names = p_input_names;

	expression = p_expression;
	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return ERR_INVALID_PARAMETER;
	}

	return OK;
}

bool Expression::has_execute_failed() const { return execution_error; }

String Expression::get_error_text() const { return error_str; }

Expression::~Expression()
{
	if (nodes) {
		memdelete(nodes);
	}
}

Expression::ENode* Expression::_parse_expression()
{
	return nullptr;
}


// ============================================================================
// Merged from core/math/a_star.cpp
// ============================================================================

#include "core/math/geometry_3d.h"

int64_t AStar3D::get_available_point_id() const
{
	if (points.has(last_free_id)) {
		int64_t cur_new_id = last_free_id + 1;
		while (points.has(cur_new_id)) {
			cur_new_id++;
		}
		last_free_id = cur_new_id;
	}

	return last_free_id;
}

void AStar3D::add_point(int64_t p_id, const Vector3& p_pos, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(p_id < 0, vformat("Can't add a point with negative id: %d.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't add a point with weight scale less than 0.0: %f.", p_weight_scale));

	Point** point_entry = points.getptr(p_id);

	if (!point_entry) {
		Point* pt = memnew(Point);
		pt->id = p_id;
		pt->pos = p_pos;
		pt->weight_scale = p_weight_scale;
		pt->prev_point = nullptr;
		pt->open_pass = 0;
		pt->closed_pass = 0;
		pt->enabled = true;
		points.insert_new(p_id, pt);
	}
	else {
		Point* found_pt = *point_entry;
		found_pt->pos = p_pos;
		found_pt->weight_scale = p_weight_scale;
	}
}

Vector3 AStar3D::get_point_position(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, Vector3(),
		vformat("Can't get point's position. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->pos;
}

void AStar3D::set_point_position(int64_t p_id, const Vector3& p_pos)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's position. Point with id: %d doesn't exist.", p_id));

	(*point_entry)->pos = p_pos;
}

real_t AStar3D::get_point_weight_scale(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, 0,
		vformat("Can't get point's weight scale. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->weight_scale;
}

void AStar3D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's weight scale. Point with id: %d doesn't exist.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	(*point_entry)->weight_scale = p_weight_scale;
}

void AStar3D::remove_point(int64_t p_id)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!point_entry, vformat("Can't remove point. Point with id: %d doesn't exist.", p_id));
	Point* p = *point_entry;

	for (KeyValue<int64_t, Point*>& kv : p->neighbors) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	for (KeyValue<int64_t, Point*>& kv : p->unlinked_neighbours) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	memdelete(p);
	points.erase(p_id);
	last_free_id = p_id;
}

void AStar3D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	ERR_FAIL_COND_MSG(
		p_id == p_with_id, vformat("Can't connect point with id: %d to itself.", p_id));

	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	a->neighbors.insert(b->id, b);

	if (p_bidirectional) {
		b->neighbors.insert(a->id, a);
	}
	else {
		b->unlinked_neighbours.insert(a->id, a);
	}

	Segment s(p_id, p_with_id);
	if (p_bidirectional) {
		s.direction = Segment::BIDIRECTIONAL;
	}

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		s.direction |= element->direction;
		if (s.direction == Segment::BIDIRECTIONAL) {
			// Both are neighbors of each other now
			a->unlinked_neighbours.erase(b->id);
			b->unlinked_neighbours.erase(a->id);
		}
		segments.remove(element);
	}

	segments.insert(s);
}

void AStar3D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	Segment s(p_id, p_with_id);
	int remove_direction = p_bidirectional ? (int)Segment::BIDIRECTIONAL : (int)s.direction;

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		// s is the new segment
		// Erase the directions to be removed
		s.direction = (element->direction & ~remove_direction);

		a->neighbors.erase(b->id);
		if (p_bidirectional) {
			b->neighbors.erase(a->id);
			if (element->direction != Segment::BIDIRECTIONAL) {
				a->unlinked_neighbours.erase(b->id);
				b->unlinked_neighbours.erase(a->id);
			}
		}
		else {
			if (s.direction == Segment::NONE) {
				b->unlinked_neighbours.erase(a->id);
			}
			else {
				a->unlinked_neighbours.insert(b->id, b);
			}
		}

		segments.remove(element);
		if (s.direction != Segment::NONE) {
			segments.insert(s);
		}
	}
}

bool AStar3D::has_point(int64_t p_id) const { return points.has(p_id); }

PackedInt64Array AStar3D::get_point_ids()
{
	PackedInt64Array point_list;

	for (KeyValue<int64_t, Point*>& kv : points) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

Vector<int64_t> AStar3D::get_point_connections(int64_t p_id)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, Vector<int64_t>(),
		vformat("Can't get point's connections. Point with id: %d doesn't exist.", p_id));

	Vector<int64_t> point_list;

	for (KeyValue<int64_t, Point*>& kv : (*p_entry)->neighbors) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

bool AStar3D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	Segment s(p_id, p_with_id);
	const HashSet<Segment, Segment>::Iterator element = segments.find(s);

	return element && (p_bidirectional || (element->direction & s.direction) == s.direction);
}

void AStar3D::clear()
{
	last_free_id = 0;
	for (KeyValue<int64_t, Point*>& kv : points) {
		memdelete(kv.value);
	}
	segments.clear();
	points.clear();
}

int64_t AStar3D::get_point_count() const { return points.size(); }

int64_t AStar3D::get_point_capacity() const { return points.get_capacity(); }

void AStar3D::reserve_space(int64_t p_num_nodes)
{
	ERR_FAIL_COND_MSG(p_num_nodes <= 0,
		vformat("New capacity must be greater than 0, new was: %d.", p_num_nodes));
	points.reserve(p_num_nodes);
}

int64_t AStar3D::get_closest_point(const Vector3& p_point, bool p_include_disabled) const
{
	int64_t closest_id = -1;
	real_t closest_dist = 1e20;

	for (const KeyValue<int64_t, Point*>& kv : points) {
		if (!p_include_disabled && !kv.value->enabled) {
			continue; // Disabled points should not be considered.
		}

		// Keep the closest point's ID, and in case of multiple closest IDs,
		// the smallest one (makes it deterministic).
		real_t d = p_point.distance_squared_to(kv.value->pos);
		int64_t id = kv.key;
		if (d <= closest_dist) {
			if (d == closest_dist && id > closest_id) { // Keep lowest ID.
				continue;
			}
			closest_dist = d;
			closest_id = id;
		}
	}

	return closest_id;
}

Vector3 AStar3D::get_closest_position_in_segment(const Vector3& p_point) const
{
	real_t closest_dist = 1e20;
	Vector3 closest_point;

	for (const Segment& E : segments) {
		const Point* from_point = *points.getptr(E.key.first);
		const Point* to_point = *points.getptr(E.key.second);

		if (!(from_point->enabled && to_point->enabled)) {
			continue;
		}

		Vector3 p =
			Geometry3D::get_closest_point_to_segment(p_point, from_point->pos, to_point->pos);
		real_t d = p_point.distance_squared_to(p);
		if (d < closest_dist) {
			closest_point = p;
			closest_dist = d;
		}
	}

	return closest_point;
}

bool AStar3D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		for (const KeyValue<int64_t, Point*>& kv : p->neighbors) {
			Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStar3D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** end_entry = points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar3D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** to_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector3> AStar3D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector3>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector3> path;
	path.resize(pc);

	{
		Vector3* w = path.ptrw();

		Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = p2->pos;
			p2 = p2->prev_point;
		}

		w[0] = p2->pos; // Assign first
	}

	return path;
}

Vector<int64_t> AStar3D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar3D::is_neighbor_filter_enabled() const { return neighbor_filter_enabled; }

void AStar3D::set_neighbor_filter_enabled(bool p_enabled) { neighbor_filter_enabled = p_enabled; }

void AStar3D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!p_entry,
		vformat("Can't set if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	p->enabled = !p_disabled;
}

bool AStar3D::is_point_disabled(int64_t p_id) const
{
	Point* const* p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, false,
		vformat("Can't get if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	return !p->enabled;
}

AStar3D::~AStar3D() { clear(); }

int64_t AStar2D::get_available_point_id() const { return astar.get_available_point_id(); }

void AStar2D::add_point(int64_t p_id, const Vector2& p_pos, real_t p_weight_scale)
{
	astar.add_point(p_id, Vector3(p_pos.x, p_pos.y, 0), p_weight_scale);
}

Vector2 AStar2D::get_point_position(int64_t p_id) const
{
	Vector3 p = astar.get_point_position(p_id);
	return Vector2(p.x, p.y);
}

void AStar2D::set_point_position(int64_t p_id, const Vector2& p_pos)
{
	astar.set_point_position(p_id, Vector3(p_pos.x, p_pos.y, 0));
}

real_t AStar2D::get_point_weight_scale(int64_t p_id) const
{
	return astar.get_point_weight_scale(p_id);
}

void AStar2D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	astar.set_point_weight_scale(p_id, p_weight_scale);
}

void AStar2D::remove_point(int64_t p_id) { astar.remove_point(p_id); }

bool AStar2D::has_point(int64_t p_id) const { return astar.has_point(p_id); }

Vector<int64_t> AStar2D::get_point_connections(int64_t p_id)
{
	return astar.get_point_connections(p_id);
}

PackedInt64Array AStar2D::get_point_ids() { return astar.get_point_ids(); }

bool AStar2D::is_neighbor_filter_enabled() const { return astar.neighbor_filter_enabled; }

void AStar2D::set_neighbor_filter_enabled(bool p_enabled)
{
	astar.neighbor_filter_enabled = p_enabled;
}

void AStar2D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	astar.set_point_disabled(p_id, p_disabled);
}

bool AStar2D::is_point_disabled(int64_t p_id) const { return astar.is_point_disabled(p_id); }

void AStar2D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.connect_points(p_id, p_with_id, p_bidirectional);
}

void AStar2D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.disconnect_points(p_id, p_with_id, p_bidirectional);
}

bool AStar2D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	return astar.are_points_connected(p_id, p_with_id, p_bidirectional);
}

int64_t AStar2D::get_point_count() const { return astar.get_point_count(); }

int64_t AStar2D::get_point_capacity() const { return astar.get_point_capacity(); }

void AStar2D::clear() { astar.clear(); }

void AStar2D::reserve_space(int64_t p_num_nodes) { astar.reserve_space(p_num_nodes); }

int64_t AStar2D::get_closest_point(const Vector2& p_point, bool p_include_disabled) const
{
	return astar.get_closest_point(Vector3(p_point.x, p_point.y, 0), p_include_disabled);
}

Vector2 AStar2D::get_closest_position_in_segment(const Vector2& p_point) const
{
	Vector3 p = astar.get_closest_position_in_segment(Vector3(p_point.x, p_point.y, 0));
	return Vector2(p.x, p.y);
}

real_t AStar2D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** end_entry = astar.points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	AStar3D::Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar2D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector2> AStar2D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** b_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *b_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		AStar3D::Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = Vector2(p2->pos.x, p2->pos.y);
			p2 = p2->prev_point;
		}

		w[0] = Vector2(p2->pos.x, p2->pos.y); // Assign first
	}

	return path;
}

Vector<int64_t> AStar2D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!to_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *to_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar2D::_solve(
	AStar3D::Point* p_begin_point, AStar3D::Point* p_end_point, bool p_allow_partial_path)
{
	astar.last_closest_point = nullptr;
	astar.pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<AStar3D::Point*> open_list;
	SortArray<AStar3D::Point*, AStar3D::SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		AStar3D::Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (astar.last_closest_point == nullptr ||
			astar.last_closest_point->abs_f_score > p->abs_f_score ||
			(astar.last_closest_point->abs_f_score >= p->abs_f_score &&
				astar.last_closest_point->abs_g_score > p->abs_g_score)) {
			astar.last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route =
 true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = astar.pass; // Mark the point as closed.

		for (KeyValue<int64_t, AStar3D::Point*>& kv : p->neighbors) {
			AStar3D::Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == astar.pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != astar.pass) { // The point wasn't inside the open list.
				e->open_pass = astar.pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

// --- a_star.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<int64_t> AStar3D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector3> AStar3D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar3D::_bind_compatibility_methods() {}

Vector<int64_t> AStar2D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStar2D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- a_star_grid_2d.cpp ---

static real_t heuristic_euclidean(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return (real_t)Math::sqrt(dx * dx + dy * dy);
}

static real_t heuristic_manhattan(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return dx + dy;
}

static real_t heuristic_octile(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	real_t F = Math::SQRT2 - 1;
	return (dx < dy) ? F * dx + dy : F * dy + dx;
}

static real_t heuristic_chebyshev(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return MAX(dx, dy);
}

static real_t (*heuristics[AStarGrid2D::HEURISTIC_MAX])(const Vector2i&, const Vector2i&) = {
	heuristic_euclidean, heuristic_manhattan, heuristic_octile, heuristic_chebyshev};

void AStarGrid2D::set_region(const Rect2i& p_region)
{
	ERR_FAIL_COND(p_region.size.x < 0 || p_region.size.y < 0);
	if (p_region != region) {
		region = p_region;
		dirty = true;
	}
}

Rect2i AStarGrid2D::get_region() const { return region; }

void AStarGrid2D::set_size(const Size2i& p_size)
{
	WARN_DEPRECATED_MSG(R"(The "size" property is deprecated, use "region" instead.)");
	ERR_FAIL_COND(p_size.x < 0 || p_size.y < 0);
	if (p_size != region.size) {
		region.size = p_size;
		dirty = true;
	}
}

Size2i AStarGrid2D::get_size() const { return region.size; }

void AStarGrid2D::set_offset(const Vector2& p_offset)
{
	if (!offset.is_equal_approx(p_offset)) {
		offset = p_offset;
		dirty = true;
	}
}

Vector2 AStarGrid2D::get_offset() const { return offset; }

void AStarGrid2D::set_cell_size(const Size2& p_cell_size)
{
	if (!cell_size.is_equal_approx(p_cell_size)) {
		cell_size = p_cell_size;
		dirty = true;
	}
}

Size2 AStarGrid2D::get_cell_size() const { return cell_size; }

void AStarGrid2D::set_cell_shape(CellShape p_cell_shape)
{
	if (cell_shape == p_cell_shape) {
		return;
	}

	ERR_FAIL_INDEX(p_cell_shape, CellShape::CELL_SHAPE_MAX);
	cell_shape = p_cell_shape;
	dirty = true;
}

AStarGrid2D::CellShape AStarGrid2D::get_cell_shape() const { return cell_shape; }

void AStarGrid2D::update()
{
	if (!dirty) {
		return;
	}

	points.clear();
	solid_mask.clear();

	const int32_t end_x = region.get_end().x;
	const int32_t end_y = region.get_end().y;
	const Vector2 half_cell_size = cell_size / 2;
	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	for (int32_t y = region.position.y; y < end_y; y++) {
		LocalVector<Point> line;
		solid_mask.push_back(true);
		for (int32_t x = region.position.x; x < end_x; x++) {
			Vector2 v = offset;
			switch (cell_shape) {
			case CELL_SHAPE_ISOMETRIC_RIGHT:
				v += half_cell_size + Vector2(x + y, y - x) * half_cell_size;
				break;
			case CELL_SHAPE_ISOMETRIC_DOWN:
				v += half_cell_size + Vector2(x - y, x + y) * half_cell_size;
				break;
			case CELL_SHAPE_SQUARE:
				v += Vector2(x, y) * cell_size;
				break;
			default:
				break;
			}
			line.push_back(Point(Vector2i(x, y), v));
			solid_mask.push_back(false);
		}
		solid_mask.push_back(true);
		points.push_back(std::move(line));
	}

	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	dirty = false;
}

bool AStarGrid2D::is_in_bounds(int32_t p_x, int32_t p_y) const
{
	return region.has_point(Vector2i(p_x, p_y));
}

bool AStarGrid2D::is_in_boundsv(const Vector2i& p_id) const { return region.has_point(p_id); }

bool AStarGrid2D::is_dirty() const { return dirty; }

void AStarGrid2D::set_jumping_enabled(bool p_enabled) { jumping_enabled = p_enabled; }

bool AStarGrid2D::is_jumping_enabled() const { return jumping_enabled; }

void AStarGrid2D::set_diagonal_mode(DiagonalMode p_diagonal_mode)
{
	ERR_FAIL_INDEX((int)p_diagonal_mode, (int)DIAGONAL_MODE_MAX);
	diagonal_mode = p_diagonal_mode;
}

AStarGrid2D::DiagonalMode AStarGrid2D::get_diagonal_mode() const { return diagonal_mode; }

void AStarGrid2D::set_default_compute_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_compute_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_compute_heuristic() const
{
	return default_compute_heuristic;
}

void AStarGrid2D::set_default_estimate_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_estimate_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_estimate_heuristic() const
{
	return default_estimate_heuristic;
}

void AStarGrid2D::set_point_solid(const Vector2i& p_id, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set if point is disabled. Point %s out of bounds %s.", p_id, region));
	_set_solid_unchecked(p_id, p_solid);
}

bool AStarGrid2D::is_point_solid(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, false, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), false,
		vformat("Can't get if point is disabled. Point %s out of bounds %s.", p_id, region));
	return _get_solid_unchecked(p_id);
}

void AStarGrid2D::set_point_weight_scale(const Vector2i& p_id, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set point's weight scale. Point %s out of bounds %s.", p_id, region));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));
	_get_point_unchecked(p_id)->weight_scale = p_weight_scale;
}

real_t AStarGrid2D::get_point_weight_scale(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, 0, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), 0,
		vformat("Can't get point's weight scale. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->weight_scale;
}

void AStarGrid2D::fill_solid_region(const Rect2i& p_region, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_set_solid_unchecked(x, y, p_solid);
		}
	}
}

void AStarGrid2D::fill_weight_scale_region(const Rect2i& p_region, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_get_point_unchecked(x, y)->weight_scale = p_weight_scale;
		}
	}
}

AStarGrid2D::Point* AStarGrid2D::_jump(Point* p_from, Point* p_to)
{
	int32_t from_x = p_from->id.x;
	int32_t from_y = p_from->id.y;

	int32_t to_x = p_to->id.x;
	int32_t to_y = p_to->id.y;

	int32_t dx = to_x - from_x;
	int32_t dy = to_y - from_y;

	if (diagonal_mode == DIAGONAL_MODE_ALWAYS ||
		diagonal_mode == DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(to_x, to_y, dx, dy);
		}

		while (_is_walkable(to_x, to_y) &&
			   (diagonal_mode == DIAGONAL_MODE_ALWAYS || _is_walkable(to_x, to_y - dy) ||
				   _is_walkable(to_x - dx, to_y))) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - dx, to_y + dy) && !_is_walkable(to_x - dx, to_y)) ||
				(_is_walkable(to_x + dx, to_y - dy) && !_is_walkable(to_x, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x + dx, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y + dy, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else if (diagonal_mode == DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(from_x, from_y, dx, dy, true);
		}

		while (_is_walkable(to_x, to_y) && _is_walkable(to_x, to_y - dy) &&
			   _is_walkable(to_x - dx, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x + dx, to_y + dy) && !_is_walkable(to_x, to_y + dy)) ||
				!_is_walkable(to_x + dx, to_y)) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else { // DIAGONAL_MODE_NEVER
		if (dy == 0) {
			return _forced_successor(from_x, from_y, dx, 0, true);
		}

		while (_is_walkable(to_x, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - 1, to_y) && !_is_walkable(to_x - 1, to_y - dy)) ||
				(_is_walkable(to_x + 1, to_y) && !_is_walkable(to_x + 1, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, 1, 0, true) != nullptr ||
				_forced_successor(to_x, to_y, -1, 0, true) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_y += dy;
		}
	}

	return nullptr;
}

AStarGrid2D::Point* AStarGrid2D::_forced_successor(
	int32_t p_x, int32_t p_y, int32_t p_dx, int32_t p_dy, bool p_inclusive)
{
	// Remembering previous results can improve performance.
	bool l_prev = false, r_prev = false, l = false, r = false;

	int32_t o_x = p_x, o_y = p_y;
	if (p_inclusive) {
		o_x += p_dx;
		o_y += p_dy;
	}

	int32_t l_x = p_x - p_dy, l_y = p_y - p_dx;
	int32_t r_x = p_x + p_dy, r_y = p_y + p_dx;

	while (_is_walkable(o_x, o_y)) {
		if (end->id.x == o_x && end->id.y == o_y) {
			return end;
		}

		l_prev = l || _is_walkable(l_x, l_y);
		r_prev = r || _is_walkable(r_x, r_y);

		l_x += p_dx;
		l_y += p_dy;
		r_x += p_dx;
		r_y += p_dy;

		l = _is_walkable(l_x, l_y);
		r = _is_walkable(r_x, r_y);

		if ((l && !l_prev) || (r && !r_prev)) {
			return _get_point_unchecked(o_x, o_y);
		}

		o_x += p_dx;
		o_y += p_dy;
	}
	return nullptr;
}

void AStarGrid2D::_get_nbors(Point* p_point, LocalVector<Point*>& r_nbors)
{
	bool ts0 = false, td0 = false, ts1 = false, td1 = false, ts2 = false, td2 = false, ts3 = false,
		 td3 = false;

	Point* left = nullptr;
	Point* right = nullptr;
	Point* top = nullptr;
	Point* bottom = nullptr;

	Point* top_left = nullptr;
	Point* top_right = nullptr;
	Point* bottom_left = nullptr;
	Point* bottom_right = nullptr;

	{
		bool has_left = false;
		bool has_right = false;

		if (p_point->id.x - 1 >= region.position.x) {
			left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y);
			has_left = true;
		}
		if (p_point->id.x + 1 < region.position.x + region.size.width) {
			right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y);
			has_right = true;
		}
		if (p_point->id.y - 1 >= region.position.y) {
			top = _get_point_unchecked(p_point->id.x, p_point->id.y - 1);
			if (has_left) {
				top_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y - 1);
			}
			if (has_right) {
				top_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y - 1);
			}
		}
		if (p_point->id.y + 1 < region.position.y + region.size.height) {
			bottom = _get_point_unchecked(p_point->id.x, p_point->id.y + 1);
			if (has_left) {
				bottom_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y + 1);
			}
			if (has_right) {
				bottom_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y + 1);
			}
		}
	}

	if (top && !_get_solid_unchecked(top->id)) {
		r_nbors.push_back(top);
		ts0 = true;
	}
	if (right && !_get_solid_unchecked(right->id)) {
		r_nbors.push_back(right);
		ts1 = true;
	}
	if (bottom && !_get_solid_unchecked(bottom->id)) {
		r_nbors.push_back(bottom);
		ts2 = true;
	}
	if (left && !_get_solid_unchecked(left->id)) {
		r_nbors.push_back(left);
		ts3 = true;
	}

	switch (diagonal_mode) {
	case DIAGONAL_MODE_ALWAYS: {
		td0 = true;
		td1 = true;
		td2 = true;
		td3 = true;
	} break;
	case DIAGONAL_MODE_NEVER: {
	} break;
	case DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE: {
		td0 = ts3 || ts0;
		td1 = ts0 || ts1;
		td2 = ts1 || ts2;
		td3 = ts2 || ts3;
	} break;
	case DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES: {
		td0 = ts3 && ts0;
		td1 = ts0 && ts1;
		td2 = ts1 && ts2;
		td3 = ts2 && ts3;
	} break;
	default:
		break;
	}

	if (td0 && (top_left && !_get_solid_unchecked(top_left->id))) {
		r_nbors.push_back(top_left);
	}
	if (td1 && (top_right && !_get_solid_unchecked(top_right->id))) {
		r_nbors.push_back(top_right);
	}
	if (td2 && (bottom_right && !_get_solid_unchecked(bottom_right->id))) {
		r_nbors.push_back(bottom_right);
	}
	if (td3 && (bottom_left && !_get_solid_unchecked(bottom_left->id))) {
		r_nbors.push_back(bottom_left);
	}
}

bool AStarGrid2D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (_get_solid_unchecked(p_begin_point->id)) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (_get_solid_unchecked(p_end_point->id) && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;
	LocalVector<Point*> nbors;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);
	end = p_end_point;

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		nbors.clear();
		_get_nbors(p, nbors);

		for (Point* e : nbors) {
			real_t weight_scale = 1.0;

			if (jumping_enabled) {
				// TODO: Make it works with weight_scale.
				e = _jump(p, e);
				if (!e || e->closed_pass == pass) {
					continue;
				}
			}
			else {
				if (_get_solid_unchecked(e->id) || e->closed_pass == pass) {
					continue;
				}
				weight_scale = e->weight_scale;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * weight_scale;
			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);

			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStarGrid2D::_estimate_cost(const Vector2i& p_from_id, const Vector2i& p_end_id)
{
	return heuristics[default_estimate_heuristic](p_from_id, p_end_id);
}

real_t AStarGrid2D::_compute_cost(const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return heuristics[default_compute_heuristic](p_from_id, p_to_id);
}

void AStarGrid2D::clear()
{
	points.clear();
	region = Rect2i();
}

Vector2 AStarGrid2D::get_point_position(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, Vector2(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), Vector2(),
		vformat("Can't get point's position. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->pos;
}

Vector<Vector2> AStarGrid2D::get_point_path(
	const Vector2i& p_from_id, const Vector2i& p_to_id, bool p_allow_partial_path)
{
	ERR_FAIL_COND_V_MSG(
		dirty, Vector<Vector2>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_from_id, region));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_to_id, region));

	Point* begin_point = _get_point(p_from_id.x, p_from_id.y);
	Point* end_point = _get_point(p_to_id.x, p_to_id.y);

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int32_t pc = 1;
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		p = end_point;
		int32_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->pos;
			p = p->prev_point;
		}

		w[0] = p->pos;
	}

	return path;
}



Vector<Vector2i> AStarGrid2D::get_id_path(Vector2i const&, Vector2i const&, bool) {}

// --- a_star_grid_2d.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<Vector2i> AStarGrid2D::_get_id_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStarGrid2D::_get_point_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStarGrid2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from core/math/geometry_2d.cpp
// ============================================================================

#include "geometry_2d.h"

#include "core/math/math_funcs_binary.h"

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Walloc-zero")
#include <thirdparty/clipper2/include/clipper2/clipper.h>
VLTR_GCC_WARNING_POP
#include <thirdparty/misc/polypartition.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <thirdparty/misc/stb_rect_pack.h>

const int clipper_precision = 5; // Based on CMP_EPSILON.

void Geometry2D::merge_many_polygons(const Vector<Vector<Vector2>> &p_polygons, Vector<Vector<Vector2>> &r_out_polygons, Vector<Vector<Vector2>> &r_out_holes) {
	using namespace Clipper2Lib;

	PathsD subjects;
	for (const Vector<Vector2> &polygon : p_polygons) {
		PathD path(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			const Vector2 &point = polygon[i];
			path[i] = PointD(point.x, point.y);
		}
		subjects.push_back(path);
	}

	PathsD solution = Union(subjects, FillRule::NonZero);
	solution = SimplifyPaths(solution, 0.01);

	r_out_polygons.clear();
	r_out_holes.clear();
	for (PathsD::size_type i = 0; i < solution.size(); ++i) {
		PathD &path = solution[i];

		Vector<Point2> output_polygon;
		output_polygon.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			output_polygon.set(j, Vector2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		if (IsPositive(path)) {
			r_out_polygons.push_back(output_polygon);
		} else {
			r_out_holes.push_back(output_polygon);
		}
	}
}

Vector<Vector<Vector2>> Geometry2D::decompose_many_polygons_in_convex(const Vector<Vector<Point2>> &p_polygons, const Vector<Vector<Point2>> &p_holes) {
	Vector<Vector<Vector2>> decomp;
	List<TPPLPoly> in_poly, out_poly;

	for (const Vector<Vector2> &polygon : p_polygons) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CCW);
		in_poly.push_back(inp);
	}
	for (const Vector<Vector2> &polygon : p_holes) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CW);
		inp.SetHole(true);
		in_poly.push_back(inp);
	}
	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // Failed.
		ERR_PRINT("Convex decomposing failed!");
		return decomp;
	}

	decomp.resize(out_poly.size());
	int idx = 0;
	for (TPPLPoly &tp : out_poly) {
		decomp.write[idx].resize(tp.GetNumPoints());

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			decomp.write[idx].write[i] = tp.GetPoint(i);
		}

		idx++;
	}

	return decomp;
}

Vector<Vector<Vector2>> Geometry2D::decompose_polygon_in_convex(const Vector<Point2> &p_polygon) {
	return Geometry2D::decompose_many_polygons_in_convex({ p_polygon }, {});
}

struct _AtlasWorkRect {
	Size2i s;
	Point2i p;
	int idx = 0;
	_FORCE_INLINE_ bool operator<(const _AtlasWorkRect &p_r) const { return s.width > p_r.s.width; }
};

struct _AtlasWorkRectResult {
	Vector<_AtlasWorkRect> result;
	int max_w = 0;
	int max_h = 0;
};

void Geometry2D::make_atlas(const Vector<Size2i> &p_rects, Vector<Point2i> &r_result, Size2i &r_size) {
	// Super simple, almost brute force scanline stacking fitter.
	// It's pretty basic for now, but it tries to make sure that the aspect ratio of the
	// resulting atlas is somehow square. This is necessary because video cards have limits
	// on texture size (usually 2048 or 4096), so the squarer a texture, the more the chances
	// that it will work in every hardware.
	// For example, it will prioritize a 1024x1024 atlas (works everywhere) instead of a
	// 256x8192 atlas (won't work anywhere).

	ERR_FAIL_COND(p_rects.is_empty());
	for (int i = 0; i < p_rects.size(); i++) {
		ERR_FAIL_COND(p_rects[i].width <= 0);
		ERR_FAIL_COND(p_rects[i].height <= 0);
	}

	Vector<_AtlasWorkRect> wrects;
	wrects.resize(p_rects.size());
	for (int i = 0; i < p_rects.size(); i++) {
		wrects.write[i].s = p_rects[i];
		wrects.write[i].idx = i;
	}
	wrects.sort();
	int widest = wrects[0].s.width;

	Vector<_AtlasWorkRectResult> results;

	for (int i = 0; i <= 12; i++) {
		int w = 1 << i;
		int max_h = 0;
		int max_w = 0;
		if (w < widest) {
			continue;
		}

		Vector<int> hmax;
		hmax.resize(w);
		for (int j = 0; j < w; j++) {
			hmax.write[j] = 0;
		}

		// Place them.
		int ofs = 0;
		int limit_h = 0;
		for (int j = 0; j < wrects.size(); j++) {
			if (ofs + wrects[j].s.width > w) {
				ofs = 0;
			}

			int from_y = 0;
			for (int k = 0; k < wrects[j].s.width; k++) {
				if (hmax[ofs + k] > from_y) {
					from_y = hmax[ofs + k];
				}
			}

			wrects.write[j].p.x = ofs;
			wrects.write[j].p.y = from_y;
			int end_h = from_y + wrects[j].s.height;
			int end_w = ofs + wrects[j].s.width;
			if (ofs == 0) {
				limit_h = end_h;
			}

			for (int k = 0; k < wrects[j].s.width; k++) {
				hmax.write[ofs + k] = end_h;
			}

			if (end_h > max_h) {
				max_h = end_h;
			}

			if (end_w > max_w) {
				max_w = end_w;
			}

			if (ofs == 0 || end_h > limit_h) { // While h limit not reached, keep stacking.
				ofs += wrects[j].s.width;
			}
		}

		_AtlasWorkRectResult result;
		result.result = wrects;
		result.max_h = max_h;
		result.max_w = max_w;
		results.push_back(result);
	}

	// Find the result with the best aspect ratio.

	int best = -1;
	real_t best_aspect = 1e20;

	for (int i = 0; i < results.size(); i++) {
		real_t h = Math::next_power_of_2((uint32_t)results[i].max_h);
		real_t w = Math::next_power_of_2((uint32_t)results[i].max_w);
		real_t aspect = h > w ? h / w : w / h;
		if (aspect < best_aspect) {
			best = i;
			best_aspect = aspect;
		}
	}

	r_result.resize(p_rects.size());

	for (int i = 0; i < p_rects.size(); i++) {
		r_result.write[results[best].result[i].idx] = results[best].result[i].p;
	}

	r_size = Size2(results[best].max_w, results[best].max_h);
}

Vector<Vector<Point2>> Geometry2D::_polypaths_do_operation(PolyBooleanOperation p_op, const Vector<Point2> &p_polypath_a, const Vector<Point2> &p_polypath_b, bool p_is_a_open) {
	using namespace Clipper2Lib;

	ClipType op = ClipType::Union;

	switch (p_op) {
		case OPERATION_UNION:
			op = ClipType::Union;
			break;
		case OPERATION_DIFFERENCE:
			op = ClipType::Difference;
			break;
		case OPERATION_INTERSECTION:
			op = ClipType::Intersection;
			break;
		case OPERATION_XOR:
			op = ClipType::Xor;
			break;
	}

	PathD path_a(p_polypath_a.size());
	for (int i = 0; i != p_polypath_a.size(); ++i) {
		path_a[i] = PointD(p_polypath_a[i].x, p_polypath_a[i].y);
	}
	PathD path_b(p_polypath_b.size());
	for (int i = 0; i != p_polypath_b.size(); ++i) {
		path_b[i] = PointD(p_polypath_b[i].x, p_polypath_b[i].y);
	}

	ClipperD clp(clipper_precision); // Scale points up internally to attain the desired precision.
	clp.PreserveCollinear(false); // Remove redundant vertices.
	if (p_is_a_open) {
		clp.AddOpenSubject({ path_a });
	} else {
		clp.AddSubject({ path_a });
	}
	clp.AddClip({ path_b });

	PathsD paths;

	if (p_is_a_open) {
		PolyTreeD tree; // Needed to populate polylines.
		clp.Execute(op, FillRule::EvenOdd, tree, paths);
	} else {
		clp.Execute(op, FillRule::EvenOdd, paths); // Works on closed polygons only.
	}

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); i++) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath;
		polypath.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath);
	}
	return polypaths;
}

Vector<Vector<Point2>> Geometry2D::_polypath_offset(const Vector<Point2> &p_polypath, real_t p_delta, PolyJoinType p_join_type, PolyEndType p_end_type) {
	using namespace Clipper2Lib;

	JoinType jt = JoinType::Square;

	switch (p_join_type) {
		case JOIN_SQUARE:
			jt = JoinType::Square;
			break;
		case JOIN_ROUND:
			jt = JoinType::Round;
			break;
		case JOIN_MITER:
			jt = JoinType::Miter;
			break;
	}

	EndType et = EndType::Polygon;

	switch (p_end_type) {
		case END_POLYGON:
			et = EndType::Polygon;
			break;
		case END_JOINED:
			et = EndType::Joined;
			break;
		case END_BUTT:
			et = EndType::Butt;
			break;
		case END_SQUARE:
			et = EndType::Square;
			break;
		case END_ROUND:
			et = EndType::Round;
			break;
	}

	PathD polypath(p_polypath.size());
	for (int i = 0; i != p_polypath.size(); ++i) {
		polypath[i] = PointD(p_polypath[i].x, p_polypath[i].y);
	}

	// Inflate/deflate.
	PathsD paths = InflatePaths({ polypath }, p_delta, jt, et, 2.0, clipper_precision, 0.25);

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); ++i) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath2;
		polypath2.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath2.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath2);
	}
	return polypaths;
}

Vector<Vector3i> Geometry2D::partial_pack_rects(const Vector<Vector2i> &p_sizes, const Size2i &p_atlas_size) {
	Vector<stbrp_node> nodes;
	nodes.resize(p_atlas_size.width);
	memset(nodes.ptrw(), 0, sizeof(stbrp_node) * nodes.size());

	stbrp_context context;
	stbrp_init_target(&context, p_atlas_size.width, p_atlas_size.height, nodes.ptrw(), p_atlas_size.width);

	Vector<stbrp_rect> rects;
	rects.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		rects.write[i].id = i;
		rects.write[i].w = p_sizes[i].width;
		rects.write[i].h = p_sizes[i].height;
		rects.write[i].x = 0;
		rects.write[i].y = 0;
		rects.write[i].was_packed = 0;
	}

	stbrp_pack_rects(&context, rects.ptrw(), rects.size());

	Vector<Vector3i> ret;
	ret.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		ret.write[rects[i].id] = Vector3i(rects[i].x, rects[i].y, rects[i].was_packed != 0 ? 1 : 0);
	}

	return ret;
}

// --- triangulate.cpp ---

real_t Triangulate::get_area(const Vector<Vector2> &p_contour) {
	int n = p_contour.size();
	const Vector2 *c = &p_contour[0];

	real_t A = 0.0;

	for (int p = n - 1, q = 0; q < n; p = q++) {
		A += c[p].cross(c[q]);
	}
	return A * 0.5f;
}

/* `is_inside_triangle` decides if a point P is inside the triangle
 * defined by A, B, C. */
bool Triangulate::is_inside_triangle(real_t p_ax, real_t p_ay, real_t p_bx, real_t p_by, real_t p_cx, real_t p_cy, real_t p_px, real_t p_py, bool p_include_edges) {
	real_t ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
	real_t cCROSSap, bCROSScp, aCROSSbp;

	ax = p_cx - p_bx;
	ay = p_cy - p_by;
	bx = p_ax - p_cx;
	by = p_ay - p_cy;
	cx = p_bx - p_ax;
	cy = p_by - p_ay;
	apx = p_px - p_ax;
	apy = p_py - p_ay;
	bpx = p_px - p_bx;
	bpy = p_py - p_by;
	cpx = p_px - p_cx;
	cpy = p_py - p_cy;

	aCROSSbp = ax * bpy - ay * bpx;
	cCROSSap = cx * apy - cy * apx;
	bCROSScp = bx * cpy - by * cpx;

	if (p_include_edges) {
		return ((aCROSSbp > 0.0f) && (bCROSScp > 0.0f) && (cCROSSap > 0.0f));
	} else {
		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
}

bool Triangulate::snip(const Vector<Vector2> &p_contour, int p_u, int p_v, int p_w, int p_n, const Vector<int> &p_values, bool p_relaxed) {
	int p;
	real_t Ax, Ay, Bx, By, Cx, Cy, Px, Py;
	const Vector2 *contour = &p_contour[0];

	Ax = contour[p_values[p_u]].x;
	Ay = contour[p_values[p_u]].y;

	Bx = contour[p_values[p_v]].x;
	By = contour[p_values[p_v]].y;

	Cx = contour[p_values[p_w]].x;
	Cy = contour[p_values[p_w]].y;

	// It can happen that the triangulation ends up with three aligned vertices to deal with.
	// In this scenario, making the check below strict may reject the possibility of
	// forming a last triangle with these aligned vertices, preventing the triangulation
	// from completing.
	// To avoid that we allow zero-area triangles if all else failed.
	float threshold = p_relaxed ? -CMP_EPSILON : CMP_EPSILON;

	if (threshold > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax)))) {
		return false;
	}

	for (p = 0; p < p_n; p++) {
		if ((p == p_u) || (p == p_v) || (p == p_w)) {
			continue;
		}
		Px = contour[p_values[p]].x;
		Py = contour[p_values[p]].y;
		if (is_inside_triangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py, p_relaxed)) {
			return false;
		}
	}

	return true;
}

bool Triangulate::triangulate(const Vector<Vector2> &p_contour, Vector<int> &r_result) {
	/* allocate and initialize list of Vertices in polygon */

	int n = p_contour.size();
	if (n < 3) {
		return false;
	}

	Vector<int> V;
	V.resize(n);

	/* we want a counter-clockwise polygon in V */

	if (0.0f < get_area(p_contour)) {
		for (int v = 0; v < n; v++) {
			V.write[v] = v;
		}
	} else {
		for (int v = 0; v < n; v++) {
			V.write[v] = (n - 1) - v;
		}
	}

	bool relaxed = false;

	int nv = n;

	/*  remove nv-2 Vertices, creating 1 triangle every time */
	int count = 2 * nv; /* error detection */

	for (int v = nv - 1; nv > 2;) {
		/* if we loop, it is probably a non-simple polygon */
		if (0 >= (count--)) {
			if (relaxed) {
				//** Triangulate: ERROR - probable bad polygon!
				return false;
			} else {
				// There may be aligned vertices that the strict
				// checks prevent from triangulating. In this situation
				// we are better off adding flat triangles than
				// failing, so we relax the checks and try one last
				// round.
				// Only relaxing the constraints as a last resort avoids
				// degenerate triangles when they aren't necessary.
				count = 2 * nv;
				relaxed = true;
			}
		}

		/* three consecutive vertices in current polygon, <u,v,w> */
		int u = v;
		if (nv <= u) {
			u = 0; /* previous */
		}
		v = u + 1;
		if (nv <= v) {
			v = 0; /* new v    */
		}
		int w = v + 1;
		if (nv <= w) {
			w = 0; /* next     */
		}

		if (snip(p_contour, u, v, w, nv, V, relaxed)) {
			int a, b, c, s, t;

			/* true names of the vertices */
			a = V[u];
			b = V[v];
			c = V[w];

			/* output Triangle */
			r_result.push_back(a);
			r_result.push_back(b);
			r_result.push_back(c);

			/* remove v from remaining polygon */
			for (s = v, t = v + 1; t < nv; s++, t++) {
				V.write[s] = V[t];
			}

			nv--;

			/* reset error detection counter */
			count = 2 * nv;
		}
	}

	return true;
}


// --- Merged from core/math/convex_hull.cpp ---
#include "core/math/geometry_3d.h"

#include "core/error/error_macros.h"
#include "core/math/aabb.h"
#include "core/math/math_defs.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/paged_allocator.h"

#include <cfloat> // FLT_MAX

//#define DEBUG_CONVEX_HULL
//#define SHOW_ITERATIONS

// -- GODOT start --
// Assembly optimizations are not used at the moment.
//#define USE_X86_64_ASM
// -- GODOT end --

#ifdef DEBUG_ENABLED
#define CHULL_ASSERT(m_cond) \
	if constexpr (true) { \
		if (unlikely(!(m_cond))) { \
			ERR_PRINT("Assertion \"" _STR(m_cond) "\" failed."); \
		} \
	} else \
		((void)0)
#else
#define CHULL_ASSERT(m_cond) \
	if constexpr (true) { \
	} else \
		((void)0)
#endif

#if defined(DEBUG_CONVEX_HULL) || defined(SHOW_ITERATIONS)
#include <cstdio>
#endif

// Convex hull implementation based on Preparata and Hong
// Ole Kniemeyer, MAXON Computer GmbH
class ConvexHullInternal {
public:
	class Point64 {
	public:
		int64_t x;
		int64_t y;
		int64_t z;

		Point64(int64_t p_x, int64_t p_y, int64_t p_z) {
			x = p_x;
			y = p_y;
			z = p_z;
		}

		bool is_zero() {
			return (x == 0) && (y == 0) && (z == 0);
		}

		int64_t dot(const Point64 &p_other) const {
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}
	};

	class Point32 {
	public:
		int32_t x = 0;
		int32_t y = 0;
		int32_t z = 0;
		int32_t index = -1;

		Point32() {
		}

		Point32(int32_t p_x, int32_t p_y, int32_t p_z) {
			x = p_x;
			y = p_y;
			z = p_z;
		}

		bool operator==(const Point32 &p_other) const {
			return (x == p_other.x) && (y == p_other.y) && (z == p_other.z);
		}

		bool operator!=(const Point32 &p_other) const {
			return (x != p_other.x) || (y != p_other.y) || (z != p_other.z);
		}

		bool is_zero() {
			return (x == 0) && (y == 0) && (z == 0);
		}

		Point64 cross(const Point32 &p_other) const {
			return Point64((int64_t)y * p_other.z - (int64_t)z * p_other.y, (int64_t)z * p_other.x - (int64_t)x * p_other.z, (int64_t)x * p_other.y - (int64_t)y * p_other.x);
		}

		Point64 cross(const Point64 &p_other) const {
			return Point64(y * p_other.z - z * p_other.y, z * p_other.x - x * p_other.z, x * p_other.y - y * p_other.x);
		}

		int64_t dot(const Point32 &p_other) const {
			return (int64_t)x * p_other.x + (int64_t)y * p_other.y + (int64_t)z * p_other.z;
		}

		int64_t dot(const Point64 &p_other) const {
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}

		Point32 operator+(const Point32 &p_other) const {
			return Point32(x + p_other.x, y + p_other.y, z + p_other.z);
		}

		Point32 operator-(const Point32 &p_other) const {
			return Point32(x - p_other.x, y - p_other.y, z - p_other.z);
		}
	};

	class Int128 {
	public:
		uint64_t low = 0;
		uint64_t high = 0;

		Int128() {
		}

		Int128(uint64_t p_low, uint64_t p_high) {
			low = p_low;
			high = p_high;
		}

		Int128(uint64_t p_low) {
			low = p_low;
			high = 0;
		}

		Int128(int64_t p_value) {
			low = p_value;
			if (p_value >= 0) {
				high = 0;
			} else {
				high = (uint64_t)-1LL;
			}
		}

		static Int128 mul(int64_t p_left, int64_t p_right);

		static Int128 mul(uint64_t p_left, uint64_t p_right);

		Int128 operator-() const {
			return Int128(uint64_t(-int64_t(low)), ~high + (low == 0));
		}

		Int128 operator+(const Int128 &p_other) const {
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__("addq %[bl], %[rl]\n\t"
					"adcq %[bh], %[rh]\n\t"
					: [rl] "=r"(result.low), [rh] "=r"(result.high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
			return result;
#else
			uint64_t lo = low + p_other.low;
			return Int128(lo, high + p_other.high + (lo < low));
#endif
		}

		Int128 operator-(const Int128 &p_other) const {
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__("subq %[bl], %[rl]\n\t"
					"sbbq %[bh], %[rh]\n\t"
					: [rl] "=r"(result.low), [rh] "=r"(result.high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
			return result;
#else
			return *this + -p_other;
#endif
		}

		Int128 &operator+=(const Int128 &p_other) {
#ifdef USE_X86_64_ASM
			__asm__("addq %[bl], %[rl]\n\t"
					"adcq %[bh], %[rh]\n\t"
					: [rl] "=r"(low), [rh] "=r"(high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
#else
			uint64_t lo = low + p_other.low;
			if (lo < low) {
				++high;
			}
			low = lo;
			high += p_other.high;
#endif
			return *this;
		}

		Int128 &operator++() {
			if (++low == 0) {
				++high;
			}
			return *this;
		}

		Int128 operator*(int64_t p_other) const;

		real_t to_scalar() const {
			return ((int64_t)high >= 0) ? real_t(high) * (real_t(0x100000000LL) * real_t(0x100000000LL)) + real_t(low) : -(-*this).to_scalar();
		}

		int32_t get_sign() const {
			return ((int64_t)high < 0) ? -1 : ((high || low) ? 1 : 0);
		}

		bool operator<(const Int128 &p_other) const {
			return (high < p_other.high) || ((high == p_other.high) && (low < p_other.low));
		}

		int32_t ucmp(const Int128 &p_other) const {
			if (high < p_other.high) {
				return -1;
			}
			if (high > p_other.high) {
				return 1;
			}
			if (low < p_other.low) {
				return -1;
			}
			if (low > p_other.low) {
				return 1;
			}
			return 0;
		}
	};

	class Rational64 {
	private:
		uint64_t numerator;
		uint64_t denominator;
		int32_t sign;

	public:
		Rational64(int64_t p_numerator, int64_t p_denominator) {
			if (p_numerator > 0) {
				sign = 1;
				numerator = (uint64_t)p_numerator;
			} else if (p_numerator < 0) {
				sign = -1;
				numerator = (uint64_t)-p_numerator;
			} else {
				sign = 0;
				numerator = 0;
			}
			if (p_denominator > 0) {
				denominator = (uint64_t)p_denominator;
			} else if (p_denominator < 0) {
				sign = -sign;
				denominator = (uint64_t)-p_denominator;
			} else {
				denominator = 0;
			}
		}

		bool is_negative_infinity() const {
			return (sign < 0) && (denominator == 0);
		}

		bool is_nan() const {
			return (sign == 0) && (denominator == 0);
		}

		int32_t compare(const Rational64 &p_other) const;

		real_t to_scalar() const {
			return sign * ((denominator == 0) ? FLT_MAX : (real_t)numerator / denominator);
		}
	};

	class Rational128 {
	private:
		Int128 numerator;
		Int128 denominator;
		int32_t sign;
		bool is_int_64;

	public:
		Rational128(int64_t p_value) {
			if (p_value > 0) {
				sign = 1;
				numerator = p_value;
			} else if (p_value < 0) {
				sign = -1;
				numerator = -p_value;
			} else {
				sign = 0;
				numerator = (uint64_t)0;
			}
			denominator = (uint64_t)1;
			is_int_64 = true;
		}

		Rational128(const Int128 &p_numerator, const Int128 &p_denominator) {
			sign = p_numerator.get_sign();
			if (sign >= 0) {
				numerator = p_numerator;
			} else {
				numerator = -p_numerator;
			}
			int32_t dsign = p_denominator.get_sign();
			if (dsign >= 0) {
				denominator = p_denominator;
			} else {
				sign = -sign;
				denominator = -p_denominator;
			}
			is_int_64 = false;
		}

		int32_t compare(const Rational128 &p_other) const;

		int32_t compare(int64_t p_int64) const;

		real_t to_scalar() const {
			return sign * ((denominator.get_sign() == 0) ? FLT_MAX : numerator.to_scalar() / denominator.to_scalar());
		}
	};

	class PointR128 {
	public:
		Int128 x;
		Int128 y;
		Int128 z;
		Int128 denominator;

		PointR128() {
		}

		PointR128(Int128 p_x, Int128 p_y, Int128 p_z, Int128 p_denominator) {
			x = p_x;
			y = p_y;
			z = p_z;
			denominator = p_denominator;
		}

		real_t xvalue() const {
			return x.to_scalar() / denominator.to_scalar();
		}

		real_t yvalue() const {
			return y.to_scalar() / denominator.to_scalar();
		}

		real_t zvalue() const {
			return z.to_scalar() / denominator.to_scalar();
		}
	};

	class Edge;
	class Face;

	class Vertex {
	public:
		Vertex *next = nullptr;
		Vertex *prev = nullptr;
		Edge *edges = nullptr;
		Face *first_nearby_face = nullptr;
		Face *last_nearby_face = nullptr;
		PointR128 point128;
		Point32 point;
		int32_t copy = -1;

		Vertex() {
		}

#ifdef DEBUG_CONVEX_HULL
		void print() {
			printf("V%d (%d, %d, %d)", point.index, point.x, point.y, point.z);
		}

		void print_graph();
#endif

		Point32 operator-(const Vertex &p_other) const {
			return point - p_other.point;
		}

		Rational128 dot(const Point64 &p_other) const {
			return (point.index >= 0) ? Rational128(point.dot(p_other)) : Rational128(point128.x * p_other.x + point128.y * p_other.y + point128.z * p_other.z, point128.denominator);
		}

		real_t xvalue() const {
			return (point.index >= 0) ? real_t(point.x) : point128.xvalue();
		}

		real_t yvalue() const {
			return (point.index >= 0) ? real_t(point.y) : point128.yvalue();
		}

		real_t zvalue() const {
			return (point.index >= 0) ? real_t(point.z) : point128.zvalue();
		}

		void receive_nearby_faces(Vertex *p_src) {
			if (last_nearby_face) {
				last_nearby_face->next_with_same_nearby_vertex = p_src->first_nearby_face;
			} else {
				first_nearby_face = p_src->first_nearby_face;
			}
			if (p_src->last_nearby_face) {
				last_nearby_face = p_src->last_nearby_face;
			}
			for (Face *f = p_src->first_nearby_face; f; f = f->next_with_same_nearby_vertex) {
				CHULL_ASSERT(f->nearby_vertex == p_src);
				f->nearby_vertex = this;
			}
			p_src->first_nearby_face = nullptr;
			p_src->last_nearby_face = nullptr;
		}
	};

	class Edge {
	public:
		Edge *next = nullptr;
		Edge *prev = nullptr;
		Edge *reverse = nullptr;
		Vertex *target = nullptr;
		Face *face = nullptr;
		int32_t copy = -1;

		void link(Edge *p_edge) {
			CHULL_ASSERT(reverse->target == p_edge->reverse->target);
			next = p_edge;
			p_edge->prev = this;
		}

#ifdef DEBUG_CONVEX_HULL
		void print() {
			printf("E%p : %d -> %d,  n=%p p=%p   (0 %d\t%d\t%d) -> (%d %d %d)", this, reverse->target->point.index, target->point.index, next, prev,
					reverse->target->point.x, reverse->target->point.y, reverse->target->point.z, target->point.x, target->point.y, target->point.z);
		}
#endif
	};

	class Face {
	public:
		Face *next = nullptr;
		Vertex *nearby_vertex = nullptr;
		Face *next_with_same_nearby_vertex = nullptr;
		Point32 origin;
		Point32 dir0;
		Point32 dir1;

		Face() {
		}

		void init(Vertex *p_a, const Vertex *p_b, const Vertex *p_c) {
			nearby_vertex = p_a;
			origin = p_a->point;
			dir0 = *p_b - *p_a;
			dir1 = *p_c - *p_a;
			if (p_a->last_nearby_face) {
				p_a->last_nearby_face->next_with_same_nearby_vertex = this;
			} else {
				p_a->first_nearby_face = this;
			}
			p_a->last_nearby_face = this;
		}

		Point64 get_normal() {
			return dir0.cross(dir1);
		}
	};

	template <typename UWord, typename UHWord>
	class DMul {
	private:
		static uint32_t high(uint64_t p_value) {
			return (uint32_t)(p_value >> 32);
		}

		static uint32_t low(uint64_t p_value) {
			return (uint32_t)p_value;
		}

		static uint64_t mul(uint32_t p_left, uint32_t p_right) {
			return (uint64_t)p_left * (uint64_t)p_right;
		}

		static void shl_half(uint64_t &p_value) {
			p_value <<= 32;
		}

		static uint64_t high(Int128 p_value) {
			return p_value.high;
		}

		static uint64_t low(Int128 p_value) {
			return p_value.low;
		}

		static Int128 mul(uint64_t p_left, uint64_t p_right) {
			return Int128::mul(p_left, p_right);
		}

		static void shl_half(Int128 &p_value) {
			p_value.high = p_value.low;
			p_value.low = 0;
		}

	public:
		static void mul(UWord p_a, UWord p_b, UWord &r_low, UWord &r_high) {
			UWord p00 = mul(low(p_a), low(p_b));
			UWord p01 = mul(low(p_a), high(p_b));
			UWord p10 = mul(high(p_a), low(p_b));
			UWord p11 = mul(high(p_a), high(p_b));
			UWord p0110 = UWord(low(p01)) + UWord(low(p10));
			p11 += high(p01);
			p11 += high(p10);
			p11 += high(p0110);
			shl_half(p0110);
			p00 += p0110;
			if (p00 < p0110) {
				++p11;
			}
			r_low = p00;
			r_high = p11;
		}
	};

private:
	class IntermediateHull {
	public:
		Vertex *min_xy = nullptr;
		Vertex *max_xy = nullptr;
		Vertex *min_yx = nullptr;
		Vertex *max_yx = nullptr;

		IntermediateHull() {
		}
	};

	enum Orientation { ORIENTATION_NONE,
		ORIENTATION_CLOCKWISE,
		ORIENTATION_COUNTER_CLOCKWISE };

	Vector3 scaling;
	Vector3 center;
	PagedAllocator<Vertex> vertex_pool;
	PagedAllocator<Edge> edge_pool;
	PagedAllocator<Face> face_pool;
	LocalVector<Vertex *> original_vertices;
	int32_t merge_stamp = 0;
	Vector3::Axis min_axis = Vector3::Axis::AXIS_X;
	Vector3::Axis med_axis = Vector3::Axis::AXIS_X;
	Vector3::Axis max_axis = Vector3::Axis::AXIS_X;
	int32_t used_edge_pairs = 0;
	int32_t max_used_edge_pairs = 0;

	static Orientation get_orientation(const Edge *p_prev, const Edge *p_next, const Point32 &p_s, const Point32 &p_t);
	Edge *find_max_angle(bool p_ccw, const Vertex *p_start, const Point32 &p_s, const Point64 &p_rxs, const Point64 &p_ssxrxs, Rational64 &p_min_cot);
	void find_edge_for_coplanar_faces(Vertex *p_c0, Vertex *p_c1, Edge *&p_e0, Edge *&p_e1, const Vertex *p_stop0, const Vertex *p_stop1);

	Edge *new_edge_pair(Vertex *p_from, Vertex *p_to);

	void remove_edge_pair(Edge *p_edge) {
		Edge *n = p_edge->next;
		Edge *r = p_edge->reverse;

		CHULL_ASSERT(p_edge->target && r->target);

		if (n != p_edge) {
			n->prev = p_edge->prev;
			p_edge->prev->next = n;
			r->target->edges = n;
		} else {
			r->target->edges = nullptr;
		}

		n = r->next;

		if (n != r) {
			n->prev = r->prev;
			r->prev->next = n;
			p_edge->target->edges = n;
		} else {
			p_edge->target->edges = nullptr;
		}

		edge_pool.free(p_edge);
		edge_pool.free(r);
		used_edge_pairs--;
	}

	void compute_internal(int32_t p_start, int32_t p_end, IntermediateHull &r_result);

	bool merge_projection(IntermediateHull &p_h0, IntermediateHull &p_h1, Vertex *&r_c0, Vertex *&r_c1);

	void merge(IntermediateHull &p_h0, IntermediateHull &p_h1);

	Vector3 to_gd_vector(const Point32 &p_v);

	Vector3 get_gd_normal(Face *p_face);

	bool shift_face(Face *p_face, real_t p_amount, LocalVector<Vertex *> &p_stack);

public:
	~ConvexHullInternal() {
		vertex_pool.reset(true);
		edge_pool.reset(true);
		face_pool.reset(true);
	}

	Vertex *vertex_list = nullptr;

	void compute(const Vector3 *p_coords, int32_t p_count);

	Vector3 get_coordinates(const Vertex *p_v);

	real_t shrink(real_t p_amount, real_t p_clamp_amount);
};

ConvexHullInternal::Int128 ConvexHullInternal::Int128::operator*(int64_t p_other) const {
	bool negative = (int64_t)high < 0;
	Int128 a = negative ? -*this : *this;
	if (p_other < 0) {
		negative = !negative;
		p_other = -p_other;
	}
	Int128 result = mul(a.low, (uint64_t)p_other);
	result.high += a.high * (uint64_t)p_other;
	return negative ? -result : result;
}

ConvexHullInternal::Int128 ConvexHullInternal::Int128::mul(int64_t p_left, int64_t p_right) {
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("imulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(p_left), [p_right] "r"(p_right)
			: "cc");
	return result;

#else
	bool negative = p_left < 0;
	if (negative) {
		p_left = -p_left;
	}
	if (p_right < 0) {
		negative = !negative;
		p_right = -p_right;
	}
	DMul<uint64_t, uint32_t>::mul((uint64_t)p_left, (uint64_t)p_right, result.low, result.high);
	return negative ? -result : result;
#endif
}

ConvexHullInternal::Int128 ConvexHullInternal::Int128::mul(uint64_t p_left, uint64_t p_right) {
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("mulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(p_left), [p_right] "r"(p_right)
			: "cc");

#else
	DMul<uint64_t, uint32_t>::mul(p_left, p_right, result.low, result.high);
#endif

	return result;
}

int32_t ConvexHullInternal::Rational64::compare(const Rational64 &p_other) const {
	if (sign != p_other.sign) {
		return sign - p_other.sign;
	} else if (sign == 0) {
		return 0;
	}

#ifdef USE_X86_64_ASM

	int32_t result;
	int64_t tmp;
	int64_t dummy;
	__asm__("mulq %[bn]\n\t"
			"movq %%rax, %[tmp]\n\t"
			"movq %%rdx, %%rbx\n\t"
			"movq %[tn], %%rax\n\t"
			"mulq %[bd]\n\t"
			"subq %[tmp], %%rax\n\t"
			"sbbq %%rbx, %%rdx\n\t" // rdx:rax contains 128-bit-difference "numerator*b.denominator - b.numerator*denominator"
			"setnsb %%bh\n\t" // bh=1 if difference is non-negative, bh=0 otherwise
			"orq %%rdx, %%rax\n\t"
			"setnzb %%bl\n\t" // bl=1 if difference if non-zero, bl=0 if it is zero
			"decb %%bh\n\t" // now bx=0x0000 if difference is zero, 0xff01 if it is negative, 0x0001 if it is positive (i.e., same sign as difference)
			"shll $16, %%ebx\n\t" // ebx has same sign as difference
			: "=&b"(result), [tmp] "=&r"(tmp), "=a"(dummy)
			: "a"(denominator), [bn] "g"(p_other.numerator), [tn] "g"(numerator), [bd] "g"(p_other.denominator)
			: "%rdx", "cc");
	// if sign is +1, only bit 0 of result is inverted, which does not change the sign of result (and cannot result in zero)
	// if sign is -1, all bits of result are inverted, which changes the sign of result (and again cannot result in zero)
	return result ? result ^ sign : 0;

#else

	return sign * Int128::mul(numerator, p_other.denominator).ucmp(Int128::mul(denominator, p_other.numerator));

#endif
}

int32_t ConvexHullInternal::Rational128::compare(const Rational128 &p_other) const {
	if (sign != p_other.sign) {
		return sign - p_other.sign;
	} else if (sign == 0) {
		return 0;
	}
	if (is_int_64) {
		return -p_other.compare(sign * (int64_t)numerator.low);
	}

	Int128 nbd_low, nbd_high, dbn_low, dbn_high;
	DMul<Int128, uint64_t>::mul(numerator, p_other.denominator, nbd_low, nbd_high);
	DMul<Int128, uint64_t>::mul(denominator, p_other.numerator, dbn_low, dbn_high);

	int32_t cmp = nbd_high.ucmp(dbn_high);
	if (cmp) {
		return cmp * sign;
	}
	return nbd_low.ucmp(dbn_low) * sign;
}

int32_t ConvexHullInternal::Rational128::compare(int64_t p_int64) const {
	if (is_int_64) {
		int64_t a = sign * (int64_t)numerator.low;
		return (a > p_int64) ? 1 : ((a < p_int64) ? -1 : 0);
	}
	if (p_int64 > 0) {
		if (sign <= 0) {
			return -1;
		}
	} else if (p_int64 < 0) {
		if (sign >= 0) {
			return 1;
		}
		p_int64 = -p_int64;
	} else {
		return sign;
	}

	return numerator.ucmp(denominator * p_int64) * sign;
}

ConvexHullInternal::Edge *ConvexHullInternal::new_edge_pair(Vertex *p_from, Vertex *p_to) {
	CHULL_ASSERT(p_from && p_to);
	Edge *e = edge_pool.alloc();
	Edge *r = edge_pool.alloc();
	e->reverse = r;
	r->reverse = e;
	e->copy = merge_stamp;
	r->copy = merge_stamp;
	e->target = p_to;
	r->target = p_from;
	e->face = nullptr;
	r->face = nullptr;
	used_edge_pairs++;
	if (used_edge_pairs > max_used_edge_pairs) {
		max_used_edge_pairs = used_edge_pairs;
	}
	return e;
}

bool ConvexHullInternal::merge_projection(IntermediateHull &r_h0, IntermediateHull &r_h1, Vertex *&r_c0, Vertex *&r_c1) {
	Vertex *v0 = r_h0.max_yx;
	Vertex *v1 = r_h1.min_yx;
	if ((v0->point.x == v1->point.x) && (v0->point.y == v1->point.y)) {
		CHULL_ASSERT(v0->point.z < v1->point.z);
		Vertex *v1p = v1->prev;
		if (v1p == v1) {
			r_c0 = v0;
			if (v1->edges) {
				CHULL_ASSERT(v1->edges->next == v1->edges);
				v1 = v1->edges->target;
				CHULL_ASSERT(v1->edges->next == v1->edges);
			}
			r_c1 = v1;
			return false;
		}
		Vertex *v1n = v1->next;
		v1p->next = v1n;
		v1n->prev = v1p;
		if (v1 == r_h1.min_xy) {
			if ((v1n->point.x < v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y < v1p->point.y))) {
				r_h1.min_xy = v1n;
			} else {
				r_h1.min_xy = v1p;
			}
		}
		if (v1 == r_h1.max_xy) {
			if ((v1n->point.x > v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y > v1p->point.y))) {
				r_h1.max_xy = v1n;
			} else {
				r_h1.max_xy = v1p;
			}
		}
	}

	v0 = r_h0.max_xy;
	v1 = r_h1.max_xy;
	Vertex *v00 = nullptr;
	Vertex *v10 = nullptr;
	int32_t sign = 1;

	for (int32_t side = 0; side <= 1; side++) {
		int32_t dx = (v1->point.x - v0->point.x) * sign;
		if (dx > 0) {
			while (true) {
				int32_t dy = v1->point.y - v0->point.y;

				Vertex *w0 = side ? v0->next : v0->prev;
				if (w0 != v0) {
					int32_t dx0 = (w0->point.x - v0->point.x) * sign;
					int32_t dy0 = w0->point.y - v0->point.y;
					if ((dy0 <= 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx <= dy * dx0)))) {
						v0 = w0;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex *w1 = side ? v1->next : v1->prev;
				if (w1 != v1) {
					int32_t dx1 = (w1->point.x - v1->point.x) * sign;
					int32_t dy1 = w1->point.y - v1->point.y;
					int32_t dxn = (w1->point.x - v0->point.x) * sign;
					if ((dxn > 0) && (dy1 < 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx < dy * dx1)))) {
						v1 = w1;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		} else if (dx < 0) {
			while (true) {
				int32_t dy = v1->point.y - v0->point.y;

				Vertex *w1 = side ? v1->prev : v1->next;
				if (w1 != v1) {
					int32_t dx1 = (w1->point.x - v1->point.x) * sign;
					int32_t dy1 = w1->point.y - v1->point.y;
					if ((dy1 >= 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx <= dy * dx1)))) {
						v1 = w1;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex *w0 = side ? v0->prev : v0->next;
				if (w0 != v0) {
					int32_t dx0 = (w0->point.x - v0->point.x) * sign;
					int32_t dy0 = w0->point.y - v0->point.y;
					int32_t dxn = (v1->point.x - w0->point.x) * sign;
					if ((dxn < 0) && (dy0 > 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx < dy * dx0)))) {
						v0 = w0;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		} else {
			int32_t x = v0->point.x;
			int32_t y0 = v0->point.y;
			Vertex *w0 = v0;
			Vertex *t;
			while (((t = side ? w0->next : w0->prev) != v0) && (t->point.x == x) && (t->point.y <= y0)) {
				w0 = t;
				y0 = t->point.y;
			}
			v0 = w0;

			int32_t y1 = v1->point.y;
			Vertex *w1 = v1;
			while (((t = side ? w1->prev : w1->next) != v1) && (t->point.x == x) && (t->point.y >= y1)) {
				w1 = t;
				y1 = t->point.y;
			}
			v1 = w1;
		}

		if (side == 0) {
			v00 = v0;
			v10 = v1;

			v0 = r_h0.min_xy;
			v1 = r_h1.min_xy;
			sign = -1;
		}
	}

	v0->prev = v1;
	v1->next = v0;

	v00->next = v10;
	v10->prev = v00;

	if (r_h1.min_xy->point.x < r_h0.min_xy->point.x) {
		r_h0.min_xy = r_h1.min_xy;
	}
	if (r_h1.max_xy->point.x >= r_h0.max_xy->point.x) {
		r_h0.max_xy = r_h1.max_xy;
	}

	r_h0.max_yx = r_h1.max_yx;

	r_c0 = v00;
	r_c1 = v10;

	return true;
}

void ConvexHullInternal::compute_internal(int32_t p_start, int32_t p_end, IntermediateHull &r_result) {
	int32_t n = p_end - p_start;
	switch (n) {
		case 0:
			r_result.min_xy = nullptr;
			r_result.max_xy = nullptr;
			r_result.min_yx = nullptr;
			r_result.max_yx = nullptr;
			return;
		case 2: {
			Vertex *v = original_vertices[p_start];
			Vertex *w = original_vertices[p_start + 1];
			if (v->point != w->point) {
				int32_t dx = v->point.x - w->point.x;
				int32_t dy = v->point.y - w->point.y;

				if ((dx == 0) && (dy == 0)) {
					if (v->point.z > w->point.z) {
						Vertex *t = w;
						w = v;
						v = t;
					}
					CHULL_ASSERT(v->point.z < w->point.z);
					v->next = v;
					v->prev = v;
					r_result.min_xy = v;
					r_result.max_xy = v;
					r_result.min_yx = v;
					r_result.max_yx = v;
				} else {
					v->next = w;
					v->prev = w;
					w->next = v;
					w->prev = v;

					if ((dx < 0) || ((dx == 0) && (dy < 0))) {
						r_result.min_xy = v;
						r_result.max_xy = w;
					} else {
						r_result.min_xy = w;
						r_result.max_xy = v;
					}

					if ((dy < 0) || ((dy == 0) && (dx < 0))) {
						r_result.min_yx = v;
						r_result.max_yx = w;
					} else {
						r_result.min_yx = w;
						r_result.max_yx = v;
					}
				}

				Edge *e = new_edge_pair(v, w);
				e->link(e);
				v->edges = e;

				e = e->reverse;
				e->link(e);
				w->edges = e;

				return;
			}
			[[fallthrough]];
		}
		case 1: {
			Vertex *v = original_vertices[p_start];
			v->edges = nullptr;
			v->next = v;
			v->prev = v;

			r_result.min_xy = v;
			r_result.max_xy = v;
			r_result.min_yx = v;
			r_result.max_yx = v;

			return;
		}
	}

	int32_t split0 = p_start + n / 2;
	Point32 p = original_vertices[split0 - 1]->point;
	int32_t split1 = split0;
	while ((split1 < p_end) && (original_vertices[split1]->point == p)) {
		split1++;
	}
	compute_internal(p_start, split0, r_result);
	IntermediateHull hull1;
	compute_internal(split1, p_end, hull1);
#ifdef DEBUG_CONVEX_HULL
	printf("\n\nMerge\n");
	r_result.print();
	hull1.print();
#endif
	merge(r_result, hull1);
#ifdef DEBUG_CONVEX_HULL
	printf("\n  Result\n");
	r_result.print();
#endif
}

#ifdef DEBUG_CONVEX_HULL
void ConvexHullInternal::IntermediateHull::print() {
	printf("    Hull\n");
	for (Vertex *v = min_xy; v;) {
		printf("      ");
		v->print();
		if (v == max_xy) {
			printf(" max_xy");
		}
		if (v == min_yx) {
			printf(" min_yx");
		}
		if (v == max_yx) {
			printf(" max_yx");
		}
		if (v->next->prev != v) {
			printf(" Inconsistency");
		}
		printf("\n");
		v = v->next;
		if (v == min_xy) {
			break;
		}
	}
	if (min_xy) {
		min_xy->copy = (min_xy->copy == -1) ? -2 : -1;
		min_xy->print_graph();
	}
}

void ConvexHullInternal::Vertex::print_graph() {
	print();
	printf("\nEdges\n");
	Edge *e = edges;
	if (e) {
		do {
			e->print();
			printf("\n");
			e = e->next;
		} while (e != edges);
		do {
			Vertex *v = e->target;
			if (v->copy != copy) {
				v->copy = copy;
				v->print_graph();
			}
			e = e->next;
		} while (e != edges);
	}
}
#endif

ConvexHullInternal::Orientation ConvexHullInternal::get_orientation(const Edge *p_prev, const Edge *p_next, const Point32 &p_s, const Point32 &p_t) {
	CHULL_ASSERT(p_prev->reverse->target == p_next->reverse->target);
	if (p_prev->next == p_next) {
		if (p_prev->prev == p_next) {
			Point64 n = p_t.cross(p_s);
			Point64 m = (*p_prev->target - *p_next->reverse->target).cross(*p_next->target - *p_next->reverse->target);
			CHULL_ASSERT(!m.is_zero());
			int64_t dot = n.dot(m);
			CHULL_ASSERT(dot != 0);
			return (dot > 0) ? ORIENTATION_COUNTER_CLOCKWISE : ORIENTATION_CLOCKWISE;
		}
		return ORIENTATION_COUNTER_CLOCKWISE;
	} else if (p_prev->prev == p_next) {
		return ORIENTATION_CLOCKWISE;
	} else {
		return ORIENTATION_NONE;
	}
}

ConvexHullInternal::Edge *ConvexHullInternal::find_max_angle(bool p_ccw, const Vertex *p_start, const Point32 &p_s, const Point64 &p_rxs, const Point64 &p_sxrxs, Rational64 &p_min_cot) {
	Edge *min_edge = nullptr;

#ifdef DEBUG_CONVEX_HULL
	printf("find max edge for %d\n", p_start->point.index);
#endif
	Edge *e = p_start->edges;
	if (e) {
		do {
			if (e->copy > merge_stamp) {
				Point32 t = *e->target - *p_start;
				Rational64 cot(t.dot(p_sxrxs), t.dot(p_rxs));
#ifdef DEBUG_CONVEX_HULL
				printf("      Angle is %f (%d) for ", Math::atan(cot.to_scalar()), (int32_t)cot.is_nan());
				e->print();
#endif
				if (cot.is_nan()) {
					CHULL_ASSERT(p_ccw ? (t.dot(p_s) < 0) : (t.dot(p_s) > 0));
				} else {
					int32_t cmp;
					if (min_edge == nullptr) {
						p_min_cot = cot;
						min_edge = e;
					} else if ((cmp = cot.compare(p_min_cot)) < 0) {
						p_min_cot = cot;
						min_edge = e;
					} else if ((cmp == 0) && (p_ccw == (get_orientation(min_edge, e, p_s, t) == ORIENTATION_COUNTER_CLOCKWISE))) {
						min_edge = e;
					}
				}
#ifdef DEBUG_CONVEX_HULL
				printf("\n");
#endif
			}
			e = e->next;
		} while (e != p_start->edges);
	}
	return min_edge;
}

void ConvexHullInternal::find_edge_for_coplanar_faces(Vertex *p_c0, Vertex *p_c1, Edge *&p_e0, Edge *&p_e1, const Vertex *p_stop0, const Vertex *p_stop1) {
	Edge *start0 = p_e0;
	Edge *start1 = p_e1;
	Point32 et0 = start0 ? start0->target->point : p_c0->point;
	Point32 et1 = start1 ? start1->target->point : p_c1->point;
	Point32 s = p_c1->point - p_c0->point;
	Point64 normal = ((start0 ? start0 : start1)->target->point - p_c0->point).cross(s);
	int64_t dist = p_c0->point.dot(normal);
	CHULL_ASSERT(!start1 || (start1->target->point.dot(normal) == dist));
	Point64 perp = s.cross(normal);
	CHULL_ASSERT(!perp.is_zero());

#ifdef DEBUG_CONVEX_HULL
	printf("   Advancing %d %d  (%p %p, %d %d)\n", p_c0->point.index, p_c1->point.index, start0, start1, start0 ? start0->target->point.index : -1, start1 ? start1->target->point.index : -1);
#endif

	int64_t max_dot0 = et0.dot(perp);
	if (p_e0) {
		while (p_e0->target != p_stop0) {
			Edge *e = p_e0->reverse->prev;
			if (e->target->point.dot(normal) < dist) {
				break;
			}
			CHULL_ASSERT(e->target->point.dot(normal) == dist);
			if (e->copy == merge_stamp) {
				break;
			}
			int64_t dot = e->target->point.dot(perp);
			if (dot <= max_dot0) {
				break;
			}
			max_dot0 = dot;
			p_e0 = e;
			et0 = e->target->point;
		}
	}

	int64_t max_dot1 = et1.dot(perp);
	if (p_e1) {
		while (p_e1->target != p_stop1) {
			Edge *e = p_e1->reverse->next;
			if (e->target->point.dot(normal) < dist) {
				break;
			}
			CHULL_ASSERT(e->target->point.dot(normal) == dist);
			if (e->copy == merge_stamp) {
				break;
			}
			int64_t dot = e->target->point.dot(perp);
			if (dot <= max_dot1) {
				break;
			}
			max_dot1 = dot;
			p_e1 = e;
			et1 = e->target->point;
		}
	}

#ifdef DEBUG_CONVEX_HULL
	printf("   Starting at %d %d\n", et0.index, et1.index);
#endif

	int64_t dx = max_dot1 - max_dot0;
	if (dx > 0) {
		while (true) {
			int64_t dy = (et1 - et0).dot(s);

			if (p_e0 && (p_e0->target != p_stop0)) {
				Edge *f0 = p_e0->next->reverse;
				if (f0->copy > merge_stamp) {
					int64_t dx0 = (f0->target->point - et0).dot(perp);
					int64_t dy0 = (f0->target->point - et0).dot(s);
					if ((dx0 == 0) ? (dy0 < 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) >= 0))) {
						et0 = f0->target->point;
						dx = (et1 - et0).dot(perp);
						p_e0 = (p_e0 == start0) ? nullptr : f0;
						continue;
					}
				}
			}

			if (p_e1 && (p_e1->target != p_stop1)) {
				Edge *f1 = p_e1->reverse->next;
				if (f1->copy > merge_stamp) {
					Point32 d1 = f1->target->point - et1;
					if (d1.dot(normal) == 0) {
						int64_t dx1 = d1.dot(perp);
						int64_t dy1 = d1.dot(s);
						int64_t dxn = (f1->target->point - et0).dot(perp);
						if ((dxn > 0) && ((dx1 == 0) ? (dy1 < 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) > 0)))) {
							p_e1 = f1;
							et1 = p_e1->target->point;
							dx = dxn;
							continue;
						}
					} else {
						CHULL_ASSERT((p_e1 == start1) && (d1.dot(normal) < 0));
					}
				}
			}

			break;
		}
	} else if (dx < 0) {
		while (true) {
			int64_t dy = (et1 - et0).dot(s);

			if (p_e1 && (p_e1->target != p_stop1)) {
				Edge *f1 = p_e1->prev->reverse;
				if (f1->copy > merge_stamp) {
					int64_t dx1 = (f1->target->point - et1).dot(perp);
					int64_t dy1 = (f1->target->point - et1).dot(s);
					if ((dx1 == 0) ? (dy1 > 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) <= 0))) {
						et1 = f1->target->point;
						dx = (et1 - et0).dot(perp);
						p_e1 = (p_e1 == start1) ? nullptr : f1;
						continue;
					}
				}
			}

			if (p_e0 && (p_e0->target != p_stop0)) {
				Edge *f0 = p_e0->reverse->prev;
				if (f0->copy > merge_stamp) {
					Point32 d0 = f0->target->point - et0;
					if (d0.dot(normal) == 0) {
						int64_t dx0 = d0.dot(perp);
						int64_t dy0 = d0.dot(s);
						int64_t dxn = (et1 - f0->target->point).dot(perp);
						if ((dxn < 0) && ((dx0 == 0) ? (dy0 > 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) < 0)))) {
							p_e0 = f0;
							et0 = p_e0->target->point;
							dx = dxn;
							continue;
						}
					} else {
						CHULL_ASSERT((p_e0 == start0) && (d0.dot(normal) < 0));
					}
				}
			}

			break;
		}
	}
#ifdef DEBUG_CONVEX_HULL
	printf("   Advanced edges to %d %d\n", et0.index, et1.index);
#endif
}

void ConvexHullInternal::merge(IntermediateHull &p_h0, IntermediateHull &p_h1) {
	if (!p_h1.max_xy) {
		return;
	}
	if (!p_h0.max_xy) {
		p_h0 = p_h1;
		return;
	}

	merge_stamp--;

	Vertex *c0 = nullptr;
	Edge *to_prev0 = nullptr;
	Edge *first_new0 = nullptr;
	Edge *pending_head0 = nullptr;
	Edge *pending_tail0 = nullptr;
	Vertex *c1 = nullptr;
	Edge *to_prev1 = nullptr;
	Edge *first_new1 = nullptr;
	Edge *pending_head1 = nullptr;
	Edge *pending_tail1 = nullptr;
	Point32 prev_point;

	if (merge_projection(p_h0, p_h1, c0, c1)) {
		Point32 s = *c1 - *c0;
		Point64 normal = Point32(0, 0, -1).cross(s);
		Point64 t = s.cross(normal);
		CHULL_ASSERT(!t.is_zero());

		Edge *e = c0->edges;
		Edge *start0 = nullptr;
		if (e) {
			do {
				int64_t dot = (*e->target - *c0).dot(normal);
				CHULL_ASSERT(dot <= 0);
				if ((dot == 0) && ((*e->target - *c0).dot(t) > 0)) {
					if (!start0 || (get_orientation(start0, e, s, Point32(0, 0, -1)) == ORIENTATION_CLOCKWISE)) {
						start0 = e;
					}
				}
				e = e->next;
			} while (e != c0->edges);
		}

		e = c1->edges;
		Edge *start1 = nullptr;
		if (e) {
			do {
				int64_t dot = (*e->target - *c1).dot(normal);
				CHULL_ASSERT(dot <= 0);
				if ((dot == 0) && ((*e->target - *c1).dot(t) > 0)) {
					if (!start1 || (get_orientation(start1, e, s, Point32(0, 0, -1)) == ORIENTATION_COUNTER_CLOCKWISE)) {
						start1 = e;
					}
				}
				e = e->next;
			} while (e != c1->edges);
		}

		if (start0 || start1) {
			find_edge_for_coplanar_faces(c0, c1, start0, start1, nullptr, nullptr);
			if (start0) {
				c0 = start0->target;
			}
			if (start1) {
				c1 = start1->target;
			}
		}

		prev_point = c1->point;
		prev_point.z++;
	} else {
		prev_point = c1->point;
		prev_point.x++;
	}

	Vertex *first0 = c0;
	Vertex *first1 = c1;
	bool first_run = true;

	while (true) {
		Point32 s = *c1 - *c0;
		Point32 r = prev_point - c0->point;
		Point64 rxs = r.cross(s);
		Point64 sxrxs = s.cross(rxs);

#ifdef DEBUG_CONVEX_HULL
		printf("\n  Checking %d %d\n", c0->point.index, c1->point.index);
#endif
		Rational64 min_cot0(0, 0);
		Edge *min0 = find_max_angle(false, c0, s, rxs, sxrxs, min_cot0);
		Rational64 min_cot1(0, 0);
		Edge *min1 = find_max_angle(true, c1, s, rxs, sxrxs, min_cot1);
		if (!min0 && !min1) {
			Edge *e = new_edge_pair(c0, c1);
			e->link(e);
			c0->edges = e;

			e = e->reverse;
			e->link(e);
			c1->edges = e;
			return;
		} else {
			int32_t cmp = !min0 ? 1 : (!min1 ? -1 : min_cot0.compare(min_cot1));
#ifdef DEBUG_CONVEX_HULL
			printf("    -> Result %d\n", cmp);
#endif
			if (first_run || ((cmp >= 0) ? !min_cot1.is_negative_infinity() : !min_cot0.is_negative_infinity())) {
				Edge *e = new_edge_pair(c0, c1);
				if (pending_tail0) {
					pending_tail0->prev = e;
				} else {
					pending_head0 = e;
				}
				e->next = pending_tail0;
				pending_tail0 = e;

				e = e->reverse;
				if (pending_tail1) {
					pending_tail1->next = e;
				} else {
					pending_head1 = e;
				}
				e->prev = pending_tail1;
				pending_tail1 = e;
			}

			Edge *e0 = min0;
			Edge *e1 = min1;

#ifdef DEBUG_CONVEX_HULL
			printf("   Found min edges to %d %d\n", e0 ? e0->target->point.index : -1, e1 ? e1->target->point.index : -1);
#endif

			if (cmp == 0) {
				find_edge_for_coplanar_faces(c0, c1, e0, e1, nullptr, nullptr);
			}

			if ((cmp >= 0) && e1) {
				if (to_prev1) {
					for (Edge *e = to_prev1->next, *n = nullptr; e != min1; e = n) {
						n = e->next;
						remove_edge_pair(e);
					}
				}

				if (pending_tail1) {
					if (to_prev1) {
						to_prev1->link(pending_head1);
					} else {
						min1->prev->link(pending_head1);
						first_new1 = pending_head1;
					}
					pending_tail1->link(min1);
					pending_head1 = nullptr;
					pending_tail1 = nullptr;
				} else if (!to_prev1) {
					first_new1 = min1;
				}

				prev_point = c1->point;
				c1 = e1->target;
				to_prev1 = e1->reverse;
			}

			if ((cmp <= 0) && e0) {
				if (to_prev0) {
					for (Edge *e = to_prev0->prev, *n = nullptr; e != min0; e = n) {
						n = e->prev;
						remove_edge_pair(e);
					}
				}

				if (pending_tail0) {
					if (to_prev0) {
						pending_head0->link(to_prev0);
					} else {
						pending_head0->link(min0->next);
						first_new0 = pending_head0;
					}
					min0->link(pending_tail0);
					pending_head0 = nullptr;
					pending_tail0 = nullptr;
				} else if (!to_prev0) {
					first_new0 = min0;
				}

				prev_point = c0->point;
				c0 = e0->target;
				to_prev0 = e0->reverse;
			}
		}

		if ((c0 == first0) && (c1 == first1)) {
			if (to_prev0 == nullptr) {
				pending_head0->link(pending_tail0);
				c0->edges = pending_tail0;
			} else {
				for (Edge *e = to_prev0->prev, *n = nullptr; e != first_new0; e = n) {
					n = e->prev;
					remove_edge_pair(e);
				}
				if (pending_tail0) {
					pending_head0->link(to_prev0);
					first_new0->link(pending_tail0);
				}
			}

			if (to_prev1 == nullptr) {
				pending_tail1->link(pending_head1);
				c1->edges = pending_tail1;
			} else {
				for (Edge *e = to_prev1->next, *n = nullptr; e != first_new1; e = n) {
					n = e->next;
					remove_edge_pair(e);
				}
				if (pending_tail1) {
					to_prev1->link(pending_head1);
					pending_tail1->link(first_new1);
				}
			}

			return;
		}

		first_run = false;
	}
}

struct PointComparator {
	_FORCE_INLINE_ bool operator()(const ConvexHullInternal::Point32 &p_left, const ConvexHullInternal::Point32 &p_right) const {
		return (p_left.y < p_right.y) || ((p_left.y == p_right.y) && ((p_left.x < p_right.x) || ((p_left.x == p_right.x) && (p_left.z < p_right.z))));
	}
};

void ConvexHullInternal::compute(const Vector3 *p_coords, int32_t p_count) {
	AABB aabb;
	for (int32_t i = 0; i < p_count; i++) {
		Vector3 p = p_coords[i];
		if (i == 0) {
			aabb.position = p;
		} else {
			aabb.expand_to(p);
		}
	}

	Vector3 s = aabb.size;
	max_axis = s.max_axis_index();
	min_axis = s.min_axis_index();
	if (min_axis == max_axis) {
		min_axis = Vector3::Axis((max_axis + 1) % 3);
	}
	med_axis = Vector3::Axis(3 - max_axis - min_axis);

	s /= real_t(10216);
	if (((med_axis + 1) % 3) != max_axis) {
		s *= -1;
	}
	scaling = s;

	if (s[0] != 0) {
		s[0] = real_t(1) / s[0];
	}
	if (s[1] != 0) {
		s[1] = real_t(1) / s[1];
	}
	if (s[2] != 0) {
		s[2] = real_t(1) / s[2];
	}

	center = aabb.position;

	LocalVector<Point32> points;
	points.resize(p_count);
	for (int32_t i = 0; i < p_count; i++) {
		Vector3 p = p_coords[i];
		p = (p - center) * s;
		points[i].x = (int32_t)p[med_axis];
		points[i].y = (int32_t)p[max_axis];
		points[i].z = (int32_t)p[min_axis];
		points[i].index = i;
	}

	points.sort_custom<PointComparator>();

	vertex_pool.reset(true);
	original_vertices.resize(p_count);
	for (int32_t i = 0; i < p_count; i++) {
		Vertex *v = vertex_pool.alloc();
		v->edges = nullptr;
		v->point = points[i];
		v->copy = -1;
		original_vertices[i] = v;
	}

	points.clear();

	edge_pool.reset(true);

	used_edge_pairs = 0;
	max_used_edge_pairs = 0;

	merge_stamp = -3;

	IntermediateHull hull;
	compute_internal(0, p_count, hull);
	vertex_list = hull.min_xy;
#ifdef DEBUG_CONVEX_HULL
	printf("max. edges %d (3v = %d)", max_used_edge_pairs, 3 * p_count);
#endif
}

Vector3 ConvexHullInternal::to_gd_vector(const Point32 &p_v) {
	Vector3 p;
	p[med_axis] = real_t(p_v.x);
	p[max_axis] = real_t(p_v.y);
	p[min_axis] = real_t(p_v.z);
	return p * scaling;
}

Vector3 ConvexHullInternal::get_gd_normal(Face *p_face) {
	return to_gd_vector(p_face->dir0).cross(to_gd_vector(p_face->dir1)).normalized();
}

Vector3 ConvexHullInternal::get_coordinates(const Vertex *p_v) {
	Vector3 p;
	p[med_axis] = p_v->xvalue();
	p[max_axis] = p_v->yvalue();
	p[min_axis] = p_v->zvalue();
	return p * scaling + center;
}

real_t ConvexHullInternal::shrink(real_t p_amount, real_t p_clamp_amount) {
	if (!vertex_list) {
		return 0;
	}
	int32_t stamp = --merge_stamp;
	LocalVector<Vertex *> stack;
	vertex_list->copy = stamp;
	stack.push_back(vertex_list);
	LocalVector<Face *> faces;

	Point32 ref = vertex_list->point;
	Int128 hull_center_x(0, 0);
	Int128 hull_center_y(0, 0);
	Int128 hull_center_z(0, 0);
	Int128 volume(0, 0);

	while (stack.size() > 0) {
		Vertex *v = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		Edge *e = v->edges;
		if (e) {
			do {
				if (e->target->copy != stamp) {
					e->target->copy = stamp;
					stack.push_back(e->target);
				}
				if (e->copy != stamp) {
					Face *face = face_pool.alloc();
					face->init(e->target, e->reverse->prev->target, v);
					faces.push_back(face);
					Edge *f = e;

					Vertex *a = nullptr;
					Vertex *b = nullptr;
					do {
						if (a && b) {
							int64_t vol = (v->point - ref).dot((a->point - ref).cross(b->point - ref));
							CHULL_ASSERT(vol >= 0);
							Point32 c = v->point + a->point + b->point + ref;
							hull_center_x += vol * c.x;
							hull_center_y += vol * c.y;
							hull_center_z += vol * c.z;
							volume += vol;
						}

						CHULL_ASSERT(f->copy != stamp);
						f->copy = stamp;
						f->face = face;

						a = b;
						b = f->target;

						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != v->edges);
		}
	}

	if (volume.get_sign() <= 0) {
		return 0;
	}

	Vector3 hull_center;
	hull_center[med_axis] = hull_center_x.to_scalar();
	hull_center[max_axis] = hull_center_y.to_scalar();
	hull_center[min_axis] = hull_center_z.to_scalar();
	hull_center /= 4 * volume.to_scalar();
	hull_center *= scaling;

	int32_t face_count = faces.size();

	if (p_clamp_amount > 0) {
		real_t min_dist = FLT_MAX;
		for (int32_t i = 0; i < face_count; i++) {
			Vector3 normal = get_gd_normal(faces[i]);
			real_t dist = normal.dot(to_gd_vector(faces[i]->origin) - hull_center);
			if (dist < min_dist) {
				min_dist = dist;
			}
		}

		if (min_dist <= 0) {
			return 0;
		}

		p_amount = MIN(p_amount, min_dist * p_clamp_amount);
	}

	uint32_t seed = 243703;
	for (int32_t i = 0; i < face_count; i++, seed = 1664525 * seed + 1013904223) {
		SWAP(faces[i], faces[seed % face_count]);
	}

	for (int32_t i = 0; i < face_count; i++) {
		if (!shift_face(faces[i], p_amount, stack)) {
			return -p_amount;
		}
	}

	return p_amount;
}

bool ConvexHullInternal::shift_face(Face *p_face, real_t p_amount, LocalVector<Vertex *> &p_stack) {
	Vector3 orig_shift = get_gd_normal(p_face) * -p_amount;
	if (scaling[0] != 0) {
		orig_shift[0] /= scaling[0];
	}
	if (scaling[1] != 0) {
		orig_shift[1] /= scaling[1];
	}
	if (scaling[2] != 0) {
		orig_shift[2] /= scaling[2];
	}
	Point32 shift((int32_t)orig_shift[med_axis], (int32_t)orig_shift[max_axis], (int32_t)orig_shift[min_axis]);
	if (shift.is_zero()) {
		return true;
	}
	Point64 normal = p_face->get_normal();
#ifdef DEBUG_CONVEX_HULL
	printf("\nShrinking p_face (%d %d %d) (%d %d %d) (%d %d %d) by (%d %d %d)\n",
			p_face->origin.x, p_face->origin.y, p_face->origin.z, p_face->dir0.x, p_face->dir0.y, p_face->dir0.z, p_face->dir1.x, p_face->dir1.y, p_face->dir1.z, shift.x, shift.y, shift.z);
#endif
	int64_t orig_dot = p_face->origin.dot(normal);
	Point32 shifted_origin = p_face->origin + shift;
	int64_t shifted_dot = shifted_origin.dot(normal);
	CHULL_ASSERT(shifted_dot <= orig_dot);
	if (shifted_dot >= orig_dot) {
		return false;
	}

	Edge *intersection = nullptr;

	Edge *start_edge = p_face->nearby_vertex->edges;
#ifdef DEBUG_CONVEX_HULL
	printf("Start edge is ");
	start_edge->print();
	printf(", normal is (%lld %lld %lld), shifted dot is %lld\n", normal.x, normal.y, normal.z, shifted_dot);
#endif
	Rational128 opt_dot = p_face->nearby_vertex->dot(normal);
	int32_t cmp = opt_dot.compare(shifted_dot);
#ifdef SHOW_ITERATIONS
	int32_t n = 0;
#endif
	if (cmp >= 0) {
		Edge *e = start_edge;
		do {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			CHULL_ASSERT(dot.compare(orig_dot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			printf("Moving downwards, edge is ");
			e->print();
			printf(", dot is %f (%f %lld)\n", (float)dot.to_scalar(), (float)opt_dot.to_scalar(), shifted_dot);
#endif
			if (dot.compare(opt_dot) < 0) {
				int32_t c = dot.compare(shifted_dot);
				opt_dot = dot;
				e = e->reverse;
				start_edge = e;
				if (c < 0) {
					intersection = e;
					break;
				}
				cmp = c;
			}
			e = e->prev;
		} while (e != start_edge);

		if (!intersection) {
			return false;
		}
	} else {
		Edge *e = start_edge;
		do {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			CHULL_ASSERT(dot.compare(orig_dot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			printf("Moving upwards, edge is ");
			e->print();
			printf(", dot is %f (%f %lld)\n", (float)dot.to_scalar(), (float)opt_dot.to_scalar(), shifted_dot);
#endif
			if (dot.compare(opt_dot) > 0) {
				cmp = dot.compare(shifted_dot);
				if (cmp >= 0) {
					intersection = e;
					break;
				}
				opt_dot = dot;
				e = e->reverse;
				start_edge = e;
			}
			e = e->prev;
		} while (e != start_edge);

		if (!intersection) {
			return true;
		}
	}

#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to find initial intersection\n", n);
#endif

	if (cmp == 0) {
		Edge *e = intersection->reverse->next;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (e->target->dot(normal).compare(shifted_dot) <= 0) {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->next;
			if (e == intersection->reverse) {
				return true;
			}
#ifdef DEBUG_CONVEX_HULL
			printf("Checking for outwards edge, current edge is ");
			e->print();
			printf("\n");
#endif
		}
#ifdef SHOW_ITERATIONS
		printf("Needed %d iterations to check for complete containment\n", n);
#endif
	}

	Edge *first_intersection = nullptr;
	Edge *face_edge = nullptr;
	Edge *first_face_edge = nullptr;

#ifdef SHOW_ITERATIONS
	int32_t m = 0;
#endif
	while (true) {
#ifdef SHOW_ITERATIONS
		m++;
#endif
#ifdef DEBUG_CONVEX_HULL
		printf("Intersecting edge is ");
		intersection->print();
		printf("\n");
#endif
		if (cmp == 0) {
			Edge *e = intersection->reverse->next;
			start_edge = e;
#ifdef SHOW_ITERATIONS
			n = 0;
#endif
			while (true) {
#ifdef SHOW_ITERATIONS
				n++;
#endif
				if (e->target->dot(normal).compare(shifted_dot) >= 0) {
					break;
				}
				intersection = e->reverse;
				e = e->next;
				if (e == start_edge) {
					return true;
				}
			}
#ifdef SHOW_ITERATIONS
			printf("Needed %d iterations to advance intersection\n", n);
#endif
		}

#ifdef DEBUG_CONVEX_HULL
		printf("Advanced intersecting edge to ");
		intersection->print();
		printf(", cmp = %d\n", cmp);
#endif

		if (!first_intersection) {
			first_intersection = intersection;
		} else if (intersection == first_intersection) {
			break;
		}

		int32_t prev_cmp = cmp;
		Edge *prev_intersection = intersection;
		Edge *prev_face_edge = face_edge;

		Edge *e = intersection->reverse;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (true) {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->reverse->prev;
			CHULL_ASSERT(e != intersection->reverse);
			cmp = e->target->dot(normal).compare(shifted_dot);
#ifdef DEBUG_CONVEX_HULL
			printf("Testing edge ");
			e->print();
			printf(" -> cmp = %d\n", cmp);
#endif
			if (cmp >= 0) {
				intersection = e;
				break;
			}
		}
#ifdef SHOW_ITERATIONS
		printf("Needed %d iterations to find other intersection of p_face\n", n);
#endif

		if (cmp > 0) {
			Vertex *removed = intersection->target;
			e = intersection->reverse;
			if (e->prev == e) {
				removed->edges = nullptr;
			} else {
				removed->edges = e->prev;
				e->prev->link(e->next);
				e->link(e);
			}
#ifdef DEBUG_CONVEX_HULL
			printf("1: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif

			Point64 n0 = intersection->face->get_normal();
			Point64 n1 = intersection->reverse->face->get_normal();
			int64_t m00 = p_face->dir0.dot(n0);
			int64_t m01 = p_face->dir1.dot(n0);
			int64_t m10 = p_face->dir0.dot(n1);
			int64_t m11 = p_face->dir1.dot(n1);
			int64_t r0 = (intersection->face->origin - shifted_origin).dot(n0);
			int64_t r1 = (intersection->reverse->face->origin - shifted_origin).dot(n1);
			Int128 det = Int128::mul(m00, m11) - Int128::mul(m01, m10);
			CHULL_ASSERT(det.get_sign() != 0);
			Vertex *v = vertex_pool.alloc();
			v->point.index = -1;
			v->copy = -1;
			v->point128 = PointR128(Int128::mul(p_face->dir0.x * r0, m11) - Int128::mul(p_face->dir0.x * r1, m01) + Int128::mul(p_face->dir1.x * r1, m00) - Int128::mul(p_face->dir1.x * r0, m10) + det * shifted_origin.x,
					Int128::mul(p_face->dir0.y * r0, m11) - Int128::mul(p_face->dir0.y * r1, m01) + Int128::mul(p_face->dir1.y * r1, m00) - Int128::mul(p_face->dir1.y * r0, m10) + det * shifted_origin.y,
					Int128::mul(p_face->dir0.z * r0, m11) - Int128::mul(p_face->dir0.z * r1, m01) + Int128::mul(p_face->dir1.z * r1, m00) - Int128::mul(p_face->dir1.z * r0, m10) + det * shifted_origin.z,
					det);
			v->point.x = (int32_t)v->point128.xvalue();
			v->point.y = (int32_t)v->point128.yvalue();
			v->point.z = (int32_t)v->point128.zvalue();
			intersection->target = v;
			v->edges = e;

			p_stack.push_back(v);
			p_stack.push_back(removed);
			p_stack.push_back(nullptr);
		}

		if (cmp || prev_cmp || (prev_intersection->reverse->next->target != intersection->target)) {
			face_edge = new_edge_pair(prev_intersection->target, intersection->target);
			if (prev_cmp == 0) {
				face_edge->link(prev_intersection->reverse->next);
			}
			if ((prev_cmp == 0) || prev_face_edge) {
				prev_intersection->reverse->link(face_edge);
			}
			if (cmp == 0) {
				intersection->reverse->prev->link(face_edge->reverse);
			}
			face_edge->reverse->link(intersection->reverse);
		} else {
			face_edge = prev_intersection->reverse->next;
		}

		if (prev_face_edge) {
			if (prev_cmp > 0) {
				face_edge->link(prev_face_edge->reverse);
			} else if (face_edge != prev_face_edge->reverse) {
				p_stack.push_back(prev_face_edge->target);
				while (face_edge->next != prev_face_edge->reverse) {
					Vertex *removed = face_edge->next->target;
					remove_edge_pair(face_edge->next);
					p_stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
					printf("2: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
				}
				p_stack.push_back(nullptr);
			}
		}
		face_edge->face = p_face;
		face_edge->reverse->face = intersection->face;

		if (!first_face_edge) {
			first_face_edge = face_edge;
		}
	}
#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to process all intersections\n", m);
#endif

	if (cmp > 0) {
		first_face_edge->reverse->target = face_edge->target;
		first_intersection->reverse->link(first_face_edge);
		first_face_edge->link(face_edge->reverse);
	} else if (first_face_edge != face_edge->reverse) {
		p_stack.push_back(face_edge->target);
		while (first_face_edge->next != face_edge->reverse) {
			Vertex *removed = first_face_edge->next->target;
			remove_edge_pair(first_face_edge->next);
			p_stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
			printf("3: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
		}
		p_stack.push_back(nullptr);
	}

	CHULL_ASSERT(p_stack.size() > 0);
	vertex_list = p_stack[0];

#ifdef DEBUG_CONVEX_HULL
	printf("Removing part\n");
#endif
#ifdef SHOW_ITERATIONS
	n = 0;
#endif
	uint32_t pos = 0;
	while (pos < p_stack.size()) {
		uint32_t end = p_stack.size();
		while (pos < end) {
			Vertex *kept = p_stack[pos++];
#ifdef DEBUG_CONVEX_HULL
			kept->print();
#endif
			bool deeper = false;
			Vertex *removed;
			while ((removed = p_stack[pos++]) != nullptr) {
#ifdef SHOW_ITERATIONS
				n++;
#endif
				kept->receive_nearby_faces(removed);
				while (removed->edges) {
					if (!deeper) {
						deeper = true;
						p_stack.push_back(kept);
					}
					p_stack.push_back(removed->edges->target);
					remove_edge_pair(removed->edges);
				}
			}
			if (deeper) {
				p_stack.push_back(nullptr);
			}
		}
	}
#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to remove part\n", n);
#endif

	p_stack.clear();
	p_face->origin = shifted_origin;

	return true;
}

static int32_t get_vertex_copy(ConvexHullInternal::Vertex *p_vertex, LocalVector<ConvexHullInternal::Vertex *> &p_vertices) {
	int32_t index = p_vertex->copy;
	if (index < 0) {
		index = p_vertices.size();
		p_vertex->copy = index;
		p_vertices.push_back(p_vertex);
#ifdef DEBUG_CONVEX_HULL
		printf("Vertex %d gets index *%d\n", p_vertex->point.index, index);
#endif
	}
	return index;
}

real_t ConvexHullComputer::compute(const Vector3 *p_coords, int32_t p_count, real_t p_shrink, real_t p_shrink_clamp) {
	vertices.clear();
	edges.clear();
	faces.clear();

	if (p_count <= 0) {
		return 0;
	}

	ConvexHullInternal hull;
	hull.compute(p_coords, p_count);

	real_t shift = 0;
	if ((p_shrink > 0) && ((shift = hull.shrink(p_shrink, p_shrink_clamp)) < 0)) {
		return shift;
	}

	LocalVector<ConvexHullInternal::Vertex *> old_vertices;
	get_vertex_copy(hull.vertex_list, old_vertices);
	int32_t copied = 0;
	while (copied < (int32_t)old_vertices.size()) {
		ConvexHullInternal::Vertex *v = old_vertices[copied];
		vertices.push_back(hull.get_coordinates(v));
		ConvexHullInternal::Edge *first_edge = v->edges;
		if (first_edge) {
			int32_t first_copy = -1;
			int32_t prev_copy = -1;
			ConvexHullInternal::Edge *e = first_edge;
			do {
				if (e->copy < 0) {
					int32_t s = edges.size();
					edges.push_back(Edge());
					edges.push_back(Edge());
					Edge *c = &edges[s];
					Edge *r = &edges[s + 1];
					e->copy = s;
					e->reverse->copy = s + 1;
					c->reverse = 1;
					r->reverse = -1;
					c->target_vertex = get_vertex_copy(e->target, old_vertices);
					r->target_vertex = copied;
#ifdef DEBUG_CONVEX_HULL
					printf("      CREATE: Vertex *%d has edge to *%d\n", copied, c->get_target_vertex());
#endif
				}
				if (prev_copy >= 0) {
					edges[e->copy].next = prev_copy - e->copy;
				} else {
					first_copy = e->copy;
				}
				prev_copy = e->copy;
				e = e->next;
			} while (e != first_edge);
			edges[first_copy].next = prev_copy - first_copy;
		}
		copied++;
	}

	for (int32_t i = 0; i < copied; i++) {
		ConvexHullInternal::Vertex *v = old_vertices[i];
		ConvexHullInternal::Edge *first_edge = v->edges;
		if (first_edge) {
			ConvexHullInternal::Edge *e = first_edge;
			do {
				if (e->copy >= 0) {
#ifdef DEBUG_CONVEX_HULL
					printf("Vertex *%d has edge to *%d\n", i, edges[e->copy].get_target_vertex());
#endif
					faces.push_back(e->copy);
					ConvexHullInternal::Edge *f = e;
					do {
#ifdef DEBUG_CONVEX_HULL
						printf("   Face *%d\n", edges[f->copy].get_target_vertex());
#endif
						f->copy = -1;
						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != first_edge);
		}
	}

	return shift;
}

Error ConvexHullComputer::convex_hull(const Vector<Vector3> &p_points, Geometry3D::MeshData &r_mesh) {
	r_mesh = Geometry3D::MeshData(); // clear

	if (p_points.is_empty()) {
		return FAILED; // matches QuickHull
	}

	ConvexHullComputer ch;
	ch.compute(p_points.ptr(), p_points.size(), -1.0, -1.0);

	r_mesh.vertices = ch.vertices;

	// Tag which face each edge belongs to
	LocalVector<int32_t> edge_faces;
	edge_faces.resize(ch.edges.size());

	for (uint32_t i = 0; i < ch.edges.size(); i++) {
		edge_faces[i] = -1;
	}

	for (uint32_t i = 0; i < ch.faces.size(); i++) {
		const Edge *e_start = &ch.edges[ch.faces[i]];
		const Edge *e = e_start;
		do {
			int64_t ofs = e - ch.edges.ptr();
			edge_faces[ofs] = i;

			e = e->get_next_edge_of_face();
		} while (e != e_start);
	}

	// Copy the edges over. There's two "half-edges" for every edge, so we pick only one of them.
	r_mesh.edges.resize(ch.edges.size() / 2);
	AHashMap<uint64_t, int32_t> edge_map(ch.edges.size() * 4); // The higher the capacity, the faster the insert

	uint32_t edges_copied = 0;
	for (uint32_t i = 0; i < ch.edges.size(); i++) {
		ERR_CONTINUE(edge_faces[i] == -1); // Safety check.

		uint32_t a = (&ch.edges[i])->get_source_vertex();
		uint32_t b = (&ch.edges[i])->get_target_vertex();
		if (a < b) { // Copy only the "canonical" edge. For the reverse edge, this will be false.
			ERR_BREAK(edges_copied >= (uint32_t)r_mesh.edges.size());
			r_mesh.edges[edges_copied].vertex_a = a;
			r_mesh.edges[edges_copied].vertex_b = b;
			r_mesh.edges[edges_copied].face_a = edge_faces[i];
			r_mesh.edges[edges_copied].face_b = -1;

			uint64_t key = a;
			key <<= 32;
			key |= b;
			edge_map.insert(key, edges_copied);

			edges_copied++;
		} else {
			uint64_t key = b;
			key <<= 32;
			key |= a;
			int32_t *index_ptr = edge_map.getptr(key);
			if (!index_ptr) {
				ERR_PRINT("Invalid edge");
			} else {
				r_mesh.edges[*index_ptr].face_b = edge_faces[i];
			}
		}
	}

	if (edges_copied != (uint32_t)r_mesh.edges.size()) {
		ERR_PRINT("Invalid edge count.");
	}

	r_mesh.faces.resize(ch.faces.size());
	for (uint32_t i = 0; i < ch.faces.size(); i++) {
		const Edge *e_start = &ch.edges[ch.faces[i]];
		const Edge *e = e_start;
		Geometry3D::MeshData::Face &face = r_mesh.faces[i];

		do {
			face.indices.push_back(e->get_target_vertex());

			e = e->get_next_edge_of_face();
		} while (e != e_start);

		// reverse indices: Godot wants clockwise, but this is counter-clockwise
		if (face.indices.size() > 2) {
			// reverse all but the first index.
			int *indices = face.indices.ptr();
			for (uint32_t c = 0; c < (face.indices.size() - 1) / 2; c++) {
				SWAP(indices[c + 1], indices[face.indices.size() - 1 - c]);
			}
		}

		// compute normal
		if (face.indices.size() >= 3) {
			face.plane = Plane(r_mesh.vertices[face.indices[0]], r_mesh.vertices[face.indices[1]], r_mesh.vertices[face.indices[2]]);
		} else {
			WARN_PRINT("Too few vertices per face.");
		}
	}

	return OK;
}

// --- quick_hull.cpp ---

uint32_t QuickHull::debug_stop_after = 0xFFFFFFFF;

Error QuickHull::build(const Vector<Vector3> &p_points, Geometry3D::MeshData &r_mesh) {
	/* CREATE AABB VOLUME */

	AABB aabb;
	for (int i = 0; i < p_points.size(); i++) {
		if (i == 0) {
			aabb.position = p_points[i];
		} else {
			aabb.expand_to(p_points[i]);
		}
	}

	if (aabb.size == Vector3()) {
		return ERR_CANT_CREATE;
	}

	Vector<bool> valid_points;
	valid_points.resize(p_points.size());
	HashSet<Vector3> valid_cache;

	for (int i = 0; i < p_points.size(); i++) {
		Vector3 sp = p_points[i].snappedf(0.0001);
		if (valid_cache.has(sp)) {
			valid_points.write[i] = false;
		} else {
			valid_points.write[i] = true;
			valid_cache.insert(sp);
		}
	}

	/* CREATE INITIAL SIMPLEX */

	int longest_axis = aabb.get_longest_axis_index();

	//first two vertices are the most distant
	int simplex[4] = { 0 };

	{
		real_t max = 0, min = 0;

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}
			real_t d = p_points[i][longest_axis];
			if (i == 0 || d < min) {
				simplex[0] = i;
				min = d;
			}

			if (i == 0 || d > max) {
				simplex[1] = i;
				max = d;
			}
		}
	}

	//third vertex is one most further away from the line

	{
		real_t maxd = 0;
		Vector3 rel12 = p_points[simplex[0]] - p_points[simplex[1]];

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}

			Vector3 n = rel12.cross(p_points[simplex[0]] - p_points[i]).cross(rel12).normalized();
			real_t d = Math::abs(n.dot(p_points[simplex[0]]) - n.dot(p_points[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[2] = i;
			}
		}
	}

	//fourth vertex is the one most further away from the plane

	{
		real_t maxd = 0;
		Plane p(p_points[simplex[0]], p_points[simplex[1]], p_points[simplex[2]]);

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}

			real_t d = Math::abs(p.distance_to(p_points[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[3] = i;
			}
		}
	}

	//compute center of simplex, this is a point always warranted to be inside
	Vector3 center;

	for (int i = 0; i < 4; i++) {
		center += p_points[simplex[i]];
	}

	center /= 4.0;

	//add faces

	List<Face> faces;

	for (int i = 0; i < 4; i++) {
		static const int face_order[4][3] = {
			{ 0, 1, 2 },
			{ 0, 1, 3 },
			{ 0, 2, 3 },
			{ 1, 2, 3 }
		};

		Face f;
		for (int j = 0; j < 3; j++) {
			f.vertices[j] = simplex[face_order[i][j]];
		}

		Plane p(p_points[f.vertices[0]], p_points[f.vertices[1]], p_points[f.vertices[2]]);

		if (p.is_point_over(center)) {
			//flip face to clockwise if facing inwards
			SWAP(f.vertices[0], f.vertices[1]);
			p = -p;
		}

		f.plane = p;

		faces.push_back(f);
	}

	real_t over_tolerance = 3 * UNIT_EPSILON * (aabb.size.x + aabb.size.y + aabb.size.z);

	/* COMPUTE AVAILABLE VERTICES */

	for (int i = 0; i < p_points.size(); i++) {
		if (i == simplex[0]) {
			continue;
		}
		if (i == simplex[1]) {
			continue;
		}
		if (i == simplex[2]) {
			continue;
		}
		if (i == simplex[3]) {
			continue;
		}
		if (!valid_points[i]) {
			continue;
		}

		for (Face &E : faces) {
			if (E.plane.distance_to(p_points[i]) > over_tolerance) {
				E.points_over.push_back(i);
				break;
			}
		}
	}

	faces.sort(); // sort them, so the ones with points are in the back

	/* BUILD HULL */

	//poop face (while still remain)
	//find further away point
	//find lit faces
	//determine horizon edges
	//build new faces with horizon edges, them assign points side from all lit faces
	//remove lit faces

	uint32_t debug_stop = debug_stop_after;

	while (debug_stop > 0 && faces.back()->get().points_over.size()) {
		debug_stop--;
		Face &f = faces.back()->get();

		//find vertex most outside
		int next = -1;
		real_t next_d = 0;

		for (int i = 0; i < f.points_over.size(); i++) {
			real_t d = f.plane.distance_to(p_points[f.points_over[i]]);

			if (d > next_d) {
				next_d = d;
				next = i;
			}
		}

		ERR_FAIL_COND_V(next == -1, ERR_BUG);

		Vector3 v = p_points[f.points_over[next]];

		//find lit faces and lit edges
		List<List<Face>::Element *> lit_faces; //lit face is a death sentence

		HashMap<Edge, FaceConnect, Edge> lit_edges; //create this on the flight, should not be that bad for performance and simplifies code a lot

		for (List<Face>::Element *E = faces.front(); E; E = E->next()) {
			if (E->get().plane.distance_to(v) > 0) {
				lit_faces.push_back(E);

				for (int i = 0; i < 3; i++) {
					uint32_t a = E->get().vertices[i];
					uint32_t b = E->get().vertices[(i + 1) % 3];
					Edge e(a, b);

					HashMap<Edge, FaceConnect, Edge>::Iterator F = lit_edges.find(e);
					if (!F) {
						F = lit_edges.insert(e, FaceConnect());
					}
					if (e.vertices[0] == a) {
						//left
						F->value.left = E;
					} else {
						F->value.right = E;
					}
				}
			}
		}

		//create new faces from horizon edges
		List<List<Face>::Element *> new_faces; //new faces

		for (KeyValue<Edge, FaceConnect> &E : lit_edges) {
			FaceConnect &fc = E.value;
			if (fc.left && fc.right) {
				continue; //edge is uninteresting, not on horizon
			}

			//create new face!

			Face face;
			face.vertices[0] = f.points_over[next];
			face.vertices[1] = E.key.vertices[0];
			face.vertices[2] = E.key.vertices[1];

			Plane p(p_points[face.vertices[0]], p_points[face.vertices[1]], p_points[face.vertices[2]]);

			if (p.is_point_over(center)) {
				//flip face to clockwise if facing inwards
				SWAP(face.vertices[0], face.vertices[1]);
				p = -p;
			}

			face.plane = p;
			new_faces.push_back(faces.push_back(face));
		}

		//distribute points into new faces

		for (List<Face>::Element *&F : lit_faces) {
			Face &lf = F->get();

			for (int i = 0; i < lf.points_over.size(); i++) {
				if (lf.points_over[i] == f.points_over[next]) { //do not add current one
					continue;
				}

				Vector3 p = p_points[lf.points_over[i]];
				for (List<Face>::Element *&E : new_faces) {
					Face &f2 = E->get();
					if (f2.plane.distance_to(p) > over_tolerance) {
						f2.points_over.push_back(lf.points_over[i]);
						break;
					}
				}
			}
		}

		//erase lit faces

		while (lit_faces.size()) {
			faces.erase(lit_faces.front()->get());
			lit_faces.pop_front();
		}

		//put faces that contain no points on the front

		for (List<Face>::Element *&E : new_faces) {
			Face &f2 = E->get();
			if (f2.points_over.is_empty()) {
				faces.move_to_front(E);
			}
		}

		//whew, done with iteration, go next
	}

	/* CREATE MESHDATA */

	//make a map of edges again
	HashMap<Edge, RetFaceConnect, Edge> ret_edges;
	List<Geometry3D::MeshData::Face> ret_faces;

	for (const Face &E : faces) {
		Geometry3D::MeshData::Face f;
		f.plane = E.plane;

		for (int i = 0; i < 3; i++) {
			f.indices.push_back(E.vertices[i]);
		}

		List<Geometry3D::MeshData::Face>::Element *F = ret_faces.push_back(f);

		for (int i = 0; i < 3; i++) {
			uint32_t a = E.vertices[i];
			uint32_t b = E.vertices[(i + 1) % 3];
			Edge e(a, b);

			HashMap<Edge, RetFaceConnect, Edge>::Iterator G = ret_edges.find(e);
			if (!G) {
				G = ret_edges.insert(e, RetFaceConnect());
			}
			if (e.vertices[0] == a) {
				//left
				G->value.left = F;
			} else {
				G->value.right = F;
			}
		}
	}

	//fill faces

	for (List<Geometry3D::MeshData::Face>::Element *E = ret_faces.front(); E; E = E->next()) {
		Geometry3D::MeshData::Face &f = E->get();

		for (uint32_t i = 0; i < f.indices.size(); i++) {
			int a = E->get().indices[i];
			int b = E->get().indices[(i + 1) % f.indices.size()];
			Edge e(a, b);

			HashMap<Edge, RetFaceConnect, Edge>::Iterator F = ret_edges.find(e);

			ERR_CONTINUE(!F);
			List<Geometry3D::MeshData::Face>::Element *O = F->value.left == E ? F->value.right : F->value.left;
			ERR_CONTINUE(O == E);
			ERR_CONTINUE(O == nullptr);

			if (O->get().plane.is_equal_approx(f.plane)) {
				//merge and delete edge and contiguous face, while repointing edges (uuugh!)
				int o_index_size = O->get().indices.size();

				for (int j = 0; j < o_index_size; j++) {
					//search a
					if (O->get().indices[j] == a) {
						//append the rest
						for (int k = 0; k < o_index_size; k++) {
							int idx = O->get().indices[(k + j) % o_index_size];
							int idxn = O->get().indices[(k + j + 1) % o_index_size];
							if (idx == b && idxn == a) { //already have b!
								break;
							}
							if (idx != a) {
								f.indices.insert(i + 1, idx);
								i++;
							}
							Edge e2(idx, idxn);

							HashMap<Edge, RetFaceConnect, Edge>::Iterator F2 = ret_edges.find(e2);
							ERR_CONTINUE(!F2);
							//change faceconnect, point to this face instead
							if (F2->value.left == O) {
								F2->value.left = E;
							} else if (F2->value.right == O) {
								F2->value.right = E;
							}
						}

						break;
					}
				}

				// remove all edge connections to this face
				for (KeyValue<Edge, RetFaceConnect> &G : ret_edges) {
					if (G.value.left == O) {
						G.value.left = nullptr;
					}

					if (G.value.right == O) {
						G.value.right = nullptr;
					}
				}

				ret_edges.remove(F); //remove the edge
				ret_faces.erase(O); //remove the face
			}
		}
	}

	//fill mesh
	r_mesh.faces.clear();
	r_mesh.faces.resize(ret_faces.size());

	HashMap<List<Geometry3D::MeshData::Face>::Element *, int> face_indices;

	int idx = 0;
	for (List<Geometry3D::MeshData::Face>::Element *E = ret_faces.front(); E; E = E->next()) {
		face_indices[E] = idx;
		r_mesh.faces[idx++] = E->get();
	}
	r_mesh.edges.resize(ret_edges.size());
	idx = 0;
	for (const KeyValue<Edge, RetFaceConnect> &E : ret_edges) {
		Geometry3D::MeshData::Edge e;
		e.vertex_a = E.key.vertices[0];
		e.vertex_b = E.key.vertices[1];
		ERR_CONTINUE(!face_indices.has(E.value.left));
		ERR_CONTINUE(!face_indices.has(E.value.right));
		e.face_a = face_indices[E.value.left];
		e.face_b = face_indices[E.value.right];
		r_mesh.edges[idx++] = e;
	}

	r_mesh.vertices = p_points;

	return OK;
}


// --- Merged from core/math/math_vectors.cpp ---
#include "core/math/transform_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/math/vector3.h"
#include "core/math/projection.h"
#include "core/string/ustring.h"

// --- vector2.cpp ---

real_t Vector2::angle() const {
	return Math::atan2(y, x);
}

Vector2 Vector2::from_angle(real_t p_angle) {
	return Vector2(Math::cos(p_angle), Math::sin(p_angle));
}

real_t Vector2::length() const {
	return Math::sqrt(x * x + y * y);
}

real_t Vector2::length_squared() const {
	return x * x + y * y;
}

void Vector2::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector2 cannot be normalized, the elements must be finite. Making (0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	real_t l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
	}
}

Vector2 Vector2::normalized() const {
	Vector2 v = *this;
	v.normalize();
	return v;
}

bool Vector2::is_normalized() const {
	// use length_squared() instead of length() to avoid sqrt(), makes it more stringent.
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON);
}

real_t Vector2::distance_to(const Vector2 &p_vector2) const {
	return Math::sqrt((x - p_vector2.x) * (x - p_vector2.x) + (y - p_vector2.y) * (y - p_vector2.y));
}

real_t Vector2::distance_squared_to(const Vector2 &p_vector2) const {
	return (x - p_vector2.x) * (x - p_vector2.x) + (y - p_vector2.y) * (y - p_vector2.y);
}

real_t Vector2::angle_to(const Vector2 &p_vector2) const {
	return Math::atan2(cross(p_vector2), dot(p_vector2));
}

real_t Vector2::angle_to_point(const Vector2 &p_vector2) const {
	return (p_vector2 - *this).angle();
}

real_t Vector2::dot(const Vector2 &p_other) const {
	return x * p_other.x + y * p_other.y;
}

real_t Vector2::cross(const Vector2 &p_other) const {
	return x * p_other.y - y * p_other.x;
}

Vector2 Vector2::sign() const {
	return Vector2(SIGN(x), SIGN(y));
}

Vector2 Vector2::floor() const {
	return Vector2(Math::floor(x), Math::floor(y));
}

Vector2 Vector2::ceil() const {
	return Vector2(Math::ceil(x), Math::ceil(y));
}

Vector2 Vector2::round() const {
	return Vector2(Math::round(x), Math::round(y));
}

Vector2 Vector2::rotated(real_t p_by) const {
	real_t sine = Math::sin(p_by);
	real_t cosi = Math::cos(p_by);
	return Vector2(
			x * cosi - y * sine,
			x * sine + y * cosi);
}

Vector2 Vector2::posmod(real_t p_mod) const {
	return Vector2(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod));
}

Vector2 Vector2::posmodv(const Vector2 &p_modv) const {
	return Vector2(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y));
}

Vector2 Vector2::project(const Vector2 &p_to) const {
	return p_to * (dot(p_to) / p_to.length_squared());
}

Vector2 Vector2::clamp(const Vector2 &p_min, const Vector2 &p_max) const {
	return Vector2(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y));
}

Vector2 Vector2::clampf(real_t p_min, real_t p_max) const {
	return Vector2(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max));
}

Vector2 Vector2::snapped(const Vector2 &p_step) const {
	return Vector2(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y));
}

Vector2 Vector2::snappedf(real_t p_step) const {
	return Vector2(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step));
}

Vector2 Vector2::limit_length(real_t p_len) const {
	const real_t l = length();
	Vector2 v = *this;
	if (l > 0 && p_len < l) {
		v /= l;
		v *= p_len;
	}

	return v;
}

Vector2 Vector2::move_toward(const Vector2 &p_to, real_t p_delta) const {
	Vector2 v = *this;
	Vector2 vd = p_to - v;
	real_t len = vd.length();
	return len <= p_delta || len < (real_t)CMP_EPSILON ? p_to : v + vd / len * p_delta;
}

// slide returns the component of the vector along the given plane, specified by its normal vector.
Vector2 Vector2::slide(const Vector2 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector2(), "The normal Vector2 " + p_normal.operator String() + "must be normalized.");
#endif
	return *this - p_normal * dot(p_normal);
}

Vector2 Vector2::bounce(const Vector2 &p_normal) const {
	return -reflect(p_normal);
}

Vector2 Vector2::reflect(const Vector2 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector2(), "The normal Vector2 " + p_normal.operator String() + "must be normalized.");
#endif
	return 2.0f * p_normal * dot(p_normal) - *this;
}

bool Vector2::is_equal_approx(const Vector2 &p_v) const {
	return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y);
}

bool Vector2::is_same(const Vector2 &p_v) const {
	return Math::is_same(x, p_v.x) && Math::is_same(y, p_v.y);
}

bool Vector2::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y);
}

bool Vector2::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y);
}

Vector2::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ")";
}

Vector2::operator Vector2i() const {
	return Vector2i(x, y);
}

// --- vector2i.cpp ---

Vector2i Vector2i::clamp(const Vector2i &p_min, const Vector2i &p_max) const {
	return Vector2i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y));
}

Vector2i Vector2i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector2i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max));
}

Vector2i Vector2i::snapped(const Vector2i &p_step) const {
	return Vector2i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y));
}

Vector2i Vector2i::snappedi(int32_t p_step) const {
	return Vector2i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step));
}

int64_t Vector2i::length_squared() const {
	return x * (int64_t)x + y * (int64_t)y;
}

double Vector2i::length() const {
	return Math::sqrt((double)length_squared());
}

Vector2i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ")";
}

Vector2i::operator Vector2() const {
	return Vector2((int32_t)x, (int32_t)y);
}

// --- vector3.cpp ---

void Vector3::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = Basis(p_axis, p_angle).xform(*this);
}

Vector3 Vector3::rotated(const Vector3 &p_axis, real_t p_angle) const {
	Vector3 r = *this;
	r.rotate(p_axis, p_angle);
	return r;
}

void Vector3::snap(const Vector3 &p_step) {
	x = Math::snapped(x, p_step.x);
	y = Math::snapped(y, p_step.y);
	z = Math::snapped(z, p_step.z);
}

Vector3 Vector3::snapped(const Vector3 &p_step) const {
	Vector3 v = *this;
	v.snap(p_step);
	return v;
}

void Vector3::snapf(real_t p_step) {
	x = Math::snapped(x, p_step);
	y = Math::snapped(y, p_step);
	z = Math::snapped(z, p_step);
}

Vector3 Vector3::snappedf(real_t p_step) const {
	Vector3 v = *this;
	v.snapf(p_step);
	return v;
}

Vector3 Vector3::limit_length(real_t p_len) const {
	const real_t l = length();
	Vector3 v = *this;
	if (l > 0 && p_len < l) {
		v /= l;
		v *= p_len;
	}

	return v;
}

Vector3 Vector3::move_toward(const Vector3 &p_to, real_t p_delta) const {
	Vector3 v = *this;
	Vector3 vd = p_to - v;
	real_t len = vd.length();
	return len <= p_delta || len < (real_t)CMP_EPSILON ? p_to : v + vd / len * p_delta;
}

Vector2 Vector3::octahedron_encode() const {
	Vector3 n = *this;
	n /= Math::abs(n.x) + Math::abs(n.y) + Math::abs(n.z);
	Vector2 o;
	if (n.z >= 0.0f) {
		o.x = n.x;
		o.y = n.y;
	} else {
		o.x = (1.0f - Math::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
		o.y = (1.0f - Math::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
	}
	o.x = o.x * 0.5f + 0.5f;
	o.y = o.y * 0.5f + 0.5f;
	return o;
}

Vector3 Vector3::octahedron_decode(const Vector2 &p_oct) {
	Vector2 f(p_oct.x * 2.0f - 1.0f, p_oct.y * 2.0f - 1.0f);
	Vector3 n(f.x, f.y, 1.0f - Math::abs(f.x) - Math::abs(f.y));
	const real_t t = CLAMP(-n.z, 0.0f, 1.0f);
	n.x += n.x >= 0 ? -t : t;
	n.y += n.y >= 0 ? -t : t;
	return n.normalized();
}

Vector2 Vector3::octahedron_tangent_encode(float p_sign) const {
	const real_t bias = 1.0f / (real_t)32767.0f;
	Vector2 res = octahedron_encode();
	res.y = MAX(res.y, bias);
	res.y = res.y * 0.5f + 0.5f;
	res.y = p_sign >= 0.0f ? res.y : 1 - res.y;
	return res;
}

Vector3 Vector3::octahedron_tangent_decode(const Vector2 &p_oct, float *r_sign) {
	Vector2 oct_compressed = p_oct;
	oct_compressed.y = oct_compressed.y * 2 - 1;
	*r_sign = oct_compressed.y >= 0.0f ? 1.0f : -1.0f;
	oct_compressed.y = Math::abs(oct_compressed.y);
	Vector3 res = Vector3::octahedron_decode(oct_compressed);
	return res;
}

Basis Vector3::outer(const Vector3 &p_with) const {
	Basis basis;
	basis.rows[0] = Vector3(x * p_with.x, x * p_with.y, x * p_with.z);
	basis.rows[1] = Vector3(y * p_with.x, y * p_with.y, y * p_with.z);
	basis.rows[2] = Vector3(z * p_with.x, z * p_with.y, z * p_with.z);
	return basis;
}

bool Vector3::is_equal_approx(const Vector3 &p_v) const {
	return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y) && Math::is_equal_approx(z, p_v.z);
}

bool Vector3::is_same(const Vector3 &p_v) const {
	return Math::is_same(x, p_v.x) && Math::is_same(y, p_v.y) && Math::is_same(z, p_v.z);
}

bool Vector3::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z);
}

bool Vector3::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z);
}

Vector3::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ", " + String::num_real(z, true) + ")";
}

Vector3::operator Vector3i() const {
	return Vector3i(x, y, z);
}

// --- vector3i.cpp ---

Vector3i::Axis Vector3i::min_axis_index() const {
	return x < y ? (x < z ? Vector3i::AXIS_X : Vector3i::AXIS_Z) : (y < z ? Vector3i::AXIS_Y : Vector3i::AXIS_Z);
}

Vector3i::Axis Vector3i::max_axis_index() const {
	return x < y ? (y < z ? Vector3i::AXIS_Z : Vector3i::AXIS_Y) : (x < z ? Vector3i::AXIS_Z : Vector3i::AXIS_X);
}

Vector3i Vector3i::clamp(const Vector3i &p_min, const Vector3i &p_max) const {
	return Vector3i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z));
}

Vector3i Vector3i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector3i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max));
}

Vector3i Vector3i::snapped(const Vector3i &p_step) const {
	return Vector3i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y),
			Math::snapped(z, p_step.z));
}

Vector3i Vector3i::snappedi(int32_t p_step) const {
	return Vector3i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step),
			Math::snapped(z, p_step));
}

Vector3i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ", " + itos(z) + ")";
}

Vector3i::operator Vector3() const {
	return Vector3(x, y, z);
}

// --- vector4.cpp ---

Vector4::Axis Vector4::min_axis_index() const {
	uint32_t min_index = 0;
	real_t min_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) <= min_value) {
			min_index = i;
			min_value = operator[](i);
		}
	}
	return Vector4::Axis(min_index);
}

Vector4::Axis Vector4::max_axis_index() const {
	uint32_t max_index = 0;
	real_t max_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) > max_value) {
			max_index = i;
			max_value = operator[](i);
		}
	}
	return Vector4::Axis(max_index);
}

bool Vector4::is_equal_approx(const Vector4 &p_vec4) const {
	return Math::is_equal_approx(x, p_vec4.x) && Math::is_equal_approx(y, p_vec4.y) && Math::is_equal_approx(z, p_vec4.z) && Math::is_equal_approx(w, p_vec4.w);
}

bool Vector4::is_same(const Vector4 &p_vec4) const {
	return Math::is_same(x, p_vec4.x) && Math::is_same(y, p_vec4.y) && Math::is_same(z, p_vec4.z) && Math::is_same(w, p_vec4.w);
}

bool Vector4::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z) && Math::is_zero_approx(w);
}

bool Vector4::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

real_t Vector4::length() const {
	return Math::sqrt(length_squared());
}

void Vector4::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector4 cannot be normalized, the elements must be finite. Making (0, 0, 0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	real_t l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}
}

Vector4 Vector4::normalized() const {
	Vector4 v = *this;
	v.normalize();
	return v;
}

bool Vector4::is_normalized() const {
	return Math::is_equal_approx(length_squared(), (real_t)1, (real_t)UNIT_EPSILON);
}

real_t Vector4::distance_to(const Vector4 &p_to) const {
	return (p_to - *this).length();
}

real_t Vector4::distance_squared_to(const Vector4 &p_to) const {
	return (p_to - *this).length_squared();
}

Vector4 Vector4::direction_to(const Vector4 &p_to) const {
	Vector4 ret(p_to.x - x, p_to.y - y, p_to.z - z, p_to.w - w);
	ret.normalize();
	return ret;
}

Vector4 Vector4::abs() const {
	return Vector4(Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w));
}

Vector4 Vector4::sign() const {
	return Vector4(SIGN(x), SIGN(y), SIGN(z), SIGN(w));
}

Vector4 Vector4::floor() const {
	return Vector4(Math::floor(x), Math::floor(y), Math::floor(z), Math::floor(w));
}

Vector4 Vector4::ceil() const {
	return Vector4(Math::ceil(x), Math::ceil(y), Math::ceil(z), Math::ceil(w));
}

Vector4 Vector4::round() const {
	return Vector4(Math::round(x), Math::round(y), Math::round(z), Math::round(w));
}

Vector4 Vector4::lerp(const Vector4 &p_to, real_t p_weight) const {
	Vector4 res = *this;
	res.x = Math::lerp(res.x, p_to.x, p_weight);
	res.y = Math::lerp(res.y, p_to.y, p_weight);
	res.z = Math::lerp(res.z, p_to.z, p_weight);
	res.w = Math::lerp(res.w, p_to.w, p_weight);
	return res;
}

Vector4 Vector4::cubic_interpolate(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight) const {
	Vector4 res = *this;
	res.x = Math::cubic_interpolate(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight);
	res.y = Math::cubic_interpolate(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight);
	res.z = Math::cubic_interpolate(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight);
	res.w = Math::cubic_interpolate(res.w, p_b.w, p_pre_a.w, p_post_b.w, p_weight);
	return res;
}

Vector4 Vector4::cubic_interpolate_in_time(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
	Vector4 res = *this;
	res.x = Math::cubic_interpolate_in_time(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.y = Math::cubic_interpolate_in_time(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.z = Math::cubic_interpolate_in_time(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.w = Math::cubic_interpolate_in_time(res.w, p_b.w, p_pre_a.w, p_post_b.w, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	return res;
}

Vector4 Vector4::posmod(real_t p_mod) const {
	return Vector4(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod), Math::fposmod(z, p_mod), Math::fposmod(w, p_mod));
}

Vector4 Vector4::posmodv(const Vector4 &p_modv) const {
	return Vector4(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y), Math::fposmod(z, p_modv.z), Math::fposmod(w, p_modv.w));
}

void Vector4::snap(const Vector4 &p_step) {
	x = Math::snapped(x, p_step.x);
	y = Math::snapped(y, p_step.y);
	z = Math::snapped(z, p_step.z);
	w = Math::snapped(w, p_step.w);
}

void Vector4::snapf(real_t p_step) {
	x = Math::snapped(x, p_step);
	y = Math::snapped(y, p_step);
	z = Math::snapped(z, p_step);
	w = Math::snapped(w, p_step);
}

Vector4 Vector4::snapped(const Vector4 &p_step) const {
	Vector4 v = *this;
	v.snap(p_step);
	return v;
}

Vector4 Vector4::snappedf(real_t p_step) const {
	Vector4 v = *this;
	v.snapf(p_step);
	return v;
}

Vector4 Vector4::inverse() const {
	return Vector4(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w);
}

Vector4 Vector4::clamp(const Vector4 &p_min, const Vector4 &p_max) const {
	return Vector4(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z),
			CLAMP(w, p_min.w, p_max.w));
}

Vector4 Vector4::clampf(real_t p_min, real_t p_max) const {
	return Vector4(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max),
			CLAMP(w, p_min, p_max));
}

Vector4::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ", " + String::num_real(z, true) + ", " + String::num_real(w, true) + ")";
}

static_assert(sizeof(Vector4) == 4 * sizeof(real_t));

Vector4::operator Vector4i() const {
	return Vector4i(x, y, z, w);
}

// --- vector4i.cpp ---

Vector4i::Axis Vector4i::min_axis_index() const {
	uint32_t min_index = 0;
	int32_t min_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) <= min_value) {
			min_index = i;
			min_value = operator[](i);
		}
	}
	return Vector4i::Axis(min_index);
}

Vector4i::Axis Vector4i::max_axis_index() const {
	uint32_t max_index = 0;
	int32_t max_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) > max_value) {
			max_index = i;
			max_value = operator[](i);
		}
	}
	return Vector4i::Axis(max_index);
}

Vector4i Vector4i::clamp(const Vector4i &p_min, const Vector4i &p_max) const {
	return Vector4i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z),
			CLAMP(w, p_min.w, p_max.w));
}

Vector4i Vector4i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector4i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max),
			CLAMP(w, p_min, p_max));
}

Vector4i Vector4i::snapped(const Vector4i &p_step) const {
	return Vector4i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y),
			Math::snapped(z, p_step.z),
			Math::snapped(w, p_step.w));
}

Vector4i Vector4i::snappedi(int32_t p_step) const {
	return Vector4i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step),
			Math::snapped(z, p_step),
			Math::snapped(w, p_step));
}

Vector4i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ", " + itos(z) + ", " + itos(w) + ")";
}

Vector4i::operator Vector4() const {
	return Vector4(x, y, z, w);
}

static_assert(sizeof(Vector4i) == 4 * sizeof(int32_t));

// --- rect2.cpp ---

bool Rect2::is_equal_approx(const Rect2 &p_rect) const {
	return position.is_equal_approx(p_rect.position) && size.is_equal_approx(p_rect.size);
}

bool Rect2::is_same(const Rect2 &p_rect) const {
	return position.is_same(p_rect.position) && size.is_same(p_rect.size);
}

bool Rect2::is_finite() const {
	return position.is_finite() && size.is_finite();
}

bool Rect2::intersects_segment(const Point2 &p_from, const Point2 &p_to, Point2 *r_pos, Point2 *r_normal) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif
	real_t min = 0, max = 1;
	int axis = 0;
	real_t sign = 0;

	for (int i = 0; i < 2; i++) {
		real_t seg_from = p_from[i];
		real_t seg_to = p_to[i];
		real_t box_begin = position[i];
		real_t box_end = box_begin + size[i];
		real_t cmin, cmax;
		real_t csign;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;

		} else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	Vector2 rel = p_to - p_from;

	if (r_normal) {
		Vector2 normal;
		normal[axis] = sign;
		*r_normal = normal;
	}

	if (r_pos) {
		*r_pos = p_from + rel * min;
	}

	return true;
}

bool Rect2::intersects_transformed(const Transform2D &p_xform, const Rect2 &p_rect) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif
	//SAT intersection between local and transformed rect2

	Vector2 xf_points[4] = {
		p_xform.xform(p_rect.position),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y)),
		p_xform.xform(Vector2(p_rect.position.x, p_rect.position.y + p_rect.size.y)),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y)),
	};

	real_t low_limit;

	//base rect2 first (faster)

	if (xf_points[0].y > position.y) {
		goto next1;
	}
	if (xf_points[1].y > position.y) {
		goto next1;
	}
	if (xf_points[2].y > position.y) {
		goto next1;
	}
	if (xf_points[3].y > position.y) {
		goto next1;
	}

	return false;

next1:

	low_limit = position.y + size.y;

	if (xf_points[0].y < low_limit) {
		goto next2;
	}
	if (xf_points[1].y < low_limit) {
		goto next2;
	}
	if (xf_points[2].y < low_limit) {
		goto next2;
	}
	if (xf_points[3].y < low_limit) {
		goto next2;
	}

	return false;

next2:

	if (xf_points[0].x > position.x) {
		goto next3;
	}
	if (xf_points[1].x > position.x) {
		goto next3;
	}
	if (xf_points[2].x > position.x) {
		goto next3;
	}
	if (xf_points[3].x > position.x) {
		goto next3;
	}

	return false;

next3:

	low_limit = position.x + size.x;

	if (xf_points[0].x < low_limit) {
		goto next4;
	}
	if (xf_points[1].x < low_limit) {
		goto next4;
	}
	if (xf_points[2].x < low_limit) {
		goto next4;
	}
	if (xf_points[3].x < low_limit) {
		goto next4;
	}

	return false;

next4:

	Vector2 xf_points2[4] = {
		position,
		Vector2(position.x + size.x, position.y),
		Vector2(position.x, position.y + size.y),
		Vector2(position.x + size.x, position.y + size.y),
	};

	real_t maxa = p_xform.columns[0].dot(xf_points2[0]);
	real_t mina = maxa;

	real_t dp = p_xform.columns[0].dot(xf_points2[1]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[0].dot(xf_points2[2]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[0].dot(xf_points2[3]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	real_t maxb = p_xform.columns[0].dot(xf_points[0]);
	real_t minb = maxb;

	dp = p_xform.columns[0].dot(xf_points[1]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[0].dot(xf_points[2]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[0].dot(xf_points[3]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	if (mina > maxb) {
		return false;
	}
	if (minb > maxa) {
		return false;
	}

	maxa = p_xform.columns[1].dot(xf_points2[0]);
	mina = maxa;

	dp = p_xform.columns[1].dot(xf_points2[1]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[1].dot(xf_points2[2]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[1].dot(xf_points2[3]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	maxb = p_xform.columns[1].dot(xf_points[0]);
	minb = maxb;

	dp = p_xform.columns[1].dot(xf_points[1]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[1].dot(xf_points[2]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[1].dot(xf_points[3]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	if (mina > maxb) {
		return false;
	}
	if (minb > maxa) {
		return false;
	}

	return true;
}

Rect2 Rect2::intersection_transformed(const Transform2D &p_xform, const Rect2 &p_rect) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif

	if ((Math::is_zero_approx(p_xform.columns[0].y) && Math::is_zero_approx(p_xform.columns[1].x)) ||
			(Math::is_zero_approx(p_xform.columns[0].x) && Math::is_zero_approx(p_xform.columns[1].y))) {
		return intersection(p_xform.xform(p_rect));
	}

	if (!intersects_transformed(p_xform, p_rect)) {
		return Rect2();
	}

	const Vector2 xf_points[4] = {
		p_xform.xform(p_rect.position),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y)),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y)),
		p_xform.xform(Vector2(p_rect.position.x, p_rect.position.y + p_rect.size.y)),
	};

	// Use Sutherland–Hodgman algorithm.

	Vector2 subject[8];
	int subject_count = 4;
	subject[0] = xf_points[0];
	subject[1] = xf_points[1];
	subject[2] = xf_points[2];
	subject[3] = xf_points[3];

	const Vector2 min = position;
	const Vector2 max = position + size;
	Vector2 intersected;

	for (int edge = 0; edge < 4; edge++) {
		const int axis = edge % 2;
		const int another_axis = 1 - axis;
		const bool is_min = (edge < 2);
		intersected[axis] = is_min ? min[axis] : max[axis];

		Vector2 output[8];
		int output_count = 0;

		Vector2 prev = subject[subject_count - 1];
		bool prev_in_halfplane = is_min ? (prev[axis] >= intersected[axis]) : (prev[axis] <= intersected[axis]);

		for (int i = 0; i < subject_count; i++) {
			const Vector2 &curr = subject[i];
			bool curr_in_halfplane = is_min ? (curr[axis] >= intersected[axis]) : (curr[axis] <= intersected[axis]);
			if (prev_in_halfplane != curr_in_halfplane) {
				// Entering/exiting the half-plane.
				real_t t = (intersected[axis] - prev[axis]) / (curr[axis] - prev[axis]);
				intersected[another_axis] = prev[another_axis] + (curr[another_axis] - prev[another_axis]) * t;
				output[output_count++] = intersected;
			}
			if (curr_in_halfplane) {
				output[output_count++] = curr;
			}
			prev = curr;
			prev_in_halfplane = curr_in_halfplane;
		}

		for (int i = 0; i < output_count; i++) {
			subject[i] = output[i];
		}

		subject_count = output_count;
		if (subject_count == 0) {
			break;
		}
	}

	if (subject_count > 0) {
		return Rect2::from_points(subject, subject_count);
	}

	// Perform a reverse containment test; the current rect may be inside the transformed rect.

	const Vector2 corners[4] = {
		position,
		Vector2(max.x, min.y),
		max,
		Vector2(min.x, max.y)
	};

	Vector2 inside_points[4];
	int inside_count = 0;

	for (int point_idx = 0; point_idx < 4; point_idx++) {
		bool has_pos = false;
		bool has_neg = false;
		for (int idx = 0; idx < 4; idx++) {
			const int next_idx = (idx + 1) % 4;
			Vector2 v0 = xf_points[next_idx] - xf_points[idx];
			Vector2 v1 = corners[point_idx] - xf_points[idx];
			real_t cross = v0.cross(v1);
			if (cross > CMP_EPSILON) {
				has_pos = true;
			}
			if (cross < -CMP_EPSILON) {
				has_neg = true;
			}
			if (has_pos && has_neg) {
				break;
			}
		}
		if (!(has_pos && has_neg)) {
			inside_points[inside_count++] = corners[point_idx];
		}
	}

	return inside_count > 0 ? Rect2::from_points(inside_points, inside_count) : Rect2();
}

Rect2::operator String() const {
	return "[P: " + position.operator String() + ", S: " + size.operator String() + "]";
}

Rect2::operator Rect2i() const {
	return Rect2i(position, size);
}

// --- rect2i.cpp ---

Rect2i::operator String() const {
	return "[P: " + String(position) + ", S: " + String(size) + "]";
}

Rect2i::operator Rect2() const {
	return Rect2(position, size);
}


// --- Merged from core/math/math_primitives.cpp ---
#include "core/error/error_macros.h"
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/color_names.inc"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/random_pcg.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "modules/modules_enabled.gen.h"

#include <thirdparty/misc/ok_color.h>

// --- plane.cpp ---

void Plane::set_normal(const Vector3& p_normal) { normal = p_normal; }

void Plane::normalize()
{
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Plane cannot be normalized, the distance and the normal should be finite.");
	}
#endif // MATH_CHECKS

	real_t l = normal.length();
	if (l == 0) {
		zero();
		return;
	}
	normal /= l;
	d /= l;
}

Plane Plane::normalized() const
{
	Plane p = *this;
	p.normalize();
	return p;
}

Vector3 Plane::get_any_perpendicular_normal() const
{
	static const Vector3 p1 = Vector3(1, 0, 0);
	static const Vector3 p2 = Vector3(0, 1, 0);
	Vector3 p;

	if (Math::abs(normal.dot(p1)) > 0.99f) { // if too similar to p1
		p = p2;								 // use p2
	}
	else {
		p = p1; // use p1
	}

	p -= normal * normal.dot(p);
	p.normalize();

	return p;
}

/* intersections */

bool Plane::intersect_3(const Plane& p_plane1, const Plane& p_plane2, Vector3* r_result) const
{
	const Plane& p_plane0 = *this;
	Vector3 normal0 = p_plane0.normal;
	Vector3 normal1 = p_plane1.normal;
	Vector3 normal2 = p_plane2.normal;

	real_t denom = vec3_cross(normal0, normal1).dot(normal2);

	if (Math::is_zero_approx(denom)) {
		return false;
	}

	if (r_result) {
		*r_result = ((vec3_cross(normal1, normal2) * p_plane0.d) +
						(vec3_cross(normal2, normal0) * p_plane1.d) +
						(vec3_cross(normal0, normal1) * p_plane2.d)) /
					denom;
	}

	return true;
}

bool Plane::intersects_ray(
	const Vector3& p_from, const Vector3& p_dir, Vector3* p_intersection) const
{
	Vector3 segment = p_dir;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_from) - d) / den;

	if (dist >
		(real_t)CMP_EPSILON) { // this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

bool Plane::intersects_segment(
	const Vector3& p_begin, const Vector3& p_end, Vector3* p_intersection) const
{
	Vector3 segment = p_begin - p_end;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_begin) - d) / den;

	if (dist < (real_t)-CMP_EPSILON || dist > (1.0f + (real_t)CMP_EPSILON)) {
		return false;
	}

	dist = -dist;
	*p_intersection = p_begin + segment * dist;

	return true;
}

/* misc */

bool Plane::is_equal_approx_any_side(const Plane& p_plane) const
{
	return (normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d)) ||
		   (normal.is_equal_approx(-p_plane.normal) && Math::is_equal_approx(d, -p_plane.d));
}

bool Plane::is_equal_approx(const Plane& p_plane) const
{
	return normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d);
}

bool Plane::is_same(const Plane& p_plane) const
{
	return normal.is_same(p_plane.normal) && Math::is_same(d, p_plane.d);
}

bool Plane::is_finite() const { return normal.is_finite() && Math::is_finite(d); }

Plane::operator String() const
{
	return "[N: " + normal.operator String() + ", D: " + String::num_real(d, false) + "]";
}

// --- face3.cpp ---

int Face3::split_by_plane(const Plane &p_plane, Face3 p_res[3], bool p_is_point_over[3]) const {
	ERR_FAIL_COND_V(is_degenerate(), 0);

	Vector3 above[4];
	int above_count = 0;

	Vector3 below[4];
	int below_count = 0;

	for (int i = 0; i < 3; i++) {
		if (p_plane.has_point(vertex[i], (real_t)CMP_EPSILON)) { // point is in plane

			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = vertex[i];
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = vertex[i];

		} else {
			if (p_plane.is_point_over(vertex[i])) {
				//Point is over
				ERR_FAIL_COND_V(above_count >= 4, 0);
				above[above_count++] = vertex[i];

			} else {
				//Point is under
				ERR_FAIL_COND_V(below_count >= 4, 0);
				below[below_count++] = vertex[i];
			}

			/* Check for Intersection between this and the next vertex*/

			Vector3 inters;
			if (!p_plane.intersects_segment(vertex[i], vertex[(i + 1) % 3], &inters)) {
				continue;
			}

			/* Intersection goes to both */
			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = inters;
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = inters;
		}
	}

	int polygons_created = 0;

	ERR_FAIL_COND_V(above_count >= 4 && below_count >= 4, 0); //bug in the algo

	if (above_count >= 3) {
		p_res[polygons_created] = Face3(above[0], above[1], above[2]);
		p_is_point_over[polygons_created] = true;
		polygons_created++;

		if (above_count == 4) {
			p_res[polygons_created] = Face3(above[2], above[3], above[0]);
			p_is_point_over[polygons_created] = true;
			polygons_created++;
		}
	}

	if (below_count >= 3) {
		p_res[polygons_created] = Face3(below[0], below[1], below[2]);
		p_is_point_over[polygons_created] = false;
		polygons_created++;

		if (below_count == 4) {
			p_res[polygons_created] = Face3(below[2], below[3], below[0]);
			p_is_point_over[polygons_created] = false;
			polygons_created++;
		}
	}

	return polygons_created;
}

bool Face3::intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::ray_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::intersects_segment(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::segment_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::is_degenerate() const {
	Vector3 normal = vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]);
	return (normal.length_squared() < (real_t)CMP_EPSILON2);
}

Vector3 Face3::get_random_point_inside() const {
	real_t a = Math::random(0.0, 1.0);
	real_t b = Math::random(0.0, 1.0);
	if (a > b) {
		SWAP(a, b);
	}

	return vertex[0] * a + vertex[1] * (b - a) + vertex[2] * (1.0f - b);
}

Plane Face3::get_plane(ClockDirection p_dir) const {
	return Plane(vertex[0], vertex[1], vertex[2], p_dir);
}

real_t Face3::get_area() const {
	return vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]).length() * 0.5f;
}

bool Face3::intersects_aabb(const AABB &p_aabb) const {
	/** TEST PLANE **/
	if (!p_aabb.intersects_plane(get_plane())) {
		return false;
	}

#define TEST_AXIS(m_ax) \
	/** TEST FACE AXIS */ \
	{ \
		real_t aabb_min = p_aabb.position.m_ax; \
		real_t aabb_max = p_aabb.position.m_ax + p_aabb.size.m_ax; \
		real_t tri_min = vertex[0].m_ax; \
		real_t tri_max = vertex[0].m_ax; \
		for (int i = 1; i < 3; i++) { \
			if (vertex[i].m_ax > tri_max) \
				tri_max = vertex[i].m_ax; \
			if (vertex[i].m_ax < tri_min) \
				tri_min = vertex[i].m_ax; \
		} \
\
		if (tri_max < aabb_min || aabb_max < tri_min) \
			return false; \
	}

	TEST_AXIS(x);
	TEST_AXIS(y);
	TEST_AXIS(z);

#undef TEST_AXIS

	/** TEST ALL EDGES **/

	const Vector3 edge_norms[3] = {
		vertex[0] - vertex[1],
		vertex[1] - vertex[2],
		vertex[2] - vertex[0],
	};

	for (int i = 0; i < 12; i++) {
		Vector3 from, to;
		p_aabb.get_edge(i, from, to);
		Vector3 e1 = from - to;
		for (int j = 0; j < 3; j++) {
			Vector3 e2 = edge_norms[j];

			Vector3 axis = vec3_cross(e1, e2);

			if (axis.length_squared() < 0.0001f) {
				continue; // coplanar
			}
			axis.normalize();

			real_t minA, maxA, minB, maxB;
			p_aabb.project_range_in_plane(Plane(axis), minA, maxA);
			project_range(axis, Transform3D(), minB, maxB);

			if (maxA < minB || maxB < minA) {
				return false;
			}
		}
	}
	return true;
}

Face3::operator String() const {
	return String() + String(vertex[0]) + ", " + String(vertex[1]) + ", " + String(vertex[2]);
}

void Face3::project_range(const Vector3 &p_normal, const Transform3D &p_transform, real_t &r_min, real_t &r_max) const {
	for (int i = 0; i < 3; i++) {
		Vector3 v = p_transform.xform(vertex[i]);
		real_t d = p_normal.dot(v);

		if (i == 0 || d > r_max) {
			r_max = d;
		}

		if (i == 0 || d < r_min) {
			r_min = d;
		}
	}
}

void Face3::get_support(const Vector3 &p_normal, const Transform3D &p_transform, Vector3 *p_vertices, int *p_count, int p_max) const {
	constexpr double face_support_threshold = 0.98;
	constexpr double edge_support_threshold = 0.05;

	if (p_max <= 0) {
		return;
	}

	Vector3 n = p_transform.basis.xform_inv(p_normal);

	/** TEST FACE AS SUPPORT **/
	if (get_plane().normal.dot(n) > face_support_threshold) {
		*p_count = MIN(3, p_max);

		for (int i = 0; i < *p_count; i++) {
			p_vertices[i] = p_transform.xform(vertex[i]);
		}

		return;
	}

	/** FIND SUPPORT VERTEX **/

	int vert_support_idx = -1;
	real_t support_max = 0;

	for (int i = 0; i < 3; i++) {
		real_t d = n.dot(vertex[i]);

		if (i == 0 || d > support_max) {
			support_max = d;
			vert_support_idx = i;
		}
	}

	/** TEST EDGES AS SUPPORT **/

	for (int i = 0; i < 3; i++) {
		if (i != vert_support_idx && i + 1 != vert_support_idx) {
			continue;
		}

		// check if edge is valid as a support
		real_t dot = (vertex[i] - vertex[(i + 1) % 3]).normalized().dot(n);
		dot = Math::abs(dot);
		if (dot < edge_support_threshold) {
			*p_count = MIN(2, p_max);

			for (int j = 0; j < *p_count; j++) {
				p_vertices[j] = p_transform.xform(vertex[(j + i) % 3]);
			}

			return;
		}
	}

	*p_count = 1;
	p_vertices[0] = p_transform.xform(vertex[vert_support_idx]);
}

Vector3 Face3::get_closest_point_to(const Vector3 &p_point) const {
	Vector3 edge0 = vertex[1] - vertex[0];
	Vector3 edge1 = vertex[2] - vertex[0];
	Vector3 v0 = vertex[0] - p_point;

	real_t a = edge0.dot(edge0);
	real_t b = edge0.dot(edge1);
	real_t c = edge1.dot(edge1);
	real_t d = edge0.dot(v0);
	real_t e = edge1.dot(v0);

	real_t det = a * c - b * b;
	real_t s = b * e - c * d;
	real_t t = b * d - a * e;

	if (s + t < det) {
		if (s < 0.f) {
			if (t < 0.f) {
				if (d < 0.f) {
					s = CLAMP(-d / a, 0.f, 1.f);
					t = 0.f;
				} else {
					s = 0.f;
					t = CLAMP(-e / c, 0.f, 1.f);
				}
			} else {
				s = 0.f;
				t = CLAMP(-e / c, 0.f, 1.f);
			}
		} else if (t < 0.f) {
			s = CLAMP(-d / a, 0.f, 1.f);
			t = 0.f;
		} else {
			real_t invDet = 1.f / det;
			s *= invDet;
			t *= invDet;
		}
	} else {
		if (s < 0.f) {
			real_t tmp0 = b + d;
			real_t tmp1 = c + e;
			if (tmp1 > tmp0) {
				real_t numer = tmp1 - tmp0;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				t = CLAMP(-e / c, 0.f, 1.f);
				s = 0.f;
			}
		} else if (t < 0.f) {
			if (a + d > b + e) {
				real_t numer = c + e - b - d;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				s = CLAMP(-d / a, 0.f, 1.f);
				t = 0.f;
			}
		} else {
			real_t numer = c + e - b - d;
			real_t denom = a - 2 * b + c;
			s = CLAMP(numer / denom, 0.f, 1.f);
			t = 1.f - s;
		}
	}

	return vertex[0] + s * edge0 + t * edge1;
}

// --- aabb.cpp ---

real_t AABB::get_volume() const { return size.x * size.y * size.z; }

void AABB::merge_with(const AABB& p_aabb)
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 beg_1, beg_2;
	Vector3 end_1, end_2;
	Vector3 min, max;

	beg_1 = position;
	beg_2 = p_aabb.position;
	end_1 = size + beg_1;
	end_2 = p_aabb.size + beg_2;

	min.x = (beg_1.x < beg_2.x) ? beg_1.x : beg_2.x;
	min.y = (beg_1.y < beg_2.y) ? beg_1.y : beg_2.y;
	min.z = (beg_1.z < beg_2.z) ? beg_1.z : beg_2.z;

	max.x = (end_1.x > end_2.x) ? end_1.x : end_2.x;
	max.y = (end_1.y > end_2.y) ? end_1.y : end_2.y;
	max.z = (end_1.z > end_2.z) ? end_1.z : end_2.z;

	position = min;
	size = max - min;
}

bool AABB::is_equal_approx(const AABB& p_aabb) const
{
	return position.is_equal_approx(p_aabb.position) && size.is_equal_approx(p_aabb.size);
}

bool AABB::is_same(const AABB& p_aabb) const
{
	return position.is_same(p_aabb.position) && size.is_same(p_aabb.size);
}

bool AABB::is_finite() const { return position.is_finite() && size.is_finite(); }

AABB AABB::intersection(const AABB& p_aabb) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 src_min = position;
	Vector3 src_max = position + size;
	Vector3 dst_min = p_aabb.position;
	Vector3 dst_max = p_aabb.position + p_aabb.size;

	Vector3 min, max;

	if (src_min.x > dst_max.x || src_max.x < dst_min.x) {
		return AABB();
	}
	else {
		min.x = (src_min.x > dst_min.x) ? src_min.x : dst_min.x;
		max.x = (src_max.x < dst_max.x) ? src_max.x : dst_max.x;
	}

	if (src_min.y > dst_max.y || src_max.y < dst_min.y) {
		return AABB();
	}
	else {
		min.y = (src_min.y > dst_min.y) ? src_min.y : dst_min.y;
		max.y = (src_max.y < dst_max.y) ? src_max.y : dst_max.y;
	}

	if (src_min.z > dst_max.z || src_max.z < dst_min.z) {
		return AABB();
	}
	else {
		min.z = (src_min.z > dst_min.z) ? src_min.z : dst_min.z;
		max.z = (src_max.z < dst_max.z) ? src_max.z : dst_max.z;
	}

	return AABB(min, max - min);
}

// Note that this routine returns the BACKTRACKED (i.e. behind the ray origin)
// intersection point + normal if INSIDE the AABB.
// The caller can therefore decide when INSIDE whether to use the
// backtracked intersection, or use p_from as the intersection, and
// carry on progressing without e.g. reflecting against the normal.
bool AABB::find_intersects_ray(const Vector3& p_from, const Vector3& p_dir, bool& r_inside,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 end = position + size;
	real_t tmin = -1e20;
	real_t tmax = 1e20;
	int axis = 0;

	// Make sure r_inside is always initialized,
	// to prevent reading uninitialized data in the client code.
	r_inside = false;

	for (int i = 0; i < 3; i++) {
		if (p_dir[i] == 0) {
			if ((p_from[i] < position[i]) || (p_from[i] > end[i])) {
				return false;
			}
		}
		else { // ray not parallel to planes in this direction
			real_t t1 = (position[i] - p_from[i]) / p_dir[i];
			real_t t2 = (end[i] - p_from[i]) / p_dir[i];

			if (t1 > t2) {
				SWAP(t1, t2);
			}
			if (t1 >= tmin) {
				tmin = t1;
				axis = i;
			}
			if (t2 < tmax) {
				if (t2 < 0) {
					return false;
				}
				tmax = t2;
			}
			if (tmin > tmax) {
				return false;
			}
		}
	}

	// Did the ray start from inside the box?
	// In which case the intersection returned is the point of entry
	// (behind the ray start) or the calling routine can use the ray origin as intersection point.
	r_inside = tmin < 0;

	if (r_intersection_point) {
		*r_intersection_point = p_from + p_dir * tmin;

		// Prevent float error by making sure the point is exactly
		// on the AABB border on the relevant axis.
		(*r_intersection_point)[axis] = (p_dir[axis] >= 0) ? position[axis] : end[axis];
	}
	if (r_normal) {
		*r_normal = Vector3();
		(*r_normal)[axis] = (p_dir[axis] >= 0) ? -1 : 1;
	}

	return true;
}

bool AABB::intersects_segment(const Vector3& p_from, const Vector3& p_to,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	real_t min = 0, max = 1;
	int axis = 0;
	real_t sign = 0;

	for (int i = 0; i < 3; i++) {
		real_t seg_from = p_from[i];
		real_t seg_to = p_to[i];
		real_t box_begin = position[i];
		real_t box_end = box_begin + size[i];
		real_t cmin, cmax;
		real_t csign;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;

		}
		else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	Vector3 rel = p_to - p_from;

	if (r_normal) {
		Vector3 normal;
		normal[axis] = sign;
		*r_normal = normal;
	}

	if (r_intersection_point) {
		*r_intersection_point = p_from + rel * min;
	}

	return true;
}

bool AABB::intersects_plane(const Plane& p_plane) const
{
	Vector3 points[8] = {
		Vector3(position.x, position.y, position.z),
		Vector3(position.x, position.y, position.z + size.z),
		Vector3(position.x, position.y + size.y, position.z),
		Vector3(position.x, position.y + size.y, position.z + size.z),
		Vector3(position.x + size.x, position.y, position.z),
		Vector3(position.x + size.x, position.y, position.z + size.z),
		Vector3(position.x + size.x, position.y + size.y, position.z),
		Vector3(position.x + size.x, position.y + size.y, position.z + size.z),
	};

	bool over = false;
	bool under = false;

	for (int i = 0; i < 8; i++) {
		if (p_plane.distance_to(points[i]) > 0) {
			over = true;
		}
		else {
			under = true;
		}
	}

	return under && over;
}

Vector3 AABB::get_longest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = Vector3(0, 1, 0);
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_longest_axis_index() const
{
	int axis = 0;
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = 1;
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = 2;
	}

	return axis;
}

Vector3 AABB::get_shortest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = Vector3(0, 1, 0);
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_shortest_axis_index() const
{
	int axis = 0;
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = 1;
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = 2;
	}

	return axis;
}

AABB AABB::merge(const AABB& p_with) const
{
	AABB aabb = *this;
	aabb.merge_with(p_with);
	return aabb;
}

AABB AABB::expand(const Vector3& p_vector) const
{
	AABB aabb = *this;
	aabb.expand_to(p_vector);
	return aabb;
}

AABB AABB::grow(real_t p_by) const
{
	AABB aabb = *this;
	aabb.grow_by(p_by);
	return aabb;
}

void AABB::get_edge(int p_edge, Vector3& r_from, Vector3& r_to) const
{
	ERR_FAIL_INDEX(p_edge, 12);
	switch (p_edge) {
	case 0: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z);
	} break;
	case 1: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z);
	} break;
	case 2: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z + size.z);

	} break;
	case 3: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z + size.z);

	} break;
	case 4: {
		r_from = Vector3(position.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);
	} break;
	case 5: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
	} break;
	case 6: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 7: {
		r_from = Vector3(position.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 8: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 9: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 10: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);

	} break;
	case 11: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);


	} break;
	}
}

AABB::operator String() const { return "[P: " + String(position) + ", S: " + String(size) + "]"; }

// --- color.cpp ---

uint32_t Color::to_argb32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);

	return c;
}

uint32_t Color::to_abgr32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);

	return c;
}

uint32_t Color::to_rgba32() const {
	uint32_t c = (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(a * 255.0f);

	return c;
}

uint64_t Color::to_abgr64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);

	return c;
}

uint64_t Color::to_argb64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);

	return c;
}

uint64_t Color::to_rgba64() const {
	uint64_t c = (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(a * 65535.0f);

	return c;
}

void _append_hex(float p_val, char32_t *r_string) {
	int v = Math::round(p_val * 255.0f);
	v = CLAMP(v, 0, 255);

	r_string[0] = hex_char_table_lower[(v >> 4) & 0xF];
	r_string[1] = hex_char_table_lower[v & 0xF];
}

String Color::to_html(bool p_alpha) const {
	String txt;
	txt.resize_uninitialized(p_alpha ? 9 : 7);
	char32_t *ptr = txt.ptrw();

	_append_hex(r, ptr + 0);
	_append_hex(g, ptr + 2);
	_append_hex(b, ptr + 4);
	if (p_alpha) {
		_append_hex(a, ptr + 6);
	}
	ptr[txt.size() - 1] = '\0';

	return txt;
}

float Color::get_h() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	if (delta == 0.0f) {
		return 0.0f;
	}

	float h;
	if (r == max) {
		h = (g - b) / delta; // between yellow & magenta
	} else if (g == max) {
		h = 2 + (b - r) / delta; // between cyan & yellow
	} else {
		h = 4 + (r - g) / delta; // between magenta & cyan
	}

	h /= 6.0f;
	if (h < 0.0f) {
		h += 1.0f;
	}

	return h;
}

float Color::get_s() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	return (max != 0.0f) ? (delta / max) : 0.0f;
}

float Color::get_v() const {
	float max = MAX(r, g);
	max = MAX(max, b);
	return max;
}

void Color::set_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	int i;
	float f, p, q, t;
	a = p_alpha;

	if (p_s == 0.0f) {
		// Achromatic (gray)
		r = g = b = p_v;
		return;
	}

	p_h *= 6.0f;
	p_h = Math::fmod(p_h, 6);
	i = Math::floor(p_h);

	f = p_h - i;
	p = p_v * (1.0f - p_s);
	q = p_v * (1.0f - p_s * f);
	t = p_v * (1.0f - p_s * (1.0f - f));

	switch (i) {
		case 0: // Red is the dominant color
			r = p_v;
			g = t;
			b = p;
			break;
		case 1: // Green is the dominant color
			r = q;
			g = p_v;
			b = p;
			break;
		case 2:
			r = p;
			g = p_v;
			b = t;
			break;
		case 3: // Blue is the dominant color
			r = p;
			g = q;
			b = p_v;
			break;
		case 4:
			r = t;
			g = p;
			b = p_v;
			break;
		default: // (5) Red is the dominant color
			r = p_v;
			g = p;
			b = q;
			break;
	}
}

void Color::set_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	ok_color::HSL hsl;
	hsl.h = p_h;
	hsl.s = p_s;
	hsl.l = p_l;
	ok_color::RGB rgb = ok_color::okhsl_to_srgb(hsl);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

void Color::set_ok_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	ok_color::HSV hsv;
	hsv.h = p_h;
	hsv.s = p_s;
	hsv.v = p_v;
	ok_color::RGB rgb = ok_color::okhsv_to_srgb(hsv);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

bool Color::is_equal_approx(const Color &p_color) const {
	return Math::is_equal_approx(r, p_color.r) && Math::is_equal_approx(g, p_color.g) && Math::is_equal_approx(b, p_color.b) && Math::is_equal_approx(a, p_color.a);
}

bool Color::is_same(const Color &p_color) const {
	return Math::is_same(r, p_color.r) && Math::is_same(g, p_color.g) && Math::is_same(b, p_color.b) && Math::is_same(a, p_color.a);
}

Color Color::clamp(const Color &p_min, const Color &p_max) const {
	return Color(
			CLAMP(r, p_min.r, p_max.r),
			CLAMP(g, p_min.g, p_max.g),
			CLAMP(b, p_min.b, p_max.b),
			CLAMP(a, p_min.a, p_max.a));
}

void Color::invert() {
	r = 1.0f - r;
	g = 1.0f - g;
	b = 1.0f - b;
}

Color Color::hex(uint32_t p_hex) {
	float a = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float b = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float g = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float r = (p_hex & 0xFF) / 255.0f;

	return Color(r, g, b, a);
}

Color Color::hex64(uint64_t p_hex) {
	float a = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float b = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float g = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float r = (p_hex & 0xFFFF) / 65535.0f;

	return Color(r, g, b, a);
}

static int _parse_col4(const String &p_str, int p_ofs) {
	char character = p_str[p_ofs];

	if (character >= '0' && character <= '9') {
		return character - '0';
	} else if (character >= 'a' && character <= 'f') {
		return character + (10 - 'a');
	} else if (character >= 'A' && character <= 'F') {
		return character + (10 - 'A');
	}
	return -1;
}

static int _parse_col8(const String &p_str, int p_ofs) {
	return _parse_col4(p_str, p_ofs) * 16 + _parse_col4(p_str, p_ofs + 1);
}

Color Color::inverted() const {
	Color c = *this;
	c.invert();
	return c;
}

Color Color::html(const String &p_rgba) {
	if (p_rgba.is_empty()) {
		return Color();
	}

	const int current_pos = (p_rgba[0] == '#') ? 1 : 0;
	const int num_of_digits = p_rgba.length() - current_pos;

	float r, g, b, a = 1.0f;

	if (num_of_digits == 3) {
		// #rgb
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
	} else if (num_of_digits == 4) {
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
		a = _parse_col4(p_rgba, current_pos + 3) / 15.0f;
	} else if (num_of_digits == 6) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
	} else if (num_of_digits == 8) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
		a = _parse_col8(p_rgba, current_pos + 6) / 255.0f;
	} else {
		ERR_FAIL_V_MSG(Color(), "Invalid color code: " + p_rgba + ".");
	}

	ERR_FAIL_COND_V_MSG(r < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(g < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(b < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(a < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");

	return Color(r, g, b, a);
}

bool Color::html_is_valid(const String &p_color) {
	String color = p_color;

	if (color.is_empty()) {
		return false;
	}

	const int current_pos = (color[0] == '#') ? 1 : 0;
	const int len = color.length();
	const int num_of_digits = len - current_pos;
	if (!(num_of_digits == 3 || num_of_digits == 4 || num_of_digits == 6 || num_of_digits == 8)) {
		return false;
	}

	// Check if each hex digit is valid.
	for (int i = current_pos; i < len; i++) {
		if (!is_hex_digit(p_color[i])) {
			return false;
		}
	}

	return true;
}

Color Color::named(const String &p_name) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		ERR_FAIL_V_MSG(Color(), "Invalid color name: " + p_name + ".");
	}
	return named_colors[idx].color;
}

Color Color::named(const String &p_name, const Color &p_default) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		return p_default;
	}
	return named_colors[idx].color;
}

int Color::find_named_color(const String &p_name) {
	String name = p_name;
	// Normalize name.
	name = name.remove_chars(" -_'.");
	name = name.to_upper();

	static HashMap<String, int> named_colors_hashmap;
	if (unlikely(named_colors_hashmap.is_empty())) {
		const int named_color_count = get_named_color_count();
		for (int i = 0; i < named_color_count; i++) {
			named_colors_hashmap[String(named_colors[i].name).remove_char('_')] = i;
		}
	}

	const HashMap<String, int>::ConstIterator E = named_colors_hashmap.find(name);
	if (E) {
		return E->value;
	}

	return -1;
}

int Color::get_named_color_count() {
	return std_size(named_colors);
}

String Color::get_named_color_name(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), "");
	return named_colors[p_idx].name;
}

Color Color::get_named_color(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), Color());
	return named_colors[p_idx].color;
}

// For a version that errors on invalid values instead of returning
// a default color, use the Color(String) constructor instead.
Color Color::from_string(const String &p_string, const Color &p_default) {
	if (html_is_valid(p_string)) {
		return html(p_string);
	} else {
		return named(p_string, p_default);
	}
}

Color Color::from_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	Color c;
	c.set_hsv(p_h, p_s, p_v, p_alpha);
	return c;
}

Color Color::from_rgbe9995(uint32_t p_rgbe) {
	float r = p_rgbe & 0x1ff;
	float g = (p_rgbe >> 9) & 0x1ff;
	float b = (p_rgbe >> 18) & 0x1ff;
	float e = (p_rgbe >> 27);
	float m = Math::pow(2.0f, e - 15.0f - 9.0f);

	float rd = r * m;
	float gd = g * m;
	float bd = b * m;

	return Color(rd, gd, bd, 1.0f);
}

Color Color::from_rgba8(int64_t p_r8, int64_t p_g8, int64_t p_b8, int64_t p_a8) {
	return Color(p_r8 / 255.0f, p_g8 / 255.0f, p_b8 / 255.0f, p_a8 / 255.0f);
}

Color::operator String() const {
	return "(" + String::num(r, 4) + ", " + String::num(g, 4) + ", " + String::num(b, 4) + ", " + String::num(a, 4) + ")";
}

Color Color::from_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsl(p_h, p_s, p_l, p_alpha);
	return c;
}

Color Color::from_ok_hsv(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsv(p_h, p_s, p_l, p_alpha);
	return c;
}

float Color::get_ok_hsl_h() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.h)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.h, 0.0f, 1.0f);
}

float Color::get_ok_hsl_s() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.s)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.s, 0.0f, 1.0f);
}

float Color::get_ok_hsl_l() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.l)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.l, 0.0f, 1.0f);
}

// --- math_funcs.cpp ---

static RandomPCG default_rand;

uint32_t Math::rand_from_seed(uint64_t *p_seed) {
	RandomPCG rng = RandomPCG(*p_seed);
	uint32_t r = rng.rand();
	*p_seed = rng.get_seed();
	return r;
}

void Math::seed(uint64_t p_value) {
	default_rand.seed(p_value);
}

void Math::randomize() {
	default_rand.randomize();
}

uint32_t Math::rand() {
	return default_rand.rand();
}

double Math::randfn(double p_mean, double p_deviation) {
	return default_rand.randfn(p_mean, p_deviation);
}

int Math::step_decimals(double p_step) {
	static const int maxn = 10;
	static const double sd[maxn] = {
		0.9999, // somehow compensate for floating point error
		0.09999,
		0.009999,
		0.0009999,
		0.00009999,
		0.000009999,
		0.0000009999,
		0.00000009999,
		0.000000009999,
		0.0000000009999
	};

	double abs = Math::abs(p_step);
	double decs = abs - (int)abs; // Strip away integer part
	for (int i = 0; i < maxn; i++) {
		if (decs >= sd[i]) {
			return i;
		}
	}

	return 0;
}

// Only meant for editor usage in float ranges, where a step of 0
// means that decimal digits should not be limited in String::num.
int Math::range_step_decimals(double p_step) {
	if (p_step < 0.0000000000001) {
		return 16; // Max value hardcoded in String::num
	}
	return step_decimals(p_step);
}

double Math::ease(double p_x, double p_c) {
	if (p_x < 0) {
		p_x = 0;
	} else if (p_x > 1.0) {
		p_x = 1.0;
	}
	if (p_c > 0) {
		if (p_c < 1.0) {
			return 1.0 - Math::pow(1.0 - p_x, 1.0 / p_c);
		} else {
			return Math::pow(p_x, p_c);
		}
	} else if (p_c < 0) {
		//inout ease

		if (p_x < 0.5) {
			return Math::pow(p_x * 2.0, -p_c) * 0.5;
		} else {
			return (1.0 - Math::pow(1.0 - (p_x - 0.5) * 2.0, -p_c)) * 0.5 + 0.5;
		}
	} else {
		return 0; // no ease (raw)
	}
}

double Math::snapped(double p_value, double p_step) {
	if (p_step != 0) {
		p_value = Math::floor(p_value / p_step + 0.5) * p_step;
	}
	return p_value;
}

uint32_t Math::larger_prime(uint32_t p_val) {
	static const uint32_t primes[] = {
		5,
		13,
		23,
		47,
		97,
		193,
		389,
		769,
		1543,
		3079,
		6151,
		12289,
		24593,
		49157,
		98317,
		196613,
		393241,
		786433,
		1572869,
		3145739,
		6291469,
		12582917,
		25165843,
		50331653,
		100663319,
		201326611,
		402653189,
		805306457,
		1610612741,
		0,
	};

	int idx = 0;
	while (true) {
		ERR_FAIL_COND_V(primes[idx] == 0, 0);
		if (primes[idx] > p_val) {
			return primes[idx];
		}
		idx++;
	}
}

double Math::random(double p_from, double p_to) {
	return default_rand.random(p_from, p_to);
}

float Math::random(float p_from, float p_to) {
	return default_rand.random(p_from, p_to);
}

int Math::random(int p_from, int p_to) {
	return default_rand.random(p_from, p_to);
}

// --- random_pcg.cpp ---

RandomPCG::RandomPCG(uint64_t p_seed, uint64_t p_inc) :
		pcg(),
		current_inc(p_inc) {
	seed(p_seed);
}

void RandomPCG::randomize() {
	seed(((uint64_t)OS::get_unix_time() + OS::get_ticks_usec()) * pcg.state + PCG_DEFAULT_INC_64);
}

int64_t RandomPCG::rand_weighted(const Vector<float> &p_weights) {
	ERR_FAIL_COND_V_MSG(p_weights.is_empty(), -1, "Weights array is empty.");
	int64_t weights_size = p_weights.size();
	const float *weights = p_weights.ptr();
	float weights_sum = 0.0;
	for (int64_t i = 0; i < weights_size; ++i) {
		ERR_FAIL_COND_V_MSG(weights[i] < 0.0, -1, vformat("Weight array contains the negative value %f at index %d.", weights[i], i));
		weights_sum += weights[i];
	}

	float remaining_distance = randf() * weights_sum;
	for (int64_t i = 0; i < weights_size; ++i) {
		remaining_distance -= weights[i];
		if (remaining_distance < 0) {
			return i;
		}
	}

	for (int64_t i = weights_size - 1; i >= 0; --i) {
		if (weights[i] > 0) {
			return i;
		}
	}
	return -1;
}

double RandomPCG::random(double p_from, double p_to) {
	return randd() * (p_to - p_from) + p_from;
}

float RandomPCG::random(float p_from, float p_to) {
	return randf() * (p_to - p_from) + p_from;
}

int RandomPCG::random(int p_from, int p_to) {
	if (p_from == p_to) {
		return p_from;
	}

	int64_t min = MIN(p_from, p_to);
	int64_t max = MAX(p_from, p_to);
	uint32_t diff = static_cast<uint32_t>(max - min);

	if (diff == UINT32_MAX) {
		// Can't add 1 to max uint32_t value for inclusive range, so call rand without passing bounds.
		return static_cast<int64_t>(rand()) + min;
	}

	return static_cast<int64_t>(rand(diff + 1U)) + min;
}

// --- static_raycaster.cpp ---

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<StaticRaycaster> StaticRaycaster::create() { return Ref<StaticRaycaster>(); }

void StaticRaycaster::free() {}

bool StaticRaycaster::intersect(Ray& p_ray) { return false; }

void StaticRaycaster::intersect(Vector<Ray>& r_rays) {}

void StaticRaycaster::add_mesh(
	const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices, unsigned int p_id)
{
}

void StaticRaycaster::commit() {}

void StaticRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void StaticRaycaster::clear_mesh_filter() {}

StaticRaycaster::StaticRaycaster() {}

StaticRaycaster::~StaticRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)


// --- Merged from core/math/math_transforms.cpp ---
#include "core/math/aabb.h"
#include "core/math/transform_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/math/transform_3d.h"
#include "core/math/geometry_3d.h"
#include "core/string/ustring.h"

// --- quaternion.cpp ---

real_t Quaternion::angle_to(const Quaternion &p_to) const {
	real_t d = dot(p_to);
	// acos does clamping.
	return Math::acos(d * d * 2 - 1);
}

Vector3 Quaternion::get_euler(EulerOrder p_order) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Vector3(0, 0, 0), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Basis(*this).get_euler(p_order);
}

bool Quaternion::is_equal_approx(const Quaternion &p_quaternion) const {
	return Math::is_equal_approx(x, p_quaternion.x) && Math::is_equal_approx(y, p_quaternion.y) && Math::is_equal_approx(z, p_quaternion.z) && Math::is_equal_approx(w, p_quaternion.w);
}

bool Quaternion::is_same(const Quaternion &p_quaternion) const {
	return Math::is_same(x, p_quaternion.x) && Math::is_same(y, p_quaternion.y) && Math::is_same(z, p_quaternion.z) && Math::is_same(w, p_quaternion.w);
}

bool Quaternion::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

real_t Quaternion::length() const {
	return Math::sqrt(length_squared());
}

void Quaternion::normalize() {
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Quaternion cannot be normalized, the elements should be finite.");
	}
#endif // MATH_CHECKS
	*this /= length();
}

Quaternion Quaternion::normalized() const {
	return *this / length();
}

bool Quaternion::is_normalized() const {
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON); //use less epsilon
}

Quaternion Quaternion::inverse() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::log() const {
	Quaternion src = *this;
	Vector3 src_v = src.get_axis() * src.get_angle();
	return Quaternion(src_v.x, src_v.y, src_v.z, 0);
}

Quaternion Quaternion::exp() const {
	Quaternion src = *this;
	Vector3 src_v = Vector3(src.x, src.y, src.z);
	real_t theta = src_v.length();
	src_v = src_v.normalized();
	if (theta < CMP_EPSILON || !src_v.is_normalized()) {
		return Quaternion(0, 0, 0, 1);
	}
	return Quaternion(src_v, theta);
}

Quaternion Quaternion::slerp(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1 = -p_to;
	} else {
		to1 = p_to;
	}

	// calculate coefficients

	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_weight) * omega) / sinom;
		scale1 = Math::sin(p_weight * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_weight;
		scale1 = p_weight;
	}
	// calculate final values
	return Quaternion(
			scale0 * x + scale1 * to1.x,
			scale0 * y + scale1 * to1.y,
			scale0 * z + scale1 * to1.z,
			scale0 * w + scale1 * to1.w);
}

Quaternion Quaternion::slerpni(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	const Quaternion &from = *this;

	real_t dot = from.dot(p_to);

	if (Math::abs(dot) > 0.9999f) {
		return from;
	}

	real_t theta = Math::acos(dot),
		   sinT = 1.0f / Math::sin(theta),
		   newFactor = Math::sin(p_weight * theta) * sinT,
		   invFactor = Math::sin((1.0f - p_weight) * theta) * sinT;

	return Quaternion(invFactor * from.x + newFactor * p_to.x,
			invFactor * from.y + newFactor * p_to.y,
			invFactor * from.z + newFactor * p_to.z,
			invFactor * from.w + newFactor * p_to.w);
}

Quaternion Quaternion::spherical_cubic_interpolate(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion Quaternion::spherical_cubic_interpolate_in_time(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight,
		real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion::operator String() const {
	return "(" + String::num_real(x, false) + ", " + String::num_real(y, false) + ", " + String::num_real(z, false) + ", " + String::num_real(w, false) + ")";
}

Vector3 Quaternion::get_axis() const {
	if (Math::abs(w) > 1 - CMP_EPSILON) {
		return Vector3(x, y, z);
	}
	real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
	return Vector3(x * r, y * r, z * r);
}

real_t Quaternion::get_angle() const {
	return 2 * Math::acos(w);
}

Quaternion::Quaternion(const Vector3 &p_axis, real_t p_angle) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	real_t d = p_axis.length();
	if (d == 0) {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	} else {
		real_t sin_angle = Math::sin(p_angle * 0.5f);
		real_t cos_angle = Math::cos(p_angle * 0.5f);
		real_t s = sin_angle / d;
		x = p_axis.x * s;
		y = p_axis.y * s;
		z = p_axis.z * s;
		w = cos_angle;
	}
}

// Euler constructor expects a vector containing the Euler angles in the format
// (ax, ay, az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Quaternion Quaternion::from_euler(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.y * 0.5f;
	real_t half_a2 = p_euler.x * 0.5f;
	real_t half_a3 = p_euler.z * 0.5f;

	// R = Y(a1).X(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	return Quaternion(
			sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3,
			sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3,
			-sin_a1 * sin_a2 * cos_a3 + cos_a1 * cos_a2 * sin_a3,
			sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3);
}

// --- basis.cpp ---

#define cofac(row1, col1, row2, col2) \
	(rows[row1][col1] * rows[row2][col2] - rows[row1][col2] * rows[row2][col1])

void Basis::invert() {
	real_t co[3] = {
		cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
	};
	real_t det = rows[0][0] * co[0] +
			rows[0][1] * co[1] +
			rows[0][2] * co[2];
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t s = 1.0f / det;

	set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
			co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
			co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

#undef cofac

void Basis::orthonormalize() {
	// Orthonormalizable check is done in Vector3 class below.
	// Gram-Schmidt Process:

	Vector3 x = get_column(0);
	Vector3 y = get_column(1);
	Vector3 z = get_column(2);

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	z.normalize();

	set_column(0, x);
	set_column(1, y);
	set_column(2, z);
}

Basis Basis::orthonormalized() const {
	Basis c = *this;
	c.orthonormalize();
	return c;
}

void Basis::orthogonalize() {
	Vector3 scl = get_scale();
	orthonormalize();
	scale_local(scl);
}

Basis Basis::orthogonalized() const {
	Basis c = *this;
	c.orthogonalize();
	return c;
}

// Returns true if the basis vectors are orthogonal (perpendicular), so it has no skew or shear, and can be decomposed into rotation and scale.
// See https://en.wikipedia.org/wiki/Orthogonal_basis
bool Basis::is_orthogonal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis vectors are orthonormal (orthogonal and normalized), so it has no scale, skew, or shear.
// See https://en.wikipedia.org/wiki/Orthonormal_basis
bool Basis::is_orthonormal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_equal_approx(x.length_squared(), 1) && Math::is_equal_approx(y.length_squared(), 1) && Math::is_equal_approx(z.length_squared(), 1) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis is conformal (orthogonal, uniform scale, preserves angles and distance ratios).
// See https://en.wikipedia.org/wiki/Conformal_linear_transformation
bool Basis::is_conformal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	const real_t x_len_sq = x.length_squared();
	return Math::is_equal_approx(x_len_sq, y.length_squared()) && Math::is_equal_approx(x_len_sq, z.length_squared()) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis only has diagonal elements, so it may only have scale or flip, but no rotation, skew, or shear.
bool Basis::is_diagonal() const {
	return (
			Math::is_zero_approx(rows[0][1]) && Math::is_zero_approx(rows[0][2]) &&
			Math::is_zero_approx(rows[1][0]) && Math::is_zero_approx(rows[1][2]) &&
			Math::is_zero_approx(rows[2][0]) && Math::is_zero_approx(rows[2][1]));
}

// Returns true if the basis is a pure rotation matrix, so it has no scale, skew, shear, or flip.
bool Basis::is_rotation() const {
	return is_conformal() && Math::is_equal_approx(determinant(), 1, (real_t)UNIT_EPSILON);
}

#ifdef MATH_CHECKS
// This method is only used once, in diagonalize. If it's desired elsewhere, feel free to remove the #ifdef.
bool Basis::is_symmetric() const {
	if (!Math::is_equal_approx(rows[0][1], rows[1][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[0][2], rows[2][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[1][2], rows[2][1])) {
		return false;
	}

	return true;
}
#endif

Basis Basis::diagonalize() {
// NOTE: only implemented for symmetric matrices
// with the Jacobi iterative method
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
#endif
	const int ite_max = 1024;

	real_t off_matrix_norm_2 = rows[0][1] * rows[0][1] + rows[0][2] * rows[0][2] + rows[1][2] * rows[1][2];

	int ite = 0;
	Basis acc_rot;
	while (off_matrix_norm_2 > (real_t)CMP_EPSILON2 && ite++ < ite_max) {
		real_t el01_2 = rows[0][1] * rows[0][1];
		real_t el02_2 = rows[0][2] * rows[0][2];
		real_t el12_2 = rows[1][2] * rows[1][2];
		// Find the pivot element
		int i, j;
		if (el01_2 > el02_2) {
			if (el12_2 > el01_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 1;
			}
		} else {
			if (el12_2 > el02_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 2;
			}
		}

		// Compute the rotation angle
		real_t angle;
		if (Math::is_equal_approx(rows[j][j], rows[i][i])) {
			angle = Math::PI / 4;
		} else {
			angle = 0.5f * Math::atan(2 * rows[i][j] / (rows[j][j] - rows[i][i]));
		}

		// Compute the rotation matrix
		Basis rot;
		rot.rows[i][i] = rot.rows[j][j] = Math::cos(angle);
		rot.rows[i][j] = -(rot.rows[j][i] = Math::sin(angle));

		// Update the off matrix norm
		off_matrix_norm_2 -= rows[i][j] * rows[i][j];

		// Apply the rotation
		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
	}

	return acc_rot;
}

Basis Basis::inverse() const {
	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {
	SWAP(rows[0][1], rows[1][0]);
	SWAP(rows[0][2], rows[2][0]);
	SWAP(rows[1][2], rows[2][1]);
}

Basis Basis::transposed() const {
	Basis tr = *this;
	tr.transpose();
	return tr;
}

Basis Basis::from_scale(const Vector3 &p_scale) {
	return Basis(p_scale.x, 0, 0, 0, p_scale.y, 0, 0, 0, p_scale.z);
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3 &p_scale) {
	rows[0] *= p_scale.x;
	rows[1] *= p_scale.y;
	rows[2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale(p_scale);
	return m;
}

void Basis::scale_local(const Vector3 &p_scale) {
	// performs a scaling in object-local coordinate system:
	// M -> (M.S.Minv).M = M.S.
	rows[0] *= p_scale;
	rows[1] *= p_scale;
	rows[2] *= p_scale;
}

void Basis::scale_orthogonal(const Vector3 &p_scale) {
	*this = scaled_orthogonal(p_scale);
}

Basis Basis::scaled_orthogonal(const Vector3 &p_scale) const {
	Basis m = *this;
	Vector3 s = Vector3(-1, -1, -1) + p_scale;
	bool sign = std::signbit(s.x + s.y + s.z);
	Basis b = m.orthonormalized();
	s = b.xform_inv(s);
	Vector3 dots;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			dots[j] += s[i] * Math::abs(m.get_column(i).normalized().dot(b.get_column(j)));
		}
	}
	if (sign != std::signbit(dots.x + dots.y + dots.z)) {
		dots = -dots;
	}
	m.scale_local(Vector3(1, 1, 1) + dots);
	return m;
}

real_t Basis::get_uniform_scale() const {
	return (rows[0].length() + rows[1].length() + rows[2].length()) / 3.0f;
}

Basis Basis::scaled_local(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale_local(p_scale);
	return m;
}

Vector3 Basis::get_scale_abs() const {
	return Vector3(
			Vector3(rows[0][0], rows[1][0], rows[2][0]).length(),
			Vector3(rows[0][1], rows[1][1], rows[2][1]).length(),
			Vector3(rows[0][2], rows[1][2], rows[2][2]).length());
}

Vector3 Basis::get_scale_global() const {
	real_t det_sign = SIGN(determinant());
	return det_sign * Vector3(rows[0].length(), rows[1].length(), rows[2].length());
}

// get_scale works with get_rotation, use get_scale_abs if you need to enforce positive signature.
Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as get_rotation_axis_angle, get_rotation, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	//
	// The rotation part of this decomposition is returned by get_rotation* functions.
	real_t det_sign = SIGN(determinant());
	return det_sign * get_scale_abs();
}

// Decomposes a Basis into a rotation-reflection matrix (an element of the group O(3)) and a positive scaling matrix as B = O.S.
// Returns the rotation-reflection matrix via reference argument, and scaling information is returned as a Vector3.
// This (internal) function is too specific and named too ugly to expose to users, and probably there's no need to do so.
Vector3 Basis::rotref_posscale_decomposition(Basis &r_rotref) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(determinant() == 0, Vector3());

	Basis m = transposed() * (*this);
	ERR_FAIL_COND_V(!m.is_diagonal(), Vector3());
#endif
	Vector3 scale = get_scale();
	Basis inv_scale = Basis().scaled(scale.inverse()); // this will also absorb the sign of scale
	r_rotref = (*this) * inv_scale;

#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!r_rotref.is_orthogonal(), Vector3());
#endif
	return scale.abs();
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used by the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3 &p_axis, real_t p_angle) const {
	return Basis(p_axis, p_angle) * (*this);
}

void Basis::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

void Basis::rotate_local(const Vector3 &p_axis, real_t p_angle) {
	// performs a rotation in object-local coordinate system:
	// M -> (M.R.Minv).M = M.R.
	*this = rotated_local(p_axis, p_angle);
}

Basis Basis::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	return (*this) * Basis(p_axis, p_angle);
}

Basis Basis::rotated(const Vector3 &p_euler, EulerOrder p_order) const {
	return Basis::from_euler(p_euler, p_order) * (*this);
}

void Basis::rotate(const Vector3 &p_euler, EulerOrder p_order) {
	*this = rotated(p_euler, p_order);
}

Basis Basis::rotated(const Quaternion &p_quaternion) const {
	return Basis(p_quaternion) * (*this);
}

void Basis::rotate(const Quaternion &p_quaternion) {
	*this = rotated(p_quaternion);
}

Vector3 Basis::get_euler_normalized(EulerOrder p_order) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler(p_order);
}

Quaternion Basis::get_rotation_quaternion() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_quaternion();
}

void Basis::rotate_to_align(Vector3 p_start_direction, Vector3 p_end_direction) {
	// Takes two vectors and rotates the basis from the first vector to the second vector.
	// Adopted from: https://gist.github.com/kevinmoran/b45980723e53edeb8a5a43c49f134724
	const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
	if (axis.length_squared() != 0) {
		real_t dot = p_start_direction.dot(p_end_direction);
		dot = CLAMP(dot, -1.0f, 1.0f);
		const real_t angle_rads = Math::acos(dot);
		*this = Basis(axis, angle_rads) * (*this);
	}
}

void Basis::get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

void Basis::get_rotation_axis_angle_local(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = transposed();
	m.orthonormalize();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
	p_angle = -p_angle;
}

Vector3 Basis::get_euler(EulerOrder p_order) const {
	// This epsilon value results in angles within a +/- 0.04 degree range being simplified/truncated.
	// Based on testing, this is the largest the epsilon can be without the angle truncation becoming
	// visually noticeable.
	const real_t epsilon = 0.00000025;

	switch (p_order) {
		case EulerOrder::XYZ: {
			// Euler angles in XYZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz          -cy*sz           sy
			//        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
			//       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

			Vector3 euler;
			real_t sy = rows[0][2];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					// is this a pure Y rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[1][2] == 0 && rows[2][1] == 0 && rows[1][1] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = 0;
						euler.y = std::atan2(rows[0][2], rows[0][0]);
						euler.z = 0;
					} else {
						euler.x = Math::atan2(-rows[1][2], rows[2][2]);
						euler.y = Math::asin(sy);
						euler.z = Math::atan2(-rows[0][1], rows[0][0]);
					}
				} else {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = -Math::PI / 2.0f;
					euler.z = 0.0f;
				}
			} else {
				euler.x = Math::atan2(rows[2][1], rows[1][1]);
				euler.y = Math::PI / 2.0f;
				euler.z = 0.0f;
			}
			return euler;
		}
		case EulerOrder::XZY: {
			// Euler angles in XZY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             -sz             cz*sy
			//        sx*sy+cx*cy*sz    cx*cz           cx*sz*sy-cy*sx
			//        cy*sx*sz          cz*sx           cx*cy+sx*sz*sy

			Vector3 euler;
			real_t sz = rows[0][1];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = Math::asin(-sz);
				} else {
					// It's -1
					euler.x = -Math::atan2(rows[1][2], rows[2][2]);
					euler.y = 0.0f;
					euler.z = Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = -Math::atan2(rows[1][2], rows[2][2]);
				euler.y = 0.0f;
				euler.z = -Math::PI / 2.0f;
			}
			return euler;
		}
		case EulerOrder::YXZ: {
			// Euler angles in YXZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz+sy*sx*sz    cz*sy*sx-cy*sz        cx*sy
			//        cx*sz             cx*cz                 -sx
			//        cy*sx*sz-cz*sy    cy*cz*sx+sy*sz        cy*cx

			Vector3 euler;

			real_t m12 = rows[1][2];

			if (m12 < (1 - epsilon)) {
				if (m12 > -(1 - epsilon)) {
					// is this a pure X rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[0][2] == 0 && rows[2][0] == 0 && rows[0][0] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = std::atan2(-m12, rows[1][1]);
						euler.y = 0;
						euler.z = 0;
					} else {
						euler.x = std::asin(-m12);
						euler.y = std::atan2(rows[0][2], rows[2][2]);
						euler.z = std::atan2(rows[1][0], rows[1][1]);
					}
				} else { // m12 == -1
					euler.x = Math::PI * 0.5f;
					euler.y = std::atan2(rows[0][1], rows[0][0]);
					euler.z = 0;
				}
			} else { // m12 == 1
				euler.x = -Math::PI * 0.5f;
				euler.y = -std::atan2(rows[0][1], rows[0][0]);
				euler.z = 0;
			}

			return euler;
		}
		case EulerOrder::YZX: {
			// Euler angles in YZX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz             sy*sx-cy*cx*sz     cx*sy+cy*sz*sx
			//        sz                cz*cx              -cz*sx
			//        -cz*sy            cy*sx+cx*sy*sz     cy*cx-sy*sz*sx

			Vector3 euler;
			real_t sz = rows[1][0];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(-rows[1][2], rows[1][1]);
					euler.y = Math::atan2(-rows[2][0], rows[0][0]);
					euler.z = Math::asin(sz);
				} else {
					// It's -1
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = 0.0f;
					euler.z = -Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = Math::atan2(rows[2][1], rows[2][2]);
				euler.y = 0.0f;
				euler.z = Math::PI / 2.0f;
			}
			return euler;
		} break;
		case EulerOrder::ZXY: {
			// Euler angles in ZXY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy-sz*sx*sy    -cx*sz                cz*sy+cy*sz*sx
			//        cy*sz+cz*sx*sy    cz*cx                 sz*sy-cz*cy*sx
			//        -cx*sy            sx                    cx*cy
			Vector3 euler;
			real_t sx = rows[2][1];
			if (sx < (1.0f - epsilon)) {
				if (sx > -(1.0f - epsilon)) {
					euler.x = Math::asin(sx);
					euler.y = Math::atan2(-rows[2][0], rows[2][2]);
					euler.z = Math::atan2(-rows[0][1], rows[1][1]);
				} else {
					// It's -1
					euler.x = -Math::PI / 2.0f;
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = 0;
				}
			} else {
				// It's 1
				euler.x = Math::PI / 2.0f;
				euler.y = Math::atan2(rows[0][2], rows[0][0]);
				euler.z = 0;
			}
			return euler;
		} break;
		case EulerOrder::ZYX: {
			// Euler angles in ZYX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             cz*sy*sx-cx*sz        sz*sx+cz*cx*cy
			//        cy*sz             cz*cx+sz*sy*sx        cx*sz*sy-cz*sx
			//        -sy               cy*sx                 cy*cx
			Vector3 euler;
			real_t sy = rows[2][0];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = Math::asin(-sy);
					euler.z = Math::atan2(rows[1][0], rows[0][0]);
				} else {
					// It's -1
					euler.x = 0;
					euler.y = Math::PI / 2.0f;
					euler.z = -Math::atan2(rows[0][1], rows[1][1]);
				}
			} else {
				// It's 1
				euler.x = 0;
				euler.y = -Math::PI / 2.0f;
				euler.z = -Math::atan2(rows[0][1], rows[1][1]);
			}
			return euler;
		}
		default: {
			ERR_FAIL_V_MSG(Vector3(), "Invalid parameter for get_euler(order)");
		}
	}
	return Vector3();
}

void Basis::set_euler(const Vector3 &p_euler, EulerOrder p_order) {
	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1, 0, 0, 0, c, -s, 0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0, s, 0, 1, 0, -s, 0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0, s, c, 0, 0, 0, 1);

	switch (p_order) {
		case EulerOrder::XYZ: {
			*this = xmat * (ymat * zmat);
		} break;
		case EulerOrder::XZY: {
			*this = xmat * zmat * ymat;
		} break;
		case EulerOrder::YXZ: {
			*this = ymat * xmat * zmat;
		} break;
		case EulerOrder::YZX: {
			*this = ymat * zmat * xmat;
		} break;
		case EulerOrder::ZXY: {
			*this = zmat * xmat * ymat;
		} break;
		case EulerOrder::ZYX: {
			*this = zmat * ymat * xmat;
		} break;
		default: {
			ERR_FAIL_MSG("Invalid Euler order parameter.");
		}
	}
}

bool Basis::is_equal_approx(const Basis &p_basis) const {
	return rows[0].is_equal_approx(p_basis.rows[0]) && rows[1].is_equal_approx(p_basis.rows[1]) && rows[2].is_equal_approx(p_basis.rows[2]);
}

bool Basis::is_same(const Basis &p_basis) const {
	return rows[0].is_same(p_basis.rows[0]) && rows[1].is_same(p_basis.rows[1]) && rows[2].is_same(p_basis.rows[2]);
}

bool Basis::is_finite() const {
	return rows[0].is_finite() && rows[1].is_finite() && rows[2].is_finite();
}

Basis::operator String() const {
	return "[X: " + get_column(0).operator String() +
			", Y: " + get_column(1).operator String() +
			", Z: " + get_column(2).operator String() + "]";
}

Quaternion Basis::get_quaternion() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_rotation(), Quaternion(), "Basis " + operator String() + " must be normalized in order to be casted to a Quaternion. Use get_rotation_quaternion() or call orthonormalized() if the Basis contains linearly independent vectors.");
#endif
	/* Allow getting a quaternion from an unnormalized transform */
	Basis m = *this;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0f) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

void Basis::get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
	/* checking this is a bad idea, because obtaining from scaled transform is a valid use case
#ifdef MATH_CHECKS
	ERR_FAIL_COND(!is_rotation());
#endif
	*/

	// https://www.euclideanspace.com/math/geometry/rotations/conversions/matrixToAngle/index.htm
	real_t x, y, z; // Variables for result.
	if (Math::is_zero_approx(rows[0][1] - rows[1][0]) && Math::is_zero_approx(rows[0][2] - rows[2][0]) && Math::is_zero_approx(rows[1][2] - rows[2][1])) {
		// Singularity found.
		// First check for identity matrix which must have +1 for all terms in leading diagonal and zero in other terms.
		if (is_diagonal() && (Math::abs(rows[0][0] + rows[1][1] + rows[2][2] - 3) < 3 * CMP_EPSILON)) {
			// This singularity is identity matrix so angle = 0.
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// Otherwise this singularity is angle = 180.
		real_t xx = (rows[0][0] + 1) / 2;
		real_t yy = (rows[1][1] + 1) / 2;
		real_t zz = (rows[2][2] + 1) / 2;
		real_t xy = (rows[0][1] + rows[1][0]) / 4;
		real_t xz = (rows[0][2] + rows[2][0]) / 4;
		real_t yz = (rows[1][2] + rows[2][1]) / 4;

		if ((xx > yy) && (xx > zz)) { // rows[0][0] is the largest diagonal term.
			if (xx < CMP_EPSILON) {
				x = 0;
				y = Math::SQRT12;
				z = Math::SQRT12;
			} else {
				x = Math::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		} else if (yy > zz) { // rows[1][1] is the largest diagonal term.
			if (yy < CMP_EPSILON) {
				x = Math::SQRT12;
				y = 0;
				z = Math::SQRT12;
			} else {
				y = Math::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		} else { // rows[2][2] is the largest diagonal term so base result on this.
			if (zz < CMP_EPSILON) {
				x = Math::SQRT12;
				y = Math::SQRT12;
				z = 0;
			} else {
				z = Math::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = Math::PI;
		return;
	}
	// As we have reached here there are no singularities so we can handle normally.
	double s = Math::sqrt((rows[2][1] - rows[1][2]) * (rows[2][1] - rows[1][2]) + (rows[0][2] - rows[2][0]) * (rows[0][2] - rows[2][0]) + (rows[1][0] - rows[0][1]) * (rows[1][0] - rows[0][1])); // Used to normalize.

	if (Math::abs(s) < CMP_EPSILON) {
		// Prevent divide by zero, should not happen if matrix is orthogonal and should be caught by singularity test above.
		s = 1;
	}

	x = (rows[2][1] - rows[1][2]) / s;
	y = (rows[0][2] - rows[2][0]) / s;
	z = (rows[1][0] - rows[0][1]) / s;

	r_axis = Vector3(x, y, z);
	// acos does clamping.
	r_angle = Math::acos((rows[0][0] + rows[1][1] + rows[2][2] - 1) / 2);
}

void Basis::set_quaternion(const Quaternion &p_quaternion) {
	real_t d = p_quaternion.length_squared();
	real_t s = 2.0f / d;
	real_t xs = p_quaternion.x * s, ys = p_quaternion.y * s, zs = p_quaternion.z * s;
	real_t wx = p_quaternion.w * xs, wy = p_quaternion.w * ys, wz = p_quaternion.w * zs;
	real_t xx = p_quaternion.x * xs, xy = p_quaternion.x * ys, xz = p_quaternion.x * zs;
	real_t yy = p_quaternion.y * ys, yz = p_quaternion.y * zs, zz = p_quaternion.z * zs;
	set(1.0f - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1.0f - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1.0f - (xx + yy));
}

void Basis::set_axis_angle(const Vector3 &p_axis, real_t p_angle) {
// Rotation matrix from axis and angle, see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	Vector3 axis_sq(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);
	real_t cosine = Math::cos(p_angle);
	rows[0][0] = axis_sq.x + cosine * (1.0f - axis_sq.x);
	rows[1][1] = axis_sq.y + cosine * (1.0f - axis_sq.y);
	rows[2][2] = axis_sq.z + cosine * (1.0f - axis_sq.z);

	real_t sine = Math::sin(p_angle);
	real_t t = 1 - cosine;

	real_t xyzt = p_axis.x * p_axis.y * t;
	real_t zyxs = p_axis.z * sine;
	rows[0][1] = xyzt - zyxs;
	rows[1][0] = xyzt + zyxs;

	xyzt = p_axis.x * p_axis.z * t;
	zyxs = p_axis.y * sine;
	rows[0][2] = xyzt + zyxs;
	rows[2][0] = xyzt - zyxs;

	xyzt = p_axis.y * p_axis.z * t;
	zyxs = p_axis.x * sine;
	rows[1][2] = xyzt - zyxs;
	rows[2][1] = xyzt + zyxs;
}

void Basis::set_axis_angle_scale(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_axis, p_angle);
}

void Basis::set_euler_scale(const Vector3 &p_euler, const Vector3 &p_scale, EulerOrder p_order) {
	_set_diagonal(p_scale);
	rotate(p_euler, p_order);
}

void Basis::set_quaternion_scale(const Quaternion &p_quaternion, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_quaternion);
}

// This also sets the non-diagonal elements to 0, which is misleading from the
// name, so we want this method to be private. Use `from_scale` externally.
void Basis::_set_diagonal(const Vector3 &p_diag) {
	rows[0][0] = p_diag.x;
	rows[0][1] = 0;
	rows[0][2] = 0;

	rows[1][0] = 0;
	rows[1][1] = p_diag.y;
	rows[1][2] = 0;

	rows[2][0] = 0;
	rows[2][1] = 0;
	rows[2][2] = p_diag.z;
}

Basis Basis::lerp(const Basis &p_to, real_t p_weight) const {
	Basis b;
	b.rows[0] = rows[0].lerp(p_to.rows[0], p_weight);
	b.rows[1] = rows[1].lerp(p_to.rows[1], p_weight);
	b.rows[2] = rows[2].lerp(p_to.rows[2], p_weight);

	return b;
}

Basis Basis::slerp(const Basis &p_to, real_t p_weight) const {
	//consider scale
	Quaternion from(*this);
	Quaternion to(p_to);

	Basis b(from.slerp(to, p_weight));
	b.rows[0] *= Math::lerp(rows[0].length(), p_to.rows[0].length(), p_weight);
	b.rows[1] *= Math::lerp(rows[1].length(), p_to.rows[1].length(), p_weight);
	b.rows[2] *= Math::lerp(rows[2].length(), p_to.rows[2].length(), p_weight);

	return b;
}

void Basis::rotate_sh(real_t *p_values) {
	// code by John Hable
	// http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
	// this code is Public Domain

	const static real_t s_c3 = 0.94617469575; // (3*sqrt(5))/(4*sqrt(pi))
	const static real_t s_c4 = -0.31539156525; // (-sqrt(5))/(4*sqrt(pi))
	const static real_t s_c5 = 0.54627421529; // (sqrt(15))/(4*sqrt(pi))

	const static real_t s_c_scale = 1.0 / 0.91529123286551084;
	const static real_t s_c_scale_inv = 0.91529123286551084;

	const static real_t s_rc2 = 1.5853309190550713 * s_c_scale;
	const static real_t s_c4_div_c3 = s_c4 / s_c3;
	const static real_t s_c4_div_c3_x2 = (s_c4 / s_c3) * 2.0;

	const static real_t s_scale_dst2 = s_c3 * s_c_scale_inv;
	const static real_t s_scale_dst4 = s_c5 * s_c_scale_inv;

	const real_t src[9] = { p_values[0], p_values[1], p_values[2], p_values[3], p_values[4], p_values[5], p_values[6], p_values[7], p_values[8] };

	real_t m00 = rows[0][0];
	real_t m01 = rows[0][1];
	real_t m02 = rows[0][2];
	real_t m10 = rows[1][0];
	real_t m11 = rows[1][1];
	real_t m12 = rows[1][2];
	real_t m20 = rows[2][0];
	real_t m21 = rows[2][1];
	real_t m22 = rows[2][2];

	p_values[0] = src[0];
	p_values[1] = m11 * src[1] - m12 * src[2] + m10 * src[3];
	p_values[2] = -m21 * src[1] + m22 * src[2] - m20 * src[3];
	p_values[3] = m01 * src[1] - m02 * src[2] + m00 * src[3];

	real_t sh0 = src[7] + src[8] + src[8] - src[5];
	real_t sh1 = src[4] + s_rc2 * src[6] + src[7] + src[8];
	real_t sh2 = src[4];
	real_t sh3 = -src[7];
	real_t sh4 = -src[5];

	// Rotations.  R0 and R1 just use the raw matrix columns
	real_t r2x = m00 + m01;
	real_t r2y = m10 + m11;
	real_t r2z = m20 + m21;

	real_t r3x = m00 + m02;
	real_t r3y = m10 + m12;
	real_t r3z = m20 + m22;

	real_t r4x = m01 + m02;
	real_t r4y = m11 + m12;
	real_t r4z = m21 + m22;

	// dense matrix multiplication one column at a time

	// column 0
	real_t sh0_x = sh0 * m00;
	real_t sh0_y = sh0 * m10;
	real_t d0 = sh0_x * m10;
	real_t d1 = sh0_y * m20;
	real_t d2 = sh0 * (m20 * m20 + s_c4_div_c3);
	real_t d3 = sh0_x * m20;
	real_t d4 = sh0_x * m00 - sh0_y * m10;

	// column 1
	real_t sh1_x = sh1 * m02;
	real_t sh1_y = sh1 * m12;
	d0 += sh1_x * m12;
	d1 += sh1_y * m22;
	d2 += sh1 * (m22 * m22 + s_c4_div_c3);
	d3 += sh1_x * m22;
	d4 += sh1_x * m02 - sh1_y * m12;

	// column 2
	real_t sh2_x = sh2 * r2x;
	real_t sh2_y = sh2 * r2y;
	d0 += sh2_x * r2y;
	d1 += sh2_y * r2z;
	d2 += sh2 * (r2z * r2z + s_c4_div_c3_x2);
	d3 += sh2_x * r2z;
	d4 += sh2_x * r2x - sh2_y * r2y;

	// column 3
	real_t sh3_x = sh3 * r3x;
	real_t sh3_y = sh3 * r3y;
	d0 += sh3_x * r3y;
	d1 += sh3_y * r3z;
	d2 += sh3 * (r3z * r3z + s_c4_div_c3_x2);
	d3 += sh3_x * r3z;
	d4 += sh3_x * r3x - sh3_y * r3y;

	// column 4
	real_t sh4_x = sh4 * r4x;
	real_t sh4_y = sh4 * r4y;
	d0 += sh4_x * r4y;
	d1 += sh4_y * r4z;
	d2 += sh4 * (r4z * r4z + s_c4_div_c3_x2);
	d3 += sh4_x * r4z;
	d4 += sh4_x * r4x - sh4_y * r4y;

	// extra multipliers
	p_values[4] = d0;
	p_values[5] = -d1;
	p_values[6] = d2 * s_scale_dst2;
	p_values[7] = -d3;
	p_values[8] = d4 * s_scale_dst4;
}

Basis Basis::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(p_target.is_zero_approx(), Basis(), "The target vector can't be zero.");
	ERR_FAIL_COND_V_MSG(p_up.is_zero_approx(), Basis(), "The up vector can't be zero.");
#endif
	Vector3 v_z = p_target.normalized();
	if (!p_use_model_front) {
		v_z = -v_z;
	}
	Vector3 v_x = p_up.cross(v_z);
	if (v_x.is_zero_approx()) {
		WARN_PRINT("Target and up vectors are colinear. This is not advised as it may cause unwanted rotation around local Z axis.");
		v_x = p_up.get_any_perpendicular(); // Vectors are almost parallel.
	}
	v_x.normalize();
	Vector3 v_y = v_z.cross(v_x);

	Basis basis;
	basis.set_columns(v_x, v_y, v_z);
	return basis;
}

// --- transform_2d.cpp ---

void Transform2D::invert()
{
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform2D::affine_inverse can handle matrices with scaling, so GDScript should eventually
	// use that.
	SWAP(columns[0][1], columns[1][0]);
	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::inverse() const
{
	Transform2D inv = *this;
	inv.invert();
	return inv;
}

void Transform2D::affine_invert()
{
	real_t det = determinant();
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t idet = 1.0f / det;

	SWAP(columns[0][0], columns[1][1]);
	columns[0] *= Vector2(idet, -idet);
	columns[1] *= Vector2(-idet, idet);

	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::affine_inverse() const
{
	Transform2D inv = *this;
	inv.affine_invert();
	return inv;
}

void Transform2D::rotate(real_t p_angle) { *this = Transform2D(p_angle, Vector2()) * (*this); }

real_t Transform2D::get_skew() const
{
	real_t det = determinant();
	return Math::acos(columns[0].normalized().dot(SIGN(det) * columns[1].normalized())) -
		   (real_t)Math::PI * 0.5f;
}

void Transform2D::set_skew(real_t p_angle)
{
	real_t det = determinant();
	columns[1] = SIGN(det) * columns[0].rotated(((real_t)Math::PI * 0.5f + p_angle)).normalized() *
				 columns[1].length();
}

real_t Transform2D::get_rotation() const { return Math::atan2(columns[0].y, columns[0].x); }

void Transform2D::set_rotation(real_t p_rot)
{
	Size2 scale = get_scale();
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	set_scale(scale);
}

Transform2D::Transform2D(real_t p_rot, const Vector2& p_pos)
{
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	columns[2] = p_pos;
}

Transform2D::Transform2D(real_t p_rot, const Size2& p_scale, real_t p_skew, const Vector2& p_pos)
{
	columns[0][0] = Math::cos(p_rot) * p_scale.x;
	columns[1][1] = Math::cos(p_rot + p_skew) * p_scale.y;
	columns[1][0] = -Math::sin(p_rot + p_skew) * p_scale.y;
	columns[0][1] = Math::sin(p_rot) * p_scale.x;
	columns[2] = p_pos;
}

Size2 Transform2D::get_scale() const
{
	real_t det_sign = SIGN(determinant());
	return Size2(columns[0].length(), det_sign * columns[1].length());
}

void Transform2D::set_scale(const Size2& p_scale)
{
	columns[0].normalize();
	columns[1].normalize();
	columns[0] *= p_scale.x;
	columns[1] *= p_scale.y;
}

void Transform2D::scale(const Size2& p_scale)
{
	scale_basis(p_scale);
	columns[2] *= p_scale;
}

void Transform2D::scale_basis(const Size2& p_scale)
{
	columns[0][0] *= p_scale.x;
	columns[0][1] *= p_scale.y;
	columns[1][0] *= p_scale.x;
	columns[1][1] *= p_scale.y;
}

void Transform2D::translate_local(real_t p_tx, real_t p_ty)
{
	translate_local(Vector2(p_tx, p_ty));
}

void Transform2D::translate_local(const Vector2& p_translation)
{
	columns[2] += basis_xform(p_translation);
}

void Transform2D::orthonormalize()
{
	// Orthonormalizable check is done in Vector2 class below.
	// Gram-Schmidt Process:

	Vector2 x = columns[0];
	Vector2 y = columns[1];

	x.normalize();
	y = y - x * x.dot(y);
	y.normalize();

	columns[0] = x;
	columns[1] = y;
}

Transform2D Transform2D::orthonormalized() const
{
	Transform2D ortho = *this;
	ortho.orthonormalize();
	return ortho;
}

bool Transform2D::is_conformal() const
{
	// Non-flipped case.
	if (Math::is_equal_approx(columns[0][0], columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], -columns[1][0])) {
		return true;
	}
	// Flipped case.
	if (Math::is_equal_approx(columns[0][0], -columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], columns[1][0])) {
		return true;
	}
	return false;
}

bool Transform2D::is_equal_approx(const Transform2D& p_transform) const
{
	return columns[0].is_equal_approx(p_transform.columns[0]) &&
		   columns[1].is_equal_approx(p_transform.columns[1]) &&
		   columns[2].is_equal_approx(p_transform.columns[2]);
}

bool Transform2D::is_same(const Transform2D& p_transform) const
{
	return columns[0].is_same(p_transform.columns[0]) &&
		   columns[1].is_same(p_transform.columns[1]) && columns[2].is_same(p_transform.columns[2]);
}

bool Transform2D::is_finite() const
{
	return columns[0].is_finite() && columns[1].is_finite() && columns[2].is_finite();
}

Transform2D Transform2D::looking_at(const Vector2& p_target) const
{
	Transform2D return_trans = Transform2D(get_rotation(), get_origin());
	Vector2 target_position = affine_inverse().xform(p_target);
	return_trans.set_rotation(
		return_trans.get_rotation() + (target_position * get_scale()).angle());
	return return_trans;
}

void Transform2D::operator*=(const Transform2D& p_transform)
{
	columns[2] = xform(p_transform.columns[2]);

	real_t x0, x1, y0, y1;

	x0 = tdotx(p_transform.columns[0]);
	x1 = tdoty(p_transform.columns[0]);
	y0 = tdotx(p_transform.columns[1]);
	y1 = tdoty(p_transform.columns[1]);

	columns[0][0] = x0;
	columns[0][1] = x1;
	columns[1][0] = y0;
	columns[1][1] = y1;
}

Transform2D Transform2D::operator*(const Transform2D& p_transform) const
{
	Transform2D t = *this;
	t *= p_transform;
	return t;
}

Transform2D Transform2D::scaled(const Size2& p_scale) const
{
	// Equivalent to left multiplication
	Transform2D copy = *this;
	copy.scale(p_scale);
	return copy;
}

Transform2D Transform2D::scaled_local(const Size2& p_scale) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0] * p_scale.x, columns[1] * p_scale.y, columns[2]);
}

Transform2D Transform2D::untranslated() const
{
	Transform2D copy = *this;
	copy.columns[2] = Vector2();
	return copy;
}

Transform2D Transform2D::translated(const Vector2& p_offset) const
{
	// Equivalent to left multiplication
	return Transform2D(columns[0], columns[1], columns[2] + p_offset);
}

Transform2D Transform2D::translated_local(const Vector2& p_offset) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0], columns[1], columns[2] + basis_xform(p_offset));
}

Transform2D Transform2D::rotated(real_t p_angle) const
{
	// Equivalent to left multiplication
	return Transform2D(p_angle, Vector2()) * (*this);
}

Transform2D Transform2D::rotated_local(real_t p_angle) const
{
	// Equivalent to right multiplication
	return (*this) * Transform2D(p_angle,
						 Vector2()); // Could be optimized, because origin transform can be skipped.
}

real_t Transform2D::determinant() const
{
	return columns[0].x * columns[1].y - columns[0].y * columns[1].x;
}

Transform2D Transform2D::interpolate_with(const Transform2D& p_transform, real_t p_weight) const
{
	return Transform2D(Math::lerp_angle(get_rotation(), p_transform.get_rotation(), p_weight),
		get_scale().lerp(p_transform.get_scale(), p_weight),
		Math::lerp_angle(get_skew(), p_transform.get_skew(), p_weight),
		get_origin().lerp(p_transform.get_origin(), p_weight));
}

Transform2D::operator String() const
{
	return "[X: " + columns[0].operator String() + ", Y: " + columns[1].operator String() +
		   ", O: " + columns[2].operator String() + "]";
}

// --- transform_3d.cpp ---

void Transform3D::affine_invert() {
	basis.invert();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::affine_inverse() const {
	Transform3D ret = *this;
	ret.affine_invert();
	return ret;
}

void Transform3D::invert() {
	basis.transpose();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::inverse() const {
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform3D::affine_inverse can handle matrices with scaling, so GDScript should eventually use that.
	Transform3D ret = *this;
	ret.invert();
	return ret;
}

void Transform3D::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

Transform3D Transform3D::rotated(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to left multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(p_basis * basis, p_basis.xform(origin));
}

Transform3D Transform3D::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to right multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(basis * p_basis, origin);
}

void Transform3D::rotate_basis(const Vector3 &p_axis, real_t p_angle) {
	basis.rotate(p_axis, p_angle);
}

Transform3D Transform3D::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(origin.is_equal_approx(p_target), Transform3D(), "The transform's origin and target can't be equal.");
#endif
	Transform3D t = *this;
	t.basis = Basis::looking_at(p_target - origin, p_up, p_use_model_front);
	return t;
}

void Transform3D::set_look_at(const Vector3 &p_eye, const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(p_eye.is_equal_approx(p_target), "The eye and target vectors can't be equal.");
#endif
	basis = Basis::looking_at(p_target - p_eye, p_up, p_use_model_front);
	origin = p_eye;
}

Transform3D Transform3D::interpolate_with(const Transform3D &p_transform, real_t p_c) const {
	Transform3D interp;

	Vector3 src_scale = basis.get_scale();
	Quaternion src_rot = basis.get_rotation_quaternion();
	Vector3 src_loc = origin;

	Vector3 dst_scale = p_transform.basis.get_scale();
	Quaternion dst_rot = p_transform.basis.get_rotation_quaternion();
	Vector3 dst_loc = p_transform.origin;

	interp.basis.set_quaternion_scale(src_rot.slerp(dst_rot, p_c).normalized(), src_scale.lerp(dst_scale, p_c));
	interp.origin = src_loc.lerp(dst_loc, p_c);

	return interp;
}

void Transform3D::scale(const Vector3 &p_scale) {
	basis.scale(p_scale);
	origin *= p_scale;
}

Transform3D Transform3D::scaled(const Vector3 &p_scale) const {
	// Equivalent to left multiplication
	return Transform3D(basis.scaled(p_scale), origin * p_scale);
}

Transform3D Transform3D::scaled_local(const Vector3 &p_scale) const {
	// Equivalent to right multiplication
	return Transform3D(basis.scaled_local(p_scale), origin);
}

void Transform3D::scale_basis(const Vector3 &p_scale) {
	basis.scale(p_scale);
}

void Transform3D::translate_local(real_t p_tx, real_t p_ty, real_t p_tz) {
	translate_local(Vector3(p_tx, p_ty, p_tz));
}

void Transform3D::translate_local(const Vector3 &p_translation) {
	for (int i = 0; i < 3; i++) {
		origin[i] += basis[i].dot(p_translation);
	}
}

Transform3D Transform3D::translated(const Vector3 &p_translation) const {
	// Equivalent to left multiplication
	return Transform3D(basis, origin + p_translation);
}

Transform3D Transform3D::translated_local(const Vector3 &p_translation) const {
	// Equivalent to right multiplication
	return Transform3D(basis, origin + basis.xform(p_translation));
}

void Transform3D::orthonormalize() {
	// Orthonormalizable check is done in Basis class below.
	basis.orthonormalize();
}

Transform3D Transform3D::orthonormalized() const {
	Transform3D _copy = *this;
	_copy.orthonormalize();
	return _copy;
}

void Transform3D::orthogonalize() {
	basis.orthogonalize();
}

Transform3D Transform3D::orthogonalized() const {
	Transform3D _copy = *this;
	_copy.orthogonalize();
	return _copy;
}

bool Transform3D::is_equal_approx(const Transform3D &p_transform) const {
	return basis.is_equal_approx(p_transform.basis) && origin.is_equal_approx(p_transform.origin);
}

bool Transform3D::is_same(const Transform3D &p_transform) const {
	return basis.is_same(p_transform.basis) && origin.is_same(p_transform.origin);
}

bool Transform3D::is_finite() const {
	return basis.is_finite() && origin.is_finite();
}

void Transform3D::operator*=(const Transform3D &p_transform) {
	origin = xform(p_transform.origin);
	basis *= p_transform.basis;
}

Transform3D Transform3D::operator*(const Transform3D &p_transform) const {
	Transform3D t = *this;
	t *= p_transform;
	return t;
}

Transform3D::operator String() const {
	return "[X: " + basis.get_column(0).operator String() +
			", Y: " + basis.get_column(1).operator String() +
			", Z: " + basis.get_column(2).operator String() +
			", O: " + origin.operator String() + "]";
}

// --- projection.cpp ---

real_t Projection::determinant() const {
	return columns[0][3] * columns[1][2] * columns[2][1] * columns[3][0] - columns[0][2] * columns[1][3] * columns[2][1] * columns[3][0] -
			columns[0][3] * columns[1][1] * columns[2][2] * columns[3][0] + columns[0][1] * columns[1][3] * columns[2][2] * columns[3][0] +
			columns[0][2] * columns[1][1] * columns[2][3] * columns[3][0] - columns[0][1] * columns[1][2] * columns[2][3] * columns[3][0] -
			columns[0][3] * columns[1][2] * columns[2][0] * columns[3][1] + columns[0][2] * columns[1][3] * columns[2][0] * columns[3][1] +
			columns[0][3] * columns[1][0] * columns[2][2] * columns[3][1] - columns[0][0] * columns[1][3] * columns[2][2] * columns[3][1] -
			columns[0][2] * columns[1][0] * columns[2][3] * columns[3][1] + columns[0][0] * columns[1][2] * columns[2][3] * columns[3][1] +
			columns[0][3] * columns[1][1] * columns[2][0] * columns[3][2] - columns[0][1] * columns[1][3] * columns[2][0] * columns[3][2] -
			columns[0][3] * columns[1][0] * columns[2][1] * columns[3][2] + columns[0][0] * columns[1][3] * columns[2][1] * columns[3][2] +
			columns[0][1] * columns[1][0] * columns[2][3] * columns[3][2] - columns[0][0] * columns[1][1] * columns[2][3] * columns[3][2] -
			columns[0][2] * columns[1][1] * columns[2][0] * columns[3][3] + columns[0][1] * columns[1][2] * columns[2][0] * columns[3][3] +
			columns[0][2] * columns[1][0] * columns[2][1] * columns[3][3] - columns[0][0] * columns[1][2] * columns[2][1] * columns[3][3] -
			columns[0][1] * columns[1][0] * columns[2][2] * columns[3][3] + columns[0][0] * columns[1][1] * columns[2][2] * columns[3][3];
}

void Projection::set_identity() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = (i == j) ? 1 : 0;
		}
	}
}

void Projection::set_zero() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = 0;
		}
	}
}

Plane Projection::xform4(const Plane &p_vec4) const {
	Plane ret;

	ret.normal.x = columns[0][0] * p_vec4.normal.x + columns[1][0] * p_vec4.normal.y + columns[2][0] * p_vec4.normal.z + columns[3][0] * p_vec4.d;
	ret.normal.y = columns[0][1] * p_vec4.normal.x + columns[1][1] * p_vec4.normal.y + columns[2][1] * p_vec4.normal.z + columns[3][1] * p_vec4.d;
	ret.normal.z = columns[0][2] * p_vec4.normal.x + columns[1][2] * p_vec4.normal.y + columns[2][2] * p_vec4.normal.z + columns[3][2] * p_vec4.d;
	ret.d = columns[0][3] * p_vec4.normal.x + columns[1][3] * p_vec4.normal.y + columns[2][3] * p_vec4.normal.z + columns[3][3] * p_vec4.d;
	return ret;
}

Vector4 Projection::xform(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[1][0] * p_vec4.y + columns[2][0] * p_vec4.z + columns[3][0] * p_vec4.w,
			columns[0][1] * p_vec4.x + columns[1][1] * p_vec4.y + columns[2][1] * p_vec4.z + columns[3][1] * p_vec4.w,
			columns[0][2] * p_vec4.x + columns[1][2] * p_vec4.y + columns[2][2] * p_vec4.z + columns[3][2] * p_vec4.w,
			columns[0][3] * p_vec4.x + columns[1][3] * p_vec4.y + columns[2][3] * p_vec4.z + columns[3][3] * p_vec4.w);
}
Vector4 Projection::xform_inv(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[0][1] * p_vec4.y + columns[0][2] * p_vec4.z + columns[0][3] * p_vec4.w,
			columns[1][0] * p_vec4.x + columns[1][1] * p_vec4.y + columns[1][2] * p_vec4.z + columns[1][3] * p_vec4.w,
			columns[2][0] * p_vec4.x + columns[2][1] * p_vec4.y + columns[2][2] * p_vec4.z + columns[2][3] * p_vec4.w,
			columns[3][0] * p_vec4.x + columns[3][1] * p_vec4.y + columns[3][2] * p_vec4.z + columns[3][3] * p_vec4.w);
}

void Projection::adjust_perspective_znear(real_t p_new_znear) {
	real_t zfar = get_z_far();
	real_t znear = p_new_znear;

	real_t deltaZ = zfar - znear;
	columns[2][2] = -(zfar + znear) / deltaZ;
	columns[3][2] = -2 * znear * zfar / deltaZ;
}

Projection Projection::create_depth_correction(bool p_flip_y) {
	Projection proj;
	proj.set_depth_correction(p_flip_y);
	return proj;
}

Projection Projection::create_light_atlas_rect(const Rect2 &p_rect) {
	Projection proj;
	proj.set_light_atlas_rect(p_rect);
	return proj;
}

Projection Projection::create_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov);
	return proj;
}

Projection Projection::create_perspective_hmd(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov, p_eye, p_intraocular_dist, p_convergence_dist);
	return proj;
}

Projection Projection::create_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	Projection proj;
	proj.set_for_hmd(p_eye, p_aspect, p_intraocular_dist, p_display_width, p_display_to_lens, p_oversample, p_z_near, p_z_far);
	return proj;
}

Projection Projection::create_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	Projection proj;
	proj.set_orthogonal(p_left, p_right, p_bottom, p_top, p_znear, p_zfar);
	return proj;
}

Projection Projection::create_orthogonal_aspect(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	Projection proj;
	proj.set_orthogonal(p_size, p_aspect, p_znear, p_zfar, p_flip_fov);
	return proj;
}

Projection Projection::create_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	Projection proj;
	proj.set_frustum(p_left, p_right, p_bottom, p_top, p_near, p_far);
	return proj;
}

Projection Projection::create_frustum_aspect(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	Projection proj;
	proj.set_frustum(p_size, p_aspect, p_offset, p_near, p_far, p_flip_fov);
	return proj;
}

Projection Projection::create_fit_aabb(const AABB &p_aabb) {
	Projection proj;
	proj.scale_translate_to_fit(p_aabb);
	return proj;
}

Projection Projection::perspective_znear_adjusted(real_t p_new_znear) const {
	Projection proj = *this;
	proj.adjust_perspective_znear(p_new_znear);
	return proj;
}

Plane Projection::get_projection_plane(Planes p_plane) const {
	const real_t *matrix = (const real_t *)columns;

	switch (p_plane) {
		case PLANE_NEAR: {
			Plane new_plane = Plane(matrix[3] + matrix[2],
					matrix[7] + matrix[6],
					matrix[11] + matrix[10],
					matrix[15] + matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_FAR: {
			Plane new_plane = Plane(matrix[3] - matrix[2],
					matrix[7] - matrix[6],
					matrix[11] - matrix[10],
					matrix[15] - matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_LEFT: {
			Plane new_plane = Plane(matrix[3] + matrix[0],
					matrix[7] + matrix[4],
					matrix[11] + matrix[8],
					matrix[15] + matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_TOP: {
			Plane new_plane = Plane(matrix[3] - matrix[1],
					matrix[7] - matrix[5],
					matrix[11] - matrix[9],
					matrix[15] - matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_RIGHT: {
			Plane new_plane = Plane(matrix[3] - matrix[0],
					matrix[7] - matrix[4],
					matrix[11] - matrix[8],
					matrix[15] - matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_BOTTOM: {
			Plane new_plane = Plane(matrix[3] + matrix[1],
					matrix[7] + matrix[5],
					matrix[11] + matrix[9],
					matrix[15] + matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
	}

	return Plane();
}

Projection Projection::flipped_y() const {
	Projection proj = *this;
	proj.flip_y();
	return proj;
}

Projection Projection ::jitter_offseted(const Vector2 &p_offset) const {
	Projection proj = *this;
	proj.add_jitter_offset(p_offset);
	return proj;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t sine, cotangent, deltaZ;
	real_t radians = Math::deg_to_rad(p_fovy_degrees / 2.0);

	deltaZ = p_z_far - p_z_near;
	sine = Math::sin(radians);

	if ((deltaZ == 0) || (sine == 0) || (p_aspect == 0)) {
		return;
	}
	cotangent = Math::cos(radians) / sine;

	set_identity();

	columns[0][0] = cotangent / p_aspect;
	columns[1][1] = cotangent;
	columns[2][2] = -(p_z_far + p_z_near) / deltaZ;
	columns[2][3] = -1;
	columns[3][2] = -2 * p_z_near * p_z_far / deltaZ;
	columns[3][3] = 0;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t left, right, modeltranslation, ymax, xmax, frustumshift;

	ymax = p_z_near * std::tan(Math::deg_to_rad(p_fovy_degrees / 2.0));
	xmax = ymax * p_aspect;
	frustumshift = (p_intraocular_dist / 2.0) * p_z_near / p_convergence_dist;

	switch (p_eye) {
		case 1: { // left eye
			left = -xmax + frustumshift;
			right = xmax + frustumshift;
			modeltranslation = p_intraocular_dist / 2.0;
		} break;
		case 2: { // right eye
			left = -xmax - frustumshift;
			right = xmax - frustumshift;
			modeltranslation = -p_intraocular_dist / 2.0;
		} break;
		default: { // mono, should give the same result as set_perspective(p_fovy_degrees,p_aspect,p_z_near,p_z_far,p_flip_fov)
			left = -xmax;
			right = xmax;
			modeltranslation = 0.0;
		} break;
	}

	set_frustum(left, right, -ymax, ymax, p_z_near, p_z_far);

	// translate matrix by (modeltranslation, 0.0, 0.0)
	Projection cm;
	cm.set_identity();
	cm.columns[3][0] = modeltranslation;
	*this = *this * cm;
}

void Projection::set_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	// we first calculate our base frustum on our values without taking our lens magnification into account.
	real_t f1 = (p_intraocular_dist * 0.5) / p_display_to_lens;
	real_t f2 = ((p_display_width - p_intraocular_dist) * 0.5) / p_display_to_lens;
	real_t f3 = (p_display_width / 4.0) / p_display_to_lens;

	// now we apply our oversample factor to increase our FOV. how much we oversample is always a balance we strike between performance and how much
	// we're willing to sacrifice in FOV.
	real_t add = ((f1 + f2) * (p_oversample - 1.0)) / 2.0;
	f1 += add;
	f2 += add;
	f3 *= p_oversample;

	// always apply KEEP_WIDTH aspect ratio
	f3 /= p_aspect;

	switch (p_eye) {
		case 1: { // left eye
			set_frustum(-f2 * p_z_near, f1 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		case 2: { // right eye
			set_frustum(-f1 * p_z_near, f2 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		default: { // mono, does not apply here!
		} break;
	}
}

void Projection::set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	set_identity();

	columns[0][0] = 2.0 / (p_right - p_left);
	columns[3][0] = -((p_right + p_left) / (p_right - p_left));
	columns[1][1] = 2.0 / (p_top - p_bottom);
	columns[3][1] = -((p_top + p_bottom) / (p_top - p_bottom));
	columns[2][2] = -2.0 / (p_zfar - p_znear);
	columns[3][2] = -((p_zfar + p_znear) / (p_zfar - p_znear));
	columns[3][3] = 1.0;
}

void Projection::set_orthogonal(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_orthogonal(-p_size / 2, +p_size / 2, -p_size / p_aspect / 2, +p_size / p_aspect / 2, p_znear, p_zfar);
}

void Projection::set_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	ERR_FAIL_COND(p_right <= p_left);
	ERR_FAIL_COND(p_top <= p_bottom);
	ERR_FAIL_COND(p_far <= p_near);

	real_t *te = &columns[0][0];
	real_t x = 2 * p_near / (p_right - p_left);
	real_t y = 2 * p_near / (p_top - p_bottom);

	real_t a = (p_right + p_left) / (p_right - p_left);
	real_t b = (p_top + p_bottom) / (p_top - p_bottom);
	real_t c = -(p_far + p_near) / (p_far - p_near);
	real_t d = -2 * p_far * p_near / (p_far - p_near);

	te[0] = x;
	te[1] = 0;
	te[2] = 0;
	te[3] = 0;
	te[4] = 0;
	te[5] = y;
	te[6] = 0;
	te[7] = 0;
	te[8] = a;
	te[9] = b;
	te[10] = c;
	te[11] = -1;
	te[12] = 0;
	te[13] = 0;
	te[14] = d;
	te[15] = 0;
}

void Projection::set_frustum(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_frustum(-p_size / 2 + p_offset.x, +p_size / 2 + p_offset.x, -p_size / p_aspect / 2 + p_offset.y, +p_size / p_aspect / 2 + p_offset.y, p_near, p_far);
}

real_t Projection::get_z_far() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] - columns[3][2]) / (columns[2][3] - columns[2][2]);
}

real_t Projection::get_z_near() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] + columns[3][2]) / (columns[2][3] + columns[2][2]);
}

Vector2 Projection::get_viewport_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_near() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

Vector2 Projection::get_far_plane_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_far() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

bool Projection::get_endpoints(const Transform3D &p_transform, Vector3 *p_8points) const {
	Vector<Plane> planes = get_projection_planes(Transform3D());
	const Planes intersections[8][3] = {
		{ PLANE_FAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_FAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_BOTTOM },
	};

	for (int i = 0; i < 8; i++) {
		Vector3 point;
		Plane a = planes[intersections[i][0]];
		Plane b = planes[intersections[i][1]];
		Plane c = planes[intersections[i][2]];
		bool res = a.intersect_3(b, c, &point);
		ERR_FAIL_COND_V(!res, false);
		p_8points[i] = p_transform.xform(point);
	}

	return true;
}

Vector<Plane> Projection::get_projection_planes(const Transform3D &p_transform) const {
	/** Fast Plane Extraction from combined modelview/projection matrices.
	 * References:
	 * https://web.archive.org/web/20011221205252/https://www.markmorley.com/opengl/frustumculling.html
	 * https://web.archive.org/web/20061020020112/https://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
	 */

	Vector<Plane> planes;
	planes.resize(6);

	const real_t *matrix = (const real_t *)columns;

	Plane new_plane;

	///////--- Near Plane ---///////
	new_plane = Plane(matrix[3] + matrix[2],
			matrix[7] + matrix[6],
			matrix[11] + matrix[10],
			matrix[15] + matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[0] = p_transform.xform(new_plane);

	///////--- Far Plane ---///////
	new_plane = Plane(matrix[3] - matrix[2],
			matrix[7] - matrix[6],
			matrix[11] - matrix[10],
			matrix[15] - matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[1] = p_transform.xform(new_plane);

	///////--- Left Plane ---///////
	new_plane = Plane(matrix[3] + matrix[0],
			matrix[7] + matrix[4],
			matrix[11] + matrix[8],
			matrix[15] + matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[2] = p_transform.xform(new_plane);

	///////--- Top Plane ---///////
	new_plane = Plane(matrix[3] - matrix[1],
			matrix[7] - matrix[5],
			matrix[11] - matrix[9],
			matrix[15] - matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[3] = p_transform.xform(new_plane);

	///////--- Right Plane ---///////
	new_plane = Plane(matrix[3] - matrix[0],
			matrix[7] - matrix[4],
			matrix[11] - matrix[8],
			matrix[15] - matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[4] = p_transform.xform(new_plane);

	///////--- Bottom Plane ---///////
	new_plane = Plane(matrix[3] + matrix[1],
			matrix[7] + matrix[5],
			matrix[11] + matrix[9],
			matrix[15] + matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[5] = p_transform.xform(new_plane);

	return planes;
}

Projection Projection::inverse() const {
	Projection cm = *this;
	cm.invert();
	return cm;
}

void Projection::invert() {
	// Adapted from Mesa's `src/util/u_math.c` `util_invert_mat4x4`.
	// MIT licensed. Copyright 2008 VMware, Inc. Authored by Jacques Leroy.
	Projection temp;
	real_t *out = (real_t *)temp.columns;
	real_t *m = (real_t *)columns;

	real_t wtmp[4][8];
	real_t m0, m1, m2, m3, s;
	real_t *r0, *r1, *r2, *r3;

#define MAT(m, r, c) (m)[(c) * 4 + (r)]

	r0 = wtmp[0];
	r1 = wtmp[1];
	r2 = wtmp[2];
	r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0);
	r0[1] = MAT(m, 0, 1);
	r0[2] = MAT(m, 0, 2);
	r0[3] = MAT(m, 0, 3);
	r0[4] = 1.0;
	r0[5] = 0.0;
	r0[6] = 0.0;
	r0[7] = 0.0;

	r1[0] = MAT(m, 1, 0);
	r1[1] = MAT(m, 1, 1);
	r1[2] = MAT(m, 1, 2);
	r1[3] = MAT(m, 1, 3);
	r1[5] = 1.0;
	r1[4] = 0.0;
	r1[6] = 0.0;
	r1[7] = 0.0;

	r2[0] = MAT(m, 2, 0);
	r2[1] = MAT(m, 2, 1);
	r2[2] = MAT(m, 2, 2);
	r2[3] = MAT(m, 2, 3);
	r2[6] = 1.0;
	r2[4] = 0.0;
	r2[5] = 0.0;
	r2[7] = 0.0;

	r3[0] = MAT(m, 3, 0);
	r3[1] = MAT(m, 3, 1);
	r3[2] = MAT(m, 3, 2);
	r3[3] = MAT(m, 3, 3);

	r3[7] = 1.0;
	r3[4] = 0.0;
	r3[5] = 0.0;
	r3[6] = 0.0;

	/* choose pivot - or die */
	if (Math::abs(r3[0]) > Math::abs(r2[0])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[0]) > Math::abs(r1[0])) {
		SWAP(r2, r1);
	}
	if (Math::abs(r1[0]) > Math::abs(r0[0])) {
		SWAP(r1, r0);
	}
	ERR_FAIL_COND(0.0 == r0[0]);

	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[1]) > Math::abs(r2[1])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[1]) > Math::abs(r1[1])) {
		SWAP(r2, r1);
	}
	ERR_FAIL_COND(0.0 == r1[1]);

	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[2]) > Math::abs(r2[2])) {
		SWAP(r3, r2);
	}
	ERR_FAIL_COND(0.0 == r2[2]);

	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3];
	r3[4] -= m3 * r2[4];
	r3[5] -= m3 * r2[5];
	r3[6] -= m3 * r2[6];
	r3[7] -= m3 * r2[7];

	/* last check */
	ERR_FAIL_COND(0.0 == r3[3]);

	s = 1.0 / r3[3]; /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;

	m2 = r2[3]; /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2);
	r2[5] = s * (r2[5] - r3[5] * m2);
	r2[6] = s * (r2[6] - r3[6] * m2);
	r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1;
	r1[5] -= r3[5] * m1;
	r1[6] -= r3[6] * m1;
	r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0;
	r0[5] -= r3[5] * m0;
	r0[6] -= r3[6] * m0;
	r0[7] -= r3[7] * m0;

	m1 = r1[2]; /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1);
	r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1);
	r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0;
	r0[5] -= r2[5] * m0;
	r0[6] -= r2[6] * m0;
	r0[7] -= r2[7] * m0;

	m0 = r0[1]; /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0);
	r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0);
	r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5];
	MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7];
	MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5];
	MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7];
	MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5];
	MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7];
	MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5];
	MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

#undef MAT

	*this = temp;
}

void Projection::flip_y() {
	for (int i = 0; i < 4; i++) {
		columns[1][i] = -columns[1][i];
	}
}

bool Projection::is_same(const Projection &p_cam) const {
	return columns[0].is_same(p_cam.columns[0]) && columns[1].is_same(p_cam.columns[1]) && columns[2].is_same(p_cam.columns[2]) && columns[3].is_same(p_cam.columns[3]);
}

void Projection::set_depth_correction(bool p_flip_y, bool p_reverse_z, bool p_remap_z) {
	// p_remap_z is used to convert from OpenGL-style clip space (-1 - 1) to Vulkan style (0 - 1).
	real_t *m = &columns[0][0];

	m[0] = 1;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_flip_y ? -1 : 1;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = p_remap_z ? (p_reverse_z ? -0.5 : 0.5) : (p_reverse_z ? -1.0 : 1.0);
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = p_remap_z ? 0.5 : 0.0;
	m[15] = 1.0;
}

void Projection::set_light_bias() {
	real_t *m = &columns[0][0];

	m[0] = 0.5;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = 0.5;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 0.5;
	m[11] = 0.0;
	m[12] = 0.5;
	m[13] = 0.5;
	m[14] = 0.5;
	m[15] = 1.0;
}

void Projection::set_light_atlas_rect(const Rect2 &p_rect) {
	real_t *m = &columns[0][0];

	m[0] = p_rect.size.width;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_rect.size.height;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 1.0;
	m[11] = 0.0;
	m[12] = p_rect.position.x;
	m[13] = p_rect.position.y;
	m[14] = 0.0;
	m[15] = 1.0;
}

Projection::operator String() const {
	return "[X: " + columns[0].operator String() +
			", Y: " + columns[1].operator String() +
			", Z: " + columns[2].operator String() +
			", W: " + columns[3].operator String() + "]";
}

real_t Projection::get_aspect() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return columns[1][1] / columns[0][0];
}

int Projection::get_pixels_per_meter(int p_for_pixel_width) const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	real_t width = 2 * (-get_z_near() * columns[2][3] + columns[3][3]) / columns[0][0];
	return p_for_pixel_width / width; // Note : return type should be real_t (kept as int for compatibility for now).
}

bool Projection::is_orthogonal() const {
	// NOTE: This assumes that the matrix is a projection across z-axis
	// i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0
	return columns[2][3] == 0.0;
}

real_t Projection::get_fov() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	if (columns[2][0] == 0) {
		return Math::rad_to_deg(2 * Math::atan2(1, columns[0][0]));
	} else {
		// The frustum is asymmetrical so we need to calculate the left and right angles separately.
		real_t right = Math::atan2(columns[2][0] + 1, columns[0][0]);
		real_t left = Math::atan2(columns[2][0] - 1, columns[0][0]);
		return Math::rad_to_deg(right - left);
	}
}

real_t Projection::get_lod_multiplier() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return 2 / columns[0][0];
}

void Projection::make_scale(const Vector3 &p_scale) {
	set_identity();
	columns[0][0] = p_scale.x;
	columns[1][1] = p_scale.y;
	columns[2][2] = p_scale.z;
}

void Projection::scale_translate_to_fit(const AABB &p_aabb) {
	Vector3 min = p_aabb.position;
	Vector3 max = p_aabb.position + p_aabb.size;

	columns[0][0] = 2 / (max.x - min.x);
	columns[1][0] = 0;
	columns[2][0] = 0;
	columns[3][0] = -(max.x + min.x) / (max.x - min.x);

	columns[0][1] = 0;
	columns[1][1] = 2 / (max.y - min.y);
	columns[2][1] = 0;
	columns[3][1] = -(max.y + min.y) / (max.y - min.y);

	columns[0][2] = 0;
	columns[1][2] = 0;
	columns[2][2] = 2 / (max.z - min.z);
	columns[3][2] = -(max.z + min.z) / (max.z - min.z);

	columns[0][3] = 0;
	columns[1][3] = 0;
	columns[2][3] = 0;
	columns[3][3] = 1;
}

void Projection::add_jitter_offset(const Vector2 &p_offset) {
	columns[3][0] += p_offset.x;
	columns[3][1] += p_offset.y;
}

Projection::operator Transform3D() const {
	Transform3D tr;
	const real_t *m = &columns[0][0];

	tr.basis.rows[0][0] = m[0];
	tr.basis.rows[1][0] = m[1];
	tr.basis.rows[2][0] = m[2];

	tr.basis.rows[0][1] = m[4];
	tr.basis.rows[1][1] = m[5];
	tr.basis.rows[2][1] = m[6];

	tr.basis.rows[0][2] = m[8];
	tr.basis.rows[1][2] = m[9];
	tr.basis.rows[2][2] = m[10];

	tr.origin.x = m[12];
	tr.origin.y = m[13];
	tr.origin.z = m[14];

	return tr;
}

Projection::Projection(const Transform3D &p_transform) {
	const Transform3D &tr = p_transform;
	real_t *m = &columns[0][0];

	m[0] = tr.basis.rows[0][0];
	m[1] = tr.basis.rows[1][0];
	m[2] = tr.basis.rows[2][0];
	m[3] = 0.0;
	m[4] = tr.basis.rows[0][1];
	m[5] = tr.basis.rows[1][1];
	m[6] = tr.basis.rows[2][1];
	m[7] = 0.0;
	m[8] = tr.basis.rows[0][2];
	m[9] = tr.basis.rows[1][2];
	m[10] = tr.basis.rows[2][2];
	m[11] = 0.0;
	m[12] = tr.origin.x;
	m[13] = tr.origin.y;
	m[14] = tr.origin.z;
	m[15] = 1.0;
}

// --- transform_interpolator.cpp ---

void TransformInterpolator::interpolate_transform_2d(const Transform2D &p_prev, const Transform2D &p_curr, Transform2D &r_result, real_t p_fraction) {
	// Special case for physics interpolation, if flipping, don't interpolate basis.
	// If the determinant polarity changes, the handedness of the coordinate system changes.
	if (_sign(p_prev.determinant()) != _sign(p_curr.determinant())) {
		r_result.columns[0] = p_curr.columns[0];
		r_result.columns[1] = p_curr.columns[1];
		r_result.set_origin(p_prev.get_origin().lerp(p_curr.get_origin(), p_fraction));
		return;
	}

	r_result = p_prev.interpolate_with(p_curr, p_fraction);
}

void TransformInterpolator::interpolate_transform_3d(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis(p_prev.basis, p_curr.basis, r_result.basis, p_fraction);
}

void TransformInterpolator::interpolate_basis(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	Method method = find_method(p_prev, p_curr);
	interpolate_basis_via_method(p_prev, p_curr, r_result, p_fraction, method);
}

void TransformInterpolator::interpolate_transform_3d_via_method(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction, Method p_method) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis_via_method(p_prev.basis, p_curr.basis, r_result.basis, p_fraction, p_method);
}

void TransformInterpolator::interpolate_basis_via_method(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction, Method p_method) {
	switch (p_method) {
		default: {
			interpolate_basis_linear(p_prev, p_curr, r_result, p_fraction);
		} break;
		case INTERP_SLERP: {
			r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);
		} break;
		case INTERP_SCALED_SLERP: {
			interpolate_basis_scaled_slerp(p_prev, p_curr, r_result, p_fraction);
		} break;
	}
}

Quaternion TransformInterpolator::_basis_to_quat_unchecked(const Basis &p_basis) {
	Basis m = p_basis;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

Quaternion TransformInterpolator::_quat_slerp_unchecked(const Quaternion &p_from, const Quaternion &p_to, real_t p_fraction) {
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// Calculate cosine.
	cosom = p_from.dot(p_to);

	// Adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1.x = -p_to.x;
		to1.y = -p_to.y;
		to1.z = -p_to.z;
		to1.w = -p_to.w;
	} else {
		to1.x = p_to.x;
		to1.y = p_to.y;
		to1.z = p_to.z;
		to1.w = p_to.w;
	}

	// Calculate coefficients.

	// This check could possibly be removed as we dealt with this
	// case in the find_method() function, but is left for safety, it probably
	// isn't a bottleneck.
	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0f - p_fraction) * omega) / sinom;
		scale1 = Math::sin(p_fraction * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_fraction;
		scale1 = p_fraction;
	}
	// Calculate final values.
	return Quaternion(
			scale0 * p_from.x + scale1 * to1.x,
			scale0 * p_from.y + scale1 * to1.y,
			scale0 * p_from.z + scale1 * to1.z,
			scale0 * p_from.w + scale1 * to1.w);
}

Basis TransformInterpolator::_basis_slerp_unchecked(Basis p_from, Basis p_to, real_t p_fraction) {
	Quaternion from = _basis_to_quat_unchecked(p_from);
	Quaternion to = _basis_to_quat_unchecked(p_to);

	Basis b(_quat_slerp_unchecked(from, to, p_fraction));
	return b;
}

void TransformInterpolator::interpolate_basis_scaled_slerp(Basis p_prev, Basis p_curr, Basis &r_result, real_t p_fraction) {
	// Normalize both and find lengths.
	Vector3 lengths_prev = _basis_orthonormalize(p_prev);
	Vector3 lengths_curr = _basis_orthonormalize(p_curr);

	r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);

	// Now the result is unit length basis, we need to scale.
	Vector3 lengths_lerped = lengths_prev + ((lengths_curr - lengths_prev) * p_fraction);

	// Keep a note that the column / row order of the basis is weird,
	// so keep an eye for bugs with this.
	r_result[0] *= lengths_lerped;
	r_result[1] *= lengths_lerped;
	r_result[2] *= lengths_lerped;
}

void TransformInterpolator::interpolate_basis_linear(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	// Interpolate basis.
	r_result = p_prev.lerp(p_curr, p_fraction);

	// It turns out we need to guard against zero scale basis.
	// This is kind of silly, as we should probably fix the bugs elsewhere in Godot that can't deal with
	// zero scale, but until that time...
	for (int n = 0; n < 3; n++) {
		Vector3 &axis = r_result[n];

		// Not ok, this could cause errors due to bugs elsewhere,
		// so we will bodge set this to a small value.
		const real_t smallest = 0.0001f;
		const real_t smallest_squared = smallest * smallest;
		if (axis.length_squared() < smallest_squared) {
			// Setting a different component to the smallest
			// helps prevent the situation where all the axes are pointing in the same direction,
			// which could be a problem for e.g. cross products...
			axis[n] = smallest;
		}
	}
}

// Returns length.
real_t TransformInterpolator::_vec3_normalize(Vector3 &p_vec) {
	real_t lengthsq = p_vec.length_squared();
	if (lengthsq == 0.0f) {
		p_vec.x = p_vec.y = p_vec.z = 0.0f;
		return 0.0f;
	}
	real_t length = Math::sqrt(lengthsq);
	p_vec.x /= length;
	p_vec.y /= length;
	p_vec.z /= length;
	return length;
}

// Returns lengths.
Vector3 TransformInterpolator::_basis_orthonormalize(Basis &r_basis) {
	// Gram-Schmidt Process.

	Vector3 x = r_basis.get_column(0);
	Vector3 y = r_basis.get_column(1);
	Vector3 z = r_basis.get_column(2);

	Vector3 lengths;

	lengths.x = _vec3_normalize(x);
	y = (y - x * (x.dot(y)));
	lengths.y = _vec3_normalize(y);
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	lengths.z = _vec3_normalize(z);

	r_basis.set_column(0, x);
	r_basis.set_column(1, y);
	r_basis.set_column(2, z);

	return lengths;
}

TransformInterpolator::Method TransformInterpolator::_test_basis(Basis p_basis, bool r_needed_normalize, Quaternion &r_quat) {
	// Axis lengths.
	Vector3 al = Vector3(p_basis.get_column(0).length_squared(),
			p_basis.get_column(1).length_squared(),
			p_basis.get_column(2).length_squared());

	// Non unit scale?
	if (r_needed_normalize || !_vec3_is_equal_approx(al, Vector3(1.0, 1.0, 1.0), (real_t)0.001f)) {
		// If the basis is not normalized (at least approximately), it will fail the checks needed for slerp.
		// So we try to detect a scaled (but not sheared) basis, which we *can* slerp by normalizing first,
		// and lerping the scales separately.

		// If any of the axes are really small, it is unlikely to be a valid rotation, or is scaled too small to deal with float error.
		const real_t sl_epsilon = 0.00001f;
		if ((al.x < sl_epsilon) ||
				(al.y < sl_epsilon) ||
				(al.z < sl_epsilon)) {
			return INTERP_LERP;
		}

		// Normalize the basis.
		Basis norm_basis = p_basis;

		al.x = Math::sqrt(al.x);
		al.y = Math::sqrt(al.y);
		al.z = Math::sqrt(al.z);

		norm_basis.set_column(0, norm_basis.get_column(0) / al.x);
		norm_basis.set_column(1, norm_basis.get_column(1) / al.y);
		norm_basis.set_column(2, norm_basis.get_column(2) / al.z);

		// This doesn't appear necessary, as the later checks will catch it.
		// if (!_basis_is_orthogonal_any_scale(norm_basis)) {
		// return INTERP_LERP;
		// }

		p_basis = norm_basis;

		// Orthonormalize not necessary as normal normalization(!) works if the
		// axes are orthonormal.
		// p_basis.orthonormalize();

		// If we needed to normalize one of the two bases, we will need to normalize both,
		// regardless of whether the 2nd needs it, just to make sure it takes the path to return
		// INTERP_SCALED_LERP on the 2nd call of _test_basis.
		r_needed_normalize = true;
	}

	// Apply less stringent tests than the built in slerp, the standard Godot slerp
	// is too susceptible to float error to be useful.
	real_t det = p_basis.determinant();
	if (!Math::is_equal_approx(det, 1, (real_t)0.01f)) {
		return INTERP_LERP;
	}

	if (!_basis_is_orthogonal(p_basis)) {
		return INTERP_LERP;
	}

	// TODO: This could possibly be less stringent too, check this.
	r_quat = _basis_to_quat_unchecked(p_basis);
	if (!r_quat.is_normalized()) {
		return INTERP_LERP;
	}

	return r_needed_normalize ? INTERP_SCALED_SLERP : INTERP_SLERP;
}

// This check doesn't seem to be needed but is preserved in case of bugs.
bool TransformInterpolator::_basis_is_orthogonal_any_scale(const Basis &p_basis) {
	Vector3 cross = p_basis.get_column(0).cross(p_basis.get_column(1));
	real_t l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	const real_t epsilon = 0.9995f;

	real_t dot = cross.dot(p_basis.get_column(2));
	if (dot < epsilon) {
		return false;
	}

	cross = p_basis.get_column(1).cross(p_basis.get_column(2));
	l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	dot = cross.dot(p_basis.get_column(0));
	if (dot < epsilon) {
		return false;
	}

	return true;
}

bool TransformInterpolator::_basis_is_orthogonal(const Basis &p_basis, real_t p_epsilon) {
	Basis identity;
	Basis m = p_basis * p_basis.transposed();

	// Less stringent tests than the standard Godot slerp.
	if (!_vec3_is_equal_approx(m[0], identity[0], p_epsilon) || !_vec3_is_equal_approx(m[1], identity[1], p_epsilon) || !_vec3_is_equal_approx(m[2], identity[2], p_epsilon)) {
		return false;
	}
	return true;
}

real_t TransformInterpolator::checksum_transform_3d(const Transform3D &p_transform) {
	// just a really basic checksum, this can probably be improved
	real_t sum = _vec3_sum(p_transform.origin);
	sum -= _vec3_sum(p_transform.basis.rows[0]);
	sum += _vec3_sum(p_transform.basis.rows[1]);
	sum -= _vec3_sum(p_transform.basis.rows[2]);
	return sum;
}

TransformInterpolator::Method TransformInterpolator::find_method(const Basis &p_a, const Basis &p_b) {
	bool needed_normalize = false;

	Quaternion q0;
	Method method = _test_basis(p_a, needed_normalize, q0);
	if (method == INTERP_LERP) {
		return method;
	}

	Quaternion q1;
	method = _test_basis(p_b, needed_normalize, q1);
	if (method == INTERP_LERP) {
		return method;
	}

	// Are they close together?
	// Apply the same test that will revert to lerp as is present in the slerp routine.
	// Calculate cosine.
	real_t cosom = Math::abs(q0.dot(q1));
	if ((1.0f - cosom) <= (real_t)CMP_EPSILON) {
		return INTERP_LERP;
	}

	return method;
}
