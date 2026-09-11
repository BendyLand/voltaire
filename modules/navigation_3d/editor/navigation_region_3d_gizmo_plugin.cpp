/**************************************************************************/
/*  navigation_region_3d_gizmo_plugin.cpp                                 */
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

#include "core/math/random_pcg.h"
#include "navigation_region_3d_gizmo_plugin.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "servers/navigation_3d/navigation_server_3d.h"

NavigationRegion3DGizmoPlugin::NavigationRegion3DGizmoPlugin()
{
	create_material("face_material",
		NavigationServer3D::get_singleton()->get_debug_navigation_geometry_face_color(), false,
		false, true);
	create_material("face_material_disabled",
		NavigationServer3D::get_singleton()->get_debug_navigation_geometry_face_disabled_color(),
		false, false, true);
	create_material("edge_material",
		NavigationServer3D::get_singleton()->get_debug_navigation_geometry_edge_color());
	create_material("edge_material_disabled",
		NavigationServer3D::get_singleton()->get_debug_navigation_geometry_edge_disabled_color());

	Color baking_aabb_material_color = Color(0.8, 0.5, 0.7);
	baking_aabb_material_color.a = 0.1;
	create_material("baking_aabb_material", baking_aabb_material_color);
}

String NavigationRegion3DGizmoPlugin::get_gizmo_name() const { return "NavigationRegion3D"; }


