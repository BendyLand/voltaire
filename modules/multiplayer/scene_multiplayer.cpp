/**************************************************************************/
/*  scene_multiplayer.cpp                                                 */
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

#include "scene_multiplayer.h"

#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "modules/multiplayer/scene_multiplayer.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

// SceneReplicationConfig

void SceneReplicationConfig::reset_state()
{
	dirty = false;
	properties.clear();
	sync_props.clear();
	spawn_props.clear();
	watch_props.clear();
}

void SceneReplicationConfig::add_property(const NodePath& p_path, int p_index)
{
	ERR_FAIL_COND(properties.find(p_path));
	ERR_FAIL_COND(p_path == NodePath());

	if (p_index < 0 || p_index == properties.size()) {
		properties.push_back(ReplicationProperty(p_path));
		dirty = true;
		return;
	}

	ERR_FAIL_INDEX(p_index, properties.size());

	List<ReplicationProperty>::Element* I = properties.front();
	int c = 0;
	while (c < p_index) {
		I = I->next();
		c++;
	}
	properties.insert_before(I, ReplicationProperty(p_path));
	dirty = true;
}

void SceneReplicationConfig::remove_property(const NodePath& p_path)
{
	properties.erase(p_path);
	dirty = true;
}

bool SceneReplicationConfig::has_property(const NodePath& p_path) const
{
	for (const ReplicationProperty& property : properties) {
		if (property.name == p_path) {
			return true;
		}
	}
	return false;
}

int SceneReplicationConfig::property_get_index(const NodePath& p_path) const
{
	int i = 0;
	for (List<ReplicationProperty>::ConstIterator itr = properties.begin(); itr != properties.end();
		 ++itr, ++i) {
		if (itr->name == p_path) {
			return i;
		}
	}
	ERR_FAIL_V(-1);
}

bool SceneReplicationConfig::property_get_spawn(const NodePath& p_path)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND_V(!E, false);
	return E->get().spawn;
}

void SceneReplicationConfig::property_set_spawn(const NodePath& p_path, bool p_enabled)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND(!E);
	if (E->get().spawn == p_enabled) {
		return;
	}
	E->get().spawn = p_enabled;
	dirty = true;
}

bool SceneReplicationConfig::property_get_sync(const NodePath& p_path)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND_V(!E, false);
	return E->get().mode == REPLICATION_MODE_ALWAYS;
}

void SceneReplicationConfig::property_set_sync(const NodePath& p_path, bool p_enabled)
{
	if (p_enabled) {
		property_set_replication_mode(p_path, REPLICATION_MODE_ALWAYS);
	}
	else if (property_get_replication_mode(p_path) == REPLICATION_MODE_ALWAYS) {
		property_set_replication_mode(p_path, REPLICATION_MODE_NEVER);
	}
}

bool SceneReplicationConfig::property_get_watch(const NodePath& p_path)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND_V(!E, false);
	return E->get().mode == REPLICATION_MODE_ON_CHANGE;
}

void SceneReplicationConfig::property_set_watch(const NodePath& p_path, bool p_enabled)
{
	if (p_enabled) {
		property_set_replication_mode(p_path, REPLICATION_MODE_ON_CHANGE);
	}
	else if (property_get_replication_mode(p_path) == REPLICATION_MODE_ON_CHANGE) {
		property_set_replication_mode(p_path, REPLICATION_MODE_NEVER);
	}
}

SceneReplicationConfig::ReplicationMode SceneReplicationConfig::property_get_replication_mode(
	const NodePath& p_path)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND_V(!E, REPLICATION_MODE_NEVER);
	return E->get().mode;
}

void SceneReplicationConfig::property_set_replication_mode(
	const NodePath& p_path, ReplicationMode p_mode)
{
	List<ReplicationProperty>::Element* E = properties.find(p_path);
	ERR_FAIL_COND(!E);
	if (E->get().mode == p_mode) {
		return;
	}
	E->get().mode = p_mode;
	dirty = true;
}

void SceneReplicationConfig::_update()
{
	if (!dirty) {
		return;
	}
	dirty = false;
	sync_props.clear();
	spawn_props.clear();
	watch_props.clear();
	for (const ReplicationProperty& prop : properties) {
		if (prop.spawn) {
			spawn_props.push_back(prop.name);
		}
		switch (prop.mode) {
		case REPLICATION_MODE_ALWAYS:
			sync_props.push_back(prop.name);
			break;
		case REPLICATION_MODE_ON_CHANGE:
			watch_props.push_back(prop.name);
			break;
		default:
			break;
		}
	}
}

