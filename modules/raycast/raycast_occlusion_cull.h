/**************************************************************************/
/*  raycast_occlusion_cull.h                                              */
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

#include "servers/rendering/renderer_scene_occlusion_cull.h"

struct Projection;

class RaycastOcclusionCull final
{
public:
	using HZBuffer = RendererSceneOcclusionCull::HZBuffer;
	struct Data;

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

private:
	static void _init_embree();
	static Vector2 _get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size);

public:
	static bool is_occluder(RID p_rid);
	static RID occluder_allocate();
	static void occluder_initialize(RID p_occluder);
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);
	static void free_occluder(RID p_occluder);

	static void add_scenario(RID p_scenario);
	static void remove_scenario(RID p_scenario);
	static void scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled);
	static void scenario_remove_instance(RID p_scenario, RID p_instance);

	static void add_buffer(RID p_buffer);
	static void remove_buffer(RID p_buffer);
	static HZBuffer* buffer_get_ptr(RID p_buffer);
	static void buffer_set_scenario(RID p_buffer, RID p_scenario);
	static void buffer_set_size(RID p_buffer, const Vector2i& p_size);
	static void buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal);

	static RID buffer_get_debug_texture(RID p_buffer);

	static void set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality);

	RaycastOcclusionCull() = delete;
	RaycastOcclusionCull(const RaycastOcclusionCull&) = delete;
	RaycastOcclusionCull& operator=(const RaycastOcclusionCull&) = delete;
	~RaycastOcclusionCull() = delete;
};
