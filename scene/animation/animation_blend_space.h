/**************************************************************************/
/*  animation_blend_space.h                                               */
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

#pragma once

#include "scene/animation/animation_tree.h"

class AnimationNodeBlendSpace final : public AnimationNode
{
	const String ERR_NO_BLEND_POINT = "No blend points exist, so blending cannot take place.";
	const String ERR_NO_TRIANGLE = "No triangles exist, so blending cannot take place.";
	const String ERR_INVALID_POINT =
		"Cyclic sync modes require that all blend points in BlendSpace use non-nested Animation "
		"nodes with a finite, immutable length.";

public:
	enum Dimension
	{
		DIMENSION_1D,
		DIMENSION_2D,
	};

	enum BlendMode
	{
		BLEND_MODE_INTERPOLATED,
		BLEND_MODE_DISCRETE,
		BLEND_MODE_DISCRETE_CARRY,
	};

	enum SyncMode
	{
		SYNC_MODE_NONE,			   // Inactive animations are frozen (not advanced).
		SYNC_MODE_INDEPENDENT,	   // Inactive animations advance with weight=0 (previous "sync"
								   // behavior).
		SYNC_MODE_CYCLIC_MUTABLE,  // Time-scaled with blend-weight-dependent cycle length
								   // (self-normalizing).
		SYNC_MODE_CYCLIC_CONSTANT, // Time-scaled to complete one cycle in cyclic_length seconds.
	};

private:
	enum
	{
		MAX_BLEND_POINTS = 64
	};

	struct BlendPoint
	{
		String name;
		Ref<AnimationNode> node;
		Vector2 position;

		void reset()
		{
			name = String();
			node = Ref<AnimationNode>();
			position = Vector2();
		}
	};

	struct BlendTriangle
	{
		int points[3] = {};
	};

	Dimension dimension = DIMENSION_1D;
	BlendPoint blend_points[MAX_BLEND_POINTS];
	int blend_points_used = 0;

	Vector<BlendTriangle> triangles;

	String blend_position = "blend_position";
	String closest = "closest";
	Vector2 max_space = Vector2(1, 1);
	Vector2 min_space = Vector2(-1, -1);
	Vector2 snap = Vector2(0.1f, 0.1f);
	String value_label = "value";
	String x_label = "x";
	String y_label = "y";
	BlendMode blend_mode = BLEND_MODE_INTERPOLATED;

	bool auto_triangles = true;
	bool triangles_dirty = false;

	SyncMode sync_mode = SYNC_MODE_NONE;
	double cyclic_length = 0.0;
	double inverted_cycle_length = 0.0; // Cached 1/cyclic_length.
	LocalVector<double> cached_lengths;
	bool lengths_dirty = true;
	bool is_contain_invalid_point = false;

	void _set_triangles(const Vector<int>& p_triangles);
	Vector<int> _get_triangles() const;

#ifndef DISABLE_DEPRECATED
	void _retry_set_triangles(const Vector<int>& p_triangles);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, float p_position, int p_at_index = -1);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index = -1);
	static void _bind_compatibility_methods();
#endif

	void _blend_triangle(
		const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights);
	void _update_triangles();
	void _queue_auto_triangles();
	void _check_can_sync();

protected:
	virtual void _tree_changed() override;

public:
	Dimension get_dimension() const { return dimension; }

	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;
	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	void add_blend_point(const Ref<AnimationNode>& p_node, float p_position,
		int p_at_index = -1, const String& p_name = String());
	void add_blend_point(const Ref<AnimationNode>& p_node, const Vector2& p_position,
		int p_at_index = -1, const String& p_name = String());

	void set_blend_point_position(int p_point, float p_position);
	void set_blend_point_position(int p_point, const Vector2& p_position);
	Vector2 get_blend_point_position(int p_point) const;
	float get_blend_point_position_1d(int p_point) const;

	void set_blend_point_node(int p_point, const Ref<AnimationNode>& p_node);
	Ref<AnimationNode> get_blend_point_node(int p_point) const;
	void set_blend_point_name(int p_point, const String& p_name);
	const String& get_blend_point_name(int p_point) const;
	int find_blend_point_by_name(const String& p_name) const;
	void remove_blend_point(int p_point);
	int get_blend_point_count() const;
	void reorder_blend_point(int p_from_index, int p_to_index);

	// 2D triangle methods
	bool has_triangle(int p_x, int p_y, int p_z) const;
	void add_triangle(int p_x, int p_y, int p_z, int p_at_index = -1);
	int get_triangle_point(int p_triangle, int p_point);
	void remove_triangle(int p_triangle);
	int get_triangle_count() const;
	Vector2 get_closest_point(const Vector2& p_point);
	void set_auto_triangles(bool p_enable);
	bool get_auto_triangles() const;

	// Space & snap (1D and 2D)
	void set_min_space(float p_min);
	void set_min_space(const Vector2& p_min);
	Vector2 get_min_space() const;
	float get_min_space_1d() const;

	void set_max_space(float p_max);
	void set_max_space(const Vector2& p_max);
	Vector2 get_max_space() const;
	float get_max_space_1d() const;

	void set_snap(float p_snap);
	void set_snap(const Vector2& p_snap);
	Vector2 get_snap() const;
	float get_snap_1d() const;

	// Labels
	void set_value_label(const String& p_label);
	String get_value_label() const;
	void set_x_label(const String& p_label);
	String get_x_label() const;
	void set_y_label(const String& p_label);
	String get_y_label() const;

	// Blend & sync modes
	void set_blend_mode(BlendMode p_blend_mode);
	BlendMode get_blend_mode() const;

	void set_cyclic_length(double p_length);
	double get_cyclic_length() const;

#ifndef DISABLE_DEPRECATED
	virtual void set_use_sync(bool p_sync) override;
	virtual bool is_using_sync() const override;
#endif // DISABLE_DEPRECATED

	void set_sync_mode(SyncMode p_sync_mode);
	SyncMode get_sync_mode() const;

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;
	virtual String get_caption() const override;
	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

	explicit AnimationNodeBlendSpace(Dimension p_dimension = DIMENSION_1D) : dimension(p_dimension)
	{
		root_node = true;
	}
};
