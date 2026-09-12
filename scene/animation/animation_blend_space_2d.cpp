/**************************************************************************/
/*  animation_blend_space_2d.cpp                                          */
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

#include "animation_blend_space_2d.compat.inc"
#include "animation_blend_space_2d.h"
#include "core/math/geometry_2d.h"
#include "scene/animation/animation_blend_tree.h"

void AnimationNodeBlendSpace2D::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (int i = 0; i < blend_points_used; i++) {
		ChildNode cn;
		cn.name = blend_points[i].name;
		cn.node = blend_points[i].node;
		r_child_nodes->push_back(cn);
	}
}

void AnimationNodeBlendSpace2D::add_blend_point(const Ref<AnimationRootNode>& p_node,
	const Vector2& p_position, int p_at_index, const StringName& p_name)
{
	ERR_FAIL_COND(blend_points_used >= MAX_BLEND_POINTS);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(p_at_index < -1 || p_at_index > blend_points_used);
#ifndef DISABLE_DEPRECATED
	if (p_name == StringName()) {
		_add_blend_point_bind_compat_110369(p_node, p_position, p_at_index);
		WARN_PRINT_ED("AnimationNodeBlendSpace2D::add_blend_point: No name provided, using safe "
					  "index as reference. In the future, empty names will be deprecated, so "
					  "explicitly passing a name is recommended.");
		return;
	}
#else
	ERR_FAIL_COND(p_name == StringName());
#endif

	if (p_at_index == -1 || p_at_index == blend_points_used) {
		p_at_index = blend_points_used;
	}
	else {
		for (int i = blend_points_used; i > p_at_index; i--) {
			blend_points[i] = blend_points[i - 1];
		}
		for (int i = 0; i < triangles.size(); i++) {
			for (int j = 0; j < 3; j++) {
				if (triangles[i].points[j] >= p_at_index) {
					triangles.write[i].points[j]++;
				}
			}
		}
	}
	blend_points[p_at_index].node = p_node;
	blend_points[p_at_index].position = p_position;
	blend_points[p_at_index].name = p_name;

	_add_node(blend_points[p_at_index].node);
	blend_points_used++;

	lengths_dirty = true;
	_queue_auto_triangles();

	_tree_changed();
}

void AnimationNodeBlendSpace2D::set_blend_point_position(int p_point, const Vector2& p_position)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	blend_points[p_point].position = p_position;
	_queue_auto_triangles();
}

void AnimationNodeBlendSpace2D::set_blend_point_node(
	int p_point, const Ref<AnimationRootNode>& p_node)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_node.is_null());

	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	blend_points[p_point].node = p_node;
	_add_node(blend_points[p_point].node);

	lengths_dirty = true;
	_tree_changed();
}

Vector2 AnimationNodeBlendSpace2D::get_blend_point_position(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Vector2());
	return blend_points[p_point].position;
}

Ref<AnimationRootNode> AnimationNodeBlendSpace2D::get_blend_point_node(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Ref<AnimationRootNode>());
	return blend_points[p_point].node;
}

const StringName& AnimationNodeBlendSpace2D::get_blend_point_name(int p_point) const
{
	const static StringName empty = StringName();
	ERR_FAIL_INDEX_V(p_point, blend_points_used, empty);
	return blend_points[p_point].name;
}

