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
#include "scene/animation/animation_mixer.h"
// --- Inlined/Folded from scene/main/multiplayer_api.h ---
#include "core/templates/hash_set.h"
#include "core/types.h"
#include "scene/main/multiplayer_peer.h"

class MultiplayerAPI : public RefCounted
{
public:
	struct SceneData;

private:
	static String default_interface;
	SceneData* scene_data = nullptr;

	Error _send(const uint8_t* p_packet, int p_packet_len);
	void _process_packet(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_raw(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_sys(int p_from, const uint8_t* p_packet, int p_packet_len,
		MultiplayerPeer::TransferMode p_mode, int p_channel);
	void _add_peer(int p_id);
	void _admit_peer(int p_id);
	void _del_peer(int p_id);
	void _update_status();

public:
	enum RPCMode
	{
		RPC_MODE_DISABLED,	// No rpc for this method, calls to this will be blocked (default)
		RPC_MODE_ANY_PEER,	// Any peer can call this RPC
		RPC_MODE_AUTHORITY, // Only the node's multiplayer authority (server by default) can call
							// this RPC
	};

	enum NetworkCommands
	{
		NETWORK_COMMAND_REMOTE_CALL = 0,
		NETWORK_COMMAND_SIMPLIFY_PATH,
		NETWORK_COMMAND_CONFIRM_PATH,
		NETWORK_COMMAND_RAW,
		NETWORK_COMMAND_SPAWN,
		NETWORK_COMMAND_DESPAWN,
		NETWORK_COMMAND_SYNC,
		NETWORK_COMMAND_SYS,
	};

	enum SysCommands
	{
		SYS_COMMAND_AUTH,
		SYS_COMMAND_ADD_PEER,
		SYS_COMMAND_DEL_PEER,
		SYS_COMMAND_RELAY,
	};

	enum
	{
		SYS_CMD_SIZE = 6, // Command + sys command + peer_id (+ optional payload).
	};

	// For each command, the 4 MSB can contain custom flags, as defined by subsystems.
	enum
	{
		CMD_FLAG_0_SHIFT = 4,
		CMD_FLAG_1_SHIFT = 5,
		CMD_FLAG_2_SHIFT = 6,
		CMD_FLAG_3_SHIFT = 7,
	};

	// This is the mask that will be used to extract the command.
	enum
	{
		CMD_MASK = 7, // 0x7 -> 0b00000111
	};

	static Ref<MultiplayerAPI> create_default_interface();
	static void set_default_interface(const String& p_interface);
	static String get_default_interface();

	Error poll();
	void set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer);
	Ref<MultiplayerPeer> get_multiplayer_peer();
	int get_unique_id();
	Vector<int> get_peer_ids();
	int get_remote_sender_id() const;

	bool has_multiplayer_peer() { return get_multiplayer_peer().is_valid(); }
	bool is_server() { return get_unique_id() == MultiplayerPeer::TARGET_PEER_SERVER; }

	void clear();

	void set_root_path(const NodePath& p_path);
	NodePath get_root_path() const;

	void disconnect_peer(int p_id);

	Error send_auth(int p_to, Vector<uint8_t> p_bytes);
	Error complete_auth(int p_peer);
	void set_auth_timeout(double p_timeout);
	double get_auth_timeout() const;
	Vector<int> get_authenticating_peer_ids();

	Error send_command(int p_to, const uint8_t* p_packet, int p_packet_len);
	Error send_bytes(Vector<uint8_t> p_data, int p_to = MultiplayerPeer::TARGET_PEER_BROADCAST,
		MultiplayerPeer::TransferMode p_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE,
		int p_channel = 0);

	HashSet<int> get_connected_peers() const;
	void set_remote_sender_override(int p_id);

	void set_refuse_new_connections(bool p_refuse);
	bool is_refusing_new_connections() const;

	void set_allow_object_decoding(bool p_enable);
	bool is_object_decoding_allowed() const;

	void set_server_relay_enabled(bool p_enabled);
	bool is_server_relay_enabled() const;

	void set_max_sync_packet_size(int p_size);
	int get_max_sync_packet_size() const;

	void set_max_delta_packet_size(int p_size);
	int get_max_delta_packet_size() const;

	MultiplayerAPI();
	~MultiplayerAPI();
};
// --- Folded from scene/main/scene_tree_fti.h ---
#include "core/os/mutex.h"
#include "core/templates/local_vector.h"

class Node3D;
class Node;
struct Transform3D;
class SceneTreeFTITests;

#ifdef DEV_ENABLED
// Uncomment this to verify traversal method results.
// #define GODOT_SCENE_TREE_FTI_VERIFY
#endif

#ifdef _3D_DISABLED
// Stubs
class SceneTreeFTI final
{
public:
	SceneTreeFTI() = delete;

	static void initialize() {}
	static void finalize() {}
	_FORCE_INLINE_ static bool is_initialized() { return true; }

	static void frame_update(Node* p_root, bool p_frame_start)
	{
		(void)p_root;
		(void)p_frame_start;
	}
	static void tick_update() {}
	static void set_enabled(Node* p_root, bool p_enabled)
	{
		(void)p_root;
		(void)p_enabled;
	}
	static bool is_enabled() { return false; }

