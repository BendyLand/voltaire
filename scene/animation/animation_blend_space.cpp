/**************************************************************************/
/*  animation_blend_space.cpp                                             */
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

#include "animation_blend_space.h"
#include "core/math/geometry_2d.h"
#include "scene/animation/animation_blend_tree.h"

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, float p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_bind_compatibility_methods() {}
#endif // DISABLE_DEPRECATED

Ref<AnimationNode> AnimationNodeBlendSpace::get_child_by_name(const String& p_name) const
{
	int point_index = find_blend_point_by_name(p_name);
	if (point_index != -1) {
		return get_blend_point_node(point_index);
	}
	return Ref<AnimationNode>();
}

void AnimationNodeBlendSpace::_tree_changed()
{
	AnimationNode::_tree_changed();
	_check_can_sync();
}

void AnimationNodeBlendSpace::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	if (dimension == DIMENSION_1D) {
		if (get_blend_point_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_BLEND_POINT));
		}
	}
	else {
		const_cast<AnimationNodeBlendSpace*>(this)->_update_triangles();
		if (get_triangle_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_TRIANGLE));
		}
	}

	const_cast<AnimationNodeBlendSpace*>(this)->_check_can_sync();
	if (is_contain_invalid_point) {
		add_validation_error(p_tree, p_path, RTR(ERR_INVALID_POINT));
	}
}

void AnimationNodeBlendSpace::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (int i = 0; i < blend_points_used; i++) {
		ChildNode cn;
		cn.name = blend_points[i].name;
		cn.node = blend_points[i].node;
		r_child_nodes->push_back(cn);
	}
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	float p_position, int p_at_index, const String& p_name)
{
	add_blend_point(p_node, Vector2(p_position, 0.0f), p_at_index, p_name);
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	const Vector2& p_position, int p_at_index, const String& p_name)
{
	ERR_FAIL_COND(blend_points_used >= MAX_BLEND_POINTS);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(p_at_index < -1 || p_at_index > blend_points_used);
#ifndef DISABLE_DEPRECATED
	if (p_name == String()) {
		_add_blend_point_bind_compat_110369(p_node, p_position, p_at_index);
		WARN_PRINT_ED("AnimationNodeBlendSpace::add_blend_point: No name provided, using safe "
					  "index as reference. In the future, empty names will be deprecated, so "
					  "explicitly passing a name is recommended.");
		return;
	}
#else
	ERR_FAIL_COND(p_name == String());
#endif

	if (p_at_index == -1 || p_at_index == blend_points_used) {
		p_at_index = blend_points_used;
	}
	else {
		for (int i = blend_points_used; i > p_at_index; i--) {
			blend_points[i] = blend_points[i - 1];
		}
		if (dimension == DIMENSION_2D) {
			for (int i = 0; i < triangles.size(); i++) {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] >= p_at_index) {
						triangles.write[i].points[j]++;
					}
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
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}

	_tree_changed();
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, float p_position)
{
	set_blend_point_position(p_point, Vector2(p_position, 0.0f));
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, const Vector2& p_position)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	blend_points[p_point].position = p_position;
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}
}

Vector2 AnimationNodeBlendSpace::get_blend_point_position(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Vector2());
	return blend_points[p_point].position;
}

float AnimationNodeBlendSpace::get_blend_point_position_1d(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, 0.0f);
	return blend_points[p_point].position.x;
}

void AnimationNodeBlendSpace::set_blend_point_node(
	int p_point, const Ref<AnimationNode>& p_node)
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

Ref<AnimationNode> AnimationNodeBlendSpace::get_blend_point_node(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Ref<AnimationNode>());
	return blend_points[p_point].node;
}

void AnimationNodeBlendSpace::set_blend_point_name(int p_point, const String& p_name)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_name.is_empty());
	blend_points[p_point].name = p_name;
}