const List<NodePath>& SceneReplicationConfig::get_spawn_properties()
{
	if (dirty) {
		_update();
	}
	return spawn_props;
}

const List<NodePath>& SceneReplicationConfig::get_sync_properties()
{
	if (dirty) {
		_update();
	}
	return sync_props;
}

const List<NodePath>& SceneReplicationConfig::get_watch_properties()
{
	if (dirty) {
		_update();
	}
	return watch_props;
}

// SceneReplicationInterface

bool SceneReplicationInterface::_has_authority(const Node* p_node)
{
	return multiplayer->has_multiplayer_peer() &&
		   p_node->get_multiplayer_authority() == multiplayer->get_unique_id();
}

Error SceneReplicationInterface::_send_raw(
	const uint8_t* p_buffer, int p_size, int p_peer, bool p_reliable)
{
	ERR_FAIL_COND_V(!p_buffer || p_size < 1, ERR_INVALID_PARAMETER);

	Ref<MultiplayerPeer> peer = multiplayer->get_multiplayer_peer();
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	peer->set_transfer_channel(0);
	peer->set_transfer_mode(p_reliable ? MultiplayerPeer::TRANSFER_MODE_RELIABLE
									   : MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);
	return multiplayer->send_command(p_peer, p_buffer, p_size);
}

void SceneReplicationInterface::set_max_sync_packet_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 128, "Sync maximum packet size must be at least 128 bytes.");
	sync_mtu = p_size;
}

int SceneReplicationInterface::get_max_sync_packet_size() const { return sync_mtu; }

void SceneReplicationInterface::set_max_delta_packet_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 128, "Sync maximum packet size must be at least 128 bytes.");
	delta_mtu = p_size;
}

int SceneReplicationInterface::get_max_delta_packet_size() const { return delta_mtu; }

// SceneRPCInterface

// The RPC meta is composed by a single byte that contains (starting from the least significant
// bit):
// - `NetworkCommands` in the first four bits.
// - `NetworkNodeIdCompression` in the next 2 bits.
// - `NetworkNameIdCompression` in the next 1 bit.
// - `byte_only_or_no_args` in the next 1 bit.
#define NODE_ID_COMPRESSION_SHIFT MultiplayerAPI::CMD_FLAG_0_SHIFT
#define NAME_ID_COMPRESSION_SHIFT MultiplayerAPI::CMD_FLAG_2_SHIFT
#define BYTE_ONLY_OR_NO_ARGS_SHIFT MultiplayerAPI::CMD_FLAG_3_SHIFT

#define NODE_ID_COMPRESSION_FLAG                                                                   \
	((1 << NODE_ID_COMPRESSION_SHIFT) | (1 << (NODE_ID_COMPRESSION_SHIFT + 1)))
#define NAME_ID_COMPRESSION_FLAG (1 << NAME_ID_COMPRESSION_SHIFT)
#define BYTE_ONLY_OR_NO_ARGS_FLAG (1 << BYTE_ONLY_OR_NO_ARGS_SHIFT)

// Returns the packet size stripping the node path added when the node is not yet cached.
static int get_packet_len(uint32_t p_node_target, int p_packet_len)
{
	if (p_node_target & 0x80000000) {
		int ofs = p_node_target & 0x7FFFFFFF;
		return p_packet_len - (p_packet_len - ofs);
	}
	else {
		return p_packet_len;
	}
}

