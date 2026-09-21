/**************************************************************************/
/*  static_body_3d.cpp                                                    */
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

#include "scene/resources/physics_material.h"
#include "static_body_3d.h"

#ifndef NAVIGATION_3D_DISABLED
#include "core/math/convex_hull.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/height_map_shape_3d.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"

RID StaticBody3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

Ref<PhysicsMaterial> StaticBody3D::get_physics_material_override() const
{
	return physics_material_override;
}

Vector3 StaticBody3D::get_constant_linear_velocity() const { return constant_linear_velocity; }

Vector3 StaticBody3D::get_constant_angular_velocity() const { return constant_angular_velocity; }

StaticBody3D::StaticBody3D(PS3DE::BodyMode p_mode) : PhysicsBody3D(p_mode) {}


