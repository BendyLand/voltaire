extern "C" {
#include <mbedtls/psa_util.h>
}
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_MULTIPLAYER_ENABLED)
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

#include "modules/register_module_types.h"

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
#endif // defined(MODULE_MULTIPLAYER_ENABLED)


// --- Merged from modules/enet/enet_connection.cpp ---
#if defined(MODULE_ENET_ENABLED)
// --- Inlined from modules/enet/enet_connection.h ---
#include <enet/enet.h>
#include "core/crypto/crypto.h"
#include "core/io/packet_peer.h"
#include "scene/main/scene_tree.h"

template <typename T> class TypedArray;

class ENetConnection;

class ENetPacketPeer final : public PacketPeer
{
private:
	ENetPeer* peer = nullptr;
	List<ENetPacket*> packet_queue;
	ENetPacket* last_packet = nullptr;

	Error _send(int p_channel, PackedByteArray p_packet, int p_flags);

	friend class ENetConnection;
	// Internally used by ENetConnection during service, destroy, etc.
	void _on_disconnect();
	void _queue_packet(ENetPacket* p_packet);

public:
	enum
	{
		PACKET_THROTTLE_SCALE = ENET_PEER_PACKET_THROTTLE_SCALE,
		PACKET_LOSS_SCALE = ENET_PEER_PACKET_LOSS_SCALE,
	};

	enum
	{
		FLAG_RELIABLE = ENET_PACKET_FLAG_RELIABLE,
		FLAG_UNSEQUENCED = ENET_PACKET_FLAG_UNSEQUENCED,
		FLAG_UNRELIABLE_FRAGMENT = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
		FLAG_ALLOWED = ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNSEQUENCED |
					   ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
	};

	enum PeerState
	{
		STATE_DISCONNECTED = ENET_PEER_STATE_DISCONNECTED,
		STATE_CONNECTING = ENET_PEER_STATE_CONNECTING,
		STATE_ACKNOWLEDGING_CONNECT = ENET_PEER_STATE_ACKNOWLEDGING_CONNECT,
		STATE_CONNECTION_PENDING = ENET_PEER_STATE_CONNECTION_PENDING,
		STATE_CONNECTION_SUCCEEDED = ENET_PEER_STATE_CONNECTION_SUCCEEDED,
		STATE_CONNECTED = ENET_PEER_STATE_CONNECTED,
		STATE_DISCONNECT_LATER = ENET_PEER_STATE_DISCONNECT_LATER,
		STATE_DISCONNECTING = ENET_PEER_STATE_DISCONNECTING,
		STATE_ACKNOWLEDGING_DISCONNECT = ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT,
		STATE_ZOMBIE = ENET_PEER_STATE_ZOMBIE,
	};

	enum PeerStatistic
	{
		PEER_PACKET_LOSS,
		PEER_PACKET_LOSS_VARIANCE,
		PEER_PACKET_LOSS_EPOCH,
		PEER_ROUND_TRIP_TIME,
		PEER_ROUND_TRIP_TIME_VARIANCE,
		PEER_LAST_ROUND_TRIP_TIME,
		PEER_LAST_ROUND_TRIP_TIME_VARIANCE,
		PEER_PACKET_THROTTLE,
		PEER_PACKET_THROTTLE_LIMIT,
		PEER_PACKET_THROTTLE_COUNTER,
		PEER_PACKET_THROTTLE_EPOCH,
		PEER_PACKET_THROTTLE_ACCELERATION,
		PEER_PACKET_THROTTLE_DECELERATION,
		PEER_PACKET_THROTTLE_INTERVAL,
	};

	int get_max_packet_size() const override;
	int get_available_packet_count() const override;
	Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	void peer_disconnect(int p_data = 0);
	void peer_disconnect_later(int p_data = 0);
	void peer_disconnect_now(int p_data = 0);

	void ping();
	void ping_interval(int p_interval);
	void reset();
	int send(uint8_t p_channel, ENetPacket* p_packet);
	void throttle_configure(int interval, int acceleration, int deceleration);
	void set_timeout(int p_timeout, int p_timeout_min, int p_timeout_max);
	double get_statistic(PeerStatistic p_stat);
	PeerState get_state() const;
	int get_channels() const;
	int get_packet_flags() const;

	// Extras
	int get_remote_port() const;

	// Used by ENetMultiplayer (TODO use meta? If only they where Strings)
	bool is_active() const;

	ENetPacketPeer(ENetPeer* p_peer);
	~ENetPacketPeer();
};

class ENetConnection final : public RefCounted
{
public:
	enum CompressionMode
	{
		COMPRESS_NONE = 0,
		COMPRESS_RANGE_CODER,
		COMPRESS_FASTLZ,
		COMPRESS_ZLIB,
		COMPRESS_ZSTD,
	};

	enum HostStatistic
	{
		HOST_TOTAL_SENT_DATA,
		HOST_TOTAL_SENT_PACKETS,
		HOST_TOTAL_RECEIVED_DATA,
		HOST_TOTAL_RECEIVED_PACKETS,
	};

	enum EventType
	{
		EVENT_ERROR = -1,
		EVENT_NONE = 0,
		EVENT_CONNECT,
		EVENT_DISCONNECT,
		EVENT_RECEIVE,
	};

	struct Event
	{
		Ref<ENetPacketPeer> peer;
		enet_uint8 channel_id = 0;
		enet_uint32 data = 0;
		ENetPacket* packet = nullptr;
	};

private:
	ENetHost* host = nullptr;
	List<Ref<ENetPacketPeer>> peers;

	EventType _parse_event(const ENetEvent& p_event, Event& r_event);
	Error _create(ENetAddress* p_address, int p_max_peers, int p_max_channels, int p_in_bandwidth,
		int p_out_bandwidth);
	void _broadcast(int p_channel, PackedByteArray p_packet, int p_flags);

	class Compressor final
	{
	private:
		CompressionMode mode = COMPRESS_NONE;
		Vector<uint8_t> src_mem;
		Vector<uint8_t> dst_mem;
		ENetCompressor enet_compressor;

		Compressor(CompressionMode mode);

		static size_t enet_compress(void* context, const ENetBuffer* inBuffers,
			size_t inBufferCount, size_t inLimit, enet_uint8* outData, size_t outLimit);
		static size_t enet_decompress(void* context, const enet_uint8* inData, size_t inLimit,
			enet_uint8* outData, size_t outLimit);

		static void enet_compressor_destroy(void* context) { memdelete((Compressor*)context); }

	public:
		static void setup(ENetHost* p_host, CompressionMode p_mode);
	};

public:
	void broadcast(enet_uint8 p_channel, ENetPacket* p_packet);
	void socket_send(const String& p_address, int p_port, const PackedByteArray& p_packet);
	Error create_host(int p_max_peers = 32, int p_max_channels = 0, int p_in_bandwidth = 0,
		int p_out_bandwidth = 0);
	void destroy();
	Ref<ENetPacketPeer> connect_to_host(
		const String& p_address, int p_port, int p_channels, int p_data = 0);
	EventType service(int p_timeout, Event& r_event);
	int check_events(EventType& r_type, Event& r_event);
	void flush();
	void bandwidth_limit(int p_in_bandwidth = 0, int p_out_bandwidth = 0);
	void channel_limit(int p_max_channels);
	void bandwidth_throttle();
	void compress(CompressionMode p_mode);
	double pop_statistic(HostStatistic p_stat);
	int get_max_channels() const;

	// Extras
	void get_peers(List<Ref<ENetPacketPeer>>& r_peers);
	int get_local_port() const;

	// Godot additions
	Error dtls_server_setup(const Ref<TLSOptions>& p_options);
	Error dtls_client_setup(const String& p_hostname, const Ref<TLSOptions>& p_options);
	void refuse_new_connections(bool p_refuse);

	ENetConnection() {}
	~ENetConnection();
};

class ENetMultiplayerPeer final : public MultiplayerPeer
{
private:
	enum
	{
		SYSMSG_ADD_PEER,
		SYSMSG_REMOVE_PEER
	};

	enum
	{
		SYSCH_RELIABLE = 0,
		SYSCH_UNRELIABLE = 1,
		SYSCH_MAX = 2
	};

	enum Mode
	{
		MODE_NONE,
		MODE_SERVER,
		MODE_CLIENT,
		MODE_MESH,
	};

	Mode active_mode = MODE_NONE;

	uint32_t unique_id = 0;

	int target_peer = 0;

	ConnectionStatus connection_status = CONNECTION_DISCONNECTED;

	HashMap<int, Ref<ENetConnection>> hosts;
	HashMap<int, Ref<ENetPacketPeer>> peers;

	struct Packet
	{
		ENetPacket* packet = nullptr;
		int from = 0;
		int channel = 0;
		TransferMode transfer_mode = TRANSFER_MODE_RELIABLE;
	};

	List<Packet> incoming_packets;

	Packet current_packet;

	void _store_packet(int32_t p_source, ENetConnection::Event& p_event);
	void _pop_current_packet();
	void _disconnect_inactive_peers();
	void _destroy_unused(ENetPacket* p_packet);

	_FORCE_INLINE_ bool _is_active() const { return active_mode != MODE_NONE; }

public:
	virtual void set_target_peer(int p_peer) override;

	virtual int get_packet_peer() const override;
	virtual TransferMode get_packet_mode() const override;
	virtual int get_packet_channel() const override;

	virtual void poll() override;
	virtual void close() override;
	virtual void disconnect_peer(int p_peer, bool p_force = false) override;

	virtual bool is_server() const override;
	virtual bool is_server_relay_supported() const override;

	// Overridden so we can instrument the DTLSServer when needed.
	virtual void set_refuse_new_connections(bool p_enabled) override;

	virtual ConnectionStatus get_connection_status() const override;

	virtual int get_unique_id() const override;

	virtual int get_max_packet_size() const override;
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override;
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	Error create_server(int p_port, int p_max_clients = 32, int p_max_channels = 0,
		int p_in_bandwidth = 0, int p_out_bandwidth = 0);
	Error create_client(const String& p_address, int p_port, int p_channel_count = 0,
		int p_in_bandwidth = 0, int p_out_bandwidth = 0, int p_local_port = 0);
	Error create_mesh(int p_id);
	Error add_mesh_peer(int p_id, Ref<ENetConnection> p_host);

	Ref<ENetConnection> get_host() const;
	Ref<ENetPacketPeer> get_peer(int p_id) const;

	ENetMultiplayerPeer() {}
	~ENetMultiplayerPeer();
};

// --- End modules/enet/enet_connection.h ---
#include "modules/register_module_types.h"

#include "core/error/error_macros.h"
#include "core/io/compression.h"
#include "core/io/ip.h"

// ENetPacketPeer

void ENetPacketPeer::peer_disconnect(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect(peer, p_data);
}

void ENetPacketPeer::peer_disconnect_later(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect_later(peer, p_data);
}

void ENetPacketPeer::peer_disconnect_now(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect_now(peer, p_data);
	_on_disconnect();
}

void ENetPacketPeer::ping()
{
	ERR_FAIL_NULL(peer);
	enet_peer_ping(peer);
}

void ENetPacketPeer::ping_interval(int p_interval)
{
	ERR_FAIL_NULL(peer);
	enet_peer_ping_interval(peer, p_interval);
}

int ENetPacketPeer::send(uint8_t p_channel, ENetPacket* p_packet)
{
	ERR_FAIL_NULL_V(peer, -1);
	ERR_FAIL_NULL_V(p_packet, -1);
	ERR_FAIL_COND_V_MSG(p_channel >= peer->channelCount, -1,
		vformat("Unable to send packet on channel %d, max channels: %d", p_channel,
			(int)peer->channelCount));
	return enet_peer_send(peer, p_channel, p_packet);
}

void ENetPacketPeer::reset()
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	enet_peer_reset(peer);
	_on_disconnect();
}

void ENetPacketPeer::throttle_configure(int p_interval, int p_acceleration, int p_deceleration)
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	enet_peer_throttle_configure(peer, p_interval, p_acceleration, p_deceleration);
}

void ENetPacketPeer::set_timeout(int p_timeout, int p_timeout_min, int p_timeout_max)
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	ERR_FAIL_COND_MSG(p_timeout > p_timeout_min || p_timeout_min > p_timeout_max,
		"Timeout limit must be less than minimum timeout, which itself must be less than maximum "
		"timeout");
	enet_peer_timeout(peer, p_timeout, p_timeout_min, p_timeout_max);
}

int ENetPacketPeer::get_max_packet_size() const { return 1 << 24; }

int ENetPacketPeer::get_available_packet_count() const { return packet_queue.size(); }

Error ENetPacketPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_NULL_V(peer, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(packet_queue.is_empty(), ERR_UNAVAILABLE);
	if (last_packet) {
		enet_packet_destroy(last_packet);
		last_packet = nullptr;
	}
	last_packet = packet_queue.front()->get();
	packet_queue.pop_front();
	*r_buffer = (const uint8_t*)(last_packet->data);
	r_buffer_size = last_packet->dataLength;
	return OK;
}

Error ENetPacketPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_NULL_V(peer, ERR_UNCONFIGURED);
	ENetPacket* packet = enet_packet_create(p_buffer, p_buffer_size, ENET_PACKET_FLAG_RELIABLE);
	return send(0, packet) < 0 ? FAILED : OK;
}

int ENetPacketPeer::get_remote_port() const
{
	ERR_FAIL_NULL_V(peer, 0);
	return peer->address.port;
}

bool ENetPacketPeer::is_active() const { return peer != nullptr; }

double ENetPacketPeer::get_statistic(PeerStatistic p_stat)
{
	ERR_FAIL_NULL_V(peer, 0);
	switch (p_stat) {
	case PEER_PACKET_LOSS:
		return peer->packetLoss;
	case PEER_PACKET_LOSS_VARIANCE:
		return peer->packetLossVariance;
	case PEER_PACKET_LOSS_EPOCH:
		return peer->packetLossEpoch;
	case PEER_ROUND_TRIP_TIME:
		return peer->roundTripTime;
	case PEER_ROUND_TRIP_TIME_VARIANCE:
		return peer->roundTripTimeVariance;
	case PEER_LAST_ROUND_TRIP_TIME:
		return peer->lastRoundTripTime;
	case PEER_LAST_ROUND_TRIP_TIME_VARIANCE:
		return peer->lastRoundTripTimeVariance;
	case PEER_PACKET_THROTTLE:
		return peer->packetThrottle;
	case PEER_PACKET_THROTTLE_LIMIT:
		return peer->packetThrottleLimit;
	case PEER_PACKET_THROTTLE_COUNTER:
		return peer->packetThrottleCounter;
	case PEER_PACKET_THROTTLE_EPOCH:
		return peer->packetThrottleEpoch;
	case PEER_PACKET_THROTTLE_ACCELERATION:
		return peer->packetThrottleAcceleration;
	case PEER_PACKET_THROTTLE_DECELERATION:
		return peer->packetThrottleDeceleration;
	case PEER_PACKET_THROTTLE_INTERVAL:
		return peer->packetThrottleInterval;
	}
	ERR_FAIL_V(0);
}

ENetPacketPeer::PeerState ENetPacketPeer::get_state() const
{
	if (!is_active()) {
		return STATE_DISCONNECTED;
	}
	return (PeerState)peer->state;
}

int ENetPacketPeer::get_channels() const
{
	ERR_FAIL_NULL_V_MSG(peer, 0, "The ENetConnection instance isn't currently active.");
	return peer->channelCount;
}

int ENetPacketPeer::get_packet_flags() const
{
	ERR_FAIL_COND_V(packet_queue.is_empty(), 0);
	return packet_queue.front()->get()->flags;
}

void ENetPacketPeer::_on_disconnect()
{
	if (peer) {
#ifdef GODOT_ENET
		enet_peer_socket_destroy(peer);
#endif
		peer->data = nullptr;
	}
	peer = nullptr;
}

void ENetPacketPeer::_queue_packet(ENetPacket* p_packet)
{
	ERR_FAIL_NULL(peer);
	packet_queue.push_back(p_packet);
}

Error ENetPacketPeer::_send(int p_channel, PackedByteArray p_packet, int p_flags)
{
	ERR_FAIL_NULL_V_MSG(peer, ERR_UNCONFIGURED, "Peer not connected.");
	ERR_FAIL_COND_V_MSG(p_channel < 0 || p_channel > (int)peer->channelCount, ERR_INVALID_PARAMETER,
		"Invalid channel");
	ERR_FAIL_COND_V_MSG(p_flags & ~FLAG_ALLOWED, ERR_INVALID_PARAMETER, "Invalid flags");
	ENetPacket* packet = enet_packet_create(p_packet.ptr(), p_packet.size(), p_flags);
	return send(p_channel, packet) == 0 ? OK : FAILED;
}

ENetPacketPeer::ENetPacketPeer(ENetPeer* p_peer)
{
	peer = p_peer;
	peer->data = this;
#ifdef GODOT_ENET
	enet_peer_socket_bind(peer);
#endif
}

ENetPacketPeer::~ENetPacketPeer()
{
	_on_disconnect();
	if (last_packet) {
		enet_packet_destroy(last_packet);
		last_packet = nullptr;
	}
	for (List<ENetPacket*>::Element* E = packet_queue.front(); E; E = E->next()) {
		enet_packet_destroy(E->get());
	}
	packet_queue.clear();
}

// ENetConnection

void ENetConnection::broadcast(enet_uint8 p_channel, ENetPacket* p_packet)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(p_channel >= host->channelLimit,
		vformat("Unable to send packet on channel %d, max channels: %d", p_channel,
			(int)host->channelLimit));
	enet_host_broadcast(host, p_channel, p_packet);
}

Error ENetConnection::create_host(
	int p_max_peers, int p_max_channels, int p_in_bandwidth, int p_out_bandwidth)
{
	return _create(nullptr, p_max_peers, p_max_channels, p_in_bandwidth, p_out_bandwidth);
}

void ENetConnection::destroy()
{
	ERR_FAIL_NULL_MSG(host, "Host already destroyed.");
	for (const Ref<ENetPacketPeer>& peer : peers) {
		peer->_on_disconnect();
	}
	peers.clear();
	enet_host_destroy(host);
	host = nullptr;
}

Ref<ENetPacketPeer> ENetConnection::connect_to_host(
	const String& p_address, int p_port, int p_channels, int p_data)
{
	Ref<ENetPacketPeer> out;
	ERR_FAIL_NULL_V_MSG(host, out, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V_MSG(peers.size(), out, "The ENetConnection is already connected to a peer.");
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, out,
		"The remote port number must be between 1 and 65535 (inclusive).");

	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
#ifdef GODOT_ENET
		ip = IP::resolve_hostname(p_address);
#else
		ip = IP::resolve_hostname(p_address, IP::TYPE_IPV4);
#endif
		ERR_FAIL_COND_V_MSG(
			!ip.is_valid(), out, "Couldn't resolve the server IP address or domain name.");
	}

	ENetAddress address;
#ifdef GODOT_ENET
	enet_address_set_ip(&address, ip.get_ipv6(), 16);
#else
	ERR_FAIL_COND_V_MSG(!ip.is_ipv4(), out,
		"Connecting to an IPv6 server isn't supported when using vanilla ENet. Recompile Godot "
		"with the bundled ENet library.");
	address.host = *(uint32_t*)ip.get_ipv4();
#endif
	address.port = p_port;

	// Initiate connection, allocating enough channels
	ENetPeer* peer = enet_host_connect(
		host, &address, p_channels > 0 ? p_channels : ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, p_data);

	if (peer == nullptr) {
		return nullptr;
	}
	out.instantiate(peer);
	peers.push_back(out);
	return out;
}

ENetConnection::EventType ENetConnection::_parse_event(const ENetEvent& p_event, Event& r_event)
{
	switch (p_event.type) {
	case ENET_EVENT_TYPE_CONNECT: {
		if (p_event.peer->data == nullptr) {
			Ref<ENetPacketPeer> pp = memnew(ENetPacketPeer(p_event.peer));
			peers.push_back(pp);
		}
		r_event.peer = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
		r_event.data = p_event.data;
		return EVENT_CONNECT;
	} break;
	case ENET_EVENT_TYPE_DISCONNECT: {
		// A peer disconnected.
		if (p_event.peer->data != nullptr) {
			Ref<ENetPacketPeer> pp = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
			pp->_on_disconnect();
			peers.erase(pp);
			r_event.peer = pp;
			r_event.data = p_event.data;
			return EVENT_DISCONNECT;
		}
		return EVENT_ERROR;
	} break;
	case ENET_EVENT_TYPE_RECEIVE: {
		// Packet received.
		if (p_event.peer->data != nullptr) {
			r_event.peer = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
			r_event.channel_id = p_event.channelID;
			r_event.packet = p_event.packet;
			return EVENT_RECEIVE;
		}
		return EVENT_ERROR;
	} break;
	case ENET_EVENT_TYPE_NONE:
		return EVENT_NONE;
	default:
		return EVENT_NONE;
	}
}

ENetConnection::EventType ENetConnection::service(int p_timeout, Event& r_event)
{
	ERR_FAIL_NULL_V_MSG(host, EVENT_ERROR, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(r_event.peer.is_valid(), EVENT_ERROR);

	// Drop peers that have already been disconnected.
	// NOTE: Forcibly disconnected peers (i.e. peers disconnected via
	// enet_peer_disconnect*) do not trigger DISCONNECTED events.
	List<Ref<ENetPacketPeer>>::Element* E = peers.front();
	while (E) {
		if (!E->get()->is_active()) {
			peers.erase(E->get());
		}
		E = E->next();
	}

	ENetEvent event;
	int ret = enet_host_service(host, &event, p_timeout);

	if (ret < 0) {
		return EVENT_ERROR;
	}
	else if (ret == 0) {
		return EVENT_NONE;
	}
	return _parse_event(event, r_event);
}

int ENetConnection::check_events(EventType& r_type, Event& r_event)
{
	ERR_FAIL_NULL_V_MSG(host, -1, "The ENetConnection instance isn't currently active.");
	ENetEvent event;
	int ret = enet_host_check_events(host, &event);
	if (ret < 0) {
		r_type = EVENT_ERROR;
		return ret;
	}
	r_type = _parse_event(event, r_event);
	return ret;
}

void ENetConnection::flush()
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_flush(host);
}

void ENetConnection::bandwidth_limit(int p_in_bandwidth, int p_out_bandwidth)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_bandwidth_limit(host, p_in_bandwidth, p_out_bandwidth);
}

void ENetConnection::channel_limit(int p_max_channels)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_channel_limit(host, p_max_channels);
}

void ENetConnection::bandwidth_throttle()
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_bandwidth_throttle(host);
}

void ENetConnection::compress(CompressionMode p_mode)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	Compressor::setup(host, p_mode);
}

double ENetConnection::pop_statistic(HostStatistic p_stat)
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	uint32_t* ptr = nullptr;
	switch (p_stat) {
	case HOST_TOTAL_SENT_DATA:
		ptr = &(host->totalSentData);
		break;
	case HOST_TOTAL_SENT_PACKETS:
		ptr = &(host->totalSentPackets);
		break;
	case HOST_TOTAL_RECEIVED_DATA:
		ptr = &(host->totalReceivedData);
		break;
	case HOST_TOTAL_RECEIVED_PACKETS:
		ptr = &(host->totalReceivedPackets);
		break;
	}
	ERR_FAIL_NULL_V_MSG(ptr, 0, "Invalid statistic: " + itos(p_stat) + ".");
	uint32_t ret = *ptr;
	*ptr = 0;
	return ret;
}

int ENetConnection::get_max_channels() const
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	return host->channelLimit;
}

int ENetConnection::get_local_port() const
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V_MSG(!(host->socket), 0, "The ENetConnection instance isn't currently bound.");
	ENetAddress address;
	ERR_FAIL_COND_V_MSG(
		enet_socket_get_address(host->socket, &address), 0, "Unable to get socket address");
	return address.port;
}

void ENetConnection::get_peers(List<Ref<ENetPacketPeer>>& r_peers)
{
	for (const Ref<ENetPacketPeer>& I : peers) {
		r_peers.push_back(I);
	}
}

Error ENetConnection::dtls_server_setup(const Ref<TLSOptions>& p_options)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_V_MSG(
		host, ERR_UNCONFIGURED, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);
	return enet_host_dtls_server_setup(host, p_options.ptr()) ? FAILED : OK;
#else
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "ENet DTLS support not available in this build.");
#endif
}

void ENetConnection::refuse_new_connections(bool p_refuse)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_refuse_new_connections(host, p_refuse);
#else
	ERR_FAIL_MSG("ENet DTLS support not available in this build.");
#endif
}

Error ENetConnection::dtls_client_setup(const String& p_hostname, const Ref<TLSOptions>& p_options)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_V_MSG(
		host, ERR_UNCONFIGURED, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(p_options.is_null() || p_options->is_server(), ERR_INVALID_PARAMETER);
	return enet_host_dtls_client_setup(host, p_hostname.utf8().get_data(), p_options.ptr()) ? FAILED
																							: OK;
#else
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "ENet DTLS support not available in this build.");
#endif
}

Error ENetConnection::_create(ENetAddress* p_address, int p_max_peers, int p_max_channels,
	int p_in_bandwidth, int p_out_bandwidth)
{
	ERR_FAIL_COND_V_MSG(
		host != nullptr, ERR_ALREADY_IN_USE, "The ENetConnection instance is already active.");
	ERR_FAIL_COND_V_MSG(p_max_peers < 1 || p_max_peers > 4095, ERR_INVALID_PARAMETER,
		"The number of clients must be set between 1 and 4095 (inclusive).");
	ERR_FAIL_COND_V_MSG(p_max_channels < 0 || p_max_channels > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT,
		ERR_INVALID_PARAMETER,
		"Invalid channel count. Must be between 0 and 255 (0 means maximum, i.e. 255)");
	ERR_FAIL_COND_V_MSG(p_in_bandwidth < 0, ERR_INVALID_PARAMETER,
		"The incoming bandwidth limit must be greater than or equal to 0 (0 disables the limit).");
	ERR_FAIL_COND_V_MSG(p_out_bandwidth < 0, ERR_INVALID_PARAMETER,
		"The outgoing bandwidth limit must be greater than or equal to 0 (0 disables the limit).");

	host = enet_host_create(p_address /* the address to bind the server host to */,
		p_max_peers /* allow up to p_max_peers connections */,
		p_max_channels /* allow up to p_max_channel to be used */,
		p_in_bandwidth /* limit incoming bandwidth if > 0 */,
		p_out_bandwidth /* limit outgoing bandwidth if > 0 */);

	ERR_FAIL_NULL_V_MSG(host, ERR_CANT_CREATE, "Couldn't create an ENet host.");
	return OK;
}