void SceneRPCInterface::process_rpc(int p_from, const uint8_t* p_packet, int p_packet_len)
{
	// Extract packet meta
	int packet_min_size = 1;
	int name_id_offset = 1;
	ERR_FAIL_COND_MSG(p_packet_len < packet_min_size, "Invalid packet received. Size too small.");
	// Compute the meta size, which depends on the compression level.
	int node_id_compression = (p_packet[0] & NODE_ID_COMPRESSION_FLAG) >> NODE_ID_COMPRESSION_SHIFT;
	int name_id_compression = (p_packet[0] & NAME_ID_COMPRESSION_FLAG) >> NAME_ID_COMPRESSION_SHIFT;

	switch (node_id_compression) {
	case NETWORK_NODE_ID_COMPRESSION_8:
		packet_min_size += 1;
		name_id_offset += 1;
		break;
	case NETWORK_NODE_ID_COMPRESSION_16:
		packet_min_size += 2;
		name_id_offset += 2;
		break;
	case NETWORK_NODE_ID_COMPRESSION_32:
		packet_min_size += 4;
		name_id_offset += 4;
		break;
	default:
		ERR_FAIL_MSG("Was not possible to extract the node id compression mode.");
	}
	switch (name_id_compression) {
	case NETWORK_NAME_ID_COMPRESSION_8:
		packet_min_size += 1;
		break;
	case NETWORK_NAME_ID_COMPRESSION_16:
		packet_min_size += 2;
		break;
	default:
		ERR_FAIL_MSG("Was not possible to extract the name id compression mode.");
	}
	ERR_FAIL_COND_MSG(p_packet_len < packet_min_size, "Invalid packet received. Size too small.");

	uint32_t node_target = 0;
	switch (node_id_compression) {
	case NETWORK_NODE_ID_COMPRESSION_8:
		node_target = p_packet[1];
		break;
	case NETWORK_NODE_ID_COMPRESSION_16:
		node_target = decode_uint16(p_packet + 1);
		break;
	case NETWORK_NODE_ID_COMPRESSION_32:
		node_target = decode_uint32(p_packet + 1);
		break;
	default:
		// Unreachable, checked before.
		CRASH_NOW();
	}

	Node* node = _process_get_node(p_from, p_packet, node_target, p_packet_len);
	ERR_FAIL_NULL_MSG(node, "Invalid packet received. Requested node was not found.");

	uint16_t name_id = 0;
	switch (name_id_compression) {
	case NETWORK_NAME_ID_COMPRESSION_8:
		name_id = p_packet[name_id_offset];
		break;
	case NETWORK_NAME_ID_COMPRESSION_16:
		name_id = decode_uint16(p_packet + name_id_offset);
		break;
	default:
		// Unreachable, checked before.
		CRASH_NOW();
	}

	const int packet_len = get_packet_len(node_target, p_packet_len);
	_process_rpc(node, name_id, p_from, p_packet, packet_len, packet_min_size);
}

void SceneRPCInterface::_process_rpc(Node*, unsigned short, int, unsigned char const*, int, int) {}

Node* SceneRPCInterface::_process_get_node(int, unsigned char const*, unsigned int, int)
{
	return nullptr;
}

// MultiplayerDebugger

int MultiplayerDebugger::BandwidthProfiler::bandwidth_usage(
	const Vector<BandwidthFrame>& p_buffer, int p_pointer)
{
	ERR_FAIL_COND_V(p_buffer.is_empty(), 0);
	int total_bandwidth = 0;

	uint64_t timestamp = OS::get_ticks_msec();
	uint64_t final_timestamp = timestamp - 1000;

	int i = (p_pointer + p_buffer.size() - 1) % p_buffer.size();

	while (i != p_pointer && p_buffer[i].packet_size > 0) {
		if (p_buffer[i].timestamp < final_timestamp) {
			return total_bandwidth;
		}
		total_bandwidth += p_buffer[i].packet_size;
		i = (i + p_buffer.size() - 1) % p_buffer.size();
	}

	ERR_FAIL_COND_V_MSG(i == p_pointer, total_bandwidth,
		"Reached the end of the bandwidth profiler buffer, values might be inaccurate.");
	return total_bandwidth;
}

void MultiplayerDebugger::initialize() {}

void MultiplayerDebugger::deinitialize() {}

// MultiplayerAPI

void MultiplayerAPI::set_root_path(const NodePath& p_path)
{
	ERR_FAIL_COND_MSG(!p_path.is_absolute() && !p_path.is_empty(),
		"MultiplayerAPI root path must be absolute.");
	scene_data->root_path = p_path;
}

NodePath MultiplayerAPI::get_root_path() const { return scene_data->root_path; }

Ref<MultiplayerPeer> MultiplayerAPI::get_multiplayer_peer() { return scene_data->multiplayer_peer; }

int MultiplayerAPI::get_remote_sender_id() const
{
	return scene_data->remote_sender_override ? scene_data->remote_sender_override
											  : scene_data->remote_sender_id;
}

HashSet<int> MultiplayerAPI::get_connected_peers() const
{
	return HashSet<int>(scene_data->connected_peers);
}