const String& AnimationNodeBlendSpace::get_blend_point_name(int p_point) const
{
	const static String empty = String();
	ERR_FAIL_INDEX_V(p_point, blend_points_used, empty);
	return blend_points[p_point].name;
}

int AnimationNodeBlendSpace::find_blend_point_by_name(const String& p_name) const
{
	for (int i = 0; i < blend_points_used; i++) {
		if (blend_points[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

void AnimationNodeBlendSpace::remove_blend_point(int p_point)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	for (int i = p_point; i < blend_points_used - 1; i++) {
		blend_points[i] = blend_points[i + 1];
	}
	blend_points[blend_points_used - 1].reset();
	blend_points_used--;

	if (dimension == DIMENSION_2D) {
		for (int i = triangles.size() - 1; i >= 0; i--) {
			if (triangles[i].points[0] == p_point || triangles[i].points[1] == p_point ||
				triangles[i].points[2] == p_point) {
				triangles.remove_at(i);
			}
			else {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] > p_point) {
						triangles.write[i].points[j]--;
					}
				}
			}
		}
		_queue_auto_triangles();
	}

	lengths_dirty = true;
	_tree_changed();
}

int AnimationNodeBlendSpace::get_blend_point_count() const { return blend_points_used; }

void AnimationNodeBlendSpace::reorder_blend_point(int p_from_index, int p_to_index)
{
	ERR_FAIL_INDEX(p_from_index, blend_points_used);
	ERR_FAIL_INDEX(p_to_index, blend_points_used);
	if (p_from_index == p_to_index) {
		return;
	}
	SWAP(blend_points[p_from_index], blend_points[p_to_index]);
	if (dimension == DIMENSION_2D) {
		for (int i = 0; i < triangles.size(); i++) {
			for (int j = 0; j < 3; j++) {
				if (triangles[i].points[j] == p_from_index) {
					triangles.write[i].points[j] = p_to_index;
				}
				else if (triangles[i].points[j] == p_to_index) {
					triangles.write[i].points[j] = p_from_index;
				}
			}
		}
	}
	lengths_dirty = true;
	_tree_changed();
}

bool AnimationNodeBlendSpace::has_triangle(int p_x, int p_y, int p_z) const
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

void AnimationNodeBlendSpace::add_triangle(int p_x, int p_y, int p_z, int p_at_index)
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

int AnimationNodeBlendSpace::get_triangle_point(int p_triangle, int p_point)
{
	_update_triangles();

	ERR_FAIL_INDEX_V(p_point, 3, -1);
	ERR_FAIL_INDEX_V(p_triangle, triangles.size(), -1);
	return triangles[p_triangle].points[p_point];
}

void AnimationNodeBlendSpace::remove_triangle(int p_triangle)
{
	ERR_FAIL_INDEX(p_triangle, triangles.size());
	triangles.remove_at(p_triangle);
}

int AnimationNodeBlendSpace::get_triangle_count() const { return triangles.size(); }

void AnimationNodeBlendSpace::set_min_space(float p_min)
{
	min_space.x = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
}

