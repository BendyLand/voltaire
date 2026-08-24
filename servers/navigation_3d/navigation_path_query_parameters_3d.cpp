/**************************************************************************/
/*  navigation_path_query_parameters_3d.cpp                               */
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

#include "core/object/class_db.h"
#include "core/variant/typed_array.h"
#include "navigation_path_query_parameters_3d.h"

void NavigationPathQueryParameters3D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	pathfinding_algorithm = p_pathfinding_algorithm;
}

NavigationPathQueryParameters3D::PathfindingAlgorithm
NavigationPathQueryParameters3D::get_pathfinding_algorithm() const
{
	return pathfinding_algorithm;
}

void NavigationPathQueryParameters3D::set_path_postprocessing(
	const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing)
{
	path_postprocessing = p_path_postprocessing;
}

NavigationPathQueryParameters3D::PathPostProcessing
NavigationPathQueryParameters3D::get_path_postprocessing() const
{
	return path_postprocessing;
}

void NavigationPathQueryParameters3D::set_map(RID p_map) { map = p_map; }

RID NavigationPathQueryParameters3D::get_map() const { return map; }

void NavigationPathQueryParameters3D::set_start_position(Vector3 p_start_position)
{
	start_position = p_start_position;
}

Vector3 NavigationPathQueryParameters3D::get_start_position() const { return start_position; }

void NavigationPathQueryParameters3D::set_target_position(Vector3 p_target_position)
{
	target_position = p_target_position;
}

Vector3 NavigationPathQueryParameters3D::get_target_position() const { return target_position; }

void NavigationPathQueryParameters3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	navigation_layers = p_navigation_layers;
}

uint32_t NavigationPathQueryParameters3D::get_navigation_layers() const
{
	return navigation_layers;
}

void NavigationPathQueryParameters3D::set_metadata_flags(
	BitField<NavigationPathQueryParameters3D::PathMetadataFlags> p_flags)
{
	metadata_flags = (int64_t)p_flags;
}

BitField<NavigationPathQueryParameters3D::PathMetadataFlags>
NavigationPathQueryParameters3D::get_metadata_flags() const
{
	return (int64_t)metadata_flags;
}

void NavigationPathQueryParameters3D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
}

bool NavigationPathQueryParameters3D::get_simplify_path() const { return simplify_path; }

void NavigationPathQueryParameters3D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
}

real_t NavigationPathQueryParameters3D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationPathQueryParameters3D::set_included_regions(const TypedArray<RID>& p_regions)
{
	_included_regions.resize(p_regions.size());
	for (uint32_t i = 0; i < _included_regions.size(); i++) {
		_included_regions[i] = p_regions[i];
	}
}

TypedArray<RID> NavigationPathQueryParameters3D::get_included_regions() const
{
	TypedArray<RID> r_regions;
	r_regions.resize(_included_regions.size());
	for (uint32_t i = 0; i < _included_regions.size(); i++) {
		r_regions[i] = _included_regions[i];
	}
	return r_regions;
}

void NavigationPathQueryParameters3D::set_excluded_regions(const TypedArray<RID>& p_regions)
{
	_excluded_regions.resize(p_regions.size());
	for (uint32_t i = 0; i < _excluded_regions.size(); i++) {
		_excluded_regions[i] = p_regions[i];
	}
}

TypedArray<RID> NavigationPathQueryParameters3D::get_excluded_regions() const
{
	TypedArray<RID> r_regions;
	r_regions.resize(_excluded_regions.size());
	for (uint32_t i = 0; i < _excluded_regions.size(); i++) {
		r_regions[i] = _excluded_regions[i];
	}
	return r_regions;
}

void NavigationPathQueryParameters3D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
}

float NavigationPathQueryParameters3D::get_path_return_max_length() const
{
	return path_return_max_length;
}

void NavigationPathQueryParameters3D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
}

float NavigationPathQueryParameters3D::get_path_return_max_radius() const
{
	return path_return_max_radius;
}

void NavigationPathQueryParameters3D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
}

int NavigationPathQueryParameters3D::get_path_search_max_polygons() const
{
	return path_search_max_polygons;
}

void NavigationPathQueryParameters3D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
}

float NavigationPathQueryParameters3D::get_path_search_max_distance() const
{
	return path_search_max_distance;
}

void NavigationPathQueryParameters3D::_bind_methods() {}


