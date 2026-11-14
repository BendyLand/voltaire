/**************************************************************************/
/*  fog.h                                                                 */
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
#include "servers/rendering/environment/renderer_fog.h"

namespace RendererDummy
{

class Fog final
{
public:
	Fog() = delete;
	~Fog() = delete;
	Fog(const Fog&) = delete;
	Fog& operator=(const Fog&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* FOG VOLUMES */

	static bool owns_fog_volume(RID p_rid) { return false; }

	static RID fog_volume_allocate() { return RID(); }

	static void fog_volume_initialize(RID p_rid) {}

	static void fog_volume_free(RID p_rid) {}

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape) {}

	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size) {}

	static void fog_volume_set_material(RID p_fog_volume, RID p_material) {}

	static AABB fog_volume_get_aabb(RID p_fog_volume) { return AABB(); }

	static RSE::FogVolumeShape fog_volume_get_shape(RID p_fog_volume)
	{
		return RSE::FOG_VOLUME_SHAPE_ELLIPSOID;
	}
};

} // namespace RendererDummy
