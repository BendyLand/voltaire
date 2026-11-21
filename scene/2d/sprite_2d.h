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
// --- Folded from scene/resources/2d/skeleton/skeleton_modification_2d.h ---
// --- Folded from scene/resources/2d/skeleton/skeleton_modification_stack_2d.h ---
#include "core/io/resource.h"

///////////////////////////////////////
// SkeletonModificationStack2D
///////////////////////////////////////

class Skeleton2D;
class SkeletonModification2D;
class Bone2D;

class SkeletonModificationStack2D : public Resource
{
	friend class Skeleton2D;
	friend class SkeletonModification2D;

protected:

public:
	Skeleton2D* skeleton = nullptr;
	bool is_setup = false;
	bool enabled = false;
	float strength = 1.0;

	enum EXECUTION_MODE
	{
		execution_mode_process,
		execution_mode_physics_process
	};

	Vector<Ref<SkeletonModification2D>> modifications;

	void setup();
	void execute(float p_delta, int p_execution_mode);

	bool editor_gizmo_dirty = false;
	void draw_editor_gizmos();
	void set_editor_gizmos_dirty(bool p_dirty);

	void enable_all_modifications(bool p_enable);
	Ref<SkeletonModification2D> get_modification(int p_mod_idx) const;
	void add_modification(Ref<SkeletonModification2D> p_mod);
	void delete_modification(int p_mod_idx);
	void set_modification(int p_mod_idx, Ref<SkeletonModification2D> p_mod);

	void set_modification_count(int p_count);
	int get_modification_count() const;

	void set_skeleton(Skeleton2D* p_skeleton);
	Skeleton2D* get_skeleton() const;

	bool get_is_setup() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	void set_strength(float p_strength);
	float get_strength() const;

	SkeletonModificationStack2D();
};



// --- End scene/resources/2d/skeleton/skeleton_modification_stack_2d.h ---

class Bone2D;
class Node2D;

class SkeletonModification2D final : public Resource {
	friend class Skeleton2D;
	friend class Bone2D;

public:
	enum Type {
		MODIFICATION_NONE,
		MODIFICATION_LOOKAT,
		MODIFICATION_CCDIK,
		MODIFICATION_FABRIK,
		MODIFICATION_JIGGLE,
		MODIFICATION_TWOBONEIK,
		MODIFICATION_PHYSICALBONES,
		MODIFICATION_STACKHOLDER,
	};

	struct LookAtData;
	struct CCDIKData;
	struct FABRIKData;
	struct JiggleData;
	struct TwoBoneIKData;
	struct PhysicalBonesData;
	struct StackHolderData;

private:
	Type modification_type = MODIFICATION_NONE;
	void *mode_data = nullptr;

	SkeletonModificationStack2D *stack = nullptr;
	int execution_mode = 0; // 0 = process
	NodePath target_node;

	bool enabled = true;
	bool is_setup = false;

	void _alloc_mode_data(Type p_type);
	void _free_mode_data();
	void _ensure_mode(Type p_expected);

	LookAtData *_get_lookat();
	const LookAtData *_get_lookat() const;
	CCDIKData *_get_ccdik();
	const CCDIKData *_get_ccdik() const;
	FABRIKData *_get_fabrik();
	const FABRIKData *_get_fabrik() const;
	JiggleData *_get_jiggle();
	const JiggleData *_get_jiggle() const;
	TwoBoneIKData *_get_twoboneik();
	const TwoBoneIKData *_get_twoboneik() const;
	PhysicalBonesData *_get_physicalbones();
	const PhysicalBonesData *_get_physicalbones() const;
	StackHolderData *_get_stackholder();
	const StackHolderData *_get_stackholder() const;

	Node2D *_resolve_node_2d(const NodePath &p_path) const;
	Bone2D *_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const;

	void _execute_lookat(float p_delta);
	void _execute_ccdik(float p_delta);
	void _execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip);
	void _execute_fabrik(float p_delta);
	void _fabrik_chain_backwards();
	void _fabrik_chain_forwards();
	void _execute_jiggle(float p_delta);
	void _execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta);
	void _update_jiggle_joint_data();
	void _execute_twoboneik(float p_delta);
	void _execute_physicalbones(float p_delta);
	void _execute_stackholder(float p_delta);

	bool _print_execution_error(bool p_condition, const String &p_message);

	virtual void reset_state() override;

