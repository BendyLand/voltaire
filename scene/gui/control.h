/**************************************************************************/
/*  control.h                                                             */
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

// --- Folded from scene/main/canvas_item.h ---
#include "scene/main/node.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"
#include "scene/resources/theme.h"

class CanvasLayer;
class Font;
class Material;
class Mesh;
class MultiMesh;
class StyleBox;
class Window;
class World2D;

class CanvasItem : public Node
{
	friend class CanvasLayer;

public:
	using DrawCallback = void (*)(CanvasItem*);
    DrawCallback draw_callback = nullptr;

	RID canvas_item;

	enum TextureFilter
	{
		TEXTURE_FILTER_PARENT_NODE,
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_MAX
	};

	enum TextureRepeat
	{
		TEXTURE_REPEAT_PARENT_NODE,
		TEXTURE_REPEAT_DISABLED,
		TEXTURE_REPEAT_ENABLED,
		TEXTURE_REPEAT_MIRROR,
		TEXTURE_REPEAT_MAX,
	};

	enum ClipChildrenMode
	{
		CLIP_CHILDREN_DISABLED,
		CLIP_CHILDREN_ONLY,
		CLIP_CHILDREN_AND_DRAW,
		CLIP_CHILDREN_MAX,
	};

	enum OversamplingWithScale
	{
		OVERSAMPLING_WITH_SCALE_PARENT_NODE,
		OVERSAMPLING_WITH_SCALE_DISABLED,
		OVERSAMPLING_WITH_SCALE_ENABLED,
		OVERSAMPLING_WITH_SCALE_MAX,
	};

private:
	mutable SelfList<Node> xform_change;

	String canvas_group;

	CanvasLayer* canvas_layer = nullptr;

	Color modulate = Color(1, 1, 1, 1);
	Color self_modulate = Color(1, 1, 1, 1);

	struct Data
	{
		// An unordered vector of `CanvasItem` children only.
		// This is a subset of the `Node::children`, purely
		// an optimization for faster traversal.
		LocalVector<CanvasItem*> canvas_item_children;
		uint32_t index_in_parent = UINT32_MAX;
	};

	Data data;

	int light_mask = 1;
	uint32_t visibility_layer = 1;

	int z_index = 0;
	bool z_relative = true;
	bool y_sort_enabled = false;

	static constexpr int DEFAULT_FONT_SIZE = 16; // Keep synced with font.h.

	Window* window = nullptr;
	bool visible = true;
	bool parent_visible_in_tree = false;
	bool pending_update = false;
	bool draw_commands_dirty = false;
	bool top_level = false;
	bool block_transform_notify = false;
	bool behind = false;
	bool use_parent_material = false;
	bool notify_local_transform = false;
	bool notify_transform = false;
	bool hide_clip_children = false;
#ifdef TOOLS_ENABLED
	mutable HashMap<String, String> instance_parameter_cache;
#endif
	OversamplingWithScale oversampling_with_scale = OVERSAMPLING_WITH_SCALE_PARENT_NODE;
	double oversampling_override = -1.0;
	bool is_oversampling_with_scale_cache = false;
	double oversampling_override_cache = -1.0;

	void _update_oversampling(bool p_propagate = false);
	bool _is_oversampling_with_scale() const;

	ClipChildrenMode clip_children_mode = CLIP_CHILDREN_DISABLED;

	mutable RSE::CanvasItemTextureFilter texture_filter_cache =
		RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR;
	mutable RSE::CanvasItemTextureRepeat texture_repeat_cache =
		RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED;
	TextureFilter texture_filter = TEXTURE_FILTER_PARENT_NODE;
	TextureRepeat texture_repeat = TEXTURE_REPEAT_PARENT_NODE;

	Ref<Material> material;

	mutable Transform2D global_transform;
	mutable MTFlag global_invalid;

	_FORCE_INLINE_ bool _is_global_invalid() const
	{
		return is_group_processing() ? global_invalid.mt.is_set() : global_invalid.st;
	}

	void _set_global_invalid(bool p_invalid) const;

	void _top_level_raise_self();

	void _propagate_visibility_changed(bool p_parent_visible_in_tree);
	void _handle_visibility_change(bool p_visible);

	virtual void _top_level_changed();
	virtual void _top_level_changed_on_parent();

	void _redraw_callback();

	void _enter_canvas();
	void _exit_canvas();

	void _window_visibility_changed();

	static CanvasItem* current_item_drawn;
	friend class Viewport;
	void _refresh_texture_repeat_cache() const;
	void _update_texture_repeat_changed(bool p_propagate);
	void _refresh_texture_filter_cache() const;
	void _update_texture_filter_changed(bool p_propagate);

	void _notify_transform_deferred();
	const String* _instance_shader_parameter_get_remap(const String& p_name) const;

protected:
#ifdef TOOLS_ENABLED
	bool _property_can_revert(const String& p_name) const;
#endif

	virtual void _physics_interpolated_changed() override;

	virtual void _update_self_texture_repeat(RSE::CanvasItemTextureRepeat p_texture_repeat);
	virtual void _update_self_texture_filter(RSE::CanvasItemTextureFilter p_texture_filter);

	void item_rect_changed(bool p_size_changed = true);

	void set_canvas_item_use_identity_transform(bool p_enable);

