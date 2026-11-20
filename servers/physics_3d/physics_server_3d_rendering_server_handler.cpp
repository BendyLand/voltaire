/**************************************************************************/
/*  physics_server_3d_rendering_server_handler.cpp                        */
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

#include "physics_server_3d_rendering_server_handler.h"

#include "servers/rendering/renderer.h"

void PhysicsServer3DRenderingServerHandler::prepare(RID p_mesh_rid, int p_surface)
{
	clear();

	ERR_FAIL_COND(!p_mesh_rid.is_valid());

	mesh = p_mesh_rid;
	surface = p_surface;

	RenderingServerTypes::SurfaceData surface_data = Renderer::mesh_get_surface(mesh, surface);

	uint32_t surface_offsets[RSE::ARRAY_MAX];
	uint32_t vertex_stride;
	uint32_t normal_tangent_stride;
	uint32_t attrib_stride;
	uint32_t skin_stride;
	Renderer::mesh_surface_make_offsets_from_format(surface_data.format,
		surface_data.vertex_count, surface_data.index_count, surface_offsets, vertex_stride,
		normal_tangent_stride, attrib_stride, skin_stride);

	buffer = surface_data.vertex_data;
	stride = vertex_stride;
	normal_stride = normal_tangent_stride;
	offset_vertices = surface_offsets[RSE::ARRAY_VERTEX];
	offset_normal = surface_offsets[RSE::ARRAY_NORMAL];
}

void PhysicsServer3DRenderingServerHandler::clear()
{
	buffer.resize(0);
	stride = 0;
	normal_stride = 0;
	offset_vertices = 0;
	offset_normal = 0;

	surface = 0;
	mesh = RID();
}

void PhysicsServer3DRenderingServerHandler::open() { write_buffer = buffer.ptrw(); }

void PhysicsServer3DRenderingServerHandler::close() { write_buffer = nullptr; }

void PhysicsServer3DRenderingServerHandler::commit_changes()
{
	Renderer::mesh_surface_update_vertex_region(mesh, surface, 0, buffer);
}

void PhysicsServer3DRenderingServerHandler::set_vertex(int p_vertex_id, const Vector3& p_vertex)
{
	ERR_FAIL_NULL(write_buffer);
	float* vertex_buffer =
		reinterpret_cast<float*>(write_buffer + p_vertex_id * stride + offset_vertices);
	*vertex_buffer++ = (float)p_vertex.x;
	*vertex_buffer++ = (float)p_vertex.y;
	*vertex_buffer++ = (float)p_vertex.z;
}

void PhysicsServer3DRenderingServerHandler::set_normal(int p_vertex_id, const Vector3& p_normal)
{
	ERR_FAIL_NULL(write_buffer);
	Vector2 res = p_normal.octahedron_encode();
	uint32_t value = 0;
	value |= (uint16_t)CLAMP(res.x * 65535, 0, 65535);
	value |= (uint16_t)CLAMP(res.y * 65535, 0, 65535) << 16;
	memcpy(&write_buffer[p_vertex_id * normal_stride + offset_normal], &value, sizeof(uint32_t));
}

void PhysicsServer3DRenderingServerHandler::set_aabb(const AABB& p_aabb)
{
	Renderer::mesh_set_custom_aabb(mesh, p_aabb);
}
