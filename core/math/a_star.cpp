/**************************************************************************/
/*  a_star.cpp                                                            */
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

#include "a_star.h"
#include "core/math/geometry_3d.h"

int64_t AStar3D::get_available_point_id() const
{
	if (points.has(last_free_id)) {
		int64_t cur_new_id = last_free_id + 1;
		while (points.has(cur_new_id)) {
			cur_new_id++;
		}
		last_free_id = cur_new_id;
	}

	return last_free_id;
}

void AStar3D::add_point(int64_t p_id, const Vector3& p_pos, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(p_id < 0, vformat("Can't add a point with negative id: %d.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't add a point with weight scale less than 0.0: %f.", p_weight_scale));

	Point** point_entry = points.getptr(p_id);

	if (!point_entry) {
		Point* pt = memnew(Point);
		pt->id = p_id;
		pt->pos = p_pos;
		pt->weight_scale = p_weight_scale;
		pt->prev_point = nullptr;
		pt->open_pass = 0;
		pt->closed_pass = 0;
		pt->enabled = true;
		points.insert_new(p_id, pt);
	}
	else {
		Point* found_pt = *point_entry;
		found_pt->pos = p_pos;
		found_pt->weight_scale = p_weight_scale;
	}
}

Vector3 AStar3D::get_point_position(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, Vector3(),
		vformat("Can't get point's position. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->pos;
}

void AStar3D::set_point_position(int64_t p_id, const Vector3& p_pos)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's position. Point with id: %d doesn't exist.", p_id));

	(*point_entry)->pos = p_pos;
}

real_t AStar3D::get_point_weight_scale(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, 0,
		vformat("Can't get point's weight scale. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->weight_scale;
}

void AStar3D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's weight scale. Point with id: %d doesn't exist.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	(*point_entry)->weight_scale = p_weight_scale;
}

void AStar3D::remove_point(int64_t p_id)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!point_entry, vformat("Can't remove point. Point with id: %d doesn't exist.", p_id));
	Point* p = *point_entry;

	for (KeyValue<int64_t, Point*>& kv : p->neighbors) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	for (KeyValue<int64_t, Point*>& kv : p->unlinked_neighbours) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	memdelete(p);
	points.erase(p_id);
	last_free_id = p_id;
}

void AStar3D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	ERR_FAIL_COND_MSG(
		p_id == p_with_id, vformat("Can't connect point with id: %d to itself.", p_id));

	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	a->neighbors.insert(b->id, b);

	if (p_bidirectional) {
		b->neighbors.insert(a->id, a);
	}
	else {
		b->unlinked_neighbours.insert(a->id, a);
	}

	Segment s(p_id, p_with_id);
	if (p_bidirectional) {
		s.direction = Segment::BIDIRECTIONAL;
	}

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		s.direction |= element->direction;
		if (s.direction == Segment::BIDIRECTIONAL) {
			// Both are neighbors of each other now
			a->unlinked_neighbours.erase(b->id);
			b->unlinked_neighbours.erase(a->id);
		}
		segments.remove(element);
	}

	segments.insert(s);
}

void AStar3D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	Segment s(p_id, p_with_id);
	int remove_direction = p_bidirectional ? (int)Segment::BIDIRECTIONAL : (int)s.direction;

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		// s is the new segment
		// Erase the directions to be removed
		s.direction = (element->direction & ~remove_direction);

		a->neighbors.erase(b->id);
		if (p_bidirectional) {
			b->neighbors.erase(a->id);
			if (element->direction != Segment::BIDIRECTIONAL) {
				a->unlinked_neighbours.erase(b->id);
				b->unlinked_neighbours.erase(a->id);
			}
		}
		else {
			if (s.direction == Segment::NONE) {
				b->unlinked_neighbours.erase(a->id);
			}
			else {
				a->unlinked_neighbours.insert(b->id, b);
			}
		}

		segments.remove(element);
		if (s.direction != Segment::NONE) {
			segments.insert(s);
		}
	}
}

bool AStar3D::has_point(int64_t p_id) const { return points.has(p_id); }

PackedInt64Array AStar3D::get_point_ids()
{
	PackedInt64Array point_list;

	for (KeyValue<int64_t, Point*>& kv : points) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

Vector<int64_t> AStar3D::get_point_connections(int64_t p_id)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, Vector<int64_t>(),
		vformat("Can't get point's connections. Point with id: %d doesn't exist.", p_id));

	Vector<int64_t> point_list;

	for (KeyValue<int64_t, Point*>& kv : (*p_entry)->neighbors) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

