/**************************************************************************/
/*  multiplayer_synchronizer.h                                            */
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

#include "core/templates/local_vector.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"
#include "scene_multiplayer.h"

class MultiplayerSynchronizer final : public Node
{
public:
	enum Mode
	{
		MODE_SYNCHRONIZER,
		MODE_SPAWNER,
	};

	enum VisibilityUpdateMode
	{
		VISIBILITY_PROCESS_IDLE,
		VISIBILITY_PROCESS_PHYSICS,
		VISIBILITY_PROCESS_NONE,
	};

	enum
	{
		INVALID_ID = 0xFF,
	};

private:
	struct Watcher
	{
		NodePath prop;
		uint64_t last_change_usec = 0;
	};

	struct SpawnableScene
	{
		String path;
		Ref<PackedScene> cache;
	};

	struct SpawnInfo
	{
		int id = INVALID_ID;

		SpawnInfo() {}
	};

	Mode mode = MODE_SYNCHRONIZER;

	// Synchronizer state.
	Ref<SceneReplicationConfig> replication_config;
	NodePath root_path = NodePath(".."); // Start with parent, like with AnimationPlayer.
	uint64_t sync_interval_usec = 0;
	uint64_t delta_interval_usec = 0;
	VisibilityUpdateMode visibility_update_mode = VISIBILITY_PROCESS_IDLE;
	HashSet<int> peer_visibility;
	Vector<Watcher> watchers;
	uint64_t last_watch_usec = 0;

	uint64_t last_sync_usec = 0;
	uint16_t last_inbound_sync = 0;
	uint32_t net_id = 0;
	bool sync_started = false;

	// Spawner state.
	LocalVector<SpawnableScene> spawnable_scenes;
	HashSet<ResourceUID::ID> spawnable_ids;
	NodePath spawn_path;
	uint32_t spawn_limit = 0;

	void _start();
	void _stop();
	void _update_process();
	Error _watch_changes(uint64_t p_usec);

	void _update_spawn_node();
	void _node_added(Node* p_node);

	Vector<String> _get_spawnable_scenes() const;
	void _set_spawnable_scenes(const Vector<String>& p_scenes);

protected:
	void _notification(int p_what);

public:
	void set_mode(Mode p_mode) { mode = p_mode; }
	Mode get_mode() const { return mode; }

	void reset();
	Node* get_root_node();

	uint32_t get_net_id() const;
	void set_net_id(uint32_t p_net_id);

	bool update_outbound_sync_time(uint64_t p_usec);
	bool update_inbound_sync_time(uint16_t p_network_time);

	PackedStringArray get_configuration_warnings() const override;

	void set_replication_interval(double p_interval);
	double get_replication_interval() const;

	void set_delta_interval(double p_interval);
	double get_delta_interval() const;

	void set_replication_config(Ref<SceneReplicationConfig> p_config);
	Ref<SceneReplicationConfig> get_replication_config();

	NodePath get_root_path() const;
	virtual void set_multiplayer_authority(int p_peer_id, bool p_recursive = true) override;

	bool is_visibility_public() const;
	void set_visibility_public(bool p_public);
	bool is_visible_to(int p_peer);
	void set_visibility_for(int p_peer, bool p_visible);
	bool get_visibility_for(int p_peer) const;
	void update_visibility(int p_for_peer);
	void set_visibility_update_mode(VisibilityUpdateMode p_mode);
	VisibilityUpdateMode get_visibility_update_mode() const;

	List<NodePath> get_delta_properties(uint64_t p_indexes);
	SceneReplicationConfig* get_replication_config_ptr() const;

	// Spawner API.
	void add_spawnable_scene(const String& p_path);
	int get_spawnable_scene_count() const;
	String get_spawnable_scene(int p_idx) const;
	void clear_spawnable_scenes();

	NodePath get_spawn_path() const;

	uint32_t get_spawn_limit() const { return spawn_limit; }
	void set_spawn_limit(uint32_t p_limit) { spawn_limit = p_limit; }

	int find_spawnable_scene_index_from_path(const String& p_path) const;
	Node* instantiate_scene(int p_idx);

	MultiplayerSynchronizer();
};