void MultiplayerAPI::set_remote_sender_override(int p_id)
{
	scene_data->remote_sender_override = p_id;
}

Error MultiplayerAPI::_send(const uint8_t* p_packet, int p_packet_len)
{
	return scene_data->multiplayer_peer->put_packet(p_packet, p_packet_len);
}

Error MultiplayerAPI::send_command(int p_to, const uint8_t* p_packet, int p_packet_len)
{
	if (scene_data->server_relay && get_unique_id() != 1 && p_to != 1 &&
		scene_data->multiplayer_peer->is_server_relay_supported()) {
		// Send relay packet.
		scene_data->relay_buffer->seek(0);
		scene_data->relay_buffer->put_u8(NETWORK_COMMAND_SYS);
		scene_data->relay_buffer->put_u8(SYS_COMMAND_RELAY);
		scene_data->relay_buffer->put_32(p_to); // Set the destination.
		scene_data->relay_buffer->put_data(p_packet, p_packet_len);
		scene_data->multiplayer_peer->set_target_peer(1);
		const Vector<uint8_t> data = scene_data->relay_buffer->get_data_array();
		return _send(data.ptr(), scene_data->relay_buffer->get_position());
	}
	if (p_to > 0) {
		ERR_FAIL_COND_V(!scene_data->connected_peers.has(p_to), ERR_BUG);
		scene_data->multiplayer_peer->set_target_peer(p_to);
		return _send(p_packet, p_packet_len);
	}
	else {
		for (const int& pid : scene_data->connected_peers) {
			if (p_to && pid == -p_to) {
				continue;
			}
			scene_data->multiplayer_peer->set_target_peer(pid);
			_send(p_packet, p_packet_len);
		}
		return OK;
	}
}

void MultiplayerAPI::_process_sys(int p_from, const uint8_t* p_packet, int p_packet_len,
	MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	ERR_FAIL_COND_MSG(p_packet_len < SYS_CMD_SIZE, "Invalid packet received. Size too small.");
	uint8_t sys_cmd_type = p_packet[1];
	int32_t peer = int32_t(decode_uint32(&p_packet[2]));
	switch (sys_cmd_type) {
	case SYS_COMMAND_ADD_PEER: {
		ERR_FAIL_COND(!scene_data->server_relay ||
					  !scene_data->multiplayer_peer->is_server_relay_supported() ||
					  get_unique_id() == 1 || p_from != 1);
		_admit_peer(peer); // Relayed peers are automatically accepted.
	} break;
	case SYS_COMMAND_DEL_PEER: {
		ERR_FAIL_COND(!scene_data->server_relay ||
					  !scene_data->multiplayer_peer->is_server_relay_supported() ||
					  get_unique_id() == 1 || p_from != 1);
		_del_peer(peer);
	} break;
	case SYS_COMMAND_RELAY: {
		ERR_FAIL_COND(!scene_data->server_relay ||
					  !scene_data->multiplayer_peer->is_server_relay_supported());
		ERR_FAIL_COND(p_packet_len < SYS_CMD_SIZE + 1);
		const uint8_t* packet = p_packet + SYS_CMD_SIZE;
		int len = p_packet_len - SYS_CMD_SIZE;
		bool should_process = false;
		if (get_unique_id() == 1) { // I am the server.
			if (unlikely(peer > 0 && !scene_data->connected_peers.has(peer))) {
				return;
			}
			// Send relay packet.
			scene_data->relay_buffer->seek(0);
			scene_data->relay_buffer->put_u8(NETWORK_COMMAND_SYS);
			scene_data->relay_buffer->put_u8(SYS_COMMAND_RELAY);
			scene_data->relay_buffer->put_32(p_from); // Set the source.
			scene_data->relay_buffer->put_data(packet, len);
			const Vector<uint8_t> data = scene_data->relay_buffer->get_data_array();
			scene_data->multiplayer_peer->set_transfer_mode(p_mode);
			scene_data->multiplayer_peer->set_transfer_channel(p_channel);
			if (peer > 0) {
				scene_data->multiplayer_peer->set_target_peer(peer);
				_send(data.ptr(), scene_data->relay_buffer->get_position());
			}
			else {
				for (const int& P : scene_data->connected_peers) {
					if (P == p_from || P == -peer) {
						continue;
					}
					scene_data->multiplayer_peer->set_target_peer(P);
					_send(data.ptr(), scene_data->relay_buffer->get_position());
				}
				if (peer != -1) {
					should_process = true;
					peer = p_from;
				}
			}
		}
		else {
			ERR_FAIL_COND(p_from != 1); // Bug.
			should_process = true;
		}
		if (should_process) {
			scene_data->remote_sender_id = peer;
			_process_packet(peer, packet, len);
			scene_data->remote_sender_id = 0;
		}
	} break;
	default: {
		ERR_FAIL();
	}
	}
}