void ENetConnection::_broadcast(int p_channel, PackedByteArray p_packet, int p_flags)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(p_channel < 0 || p_channel > (int)host->channelLimit, "Invalid channel");
	ERR_FAIL_COND_MSG(p_flags & ~ENetPacketPeer::FLAG_ALLOWED, "Invalid flags");
	ENetPacket* pkt = enet_packet_create(p_packet.ptr(), p_packet.size(), p_flags);
	broadcast(p_channel, pkt);
}

void ENetConnection::socket_send(
	const String& p_address, int p_port, const PackedByteArray& p_packet)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(!(host->socket), "The ENetConnection instance isn't currently bound.");
	ERR_FAIL_COND_MSG(p_port < 1 || p_port > 65535,
		"The remote port number must be between 1 and 65535 (inclusive).");

	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
#ifdef GODOT_ENET
		ip = IP::resolve_hostname(p_address);
#else
		ip = IP::resolve_hostname(p_address, IP::TYPE_IPV4);
#endif
		ERR_FAIL_COND_MSG(!ip.is_valid(), "Couldn't resolve the server IP address or domain name.");
	}

	ENetAddress address;
#ifdef GODOT_ENET
	enet_address_set_ip(&address, ip.get_ipv6(), 16);
#else
	ERR_FAIL_COND_MSG(!ip.is_ipv4(),
		"Connecting to an IPv6 server isn't supported when using vanilla ENet. "
		"Recompile Godot with the bundled ENet library.");
	address.host = *(uint32_t*)ip.get_ipv4();
#endif
	address.port = p_port;

	ENetBuffer enet_buffers[1];
	enet_buffers[0].data = (void*)p_packet.ptr();
	enet_buffers[0].dataLength = p_packet.size();

	enet_socket_send(host->socket, &address, enet_buffers, 1);
}

ENetConnection::~ENetConnection()
{
	if (host) {
		destroy();
	}
}

size_t ENetConnection::Compressor::enet_compress(void* context, const ENetBuffer* inBuffers,
	size_t inBufferCount, size_t inLimit, enet_uint8* outData, size_t outLimit)
{
	Compressor* compressor = (Compressor*)(context);

	if (size_t(compressor->src_mem.size()) < inLimit) {
		compressor->src_mem.resize(inLimit);
	}

	size_t total = inLimit;
	size_t ofs = 0;
	while (total) {
		for (size_t i = 0; i < inBufferCount; i++) {
			const size_t to_copy = MIN(total, inBuffers[i].dataLength);
			memcpy(&compressor->src_mem.write[ofs], inBuffers[i].data, to_copy);
			ofs += to_copy;
			total -= to_copy;
		}
	}

	Compression::Mode mode;

	switch (compressor->mode) {
	case COMPRESS_FASTLZ: {
		mode = Compression::MODE_FASTLZ;
	} break;
	case COMPRESS_ZLIB: {
		mode = Compression::MODE_DEFLATE;
	} break;
	case COMPRESS_ZSTD: {
		mode = Compression::MODE_ZSTD;
	} break;
	default: {
		ERR_FAIL_V_MSG(0, vformat("Invalid ENet compression mode: %d", compressor->mode));
	}
	}

	const int64_t req_size = Compression::get_max_compressed_buffer_size(ofs, mode);
	if (compressor->dst_mem.size() < req_size) {
		compressor->dst_mem.resize(req_size);
	}
	const int64_t ret =
		Compression::compress(compressor->dst_mem.ptrw(), compressor->src_mem.ptr(), ofs, mode);

	if (ret < 0) {
		return 0;
	}

	const size_t ret_size = size_t(ret);
	if (ret_size > outLimit) {
		return 0; // Do not bother
	}

	memcpy(outData, compressor->dst_mem.ptr(), ret_size);

	return ret;
}

size_t ENetConnection::Compressor::enet_decompress(
	void* context, const enet_uint8* inData, size_t inLimit, enet_uint8* outData, size_t outLimit)
{
	Compressor* compressor = (Compressor*)(context);
	int64_t ret = -1;
	switch (compressor->mode) {
	case COMPRESS_FASTLZ: {
		ret = Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_FASTLZ);
	} break;
	case COMPRESS_ZLIB: {
		ret =
			Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_DEFLATE);
	} break;
	case COMPRESS_ZSTD: {
		ret = Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_ZSTD);
	} break;
	default: {
	}
	}
	if (ret < 0) {
		return 0;
	}
	else {
		return ret;
	}
}

void ENetConnection::Compressor::setup(ENetHost* p_host, CompressionMode p_mode)
{
	ERR_FAIL_NULL(p_host);
	switch (p_mode) {
	case COMPRESS_NONE: {
		enet_host_compress(p_host, nullptr);
	} break;
	case COMPRESS_RANGE_CODER: {
		enet_host_compress_with_range_coder(p_host);
	} break;
	case COMPRESS_FASTLZ:
	case COMPRESS_ZLIB:
	case COMPRESS_ZSTD: {
		Compressor* compressor = memnew(Compressor(p_mode));
		enet_host_compress(p_host, &(compressor->enet_compressor));
	} break;
	}
}

ENetConnection::Compressor::Compressor(CompressionMode p_mode)
{
	mode = p_mode;
	enet_compressor.context = this;
	enet_compressor.compress = enet_compress;
	enet_compressor.decompress = enet_decompress;
	enet_compressor.destroy = enet_compressor_destroy;
}

// ENetMultiplayerPeer

void ENetMultiplayerPeer::set_target_peer(int p_peer) { target_peer = p_peer; }

int ENetMultiplayerPeer::get_packet_peer() const
{
	ERR_FAIL_COND_V_MSG(!_is_active(), 1, "The multiplayer instance isn't currently active.");
	ERR_FAIL_COND_V(incoming_packets.is_empty(), 1);

	return incoming_packets.front()->get().from;
}

MultiplayerPeer::TransferMode ENetMultiplayerPeer::get_packet_mode() const
{
	ERR_FAIL_COND_V_MSG(
		!_is_active(), TRANSFER_MODE_RELIABLE, "The multiplayer instance isn't currently active.");
	ERR_FAIL_COND_V(incoming_packets.is_empty(), TRANSFER_MODE_RELIABLE);
	return incoming_packets.front()->get().transfer_mode;
}

int ENetMultiplayerPeer::get_packet_channel() const
{
	ERR_FAIL_COND_V_MSG(!_is_active(), 1, "The multiplayer instance isn't currently active.");
	ERR_FAIL_COND_V(incoming_packets.is_empty(), 1);
	int ch = incoming_packets.front()->get().channel;
	if (ch >= SYSCH_MAX) { // First 2 channels are reserved.
		return ch - SYSCH_MAX + 1;
	}
	return 0;
}

Error ENetMultiplayerPeer::create_server(
	int p_port, int p_max_clients, int p_max_channels, int p_in_bandwidth, int p_out_bandwidth)
{
	ERR_FAIL_COND_V_MSG(
		_is_active(), ERR_ALREADY_IN_USE, "The multiplayer instance is already active.");
	set_refuse_new_connections(false);
	Ref<ENetConnection> host;
	host.instantiate();
	active_mode = MODE_SERVER;
	unique_id = 1;
	connection_status = CONNECTION_CONNECTED;
	hosts[0] = host;
	return OK;
}

Error ENetMultiplayerPeer::create_client(const String& p_address, int p_port, int p_channel_count,
	int p_in_bandwidth, int p_out_bandwidth, int p_local_port)
{
	ERR_FAIL_COND_V_MSG(
		_is_active(), ERR_ALREADY_IN_USE, "The multiplayer instance is already active.");
	set_refuse_new_connections(false);
	Ref<ENetConnection> host;
	host.instantiate();
	Error err;
	err = host->create_host(1, 0, p_in_bandwidth, p_out_bandwidth);
	if (err != OK) {
		return err;
	}

	unique_id = generate_unique_id();

	Ref<ENetPacketPeer> peer = host->connect_to_host(
		p_address, p_port, p_channel_count > 0 ? p_channel_count + SYSCH_MAX : 0, unique_id);
	if (peer.is_null()) {
		host->destroy();
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "Couldn't connect to the ENet multiplayer server.");
	}

	// Need to wait for CONNECT event.
	connection_status = CONNECTION_CONNECTING;
	active_mode = MODE_CLIENT;
	peers[1] = peer;
	hosts[0] = host;

	return OK;
}

Error ENetMultiplayerPeer::create_mesh(int p_id)
{
	ERR_FAIL_COND_V_MSG(p_id <= 0, ERR_INVALID_PARAMETER, "The unique ID must be greater then 0");
	ERR_FAIL_COND_V_MSG(
		_is_active(), ERR_ALREADY_IN_USE, "The multiplayer instance is already active.");
	active_mode = MODE_MESH;
	unique_id = p_id;
	connection_status = CONNECTION_CONNECTED;
	return OK;
}

Error ENetMultiplayerPeer::add_mesh_peer(int p_id, Ref<ENetConnection> p_host)
{
	ERR_FAIL_COND_V(p_host.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(active_mode != MODE_MESH, ERR_UNCONFIGURED,
		"The multiplayer instance is not configured as a mesh. Call 'create_mesh' first.");
	List<Ref<ENetPacketPeer>> host_peers;
	p_host->get_peers(host_peers);
	ERR_FAIL_COND_V_MSG(host_peers.size() != 1 || host_peers.front()->get()->get_state() !=
													  ENetPacketPeer::STATE_CONNECTED,
		ERR_INVALID_PARAMETER,
		"The provided host must have exactly one peer in the connected state.");
	hosts[p_id] = p_host;
	peers[p_id] = host_peers.front()->get();
	return OK;
}

void ENetMultiplayerPeer::_store_packet(int32_t p_source, ENetConnection::Event& p_event)
{
	Packet packet;
	packet.packet = p_event.packet;
	packet.channel = p_event.channel_id;
	packet.from = p_source;
	if (p_event.packet->flags & ENET_PACKET_FLAG_RELIABLE) {
		packet.transfer_mode = TRANSFER_MODE_RELIABLE;
	}
	else if (p_event.packet->flags & ENET_PACKET_FLAG_UNSEQUENCED) {
		packet.transfer_mode = TRANSFER_MODE_UNRELIABLE;
	}
	else {
		packet.transfer_mode = TRANSFER_MODE_UNRELIABLE_ORDERED;
	}
	packet.packet->referenceCount++;
	incoming_packets.push_back(packet);
}

void ENetMultiplayerPeer::_disconnect_inactive_peers()
{
	HashSet<int> to_drop;
	for (const KeyValue<int, Ref<ENetPacketPeer>>& E : peers) {
		if (E.value->is_active()) {
			continue;
		}
		to_drop.insert(E.key);
	}
	for (const int& P : to_drop) {
		peers.erase(P);
		if (hosts.has(P)) {
			hosts.erase(P);
		}
		ERR_CONTINUE(active_mode == MODE_CLIENT && P != TARGET_PEER_SERVER);
	}
}

bool ENetMultiplayerPeer::is_server() const { return active_mode == MODE_SERVER; }

bool ENetMultiplayerPeer::is_server_relay_supported() const
{
	return active_mode == MODE_SERVER || active_mode == MODE_CLIENT;
}

void ENetMultiplayerPeer::disconnect_peer(int p_peer, bool p_force)
{
	ERR_FAIL_COND(!_is_active() || !peers.has(p_peer));
	peers[p_peer]->peer_disconnect(0); // Will be removed during next poll.
	if (active_mode == MODE_CLIENT || active_mode == MODE_SERVER) {
		hosts[0]->flush();
	}
	else {
		ERR_FAIL_COND(!hosts.has(p_peer));
		hosts[p_peer]->flush();
	}
	if (p_force) {
		peers.erase(p_peer);
		if (hosts.has(p_peer)) {
			hosts.erase(p_peer);
		}
		if (active_mode == MODE_CLIENT) {
			hosts.clear(); // Avoid flushing again.
			close();
		}
	}
}

void ENetMultiplayerPeer::close()
{
	if (!_is_active()) {
		return;
	}

	_pop_current_packet();

	for (KeyValue<int, Ref<ENetPacketPeer>>& E : peers) {
		if (E.value.is_valid() && E.value->get_state() == ENetPacketPeer::STATE_CONNECTED) {
			E.value->peer_disconnect_now(0);
		}
	}
	for (KeyValue<int, Ref<ENetConnection>>& E : hosts) {
		E.value->flush();
		E.value->destroy();
	}

	active_mode = MODE_NONE;
	incoming_packets.clear();
	peers.clear();
	hosts.clear();
	unique_id = 0;
	connection_status = CONNECTION_DISCONNECTED;
	set_refuse_new_connections(false);
}

int ENetMultiplayerPeer::get_available_packet_count() const { return incoming_packets.size(); }

Error ENetMultiplayerPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_COND_V_MSG(
		incoming_packets.is_empty(), ERR_UNAVAILABLE, "No incoming packets available.");

	_pop_current_packet();

	current_packet = incoming_packets.front()->get();
	incoming_packets.pop_front();

	*r_buffer = (const uint8_t*)(current_packet.packet->data);
	r_buffer_size = current_packet.packet->dataLength;

	return OK;
}

Error ENetMultiplayerPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V_MSG(
		!_is_active(), ERR_UNCONFIGURED, "The multiplayer instance isn't currently active.");
	ERR_FAIL_COND_V_MSG(connection_status != CONNECTION_CONNECTED, ERR_UNCONFIGURED,
		"The multiplayer instance isn't currently connected to any server or client.");
	ERR_FAIL_COND_V_MSG(target_peer != 0 && !peers.has(Math::abs(target_peer)),
		ERR_INVALID_PARAMETER, vformat("Invalid target peer: %d", target_peer));
	ERR_FAIL_COND_V(active_mode == MODE_CLIENT && !peers.has(1), ERR_BUG);

	int packet_flags = 0;
	int channel = SYSCH_RELIABLE;
	int tr_channel = get_transfer_channel();
	switch (get_transfer_mode()) {
	case TRANSFER_MODE_UNRELIABLE: {
		packet_flags = ENET_PACKET_FLAG_UNSEQUENCED | ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
		channel = SYSCH_UNRELIABLE;
	} break;
	case TRANSFER_MODE_UNRELIABLE_ORDERED: {
		packet_flags = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
		channel = SYSCH_UNRELIABLE;
	} break;
	case TRANSFER_MODE_RELIABLE: {
		packet_flags = ENET_PACKET_FLAG_RELIABLE;
		channel = SYSCH_RELIABLE;
	} break;
	}
	if (tr_channel > 0) {
		channel = SYSCH_MAX + tr_channel - 1;
	}

#ifdef DEBUG_ENABLED
	if ((packet_flags & ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT) &&
		p_buffer_size > ENET_HOST_DEFAULT_MTU) {
		WARN_PRINT_ONCE(vformat("Sending %d bytes unreliably which is above the MTU (%d), this "
								"will result in higher packet loss",
			p_buffer_size, ENET_HOST_DEFAULT_MTU));
	}
#endif

	ENetPacket* packet = enet_packet_create(nullptr, p_buffer_size, packet_flags);
	memcpy(&packet->data[0], p_buffer, p_buffer_size);

	if (is_server()) {
		if (target_peer == 0) {
			hosts[0]->broadcast(channel, packet);
		}
		else if (target_peer < 0) {
			// Send to all but one and make copies for sending.
			int exclude = -target_peer;
			for (KeyValue<int, Ref<ENetPacketPeer>>& E : peers) {
				if (E.key == exclude) {
					continue;
				}
				E.value->send(channel, packet);
			}
			_destroy_unused(packet);
		}
		else {
			peers[target_peer]->send(channel, packet);
		}
		ERR_FAIL_COND_V(!hosts.has(0), ERR_BUG);
		hosts[0]->flush();
	}
	else if (active_mode == MODE_CLIENT) {
		peers[1]->send(channel, packet); // Send to server for broadcast.
		ERR_FAIL_COND_V(!hosts.has(0), ERR_BUG);
		hosts[0]->flush();
	}
	else {
		if (target_peer <= 0) {
			int exclude = Math::abs(target_peer);
			for (KeyValue<int, Ref<ENetPacketPeer>>& E : peers) {
				if (E.key == exclude) {
					continue;
				}
				E.value->send(channel, packet);
				ERR_CONTINUE(!hosts.has(E.key));
				hosts[E.key]->flush();
			}
			_destroy_unused(packet);
		}
		else {
			peers[target_peer]->send(channel, packet);
			ERR_FAIL_COND_V(!hosts.has(target_peer), ERR_BUG);
			hosts[target_peer]->flush();
		}
	}

	return OK;
}

int ENetMultiplayerPeer::get_max_packet_size() const
{
	return 1 << 24; // Anything is good
}

void ENetMultiplayerPeer::_pop_current_packet()
{
	if (current_packet.packet) {
		current_packet.packet->referenceCount--;
		_destroy_unused(current_packet.packet);
		current_packet.packet = nullptr;
		current_packet.from = 0;
		current_packet.channel = -1;
	}
}

MultiplayerPeer::ConnectionStatus ENetMultiplayerPeer::get_connection_status() const
{
	return connection_status;
}

int ENetMultiplayerPeer::get_unique_id() const
{
	ERR_FAIL_COND_V_MSG(!_is_active(), 0, "The multiplayer instance isn't currently active.");
	return unique_id;
}

void ENetMultiplayerPeer::set_refuse_new_connections(bool p_enabled)
{
#ifdef GODOT_ENET
	if (_is_active()) {
		for (KeyValue<int, Ref<ENetConnection>>& E : hosts) {
			E.value->refuse_new_connections(p_enabled);
		}
	}
#endif
	MultiplayerPeer::set_refuse_new_connections(p_enabled);
}

Ref<ENetConnection> ENetMultiplayerPeer::get_host() const
{
	ERR_FAIL_COND_V(!_is_active(), nullptr);
	ERR_FAIL_COND_V(active_mode == MODE_MESH, nullptr);
	return hosts[0];
}

Ref<ENetPacketPeer> ENetMultiplayerPeer::get_peer(int p_id) const
{
	ERR_FAIL_COND_V(!_is_active(), nullptr);
	ERR_FAIL_COND_V(!peers.has(p_id), nullptr);
	ERR_FAIL_COND_V(active_mode == MODE_CLIENT && p_id != 1, nullptr);
	return peers[p_id];
}

void ENetMultiplayerPeer::_destroy_unused(ENetPacket* p_packet)
{
	if (p_packet->referenceCount == 0) {
		enet_packet_destroy(p_packet);
	}
}

ENetMultiplayerPeer::~ENetMultiplayerPeer()
{
	if (_is_active()) {
		close();
	}
}

void ENetMultiplayerPeer::poll() {}

// Module registration

static bool enet_ok = false;

void initialize_enet_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (enet_initialize() != 0) {
		ERR_PRINT("ENet initialization failure");
	}
	else {
		enet_ok = true;
	}
}

void uninitialize_enet_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (enet_ok) {
		enet_deinitialize();
	}
}
#endif // defined(MODULE_ENET_ENABLED)


// --- Merged from modules/mbedtls/crypto_mbedtls.cpp ---
#if defined(MODULE_MBEDTLS_ENABLED)
// --- Inlined from modules/mbedtls/crypto_mbedtls.h ---
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/timing.h>

#include "core/crypto/crypto.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/stream_peer.h"

#if MBEDTLS_VERSION_MAJOR < 4
int godot_mbedtls_random_compat(void* p_rng, unsigned char* r_output, size_t p_output_len);
#define GODOT_MBEDTLS_COMPAT_ARGS(x) x, godot_mbedtls_random_compat, nullptr
#define GODOT_MBEDTLS_COMPAT_SSL_CONF                                                              \
	mbedtls_ssl_conf_rng(&conf, godot_mbedtls_random_compat, nullptr);
#else
#define GODOT_MBEDTLS_COMPAT_ARGS(x) x
#define GODOT_MBEDTLS_COMPAT_SSL_CONF
#endif

struct CryptoKey::Data
{
	psa_key_id_t pk_slot = PSA_KEY_ID_NULL;
	psa_key_type_t pk_type = PSA_KEY_TYPE_NONE;
	size_t pk_size = 0;
	bool public_only = true;

	void clear();
	int parse_key(const uint8_t* p_buf, int p_size, bool p_public_only);
	psa_key_id_t reimport(psa_key_usage_t p_usage,
		psa_algorithm_t p_alg); // You *MUST* call psa_destroy_key when you are done with the key.

	static Ref<CryptoKey> generate(psa_key_type_t p_type, size_t p_size);
};

struct X509Certificate::Data
{
	mbedtls_x509_crt cert;
	int locks = 0;

	Data()
	{
		mbedtls_x509_crt_init(&cert);
	}

	_FORCE_INLINE_ void lock() { locks++; }
	_FORCE_INLINE_ void unlock() { locks--; }
};

struct HMACContext::Data
{
	psa_mac_operation_t mac_op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t key_slot = PSA_KEY_ID_NULL;
	int hash_len = 0;

	void clear();
	static bool is_hash_type_allowed(HashingContext::HashType p_hash_type);
};

namespace MbedTLSModule
{
void initialize_crypto();
void finalize_crypto();
Ref<X509Certificate> get_default_certificates();
psa_algorithm_t psa_alg_from_hashtype(HashingContext::HashType p_hash_type, int& r_size);
mbedtls_md_type_t md_type_from_hashtype(HashingContext::HashType p_hash_type, int& r_size);
} // namespace MbedTLSModule

class TLSContextMbedTLS;

class CookieContextMbedTLS final : public RefCounted
{
	friend class TLSContextMbedTLS;

private:
	bool inited = false;
	mbedtls_ssl_cookie_ctx cookie_ctx;

public:
	Error setup();
	void clear();

	CookieContextMbedTLS();
	~CookieContextMbedTLS();
};

class TLSContextMbedTLS final : public RefCounted
{
private:
	bool inited = false;
	mbedtls_pk_context pk;
	Ref<X509Certificate> certs;
	Ref<CookieContextMbedTLS> cookies;
	mbedtls_ssl_context tls;
	mbedtls_ssl_config conf;

	Error _setup(int p_endpoint, int p_transport, int p_authmode);

public:
	static void print_mbedtls_error(int p_ret);

	Error init_server(int p_transport, Ref<TLSOptions> p_options,
		Ref<CookieContextMbedTLS> p_cookies = Ref<CookieContextMbedTLS>());
	Error init_client(int p_transport, const String& p_hostname, Ref<TLSOptions> p_options);
	void clear();

	mbedtls_ssl_context* get_context();

	TLSContextMbedTLS();
	~TLSContextMbedTLS();
};

struct StreamPeerTLS::Data
{
	Status status = STATUS_DISCONNECTED;
	String hostname;
	Ref<StreamPeer> base;
	Ref<TLSContextMbedTLS> tls_ctx;

	Data()
	{
		tls_ctx.instantiate();
	}

	static int bio_recv(void* ctx, unsigned char* buf, size_t len);
	static int bio_send(void* ctx, const unsigned char* buf, size_t len);
	void cleanup();
	Error do_handshake(StreamPeerTLS* p_owner);
};

struct PacketPeerDTLS::Data
{
	enum
	{
		PACKET_BUFFER_SIZE = 65536
	};

	uint8_t packet_buffer[PACKET_BUFFER_SIZE];
	Status status = STATUS_DISCONNECTED;
	String hostname;
	Ref<PacketPeerUDP> base;
	Ref<TLSContextMbedTLS> tls_ctx;
	mbedtls_timing_delay_context timer;

	Data()
	{
		tls_ctx.instantiate();
	}

	static int bio_recv(void* ctx, unsigned char* buf, size_t len);
	static int bio_send(void* ctx, const unsigned char* buf, size_t len);
	void cleanup();
	Error do_handshake();
	int set_cookie();
	Error accept_peer(Ref<PacketPeerUDP> p_base, Ref<TLSOptions> p_options,
		Ref<CookieContextMbedTLS> p_cookies = Ref<CookieContextMbedTLS>());
};

struct DTLSServer::Data
{
	Ref<TLSOptions> tls_options;
	Ref<CookieContextMbedTLS> cookies;

	Data()
	{
		cookies.instantiate();
	}
};

// --- End modules/mbedtls/crypto_mbedtls.h ---
// --- Inlined/Folded from modules/mbedtls/mbedtls_cpp_compat.h ---
#include <mbedtls/psa_util.h>

#ifdef MBEDTLS_ECDSA_DER_MAX_LEN
#define GODOT_MBEDTLS_ECDSA_DER_MAX_LEN MBEDTLS_ECDSA_DER_MAX_LEN
#else
#define GODOT_MBEDTLS_ECDSA_DER_MAX_LEN 192
#endif

// The compatibility functions defined in <mbedtls/psa_util.h> are not guarded by
// __cplusplus ifdefs which results in incorrect symbols when included from C++.
// For this reason, (and to avoid problems if this is fixed in future versions of
// the header) we define our own wrappers implemented in pure C.
#ifdef __cplusplus
extern "C" {
#endif
int godot_mbedtls_ecdsa_raw_to_der(size_t bits, const unsigned char *raw, size_t raw_len, unsigned char *der, size_t der_size, size_t *der_len);
int godot_mbedtls_ecdsa_der_to_raw(size_t bits, const unsigned char *der, size_t der_len, unsigned char *raw, size_t raw_size, size_t *raw_len);
#ifdef __cplusplus
}
#endif
#include "modules/register_module_types.h"