	static void node_3d_notify_changed(Node3D& r_node, bool p_transform_changed)
	{
		(void)r_node;
		(void)p_transform_changed;
	}
	static void node_3d_notify_delete(Node3D* p_node) { (void)p_node; }
	static void node_3d_request_reset(Node3D* p_node) { (void)p_node; }
};
#else

// Important.
// This class uses raw pointers, so it is essential that on deletion, this class is notified
// so that any references can be cleared up to prevent dangling pointer access.

class SceneTreeFTI final
{
	friend class SceneTreeFTITests;

public:
	enum TraversalMode : unsigned
	{
		TM_DEFAULT,
		TM_LEGACY,
		TM_DEBUG,
	};

	struct Data
	{
		static const uint32_t scene_tree_depth_limit = 48;

		// Prev / Curr lists of Node3Ds having local xforms pumped.
		LocalVector<Node3D*> tick_xform_list[2];

		// The frame lists are changed nodes that need to start traversal,
		// either longterm (on the tick list) or single frame forced.
		LocalVector<Node3D*> frame_xform_list;
		LocalVector<Node3D*> frame_xform_list_forced;

		// Prev / Curr lists of Node3Ds having actively interpolated properties.
		LocalVector<Node3D*> tick_property_list[2];

		LocalVector<Node3D*> frame_property_list;
		LocalVector<Node3D*> request_reset_list;
		LocalVector<Node3D*> dirty_node_depth_lists[scene_tree_depth_limit];

		// When we are using two alternating lists,
		// which one is current.
		uint32_t mirror = 0;

		// Global on / off switch for SceneTreeFTI.
		bool enabled = false;

		// Whether we are in physics ticks, or in a frame.
		bool in_frame = false;

		// Updating at the start of the frame, or the end on second pass.
		bool frame_start = true;

		Mutex mutex;

		TraversalMode traversal_mode = TM_DEFAULT;
		bool use_optimized_traversal_method = true;

		// DEBUGGING
		bool periodic_debug_log = false;
		uint32_t debug_node_count = 0;
		uint32_t debug_nodes_processed = 0;

#ifdef GODOT_SCENE_TREE_FTI_VERIFY
		SceneTreeFTITests* _tests = nullptr;
#endif
	};

	static inline Data* data = nullptr;

private:
	static void _update_dirty_nodes(Node* p_node, uint32_t p_current_half_frame,
		float p_interpolation_fraction, bool p_active,
		const Transform3D* p_parent_global_xform = nullptr, int p_depth = 0);
	static void _update_request_resets();

	static void _reset_flags(Node* p_node);
	static void _reset_node3d_flags(Node3D& r_node);
	static void _node_3d_notify_set_xform(Node3D& r_node);
	static void _node_3d_notify_set_property(Node3D& r_node);

	static void _node_add_to_frame_list(Node3D& r_node, bool p_forced);
	static void _node_remove_from_frame_list(Node3D& r_node, bool p_forced);

	static void _create_depth_lists();
	static void _clear_depth_lists();

public:
	SceneTreeFTI() = delete;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	// Hottest function, allow inlining the data->enabled check.
	static void node_3d_notify_changed(Node3D& r_node, bool p_transform_changed)
	{
		if (!data || !data->enabled) {
			return;
		}
		MutexLock lock(data->mutex);

		if (p_transform_changed) {
			_node_3d_notify_set_xform(r_node);
		}
		else {
			_node_3d_notify_set_property(r_node);
		}
	}

	static void node_3d_request_reset(Node3D* p_node);
	static void node_3d_notify_delete(Node3D* p_node);

	// Calculate interpolated xforms, send to visual server.
	static void frame_update(Node* p_root, bool p_frame_start);

	// Update local xform pumps.
	static void tick_update();

	static void set_enabled(Node* p_root, bool p_enabled);
	static bool is_enabled() { return data ? data->enabled : false; }

	static void set_debug_next_frame()
	{
		if (data) {
			data->periodic_debug_log = true;
		}
	}
};

#endif // ndef _3D_DISABLED

// --- End scene/main/scene_tree_fti.h ---

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

// --- Folded from scene/main/instance_placeholder.h ---
#include "scene/main/node.h"

class PackedScene;

class InstancePlaceholder : public Node
{
	String path;

private:
	Node* try_get_node(
		InstancePlaceholder* p_placeholder, Node* p_instance, const NodePath& p_path);

public:
	void set_instance_path(const String& p_name);
	String get_instance_path() const;

	Node* create_instance(
		bool p_replace = false, const Ref<PackedScene>& p_custom_scene = Ref<PackedScene>());

	InstancePlaceholder();
};



// --- End scene/main/instance_placeholder.h ---

// --- Folded from scene/main/missing_node.h ---
#include "scene/main/node.h"

class MissingNode : public Node
{
	String original_class;
	String original_scene;
	bool recording_properties = false;
	bool recording_signals = false;

protected:

public:
	void set_original_class(const String& p_class);
	String get_original_class() const;

	void set_original_scene(const String& p_scene);
	String get_original_scene() const;

	void set_recording_properties(bool p_enable);
	bool is_recording_properties() const;

	void set_recording_signals(bool p_enable);
	bool is_recording_signals() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	MissingNode();
};



// --- End scene/main/missing_node.h ---
