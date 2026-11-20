/**************************************************************************/
/*  graph_node.h                                                          */
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

#include "scene/gui/control.h"

class Label;

class GraphNode : public Control
{
	friend class GraphEdit;

public:
	enum Mode
	{
		MODE_ELEMENT,
		MODE_NODE,
		MODE_FRAME,
	};

private:
	struct Slot
	{
		bool enable_left = false;
		int type_left = 0;
		Color color_left = Color(1, 1, 1, 1);
		Ref<Texture2D> custom_port_icon_left;

		bool enable_right = false;
		int type_right = 0;
		Color color_right = Color(1, 1, 1, 1);
		Ref<Texture2D> custom_port_icon_right;

		bool draw_stylebox = true;
	};

	struct PortCache
	{
		Vector2 pos;
		int slot_index = 0;
		int type = 0;
		Color color;
	};

	struct _MinSizeCache
	{
		int min_size = 0;
		int max_size = -1;
		bool will_stretch = false;
		int final_size = 0;
	};

	enum CustomAccessibilityAction
	{
		ACTION_CONNECT_INPUT,
		ACTION_CONNECT_OUTPUT,
		ACTION_FOLLOW_INPUT,
		ACTION_FOLLOW_OUTPUT,
	};

	Mode mode = MODE_NODE;

	// Shared graph element state.
	bool selected = false;
	bool resizable = false;
	bool resizing = false;
	bool draggable = true;
	bool selectable = true;

	Vector2 drag_from;
	Vector2 resizing_from;
	Vector2 resizing_from_size;
	Vector2 position_offset;

	bool scaling_menus = false;

	// Shared titlebar state (used in MODE_NODE and MODE_FRAME).
	Control* titlebar_hbox = nullptr;
	Label* title_label = nullptr;
	String title;

	// Slot/port state (active in MODE_NODE).
	Vector<PortCache> left_port_cache;
	Vector<PortCache> right_port_cache;
	HashMap<int, Slot> slot_table;
	Vector<int> slot_y_cache;

	Control::FocusMode slots_focus_mode = Control::FOCUS_ACCESSIBILITY;
	int slot_count = 0;
	int selected_slot = -1;
	bool port_pos_dirty = true;
	bool ignore_invalid_connection_type = false;

	// Frame autoshrink/tint state (active in MODE_FRAME).
	bool autoshrink_enabled = true;
	int autoshrink_margin = 40;
	int drag_margin = 16;
	bool tint_color_enabled = false;
	Color tint_color = Color(0.3, 0.3, 0.3, 0.75);

	struct ThemeCache
	{
		Ref<StyleBox> panel;
		Ref<StyleBox> panel_selected;
		Ref<StyleBox> panel_focus;
		Ref<StyleBox> titlebar;
		Ref<StyleBox> titlebar_selected;
		Ref<StyleBox> slot;
		Ref<StyleBox> slot_selected;

		int separation = 0;
		int port_h_offset = 0;

		Ref<Texture2D> port;
		Ref<Texture2D> resizer;
		Color resizer_color;
	} theme_cache;

#ifdef TOOLS_ENABLED
	void _edit_set_position(const Point2& p_position) override;
#endif

	void _port_pos_update();
	Size2 _get_minimum_size(bool p_use_desired_sizes) const;

protected:
	void _notification(int p_what);
	void draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color);
	void _resort();

	virtual String _get_accessibility_name() const override;