bool AStar3D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	Segment s(p_id, p_with_id);
	const HashSet<Segment, Segment>::Iterator element = segments.find(s);

	return element && (p_bidirectional || (element->direction & s.direction) == s.direction);
}

void AStar3D::clear()
{
	last_free_id = 0;
	for (KeyValue<int64_t, Point*>& kv : points) {
		memdelete(kv.value);
	}
	segments.clear();
	points.clear();
}

int64_t AStar3D::get_point_count() const { return points.size(); }

int64_t AStar3D::get_point_capacity() const { return points.get_capacity(); }

void AStar3D::reserve_space(int64_t p_num_nodes)
{
	ERR_FAIL_COND_MSG(p_num_nodes <= 0,
		vformat("New capacity must be greater than 0, new was: %d.", p_num_nodes));
	points.reserve(p_num_nodes);
}

int64_t AStar3D::get_closest_point(const Vector3& p_point, bool p_include_disabled) const
{
	int64_t closest_id = -1;
	real_t closest_dist = 1e20;

	for (const KeyValue<int64_t, Point*>& kv : points) {
		if (!p_include_disabled && !kv.value->enabled) {
			continue; // Disabled points should not be considered.
		}

		// Keep the closest point's ID, and in case of multiple closest IDs,
		// the smallest one (makes it deterministic).
		real_t d = p_point.distance_squared_to(kv.value->pos);
		int64_t id = kv.key;
		if (d <= closest_dist) {
			if (d == closest_dist && id > closest_id) { // Keep lowest ID.
				continue;
			}
			closest_dist = d;
			closest_id = id;
		}
	}

	return closest_id;
}

Vector3 AStar3D::get_closest_position_in_segment(const Vector3& p_point) const
{
	real_t closest_dist = 1e20;
	Vector3 closest_point;

	for (const Segment& E : segments) {
		const Point* from_point = *points.getptr(E.key.first);
		const Point* to_point = *points.getptr(E.key.second);

		if (!(from_point->enabled && to_point->enabled)) {
			continue;
		}

		Vector3 p =
			Geometry3D::get_closest_point_to_segment(p_point, from_point->pos, to_point->pos);
		real_t d = p_point.distance_squared_to(p);
		if (d < closest_dist) {
			closest_point = p;
			closest_dist = d;
		}
	}

	return closest_point;
}

bool AStar3D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		for (const KeyValue<int64_t, Point*>& kv : p->neighbors) {
			Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStar3D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** end_entry = points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar3D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** to_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector3> AStar3D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector3>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector3> path;
	path.resize(pc);

	{
		Vector3* w = path.ptrw();

		Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = p2->pos;
			p2 = p2->prev_point;
		}

		w[0] = p2->pos; // Assign first
	}

	return path;
}

Vector<int64_t> AStar3D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar3D::is_neighbor_filter_enabled() const { return neighbor_filter_enabled; }

void AStar3D::set_neighbor_filter_enabled(bool p_enabled) { neighbor_filter_enabled = p_enabled; }

void AStar3D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!p_entry,
		vformat("Can't set if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	p->enabled = !p_disabled;
}

bool AStar3D::is_point_disabled(int64_t p_id) const
{
	Point* const* p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, false,
		vformat("Can't get if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	return !p->enabled;
}

AStar3D::~AStar3D() { clear(); }

int64_t AStar2D::get_available_point_id() const { return astar.get_available_point_id(); }

void AStar2D::add_point(int64_t p_id, const Vector2& p_pos, real_t p_weight_scale)
{
	astar.add_point(p_id, Vector3(p_pos.x, p_pos.y, 0), p_weight_scale);
}

Vector2 AStar2D::get_point_position(int64_t p_id) const
{
	Vector3 p = astar.get_point_position(p_id);
	return Vector2(p.x, p.y);
}

void AStar2D::set_point_position(int64_t p_id, const Vector2& p_pos)
{
	astar.set_point_position(p_id, Vector3(p_pos.x, p_pos.y, 0));
}

real_t AStar2D::get_point_weight_scale(int64_t p_id) const
{
	return astar.get_point_weight_scale(p_id);
}

void AStar2D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	astar.set_point_weight_scale(p_id, p_weight_scale);
}

void AStar2D::remove_point(int64_t p_id) { astar.remove_point(p_id); }

