/**************************************************************************/
/*  node.h                                                                */
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

#include "core/os/os.h"
// --- Folded from core/templates/iterable.h ---
#include <utility>

template <typename I>
class Iterable {
	I _begin;
	I _end;

public:
	I begin() { return _begin; }
	I end() { return _end; }

	Iterable(I &&p_begin, I &&p_end) :
			_begin(std::move(p_begin)), _end(std::move(p_end)) {}
	Iterable(const I &p_begin, const I &p_end) :
			_begin(p_begin), _end(p_end) {}
};

// --- End core/templates/iterable.h ---
// --- Folded from core/templates/mem_unique_ptr.h ---
#ifndef MEM_UNIQUE_PTR_H
#define MEM_UNIQUE_PTR_H

#include <memory>
#include <utility>
#include "core/os/memory.h"

template <typename T> struct MemDeleter
{
	void operator()(T* p) const
	{
		if (p) {
			memdelete(p);
		}
	}
};

template <typename T> using mem_unique_ptr = std::unique_ptr<T, MemDeleter<T>>;

template <typename T, typename... Args> mem_unique_ptr<T> mem_make_unique(Args&&... p_args)
{
	return mem_unique_ptr<T>(memnew(T(std::forward<Args>(p_args)...)));
}

#endif // MEM_UNIQUE_PTR_H


// --- End core/templates/mem_unique_ptr.h ---
class PackedScene;
// --- Inlined/Folded from scene/scene_string_names.h ---
#include "core/os/memory.h"
#include "core/string/ustring.h"

class SceneStrings final
{
public:
	struct Data
	{
		const String resized = "resized";
		const String draw = "draw";
		const String hidden = "hidden";
		const String visibility_changed = "visibility_changed";

		const String input_event = "input_event";
		const String gui_input = "gui_input";
		const String window_input = "window_input";
		const String nonclient_window_input = "nonclient_window_input";

		const String tree_entered = "tree_entered";
		const String tree_exiting = "tree_exiting";
		const String tree_exited = "tree_exited";
		const String ready = "ready";
		const String _ready = "_ready";

		const String item_rect_changed = "item_rect_changed";
		const String size_flags_changed = "size_flags_changed";
		const String maximum_size_changed = "maximum_size_changed";
		const String minimum_size_changed = "minimum_size_changed";
		const String sleeping_state_changed = "sleeping_state_changed";
		const String node_configuration_warning_changed = "node_configuration_warning_changed";
		const String update = "update";
		const String updated = "updated";

		const String line_separation = "line_separation";
		const String paragraph_separation = "paragraph_separation";
		const String font = "font";
		const String font_size = "font_size";
		const String font_color = "font_color";

		const String mouse_entered = "mouse_entered";
		const String mouse_exited = "mouse_exited";
		const String mouse_shape_entered = "mouse_shape_entered";
		const String mouse_shape_exited = "mouse_shape_exited";
		const String focus_entered = "focus_entered";
		const String focus_exited = "focus_exited";

		const String pre_sort_children = "pre_sort_children";
		const String sort_children = "sort_children";

		const String finished = "finished";
		const String animation_finished = "animation_finished";
		const String animation_changed = "animation_changed";
		const String animation_started = "animation_started";
		const String RESET = "RESET";

		const String pose_updated = "pose_updated";
		const String skeleton_updated = "skeleton_updated";
		const String bone_enabled_changed = "bone_enabled_changed";
		const String show_rest_only_changed = "show_rest_only_changed";

		const String body_shape_entered = "body_shape_entered";
		const String body_entered = "body_entered";
		const String body_shape_exited = "body_shape_exited";
		const String body_exited = "body_exited";

		const String area_shape_entered = "area_shape_entered";
		const String area_shape_exited = "area_shape_exited";

		const String screen_entered = "screen_entered";
		const String screen_exited = "screen_exited";

		const String _spatial_editor_group = "_spatial_editor_group";
		const String _request_gizmo = "_request_gizmo";

		const String offset = "offset";
		const String rotation_mode = "rotation_mode";
		const String rotate = "rotate";
		const String h_offset = "h_offset";
		const String v_offset = "v_offset";

		const String area_entered = "area_entered";
		const String area_exited = "area_exited";

		const String frame_changed = "frame_changed";
		const String texture_changed = "texture_changed";

		const String autoplay = "autoplay";
		const String blend_times = "blend_times";
		const String speed = "speed";

		const String default_ = "default"; // default would conflict with C++ keyword.
		const String output = "output";

		const String Master = "Master"; // Audio bus name.

		const String theme_changed = "theme_changed";
		const String shader = "shader";
		const String shader_overrides_group = "_shader_overrides_group_";
		const String shader_overrides_group_active = "_shader_overrides_group_active_";

		const String _custom_type_script = "_custom_type_script";

		const String pressed = "pressed";
		const String id_pressed = "id_pressed";
		const String toggled = "toggled";
		const String hover = "hover";

		const String panel = "panel";
		const String item_selected = "item_selected";
		const String confirmed = "confirmed";

		const String text_changed = "text_changed";
		const String text_submitted = "text_submitted";
		const String value_changed = "value_changed";

		const String Start = "Start";
		const String End = "End";
		const String state_started = "state_started";
		const String state_finished = "state_finished";

		const String FlatButton = "FlatButton";
	};

	static inline Data* data = nullptr;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (!data) {
			initialize();
		}
		return data;
	}

	SceneStrings() = delete;
	~SceneStrings() = delete;
};

#define SceneString(m_name) SceneStrings::_get_data()->m_name

class MultiplayerAPI;
class NodePath;
class Resource;
class SceneState;
class SceneTree;
class Tween;
class Viewport;
class Window;
struct SceneTreeGroup;

SAFE_FLAG_TYPE_PUN_GUARANTEES
SAFE_NUMERIC_TYPE_PUN_GUARANTEES(uint32_t)

class Node
{
protected:
	// During group processing, these are thread-safe.
	// Outside group processing, these avoid the cost of sync by working as plain primitive types.
	union MTFlag
	{
		SafeFlag mt;
		bool st;

		MTFlag() : mt{} {}
	};

	template <typename T> union MTNumeric
	{
		SafeNumeric<T> mt;
		T st;

		MTNumeric() : mt{} {}
	};

public:
	void notification(int p_what, bool p_reversed = false)
	{
		_notification(p_what);
	}
	// N.B. Any enum stored as a bitfield should be specified as UNSIGNED to work around
	// some compilers trying to store it as signed, and requiring 1 more bit than necessary.
	enum ProcessMode : unsigned int
	{
		PROCESS_MODE_INHERIT,	  // same as parent node
		PROCESS_MODE_PAUSABLE,	  // process only if not paused
		PROCESS_MODE_WHEN_PAUSED, // process only if paused
		PROCESS_MODE_ALWAYS,	  // process always
		PROCESS_MODE_DISABLED,	  // never process
	};