public:
	Mode get_mode() const { return mode; }
	void set_mode(Mode p_mode);
	bool is_frame() const { return mode == MODE_FRAME; }
	bool is_node() const { return mode == MODE_NODE; }
	bool is_element_only() const { return mode == MODE_ELEMENT; }

	// GraphElement API.
	void set_position_offset(const Vector2& p_offset);
	Vector2 get_position_offset() const { return position_offset; }

	void set_selected(bool p_selected);
	bool is_selected() const { return selected; }

	void set_drag(bool p_drag);
	Vector2 get_drag_from() const { return drag_from; }

	void set_resizable(bool p_enable);
	bool is_resizable() const { return resizable; }

	void set_draggable(bool p_draggable) { draggable = p_draggable; }
	bool is_draggable() const { return draggable; }

	void set_selectable(bool p_selectable);
	bool is_selectable() const { return selectable; }

	void set_scaling_menus(bool p_scaling_menus) { scaling_menus = p_scaling_menus; }
	bool is_scaling_menus() const { return scaling_menus; }

	bool is_resizing() const { return resizing; }

	// Shared titlebar API.
	void set_title(const String& p_title);
	String get_title() const { return title; }

	Control* get_titlebar_hbox() { return titlebar_hbox; }
	Size2 get_titlebar_size() const;

	// MODE_FRAME API.
	void set_autoshrink_enabled(bool p_enable);
	bool is_autoshrink_enabled() const { return autoshrink_enabled; }

	void set_autoshrink_margin(const int& p_margin);
	int get_autoshrink_margin() const { return autoshrink_margin; }

	void set_drag_margin(int p_margin) { drag_margin = p_margin; }
	int get_drag_margin() const { return drag_margin; }

	void set_tint_color_enabled(bool p_enable);
	bool is_tint_color_enabled() const { return tint_color_enabled; }

	void set_tint_color(const Color& p_tint_color);
	Color get_tint_color() const { return tint_color; }

	// MODE_NODE slot/port API.
	virtual String get_accessibility_container_name(const Node* p_node) const override;

	void set_slot(int p_slot_index, bool p_enable_left, int p_type_left, const Color& p_color_left,
		bool p_enable_right, int p_type_right, const Color& p_color_right,
		const Ref<Texture2D>& p_custom_left = Ref<Texture2D>(),
		const Ref<Texture2D>& p_custom_right = Ref<Texture2D>(), bool p_draw_stylebox = true);
	void clear_slot(int p_slot_index);
	void clear_all_slots();

	bool is_slot_enabled_left(int p_slot_index) const;
	void set_slot_enabled_left(int p_slot_index, bool p_enable);

	void set_slot_type_left(int p_slot_index, int p_type);
	int get_slot_type_left(int p_slot_index) const;

	void set_slot_color_left(int p_slot_index, const Color& p_color);
	Color get_slot_color_left(int p_slot_index) const;

	void set_slot_custom_icon_left(int p_slot_index, const Ref<Texture2D>& p_custom_icon);
	Ref<Texture2D> get_slot_custom_icon_left(int p_slot_index) const;

	bool is_slot_enabled_right(int p_slot_index) const;
	void set_slot_enabled_right(int p_slot_index, bool p_enable);

	void set_slot_type_right(int p_slot_index, int p_type);
	int get_slot_type_right(int p_slot_index) const;

	void set_slot_color_right(int p_slot_index, const Color& p_color);
	Color get_slot_color_right(int p_slot_index) const;

	void set_slot_custom_icon_right(int p_slot_index, const Ref<Texture2D>& p_custom_icon);
	Ref<Texture2D> get_slot_custom_icon_right(int p_slot_index) const;

	bool is_slot_draw_stylebox(int p_slot_index) const;
	void set_slot_draw_stylebox(int p_slot_index, bool p_enable);

	void set_ignore_invalid_connection_type(bool p_ignore);
	bool is_ignoring_valid_connection_type() const;

	int get_input_port_count();
	Vector2 get_input_port_position(int p_port_idx);
	int get_input_port_type(int p_port_idx);
	Color get_input_port_color(int p_port_idx);
	int get_input_port_slot(int p_port_idx);

	int get_output_port_count();
	Vector2 get_output_port_position(int p_port_idx);
	int get_output_port_type(int p_port_idx);
	Color get_output_port_color(int p_port_idx);
	int get_output_port_slot(int p_port_idx);

	void set_slots_focus_mode(Control::FocusMode p_focus_mode);
	Control::FocusMode get_slots_focus_mode() const;

	virtual bool has_point(const Point2& p_point) const override;
	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_desired_size() const override;

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	GraphNode(Mode p_mode = MODE_NODE);
};