bool AStar2D::has_point(int64_t p_id) const { return astar.has_point(p_id); }

Vector<int64_t> AStar2D::get_point_connections(int64_t p_id)
{
	return astar.get_point_connections(p_id);
}

PackedInt64Array AStar2D::get_point_ids() { return astar.get_point_ids(); }

bool AStar2D::is_neighbor_filter_enabled() const { return astar.neighbor_filter_enabled; }

void AStar2D::set_neighbor_filter_enabled(bool p_enabled)
{
	astar.neighbor_filter_enabled = p_enabled;
}

void AStar2D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	astar.set_point_disabled(p_id, p_disabled);
}

bool AStar2D::is_point_disabled(int64_t p_id) const { return astar.is_point_disabled(p_id); }

void AStar2D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.connect_points(p_id, p_with_id, p_bidirectional);
}

void AStar2D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.disconnect_points(p_id, p_with_id, p_bidirectional);
}

bool AStar2D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	return astar.are_points_connected(p_id, p_with_id, p_bidirectional);
}

int64_t AStar2D::get_point_count() const { return astar.get_point_count(); }

int64_t AStar2D::get_point_capacity() const { return astar.get_point_capacity(); }

void AStar2D::clear() { astar.clear(); }

void AStar2D::reserve_space(int64_t p_num_nodes) { astar.reserve_space(p_num_nodes); }

int64_t AStar2D::get_closest_point(const Vector2& p_point, bool p_include_disabled) const
{
	return astar.get_closest_point(Vector3(p_point.x, p_point.y, 0), p_include_disabled);
}

Vector2 AStar2D::get_closest_position_in_segment(const Vector2& p_point) const
{
	Vector3 p = astar.get_closest_position_in_segment(Vector3(p_point.x, p_point.y, 0));
	return Vector2(p.x, p.y);
}

real_t AStar2D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** end_entry = astar.points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	AStar3D::Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar2D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector2> AStar2D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** b_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *b_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		AStar3D::Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = Vector2(p2->pos.x, p2->pos.y);
			p2 = p2->prev_point;
		}

		w[0] = Vector2(p2->pos.x, p2->pos.y); // Assign first
	}

	return path;
}