#include <mbedtls/debug.h>
#include <mbedtls/md.h>
#include <mbedtls/pem.h>
#include <mbedtls/psa_util.h>
#include <psa/crypto.h>

#include "core/config/project_settings.h"
#include "core/io/certs_compressed.gen.h"
#include "core/io/compression.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer_socket.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#define PEM_BEGIN_CRT "-----BEGIN CERTIFICATE-----\n"
#define PEM_END_CRT "-----END CERTIFICATE-----\n"
#define PEM_MIN_SIZE 54

#if MBEDTLS_VERSION_MAJOR < 4
int godot_mbedtls_random_compat(void* p_rng, unsigned char* r_output, size_t p_output_len)
{
	return psa_generate_random(r_output, p_output_len);
}
#endif

static Ref<X509Certificate> default_certs;

/// CryptoKey

CryptoKey::CryptoKey()
{
	data = memnew(Data);
}

CryptoKey::~CryptoKey()
{
	if (data) {
		data->clear();
		memdelete(data);
		data = nullptr;
	}
}

CryptoKey* CryptoKey::create(bool p_notify_postinitialize)
{
	return memnew(CryptoKey);
}

bool CryptoKey::is_available()
{
	return true;
}

bool CryptoKey::is_public_only() const
{
	return data->public_only;
}

Ref<CryptoKey> CryptoKey::Data::generate(psa_key_type_t p_type, size_t p_size)
{
	psa_key_id_t slot = PSA_KEY_ID_NULL;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, p_type);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY);
	psa_set_key_bits(&attr, p_size);
	int ret = psa_generate_key(&attr, &slot);
	ERR_FAIL_COND_V(ret != 0, nullptr);
	Ref<CryptoKey> out;
	out.instantiate();
	out->data->public_only = false;
	out->data->pk_slot = slot;
	out->data->pk_type = p_type;
	out->data->pk_size = p_size;
	return out;
}

Error CryptoKey::load(const String& p_path, bool p_public_only)
{
	PackedByteArray out;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(
		f.is_null(), ERR_INVALID_PARAMETER, "Cannot open CryptoKey file '" + p_path + "'.");

	uint64_t flen = f->get_length();
	out.resize(flen + 1);
	f->get_buffer(out.ptrw(), flen);
	out.write[flen] = 0; // string terminator

	int ret = data->parse_key(out.ptr(), out.size(), p_public_only);
	// We MUST zeroize the memory for safety!
	mbedtls_platform_zeroize(out.ptrw(), out.size());
	ERR_FAIL_COND_V_MSG(ret, FAILED, "Error parsing key '" + itos(ret) + "'.");

	data->public_only = p_public_only;
	return OK;
}

Error CryptoKey::save(const String& p_path, bool p_public_only)
{
	ERR_FAIL_COND_V_MSG(data->public_only && !p_public_only, ERR_INVALID_PARAMETER,
		"The key only has the public part. Cannot save as a private key.");
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(
		f.is_null(), ERR_INVALID_PARAMETER, "Cannot save CryptoKey file '" + p_path + "'.");

	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = 0;
	if (p_public_only) {
		ret = mbedtls_pk_copy_public_from_psa(data->pk_slot, &ctx);
	}
	else {
		ret = mbedtls_pk_copy_from_psa(data->pk_slot, &ctx);
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(FAILED, "Failed to import key: " + itos(ret));
	}

	unsigned char w[16000];
	memset(w, 0, sizeof(w));
	if (p_public_only) {
		ret = mbedtls_pk_write_pubkey_pem(&ctx, w, sizeof(w));
	}
	else {
		ret = mbedtls_pk_write_key_pem(&ctx, w, sizeof(w));
	}
	mbedtls_pk_free(&ctx);

	if (ret != 0) {
		mbedtls_platform_zeroize(w, sizeof(w)); // Zeroize anything we might have written.
		ERR_FAIL_V_MSG(FAILED, "Error writing key '" + itos(ret) + "'.");
	}

	size_t len = strlen((char*)w);
	f->store_buffer(w, len);
	mbedtls_platform_zeroize(w, sizeof(w)); // Zeroize temporary buffer.
	return OK;
}

Error CryptoKey::load_from_string(const String& p_string_key, bool p_public_only)
{
	const CharString string_key_utf8 = p_string_key.utf8();
	int ret = data->parse_key(
		(const unsigned char*)string_key_utf8.get_data(), string_key_utf8.size(), p_public_only);
	ERR_FAIL_COND_V_MSG(ret, FAILED, "Error parsing key '" + itos(ret) + "'.");

	data->public_only = p_public_only;
	return OK;
}

String CryptoKey::save_to_string(bool p_public_only)
{
	ERR_FAIL_COND_V_MSG(data->public_only && !p_public_only, "",
		"The key only has the public part. Cannot save as a private key.");
	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = 0;
	if (p_public_only) {
		ret = mbedtls_pk_copy_public_from_psa(data->pk_slot, &ctx);
	}
	else {
		ret = mbedtls_pk_copy_from_psa(data->pk_slot, &ctx);
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG("", "Failed to import key: " + itos(ret));
	}

	unsigned char w[16000];
	memset(w, 0, sizeof(w));
	if (p_public_only) {
		ret = mbedtls_pk_write_pubkey_pem(&ctx, w, sizeof(w));
	}
	else {
		ret = mbedtls_pk_write_key_pem(&ctx, w, sizeof(w));
	}
	mbedtls_pk_free(&ctx);
	if (ret != 0) {
		mbedtls_platform_zeroize(w, sizeof(w));
		ERR_FAIL_V_MSG("", "Error saving key '" + itos(ret) + "'.");
	}
	String s = String::utf8((char*)w);
	mbedtls_platform_zeroize(w, sizeof(w));
	return s;
}

psa_key_id_t CryptoKey::Data::reimport(psa_key_usage_t p_usage, psa_algorithm_t p_alg)
{
	ERR_FAIL_COND_V(pk_slot == PSA_KEY_ID_NULL, PSA_KEY_ID_NULL);

	// We can't copy the key, as we are not allowed to change the usage/algorithm.
	// We will need to export and reimport the key.
	psa_key_id_t slot = PSA_KEY_ID_NULL;
	int size = PSA_EXPORT_KEY_OUTPUT_SIZE(pk_type, pk_size);
	uint8_t* buf = (uint8_t*)alloca(size);
	size_t out_size = 0;
	int ret = psa_export_key(pk_slot, buf, size, &out_size);
	ERR_FAIL_COND_V_MSG(ret, slot, "Failed to export the key: " + itos(ret));

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, pk_type);
	psa_set_key_bits(&attr, pk_size);
	psa_set_key_usage_flags(&attr, p_usage);
	psa_set_key_algorithm(&attr, p_alg);
	ret = psa_import_key(&attr, buf, out_size, &slot);
	mbedtls_platform_zeroize(buf, size);

	ERR_FAIL_COND_V_MSG(ret, slot, "Failed to re-import key: " + itos(ret));
	return slot;
}

void CryptoKey::Data::clear()
{
	psa_destroy_key(pk_slot);
	pk_slot = PSA_KEY_ID_NULL;
	pk_type = PSA_KEY_TYPE_NONE;
	pk_size = 0;
}

int CryptoKey::Data::parse_key(const uint8_t* p_buf, int p_size, bool p_public_only)
{
	clear();

	// Not yet fully supported via PSA
	// See: https://github.com/ARM-software/psa-api/issues/50
	psa_key_usage_t flags = PSA_KEY_USAGE_VERIFY_HASH;
	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = -1;
	if (p_public_only) {
		ret = mbedtls_pk_parse_public_key(&ctx, p_buf, p_size);
	}
	else {
		flags = PSA_KEY_USAGE_SIGN_HASH;
		ret = mbedtls_pk_parse_key(&ctx, p_buf, p_size, nullptr, GODOT_MBEDTLS_COMPAT_ARGS(0));
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(PSA_ERROR_GENERIC_ERROR, "Failed to parse key " + itos(ret));
	}
	psa_key_attributes_t import_attr = PSA_KEY_ATTRIBUTES_INIT;
	ret = mbedtls_pk_get_psa_attributes(&ctx, flags, &import_attr);
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(PSA_ERROR_GENERIC_ERROR, "Failed to get key attributes " + itos(ret));
	}
	pk_type = psa_get_key_type(&import_attr);
	pk_size = psa_get_key_bits(&import_attr);
	psa_reset_key_attributes(&import_attr);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, pk_type);
	psa_set_key_bits(&attr, pk_size);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY);
	ret = mbedtls_pk_import_into_psa(&ctx, &attr, &pk_slot);
	mbedtls_pk_free(&ctx);
	if (ret != PSA_SUCCESS) {
		clear();
	}
	return ret;
}

/// X509Certificate

X509Certificate::X509Certificate()
{
	data = memnew(Data);
}

X509Certificate::~X509Certificate()
{
	if (data) {
		int locks = data->locks;
		mbedtls_x509_crt_free(&data->cert);
		memdelete(data);
		data = nullptr;
		ERR_FAIL_COND(locks);
	}
}

X509Certificate* X509Certificate::create(bool p_notify_postinitialize)
{
	return memnew(X509Certificate);
}

bool X509Certificate::is_available()
{
	return true;
}

Error X509Certificate::load(const String& p_path)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");

	PackedByteArray out;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER,
		vformat("Cannot open X509Certificate file '%s'.", p_path));

	uint64_t flen = f->get_length();
	out.resize(flen + 1);
	f->get_buffer(out.ptrw(), flen);
	out.write[flen] = 0; // string terminator

	int ret = mbedtls_x509_crt_parse(&data->cert, out.ptr(), out.size());
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED,
		vformat("Error parsing X509 certificates from file '%s': %d.", p_path, ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat("MbedTLS: Some X509 certificates could not be parsed from file '%s' "
							  "(%d certificates skipped).",
			p_path, ret));
	}

	return OK;
}

Error X509Certificate::load_from_memory(const uint8_t* p_buffer, int p_len)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");

	int ret = mbedtls_x509_crt_parse(&data->cert, p_buffer, p_len);
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED, vformat("Error parsing X509 certificates: %d.", ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat(
			"MbedTLS: Some X509 certificates could not be parsed (%d certificates skipped).", ret));
	}
	return OK;
}

Error X509Certificate::save(const String& p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER,
		vformat("Cannot save X509Certificate file '%s'.", p_path));

	mbedtls_x509_crt* crt = &data->cert;
	while (crt) {
		unsigned char w[4096];
		size_t wrote = 0;
		int ret = mbedtls_pem_write_buffer(
			PEM_BEGIN_CRT, PEM_END_CRT, data->cert.raw.p, data->cert.raw.len, w, sizeof(w), &wrote);
		if (ret != 0 || wrote == 0) {
			ERR_FAIL_V_MSG(FAILED, "Error writing certificate '" + itos(ret) + "'.");
		}

		f->store_buffer(w, wrote - 1); // don't write the string terminator
		crt = crt->next;
	}
	return OK;
}

String X509Certificate::save_to_string()
{
	String buffer;
	mbedtls_x509_crt* crt = &data->cert;
	while (crt) {
		unsigned char w[4096];
		size_t wrote = 0;
		int ret = mbedtls_pem_write_buffer(
			PEM_BEGIN_CRT, PEM_END_CRT, data->cert.raw.p, data->cert.raw.len, w, sizeof(w), &wrote);
		ERR_FAIL_COND_V_MSG(ret != 0 || wrote == 0, String(), "Error saving the certificate.");

		// PEM is base64, aka ascii
		buffer += String::ascii(Span((char*)w, wrote));
		crt = crt->next;
	}
	if (buffer.length() <= PEM_MIN_SIZE) {
		// When the returned value of variable 'buffer' would consist of no Base-64 data, return an
		// empty String instead.
		return String();
	}
	return buffer;
}

Error X509Certificate::load_from_string(const String& p_string_key)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");
	CharString cs = p_string_key.utf8();

	int ret = mbedtls_x509_crt_parse(&data->cert, (const unsigned char*)cs.get_data(), cs.size());
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED, vformat("Error parsing X509 certificates: %d.", ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat(
			"MbedTLS: Some X509 certificates could not be parsed (%d certificates skipped).", ret));
	}

	return OK;
}

/// HMACContext

bool HMACContext::Data::is_hash_type_allowed(HashingContext::HashType p_hash_type)
{
	switch (p_hash_type) {
	case HashingContext::HASH_SHA1:
	case HashingContext::HASH_SHA256:
		return true;
	default:
		return false;
	}
}

HMACContext::HMACContext()
{
	data = memnew(Data);
}

HMACContext::~HMACContext()
{
	if (data) {
		data->clear();
		memdelete(data);
		data = nullptr;
	}
}

HMACContext* HMACContext::create(bool p_notify_postinitialize)
{
	return memnew(HMACContext);
}

bool HMACContext::is_available()
{
	return true;
}

Error HMACContext::start(HashingContext::HashType p_hash_type, const PackedByteArray& p_key)
{
	ERR_FAIL_COND_V_MSG(data->key_slot != PSA_KEY_ID_NULL, ERR_FILE_ALREADY_IN_USE,
		"HMACContext already started.");

	// HMAC keys can be any size.
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), ERR_INVALID_PARAMETER, "Key must not be empty.");

	bool allowed = Data::is_hash_type_allowed(p_hash_type);
	ERR_FAIL_COND_V_MSG(!allowed, ERR_INVALID_PARAMETER, "Unsupported hash type.");

	psa_algorithm_t ht =
		PSA_ALG_HMAC(MbedTLSModule::psa_alg_from_hashtype(p_hash_type, data->hash_len));
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&attr, ht);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
	int ret = psa_import_key(&attr, p_key.ptr(), p_key.size(), &data->key_slot);
	ERR_FAIL_COND_V_MSG(ret != PSA_SUCCESS, FAILED, "Error import key: " + itos(ret));

	ret = psa_mac_sign_setup(&data->mac_op, data->key_slot, ht);
	if (ret != PSA_SUCCESS) {
		data->clear();
		ERR_FAIL_V_MSG(FAILED, "Failed to setup HMAC signing: " + itos(ret));
	}
	return OK;
}

Error HMACContext::update(const PackedByteArray& p_data)
{
	ERR_FAIL_COND_V_MSG(data->key_slot == PSA_KEY_ID_NULL, ERR_INVALID_DATA,
		"Start must be called before update.");

	ERR_FAIL_COND_V_MSG(p_data.is_empty(), ERR_INVALID_PARAMETER, "Src must not be empty.");

	int ret = psa_mac_update(&data->mac_op, (const uint8_t*)p_data.ptr(), (size_t)p_data.size());
	ERR_FAIL_COND_V_MSG(ret != PSA_SUCCESS, FAILED, "Failed to update HMAC: " + itos(ret));
	return OK;
}

PackedByteArray HMACContext::finish()
{
	ERR_FAIL_COND_V_MSG(data->key_slot == PSA_KEY_ID_NULL, PackedByteArray(),
		"Start must be called before finish.");
	ERR_FAIL_COND_V_MSG(data->hash_len == 0, PackedByteArray(), "Unsupported hash type.");

	PackedByteArray out;
	out.resize(data->hash_len);
	size_t mac_length = 0;
	int ret = psa_mac_sign_finish(&data->mac_op, out.ptrw(), (size_t)out.size(), &mac_length);
	data->clear();
	ERR_FAIL_COND_V_MSG(ret, PackedByteArray(), "Error received while finishing HMAC");
	ERR_FAIL_COND_V(mac_length != (size_t)out.size(), PackedByteArray()); // Bug?
	return out;
}

void HMACContext::Data::clear()
{
	if (key_slot == PSA_KEY_ID_NULL) {
		return;
	}
	hash_len = 0;
	psa_mac_abort(&mac_op);
	mac_op = PSA_MAC_OPERATION_INIT;
	psa_destroy_key(key_slot);
	key_slot = PSA_KEY_ID_NULL;
}

/// Crypto & MbedTLSModule

namespace MbedTLSModule
{

void initialize_crypto() {}

void finalize_crypto()
{
	default_certs = nullptr;
}

Ref<X509Certificate> get_default_certificates()
{
	return default_certs;
}

psa_algorithm_t psa_alg_from_hashtype(HashingContext::HashType p_hash_type, int& r_size)
{
	switch (p_hash_type) {
	case HashingContext::HASH_MD5:
		r_size = 16;
		return PSA_ALG_MD5;
	case HashingContext::HASH_SHA1:
		r_size = 20;
		return PSA_ALG_SHA_1;
	case HashingContext::HASH_SHA256:
		r_size = 32;
		return PSA_ALG_SHA_256;
	default:
		r_size = 0;
		ERR_FAIL_V_MSG(PSA_ALG_NONE, "Invalid hash type.");
	}
}

mbedtls_md_type_t md_type_from_hashtype(HashingContext::HashType p_hash_type, int& r_size)
{
	switch (p_hash_type) {
	case HashingContext::HASH_MD5:
		r_size = 16;
		return MBEDTLS_MD_MD5;
	case HashingContext::HASH_SHA1:
		r_size = 20;
		return MBEDTLS_MD_SHA1;
	case HashingContext::HASH_SHA256:
		r_size = 32;
		return MBEDTLS_MD_SHA256;
	default:
		r_size = 0;
		ERR_FAIL_V_MSG(MBEDTLS_MD_NONE, "Invalid hash type.");
	}
}

} // namespace MbedTLSModule

Crypto* Crypto::create(bool p_notify_postinitialize)
{
	return memnew(Crypto);
}

bool Crypto::is_available()
{
	return true;
}

Crypto::Crypto() {}

Crypto::~Crypto() {}

void Crypto::load_default_certificates(const String& p_path)
{
	ERR_FAIL_COND(default_certs.is_valid());

	default_certs = memnew(X509Certificate);
	ERR_FAIL_COND(default_certs.is_null());

	if (!p_path.is_empty()) {
		// Use certs defined in project settings.
		default_certs->load(p_path);
	}
	else {
		// Try to use system certs otherwise.
		String system_certs = OS::get_system_ca_certificates();
		if (!system_certs.is_empty()) {
			CharString cs = system_certs.utf8();
			default_certs->load_from_memory((const uint8_t*)cs.get_data(), cs.size());
			print_verbose("Loaded system CA certificates");
		}
#ifdef BUILTIN_CERTS_ENABLED
		else {
			// Use builtin certs if there are no system certs.
			PackedByteArray certs;
			certs.resize(_certs_uncompressed_size + 1);
			const int64_t decompressed_size =
				Compression::decompress(certs.ptrw(), _certs_uncompressed_size, _certs_compressed,
					_certs_compressed_size, Compression::MODE_DEFLATE);
			ERR_FAIL_COND_MSG(decompressed_size != _certs_uncompressed_size,
				"Error decompressing builtin CA certificates. Decompressed size did not match "
				"expected size.");
			certs.write[_certs_uncompressed_size] = 0; // Make sure it ends with string terminator
			default_certs->load_from_memory(certs.ptr(), certs.size());
			print_verbose("Loaded builtin CA certificates");
		}
#endif
	}
}

Ref<CryptoKey> Crypto::generate_rsa(int p_bits)
{
	return CryptoKey::Data::generate(PSA_KEY_TYPE_RSA_KEY_PAIR, p_bits);
}

Ref<X509Certificate> Crypto::generate_self_signed_certificate(Ref<CryptoKey> p_key,
	const String& p_issuer_name, const String& p_not_before, const String& p_not_after)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), nullptr, "Invalid private key argument.");

	uint8_t rand_serial[20] = {};
	int ret = psa_generate_random(rand_serial, sizeof(rand_serial));
	ERR_FAIL_COND_V(ret, nullptr);

	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	ret = mbedtls_pk_copy_from_psa(p_key->get_data()->pk_slot, &ctx);
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(nullptr, "Failed to import private key: " + itos(ret));
	}

	mbedtls_x509write_cert crt;
	mbedtls_x509write_crt_init(&crt);
	mbedtls_x509write_crt_set_subject_key(&crt, &ctx);
	mbedtls_x509write_crt_set_issuer_key(&crt, &ctx);
	mbedtls_x509write_crt_set_subject_name(&crt, p_issuer_name.utf8().get_data());
	mbedtls_x509write_crt_set_issuer_name(&crt, p_issuer_name.utf8().get_data());
	mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
	mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
	mbedtls_x509write_crt_set_serial_raw(&crt, rand_serial, sizeof(rand_serial));
	mbedtls_x509write_crt_set_validity(
		&crt, p_not_before.utf8().get_data(), p_not_after.utf8().get_data());
	mbedtls_x509write_crt_set_basic_constraints(&crt, 1, 0);

	unsigned char buf[4096];
	memset(buf, 0, 4096);

	ret = mbedtls_x509write_crt_pem(&crt, buf, GODOT_MBEDTLS_COMPAT_ARGS(4096));
	mbedtls_x509write_crt_free(&crt);
	mbedtls_pk_free(&ctx);
	ERR_FAIL_COND_V_MSG(ret != 0, nullptr, "Failed to generate certificate: " + itos(ret));
	buf[4095] = '\0'; // Make sure strlen can't fail.

	Ref<X509Certificate> out;
	out.instantiate();
	out->load_from_memory(buf, strlen((char*)buf) + 1); // Use strlen to find correct output size.
	return out;
}

PackedByteArray Crypto::generate_random_bytes(int p_bytes)
{
	ERR_FAIL_COND_V(p_bytes < 0, PackedByteArray());
	PackedByteArray out;
	out.resize(p_bytes);
	int ret = psa_generate_random(out.ptrw(), out.size());
	ERR_FAIL_COND_V(ret != PSA_SUCCESS, PackedByteArray());
	return out;
}

Vector<uint8_t> Crypto::sign(
	HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash, Ref<CryptoKey> p_key)
{
	int size;
	psa_algorithm_t hash_type = MbedTLSModule::psa_alg_from_hashtype(p_hash_type, size);
	ERR_FAIL_COND_V_MSG(hash_type == PSA_ALG_NONE, Vector<uint8_t>(), "Invalid hash type.");
	ERR_FAIL_COND_V_MSG(p_hash.size() != size, Vector<uint8_t>(),
		"Invalid hash provided. Size must be " + itos(size));
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	ERR_FAIL_COND_V_MSG(p_key->is_public_only(), Vector<uint8_t>(),
		"Invalid key provided. Cannot sign with public_only keys.");
	psa_key_type_t key_type = p_key->get_data()->pk_type;
	psa_algorithm_t alg = PSA_ALG_NONE;
	if (PSA_KEY_TYPE_IS_RSA(key_type)) {
		alg = PSA_ALG_RSA_PKCS1V15_SIGN(hash_type);
	}
	else if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		alg = PSA_ALG_ECDSA(hash_type);
	}
	ERR_FAIL_COND_V_MSG(
		alg == PSA_ALG_NONE, Vector<uint8_t>(), "Unknown key type: " + itos(key_type));
	unsigned char buf[PSA_SIGNATURE_MAX_SIZE];
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_SIGN_HASH, alg);
	size_t sig_size = 0;
	int ret = psa_sign_hash(pk, alg, p_hash.ptr(), p_hash.size(), buf, sizeof(buf), &sig_size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while signing: " + itos(ret));

	Vector<uint8_t> out;
	if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		// Historically, mbedTLS used DER format for ECDA signatures.
		// We need to convert the output from raw to DER before returning it for compatility.
		size_t key_size = p_key->get_data()->pk_size;
		size_t out_size = 0;
		uint8_t der[GODOT_MBEDTLS_ECDSA_DER_MAX_LEN];
		ret = godot_mbedtls_ecdsa_raw_to_der(key_size, buf, sig_size, der, sizeof(der), &out_size);
		ERR_FAIL_COND_V_MSG(
			ret, Vector<uint8_t>(), "Failed to convert signature to DER format: " + itos(ret));
		out.resize(out_size);
		memcpy(out.ptrw(), der, out_size);
	}
	else {
		out.resize(sig_size);
		memcpy(out.ptrw(), buf, sig_size);
	}
	return out;
}

bool Crypto::verify(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	const Vector<uint8_t>& p_signature, Ref<CryptoKey> p_key)
{
	int size;
	psa_algorithm_t hash_type = MbedTLSModule::psa_alg_from_hashtype(p_hash_type, size);
	ERR_FAIL_COND_V_MSG(hash_type == PSA_ALG_NONE, false, "Invalid hash type.");
	ERR_FAIL_COND_V_MSG(
		p_hash.size() != size, false, "Invalid hash provided. Size must be " + itos(size));
	ERR_FAIL_COND_V_MSG(p_key.is_null(), false, "Invalid key provided.");
	psa_key_type_t key_type = p_key->get_data()->pk_type;
	psa_algorithm_t alg = PSA_ALG_NONE;
	Vector<uint8_t> sig;
	if (PSA_KEY_TYPE_IS_RSA(key_type)) {
		alg = PSA_ALG_RSA_PKCS1V15_SIGN(hash_type);
		sig = p_signature;
	}
	else if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		alg = PSA_ALG_ECDSA(hash_type);
		// Historically, mbedTLS used DER format for ECDA signatures.
		// We need to convert the input from DER to raw before returning it for compatility.
		size_t key_size = p_key->get_data()->pk_size;
		size_t out_size = 0;
		uint8_t raw_sig[PSA_SIGNATURE_MAX_SIZE];
		int ret = godot_mbedtls_ecdsa_der_to_raw(
			key_size, p_signature.ptr(), p_signature.size(), raw_sig, sizeof(raw_sig), &out_size);
		if (ret) {
			return false;
		}
		sig.resize(out_size);
		memcpy(sig.ptrw(), raw_sig, out_size);
	}
	ERR_FAIL_COND_V_MSG(alg == PSA_ALG_NONE, false, "Unknown key type: " + itos(key_type));
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_VERIFY_HASH, alg);
	int ret = psa_verify_hash(pk, alg, p_hash.ptr(), p_hash.size(), sig.ptr(), sig.size());
	psa_destroy_key(pk);
	return ret == PSA_SUCCESS;
}