Error MultiplayerAPI::send_bytes(
	Vector<uint8_t> p_data, int p_to, MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	ERR_FAIL_COND_V_MSG(p_data.is_empty(), ERR_INVALID_DATA, "Trying to send an empty raw packet.");
	ERR_FAIL_COND_V_MSG(scene_data->multiplayer_peer.is_null(), ERR_UNCONFIGURED,
		"Trying to send a raw packet while no multiplayer peer is active.");
	ERR_FAIL_COND_V_MSG(
		scene_data->multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED,
		"Trying to send a raw packet via a multiplayer peer which is not connected.");

	if (scene_data->packet_cache.size() < p_data.size() + 1) {
		scene_data->packet_cache.resize(p_data.size() + 1);
	}

	const uint8_t* r = p_data.ptr();
	scene_data->packet_cache.write[0] = NETWORK_COMMAND_RAW;
	memcpy(&scene_data->packet_cache.write[1], &r[0], p_data.size());

	scene_data->multiplayer_peer->set_transfer_channel(p_channel);
	scene_data->multiplayer_peer->set_transfer_mode(p_mode);
	return send_command(p_to, scene_data->packet_cache.ptr(), p_data.size() + 1);
}

Error MultiplayerAPI::send_auth(int p_to, Vector<uint8_t> p_data)
{
	ERR_FAIL_COND_V(scene_data->multiplayer_peer.is_null() ||
						scene_data->multiplayer_peer->get_connection_status() !=
							MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!scene_data->pending_peers.has(p_to), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_data.is_empty(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(scene_data->pending_peers[p_to].local, ERR_FILE_CANT_WRITE,
		"The authentication session was previously marked as completed, no more authentication "
		"data can be sent.");
	ERR_FAIL_COND_V_MSG(scene_data->pending_peers[p_to].remote, ERR_FILE_CANT_WRITE,
		"The remote peer notified that the authentication session was completed, no more "
		"authentication data can be sent.");

	if (scene_data->packet_cache.size() < p_data.size() + 2) {
		scene_data->packet_cache.resize(p_data.size() + 2);
	}

	scene_data->packet_cache.write[0] = NETWORK_COMMAND_SYS;
	scene_data->packet_cache.write[1] = SYS_COMMAND_AUTH;
	memcpy(&scene_data->packet_cache.write[2], p_data.ptr(), p_data.size());

	scene_data->multiplayer_peer->set_target_peer(p_to);
	scene_data->multiplayer_peer->set_transfer_channel(0);
	scene_data->multiplayer_peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	return _send(scene_data->packet_cache.ptr(), p_data.size() + 2);
}

Error MultiplayerAPI::complete_auth(int p_peer)
{
	ERR_FAIL_COND_V(scene_data->multiplayer_peer.is_null() ||
						scene_data->multiplayer_peer->get_connection_status() !=
							MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!scene_data->pending_peers.has(p_peer), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(scene_data->pending_peers[p_peer].local, ERR_FILE_CANT_WRITE,
		"The authentication session was already marked as completed.");
	scene_data->pending_peers[p_peer].local = true;

	// Notify the remote peer that the authentication has completed.
	uint8_t buf[2] = {NETWORK_COMMAND_SYS, SYS_COMMAND_AUTH};
	scene_data->multiplayer_peer->set_target_peer(p_peer);
	scene_data->multiplayer_peer->set_transfer_channel(0);
	scene_data->multiplayer_peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	Error err = _send(buf, 2);

	// The remote peer already reported the authentication as completed, so admit the peer.
	// May generate new packets, so it must happen after sending confirmation.
	if (scene_data->pending_peers[p_peer].remote) {
		scene_data->pending_peers.erase(p_peer);
		_admit_peer(p_peer);
	}
	return err;
}

void MultiplayerAPI::set_auth_timeout(double p_timeout)
{
	ERR_FAIL_COND_MSG(
		p_timeout < 0, "Timeout must be greater or equal to 0 (where 0 means no timeout)");
	scene_data->auth_timeout = uint64_t(p_timeout * 1000);
}

double MultiplayerAPI::get_auth_timeout() const { return double(scene_data->auth_timeout) / 1000.0; }

int MultiplayerAPI::get_unique_id()
{
	ERR_FAIL_COND_V_MSG(scene_data->multiplayer_peer.is_null(), 0,
		"No multiplayer peer is assigned. Unable to get unique ID.");
	return scene_data->multiplayer_peer->get_unique_id();
}

void MultiplayerAPI::disconnect_peer(int p_id)
{
	ERR_FAIL_COND_MSG(scene_data->multiplayer_peer.is_null() ||
						  scene_data->multiplayer_peer->get_connection_status() !=
							  MultiplayerPeer::CONNECTION_CONNECTED,
		"MultiplayerPeer is not connected.");
	if (scene_data->pending_peers.has(p_id)) {
		scene_data->pending_peers.erase(p_id);
	}
	else if (scene_data->connected_peers.has(p_id)) {
		_del_peer(p_id);
	}
	scene_data->multiplayer_peer->disconnect_peer(p_id);
}

void MultiplayerAPI::set_refuse_new_connections(bool p_refuse)
{
	ERR_FAIL_COND_MSG(scene_data->multiplayer_peer.is_null(),
		"No multiplayer peer is assigned. Unable to set 'refuse_new_connections'.");
	scene_data->multiplayer_peer->set_refuse_new_connections(p_refuse);
}

bool MultiplayerAPI::is_refusing_new_connections() const
{
	ERR_FAIL_COND_V_MSG(scene_data->multiplayer_peer.is_null(), false,
		"No multiplayer peer is assigned. Unable to get 'refuse_new_connections'.");
	return scene_data->multiplayer_peer->is_refusing_new_connections();
}

Vector<int> MultiplayerAPI::get_peer_ids()
{
	ERR_FAIL_COND_V_MSG(scene_data->multiplayer_peer.is_null(), Vector<int>(),
		"No multiplayer peer is assigned. Assume no peers are connected.");

	Vector<int> ret;
	for (const int& E : scene_data->connected_peers) {
		ret.push_back(E);
	}

	return ret;
}

Vector<int> MultiplayerAPI::get_authenticating_peer_ids()
{
	Vector<int> out;
	out.resize(scene_data->pending_peers.size());
	int idx = 0;
	for (const KeyValue<int, SceneData::PendingPeer>& E : scene_data->pending_peers) {
		out.write[idx++] = E.key;
	}
	return out;
}

void MultiplayerAPI::set_allow_object_decoding(bool p_enable)
{
	scene_data->allow_object_decoding = p_enable;
}

bool MultiplayerAPI::is_object_decoding_allowed() const
{
	return scene_data->allow_object_decoding;
}

void MultiplayerAPI::set_server_relay_enabled(bool p_enabled)
{
	scene_data->server_relay = p_enabled;
}

bool MultiplayerAPI::is_server_relay_enabled() const { return scene_data->server_relay; }

void MultiplayerAPI::set_max_sync_packet_size(int p_size)
{
	scene_data->replicator->set_max_sync_packet_size(p_size);
}

int MultiplayerAPI::get_max_sync_packet_size() const
{
	return scene_data->replicator->get_max_sync_packet_size();
}

void MultiplayerAPI::set_max_delta_packet_size(int p_size)
{
	scene_data->replicator->set_max_delta_packet_size(p_size);
}

int MultiplayerAPI::get_max_delta_packet_size() const
{
	return scene_data->replicator->get_max_delta_packet_size();
}

void MultiplayerAPI::set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer)
{
	if (p_peer == scene_data->multiplayer_peer) {
		return;
	}
	ERR_FAIL_COND_MSG(p_peer.is_valid() && p_peer->get_connection_status() ==
											   MultiplayerPeer::CONNECTION_DISCONNECTED,
		"Supplied MultiplayerPeer must be connecting or connected.");
	clear();
	scene_data->multiplayer_peer = p_peer;
}

MultiplayerAPI::MultiplayerAPI()
{
	scene_data = memnew(SceneData);
	scene_data->relay_buffer.instantiate();
	scene_data->cache.instantiate(this);
	scene_data->replicator.instantiate(this, scene_data->cache.ptr());
	scene_data->rpc.instantiate(this, scene_data->cache.ptr(), scene_data->replicator.ptr());
	set_multiplayer_peer(Ref<OfflineMultiplayerPeer>(memnew(OfflineMultiplayerPeer)));
}

MultiplayerAPI::~MultiplayerAPI()
{
	clear();
	scene_data->rpc.unref();
	scene_data->replicator.unref();
	scene_data->cache.unref();
	memdelete(scene_data);
	scene_data = nullptr;
}

void MultiplayerAPI::_add_peer(int) {}

void MultiplayerAPI::_admit_peer(int) {}

void MultiplayerAPI::_process_packet(int, unsigned char const*, int) {}

void MultiplayerAPI::_process_raw(int, unsigned char const*, int) {}

void MultiplayerAPI::_del_peer(int) {}

void MultiplayerAPI::_update_status() {}

void MultiplayerAPI::clear()
{
	scene_data->pending_peers.clear();
	scene_data->connected_peers.clear();
	scene_data->packet_cache.clear();
	scene_data->relay_buffer->clear();
}

Error MultiplayerAPI::poll() { return OK; }

// ============================================================================
// Merged from modules/multiplayer/register_types.cpp
// ============================================================================

#include "register_types.h"

#include "modules/multiplayer/scene_multiplayer.h"
#include "scene_multiplayer.h"

void initialize_multiplayer_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		MultiplayerAPI::set_default_interface("MultiplayerAPI");
		MultiplayerDebugger::initialize();
	}
}