Vector<int64_t> AStar2D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!to_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *to_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar2D::_solve(
	AStar3D::Point* p_begin_point, AStar3D::Point* p_end_point, bool p_allow_partial_path)
{
	astar.last_closest_point = nullptr;
	astar.pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<AStar3D::Point*> open_list;
	SortArray<AStar3D::Point*, AStar3D::SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		AStar3D::Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (astar.last_closest_point == nullptr ||
			astar.last_closest_point->abs_f_score > p->abs_f_score ||
			(astar.last_closest_point->abs_f_score >= p->abs_f_score &&
				astar.last_closest_point->abs_g_score > p->abs_g_score)) {
			astar.last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route =
 true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = astar.pass; // Mark the point as closed.

		for (KeyValue<int64_t, AStar3D::Point*>& kv : p->neighbors) {
			AStar3D::Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == astar.pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != astar.pass) { // The point wasn't inside the open list.
				e->open_pass = astar.pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

// --- a_star.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<int64_t> AStar3D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector3> AStar3D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar3D::_bind_compatibility_methods() {}

Vector<int64_t> AStar2D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStar2D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- a_star_grid_2d.cpp ---

static real_t heuristic_euclidean(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return (real_t)Math::sqrt(dx * dx + dy * dy);
}

static real_t heuristic_manhattan(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return dx + dy;
}

static real_t heuristic_octile(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	real_t F = Math::SQRT2 - 1;
	return (dx < dy) ? F * dx + dy : F * dy + dx;
}

static real_t heuristic_chebyshev(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return MAX(dx, dy);
}

static real_t (*heuristics[AStarGrid2D::HEURISTIC_MAX])(const Vector2i&, const Vector2i&) = {
	heuristic_euclidean, heuristic_manhattan, heuristic_octile, heuristic_chebyshev};

void AStarGrid2D::set_region(const Rect2i& p_region)
{
	ERR_FAIL_COND(p_region.size.x < 0 || p_region.size.y < 0);
	if (p_region != region) {
		region = p_region;
		dirty = true;
	}
}

Rect2i AStarGrid2D::get_region() const { return region; }

void AStarGrid2D::set_size(const Size2i& p_size)
{
	WARN_DEPRECATED_MSG(R"(The "size" property is deprecated, use "region" instead.)");
	ERR_FAIL_COND(p_size.x < 0 || p_size.y < 0);
	if (p_size != region.size) {
		region.size = p_size;
		dirty = true;
	}
}

Size2i AStarGrid2D::get_size() const { return region.size; }

void AStarGrid2D::set_offset(const Vector2& p_offset)
{
	if (!offset.is_equal_approx(p_offset)) {
		offset = p_offset;
		dirty = true;
	}
}

Vector2 AStarGrid2D::get_offset() const { return offset; }

void AStarGrid2D::set_cell_size(const Size2& p_cell_size)
{
	if (!cell_size.is_equal_approx(p_cell_size)) {
		cell_size = p_cell_size;
		dirty = true;
	}
}

Size2 AStarGrid2D::get_cell_size() const { return cell_size; }

void AStarGrid2D::set_cell_shape(CellShape p_cell_shape)
{
	if (cell_shape == p_cell_shape) {
		return;
	}

	ERR_FAIL_INDEX(p_cell_shape, CellShape::CELL_SHAPE_MAX);
	cell_shape = p_cell_shape;
	dirty = true;
}

AStarGrid2D::CellShape AStarGrid2D::get_cell_shape() const { return cell_shape; }

void AStarGrid2D::update()
{
	if (!dirty) {
		return;
	}

	points.clear();
	solid_mask.clear();

	const int32_t end_x = region.get_end().x;
	const int32_t end_y = region.get_end().y;
	const Vector2 half_cell_size = cell_size / 2;
	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	for (int32_t y = region.position.y; y < end_y; y++) {
		LocalVector<Point> line;
		solid_mask.push_back(true);
		for (int32_t x = region.position.x; x < end_x; x++) {
			Vector2 v = offset;
			switch (cell_shape) {
			case CELL_SHAPE_ISOMETRIC_RIGHT:
				v += half_cell_size + Vector2(x + y, y - x) * half_cell_size;
				break;
			case CELL_SHAPE_ISOMETRIC_DOWN:
				v += half_cell_size + Vector2(x - y, x + y) * half_cell_size;
				break;
			case CELL_SHAPE_SQUARE:
				v += Vector2(x, y) * cell_size;
				break;
			default:
				break;
			}
			line.push_back(Point(Vector2i(x, y), v));
			solid_mask.push_back(false);
		}
		solid_mask.push_back(true);
		points.push_back(std::move(line));
	}

	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	dirty = false;
}

bool AStarGrid2D::is_in_bounds(int32_t p_x, int32_t p_y) const
{
	return region.has_point(Vector2i(p_x, p_y));
}

bool AStarGrid2D::is_in_boundsv(const Vector2i& p_id) const { return region.has_point(p_id); }

bool AStarGrid2D::is_dirty() const { return dirty; }

void AStarGrid2D::set_jumping_enabled(bool p_enabled) { jumping_enabled = p_enabled; }

bool AStarGrid2D::is_jumping_enabled() const { return jumping_enabled; }

void AStarGrid2D::set_diagonal_mode(DiagonalMode p_diagonal_mode)
{
	ERR_FAIL_INDEX((int)p_diagonal_mode, (int)DIAGONAL_MODE_MAX);
	diagonal_mode = p_diagonal_mode;
}

AStarGrid2D::DiagonalMode AStarGrid2D::get_diagonal_mode() const { return diagonal_mode; }

void AStarGrid2D::set_default_compute_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_compute_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_compute_heuristic() const
{
	return default_compute_heuristic;
}

void AStarGrid2D::set_default_estimate_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_estimate_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_estimate_heuristic() const
{
	return default_estimate_heuristic;
}

void AStarGrid2D::set_point_solid(const Vector2i& p_id, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set if point is disabled. Point %s out of bounds %s.", p_id, region));
	_set_solid_unchecked(p_id, p_solid);
}

bool AStarGrid2D::is_point_solid(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, false, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), false,
		vformat("Can't get if point is disabled. Point %s out of bounds %s.", p_id, region));
	return _get_solid_unchecked(p_id);
}

void AStarGrid2D::set_point_weight_scale(const Vector2i& p_id, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set point's weight scale. Point %s out of bounds %s.", p_id, region));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));
	_get_point_unchecked(p_id)->weight_scale = p_weight_scale;
}

real_t AStarGrid2D::get_point_weight_scale(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, 0, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), 0,
		vformat("Can't get point's weight scale. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->weight_scale;
}

void AStarGrid2D::fill_solid_region(const Rect2i& p_region, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_set_solid_unchecked(x, y, p_solid);
		}
	}
}

