/**************************************************************************/
/*  mesh_data_tool.cpp                                                    */
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

#include "mesh_data_tool.compat.inc"
#include "mesh_data_tool.h"

void MeshDataTool::clear()
{
	vertices.clear();
	edges.clear();
	faces.clear();
	material = Ref<Material>();
	format = 0;
}

uint64_t MeshDataTool::get_format() const { return format; }

int MeshDataTool::get_vertex_count() const { return vertices.size(); }

int MeshDataTool::get_edge_count() const { return edges.size(); }

int MeshDataTool::get_face_count() const { return faces.size(); }

Vector3 MeshDataTool::get_vertex(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector3());
	return vertices[p_idx].vertex;
}

void MeshDataTool::set_vertex(int p_idx, const Vector3& p_vertex)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].vertex = p_vertex;
}

Vector3 MeshDataTool::get_vertex_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector3());
	return vertices[p_idx].normal;
}

void MeshDataTool::set_vertex_normal(int p_idx, const Vector3& p_normal)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].normal = p_normal;
	format |= Mesh::ARRAY_FORMAT_NORMAL;
}

Plane MeshDataTool::get_vertex_tangent(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Plane());
	return vertices[p_idx].tangent;
}

void MeshDataTool::set_vertex_tangent(int p_idx, const Plane& p_tangent)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].tangent = p_tangent;
	format |= Mesh::ARRAY_FORMAT_TANGENT;
}

Vector2 MeshDataTool::get_vertex_uv(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector2());
	return vertices[p_idx].uv;
}

void MeshDataTool::set_vertex_uv(int p_idx, const Vector2& p_uv)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].uv = p_uv;
	format |= Mesh::ARRAY_FORMAT_TEX_UV;
}

Vector2 MeshDataTool::get_vertex_uv2(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector2());
	return vertices[p_idx].uv2;
}

void MeshDataTool::set_vertex_uv2(int p_idx, const Vector2& p_uv2)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].uv2 = p_uv2;
	format |= Mesh::ARRAY_FORMAT_TEX_UV2;
}

Color MeshDataTool::get_vertex_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Color());
	return vertices[p_idx].color;
}

void MeshDataTool::set_vertex_color(int p_idx, const Color& p_color)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].color = p_color;
	format |= Mesh::ARRAY_FORMAT_COLOR;
}

Vector<int> MeshDataTool::get_vertex_bones(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].bones;
}

void MeshDataTool::set_vertex_bones(int p_idx, const Vector<int>& p_bones)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	ERR_FAIL_COND(p_bones.size() != 4);
	vertices.write[p_idx].bones = p_bones;
	format |= Mesh::ARRAY_FORMAT_BONES;
}

Vector<float> MeshDataTool::get_vertex_weights(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<float>());
	return vertices[p_idx].weights;
}

void MeshDataTool::set_vertex_weights(int p_idx, const Vector<float>& p_weights)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	ERR_FAIL_COND(p_weights.size() != 4);
	vertices.write[p_idx].weights = p_weights;
	format |= Mesh::ARRAY_FORMAT_WEIGHTS;
}

Vector<int> MeshDataTool::get_vertex_edges(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].edges;
}

Vector<int> MeshDataTool::get_vertex_faces(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].faces;
}

int MeshDataTool::get_edge_vertex(int p_edge, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_edge, edges.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 2, -1);
	return edges[p_edge].vertex[p_vertex];
}

Vector<int> MeshDataTool::get_edge_faces(int p_edge) const
{
	ERR_FAIL_INDEX_V(p_edge, edges.size(), Vector<int>());
	return edges[p_edge].faces;
}

int MeshDataTool::get_face_vertex(int p_face, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 3, -1);
	return faces[p_face].v[p_vertex];
}

int MeshDataTool::get_face_edge(int p_face, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 3, -1);
	return faces[p_face].edges[p_vertex];
}

Vector3 MeshDataTool::get_face_normal(int p_face) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), Vector3());
	Vector3 v0 = vertices[faces[p_face].v[0]].vertex;
	Vector3 v1 = vertices[faces[p_face].v[1]].vertex;
	Vector3 v2 = vertices[faces[p_face].v[2]].vertex;

	return Plane(v0, v1, v2).normal;
}

Ref<Material> MeshDataTool::get_material() const { return material; }

void MeshDataTool::set_material(const Ref<Material>& p_material) { material = p_material; }

MeshDataTool::MeshDataTool() { clear(); }


