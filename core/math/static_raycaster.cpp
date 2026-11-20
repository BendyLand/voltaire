/**************************************************************************/
/*  static_raycaster.cpp                                                  */
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

#include "static_raycaster.h"

#include "modules/modules_enabled.gen.h"

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
