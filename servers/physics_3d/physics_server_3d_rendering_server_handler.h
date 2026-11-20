/**************************************************************************/
/*  physics_server_3d_rendering_server_handler.h                          */
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

#pragma once

#include "core/math/aabb.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"

class PhysicsServer3DRenderingServerHandler final
{
	RID mesh;
	int surface = 0;
	Vector<uint8_t> buffer;
	uint32_t stride = 0;
	uint32_t normal_stride = 0;
	uint32_t offset_vertices = 0;
	uint32_t offset_normal = 0;

	uint8_t* write_buffer = nullptr;

public:
	PhysicsServer3DRenderingServerHandler() = default;
	~PhysicsServer3DRenderingServerHandler() = default;

	bool is_ready(RID p_mesh_rid) const { return mesh.is_valid() && mesh == p_mesh_rid; }

	void prepare(RID p_mesh_rid, int p_surface);
	void clear();
	void open();
	void close();
	void commit_changes();

	void set_vertex(int p_vertex_id, const Vector3& p_vertex);
	void set_normal(int p_vertex_id, const Vector3& p_normal);
	void set_aabb(const AABB& p_aabb);
};