	enum ProcessThreadGroup
	{
		PROCESS_THREAD_GROUP_INHERIT,
		PROCESS_THREAD_GROUP_MAIN_THREAD,
		PROCESS_THREAD_GROUP_SUB_THREAD,
	};

	enum ProcessThreadMessages
	{
		FLAG_PROCESS_THREAD_MESSAGES = 1,
		FLAG_PROCESS_THREAD_MESSAGES_PHYSICS = 2,
		FLAG_PROCESS_THREAD_MESSAGES_ALL = 3,
	};

	enum PhysicsInterpolationMode : unsigned int
	{
		PHYSICS_INTERPOLATION_MODE_INHERIT,
		PHYSICS_INTERPOLATION_MODE_ON,
		PHYSICS_INTERPOLATION_MODE_OFF,
	};

	enum DuplicateFlags
	{
		DUPLICATE_SIGNALS = 1,
		DUPLICATE_GROUPS = 2,
		DUPLICATE_SCRIPTS = 4,
		DUPLICATE_USE_INSTANTIATION = 8,
		DUPLICATE_INTERNAL_STATE = 16,
		DUPLICATE_DEFAULT =
			DUPLICATE_SIGNALS | DUPLICATE_GROUPS | DUPLICATE_SCRIPTS | DUPLICATE_USE_INSTANTIATION,
#ifdef TOOLS_ENABLED
		DUPLICATE_FROM_EDITOR = 32,
#endif
	};

	enum NameCasing
	{
		NAME_CASING_PASCAL_CASE,
		NAME_CASING_CAMEL_CASE,
		NAME_CASING_SNAKE_CASE,
		NAME_CASING_KEBAB_CASE,
	};

	enum InternalMode
	{
		INTERNAL_MODE_DISABLED,
		INTERNAL_MODE_FRONT,
		INTERNAL_MODE_BACK,
	};

	enum AutoTranslateMode : unsigned int
	{
		AUTO_TRANSLATE_MODE_INHERIT,
		AUTO_TRANSLATE_MODE_ALWAYS,
		AUTO_TRANSLATE_MODE_DISABLED,
	};

	struct Comparator
	{
		bool operator()(const Node* p_a, const Node* p_b) const
		{
			return p_b->is_greater_than(p_a);
		}
	};

#ifdef DEBUG_ENABLED
	static SafeNumeric<uint64_t> total_node_count;
#endif
	enum
	{
		UNIQUE_SCENE_ID_UNASSIGNED = 0
	};

	void _update_process(bool p_enable, bool p_for_children);

	struct ChildrenIterator
	{
		_FORCE_INLINE_ Node*& operator*() const { return *_ptr; }

		_FORCE_INLINE_ Node** operator->() const { return _ptr; }

		_FORCE_INLINE_ ChildrenIterator& operator++()
		{
			_ptr++;
			return *this;
		}

		_FORCE_INLINE_ ChildrenIterator& operator--()
		{
			_ptr--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ChildrenIterator& b) const { return _ptr == b._ptr; }

		_FORCE_INLINE_ bool operator!=(const ChildrenIterator& b) const { return _ptr != b._ptr; }

		ChildrenIterator(Node** p_ptr) { _ptr = p_ptr; }

		ChildrenIterator() {}

		ChildrenIterator(const ChildrenIterator& p_it) { _ptr = p_it._ptr; }

	private:
		Node** _ptr = nullptr;
	};

private:
	struct GroupData
	{
		bool persistent = false;
		SceneTreeGroup* group = nullptr;
	};

	struct ComparatorByIndex
	{
		bool operator()(const Node* p_left, const Node* p_right) const
		{
			static const uint32_t order[3] = {1, 0, 2};
			uint32_t order_left = order[p_left->data.internal_mode];
			uint32_t order_right = order[p_right->data.internal_mode];
			if (order_left == order_right) {
				return p_left->data.index < p_right->data.index;
			}
			return order_left < order_right;
		}
	};

	struct ComparatorWithPriority
	{
		bool operator()(const Node* p_a, const Node* p_b) const
		{
			return p_b->data.process_priority == p_a->data.process_priority
					   ? p_b->is_greater_than(p_a)
					   : p_b->data.process_priority > p_a->data.process_priority;
		}
	};

	struct ComparatorWithPhysicsPriority
	{
		bool operator()(const Node* p_a, const Node* p_b) const
		{
			return p_b->data.physics_process_priority == p_a->data.physics_process_priority
					   ? p_b->is_greater_than(p_a)
					   : p_b->data.physics_process_priority > p_a->data.physics_process_priority;
		}
	};

	// This Data struct is to avoid namespace pollution in derived classes.
	struct Data
	{
		String scene_file_path;
		Ref<SceneState> instance_state;
		Ref<SceneState> inherited_state;
		bool inside_tree = false;

		Node* parent = nullptr;
		Node* owner = nullptr;
		HashMap<String, Node*> children;
		mutable bool children_cache_dirty = false;
		mutable LocalVector<Node*> children_cache;
		HashMap<String, Node*> owned_unique_nodes;
		bool unique_name_in_owner = false;
		InternalMode internal_mode = INTERNAL_MODE_DISABLED;
		mutable int internal_children_front_count_cache = 0;
		mutable int internal_children_back_count_cache = 0;
		mutable int external_children_count_cache = 0;
		mutable int index = -1; // relative to front, normal or back.
		int32_t depth = -1;
		int blocked = 0; // Safeguard that throws an error when attempting to modify the tree in a
						 // harmful way while being traversed.
		String name;

		String editor_description;

		Viewport* viewport = nullptr;

		mutable RID accessibility_element;

		HashMap<String, GroupData> grouped;
		List<Node*>::Element* OW = nullptr; // Owned element.
		List<Node*> owned;

		Node* process_owner = nullptr;
		ProcessThreadGroup process_thread_group = PROCESS_THREAD_GROUP_INHERIT;
		Node* process_thread_group_owner = nullptr;
		int process_thread_group_order = 0;
		uint32_t process_thread_messages = 0;
		void* process_group = nullptr; // to avoid cyclic dependency

		int multiplayer_authority = 1; // Server by default.

		// Variables used to properly sort the node when processing, ignored otherwise.
		int process_priority = 0;
		int physics_process_priority = 0;