Vector<uint8_t> Crypto::encrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_plaintext)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	uint8_t buf[PSA_ASYMMETRIC_ENCRYPT_OUTPUT_MAX_SIZE];
	size_t size;
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_RSA_PKCS1V15_CRYPT);
	int ret = psa_asymmetric_encrypt(pk, PSA_ALG_RSA_PKCS1V15_CRYPT, p_plaintext.ptr(),
		p_plaintext.size(), nullptr, 0, buf, sizeof(buf), &size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while encrypting: " + itos(ret));
	Vector<uint8_t> out;
	out.resize(size);
	memcpy(out.ptrw(), buf, size);
	return out;
}

Vector<uint8_t> Crypto::decrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_ciphertext)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	ERR_FAIL_COND_V_MSG(p_key->is_public_only(), Vector<uint8_t>(),
		"Invalid key provided. Cannot decrypt using a public_only key.");
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_DECRYPT, PSA_ALG_RSA_PKCS1V15_CRYPT);
	uint8_t buf[PSA_ASYMMETRIC_DECRYPT_OUTPUT_MAX_SIZE];
	size_t size;
	int ret = psa_asymmetric_decrypt(pk, PSA_ALG_RSA_PKCS1V15_CRYPT, p_ciphertext.ptr(),
		p_ciphertext.size(), nullptr, 0, buf, sizeof(buf), &size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while decrypting: " + itos(ret));
	Vector<uint8_t> out;
	out.resize(size);
	memcpy(out.ptrw(), buf, size);
	return out;
}

/// CookieContextMbedTLS & TLSContextMbedTLS

static void my_debug(void* ctx, int level, const char* file, int line, const char* str)
{
	printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}

void TLSContextMbedTLS::print_mbedtls_error(int p_ret)
{
	printf("mbedtls error: returned -0x%x\n\n", -p_ret);
	fflush(stdout);
}

Error CookieContextMbedTLS::setup()
{
	ERR_FAIL_COND_V_MSG(inited, ERR_ALREADY_IN_USE, "This cookie context is already in use");

	mbedtls_ssl_cookie_init(&cookie_ctx);
	inited = true;

	int ret = mbedtls_ssl_cookie_setup(GODOT_MBEDTLS_COMPAT_ARGS(&cookie_ctx));
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(FAILED, "mbedtls_ssl_cookie_setup returned an error " + itos(ret));
	}
	return OK;
}

void CookieContextMbedTLS::clear()
{
	if (!inited) {
		return;
	}
	mbedtls_ssl_cookie_free(&cookie_ctx);
	inited = false;
}

CookieContextMbedTLS::CookieContextMbedTLS() {}

CookieContextMbedTLS::~CookieContextMbedTLS() { clear(); }

Error TLSContextMbedTLS::_setup(int p_endpoint, int p_transport, int p_authmode)
{
	ERR_FAIL_COND_V_MSG(inited, ERR_ALREADY_IN_USE, "This SSL context is already active");

	mbedtls_ssl_init(&tls);
	mbedtls_ssl_config_init(&conf);
	mbedtls_pk_init(&pk);
	inited = true;

	int ret =
		mbedtls_ssl_config_defaults(&conf, p_endpoint, p_transport, MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(FAILED, "mbedtls_ssl_config_defaults returned an error" + itos(ret));
	}
	mbedtls_ssl_conf_authmode(&conf, p_authmode);
	GODOT_MBEDTLS_COMPAT_SSL_CONF;
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
	return OK;
}

Error TLSContextMbedTLS::init_server(
	int p_transport, Ref<TLSOptions> p_options, Ref<CookieContextMbedTLS> p_cookies)
{
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);

	// Check key and certificate(s)
	Ref<CryptoKey> pkey = p_options->get_private_key();
	certs = p_options->get_own_certificate();
	ERR_FAIL_COND_V(pkey.is_null() || certs.is_null(), ERR_INVALID_PARAMETER);

	Error err =
		_setup(MBEDTLS_SSL_IS_SERVER, p_transport, MBEDTLS_SSL_VERIFY_NONE); // TODO client auth.
	ERR_FAIL_COND_V(err != OK, err);

	// Locking key and certificate(s)
	certs->get_data()->lock();

	// Adding key and certificate
	int ret = mbedtls_pk_copy_from_psa(pkey->get_data()->pk_slot, &pk);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, "Failed to import key " + itos(ret));
	}
	ret = mbedtls_ssl_conf_own_cert(&conf, &certs->get_data()->cert, &pk);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, "Invalid cert/key combination " + itos(ret));
	}
	// Adding CA chain if available.
	if (certs->get_data()->cert.next) {
		mbedtls_ssl_conf_ca_chain(&conf, certs->get_data()->cert.next, nullptr);
	}
	// DTLS Cookies
	if (p_transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM) {
		if (p_cookies.is_null() || !p_cookies->inited) {
			clear();
			ERR_FAIL_V(ERR_BUG);
		}
		cookies = p_cookies;
		mbedtls_ssl_conf_dtls_cookies(
			&conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &(cookies->cookie_ctx));
	}

	mbedtls_ssl_setup(&tls, &conf);
	return OK;
}

Error TLSContextMbedTLS::init_client(
	int p_transport, const String& p_hostname, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_options.is_null() || p_options->is_server(), ERR_INVALID_PARAMETER);

	int authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
	bool unsafe = p_options->is_unsafe_client();
	if (unsafe && p_options->get_trusted_ca_chain().is_null()) {
		authmode = MBEDTLS_SSL_VERIFY_NONE;
	}

	Error err = _setup(MBEDTLS_SSL_IS_CLIENT, p_transport, authmode);
	ERR_FAIL_COND_V(err != OK, err);

	if (unsafe) {
		// No hostname verification for unsafe clients.
		mbedtls_ssl_set_hostname(&tls, nullptr);
	}
	else {
		String cn = p_options->get_common_name_override();
		if (cn.is_empty()) {
			cn = p_hostname;
		}
		mbedtls_ssl_set_hostname(&tls, cn.utf8().get_data());
	}

	Ref<X509Certificate> cas;

	if (p_options->get_trusted_ca_chain().is_valid()) {
		// Locking CA certificates
		certs = p_options->get_trusted_ca_chain();
		certs->get_data()->lock();
		cas = certs;
	}
	else {
		// Fall back to default certificates (no need to lock those).
		cas = MbedTLSModule::get_default_certificates();
		if (cas.is_null()) {
			clear();
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "SSL module failed to initialize!");
		}
	}

	// Set valid CAs
	mbedtls_ssl_conf_ca_chain(&conf, &cas->get_data()->cert, nullptr);
	mbedtls_ssl_setup(&tls, &conf);
	return OK;
}

void TLSContextMbedTLS::clear()
{
	if (!inited) {
		return;
	}
	mbedtls_ssl_free(&tls);
	mbedtls_ssl_config_free(&conf);
	mbedtls_pk_free(&pk);
	// Unlock certificates
	if (certs.is_valid()) {
		certs->get_data()->unlock();
	}
	certs = Ref<X509Certificate>();
	cookies = Ref<CookieContextMbedTLS>();
	inited = false;
}

mbedtls_ssl_context* TLSContextMbedTLS::get_context()
{
	ERR_FAIL_COND_V(!inited, nullptr);
	return &tls;
}

TLSContextMbedTLS::TLSContextMbedTLS() {}

TLSContextMbedTLS::~TLSContextMbedTLS() { clear(); }

/// StreamPeerTLS

int StreamPeerTLS::Data::bio_send(void* ctx, const unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	StreamPeerTLS::Data* sp = static_cast<StreamPeerTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int sent;
	Error err = sp->base->put_partial_data((const uint8_t*)buf, len, sent);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (sent == 0) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	return sent;
}

int StreamPeerTLS::Data::bio_recv(void* ctx, unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	StreamPeerTLS::Data* sp = static_cast<StreamPeerTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int got;
	Error err = sp->base->get_partial_data((uint8_t*)buf, len, got);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (got == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	return got;
}

void StreamPeerTLS::Data::cleanup()
{
	tls_ctx->clear();
	base = Ref<StreamPeer>();
	status = STATUS_DISCONNECTED;
}

Error StreamPeerTLS::Data::do_handshake(StreamPeerTLS* p_owner)
{
	int ret = mbedtls_ssl_handshake(tls_ctx->get_context());
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Handshake is still in progress, will retry via poll later.
		return OK;
	}
	else if (ret != 0) {
		// An error occurred.
		ERR_PRINT("TLS handshake error: " + itos(ret));
		TLSContextMbedTLS::print_mbedtls_error(ret);
		p_owner->disconnect_from_stream();
		status = STATUS_ERROR;
		return FAILED;
	}

	status = STATUS_CONNECTED;
	return OK;
}

Error StreamPeerTLS::connect_to_stream(
	Ref<StreamPeer> p_base, const String& p_common_name, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_client(MBEDTLS_SSL_TRANSPORT_STREAM, p_common_name,
		p_options.is_valid() ? p_options : TLSOptions::client());
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;
	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake(this) != OK) {
		data->status = STATUS_ERROR_HOSTNAME_MISMATCH;
		return FAILED;
	}

	return OK;
}

Error StreamPeerTLS::accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_server(MBEDTLS_SSL_TRANSPORT_STREAM, p_options);
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;

	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake(this) != OK) {
		return FAILED;
	}

	data->status = STATUS_CONNECTED;
	return OK;
}

Error StreamPeerTLS::put_data(const uint8_t* p_data, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	Error err;
	int sent = 0;

	while (p_bytes > 0) {
		err = put_partial_data(p_data, p_bytes, sent);

		if (err != OK) {
			return err;
		}

		p_data += sent;
		p_bytes -= sent;
	}

	return OK;
}

Error StreamPeerTLS::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_sent = 0;

	if (p_bytes == 0) {
		return OK;
	}

	do {
		int ret =
			mbedtls_ssl_write(data->tls_ctx->get_context(), &p_data[r_sent], p_bytes - r_sent);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// Non blocking IO.
			break;
		}
		else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Clean close
			disconnect_from_stream();
			return ERR_FILE_EOF;
		}
		else if (ret <= 0) {
			TLSContextMbedTLS::print_mbedtls_error(ret);
			disconnect_from_stream();
			return ERR_CONNECTION_ERROR;
		}
		r_sent += ret;

	} while (r_sent < p_bytes);

	return OK;
}

Error StreamPeerTLS::get_data(uint8_t* p_buffer, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	Error err;

	int got = 0;
	while (p_bytes > 0) {
		err = get_partial_data(p_buffer, p_bytes, got);

		if (err != OK) {
			return err;
		}

		p_buffer += got;
		p_bytes -= got;
	}

	return OK;
}

Error StreamPeerTLS::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_received = 0;

	do {
		int ret = mbedtls_ssl_read(
			data->tls_ctx->get_context(), &p_buffer[r_received], p_bytes - r_received);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// Non blocking IO.
			break;
		}
		else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Clean close
			disconnect_from_stream();
			return ERR_FILE_EOF;
		}
		else if (ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
			continue; // Re-connection cookies are not currently supported, ignore them.
		}
		else if (ret <= 0) {
			TLSContextMbedTLS::print_mbedtls_error(ret);
			disconnect_from_stream();
			return ERR_CONNECTION_ERROR;
		}

		r_received += ret;

	} while (r_received < p_bytes);

	return OK;
}

void StreamPeerTLS::poll()
{
	ERR_FAIL_COND(data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING);
	ERR_FAIL_COND(data->base.is_null());

	if (data->status == STATUS_HANDSHAKING) {
		data->do_handshake(this);
		return;
	}

	// We could pass nullptr as second parameter, but some behavior sanitizers don't seem to like
	// that. Passing a 1 byte buffer to workaround it.
	uint8_t byte;
	int ret = mbedtls_ssl_read(data->tls_ctx->get_context(), &byte, 0);

	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Nothing to read/write (non blocking IO)
	}
	else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		// Clean close (disconnect)
		disconnect_from_stream();
		return;
	}
	else if (ret < 0) {
		TLSContextMbedTLS::print_mbedtls_error(ret);
		disconnect_from_stream();
		return;
	}

	Ref<StreamPeerSocket> tcp = data->base;
	if (tcp.is_valid() && tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		disconnect_from_stream();
		return;
	}
}

int StreamPeerTLS::get_available_bytes() const
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, 0);

	return mbedtls_ssl_get_bytes_avail(data->tls_ctx->get_context());
}

StreamPeerTLS::StreamPeerTLS()
{
	data = memnew(Data);
}

StreamPeerTLS::~StreamPeerTLS()
{
	if (data) {
		disconnect_from_stream();
		memdelete(data);
		data = nullptr;
	}
}

void StreamPeerTLS::disconnect_from_stream()
{
	if (data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING) {
		return;
	}

	Ref<StreamPeerSocket> tcp = data->base;
	if (tcp.is_valid() && tcp->get_status() == StreamPeerSocket::STATUS_CONNECTED) {
		// We are still connected on the socket, try to send close notify.
		mbedtls_ssl_close_notify(data->tls_ctx->get_context());
	}

	data->cleanup();
}

StreamPeerTLS::Status StreamPeerTLS::get_status() const
{
	return data->status;
}

Ref<StreamPeer> StreamPeerTLS::get_stream() const
{
	return data->base;
}

StreamPeerTLS* StreamPeerTLS::create(bool p_notify_postinitialize)
{
	return memnew(StreamPeerTLS);
}

bool StreamPeerTLS::is_available()
{
	return true;
}

/// PacketPeerDTLS

int PacketPeerDTLS::Data::bio_send(void* ctx, const unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	PacketPeerDTLS::Data* sp = static_cast<PacketPeerDTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	Error err = sp->base->put_packet((const uint8_t*)buf, len);
	if (err == ERR_BUSY) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	else if (err != OK) {
		ERR_FAIL_V(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
	}
	return len;
}

int PacketPeerDTLS::Data::bio_recv(void* ctx, unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	PacketPeerDTLS::Data* sp = static_cast<PacketPeerDTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int pc = sp->base->get_available_packet_count();
	if (pc == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	else if (pc < 0) {
		ERR_FAIL_V(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
	}

	const uint8_t* buffer;
	int buffer_size = 0;
	Error err = sp->base->get_packet(&buffer, buffer_size);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	memcpy(buf, buffer, buffer_size);
	return buffer_size;
}

void PacketPeerDTLS::Data::cleanup()
{
	tls_ctx->clear();
	base = Ref<PacketPeerUDP>();
	status = STATUS_DISCONNECTED;
}

int PacketPeerDTLS::Data::set_cookie()
{
	// Setup DTLS session cookie for this client
	uint8_t client_id[18];
	IPAddress addr = base->get_packet_address();
	uint16_t port = base->get_packet_port();
	memcpy(client_id, addr.get_ipv6(), 16);
	memcpy(&client_id[16], (uint8_t*)&port, 2);
	return mbedtls_ssl_set_client_transport_id(tls_ctx->get_context(), client_id, 18);
}

Error PacketPeerDTLS::Data::do_handshake()
{
	int ret = 0;
	while ((ret = mbedtls_ssl_handshake(tls_ctx->get_context())) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret != MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
				ERR_PRINT("TLS handshake error: " + itos(ret));
				TLSContextMbedTLS::print_mbedtls_error(ret);
			}
			cleanup();
			status = STATUS_ERROR;
			return FAILED;
		}
		// Will retry via poll later
		return OK;
	}

	status = STATUS_CONNECTED;
	return OK;
}

Error PacketPeerDTLS::connect_to_peer(
	Ref<PacketPeerUDP> p_base, const String& p_hostname, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null() || !p_base->is_socket_connected(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_client(MBEDTLS_SSL_TRANSPORT_DATAGRAM, p_hostname,
		p_options.is_valid() ? p_options : TLSOptions::client());
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;

	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);
	mbedtls_ssl_set_timer_cb(
		data->tls_ctx->get_context(), &data->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake() != OK) {
		data->status = STATUS_ERROR_HOSTNAME_MISMATCH;
		return FAILED;
	}

	return OK;
}

Error PacketPeerDTLS::Data::accept_peer(
	Ref<PacketPeerUDP> p_base, Ref<TLSOptions> p_options, Ref<CookieContextMbedTLS> p_cookies)
{
	ERR_FAIL_COND_V(p_base.is_null() || !p_base->is_socket_connected(), ERR_INVALID_PARAMETER);

	Error err = tls_ctx->init_server(MBEDTLS_SSL_TRANSPORT_DATAGRAM, p_options, p_cookies);
	ERR_FAIL_COND_V(err != OK, err);

	base = p_base;
	base->set_blocking_mode(false);

	mbedtls_ssl_session_reset(tls_ctx->get_context());

	int ret = set_cookie();
	if (ret != 0) {
		cleanup();
		ERR_FAIL_V_MSG(FAILED, "Error setting DTLS client cookie");
	}

	mbedtls_ssl_set_bio(tls_ctx->get_context(), this, bio_send, bio_recv, nullptr);
	mbedtls_ssl_set_timer_cb(
		tls_ctx->get_context(), &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	status = STATUS_HANDSHAKING;

	if (do_handshake() != OK) {
		status = STATUS_ERROR;
		return FAILED;
	}

	return OK;
}

Error PacketPeerDTLS::put_packet(const uint8_t* p_buffer, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	if (p_bytes == 0) {
		return OK;
	}

	int ret = mbedtls_ssl_write(data->tls_ctx->get_context(), p_buffer, p_bytes);
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Non blocking io.
	}
	else if (ret <= 0) {
		TLSContextMbedTLS::print_mbedtls_error(ret);
		data->cleanup();
		return ERR_CONNECTION_ERROR;
	}

	return OK;
}

Error PacketPeerDTLS::get_packet(const uint8_t** r_buffer, int& r_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_bytes = 0;

	int ret = mbedtls_ssl_read(
		data->tls_ctx->get_context(), data->packet_buffer, Data::PACKET_BUFFER_SIZE);
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		ret = 0; // non blocking io
	}
	else if (ret <= 0) {
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Also send close notify back
			disconnect_from_peer();
		}
		else {
			data->cleanup();
			data->status = STATUS_ERROR;
			TLSContextMbedTLS::print_mbedtls_error(ret);
		}
		return ERR_CONNECTION_ERROR;
	}
	*r_buffer = data->packet_buffer;
	r_bytes = ret;

	return OK;
}

void PacketPeerDTLS::poll()
{
	if (data->status == STATUS_HANDSHAKING) {
		data->do_handshake();
		return;
	}
	else if (data->status != STATUS_CONNECTED) {
		return;
	}

	ERR_FAIL_COND(data->base.is_null());

	int ret = mbedtls_ssl_read(data->tls_ctx->get_context(), nullptr, 0);

	if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Also send close notify back
			disconnect_from_peer();
		}
		else {
			data->cleanup();
			data->status = STATUS_ERROR;
			TLSContextMbedTLS::print_mbedtls_error(ret);
		}
	}
}

int PacketPeerDTLS::get_available_packet_count() const
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, 0);

	return mbedtls_ssl_get_bytes_avail(data->tls_ctx->get_context()) > 0 ? 1 : 0;
}

int PacketPeerDTLS::get_max_packet_size() const
{
	return 488; // 512 (UDP in Godot) - 24 (DTLS header)
}

PacketPeerDTLS::PacketPeerDTLS()
{
	data = memnew(Data);
}

PacketPeerDTLS::~PacketPeerDTLS()
{
	if (data) {
		disconnect_from_peer();
		memdelete(data);
		data = nullptr;
	}
}

void PacketPeerDTLS::disconnect_from_peer()
{
	if (data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING) {
		return;
	}

	if (data->status == STATUS_CONNECTED) {
		int ret = 0;
		// Send SSL close notification, blocking, but ignore other errors.
		do {
			ret = mbedtls_ssl_close_notify(data->tls_ctx->get_context());
		} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
	}

	data->cleanup();
}

PacketPeerDTLS::Status PacketPeerDTLS::get_status() const
{
	return data->status;
}

PacketPeerDTLS* PacketPeerDTLS::create(bool p_notify_postinitialize)
{
	return memnew(PacketPeerDTLS);
}

bool PacketPeerDTLS::is_available()
{
	return true;
}

/// DTLSServer

Error DTLSServer::setup(Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(data->cookies->setup() != OK, ERR_ALREADY_IN_USE);
	data->tls_options = p_options;
	return OK;
}

void DTLSServer::stop()
{
	data->cookies->clear();
}

Ref<PacketPeerDTLS> DTLSServer::take_connection(Ref<PacketPeerUDP> p_udp_peer)
{
	Ref<PacketPeerDTLS> out;

	ERR_FAIL_COND_V(data->tls_options.is_null(), out);
	ERR_FAIL_COND_V(p_udp_peer.is_null(), out);

	out.instantiate();
	out->get_data()->accept_peer(p_udp_peer, data->tls_options, data->cookies);
	return out;
}

DTLSServer* DTLSServer::create(bool p_notify_postinitialize)
{
	return memnew(DTLSServer);
}

bool DTLSServer::is_available()
{
	return true;
}

DTLSServer::DTLSServer()
{
	data = memnew(Data);
}

DTLSServer::~DTLSServer()
{
	if (data) {
		stop();
		memdelete(data);
		data = nullptr;
	}
}

/// Module registration

static bool godot_mbedtls_initialized = false;

void initialize_mbedtls_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

#ifdef DEBUG_ENABLED
	if (OS::is_stdout_verbose()) {
		mbedtls_debug_set_threshold(1);
	}
#endif

	godot_mbedtls_initialized = true;

	MbedTLSModule::initialize_crypto();
}

void uninitialize_mbedtls_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	if (!godot_mbedtls_initialized) {
		return;
	}

	MbedTLSModule::finalize_crypto();
	godot_mbedtls_initialized = false;
}


// --- Merged from modules/mbedtls/mbedtls_cpp_compat.c ---


int godot_mbedtls_ecdsa_raw_to_der(size_t bits, const unsigned char *raw, size_t raw_len, unsigned char *der, size_t der_size, size_t *der_len) {
	return mbedtls_ecdsa_raw_to_der(bits, raw, raw_len, der, der_size, der_len);
}
int godot_mbedtls_ecdsa_der_to_raw(size_t bits, const unsigned char *der, size_t der_len, unsigned char *raw, size_t raw_size, size_t *raw_len) {
	return mbedtls_ecdsa_der_to_raw(bits, der, der_len, raw, raw_size, raw_len);
}
#endif // defined(MODULE_MBEDTLS_ENABLED)


// --- Merged from modules/upnp/upnp.cpp ---
#if defined(MODULE_UPNP_ENABLED)
// --- Inlined from modules/upnp/upnp.h ---
#include "core/types.h"

struct UPNPDev;

class UPNPDevice final : public RefCounted
{
public:
	enum IGDStatus
	{
		IGD_STATUS_OK,
		IGD_STATUS_HTTP_ERROR,
		IGD_STATUS_HTTP_EMPTY,
		IGD_STATUS_NO_URLS,
		IGD_STATUS_NO_IGD,
		IGD_STATUS_DISCONNECTED,
		IGD_STATUS_UNKNOWN_DEVICE,
		IGD_STATUS_INVALID_CONTROL,
		IGD_STATUS_MALLOC_ERROR,
		IGD_STATUS_UNKNOWN_ERROR,
	};

private:
	String description_url;
	String service_type;
	String igd_control_url;
	String igd_service_type;
	String igd_our_addr;
	IGDStatus igd_status = IGD_STATUS_UNKNOWN_ERROR;

public:
	static UPNPDevice* create(bool p_notify_postinitialize = true)
	{
		return memnew(UPNPDevice);
	}

	void set_description_url(const String& url);
	String get_description_url() const;

	void set_service_type(const String& type);
	String get_service_type() const;

	void set_igd_control_url(const String& url);
	String get_igd_control_url() const;

	void set_igd_service_type(const String& type);
	String get_igd_service_type() const;

	void set_igd_our_addr(const String& addr);
	String get_igd_our_addr() const;

	void set_igd_status(IGDStatus status);
	IGDStatus get_igd_status() const;

	bool is_valid_gateway() const;
	String query_external_address() const;
	int add_port_mapping(int port, int port_internal = 0, String desc = "",
		String proto = "UDP", int duration = 0) const;
	int delete_port_mapping(int port, String proto = "UDP") const;

	UPNPDevice() {}
	~UPNPDevice() {}
};

class UPNP final : public RefCounted
{
private:
	String discover_multicast_if = "";
	int discover_local_port = 0;
	bool discover_ipv6 = false;

	Vector<Ref<UPNPDevice>> devices;

	bool is_common_device(const String& dev) const;
	void add_device_to_list(UPNPDev* dev, UPNPDev* devlist);
	void parse_igd(Ref<UPNPDevice> dev, UPNPDev* devlist);
	char* load_description(const String& url, int* size, int* status_code) const;

public:
	enum UPNPResult
	{
		UPNP_RESULT_SUCCESS,
		UPNP_RESULT_NOT_AUTHORIZED,
		UPNP_RESULT_PORT_MAPPING_NOT_FOUND,
		UPNP_RESULT_INCONSISTENT_PARAMETERS,
		UPNP_RESULT_NO_SUCH_ENTRY_IN_ARRAY,
		UPNP_RESULT_ACTION_FAILED,
		UPNP_RESULT_SRC_IP_WILDCARD_NOT_PERMITTED,
		UPNP_RESULT_EXT_PORT_WILDCARD_NOT_PERMITTED,
		UPNP_RESULT_INT_PORT_WILDCARD_NOT_PERMITTED,
		UPNP_RESULT_REMOTE_HOST_MUST_BE_WILDCARD,
		UPNP_RESULT_EXT_PORT_MUST_BE_WILDCARD,
		UPNP_RESULT_NO_PORT_MAPS_AVAILABLE,
		UPNP_RESULT_CONFLICT_WITH_OTHER_MECHANISM,
		UPNP_RESULT_CONFLICT_WITH_OTHER_MAPPING,
		UPNP_RESULT_SAME_PORT_VALUES_REQUIRED,
		UPNP_RESULT_ONLY_PERMANENT_LEASE_SUPPORTED,
		UPNP_RESULT_INVALID_GATEWAY,
		UPNP_RESULT_INVALID_PORT,
		UPNP_RESULT_INVALID_PROTOCOL,
		UPNP_RESULT_INVALID_DURATION,
		UPNP_RESULT_INVALID_ARGS,
		UPNP_RESULT_INVALID_RESPONSE,
		UPNP_RESULT_INVALID_PARAM,
		UPNP_RESULT_HTTP_ERROR,
		UPNP_RESULT_SOCKET_ERROR,
		UPNP_RESULT_MEM_ALLOC_ERROR,
		UPNP_RESULT_NO_GATEWAY,
		UPNP_RESULT_NO_DEVICES,
		UPNP_RESULT_UNKNOWN_ERROR,
	};

