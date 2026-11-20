/**************************************************************************/
/*  scene_multiplayer.h                                                   */
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

#include "core/io/stream_peer.h"
#include "scene/main/multiplayer_api.h"
#include "scene_cache_interface.h"
#include "scene_replication_interface.h"
#include "scene_rpc_interface.h"

class OfflineMultiplayerPeer : public MultiplayerPeer
{
public:
	virtual int get_available_packet_count() const override { return 0; }

	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override
	{
		*r_buffer = nullptr;
		r_buffer_size = 0;
		return OK;
	}

	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override { return OK; }

	virtual int get_max_packet_size() const override { return 0; }

	virtual void set_target_peer(int p_peer_id) override {}

	virtual int get_packet_peer() const override { return 0; }

	virtual TransferMode get_packet_mode() const override { return TRANSFER_MODE_RELIABLE; }

	virtual int get_packet_channel() const override { return 0; }

	virtual void disconnect_peer(int p_peer, bool p_force = false) override {}

	virtual bool is_server() const override { return true; }

	virtual void poll() override {}

	virtual void close() override {}

	virtual int get_unique_id() const override { return TARGET_PEER_SERVER; }

	virtual ConnectionStatus get_connection_status() const override { return CONNECTION_CONNECTED; }
};

struct MultiplayerAPI::SceneData
{
	struct PendingPeer
	{
		bool local = false;
		bool remote = false;
		uint64_t time = 0;
	};

	Ref<MultiplayerPeer> multiplayer_peer;
	MultiplayerPeer::ConnectionStatus last_connection_status =
		MultiplayerPeer::CONNECTION_DISCONNECTED;
	HashMap<int, PendingPeer> pending_peers; // true if locally finalized.
	uint64_t auth_timeout = 3000;
	HashSet<int> connected_peers;
	int remote_sender_id = 0;
	int remote_sender_override = 0;

	Vector<uint8_t> packet_cache;

	NodePath root_path;
	bool allow_object_decoding = false;
	bool server_relay = true;
	Ref<StreamPeerBuffer> relay_buffer;

	Ref<SceneCacheInterface> cache;
	Ref<SceneReplicationInterface> replicator;
	Ref<SceneRPCInterface> rpc;
};
