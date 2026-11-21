/**************************************************************************/
/*  sprite_2d.h                                                           */
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

#include "scene/2d/node_2d.h"
#include "scene/resources/texture.h"

class Sprite2D : public Node2D
{
	Ref<Texture2D> texture;

#ifdef TOOLS_ENABLED
	bool dragging_to_resize_rect = false;
#endif // TOOLS_ENABLED

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;
	bool region_enabled = false;
	Rect2 region_rect;
	bool region_filter_clip_enabled = false;

	int frame = 0;

	int vframes = 1;
	int hframes = 1;

	void _get_rects(Rect2& r_src_rect, Rect2& r_dst_rect, bool& r_filter_clip_enabled) const;
	Point2 _get_rect_offset(const Size2i& p_size) const;

	void _texture_changed();
	void _emit_region_rect_enabled();

protected:
	void _notification(int p_what);


public:
#ifdef DEBUG_ENABLED
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;

	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	bool is_pixel_opaque(const Point2& p_point) const;
	bool is_editor_region_rect_draggable() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_region_enabled(bool p_enabled);
	bool is_region_enabled() const;

	void set_region_filter_clip_enabled(bool p_enabled);
	bool is_region_filter_clip_enabled() const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2i& p_coord);
	Vector2i get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	Rect2 get_rect() const;
	virtual Rect2 get_anchorable_rect() const override;

	Sprite2D();
};

// --- Folded from scene/2d/polygon_2d.h ---
#include "scene/2d/node_2d.h"

class NavigationPolygon;
class NavigationMeshSourceGeometryData2D;

class Polygon2D : public Node2D
{
	Vector<Vector2> polygon;
	Vector<Vector2> uv;
	Vector<Color> vertex_colors;
	int internal_vertices = 0;

	struct Bone
	{
		NodePath path;
		Vector<float> weights;
	};

	Vector<Bone> bone_weights;

	Color color = Color(1, 1, 1);
	Ref<Texture2D> texture;

	Size2 tex_scale = Vector2(1, 1);
	Vector2 tex_ofs;
	real_t tex_rot = 0.0;
	bool invert = false;
	real_t invert_border = 100.0;
	bool antialiased = false;

	Vector2 offset;
	mutable bool rect_cache_dirty = true;
	mutable Rect2 item_rect;

	NodePath skeleton;

	void _skeleton_bone_setup_changed();

	RID mesh;
	int last_len = 0;
	int last_index_count = 0;
	bool last_has_uv = false;
	bool last_has_color = false;
	bool last_has_bones = false;

protected:
	void _notification(int p_what);

public:
#ifdef TOOLS_ENABLED
	virtual void _edit_set_pivot(const Point2& p_pivot) override;
	virtual Point2 _edit_get_pivot() const override;
	virtual bool _edit_use_pivot() const override;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;

	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	void set_internal_vertex_count(int p_count);
	int get_internal_vertex_count() const;

	void set_uv(const Vector<Vector2>& p_uv);
	Vector<Vector2> get_uv() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_vertex_colors(const Vector<Color>& p_colors);
	Vector<Color> get_vertex_colors() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_offset(const Vector2& p_offset);
	Vector2 get_texture_offset() const;

	void set_texture_rotation(real_t p_rot);
	real_t get_texture_rotation() const;

	void set_texture_scale(const Size2& p_scale);
	Size2 get_texture_scale() const;

	void set_invert(bool p_invert);
	bool get_invert() const;

	void set_antialiased(bool p_antialiased);
	bool get_antialiased() const;

	void set_invert_border(real_t p_invert_border);
	real_t get_invert_border() const;

	void set_offset(const Vector2& p_offset);
	Vector2 get_offset() const;

	void add_bone(
		const NodePath& p_path = NodePath(), const Vector<float>& p_weights = Vector<float>());
	int get_bone_count() const;
	NodePath get_bone_path(int p_index) const;
	const Vector<float>& get_bone_weights(int p_index) const _LIFETIME_BOUND_;
	void erase_bone(int p_idx);
	void clear_bones();
	void set_bone_weights(int p_index, const Vector<float>& p_weights);
	void set_bone_path(int p_index, const NodePath& p_path);