	void _notification(int p_what);
	static void _bind_methods();

#ifndef DISABLE_DEPRECATED
	void _draw_string_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_string_outline_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_outline_bind_compat_104872(const Ref<Font>& p_font,
		const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_char_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
		const String& p_char, int p_font_size = DEFAULT_FONT_SIZE,
		const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	void _draw_char_outline_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
		const String& p_char, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	void _draw_circle_bind_compat_84472(const Point2& p_pos, real_t p_radius, const Color& p_color);
	void _draw_rect_bind_compat_84523(
		const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width);
	void _draw_dashed_line_bind_compat_84523(const Point2& p_from, const Point2& p_to,
		const Color& p_color, real_t p_width, real_t p_dash, bool p_aligned);
	void _draw_multiline_bind_compat_84523(
		const Vector<Point2>& p_points, const Color& p_color, real_t p_width);
	void _draw_multiline_colors_bind_compat_84523(
		const Vector<Point2>& p_points, const Vector<Color>& p_colors, real_t p_width);
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

	_FORCE_INLINE_ void set_hide_clip_children(bool p_value) { hide_clip_children = p_value; }

public:
	bool drawing = false;
	enum
	{
		NOTIFICATION_TRANSFORM_CHANGED = 2000, // Keep in sync with SceneTree.
		NOTIFICATION_DRAW = 30,
		NOTIFICATION_VISIBILITY_CHANGED = 31,
		NOTIFICATION_ENTER_CANVAS = 32,
		NOTIFICATION_EXIT_CANVAS = 33,
		NOTIFICATION_LOCAL_TRANSFORM_CHANGED = 35,
		NOTIFICATION_WORLD_2D_CHANGED = 36,
	};

	/* EDITOR AND DEBUGGING */

#ifdef TOOLS_ENABLED
	// Save and restore a CanvasItem state

	// Used to move the node
	virtual void _edit_set_position(const Point2& p_position);
	virtual Point2 _edit_get_position() const;

	// Used to scale the node
	virtual void _edit_set_scale(const Size2& p_scale);
	virtual Size2 _edit_get_scale() const;

	// Used to rotate the node
	virtual bool _edit_use_rotation() const { return false; }

	virtual void _edit_set_rotation(real_t p_rotation) {}

	virtual real_t _edit_get_rotation() const { return 0.0; }

	// Used to resize/move the node
	virtual void _edit_set_rect(const Rect2& p_rect) {}

	virtual Size2 _edit_get_minimum_size() const { return Size2(-1, -1); } // LOOKS WEIRD

	// Used to set a pivot
	virtual bool _edit_use_pivot() const { return false; }

	virtual void _edit_set_pivot(const Point2& p_pivot) {}

	virtual Point2 _edit_get_pivot() const { return Point2(); }

	virtual Transform2D _edit_get_transform() const;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	// Those need to be available in debug runtime, to allow for node selection.

	// Select the node.
	virtual bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;

	// Used to resize/move the node.
	virtual bool _edit_use_rect() const
	{
		return false;
	} // Maybe replace with _edit_get_editmode().

	virtual Rect2 _edit_get_rect() const { return Rect2(0, 0, 0, 0); }
#endif // DEBUG_ENABLED

	void update_draw_order();

	/* VISIBILITY */

	void set_visible(bool p_visible);
	bool is_visible() const;
	bool is_visible_in_tree() const;
	void show();
	void hide();

	void queue_redraw();
	void move_to_front();

	void set_clip_children_mode(ClipChildrenMode p_clip_mode);
	ClipChildrenMode get_clip_children_mode() const;

	virtual void set_light_mask(int p_light_mask);
	int get_light_mask() const;

	void set_modulate(const Color& p_modulate);
	Color get_modulate() const;
	Color get_modulate_in_tree() const;

	virtual void set_self_modulate(const Color& p_self_modulate);
	Color get_self_modulate() const;

	void set_visibility_layer(uint32_t p_visibility_layer);
	uint32_t get_visibility_layer() const;

	void set_visibility_layer_bit(uint32_t p_visibility_layer, bool p_enable);
	bool get_visibility_layer_bit(uint32_t p_visibility_layer) const;

	/* ORDERING */

	virtual void set_z_index(int p_z);
	int get_z_index() const;
	int get_effective_z_index() const;

	void set_z_as_relative(bool p_enabled);
	bool is_z_relative() const;

	virtual void set_y_sort_enabled(bool p_enabled);
	virtual bool is_y_sort_enabled() const;

	/* DRAWING API */

	void draw_dashed_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
		real_t p_width = -1.0, real_t p_dash = 2.0, bool p_aligned = true,
		bool p_antialiased = false);
	void draw_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
		real_t p_width = -1.0, bool p_antialiased = false);
	void draw_polyline(const Vector<Point2>& p_points, const Color& p_color, real_t p_width = -1.0,
		bool p_antialiased = false);
	void draw_polyline_colors(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		real_t p_width = -1.0, bool p_antialiased = false);
	void draw_ellipse_arc(const Vector2& p_center, real_t p_major, real_t p_minor,
		real_t p_start_angle, real_t p_end_angle, int p_point_count, const Color& p_color,
		real_t p_width = -1.0, bool p_antialiased = false);
	void draw_arc(const Vector2& p_center, real_t p_radius, real_t p_start_angle,
		real_t p_end_angle, int p_point_count, const Color& p_color, real_t p_width = -1.0,
		bool p_antialiased = false);
	void draw_multiline(const Vector<Point2>& p_points, const Color& p_color, real_t p_width = -1.0,
		bool p_antialiased = false);
	void draw_multiline_colors(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		real_t p_width = -1.0, bool p_antialiased = false);
	void draw_rect(const Rect2& p_rect, const Color& p_color, bool p_filled = true,
		real_t p_width = -1.0, bool p_antialiased = false);
	void draw_ellipse(const Point2& p_pos, real_t p_major, real_t p_minor, const Color& p_color,
		bool p_filled = true, real_t p_width = -1.0, bool p_antialiased = false);
	void draw_circle(const Point2& p_pos, real_t p_radius, const Color& p_color,
		bool p_filled = true, real_t p_width = -1.0, bool p_antialiased = false);
	void draw_texture(
		Texture2D* rp_texture, const Point2& p_pos, const Color& p_modulate = Color(1, 1, 1, 1));
	void draw_texture_rect(Texture2D* rp_texture, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	void draw_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	void draw_msdf_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), double p_outline = 0.0,
		double p_pixel_range = 4.0, double p_scale = 1.0);
	void draw_lcd_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	void draw_style_box(StyleBox* rp_style_box, const Rect2& p_rect);
	void draw_primitive(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture = Ref<Texture2D>());
	void draw_polygon(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(),
		Ref<Texture2D> p_texture = Ref<Texture2D>());
	void draw_colored_polygon(const Vector<Point2>& p_points, const Color& p_color,
		const Vector<Point2>& p_uvs = Vector<Point2>(),
		Ref<Texture2D> p_texture = Ref<Texture2D>());

	void draw_mesh(Mesh* rp_mesh, const Ref<Texture2D>& p_texture,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1));
	void draw_multimesh(MultiMesh* rp_multimesh, const Ref<Texture2D>& p_texture);

	void draw_string(Font* rp_font, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	void draw_multiline_string(Font* rp_font, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	void draw_string_outline(Font* rp_font, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	void draw_multiline_string_outline(Font* rp_font, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	void draw_char(Font* rp_font, const Point2& p_pos, const String& p_char,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		float p_oversampling = 0.0) const;
	void draw_char_outline(Font* rp_font, const Point2& p_pos, const String& p_char,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0), float p_oversampling = 0.0) const;

	void draw_set_transform(
		const Point2& p_offset, real_t p_rot = 0.0, const Size2& p_scale = Size2(1.0, 1.0));
	void draw_set_transform_matrix(const Transform2D& p_matrix);
	void draw_animation_slice(
		double p_animation_length, double p_slice_begin, double p_slice_end, double p_offset = 0);
	void draw_end_animation();

	static CanvasItem* get_current_item_drawn();

	/* RECT / TRANSFORM */

	void set_as_top_level(bool p_top_level);
	bool is_set_as_top_level() const;

	void set_draw_behind_parent(bool p_enable);
	bool is_draw_behind_parent_enabled() const;

	CanvasItem* get_parent_item() const;

	virtual Transform2D get_transform() const;

	virtual Transform2D get_global_transform() const;
	virtual Transform2D get_global_transform_const() const;
	virtual Transform2D get_global_transform_with_canvas() const;
	virtual Transform2D get_screen_transform() const;

	CanvasItem* get_top_level() const;

	_FORCE_INLINE_ RID get_canvas_item() const { return canvas_item; }

	void set_block_transform_notify(bool p_enable);
	bool is_block_transform_notify_enabled() const;

	Transform2D get_canvas_transform() const;
	Transform2D get_viewport_transform() const;
	Rect2 get_viewport_rect() const;
	RID get_viewport_rid() const;
	RID get_canvas() const;
	Ref<World2D> get_world_2d() const;

	virtual void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	virtual void set_use_parent_material(bool p_use_parent_material);
	bool get_use_parent_material() const;

	InputEvent make_input_local(const InputEvent& p_event) const;
	Vector2 make_canvas_position_local(const Vector2& screen_point) const;

	Vector2 get_global_mouse_position() const;
	Vector2 get_local_mouse_position() const;

	void set_notify_local_transform(bool p_enable);
	bool is_local_transform_notification_enabled() const;

	void set_notify_transform(bool p_enable);
	bool is_transform_notification_enabled() const;

	void force_update_transform();

	virtual void set_texture_filter(TextureFilter p_texture_filter);
	TextureFilter get_texture_filter() const;

	virtual void set_texture_repeat(TextureRepeat p_texture_repeat);
	TextureRepeat get_texture_repeat() const;

	TextureFilter get_texture_filter_in_tree() const;
	TextureRepeat get_texture_repeat_in_tree() const;

	OversamplingWithScale get_oversampling_with_scale() const;
	void set_oversampling_with_scale(OversamplingWithScale p_mode);

	// Used by control nodes to retrieve the parent's anchorable area
	virtual Rect2 get_anchorable_rect() const { return Rect2(0, 0, 0, 0); }

	int get_canvas_layer() const;
	CanvasLayer* get_canvas_layer_node() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	CanvasItem();
	~CanvasItem();
};

// --- Folded from scene/main/canvas_layer.h ---
#include "scene/main/node.h"

class Viewport;

class CanvasLayer : public Node
{
	mutable bool locrotscale_dirty = false;
	mutable Vector2 ofs;
	mutable Size2 scale = Vector2(1, 1);
	mutable real_t rot = 0.0;
	int layer = 1;
	Transform2D transform;
	RID canvas;

	Viewport* custom_viewport = nullptr;

	RID viewport;
	Viewport* vp = nullptr;

	int sort_index = 0;
	bool visible = true;

	bool follow_viewport = false;
	float follow_viewport_scale = 1.0;

	struct ParallaxState
	{
		Point2 offset;
		real_t scale = 1.0;
		Point2 base_offset;
		Point2 base_scale = Vector2(1, 1);
		Point2 screen_offset;
		String group_name;
		Point2 limit_begin;
		Point2 limit_end;
		Point2 final_offset;
		bool ignore_camera_zoom = false;
	};

	bool parallax_enabled = false;
	ParallaxState parallax;

	void _update_xform();
	void _update_locrotscale() const;
	void _update_follow_viewport(bool p_force_exit = false);
	void _update_scroll();

protected:
	void _notification(int p_what);

public:
	void update_draw_order();

	void set_layer(int p_xform);
	int get_layer() const;

	void set_visible(bool p_visible);
	bool is_visible() const;
	void show();
	void hide();

	void set_transform(const Transform2D& p_xform);
	Transform2D get_transform() const;
	Transform2D get_final_transform() const;

	void set_offset(const Vector2& p_offset);
	Vector2 get_offset() const;

	void set_rotation(real_t p_radians);
	real_t get_rotation() const;

	void set_scale(const Size2& p_scale);
	Size2 get_scale() const;

	Size2 get_viewport_size() const;

	RID get_viewport() const;

	void set_custom_viewport(Node* p_viewport);
	Node* get_custom_viewport() const;

	void reset_sort_index();
	int get_sort_index();

	void set_follow_viewport(bool p_enable);
	bool is_following_viewport() const;

	void set_follow_viewport_scale(float p_ratio);
	float get_follow_viewport_scale() const;

	RID get_canvas() const;

	// Parallax background support.
	void set_parallax_enabled(bool p_enabled);
	bool is_parallax_enabled() const { return parallax_enabled; }

	void _camera_moved(const Transform2D& p_transform, const Point2& p_screen_offset,
		const Point2& p_adj_screen_offset);

	void set_scroll_offset(const Point2& p_ofs);
	Point2 get_scroll_offset() const;

	void set_scroll_scale(real_t p_scale);
	real_t get_scroll_scale() const;

	void set_scroll_base_offset(const Point2& p_ofs);
	Point2 get_scroll_base_offset() const;

	void set_scroll_base_scale(const Point2& p_ofs);
	Point2 get_scroll_base_scale() const;

	void set_limit_begin(const Point2& p_ofs);
	Point2 get_limit_begin() const;

	void set_limit_end(const Point2& p_ofs);
	Point2 get_limit_end() const;

	void set_ignore_camera_zoom(bool p_ignore);
	bool is_ignore_camera_zoom() const;

	Vector2 get_final_offset() const;

	CanvasLayer(bool p_parallax = false);
	~CanvasLayer();
};
// --- End scene/main/canvas_item.h ---
#include "scene/resources/material.h"
#include "scene/resources/theme.h"
#include "servers/display/display_server.h"

class Label;
class Panel;
class ThemeContext;
class ThemeOwner;
class Viewport;
struct Transform2D;

class Control : public CanvasItem
{
#ifdef TOOLS_ENABLED
	bool saving = false;
#endif // TOOLS_ENABLED

public:
	enum Anchor
	{
		ANCHOR_BEGIN = 0,
		ANCHOR_END = 1
	};

	enum GrowDirection
	{
		GROW_DIRECTION_BEGIN,
		GROW_DIRECTION_END,
		GROW_DIRECTION_BOTH
	};

	enum FocusMode
	{
		FOCUS_NONE,
		FOCUS_CLICK,
		FOCUS_ALL,
		FOCUS_ACCESSIBILITY,
	};

	enum FocusBehaviorRecursive
	{
		FOCUS_BEHAVIOR_INHERITED,
		FOCUS_BEHAVIOR_DISABLED,
		FOCUS_BEHAVIOR_ENABLED,
	};

	enum SizeFlags
	{
		SIZE_SHRINK_BEGIN = 0,
		SIZE_FILL = 1,
		SIZE_EXPAND = 2,
		SIZE_SHRINK_CENTER = 4,
		SIZE_SHRINK_END = 8,

		SIZE_EXPAND_FILL = SIZE_EXPAND | SIZE_FILL,
	};

	enum MouseFilter
	{
		MOUSE_FILTER_STOP,
		MOUSE_FILTER_PASS,
		MOUSE_FILTER_IGNORE
	};

	enum MouseBehaviorRecursive
	{
		MOUSE_BEHAVIOR_INHERITED,
		MOUSE_BEHAVIOR_DISABLED,
		MOUSE_BEHAVIOR_ENABLED,
	};

	// Keep synced with InputClassEnums and DisplayServerEnums enums.
	enum CursorShape
	{
		CURSOR_ARROW,
		CURSOR_IBEAM,
		CURSOR_POINTING_HAND,
		CURSOR_CROSS,
		CURSOR_WAIT,
		CURSOR_BUSY,
		CURSOR_DRAG,
		CURSOR_CAN_DROP,
		CURSOR_FORBIDDEN,
		CURSOR_VSIZE,
		CURSOR_HSIZE,
		CURSOR_BDIAGSIZE,
		CURSOR_FDIAGSIZE,
		CURSOR_MOVE,
		CURSOR_VSPLIT,
		CURSOR_HSPLIT,
		CURSOR_HELP,
		CURSOR_MAX
	};

	enum LayoutPreset
	{
		PRESET_TOP_LEFT,
		PRESET_TOP_RIGHT,
		PRESET_BOTTOM_LEFT,
		PRESET_BOTTOM_RIGHT,
		PRESET_CENTER_LEFT,
		PRESET_CENTER_TOP,
		PRESET_CENTER_RIGHT,
		PRESET_CENTER_BOTTOM,
		PRESET_CENTER,
		PRESET_LEFT_WIDE,
		PRESET_TOP_WIDE,
		PRESET_RIGHT_WIDE,
		PRESET_BOTTOM_WIDE,
		PRESET_VCENTER_WIDE,
		PRESET_HCENTER_WIDE,
		PRESET_FULL_RECT
	};

	enum LayoutPresetMode
	{
		PRESET_MODE_MINSIZE,
		PRESET_MODE_KEEP_WIDTH,
		PRESET_MODE_KEEP_HEIGHT,
		PRESET_MODE_KEEP_SIZE
	};

	enum LayoutMode
	{
		LAYOUT_MODE_POSITION,
		LAYOUT_MODE_ANCHORS,
		LAYOUT_MODE_CONTAINER,
		LAYOUT_MODE_UNCONTROLLED,
	};

	enum LayoutDirection
	{
		LAYOUT_DIRECTION_INHERITED,
		LAYOUT_DIRECTION_APPLICATION_LOCALE,
		LAYOUT_DIRECTION_LTR,
		LAYOUT_DIRECTION_RTL,
		LAYOUT_DIRECTION_SYSTEM_LOCALE,
		LAYOUT_DIRECTION_MAX,
#ifndef DISABLE_DEPRECATED
		LAYOUT_DIRECTION_LOCALE = LAYOUT_DIRECTION_APPLICATION_LOCALE,
#endif // DISABLE_DEPRECATED
	};

	enum TextDirection
	{
		TEXT_DIRECTION_AUTO = TextServer::DIRECTION_AUTO,
		TEXT_DIRECTION_LTR = TextServer::DIRECTION_LTR,
		TEXT_DIRECTION_RTL = TextServer::DIRECTION_RTL,
		TEXT_DIRECTION_INHERITED = TextServer::DIRECTION_INHERITED,
	};

	struct LayoutConfig
	{
		enum Type
		{
			LAYOUT_NONE,
			LAYOUT_BOX,
			LAYOUT_GRID,
			LAYOUT_FLOW,
			LAYOUT_MARGIN,
			LAYOUT_CENTER,
			LAYOUT_ASPECT_RATIO,
			LAYOUT_SPLIT,
		};

		enum AlignmentMode
		{
			ALIGNMENT_BEGIN,
			ALIGNMENT_CENTER,
			ALIGNMENT_END,
		};

		enum LastWrapAlignmentMode
		{
			LAST_WRAP_ALIGNMENT_INHERIT,
			LAST_WRAP_ALIGNMENT_BEGIN,
			LAST_WRAP_ALIGNMENT_CENTER,
			LAST_WRAP_ALIGNMENT_END,
		};

		enum StretchMode
		{
			STRETCH_WIDTH_CONTROLS_HEIGHT,
			STRETCH_HEIGHT_CONTROLS_WIDTH,
			STRETCH_FIT,
			STRETCH_COVER,
		};

		enum SortableVisibilityMode
		{
			SORTABLE_VISIBILITY_VISIBLE,
			SORTABLE_VISIBILITY_VISIBLE_IN_TREE,
			SORTABLE_VISIBILITY_IGNORE,
		};

		Type type = LAYOUT_NONE;
		Orientation orientation = HORIZONTAL;
		AlignmentMode alignment = ALIGNMENT_BEGIN;
		AlignmentMode v_alignment = ALIGNMENT_CENTER;
		LastWrapAlignmentMode last_wrap_alignment = LAST_WRAP_ALIGNMENT_INHERIT;
		StretchMode stretch_mode = STRETCH_FIT;
		int columns = 1;
		int separation = 0;
		int h_separation = 0;
		int v_separation = 0;
		int margins[4] = {0, 0, 0, 0};
		float ratio = 1.0f;
		bool reverse_fill = false;
		bool use_top_left = false;
		bool accessibility_region = false;
		bool pending_sort = false;
	};

	LayoutConfig layout;

private:
	// This Data struct is to avoid namespace pollution in derived classes.
	struct Data
	{
		struct OffsetTransform
		{
			static constexpr Vector2 DEFAULT_TRANSLATION_ABSOLUTE = Vector2();
			static constexpr Vector2 DEFAULT_TRANSLATION_RELATIVE = Vector2();
			static constexpr Vector2 DEFAULT_SCALE = Vector2(1, 1);
			static constexpr real_t DEFAULT_ROTATION = 0.0;
			static constexpr Vector2 DEFAULT_PIVOT_ABSOLUTE = Vector2();
			static constexpr Vector2 DEFAULT_PIVOT_RELATIVE = Vector2(0.5, 0.5);
			static constexpr bool DEFAULT_VISUAL_ONLY = true;

			bool enabled = false;
			Vector2 translation_absolute = DEFAULT_TRANSLATION_ABSOLUTE;
			Vector2 translation_relative = DEFAULT_TRANSLATION_RELATIVE;
			Vector2 scale = DEFAULT_SCALE;
			real_t rotation = DEFAULT_ROTATION;
			Vector2 pivot_absolute = DEFAULT_PIVOT_ABSOLUTE;
			Vector2 pivot_relative = DEFAULT_PIVOT_RELATIVE;
			bool visual_only = DEFAULT_VISUAL_ONLY;
		};

		bool initialized = false;

		// Global relations.

		List<Control*>::Element* RI = nullptr;

		Control* parent_control = nullptr;
		Window* parent_window = nullptr;
		CanvasItem* parent_canvas_item = nullptr;

		// Positioning and sizing.
		LayoutMode stored_layout_mode = LayoutMode::LAYOUT_MODE_POSITION;
		bool stored_use_custom_anchors = false;

		real_t offset[4] = {0.0, 0.0, 0.0, 0.0};
		real_t anchor[4] = {ANCHOR_BEGIN, ANCHOR_BEGIN, ANCHOR_BEGIN, ANCHOR_BEGIN};
		FocusMode focus_mode = FOCUS_NONE;
		FocusBehaviorRecursive focus_behavior_recursive = FOCUS_BEHAVIOR_INHERITED;
		bool parent_focus_behavior_recursive_enabled = false;
		GrowDirection h_grow = GROW_DIRECTION_END;
		GrowDirection v_grow = GROW_DIRECTION_END;

		real_t rotation = 0.0;
		Vector2 scale = Vector2(1, 1);
		Vector2 pivot_offset;
		Vector2 pivot_offset_ratio;

		OffsetTransform* offset_transform = nullptr;

		Point2 pos_cache;
		Size2 size_cache;

		mutable Size2 maximum_size_cache;
		mutable bool maximum_size_valid = false;

		mutable Size2 parent_maximum_size_cache = Size2(-1, -1);

		Size2 last_maximum_size;
		bool updating_last_maximum_size = false;
		bool block_maximum_size_adjust = false;

		mutable Size2 minimum_size_cache;
		mutable bool minimum_size_valid = false;

		Size2 last_minimum_size;
		bool updating_last_minimum_size = false;
		bool block_minimum_size_adjust = false;

		mutable Size2 desired_size_cache;
		mutable bool desired_size_valid = false;

		Size2 last_desired_size;
		bool updating_last_desired_size = false;

		bool expanded_by_desired_size = false;

		bool layout_pending = false;

		bool size_warning = true;

		// Container sizing.

		uint32_t h_size_flags = SIZE_FILL;
		uint32_t v_size_flags = SIZE_FILL;
		real_t expand = 1.0;
		Size2 custom_maximum_size = Size2(-1, -1);
		Size2 custom_minimum_size;

		bool propagate_maximum_size = false;

		// Input events and rendering.

		MouseFilter mouse_filter = MOUSE_FILTER_STOP;
		MouseBehaviorRecursive mouse_behavior_recursive = MOUSE_BEHAVIOR_INHERITED;
		bool parent_mouse_behavior_recursive_enabled = true;
		bool force_pass_scroll_events = true;

		bool clip_contents = false;
		bool disable_visibility_clip = false;

		CursorShape default_cursor = CURSOR_ARROW;

		// Focus.

		NodePath focus_neighbor[4];
		NodePath focus_next;
		NodePath focus_prev;


		// Accessibility.

		String accessibility_name;
		String accessibility_description;
		AccessibilityServerEnums::AccessibilityLiveMode accessibility_live =
			AccessibilityServerEnums::AccessibilityLiveMode::LIVE_OFF;

		Vector<NodePath> accessibility_controls_nodes;
		Vector<NodePath> accessibility_described_by_nodes;
		Vector<NodePath> accessibility_labeled_by_nodes;
		Vector<NodePath> accessibility_flow_to_nodes;

		// Theming.

		ThemeOwner* theme_owner = nullptr;
		Ref<Theme> theme;
		String theme_type_variation;

		bool bulk_theme_override = false;
		Theme::ThemeIconMap theme_icon_override;
		Theme::ThemeStyleMap theme_style_override;
		Theme::ThemeFontMap theme_font_override;
		Theme::ThemeFontSizeMap theme_font_size_override;
		Theme::ThemeColorMap theme_color_override;
		Theme::ThemeConstantMap theme_constant_override;

		mutable HashMap<String, Theme::ThemeIconMap> theme_icon_cache;
		mutable HashMap<String, Theme::ThemeStyleMap> theme_style_cache;
		mutable HashMap<String, Theme::ThemeFontMap> theme_font_cache;
		mutable HashMap<String, Theme::ThemeFontSizeMap> theme_font_size_cache;
		mutable HashMap<String, Theme::ThemeColorMap> theme_color_cache;
		mutable HashMap<String, Theme::ThemeConstantMap> theme_constant_cache;

		// Internationalization.

		LayoutDirection layout_dir = LAYOUT_DIRECTION_INHERITED;
		mutable bool is_rtl_dirty = true;
		mutable bool is_rtl = false;

		bool localize_numeral_system = true;

		// Extra properties.

		String tooltip;
		String translation_context;
		AutoTranslateMode tooltip_auto_translate_mode = AUTO_TRANSLATE_MODE_INHERIT;

	} data;

	// Dynamic properties.

	static constexpr unsigned properties_managed_by_container_count = 12;
	static String properties_managed_by_container[properties_managed_by_container_count];

	// Global relations.

	friend class Viewport;

	// Positioning and sizing.

	void _update_canvas_item_transform();
	Transform2D _get_internal_transform() const;

	void _set_anchor(Side p_side, real_t p_anchor);
	void _set_position(const Point2& p_point);
	void _set_global_position(const Point2& p_point);
	void _set_size(const Size2& p_size);

	void _compute_layout_rect(Rect2 p_rect, bool p_keep_offsets = false);

	void _set_layout_mode(LayoutMode p_mode);
	void _update_layout_mode();
	LayoutMode _get_layout_mode() const;
	LayoutMode _get_default_layout_mode() const;
	void _set_anchors_layout_preset(int p_preset);
	int _get_anchors_layout_preset() const;

	void _update_maximum_size_cache() const;
	void _update_maximum_size();
	void _update_minimum_size_cache() const;
	void _update_minimum_size();
	void _update_desired_size_cache() const;
	void _update_desired_size();
	void _grow_to_desired_size();
	void _size_changed();

	void _top_level_changed() override {
	} // Controls don't need to do anything, only other CanvasItems.

	void _top_level_changed_on_parent() override;

	void _clear_size_warning();

	// Input events.

	void _call_gui_input(const InputEvent& p_event);

	// Mouse Filter.

	bool _is_mouse_filter_enabled() const;
	void _update_mouse_behavior_recursive();
	void _propagate_mouse_behavior_recursive_recursively(bool p_enabled, bool p_skip_non_inherited);

	// Focus.

	void _window_find_focus_neighbor(const Vector2& p_dir, Node* p_at, const Rect2& p_rect,
		const Rect2& p_clamp, real_t p_min, real_t& r_closest_dist_squared, Control** r_closest);
	Control* _get_focus_neighbor(Side p_side, int p_count = 0);
	bool _is_focus_mode_enabled() const;
	void _update_focus_behavior_recursive();
	void _propagate_focus_behavior_recursive_recursively(bool p_enabled, bool p_skip_non_inherited);

	// Theming.

	void _theme_changed();
	void _notify_theme_override_changed();
	void _invalidate_theme_cache();

	void _ensure_allocated_offset_transform();

	// Extra properties.

	static int root_layout_direction;

protected:
	bool _property_can_revert(const String& p_name) const;

	// Localization

	virtual String _get_translation_context_with_override(
		const String& p_context) const override;

	// Theming.

	virtual void _update_theme_item_cache();

	// Base object overrides.

	void _notification(int p_notification);
	static void _bind_methods();

#ifndef DISABLE_DEPRECATED
	bool _has_focus_bind_compat_110250() const;
	void _grab_focus_bind_compat_110250();
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

	// Focus.
	bool _is_focusable() const;

	// Node overrides.

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	enum
	{
		NOTIFICATION_RESIZED = 40,
		NOTIFICATION_MOUSE_ENTER = 41,
		NOTIFICATION_MOUSE_EXIT = 42,
		NOTIFICATION_FOCUS_ENTER = 43,
		NOTIFICATION_FOCUS_EXIT = 44,
		NOTIFICATION_THEME_CHANGED = 45,
		NOTIFICATION_SCROLL_BEGIN = 47,
		NOTIFICATION_SCROLL_END = 48,
		NOTIFICATION_LAYOUT_DIRECTION_CHANGED = 49,
		NOTIFICATION_PRE_SORT_CHILDREN = 50,
		NOTIFICATION_SORT_CHILDREN = 51,
		NOTIFICATION_MOUSE_ENTER_SELF = 60,
		NOTIFICATION_MOUSE_EXIT_SELF = 61,
	};

	// Editor plugin interoperability.

	// TODO: Decouple controls from their editor plugin and get rid of this.
#ifdef TOOLS_ENABLED
	virtual void _edit_set_position(const Point2& p_position) override;
	virtual Point2 _edit_get_position() const override;

	virtual void _edit_set_scale(const Size2& p_scale) override;
	virtual Size2 _edit_get_scale() const override;

	virtual void _edit_set_rect(const Rect2& p_edit_rect) override;

	virtual void _edit_set_rotation(real_t p_rotation) override;
	virtual real_t _edit_get_rotation() const override;
	virtual bool _edit_use_rotation() const override;

	virtual void _edit_set_pivot(const Point2& p_pivot) override;
	virtual Point2 _edit_get_pivot() const override;
	virtual bool _edit_use_pivot() const override;

	virtual Size2 _edit_get_minimum_size() const override;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void reparent(Node* p_parent, bool p_keep_global_transform = true);

	// Editor integration.

	static void set_root_layout_direction(int p_root_dir);

	PackedStringArray get_configuration_warnings() const override;
	PackedStringArray get_accessibility_configuration_warnings() const ;
#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif // TOOLS_ENABLED

	virtual bool is_text_field() const;

	// Global relations.

	Control* get_parent_control() const;
	Window* get_parent_window() const;
	Control* get_root_parent_control() const;

	Size2 get_parent_area_size() const;
	Rect2 get_parent_anchorable_rect() const;

	// Positioning and sizing.

	virtual Transform2D get_transform() const override;

	void set_anchor(Side p_side, real_t p_anchor, bool p_keep_offset = true,
		bool p_push_opposite_anchor = true);
	real_t get_anchor(Side p_side) const;
	void set_offset(Side p_side, real_t p_value);
	real_t get_offset(Side p_side) const;
	void set_anchor_and_offset(
		Side p_side, real_t p_anchor, real_t p_pos, bool p_push_opposite_anchor = true);

	// TODO: Rename to set_begin/end_offsets ?
	void set_begin(const Point2& p_point);
	Point2 get_begin() const;
	void set_end(const Point2& p_point);
	Point2 get_end() const;

	void set_h_grow_direction(GrowDirection p_direction);
	GrowDirection get_h_grow_direction() const;
	void set_v_grow_direction(GrowDirection p_direction);
	GrowDirection get_v_grow_direction() const;

	void set_anchors_preset(LayoutPreset p_preset, bool p_keep_offsets = true);
	void set_offsets_preset(LayoutPreset p_preset,
		LayoutPresetMode p_resize_mode = PRESET_MODE_MINSIZE, int p_margin = 0);
	void set_anchors_and_offsets_preset(LayoutPreset p_preset,
		LayoutPresetMode p_resize_mode = PRESET_MODE_MINSIZE, int p_margin = 0);
	void set_grow_direction_preset(LayoutPreset p_preset);

	void set_position(const Point2& p_point, bool p_keep_offsets = false);
	void set_global_position(const Point2& p_point, bool p_keep_offsets = false);
	Point2 get_position() const;
	Point2 get_global_position() const;
	Point2 get_screen_position() const;

	void set_size(const Size2& p_size, bool p_keep_offsets = false);
	Size2 get_size() const;
	void reset_size();

	void set_rect(const Rect2&
			p_rect); // Reset anchors to begin and set rect, for faster container children sorting.
	Rect2 get_rect() const;
	Rect2 get_global_rect() const;
	Rect2 get_screen_rect() const;
	Rect2 get_anchorable_rect() const override;

	void set_scale(const Vector2& p_scale);
	Vector2 get_scale() const;
	void set_rotation(real_t p_radians);
	void set_rotation_degrees(real_t p_degrees);
	real_t get_rotation() const;
	real_t get_rotation_degrees() const;
	void set_pivot_offset_ratio(const Vector2& p_ratio);
	Vector2 get_pivot_offset_ratio() const;
	void set_pivot_offset(const Vector2& p_pivot);
	Vector2 get_pivot_offset() const;
	Vector2 get_combined_pivot_offset() const;

	void set_propagate_maximum_size(bool p_propagate);
	bool is_propagating_maximum_size();

	void update_maximum_size();
	void update_minimum_size();
	void update_desired_size();

	void grow_to_desired_size();
	bool is_expanded_by_desired_size() const;

	void set_block_maximum_size_adjust(bool p_block);
	void set_block_minimum_size_adjust(bool p_block);

	virtual Size2 get_maximum_size() const;
	virtual Size2 get_combined_maximum_size() const;
	virtual Size2 get_inner_combined_maximum_size() const;

	void set_custom_maximum_size(const Size2& p_custom);
	Size2 get_custom_maximum_size() const;

	void set_parent_maximum_size_cache(const Size2& p_size);

	virtual Size2 get_minimum_size() const;
	virtual Size2 get_combined_minimum_size() const;

	void set_custom_minimum_size(const Size2& p_custom);
	Size2 get_custom_minimum_size() const;

	virtual Size2 get_bound_minimum_size() const;

	Size2 get_bound_desired_size() const;
	virtual Size2 get_desired_size() const;

	bool is_layout_pending() const;
	bool is_layout_pending_in_tree() const;
	void layout_pending_start();
	void layout_pending_finish();
	Control* get_layout_pending_control_in_tree() const;

	// Container sizing & composed layout.

	void set_h_size_flags(uint32_t p_flags);
	uint32_t get_h_size_flags() const;
	void set_v_size_flags(uint32_t p_flags);
	uint32_t get_v_size_flags() const;
	void set_stretch_ratio(real_t p_ratio);
	real_t get_stretch_ratio() const;

	void set_layout_type(LayoutConfig::Type p_type);
	LayoutConfig::Type get_layout_type() const { return layout.type; }
	void queue_sort();
	void fit_child_in_rect(Control* p_child, const Rect2& p_rect);
	Control* as_sortable_control(Node* p_node,
		LayoutConfig::SortableVisibilityMode p_visibility_mode =
			LayoutConfig::SORTABLE_VISIBILITY_VISIBLE_IN_TREE) const;
	Control* add_spacer(bool p_begin = false);

	static Control* create_box(Orientation p_orientation,
		LayoutConfig::AlignmentMode p_alignment = LayoutConfig::ALIGNMENT_BEGIN);
	static Control* create_hbox(
		LayoutConfig::AlignmentMode p_alignment = LayoutConfig::ALIGNMENT_BEGIN)
	{
		return create_box(HORIZONTAL, p_alignment);
	}
	static Control* create_vbox(
		LayoutConfig::AlignmentMode p_alignment = LayoutConfig::ALIGNMENT_BEGIN)
	{
		return create_box(VERTICAL, p_alignment);
	}
	static Control* create_grid(int p_columns = 1);
	static Control* create_flow(Orientation p_orientation = HORIZONTAL);
	static Control* create_margin(
		int p_left = 0, int p_top = 0, int p_right = 0, int p_bottom = 0);

	virtual Vector<int> get_allowed_size_flags_horizontal() const;
	virtual Vector<int> get_allowed_size_flags_vertical() const;

	// Offset transform.

	void set_offset_transform_enabled(bool p_enabled);
	bool is_offset_transform_enabled() const;
	void set_offset_transform_position(const Vector2& p_offset);
	Vector2 get_offset_transform_position() const;
	void set_offset_transform_position_ratio(const Vector2& p_offset);
	Vector2 get_offset_transform_position_ratio() const;
	void set_offset_transform_scale(const Vector2& p_scale);
	Vector2 get_offset_transform_scale() const;
	void set_offset_transform_rotation(real_t p_rotation);
	real_t get_offset_transform_rotation() const;
	void set_offset_transform_pivot(const Vector2& p_pivot);
	Vector2 get_offset_transform_pivot() const;
	void set_offset_transform_pivot_ratio(const Vector2& p_pivot);
	Vector2 get_offset_transform_pivot_ratio() const;
	void set_offset_transform_visual_only(bool p_enabled);
	bool is_offset_transform_visual_only() const;
	Transform2D get_offset_transform() const;

	// Input events.
	void accept_event();

	virtual bool has_point(const Point2& p_point) const;

	void set_mouse_filter(MouseFilter p_filter);
	MouseFilter get_mouse_filter() const;
	MouseFilter get_mouse_filter_with_override() const;

	void set_mouse_behavior_recursive(MouseBehaviorRecursive p_mouse_behavior_recursive);
	MouseBehaviorRecursive get_mouse_behavior_recursive() const;

	void set_force_pass_scroll_events(bool p_force_pass_scroll_events);
	bool is_force_pass_scroll_events() const;

	void warp_mouse(const Point2& p_position);

	bool is_focus_owner_in_shortcut_context() const;
	void set_shortcut_context(const Node* p_node);
	Node* get_shortcut_context() const;

	// Drag and drop handling.

	void set_drag_preview(Control* p_control);
	void accessibility_drag();
	void accessibility_drop();
	bool is_drag_successful() const;

	// Focus.

	void set_focus_mode(FocusMode p_focus_mode);
	FocusMode get_focus_mode() const;
	FocusMode get_focus_mode_with_override() const;
	void set_focus_behavior_recursive(FocusBehaviorRecursive p_focus_behavior_recursive);
	FocusBehaviorRecursive get_focus_behavior_recursive() const;
	bool has_focus(bool p_ignore_hidden_focus = false) const;
	void grab_focus(bool p_hide_focus = false);
	void grab_click_focus();
	void release_focus();

	Control* find_next_valid_focus() const;
	Control* find_prev_valid_focus() const;
	Control* find_valid_focus_neighbor(Side p_size) const;

	void set_focus_neighbor(Side p_side, const NodePath& p_neighbor);
	NodePath get_focus_neighbor(Side p_side) const;

	void set_focus_next(const NodePath& p_next);
	NodePath get_focus_next() const;
	void set_focus_previous(const NodePath& p_prev);
	NodePath get_focus_previous() const;

	// Accessibility.

	virtual String get_accessibility_container_name(const Node* p_node) const;

	void set_accessibility_name(const String& p_name);
	String get_accessibility_name() const;

	virtual String _get_accessibility_name() const;

	void set_accessibility_description(const String& p_description);
	String get_accessibility_description() const;

	void set_accessibility_live(AccessibilityServerEnums::AccessibilityLiveMode p_mode);
	AccessibilityServerEnums::AccessibilityLiveMode get_accessibility_live() const;

	void set_accessibility_controls_nodes(const TypedArray<NodePath>& p_node_path);
	TypedArray<NodePath> get_accessibility_controls_nodes() const;

	void set_accessibility_described_by_nodes(const TypedArray<NodePath>& p_node_path);
	TypedArray<NodePath> get_accessibility_described_by_nodes() const;

	void set_accessibility_labeled_by_nodes(const TypedArray<NodePath>& p_node_path);
	TypedArray<NodePath> get_accessibility_labeled_by_nodes() const;

	void set_accessibility_flow_to_nodes(const TypedArray<NodePath>& p_node_path);
	TypedArray<NodePath> get_accessibility_flow_to_nodes() const;

	virtual Transform2D get_accessibility_transform() const override { return get_transform(); }

	// Rendering.

	void set_default_cursor_shape(CursorShape p_shape);
	CursorShape get_default_cursor_shape() const;
	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2()) const;

	void set_clip_contents(bool p_clip);
	bool is_clipping_contents();

	void set_disable_visibility_clip(bool p_ignore);
	bool is_visibility_clip_disabled() const;

	// Theming.

	void set_theme_owner_node(Node* p_node);
	Node* get_theme_owner_node() const;
	bool has_theme_owner_node() const;

	void set_theme_context(ThemeContext* p_context, bool p_propagate = true);

	void set_theme(const Ref<Theme>& p_theme);
	Ref<Theme> get_theme() const;

	void set_theme_type_variation(const String& p_theme_type);
	String get_theme_type_variation() const;

	void begin_bulk_theme_override();
	void end_bulk_theme_override();

	void add_theme_icon_override(const String& p_name, Texture2D* rp_icon);
	void add_theme_style_override(const String& p_name, StyleBox* rp_style);
	void add_theme_font_override(const String& p_name, Font* rp_font);
	void add_theme_font_size_override(const String& p_name, int p_font_size);
	void add_theme_color_override(const String& p_name, const Color& p_color);
	void add_theme_constant_override(const String& p_name, int p_constant);

	void remove_theme_icon_override(const String& p_name);
	void remove_theme_style_override(const String& p_name);
	void remove_theme_font_override(const String& p_name);
	void remove_theme_font_size_override(const String& p_name);
	void remove_theme_color_override(const String& p_name);
	void remove_theme_constant_override(const String& p_name);

	Ref<Texture2D> get_theme_icon(
		const String& p_name, const String& p_theme_type = String()) const;
	Ref<StyleBox> get_theme_stylebox(
		const String& p_name, const String& p_theme_type = String()) const;
	Ref<Font> get_theme_font(
		const String& p_name, const String& p_theme_type = String()) const;
	int get_theme_font_size(
		const String& p_name, const String& p_theme_type = String()) const;
	Color get_theme_color(
		const String& p_name, const String& p_theme_type = String()) const;
	int get_theme_constant(
		const String& p_name, const String& p_theme_type = String()) const;
#ifdef TOOLS_ENABLED
	Ref<Texture2D> get_editor_theme_icon(const String& p_name) const;
#endif // TOOLS_ENABLED

	bool has_theme_icon_override(const String& p_name) const;
	bool has_theme_stylebox_override(const String& p_name) const;
	bool has_theme_font_override(const String& p_name) const;
	bool has_theme_font_size_override(const String& p_name) const;
	bool has_theme_color_override(const String& p_name) const;
	bool has_theme_constant_override(const String& p_name) const;

	bool has_theme_icon(
		const String& p_name, const String& p_theme_type = String()) const;
	bool has_theme_stylebox(
		const String& p_name, const String& p_theme_type = String()) const;
	bool has_theme_font(
		const String& p_name, const String& p_theme_type = String()) const;
	bool has_theme_font_size(
		const String& p_name, const String& p_theme_type = String()) const;
	bool has_theme_color(
		const String& p_name, const String& p_theme_type = String()) const;
	bool has_theme_constant(
		const String& p_name, const String& p_theme_type = String()) const;

	float get_theme_default_base_scale() const;
	Ref<Font> get_theme_default_font() const;
	int get_theme_default_font_size() const;

	// Internationalization.

	void set_layout_direction(LayoutDirection p_direction);
	LayoutDirection get_layout_direction() const;
	virtual bool is_layout_rtl() const;

	void set_localize_numeral_system(bool p_enable);
	bool is_localizing_numeral_system() const;

#ifndef DISABLE_DEPRECATED
	void set_auto_translate(bool p_enable);
	bool is_auto_translating() const;
#endif // DISABLE_DEPRECATED

	void set_tooltip_auto_translate_mode(AutoTranslateMode p_mode);
	AutoTranslateMode get_tooltip_auto_translate_mode() const;
	virtual AutoTranslateMode get_tooltip_auto_translate_mode_at(const Vector2& p_at) const;

	// Extra properties.

	String get_tooltip_text() const;
	void set_tooltip_text(const String& text);
	String get_translation_context() const;
	void set_translation_context(const String& p_context);
	virtual Control* make_custom_tooltip(const String& p_text) const;

	virtual String accessibility_get_contextual_info() const;

	Control() = default;
	~Control() = default;
};

// G = get_drag_data_fw, C = can_drop_data_fw, D = drop_data_fw, U = underscore
#define SET_DRAG_FORWARDING_CD(from, to)                                                           \
	from->set_drag_forwarding(Callable(), callable_mp(this, &to::can_drop_data_fw).bind(from),     \
		callable_mp(this, &to::drop_data_fw).bind(from));
#define SET_DRAG_FORWARDING_CDU(from, to)                                                          \
	from->set_drag_forwarding(Callable(), callable_mp(this, &to::_can_drop_data_fw).bind(from),    \
		callable_mp(this, &to::_drop_data_fw).bind(from));
#define SET_DRAG_FORWARDING_GCD(from, to)                                                          \
	from->set_drag_forwarding(callable_mp(this, &to::get_drag_data_fw).bind(from),                 \
		callable_mp(this, &to::can_drop_data_fw).bind(from),                                       \
		callable_mp(this, &to::drop_data_fw).bind(from));
#define SET_DRAG_FORWARDING_GCDU(from, to)                                                         \
	from->set_drag_forwarding(callable_mp(this, &to::_get_drag_data_fw).bind(from),                \
		callable_mp(this, &to::_can_drop_data_fw).bind(from),                                      \
		callable_mp(this, &to::_drop_data_fw).bind(from));


