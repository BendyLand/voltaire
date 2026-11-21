/**************************************************************************/
/*  graph_edit.h                                                          */
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

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "scene/gui/control.h"
// --- Folded from scene/gui/graph_node.h ---
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

// --- End scene/gui/graph_node.h ---
#include "scene/resources/shader.h"

class Button;
class GraphEdit;
class Label;
class Line2D;
class Panel;
class ScrollBar;
class Shortcut;
class SpinBox;

class ViewPanner final : public RefCounted
{
public:
	enum ControlScheme
	{
		SCROLL_ZOOMS,
		SCROLL_PANS,
	};

	enum PanAxis
	{
		PAN_AXIS_BOTH,
		PAN_AXIS_HORIZONTAL,
		PAN_AXIS_VERTICAL,
	};

	enum DragType
	{
		DRAG_TYPE_NONE,
		DRAG_TYPE_PAN,
		DRAG_TYPE_ZOOM,
	};

	enum ZoomStyle
	{
		ZOOM_VERTICAL,
		ZOOM_HORIZONTAL,
	};

private:
	int scroll_speed = 32;
	float scroll_zoom_factor = 1.1;
	PanAxis pan_axis = PAN_AXIS_BOTH;

	bool pan_key_pressed = false;
	bool force_drag = false;

	DragType drag_type = DragType::DRAG_TYPE_NONE;

	ZoomStyle zoom_style = ZoomStyle::ZOOM_VERTICAL;

	Vector2 drag_zoom_position;
	float drag_zoom_sensitivity_factor = -0.01f;

	bool enable_rmb = false;
	bool simple_panning_enabled = false;

	Ref<Shortcut> pan_view_shortcut;

	ControlScheme control_scheme = SCROLL_ZOOMS;
	Node* warped_panning_owner = nullptr;

public:
	void set_control_scheme(ControlScheme p_scheme);
	void set_enable_rmb(bool p_enable);
	void set_pan_shortcut(Ref<Shortcut> p_shortcut);
	void set_simple_panning_enabled(bool p_enabled);
	void set_scroll_speed(int p_scroll_speed);
	void set_scroll_zoom_factor(float p_scroll_zoom_factor);
	void set_pan_axis(PanAxis p_pan_axis);
	void set_zoom_style(ZoomStyle p_zoom_style);

	void setup(ControlScheme p_scheme, Ref<Shortcut> p_shortcut, bool p_simple_panning);
	void setup_warped_panning(Node* p_owner, bool p_allowed);

	bool is_panning() const;
	void set_force_drag(bool p_force);

	bool gui_input(const InputEvent& p_ev, Rect2 p_canvas_rect = Rect2());
	void release_pan_key();

	ViewPanner() = default;
};

class GraphEditArranger final : public RefCounted
{
	enum SET_OPERATIONS
	{
		IS_EQUAL,
		IS_SUBSET,
		DIFFERENCE,
		UNION,
	};

	GraphEdit* graph_edit = nullptr;
	bool arranging_graph = false;

	int _set_operations(
		SET_OPERATIONS p_operation, HashSet<String>& r_u, const HashSet<String>& r_v);
	HashMap<int, Vector<String>> _layering(const HashSet<String>& r_selected_nodes,
		const HashMap<String, HashSet<String>>& r_upper_neighbours);
	void _crossing_minimisation(HashMap<int, Vector<String>>& r_layers,
		const HashMap<String, HashSet<String>>& r_upper_neighbours);

public:
	void arrange_nodes();

	GraphEditArranger(GraphEdit* p_graph_edit) : graph_edit(p_graph_edit) {}
};

class GraphEditFilter : public Control
{
	friend class GraphEdit;
	friend class GraphEditMinimap;

	GraphEdit* ge = nullptr;

	virtual bool has_point(const Point2& p_point) const override;

public:
	GraphEditFilter(GraphEdit* p_edit);
};

class GraphEditMinimap : public Control
{
	friend class GraphEdit;
	friend class GraphEditFilter;

	GraphEdit* ge = nullptr;

	Vector2 minimap_padding;
	Vector2 minimap_offset;
	Vector2 graph_proportions = Vector2(1, 1);
	Vector2 graph_padding = Vector2(0, 0);
	Vector2 camera_position = Vector2(100, 50);
	Vector2 camera_size = Vector2(200, 200);

	bool is_pressing = false;
	bool is_resizing = false;

	struct ThemeCache
	{
		Ref<StyleBox> panel;
		Ref<StyleBox> node_style;
		Ref<StyleBox> camera_style;

		Ref<Texture2D> resizer;
		Color resizer_color;
	} theme_cache;

	Vector2 _get_render_size();
	Vector2 _get_graph_offset();
	Vector2 _get_graph_size();