		// Keep bitpacked values together to get better packing.
		ProcessMode process_mode : 3;
		PhysicsInterpolationMode physics_interpolation_mode : 2;
		AutoTranslateMode auto_translate_mode : 2;

		bool physics_process : 1;
		bool process : 1;

		bool physics_process_internal : 1;
		bool process_internal : 1;

		bool input : 1;
		bool shortcut_input : 1;
		bool unhandled_input : 1;
		bool unhandled_key_input : 1;

		// Physics interpolation can be turned on and off on a per node basis.
		// This only takes effect when the SceneTree (or project setting) physics interpolation
		// is switched on.
		bool physics_interpolated : 1;

		// We can auto-reset physics interpolation when e.g. adding a node for the first time.
		bool physics_interpolation_reset_requested : 1;

		// Most nodes need not be interpolated in the scene tree, physics interpolation
		// is normally only needed in the Renderer. However if we need to read the
		// interpolated transform of a node in the SceneTree, it is necessary to duplicate
		// the interpolation logic client side, in order to prevent stalling the Renderer
		// by reading back.
		bool physics_interpolated_client_side : 1;

		// For certain nodes (e.g. CPU particles in global mode)
		// it can be useful to not send the instance transform to the
		// Renderer, and specify the mesh in world space.
		bool use_identity_transform : 1;

		bool use_placeholder : 1;

		bool display_folded : 1;
		bool editable_instance : 1;

		bool ready_notified : 1;
		bool ready_first : 1;

		mutable bool is_auto_translating : 1;
		mutable bool is_auto_translate_dirty : 1;

		mutable bool is_translation_domain_inherited : 1;
		mutable bool is_translation_domain_dirty : 1;

		int32_t unique_scene_id = UNIQUE_SCENE_ID_UNASSIGNED;

		mutable NodePath* path_cache = nullptr;
	};

	Data data;

	String _get_tree_string_pretty(const String& p_prefix, bool p_last);
	String _get_tree_string(const Node* p_node);

	Node* _get_child_by_name(const String& p_name) const;

	void _replace_connections_target(Node* p_new_target);

	void _validate_child_name(Node* p_child, bool p_force_human_readable = false);
	void _generate_serial_child_name(const Node* p_child, String& name) const;

	void _propagate_enter_tree();
	void _propagate_ready();
	void _propagate_exit_tree();
	void _propagate_after_exit_tree();
	void _propagate_physics_interpolated(bool p_interpolated);
	void _propagate_physics_interpolation_reset_requested(bool p_requested);
	void _propagate_process_owner(
		Node* p_owner, int p_pause_notification, int p_enabled_notification);
	void _propagate_groups_dirty();
	void _propagate_translation_domain_dirty();

	void _duplicate_scripts(const Node* p_original, Node* p_copy) const;
	void _duplicate_properties(
		const Node* p_root, const Node* p_original, Node* p_copy, int p_flags) const;
	void _duplicate_signals(const Node* p_original, Node* p_copy) const;
	Node* _duplicate(int p_flags, HashMap<const Node*, Node*>* r_duplimap = nullptr) const;

	friend class SceneTree;

	void _set_tree(bool p_in_tree);
	void _propagate_pause_notification(bool p_enable);
	void _propagate_suspend_notification(bool p_enable);

	_FORCE_INLINE_ bool _can_process(bool p_paused) const;
	_FORCE_INLINE_ bool _is_enabled() const;

	void _release_unique_name_in_owner();
	void _acquire_unique_name_in_owner();

	void _clean_up_owner();

	_FORCE_INLINE_ void _update_children_cache() const
	{
		if (unlikely(data.children_cache_dirty)) {
			_update_children_cache_impl();
		}
	}

	void _update_children_cache_impl() const;

	// Process group management
	void _add_process_group();
	void _remove_process_group();
	void _add_to_process_thread_group();
	void _remove_from_process_thread_group();
	void _remove_tree_from_process_thread_group();
	void _add_tree_to_process_thread_group(Node* p_owner);

	static thread_local Node* current_process_thread_group;

	// Editor only signal to keep the SceneTreeEditor in sync.
#ifdef TOOLS_ENABLED
	void _emit_editor_state_changed();
#else
	void _emit_editor_state_changed() {}
#endif

protected:
	void _block() { data.blocked++; }

	void _unblock() { data.blocked--; }

	void _notification(int p_notification);

	virtual void _physics_interpolated_changed();

	virtual void add_child_notify(Node* p_child);
	virtual void remove_child_notify(Node* p_child);
	virtual void move_child_notify(Node* p_child);
	virtual void owner_changed_notify();

	void _propagate_replace_owner(Node* p_owner, Node* p_by_owner);

	static void _bind_methods();
	static String _get_name_num_separator();

	friend class SceneState;

	void _add_child_nocheck(Node* p_child, const String& p_name,
		InternalMode p_internal_mode = INTERNAL_MODE_DISABLED);
	void _set_owner_nocheck(Node* p_owner);
	void _set_name_nocheck(const String& p_name);

	void _set_physics_interpolated_client_side(bool p_enable)
	{
		data.physics_interpolated_client_side = p_enable;
	}

	bool _is_physics_interpolated_client_side() const
	{
		return data.physics_interpolated_client_side;
	}

	void _set_physics_interpolation_reset_requested(bool p_enable)
	{
		data.physics_interpolation_reset_requested = p_enable;
	}

	bool _is_physics_interpolation_reset_requested() const
	{
		return data.physics_interpolation_reset_requested;
	}

	void _set_use_identity_transform(bool p_enable) { data.use_identity_transform = p_enable; }

	bool _is_using_identity_transform() const { return data.use_identity_transform; }

	int32_t _get_scene_tree_depth() const { return data.depth; }

	// call from SceneTree
	void _call_input(const InputEvent& p_event);
	void _call_shortcut_input(const InputEvent& p_event);
	void _call_unhandled_input(const InputEvent& p_event);
	void _call_unhandled_key_input(const InputEvent& p_event);

	virtual String _to_string();

	// Localization

	virtual String _get_translation_context_with_override(const String& p_context) const
	{
		return p_context;
	}

protected:
	virtual bool _uses_signal_mutex() const { return false; } // Node uses thread guards instead.

