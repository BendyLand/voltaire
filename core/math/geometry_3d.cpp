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

#include "servers/rendering/renderer_scene_cull.h"

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
#include "triangle_mesh.h"

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

#include "a_star.h"
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
