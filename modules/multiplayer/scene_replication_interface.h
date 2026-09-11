/**************************************************************************/
/*  scene_replication_interface.h                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/templates/rb_set.h"
#include "multiplayer_spawner.h"
#include "multiplayer_synchronizer.h"

class SceneMultiplayer;
class SceneCacheInterface;

class SceneReplicationInterface : public RefCounted
{
private:
	struct TrackedNode
	{
		uint32_t net_id = 0;
		uint32_t remote_peer = 0;

		TrackedNode() {}
	};

	// Replication state.
	uint32_t last_net_id = 0;

	int pending_spawn_remote = 0;
	const uint8_t* pending_buffer = nullptr;
	int pending_buffer_size = 0;
	List<uint32_t> pending_sync_net_ids;

	// Replicator config.
	SceneMultiplayer* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	PackedByteArray packet_cache;
	int sync_mtu = 1350; // Highly dependent on underlying protocol.
	int delta_mtu = 65535;

	bool _has_authority(const Node* p_node);
	bool _verify_synchronizer(int p_peer, MultiplayerSynchronizer* p_sync, uint32_t& r_net_id);
	MultiplayerSynchronizer* _find_synchronizer(int p_peer, uint32_t p_net_ida);

	Error _make_spawn_packet(Node* p_node, MultiplayerSpawner* p_spawner, int& r_len);
	Error _make_despawn_packet(Node* p_node, int& r_len);
	Error _send_raw(const uint8_t* p_buffer, int p_size, int p_peer, bool p_reliable);

	Error _update_sync_visibility(int p_peer, MultiplayerSynchronizer* p_sync);

public:
	static void make_default();

	void on_reset();
	void on_peer_change(int p_id, bool p_connected);

	void on_network_process();

	Error on_spawn_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_despawn_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_sync_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_delta_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);

	void set_max_sync_packet_size(int p_size);
	int get_max_sync_packet_size() const;

	void set_max_delta_packet_size(int p_size);
	int get_max_delta_packet_size() const;

	SceneReplicationInterface(SceneMultiplayer* p_multiplayer, SceneCacheInterface* p_cache)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
	}
};