	virtual void input(const InputEvent& p_event);
	virtual void unhandled_input(const InputEvent& p_event);
	virtual void unhandled_key_input(const InputEvent& p_key_event);

public:
	enum
	{
		// You can make your own, but don't use the same numbers as other notifications in other
		// nodes.
		NOTIFICATION_ENTER_TREE = 10,
		NOTIFICATION_EXIT_TREE = 11,
		NOTIFICATION_MOVED_IN_PARENT = 12,
		NOTIFICATION_READY = 13,
		NOTIFICATION_PAUSED = 14,
		NOTIFICATION_UNPAUSED = 15,
		NOTIFICATION_PHYSICS_PROCESS = 16,
		NOTIFICATION_PROCESS = 17,
		NOTIFICATION_PARENTED = 18,
		NOTIFICATION_UNPARENTED = 19,
		NOTIFICATION_SCENE_INSTANTIATED = 20,
		NOTIFICATION_DRAG_BEGIN = 21,
		NOTIFICATION_DRAG_END = 22,
		NOTIFICATION_PATH_RENAMED = 23,
		NOTIFICATION_CHILD_ORDER_CHANGED = 24,
		NOTIFICATION_INTERNAL_PROCESS = 25,
		NOTIFICATION_INTERNAL_PHYSICS_PROCESS = 26,
		NOTIFICATION_POST_ENTER_TREE = 27,
		NOTIFICATION_DISABLED = 28,
		NOTIFICATION_ENABLED = 29,
		NOTIFICATION_RESET_PHYSICS_INTERPOLATION = 2001, // A GodotSpace Odyssey.
														 // Keep these linked to Node.

		NOTIFICATION_ACCESSIBILITY_UPDATE = 3000,
		NOTIFICATION_ACCESSIBILITY_INVALIDATE = 3001,

		NOTIFICATION_WM_MOUSE_ENTER = 1002,
		NOTIFICATION_WM_MOUSE_EXIT = 1003,
		NOTIFICATION_WM_WINDOW_FOCUS_IN = 1004,
		NOTIFICATION_WM_WINDOW_FOCUS_OUT = 1005,
		NOTIFICATION_WM_CLOSE_REQUEST = 1006,
		NOTIFICATION_WM_GO_BACK_REQUEST = 1007,
		NOTIFICATION_WM_SIZE_CHANGED = 1008,
		NOTIFICATION_WM_DPI_CHANGE = 1009,
		NOTIFICATION_VP_MOUSE_ENTER = 1010,
		NOTIFICATION_VP_MOUSE_EXIT = 1011,
		NOTIFICATION_WM_POSITION_CHANGED = 1012,
		NOTIFICATION_WM_OUTPUT_MAX_LINEAR_VALUE_CHANGED = 1013,

		// Keep these in sync with MainLoop.
		NOTIFICATION_OS_MEMORY_WARNING = 2009,
		NOTIFICATION_TRANSLATION_CHANGED = 2010,
		NOTIFICATION_WM_ABOUT = 2011,
		NOTIFICATION_CRASH = 2012,
		NOTIFICATION_OS_IME_UPDATE = 2013,
		NOTIFICATION_APPLICATION_RESUMED = 2014,
		NOTIFICATION_APPLICATION_PAUSED = 2015,
		NOTIFICATION_APPLICATION_FOCUS_IN = 2016,
		NOTIFICATION_APPLICATION_FOCUS_OUT = 2017,
		NOTIFICATION_TEXT_SERVER_CHANGED = 2018,
		NOTIFICATION_APPLICATION_PIP_MODE_ENTERED = 2019,
		NOTIFICATION_APPLICATION_PIP_MODE_EXITED = 2020,

		// Editor specific node notifications
		NOTIFICATION_EDITOR_PRE_SAVE = 9001,
		NOTIFICATION_EDITOR_POST_SAVE = 9002,
		NOTIFICATION_SUSPENDED = 9003,
		NOTIFICATION_UNSUSPENDED = 9004
	};

	/* NODE/TREE */

	String get_name() const;
	String get_description(bool p_show_not_in_tree = false) const;
	void set_name(const String& p_name);

	InternalMode get_internal_mode() const;

	// FIX: Check the hardcoded default args; remove if possible
	void add_child(Node* rp_child, bool p_force_readable_name = false,
		InternalMode p_internal = InternalMode::INTERNAL_MODE_DISABLED);
	// FIX: Check the hardcoded default arg; remove if possible
	void add_sibling(Node* rp_sibling, bool p_force_readable_name = false);
	void remove_child(Node* rp_child);

	/// Optimal way to iterate the children of this node.
	/// The caller is responsible to ensure:
	/// - The thread has the rights to access the node (is_accessible_from_caller_thread() == true).
	/// - No children are inserted, removed, or have their index changed during iteration.
	template <bool p_include_internal = true> Iterable<ChildrenIterator> iterate_children() const;

	int get_child_count(bool p_include_internal = true) const;
	Node* get_child(int p_index, bool p_include_internal = true) const;
	Vector<Node*> get_children(bool p_include_internal = true) const;
	bool has_node(const NodePath& p_path) const;
	Node* get_node(const NodePath& p_path) const;
	Node* get_node_or_null(const NodePath& p_path) const;
	Node* find_child(const String& p_pattern, bool p_recursive = true, bool p_owned = true) const;
	Vector<Node*> find_children(const String& p_pattern, const String& p_type,
		bool p_recursive = true, bool p_owned = true) const;
	bool has_node_and_resource(const NodePath& p_path) const;
	Node* get_node_and_resource(const NodePath& p_path, Ref<Resource>& r_res,
		Vector<String>& r_leftover_subpath, bool p_last_is_property = true) const;

	void reparent(Node* rp_parent, bool p_keep_global_transform = true);
	Node* get_parent() const;
	Node* find_parent(const String& p_pattern) const;

	void set_unique_scene_id(int32_t p_unique_id);
	int32_t get_unique_scene_id() const;

	Window* get_window() const;
	Window* get_non_popup_window() const;
	Window* get_last_exclusive_window() const;

	_FORCE_INLINE_ bool is_inside_tree() const { return data.inside_tree; }

	bool is_internal() const { return data.internal_mode != INTERNAL_MODE_DISABLED; }

	bool is_ancestor_of(const Node* rp_node) const;
	bool is_greater_than(const Node* rp_node) const;

	NodePath get_path() const;
	NodePath get_path_to(const Node* rp_node, bool p_use_unique_path = false) const;
	Node* find_common_parent_with(const Node* p_node) const;

	void add_to_group(const String& p_identifier, bool p_persistent = false);
	void remove_from_group(const String& p_identifier);
	bool is_in_group(const String& p_identifier) const;

	struct GroupInfo
	{
		String name;
		bool persistent = false;
	};

	void get_groups(List<GroupInfo>* p_groups) const;
	int get_persistent_group_count() const;

	void move_child(Node* rp_child, int p_index);
	void _move_child(Node* p_child, int p_index, bool p_ignore_end = false);