	Vector2 _convert_from_graph_position(const Vector2& p_position);
	Vector2 _convert_to_graph_position(const Vector2& p_position);

	void _adjust_graph_scroll(const Vector2& p_offset);

protected:

public:
	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void update_minimap();
	Rect2 get_camera_rect();

	GraphEditMinimap(GraphEdit* p_edit);
};

class GraphEdit : public Control
{
public:
	struct Connection : RefCounted
	{
		String from_node;
		String to_node;
		int from_port = 0;
		int to_port = 0;
		float activity = 0.0;
		bool keep_alive = true;

	private:
		struct Cache
		{
			bool dirty = true;
			Vector2 from_pos; // In graph space.
			Vector2 to_pos;	  // In graph space.
			Color from_color;
			Color to_color;
			Rect2 aabb;				// In local screen space.
			Line2D* line = nullptr; // In local screen space.
		} _cache;

		friend class GraphEdit;
	};

	// Should be in sync with ControlScheme in ViewPanner.
	enum PanningScheme
	{
		SCROLL_ZOOMS,
		SCROLL_PANS,
	};

	enum GridPattern
	{
		GRID_PATTERN_LINES,
		GRID_PATTERN_DOTS
	};

private:
	struct ConnectionType
	{
		union
		{
			uint64_t key = 0;

			struct
			{
				uint32_t type_a;
				uint32_t type_b;
			};
		};

		static uint32_t hash(const ConnectionType& p_conn) { return hash_one_uint64(p_conn.key); }

		bool operator==(const ConnectionType& p_type) const { return key == p_type.key; }

		ConnectionType(uint32_t a = 0, uint32_t b = 0)
		{
			type_a = a;
			type_b = b;
		}
	};

	Label* zoom_label = nullptr;
	Button* zoom_minus_button = nullptr;
	Button* zoom_reset_button = nullptr;
	Button* zoom_plus_button = nullptr;

	Button* toggle_snapping_button = nullptr;
	SpinBox* snapping_distance_spinbox = nullptr;
	Button* toggle_grid_button = nullptr;
	Button* minimap_button = nullptr;
	Button* arrange_button = nullptr;

	ScrollBar* h_scrollbar = nullptr;
	ScrollBar* v_scrollbar = nullptr;

	Ref<ViewPanner> panner;
	bool warped_panning = true;

	bool show_menu = true;
	bool show_zoom_label = false;
	bool show_grid_buttons = true;
	bool show_zoom_buttons = true;
	bool show_minimap_button = true;
	bool show_arrange_button = true;

	bool snapping_enabled = true;
	int snapping_distance = 20;
	bool show_grid = true;
	GridPattern grid_pattern = GRID_PATTERN_LINES;

	bool keyboard_connecting = false;
	bool connecting = false;
	String connecting_from_node;
	bool connecting_from_output = false;
	int connecting_type = 0;
	Color connecting_color;
	Vector2 connecting_to_point; // In local screen space.
	bool connecting_target_valid = false;
	String connecting_target_node;
	int connecting_from_port_index = 0;
	int connecting_target_port_index = 0;

	bool just_disconnected = false;
	bool connecting_valid = false;

	Vector2 click_pos;

	PanningScheme panning_scheme = SCROLL_ZOOMS;
	bool dragging = false;
	bool just_selected = false;
	bool moving_selection = false;
	Vector2 drag_accum;

	float zoom = 1.0;
	float zoom_step = 1.2;
	// Proper values set in constructor.
	float zoom_min = 0.0;
	float zoom_max = 0.0;

	Vector2 min_scroll_offset;
	Vector2 max_scroll_offset;
	Vector2 scroll_offset;

	bool box_selecting = false;
	bool box_selection_mode_additive = false;
	Point2 box_selecting_from;
	Point2 box_selecting_to;
	Rect2 box_selecting_rect;
	List<GraphNode*> prev_selected;

	bool setting_scroll_offset = false;
	bool right_disconnects = false;
	bool updating = false;
	bool awaiting_scroll_offset_update = false;

	Vector<Ref<Connection>> connections;
	HashMap<String, List<Ref<Connection>>> connection_map;
	Ref<Connection> hovered_connection;

	float lines_thickness = 4.0f;
	float lines_curvature = 0.5f;
	bool lines_antialiased = true;

	Panel* menu_panel = nullptr;
	Control* menu_hbox = nullptr;
	Control* connections_layer = nullptr;

	GraphEditFilter* top_connection_layer =
		nullptr; // Draws a dragged connection. Necessary since the connection line shader can't be
				 // applied to the whole top layer.
	Line2D* dragged_connection_line = nullptr;
	Control* top_layer = nullptr; // Used for drawing the box selection rect. Contains the minimap,
								  // menu panel and the scrollbars.