void uninitialize_multiplayer_module(ModuleInitializationLevel p_level)
{
	MultiplayerDebugger::deinitialize();
}

// ============================================================================
// Merged from modules/multiplayer/multiplayer_synchronizer.cpp
// ============================================================================

#include "modules/multiplayer/scene_multiplayer.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"

void MultiplayerSynchronizer::reset()
{
	net_id = 0;
	last_sync_usec = 0;
	last_inbound_sync = 0;
	last_watch_usec = 0;
	sync_started = false;
	watchers.clear();
}

uint32_t MultiplayerSynchronizer::get_net_id() const { return net_id; }

void MultiplayerSynchronizer::set_net_id(uint32_t p_net_id) { net_id = p_net_id; }

bool MultiplayerSynchronizer::update_outbound_sync_time(uint64_t p_usec)
{
	if (last_sync_usec == p_usec) {
		// last_sync_usec has been updated in this frame.
		return true;
	}
	if (p_usec < last_sync_usec + sync_interval_usec) {
		// Too soon, should skip this synchronization frame.
		return false;
	}
	last_sync_usec = p_usec;
	return true;
}

bool MultiplayerSynchronizer::update_inbound_sync_time(uint16_t p_network_time)
{
	if (!sync_started) {
		sync_started = true;
	}
	else if (p_network_time <= last_inbound_sync && last_inbound_sync - p_network_time < 32767) {
		return false;
	}
	last_inbound_sync = p_network_time;
	return true;
}

