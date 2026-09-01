/**************************************************************************/
/*  a_star_grid_2d.cpp                                                    */
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

#include "a_star_grid_2d.compat.inc"
#include "a_star_grid_2d.h"

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

void AStarGrid2D::_bind_methods() {}

