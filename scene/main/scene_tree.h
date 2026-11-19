/**************************************************************************/
/*  scene_tree.h                                                          */
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

#include <cstdlib>
#include "core/os/main_loop.h"
#include "core/os/thread_safe.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/self_list.h"
#include "core/types.h"
#include "scene/animation/tween.h"
#include "scene/main/multiplayer_api.h"
#include "scene/main/scene_tree_fti.h"

class ArrayMesh;
struct InputEvent;
class Material;
class MultiplayerAPI;
class Node;
class PackedScene;
class Tween;
class Viewport;
class Window;

#ifndef _3D_DISABLED
class Node3D;
#endif

class SceneTreeTimer : public RefCounted
{
	double time_left = 0.0;
	bool process_always = true;
	bool process_in_physics = false;
	bool ignore_time_scale = false;

public:
	void set_time_left(double p_time);
	double get_time_left() const;

	void set_process_always(bool p_process_always);
	bool is_process_always();

	void set_process_in_physics(bool p_process_in_physics);
	bool is_process_in_physics();

	void set_ignore_time_scale(bool p_ignore);
	bool is_ignoring_time_scale();

	void release_connections();
};

struct SceneTreeGroup
{
	Vector<Node*> nodes;
	bool changed = false;
};

class SceneTree final
{
	static inline Mutex _thread_safe_;

public:
	typedef void (*IdleCallback)();

	struct ProcessGroup
	{
		Vector<Node*> nodes;
		Vector<Node*> physics_nodes;
		bool node_order_dirty = true;
		bool physics_node_order_dirty = true;
		bool removed = false;
		Node* owner = nullptr;
		uint64_t last_pass = 0;
	};

	struct ProcessGroupSort
	{
		_FORCE_INLINE_ bool operator()(
			const ProcessGroup* p_left, const ProcessGroup* p_right) const;
	};

#ifndef _3D_DISABLED
	struct ClientPhysicsInterpolation
	{
		SelfList<Node3D>::List _node_3d_list;
		void physics_process();
	};
#endif

	struct UGCall
	{
		String group;
		String call;

		static uint32_t hash(const UGCall& p_val) { return p_val.group.hash() ^ p_val.call.hash(); }

		bool operator==(const UGCall& p_with) const
		{
			return group == p_with.group && call == p_with.call;
		}

		bool operator<(const UGCall& p_with) const
		{
			return group == p_with.group ? call < p_with.call : group < p_with.group;
		}
	};

	struct Data
	{
		PagedAllocator<ProcessGroup, true> group_allocator;
		LocalVector<ProcessGroup*> process_groups;
		bool process_groups_dirty = true;
		LocalVector<ProcessGroup*> local_process_group_cache;
		uint64_t process_last_pass = 1;

		ProcessGroup default_process_group;

		bool node_threading_disabled = false;

#ifndef _3D_DISABLED
		ClientPhysicsInterpolation _client_physics_interpolation;
#endif

		Window* root = nullptr;

		double physics_process_time = 0.0;
		double process_time = 0.0;
		bool accept_quit = true;
		bool quit_on_go_back = true;

#ifdef DEBUG_ENABLED
		bool debug_collisions_hint = false;
		bool debug_paths_hint = false;
		bool debug_navigation_hint = false;
#endif
		bool paused = false;
		bool suspended = false;

		HashMap<String, SceneTreeGroup> group_map;
		bool _quit = false;

		String tree_changed_name = "tree_changed";
		String node_added_name = "node_added";
		String node_removed_name = "node_removed";
		String node_renamed_name = "node_renamed";

		int64_t current_frame = 0;
		int nodes_in_tree_count = 0;

#ifdef TOOLS_ENABLED
		Node* edited_scene_root = nullptr;
#endif

		int nodes_removed_on_group_call_lock = 0;
		HashSet<Node*> nodes_removed_on_group_call;

		uint64_t accessibility_upd_per_sec = 0;
		bool accessibility_force_update = true;
		uint64_t accessibility_last_update = 0;

		bool ugc_locked = false;

		Node* current_scene = nullptr;

		Color debug_collisions_color;
		Color debug_collision_contact_color;
		Color debug_paths_color;
		float debug_paths_width = 1.0f;
		Ref<ArrayMesh> debug_contact_mesh;
		Ref<Material> debug_paths_material;
		Ref<Material> collision_material;
		int collision_debug_contacts = 0;

		List<Ref<SceneTreeTimer>> timers;
		List<Ref<Tween>> tweens;

