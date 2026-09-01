/**************************************************************************/
/*  nav_region_2d.cpp                                                     */
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

#include "2d/nav_mesh_queries_2d.h"
#include "2d/nav_region_builder_2d.h"
#include "2d/nav_region_iteration_2d.h"
#include "core/config/project_settings.h"
#include "nav_map_2d.h"
#include "nav_region_2d.h"

using namespace Nav2D;

void NavRegion2D::set_map(NavMap2D* p_map)
{
	if (map == p_map) {
		return;
	}

	cancel_async_thread_join();
	cancel_sync_request();

	if (map) {
		map->remove_region(this);
	}

	map = p_map;
	iteration_dirty = true;

	if (map) {
		map->add_region(this);
		request_sync();
	}
}

void NavRegion2D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections != p_enabled) {
		use_edge_connections = p_enabled;
		iteration_dirty = true;
	}

	request_sync();
}

void NavRegion2D::set_transform(Transform2D p_transform)
{
	if (transform == p_transform) {
		return;
	}
	transform = p_transform;
	iteration_dirty = true;

	request_sync();
}

ClosestPointQueryResult NavRegion2D::get_closest_point_info(const Vector2& p_point) const
{
	RWLockRead read_lock(region_rwlock);

	return NavMeshQueries2D::polygons_get_closest_point_info(get_polygons(), p_point);
}

Vector2 NavRegion2D::get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const
{
	RWLockRead read_lock(region_rwlock);

	if (!get_enabled()) {
		return Vector2();
	}

	return NavMeshQueries2D::polygons_get_random_point(
		get_polygons(), p_navigation_layers, p_uniformly);
}

void NavRegion2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}
	navigation_layers = p_navigation_layers;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_enter_cost(real_t p_enter_cost)
{
	real_t new_enter_cost = MAX(p_enter_cost,
 0.0);
	if (enter_cost == new_enter_cost) {
		return;
	}
	enter_cost = new_enter_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_travel_cost(real_t p_travel_cost)
{
	real_t new_travel_cost = MAX(p_travel_cost, 0.0);
	if (travel_cost == new_travel_cost) {
		return;
	}
	travel_cost = new_travel_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::scratch_polygons()
{
	iteration_dirty = true;

	request_sync();
}

real_t NavRegion2D::get_surface_area() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_surface_area();
}

Rect2 NavRegion2D::get_bounds() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_bounds();
}

const LocalVector<Nav2D::Polygon>& NavRegion2D::get_polygons() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_navmesh_polygons();
}

bool NavRegion2D::sync()
{
	bool requires_map_update = false;
	if (!map) {
		return requires_map_update;
	}

	if (iteration_dirty && !iteration_building && !iteration_ready) {
		_build_iteration();
	}

	if (iteration_ready) {
		_sync_iteration();
		requires_map_update = true;
	}

	return requires_map_update;
}

void NavRegion2D::_build_iteration_threaded(void* p_arg)
{
	NavRegionIterationBuild2D* _iteration_build = static_cast<NavRegionIterationBuild2D*>(p_arg);

	NavRegionBuilder2D::build_iteration(*_iteration_build);
}

void NavRegion2D::_sync_iteration()
{
	if (iteration_building || !iteration_ready) {
		return;
	}

	performance_data.pm_polygon_count = iteration_build.performance_data.pm_polygon_count;
	performance_data.pm_edge_count = iteration_build.performance_data.pm_edge_count;
	performance_data.pm_edge_merge_count = iteration_build.performance_data.pm_edge_merge_count;

	RWLockWrite write_lock(iteration_rwlock);
	ERR_FAIL_COND(iteration.is_null());
	iteration = Ref<NavRegionIteration2D>();
	DEV_ASSERT(iteration.is_null());
	iteration = iteration_build.region_iteration;
	iteration_build.region_iteration = Ref<NavRegionIteration2D>();
	DEV_ASSERT(iteration_build.region_iteration.is_null());
	iteration_id = iteration_id % UINT32_MAX + 1;

	iteration_ready = false;

	cancel_async_thread_join();
}

Ref<NavRegionIteration2D> NavRegion2D::get_iteration()
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration;
}

void NavRegion2D::set_use_async_iterations(bool p_enabled)
{
	if (use_async_iterations == p_enabled) {
		return;
	}
#ifdef THREADS_ENABLED
	use_async_iterations = p_enabled;
#endif
}

bool NavRegion2D::get_use_async_iterations() const { return use_async_iterations; }

NavRegion2D::NavRegion2D() : sync_dirty_request_list_element(this), async_list_element(this)
{
	type = NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_REGION;
	iteration_build.region = this;
	iteration.instantiate();

#ifndef THREADS_ENABLED
	use_async_iterations = false;
#endif
}

NavRegion2D::~NavRegion2D()
{
	cancel_async_thread_join();
	cancel_sync_request();

	iteration_build.region = nullptr;
	iteration_build.region_iteration = Ref<NavRegionIteration2D>();
	iteration = Ref<NavRegionIteration2D>();
}