	void set_owner(Node* p_owner);
	Node* get_owner() const;
	void get_owned_by(Node* p_by, List<Node*>* p_owned);

	void set_unique_name_in_owner(bool p_enabled);
	bool is_unique_name_in_owner() const;

	_FORCE_INLINE_ int get_index(bool p_include_internal = true) const
	{
		// p_include_internal = false doesn't make sense if the node is internal.
		ERR_FAIL_COND_V_MSG(!p_include_internal && data.internal_mode != INTERNAL_MODE_DISABLED, -1,
			"Node is internal. Can't get index with 'include_internal' being false.");
		if (!data.parent) {
			return data.index;
		}
		data.parent->_update_children_cache();

		if (!p_include_internal) {
			return data.index;
		}
		else {
			switch (data.internal_mode) {
			case INTERNAL_MODE_DISABLED: {
				return data.parent->data.internal_children_front_count_cache + data.index;
			} break;
			case INTERNAL_MODE_FRONT: {
				return data.index;
			} break;
			case INTERNAL_MODE_BACK: {
				return data.parent->data.internal_children_front_count_cache +
					   data.parent->data.external_children_count_cache + data.index;
			} break;
			}
			return -1;
		}
	}

	Tween* create_tween();

	void print_tree();
	void print_tree_pretty();
	String get_tree_string();
	String get_tree_string_pretty();

	void set_scene_file_path(const String& p_scene_file_path);
	String get_scene_file_path() const;

	void set_editor_description(const String& p_editor_description);
	String get_editor_description() const;

	void set_editable_instance(Node* rp_node, bool p_editable);
	bool is_editable_instance(const Node* p_node) const;
	Node* get_deepest_editable_node(Node* p_start_node) const;

#ifdef TOOLS_ENABLED
	void set_property_pinned(const String& p_property, bool p_pinned);
	bool is_property_pinned(const String& p_property) const;
	virtual String get_property_store_alias(const String& p_property) const;
	bool is_part_of_edited_scene() const;
#else
	bool is_part_of_edited_scene() const { return false; }
#endif
	void get_storable_properties(HashSet<String>& r_storable_properties) const;

	/* NOTIFICATIONS */

	void propagate_notification(int p_notification);

	/* PROCESSING */

	void set_physics_process(bool p_process);
	double get_physics_process_delta_time() const;
	bool is_physics_processing() const;

	void set_process(bool p_process);
	double get_process_delta_time() const;
	bool is_processing() const;

	void set_physics_process_internal(bool p_process_internal);
	bool is_physics_processing_internal() const;

	void set_process_internal(bool p_process_internal);
	bool is_processing_internal() const;

	void set_process_priority(int p_priority);
	int get_process_priority() const;

	void set_process_thread_group_order(int p_order);
	int get_process_thread_group_order() const;

	void set_physics_process_priority(int p_priority);
	int get_physics_process_priority() const;

	void set_process_input(bool p_enable);
	bool is_processing_input() const;

	void set_process_shortcut_input(bool p_enable);
	bool is_processing_shortcut_input() const;

	void set_process_unhandled_input(bool p_enable);
	bool is_processing_unhandled_input() const;

	void set_process_unhandled_key_input(bool p_enable);
	bool is_processing_unhandled_key_input() const;

	_FORCE_INLINE_ bool _is_any_processing() const
	{
		return data.process || data.process_internal || data.physics_process ||
			   data.physics_process_internal;
	}

	_FORCE_INLINE_ bool is_accessible_from_caller_thread() const
	{
		if (current_process_thread_group == nullptr) {
			// No thread processing.
			// Only accessible if node is outside the scene tree
			// or access will happen from a node-safe thread.
			return !data.inside_tree || is_current_thread_safe_for_nodes();
		}
		else {
			// Thread processing.
			return current_process_thread_group == data.process_thread_group_owner;
		}
	}

	_FORCE_INLINE_ bool is_readable_from_caller_thread() const
	{
		if (current_process_thread_group == nullptr) {
			// No thread processing.
			// Only accessible if node is outside the scene tree
			// or access will happen from a node-safe thread.
			return is_current_thread_safe_for_nodes() || unlikely(!data.inside_tree);
		}
		else {
			// Thread processing.
			return true;
		}
	}

	_FORCE_INLINE_ static bool is_group_processing() { return current_process_thread_group; }

	void set_process_thread_messages(uint32_t p_flags);
	uint32_t get_process_thread_messages() const;

	void queue_accessibility_update();

	virtual RID get_accessibility_element() const;
	virtual RID get_focused_accessibility_element() const;

	virtual bool accessibility_override_tree_hierarchy() const { return false; }

	virtual Transform2D get_accessibility_transform() const;