	GraphEditMinimap* minimap = nullptr;

	static Ref<Shader> default_connections_shader;
	Ref<Shader> connections_shader;

	Ref<GraphEditArranger> arranger;

	HashSet<ConnectionType, ConnectionType> valid_connection_types;
	HashSet<int> valid_left_disconnect_types;
	HashSet<int> valid_right_disconnect_types;

	struct ThemeCache
	{
		float base_scale = 1.0;

		Ref<StyleBox> panel;
		Ref<StyleBox> panel_focus;
		Color grid_major;
		Color grid_minor;

		Color activity_color;
		Color connection_hover_tint_color;
		int connection_hover_thickness;
		Color connection_valid_target_tint_color;
		Color connection_rim_color;

		Color selection_fill;
		Color selection_stroke;

		Ref<StyleBox> menu_panel;

		Ref<Texture2D> zoom_in;
		Ref<Texture2D> zoom_out;
		Ref<Texture2D> zoom_reset;

		Ref<Texture2D> snapping_toggle;
		Ref<Texture2D> grid_toggle;
		Ref<Texture2D> minimap_toggle;
		Ref<Texture2D> layout;

		float port_hotzone_inner_extent = 0.0;
		float port_hotzone_outer_extent = 0.0;
	} theme_cache;

	// This separates the children in two layers to ensure the order
	// of both background nodes (e.g frame nodes) and foreground nodes (connectable nodes).
	int background_nodes_separator_idx = 0;

	HashMap<String, HashSet<String>> frame_attached_nodes;
	HashMap<String, String> linked_parent_map;

	void _pan_callback(Vector2 p_scroll_vec, const InputEvent& p_event);
	void _zoom_callback(float p_zoom_factor, Vector2 p_origin, const InputEvent& p_event);

	void _zoom_minus();
	void _zoom_reset();
	void _zoom_plus();
	void _update_zoom_label();

	void _graph_element_selected(Node* p_node);
	void _graph_element_deselected(Node* p_node);
	void _graph_element_visibility_changed(GraphNode* p_graph_element);
	void _graph_element_resize_request(const Vector2& p_new_minsize, Node* p_node);
	void _graph_frame_autoshrink_changed(const Vector2& p_new_minsize, GraphNode* p_frame);
	void _graph_element_moved(Node* p_node);
	void _graph_node_slot_updated(int p_index, Node* p_node);
	void _graph_node_rect_changed(GraphNode* p_node);

	void _ensure_node_order_from_root(const String& p_node);
	void _ensure_node_order_from(Node* p_node);

	void _update_scrollbars();
	void _update_scroll_offset();
	void _scrollbar_moved(double);
	void _top_connection_layer_input(const InputEvent& p_ev);

	float _get_shader_line_width();
	void _draw_minimap_connection_line(const Vector2& p_from_graph_position,
		const Vector2& p_to_graph_position, const Color& p_from_color, const Color& p_to_color);
	void _invalidate_connection_line_cache();
	void _update_top_connection_layer();
	void _update_connections();

	void _top_layer_draw();
	void _minimap_draw();
	void _draw_grid();

	bool is_in_port_hotzone(
		const Vector2& p_pos, const Vector2& p_mouse_pos, const Vector2i& p_port_size, bool p_left);

	Rect2 _compute_shrinked_frame_rect(const GraphNode* p_frame);
	void _set_drag_frame_attached_nodes(GraphNode* p_frame, bool p_drag);
	void _set_position_of_frame_attached_nodes(GraphNode* p_frame, const Vector2& p_pos);

	friend class GraphEditFilter;
	bool _filter_input(const Point2& p_point);
	void _snapping_toggled();
	void _snapping_distance_changed(double);
	void _show_grid_toggled();

	friend class GraphEditMinimap;
	void _minimap_toggled();

	bool _check_clickable_control(
		Control* p_control, const Vector2& r_mouse_pos, const Vector2& p_offset);

#ifndef DISABLE_DEPRECATED
	bool _is_arrange_nodes_button_hidden_bind_compat_81582() const;
	PackedVector2Array _get_connection_line_bind_compat_86158(
		const Vector2& p_from, const Vector2& p_to);
	Error _connect_node_bind_compat_97449(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);
#endif

protected:
	virtual void _update_theme_item_cache() override;

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

	void _notification(int p_what);
#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
#endif

	virtual bool is_in_input_hotzone(GraphNode* p_graph_node, int p_port_idx,
		const Vector2& p_mouse_pos, const Vector2i& p_port_size);
	virtual bool is_in_output_hotzone(GraphNode* p_graph_node, int p_port_idx,
		const Vector2& p_mouse_pos, const Vector2i& p_port_size);

public:
	static void init_shaders();
	static void finish_shaders();

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void key_input(const InputEvent& p_ev);