void AStarGrid2D::fill_weight_scale_region(const Rect2i& p_region, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_get_point_unchecked(x, y)->weight_scale = p_weight_scale;
		}
	}
}

AStarGrid2D::Point* AStarGrid2D::_jump(Point* p_from, Point* p_to)
{
	int32_t from_x = p_from->id.x;
	int32_t from_y = p_from->id.y;

	int32_t to_x = p_to->id.x;
	int32_t to_y = p_to->id.y;

	int32_t dx = to_x - from_x;
	int32_t dy = to_y - from_y;

	if (diagonal_mode == DIAGONAL_MODE_ALWAYS ||
		diagonal_mode == DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(to_x, to_y, dx, dy);
		}

		while (_is_walkable(to_x, to_y) &&
			   (diagonal_mode == DIAGONAL_MODE_ALWAYS || _is_walkable(to_x, to_y - dy) ||
				   _is_walkable(to_x - dx, to_y))) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - dx, to_y + dy) && !_is_walkable(to_x - dx, to_y)) ||
				(_is_walkable(to_x + dx, to_y - dy) && !_is_walkable(to_x, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x + dx, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y + dy, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else if (diagonal_mode == DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(from_x, from_y, dx, dy, true);
		}

		while (_is_walkable(to_x, to_y) && _is_walkable(to_x, to_y - dy) &&
			   _is_walkable(to_x - dx, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x + dx, to_y + dy) && !_is_walkable(to_x, to_y + dy)) ||
				!_is_walkable(to_x + dx, to_y)) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else { // DIAGONAL_MODE_NEVER
		if (dy == 0) {
			return _forced_successor(from_x, from_y, dx, 0, true);
		}

		while (_is_walkable(to_x, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - 1, to_y) && !_is_walkable(to_x - 1, to_y - dy)) ||
				(_is_walkable(to_x + 1, to_y) && !_is_walkable(to_x + 1, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, 1, 0, true) != nullptr ||
				_forced_successor(to_x, to_y, -1, 0, true) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_y += dy;
		}
	}

	return nullptr;
}

AStarGrid2D::Point* AStarGrid2D::_forced_successor(
	int32_t p_x, int32_t p_y, int32_t p_dx, int32_t p_dy, bool p_inclusive)
{
	// Remembering previous results can improve performance.
	bool l_prev = false, r_prev = false, l = false, r = false;

	int32_t o_x = p_x, o_y = p_y;
	if (p_inclusive) {
		o_x += p_dx;
		o_y += p_dy;
	}

	int32_t l_x = p_x - p_dy, l_y = p_y - p_dx;
	int32_t r_x = p_x + p_dy, r_y = p_y + p_dx;

	while (_is_walkable(o_x, o_y)) {
		if (end->id.x == o_x && end->id.y == o_y) {
			return end;
		}

		l_prev = l || _is_walkable(l_x, l_y);
		r_prev = r || _is_walkable(r_x, r_y);

		l_x += p_dx;
		l_y += p_dy;
		r_x += p_dx;
		r_y += p_dy;

		l = _is_walkable(l_x, l_y);
		r = _is_walkable(r_x, r_y);

		if ((l && !l_prev) || (r && !r_prev)) {
			return _get_point_unchecked(o_x, o_y);
		}

		o_x += p_dx;
		o_y += p_dy;
	}
	return nullptr;
}

