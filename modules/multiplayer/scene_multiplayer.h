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

#include "core/io/resource.h"
#include "core/io/stream_peer.h"
#include "core/templates/rb_set.h"
#include "scene/main/multiplayer_api.h"

class Node;
class MultiplayerSynchronizer;

class OfflineMultiplayerPeer final : public MultiplayerPeer
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

class SceneReplicationConfig final : public Resource
{
public:
	enum ReplicationMode
	{
		REPLICATION_MODE_NEVER,
		REPLICATION_MODE_ALWAYS,
		REPLICATION_MODE_ON_CHANGE,
	};

private:
	struct ReplicationProperty
	{
		NodePath name;
		bool spawn = true;
		ReplicationMode mode = REPLICATION_MODE_ALWAYS;

		bool operator==(const ReplicationProperty& p_to) { return name == p_to.name; }

		ReplicationProperty() {}

		ReplicationProperty(const NodePath& p_name) { name = p_name; }
	};

	List<ReplicationProperty> properties;
	List<NodePath> spawn_props;
	List<NodePath> sync_props;
	List<NodePath> watch_props;
	bool dirty = false;

	void _update();

public:
	virtual void reset_state() override; // Required since we use variable amount of properties.

	void add_property(const NodePath& p_path, int p_index = -1);
	void remove_property(const NodePath& p_path);
	bool has_property(const NodePath& p_path) const;

	int property_get_index(const NodePath& p_path) const;
	bool property_get_spawn(const NodePath& p_path);
	void property_set_spawn(const NodePath& p_path, bool p_enabled);

	bool property_get_sync(const NodePath& p_path);
	void property_set_sync(const NodePath& p_path, bool p_enabled);

	bool property_get_watch(const NodePath& p_path);
	void property_set_watch(const NodePath& p_path, bool p_enabled);

	ReplicationMode property_get_replication_mode(const NodePath& p_path);
	void property_set_replication_mode(const NodePath& p_path, ReplicationMode p_mode);

	const List<NodePath>& get_spawn_properties();
	const List<NodePath>& get_sync_properties();
	const List<NodePath>& get_watch_properties();

	SceneReplicationConfig() {}
};

class SceneCacheInterface final : public RefCounted
{
private:
	MultiplayerAPI* multiplayer = nullptr;

	// path sent caches
	struct NodeCache
	{
		int cache_id = 0;
		HashMap<int, int> recv_ids;			// peer id, remote cache id
		HashMap<int, bool> confirmed_peers; // peer id, confirmed
	};

	struct RecvNode
	{
		NodePath path;
	};

	struct PeerInfo
	{
		HashMap<int, RecvNode> recv_nodes; // remote cache id, (ObjectID, NodePath)
	};

	HashMap<int, PeerInfo> peers_info;
	int last_send_cache_id = 1;

	NodeCache& _track(Node* p_node);

public:
	SceneCacheInterface(MultiplayerAPI* p_multiplayer) { multiplayer = p_multiplayer; }
};

class SceneReplicationInterface final : public RefCounted
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
	MultiplayerAPI* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	PackedByteArray packet_cache;
	int sync_mtu = 1350; // Highly dependent on underlying protocol.
	int delta_mtu = 65535;

	bool _has_authority(const Node* p_node);
	bool _verify_synchronizer(int p_peer, MultiplayerSynchronizer* p_sync, uint32_t& r_net_id);
	MultiplayerSynchronizer* _find_synchronizer(int p_peer, uint32_t p_net_ida);

	Error _make_spawn_packet(Node* p_node, MultiplayerSynchronizer* p_spawner, int& r_len);
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

	SceneReplicationInterface(MultiplayerAPI* p_multiplayer, SceneCacheInterface* p_cache)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
	}
};

class SceneRPCInterface final : public RefCounted
{
private:
	struct RPCConfig
	{
		String name;
		MultiplayerAPI::RPCMode rpc_mode = MultiplayerAPI::RPC_MODE_DISABLED;
		bool call_local = false;
		MultiplayerPeer::TransferMode transfer_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
		int channel = 0;

		bool operator==(const RPCConfig& p_other) const { return name == p_other.name; }
	};

	struct RPCConfigCache
	{
		HashMap<uint16_t, RPCConfig> configs;
		HashMap<String, uint16_t> ids;
	};

	struct SortRPCConfig
	{
		bool operator()(const RPCConfig& p_a, const RPCConfig& p_b) const
		{
			return p_a.name < p_b.name;
		}
	};

	enum NetworkNodeIdCompression
	{
		NETWORK_NODE_ID_COMPRESSION_8 = 0,
		NETWORK_NODE_ID_COMPRESSION_16,
		NETWORK_NODE_ID_COMPRESSION_32,
	};

	enum NetworkNameIdCompression
	{
		NETWORK_NAME_ID_COMPRESSION_8 = 0,
		NETWORK_NAME_ID_COMPRESSION_16,
	};

	MultiplayerAPI* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	SceneReplicationInterface* multiplayer_replicator = nullptr;

	Vector<uint8_t> packet_cache;

protected:
	void _process_rpc(Node* p_node, const uint16_t p_rpc_method_id, int p_from,
		const uint8_t* p_packet, int p_packet_len, int p_offset);

	Node* _process_get_node(
		int p_from, const uint8_t* p_packet, uint32_t p_node_target, int p_packet_len);

	const RPCConfigCache& _get_node_config(const Node* p_node);

public:
	void process_rpc(int p_from, const uint8_t* p_packet, int p_packet_len);

	SceneRPCInterface(MultiplayerAPI* p_multiplayer, SceneCacheInterface* p_cache,
		SceneReplicationInterface* p_replicator)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
		multiplayer_replicator = p_replicator;
	}
};

class MultiplayerDebugger final
{
public:
	struct RPCNodeInfo
	{
		String node_path;
		int incoming_rpc = 0;
		int incoming_size = 0;
		int outgoing_rpc = 0;
		int outgoing_size = 0;
	};

	struct RPCFrame
	{
		Vector<RPCNodeInfo> infos;
	};

	struct SyncInfo
	{
		int incoming_syncs = 0;
		int incoming_size = 0;
		int outgoing_syncs = 0;
		int outgoing_size = 0;

		SyncInfo() {}

		SyncInfo(MultiplayerSynchronizer* p_sync);
	};

private:
	class BandwidthProfiler
	{
	protected:
		struct BandwidthFrame
		{
			uint32_t timestamp;
			int packet_size;
		};

		int bandwidth_in_ptr = 0;
		Vector<BandwidthFrame> bandwidth_in;
		int bandwidth_out_ptr = 0;
		Vector<BandwidthFrame> bandwidth_out;
		uint64_t last_bandwidth_time = 0;

		int bandwidth_usage(const Vector<BandwidthFrame>& p_buffer, int p_pointer);
	};

	class RPCProfiler
	{
	private:
		uint64_t last_profile_time = 0;
	};

	class ReplicationProfiler
	{
	private:
		uint64_t last_profile_time = 0;
	};

public:
	static void initialize();
	static void deinitialize();
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