int AnimationNodeBlendSpace2D::find_blend_point_by_name(const StringName& p_name) const
{
	for (int i = 0; i < blend_points_used; i++) {
		if (blend_points[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

int AnimationNodeBlendSpace2D::get_blend_point_count() const { return blend_points_used; }

bool AnimationNodeBlendSpace2D::has_triangle(int p_x, int p_y, int p_z) const
{
	ERR_FAIL_INDEX_V(p_x, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_y, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_z, blend_points_used, false);

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		if (all_equal) {
			return true;
		}
	}

	return false;
}

void AnimationNodeBlendSpace2D::add_triangle(int p_x, int p_y, int p_z, int p_at_index)
{
	ERR_FAIL_INDEX(p_x, blend_points_used);
	ERR_FAIL_INDEX(p_y, blend_points_used);
	ERR_FAIL_INDEX(p_z, blend_points_used);

	_update_triangles();

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		ERR_FAIL_COND(all_equal);
	}

	if (p_at_index == -1 || p_at_index == triangles.size()) {
		triangles.push_back(t);
	}
	else {
		triangles.insert(p_at_index, t);
	}
}

int AnimationNodeBlendSpace2D::get_triangle_point(int p_triangle, int p_point)
{
	_update_triangles();

	ERR_FAIL_INDEX_V(p_point, 3, -1);
	ERR_FAIL_INDEX_V(p_triangle, triangles.size(), -1);
	return triangles[p_triangle].points[p_point];
}

void AnimationNodeBlendSpace2D::remove_triangle(int p_triangle)
{
	ERR_FAIL_INDEX(p_triangle, triangles.size());

	triangles.remove_at(p_triangle);
}

int AnimationNodeBlendSpace2D::get_triangle_count() const { return triangles.size(); }

void AnimationNodeBlendSpace2D::set_min_space(const Vector2& p_min)
{
	min_space = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
	if (min_space.y >= max_space.y) {
		min_space.y = max_space.y - 1;
	}
}

Vector2 AnimationNodeBlendSpace2D::get_min_space() const { return min_space; }

void AnimationNodeBlendSpace2D::set_max_space(const Vector2& p_max)
{
	max_space = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
	if (max_space.y <= min_space.y) {
		max_space.y = min_space.y + 1;
	}
}

Vector2 AnimationNodeBlendSpace2D::get_max_space() const { return max_space; }

void AnimationNodeBlendSpace2D::set_snap(const Vector2& p_snap) { snap = p_snap; }

Vector2 AnimationNodeBlendSpace2D::get_snap() const { return snap; }

void AnimationNodeBlendSpace2D::set_x_label(const String& p_label) { x_label = p_label; }

String AnimationNodeBlendSpace2D::get_x_label() const { return x_label; }

void AnimationNodeBlendSpace2D::set_y_label(const String& p_label) { y_label = p_label; }

String AnimationNodeBlendSpace2D::get_y_label() const { return y_label; }

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace2D::_retry_set_triangles(const Vector<int>& p_triangles)
{
	for (int i = 0; i < p_triangles.size(); i += 3) {
		int x = p_triangles[i + 0];
		int y = p_triangles[i + 1];
		int z = p_triangles[i + 2];
		if (x >= blend_points_used || y >= blend_points_used || z >= blend_points_used ||
			has_triangle(x, y, z)) {
			continue;
		}
		add_triangle(x, y, z);
	}
}
#endif // DISABLE_DEPRECATED

Vector<int> AnimationNodeBlendSpace2D::_get_triangles() const
{
	Vector<int> t;
	if (auto_triangles && triangles_dirty) {
		return t;
	}

	t.resize(triangles.size() * 3);
	for (int i = 0; i < triangles.size(); i++) {
		t.write[i * 3 + 0] = triangles[i].points[0];
		t.write[i * 3 + 1] = triangles[i].points[1];
		t.write[i * 3 + 2] = triangles[i].points[2];
	}
	return t;
}

Vector2 AnimationNodeBlendSpace2D::get_closest_point(const Vector2& p_point)
{
	_update_triangles();

	if (triangles.is_empty()) {
		return Vector2();
	}

	Vector2 best_point;
	bool first = true;

	for (int i = 0; i < triangles.size(); i++) {
		Vector2 points[3];
		for (int j = 0; j < 3; j++) {
			points[j] = get_blend_point_position(get_triangle_point(i, j));
		}

		if (Geometry2D::is_point_in_triangle(p_point, points[0], points[1], points[2])) {
			return p_point;
		}

		for (int j = 0; j < 3; j++) {
			const Vector2 segment_a = points[j];
			const Vector2 segment_b = points[(j + 1) % 3];
			Vector2 closest_point =
				Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
			if (first || closest_point.distance_to(p_point) < best_point.distance_to(p_point)) {
				best_point = closest_point;
				first = false;
			}
		}
	}

	return best_point;
}

void AnimationNodeBlendSpace2D::_blend_triangle(
	const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights)
{
	if (p_pos.is_equal_approx(p_points[0])) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[1])) {
		r_weights[0] = 0;
		r_weights[1] = 1;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[2])) {
		r_weights[0] = 0;
		r_weights[1] = 0;
		r_weights[2] = 1;
		return;
	}

	Vector2 v0 = p_points[1] - p_points[0];
	Vector2 v1 = p_points[2] - p_points[0];
	Vector2 v2 = p_pos - p_points[0];

	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	r_weights[0] = u;
	r_weights[1] = v;
	r_weights[2] = w;
}