PackedStringArray MultiplayerSynchronizer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (mode == MODE_SPAWNER) {
		if (spawn_path.is_empty() || !has_node(spawn_path)) {
			warnings.push_back(RTR("A valid NodePath must be set in the \"Spawn Path\" property in "
								   "order for MultiplayerSpawner to be able to spawn Nodes."));
		}
	}
	else {
		if (root_path.is_empty() || !has_node(root_path)) {
			warnings.push_back(
				RTR("A valid NodePath must be set in the \"Root Path\" property in order for "
					"MultiplayerSynchronizer to be able to synchronize properties."));
		}
	}

	return warnings;
}

bool MultiplayerSynchronizer::is_visibility_public() const { return peer_visibility.has(0); }

void MultiplayerSynchronizer::set_visibility_public(bool p_visible)
{
	set_visibility_for(0, p_visible);
}

void MultiplayerSynchronizer::set_visibility_for(int p_peer, bool p_visible)
{
	if (peer_visibility.has(p_peer) == p_visible) {
		return;
	}
	if (p_visible) {
		peer_visibility.insert(p_peer);
	}
	else {
		peer_visibility.erase(p_peer);
	}
	update_visibility(p_peer);
}

bool MultiplayerSynchronizer::get_visibility_for(int p_peer) const
{
	return peer_visibility.has(p_peer);
}

void MultiplayerSynchronizer::set_visibility_update_mode(VisibilityUpdateMode p_mode)
{
	visibility_update_mode = p_mode;
	_update_process();
}