	static UPNP* create(bool p_notify_postinitialize = true)
	{
		return memnew(UPNP);
	}

	static int upnp_result(int in);

	int get_device_count() const;
	Ref<UPNPDevice> get_device(int index) const;
	void add_device(Ref<UPNPDevice> device);
	void set_device(int index, Ref<UPNPDevice> device);
	void remove_device(int index);
	void clear_devices();

	Ref<UPNPDevice> get_gateway() const;

	int discover(
		int timeout = 2000, int ttl = 2, const String& device_filter = "InternetGatewayDevice");

	String query_external_address() const;

	int add_port_mapping(int port, int port_internal = 0, String desc = "",
		String proto = "UDP", int duration = 0) const;
	int delete_port_mapping(int port, String proto = "UDP") const;

	void set_discover_multicast_if(const String& m_if);
	String get_discover_multicast_if() const;

	void set_discover_local_port(int port);
	int get_discover_local_port() const;

	void set_discover_ipv6(bool ipv6);
	bool is_discover_ipv6() const;

	UPNP() {}
	~UPNP() {}
};

// --- End modules/upnp/upnp.h ---
#include "modules/register_module_types.h"

#ifndef WEB_ENABLED
#include <cstdlib>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#endif

// UPNPDevice

String UPNPDevice::query_external_address() const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(!is_valid_gateway(), "", "The Internet Gateway Device must be valid.");

	char addr[16];
	int i = UPNP_GetExternalIPAddress(
		igd_control_url.utf8().get_data(), igd_service_type.utf8().get_data(), (char*)&addr);

	ERR_FAIL_COND_V_MSG(i != UPNPCOMMAND_SUCCESS, "", "Couldn't get external IP address.");

	return String(addr);
#else
	return String();
#endif
}

int UPNPDevice::add_port_mapping(
	int port, int port_internal, String desc, String proto, int duration) const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(!is_valid_gateway(), UPNP::UPNP_RESULT_INVALID_GATEWAY,
		"The Internet Gateway Device must be valid.");
	ERR_FAIL_COND_V_MSG(port < 1 || port > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 1 and 65535 (inclusive).");
	ERR_FAIL_COND_V_MSG(port_internal < 0 || port_internal > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 0 and 65535 (inclusive)."); // Needs to allow 0 because
																		 // 0 signifies "use
																		 // external port as
																		 // internal port"
	ERR_FAIL_COND_V_MSG(proto != "UDP" && proto != "TCP", UPNP::UPNP_RESULT_INVALID_PROTOCOL,
		"The protocol must be either TCP or UDP.");
	ERR_FAIL_COND_V_MSG(duration < 0, UPNP::UPNP_RESULT_INVALID_DURATION,
		"The port mapping's lease duration can't be negative.");

	if (port_internal < 1) {
		port_internal = port;
	}

	int i = UPNP_AddPortMapping(igd_control_url.utf8().get_data(),
		igd_service_type.utf8().get_data(), itos(port).utf8().get_data(),
		itos(port_internal).utf8().get_data(), igd_our_addr.utf8().get_data(),
		desc.is_empty() ? nullptr : desc.utf8().get_data(), proto.utf8().get_data(),
		nullptr, // Remote host, always nullptr as IGDs don't support it
		duration > 0 ? itos(duration).utf8().get_data() : nullptr);

	ERR_FAIL_COND_V_MSG(
		i != UPNPCOMMAND_SUCCESS, UPNP::upnp_result(i), "Couldn't add port mapping.");

	return UPNP::UPNP_RESULT_SUCCESS;
#else
	return UPNP::UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

int UPNPDevice::delete_port_mapping(int port, String proto) const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(port < 1 || port > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 1 and 65535 (inclusive).");
	ERR_FAIL_COND_V_MSG(proto != "UDP" && proto != "TCP", UPNP::UPNP_RESULT_INVALID_PROTOCOL,
		"The protocol must be either TCP or UDP.");

	int i = UPNP_DeletePortMapping(igd_control_url.utf8().get_data(),
		igd_service_type.utf8().get_data(), itos(port).utf8().get_data(), proto.utf8().get_data(),
		nullptr // Remote host, always nullptr as IGDs don't support it
	);

	ERR_FAIL_COND_V_MSG(
		i != UPNPCOMMAND_SUCCESS, UPNP::upnp_result(i), "Couldn't delete port mapping.");

	return UPNP::UPNP_RESULT_SUCCESS;
#else
	return UPNP::UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

void UPNPDevice::set_description_url(const String& url) { description_url = url; }

String UPNPDevice::get_description_url() const { return description_url; }

void UPNPDevice::set_service_type(const String& type) { service_type = type; }

String UPNPDevice::get_service_type() const { return service_type; }

void UPNPDevice::set_igd_control_url(const String& url) { igd_control_url = url; }

String UPNPDevice::get_igd_control_url() const { return igd_control_url; }

void UPNPDevice::set_igd_service_type(const String& type) { igd_service_type = type; }

String UPNPDevice::get_igd_service_type() const { return igd_service_type; }

void UPNPDevice::set_igd_our_addr(const String& addr) { igd_our_addr = addr; }

String UPNPDevice::get_igd_our_addr() const { return igd_our_addr; }

void UPNPDevice::set_igd_status(IGDStatus status) { igd_status = status; }

UPNPDevice::IGDStatus UPNPDevice::get_igd_status() const { return igd_status; }

bool UPNPDevice::is_valid_gateway() const { return igd_status == IGD_STATUS_OK; }

// UPNP

bool UPNP::is_common_device(const String& dev) const
{
	return dev.is_empty() || dev.contains("InternetGatewayDevice") ||
		   dev.contains("WANIPConnection") || dev.contains("WANPPPConnection") ||
		   dev.contains("rootdevice");
}

int UPNP::discover(int timeout, int ttl, const String& device_filter)
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(
		timeout < 0, UPNP_RESULT_INVALID_PARAM, "The response's wait time can't be negative.");
	ERR_FAIL_COND_V_MSG(ttl < 0 || ttl > 255, UPNP_RESULT_INVALID_PARAM,
		"The time-to-live must be set between 0 and 255 (inclusive).");

	devices.clear();

	int error = 0;
	struct UPNPDev* devlist;

	CharString cs = discover_multicast_if.utf8();
	const char* m_if = cs.length() ? cs.get_data() : nullptr;
	if (is_common_device(device_filter)) {
		devlist =
			upnpDiscover(timeout, m_if, nullptr, discover_local_port, discover_ipv6, ttl, &error);
	}
	else {
		devlist = upnpDiscoverAll(
			timeout, m_if, nullptr, discover_local_port, discover_ipv6, ttl, &error);
	}

	if (error != UPNPDISCOVER_SUCCESS) {
		switch (error) {
		case UPNPDISCOVER_SOCKET_ERROR:
			return UPNP_RESULT_SOCKET_ERROR;
		case UPNPDISCOVER_MEMORY_ERROR:
			return UPNP_RESULT_MEM_ALLOC_ERROR;
		default:
			return UPNP_RESULT_UNKNOWN_ERROR;
		}
	}

	if (!devlist) {
		return UPNP_RESULT_NO_DEVICES;
	}

	struct UPNPDev* dev = devlist;

	while (dev) {
		if (device_filter.is_empty() || strstr(dev->st, device_filter.utf8().get_data())) {
			add_device_to_list(dev, devlist);
		}

		dev = dev->pNext;
	}

	freeUPNPDevlist(devlist);

	return UPNP_RESULT_SUCCESS;
#else
	return UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

void UPNP::add_device_to_list(UPNPDev* dev, UPNPDev* devlist)
{
#ifndef WEB_ENABLED
	Ref<UPNPDevice> new_device;
	new_device.instantiate();

	new_device->set_description_url(dev->descURL);
	new_device->set_service_type(dev->st);

	parse_igd(new_device, devlist);

	devices.push_back(new_device);
#endif
}

char* UPNP::load_description(const String& url, int* size, int* status_code) const
{
#ifndef WEB_ENABLED
	return (char*)miniwget(url.utf8().get_data(), size, 0, status_code);
#else
	return nullptr;
#endif
}

void UPNP::parse_igd(Ref<UPNPDevice> dev, UPNPDev* devlist)
{
#ifndef WEB_ENABLED
	int size = 0;
	int status_code = -1;
	char* xml = load_description(dev->get_description_url(), &size, &status_code);

	if (status_code != 200) {
		dev->set_igd_status(UPNPDevice::IGD_STATUS_HTTP_ERROR);
		return;
	}

	if (!xml || size < 1) {
		dev->set_igd_status(UPNPDevice::IGD_STATUS_HTTP_EMPTY);
		return;
	}

	struct UPNPUrls urls = {};
	struct IGDdatas data;

	parserootdesc(xml, size, &data);
	free(xml);
	xml = nullptr;

	GetUPNPUrls(&urls, &data, dev->get_description_url().utf8().get_data(), 0);

	char addr[16];
#if MINIUPNPC_API_VERSION >= 18
	int i = UPNP_GetValidIGD(devlist, &urls, &data, (char*)&addr, 16, nullptr, 0);
#else
	int i = UPNP_GetValidIGD(devlist, &urls, &data, (char*)&addr, 16);
#endif

	if (i != 1) {
		FreeUPNPUrls(&urls);

		switch (i) {
		case 0:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_NO_IGD);
			return;
		case 2:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_DISCONNECTED);
			return;
		case 3:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_UNKNOWN_DEVICE);
			return;
		default:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_UNKNOWN_ERROR);
			return;
		}
	}

	if (urls.controlURL[0] == '\0') {
		FreeUPNPUrls(&urls);
		dev->set_igd_status(UPNPDevice::IGD_STATUS_INVALID_CONTROL);
		return;
	}

	dev->set_igd_control_url(urls.controlURL);
	dev->set_igd_service_type(data.first.servicetype);
	dev->set_igd_our_addr(addr);
	dev->set_igd_status(UPNPDevice::IGD_STATUS_OK);

	FreeUPNPUrls(&urls);
#endif
}

int UPNP::upnp_result(int in)
{
#ifndef WEB_ENABLED
	switch (in) {
	case UPNPCOMMAND_SUCCESS:
		return UPNP_RESULT_SUCCESS;
	case UPNPCOMMAND_UNKNOWN_ERROR:
		return UPNP_RESULT_UNKNOWN_ERROR;
	case UPNPCOMMAND_INVALID_ARGS:
		return UPNP_RESULT_INVALID_ARGS;
	case UPNPCOMMAND_HTTP_ERROR:
		return UPNP_RESULT_HTTP_ERROR;
	case UPNPCOMMAND_INVALID_RESPONSE:
		return UPNP_RESULT_INVALID_RESPONSE;
	case UPNPCOMMAND_MEM_ALLOC_ERROR:
		return UPNP_RESULT_MEM_ALLOC_ERROR;

	case 402:
		return UPNP_RESULT_INVALID_ARGS;
	case 403:
		return UPNP_RESULT_NOT_AUTHORIZED;
	case 501:
		return UPNP_RESULT_ACTION_FAILED;
	case 606:
		return UPNP_RESULT_NOT_AUTHORIZED;
	case 714:
		return UPNP_RESULT_NO_SUCH_ENTRY_IN_ARRAY;
	case 715:
		return UPNP_RESULT_SRC_IP_WILDCARD_NOT_PERMITTED;
	case 716:
		return UPNP_RESULT_EXT_PORT_WILDCARD_NOT_PERMITTED;
	case 718:
		return UPNP_RESULT_CONFLICT_WITH_OTHER_MAPPING;
	case 724:
		return UPNP_RESULT_SAME_PORT_VALUES_REQUIRED;
	case 725:
		return UPNP_RESULT_ONLY_PERMANENT_LEASE_SUPPORTED;
	case 726:
		return UPNP_RESULT_REMOTE_HOST_MUST_BE_WILDCARD;
	case 727:
		return UPNP_RESULT_EXT_PORT_MUST_BE_WILDCARD;
	case 728:
		return UPNP_RESULT_NO_PORT_MAPS_AVAILABLE;
	case 729:
		return UPNP_RESULT_CONFLICT_WITH_OTHER_MECHANISM;
	case 732:
		return UPNP_RESULT_INT_PORT_WILDCARD_NOT_PERMITTED;
	case 733:
		return UPNP_RESULT_INCONSISTENT_PARAMETERS;
	}
#endif

	return UPNP_RESULT_UNKNOWN_ERROR;
}

int UPNP::get_device_count() const { return devices.size(); }

Ref<UPNPDevice> UPNP::get_device(int index) const
{
	ERR_FAIL_INDEX_V(index, devices.size(), nullptr);

	return devices.get(index);
}

void UPNP::add_device(Ref<UPNPDevice> device)
{
	ERR_FAIL_COND(device.is_null());

	devices.push_back(device);
}

void UPNP::set_device(int index, Ref<UPNPDevice> device)
{
	ERR_FAIL_INDEX(index, devices.size());
	ERR_FAIL_COND(device.is_null());

	devices.set(index, device);
}

void UPNP::remove_device(int index)
{
	ERR_FAIL_INDEX(index, devices.size());

	devices.remove_at(index);
}

void UPNP::clear_devices() { devices.clear(); }

Ref<UPNPDevice> UPNP::get_gateway() const
{
	ERR_FAIL_COND_V_MSG(devices.is_empty(), nullptr, "Couldn't find any UPNPDevices.");

	for (int i = 0; i < devices.size(); i++) {
		Ref<UPNPDevice> dev = get_device(i);

		if (dev.is_valid() && dev->is_valid_gateway()) {
			return dev;
		}
	}

	return nullptr;
}

void UPNP::set_discover_multicast_if(const String& m_if) { discover_multicast_if = m_if; }

String UPNP::get_discover_multicast_if() const { return discover_multicast_if; }

void UPNP::set_discover_local_port(int port) { discover_local_port = port; }

int UPNP::get_discover_local_port() const { return discover_local_port; }

void UPNP::set_discover_ipv6(bool ipv6) { discover_ipv6 = ipv6; }

bool UPNP::is_discover_ipv6() const { return discover_ipv6; }

String UPNP::query_external_address() const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return "";
	}

	return dev->query_external_address();
}

int UPNP::add_port_mapping(
	int port, int port_internal, String desc, String proto, int duration) const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return UPNP_RESULT_NO_GATEWAY;
	}

	return dev->add_port_mapping(port, port_internal, desc, proto, duration);
}

int UPNP::delete_port_mapping(int port, String proto) const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return UPNP_RESULT_NO_GATEWAY;
	}

	return dev->delete_port_mapping(port, proto);
}

// Module registration

void initialize_upnp_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_upnp_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_UPNP_ENABLED)


// --- Merged from modules/webrtc/webrtc_peer_connection.cpp ---
#if defined(MODULE_WEBRTC_ENABLED)
// --- Inlined from modules/webrtc/webrtc_peer_connection.h ---
#include "core/io/packet_peer.h"
#include "core/types.h"
#include "scene/main/scene_tree.h"

class WebRTCDataChannel final : public PacketPeer
{
public:
	enum WriteMode
	{
		WRITE_MODE_TEXT,
		WRITE_MODE_BINARY,
	};

	enum ChannelState
	{
		STATE_CONNECTING,
		STATE_OPEN,
		STATE_CLOSING,
		STATE_CLOSED
	};

private:
	unsigned int _in_buffer_shift = 16;
	WriteMode _write_mode = WRITE_MODE_BINARY;

#ifdef WEB_ENABLED
	String _label;
	String _protocol;

	bool _was_string = false;

	enum
	{
		PACKET_BUFFER_SIZE = 65536 - 5 // 4 bytes for the size, 1 for type
	};

	int _js_id = 0;
	RingBuffer<uint8_t> in_buffer;
	int queue_count = 0;
	uint8_t packet_buffer[PACKET_BUFFER_SIZE];

	static void _on_open(void* p_obj);
	static void _on_close(void* p_obj);
	static void _on_error(void* p_obj);
	static void _on_message(void* p_obj, const uint8_t* p_data, int p_size, int p_is_string);
#endif

public:
	void set_write_mode(WriteMode mode);
	WriteMode get_write_mode() const;
	bool was_string_packet() const;

	ChannelState get_ready_state() const;
	String get_label() const;
	bool is_ordered() const;
	int get_id() const;
	int get_max_packet_life_time() const;
	int get_max_retransmits() const;
	String get_protocol() const;
	bool is_negotiated() const;
	int get_buffered_amount() const;

	Error poll();
	void close();

	// PacketPeer
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;
	virtual int get_max_packet_size() const override;

	WebRTCDataChannel();
#ifdef WEB_ENABLED
	WebRTCDataChannel(int js_id);
#endif
	~WebRTCDataChannel();
};

class WebRTCPeerConnection final : public RefCounted
{
public:
	enum ConnectionState
	{
		STATE_NEW,
		STATE_CONNECTING,
		STATE_CONNECTED,
		STATE_DISCONNECTED,
		STATE_FAILED,
		STATE_CLOSED
	};

	enum GatheringState
	{
		GATHERING_STATE_NEW,
		GATHERING_STATE_GATHERING,
		GATHERING_STATE_COMPLETE,
	};

	enum SignalingState
	{
		SIGNALING_STATE_STABLE,
		SIGNALING_STATE_HAVE_LOCAL_OFFER,
		SIGNALING_STATE_HAVE_REMOTE_OFFER,
		SIGNALING_STATE_HAVE_LOCAL_PRANSWER,
		SIGNALING_STATE_HAVE_REMOTE_PRANSWER,
		SIGNALING_STATE_CLOSED,
	};

private:
	static String default_extension;

#ifdef WEB_ENABLED
	int _js_id = 0;
	ConnectionState _conn_state = STATE_NEW;
	GatheringState _gathering_state = GATHERING_STATE_NEW;
	SignalingState _signaling_state = SIGNALING_STATE_STABLE;

	static void _on_connection_state_changed(void* p_obj, int p_state);
	static void _on_gathering_state_changed(void* p_obj, int p_state);
	static void _on_signaling_state_changed(void* p_obj, int p_state);
	static void _on_ice_candidate(
		void* p_obj, const char* p_mid_name, int p_mline_idx, const char* p_candidate);
	static void _on_data_channel(void* p_obj, int p_channel);
	static void _on_session_created(void* p_obj, const char* p_type, const char* p_session);
	static void _on_error(void* p_obj);
#endif

public:
	static void set_default_extension(const String& p_name);

	ConnectionState get_connection_state() const;
	GatheringState get_gathering_state() const;
	SignalingState get_signaling_state() const;

	Error initialize(const String& p_config_json = "{}");
	Ref<WebRTCDataChannel> create_data_channel(
		const String& p_channel_name, const String& p_channel_config_json = "{}");
	Error create_offer();
	Error set_remote_description(const String& p_type, const String& p_sdp);
	Error set_local_description(const String& p_type, const String& p_sdp);
	Error add_ice_candidate(
		const String& p_sdp_mid_name, int p_sdp_mline_index_name, const String& p_sdp_name);
	Error poll();
	void close();

	static WebRTCPeerConnection* create(bool p_notify_postinitialize = true);

	WebRTCPeerConnection();
	~WebRTCPeerConnection();
};

class WebRTCMultiplayerPeer final : public MultiplayerPeer
{
private:
	enum
	{
		CH_RELIABLE = 0,
		CH_ORDERED = 1,
		CH_UNRELIABLE = 2,
		CH_RESERVED_MAX = 3
	};

	enum NetworkMode
	{
		MODE_NONE,
		MODE_SERVER,
		MODE_CLIENT,
		MODE_MESH,
	};

	class ConnectedPeer final : public RefCounted
	{
	public:
		Ref<WebRTCPeerConnection> connection;
		List<Ref<WebRTCDataChannel>> channels;
		bool connected;

		ConnectedPeer()
		{
			connected = false;
			for (int i = 0; i < CH_RESERVED_MAX; i++) {
				channels.push_front(Ref<WebRTCDataChannel>());
			}
		}
	};

	uint32_t unique_id = 0;
	int target_peer = 0;
	int client_count = 0;
	ConnectionStatus connection_status = CONNECTION_DISCONNECTED;
	int next_packet_peer = 0;
	int next_packet_channel = 0;
	NetworkMode network_mode = MODE_NONE;

	HashMap<int, Ref<ConnectedPeer>> peer_map;
	List<TransferMode> channels_modes;

	void _find_next_peer();
	Ref<ConnectedPeer> _get_next_peer();

public:
	WebRTCMultiplayerPeer() {}
	~WebRTCMultiplayerPeer();

	Error add_peer(
		const Ref<WebRTCPeerConnection>& p_peer, int p_peer_id, int p_unreliable_lifetime = 1);
	void remove_peer(int p_peer_id);
	bool has_peer(int p_peer_id);

	// PacketPeer
	virtual Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;
	virtual int get_available_packet_count() const override;
	virtual int get_max_packet_size() const override;

	// MultiplayerPeer
	virtual void set_target_peer(int p_peer_id) override;

	virtual int get_unique_id() const override;
	virtual int get_packet_peer() const override;
	virtual int get_packet_channel() const override;
	virtual TransferMode get_packet_mode() const override;

	virtual bool is_server() const override;
	virtual bool is_server_relay_supported() const override;

	virtual void poll() override;
	virtual void close() override;
	virtual void disconnect_peer(int p_peer_id, bool p_force = false) override;

	virtual ConnectionStatus get_connection_status() const override;
};

// --- End modules/webrtc/webrtc_peer_connection.h ---
#include "modules/register_module_types.h"

#include "core/config/project_settings.h"

#ifdef WEB_ENABLED
#include <emscripten.h>

extern "C" {
typedef void (*RTCChOnOpen)(void* p_obj);
typedef void (*RTCChOnMessage)(void* p_obj, const uint8_t* p_buffer, int p_size, int p_is_string);
typedef void (*RTCChOnClose)(void* p_obj);
typedef void (*RTCChOnError)(void* p_obj);

extern int godot_js_rtc_datachannel_ready_state_get(int p_id);
extern int godot_js_rtc_datachannel_send(
	int p_id, const uint8_t* p_buffer, int p_length, int p_raw);
extern int godot_js_rtc_datachannel_is_ordered(int p_id);
extern int godot_js_rtc_datachannel_id_get(int p_id);
extern int godot_js_rtc_datachannel_max_packet_lifetime_get(int p_id);
extern int godot_js_rtc_datachannel_max_retransmits_get(int p_id);
extern int godot_js_rtc_datachannel_is_negotiated(int p_id);
extern int godot_js_rtc_datachannel_get_buffered_amount(int p_id);
extern char* godot_js_rtc_datachannel_label_get(int p_id); // Must free the returned string.
extern char* godot_js_rtc_datachannel_protocol_get(int p_id); // Must free the returned string.
extern void godot_js_rtc_datachannel_destroy(int p_id);
extern void godot_js_rtc_datachannel_connect(int p_id, void* p_obj, RTCChOnOpen p_on_open,
	RTCChOnMessage p_on_message, RTCChOnError p_on_error, RTCChOnClose p_on_close);
extern void godot_js_rtc_datachannel_close(int p_id);

typedef void (*RTCOnIceConnectionStateChange)(void* p_obj, int p_state);
typedef void (*RTCOnIceGatheringStateChange)(void* p_obj, int p_state);
typedef void (*RTCOnSignalingStateChange)(void* p_obj, int p_state);
typedef void (*RTCOnIceCandidate)(
	void* p_obj, const char* p_mid, int p_mline_idx, const char* p_candidate);
typedef void (*RTCOnDataChannel)(void* p_obj, int p_id);
typedef void (*RTCOnSession)(void* p_obj, const char* p_type, const char* p_sdp);
typedef void (*RTCOnError)(void* p_obj);
extern int godot_js_rtc_pc_create(const char* p_config, void* p_obj,
	RTCOnIceConnectionStateChange p_on_connection_state_change,
	RTCOnIceGatheringStateChange p_on_gathering_state_change,
	RTCOnSignalingStateChange p_on_signaling_state_change, RTCOnIceCandidate p_on_candidate,
	RTCOnDataChannel p_on_datachannel);
extern void godot_js_rtc_pc_close(int p_id);
extern void godot_js_rtc_pc_destroy(int p_id);
extern void godot_js_rtc_pc_offer_create(
	int p_id, void* p_obj, RTCOnSession p_on_session, RTCOnError p_on_error);
extern void godot_js_rtc_pc_local_description_set(
	int p_id, const char* p_type, const char* p_sdp, void* p_obj, RTCOnError p_on_error);
extern void godot_js_rtc_pc_remote_description_set(int p_id, const char* p_type, const char* p_sdp,
	void* p_obj, RTCOnSession p_on_session, RTCOnError p_on_error);
extern void godot_js_rtc_pc_ice_candidate_add(
	int p_id, const char* p_mid_name, int p_mline_idx, const char* p_sdo);
extern int godot_js_rtc_pc_datachannel_create(
	int p_id, const char* p_label, const char* p_config);
}
#endif

// WebRTCDataChannel

void WebRTCDataChannel::set_write_mode(WriteMode p_mode) { _write_mode = p_mode; }

WebRTCDataChannel::WriteMode WebRTCDataChannel::get_write_mode() const { return _write_mode; }

int WebRTCDataChannel::get_max_packet_size() const { return 1200; }

#ifdef WEB_ENABLED