void AnimationNodeBlendSpace2D::_check_can_sync()
{
	is_contain_invalid_point = false;
	if (sync_mode < SYNC_MODE_CYCLIC_MUTABLE) {
		return;
	}
	for (int i = 0; i < blend_points_used; i++) {
		Ref<AnimationNodeAnimation> na =
			static_cast<Ref<AnimationNodeAnimation>>(blend_points[i].node);
		if (na.is_null()) {
			is_contain_invalid_point = true;
			break;
		}
	}
}

String AnimationNodeBlendSpace2D::get_caption() const { return "BlendSpace2D"; }

void AnimationNodeBlendSpace2D::set_auto_triangles(bool p_enable)
{
	if (auto_triangles == p_enable) {
		return;
	}

	auto_triangles = p_enable;
	_queue_auto_triangles();
}

bool AnimationNodeBlendSpace2D::get_auto_triangles() const { return auto_triangles; }

Ref<AnimationNode> AnimationNodeBlendSpace2D::get_child_by_name(const StringName& p_name) const
{
	int point_index = find_blend_point_by_name(p_name);
	if (point_index != -1) {
		return get_blend_point_node(point_index);
	}
	return Ref<AnimationRootNode>();
}

void AnimationNodeBlendSpace2D::set_blend_mode(BlendMode p_blend_mode)
{
	blend_mode = p_blend_mode;
}

AnimationNodeBlendSpace2D::BlendMode AnimationNodeBlendSpace2D::get_blend_mode() const
{
	return blend_mode;
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace2D::set_use_sync(bool p_sync)
{
	sync_mode = p_sync ? SYNC_MODE_INDEPENDENT : SYNC_MODE_NONE;
}

bool AnimationNodeBlendSpace2D::is_using_sync() const { return sync_mode != SYNC_MODE_NONE; }
#endif // DISABLE_DEPRECATED

void AnimationNodeBlendSpace2D::set_sync_mode(SyncMode p_sync_mode)
{
	sync_mode = p_sync_mode;
	_check_can_sync();
}

AnimationNodeBlendSpace2D::SyncMode AnimationNodeBlendSpace2D::get_sync_mode() const
{
	return sync_mode;
}

void AnimationNodeBlendSpace2D::set_cyclic_length(double p_length)
{
	cyclic_length = p_length;
	inverted_cycle_length = (p_length > CMP_EPSILON) ? (1.0 / p_length) : 0.0;
}

double AnimationNodeBlendSpace2D::get_cyclic_length() const { return cyclic_length; }

void AnimationNodeBlendSpace2D::_tree_changed()
{
	AnimationRootNode::_tree_changed();
	_check_can_sync();
}

void AnimationNodeBlendSpace2D::validate_node(
	const AnimationTree* p_tree, const StringName& p_path) const
{
	AnimationRootNode::validate_node(p_tree, p_path);

	const_cast<AnimationNodeBlendSpace2D*>(this)->_update_triangles();
	if (get_triangle_count() == 0) {
		add_validation_error(p_tree, p_path, RTR(ERR_NO_TRIANGLE));
	}

	const_cast<AnimationNodeBlendSpace2D*>(this)->_check_can_sync();
	if (is_contain_invalid_point) {
		add_validation_error(p_tree, p_path, RTR(ERR_INVALID_POINT));
	}
}