	Node* duplicate(int p_flags = DUPLICATE_GROUPS | DUPLICATE_SIGNALS | DUPLICATE_SCRIPTS) const;
#ifdef TOOLS_ENABLED
	Node* duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap) const;
	Node* duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap, Node* p_scene_root,
		HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const;
	void remap_node_resources(Node* p_node, Node* p_scene_root,
		HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const;
	void remap_nested_resources(
		Ref<Resource> p_resource, HashMap<Ref<Resource>, Ref<Resource>>& p_resource_remap) const;
#endif

	// used by editors, to save what has changed only
	void set_scene_instance_state(const Ref<SceneState>& p_state);
	Ref<SceneState> get_scene_instance_state() const;

	void set_scene_inherited_state(const Ref<SceneState>& p_state);
	Ref<SceneState> get_scene_inherited_state() const;

	void set_scene_instance_load_placeholder(bool p_enable);
	bool get_scene_instance_load_placeholder() const;

	void replace_by(Node* rp_node, bool p_keep_groups = true);

	void set_process_mode(ProcessMode p_mode);
	ProcessMode get_process_mode() const;
	bool can_process() const;
	bool can_process_notification(int p_what) const;

	void set_physics_interpolation_mode(PhysicsInterpolationMode p_mode);

	PhysicsInterpolationMode get_physics_interpolation_mode() const
	{
		return data.physics_interpolation_mode;
	}

	_FORCE_INLINE_ bool is_physics_interpolated() const { return data.physics_interpolated; }

	bool is_physics_interpolated_and_enabled() const;
	void reset_physics_interpolation();

	bool is_enabled() const;
	bool is_ready() const;

	void request_ready();

	void set_process_thread_group(ProcessThreadGroup p_mode);
	ProcessThreadGroup get_process_thread_group() const;

	static void print_orphan_nodes();

#ifdef TOOLS_ENABLED
	String validate_child_name(Node* p_child);
	String prevalidate_child_name(Node* p_child, String p_name);
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif
	static String adjust_name_casing(const String& p_name);

	void queue_free();

	// hacks for speed
	static void init_node_hrcr();

	bool is_owned_by_parent() const;

	void clear_internal_tree_resource_paths();

	_FORCE_INLINE_ Viewport* get_viewport() const { return data.viewport; }

	virtual Vector<String> get_configuration_warnings() const;

	void update_configuration_warnings();

	void set_display_folded(bool p_folded);
	bool is_displayed_folded() const;

	/* NETWORK */

	virtual void set_multiplayer_authority(int p_peer_id, bool p_recursive = true);
	int get_multiplayer_authority() const;
	bool is_multiplayer_authority() const;

	template <typename... VarArgs> Error rpc(const String& p_method, VarArgs... p_args);

	template <typename... VarArgs>
	Error rpc_id(int p_peer_id, const String& p_method, VarArgs... p_args);

	Ref<MultiplayerAPI> get_multiplayer() const;

	/* INTERNATIONALIZATION */

	void set_auto_translate_mode(AutoTranslateMode p_mode);
	AutoTranslateMode get_auto_translate_mode() const;
	bool can_auto_translate() const;

	virtual String get_translation_domain() const;
	virtual void set_translation_domain(const String& p_domain);
	void set_translation_domain_inherited();

	/* THREADING */

	void notify_deferred_thread_group(int p_notification);

	void notify_thread_safe(int p_notification);

	/* HELPER */

	bool is_instance() const { return !data.scene_file_path.is_empty(); }

	// These inherited functions need proper multithread locking when overridden in Node.
#ifdef DEBUG_ENABLED

	virtual bool has_meta(const String& p_name) const;
	virtual void remove_meta(const String& p_name);
	virtual void get_meta_list(List<String>* p_list) const;

	virtual bool has_signal(const String& p_name) const;
	virtual int get_persistent_signal_connection_count() const;

	virtual bool has_connections(const String& p_signal) const;
#endif
	Node();
	~Node();
};

typedef HashSet<Node*, Node::Comparator> NodeSet;

// Template definitions must be in the header so they are always fully initialized before their
// usage. See this StackOverflow question for more information:
// https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file

template <typename... VarArgs> Error Node::rpc(const String& p_method, VarArgs... p_args)
{
	return rpc_id(0, p_method, p_args...);
}

#ifdef DEBUG_ENABLED
#define ERR_THREAD_GUARD                                                                           \
	ERR_FAIL_COND_MSG(!is_accessible_from_caller_thread(),                                         \
		vformat("%s: The caller thread can't call the function `%s()` on this node. Use "          \
				"`call_deferred()` or `call_deferred_thread_group()` instead.",                    \
			get_description(), FUNCTION_STR));
#define ERR_THREAD_GUARD_V(m_ret)                                                                  \
	ERR_FAIL_COND_V_MSG(!is_accessible_from_caller_thread(), (m_ret),                              \
		vformat("%s: The caller thread can't call the function `%s()` on this node. Use "          \
				"`call_deferred()` or `call_deferred_thread_group()` instead.",                    \
			get_description(), FUNCTION_STR));
#define ERR_MAIN_THREAD_GUARD                                                                      \
	ERR_FAIL_COND_MSG(is_inside_tree() && !is_current_thread_safe_for_nodes(),                     \
		vformat("%s: The function `%s()` on this node can only be accessed from the main thread. " \
				"Use `call_deferred()` instead.",                                                  \
			get_description(), FUNCTION_STR));
#define ERR_MAIN_THREAD_GUARD_V(m_ret)                                                             \
	ERR_FAIL_COND_V_MSG(is_inside_tree() && !is_current_thread_safe_for_nodes(), (m_ret),          \
		vformat("%s: The function `%s()` on this node can only be accessed from the main thread. " \
				"Use `call_deferred()` instead.",                                                  \
			get_description(), FUNCTION_STR));
#define ERR_READ_THREAD_GUARD                                                                      \
	ERR_FAIL_COND_MSG(!is_readable_from_caller_thread(),                                           \
		vformat("%s: The function `%s()` on this node can only be accessed from either the main "  \
				"thread or a thread group. Use `call_deferred()` instead.",                        \
			get_description(), FUNCTION_STR));
#define ERR_READ_THREAD_GUARD_V(m_ret)                                                             \
	ERR_FAIL_COND_V_MSG(!is_readable_from_caller_thread(), (m_ret),                                \
		vformat("%s: The function `%s()` on this node can only be accessed from either the main "  \
				"thread or a thread group. Use `call_deferred()` instead.",                        \
			get_description(), FUNCTION_STR));
#else
#define ERR_THREAD_GUARD
#define ERR_THREAD_GUARD_V(m_ret)
#define ERR_MAIN_THREAD_GUARD
#define ERR_MAIN_THREAD_GUARD_V(m_ret)
#define ERR_READ_THREAD_GUARD
#define ERR_READ_THREAD_GUARD_V(m_ret)
#endif

// Add these macro to your class's 'get_configuration_warnings' function to have warnings show up in
// the scene tree inspector.
#define DEPRECATED_NODE_WARNING                                                                    \
	warnings.push_back(                                                                            \
		RTR("This node is marked as deprecated and will be removed in future versions.\nPlease "   \
			"check the Godot documentation for information about migration."));
#define EXPERIMENTAL_NODE_WARNING                                                                  \
	warnings.push_back(RTR("This node is marked as experimental and may be subject to removal or " \
						   "major changes in future versions."));

// --- Folded from scene/property_utils.h ---

// --- Folded from scene/resources/packed_scene.h ---
#include "core/types.h"
#include "core/typedefs.h"


class PackedScene;

class SceneState : public RefCounted
{
	Vector<String> names;
	Vector<NodePath> node_paths;
	Vector<PackedInt32Array> id_paths;
	mutable PackedInt32Array ids;
	Vector<NodePath> editable_instances;
	mutable HashMap<NodePath, int> node_path_cache;
	mutable HashMap<int, int> base_scene_node_remap;

	int base_scene_idx = -1;

	enum
	{
		NO_PARENT_SAVED = 0x7FFFFFFF,
		NAME_INDEX_BITS = 18,
		NAME_MASK = (1 << NAME_INDEX_BITS) - 1,
	};

	struct NodeData
	{
		int parent = 0;
		int owner = 0;
		int type = 0;
		int name = 0;
		int instance = 0;
		int index = 0;

		struct Property
		{
			int name = 0;
			int value = 0;
		};