void AnimationNodeBlendSpace::set_min_space(const Vector2& p_min)
{
	min_space = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
	if (min_space.y >= max_space.y) {
		min_space.y = max_space.y - 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_min_space() const { return min_space; }

float AnimationNodeBlendSpace::get_min_space_1d() const { return min_space.x; }

void AnimationNodeBlendSpace::set_max_space(float p_max)
{
	max_space.x = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
}

void AnimationNodeBlendSpace::set_max_space(const Vector2& p_max)
{
	max_space = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
	if (max_space.y <= min_space.y) {
		max_space.y = min_space.y + 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_max_space() const { return max_space; }

float AnimationNodeBlendSpace::get_max_space_1d() const { return max_space.x; }

void AnimationNodeBlendSpace::set_snap(float p_snap) { snap.x = p_snap; }

void AnimationNodeBlendSpace::set_snap(const Vector2& p_snap) { snap = p_snap; }

Vector2 AnimationNodeBlendSpace::get_snap() const { return snap; }

float AnimationNodeBlendSpace::get_snap_1d() const { return snap.x; }

void AnimationNodeBlendSpace::set_value_label(const String& p_label) { value_label = p_label; }

String AnimationNodeBlendSpace::get_value_label() const { return value_label; }

void AnimationNodeBlendSpace::set_x_label(const String& p_label) { x_label = p_label; }

String AnimationNodeBlendSpace::get_x_label() const { return x_label; }

void AnimationNodeBlendSpace::set_y_label(const String& p_label) { y_label = p_label; }

String AnimationNodeBlendSpace::get_y_label() const { return y_label; }

void AnimationNodeBlendSpace::_set_triangles(const Vector<int>& p_triangles)
{
	triangles.clear();
	if (p_triangles.size() % 3 != 0) {
		return;
	}
	for (int i = 0; i < p_triangles.size(); i += 3) {
		add_triangle(p_triangles[i + 0], p_triangles[i + 1], p_triangles[i + 2]);
	}
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_retry_set_triangles(const Vector<int>& p_triangles)
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

Vector<int> AnimationNodeBlendSpace::_get_triangles() const
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

Vector2 AnimationNodeBlendSpace::get_closest_point(const Vector2& p_point)
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

void AnimationNodeBlendSpace::_blend_triangle(
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

void AnimationNodeBlendSpace::_check_can_sync()
{
	is_contain_invalid_point = false;
	if (sync_mode < SYNC_MODE_CYCLIC_MUTABLE) {
		return;
	}
	for (int i = 0; i < blend_points_used; i++) {
		Ref<AnimationNodeBlendNode> na = blend_points[i].node;
		if (na.is_null() || na->get_node_type() != AnimationNodeBlendNode::NODE_ANIMATION) {
			is_contain_invalid_point = true;
			break;
		}
	}
}

String AnimationNodeBlendSpace::get_caption() const
{
	return dimension == DIMENSION_2D ? "BlendSpace2D" : "BlendSpace1D";
}

void AnimationNodeBlendSpace::set_auto_triangles(bool p_enable)
{
	if (auto_triangles == p_enable) {
		return;
	}

	auto_triangles = p_enable;
	_queue_auto_triangles();
}

bool AnimationNodeBlendSpace::get_auto_triangles() const { return auto_triangles; }

void AnimationNodeBlendSpace::set_blend_mode(BlendMode p_blend_mode)
{
	blend_mode = p_blend_mode;
}

AnimationNodeBlendSpace::BlendMode AnimationNodeBlendSpace::get_blend_mode() const
{
	return blend_mode;
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::set_use_sync(bool p_sync)
{
	sync_mode = p_sync ? SYNC_MODE_INDEPENDENT : SYNC_MODE_NONE;
}

bool AnimationNodeBlendSpace::is_using_sync() const { return sync_mode != SYNC_MODE_NONE; }
#endif // DISABLE_DEPRECATED

void AnimationNodeBlendSpace::set_sync_mode(SyncMode p_sync_mode)
{
	sync_mode = p_sync_mode;
	_check_can_sync();
}

AnimationNodeBlendSpace::SyncMode AnimationNodeBlendSpace::get_sync_mode() const
{
	return sync_mode;
}

void AnimationNodeBlendSpace::set_cyclic_length(double p_length)
{
	cyclic_length = p_length;
	inverted_cycle_length = (p_length > CMP_EPSILON) ? (1.0 / p_length) : 0.0;
}

double AnimationNodeBlendSpace::get_cyclic_length() const { return cyclic_length; }

AnimationNode::NodeTimeInfo AnimationNodeBlendSpace::_process(
	AnimationNode::ProcessState& r_process_state, AnimationNodeInstance& p_instance,
	const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only)
{
	return AnimationNode::NodeTimeInfo();
}

void AnimationNodeBlendSpace::_update_triangles() {}

void AnimationNodeBlendSpace::_queue_auto_triangles() {}
