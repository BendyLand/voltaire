/**************************************************************************/
/*  collision_polygon_3d_gizmo_plugin.cpp                                 */
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

#include "collision_polygon_3d_gizmo_plugin.h"
#include "core/math/geometry_2d.h"
#include "scene/3d/physics/collision_polygon_3d.h"

CollisionPolygon3DGizmoPlugin::CollisionPolygon3DGizmoPlugin()
{
	create_collision_material("shape_material", 2.0);
	create_collision_material("shape_material_arraymesh", 0.0625);

	create_collision_material("shape_material_disabled", 0.0625);
	create_collision_material("shape_material_arraymesh_disabled", 0.015625);
}

void CollisionPolygon3DGizmoPlugin::create_collision_material(const String& p_name, float p_alpha)
{
	Vector<Ref<StandardMaterial3D>> mats;

	const Color collision_color(1.0, 1.0, 1.0, p_alpha);

	for (int i = 0; i < 4; i++) {
		bool instantiated = i < 2;

		Ref<StandardMaterial3D> material;
		material.instantiate();

		Color color = collision_color;
		color.a *= instantiated ? 0.25 : 1.0;

		material->set_albedo(color);
		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
		material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 1);
		material->set_cull_mode(StandardMaterial3D::CULL_BACK);
		material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
		material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);

		mats.push_back(material);
	}

	materials[p_name] = mats;
}

String CollisionPolygon3DGizmoPlugin::get_gizmo_name() const { return "CollisionPolygon3D"; }