	void set_skeleton(const NodePath& p_skeleton);
	NodePath get_skeleton() const;

#ifndef NAVIGATION_2D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

public:
#ifndef NAVIGATION_2D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	Polygon2D();
	~Polygon2D();
};



// --- End scene/2d/polygon_2d.h ---

// --- Folded from scene/2d/skeleton_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"

class Skeleton2D;

class Bone2D : public Node2D
{
	friend class Skeleton2D;
#ifdef TOOLS_ENABLED
	friend class AnimatedValuesBackup;
#endif

	Bone2D* parent_bone = nullptr;
	Skeleton2D* skeleton = nullptr;
	Transform2D rest;

	bool autocalculate_length_and_angle = true;
	real_t length = 16;
	real_t bone_angle = 0;

	int skeleton_index = -1;

	void calculate_length_and_rotation();

#ifdef TOOLS_ENABLED
	RID editor_gizmo_rid;
	bool _editor_get_bone_shape(
		Vector<Vector2>* p_shape, Vector<Vector2>* p_outline_shape, Bone2D* p_other_bone);
	bool _editor_show_bone_gizmo = true;
#endif // TOOLS ENABLED

protected:
	void _notification(int p_what);

public:
	Transform2D cache_transform;
	bool copy_transform_to_cache = true;

	Transform2D get_rest() const;
	void apply_rest();
	Transform2D get_skeleton_rest() const;

	PackedStringArray get_configuration_warnings() const override;

	void set_autocalculate_length_and_angle(bool p_autocalculate);
	bool get_autocalculate_length_and_angle() const;
	void set_length(real_t p_length);
	real_t get_length() const;
	void set_bone_angle(real_t p_angle);
	real_t get_bone_angle() const;

	int get_index_in_skeleton() const;

#ifdef TOOLS_ENABLED
	void _editor_set_show_bone_gizmo(bool p_show_gizmo);
	bool _editor_get_show_bone_gizmo() const;
#endif // TOOLS_ENABLED

	Bone2D();
	~Bone2D();
};

class SkeletonModificationStack2D;

class Skeleton2D : public Node2D
{
	friend class Bone2D;
#ifdef TOOLS_ENABLED
	friend class AnimatedValuesBackup;
#endif

	struct Bone
	{
		bool operator<(const Bone& p_bone) const { return p_bone.bone->is_greater_than(bone); }

		Bone2D* bone = nullptr;
		int parent_index = 0;
		Transform2D accum_transform;
		Transform2D rest_inverse;

		// Transform2D local_pose_cache;
		Transform2D local_pose_override;
		real_t local_pose_override_amount = 0;
		bool local_pose_override_persistent = false;
	};

	LocalVector<Bone> bones;

	bool bone_setup_dirty = true;
	void _make_bone_setup_dirty();
	void _update_bone_setup();

	bool transform_dirty = true;
	void _make_transform_dirty();
	void _update_transform();

	RID skeleton;

	Ref<SkeletonModificationStack2D> modification_stack;

	///////////////////////////////////////////////////////
	// INTERPOLATION
	struct InterpolationData
	{
		Transform2D xform_curr;
		Transform2D xform_prev;
		uint32_t last_update_physics_tick = UINT32_MAX; // Ensure tick 0 is detected as a change.
	} _interpolation_data;

	void _update_process_mode();
	void _ensure_update_interpolation_data();

protected:
	virtual void _physics_interpolated_changed() override;
	///////////////////////////////////////////////////////

	void _notification(int p_what);

public:
	int get_bone_count() const;
	Bone2D* get_bone(int p_idx);

	RID get_skeleton() const;

	void set_bone_local_pose_override(
		int p_bone_idx, Transform2D p_override, real_t p_amount, bool p_persistent = true);
	Transform2D get_bone_local_pose_override(int p_bone_idx);

	Ref<SkeletonModificationStack2D> get_modification_stack() const;
	void set_modification_stack(Ref<SkeletonModificationStack2D> p_stack);
	void execute_modifications(real_t p_delta, int p_execution_mode);

	Skeleton2D();
	~Skeleton2D();
};



// --- End scene/2d/skeleton_2d.h ---