MultiplayerSynchronizer::VisibilityUpdateMode
MultiplayerSynchronizer::get_visibility_update_mode() const
{
	return visibility_update_mode;
}

void MultiplayerSynchronizer::_notification(int p_what)
{
#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint()) {
		return;
	}
#endif
	if (mode == MODE_SPAWNER || root_path.is_empty()) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_start();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_stop();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS:
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		update_visibility(0);
	} break;
	}
}

void MultiplayerSynchronizer::set_replication_interval(double p_interval)
{
	ERR_FAIL_COND_MSG(
		p_interval < 0, "Interval must be greater or equal to 0 (where 0 means default)");
	sync_interval_usec = uint64_t(p_interval * 1000 * 1000);
}

double MultiplayerSynchronizer::get_replication_interval() const
{
	return double(sync_interval_usec) / 1000.0 / 1000.0;
}

void MultiplayerSynchronizer::set_delta_interval(double p_interval)
{
	ERR_FAIL_COND_MSG(
		p_interval < 0, "Interval must be greater or equal to 0 (where 0 means default)");
	delta_interval_usec = uint64_t(p_interval * 1000 * 1000);
}

double MultiplayerSynchronizer::get_delta_interval() const
{
	return double(delta_interval_usec) / 1000.0 / 1000.0;
}

void MultiplayerSynchronizer::set_replication_config(Ref<SceneReplicationConfig> p_config)
{
	replication_config = p_config;
}

Ref<SceneReplicationConfig> MultiplayerSynchronizer::get_replication_config()
{
	return replication_config;
}

NodePath MultiplayerSynchronizer::get_root_path() const { return root_path; }

void MultiplayerSynchronizer::set_multiplayer_authority(int p_peer_id, bool p_recursive)
{
	if (get_multiplayer_authority() == p_peer_id) {
		return;
	}
	_stop();
	Node::set_multiplayer_authority(p_peer_id, p_recursive);
	_start();
}

List<NodePath> MultiplayerSynchronizer::get_delta_properties(uint64_t p_indexes)
{
	List<NodePath> out;
	ERR_FAIL_COND_V(replication_config.is_null(), out);
	const List<NodePath> watch_props(replication_config->get_watch_properties());
	int idx = 0;
	for (const NodePath& prop : watch_props) {
		if ((p_indexes & (1ULL << idx++)) == 0) {
			continue;
		}
		out.push_back(prop);
	}
	return out;
}

SceneReplicationConfig* MultiplayerSynchronizer::get_replication_config_ptr() const
{
	return replication_config.ptr();
}

int MultiplayerSynchronizer::get_spawnable_scene_count() const { return spawnable_scenes.size(); }

String MultiplayerSynchronizer::get_spawnable_scene(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, (int)spawnable_scenes.size(), "");
	return spawnable_scenes[p_idx].path;
}

Vector<String> MultiplayerSynchronizer::_get_spawnable_scenes() const
{
	Vector<String> ss;
	ss.resize(spawnable_scenes.size());
	for (int i = 0; i < ss.size(); i++) {
		ss.write[i] = ResourceUID::path_to_uid(spawnable_scenes[i].path);
	}
	return ss;
}

void MultiplayerSynchronizer::_set_spawnable_scenes(const Vector<String>& p_scenes)
{
	clear_spawnable_scenes();
	for (int i = 0; i < p_scenes.size(); i++) {
		add_spawnable_scene(p_scenes[i]);
	}
}

NodePath MultiplayerSynchronizer::get_spawn_path() const { return spawn_path; }

int MultiplayerSynchronizer::find_spawnable_scene_index_from_path(const String& p_scene) const
{
	for (uint32_t i = 0; i < spawnable_scenes.size(); i++) {
		if (spawnable_scenes[i].path == p_scene) {
			return i;
		}
	}
	return INVALID_ID;
}

void MultiplayerSynchronizer::clear_spawnable_scenes() {}

void MultiplayerSynchronizer::add_spawnable_scene(const String&) {}

MultiplayerSynchronizer::MultiplayerSynchronizer()
{
	// Publicly visible by default.
	peer_visibility.insert(0);
}

void MultiplayerSynchronizer::update_visibility(int) {}

void MultiplayerSynchronizer::_stop() {}

void MultiplayerSynchronizer::_start() {}

void MultiplayerSynchronizer::_update_process() {}