void WebRTCDataChannel::_on_open(void* p_obj)
{
	WebRTCDataChannel* peer = static_cast<WebRTCDataChannel*>(p_obj);
	peer->in_buffer.resize(peer->_in_buffer_shift);
}

void WebRTCDataChannel::_on_close(void* p_obj)
{
	WebRTCDataChannel* peer = static_cast<WebRTCDataChannel*>(p_obj);
	peer->close();
}

void WebRTCDataChannel::_on_error(void* p_obj)
{
	WebRTCDataChannel* peer = static_cast<WebRTCDataChannel*>(p_obj);
	peer->close();
}

void WebRTCDataChannel::_on_message(
	void* p_obj, const uint8_t* p_data, int p_size, int p_is_string)
{
	WebRTCDataChannel* peer = static_cast<WebRTCDataChannel*>(p_obj);
	RingBuffer<uint8_t>& in_buffer = peer->in_buffer;

	ERR_FAIL_COND_MSG(in_buffer.space_left() < (int)(p_size + 5), "Buffer full! Dropping data.");

	uint8_t is_string = p_is_string ? 1 : 0;
	in_buffer.write((uint8_t*)&p_size, 4);
	in_buffer.write((uint8_t*)&is_string, 1);
	in_buffer.write(p_data, p_size);
	peer->queue_count++;
}

void WebRTCDataChannel::close()
{
	in_buffer.resize(0);
	queue_count = 0;
	_was_string = false;
	godot_js_rtc_datachannel_close(_js_id);
}

Error WebRTCDataChannel::poll() { return OK; }

WebRTCDataChannel::ChannelState WebRTCDataChannel::get_ready_state() const
{
	return (ChannelState)godot_js_rtc_datachannel_ready_state_get(_js_id);
}

int WebRTCDataChannel::get_available_packet_count() const { return queue_count; }

Error WebRTCDataChannel::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_COND_V(get_ready_state() != STATE_OPEN, ERR_UNCONFIGURED);

	if (queue_count == 0) {
		return ERR_UNAVAILABLE;
	}

	uint32_t to_read = 0;
	uint32_t left = 0;
	uint8_t is_string = 0;
	r_buffer_size = 0;

	in_buffer.read((uint8_t*)&to_read, 4);
	--queue_count;
	left = in_buffer.data_left();

	if (left < to_read + 1) {
		in_buffer.advance_read(left);
		return FAILED;
	}

	in_buffer.read(&is_string, 1);
	_was_string = is_string == 1;
	in_buffer.read(packet_buffer, to_read);
	*r_buffer = packet_buffer;
	r_buffer_size = to_read;

	return OK;
}

Error WebRTCDataChannel::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(get_ready_state() != STATE_OPEN, ERR_UNCONFIGURED);

	int is_bin = _write_mode == WRITE_MODE_BINARY ? 1 : 0;
	godot_js_rtc_datachannel_send(_js_id, p_buffer, p_buffer_size, is_bin);
	return OK;
}

bool WebRTCDataChannel::was_string_packet() const { return _was_string; }

String WebRTCDataChannel::get_label() const { return _label; }

bool WebRTCDataChannel::is_ordered() const { return godot_js_rtc_datachannel_is_ordered(_js_id); }

int WebRTCDataChannel::get_id() const { return godot_js_rtc_datachannel_id_get(_js_id); }

int WebRTCDataChannel::get_max_packet_life_time() const
{
	return godot_js_rtc_datachannel_max_packet_lifetime_get(_js_id);
}

int WebRTCDataChannel::get_max_retransmits() const
{
	return godot_js_rtc_datachannel_max_retransmits_get(_js_id);
}

String WebRTCDataChannel::get_protocol() const { return _protocol; }

bool WebRTCDataChannel::is_negotiated() const
{
	return godot_js_rtc_datachannel_is_negotiated(_js_id);
}

int WebRTCDataChannel::get_buffered_amount() const
{
	return godot_js_rtc_datachannel_get_buffered_amount(_js_id);
}

WebRTCDataChannel::WebRTCDataChannel() {}

WebRTCDataChannel::WebRTCDataChannel(int js_id)
{
	_js_id = js_id;

	godot_js_rtc_datachannel_connect(js_id, this, &_on_open, &_on_message, &_on_error, &_on_close);
	char* label = godot_js_rtc_datachannel_label_get(js_id);
	if (label) {
		_label.clear();
		_label.append_utf8(label);
		free(label);
	}
	char* protocol = godot_js_rtc_datachannel_protocol_get(js_id);
	if (protocol) {
		_protocol.clear();
		_protocol.append_utf8(protocol);
		free(protocol);
	}
}

WebRTCDataChannel::~WebRTCDataChannel()
{
	close();
	godot_js_rtc_datachannel_destroy(_js_id);
}

#else

bool WebRTCDataChannel::was_string_packet() const { return false; }

WebRTCDataChannel::ChannelState WebRTCDataChannel::get_ready_state() const { return STATE_CLOSED; }

String WebRTCDataChannel::get_label() const { return String(); }

bool WebRTCDataChannel::is_ordered() const { return true; }

int WebRTCDataChannel::get_id() const { return 65535; }

int WebRTCDataChannel::get_max_packet_life_time() const { return 65535; }

int WebRTCDataChannel::get_max_retransmits() const { return 65535; }

String WebRTCDataChannel::get_protocol() const { return String(); }

bool WebRTCDataChannel::is_negotiated() const { return false; }

int WebRTCDataChannel::get_buffered_amount() const { return 0; }

Error WebRTCDataChannel::poll() { return ERR_UNAVAILABLE; }

void WebRTCDataChannel::close() {}

int WebRTCDataChannel::get_available_packet_count() const { return 0; }

Error WebRTCDataChannel::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	return ERR_UNAVAILABLE;
}

Error WebRTCDataChannel::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	return ERR_UNAVAILABLE;
}

WebRTCDataChannel::WebRTCDataChannel() {}

WebRTCDataChannel::~WebRTCDataChannel() {}

#endif // WEB_ENABLED

// WebRTCPeerConnection

String WebRTCPeerConnection::default_extension;

void WebRTCPeerConnection::set_default_extension(const String& p_extension)
{
	default_extension = String(p_extension);
}

WebRTCPeerConnection* WebRTCPeerConnection::create(bool p_notify_postinitialize)
{
#ifndef WEB_ENABLED
	if (default_extension == String()) {
		WARN_PRINT_ONCE("No default WebRTC extension configured.");
	}
#endif
	return memnew(WebRTCPeerConnection);
}

#ifdef WEB_ENABLED

void WebRTCPeerConnection::_on_ice_candidate(
	void* p_obj, const char* p_mid_name, int p_mline_idx, const char* p_candidate)
{
	(void)p_obj;
	(void)p_mid_name;
	(void)p_mline_idx;
	(void)p_candidate;
}

void WebRTCPeerConnection::_on_session_created(
	void* p_obj, const char* p_type, const char* p_session)
{
	(void)p_obj;
	(void)p_type;
	(void)p_session;
}

void WebRTCPeerConnection::_on_connection_state_changed(void* p_obj, int p_state)
{
	WebRTCPeerConnection* peer = static_cast<WebRTCPeerConnection*>(p_obj);
	peer->_conn_state = (ConnectionState)p_state;
}

void WebRTCPeerConnection::_on_gathering_state_changed(void* p_obj, int p_state)
{
	WebRTCPeerConnection* peer = static_cast<WebRTCPeerConnection*>(p_obj);
	peer->_gathering_state = (GatheringState)p_state;
}

void WebRTCPeerConnection::_on_signaling_state_changed(void* p_obj, int p_state)
{
	WebRTCPeerConnection* peer = static_cast<WebRTCPeerConnection*>(p_obj);
	peer->_signaling_state = (SignalingState)p_state;
}

void WebRTCPeerConnection::_on_error(void* p_obj)
{
	(void)p_obj;
	ERR_PRINT("RTCPeerConnection error!");
}

void WebRTCPeerConnection::_on_data_channel(void* p_obj, int p_id)
{
	(void)p_obj;
	Ref<WebRTCDataChannel> ch(memnew(WebRTCDataChannel(p_id)));
	(void)ch;
}

void WebRTCPeerConnection::close()
{
	godot_js_rtc_pc_close(_js_id);
	_conn_state = STATE_CLOSED;
}

Error WebRTCPeerConnection::create_offer()
{
	ERR_FAIL_COND_V(_conn_state != STATE_NEW, FAILED);

	_conn_state = STATE_CONNECTING;
	godot_js_rtc_pc_offer_create(_js_id, this, &_on_session_created, &_on_error);
	return OK;
}

Error WebRTCPeerConnection::set_local_description(const String& p_type, const String& p_sdp)
{
	godot_js_rtc_pc_local_description_set(
		_js_id, p_type.utf8().get_data(), p_sdp.utf8().get_data(), this, &_on_error);
	return OK;
}

Error WebRTCPeerConnection::set_remote_description(const String& p_type, const String& p_sdp)
{
	if (p_type == "offer") {
		ERR_FAIL_COND_V(_conn_state != STATE_NEW, FAILED);
		_conn_state = STATE_CONNECTING;
	}
	godot_js_rtc_pc_remote_description_set(_js_id, p_type.utf8().get_data(),
		p_sdp.utf8().get_data(), this, &_on_session_created, &_on_error);
	return OK;
}

Error WebRTCPeerConnection::add_ice_candidate(
	const String& p_sdp_mid_name, int p_sdp_mline_index_name, const String& p_sdp_name)
{
	godot_js_rtc_pc_ice_candidate_add(_js_id, p_sdp_mid_name.utf8().get_data(),
		p_sdp_mline_index_name, p_sdp_name.utf8().get_data());
	return OK;
}

Error WebRTCPeerConnection::initialize(const String& p_config_json)
{
	if (_js_id) {
		godot_js_rtc_pc_destroy(_js_id);
		_js_id = 0;
	}
	_conn_state = STATE_NEW;

	_js_id = godot_js_rtc_pc_create(p_config_json.utf8().get_data(), this,
		&_on_connection_state_changed, &_on_gathering_state_changed, &_on_signaling_state_changed,
		&_on_ice_candidate, &_on_data_channel);
	return _js_id ? OK : FAILED;
}

Ref<WebRTCDataChannel> WebRTCPeerConnection::create_data_channel(
	const String& p_channel, const String& p_channel_config_json)
{
	ERR_FAIL_COND_V(_conn_state != STATE_NEW, nullptr);

	int id = godot_js_rtc_pc_datachannel_create(
		_js_id, p_channel.utf8().get_data(), p_channel_config_json.utf8().get_data());
	ERR_FAIL_COND_V(id == 0, nullptr);
	return memnew(WebRTCDataChannel(id));
}

Error WebRTCPeerConnection::poll() { return OK; }

WebRTCPeerConnection::GatheringState WebRTCPeerConnection::get_gathering_state() const
{
	return _gathering_state;
}

WebRTCPeerConnection::SignalingState WebRTCPeerConnection::get_signaling_state() const
{
	return _signaling_state;
}

WebRTCPeerConnection::ConnectionState WebRTCPeerConnection::get_connection_state() const
{
	return _conn_state;
}

WebRTCPeerConnection::WebRTCPeerConnection() { initialize(); }

WebRTCPeerConnection::~WebRTCPeerConnection()
{
	close();
	if (_js_id) {
		godot_js_rtc_pc_destroy(_js_id);
		_js_id = 0;
	}
}

#else

WebRTCPeerConnection::ConnectionState WebRTCPeerConnection::get_connection_state() const
{
	return STATE_DISCONNECTED;
}

WebRTCPeerConnection::GatheringState WebRTCPeerConnection::get_gathering_state() const
{
	return GATHERING_STATE_NEW;
}

WebRTCPeerConnection::SignalingState WebRTCPeerConnection::get_signaling_state() const
{
	return SIGNALING_STATE_STABLE;
}

Error WebRTCPeerConnection::initialize(const String& p_config_json) { return ERR_UNAVAILABLE; }

Ref<WebRTCDataChannel> WebRTCPeerConnection::create_data_channel(
	const String& p_channel_name, const String& p_channel_config_json)
{
	return nullptr;
}

Error WebRTCPeerConnection::create_offer() { return ERR_UNAVAILABLE; }

Error WebRTCPeerConnection::set_remote_description(const String& p_type, const String& p_sdp)
{
	return ERR_UNAVAILABLE;
}

Error WebRTCPeerConnection::set_local_description(const String& p_type, const String& p_sdp)
{
	return ERR_UNAVAILABLE;
}

Error WebRTCPeerConnection::add_ice_candidate(
	const String& p_sdp_mid, int p_sdp_mline_index_name, const String& p_sdp_name)
{
	return ERR_UNAVAILABLE;
}

Error WebRTCPeerConnection::poll() { return ERR_UNAVAILABLE; }

WebRTCPeerConnection::WebRTCPeerConnection() {}

WebRTCPeerConnection::~WebRTCPeerConnection() {}

void WebRTCPeerConnection::close() {}

#endif // WEB_ENABLED

// WebRTCMultiplayerPeer

void WebRTCMultiplayerPeer::set_target_peer(int p_peer_id) { target_peer = p_peer_id; }

/* Returns the ID of the MultiplayerPeer who sent the most recent packet: */
int WebRTCMultiplayerPeer::get_packet_peer() const { return next_packet_peer; }

int WebRTCMultiplayerPeer::get_packet_channel() const
{
	return next_packet_channel < CH_RESERVED_MAX ? 0 : next_packet_channel - CH_RESERVED_MAX + 1;
}

MultiplayerPeer::TransferMode WebRTCMultiplayerPeer::get_packet_mode() const
{
	ERR_FAIL_INDEX_V(next_packet_channel, channels_modes.size(), TRANSFER_MODE_RELIABLE);
	return channels_modes.get(next_packet_channel);
}

bool WebRTCMultiplayerPeer::is_server() const { return unique_id == TARGET_PEER_SERVER; }

void WebRTCMultiplayerPeer::_find_next_peer()
{
	HashMap<int, Ref<ConnectedPeer>>::Iterator E = peer_map.find(next_packet_peer);
	if (E) {
		++E;
	}
	// After last.
	while (E) {
		if (!E->value->connected) {
			++E;
			continue;
		}
		int idx = 0;
		for (const Ref<WebRTCDataChannel>& F : E->value->channels) {
			if (F->get_available_packet_count()) {
				next_packet_channel = idx;
				next_packet_peer = E->key;
				return;
			}
			idx++;
		}
		++E;
	}
	E = peer_map.begin();
	// Before last
	while (E) {
		if (!E->value->connected) {
			++E;
			continue;
		}
		int idx = 0;
		for (const Ref<WebRTCDataChannel>& F : E->value->channels) {
			if (F->get_available_packet_count()) {
				next_packet_channel = idx;
				next_packet_peer = E->key;
				return;
			}
			idx++;
		}
		if (E->key == (int)next_packet_peer) {
			break;
		}
		++E;
	}
	// No packet found
	next_packet_channel = 0;
	next_packet_peer = 0;
}

MultiplayerPeer::ConnectionStatus WebRTCMultiplayerPeer::get_connection_status() const
{
	return connection_status;
}

bool WebRTCMultiplayerPeer::is_server_relay_supported() const
{
	return network_mode == MODE_SERVER || network_mode == MODE_CLIENT;
}

int WebRTCMultiplayerPeer::get_unique_id() const
{
	ERR_FAIL_COND_V(connection_status == CONNECTION_DISCONNECTED, 1);
	return unique_id;
}

bool WebRTCMultiplayerPeer::has_peer(int p_peer_id) { return peer_map.has(p_peer_id); }

void WebRTCMultiplayerPeer::disconnect_peer(int p_peer_id, bool p_force)
{
	ERR_FAIL_COND(!peer_map.has(p_peer_id));
	if (p_force) {
		peer_map.erase(p_peer_id);
		if (network_mode == MODE_CLIENT && p_peer_id == TARGET_PEER_SERVER) {
			connection_status = CONNECTION_DISCONNECTED;
		}
	}
	else {
		peer_map[p_peer_id]->connection->close(); // Will be removed during next poll.
	}
}

Error WebRTCMultiplayerPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	// Peer not available
	if (next_packet_peer == 0 || !peer_map.has(next_packet_peer)) {
		_find_next_peer();
		ERR_FAIL_V(ERR_UNAVAILABLE);
	}
	for (Ref<WebRTCDataChannel>& E : peer_map[next_packet_peer]->channels) {
		if (E->get_available_packet_count()) {
			Error err = E->get_packet(r_buffer, r_buffer_size);
			_find_next_peer();
			return err;
		}
	}
	// Channels for that peer were empty. Bug?
	_find_next_peer();
	ERR_FAIL_V(ERR_BUG);
}

Error WebRTCMultiplayerPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(connection_status == CONNECTION_DISCONNECTED, ERR_UNCONFIGURED);

	int ch = get_transfer_channel();
	if (ch == 0) {
		switch (get_transfer_mode()) {
		case TRANSFER_MODE_RELIABLE:
			ch = CH_RELIABLE;
			break;
		case TRANSFER_MODE_UNRELIABLE_ORDERED:
			ch = CH_ORDERED;
			break;
		case TRANSFER_MODE_UNRELIABLE:
			ch = CH_UNRELIABLE;
			break;
		}
	}
	else {
		ch += CH_RESERVED_MAX - 1;
	}

	if (target_peer > 0) {
		HashMap<int, Ref<ConnectedPeer>>::Iterator E = peer_map.find(target_peer);
		ERR_FAIL_COND_V_MSG(
			!E, ERR_INVALID_PARAMETER, "Invalid target peer: " + itos(target_peer) + ".");

		ERR_FAIL_COND_V_MSG(E->value->channels.size() <= ch, ERR_INVALID_PARAMETER,
			vformat("Unable to send packet on channel %d, max channels: %d", ch,
				E->value->channels.size()));
		ERR_FAIL_COND_V(E->value->channels.get(ch).is_null(), ERR_BUG);
		return E->value->channels.get(ch)->put_packet(p_buffer, p_buffer_size);
	}
	else {
		int exclude = -target_peer;

		for (KeyValue<int, Ref<ConnectedPeer>>& F : peer_map) {
			// Exclude packet. If target_peer == 0 then don't exclude any packets
			if (target_peer != 0 && F.key == exclude) {
				continue;
			}

			ERR_CONTINUE_MSG(F.value->channels.size() <= ch,
				vformat("Unable to send packet on channel %d, max channels: %d", ch,
					F.value->channels.size()));
			ERR_CONTINUE(F.value->channels.get(ch).is_null());
			F.value->channels.get(ch)->put_packet(p_buffer, p_buffer_size);
		}
	}
	return OK;
}

int WebRTCMultiplayerPeer::get_available_packet_count() const
{
	if (next_packet_peer == 0) {
		return 0; // To be sure next call to get_packet works if size > 0 .
	}
	int size = 0;
	for (const KeyValue<int, Ref<ConnectedPeer>>& E : peer_map) {
		if (!E.value->connected) {
			continue;
		}
		for (const Ref<WebRTCDataChannel>& F : E.value->channels) {
			size += F->get_available_packet_count();
		}
	}
	return size;
}

int WebRTCMultiplayerPeer::get_max_packet_size() const { return 1200; }

WebRTCMultiplayerPeer::~WebRTCMultiplayerPeer() { close(); }

void WebRTCMultiplayerPeer::close() {}

void WebRTCMultiplayerPeer::poll() {}

// Module registration

void uninitialize_webrtc_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void initialize_webrtc_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_WEBRTC_ENABLED)


// --- Merged from modules/websocket/websocket_peer.cpp ---
#if defined(MODULE_WEBSOCKET_ENABLED)
// --- Inlined from modules/websocket/websocket_peer.h ---
#include "core/crypto/crypto.h"
#include "core/io/ip.h"
#include "core/io/packet_peer.h"
#include "core/io/stream_peer_socket.h"
#include "core/templates/list.h"
#include "core/types.h"
#include "scene/main/scene_tree.h"

#ifdef WEB_ENABLED
#include <emscripten.h>

extern "C" {
typedef void (*WSOnOpen)(void* p_ref, char* p_protocol);
typedef void (*WSOnMessage)(void* p_ref, const uint8_t* p_buf, int p_buf_len, int p_is_string);
typedef void (*WSOnClose)(void* p_ref, int p_code, const char* p_reason, int p_is_clean);
typedef void (*WSOnError)(void* p_ref);

extern int godot_js_websocket_create(void* p_ref, const char* p_url, const char* p_proto,
	WSOnOpen p_on_open, WSOnMessage p_on_message, WSOnError p_on_error, WSOnClose p_on_close);
extern int godot_js_websocket_send(int p_id, const uint8_t* p_buf, int p_buf_len, int p_raw);
extern int godot_js_websocket_buffered_amount(int p_id);
extern void godot_js_websocket_close(int p_id, int p_code, const char* p_reason);
extern void godot_js_websocket_destroy(int p_id);
}
#else
#include <wslay/wslay.h>
#include "core/crypto/crypto_core.h"
#include "core/io/stream_peer_socket.h"

#define WSL_MAX_HEADER_SIZE 4096
#endif

template <typename T>
class PacketBuffer final
{
private:
	typedef struct
	{
		uint32_t size;
		T info;
	} _Packet;

	Vector<_Packet> _packets;
	int _queued = 0;
	int _write_pos = 0;
	int _read_pos = 0;
	RingBuffer<uint8_t> _payload;

public:
	Error write_packet(const uint8_t* p_payload, uint32_t p_size, const T* p_info)
	{
		ERR_FAIL_COND_V_MSG(p_payload && (uint32_t)_payload.space_left() < p_size,
			ERR_OUT_OF_MEMORY, "Buffer payload full! Dropping data.");
		ERR_FAIL_COND_V_MSG(p_info && _queued >= _packets.size(), ERR_OUT_OF_MEMORY,
			"Too many packets in queue! Dropping data.");

		// If p_info is nullptr, only the payload is written
		if (p_info) {
			ERR_FAIL_COND_V(_write_pos > _packets.size(), ERR_OUT_OF_MEMORY);
			_Packet p;
			p.size = p_size;
			p.info = *p_info;
			_packets.write[_write_pos] = p;
			_queued += 1;
			_write_pos++;
			if (_write_pos >= _packets.size()) {
				_write_pos = 0;
			}
		}

		// If p_payload is nullptr, only the packet information is written.
		if (p_payload) {
			_payload.write((const uint8_t*)p_payload, p_size);
		}

		return OK;
	}

	Error read_packet(uint8_t* r_payload, int p_bytes, T* r_info, int& r_read)
	{
		ERR_FAIL_COND_V(_queued < 1, ERR_UNAVAILABLE);
		_Packet p = _packets[_read_pos];
		_read_pos += 1;
		if (_read_pos >= _packets.size()) {
			_read_pos = 0;
		}
		_queued -= 1;

		ERR_FAIL_COND_V(_payload.data_left() < (int)p.size, ERR_BUG);
		ERR_FAIL_COND_V(p_bytes < (int)p.size, ERR_OUT_OF_MEMORY);

		r_read = p.size;
		memcpy(r_info, &p.info, sizeof(T));
		_payload.read(r_payload, p.size);
		return OK;
	}

	void resize(int p_buf_shift, int p_max_packets)
	{
		_payload.resize(p_buf_shift);
		_packets.resize(p_max_packets);
		_read_pos = 0;
		_write_pos = 0;
		_queued = 0;
	}

	int packets_left() const { return _queued; }

	int payload_space_left() const { return _payload.space_left(); }

	int packets_space_left() const { return _packets.size() - _queued; }

	void clear()
	{
		_payload.resize(0);
		_packets.resize(0);
		_read_pos = 0;
		_write_pos = 0;
		_queued = 0;
	}

	PacketBuffer() { clear(); }

	~PacketBuffer() { clear(); }
};

class WebSocketPeer final : public PacketPeer
{
public:
	enum State
	{
		STATE_CONNECTING,
		STATE_OPEN,
		STATE_CLOSING,
		STATE_CLOSED
	};

	enum WriteMode
	{
		WRITE_MODE_TEXT,
		WRITE_MODE_BINARY,
	};

	enum
	{
		DEFAULT_BUFFER_SIZE = 65535,
	};

private:
	Vector<String> supported_protocols;
	Vector<String> handshake_headers;

	int outbound_buffer_size = DEFAULT_BUFFER_SIZE;
	int inbound_buffer_size = DEFAULT_BUFFER_SIZE;
	int max_queued_packets = 4096;
	uint64_t heartbeat_interval_msec = 0;

	State ready_state = STATE_CLOSED;
	Vector<uint8_t> packet_buffer;
	PacketBuffer<uint8_t> in_buffer;
	uint8_t was_string = 0;
	int close_code = -1;
	String close_reason;
	String selected_protocol;
	String requested_url;

	Error _send_bind(const PackedByteArray& p_data, WriteMode p_mode = WRITE_MODE_BINARY);
	Vector<String> _get_supported_protocols() const;
	Vector<String> _get_handshake_headers() const;
	void _clear();

#ifdef WEB_ENABLED
	int peer_sock = -1;

	static void _esws_on_connect(void* obj, char* proto);
	static void _esws_on_message(
		void* obj, const uint8_t* p_data, int p_data_size, int p_is_string);
	static void _esws_on_error(void* obj);
	static void _esws_on_close(void* obj, int code, const char* reason, int was_clean);

	Error _send(const uint8_t* p_buffer, int p_buffer_size, bool p_binary);
#else
	static ssize_t _wsl_recv_callback(
		wslay_event_context_ptr ctx, uint8_t* data, size_t len, int flags, void* user_data);
	static void _wsl_recv_start_callback(wslay_event_context_ptr ctx,
		const struct wslay_event_on_frame_recv_start_arg* arg, void* user_data);
	static void _wsl_frame_recv_chunk_callback(wslay_event_context_ptr ctx,
		const struct wslay_event_on_frame_recv_chunk_arg* arg, void* user_data);