void AStarGrid2D::_get_nbors(Point* p_point, LocalVector<Point*>& r_nbors)
{
	bool ts0 = false, td0 = false, ts1 = false, td1 = false, ts2 = false, td2 = false, ts3 = false,
		 td3 = false;

	Point* left = nullptr;
	Point* right = nullptr;
	Point* top = nullptr;
	Point* bottom = nullptr;

	Point* top_left = nullptr;
	Point* top_right = nullptr;
	Point* bottom_left = nullptr;
	Point* bottom_right = nullptr;

	{
		bool has_left = false;
		bool has_right = false;

		if (p_point->id.x - 1 >= region.position.x) {
			left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y);
			has_left = true;
		}
		if (p_point->id.x + 1 < region.position.x + region.size.width) {
			right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y);
			has_right = true;
		}
		if (p_point->id.y - 1 >= region.position.y) {
			top = _get_point_unchecked(p_point->id.x, p_point->id.y - 1);
			if (has_left) {
				top_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y - 1);
			}
			if (has_right) {
				top_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y - 1);
			}
		}
		if (p_point->id.y + 1 < region.position.y + region.size.height) {
			bottom = _get_point_unchecked(p_point->id.x, p_point->id.y + 1);
			if (has_left) {
				bottom_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y + 1);
			}
			if (has_right) {
				bottom_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y + 1);
			}
		}
	}

	if (top && !_get_solid_unchecked(top->id)) {
		r_nbors.push_back(top);
		ts0 = true;
	}
	if (right && !_get_solid_unchecked(right->id)) {
		r_nbors.push_back(right);
		ts1 = true;
	}
	if (bottom && !_get_solid_unchecked(bottom->id)) {
		r_nbors.push_back(bottom);
		ts2 = true;
	}
	if (left && !_get_solid_unchecked(left->id)) {
		r_nbors.push_back(left);
		ts3 = true;
	}

	switch (diagonal_mode) {
	case DIAGONAL_MODE_ALWAYS: {
		td0 = true;
		td1 = true;
		td2 = true;
		td3 = true;
	} break;
	case DIAGONAL_MODE_NEVER: {
	} break;
	case DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE: {
		td0 = ts3 || ts0;
		td1 = ts0 || ts1;
		td2 = ts1 || ts2;
		td3 = ts2 || ts3;
	} break;
	case DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES: {
		td0 = ts3 && ts0;
		td1 = ts0 && ts1;
		td2 = ts1 && ts2;
		td3 = ts2 && ts3;
	} break;
	default:
		break;
	}

	if (td0 && (top_left && !_get_solid_unchecked(top_left->id))) {
		r_nbors.push_back(top_left);
	}
	if (td1 && (top_right && !_get_solid_unchecked(top_right->id))) {
		r_nbors.push_back(top_right);
	}
	if (td2 && (bottom_right && !_get_solid_unchecked(bottom_right->id))) {
		r_nbors.push_back(bottom_right);
	}
	if (td3 && (bottom_left && !_get_solid_unchecked(bottom_left->id))) {
		r_nbors.push_back(bottom_left);
	}
}

bool AStarGrid2D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (_get_solid_unchecked(p_begin_point->id)) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (_get_solid_unchecked(p_end_point->id) && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;
	LocalVector<Point*> nbors;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);
	end = p_end_point;

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		nbors.clear();
		_get_nbors(p, nbors);

		for (Point* e : nbors) {
			real_t weight_scale = 1.0;

			if (jumping_enabled) {
				// TODO: Make it works with weight_scale.
				e = _jump(p, e);
				if (!e || e->closed_pass == pass) {
					continue;
				}
			}
			else {
				if (_get_solid_unchecked(e->id) || e->closed_pass == pass) {
					continue;
				}
				weight_scale = e->weight_scale;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * weight_scale;
			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);

			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStarGrid2D::_estimate_cost(const Vector2i& p_from_id, const Vector2i& p_end_id)
{
	return heuristics[default_estimate_heuristic](p_from_id, p_end_id);
}

real_t AStarGrid2D::_compute_cost(const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return heuristics[default_compute_heuristic](p_from_id, p_to_id);
}

void AStarGrid2D::clear()
{
	points.clear();
	region = Rect2i();
}

Vector2 AStarGrid2D::get_point_position(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, Vector2(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), Vector2(),
		vformat("Can't get point's position. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->pos;
}

Vector<Vector2> AStarGrid2D::get_point_path(
	const Vector2i& p_from_id, const Vector2i& p_to_id, bool p_allow_partial_path)
{
	ERR_FAIL_COND_V_MSG(
		dirty, Vector<Vector2>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_from_id, region));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_to_id, region));

	Point* begin_point = _get_point(p_from_id.x, p_from_id.y);
	Point* end_point = _get_point(p_to_id.x, p_to_id.y);

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int32_t pc = 1;
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		p = end_point;
		int32_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->pos;
			p = p->prev_point;
		}

		w[0] = p->pos;
	}

	return path;
}



Vector<Vector2i> AStarGrid2D::get_id_path(Vector2i const&, Vector2i const&, bool) {}

// --- a_star_grid_2d.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<Vector2i> AStarGrid2D::_get_id_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStarGrid2D::_get_point_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStarGrid2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