		Ref<MultiplayerAPI> multiplayer;
		HashMap<NodePath, Ref<MultiplayerAPI>> custom_multiplayers;
		bool multiplayer_poll = true;

		SelfList<Node>::List xform_change_list;
	};

	static inline Data* data = nullptr;

private:
	// Static so we can get directly instead of via SceneTree::data.
	static bool _physics_interpolation_enabled;
	static bool _physics_interpolation_enabled_in_project;

	static void _flush_ugc();
	_FORCE_INLINE_ static void _update_group_order(SceneTreeGroup& g);
	static void _flush_scene_change();

	friend class Node;
	friend class CanvasItem;
	friend class Node3D;
	friend class Viewport;
#ifdef DEBUG_ENABLED
	friend class LiveEditor;
#endif

	static void tree_changed();
	static void node_added(Node* p_node);
	static void node_removed(Node* p_node);
	static void node_renamed(Node* p_node);
	static void process_timers(double p_delta, bool p_physics_frame);
	static void process_tweens(double p_delta, bool p_physics_frame);

	static SceneTreeGroup* add_to_group(const String& p_group, Node* p_node);
	static void remove_from_group(const String& p_group, Node* p_node);

	static void _process_group(ProcessGroup* p_group, bool p_physics);
	static void _process_groups_thread(uint32_t p_index, bool p_physics);
	static void _process(bool p_physics);

	static void _remove_process_group(Node* p_node);
	static void _add_process_group(Node* p_node);
	static void _remove_node_from_process_group(Node* p_node, Node* p_owner);
	static void _add_node_to_process_group(Node* p_node, Node* p_owner);

	static void _flush_delete_queue();

	enum
	{
		MAX_IDLE_CALLBACKS = 256
	};

	static IdleCallback idle_callbacks[MAX_IDLE_CALLBACKS];
	static int idle_callback_count;
	static void _call_idle_callbacks();

	static void _main_window_focus_in();
	static void _main_window_close();
	static void _main_window_go_back();

	enum CallInputType
	{
		CALL_INPUT_TYPE_INPUT,
		CALL_INPUT_TYPE_SHORTCUT_INPUT,
		CALL_INPUT_TYPE_UNHANDLED_INPUT,
		CALL_INPUT_TYPE_UNHANDLED_KEY_INPUT,
	};

	static void _call_input_pause(const String& p_group, CallInputType p_call_type,
		const InputEvent& p_input, Viewport* p_viewport);

public:
	SceneTree() = delete;

	enum
	{
		// Keep in sync with CanvasItem, Node3D, and MainLoop.
		NOTIFICATION_TRANSFORM_CHANGED = 2000,
		NOTIFICATION_OS_MEMORY_WARNING = MainLoop::NOTIFICATION_OS_MEMORY_WARNING,
		NOTIFICATION_TRANSLATION_CHANGED = MainLoop::NOTIFICATION_TRANSLATION_CHANGED,
		NOTIFICATION_WM_ABOUT = MainLoop::NOTIFICATION_WM_ABOUT,
		NOTIFICATION_CRASH = MainLoop::NOTIFICATION_CRASH,
		NOTIFICATION_OS_IME_UPDATE = MainLoop::NOTIFICATION_OS_IME_UPDATE,
		NOTIFICATION_APPLICATION_RESUMED = MainLoop::NOTIFICATION_APPLICATION_RESUMED,
		NOTIFICATION_APPLICATION_PAUSED = MainLoop::NOTIFICATION_APPLICATION_PAUSED,
		NOTIFICATION_APPLICATION_FOCUS_IN = MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN,
		NOTIFICATION_APPLICATION_FOCUS_OUT = MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT,
		NOTIFICATION_TEXT_SERVER_CHANGED = MainLoop::NOTIFICATION_TEXT_SERVER_CHANGED,
		NOTIFICATION_APPLICATION_PIP_MODE_ENTERED =
			MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_ENTERED,
		NOTIFICATION_APPLICATION_PIP_MODE_EXITED =
			MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_EXITED,
	};

	enum GroupCallFlags
	{
		GROUP_CALL_DEFAULT = 0,
		GROUP_CALL_REVERSE = 1,
		GROUP_CALL_DEFERRED = 2,
		GROUP_CALL_UNIQUE = 4,
	};

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void initialize_tree();
	static void finalize_tree();
	static void notification(int p_notification);

	static Window* get_root();

	static void notify_group_flags(
		uint32_t p_call_flags, const String& p_group, int p_notification);
	static void notify_group(const String& p_group, int p_notification);