	static ssize_t _wsl_send_callback(
		wslay_event_context_ptr ctx, const uint8_t* data, size_t len, int flags, void* user_data);
	static int _wsl_genmask_callback(
		wslay_event_context_ptr ctx, uint8_t* buf, size_t len, void* user_data);
	static void _wsl_msg_recv_callback(wslay_event_context_ptr ctx,
		const struct wslay_event_on_msg_recv_arg* arg, void* user_data);

	static wslay_event_callbacks _wsl_callbacks;

	static String _compute_key_response(const String& p_key);
	static String _generate_key();

	class Resolver final
	{
		IP::ResolverID resolver_id = IP::RESOLVER_INVALID_ID;
		int port = 0;

	public:
		void try_next_candidate(const Ref<StreamPeerSocket>& p_tcp);
		void start(const String& p_host, int p_port);
		void stop();

		Resolver() {}
	};

	struct PendingMessage
	{
		size_t payload_size = 0;
		uint8_t opcode = 0;

		void clear()
		{
			payload_size = 0;
			opcode = 0;
		}
	};

	Resolver resolver;
	bool is_server = false;
	Ref<StreamPeerSocket> tcp;
	Ref<StreamPeer> connection;
	wslay_event_context_ptr wsl_ctx = nullptr;

	String requested_host;
	bool pending_request = true;
	Ref<StreamPeerBuffer> handshake_buffer;
	String session_key;

	uint64_t last_heartbeat = 0;
	bool heartbeat_waiting = false;
	PendingMessage pending_message;

	bool use_tls = true;
	Ref<TLSOptions> tls_options;

	Error _send(const uint8_t* p_buffer, int p_buffer_size, wslay_opcode p_opcode);

	Error _do_server_handshake();
	bool _parse_client_request();

	void _do_client_handshake();
	bool _verify_server_response();
#endif

public:
	static WebSocketPeer* create(bool p_notify_postinitialize = true)
	{
		return memnew(WebSocketPeer);
	}

	// PacketPeer
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override;
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;
	virtual int get_max_packet_size() const override { return packet_buffer.size(); }

	// WebSocketPeer
	Error connect_to_url(
		const String& p_url, const Ref<TLSOptions>& p_options = Ref<TLSOptions>());
	Error accept_stream(const Ref<StreamPeer>& p_stream);

	Error send(const uint8_t* p_buffer, int p_buffer_size, WriteMode p_mode);
	void close(int p_code = 1000, const String& p_reason = "");

	IPAddress get_connected_host() const;
	uint16_t get_connected_port() const;
	bool was_string_packet() const { return was_string; }
	void set_no_delay(bool p_enabled);
	int get_current_outbound_buffered_amount() const;
	String get_selected_protocol() const { return selected_protocol; }
	String get_requested_url() const { return requested_url; }

	void poll();
	State get_ready_state() const { return ready_state; }
	int get_close_code() const { return close_code; }
	String get_close_reason() const { return close_reason; }

	Error send_text(const String& p_text);

	void set_supported_protocols(const Vector<String>& p_protocols);
	const Vector<String> get_supported_protocols() const;

	void set_handshake_headers(const Vector<String>& p_headers);
	const Vector<String> get_handshake_headers() const;

	void set_outbound_buffer_size(int p_buffer_size);
	int get_outbound_buffer_size() const;

	void set_inbound_buffer_size(int p_buffer_size);
	int get_inbound_buffer_size() const;

	void set_max_queued_packets(int p_max_queued_packets);
	int get_max_queued_packets() const;

	double get_heartbeat_interval() const;
	void set_heartbeat_interval(double p_interval);

	WebSocketPeer();
	~WebSocketPeer();
};

class WebSocketMultiplayerPeer final : public MultiplayerPeer
{
private:
	Ref<WebSocketPeer> _create_peer();

	enum
	{
		SYS_NONE = 0,
		SYS_ADD = 1,
		SYS_DEL = 2,
		SYS_ID = 3,

		PROTO_SIZE = 9
	};

	struct Packet
	{
		int source = 0;
		uint8_t* data = nullptr;
		uint32_t size = 0;
	};

	struct PendingPeer
	{
		uint64_t time = 0;
		Ref<StreamPeerSocket> tcp;
		Ref<StreamPeer> connection;
		Ref<WebSocketPeer> ws;
	};

	uint64_t handshake_timeout = 3000;
	Ref<WebSocketPeer> peer_config;
	HashMap<int, PendingPeer> pending_peers;
	Ref<SocketServer> tcp_server;
	Ref<TLSOptions> tls_server_options;

	ConnectionStatus connection_status = CONNECTION_DISCONNECTED;

	List<Packet> incoming_packets;
	HashMap<int, Ref<WebSocketPeer>> peers_map;
	Packet current_packet;

	int target_peer = 0;
	int unique_id = 0;

	void _poll_client();
	void _poll_server();
	void _clear();

public:
	/* MultiplayerPeer */
	virtual void set_target_peer(int p_target_peer) override;
	virtual int get_packet_peer() const override;

	virtual int get_packet_channel() const override { return 0; }

	virtual TransferMode get_packet_mode() const override { return TRANSFER_MODE_RELIABLE; }

	virtual int get_unique_id() const override;

	virtual bool is_server_relay_supported() const override { return true; }

	virtual int get_max_packet_size() const override;
	virtual bool is_server() const override;
	virtual void poll() override;
	virtual void close() override;
	virtual void disconnect_peer(int p_peer_id, bool p_force = false) override;

	virtual ConnectionStatus get_connection_status() const override;

	/* PacketPeer */
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override;
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	/* WebSocketPeer */
	Ref<WebSocketPeer> get_peer(int p_peer_id) const;

	Error create_client(const String& p_url, const Ref<TLSOptions>& p_options);
	Error create_server(int p_port, IPAddress p_bind_ip, const Ref<TLSOptions>& p_options);

	void set_supported_protocols(const Vector<String>& p_protocols);
	Vector<String> get_supported_protocols() const;

	void set_handshake_headers(const Vector<String>& p_headers);
	Vector<String> get_handshake_headers() const;

	void set_outbound_buffer_size(int p_buffer_size);
	int get_outbound_buffer_size() const;

	void set_inbound_buffer_size(int p_buffer_size);
	int get_inbound_buffer_size() const;

	float get_handshake_timeout() const;
	void set_handshake_timeout(float p_timeout);

	IPAddress get_peer_address(int p_peer_id) const;
	int get_peer_port(int p_peer_id) const;

	void set_max_queued_packets(int p_max_queued_packets);
	int get_max_queued_packets() const;

	WebSocketMultiplayerPeer();
	~WebSocketMultiplayerPeer();
};

class RemoteDebuggerPeerWebSocket final
{
	Ref<WebSocketPeer> ws_peer;

	int max_queued_messages = 4096;

public:
	Error connect_to_host(const String& p_uri);

	bool is_peer_connected();
	int get_max_message_size() const;
	bool has_message();
	void close();
	void poll();
	bool can_block() const;

	RemoteDebuggerPeerWebSocket(const Ref<WebSocketPeer>& p_peer = Ref<WebSocketPeer>());
};

// --- End modules/websocket/websocket_peer.h ---
#include "modules/register_module_types.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/ip.h"
#include "core/io/stream_peer.h"
#include "core/os/os.h"

// WebSocketPeer (shared configuration)

Error WebSocketPeer::_send_bind(const PackedByteArray& p_message, WriteMode p_mode)
{
	return send(p_message.ptr(), p_message.size(), p_mode);
}

Error WebSocketPeer::send_text(const String& p_text)
{
	const CharString cs = p_text.utf8();
	return send((const uint8_t*)cs.ptr(), cs.length(), WRITE_MODE_TEXT);
}

void WebSocketPeer::set_supported_protocols(const Vector<String>& p_protocols)
{
	// Strip edges from protocols.
	supported_protocols.resize(p_protocols.size());
	for (int i = 0; i < p_protocols.size(); i++) {
		supported_protocols.write[i] = p_protocols[i].strip_edges();
	}
}

const Vector<String> WebSocketPeer::get_supported_protocols() const { return supported_protocols; }

Vector<String> WebSocketPeer::_get_supported_protocols() const
{
	Vector<String> out;
	out.append_array(supported_protocols);
	return out;
}

void WebSocketPeer::set_handshake_headers(const Vector<String>& p_headers)
{
	handshake_headers = p_headers;
}

const Vector<String> WebSocketPeer::get_handshake_headers() const { return handshake_headers; }

Vector<String> WebSocketPeer::_get_handshake_headers() const
{
	Vector<String> out;
	out.append_array(handshake_headers);
	return out;
}

void WebSocketPeer::set_outbound_buffer_size(int p_buffer_size)
{
	outbound_buffer_size = p_buffer_size;
}

int WebSocketPeer::get_outbound_buffer_size() const { return outbound_buffer_size; }

void WebSocketPeer::set_inbound_buffer_size(int p_buffer_size)
{
	inbound_buffer_size = p_buffer_size;
}

int WebSocketPeer::get_inbound_buffer_size() const { return inbound_buffer_size; }

void WebSocketPeer::set_max_queued_packets(int p_max_queued_packets)
{
	max_queued_packets = p_max_queued_packets;
}

int WebSocketPeer::get_max_queued_packets() const { return max_queued_packets; }

double WebSocketPeer::get_heartbeat_interval() const { return heartbeat_interval_msec / 1000.0; }

void WebSocketPeer::set_heartbeat_interval(double p_interval)
{
	ERR_FAIL_COND(p_interval < 0);
	heartbeat_interval_msec = p_interval * 1000.0;
}

#ifdef WEB_ENABLED

// WebSocketPeer (Emscripten / Web implementation)

void WebSocketPeer::_esws_on_connect(void* p_obj, char* p_proto)
{
	WebSocketPeer* peer = static_cast<WebSocketPeer*>(p_obj);
	peer->ready_state = STATE_OPEN;
	peer->selected_protocol.clear();
	peer->selected_protocol.append_utf8(p_proto);
}

void WebSocketPeer::_esws_on_message(
	void* p_obj, const uint8_t* p_data, int p_data_size, int p_is_string)
{
	WebSocketPeer* peer = static_cast<WebSocketPeer*>(p_obj);
	uint8_t is_string = p_is_string ? 1 : 0;
	peer->in_buffer.write_packet(p_data, p_data_size, &is_string);
}

void WebSocketPeer::_esws_on_error(void* p_obj)
{
	WebSocketPeer* peer = static_cast<WebSocketPeer*>(p_obj);
	peer->ready_state = STATE_CLOSED;
}

void WebSocketPeer::_esws_on_close(void* p_obj, int p_code, const char* p_reason, int p_was_clean)
{
	WebSocketPeer* peer = static_cast<WebSocketPeer*>(p_obj);
	peer->close_code = p_code;
	peer->close_reason.clear();
	peer->close_reason.append_utf8(p_reason);
	peer->ready_state = STATE_CLOSED;
}

Error WebSocketPeer::connect_to_url(const String& p_url, const Ref<TLSOptions>& p_tls_options)
{
	ERR_FAIL_COND_V(p_url.is_empty(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_tls_options.is_valid() && p_tls_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(
		ready_state != STATE_CLOSED && ready_state != STATE_CLOSING, ERR_ALREADY_IN_USE);

	_clear();

	String host;
	String path;
	String scheme;
	String fragment;
	int port = 0;
	Error err = p_url.parse_url(scheme, host, port, path, fragment);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Invalid URL: " + p_url);

	if (scheme.is_empty()) {
		scheme = "ws://";
	}
	ERR_FAIL_COND_V_MSG(scheme != "ws://" && scheme != "wss://", ERR_INVALID_PARAMETER,
		vformat("Invalid protocol: \"%s\" (must be either \"ws://\" or \"wss://\").", scheme));

	String proto_string;
	for (int i = 0; i < supported_protocols.size(); i++) {
		if (i != 0) {
			proto_string += ",";
		}
		proto_string += supported_protocols[i];
	}

	if (handshake_headers.size()) {
		WARN_PRINT_ONCE("Custom headers are not supported in Web platform.");
	}

	requested_url = scheme + host;

	if (port && ((scheme == "ws://" && port != 80) || (scheme == "wss://" && port != 443))) {
		requested_url += ":" + String::num_int64(port);
	}

	if (!path.is_empty()) {
		requested_url += path;
	}

	peer_sock = godot_js_websocket_create(this, requested_url.utf8().get_data(),
		proto_string.utf8().get_data(), &_esws_on_connect, &_esws_on_message, &_esws_on_error,
		&_esws_on_close);
	if (peer_sock == -1) {
		return FAILED;
	}
	in_buffer.resize(Math::nearest_shift((uint32_t)inbound_buffer_size), max_queued_packets);
	packet_buffer.resize(inbound_buffer_size);
	ready_state = STATE_CONNECTING;
	return OK;
}

Error WebSocketPeer::accept_stream(const Ref<StreamPeer>& p_stream)
{
	WARN_PRINT_ONCE("Acting as WebSocket server is not supported in Web platforms.");
	return ERR_UNAVAILABLE;
}

Error WebSocketPeer::_send(const uint8_t* p_buffer, int p_buffer_size, bool p_binary)
{
	ERR_FAIL_COND_V(outbound_buffer_size > 0 && (get_current_outbound_buffered_amount() +
													p_buffer_size >= outbound_buffer_size),
		ERR_OUT_OF_MEMORY);

	if (godot_js_websocket_send(peer_sock, p_buffer, p_buffer_size, p_binary ? 1 : 0) != 0) {
		return FAILED;
	}
	return OK;
}

Error WebSocketPeer::send(const uint8_t* p_buffer, int p_buffer_size, WriteMode p_mode)
{
	return _send(p_buffer, p_buffer_size, p_mode == WRITE_MODE_BINARY);
}

Error WebSocketPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	return _send(p_buffer, p_buffer_size, true);
}

Error WebSocketPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	if (in_buffer.packets_left() == 0) {
		return ERR_UNAVAILABLE;
	}

	int read = 0;
	Error err =
		in_buffer.read_packet(packet_buffer.ptrw(), packet_buffer.size(), &was_string, read);
	ERR_FAIL_COND_V(err != OK, err);

	*r_buffer = packet_buffer.ptr();
	r_buffer_size = read;

	return OK;
}

int WebSocketPeer::get_available_packet_count() const
{
	return in_buffer.packets_left();
}

int WebSocketPeer::get_current_outbound_buffered_amount() const
{
	if (peer_sock != -1) {
		return godot_js_websocket_buffered_amount(peer_sock);
	}
	return 0;
}

void WebSocketPeer::_clear()
{
	if (peer_sock != -1) {
		godot_js_websocket_destroy(peer_sock);
		peer_sock = -1;
	}
	ready_state = STATE_CLOSED;
	was_string = 0;
	close_code = -1;
	close_reason.clear();
	selected_protocol.clear();
	requested_url.clear();
	in_buffer.clear();
	packet_buffer.clear();
}

void WebSocketPeer::close(int p_code, const String& p_reason)
{
	if (p_code < 0) {
		if (peer_sock != -1) {
			godot_js_websocket_destroy(peer_sock);
			peer_sock = -1;
		}
		ready_state = STATE_CLOSED;
	}
	if (ready_state == STATE_CONNECTING || ready_state == STATE_OPEN) {
		ready_state = STATE_CLOSING;
		if (peer_sock != -1) {
			godot_js_websocket_close(peer_sock, p_code, p_reason.utf8().get_data());
		}
		else {
			ready_state = STATE_CLOSED;
		}
	}
	in_buffer.clear();
	packet_buffer.clear();
}

void WebSocketPeer::poll()
{
	// Automatically polled by the navigator.
}

IPAddress WebSocketPeer::get_connected_host() const
{
	ERR_FAIL_V_MSG(IPAddress(), "Not supported in Web export.");
}

uint16_t WebSocketPeer::get_connected_port() const
{
	ERR_FAIL_V_MSG(0, "Not supported in Web export.");
}

void WebSocketPeer::set_no_delay(bool p_enabled)
{
	ERR_FAIL_MSG("'set_no_delay' is not supported in Web export.");
}

WebSocketPeer::WebSocketPeer() {}

WebSocketPeer::~WebSocketPeer() { _clear(); }

#else

// WebSocketPeer (wslay implementation)

void WebSocketPeer::Resolver::stop()
{
	if (resolver_id != IP::RESOLVER_INVALID_ID) {
		IP::erase_resolve_item(resolver_id);
		resolver_id = IP::RESOLVER_INVALID_ID;
	}
	port = 0;
}

void WebSocketPeer::Resolver::try_next_candidate(const Ref<StreamPeerSocket>&) {}

void WebSocketPeer::Resolver::start(const String&, int) {}

bool WebSocketPeer::_parse_client_request()
{
	Vector<String> psa = String::ascii(Span((const char*)handshake_buffer->get_data_array().ptr(),
										   handshake_buffer->get_position() - 4))
							 .split("\r\n");
	int len = psa.size();
	ERR_FAIL_COND_V_MSG(
		len < 4, false, "Not enough response headers, got: " + itos(len) + ", expected >= 4.");

	Vector<String> req = psa[0].split(" ", false);
	ERR_FAIL_COND_V_MSG(req.size() < 2, false, "Invalid protocol or status code.");

	// Wrong protocol
	ERR_FAIL_COND_V_MSG(
		req[0] != "GET" || req[2] != "HTTP/1.1", false, "Invalid method or HTTP version.");

	HashMap<String, String> headers;
	for (int i = 1; i < len; i++) {
		Vector<String> header = psa[i].split(":", false, 1);
		ERR_FAIL_COND_V_MSG(header.size() != 2, false, "Invalid header -> " + psa[i]);
		String name = header[0].to_lower();
		String value = header[1].strip_edges();
		if (headers.has(name)) {
			headers[name] += "," + value;
		}
		else {
			headers[name] = value;
		}
	}
	requested_host = headers.has("host") ? headers.get("host") : "";
	requested_url = (use_tls ? "wss://" : "ws://") + requested_host + req[1];
#define WSL_CHECK(NAME, VALUE)                                                                     \
	ERR_FAIL_COND_V_MSG(!headers.has(NAME) || headers[NAME].to_lower() != VALUE, false,            \
		"Missing or invalid header '" + String(NAME) + "'. Expected value '" + VALUE + "'.");
#define WSL_CHECK_EX(NAME)                                                                         \
	ERR_FAIL_COND_V_MSG(!headers.has(NAME), false, "Missing header '" + String(NAME) + "'.");
	WSL_CHECK("upgrade", "websocket");
	WSL_CHECK("sec-websocket-version", "13");
	WSL_CHECK_EX("sec-websocket-key");
	WSL_CHECK_EX("connection");
#undef WSL_CHECK_EX
#undef WSL_CHECK
	session_key = headers["sec-websocket-key"];
	if (headers.has("sec-websocket-protocol")) {
		Vector<String> protos = headers["sec-websocket-protocol"].split(",");
		for (int i = 0; i < protos.size(); i++) {
			String proto = protos[i].strip_edges();
			// Check if we have the given protocol
			for (int j = 0; j < supported_protocols.size(); j++) {
				if (proto != supported_protocols[j]) {
					continue;
				}
				selected_protocol = proto;
				break;
			}
			// Found a protocol
			if (!selected_protocol.is_empty()) {
				break;
			}
		}
		if (selected_protocol.is_empty()) { // Invalid protocol(s) requested
			return false;
		}
	}
	else if (supported_protocols.size() > 0) { // No protocol requested, but we need one
		return false;
	}
	return true;
}

Error WebSocketPeer::_do_server_handshake()
{
	if (use_tls) {
		Ref<StreamPeerTLS> tls = static_cast<Ref<StreamPeerTLS>>(connection);
		if (tls.is_null()) {
			ERR_FAIL_V_MSG(ERR_BUG, "Couldn't get StreamPeerTLS for WebSocket handshake.");
			close(-1);
			return FAILED;
		}
		tls->poll();
		if (tls->get_status() == StreamPeerTLS::STATUS_HANDSHAKING) {
			return OK; // Pending handshake
		}
		else if (tls->get_status() != StreamPeerTLS::STATUS_CONNECTED) {
			print_verbose(vformat(
				"WebSocket SSL connection error during handshake (StreamPeerTLS status code %d).",
				tls->get_status()));
			close(-1);
			return FAILED;
		}
	}

	if (pending_request) {
		int read = 0;
		while (true) {
			ERR_FAIL_COND_V_MSG(handshake_buffer->get_available_bytes() < 1, ERR_OUT_OF_MEMORY,
				"WebSocket response headers are too big.");
			int pos = handshake_buffer->get_position();
			uint8_t byte;
			Error err = connection->get_partial_data(&byte, 1, read);
			if (err != OK) { // Got an error
				print_verbose(vformat(
					"WebSocket error while getting partial data (StreamPeer error code %d).", err));
				close(-1);
				return FAILED;
			}
			else if (read != 1) { // Busy, wait next poll
				return OK;
			}
			handshake_buffer->put_u8(byte);
			const char* r = (const char*)handshake_buffer->get_data_array().ptr();
			int l = pos;
			if (l > 3 && r[l] == '\n' && r[l - 1] == '\r' && r[l - 2] == '\n' && r[l - 3] == '\r') {
				if (!_parse_client_request()) {
					close(-1);
					return FAILED;
				}
				String s = "HTTP/1.1 101 Switching Protocols\r\n";
				s += "Upgrade: websocket\r\n";
				s += "Connection: Upgrade\r\n";
				s += "Sec-WebSocket-Accept: " + _compute_key_response(session_key) + "\r\n";
				if (!selected_protocol.is_empty()) {
					s += "Sec-WebSocket-Protocol: " + selected_protocol + "\r\n";
				}
				for (int i = 0; i < handshake_headers.size(); i++) {
					s += handshake_headers[i] + "\r\n";
				}
				s += "\r\n";
				CharString cs = s.utf8();
				handshake_buffer->clear();
				handshake_buffer->put_data((const uint8_t*)cs.get_data(), cs.length());
				handshake_buffer->seek(0);
				pending_request = false;
				break;
			}
		}
	}

	if (pending_request) { // Still pending.
		return OK;
	}

	int left = handshake_buffer->get_available_bytes();
	if (left) {
		Vector<uint8_t> data = handshake_buffer->get_data_array();
		int pos = handshake_buffer->get_position();
		int sent = 0;
		Error err = connection->put_partial_data(data.ptr() + pos, left, sent);
		if (err != OK) {
			print_verbose(vformat(
				"WebSocket error while putting partial data (StreamPeer error code %d).", err));
			close(-1);
			return err;
		}
		handshake_buffer->seek(pos + sent);
		left -= sent;
		if (left == 0) {
			resolver.stop();
			// Response sent, initialize wslay context.
			wslay_event_context_server_init(&wsl_ctx, &_wsl_callbacks, this);
			wslay_event_config_set_no_buffering(wsl_ctx, 1);
			wslay_event_config_set_max_recv_msg_length(wsl_ctx, inbound_buffer_size);
			in_buffer.resize(
				Math::nearest_shift((uint32_t)inbound_buffer_size), max_queued_packets);
			packet_buffer.resize(inbound_buffer_size);
			ready_state = STATE_OPEN;
		}
	}

	return OK;
}

void WebSocketPeer::_do_client_handshake() {}

bool WebSocketPeer::_verify_server_response()
{
	Vector<String> psa = String::ascii(Span((const char*)handshake_buffer->get_data_array().ptr(),
										   handshake_buffer->get_position() - 4))
							 .split("\r\n");
	int len = psa.size();
	ERR_FAIL_COND_V_MSG(
		len < 4, false, "Not enough response headers. Got: " + itos(len) + ", expected >= 4.");

	Vector<String> req = psa[0].split(" ", false);
	ERR_FAIL_COND_V_MSG(req.size() < 2, false,
		"Invalid protocol or status code. Got '" + psa[0] + "', expected 'HTTP/1.1 101'.");

	// Wrong protocol
	ERR_FAIL_COND_V_MSG(req[0] != "HTTP/1.1", false,
		"Invalid protocol. Got: '" + req[0] + "', expected 'HTTP/1.1'.");
	ERR_FAIL_COND_V_MSG(
		req[1] != "101", false, "Invalid status code. Got: '" + req[1] + "', expected '101'.");

	HashMap<String, String> headers;
	for (int i = 1; i < len; i++) {
		Vector<String> header = psa[i].split(":", false, 1);
		ERR_FAIL_COND_V_MSG(header.size() != 2, false, "Invalid header -> " + psa[i] + ".");
		String name = header[0].to_lower();
		String value = header[1].strip_edges();
		if (headers.has(name)) {
			headers[name] += "," + value;
		}
		else {
			headers[name] = value;
		}
	}

#define WSL_CHECK(NAME, VALUE)                                                                     \
	ERR_FAIL_COND_V_MSG(!headers.has(NAME) || headers[NAME].to_lower() != VALUE, false,            \
		"Missing or invalid header '" + String(NAME) + "'. Expected value '" + VALUE + "'.");
#define WSL_CHECK_NC(NAME, VALUE)                                                                  \
	ERR_FAIL_COND_V_MSG(!headers.has(NAME) || headers[NAME] != VALUE, false,                       \
		"Missing or invalid header '" + String(NAME) + "'. Expected value '" + VALUE + "'.");
	WSL_CHECK("connection", "upgrade");
	WSL_CHECK("upgrade", "websocket");
	WSL_CHECK_NC("sec-websocket-accept", _compute_key_response(session_key));
#undef WSL_CHECK_NC
#undef WSL_CHECK
	if (supported_protocols.is_empty()) {
		// We didn't request a custom protocol
		ERR_FAIL_COND_V_MSG(headers.has("sec-websocket-protocol"), false,
			"Received unrequested sub-protocol -> " + headers["sec-websocket-protocol"]);
	}
	else {
		// We requested at least one custom protocol but didn't receive one
		ERR_FAIL_COND_V_MSG(!headers.has("sec-websocket-protocol"), false,
			"Requested sub-protocol(s) but received none.");
		// Check received sub-protocol was one of those requested.
		selected_protocol = headers["sec-websocket-protocol"];
		bool valid = false;
		for (int i = 0; i < supported_protocols.size(); i++) {
			if (supported_protocols[i] != selected_protocol) {
				continue;
			}
			valid = true;
			break;
		}
		if (!valid) {
			ERR_FAIL_V_MSG(false, "Received unrequested sub-protocol -> " + selected_protocol);
		}
	}
	return true;
}