	// This method has to be public (for undo redo).
	// TODO: Find a better way to do this.
	void _update_graph_frame(GraphNode* p_frame);

	// Connection related methods.
	Error connect_node(const String& p_from, int p_from_port, const String& p_to,
		int p_to_port, bool keep_alive = false);
	bool is_node_connected(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);
	int get_connection_count(const String& p_node, int p_port);
	GraphNode* get_input_connection_target(const String& p_node, int p_port);
	GraphNode* get_output_connection_target(const String& p_node, int p_port);
	String get_connections_description(const String& p_node, int p_port);
	void disconnect_node(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);

	void force_connection_drag_end();
	const Vector<Ref<Connection>>& get_connections() const;
	void clear_connections();
	virtual PackedVector2Array get_connection_line(
		const Vector2& p_from, const Vector2& p_to) const;
	Ref<Connection> get_closest_connection_at_point(
		const Vector2& p_point, float p_max_distance = 4.0) const;
	List<Ref<Connection>> get_connections_intersecting_with_rect(const Rect2& p_rect) const;

	bool is_keyboard_connecting() const { return keyboard_connecting; }

	void start_keyboard_connecting(GraphNode* p_node, int p_in_port, int p_out_port);
	void end_keyboard_connecting(GraphNode* p_node, int p_in_port, int p_out_port);

	virtual bool is_node_hover_valid(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);

	void set_connection_activity(const String& p_from, int p_from_port, const String& p_to,
		int p_to_port, float p_activity);
	void reset_all_connection_activity();

	void add_valid_connection_type(int p_type, int p_with_type);
	void remove_valid_connection_type(int p_type, int p_with_type);
	bool is_valid_connection_type(int p_type, int p_with_type) const;

	// Frame related methods (GraphNode in MODE_FRAME).
	void attach_graph_element_to_frame(
		const String& p_graph_element, const String& p_parent_frame);
	void detach_graph_element_from_frame(const String& p_graph_element);
	GraphNode* get_element_frame(const String& p_attached_graph_element);
	Vector<String> get_attached_nodes_of_frame(const String& p_graph_frame) const;

	void set_panning_scheme(PanningScheme p_scheme);
	PanningScheme get_panning_scheme() const;

	void set_zoom(float p_zoom);
	void set_zoom_custom(float p_zoom, const Vector2& p_center);
	float get_zoom() const;

	void set_zoom_min(float p_zoom_min);
	float get_zoom_min() const;

	void set_zoom_max(float p_zoom_max);
	float get_zoom_max() const;

	void set_zoom_step(float p_zoom_step);
	float get_zoom_step() const;

	void set_minimap_size(Vector2 p_size);
	Vector2 get_minimap_size() const;
	void set_minimap_opacity(float p_opacity);
	float get_minimap_opacity() const;

	void set_minimap_enabled(bool p_enable);
	bool is_minimap_enabled() const;

	bool is_showing_menu() const;
	bool is_showing_zoom_label() const;
	bool is_showing_grid_buttons() const;
	bool is_showing_zoom_buttons() const;
	bool is_showing_minimap_button() const;
	bool is_showing_arrange_button() const;

	Control* get_top_layer() const { return top_layer; }

	GraphEditMinimap* get_minimap() const { return minimap; }

	void override_connections_shader(const Ref<Shader>& p_shader);

	void set_right_disconnects(bool p_enable);
	bool is_right_disconnects_enabled() const;

	void add_valid_right_disconnect_type(int p_type);
	void remove_valid_right_disconnect_type(int p_type);

	void add_valid_left_disconnect_type(int p_type);
	void remove_valid_left_disconnect_type(int p_type);

	void set_scroll_offset(const Vector2& p_ofs);
	Vector2 get_scroll_offset() const;

	void set_selected(Node* p_child);

	void set_snapping_enabled(bool p_enable);
	bool is_snapping_enabled() const;

	void set_snapping_distance(int p_snapping_distance);
	int get_snapping_distance() const;

	void set_show_grid(bool p_enable);
	bool is_showing_grid() const;

	void set_grid_pattern(GridPattern p_pattern);
	GridPattern get_grid_pattern() const;

	void set_connection_lines_curvature(float p_curvature);
	float get_connection_lines_curvature() const;

	void set_connection_lines_thickness(float p_thickness);
	float get_connection_lines_thickness() const;

	void set_connection_lines_antialiased(bool p_antialiased);
	bool is_connection_lines_antialiased() const;

	Control* get_menu_hbox();
	Ref<ViewPanner> get_panner();
	void set_warped_panning(bool p_warped);
	void update_warped_panning();

	void arrange_nodes();

	GraphEdit();
};