	static void flush_transform_notifications();

	static bool is_accessibility_enabled();
	static bool is_accessibility_supported();
	static void _accessibility_force_update();
	static void _accessibility_notify_change(const Node* p_node, bool p_remove = false);
	static void _flush_accessibility_changes();
	static void _process_accessibility_changes(int p_window_id);

	static void iteration_prepare();
	static bool physics_process(double p_time);
	static void iteration_end();
	static bool process(double p_time);

	static bool is_auto_accept_quit();
	static void set_auto_accept_quit(bool p_enable);

	static bool is_quit_on_go_back();
	static void set_quit_on_go_back(bool p_enable);

	static void quit(int p_exit_code = EXIT_SUCCESS);

	_FORCE_INLINE_ static double get_physics_process_time()
	{
		return data ? data->physics_process_time : 0.0;
	}

	_FORCE_INLINE_ static double get_process_time() { return data ? data->process_time : 0.0; }

	static void set_pause(bool p_enabled);
	static bool is_paused();
	static void set_suspend(bool p_enabled);
	static bool is_suspended();

#ifdef DEBUG_ENABLED
	static void set_debug_collisions_hint(bool p_enabled);
	static bool is_debugging_collisions_hint();

	static void set_debug_paths_hint(bool p_enabled);
	static bool is_debugging_paths_hint();

	static void set_debug_navigation_hint(bool p_enabled);
	static bool is_debugging_navigation_hint();
#else
	static void set_debug_collisions_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_collisions_hint() { return false; }

	static void set_debug_paths_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_paths_hint() { return false; }

	static void set_debug_navigation_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_navigation_hint() { return false; }
#endif

	static void set_debug_collisions_color(const Color& p_color);
	static Color get_debug_collisions_color();

	static void set_debug_collision_contact_color(const Color& p_color);
	static Color get_debug_collision_contact_color();

	static void set_debug_paths_color(const Color& p_color);
	static Color get_debug_paths_color();

	static void set_debug_paths_width(float p_width);
	static float get_debug_paths_width();

	static Ref<Material> get_debug_paths_material();
	static Ref<Material> get_debug_collision_material();
	static Ref<ArrayMesh> get_debug_contact_mesh();

	static int get_collision_debug_contact_count()
	{
		return data ? data->collision_debug_contacts : 0;
	}

	static int64_t get_frame();

	static int get_node_count();

	static Vector<Node*> get_nodes_in_group(const String& p_group);
	static Node* get_first_node_in_group(const String& p_group);
	static bool has_group(const String& p_identifier);
	static int get_node_count_in_group(const String& p_group);

	static void set_edited_scene_root(Node* p_node);
	static Node* get_edited_scene_root();

	static void set_current_scene(Node* p_scene);
	static Node* get_current_scene();
	static Error change_scene_to_file(const String& p_path);
	static Error change_scene_to_packed(PackedScene* rp_scene);
	static Error change_scene_to_node(Node* rp_node);
	static Error reload_current_scene();
	static void unload_current_scene();

	static SceneTreeTimer* create_timer(double p_delay_sec, bool p_process_always = true,
		bool p_process_in_physics = false, bool p_ignore_time_scale = false);
	static Tween* create_tween();
	static void remove_tween(const Ref<Tween>& p_tween);

	// used by Main::start, don't use otherwise
	static void add_current_scene(Node* p_current);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	// network API
	static MultiplayerAPI* get_multiplayer(const NodePath& p_for_path = NodePath());
	static void set_multiplayer(
		Ref<MultiplayerAPI> p_multiplayer, const NodePath& p_root_path = NodePath());
	static void set_multiplayer_poll_enabled(bool p_enabled);
	static bool is_multiplayer_poll_enabled();

	static void add_idle_callback(IdleCallback p_callback);

	static void set_disable_node_threading(bool p_disable);

	static void set_physics_interpolation_enabled(bool p_enabled);
	static bool is_physics_interpolation_enabled() { return _physics_interpolation_enabled; }

	// Different name to disambiguate fast static versions from the user bound versions.
	static bool is_fti_enabled() { return _physics_interpolation_enabled; }
	static bool is_fti_enabled_in_project() { return _physics_interpolation_enabled_in_project; }

#ifndef _3D_DISABLED
	static void client_physics_interpolation_add_node_3d(SelfList<Node3D>* p_elem);
	static void client_physics_interpolation_remove_node_3d(SelfList<Node3D>* p_elem);
#endif
};