Error WebSocketPeer::connect_to_url(const String& p_url, const Ref<TLSOptions>& p_options)
{
	ERR_FAIL_COND_V(p_url.is_empty(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_options.is_valid() && p_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(
		ready_state != STATE_CLOSED && ready_state != STATE_CLOSING, ERR_ALREADY_IN_USE);

	_clear();

	String host;
	String path;
	String scheme;
	String fragment;
	int port = 0;
	Error err = p_url.parse_url(scheme, host, port, path, fragment);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Invalid URL: " + p_url);
	if (scheme.is_empty()) {
		scheme = "ws://";
	}
	ERR_FAIL_COND_V_MSG(scheme != "ws://" && scheme != "wss://", ERR_INVALID_PARAMETER,
		vformat("Invalid protocol: \"%s\" (must be either \"ws://\" or \"wss://\").", scheme));

	use_tls = false;
	if (scheme == "wss://") {
		use_tls = true;
	}
	if (port == 0) {
		port = use_tls ? 443 : 80;
	}
	if (path.is_empty()) {
		path = "/";
	}

	ERR_FAIL_COND_V_MSG(use_tls && !StreamPeerTLS::is_available(), ERR_UNAVAILABLE,
		"WSS is not available in this build.");

	requested_url = p_url;
	requested_host = host;

	if (p_options.is_valid()) {
		tls_options = p_options;
	}
	else {
		tls_options = TLSOptions::client();
	}

	tcp.instantiate();

	resolver.start(host, port);
	resolver.try_next_candidate(tcp);

	if (tcp->get_status() != StreamPeerSocket::STATUS_CONNECTING &&
		tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		_clear();
		return FAILED;
	}
	connection = tcp;

	// Prepare handshake request.
	session_key = _generate_key();
	String request = "GET " + path + " HTTP/1.1\r\n";
	String port_string;
	if ((port != 80 && !use_tls) || (port != 443 && use_tls)) {
		port_string = ":" + itos(port);
	}
	request += "Host: " + host + port_string + "\r\n";
	request += "Upgrade: websocket\r\n";
	request += "Connection: Upgrade\r\n";
	request += "Sec-WebSocket-Key: " + session_key + "\r\n";
	request += "Sec-WebSocket-Version: 13\r\n";
	if (supported_protocols.size() > 0) {
		request += "Sec-WebSocket-Protocol: ";
		for (int i = 0; i < supported_protocols.size(); i++) {
			if (i != 0) {
				request += ",";
			}
			request += supported_protocols[i];
		}
		request += "\r\n";
	}
	for (int i = 0; i < handshake_headers.size(); i++) {
		request += handshake_headers[i] + "\r\n";
	}
	request += "\r\n";
	CharString cs = request.utf8();
	handshake_buffer->put_data((const uint8_t*)cs.get_data(), cs.length());
	handshake_buffer->seek(0);
	ready_state = STATE_CONNECTING;
	is_server = false;
	return OK;
}

Error WebSocketPeer::accept_stream(const Ref<StreamPeer>& p_stream) { return OK; }

ssize_t WebSocketPeer::_wsl_recv_callback(
	wslay_event_context_ptr ctx, uint8_t* data, size_t len, int flags, void* user_data)
{
	WebSocketPeer* peer = (WebSocketPeer*)user_data;
	Ref<StreamPeer> conn = peer->connection;
	if (conn.is_null()) {
		wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
		return -1;
	}
	// Make sure we don't read more than what our buffer can hold.
	size_t buffer_limit = MIN(peer->in_buffer.payload_space_left(),
		peer->in_buffer.packets_space_left() *
			2); // The minimum size of a websocket message is 2 bytes.
	size_t to_read = MIN(len, buffer_limit);
	if (to_read == 0) {
		wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
		return -1;
	}
	int read = 0;
	Error err = conn->get_partial_data(data, to_read, read);
	if (err != OK) {
		print_verbose(
			"Websocket get data error: " + itos(err) + ", read (should be 0!): " + itos(read));
		wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
		return -1;
	}
	if (read == 0) {
		wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
		return -1;
	}
	return read;
}

void WebSocketPeer::_wsl_recv_start_callback(wslay_event_context_ptr ctx,
	const struct wslay_event_on_frame_recv_start_arg* arg, void* user_data)
{
	WebSocketPeer* peer = (WebSocketPeer*)user_data;
	uint8_t op = arg->opcode;
	if (op == WSLAY_TEXT_FRAME || op == WSLAY_BINARY_FRAME) {
		// Get ready to process a data package.
		PendingMessage& pm = peer->pending_message;
		pm.opcode = op;
	}
}

void WebSocketPeer::_wsl_frame_recv_chunk_callback(wslay_event_context_ptr ctx,
	const struct wslay_event_on_frame_recv_chunk_arg* arg, void* user_data)
{
	WebSocketPeer* peer = (WebSocketPeer*)user_data;
	PendingMessage& pm = peer->pending_message;
	if (pm.opcode != 0) {
		// Only write the payload.
		peer->in_buffer.write_packet(arg->data, arg->data_length, nullptr);
		pm.payload_size += arg->data_length;
	}
}

ssize_t WebSocketPeer::_wsl_send_callback(
	wslay_event_context_ptr ctx, const uint8_t* data, size_t len, int flags, void* user_data)
{
	WebSocketPeer* peer = (WebSocketPeer*)user_data;
	Ref<StreamPeer> conn = peer->connection;
	if (conn.is_null()) {
		wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
		return -1;
	}
	int sent = 0;
	Error err = conn->put_partial_data(data, len, sent);
	if (err != OK) {
		wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
		return -1;
	}
	if (sent == 0) {
		wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
		return -1;
	}
	return sent;
}

int WebSocketPeer::_wsl_genmask_callback(
	wslay_event_context_ptr ctx, uint8_t* buf, size_t len, void* user_data)
{
	Error err = CryptoCore::generate_random(buf, len);
	ERR_FAIL_COND_V(err != OK, WSLAY_ERR_CALLBACK_FAILURE);
	return 0;
}

void WebSocketPeer::_wsl_msg_recv_callback(
	wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg* arg, void* user_data)
{
	WebSocketPeer* peer = (WebSocketPeer*)user_data;
	uint8_t op = arg->opcode;

	if (op == WSLAY_CONNECTION_CLOSE) {
		// Close request or confirmation.
		peer->close_code = arg->status_code;
		size_t len = arg->msg_length;
		peer->close_reason.clear();
		if (len > 2 /* first 2 bytes = close code */) {
			peer->close_reason.append_utf8((char*)arg->msg + 2, len - 2);
		}
		if (peer->ready_state == STATE_OPEN) {
			peer->ready_state = STATE_CLOSING;
		}
		return;
	}

	if (op == WSLAY_PONG) {
		peer->heartbeat_waiting = false;
	}
	else if (op == WSLAY_TEXT_FRAME || op == WSLAY_BINARY_FRAME) {
		PendingMessage& pm = peer->pending_message;
		ERR_FAIL_COND(pm.opcode != op);
		// Only write the packet (since it's now completed).
		uint8_t is_string = pm.opcode == WSLAY_TEXT_FRAME ? 1 : 0;
		peer->in_buffer.write_packet(nullptr, pm.payload_size, &is_string);
		pm.clear();
	}
	// Ping.
}

wslay_event_callbacks WebSocketPeer::_wsl_callbacks = {_wsl_recv_callback, _wsl_send_callback,
	_wsl_genmask_callback, _wsl_recv_start_callback, _wsl_frame_recv_chunk_callback, nullptr,
	_wsl_msg_recv_callback};

String WebSocketPeer::_generate_key()
{
	// Random key
	Vector<uint8_t> bkey;
	int len = 16; // 16 bytes, as per RFC
	bkey.resize(len);
	_wsl_genmask_callback(nullptr, bkey.ptrw(), len, nullptr);
	return CryptoCore::b64_encode_str(bkey.ptrw(), len);
}

String WebSocketPeer::_compute_key_response(const String& p_key)
{
	String key = p_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // Magic UUID as per RFC
	Vector<uint8_t> sha = key.sha1_buffer();
	return CryptoCore::b64_encode_str(sha.ptr(), sha.size());
}

void WebSocketPeer::poll()
{
	// Nothing to do.
	if (ready_state == STATE_CLOSED) {
		return;
	}

	if (ready_state == STATE_CONNECTING) {
		if (is_server) {
			_do_server_handshake();
		}
		else {
			_do_client_handshake();
		}
	}

	if (ready_state == STATE_OPEN || ready_state == STATE_CLOSING) {
		ERR_FAIL_NULL(wsl_ctx);
		uint64_t ticks = OS::get_ticks_msec();
		int err = 0;
		if (heartbeat_interval_msec != 0 && ticks - last_heartbeat > heartbeat_interval_msec &&
			ready_state == STATE_OPEN) {
			if (heartbeat_waiting) {
				wslay_event_context_free(wsl_ctx);
				wsl_ctx = nullptr;
				close(-1);
				return;
			}
			heartbeat_waiting = true;
			struct wslay_event_msg msg;
			msg.opcode = WSLAY_PING;
			msg.msg = nullptr;
			msg.msg_length = 0;
			err = wslay_event_queue_msg(wsl_ctx, &msg);
			if (err == 0) {
				last_heartbeat = ticks;
			}
			else {
				print_verbose("Websocket (wslay) failed to send ping: " + itos(err));
				wslay_event_context_free(wsl_ctx);
				wsl_ctx = nullptr;
				close(-1);
				return;
			}
		}
		if ((err = wslay_event_recv(wsl_ctx)) != 0 || (err = wslay_event_send(wsl_ctx)) != 0) {
			// Error close.
			print_verbose("Websocket (wslay) poll error: " + itos(err));
			wslay_event_context_free(wsl_ctx);
			wsl_ctx = nullptr;
			close(-1);
			return;
		}
		if (wslay_event_get_close_sent(wsl_ctx)) {
			if (wslay_event_get_close_received(wsl_ctx)) {
				// Clean close.
				wslay_event_context_free(wsl_ctx);
				wsl_ctx = nullptr;
				close(-1);
				return;
			}
			else if (!wslay_event_get_read_enabled(wsl_ctx)) {
				// Some protocol error caused wslay to stop processing incoming events, we'll never
				// receive a close from the other peer.
				close_code = wslay_event_get_status_code_sent(wsl_ctx);
				switch (close_code) {
				case WSLAY_CODE_MESSAGE_TOO_BIG:
					close_reason = "Message too big";
					break;
				case WSLAY_CODE_PROTOCOL_ERROR:
					close_reason = "Protocol error";
					break;
				case WSLAY_CODE_ABNORMAL_CLOSURE:
					close_reason = "Abnormal closure";
					break;
				case WSLAY_CODE_INVALID_FRAME_PAYLOAD_DATA:
					close_reason = "Invalid frame payload data";
					break;
				default:
					close_reason = "Unknown";
				}
				wslay_event_context_free(wsl_ctx);
				wsl_ctx = nullptr;
				close(-1);
				return;
			}
		}
	}
}

Error WebSocketPeer::_send(const uint8_t* p_buffer, int p_buffer_size, wslay_opcode p_opcode)
{
	ERR_FAIL_COND_V(ready_state != STATE_OPEN, FAILED);
	ERR_FAIL_COND_V(wslay_event_get_queued_msg_count(wsl_ctx) >= (uint32_t)max_queued_packets,
		ERR_OUT_OF_MEMORY);
	ERR_FAIL_COND_V(
		outbound_buffer_size > 0 && (wslay_event_get_queued_msg_length(wsl_ctx) + p_buffer_size >
										(uint32_t)outbound_buffer_size),
		ERR_OUT_OF_MEMORY);

	struct wslay_event_msg msg;
	msg.opcode = p_opcode;
	msg.msg = p_buffer;
	msg.msg_length = p_buffer_size;

	// Queue & send message.
	if (wslay_event_queue_msg(wsl_ctx, &msg) != 0 || wslay_event_send(wsl_ctx) != 0) {
		close(-1);
		return FAILED;
	}
	return OK;
}

Error WebSocketPeer::send(const uint8_t* p_buffer, int p_buffer_size, WriteMode p_mode)
{
	wslay_opcode opcode = p_mode == WRITE_MODE_TEXT ? WSLAY_TEXT_FRAME : WSLAY_BINARY_FRAME;
	return _send(p_buffer, p_buffer_size, opcode);
}

Error WebSocketPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	return _send(p_buffer, p_buffer_size, WSLAY_BINARY_FRAME);
}

Error WebSocketPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	r_buffer_size = 0;

	ERR_FAIL_COND_V(ready_state != STATE_OPEN, FAILED);

	if (in_buffer.packets_left() == 0) {
		return ERR_UNAVAILABLE;
	}

	int read = 0;
	uint8_t* rw = packet_buffer.ptrw();
	in_buffer.read_packet(rw, packet_buffer.size(), &was_string, read);

	*r_buffer = rw;
	r_buffer_size = read;

	return OK;
}

int WebSocketPeer::get_available_packet_count() const
{
	if (ready_state != STATE_OPEN) {
		return 0;
	}

	return in_buffer.packets_left();
}

int WebSocketPeer::get_current_outbound_buffered_amount() const
{
	if (ready_state != STATE_OPEN) {
		return 0;
	}

	return wslay_event_get_queued_msg_length(wsl_ctx);
}

void WebSocketPeer::close(int p_code, const String& p_reason)
{
	if (p_code < 0) {
		// Force immediate close.
		ready_state = STATE_CLOSED;
	}

	if (ready_state == STATE_OPEN && !wslay_event_get_close_sent(wsl_ctx)) {
		CharString cs = p_reason.utf8();
		wslay_event_queue_close(wsl_ctx, p_code, (uint8_t*)cs.ptr(), cs.length());
		wslay_event_send(wsl_ctx);
		ready_state = STATE_CLOSING;
	}
	else if (ready_state == STATE_CONNECTING || ready_state == STATE_CLOSED) {
		ready_state = STATE_CLOSED;
		connection.unref();
		if (tcp.is_valid()) {
			tcp->disconnect_from_host();
			tcp.unref();
		}
	}

	if (ready_state == STATE_CLOSED) {
		heartbeat_waiting = false;
		in_buffer.clear();
		packet_buffer.resize(0);
		pending_message.clear();
	}
}

IPAddress WebSocketPeer::get_connected_host() const
{
	ERR_FAIL_COND_V(tcp.is_null(), IPAddress());
	return tcp->get_connected_host();
}

uint16_t WebSocketPeer::get_connected_port() const
{
	ERR_FAIL_COND_V(tcp.is_null(), 0);
	return tcp->get_connected_port();
}

void WebSocketPeer::set_no_delay(bool p_enabled)
{
	ERR_FAIL_COND(tcp.is_null());
	tcp->set_no_delay(p_enabled);
}

void WebSocketPeer::_clear()
{
	// Connection info.
	ready_state = STATE_CLOSED;
	is_server = false;
	connection.unref();
	if (tcp.is_valid()) {
		tcp->disconnect_from_host();
		tcp.unref();
	}
	if (wsl_ctx) {
		wslay_event_context_free(wsl_ctx);
		wsl_ctx = nullptr;
	}

	resolver.stop();
	requested_url.clear();
	requested_host.clear();
	pending_request = true;
	handshake_buffer->clear();
	selected_protocol.clear();
	session_key.clear();

	// Pending packets info.
	was_string = 0;
	in_buffer.clear();
	packet_buffer.clear();

	// Close code info.
	close_code = -1;
	close_reason.clear();
}

WebSocketPeer::WebSocketPeer() { handshake_buffer.instantiate(); }

WebSocketPeer::~WebSocketPeer() { close(-1); }

#endif // WEB_ENABLED

// WebSocketMultiplayerPeer

WebSocketMultiplayerPeer::WebSocketMultiplayerPeer()
{
	peer_config = Ref<WebSocketPeer>(WebSocketPeer::create());
}

WebSocketMultiplayerPeer::~WebSocketMultiplayerPeer() { _clear(); }

Ref<WebSocketPeer> WebSocketMultiplayerPeer::_create_peer()
{
	Ref<WebSocketPeer> peer = Ref<WebSocketPeer>(WebSocketPeer::create());
	peer->set_supported_protocols(get_supported_protocols());
	peer->set_handshake_headers(get_handshake_headers());
	peer->set_inbound_buffer_size(get_inbound_buffer_size());
	peer->set_outbound_buffer_size(get_outbound_buffer_size());
	peer->set_max_queued_packets(get_max_queued_packets());
	return peer;
}

void WebSocketMultiplayerPeer::_clear()
{
	connection_status = CONNECTION_DISCONNECTED;
	unique_id = 0;
	peers_map.clear();
	tcp_server.unref();
	pending_peers.clear();
	tls_server_options.unref();
	if (current_packet.data != nullptr) {
		memfree(current_packet.data);
		current_packet.data = nullptr;
	}

	for (Packet& E : incoming_packets) {
		memfree(E.data);
		E.data = nullptr;
	}

	incoming_packets.clear();
}

int WebSocketMultiplayerPeer::get_available_packet_count() const { return incoming_packets.size(); }

Error WebSocketMultiplayerPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_COND_V(get_connection_status() != CONNECTION_CONNECTED, ERR_UNCONFIGURED);

	r_buffer_size = 0;

	if (current_packet.data != nullptr) {
		memfree(current_packet.data);
		current_packet.data = nullptr;
	}

	ERR_FAIL_COND_V(incoming_packets.is_empty(), ERR_UNAVAILABLE);

	current_packet = incoming_packets.front()->get();
	incoming_packets.pop_front();

	*r_buffer = current_packet.data;
	r_buffer_size = current_packet.size;

	return OK;
}

Error WebSocketMultiplayerPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(get_connection_status() != CONNECTION_CONNECTED, ERR_UNCONFIGURED);

	if (is_server()) {
		if (target_peer > 0) {
			ERR_FAIL_COND_V_MSG(!peers_map.has(target_peer), ERR_INVALID_PARAMETER,
				"Peer not found: " + itos(target_peer));
			get_peer(target_peer)->put_packet(p_buffer, p_buffer_size);
		}
		else {
			for (KeyValue<int, Ref<WebSocketPeer>>& E : peers_map) {
				if (target_peer && -target_peer == E.key) {
					continue; // Excluded.
				}
				E.value->put_packet(p_buffer, p_buffer_size);
			}
		}
		return OK;
	}
	else {
		return get_peer(1)->put_packet(p_buffer, p_buffer_size);
	}
}

void WebSocketMultiplayerPeer::set_target_peer(int p_target_peer) { target_peer = p_target_peer; }

int WebSocketMultiplayerPeer::get_packet_peer() const
{
	ERR_FAIL_COND_V(incoming_packets.is_empty(), 1);

	return incoming_packets.front()->get().source;
}

int WebSocketMultiplayerPeer::get_unique_id() const { return unique_id; }

int WebSocketMultiplayerPeer::get_max_packet_size() const
{
	return get_outbound_buffer_size() - PROTO_SIZE;
}

Error WebSocketMultiplayerPeer::create_server(
	int p_port, IPAddress p_bind_ip, const Ref<TLSOptions>& p_options)
{
	ERR_FAIL_COND_V(get_connection_status() != CONNECTION_DISCONNECTED, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_options.is_valid() && !p_options->is_server(), ERR_INVALID_PARAMETER);
	_clear();
	tcp_server.instantiate();
	Error err = tcp_server->listen(p_port, p_bind_ip);
	if (err != OK) {
		tcp_server.unref();
		return err;
	}
	unique_id = 1;
	connection_status = CONNECTION_CONNECTED;
	tls_server_options = p_options;
	return OK;
}

Error WebSocketMultiplayerPeer::create_client(const String& p_url, const Ref<TLSOptions>& p_options)
{
	ERR_FAIL_COND_V(get_connection_status() != CONNECTION_DISCONNECTED, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_options.is_valid() && p_options->is_server(), ERR_INVALID_PARAMETER);
	_clear();
	Ref<WebSocketPeer> peer = _create_peer();
	Error err = peer->connect_to_url(p_url, p_options);
	if (err != OK) {
		return err;
	}
	PendingPeer pending;
	pending.time = OS::get_ticks_msec();
	pending_peers[1] = pending;
	peers_map[1] = peer;
	connection_status = CONNECTION_CONNECTING;
	return OK;
}

bool WebSocketMultiplayerPeer::is_server() const { return tcp_server.is_valid(); }

void WebSocketMultiplayerPeer::poll()
{
	if (connection_status == CONNECTION_DISCONNECTED) {
		return;
	}
	if (is_server()) {
		_poll_server();
	}
	else {
		_poll_client();
	}
}

MultiplayerPeer::ConnectionStatus WebSocketMultiplayerPeer::get_connection_status() const
{
	return connection_status;
}

Ref<WebSocketPeer> WebSocketMultiplayerPeer::get_peer(int p_id) const
{
	ERR_FAIL_COND_V(!peers_map.has(p_id), Ref<WebSocketPeer>());
	return peers_map[p_id];
}

void WebSocketMultiplayerPeer::set_supported_protocols(const Vector<String>& p_protocols)
{
	peer_config->set_supported_protocols(p_protocols);
}

Vector<String> WebSocketMultiplayerPeer::get_supported_protocols() const
{
	return peer_config->get_supported_protocols();
}

void WebSocketMultiplayerPeer::set_handshake_headers(const Vector<String>& p_headers)
{
	peer_config->set_handshake_headers(p_headers);
}

Vector<String> WebSocketMultiplayerPeer::get_handshake_headers() const
{
	return peer_config->get_handshake_headers();
}

void WebSocketMultiplayerPeer::set_outbound_buffer_size(int p_buffer_size)
{
	peer_config->set_outbound_buffer_size(p_buffer_size);
}

int WebSocketMultiplayerPeer::get_outbound_buffer_size() const
{
	return peer_config->get_outbound_buffer_size();
}

void WebSocketMultiplayerPeer::set_inbound_buffer_size(int p_buffer_size)
{
	peer_config->set_inbound_buffer_size(p_buffer_size);
}

int WebSocketMultiplayerPeer::get_inbound_buffer_size() const
{
	return peer_config->get_inbound_buffer_size();
}

void WebSocketMultiplayerPeer::set_max_queued_packets(int p_max_queued_packets)
{
	peer_config->set_max_queued_packets(p_max_queued_packets);
}

int WebSocketMultiplayerPeer::get_max_queued_packets() const
{
	return peer_config->get_max_queued_packets();
}

float WebSocketMultiplayerPeer::get_handshake_timeout() const { return handshake_timeout / 1000.0; }

void WebSocketMultiplayerPeer::set_handshake_timeout(float p_timeout)
{
	ERR_FAIL_COND(p_timeout <= 0.0);
	handshake_timeout = p_timeout * 1000;
}

IPAddress WebSocketMultiplayerPeer::get_peer_address(int p_peer_id) const
{
	ERR_FAIL_COND_V(!peers_map.has(p_peer_id), IPAddress());
	return peers_map[p_peer_id]->get_connected_host();
}

int WebSocketMultiplayerPeer::get_peer_port(int p_peer_id) const
{
	ERR_FAIL_COND_V(!peers_map.has(p_peer_id), 0);
	return peers_map[p_peer_id]->get_connected_port();
}

void WebSocketMultiplayerPeer::disconnect_peer(int p_peer_id, bool p_force)
{
	ERR_FAIL_COND(!peers_map.has(p_peer_id));
	peers_map[p_peer_id]->close();
	if (p_force) {
		peers_map.erase(p_peer_id);
		if (!is_server()) {
			_clear();
		}
	}
}

void WebSocketMultiplayerPeer::close() { _clear(); }

void WebSocketMultiplayerPeer::_poll_server() {}

void WebSocketMultiplayerPeer::_poll_client() {}

// RemoteDebuggerPeerWebSocket

Error RemoteDebuggerPeerWebSocket::connect_to_host(const String& p_uri)
{
	ws_peer = Ref<WebSocketPeer>(WebSocketPeer::create());
	ERR_FAIL_COND_V(ws_peer.is_null(), ERR_BUG);

	Vector<String> protocols;
	protocols.push_back("binary"); // Compatibility for emscripten TCP-to-WebSocket.

	ws_peer->set_supported_protocols(protocols);
	ws_peer->set_max_queued_packets(max_queued_messages);
	ws_peer->set_inbound_buffer_size((1 << 23) - 1);
	ws_peer->set_outbound_buffer_size((1 << 23) - 1);

	Error err = ws_peer->connect_to_url(p_uri);
	ERR_FAIL_COND_V(err != OK, err);

	ws_peer->poll();
	WebSocketPeer::State ready_state = ws_peer->get_ready_state();
	if (ready_state != WebSocketPeer::STATE_CONNECTING &&
		ready_state != WebSocketPeer::STATE_OPEN) {
		ERR_PRINT(
			vformat("Remote Debugger: Unable to connect. State: %s.", ws_peer->get_ready_state()));
		return FAILED;
	}

	return OK;
}

bool RemoteDebuggerPeerWebSocket::is_peer_connected()
{
	return ws_peer.is_valid() && (ws_peer->get_ready_state() == WebSocketPeer::STATE_OPEN ||
									 ws_peer->get_ready_state() == WebSocketPeer::STATE_CONNECTING);
}

int RemoteDebuggerPeerWebSocket::get_max_message_size() const
{
	ERR_FAIL_COND_V(ws_peer.is_null(), 0);
	return ws_peer->get_max_packet_size();
}

void RemoteDebuggerPeerWebSocket::close()
{
	if (ws_peer.is_valid()) {
		ws_peer.unref();
	}
}

bool RemoteDebuggerPeerWebSocket::can_block() const
{
#ifdef WEB_ENABLED
	return false;
#else
	return true;
#endif
}

// Module registration

void uninitialize_websocket_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}
}

void initialize_websocket_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_WEBSOCKET_ENABLED)