		Vector<Property> properties;
		Vector<int> groups;
	};

	struct DeferredNodePathProperties
	{
		String property;
	};

	Vector<NodeData> nodes;

	struct ConnectionData
	{
		int from = 0;
		int to = 0;
		int signal = 0;
		int method = 0;
		int flags = 0;
		int unbinds = 0;
		Vector<int> binds;
	};

	Vector<ConnectionData> connections;

	String path;

	uint64_t last_modified_time = 0;

	static bool disable_placeholders;

	Vector<String> _get_node_groups(int p_idx) const;

	int _find_base_scene_node_remap_key(int p_idx) const;

	Node* _recover_node_path_index(Node* p_base, int p_idx) const;

#ifdef TOOLS_ENABLED
public:
	typedef void (*InstantiationWarningNotify)(const String& p_warning);

private:
	static InstantiationWarningNotify instantiation_warn_notify;
#endif

protected:

public:
	enum
	{
		FLAG_ID_IS_PATH = (1 << 30),
		TYPE_INSTANTIATED = 0x7FFFFFFF,
		FLAG_INSTANCE_IS_PLACEHOLDER = (1 << 30),
		FLAG_PATH_PROPERTY_IS_NODE = (1 << 30),
		FLAG_PROP_NAME_MASK = FLAG_PATH_PROPERTY_IS_NODE - 1,
		FLAG_MASK = (1 << 24) - 1,
	};

	enum GenEditState
	{
		GEN_EDIT_STATE_DISABLED,
		GEN_EDIT_STATE_INSTANCE,
		GEN_EDIT_STATE_MAIN,
		GEN_EDIT_STATE_MAIN_INHERITED,
	};

	struct PackState
	{
		Ref<SceneState> state;
		int node = -1;
	};

	static void set_disable_placeholders(bool p_disable);
	static Ref<Resource> get_remap_resource(const Ref<Resource>& p_resource,
		HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& remap_cache,
		const Ref<Resource>& p_fallback, Node* p_for_scene);

	int find_node_by_path(const NodePath& p_node) const;
	bool is_node_in_group(int p_node, const String& p_group) const;
	bool is_connection(
		int p_node, const String& p_signal, int p_to_node, const String& p_to_method) const;

	Error pack(Node* p_scene);

	void set_path(const String& p_path);
	String get_path() const;

	void clear();
	Error copy_from(const Ref<SceneState>& p_scene_state);

	bool can_instantiate() const;
	Node* instantiate(GenEditState p_edit_state) const;

	Ref<SceneState> get_base_scene_state() const;

	void update_instance_resource(String p_path, Ref<PackedScene> p_packed_scene);

	// unbuild API

	int get_node_count() const;
	String get_node_type(int p_idx) const;
	String get_node_name(int p_idx) const;
	NodePath get_node_path(int p_idx, bool p_for_parent = false) const;
	int32_t get_node_unique_id(int p_idx) const;
	PackedInt32Array get_node_id_path(int p_idx) const;
	PackedInt32Array get_node_parent_id_path(int p_idx) const;
	NodePath get_node_owner_path(int p_idx) const;
	PackedInt32Array get_node_owner_id_path(int p_idx) const;
	Ref<PackedScene> get_node_instance(int p_idx) const;
	String get_node_instance_placeholder(int p_idx) const;
	bool is_node_instance_placeholder(int p_idx) const;
	Vector<String> get_node_groups(int p_idx) const;
	int get_node_index(int p_idx) const;

	int get_node_property_count(int p_idx) const;
	String get_node_property_name(int p_idx, int p_prop) const;
	Vector<String> get_node_deferred_nodepath_properties(int p_idx) const;

	int get_connection_count() const;
	NodePath get_connection_source(int p_idx) const;
	String get_connection_signal(int p_idx) const;
	NodePath get_connection_target(int p_idx) const;
	String get_connection_method(int p_idx) const;

	PackedInt32Array get_connection_source_id_path(int p_idx) const;
	PackedInt32Array get_connection_target_id_path(int p_idx) const;

	int get_connection_flags(int p_idx) const;
	int get_connection_unbinds(int p_idx) const;

	bool has_connection(const NodePath& p_node_from, const String& p_signal,
		const NodePath& p_node_to, const String& p_method, bool p_no_inheritance = false);

	Vector<NodePath> get_editable_instances() const;
	Ref<Resource> get_sub_resource(const String& p_path);
	Vector<Ref<Resource>> get_sub_resources();

	// build API

	int add_name(const String& p_name);
	int add_node_path(const NodePath& p_path, const PackedInt32Array& p_uid_path);
	int add_node(int p_parent, int p_owner, int p_type, int p_name, int p_instance, int p_index,
		int32_t p_unique_id);
	void add_node_property(int p_node, int p_name, int p_value, bool p_deferred_node_path = false);
	void add_node_group(int p_node, int p_group);
	void set_base_scene(int p_idx);
	void add_connection(int p_from, int p_to, int p_signal, int p_method, int p_flags,
		int p_unbinds, const Vector<int>& p_binds);
	void add_editable_instance(const NodePath& p_path);

	bool remove_group_references(const String& p_name);
	bool rename_group_references(const String& p_old_name, const String& p_new_name);
	HashSet<String> get_all_groups();

	virtual void set_last_modified_time(uint64_t p_time) { last_modified_time = p_time; }

	uint64_t get_last_modified_time() const { return last_modified_time; }

	// Used when saving pointers (saves a path property instead).
	static String get_meta_pointer_property(const String& p_property);

#ifdef TOOLS_ENABLED
	static void set_instantiation_warning_notify_func(InstantiationWarningNotify p_warn_notify)
	{
		instantiation_warn_notify = p_warn_notify;
	}
#endif

	SceneState() = default;
};


class PackedScene : public Resource
{
	Ref<SceneState> state;

protected:
	virtual bool editor_can_reload_from_file() override
	{
		return false;
	} // this is handled by editor better

	virtual void reset_state() override;

public:
	enum GenEditState
	{
		GEN_EDIT_STATE_DISABLED,
		GEN_EDIT_STATE_INSTANCE,
		GEN_EDIT_STATE_MAIN,
		GEN_EDIT_STATE_MAIN_INHERITED,
	};

	Error pack(Node* p_scene);

	void clear();

	bool can_instantiate() const;
	Node* instantiate(GenEditState p_edit_state = GEN_EDIT_STATE_DISABLED) const;

	void recreate_state();
	void replace_state(Ref<SceneState> p_by);

	virtual void reload_from_file() override;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;
	virtual void set_path_cache(const String& p_path) override;

	Ref<SceneState> get_state() const;