public:
	bool editor_draw_gizmo = false;

	SkeletonModification2D(Type p_type = MODIFICATION_NONE);
	~SkeletonModification2D();

	void set_modification_type(Type p_type);
	Type get_modification_type() const { return modification_type; }

	void _execute(float p_delta);
	void _setup_modification(SkeletonModificationStack2D *p_stack);
	void _draw_editor_gizmo();

	void set_editor_draw_gizmo(bool p_draw_gizmo);
	bool get_editor_draw_gizmo() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	Ref<SkeletonModificationStack2D> get_modification_stack();
	void set_is_setup(bool p_setup);
	bool get_is_setup() const;

	void set_execution_mode(int p_mode);
	int get_execution_mode() const;

	float clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert_clamp = false);
	void editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted);

	// Shared target node (LookAt, CCDIK, FABRIK, Jiggle, TwoBoneIK).
	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	// LookAt accessors.
	void set_bone2d_node(const NodePath &p_target_node);
	NodePath get_bone2d_node() const;
	void set_bone_index(int p_idx);
	int get_bone_index() const;
	void set_additional_rotation(float p_rotation);
	float get_additional_rotation() const;
	void set_enable_constraint(bool p_constraint);
	bool get_enable_constraint() const;
	void set_constraint_angle_min(float p_angle_min);
	float get_constraint_angle_min() const;
	void set_constraint_angle_max(float p_angle_max);
	float get_constraint_angle_max() const;
	void set_constraint_angle_invert(bool p_invert);
	bool get_constraint_angle_invert() const;
	void set_constraint_in_localspace(bool p_constraint_in_localspace);
	bool get_constraint_in_localspace() const;

	// CCDIK accessors.
	void set_tip_node(const NodePath &p_tip_node);
	NodePath get_tip_node() const;
	int get_ccdik_data_chain_length() const;
	void set_ccdik_data_chain_length(int p_new_length);
	void set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_ccdik_joint_bone2d_node(int p_joint_idx) const;
	void set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_ccdik_joint_bone_index(int p_joint_idx) const;
	void set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint);
	bool get_ccdik_joint_rotate_from_joint(int p_joint_idx) const;
	void set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint);
	bool get_ccdik_joint_enable_constraint(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min);
	float get_ccdik_joint_constraint_angle_min(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max);
	float get_ccdik_joint_constraint_angle_max(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert);
	bool get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const;
	void set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace);
	bool get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const;
	void set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo);
	bool get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const;

	// FABRIK accessors.
	int get_fabrik_data_chain_length() const;
	void set_fabrik_data_chain_length(int p_new_length);
	void set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_fabrik_joint_bone2d_node(int p_joint_idx) const;
	void set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_fabrik_joint_bone_index(int p_joint_idx) const;
	void set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position);
	Vector2 get_fabrik_joint_magnet_position(int p_joint_idx) const;
	void set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation);
	bool get_fabrik_joint_use_target_rotation(int p_joint_idx) const;
	void set_chain_tolerance(float p_tolerance);
	float get_chain_tolerance() const;
	void set_chain_max_iterations(int p_iterations);
	int get_chain_max_iterations() const;

	// Jiggle accessors.
	void reset();
	void set_stiffness(float p_stiffness);
	float get_stiffness() const;
	void set_mass(float p_mass);
	float get_mass() const;
	void set_damping(float p_damping);
	float get_damping() const;
	void set_use_gravity(bool p_use_gravity);
	bool get_use_gravity() const;
	void set_gravity(Vector2 p_gravity);
	Vector2 get_gravity() const;
	void set_use_colliders(bool p_use_colliders);
	bool get_use_colliders() const;
	void set_collision_mask(int p_mask);
	int get_collision_mask() const;
	int get_jiggle_data_chain_length() const;
	void set_jiggle_data_chain_length(int p_new_length);
	void set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_jiggle_joint_bone2d_node(int p_joint_idx) const;
	void set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_jiggle_joint_bone_index(int p_joint_idx) const;
	void set_jiggle_joint_override(int p_joint_idx, bool p_override);
	bool get_jiggle_joint_override(int p_joint_idx) const;
	void set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness);
	float get_jiggle_joint_stiffness(int p_joint_idx) const;
	void set_jiggle_joint_mass(int p_joint_idx, float p_mass);
	float get_jiggle_joint_mass(int p_joint_idx) const;
	void set_jiggle_joint_damping(int p_joint_idx, float p_damping);
	float get_jiggle_joint_damping(int p_joint_idx) const;
	void set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity);
	bool get_jiggle_joint_use_gravity(int p_joint_idx) const;
	void set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity);
	Vector2 get_jiggle_joint_gravity(int p_joint_idx) const;

	// TwoBoneIK accessors.
	void set_target_minimum_distance(float p_minimum_distance);
	float get_target_minimum_distance() const;
	void set_target_maximum_distance(float p_maximum_distance);
	float get_target_maximum_distance() const;
	void set_flip_bend_direction(bool p_flip_direction);
	bool get_flip_bend_direction() const;
	void set_joint_one_bone2d_node(const NodePath &p_node);
	NodePath get_joint_one_bone2d_node() const;
	void set_joint_one_bone_idx(int p_bone_idx);
	int get_joint_one_bone_idx() const;
	void set_joint_two_bone2d_node(const NodePath &p_node);
	NodePath get_joint_two_bone2d_node() const;
	void set_joint_two_bone_idx(int p_bone_idx);
	int get_joint_two_bone_idx() const;
#ifdef TOOLS_ENABLED
	void set_editor_draw_min_max(bool p_draw);
	bool get_editor_draw_min_max() const;
#endif // TOOLS_ENABLED

	// PhysicalBones accessors.
	int get_physical_bone_chain_length() const;
	void set_physical_bone_chain_length(int p_new_length);
	void set_physical_bone_node(int p_joint_idx, const NodePath &p_path);
	NodePath get_physical_bone_node(int p_joint_idx) const;
	void fetch_physical_bones();
	void start_simulation(const Vector<String> &p_bones = Vector<String>());
	void stop_simulation(const Vector<String> &p_bones = Vector<String>());

	// StackHolder accessors.
	void set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack);
	Ref<SkeletonModificationStack2D> get_held_modification_stack() const;
};

// --- End scene/resources/2d/skeleton/skeleton_modification_2d.h ---

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