	PackedScene();
};

// --- Folded from scene/resources/resource_format_text.h ---
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/types.h"


class ResourceLoaderText
{
public:
	enum
	{
		// Version 2: Changed names for Basis, AABB, Vectors, etc.
		// Version 3: New string ID for ext/subresources, breaks forward compat.
		// Version 4: PackedByteArray can be base64 encoded, and PackedVector4Array was added.
		FORMAT_VERSION = 4,
		// For compat, save as version 3 if not using PackedVector4Array or no big PackedByteArray.
		FORMAT_VERSION_COMPAT = 3,
	};

private:
	bool translation_remapped = false;
	String local_path;
	String res_path;
	String error_text;

	Ref<FileAccess> f;

	struct ExtResource
	{
		Ref<ResourceLoader::LoadToken> load_token;
		String path;
		String type;
	};

	bool is_scene = false;
	int format_version;
	String res_type;

	bool ignore_resource_parsing = false;

	HashMap<String, ExtResource> ext_resources;
	HashMap<String, Ref<Resource>> int_resources;

	int resources_total = 0;
	int resource_current = 0;

	String resource_type;
	String script_class;

	ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
	ResourceFormatLoader::CacheMode cache_mode_for_external =
		ResourceFormatLoader::CACHE_MODE_REUSE;

	bool use_sub_threads = false;
	float* progress = nullptr;

	mutable int lines = 0;

	ResourceUID::ID res_uid = ResourceUID::INVALID_ID;

	HashMap<String, String> remaps;

	void _count_resources();

	struct DummyReadData
	{
		bool no_placeholders = false;
		HashMap<Ref<Resource>, int> external_resources;
		HashMap<String, Ref<Resource>> rev_external_resources;
		HashMap<Ref<Resource>, int> resource_index_map;
		HashMap<String, Ref<Resource>> resource_map;
	};

	void _printerr();

	friend class ResourceFormatLoaderText;
	friend
 class ResourceFormatSaverText;

	Error error = OK;

	Ref<Resource> resource;

public:
	Ref<Resource> get_resource();
	Error load();
	Error set_uid(Ref<FileAccess> p_f, ResourceUID::ID p_uid);
	int get_stage() const;
	int get_stage_count() const;
	void set_translation_remapped(bool p_remapped);

	void open(Ref<FileAccess> p_f, bool p_skip_first_tag = false);
	String recognize(Ref<FileAccess> p_f);
	String recognize_script_class(Ref<FileAccess> p_f);
	ResourceUID::ID get_uid(Ref<FileAccess> p_f);
	void get_dependencies(Ref<FileAccess> p_f, List<String>* p_dependencies, bool p_add_types);
	Error rename_dependencies(
		Ref<FileAccess> p_f, const String& p_path, const HashMap<String, String>& p_map);
	Error get_classes_used(HashSet<String>* r_classes);

	ResourceLoaderText() = default;
};

class ResourceFormatLoaderText : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override;

	virtual String get_resource_type(const String& p_path) const override;
	virtual String get_resource_script_class(const String& p_path) const override;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override;
	virtual bool has_custom_uid_support() const override;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override;
	virtual Error rename_dependencies(
		const String& p_path, const HashMap<String, String>& p_map) override;

	ResourceFormatLoaderText() = default;
};

class ResourceFormatSaverTextInstance
{
	String local_path;

	Ref<PackedScene> packed_scene;

	bool takeover_paths = false;
	bool relative_paths = false;
	bool bundle_resources = false;
	bool skip_editor = false;

	struct NonPersistentKey
	{ // for resource properties generated on the fly
		Ref<Resource> base;
		String property;

		bool operator<(const NonPersistentKey& p_key) const
		{
			return base == p_key.base ? property < p_key.property : base < p_key.base;
		}
	};

	HashSet<Ref<Resource>> resource_set;
	List<Ref<Resource>> saved_resources;
	HashMap<Ref<Resource>, String> external_resources;
	HashMap<Ref<Resource>, String> internal_resources;
	bool use_compat = true;

	struct ResourceSort
	{
		Ref<Resource> resource;
		String id;

		bool operator<(const ResourceSort& p_right) const
		{
			return id.naturalnocasecmp_to(p_right.id) < 0;
		}
	};

	static String _write_resources(void* ud, const Ref<Resource>& p_resource);
	String _write_resource(const Ref<Resource>& res);

public:
	Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
};

class ResourceFormatSaverText : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceFormatSaverText() = default;
};



// --- End scene/resources/resource_format_text.h ---

// --- Folded from scene/resources/video_stream.h ---
#include "servers/display/display_server.h"

class VideoStreamPlayback : public Resource
{
public:
	typedef int (*AudioMixCallback)(void* p_udata, const float* p_data, int p_frames);
	struct TheoraData;

private:
	TheoraData* data = nullptr;

protected:
	AudioMixCallback mix_callback = nullptr;
	void* mix_udata = nullptr;
	mutable int _channel_count = 0; // Used only to assist with bounds checking in mix_audio.

	int mix_audio(int num_frames, PackedFloat32Array buffer = {}, int offset = 0);

public:
	VideoStreamPlayback();
	~VideoStreamPlayback();

	void stop();
	void play();

	bool is_playing() const;

	void set_paused(bool p_paused);
	bool is_paused() const;

	double get_length() const;

	double get_playback_position() const;
	void seek(double p_time);

	void set_file(const String& p_file);
	void set_audio_track(int p_idx);

	Ref<Texture2D> get_texture() const;
	void update(double p_delta);

	void set_mix_callback(AudioMixCallback p_callback, void* p_userdata);
	int get_channels() const;
	int get_mix_rate() const;
};

class VideoStream : public Resource
{
protected:
	String file;
	int audio_track = 0;

public:
	void set_file(const String& p_file);
	String get_file();

	void set_audio_track(int p_track);
	Ref<VideoStreamPlayback> instantiate_playback();

	VideoStream();
	~VideoStream();
};

// --- End scene/resources/video_stream.h ---
// --- End scene/resources/packed_scene.h ---

class PropertyUtils
{
public:
	// Gets the instance/inheritance states of this node, in order of precedence,
	// that is, from the topmost (the most able to override values) to the lowermost
	// (Note that in nested instantiation, the one with the greatest precedence is the furthest
	// in the tree, since every owner found while traversing towards the root gets a chance
	// to override property values.)
	static Vector<SceneState::PackState> get_node_states_stack(
		const Node* p_node, const Node* p_owner = nullptr, bool* r_instantiated_by_owner = nullptr);
};
// --- End scene/property_utils.h ---

