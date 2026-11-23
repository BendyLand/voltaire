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
#include "core/io/stream_peer.h"
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
#include "core/io/stream_peer.h"
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
#include "core/io/stream_peer.h"
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
#include "core/io/stream_peer.h"
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
#include "core/io/stream_peer.h"

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

// === Merged from modules/interactive_music/audio_stream_interactive.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_INTERACTIVE_MUSIC_ENABLED)
// --- Inlined from modules/interactive_music/audio_stream_interactive.h ---
#include "core/templates/local_vector.h"
#include "servers/audio/audio_stream.h"

class AudioStreamPlaybackInteractive;

class AudioStreamInteractive final : public AudioStream
{
public:
	enum Mode
	{
		MODE_CLIPS,
		MODE_PLAYLIST,
		MODE_SYNCHRONIZED,
	};

	enum TransitionFromTime
	{
		TRANSITION_FROM_TIME_IMMEDIATE,
		TRANSITION_FROM_TIME_NEXT_BEAT,
		TRANSITION_FROM_TIME_NEXT_BAR,
		TRANSITION_FROM_TIME_END,
		TRANSITION_FROM_TIME_MAX
	};

	enum TransitionToTime
	{
		TRANSITION_TO_TIME_SAME_POSITION,
		TRANSITION_TO_TIME_START,
		TRANSITION_TO_TIME_PREVIOUS_POSITION,
		TRANSITION_TO_TIME_MAX,
	};

	enum FadeMode
	{
		FADE_DISABLED,
		FADE_IN,
		FADE_OUT,
		FADE_CROSS,
		FADE_AUTOMATIC,
		FADE_MAX
	};

	enum AutoAdvanceMode
	{
		AUTO_ADVANCE_DISABLED,
		AUTO_ADVANCE_ENABLED,
		AUTO_ADVANCE_RETURN_TO_HOLD,
	};

	enum
	{
		CLIP_ANY = -1
	};

private:
	friend class AudioStreamPlaybackInteractive;

	enum
	{
		MAX_CLIPS = 63, // Because we use bitmasks for transition matching.
		MAX_TRANSITIONS = 63,
		MAX_STREAMS = 64,
		MAX_SYNC_STREAMS = 32,
	};

	struct Clip
	{
		String name;
		Ref<AudioStream> stream;

		AutoAdvanceMode auto_advance = AUTO_ADVANCE_DISABLED;
		int auto_advance_next_clip = 0;
	};

	struct Transition
	{
		TransitionFromTime from_time = TRANSITION_FROM_TIME_NEXT_BEAT;
		TransitionToTime to_time = TRANSITION_TO_TIME_START;
		FadeMode fade_mode = FADE_AUTOMATIC;
		float fade_beats = 1;
		bool use_filler_clip = false;
		int filler_clip = 0;
		bool hold_previous = false;
	};

	struct TransitionKey
	{
		uint32_t from_clip;
		uint32_t to_clip;

		bool operator==(const TransitionKey& p_key) const
		{
			return from_clip == p_key.from_clip && to_clip == p_key.to_clip;
		}

		TransitionKey(uint32_t p_from_clip = 0, uint32_t p_to_clip = 0)
		{
			from_clip = p_from_clip;
			to_clip = p_to_clip;
		}
	};

	struct TransitionKeyHasher
	{
		static _FORCE_INLINE_ uint32_t hash(const TransitionKey& p_key)
		{
			uint32_t h = hash_murmur3_one_32(p_key.from_clip);
			return hash_murmur3_one_32(p_key.to_clip, h);
		}
	};

	Mode mode = MODE_CLIPS;

	// Clips / transitions state (MODE_CLIPS)
	int sample_rate = 44100;
	bool stereo = true;
	int initial_clip = 0;
	int clip_count = 0;
	double time = 0;
	uint64_t version = 1; // Used to stop playback instances for incompatibility.
	LocalVector<Clip> clips;
	HashMap<TransitionKey, Transition, TransitionKeyHasher> transition_map;

	// Playlist & Synchronized state (MODE_PLAYLIST / MODE_SYNCHRONIZED)
	bool shuffle = false;
	bool loop = true;
	double fade_time = 0.3;
	int stream_count = 0;
	LocalVector<Ref<AudioStream>> audio_streams;
	LocalVector<float> audio_stream_volume_db;

	HashSet<AudioStreamPlaybackInteractive*> playbacks;

#ifdef TOOLS_ENABLED
	mutable String stream_name_cache;
	String _get_streams_hint() const;
#endif

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const;

	// CLIPS (MODE_CLIPS)
	void set_clip_count(int p_count);
	int get_clip_count() const;

	void set_initial_clip(int p_clip);
	int get_initial_clip() const;

	void set_clip_name(int p_clip, const String& p_name);
	String get_clip_name(int p_clip) const;

	void set_clip_stream(int p_clip, const Ref<AudioStream>& p_stream);
	Ref<AudioStream> get_clip_stream(int p_clip) const;

	void set_clip_auto_advance(int p_clip, AutoAdvanceMode p_mode);
	AutoAdvanceMode get_clip_auto_advance(int p_clip) const;

	void set_clip_auto_advance_next_clip(int p_clip, int p_index);
	int get_clip_auto_advance_next_clip(int p_clip) const;

	// TRANSITIONS (MODE_CLIPS)
	void add_transition(int p_from_clip, int p_to_clip, TransitionFromTime p_from_time,
		TransitionToTime p_to_time, FadeMode p_fade_mode, float p_fade_beats,
		bool p_use_filler_flip = false, int p_filler_clip = -1, bool p_hold_previous = false);
	TransitionFromTime get_transition_from_time(int p_from_clip, int p_to_clip) const;
	TransitionToTime get_transition_to_time(int p_from_clip, int p_to_clip) const;
	FadeMode get_transition_fade_mode(int p_from_clip, int p_to_clip) const;
	float get_transition_fade_beats(int p_from_clip, int p_to_clip) const;
	bool is_transition_using_filler_clip(int p_from_clip, int p_to_clip) const;
	int get_transition_filler_clip(int p_from_clip, int p_to_clip) const;
	bool is_transition_holding_previous(int p_from_clip, int p_to_clip) const;

	bool has_transition(int p_from_clip, int p_to_clip) const;
	void erase_transition(int p_from_clip, int p_to_clip);

	PackedInt32Array get_transition_list() const;

	// PLAYLIST & SYNCHRONIZED STREAMS
	void set_stream_count(int p_count);
	int get_stream_count() const;

	// PLAYLIST (MODE_PLAYLIST)
	void set_fade_time(float p_time);
	float get_fade_time() const;
	void set_shuffle(bool p_shuffle);
	bool get_shuffle() const;
	void set_loop(bool p_loop);
	void set_list_stream(int p_stream_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_list_stream(int p_stream_index) const;

	// SYNCHRONIZED (MODE_SYNCHRONIZED)
	void set_sync_stream(int p_stream_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_sync_stream(int p_stream_index) const;
	void set_sync_stream_volume(int p_stream_index, float p_db);
	float get_sync_stream_volume(int p_stream_index) const;

	// AudioStream overrides
	virtual double get_bpm() const override;
	virtual int get_beat_count() const override;
	virtual int get_bar_beats() const override;
	virtual bool has_loop() const override;
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual double get_length() const override;
	virtual bool is_meta_stream() const override { return true; }

	AudioStreamInteractive();
};

class AudioStreamPlaybackInteractive final : public AudioStreamPlayback
{
	friend class AudioStreamInteractive;

private:
	enum
	{
		BUFFER_SIZE = 1024,
		MIX_BUFFER_SIZE = 128,
	};

	struct ClipState
	{
		Ref<AudioStream> stream;
		Ref<AudioStreamPlayback> playback;
		bool active = false;
		double fade_wait = 0; // Time to wait until fade kicks-in
		double fade_volume = 1.0;
		double fade_speed = 0; // Fade speed, negative or positive
		int auto_advance = -1;
		bool first_mix = true;
		double previous_position = 0;

		void reset_fade()
		{
			fade_wait = 0;
			fade_volume = 1.0;
			fade_speed = 0;
		}
	};

	struct ClipsPlayback
	{
		uint64_t version = 0;
		LocalVector<ClipState> states;
		int playback_current = -1;
		int return_memory = -1;
		int switch_request = -1;
	};

	struct PlaylistPlayback
	{
		LocalVector<Ref<AudioStreamPlayback>> playback;
		LocalVector<int> play_order;
		double stream_todo = 0.0;
		int fade_index = -1;
		double fade_volume = 1.0;
		int play_index = 0;
		double offset = 0.0;
		int loop_count = 0;
	};

	struct SyncPlayback
	{
		LocalVector<Ref<AudioStreamPlayback>> playback;
	};

	Ref<AudioStreamInteractive> stream;
	bool active = false;

	AudioFrame mix_buffer[BUFFER_SIZE];
	AudioFrame temp_buffer[BUFFER_SIZE];

	ClipsPlayback clips;
	PlaylistPlayback playlist;
	SyncPlayback sync;

	// Clips helpers
	void _mix_internal(int p_frames);
	void _mix_internal_state(int p_state_idx, int p_frames);
	void _queue(int p_to_clip_index, bool p_is_auto_advance);

	// Playlist / Synchronized helpers
	void _update_order();
	void _update_playback_instances();

	int _mix_clips(AudioFrame* p_buffer, float p_rate_scale, int p_frames);
	int _mix_playlist(AudioFrame* p_buffer, float p_rate_scale, int p_frames);
	int _mix_sync(AudioFrame* p_buffer, float p_rate_scale, int p_frames);

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;
	virtual int get_loop_count() const override;
	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	virtual void tag_used_streams() override;

	void switch_to_clip_by_name(const String& p_name);
	void switch_to_clip(int p_index);
	int get_current_clip_index() const;

	AudioStreamPlaybackInteractive() = default;
	~AudioStreamPlaybackInteractive();
};

// --- End modules/interactive_music/audio_stream_interactive.h ---
#include "modules/register_module_types.h"
#include "core/config/engine.h"
#include "core/math/math_funcs.h"

AudioStreamInteractive::AudioStreamInteractive() {}

void AudioStreamInteractive::set_mode(Mode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if (mode == p_mode) {
		return;
	}

	AudioServer::lock();
	mode = p_mode;
	if (mode == MODE_SYNCHRONIZED && stream_count > MAX_SYNC_STREAMS) {
		stream_count = MAX_SYNC_STREAMS;
	}
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		if ((uint32_t)stream_count > audio_streams.size()) {
			audio_streams.resize(stream_count);
		}
		if (mode == MODE_SYNCHRONIZED && (uint32_t)stream_count > audio_stream_volume_db.size()) {
			uint32_t old_size = audio_stream_volume_db.size();
			audio_stream_volume_db.resize(stream_count);
			for (uint32_t i = old_size; i < (uint32_t)stream_count; i++) {
				audio_stream_volume_db[i] = 0.0f;
			}
		}
	}
	version++;
	for (AudioStreamPlaybackInteractive* E : playbacks) {
		E->stop();
		if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

AudioStreamInteractive::Mode AudioStreamInteractive::get_mode() const { return mode; }

Ref<AudioStreamPlayback> AudioStreamInteractive::instantiate_playback()
{
	Ref<AudioStreamPlaybackInteractive> pb;
	pb.instantiate();
	pb->stream = Ref<AudioStreamInteractive>(this);
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		pb->_update_playback_instances();
	}
	playbacks.insert(pb.ptr());
	return pb;
}

// CLIPS (MODE_CLIPS)

void AudioStreamInteractive::set_clip_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0 || p_count > MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_count > clips.size()) {
		clips.resize(p_count);
	}
	clip_count = p_count;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

int AudioStreamInteractive::get_clip_count() const { return clip_count; }

void AudioStreamInteractive::set_initial_clip(int p_clip)
{
	ERR_FAIL_INDEX(p_clip, clip_count);
	initial_clip = p_clip;
}

int AudioStreamInteractive::get_initial_clip() const { return initial_clip; }

void AudioStreamInteractive::set_clip_name(int p_clip, const String& p_name)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].name = p_name;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
}

String AudioStreamInteractive::get_clip_name(int p_clip) const
{
	ERR_FAIL_COND_V(p_clip < -1 || p_clip >= MAX_CLIPS, String());
	if (p_clip == CLIP_ANY) {
		return RTR("All Clips");
	}
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].name : String();
}

void AudioStreamInteractive::set_clip_stream(int p_clip, const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].stream = p_stream;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_clip_stream(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, Ref<AudioStream>());
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].stream : Ref<AudioStream>();
}

void AudioStreamInteractive::set_clip_auto_advance(int p_clip, AutoAdvanceMode p_mode)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance = p_mode;
}

AudioStreamInteractive::AutoAdvanceMode AudioStreamInteractive::get_clip_auto_advance(
	int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, AUTO_ADVANCE_DISABLED);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance : AUTO_ADVANCE_DISABLED;
}

void AudioStreamInteractive::set_clip_auto_advance_next_clip(int p_clip, int p_index)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance_next_clip = p_index;
}

int AudioStreamInteractive::get_clip_auto_advance_next_clip(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, -1);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance_next_clip : 0;
}

// TRANSITIONS (MODE_CLIPS)

bool AudioStreamInteractive::has_transition(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	return transition_map.has(tk);
}

void AudioStreamInteractive::erase_transition(int p_from_clip, int p_to_clip)
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND(!transition_map.has(tk));
	AudioDriverManager::get_active_driver()->lock();
	transition_map.erase(tk);
	AudioDriverManager::get_active_driver()->unlock();
}

PackedInt32Array AudioStreamInteractive::get_transition_list() const
{
	PackedInt32Array ret;

	for (const KeyValue<TransitionKey, Transition>& K : transition_map) {
		ret.push_back(K.key.from_clip);
		ret.push_back(K.key.to_clip);
	}
	return ret;
}

void AudioStreamInteractive::add_transition(int p_from_clip, int p_to_clip,
	TransitionFromTime p_from_time, TransitionToTime p_to_time, FadeMode p_fade_mode,
	float p_fade_beats, bool p_use_filler_flip, int p_filler_clip, bool p_hold_previous)
{
	ERR_FAIL_COND(p_from_clip < CLIP_ANY || p_from_clip >= clip_count);
	ERR_FAIL_COND(p_to_clip < CLIP_ANY || p_to_clip >= clip_count);
	ERR_FAIL_UNSIGNED_INDEX(p_from_time, TRANSITION_FROM_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_to_time, TRANSITION_TO_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_fade_mode, FADE_MAX);

	Transition tr;
	tr.from_time = p_from_time;
	tr.to_time = p_to_time;
	tr.fade_mode = p_fade_mode;
	tr.fade_beats = p_fade_beats;
	tr.use_filler_clip = p_use_filler_flip;
	tr.filler_clip = p_filler_clip;
	tr.hold_previous = p_hold_previous;

	TransitionKey tk(p_from_clip, p_to_clip);

	AudioDriverManager::get_active_driver()->lock();
	transition_map[tk] = tr;
	AudioDriverManager::get_active_driver()->unlock();
}

AudioStreamInteractive::TransitionFromTime AudioStreamInteractive::get_transition_from_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_FROM_TIME_END);
	return transition_map[tk].from_time;
}

AudioStreamInteractive::TransitionToTime AudioStreamInteractive::get_transition_to_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_TO_TIME_START);
	return transition_map[tk].to_time;
}

AudioStreamInteractive::FadeMode AudioStreamInteractive::get_transition_fade_mode(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), FADE_DISABLED);
	return transition_map[tk].fade_mode;
}

float AudioStreamInteractive::get_transition_fade_beats(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].fade_beats;
}

bool AudioStreamInteractive::is_transition_using_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].use_filler_clip;
}

int AudioStreamInteractive::get_transition_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].filler_clip;
}

bool AudioStreamInteractive::is_transition_holding_previous(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].hold_previous;
}

// PLAYLIST & SYNCHRONIZED STREAMS

void AudioStreamInteractive::set_stream_count(int p_count)
{
	int max_streams = (mode == MODE_SYNCHRONIZED) ? MAX_SYNC_STREAMS : MAX_STREAMS;
	ERR_FAIL_COND(p_count < 0 || p_count > max_streams);
	AudioServer::lock();
	if ((uint32_t)p_count > audio_streams.size()) {
		audio_streams.resize(p_count);
	}
	if ((uint32_t)p_count > audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_count);
		for (uint32_t i = old_size; i < (uint32_t)p_count; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	stream_count = p_count;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

int AudioStreamInteractive::get_stream_count() const { return stream_count; }

void AudioStreamInteractive::set_fade_time(float p_time) { fade_time = p_time; }

float AudioStreamInteractive::get_fade_time() const { return fade_time; }

void AudioStreamInteractive::set_shuffle(bool p_shuffle) { shuffle = p_shuffle; }

bool AudioStreamInteractive::get_shuffle() const { return shuffle; }

void AudioStreamInteractive::set_loop(bool p_loop) { loop = p_loop; }

void AudioStreamInteractive::set_list_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_list_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_sync_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream_volume(int p_stream_index, float p_db)
{
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);
	if ((uint32_t)p_stream_index >= audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_stream_index + 1);
		for (uint32_t i = old_size; i <= (uint32_t)p_stream_index; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	audio_stream_volume_db[p_stream_index] = p_db;
}

float AudioStreamInteractive::get_sync_stream_volume(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, 0);
	return (uint32_t)p_stream_index < audio_stream_volume_db.size()
			   ? audio_stream_volume_db[p_stream_index]
			   : 0.0f;
}

// AudioStream virtual overrides

double AudioStreamInteractive::get_bpm() const
{
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				if (bpm != 0.0) {
					return bpm;
				}
			}
		}
	}
	return 0.0;
}

int AudioStreamInteractive::get_beat_count() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int max_beats = 0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_beats = MAX(max_beats, audio_streams[i]->get_beat_count());
			}
		}
		return max_beats;
	}
	return 0;
}

int AudioStreamInteractive::get_bar_beats() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				int bar_beats = audio_streams[i]->get_bar_beats();
				if (bar_beats != 0) {
					return bar_beats;
				}
			}
		}
		return 0;
	}
	return AudioStream::get_bar_beats();
}

bool AudioStreamInteractive::has_loop() const
{
	switch (mode) {
	case MODE_PLAYLIST:
		return loop;
	case MODE_SYNCHRONIZED: {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid() && audio_streams[i]->has_loop()) {
				return true;
			}
		}
		return false;
	}
	case MODE_CLIPS:
	default:
		return false;
	}
}

double AudioStreamInteractive::get_length() const
{
	switch (mode) {
	case MODE_PLAYLIST: {
		double total_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				int beat_count = audio_streams[i]->get_beat_count();
				if (bpm > 0.0 && beat_count > 0) {
					total_length += beat_count * 60.0 / bpm;
				}
				else {
					total_length += audio_streams[i]->get_length();
				}
			}
		}
		return total_length;
	}
	case MODE_SYNCHRONIZED: {
		double max_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_length = MAX(max_length, audio_streams[i]->get_length());
			}
		}
		return max_length;
	}
	case MODE_CLIPS:
	default:
		return 0.0;
	}
}

#ifdef TOOLS_ENABLED

String AudioStreamInteractive::_get_streams_hint() const
{
	if (!stream_name_cache.is_empty()) {
		return stream_name_cache;
	}

	int count = MIN(clip_count, (int)clips.size());
	for (int i = 0; i < count; i++) {
		if (i > 0) {
			stream_name_cache += ",";
		}
		String n = String(clips[i].name).replace_char(',', ' ');

		if (n == "" && clips[i].stream.is_valid()) {
			if (!clips[i].stream->get_name().is_empty()) {
				n = clips[i].stream->get_name().replace_char(',', ' ');
			}
			else if (clips[i].stream->get_path().is_resource_file()) {
				n = clips[i].stream->get_path().get_file().replace_char(',', ' ');
			}
		}

		if (n == "") {
			n = "Clip " + itos(i);
		}

		stream_name_cache += n;
	}

	return stream_name_cache;
}

#endif

///////////////////////////////////////
// AudioStreamPlaybackInteractive

AudioStreamPlaybackInteractive::~AudioStreamPlaybackInteractive()
{
	if (stream.is_valid()) {
		stream->playbacks.erase(this);
	}
}

void AudioStreamPlaybackInteractive::stop()
{
	active = false;

	for (uint32_t i = 0; i < clips.states.size(); i++) {
		if (clips.states[i].playback.is_valid()) {
			clips.states[i].playback->stop();
		}
		clips.states[i].fade_speed = 0.0;
		clips.states[i].fade_volume = 0.0;
		clips.states[i].fade_wait = 0.0;
		clips.states[i].reset_fade();
		clips.states[i].active = false;
		clips.states[i].auto_advance = -1;
		clips.states[i].first_mix = true;
	}
	clips.playback_current = -1;

	for (uint32_t i = 0; i < playlist.playback.size(); i++) {
		if (playlist.playback[i].is_valid()) {
			playlist.playback[i]->stop();
		}
	}

	for (uint32_t i = 0; i < sync.playback.size(); i++) {
		if (sync.playback[i].is_valid()) {
			sync.playback[i]->stop();
		}
	}
}

void AudioStreamPlaybackInteractive::_update_order()
{
	int count = stream->stream_count;
	playlist.play_order.resize(count);
	for (int i = 0; i < count; i++) {
		playlist.play_order[i] = i;
	}

	if (stream->shuffle) {
		for (int i = 0; i < count; i++) {
			int swap_with = Math::rand() % uint32_t(count);
			SWAP(playlist.play_order[i], playlist.play_order[swap_with]);
		}
	}
}

void AudioStreamPlaybackInteractive::_update_playback_instances()
{
	stop();

	if (stream.is_null()) {
		return;
	}

	int count = stream->stream_count;
	if (stream->mode == AudioStreamInteractive::MODE_PLAYLIST) {
		playlist.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				playlist.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				playlist.playback[i].unref();
			}
		}
	}
	else if (stream->mode == AudioStreamInteractive::MODE_SYNCHRONIZED) {
		sync.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				sync.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				sync.playback[i].unref();
			}
		}
	}
}

void AudioStreamPlaybackInteractive::start(double p_from_pos)
{
	if (stream.is_null()) {
		return;
	}

	if (active) {
		stop();
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_CLIPS: {
		if (clips.states.size() < AudioStreamInteractive::MAX_CLIPS) {
			clips.states.resize(AudioStreamInteractive::MAX_CLIPS);
		}
		if (clips.version != stream->version) {
			for (int i = 0; i < AudioStreamInteractive::MAX_CLIPS; i++) {
				Ref<AudioStream> src_stream;
				if (i < stream->clip_count && (uint32_t)i < stream->clips.size()) {
					src_stream = stream->clips[i].stream;
				}
				if (clips.states[i].stream != src_stream) {
					clips.states[i].stream.unref();
					clips.states[i].playback.unref();

					clips.states[i].stream = src_stream;
					if (src_stream.is_valid()) {
						clips.states[i].playback = src_stream->instantiate_playback();
					}
				}
			}

			clips.version = stream->version;
		}

		int current = stream->initial_clip;
		if (current < 0 || current >= stream->clip_count) {
			return; // No playback possible.
		}
		if (clips.states[current].playback.is_null()) {
			return; // No playback possible.
		}
		active = true;

		_queue(current, false);
	} break;

	case AudioStreamInteractive::MODE_PLAYLIST: {
		if ((int)playlist.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		p_from_pos = MAX(0.0, p_from_pos);

		float pl_length = stream->get_length();
		if (p_from_pos >= pl_length) {
			if (!stream->loop || pl_length <= 0.0f) {
				return; // No loop, end.
			}
			p_from_pos = Math::fmod((float)p_from_pos, pl_length);
		}

		_update_order();

		playlist.play_index = -1;

		double play_ofs = p_from_pos;
		for (int i = 0; i < stream->stream_count; i++) {
			int idx = playlist.play_order[i];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid()) {
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();
				double length;
				if (bpm > 0.0 && beat_count > 0) {
					length = beat_count * 60.0 / bpm;
				}
				else {
					length = stream->audio_streams[idx]->get_length();
				}
				if (play_ofs < length) {
					playlist.play_index = i;
					playlist.stream_todo = length - play_ofs;
					break;
				}
				else {
					play_ofs -= length;
				}
			}
		}

		if (playlist.play_index == -1) {
			return;
		}

		int first_idx = playlist.play_order[playlist.play_index];
		if ((uint32_t)first_idx >= playlist.playback.size() ||
			playlist.playback[first_idx].is_null()) {
			return;
		}

		playlist.playback[first_idx]->start(play_ofs);
		playlist.fade_index = -1;
		playlist.loop_count = 0;
		playlist.offset = p_from_pos;

		active = true;
	} break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		if ((int)sync.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		for (int i = 0; i < stream->stream_count; i++) {
			if ((uint32_t)i < sync.playback.size() && sync.playback[i].is_valid()) {
				sync.playback[i]->start(p_from_pos);
				active = true;
			}
		}
	} break;
	}
}

void AudioStreamPlaybackInteractive::_queue(int p_to_clip_index, bool p_is_auto_advance)
{
	ERR_FAIL_INDEX(p_to_clip_index, stream->clip_count);
	ERR_FAIL_COND(clips.states[p_to_clip_index].playback.is_null());

	if (clips.playback_current == -1) {
		// Nothing to do, start.
		int current = p_to_clip_index;
		ClipState& state = clips.states[current];
		state.active = true;
		state.fade_wait = 0;
		state.fade_volume = 1.0;
		state.fade_speed = 0;
		state.first_mix = true;

		state.playback->start(0);

		clips.playback_current = current;

		if (stream->clips[current].auto_advance == AudioStreamInteractive::AUTO_ADVANCE_ENABLED &&
			stream->clips[current].auto_advance_next_clip >= 0 &&
			stream->clips[current].auto_advance_next_clip < stream->clip_count &&
			stream->clips[current].auto_advance_next_clip != current) {
			// prepare auto advance
			state.auto_advance = stream->clips[current].auto_advance_next_clip;
		}
		return;
	}

	for (int i = 0; i < stream->clip_count; i++) {
		if (i == clips.playback_current || i == p_to_clip_index) {
			continue;
		}
		if (clips.states[i].active &&
			clips.states[i].fade_wait > 0) { // Waiting to kick in, terminate because change of plans.
			clips.states[i].playback->stop();
			clips.states[i].reset_fade();
			clips.states[i].active = false;
		}
	}

	ClipState& from_state = clips.states[clips.playback_current];
	ClipState& to_state = clips.states[p_to_clip_index];

	AudioStreamInteractive::Transition transition; // Use an empty transition by default

	AudioStreamInteractive::TransitionKey tkeys[4] = {
		AudioStreamInteractive::TransitionKey(clips.playback_current, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			clips.playback_current, AudioStreamInteractive::CLIP_ANY),
		AudioStreamInteractive::TransitionKey(AudioStreamInteractive::CLIP_ANY, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			AudioStreamInteractive::CLIP_ANY, AudioStreamInteractive::CLIP_ANY)};

	for (int i = 0; i < 4; i++) {
		if (stream->transition_map.has(tkeys[i])) {
			transition = stream->transition_map[tkeys[i]];
			break;
		}
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_AUTOMATIC) {
		// Adjust automatic mode based on context.
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_START) {
			transition.fade_mode = AudioStreamInteractive::FADE_OUT;
		}
		else {
			transition.fade_mode = AudioStreamInteractive::FADE_CROSS;
		}
	}

	if (p_is_auto_advance) {
		transition.from_time = AudioStreamInteractive::TRANSITION_FROM_TIME_END;
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION) {
			transition.to_time = AudioStreamInteractive::TRANSITION_TO_TIME_START;
		}
	}

	// Prepare the fadeout
	float current_pos = from_state.playback->get_playback_position();

	float src_fade_wait = 0;
	float dst_seek_to = 0;
	float fade_speed = 0;
	bool src_no_loop = false;

	if (from_state.stream->get_bpm()) {
		// Check if source speed has BPM, if so, transition syncs to BPM
		float beat_sec = 60 / float(from_state.stream->get_bpm());
		switch (transition.from_time) {
		case AudioStreamInteractive::TRANSITION_FROM_TIME_IMMEDIATE: {
			src_fade_wait = 0;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BEAT: {
			float remainder = Math::fmod(current_pos, beat_sec);
			src_fade_wait = beat_sec - remainder;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BAR: {
			if (from_state.stream->get_bar_beats() > 0) {
				float bar_sec = beat_sec * from_state.stream->get_bar_beats();
				float remainder = Math::fmod(current_pos, bar_sec);
				src_fade_wait = bar_sec - remainder;
			}
			else {
				// Stream does not have a number of beats per bar - avoid NaN, and play immediately.
				src_fade_wait = 0;
			}
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_END: {
			float end = from_state.stream->get_beat_count() > 0
							? float(from_state.stream->get_beat_count() * beat_sec)
							: from_state.stream->get_length();
			if (end == 0) {
				// Stream does not have a length.
				src_fade_wait = 0;
			}
			else {
				src_fade_wait = end - current_pos;
			}

			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}

		} break;
		default: {
		}
		}
		// Fade speed also aligned to BPM
		fade_speed = 1.0 / (transition.fade_beats * beat_sec);
	}
	else {
		// Source has no BPM, so just simple transition.
		if (transition.from_time == AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			from_state.stream->get_length() > 0) {
			float end = from_state.stream->get_length();
			src_fade_wait = end - current_pos;
			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}
		}
		else {
			src_fade_wait = 0;
		}
		fade_speed = 1.0 / transition.fade_beats;
	}

	if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_PREVIOUS_POSITION &&
		to_state.stream->get_length() > 0.0) {
		dst_seek_to = to_state.previous_position;
	}
	else if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION &&
			 transition.from_time != AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			 to_state.stream->get_length() > 0.0) {
		// Seeking to basically same position as when we start fading.
		dst_seek_to = current_pos + src_fade_wait;
		float end;
		if (to_state.stream->get_bpm() > 0 && to_state.stream->get_beat_count()) {
			float beat_sec = 60 / float(to_state.stream->get_bpm());
			end = to_state.stream->get_beat_count() * beat_sec;
		}
		else {
			end = to_state.stream->get_length();
		}

		if (dst_seek_to > end) {
			// Seeking too far away.
			dst_seek_to = 0; // past end, loop to beginning.
		}

	}
	else {
		// Seek to Start
		dst_seek_to = 0.0;
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
		transition.fade_mode == AudioStreamInteractive::FADE_IN) {
		if (src_no_loop) {
			// If there is no fade in the source stream, then let it continue until it ends.
			from_state.fade_wait = 0;
			from_state.fade_speed = 0;
		}
		else {
			// Otherwise force a very quick fade to avoid clicks
			from_state.fade_wait = src_fade_wait;
			from_state.fade_speed = 1.0 / -0.001;
		}
	}
	else {
		// Regular fade.
		from_state.fade_wait = src_fade_wait;
		from_state.fade_speed = -fade_speed;
	}
	// keep volume, since it may have been fading in from something else.

	to_state.playback->start(dst_seek_to);
	to_state.active = true;
	to_state.fade_volume = 0.0;
	to_state.first_mix = true;

	int auto_advance_to = -1;

	if (stream->clips[p_to_clip_index].auto_advance ==
		AudioStreamInteractive::AUTO_ADVANCE_ENABLED) {
		int next_clip = stream->clips[p_to_clip_index].auto_advance_next_clip;
		if (next_clip >= 0 && next_clip < (int)stream->clip_count &&
			clips.states[next_clip].playback.is_valid() && next_clip != p_to_clip_index &&
			(!transition.use_filler_clip || next_clip != transition.filler_clip)) {
			auto_advance_to = next_clip;
		}
	}

	if (clips.return_memory != -1 && stream->clips[p_to_clip_index].auto_advance ==
										 AudioStreamInteractive::AUTO_ADVANCE_RETURN_TO_HOLD) {
		auto_advance_to = clips.return_memory;
		clips.return_memory = -1;
	}

	if (transition.hold_previous) {
		clips.return_memory = clips.playback_current;
	}

	if (transition.use_filler_clip && transition.filler_clip >= 0 &&
		transition.filler_clip < (int)stream->clip_count &&
		clips.states[transition.filler_clip].playback.is_valid() &&
		clips.playback_current != transition.filler_clip &&
		p_to_clip_index != transition.filler_clip) {
		ClipState& filler_state = clips.states[transition.filler_clip];

		filler_state.playback->start(0);
		filler_state.active = true;

		// Filler state does not fade (bake fade in the audio clip if you want fading).
		filler_state.fade_volume = 1.0;
		filler_state.fade_speed = 0.0;

		filler_state.fade_wait = src_fade_wait;
		filler_state.first_mix = true;

		float filler_end;
		if (filler_state.stream->get_bpm() > 0 && filler_state.stream->get_beat_count() > 0) {
			float filler_beat_sec = 60 / float(filler_state.stream->get_bpm());
			filler_end = filler_beat_sec * filler_state.stream->get_beat_count();
		}
		else {
			filler_end = filler_state.stream->get_length();
		}

		if (!filler_state.stream->has_loop()) {
			src_no_loop = true;
		}

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			// No fading, immediately start at full volume.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = 1.0; // start at full volume, as filler is meant as a transition.
		}
		else {
			// Fade enable, prepare fade.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.fade_wait = src_fade_wait + filler_end;

	}
	else {
		to_state.fade_wait = src_fade_wait;

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			to_state.fade_volume = 1.0;
			to_state.fade_speed = 0.0;
		}
		else {
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.auto_advance = auto_advance_to;
	}
}

void AudioStreamPlaybackInteractive::seek(double p_time)
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		stop();
		start(p_time);
		break;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid()) {
				sync.playback[i]->seek(p_time);
			}
		}
	} break;
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		// Seek not supported in clips mode.
		break;
	}
}

int AudioStreamPlaybackInteractive::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return _mix_playlist(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		return _mix_sync(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return _mix_clips(p_buffer, p_rate_scale, p_frames);
	}
}

int AudioStreamPlaybackInteractive::_mix_clips(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (active && clips.version != stream->version) {
		stop();
	}

	if (clips.switch_request != -1) {
		if (active) {
			_queue(clips.switch_request, false);
		}
		clips.switch_request = -1;
	}

	if (!active) {
		return 0;
	}

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, BUFFER_SIZE);
		_mix_internal(to_mix);
		for (int i = 0; i < to_mix; i++) {
			p_buffer[i] = mix_buffer[i];
		}
		p_buffer += to_mix;
		todo -= to_mix;
	}

	return p_frames;
}

void AudioStreamPlaybackInteractive::_mix_internal(int p_frames)
{
	for (int i = 0; i < p_frames; i++) {
		mix_buffer[i] = AudioFrame(0, 0);
	}

	bool any_active = false;
	int count = MIN(stream->clip_count, (int)clips.states.size());
	for (int i = 0; i < count; i++) {
		if (!clips.states[i].active) {
			continue;
		}

		_mix_internal_state(i, p_frames);

		any_active = clips.states[i].active || any_active;
	}
	if (!any_active) {
		active = false;
		clips.playback_current = -1;
	}
}

void AudioStreamPlaybackInteractive::_mix_internal_state(int p_state_idx, int p_frames)
{
	ClipState& state = clips.states[p_state_idx];
	double mix_rate = double(AudioServer::get_mix_rate());
	double frame_inc = 1.0 / mix_rate;

	int from_frame = 0;
	int queue_next = -1;

	if (state.first_mix) {
		// Did not start mixing yet, wait.
		double mix_time = p_frames * frame_inc;
		if (state.fade_wait < mix_time) {
			// time to start!
			from_frame = state.fade_wait * mix_rate;
			state.fade_wait = 0;
			if (state.fade_speed == 0.0) {
				queue_next = state.auto_advance;
			}
			clips.playback_current = p_state_idx;
			state.first_mix = false;
		}
		else {
			// This is for fade in of new stream.
			state.fade_wait -= mix_time;
			return; // Nothing to do
		}
	}

	state.previous_position = state.playback->get_playback_position();
	state.playback->mix(temp_buffer + from_frame, 1.0, p_frames - from_frame);

	double frame_fade_inc = state.fade_speed * frame_inc;
	for (int i = from_frame; i < p_frames; i++) {
		if (state.fade_wait) {
			// This is for fade out of existing stream;
			state.fade_wait -= frame_inc;
			if (state.fade_wait < 0.0) {
				state.fade_wait = 0.0;
			}
		}
		else if (frame_fade_inc > 0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume >= 1.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 1.0;
				queue_next = state.auto_advance;
			}
		}
		else if (frame_fade_inc < 0.0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume <= 0.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 0.0;
				state.playback->stop(); // Stop playback and break, no point to continue mixing
				break;
			}
		}

		mix_buffer[i] += temp_buffer[i] * state.fade_volume;
		state.previous_position += frame_inc;
	}

	if (!state.playback->is_playing()) {
		// It finished because it either reached end or faded out, so deactivate and continue.
		state.active = false;
	}
	if (queue_next != -1) {
		_queue(queue_next, true);
	}
}

int AudioStreamPlaybackInteractive::_mix_playlist(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	double time_dec = (1.0 / AudioServer::get_mix_rate());
	double fade_dec = (1.0 / stream->fade_time) / AudioServer::get_mix_rate();

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		playlist.playback[playlist.play_order[playlist.play_index]]->mix(mix_buffer, 1.0, to_mix);
		if (playlist.fade_index != -1) {
			playlist.playback[playlist.fade_index]->mix(temp_buffer, 1.0, to_mix);
		}

		playlist.offset += time_dec * to_mix;

		for (int i = 0; i < to_mix; i++) {
			*p_buffer = mix_buffer[i];
			playlist.stream_todo -= time_dec;
			if (playlist.stream_todo < 0) {
				// find next stream.
				int prev = playlist.play_order[playlist.play_index];

				for (int j = 0; j < stream->stream_count; j++) {
					playlist.play_index++;
					if (playlist.play_index >= stream->stream_count) {
						// No loop, exit.
						if (!stream->loop) {
							for (int k = i; k < todo - i; k++) {
								p_buffer[k] = AudioFrame(0, 0);
							}
							todo = to_mix;
							active = false;
							break;
						}

						_update_order();
						playlist.play_index = 0;
						playlist.loop_count++;
						playlist.offset = time_dec * (to_mix - i);
					}

					if (playlist.playback[playlist.play_order[playlist.play_index]].is_valid()) {
						break;
					}
				}

				if (!active) {
					break;
				}

				if (playlist.playback[playlist.play_order[playlist.play_index]].is_null()) {
					todo = to_mix; // Weird error.
					active = false;
					break;
				}

				bool restart = true;
				if (prev == playlist.play_order[playlist.play_index]) {
					// Went back to the same one, continue loop (if it loops) or restart if it does
					// not.
					if (stream->audio_streams[prev]->has_loop()) {
						restart = false;
					}
					playlist.fade_index = -1;
				}
				else {
					// Move current mixed data to fade buffer (temp_buffer).
					for (int j = i; j < to_mix; j++) {
						temp_buffer[j] = mix_buffer[j];
					}

					playlist.fade_index = prev;
					playlist.fade_volume = 1.0;
				}

				int idx = playlist.play_order[playlist.play_index];

				if (restart) {
					playlist.playback[idx]->start(0); // No loop, just cold-restart.
					playlist.playback[idx]->mix(
						mix_buffer + i, 1.0, to_mix - i); // Fill rest of mix buffer
				}

				// Update fade todo.
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();

				if (bpm > 0.0 && beat_count > 0) {
					playlist.stream_todo = beat_count * 60.0 / bpm;
				}
				else {
					playlist.stream_todo = stream->audio_streams[idx]->get_length();
				}
			}

			if (playlist.fade_index != -1) {
				*p_buffer += temp_buffer[i] * playlist.fade_volume;
				playlist.fade_volume -= fade_dec;
				if (playlist.fade_volume <= 0.0) {
					playlist.playback[playlist.fade_index]->stop();
					playlist.fade_index = -1;
				}
			}

			p_buffer++;
		}

		todo -= to_mix;
	}

	return p_frames;
}

int AudioStreamPlaybackInteractive::_mix_sync(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	int todo = p_frames;
	int count = MIN(stream->stream_count, (int)sync.playback.size());

	bool any_active = false;
	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		bool first = true;
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float db = (uint32_t)i < stream->audio_stream_volume_db.size()
							   ? stream->audio_stream_volume_db[i]
							   : 0.0f;
				float volume = Math::db_to_linear(db);
				if (first) {
					sync.playback[i]->mix(p_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] *= volume;
					}
					first = false;
					any_active = true;
				}
				else {
					sync.playback[i]->mix(mix_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] += mix_buffer[j] * volume;
					}
				}
			}
		}

		if (first) {
			// Nothing mixed, put zeroes.
			for (int j = 0; j < to_mix; j++) {
				p_buffer[j] = AudioFrame(0, 0);
			}
		}

		p_buffer += to_mix;
		todo -= to_mix;
	}

	if (!any_active) {
		active = false;
	}
	return p_frames;
}

void AudioStreamPlaybackInteractive::tag_used_streams()
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		if (active && (uint32_t)playlist.play_index < playlist.play_order.size()) {
			int idx = playlist.play_order[playlist.play_index];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid() && (uint32_t)idx < playlist.playback.size() &&
				playlist.playback[idx].is_valid()) {
				stream->audio_streams[idx]->tag_used(
					playlist.playback[idx]->get_playback_position());
			}
		}
		stream->tag_used(0);
		break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		if (active) {
			int count = MIN(stream->stream_count, (int)sync.playback.size());
			for (int i = 0; i < count; i++) {
				if (sync.playback[i].is_valid() && sync.playback[i]->is_playing() &&
					(uint32_t)i < stream->audio_streams.size() &&
					stream->audio_streams[i].is_valid()) {
					stream->audio_streams[i]->tag_used(sync.playback[i]->get_playback_position());
				}
			}
			stream->tag_used(0);
		}
		break;

	case AudioStreamInteractive::MODE_CLIPS:
	default: {
		int count = MIN(stream->clip_count, (int)clips.states.size());
		for (int i = 0; i < count; i++) {
			if (clips.states[i].active && !clips.states[i].first_mix &&
				clips.states[i].playback->is_playing()) {
				clips.states[i].stream->tag_used(
					clips.states[i].playback->get_playback_position());
			}
		}
		stream->tag_used(0);
	} break;
	}
}

void AudioStreamPlaybackInteractive::switch_to_clip_by_name(const String& p_name)
{
	if (p_name == String()) {
		clips.switch_request = -1;
		return;
	}

	ERR_FAIL_COND_MSG(stream.is_null(), "Attempted to switch while not playing back any stream.");

	for (int i = 0; i < stream->get_clip_count(); i++) {
		if (stream->get_clip_name(i) == p_name) {
			clips.switch_request = i;
			return;
		}
	}
	ERR_FAIL_MSG("Clip not found: " + String(p_name));
}

void AudioStreamPlaybackInteractive::switch_to_clip(int p_index)
{
	clips.switch_request = p_index;
}

int AudioStreamPlaybackInteractive::get_current_clip_index() const
{
	return clips.playback_current;
}

int AudioStreamPlaybackInteractive::get_loop_count() const
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.loop_count;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int min_loops = 0;
		bool min_loops_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				int loops = sync.playback[i]->get_loop_count();
				if (!min_loops_found || loops < min_loops) {
					min_loops = loops;
					min_loops_found = true;
				}
			}
		}
		return min_loops;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0;
	}
}

double AudioStreamPlaybackInteractive::get_playback_position() const
{
	if (stream.is_null()) {
		return 0.0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.offset;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		float max_pos = 0;
		bool pos_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float pos = sync.playback[i]->get_playback_position();
				if (!pos_found || pos > max_pos) {
					max_pos = pos;
					pos_found = true;
				}
			}
		}
		return max_pos;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0.0;
	}
}

bool AudioStreamPlaybackInteractive::is_playing() const { return active; }

void initialize_interactive_music_module(ModuleInitializationLevel p_level) {}

void uninitialize_interactive_music_module(ModuleInitializationLevel p_level)
{
	// Nothing to do here.
}
#endif // defined(MODULE_INTERACTIVE_MUSIC_ENABLED)


// --- Merged from modules/mp3/audio_stream_mp3.cpp ---
#if defined(MODULE_MP3_ENABLED)
#define DR_MP3_FLOAT_OUTPUT
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO

#include <thirdparty/dr_libs/dr_bridge.h>
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "modules/register_module_types.h"
// --- Inlined from modules/mp3/audio_stream_mp3.h ---
#include <thirdparty/dr_libs/dr_mp3.h>
#include "servers/audio/audio_stream.h"

class AudioStreamMP3;

class AudioStreamPlaybackMP3 : public AudioStreamPlaybackResampled
{
	enum
	{
		FADE_SIZE = 256
	};

	AudioFrame loop_fade[FADE_SIZE];
	int loop_fade_remaining = FADE_SIZE;

	bool looping_override = false;
	bool looping = false;
	drmp3 mp3d = {};
	uint32_t frames_mixed = 0;
	bool active = false;
	int loops = 0;

	friend class AudioStreamMP3;

	Ref<AudioStreamMP3> mp3_stream;

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackMP3() {}

	~AudioStreamPlaybackMP3();
};

class AudioStreamMP3 final : public AudioStream
{
	friend class AudioStreamPlaybackMP3;

	TightLocalVector<uint8_t> data;

	float sample_rate = 1.0;
	int channels = 1;
	float length = 0.0;
	bool loop = false;
	float loop_offset = 0.0;

	double bpm = 0;
	int beat_count = 0;
	int bar_beats = 4;

protected:

public:
	static Ref<AudioStreamMP3> load_from_buffer(const Vector<uint8_t>& p_stream_data);
	static Ref<AudioStreamMP3> load_from_file(const String& p_path);

	void set_loop(bool p_enable);
	virtual bool has_loop() const override;

	void set_loop_offset(double p_seconds);
	double get_loop_offset() const;

	void set_bpm(double p_bpm);
	virtual double get_bpm() const override;

	void set_beat_count(int p_beat_count);
	virtual int get_beat_count() const override;

	void set_bar_beats(int p_bar_beats);
	virtual int get_bar_beats() const override;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	void set_data(const Vector<uint8_t>& p_data);
	Vector<uint8_t> get_data() const;

	virtual double get_length() const override;

	virtual bool is_monophonic() const override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;
};

#include "core/io/resource_loader.h"

class ResourceImporterMP3 final : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

#ifdef TOOLS_ENABLED
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;
#endif

	virtual bool can_import_threaded() const override { return true; }

	ResourceImporterMP3() = default;
};

// --- End modules/mp3/audio_stream_mp3.h ---
#include "core/io/file_access.h"

int AudioStreamPlaybackMP3::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (!active) {
		return 0;
	}

	int todo = p_frames;

	int frames_mixed_this_step = p_frames;

	int beat_length_frames = -1;
	bool use_loop = looping_override ? looping : mp3_stream->loop;

	bool beat_loop = use_loop && mp3_stream->get_bpm() > 0 && mp3_stream->get_beat_count() > 0;
	if (beat_loop) {
		beat_length_frames =
			mp3_stream->get_beat_count() * mp3_stream->sample_rate * 60 / mp3_stream->get_bpm();
	}

	while (todo && active) {
		drmp3d_sample_t buf_frame[2];

		int samples_mixed = drmp3_read_pcm_frames_f32(&mp3d, 1, buf_frame);

		if (samples_mixed) {
			p_buffer[p_frames - todo] = AudioFrame(buf_frame[0], buf_frame[mp3d.channels - 1]);
			if (loop_fade_remaining < FADE_SIZE) {
				p_buffer[p_frames - todo] +=
					loop_fade[loop_fade_remaining] *
					(float(FADE_SIZE - loop_fade_remaining) / float(FADE_SIZE));
				loop_fade_remaining++;
			}
			--todo;
			++frames_mixed;

			if (beat_loop && (int)frames_mixed >= beat_length_frames) {
				for (int i = 0; i < FADE_SIZE; i++) {
					samples_mixed = drmp3_read_pcm_frames_f32(&mp3d, 1, buf_frame);
					loop_fade[i] = AudioFrame(buf_frame[0], buf_frame[mp3d.channels - 1]);
					if (!samples_mixed) {
						break;
					}
				}
				loop_fade_remaining = 0;
				seek(mp3_stream->loop_offset);
				loops++;
			}
		}

		else {
			// EOF
			if (use_loop) {
				seek(mp3_stream->loop_offset);
				loops++;
			}
			else {
				frames_mixed_this_step = p_frames - todo;
				// fill remainder with silence
				for (int i = p_frames - todo; i < p_frames; i++) {
					p_buffer[i] = AudioFrame(0, 0);
				}
				active = false;
				todo = 0;
			}
		}
	}
	return frames_mixed_this_step;
}

float AudioStreamPlaybackMP3::get_stream_sampling_rate() { return mp3_stream->sample_rate; }

void AudioStreamPlaybackMP3::start(double p_from_pos)
{
	active = true;
	seek(p_from_pos);
	loops = 0;
	begin_resample();
}

void AudioStreamPlaybackMP3::stop() { active = false; }

bool AudioStreamPlaybackMP3::is_playing() const { return active; }

int AudioStreamPlaybackMP3::get_loop_count() const { return loops; }

double AudioStreamPlaybackMP3::get_playback_position() const
{
	return double(frames_mixed) / mp3_stream->sample_rate;
}

void AudioStreamPlaybackMP3::seek(double p_time)
{
	if (!active) {
		return;
	}

	if (p_time >= mp3_stream->get_length()) {
		p_time = 0;
	}

	frames_mixed = uint32_t(mp3_stream->sample_rate * p_time);
	drmp3_seek_to_pcm_frame(&mp3d, (uint64_t)frames_mixed);
}

void AudioStreamPlaybackMP3::tag_used_streams() { mp3_stream->tag_used(get_playback_position()); }

void AudioStreamPlaybackMP3::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackMP3::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackMP3::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackMP3::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackMP3::~AudioStreamPlaybackMP3() { drmp3_uninit(&mp3d); }

Ref<AudioStreamPlayback> AudioStreamMP3::instantiate_playback()
{
	Ref<AudioStreamPlaybackMP3> mp3s;

	ERR_FAIL_COND_V_MSG(data.is_empty(), mp3s,
		"This AudioStreamMP3 does not have an audio file assigned "
		"to it. AudioStreamMP3 should not be created from the "
		"inspector or with `.new()`. Instead, load an audio file.");

	mp3s.instantiate();
	mp3s->mp3_stream = Ref<AudioStreamMP3>(this);

	int success = drmp3_init_memory(
		&mp3s->mp3d, data.ptr(), data.size(), (drmp3_allocation_callbacks*)&dr_alloc_calls);

	mp3s->frames_mixed = 0;
	mp3s->active = false;
	mp3s->loops = 0;

	ERR_FAIL_COND_V(!success, Ref<AudioStreamPlaybackMP3>());

	return mp3s;
}

void AudioStreamMP3::set_data(const Vector<uint8_t>& p_data)
{
	drmp3* mp3d = memnew(drmp3);
	int success = drmp3_init_memory(
		mp3d, p_data.ptr(), p_data.size(), (drmp3_allocation_callbacks*)&dr_alloc_calls);
	if (!success || mp3d->sampleRate == 0) {
		memdelete(mp3d);
		ERR_FAIL_MSG("Failed to decode mp3 file. Make sure it is a valid mp3 audio file.");
	}

	channels = mp3d->channels;
	sample_rate = mp3d->sampleRate;
	length = float(drmp3_get_pcm_frame_count(mp3d)) / (mp3d->sampleRate);

	drmp3_uninit(mp3d);
	memdelete(mp3d);

	data = p_data;
}

Vector<uint8_t> AudioStreamMP3::get_data() const { return Vector<uint8_t>(data); }

void AudioStreamMP3::set_loop(bool p_enable) { loop = p_enable; }

bool AudioStreamMP3::has_loop() const { return loop; }

void AudioStreamMP3::set_loop_offset(double p_seconds) { loop_offset = p_seconds; }

double AudioStreamMP3::get_loop_offset() const { return loop_offset; }

double AudioStreamMP3::get_length() const { return length; }

bool AudioStreamMP3::is_monophonic() const { return false; }

void AudioStreamMP3::set_bpm(double p_bpm)
{
	ERR_FAIL_COND(p_bpm < 0);
	bpm = p_bpm;
	emit_changed();
}

double AudioStreamMP3::get_bpm() const { return bpm; }

void AudioStreamMP3::set_beat_count(int p_beat_count)
{
	ERR_FAIL_COND(p_beat_count < 0);
	beat_count = p_beat_count;
	emit_changed();
}

int AudioStreamMP3::get_beat_count() const { return beat_count; }

void AudioStreamMP3::set_bar_beats(int p_bar_beats)
{
	ERR_FAIL_COND(p_bar_beats < 0);
	bar_beats = p_bar_beats;
	emit_changed();
}

int AudioStreamMP3::get_bar_beats() const { return bar_beats; }

Ref<AudioSample> AudioStreamMP3::generate_sample() const
{
	Ref<AudioSample> sample;
	sample.instantiate();
	sample->loop_mode =
		loop ? AudioSample::LoopMode::LOOP_FORWARD : AudioSample::LoopMode::LOOP_DISABLED;
	sample->loop_begin = loop_offset;
	sample->loop_end = 0;
	return sample
;
}

Ref<AudioStreamMP3> AudioStreamMP3::load_from_buffer(const Vector<uint8_t>& p_stream_data)
{
	Ref<AudioStreamMP3> mp3_stream;
	mp3_stream.instantiate();
	mp3_stream->set_data(p_stream_data);
	ERR_FAIL_COND_V_MSG(mp3_stream->get_data().is_empty(), Ref<AudioStreamMP3>(),
		"MP3 decoding failed. Check that your data is a valid MP3 audio stream.");
	return mp3_stream;
}

Ref<AudioStreamMP3> AudioStreamMP3::load_from_file(const String& p_path)
{
	const Vector<uint8_t> stream_data = FileAccess::get_file_as_bytes(p_path);
	ERR_FAIL_COND_V_MSG(
		stream_data.is_empty(), Ref<AudioStreamMP3>(), vformat("Cannot open file '%s'.", p_path));
	return load_from_buffer(stream_data);
}

String ResourceImporterMP3::get_importer_name() const { return "mp3"; }

String ResourceImporterMP3::get_visible_name() const { return "MP3"; }

void ResourceImporterMP3::get_recognized_extensions(List<String>* p_extensions) const
{
#ifndef DR_MP3_ONLY_MP3
	p_extensions->push_back("mp1");
	p_extensions->push_back("mp2");
#endif
	p_extensions->push_back("mp3");
}

String ResourceImporterMP3::get_save_extension() const { return "mp3str"; }

String ResourceImporterMP3::get_resource_type() const { return "AudioStreamMP3"; }

int ResourceImporterMP3::get_preset_count() const { return 0; }

String ResourceImporterMP3::get_preset_name(int p_idx) const { return String(); }

#ifdef TOOLS_ENABLED
bool ResourceImporterMP3::has_advanced_options() const { return false; }

void ResourceImporterMP3::show_advanced_options(const String& p_path) {}
#endif

void ResourceImporterMP3::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

#ifdef TOOLS_ENABLED
#endif

void initialize_mp3_module(ModuleInitializationLevel p_level)
{
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		Ref<ResourceImporterMP3> mp3_import;
		mp3_import.instantiate();
		ResourceFormatImporter::add_importer(mp3_import);
	}
#endif
}

void uninitialize_mp3_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_MP3_ENABLED)


// --- Merged from modules/ogg/ogg_packet_sequence.cpp ---
#if defined(MODULE_OGG_ENABLED)
// --- Inlined/Folded from modules/ogg/ogg_packet_sequence.h ---
#include <ogg/ogg.h>
#include "core/io/resource.h"

class OggPacketSequencePlayback;

class OggPacketSequence : public Resource
{
	friend class OggPacketSequencePlayback;

	// List of pages, each of which is a list of packets on that page. The innermost
	// PackedByteArrays contain complete ogg packets.
	Vector<Vector<PackedByteArray>> page_data;

	// List of the granule position for each page.
	Vector<uint64_t> page_granule_positions;

	// The page after the current last page. Similar semantics to an end() iterator.
	int64_t end_page = 0;

	uint64_t data_version = 0;

	float sampling_rate = 0;
	float length = 0;

protected:

public:
	// Pushes information about all the pages that ended on this page.
	// This should be called for each page, even for pages that no packets ended on.
	void push_page(int64_t p_granule_pos, const Vector<PackedByteArray>& p_data);

	void set_packet_granule_positions(const PackedInt64Array& p_granule_positions);
	PackedInt64Array get_packet_granule_positions() const;

	// Sets a sampling rate associated with this object. OggPacketSequence doesn't understand
	// codecs, so this value is naively stored as a convenience.
	void set_sampling_rate(float p_sampling_rate);

	// Returns a sampling rate previously set by set_sampling_rate().
	float get_sampling_rate() const;

	// Returns a length previously set by set_length().
	float get_length() const;

	// Returns the granule position of the last page in this sequence.
	int64_t get_final_granule_pos() const;

	Ref<OggPacketSequencePlayback> instantiate_playback();

	OggPacketSequence() {}

	virtual ~OggPacketSequence() {}
};

class OggPacketSequencePlayback : public RefCounted
{
	friend class OggPacketSequence;

	Ref<OggPacketSequence> ogg_packet_sequence;

	mutable int64_t page_cursor = 0;
	mutable int32_t packet_cursor = 0;

	mutable ogg_packet* packet;

	uint64_t data_version = 0;

	mutable int64_t packetno = 0;

	// Recursive bisection search for the correct page.
	uint32_t seek_page_internal(
		int64_t granule, uint32_t after_page_inclusive, uint32_t before_page_inclusive);

public:
	// Calling functions must not modify this packet.
	// Returns true on success, false on error or if there is no next packet.
	bool next_ogg_packet(ogg_packet** p_packet) const;

	// Seeks to the page such that the previous page has a granule position less than or equal to
	// this value, and the current page has a granule position greater than this value. Returns true
	// on success, false on failure.
	bool seek_page(int64_t p_granule_pos);

	// Gets the current page number.
	int64_t get_page_number() const;

	// Moves to a specific page in the stream.
	// Returns true on success, false if the page number is out of bounds.
	bool set_page_number(int64_t p_page_number);

	OggPacketSequencePlayback();
	virtual ~OggPacketSequencePlayback();
};
#include "modules/register_module_types.h"

void OggPacketSequence::push_page(int64_t p_granule_pos, const Vector<PackedByteArray>& p_data)
{
	Vector<PackedByteArray> data_stored;
	for (int i = 0; i < p_data.size(); i++) {
		data_stored.push_back(p_data[i]);
	}
	page_granule_positions.push_back(p_granule_pos);
	page_data.push_back(data_stored);
	data_version++;
}

void OggPacketSequence::set_packet_granule_positions(const PackedInt64Array& p_granule_positions)
{
	data_version++; // Update the data version so old playbacks know that they can't rely on us
					// anymore.
	page_granule_positions.clear();
	for (int page_idx = 0; page_idx < p_granule_positions.size(); page_idx++) {
		int64_t granule_pos = p_granule_positions[page_idx];
		page_granule_positions.push_back(granule_pos);
	}
}

PackedInt64Array OggPacketSequence::get_packet_granule_positions() const
{
	PackedInt64Array ret;
	for (int64_t granule_pos : page_granule_positions) {
		ret.push_back(granule_pos);
	}
	return ret;
}

void OggPacketSequence::set_sampling_rate(float p_sampling_rate)
{
	sampling_rate = p_sampling_rate;
}

float OggPacketSequence::get_sampling_rate() const { return sampling_rate; }

int64_t OggPacketSequence::get_final_granule_pos() const
{
	if (!page_granule_positions.is_empty()) {
		return page_granule_positions[page_granule_positions.size() - 1];
	}
	return -1;
}

float OggPacketSequence::get_length() const
{
	int64_t granule_pos = get_final_granule_pos();
	if (granule_pos < 0) {
		return 0;
	}
	return granule_pos / sampling_rate;
}

Ref<OggPacketSequencePlayback> OggPacketSequence::instantiate_playback()
{
	Ref<OggPacketSequencePlayback> playback;
	playback.instantiate();
	playback->ogg_packet_sequence = Ref<OggPacketSequence>(this);
	playback->data_version = data_version;

	return playback;
}


bool OggPacketSequencePlayback::next_ogg_packet(ogg_packet** p_packet) const
{
	ERR_FAIL_COND_V(data_version != ogg_packet_sequence->data_version, false);
	ERR_FAIL_COND_V(ogg_packet_sequence->page_data.is_empty(), false);
	ERR_FAIL_COND_V(ogg_packet_sequence->page_granule_positions.is_empty(), false);
	ERR_FAIL_COND_V(page_cursor >= ogg_packet_sequence->page_data.size(), false);

	// Move on to the next page if need be. This happens first to help simplify seek logic.
	while (packet_cursor >= ogg_packet_sequence->page_data[page_cursor].size()) {
		packet_cursor = 0;
		page_cursor++;
		if (page_cursor >= ogg_packet_sequence->page_data.size()) {
			return false;
		}
	}

	ERR_FAIL_COND_V(page_cursor >= ogg_packet_sequence->page_data.size(), false);

	packet->b_o_s = page_cursor == 0 && packet_cursor == 0;
	packet->e_o_s = page_cursor == ogg_packet_sequence->page_data.size() - 1 &&
					packet_cursor == ogg_packet_sequence->page_data[page_cursor].size() - 1;
	packet->granulepos = packet_cursor == ogg_packet_sequence->page_data[page_cursor].size() - 1
							 ? ogg_packet_sequence->page_granule_positions[page_cursor]
							 : -1;
	packet->packetno = packetno++;
	packet->bytes = ogg_packet_sequence->page_data[page_cursor][packet_cursor].size();
	packet->packet =
		(unsigned char*)(ogg_packet_sequence->page_data[page_cursor][packet_cursor].ptr());

	*p_packet = packet;

	packet_cursor++;

	return true;
}

uint32_t OggPacketSequencePlayback::seek_page_internal(
	int64_t granule, uint32_t after_page_inclusive, uint32_t before_page_inclusive)
{
	// FIXME: This function needs better corner case handling.
	if (before_page_inclusive == after_page_inclusive) {
		return before_page_inclusive;
	}
	uint32_t actual_middle_page =
		after_page_inclusive + (before_page_inclusive - after_page_inclusive) / 2;
	// Complicating the bisection search algorithm, the middle page might not have a packet that
	// ends on it, which means it might not have a correct granule position. Find a nearby page that
	// does have a packet ending on it.
	uint32_t bisection_page = -1;
	// Don't include before_page_inclusive because that always succeeds and will cause infinite
	// recursion later.
	for (uint32_t test_page = actual_middle_page; test_page < before_page_inclusive; test_page++) {
		if (ogg_packet_sequence->page_data[test_page].size() > 0) {
			bisection_page = test_page;
			break;
		}
	}
	// Check if we have to go backwards.
	if (bisection_page == (unsigned int)-1) {
		for (uint32_t test_page = actual_middle_page; test_page >= after_page_inclusive;
			 test_page--) {
			if (ogg_packet_sequence->page_data[test_page].size() > 0) {
				bisection_page = test_page;
				break;
			}
		}
	}
	if (bisection_page == (unsigned int)-1) {
		return -1;
	}

	int64_t bisection_granule_pos = ogg_packet_sequence->page_granule_positions[bisection_page];
	if (granule > bisection_granule_pos) {
		return seek_page_internal(granule, bisection_page + 1, before_page_inclusive);
	}
	else {
		return seek_page_internal(granule, after_page_inclusive, bisection_page);
	}
}

bool OggPacketSequencePlayback::seek_page(int64_t p_granule_pos)
{
	int correct_page =
		seek_page_internal(p_granule_pos, 0, ogg_packet_sequence->page_data.size() - 1);
	if (correct_page == -1) {
		return false;
	}

	packet_cursor = 0;
	page_cursor = correct_page;

	// Don't pretend subsequent packets are contiguous with previous ones.
	packetno = 0;

	return true;
}

int64_t OggPacketSequencePlayback::get_page_number() const { return page_cursor; }

bool OggPacketSequencePlayback::set_page_number(int64_t p_page_number)
{
	if (p_page_number >= 0 && p_page_number < ogg_packet_sequence->page_data.size()) {
		page_cursor = p_page_number;
		packet_cursor = 0;
		packetno = 0;
		return true;
	}
	return false;
}

OggPacketSequencePlayback::OggPacketSequencePlayback() { packet = new ogg_packet(); }

OggPacketSequencePlayback::~OggPacketSequencePlayback() { delete packet; }

void initialize_ogg_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_ogg_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_OGG_ENABLED)


// --- Merged from modules/vorbis/audio_stream_ogg_vorbis.cpp ---
#if defined(MODULE_VORBIS_ENABLED)
#include <ogg/ogg.h>
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "modules/register_module_types.h"
// --- Inlined from modules/vorbis/audio_stream_ogg_vorbis.h ---
#include <vorbis/codec.h>

#include "servers/audio/audio_stream.h"

class AudioStreamOggVorbis;

class AudioStreamPlaybackOggVorbis : public AudioStreamPlaybackResampled
{
	uint32_t frames_mixed = 0;
	bool active = false;
	bool looping_override = false;
	bool looping = false;
	int loops = 0;

	enum
	{
		FADE_SIZE = 256
	};

	AudioFrame loop_fade[FADE_SIZE];
	int loop_fade_remaining = FADE_SIZE;

	vorbis_info info;
	vorbis_comment comment;
	vorbis_dsp_state dsp_state;
	vorbis_block block;

	bool info_is_allocated = false;
	bool comment_is_allocated = false;
	bool dsp_state_is_allocated = false;
	bool block_is_allocated = false;

	bool ready = false;

	bool have_samples_left = false;
	bool have_packets_left = false;

	friend class AudioStreamOggVorbis;

	Ref<OggPacketSequence> vorbis_data;
	Ref<OggPacketSequencePlayback> vorbis_data_playback;
	Ref<AudioStreamOggVorbis> vorbis_stream;

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

	int _mix_frames(AudioFrame* p_buffer, int p_frames);
	int _mix_frames_vorbis(AudioFrame* p_buffer, int p_frames);

	// Allocates vorbis data structures. Returns true upon success, false on failure.
	bool _alloc_vorbis();

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackOggVorbis() {}

	~AudioStreamPlaybackOggVorbis();
};

class AudioStreamOggVorbis final : public AudioStream
{
	friend class AudioStreamPlaybackOggVorbis;

	int channels = 1;
	double length = 0.0;
	bool loop = false;
	double loop_offset = 0.0;

	// Performs a seek to the beginning of the stream, should not be called during playback!
	// Also causes allocation and deallocation.
	void maybe_update_info();

	Ref<OggPacketSequence> packet_sequence;

	double bpm = 0;
	int beat_count = 0;
	int bar_beats = 4;

public:
	static Ref<AudioStreamOggVorbis> load_from_file(const String& p_path);
	static Ref<AudioStreamOggVorbis> load_from_buffer(const Vector<uint8_t>& p_stream_data);

	void set_loop(bool p_enable);
	virtual bool has_loop() const override;

	void set_loop_offset(double p_seconds);
	double get_loop_offset() const;

	void set_bpm(double p_bpm);
	virtual double get_bpm() const override;

	void set_beat_count(int p_beat_count);
	virtual int get_beat_count() const override;

	void set_bar_beats(int p_bar_beats);
	virtual int get_bar_beats() const override;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	void set_packet_sequence(Ref<OggPacketSequence> p_packet_sequence);
	Ref<OggPacketSequence> get_packet_sequence() const;

	virtual double get_length() const override; // if supported, otherwise return 0

	virtual bool is_monophonic() const override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;
};

#include "core/io/resource_loader.h"

class ResourceImporterOggVorbis final : public ResourceImporter
{
protected:

public:
#ifdef TOOLS_ENABLED
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;
#endif

#ifndef DISABLE_DEPRECATED
	static Ref<AudioStreamOggVorbis> load_from_file(const String& p_path);
	static Ref<AudioStreamOggVorbis> load_from_buffer(const Vector<uint8_t>& p_stream_data);
#endif

	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;
	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	virtual bool can_import_threaded() const override { return true; }

	ResourceImporterOggVorbis();
};

// --- End modules/vorbis/audio_stream_ogg_vorbis.h ---
#include "core/io/file_access.h"
#include "core/templates/rb_map.h"

int AudioStreamPlaybackOggVorbis::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	ERR_FAIL_COND_V(!ready, 0);

	if (!active) {
		return 0;
	}

	int todo = p_frames;

	int beat_length_frames = -1;
	bool use_loop = looping_override ? looping : vorbis_stream->loop;

	if (use_loop && vorbis_stream->get_bpm() > 0 && vorbis_stream->get_beat_count() > 0) {
		beat_length_frames = vorbis_stream->get_beat_count() * vorbis_data->get_sampling_rate() *
							 60 / vorbis_stream->get_bpm();
	}

	while (todo > 0 && active) {
		AudioFrame* buffer = p_buffer;
		buffer += p_frames - todo;

		int to_mix = todo;
		if (beat_length_frames >= 0 && (beat_length_frames - (int)frames_mixed) < to_mix) {
			to_mix = MAX(0, beat_length_frames - (int)frames_mixed);
		}

		int mixed = _mix_frames_vorbis(buffer, to_mix);
		ERR_FAIL_COND_V(mixed < 0, 0);
		todo -= mixed;
		frames_mixed += mixed;

		if (loop_fade_remaining < FADE_SIZE) {
			int to_fade = loop_fade_remaining + MIN(FADE_SIZE - loop_fade_remaining, mixed);
			for (int i = loop_fade_remaining; i < to_fade; i++) {
				buffer[i - loop_fade_remaining] +=
					loop_fade[i] * (float(FADE_SIZE - i) / float(FADE_SIZE));
			}
			loop_fade_remaining = to_fade;
		}

		if (beat_length_frames >= 0) {
			/**
			 * Length determined by beat length
			 * This code is commented out because, in practice, it is preferred that the fade
			 * is done by the transitioner and this stream just goes on until it ends while fading
			out.
			 *
			 * End fade implementation is left here for reference in case at some point this feature
			 * is desired.

			if (!beat_loop && (int)frames_mixed > beat_length_frames - FADE_SIZE) {
				print_line("beat length fade/after mix?");
				//No loop, just fade and finish
				for (int i = 0; i < mixed; i++) {
					int idx = frames_mixed + i - mixed;
					buffer[i] *= 1.0 - float(MAX(0, (idx - (beat_length_frames - FADE_SIZE)))) /
			float(FADE_SIZE);
				}
				if ((int)frames_mixed == beat_length_frames) {
					for (int i = p_frames - todo; i < p_frames; i++) {
						p_buffer[i] = AudioFrame(0, 0);
					}
					active = false;
					break;
				}
			}
			else
			**/

			if (use_loop && beat_length_frames <= (int)frames_mixed) {
				// End of file when doing beat-based looping. <= used instead of == because importer
				// editing
				if (!have_packets_left && !have_samples_left) {
					// Nothing remaining, so do nothing.
					loop_fade_remaining = FADE_SIZE;
				}
				else {
					// Add some loop fade;
					int faded_mix = _mix_frames_vorbis(loop_fade, FADE_SIZE);

					for (int i = faded_mix; i < FADE_SIZE; i++) {
						// In case lesss was mixed, pad with zeros
						loop_fade[i] = AudioFrame(0, 0);
					}
					loop_fade_remaining = 0;
				}

				seek(vorbis_stream->loop_offset);
				loops++;
				// We still have buffer to fill, start from this element in the next iteration.
				continue;
			}
		}

		if (!have_packets_left && !have_samples_left) {
			// Actual end of file!
			bool is_not_empty = mixed > 0 || vorbis_stream->get_length() > 0;
			if (use_loop && is_not_empty) {
				// loop

				seek(vorbis_stream->loop_offset);
				loops++;
				// We still have buffer to fill, start from this element in the next iteration.

			}
			else {
				for (int i = p_frames - todo; i < p_frames; i++) {
					p_buffer[i] = AudioFrame(0, 0);
				}
				active = false;
			}
		}
	}
	return p_frames - todo;
}

int AudioStreamPlaybackOggVorbis::_mix_frames_vorbis(AudioFrame* p_buffer, int p_frames)
{
	ERR_FAIL_COND_V(!ready, p_frames);
	if (!have_samples_left) {
		ogg_packet* packet = nullptr;
		int err;

		if (!vorbis_data_playback->next_ogg_packet(&packet)) {
			have_packets_left = false;
			WARN_PRINT("ran out of packets in stream");
			return -1;
		}

		err = vorbis_synthesis(&block, packet);
		ERR_FAIL_COND_V_MSG(err != 0, p_frames, "Error during vorbis synthesis " + itos(err));

		err = vorbis_synthesis_blockin(&dsp_state, &block);
		ERR_FAIL_COND_V_MSG(
			err != 0, p_frames, "Error during vorbis block processing " + itos(err));

		have_packets_left = !packet->e_o_s;
	}

	float** pcm; // Accessed with pcm[channel_idx][sample_idx].

	int frames = vorbis_synthesis_pcmout(&dsp_state, &pcm);
	if (frames > p_frames) {
		frames = p_frames;
		have_samples_left = true;
	}
	else {
		have_samples_left = false;
	}

	if (info.channels > 1) {
		for (int frame = 0; frame < frames; frame++) {
			p_buffer[frame].left = pcm[0][frame];
			p_buffer[frame].right = pcm[1][frame];
		}
	}
	else {
		for (int frame = 0; frame < frames; frame++) {
			p_buffer[frame].left = pcm[0][frame];
			p_buffer[frame].right = pcm[0][frame];
		}
	}
	vorbis_synthesis_read(&dsp_state, frames);
	return frames;
}

float AudioStreamPlaybackOggVorbis::get_stream_sampling_rate()
{
	return vorbis_data->get_sampling_rate();
}

bool AudioStreamPlaybackOggVorbis::_alloc_vorbis()
{
	vorbis_info_init(&info);
	info_is_allocated = true;
	vorbis_comment_init(&comment);
	comment_is_allocated = true;

	ERR_FAIL_COND_V(vorbis_data.is_null(), false);
	vorbis_data_playback = vorbis_data->instantiate_playback();

	ogg_packet* packet;
	int err;

	for (int i = 0; i < 3; i++) {
		if (!vorbis_data_playback->next_ogg_packet(&packet)) {
			WARN_PRINT("Not enough packets to parse header");
			return false;
		}

		err = vorbis_synthesis_headerin(&info, &comment, packet);
		ERR_FAIL_COND_V_MSG(err != 0, false, "Error parsing header");
	}

	err = vorbis_synthesis_init(&dsp_state, &info);
	ERR_FAIL_COND_V_MSG(err != 0, false, "Error initializing dsp state");
	dsp_state_is_allocated = true;

	err = vorbis_block_init(&dsp_state, &block);
	ERR_FAIL_COND_V_MSG(err != 0, false, "Error initializing block");
	block_is_allocated = true;

	ready = true;

	return true;
}

void AudioStreamPlaybackOggVorbis::start(double p_from_pos)
{
	ERR_FAIL_COND(!ready);
	loop_fade_remaining = FADE_SIZE;
	active = true;
	seek(p_from_pos);
	loops = 0;
	begin_resample();
}

void AudioStreamPlaybackOggVorbis::stop() { active = false; }

bool AudioStreamPlaybackOggVorbis::is_playing() const { return active; }

int AudioStreamPlaybackOggVorbis::get_loop_count() const { return loops; }

double AudioStreamPlaybackOggVorbis::get_playback_position() const
{
	return double(frames_mixed) / (double)vorbis_data->get_sampling_rate();
}

void AudioStreamPlaybackOggVorbis::tag_used_streams()
{
	vorbis_stream->tag_used(get_playback_position());
}

void AudioStreamPlaybackOggVorbis::seek(double p_time)
{
	ERR_FAIL_COND(!ready);
	ERR_FAIL_COND(vorbis_stream.is_null());
	if (!active) {
		return;
	}

	if (p_time >= vorbis_stream->get_length()) {
		p_time = 0;
	}

	frames_mixed = uint32_t(vorbis_data->get_sampling_rate() * p_time);

	const int64_t desired_sample = p_time * get_stream_sampling_rate();

	if (!vorbis_data_playback->seek_page(desired_sample)) {
		WARN_PRINT("seek failed");
		return;
	}

	// We want to start decoding before the page that we expect the sample to be in (the sample may
	// be part of a partial packet across page boundaries). Otherwise, the decoder may not have
	// synchronized before reaching the sample.
	int64_t start_page_number = vorbis_data_playback->get_page_number() - 1;
	if (start_page_number < 0) {
		start_page_number = 0;
	}

	while (true) {
		ogg_packet* packet;
		int err;

		// We start at an unknown granule position.
		int64_t granule_pos = -1;

		// Decode data until we get to the desired sample or notice that we have read past it.
		vorbis_data_playback->set_page_number(start_page_number);
		vorbis_synthesis_restart(&dsp_state);

		while (true) {
			if (!vorbis_data_playback->next_ogg_packet(&packet)) {
				WARN_PRINT_ONCE("Seeking beyond limits");
				return;
			}

			err = vorbis_synthesis(&block, packet);
			if (err != OV_ENOTAUDIO) {
				ERR_FAIL_COND_MSG(err != 0, "Error during vorbis synthesis " + itos(err) + ".");

				err = vorbis_synthesis_blockin(&dsp_state, &block);
				ERR_FAIL_COND_MSG(
					err != 0, "Error during vorbis block processing " + itos(err) + ".");

				int samples_out = vorbis_synthesis_pcmout(&dsp_state, nullptr);

				if (granule_pos < 0) {
					// We don't know where we are yet, so just keep on decoding.
					err = vorbis_synthesis_read(&dsp_state, samples_out);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
				}
				else if (granule_pos + samples_out >= desired_sample) {
					// Our sample is in this block. Skip the beginning of the block up to the
					// sample, then return.
					int skip_samples = (int)(desired_sample - granule_pos);
					err = vorbis_synthesis_read(&dsp_state, skip_samples);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
					have_samples_left = skip_samples < samples_out;
					have_packets_left = !packet->e_o_s;
					return;
				}
				else {
					// Our sample is not in this block. Skip it.
					err = vorbis_synthesis_read(&dsp_state, samples_out);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
					granule_pos += samples_out;
				}
			}
			if (packet->granulepos != -1) {
				// We found an update to our granule position.
				granule_pos = packet->granulepos;
				if (granule_pos > desired_sample) {
					// We've read past our sample. We need to start on an earlier page.
					if (start_page_number == 0) {
						// We didn't find the sample even reading from the beginning.
						have_samples_left = false;
						have_packets_left = !packet->e_o_s;
						return;
					}
					start_page_number--;
					break;
				}
			}
			if (packet->e_o_s) {
				// We've reached the end of the stream and didn't find our sample.
				have_samples_left = false;
				have_packets_left = false;
				return;
			}
		}
	}
}

void AudioStreamPlaybackOggVorbis::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackOggVorbis::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackOggVorbis::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackOggVorbis::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackOggVorbis::~AudioStreamPlaybackOggVorbis()
{
	if (block_is_allocated) {
		vorbis_block_clear(&block);
	}
	if (dsp_state_is_allocated) {
		vorbis_dsp_clear(&dsp_state);
	}
	if (comment_is_allocated) {
		vorbis_comment_clear(&comment);
	}
	if (info_is_allocated) {
		vorbis_info_clear(&info);
	}
}

Ref<AudioStreamPlayback> AudioStreamOggVorbis::instantiate_playback()
{
	Ref<AudioStreamPlaybackOggVorbis> ovs;

	ERR_FAIL_COND_V(packet_sequence.is_null(), nullptr);

	ovs.instantiate();
	ovs->vorbis_stream = Ref<AudioStreamOggVorbis>(this);
	ovs->vorbis_data = packet_sequence;
	ovs->frames_mixed = 0;
	ovs->active = false;
	ovs->loops = 0;
	if (ovs->_alloc_vorbis()) {
		return ovs;
	}
	// Failed to allocate data structures.
	return nullptr;
}

void AudioStreamOggVorbis::set_packet_sequence(Ref<OggPacketSequence> p_packet_sequence)
{
	packet_sequence = p_packet_sequence;
	if (packet_sequence.is_valid()) {
		maybe_update_info();
	}
}

Ref<OggPacketSequence> AudioStreamOggVorbis::get_packet_sequence() const { return packet_sequence; }

void AudioStreamOggVorbis::set_loop(bool p_enable) { loop = p_enable; }

bool AudioStreamOggVorbis::has_loop() const { return loop; }

void AudioStreamOggVorbis::set_loop_offset(double p_seconds) { loop_offset = p_seconds; }

double AudioStreamOggVorbis::get_loop_offset() const { return loop_offset; }

double AudioStreamOggVorbis::get_length() const
{
	ERR_FAIL_COND_V(packet_sequence.is_null(), 0);
	return packet_sequence->get_length();
}

void AudioStreamOggVorbis::set_bpm(double p_bpm)
{
	ERR_FAIL_COND(p_bpm < 0);
	bpm = p_bpm;
	emit_changed();
}

double AudioStreamOggVorbis::get_bpm() const { return bpm; }

void AudioStreamOggVorbis::set_beat_count(int p_beat_count)
{
	ERR_FAIL_COND(p_beat_count < 0);
	beat_count = p_beat_count;
	emit_changed();
}

int AudioStreamOggVorbis::get_beat_count() const { return beat_count; }

void AudioStreamOggVorbis::set_bar_beats(int p_bar_beats)
{
	ERR_FAIL_COND(p_bar_beats < 2);
	bar_beats = p_bar_beats;
	emit_changed();
}

int AudioStreamOggVorbis::get_bar_beats() const { return bar_beats; }

bool AudioStreamOggVorbis::is_monophonic() const { return false; }

Ref<AudioStreamOggVorbis> AudioStreamOggVorbis::load_from_buffer(
	const Vector<uint8_t>& p_stream_data)
{
	Ref<AudioStreamOggVorbis> ogg_vorbis_stream;
	ogg_vorbis_stream.instantiate();

	Ref<OggPacketSequence> ogg_packet_sequence;
	ogg_packet_sequence.instantiate();

	ogg_stream_state stream_state;
	ogg_sync_state sync_state;
	ogg_page page;
	ogg_packet packet;
	bool initialized_stream = false;

	ogg_sync_init(&sync_state);
	const long OGG_SYNC_BUFFER_SIZE = 8192;
	int err;
	size_t cursor = 0;
	size_t packet_count = 0;
	bool done = false;
	while (!done) {
		err = ogg_sync_check(&sync_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
		while (ogg_sync_pageout(&sync_state, &page) != 1) {
			if (cursor >= size_t(p_stream_data.size())) {
				done = true;
				break;
			}
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
			char* sync_buf = ogg_sync_buffer(&sync_state, OGG_SYNC_BUFFER_SIZE);
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
			size_t copy_size = p_stream_data.size() - cursor;
			if (copy_size > OGG_SYNC_BUFFER_SIZE) {
				copy_size = OGG_SYNC_BUFFER_SIZE;
			}
			memcpy(sync_buf, &p_stream_data[cursor], copy_size);
			ogg_sync_wrote(&sync_state, copy_size);
			cursor += copy_size;
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
		}
		if (done) {
			break;
		}
		err = ogg_sync_check(&sync_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));

		// Have a page now.
		if (!initialized_stream) {
			if (ogg_stream_init(&stream_state, ogg_page_serialno(&page))) {
				ERR_FAIL_V_MSG(
					Ref<AudioStreamOggVorbis>(), "Failed allocating memory for Ogg Vorbis stream.");
			}
			initialized_stream = true;
		}
		ogg_stream_pagein(&stream_state, &page);
		err = ogg_stream_check(&stream_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg stream error " + itos(err));
		int desync_iters = 0;

		RBMap<uint64_t, Vector<Vector<uint8_t>>> sorted_packets;
		int64_t granule_pos = 0;

		while (true) {
			err = ogg_stream_packetout(&stream_state, &packet);
			if (err == -1) {
				// According to the docs this is usually recoverable, but don't sit here spinning
				// forever.
				desync_iters++;
				WARN_PRINT_ONCE("Desync during ogg import.");
				ERR_FAIL_COND_V_MSG(desync_iters > 100, Ref<AudioStreamOggVorbis>(),
					"Packet sync issue during Ogg import");
				continue;
			}
			else if (err == 0) {
				// Not enough data to fully reconstruct a packet. Go on to the next page.
				break;
			}
			if (packet_count == 0 && vorbis_synthesis_idheader(&packet) == 0) {
				print_verbose("Found a non-vorbis-header packet in a header position");
				// Clearly this logical stream is not a vorbis stream, so destroy it and try again
				// with the next page.
				if (initialized_stream) {
					ogg_stream_clear(&stream_state);
					initialized_stream = false;
				}
				break;
			}
			if (packet.granulepos > granule_pos) {
				granule_pos = packet.granulepos;
			}

			if (packet.bytes > 0) {
				PackedByteArray data;
				data.resize(packet.bytes);
				memcpy(data.ptrw(), packet.packet, packet.bytes);
				sorted_packets[granule_pos].push_back(data);
				packet_count++;
			}
		}
		Vector<Vector<uint8_t>> packet_data;
		for (const KeyValue<uint64_t, Vector<Vector<uint8_t>>>& pair : sorted_packets) {
			for (const Vector<uint8_t>& packets : pair.value) {
				packet_data.push_back(packets);
			}
		}
		if (initialized_stream && packet_data.size() > 0) {
			ogg_packet_sequence->push_page(ogg_page_granulepos(&page), packet_data);
		}
	}
	if (initialized_stream) {
		ogg_stream_clear(&stream_state);
	}
	ogg_sync_clear(&sync_state);

	if (ogg_packet_sequence->get_packet_granule_positions().is_empty()) {
		ERR_FAIL_V_MSG(Ref<AudioStreamOggVorbis>(),
			"Ogg Vorbis decoding failed. Check that your data is a valid Ogg Vorbis audio stream.");
	}

	ogg_vorbis_stream->set_packet_sequence(ogg_packet_sequence);

	return ogg_vorbis_stream;
}

Ref<AudioStreamOggVorbis> AudioStreamOggVorbis::load_from_file(const String& p_path)
{
	const Vector<uint8_t> stream_data = FileAccess::get_file_as_bytes(p_path);
	ERR_FAIL_COND_V_MSG(stream_data.is_empty(), Ref<AudioStreamOggVorbis>(),
		vformat("Cannot open file '%s'.", p_path));
	return load_from_buffer(stream_data);
}

void AudioStreamOggVorbis::maybe_update_info() {}

Ref<AudioSample> AudioStreamOggVorbis::generate_sample() const { return Ref<AudioSample>(); }

String ResourceImporterOggVorbis::get_importer_name() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_visible_name() const { return "Ogg Vorbis"; }

void ResourceImporterOggVorbis::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogg");
}

String ResourceImporterOggVorbis::get_save_extension() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_resource_type() const { return "AudioStreamOggVorbis"; }

#ifdef TOOLS_ENABLED
bool ResourceImporterOggVorbis::has_advanced_options() const { return false; }

void ResourceImporterOggVorbis::show_advanced_options(const String& p_path) {}
#endif

#ifndef DISABLE_DEPRECATED
Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_buffer(
	const Vector<uint8_t>& p_stream_data)
{
	return AudioStreamOggVorbis::load_from_buffer(p_stream_data);
}

Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_file(const String& p_path)
{
	return AudioStreamOggVorbis::load_from_file(p_path);
}
#endif

ResourceImporterOggVorbis::ResourceImporterOggVorbis() {}

int ResourceImporterOggVorbis::get_preset_count() const { return 0; }

String ResourceImporterOggVorbis::get_preset_name(int p_preset) const { return String(); }

void ResourceImporterOggVorbis::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

#ifdef TOOLS_ENABLED
#endif

void initialize_vorbis_module(ModuleInitializationLevel p_level)
{
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		Ref<ResourceImporterOggVorbis> ogg_vorbis_importer;
		ogg_vorbis_importer.instantiate();
		ResourceFormatImporter::add_importer(ogg_vorbis_importer);
	}
#endif
}

void uninitialize_vorbis_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_VORBIS_ENABLED)


// --- Merged from modules/theora/video_stream_theora.cpp ---
#if defined(MODULE_THEORA_ENABLED)
// --- Inlined from modules/theora/video_stream_theora.h ---
#include "core/io/resource_loader.h"

class ResourceFormatLoaderTheora final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};

#ifdef TOOLS_ENABLED
#include <theora/theoraenc.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include "core/io/file_access.h"
#include "servers/audio/audio_server.h"
#include "main/main.h"

class MovieWriterOGV final : public MovieWriter
{
	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;
	uint32_t audio_ch = 0;
	uint32_t audio_frames = 0;

	Ref<FileAccess> f;

	// Vorbis quality -0.1 to 1 (-0.1 yields smallest files but lowest fidelity; 1 yields highest
	// fidelity but large files. '0.2' is a reasonable default).
	float audio_quality = 0.5;

	// Bitrate target for Theora video.
	int video_bitrate = 0;

	// Theora quality selector from 0 to 1.0 (0 yields smallest files but lowest video quality. 1.0
	// yields highest fidelity but large files).
	float video_quality = 0.75;

	// Video stream keyframe frequency (one every N frames).
	ogg_uint32_t keyframe_frequency = 64;

	// Sets the encoder speed level. Higher speed levels favor quicker encoding over better quality
	// per bit. Depending on the encoding mode, and the internal algorithms used, quality may
	// actually improve with higher speeds, but in this case bitrate will also likely increase. The
	// maximum value, and the meaning of each value, are implementation-specific and may change
	// depending on the current encoding mode.
	int speed = 4;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state to;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state vo;

	// Theora encoding context.
	th_enc_ctx* td;

	// Theora bitstream information.
	th_info ti;

	// Theora comment information.
	th_comment tc;

	// Vorbis bitstream information.
	vorbis_info vi;

	// Vorbis comment information.
	vorbis_comment vc;

	// Central working state for the packet->PCM decoder.
	vorbis_dsp_state vd;

	// Local working space for packet->PCM decode.
	vorbis_block vb;

	// Video buffer.
	uint8_t *y, *u, *v;
	th_ycbcr_buffer ycbcr;

	bool audio_flag = false;
	bool video_flag = false;
	ogg_page audio_page;
	ogg_page video_page;
	ogg_page backup_page;
	unsigned int backup_page_size = 0;
	unsigned char* backup_page_data = nullptr;

	void write_to_file(bool p_finish = false);
	void push_audio(const int32_t* p_audio_data);
	void push_video(const Ref<Image>& p_image);
	void pull_audio(bool p_last = false);
	void pull_video(bool p_last = false);
	void save_page(ogg_page page);
	void restore_page(ogg_page* page);

	inline int ilog(unsigned _v)
	{
		int ret;
		for (ret = 0; _v; ret++) {
			_v >>= 1;
		}
		return ret;
	}

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String>* r_extensions) const override;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path) override;
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String& p_path) const override;

public:
	MovieWriterOGV();
};
#endif // TOOLS_ENABLED

// --- End modules/theora/video_stream_theora.h ---
#include "modules/register_module_types.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"

#include <theora/theoradec.h>
#include <thirdparty/misc/yuv2rgb.h>
#include <vorbis/codec.h>

struct VideoStreamPlayback::TheoraData
{
	Image::Format format = Image::Format::FORMAT_L8;
	Vector<uint8_t> frame_data;
	int frames_pending = 0;
	Ref<FileAccess> file;
	String file_name;
	Point2i size;
	Rect2i region;

	float* audio_buffer = nullptr;
	int audio_buffer_size = 0;
	int audio_ptr_start = 0;
	int audio_ptr_end = 0;

	bool theora_eos = false;
	bool vorbis_eos = false;

	ogg_sync_state oy = {};
	ogg_stream_state vo = {};
	ogg_stream_state to = {};
	th_info ti = {};
	th_comment tc = {};
	th_dec_ctx* td = nullptr;
	vorbis_info vi = {};
	vorbis_dsp_state vd = {};
	vorbis_block vb = {};
	vorbis_comment vc = {};
	th_pixel_fmt px_fmt = TH_PF_420;
	double frame_duration = 0;
	double stream_length = 0;
	int64_t stream_data_offset = 0;
	int64_t stream_data_size = 0;

	int pp_level_max = 0;
	int pp_level = 0;
	int pp_inc = 0;

	bool playing = false;
	bool paused = false;

	bool dup_frame = false;
	bool has_video = false;
	bool has_audio = false;
	bool video_ready = false;
	bool video_done = false;
	bool audio_done = false;

	double time = 0;
	double next_frame_time = 0;
	double current_frame_time = 0;
	double delay_compensation = 0;

	Ref<ImageTexture> texture;
	int audio_track = 0;

	int buffer_data()
	{
		char* buffer = ogg_sync_buffer(&oy, 4096);
		uint64_t bytes = file->get_buffer((uint8_t*)buffer, 4096);
		ogg_sync_wrote(&oy, bytes);
		return bytes;
	}

	int queue_page(ogg_page* page)
	{
		ogg_stream_pagein(&to, page);
		if (to.e_o_s) {
			theora_eos = true;
		}
		if (has_audio) {
			ogg_stream_pagein(&vo, page);
			if (vo.e_o_s) {
				vorbis_eos = true;
			}
		}
		return 0;
	}

	int read_page(ogg_page* page)
	{
		int ret = 0;
		while (ret <= 0) {
			ret = ogg_sync_pageout(&oy, page);
			if (ret <= 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					return 0;
				}
			}
		}
		return ret;
	}

	double get_page_time(ogg_page* page)
	{
		uint64_t granulepos = ogg_page_granulepos(page);
		int page_serialno = ogg_page_serialno(page);
		double page_time = -1;

		if (page_serialno == to.serialno) {
			page_time = th_granule_time(td, granulepos);
		}
		if (has_audio && page_serialno == vo.serialno) {
			page_time = vorbis_granule_time(&vd, granulepos);
		}

		return page_time;
	}

	int feed_pages()
	{
		int pages = 0;
		ogg_page og;

		while (pages == 0) {
			while (ogg_sync_pageout(&oy, &og) > 0) {
				queue_page(&og);
				pages++;
			}
			if (pages == 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					break;
				}
			}
		}

		return pages;
	}

	int64_t seek_streams(
		double p_time, int64_t& cur_video_granulepos, int64_t& cur_audio_granulepos)
	{
		const int64_t min_seek = 512 * 1024;
		int64_t target_video_granulepos;
		int64_t target_audio_granulepos;
		double target_time = 0;
		int64_t seek_pos;

		if (p_time == -1) {
			seek_pos = MAX(stream_data_size - min_seek, stream_data_offset);
			target_video_granulepos = INT64_MAX;
			target_audio_granulepos = INT64_MAX;
		}
		else {
			int64_t video_frame = (int64_t)(p_time / frame_duration);
			target_video_granulepos = MAX(1LL, video_frame - (1LL << ti.keyframe_granule_shift))
									  << ti.keyframe_granule_shift;
			target_audio_granulepos = 0;
			seek_pos = MAX(((target_video_granulepos >> ti.keyframe_granule_shift) - 1) *
							   frame_duration * stream_data_size / stream_length,
				stream_data_offset);
			target_time = th_granule_time(td, target_video_granulepos);
			if (has_audio) {
				target_audio_granulepos = video_frame * frame_duration * vi.rate;
				target_time = MIN(target_time, vorbis_granule_time(&vd, target_audio_granulepos));
			}
		}

		int64_t video_seek_pos = seek_pos;
		int64_t audio_seek_pos = seek_pos;
		double backtrack_time = 0;
		bool video_catch = false;
		bool audio_catch = false;
		int64_t last_video_granule_seek_pos = seek_pos;
		int64_t last_audio_granule_seek_pos = seek_pos;

		cur_video_granulepos = -1;
		cur_audio_granulepos = -1;

		while (!video_catch || (has_audio && !audio_catch)) {
			if (seek_pos < stream_data_offset) {
				seek_pos = stream_data_offset;
			}
			file->seek(seek_pos);
			ogg_sync_reset(&oy);

			backtrack_time = 0;
			last_video_granule_seek_pos = seek_pos;
			last_audio_granule_seek_pos = seek_pos;
			while (!video_catch || (has_audio && !audio_catch)) {
				ogg_page page;
				uint64_t last_seek_pos = file->get_position() - oy.fill + oy.returned;
				int ret = read_page(&page);
				if (ret <= 0) {
					if (seek_pos < stream_data_offset) {
						return -1;
					}
					seek_pos -= min_seek;
					break;
				}
				int64_t cur_granulepos = ogg_page_granulepos(&page);
				if (cur_granulepos >= 0) {
					int page_serialno = ogg_page_serialno(&page);
					if (!video_catch && page_serialno == to.serialno) {
						if (cur_granulepos >= target_video_granulepos) {
							video_catch = true;
							if (cur_video_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + th_granule_time(td, cur_granulepos) - target_time);
							}
						}
						else {
							video_seek_pos = last_video_granule_seek_pos;
							cur_video_granulepos = cur_granulepos;
						}
						last_video_granule_seek_pos = last_seek_pos;
					}
					if ((has_audio && !audio_catch) && page_serialno == vo.serialno) {
						if (cur_granulepos >= target_audio_granulepos) {
							audio_catch = true;
							if (cur_audio_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + vorbis_granule_time(&vd, cur_granulepos) - target_time);
							}
						}
						else {
							audio_seek_pos = last_audio_granule_seek_pos;
							cur_audio_granulepos = cur_granulepos;
						}
						last_audio_granule_seek_pos = last_seek_pos;
					}
				}
			}
			if (backtrack_time > 0) {
				if (seek_pos <= stream_data_offset) {
					break;
				}
				int64_t delta_seek = MAX(backtrack_time * stream_data_size / stream_length, min_seek);
				seek_pos -= delta_seek;
			}
			video_catch = cur_video_granulepos != -1;
			audio_catch = cur_audio_granulepos != -1;
		}

		if (cur_video_granulepos < (1LL << ti.keyframe_granule_shift)) {
			video_seek_pos = stream_data_offset;
			cur_video_granulepos = 1LL << ti.keyframe_granule_shift;
		}
		if (has_audio) {
			if (cur_audio_granulepos == -1) {
				audio_seek_pos = stream_data_offset;
				cur_audio_granulepos = 0;
			}
			seek_pos = MIN(video_seek_pos, audio_seek_pos);
		}
		else {
			seek_pos = video_seek_pos;
		}

		return seek_pos;
	}

	void video_write(th_ycbcr_buffer yuv)
	{
		uint8_t* w = frame_data.ptrw();
		char* dst = (char*)w;
		uint32_t y_offset = region.position.y * yuv[0].stride + region.position.x;
		uint32_t uv_offset = 0;

		if (px_fmt == TH_PF_444) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x;
			yuv444_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_422) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x / 2;
			yuv422_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_420) {
			uv_offset += region.position.y * yuv[1].stride / 2 + region.position.x / 2;
			yuv420_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}

		Ref<Image> img;
		img.instantiate(region.size.x, region.size.y, false, Image::FORMAT_RGBA8, frame_data);
		texture->update(img);
	}

	void clear()
	{
		if (!file.is_null()) {
			file.unref();
		}
		if (has_audio) {
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vd);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			ogg_stream_clear(&vo);
			if (audio_buffer_size) {
				memdelete_arr(audio_buffer);
			}
		}
		if (has_video) {
			th_decode_free(td);
			th_comment_clear(&tc);
			th_info_clear(&ti);
			ogg_stream_clear(&to);
			ogg_sync_clear(&oy);
		}

		audio_buffer = nullptr;
		playing = false;
		has_video = false;
		has_audio = false;
		theora_eos = false;
		vorbis_eos = false;
	}

	void find_streams(th_setup_info*& ts)
	{
		ogg_stream_state test;
		ogg_packet op;
		ogg_page og;
		int stateflag = 0;
		int audio_track_skip = audio_track;

		while (!stateflag) {
			int ret = buffer_data();
			if (!ret) {
				break;
			}
			while (ogg_sync_pageout(&oy, &og) > 0) {
				if (!ogg_page_bos(&og)) {
					queue_page(&og);
					stateflag = 1;
					break;
				}

				ogg_stream_init(&test, ogg_page_serialno(&og));
				ogg_stream_pagein(&test, &og);
				ogg_stream_packetout(&test, &op);

				if (!has_video && th_decode_headerin(&ti, &tc, &ts, &op) >= 0) {
					memcpy(&to, &test, sizeof(test));
					has_video = true;
				}
				else if (!has_audio && vorbis_synthesis_headerin(&vi, &vc, &op) >= 0) {
					if (audio_track_skip) {
						vorbis_info_clear(&vi);
						vorbis_comment_clear(&vc);
						ogg_stream_clear(&test);
						vorbis_info_init(&vi);
						vorbis_comment_init(&vc);
						audio_track_skip--;
					}
					else {
						memcpy(&vo, &test, sizeof(test));
						has_audio = true;
					}
				}
				else {
					ogg_stream_clear(&test);
				}
			}
		}
	}

	void read_headers(th_setup_info*& ts)
	{
		ogg_packet op;
		int theora_header_packets = 1;
		int vorbis_header_packets = 1;

		while (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
			while (theora_header_packets < 3 && ogg_stream_packetout(&to, &op) > 0) {
				if (th_decode_headerin(&ti, &tc, &ts, &op) > 0) {
					theora_header_packets++;
				}
			}

			while (has_audio && vorbis_header_packets < 3 && ogg_stream_packetout(&vo, &op) > 0) {
				if (!vorbis_synthesis_headerin(&vi, &vc, &op)) {
					vorbis_header_packets++;
				}
			}

			if (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
				ogg_page page;
				if (read_page(&page)) {
					queue_page(&page);
				}
				else {
					fprintf(stderr, "End of file while searching for codec headers.\n");
					break;
				}
			}
		}

		has_video = theora_header_packets == 3;
		has_audio = vorbis_header_packets == 3;
	}

	double get_time() const
	{
		return time - delay_compensation;
	}

	_FORCE_INLINE_ bool send_audio(AudioMixCallback p_callback, void* p_udata)
	{
		if (audio_ptr_end > 0) {
			int mixed = p_callback
				? p_callback(
					  p_udata, &audio_buffer[audio_ptr_start * vi.channels], audio_ptr_end - audio_ptr_start)
				: (audio_ptr_end - audio_ptr_start);
			audio_ptr_start += mixed;
			if (audio_ptr_start == audio_ptr_end) {
				audio_ptr_start = 0;
				audio_ptr_end = 0;
			}
			else {
				return false;
			}
		}
		return true;
	}
};

void VideoStreamPlayback::set_file(const String& p_file)
{
	ERR_FAIL_COND(data->playing);
	th_setup_info* ts = nullptr;

	data->clear();

	data->file = FileAccess::open(p_file, FileAccess::READ);
	ERR_FAIL_COND_MSG(data->file.is_null(), "Cannot open file '" + p_file + "'.");

	data->file_name = p_file;

	ogg_sync_init(&data->oy);

	vorbis_info_init(&data->vi);
	vorbis_comment_init(&data->vc);

	th_comment_init(&data->tc);
	th_info_init(&data->ti);

	memset(&data->to, 0, sizeof(data->to));
	memset(&data->vo, 0, sizeof(data->vo));

	data->find_streams(ts);
	data->read_headers(ts);

	if (!data->has_audio) {
		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}
	}

	if (!data->has_video) {
		th_setup_free(ts);
		th_comment_clear(&data->tc);
		th_info_clear(&data->ti);
		if (!ogg_stream_check(&data->to)) {
			ogg_stream_clear(&data->to);
		}
		data->file.unref();

		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}

		data->has_video = false;
		data->has_audio = false;
		ERR_FAIL_MSG("File '" + p_file + "' has no video stream.");
	}

	data->td = th_decode_alloc(&data->ti, ts);
	th_setup_free(ts);
	data->px_fmt = data->ti.pixel_fmt;
	switch (data->ti.pixel_fmt) {
	case TH_PF_420:
	case TH_PF_422:
	case TH_PF_444:
		break;
	default:
		WARN_PRINT(" video\n  (UNKNOWN Chroma sampling!)\n");
		break;
	}
	th_decode_ctl(
		data->td, TH_DECCTL_GET_PPLEVEL_MAX, &data->pp_level_max, sizeof(data->pp_level_max));
	data->pp_level = 0;
	th_decode_ctl(data->td, TH_DECCTL_SET_PPLEVEL, &data->pp_level, sizeof(data->pp_level));
	data->pp_inc = 0;

	data->size.x = data->ti.frame_width;
	data->size.y = data->ti.frame_height;
	data->region.position.x = data->ti.pic_x;
	data->region.position.y = data->ti.pic_y;
	data->region.size.x = data->ti.pic_width;
	data->region.size.y = data->ti.pic_height;

	Ref<Image> img = Image::create_empty(
		data->region.size.x, data->region.size.y, false, Image::FORMAT_RGBA8);
	data->texture->set_image(img);
	data->frame_data.resize(data->region.size.x * data->region.size.y * 4);

	data->frame_duration = (double)data->ti.fps_denominator / data->ti.fps_numerator;

	if (data->has_audio) {
		vorbis_synthesis_init(&data->vd, &data->vi);
		vorbis_block_init(&data->vd, &data->vb);
		data->audio_buffer_size = MIN(data->vi.channels, 8) * 1024;
		data->audio_buffer = memnew_arr(float, data->audio_buffer_size);
	}

	data->stream_data_offset = data->file->get_position() - data->oy.fill + data->oy.returned;
	data->stream_data_size = data->file->get_length() - data->stream_data_offset;

	int64_t seek_pos = MAX(data->stream_data_offset, (int64_t)data->file->get_length() - 64 * 1024);
	int64_t video_granulepos = INT64_MAX;
	int64_t audio_granulepos = INT64_MAX;
	data->file->seek(seek_pos);
	seek_pos = data->seek_streams(-1, video_granulepos, audio_granulepos);
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->stream_length = 0;
	ogg_page page;
	while (data->read_page(&page) > 0) {
		data->stream_length = MAX(data->stream_length, data->get_page_time(&page));
	}

	seek(0);
}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return data->texture; }

void VideoStreamPlayback::update(double p_delta)
{
	if (data->file.is_null()) {
		return;
	}

	if (!data->playing || data->paused) {
		return;
	}

	data->time += p_delta;

	double comp_time = data->get_time();
	bool audio_ready = false;

	while ((!audio_ready && !data->audio_done) || (!data->video_ready && !data->video_done)) {
		ogg_packet op;

		while (!audio_ready && !data->audio_done) {
			if (!data->send_audio(mix_callback, mix_udata)) {
				audio_ready = true;
				break;
			}

			float** pcm;
			int ret = vorbis_synthesis_pcmout(&data->vd, &pcm);
			if (ret > 0) {
				int frames_read = 0;
				while (frames_read < ret) {
					int m = MIN(data->audio_buffer_size / data->vi.channels, ret - frames_read);
					int count = 0;
					for (int j = 0; j < m; j++) {
						for (int i = 0; i < data->vi.channels; i++) {
							data->audio_buffer[count++] = pcm[i][frames_read + j];
						}
					}
					frames_read += m;
					data->audio_ptr_end = m;
					if (!data->send_audio(mix_callback, mix_udata)) {
						audio_ready = true;
						break;
					}
				}
				vorbis_synthesis_read(&data->vd, frames_read);
			}
			else {
				if (ogg_stream_packetout(&data->vo, &op) > 0) {
					if (vorbis_synthesis(&data->vb, &op) == 0) {
						vorbis_synthesis_blockin(&data->vd, &data->vb);
					}
				}
				else {
					data->audio_done = data->vorbis_eos;
					break;
				}
			}
		}

		while (!data->video_ready && !data->video_done) {
			if (ogg_stream_packetout(&data->to, &op) > 0) {
				if (op.granulepos >= 0) {
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
				}
				int64_t videobuf_granulepos;
				int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
				if (ret == 0 || ret == TH_DUPFRAME) {
					data->next_frame_time = th_granule_time(data->td, videobuf_granulepos);
					if (data->next_frame_time > comp_time) {
						data->dup_frame = (ret == TH_DUPFRAME);
						data->video_ready = true;
					}
					else {
						data->pp_inc = data->pp_level > 0 ? -1 : 0;
					}
				}
			}
			else {
				data->video_done = data->theora_eos;
				break;
			}
		}

		if (!data->video_ready || !audio_ready) {
			int ret = data->feed_pages();
			if (ret == 0) {
				data->vorbis_eos = true;
				data->theora_eos = true;
				break;
			}
		}

		double tdiff = data->next_frame_time - comp_time;
		if (tdiff > data->ti.fps_denominator * 0.25 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level < data->pp_level_max ? 1 : 0;
		}
		else if (tdiff < data->ti.fps_denominator * 0.05 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level > 0 ? -1 : 0;
		}
	}

	if (!data->video_ready && data->video_done && data->audio_done) {
		stop();
		return;
	}

	if (data->video_ready && comp_time >= data->current_frame_time) {
		if (!data->dup_frame) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
		}
		data->dup_frame = false;
		data->video_ready = false;
		data->current_frame_time = data->next_frame_time;
	}
}

void VideoStreamPlayback::stop()
{
	data->playing = false;
	seek(0);
}

void VideoStreamPlayback::play()
{
	if (!data->has_video) {
		return;
	}
	if (!data->playing) {
		data->time = 0;
	}
	data->playing = true;
}

bool VideoStreamPlayback::is_playing() const { return data->playing; }

void VideoStreamPlayback::set_paused(bool p_paused) { data->paused = p_paused; }

bool VideoStreamPlayback::is_paused() const { return data->paused; }

double VideoStreamPlayback::get_length() const { return data->stream_length; }

double VideoStreamPlayback::get_playback_position() const { return data->get_time(); }

void VideoStreamPlayback::seek(double p_time)
{
	if (data->file.is_null()) {
		return;
	}
	if (p_time >= data->stream_length) {
		return;
	}

	data->video_ready = false;
	data->next_frame_time = 0;
	data->current_frame_time = -1;
	data->dup_frame = false;
	data->video_done = false;
	data->audio_done = !data->has_audio;
	data->theora_eos = false;
	data->vorbis_eos = false;
	data->audio_ptr_start = 0;
	data->audio_ptr_end = 0;

	ogg_stream_reset(&data->to);
	if (data->has_audio) {
		ogg_stream_reset(&data->vo);
		vorbis_synthesis_restart(&data->vd);
	}

	int64_t seek_pos;
	int64_t video_granulepos;
	int64_t audio_granulepos;
	seek_pos = data->seek_streams(p_time, video_granulepos, audio_granulepos);
	if (seek_pos < 0) {
		return;
	}
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->time = p_time;

	double last_audio_time = 0;
	double last_video_time = 0;
	bool first_frame_decoded = false;
	bool start_audio = (audio_granulepos == 0);
	bool start_video = (video_granulepos == (1LL << data->ti.keyframe_granule_shift));
	bool keyframe_found = false;
	uint64_t current_frame = 0;

	while ((data->has_audio && last_audio_time < p_time) || (last_video_time <= p_time)) {
		ogg_packet op;
		if (data->feed_pages() == 0) {
			break;
		}
		while (data->has_audio && last_audio_time < p_time && ogg_stream_packetout(&data->vo, &op) > 0) {
			if (start_audio) {
				if (vorbis_synthesis(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
					float** pcm;
					int samples_left = ceil((p_time - last_audio_time) * data->vi.rate);
					int samples_read = vorbis_synthesis_pcmout(&data->vd, &pcm);
					int samples_consumed = MIN(samples_left, samples_read);
					vorbis_synthesis_read(&data->vd, samples_consumed);
					last_audio_time += (double)samples_consumed / data->vi.rate;
				}
			}
			else if (op.granulepos >= audio_granulepos) {
				last_audio_time = vorbis_granule_time(&data->vd, op.granulepos);
				if (vorbis_synthesis_trackonly(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
				}
				start_audio = true;
			}
		}
		while (last_video_time <= p_time && ogg_stream_packetout(&data->to, &op) > 0) {
			if (!start_video &&
				(op.granulepos >= video_granulepos ||
					video_granulepos == (1LL << data->ti.keyframe_granule_shift))) {
				if (op.granulepos > 0) {
					current_frame = th_granule_frame(data->td, op.granulepos);
				}
				start_video = true;
			}
			if (start_video) {
				if (!keyframe_found && th_packet_iskeyframe(&op)) {
					keyframe_found = true;
					int64_t cur_granulepos = (current_frame + 1) << data->ti.keyframe_granule_shift;
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &cur_granulepos, sizeof(cur_granulepos));
				}
				if (keyframe_found) {
					int64_t videobuf_granulepos;
					if (op.granulepos >= 0) {
						th_decode_ctl(
							data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
					}
					int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
					if (ret == 0 || ret == TH_DUPFRAME) {
						last_video_time = th_granule_time(data->td, videobuf_granulepos);
						first_frame_decoded = true;
					}
				}
				else {
					current_frame++;
				}
			}
		}
	}

	if (first_frame_decoded) {
		if (is_playing()) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
			data->current_frame_time = last_video_time;
		}
		else {
			data->next_frame_time = data->current_frame_time;
			data->video_ready = true;
		}
	}
}

int VideoStreamPlayback::get_channels() const
{
	_channel_count = data->vi.channels;
	return data->vi.channels;
}

void VideoStreamPlayback::set_audio_track(int p_idx) { data->audio_track = p_idx; }

int VideoStreamPlayback::get_mix_rate() const { return data->vi.rate; }

VideoStreamPlayback::VideoStreamPlayback()
{
	data = memnew(TheoraData);
	data->texture.instantiate();
}

VideoStreamPlayback::~VideoStreamPlayback()
{
	data->clear();
	memdelete(data);
	data = nullptr;
}

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	Ref<VideoStreamPlayback> pb = memnew(VideoStreamPlayback);
	pb->set_audio_track(audio_track);
	pb->set_file(file);
	return pb;
}

Ref<Resource> ResourceFormatLoaderTheora::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	Ref<VideoStream> stream;
	stream.instantiate();
	stream->set_file(p_path);

	if (r_error) {
		*r_error = OK;
	}

	return stream;
}

void ResourceFormatLoaderTheora::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogv");
}

bool ResourceFormatLoaderTheora::handles_type(const String& p_type) const
{
	return p_type == "VideoStream";
}

String ResourceFormatLoaderTheora::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ogv")) {
		return "VideoStream";
	}
	return "";
}

#ifdef TOOLS_ENABLED
#include <cstdlib>

// For reference, see:
// - https://stackoverflow.com/a/9467305
// - https://en.wikipedia.org/wiki/YCbCr#Approximate_8-bit_matrices_for_BT.601

static void _rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height, size_t pixel_size) {
	size_t uvpos = 0;
	size_t i = 0;
	for (size_t line = 0; line < height; line += 2) {
		for (size_t x = 0; x < width; x += 2) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

			u[uvpos] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
			v[uvpos] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;
			uvpos++;

			r = rgb[pixel_size * i];
			g = rgb[pixel_size * i + 1];
			b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
		for (size_t x = 0; x < width; x += 1) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
	}
}

static void rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgb, width, height, 3);
}

static void rgba2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgba, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgba, width, height, 4);
}

#include <cstdlib>

void MovieWriterOGV::push_audio(const int32_t* p_audio_data)
{
	// Read and process more audio.
	float** vorbis_buffer = vorbis_analysis_buffer(&vd, audio_frames);

	// Deinterleave samples.
	uint32_t count = 0;
	for (uint32_t i = 0; i < audio_frames; i++) {
		for (uint32_t j = 0; j < audio_ch; j++) {
			vorbis_buffer[j][i] = p_audio_data[count] / 2147483647.f;
			count++;
		}
	}

	vorbis_analysis_wrote(&vd, audio_frames);
}

void MovieWriterOGV::pull_audio(bool p_last)
{
	ogg_packet op;

	while (vorbis_analysis_blockout(&vd, &vb) > 0) {
		// Analysis, assume we want to use bitrate management.
		vorbis_analysis(&vb, nullptr);
		vorbis_bitrate_addblock(&vb);

		// Weld packets into the bitstream.
		while (vorbis_bitrate_flushpacket(&vd, &op) > 0) {
			ogg_stream_packetin(&vo, &op);
		}
	}

	if (p_last) {
		vorbis_analysis_wrote(&vd, 0);
		pull_audio();
	}
}

void MovieWriterOGV::push_video(const Ref<Image>& p_image)
{
	PackedByteArray data = p_image->get_data();
	if (p_image->get_format() == Image::FORMAT_RGBA8) {
		rgba2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	else {
		rgb2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	th_encode_ycbcr_in(td, ycbcr);
}

void MovieWriterOGV::pull_video(bool p_last)
{
	ogg_packet op;

	int ret = 0;
	do {
		ret = th_encode_packetout(td, p_last, &op);
		if (ret > 0) {
			ogg_stream_packetin(&to, &op);
		}
	} while (ret > 0);
}

uint32_t MovieWriterOGV::get_audio_mix_rate() const { return mix_rate; }

AudioServer::SpeakerMode MovieWriterOGV::get_audio_speaker_mode() const { return speaker_mode; }

bool MovieWriterOGV::handles_file(const String& p_path) const
{
	return p_path.has_extension("ogv");
}

void MovieWriterOGV::get_supported_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("ogv");
}

Error MovieWriterOGV::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	ERR_FAIL_COND_V_MSG((p_movie_size.width & 1) || (p_movie_size.height & 1), ERR_UNAVAILABLE,
		"Both video dimensions must be even.");
	base_path = p_base_path.get_basename();
	if (base_path.is_relative_path()) {
		base_path = "res://" + base_path;
	}
	base_path += ".ogv";

	f = FileAccess::open(base_path, FileAccess::WRITE_READ);
	ERR_FAIL_COND_V(f.is_null(), ERR_CANT_OPEN);

	fps = p_fps;

	audio_ch = 2;
	switch (speaker_mode) {
	case AudioServer::SPEAKER_MODE_STEREO:
		audio_ch = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		audio_ch = 4;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		audio_ch = 6;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		audio_ch = 8;
		break;
	}
	audio_frames = mix_rate / fps;

	// Set up Ogg output streams.
	srand(time(nullptr));
	ogg_stream_init(&to, rand()); // Video.
	ogg_stream_init(&vo, rand()); // Audio.

	// Initialize Vorbis audio encoding.
	vorbis_info_init(&vi);
	int ret = vorbis_encode_init_vbr(&vi, audio_ch, mix_rate, audio_quality);
	ERR_FAIL_COND_V_MSG(ret, ERR_UNAVAILABLE,
		"The Ogg Vorbis encoder couldn't set up a mode according to the requested quality or "
		"bitrate.");

	vorbis_comment_init(&vc);
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	// Set up Theora encoder.
	// Theora has a divisible-by-16 restriction for the encoded frame size
	// scale the picture size up to the nearest /16 and calculate offsets.
	int pic_w = p_movie_size.width;
	int pic_h = p_movie_size.height;
	int frame_w = (pic_w + 15) & ~0xF;
	int frame_h = (pic_h + 15) & ~0xF;
	// Force the offsets to be even so that chroma samples line up like we expect.
	int pic_x = (frame_w - pic_w) / 2 & ~1;
	int pic_y = (frame_h - pic_h) / 2 & ~1;

	y = (uint8_t*)memalloc(pic_w * pic_h);
	u = (uint8_t*)memalloc(pic_w * pic_h / 4);
	v = (uint8_t*)memalloc(pic_w * pic_h / 4);

	// We submit the buffer using the size of the picture region.
	// libtheora will pad the picture region out to the full frame size for us,
	// whether we pass in a full frame or not.
	ycbcr[0].width = pic_w;
	ycbcr[0].height = pic_h;
	ycbcr[0].stride = pic_w;
	ycbcr[0].data = y;
	ycbcr[1].width = pic_w / 2;
	ycbcr[1].height = pic_h / 2;
	ycbcr[1].stride = pic_w / 2;
	ycbcr[1].data = u;
	ycbcr[2].width = pic_w / 2;
	ycbcr[2].height = pic_h / 2;
	ycbcr[2].stride = pic_w / 2;
	ycbcr[2].data = v;

	th_info_init(&ti);
	ti.frame_width = frame_w;
	ti.frame_height = frame_h;
	ti.pic_width = pic_w;
	ti.pic_height = pic_h;
	ti.pic_x = pic_x;
	ti.pic_y = pic_y;
	ti.fps_numerator = fps;
	ti.fps_denominator = 1;
	ti.aspect_numerator = 1;
	ti.aspect_denominator = 1;
	ti.colorspace = TH_CS_UNSPECIFIED;
	// Account for the Ogg page overhead.
	// This is 1 byte per 255 for lacing values, plus 26 bytes per 4096 bytes for
	// the page header, plus approximately 1/2 byte per packet (not accounted for here).
	ti.target_bitrate = (int)(64870 * (ogg_int64_t)video_bitrate >> 16);
	ti.quality = video_quality * 63;
	ti.pixel_fmt = TH_PF_420;
	td = th_encode_alloc(&ti);
	th_info_clear(&ti);
	ERR_FAIL_NULL_V_MSG(td, ERR_UNCONFIGURED,
		"Couldn't create a Theora encoder instance. Check that the video parameters are valid.");

	// Setting just the granule shift only allows power-of-two keyframe spacing.
	// Set the actual requested spacing.
	ret = th_encode_ctl(td, TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &keyframe_frequency,
		sizeof(keyframe_frequency));
	if (ret < 0) {
		ERR_PRINT("Couldn't set keyframe interval.");
	}

	// Speed should also be set after the current encoder mode is established,
	// since the available speed levels may change depending on the encoder mode.
	if (speed >= 0) {
		int speed_max;
		ret = th_encode_ctl(td, TH_ENCCTL_GET_SPLEVEL_MAX, &speed_max, sizeof(speed_max));
		if (ret < 0) {
			WARN_PRINT("Couldn't determine maximum speed level.");
			speed_max = 0;
		}
		ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed, sizeof(speed));
		if (ret < 0) {
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed, speed_max));
			}
			if (speed > speed_max) {
				WARN_PRINT(vformat("Setting speed level to %d instead.", speed_max));
			}
			ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed_max, sizeof(speed_max));
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed_max, speed_max));
			}
		}
	}

	// Write the bitstream header packets with proper page interleave.
	th_comment_init(&tc);
	// The first packet will get its own page automatically.
	ogg_packet op;
	if (th_encode_flushheader(td, &tc, &op) <= 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
	}

	ogg_stream_packetin(&to, &op);
	if (ogg_stream_pageout(&to, &video_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(video_page.header, video_page.header_len);
	f->store_buffer(video_page.body, video_page.body_len);

	// Create the remaining Theora headers.
	while (true) {
		ret = th_encode_flushheader(td, &tc, &op);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
		}
		else if (ret == 0) {
			break;
		}
		ogg_stream_packetin(&to, &op);
	}

	// Vorbis streams start with 3 standard header packets.
	ogg_packet id;
	ogg_packet comment;
	ogg_packet code;
	if (vorbis_analysis_headerout(&vd, &vc, &id, &comment, &code) < 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Vorbis library error.");
	}

	// ID header is automatically placed in its own page.
	ogg_stream_packetin(&vo, &id);
	if (ogg_stream_pageout(&vo, &audio_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(audio_page.header, audio_page.header_len);
	f->store_buffer(audio_page.body, audio_page.body_len);

	// Append remaining Vorbis header packets.
	ogg_stream_packetin(&vo, &comment);
	ogg_stream_packetin(&vo, &code);

	// Flush the rest of our headers. This ensures the actual data in each stream will start on a
	// new page, as per spec.
	while (true) {
		ret = ogg_stream_flush(&to, &video_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(video_page.header, video_page.header_len);
		f->store_buffer(video_page.body, video_page.body_len);
	}

	while (true) {
		ret = ogg_stream_flush(&vo, &audio_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(audio_page.header, audio_page.header_len);
		f->store_buffer(audio_page.body, audio_page.body_len);
	}

	return OK;
}

// The order of the operations has been chosen so we're one frame behind writing to the stream so we
// can put the eos mark in the last frame. Flushing streams to the file every X frames is done to
// improve audio/video page interleaving thus avoiding large runs of video or audio pages.
Error MovieWriterOGV::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	ERR_FAIL_COND_V(f.is_null() || td == nullptr, ERR_UNCONFIGURED);

	frame_count++;

	pull_audio();
	pull_video();

	if ((frame_count % 8) == 0) {
		write_to_file();
	}

	push_audio(p_audio_data);
	push_video(p_image);

	return OK;
}

void MovieWriterOGV::save_page(ogg_page page)
{
	unsigned int page_size = page.header_len + page.body_len;
	if (page_size > backup_page_size) {
		backup_page_data = (unsigned char*)memrealloc(backup_page_data, page_size);
		backup_page_size = page_size;
	}
	backup_page.header = backup_page_data;
	backup_page.header_len = page.header_len;
	backup_page.body = backup_page_data + page.header_len;
	backup_page.body_len = page.body_len;
	memcpy(backup_page.header, page.header, page.header_len);
	memcpy(backup_page.body, page.body, page.body_len);
}

void MovieWriterOGV::restore_page(ogg_page* page)
{
	page->header = backup_page.header;
	page->header_len = backup_page.header_len;
	page->body = backup_page.body;
	page->body_len = backup_page.body_len;
}

// The added complexity here is because we have to ensure pages are written in ascending timestamp
// order. libOgg doesn't allow checking the next page granulepos without requesting the page, and
// once requested it can't be returned, thus, we need to save it so that it doesn't get erased by
// the next `ogg_stream_packetin` call.
void MovieWriterOGV::write_to_file(bool p_finish)
{
	if (audio_flag) {
		restore_page(&audio_page);
	}
	else {
		audio_flag = ogg_stream_flush(&vo, &audio_page);
	}
	if (video_flag) {
		restore_page(&video_page);
	}
	else {
		video_flag = ogg_stream_flush(&to, &video_page);
	}

	bool finishing = p_finish && (audio_flag || video_flag);
	while (finishing || (audio_flag && video_flag)) {
		double audiotime = vorbis_granule_time(&vd, ogg_page_granulepos(&audio_page));
		double videotime = th_granule_time(td, ogg_page_granulepos(&video_page));
		bool video_first = audiotime >= videotime;

		if (video_flag && video_first) {
			// Flush a video page.
			f->store_buffer(video_page.header, video_page.header_len);
			f->store_buffer(video_page.body, video_page.body_len);
			video_flag = ogg_stream_flush(&to, &video_page) > 0;
		}
		else {
			// Flush an audio page.
			f->store_buffer(audio_page.header, audio_page.header_len);
			f->store_buffer(audio_page.body, audio_page.body_len);
			audio_flag = ogg_stream_flush(&vo, &audio_page) > 0;
		}
		finishing = p_finish && (audio_flag || video_flag);
	}

	if (video_flag) {
		save_page(video_page);
	}
	else if (audio_flag) {
		save_page(audio_page);
	}
}

void MovieWriterOGV::write_end()
{
	pull_audio(true);
	pull_video(true);
	write_to_file(true);

	th_encode_free(td);

	ogg_stream_clear(&vo);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	ogg_stream_clear(&to);
	th_comment_clear(&tc);

	memfree(y);
	memfree(u);
	memfree(v);

	if (backup_page_data != nullptr) {
		memfree(backup_page_data);
	}

	if (f.is_valid()) {
		f.unref();
	}
}



MovieWriterOGV::MovieWriterOGV() {}
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
#endif

static Ref<ResourceFormatLoaderTheora> resource_loader_theora;
#ifdef TOOLS_ENABLED
static MovieWriterOGV* writer_ogv = nullptr;
#endif

void initialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		writer_ogv = memnew(MovieWriterOGV);
		MovieWriter::add_writer(writer_ogv);
#endif
	} break;

	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		resource_loader_theora.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_theora, true);
	} break;
	default:
		break;
	}
}

void uninitialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		ResourceLoader::remove_resource_format_loader(resource_loader_theora);
		resource_loader_theora.unref();
	} break;

	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		memdelete(writer_ogv);
#endif
	} break;
	default:
		break;
	}
}
#endif // defined(MODULE_THEORA_ENABLED)

// === Merged from modules/mobile_vr/mobile_vr_interface.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_MOBILE_VR_ENABLED)
#include "core/input/input.h"
#include "modules/register_module_types.h"
#include "core/os/os.h"
// --- Inlined from modules/mobile_vr/mobile_vr_interface.h ---
#include "servers/xr/xr_server.h"

/**
	The mobile interface is a native VR interface that can be used on Android and iOS phones.
	It contains a basic implementation supporting 3DOF tracking if a gyroscope and accelerometer are
	present and sets up the proper projection matrices based on the values provided.

	We're planning to eventually do separate interfaces towards mobile SDKs that have far more
   capabilities and do not rely on the user providing most of these settings (though enhancing this
   with auto detection features based on the device we're running on would be cool). I'm mostly
   adding this as an example or base plate for more advanced interfaces.
*/

class MobileVRInterface : public XRInterface
{
private:
	bool initialized = false;
	XRInterface::TrackingStatus tracking_state;
	XRPose::TrackingConfidence tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;

	// Just set some defaults for these. At some point we need to look at adding a lookup table for
	// common device + headset combos and/or support reading cardboard QR codes
	double eye_height = 1.85;
	uint64_t last_ticks = 0;

	double intraocular_dist = 6.0;
	double display_width = 14.5;
	double display_to_lens = 4.0;
	double oversample = 1.5;

	Rect2 offset_rect = Rect2(0, 0, 1, 1); // Full screen rect.

	double k1 = 0.215;
	double k2 = 0.215;
	double aspect = 1.0;

	// at a minimum we need a tracker for our head
	Ref<XRTracker> head;
	Transform3D head_transform;

	XRVRS xr_vrs;

	/*
		logic for processing our sensor data, this was originally in our positional tracker logic
	   but I think that doesn't make sense in hindsight. It only makes marginally more sense to park
	   it here for now, this probably deserves an object of its own
	*/
	Vector3 scale_magneto(const Vector3& p_magnetometer);
	Basis combine_acc_mag(const Vector3& p_grav, const Vector3& p_magneto);

	int mag_count = 0;
	bool has_gyro = false;
	bool sensor_first = false;
	Vector3 last_accerometer_data;
	Vector3 last_magnetometer_data;
	Vector3 mag_current_min;
	Vector3 mag_current_max;
	Vector3 mag_next_min;
	Vector3 mag_next_max;

	///@TODO a few support functions for trackers, most are math related and should likely be moved
	///elsewhere
	float floor_decimals(const float p_value, const float p_decimals)
	{
		float power_of_10 = std::pow(10.0f, p_decimals);
		return std::floor(p_value * power_of_10) / power_of_10;
	}

	Vector3 floor_decimals(const Vector3& p_vector, const float p_decimals)
	{
		return Vector3(floor_decimals(p_vector.x, p_decimals),
			floor_decimals(p_vector.y, p_decimals), floor_decimals(p_vector.z, p_decimals));
	}

	Vector3 low_pass(const Vector3& p_vector, const Vector3& p_last_vector, const float p_factor)
	{
		return p_vector + (p_factor * (p_last_vector - p_vector));
	}

	Vector3 scrub(const Vector3& p_vector, const Vector3& p_last_vector, const float p_decimals,
		const float p_factor)
	{
		return low_pass(floor_decimals(p_vector, p_decimals), p_last_vector, p_factor);
	}

	void set_position_from_sensors();

protected:

public:
	void set_eye_height(const double p_eye_height);
	double get_eye_height() const;

	void set_iod(const double p_iod);
	double get_iod() const;

	void set_display_width(const double p_display_width);
	double get_display_width() const;

	void set_offset_rect(const Rect2& p_offset_rect);
	Rect2 get_offset_rect() const;

	void set_display_to_lens(const double p_display_to_lens);
	double get_display_to_lens() const;

	void set_oversample(const double p_oversample);
	double get_oversample() const;

	void set_k1(const double p_k1);
	double get_k1() const;

	void set_k2(const double p_k2);
	double get_k2() const;

	float get_vrs_min_radius() const;
	void set_vrs_min_radius(float p_vrs_min_radius);

	float get_vrs_strength() const;
	void set_vrs_strength(float p_vrs_strength);

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual TrackingStatus get_tracking_status() const override;

	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual XRInterface::PlayAreaMode get_play_area_mode() const override;
	virtual bool set_play_area_mode(XRInterface::PlayAreaMode p_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(
		uint32_t p_view, const Transform3D& p_cam_transform) override;
	virtual Projection get_projection_for_view(
		uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(
		RID p_render_target, const Rect2& p_screen_rect) override;

	virtual void process() override;

	virtual RID get_vrs_texture() override;

	MobileVRInterface();
	~MobileVRInterface();
};



// --- End modules/mobile_vr/mobile_vr_interface.h ---
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server_types.h"

String MobileVRInterface::get_name() const { return "Native mobile"; }

uint32_t MobileVRInterface::get_capabilities() const { return XRInterface::XR_STEREO; }

Vector3 MobileVRInterface::scale_magneto(const Vector3& p_magnetometer)
{
	// Our magnetometer doesn't give us nice clean data.
	// Well it may on macOS because we're getting a calibrated value in the current implementation
	// but Android we're getting raw data. This is a fairly simple adjustment we can do to correct
	// for the magnetometer data being elliptical

	Vector3 mag_raw = p_magnetometer;
	Vector3 mag_scaled = p_magnetometer;

	// update our variables every x frames
	if (mag_count > 20) {
		mag_current_min = mag_next_min;
		mag_current_max = mag_next_max;
		mag_count = 0;
	}
	else {
		mag_count++;
	};

	// adjust our min and max
	if (mag_raw.x > mag_next_max.x) {
		mag_next_max.x = mag_raw.x;
	}
	if (mag_raw.y > mag_next_max.y) {
		mag_next_max.y = mag_raw.y;
	}
	if (mag_raw.z > mag_next_max.z) {
		mag_next_max.z = mag_raw.z;
	}

	if (mag_raw.x < mag_next_min.x) {
		mag_next_min.x = mag_raw.x;
	}
	if (mag_raw.y < mag_next_min.y) {
		mag_next_min.y = mag_raw.y;
	}
	if (mag_raw.z < mag_next_min.z) {
		mag_next_min.z = mag_raw.z;
	}

	// scale our x, y and z
	if (!(mag_current_max.x - mag_current_min.x)) {
		mag_raw.x -= (mag_current_min.x + mag_current_max.x) / 2.0;
		mag_scaled.x =
			(mag_raw.x - mag_current_min.x) / ((mag_current_max.x - mag_current_min.x) * 2.0 - 1.0);
	};

	if (!(mag_current_max.y - mag_current_min.y)) {
		mag_raw.y -= (mag_current_min.y + mag_current_max.y) / 2.0;
		mag_scaled.y =
			(mag_raw.y - mag_current_min.y) / ((mag_current_max.y - mag_current_min.y) * 2.0 - 1.0);
	};

	if (!(mag_current_max.z - mag_current_min.z)) {
		mag_raw.z -= (mag_current_min.z + mag_current_max.z) / 2.0;
		mag_scaled.z =
			(mag_raw.z - mag_current_min.z) / ((mag_current_max.z - mag_current_min.z) * 2.0 - 1.0);
	};

	return mag_scaled;
}

Basis MobileVRInterface::combine_acc_mag(const Vector3& p_grav, const Vector3& p_magneto)
{
	// yup, stock standard cross product solution...
	Vector3 up = -p_grav.normalized();

	Vector3 magneto_east =
		up.cross(p_magneto.normalized()); // or is this west?, but should be horizon aligned now
	magneto_east.normalize();

	Vector3 magneto = up.cross(magneto_east); // and now we have a horizon aligned north
	magneto.normalize();

	// We use our gravity and magnetometer vectors to construct our matrix
	Basis acc_mag_m3;
	acc_mag_m3.rows[0] = -magneto_east;
	acc_mag_m3.rows[1] = up;
	acc_mag_m3.rows[2] = magneto;

	return acc_mag_m3;
}

void MobileVRInterface::set_position_from_sensors()
{
	_THREAD_SAFE_METHOD_

	// this is a helper function that attempts to adjust our transform using our 9dof sensors
	// 9dof is a misleading marketing term coming from 3 accelerometer axis + 3 gyro axis + 3
	// magnetometer axis = 9 axis but in reality this only offers 3 dof (yaw, pitch, roll)
	// orientation

	Basis orientation = head_transform.basis;

	uint64_t ticks = OS::get_ticks_usec();
	uint64_t ticks_elapsed = ticks - last_ticks;
	float delta_time = (double)ticks_elapsed / 1000000.0;

	// few things we need
	Vector3 down(0.0, -1.0, 0.0); // Down is Y negative

	// make copies of our inputs
	bool has_grav = false;
	Vector3 acc = Input::get_accelerometer();
	Vector3 gyro = Input::get_gyroscope();
	Vector3 grav = Input::get_gravity();
	Vector3 magneto = scale_magneto(
		Input::get_magnetometer()); // this may be overkill on iOS because we're already getting a
									// calibrated magnetometer reading

	if (sensor_first) {
		sensor_first = false;
	}
	else {
		acc = scrub(acc, last_accerometer_data, 2, 0.2f);
		magneto = scrub(magneto, last_magnetometer_data, 3, 0.3f);
	};

	last_accerometer_data = acc;
	last_magnetometer_data = magneto;

	if (grav.length() < 0.1f) {
		// not ideal but use our accelerometer, this will contain shaky user behavior
		// maybe look into some math but I'm guessing that if this isn't available, it's because we
		// lack the gyro sensor to actually work out what a stable gravity vector is
		grav = acc;
		if (grav.length() > 0.1f) {
			has_grav = true;
		};
	}
	else {
		has_grav = true;
	};

	bool has_magneto = magneto.length() > 0.1f;
	if (gyro.length() > 0.1f) {
		/* this can return to 0.0 if the user doesn't move the phone, so once on, it's on */
		has_gyro = true;
	};

	if (has_gyro) {
		// start with applying our gyro (do NOT smooth our gyro!)
		Basis rotate;
		rotate.rotate(orientation.get_column(0), gyro.x * delta_time);
		rotate.rotate(orientation.get_column(1), gyro.y * delta_time);
		rotate.rotate(orientation.get_column(2), gyro.z * delta_time);
		orientation = rotate * orientation;

		tracking_state = XRInterface::XR_NORMAL_TRACKING;
		tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
	};

	///@TODO improve this, the magnetometer is very fidgety sometimes flipping the axis for no
	///apparent reason (probably a bug on my part)
	// if you have a gyro + accelerometer that combo tends to be better than combining all three but
	// without a gyro you need the magnetometer..
	if (has_magneto && has_grav && !has_gyro) {
		// convert to quaternions, easier to smooth those out
		Quaternion transform_quat(orientation);
		Quaternion acc_mag_quat(combine_acc_mag(grav, magneto));
		transform_quat = transform_quat.slerp(acc_mag_quat, 0.1);
		orientation = Basis(transform_quat);

		tracking_state = XRInterface::XR_NORMAL_TRACKING;
		tracking_confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
	}
	else if (has_grav) {
		// use gravity vector to make sure down is down...
		// transform gravity into our world space
		grav.normalize();
		Vector3 grav_adj = orientation.xform(grav);
		float dot = grav_adj.dot(down);
		if ((dot > -1.0) && (dot < 1.0)) {
			// axis around which we have this rotation
			Vector3 axis = grav_adj.cross(down);
			axis.normalize();

			Basis drift_compensation(axis, std::acos(dot) * delta_time * 10);
			orientation = drift_compensation * orientation;
		};
	};

	// and copy to our head transform
	head_transform.basis = orientation.orthonormalized();

	last_ticks = ticks;
}


void MobileVRInterface::set_eye_height(const double p_eye_height) { eye_height = p_eye_height; }

double MobileVRInterface::get_eye_height() const { return eye_height; }

void MobileVRInterface::set_offset_rect(const Rect2& p_offset_rect) { offset_rect = p_offset_rect; }

Rect2 MobileVRInterface::get_offset_rect() const { return offset_rect; }

void MobileVRInterface::set_iod(const double p_iod) { intraocular_dist = p_iod; }

double MobileVRInterface::get_iod() const { return intraocular_dist; }

void MobileVRInterface::set_display_width(const double p_display_width)
{
	display_width = p_display_width;
}

double MobileVRInterface::get_display_width() const { return display_width; }

void MobileVRInterface::set_display_to_lens(const double p_display_to_lens)
{
	display_to_lens = p_display_to_lens;
}

double MobileVRInterface::get_display_to_lens() const { return display_to_lens; }

void MobileVRInterface::set_oversample(const double p_oversample) { oversample = p_oversample; }

double MobileVRInterface::get_oversample() const { return oversample; }

void MobileVRInterface::set_k1(const double p_k1) { k1 = p_k1; }

double MobileVRInterface::get_k1() const { return k1; }

void MobileVRInterface::set_k2(const double p_k2) { k2 = p_k2; }

double MobileVRInterface::get_k2() const { return k2; }

float MobileVRInterface::get_vrs_min_radius() const { return xr_vrs.get_vrs_min_radius(); }

void MobileVRInterface::set_vrs_min_radius(float p_vrs_min_radius)
{
	xr_vrs.set_vrs_min_radius(p_vrs_min_radius);
}

float MobileVRInterface::get_vrs_strength() const { return xr_vrs.get_vrs_strength(); }

void MobileVRInterface::set_vrs_strength(float p_vrs_strength)
{
	xr_vrs.set_vrs_strength(p_vrs_strength);
}

uint32_t MobileVRInterface::get_view_count()
{
	// needs stereo...
	return 2;
}

XRInterface::TrackingStatus MobileVRInterface::get_tracking_status() const
{
	return tracking_state;
}

bool MobileVRInterface::is_initialized() const { return (initialized); }

bool MobileVRInterface::initialize()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	if (!initialized) {
		// reset our sensor data
		mag_count = 0;
		has_gyro = false;
		sensor_first = true;
		mag_next_min = Vector3(10000, 10000, 10000);
		mag_next_max = Vector3(-10000, -10000, -10000);
		mag_current_min = Vector3(0, 0, 0);
		mag_current_max = Vector3(0, 0, 0);
		head_transform.basis = Basis();
		head_transform.origin = Vector3(0.0, eye_height, 0.0);

		// we must create a tracker for our head
		head.instantiate();
		head->set_tracker_type(XRServer::TRACKER_HEAD);
		head->set_tracker_name("head");
		head->set_tracker_desc("Players head");
		XRServer::add_tracker(head);

		// make this our primary interface
		XRServer::set_primary_interface(this);

		last_ticks = OS::get_ticks_usec();

		initialized = true;
	};

	return true;
}

void MobileVRInterface::uninitialize()
{
	if (initialized) {
		// do any cleanup here...
		if (XRServer::is_initialized()) {
			if (head.is_valid()) {
				XRServer::remove_tracker(head);

				head.unref();
			}

			if (XRServer::get_primary_interface() == this) {
				// no longer our primary interface
				XRServer::set_primary_interface(nullptr);
			}
		}

		initialized = false;
	};
}

bool MobileVRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	// This interface has no positional tracking so fix this to 3DOF
	return p_mode == XR_PLAY_AREA_3DOF;
}

XRInterface::PlayAreaMode MobileVRInterface::get_play_area_mode() const
{
	return XR_PLAY_AREA_3DOF;
}

bool MobileVRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_3DOF;
}

Size2 MobileVRInterface::get_render_target_size()
{
	_THREAD_SAFE_METHOD_

	// we use half our window size
	Size2 target_size = DisplayServer::window_get_size();

	target_size.x *= 0.5 * oversample;
	target_size.y *= oversample;

	return target_size;
}

Transform3D MobileVRInterface::get_camera_transform()
{
	_THREAD_SAFE_METHOD_

	Transform3D transform_for_eye;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform_for_eye);

	if (initialized) {
		float world_scale = XRServer::get_world_scale();

		// just scale our origin point of our transform
		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		transform_for_eye = (XRServer::get_reference_frame()) * _head_transform;
	}

	return transform_for_eye;
}

Transform3D MobileVRInterface::get_transform_for_view(
	uint32_t p_view, const Transform3D& p_cam_transform)
{
	_THREAD_SAFE_METHOD_

	Transform3D transform_for_eye;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform_for_eye);

	if (initialized) {
		float world_scale = XRServer::get_world_scale();

		// we don't need to check for the existence of our HMD, doesn't affect our values...
		// note * 0.01 to convert cm to m and * 0.5 as we're moving half in each direction...
		if (p_view == 0) {
			transform_for_eye.origin.x = -(intraocular_dist * 0.01 * 0.5 * world_scale);
		}
		else if (p_view == 1) {
			transform_for_eye.origin.x = intraocular_dist * 0.01 * 0.5 * world_scale;
		}
		else {
			// should not have any other values..
		};

		// just scale our origin point of our transform
		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		transform_for_eye = p_cam_transform * (XRServer::get_reference_frame()) * _head_transform *
							transform_for_eye;
	}
	else {
		// huh? well just return what we got....
		transform_for_eye = p_cam_transform;
	};

	return transform_for_eye;
}

Projection MobileVRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	_THREAD_SAFE_METHOD_

	Projection eye;

	aspect = p_aspect;
	eye.set_for_hmd(p_view + 1, p_aspect, intraocular_dist, display_width, display_to_lens,
		oversample, p_z_near, p_z_far);

	return eye;
}

Vector<RenderingServerTypes::BlitToScreen> MobileVRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	_THREAD_SAFE_METHOD_

	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

	// We must have a valid render target.
	ERR_FAIL_COND_V(!p_render_target.is_valid(), blit_to_screen);

	// We will only output to screen if this is our main viewport.
	if (p_screen_rect == Rect2()) {
		// Warn the developer once, it's up to the developer to output to screen.
		WARN_PRINT_ONCE("SubViewport used with MobileVRInterface, no output to screen");

		return blit_to_screen;
	}

	Rect2 modified_screen_rect =
		Rect2(p_screen_rect.position + offset_rect.position * p_screen_rect.size,
			p_screen_rect.size * offset_rect.size);

	// and add our blits
	RenderingServerTypes::BlitToScreen blit;
	blit.render_target = p_render_target;
	blit.multi_view.use_layer = true;
	blit.lens_distortion.apply = true;
	blit.lens_distortion.k1 = k1;
	blit.lens_distortion.k2 = k2;
	blit.lens_distortion.upscale = oversample;
	blit.lens_distortion.aspect_ratio = aspect;

	// left eye
	blit.dst_rect = modified_screen_rect;
	blit.dst_rect.size.width *= 0.5;
	blit.multi_view.layer = 0;
	blit.lens_distortion.eye_center.x =
		((-intraocular_dist / 2.0) + (display_width / 4.0)) / (display_width / 2.0);
	blit_to_screen.push_back(blit);

	// right eye
	blit.dst_rect = modified_screen_rect;
	blit.dst_rect.size.width *= 0.5;
	blit.dst_rect.position.x += blit.dst_rect.size.width;
	blit.multi_view.layer = 1;
	blit.lens_distortion.eye_center.x =
		((intraocular_dist / 2.0) - (display_width / 4.0)) / (display_width / 2.0);
	blit_to_screen.push_back(blit);

	return blit_to_screen;
}

void MobileVRInterface::process()
{
	_THREAD_SAFE_METHOD_

	if (initialized) {
		// update our head transform orientation
		set_position_from_sensors();

		// update our head transform position (should be constant)
		head_transform.origin = Vector3(0.0, eye_height, 0.0);

		if (head.is_valid()) {
			// Set our head position, note in real space, reference frame and world scale is applied
			// later
			head->set_pose("default", head_transform, Vector3(), Vector3(), tracking_confidence);
		}
	};
}

RID MobileVRInterface::get_vrs_texture()
{
	PackedVector2Array eye_foci;

	Size2 target_size = get_render_target_size();
	real_t aspect_ratio = target_size.x / target_size.y;
	uint32_t view_count = get_view_count();

	for (uint32_t v = 0; v < view_count; v++) {
		Projection cm = get_projection_for_view(v, aspect_ratio, 0.1, 1000.0);
		Vector3 center = cm.xform(Vector3(0.0, 0.0, 999.0));

		eye_foci.push_back(Vector2(center.x, center.y));
	}

	return xr_vrs.make_vrs_texture(target_size, eye_foci);
}

MobileVRInterface::MobileVRInterface() {}

MobileVRInterface::~MobileVRInterface()
{
	// and make sure we cleanup if we haven't already
	if (is_initialized()) {
		uninitialize();
	};
}

static Ref<MobileVRInterface> mobile_vr;

void initialize_mobile_vr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	if (XRServer::is_initialized()) {
		mobile_vr.instantiate();
		XRServer::add_interface(mobile_vr);
	}
}

void uninitialize_mobile_vr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (mobile_vr.is_valid()) {
		// uninitialize our interface if it is initialized
		if (mobile_vr->is_initialized()) {
			mobile_vr->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			XRServer::remove_interface(mobile_vr);
		}

		// and release
		mobile_vr.unref();
	}
}
#endif // defined(MODULE_MOBILE_VR_ENABLED)


// --- Merged from modules/webxr/webxr_interface.cpp ---
#if defined(MODULE_WEBXR_ENABLED)
// --- Inlined from modules/webxr/webxr_interface.h ---
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "servers/xr/xr_server.h"

#ifdef WEB_ENABLED
#include "core/templates/rb_map.h"

namespace GLES3 {
class TextureStorage;
}
#endif // WEB_ENABLED

/**
	The WebXR interface is a VR/AR interface that can be used on the web.
*/

class WebXRInterface final : public XRInterface {
public:
	enum TargetRayMode {
		TARGET_RAY_MODE_UNKNOWN,
		TARGET_RAY_MODE_GAZE,
		TARGET_RAY_MODE_TRACKED_POINTER,
		TARGET_RAY_MODE_SCREEN,
	};

protected:
#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	Ref<XRTracker> _get_input_source_tracker_bind_compat_90645(int p_input_source_id) const;
#endif

public:
	void is_session_supported(const String &p_session_mode);
	void set_session_mode(const String &p_session_mode);
	String get_session_mode() const;
	void set_required_features(const String &p_required_features);
	String get_required_features() const;
	void set_optional_features(const String &p_optional_features);
	String get_optional_features() const;
	void set_requested_reference_space_types(const String &p_requested_reference_space_types);
	String get_requested_reference_space_types() const;
	String get_reference_space_type() const;
	String get_enabled_features() const;
	bool is_input_source_active(int p_input_source_id) const;
	Ref<XRTracker> get_input_source_tracker(int p_input_source_id) const;
	TargetRayMode get_input_source_target_ray_mode(int p_input_source_id) const;
	String get_visibility_state() const;
	float get_display_refresh_rate() const;
	void set_display_refresh_rate(float p_refresh_rate);
	Vector<float> get_available_display_refresh_rates() const;
	Vector<EnvironmentBlendMode> get_supported_environment_blend_modes();
	HashMap<String, String> get_system_info();

	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

#ifdef WEB_ENABLED
private:
	bool initialized;
	Ref<XRTracker> head_tracker;
	Transform3D head_transform;

	String session_mode;
	String required_features;
	String optional_features;
	String requested_reference_space_types;
	String reference_space_type;
	String enabled_features;

	XRInterface::EnvironmentBlendMode environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;

	Size2 render_targetsize;
	RBMap<unsigned int, RID> texture_cache;
	struct Touch {
		bool is_touching = false;
		Vector2 position;
	} touches[5];

	static constexpr uint8_t input_source_count = 16;

	struct InputSource {
		Ref<XRTracker> tracker;
		bool active = false;
		TargetRayMode target_ray_mode;
		int touch_index = -1;
	} input_sources[input_source_count];

	static const int WEBXR_HAND_JOINT_MAX = 25;
	static const int HAND_MAX = 2;
	Ref<XRTracker> hand_trackers[HAND_MAX];

	RID color_texture;
	RID depth_texture;

	RID _get_color_texture();
	RID _get_depth_texture();
	RID _get_texture(unsigned int p_texture_id);
	Transform3D _js_matrix_to_transform(float *p_js_matrix);
	void _update_input_source(int p_input_source_id);

	Vector2 _get_screen_position_from_joy_vector(const Vector2 &p_joy_vector);

public:
	virtual PackedVector3Array get_play_area() const override;
	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) override;

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) override;
	virtual Projection get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;
	virtual bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) override;
	virtual RID get_color_texture() override;
	virtual RID get_depth_texture() override;
	virtual RID get_velocity_texture() override;

	virtual void process() override;

	void _on_input_event(int p_event_type, int p_input_source_id);

	// Internal setters used by callbacks from Emscripten.
	inline void _set_reference_space_type(const String &p_reference_space_type) { reference_space_type = p_reference_space_type; }
	inline void _set_enabled_features(const String &p_enabled_features) { enabled_features = p_enabled_features; }
	void _set_environment_blend_mode(const String &p_blend_mode_string);

	WebXRInterface();
	~WebXRInterface();

private:
	String tracker_names[16] = {
		String("left_hand"),
		String("right_hand"),
		String("tracker_2"),
		String("tracker_3"),
		String("tracker_4"),
		String("tracker_5"),
		String("tracker_6"),
		String("tracker_7"),
		String("tracker_8"),
		String("tracker_9"),
		String("tracker_10"),
		String("tracker_11"),
		String("tracker_12"),
		String("tracker_13"),
		String("tracker_14"),
		String("tracker_15"),
	};

	String touch_names[5] = {
		String("touch_0"),
		String("touch_1"),
		String("touch_2"),
		String("touch_3"),
		String("touch_4"),
	};

	String standard_axis_names[10] = {
		String("touchpad_x"),
		String("touchpad_y"),
		String("thumbstick_x"),
		String("thumbstick_y"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String standard_vector_names[2] = {
		String("touchpad"),
		String("thumbstick"),
	};

	String standard_button_names[10] = {
		String("trigger_click"),
		String("grip_click"),
		String("touchpad_click"),
		String("thumbstick_click"),
		String("ax_button"),
		String("by_button"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String standard_button_pressure_names[10] = {
		String("trigger"),
		String("grip"),
		String("touchpad_click_pressure"),
		String("thumbstick_click_pressure"),
		String("ax_button_pressure"),
		String("by_button_pressure"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};

	String unknown_button_names[10] = {
		String("button_0"),
		String("button_1"),
		String("button_2"),
		String("button_3"),
		String("button_4"),
		String("button_5"),
		String("button_6"),
		String("button_7"),
		String("button_8"),
		String("button_9"),
	};

	String unknown_axis_names[10] = {
		String("axis_0"),
		String("axis_1"),
		String("axis_2"),
		String("axis_3"),
		String("axis_4"),
		String("axis_5"),
		String("axis_6"),
		String("axis_7"),
		String("axis_8"),
		String("axis_9"),
	};

	String unknown_button_pressure_names[10] = {
		String("button_pressure_0"),
		String("button_pressure_1"),
		String("button_pressure_2"),
		String("button_pressure_3"),
		String("button_pressure_4"),
		String("button_pressure_5"),
		String("button_pressure_6"),
		String("button_pressure_7"),
		String("button_pressure_8"),
		String("button_pressure_9"),
	};
#else // !WEB_ENABLED
public:
	WebXRInterface() = default;
	~WebXRInterface() = default;
#endif // WEB_ENABLED
};

// --- End modules/webxr/webxr_interface.h ---
#include "modules/register_module_types.h"

#ifndef DISABLE_DEPRECATED


Ref<XRTracker> WebXRInterface::_get_input_source_tracker_bind_compat_90645(
	int p_input_source_id) const
{
	return get_input_source_tracker(p_input_source_id);
}

void WebXRInterface::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#ifdef WEB_ENABLED
// --- Inlined from modules/webxr/godot_webxr.h ---
#ifdef __cplusplus
extern "C" {
#endif

enum WebXRInputEvent {
	WEBXR_INPUT_EVENT_SELECTSTART,
	WEBXR_INPUT_EVENT_SELECTEND,
	WEBXR_INPUT_EVENT_SQUEEZESTART,
	WEBXR_INPUT_EVENT_SQUEEZEEND,
};

typedef void (*GodotWebXRSupportedCallback)(char *p_session_mode, int p_supported);
typedef void (*GodotWebXRStartedCallback)(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode);
typedef void (*GodotWebXREndedCallback)();
typedef void (*GodotWebXRFailedCallback)(char *p_message);
typedef void (*GodotWebXRInputEventCallback)(int p_event_type, int p_input_source_id);
typedef void (*GodotWebXRSimpleEventCallback)(char *p_signal_name);

extern int godot_webxr_is_supported();
extern void godot_webxr_is_session_supported(const char *p_session_mode, GodotWebXRSupportedCallback p_callback);

extern void godot_webxr_initialize(
		const char *p_session_mode,
		const char *p_required_features,
		const char *p_optional_features,
		const char *p_requested_reference_space_types,
		GodotWebXRStartedCallback p_on_session_started,
		GodotWebXREndedCallback p_on_session_ended,
		GodotWebXRFailedCallback p_on_session_failed,
		GodotWebXRInputEventCallback p_on_input_event,
		GodotWebXRSimpleEventCallback p_on_simple_event);
extern void godot_webxr_uninitialize();

extern int godot_webxr_get_view_count();
extern bool godot_webxr_get_render_target_size(int *r_size);
extern bool godot_webxr_get_transform_for_view(int p_view, float *r_transform);
extern bool godot_webxr_get_projection_for_view(int p_view, float *r_transform);
extern unsigned int godot_webxr_get_color_texture();
extern unsigned int godot_webxr_get_depth_texture();
extern unsigned int godot_webxr_get_velocity_texture();

extern bool godot_webxr_update_input_source(
		int p_input_source_id,
		float *r_target_pose,
		int *r_target_ray_mode,
		int *r_touch_index,
		int *r_has_grip_pose,
		float *r_grip_pose,
		int *r_has_standard_mapping,
		int *r_button_count,
		float *r_buttons,
		int *r_axes_count,
		float *r_axes,
		int *r_has_hand_data,
		float *r_hand_joints,
		float *r_hand_radii);

extern char *godot_webxr_get_visibility_state();
extern int godot_webxr_get_bounds_geometry(float **r_points);

extern float godot_webxr_get_frame_rate();
extern void godot_webxr_update_target_frame_rate(float p_frame_rate);
extern int godot_webxr_get_supported_frame_rates(float **r_frame_rates);

#ifdef __cplusplus
}
#endif

// --- End modules/webxr/godot_webxr.h ---

#include "core/input/input.h"
#include "core/os/os.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "servers/xr/xr_server.h"

#include <emscripten.h>

#include <cstdlib>

void _emwebxr_on_session_supported(char *p_session_mode, int p_supported) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	(void)p_session_mode;
	(void)p_supported;
}

void _emwebxr_on_session_started(char *p_reference_space_type, char *p_enabled_features, char *p_environment_blend_mode) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	interface->_set_reference_space_type(reference_space_type);
	interface->_set_enabled_features(p_enabled_features);
	interface->_set_environment_blend_mode(p_environment_blend_mode);
}

void _emwebxr_on_session_ended() {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
}

void _emwebxr_on_session_failed(char *p_message) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();

	ERR_PRINT(String(p_message));
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_input_event(int p_event_type, int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->_on_input_event(p_event_type, p_input_source_id);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_simple_event(char *p_signal_name) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	Ref<WebXRInterface> interface = XRServer::find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String signal_name = String(p_signal_name);
	(void)signal_name;
}

void WebXRInterface::is_session_supported(const String &p_session_mode) {
	godot_webxr_is_session_supported(p_session_mode.utf8().get_data(), &_emwebxr_on_session_supported);
}

void WebXRInterface::set_session_mode(const String &p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterface::get_session_mode() const {
	return session_mode;
}

void WebXRInterface::set_required_features(const String &p_required_features) {
	required_features = p_required_features;
}

String WebXRInterface::get_required_features() const {
	return required_features;
}

void WebXRInterface::set_optional_features(const String &p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterface::get_optional_features() const {
	return optional_features;
}

void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterface::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

String WebXRInterface::get_reference_space_type() const {
	return reference_space_type;
}

String WebXRInterface::get_enabled_features() const {
	return enabled_features;
}

bool WebXRInterface::is_input_source_active(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, false);
	return input_sources[p_input_source_id].active;
}

Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, Ref<XRTracker>());
	return input_sources[p_input_source_id].tracker;
}

WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, WebXRInterface::TARGET_RAY_MODE_UNKNOWN);
	if (!input_sources[p_input_source_id].active) {
		return WebXRInterface::TARGET_RAY_MODE_UNKNOWN;
	}
	return input_sources[p_input_source_id].target_ray_mode;
}

String WebXRInterface::get_visibility_state() const {
	char *c_str = godot_webxr_get_visibility_state();
	if (c_str) {
		String visibility_state = String(c_str);
		free(c_str);

		return visibility_state;
	}
	return String();
}

PackedVector3Array WebXRInterface::get_play_area() const {
	PackedVector3Array ret;

	float *points;
	int point_count = godot_webxr_get_bounds_geometry(&points);
	if (point_count > 0) {
		ret.resize(point_count);
		for (int i = 0; i < point_count; i++) {
			float *js_vector3 = points + (i * 3);
			ret.set(i, Vector3(js_vector3[0], js_vector3[1], js_vector3[2]));
		}
		free(points);
	}

	return ret;
}

float WebXRInterface::get_display_refresh_rate() const {
	return godot_webxr_get_frame_rate();
}

void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {
	godot_webxr_update_target_frame_rate(p_refresh_rate);
}

Vector<float> WebXRInterface::get_available_display_refresh_rates() const {
	Vector<float> ret;

	float *rates;
	int rate_count = godot_webxr_get_supported_frame_rates(&rates);
	if (rate_count > 0) {
		ret.resize(rate_count);
		for (int i = 0; i < rate_count; i++) {
			ret.write[i] = rates[i];
		}
		free(rates);
	}

	return ret;
}

Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() {
	Vector<XRInterface::EnvironmentBlendMode> blend_modes;
	// The blend mode can't be changed, so return the current blend mode as the only supported one.
	blend_modes.push_back(environment_blend_mode);
	return blend_modes;
}

XRInterface::EnvironmentBlendMode WebXRInterface::get_environment_blend_mode() const {
	return environment_blend_mode;
}

bool WebXRInterface::set_environment_blend_mode(EnvironmentBlendMode p_new_environment_blend_mode) {
	if (environment_blend_mode == p_new_environment_blend_mode) {
		// Environment blend mode can't be changed, but we'll consider it a success to set it
		// to what it already is.
		return true;
	}
	return false;
}

void WebXRInterface::_set_environment_blend_mode(const String &p_blend_mode_string) {
	if (p_blend_mode_string == "opaque") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
	} else if (p_blend_mode_string == "additive") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ADDITIVE;
	} else if (p_blend_mode_string == "alpha-blend") {
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
	} else {
		// Not all browsers can give us this information, so as a fallback,
		// we'll make some guesses about the blend mode.
		if (session_mode == "immersive-ar") {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} else {
			environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		}
	}
}

String WebXRInterface::get_name() const {
	return "WebXR";
}

uint32_t WebXRInterface::get_capabilities() const {
	return XRInterface::XR_STEREO | XRInterface::XR_MONO | XRInterface::XR_VR | XRInterface::XR_AR;
}

uint32_t WebXRInterface::get_view_count() {
	return godot_webxr_get_view_count();
}

bool WebXRInterface::is_initialized() const {
	return (initialized);
}

bool WebXRInterface::initialize() {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	if (!initialized) {
		if (!godot_webxr_is_supported()) {
			ERR_PRINT("WebXR is unsupported by this web browser.");
			return false;
		}

		if (session_mode == "immersive-vr" && !GLES3::Config::multiview_supported) {
			ERR_PRINT("Stereo rendering in Godot requires multiview, but this web browser doesn't support it.");
			return false;
		}

		if (requested_reference_space_types.is_empty()) {
			ERR_PRINT("No reference spaces were requested.");
			return false;
		}

		enabled_features.clear();

		// We must create a tracker for our head.
		head_transform.basis = Basis();
		head_transform.origin = Vector3();
		head_tracker.instantiate();
		head_tracker->set_tracker_type(XRServer::TRACKER_HEAD);
		head_tracker->set_tracker_name("head");
		head_tracker->set_tracker_desc("Players head");
		XRServer::add_tracker(head_tracker);

		// Make this our primary interface.
		XRServer::set_primary_interface(this);

		// Clear render_targetsize to make sure it gets reset to the new size.
		// Clearing in uninitialize() doesn't work because a frame can still be
		// rendered after it's called, which will fill render_targetsize again.
		render_targetsize.width = 0;
		render_targetsize.height = 0;

		initialized = true;

		godot_webxr_initialize(
				session_mode.utf8().get_data(),
				required_features.utf8().get_data(),
				optional_features.utf8().get_data(),
				requested_reference_space_types.utf8().get_data(),
				&_emwebxr_on_session_started,
				&_emwebxr_on_session_ended,
				&_emwebxr_on_session_failed,
				&_emwebxr_on_input_event,
				&_emwebxr_on_simple_event);
	};

	return true;
}

void WebXRInterface::uninitialize() {
	if (initialized) {
		if (XRServer::is_initialized()) {
			if (head_tracker.is_valid()) {
				XRServer::remove_tracker(head_tracker);

				head_tracker.unref();
			}

			for (int i = 0; i < HAND_MAX; i++) {
				if (hand_trackers[i].is_valid()) {
					XRServer::remove_tracker(hand_trackers[i]);

					hand_trackers[i].unref();
				}
			}

			if (XRServer::get_primary_interface() == this) {
				// no longer our primary interface
				XRServer::set_primary_interface(nullptr);
			}
		}

		godot_webxr_uninitialize();

		if (GLES3::TextureStorage::is_initialized()) {
			for (KeyValue<unsigned int, RID> &E : texture_cache) {
				// Forcibly mark as not part of a render target so we can free it.
				GLES3::Texture *texture = GLES3::TextureStorage::get_texture(E.value);
				texture->is_render_target = false;

				GLES3::TextureStorage::texture_free(E.value);
			}
		}

		texture_cache.clear();
		reference_space_type.clear();
		enabled_features.clear();
		environment_blend_mode = XRInterface::XR_ENV_BLEND_MODE_OPAQUE;
		initialized = false;
	};
}

HashMap<String, String> WebXRInterface::get_system_info() {
	HashMap<String, String> dict;

	// TODO get actual information from WebXR to return here
	dict[String("XRRuntimeName")] = String("WebXR");
	dict[String("XRRuntimeVersion")] = String("");

	return dict;
}

Transform3D WebXRInterface::_js_matrix_to_transform(float *p_js_matrix) {
	Transform3D transform;

	transform.basis.rows[0].x = p_js_matrix[0];
	transform.basis.rows[1].x = p_js_matrix[1];
	transform.basis.rows[2].x = p_js_matrix[2];
	transform.basis.rows[0].y = p_js_matrix[4];
	transform.basis.rows[1].y = p_js_matrix[5];
	transform.basis.rows[2].y = p_js_matrix[6];
	transform.basis.rows[0].z = p_js_matrix[8];
	transform.basis.rows[1].z = p_js_matrix[9];
	transform.basis.rows[2].z = p_js_matrix[10];
	transform.origin.x = p_js_matrix[12];
	transform.origin.y = p_js_matrix[13];
	transform.origin.z = p_js_matrix[14];

	return transform;
}

Size2 WebXRInterface::get_render_target_size() {
	if (render_targetsize.width != 0 && render_targetsize.height != 0) {
		return render_targetsize;
	}

	int js_size[2];
	bool has_size = godot_webxr_get_render_target_size(js_size);

	if (!initialized || !has_size) {
		// As a temporary default (until WebXR is fully initialized), use the
		// window size.
		return DisplayServer::window_get_size();
	}

	render_targetsize.width = (float)js_size[0];
	render_targetsize.height = (float)js_size[1];

	return render_targetsize;
}

Transform3D WebXRInterface::get_camera_transform() {
	Transform3D camera_transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), camera_transform);

	if (initialized) {
		double world_scale = XRServer::get_world_scale();

		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		camera_transform = (XRServer::get_reference_frame()) * _head_transform;
	}

	return camera_transform;
}

Transform3D WebXRInterface::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	ERR_FAIL_COND_V(!XRServer::is_initialized(), p_cam_transform);
	ERR_FAIL_COND_V(!initialized, p_cam_transform);

	float js_matrix[16];
	bool has_transform = godot_webxr_get_transform_for_view(p_view, js_matrix);
	if (!has_transform) {
		return p_cam_transform;
	}

	Transform3D transform_for_view = _js_matrix_to_transform(js_matrix);

	double world_scale = XRServer::get_world_scale();
	transform_for_view.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * transform_for_view;
}

Projection WebXRInterface::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	Projection view;

	ERR_FAIL_COND_V(!initialized, view);

	float js_matrix[16];
	bool has_projection = godot_webxr_get_projection_for_view(p_view, js_matrix);
	if (!has_projection) {
		return view;
	}

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			view.columns[i][j] = js_matrix[k++];
		}
	}

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	view.columns[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	view.columns[3][2] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return view;
}

bool WebXRInterface::pre_draw_viewport(RID p_render_target) {
	if (!GLES3::TextureStorage::is_initialized()) {
		return false;
	}

	// Cache the resources so we don't have to get them from JS twice.
	color_texture = _get_color_texture();
	depth_texture = _get_depth_texture();

	// Per the WebXR spec, it returns "opaque textures" to us, which may be the
	// same WebGLTexture object (which would be the same GLuint in C++) but
	// represent a different underlying resource (probably the next texture in
	// the XR device's swap chain). In order to render to this texture, we need
	// to re-attach it to the FBO, otherwise we get an "incomplete FBO" error.
	//
	// See: https://immersive-web.github.io/layers/#xropaquetextures
	//
	// So, even if the color and depth textures have the same GLuint as the last
	// frame, we need to re-attach them again.
	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, true);

	return true;
}

Vector<RenderingServerTypes::BlitToScreen> WebXRInterface::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

	if (!GLES3::TextureStorage::is_initialized()) {
		return blit_to_screen;
	}

	GLES3::TextureStorage::render_target_set_reattach_textures(p_render_target, false);

	return blit_to_screen;
}

RID WebXRInterface::_get_color_texture() {
	unsigned int texture_id = godot_webxr_get_color_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_depth_texture() {
	unsigned int texture_id = godot_webxr_get_depth_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterface::_get_texture(unsigned int p_texture_id) {
	RBMap<unsigned int, RID>::Element *cache = texture_cache.find(p_texture_id);
	if (cache != nullptr) {
		return cache->get();
	}

	if (!GLES3::TextureStorage::is_initialized()) {
		return RID();
	}

	uint32_t view_count = godot_webxr_get_view_count();
	Size2 texture_size = get_render_target_size();

	RID texture = GLES3::TextureStorage::texture_create_from_native_handle(
			view_count == 1 ? RSE::TEXTURE_TYPE_2D : RSE::TEXTURE_TYPE_LAYERED,
			Image::FORMAT_RGBA8,
			p_texture_id,
			(int)texture_size.width,
			(int)texture_size.height,
			1,
			view_count);

	texture_cache.insert(p_texture_id, texture);

	return texture;
}

RID WebXRInterface::get_color_texture() {
	return color_texture;
}

RID WebXRInterface::get_depth_texture() {
	return depth_texture;
}

RID WebXRInterface::get_velocity_texture() {
	unsigned int texture_id = godot_webxr_get_velocity_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

void WebXRInterface::process() {
	if (initialized) {
		// Get the "head" position.
		float js_matrix[16];
		if (godot_webxr_get_transform_for_view(-1, js_matrix)) {
			head_transform = _js_matrix_to_transform(js_matrix);
		}
		if (head_tracker.is_valid()) {
			head_tracker->set_pose("default", head_transform, Vector3(), Vector3());
		}

		// Update all input sources.
		for (int i = 0; i < input_source_count; i++) {
			_update_input_source(i);
		}
	};
}

void WebXRInterface::_update_input_source(int p_input_source_id) {
	ERR_FAIL_COND(!XRServer::is_initialized());

	InputSource &input_source = input_sources[p_input_source_id];

	float target_pose[16];
	int tmp_target_ray_mode;
	int touch_index;
	int has_grip_pose;
	float grip_pose[16];
	int has_standard_mapping;
	int button_count;
	float buttons[10];
	int axes_count;
	float axes[10];
	int has_hand_data;
	float hand_joints[WEBXR_HAND_JOINT_MAX * 16];
	float hand_radii[WEBXR_HAND_JOINT_MAX];

	input_source.active = godot_webxr_update_input_source(
			p_input_source_id,
			target_pose,
			&tmp_target_ray_mode,
			&touch_index,
			&has_grip_pose,
			grip_pose,
			&has_standard_mapping,
			&button_count,
			buttons,
			&axes_count,
			axes,
			&has_hand_data,
			hand_joints,
			hand_radii);

	if (!input_source.active) {
		if (input_source.tracker.is_valid()) {
			XRServer::remove_tracker(input_source.tracker);
			input_source.tracker.unref();
		}
		return;
	}

	input_source.target_ray_mode = (WebXRInterface::TargetRayMode)tmp_target_ray_mode;
	input_source.touch_index = touch_index;

	Ref<XRTracker> &tracker = input_source.tracker;

	if (tracker.is_null()) {
		tracker.instantiate();
		tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);

		String tracker_name;
		if (input_source.target_ray_mode == WebXRInterface::TargetRayMode::TARGET_RAY_MODE_SCREEN) {
			tracker_name = touch_names[touch_index];
		} else {
			tracker_name = tracker_names[p_input_source_id];
		}

		// Input source id's 0 and 1 are always the left and right hands.
		if (p_input_source_id < 2) {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(p_input_source_id == 0 ? "Left hand controller" : "Right hand controller");
			tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
		} else {
			tracker->set_tracker_name(tracker_name);
			tracker->set_tracker_desc(tracker_name);
		}
		XRServer::add_tracker(tracker);
	}

	Transform3D aim_transform = _js_matrix_to_transform(target_pose);
	tracker->set_pose(SceneString(default_), aim_transform, Vector3(), Vector3());
	tracker->set_pose(String("aim"), aim_transform, Vector3(), Vector3());
	if (has_grip_pose) {
		tracker->set_pose(String("grip"), _js_matrix_to_transform(grip_pose), Vector3(), Vector3());
	}

	for (int i = 0; i < button_count; i++) {
		String button_name = has_standard_mapping ? standard_button_names[i] : unknown_button_names[i];
		String button_pressure_name = has_standard_mapping ? standard_button_pressure_names[i] : unknown_button_pressure_names[i];
		float value = buttons[i];
		bool state = value > 0.0;
		tracker->set_input(button_name, state);
		tracker->set_input(button_pressure_name, value);
	}

	for (int i = 0; i < axes_count; i++) {
		String axis_name = has_standard_mapping ? standard_axis_names[i] : unknown_axis_names[i];
		float value = axes[i];
		if (has_standard_mapping && (i == 1 || i == 3)) {
			// Invert the Y-axis on thumbsticks and trackpads, in order to
			// match OpenXR and other XR platform SDKs.
			value = -value;
		}
		tracker->set_input(axis_name, value);
	}

	// Also create Vector2's for the thumbstick and trackpad when we have the
	// standard mapping.
	if (has_standard_mapping) {
		if (axes_count >= 2) {
			tracker->set_input(standard_vector_names[0], Vector2(axes[0], -axes[1]));
		}
		if (axes_count >= 4) {
			tracker->set_input(standard_vector_names[1], Vector2(axes[2], -axes[3]));
		}
	}

	if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
		if (touch_index < 5 && axes_count >= 2) {
			Vector2 joy_vector = Vector2(axes[0], axes[1]);
			Vector2 position = _get_screen_position_from_joy_vector(joy_vector);

			if (touches[touch_index].is_touching) {
				Vector2 delta = position - touches[touch_index].position;

				// If position has changed by at least 1 pixel, generate a drag event.
				if (std::abs(delta.x) >= 1.0 || std::abs(delta.y) >= 1.0) {
					InputEvent event = InputEvent::make_screen_drag();
					event.screen_drag.index = touch_index;
					event.screen_drag.position = position;
					event.screen_drag.relative = delta;
					event.screen_drag.screen_relative = delta;
					Input::parse_input_event(event);
				}
			}

			touches[touch_index].position = position;
		}
	}

	if (p_input_source_id < 2) {
		Ref<XRTracker> hand_tracker = hand_trackers[p_input_source_id];
		if (has_hand_data) {
			// Transform orientations to match Godot Humanoid skeleton.
			const Basis bone_adjustment(
					Vector3(-1.0, 0.0, 0.0),
					Vector3(0.0, 0.0, -1.0),
					Vector3(0.0, -1.0, 0.0));

			if (unlikely(hand_tracker.is_null())) {
				hand_tracker.instantiate();
				hand_tracker->set_tracker_type(XRServer::TRACKER_HAND);
				hand_tracker->set_tracker_hand(p_input_source_id == 0 ? XRTracker::TRACKER_HAND_LEFT : XRTracker::TRACKER_HAND_RIGHT);
				hand_tracker->set_tracker_name(p_input_source_id == 0 ? "/user/hand_tracker/left" : "/user/hand_tracker/right");

				// These flags always apply, since WebXR doesn't give us enough insight to be more fine grained.
				uint32_t joint_flags(XRTracker::HAND_JOINT_FLAG_POSITION_VALID | XRTracker::HAND_JOINT_FLAG_ORIENTATION_VALID | XRTracker::HAND_JOINT_FLAG_POSITION_TRACKED | XRTracker::HAND_JOINT_FLAG_ORIENTATION_TRACKED);
				for (int godot_joint = 0; godot_joint < XRTracker::HAND_JOINT_MAX; godot_joint++) {
					hand_tracker->set_hand_joint_flags((XRTracker::HandJoint)godot_joint, joint_flags);
				}

				hand_trackers[p_input_source_id] = hand_tracker;
				XRServer::add_tracker(hand_tracker);
			}

			hand_tracker->set_has_tracking_data(true);
			for (int webxr_joint = 0; webxr_joint < WEBXR_HAND_JOINT_MAX; webxr_joint++) {
				XRTracker::HandJoint godot_joint = (XRTracker::HandJoint)(webxr_joint + 1);

				Transform3D joint_transform = _js_matrix_to_transform(hand_joints + (16 * webxr_joint));
				joint_transform.basis *= bone_adjustment;
				hand_tracker->set_hand_joint_transform(godot_joint, joint_transform);

				hand_tracker->set_hand_joint_radius(godot_joint, hand_radii[webxr_joint]);
			}

			// WebXR doesn't have a palm joint, so we calculate it by finding the middle of the middle finger metacarpal bone.
			{
				// Start by getting the middle finger metacarpal joint.
				// Note: 10 is the WebXR middle finger metacarpal joint.
				Transform3D palm_transform = _js_matrix_to_transform(hand_joints + (10 * 16));
				palm_transform.basis *= bone_adjustment;

				// Get the middle finger phalanx position.
				// Note: 11 is the WebXR middle finger phalanx proximal joint and 12 is the origin offset.
				const float *phalanx_pos = hand_joints + (11 * 16) + 12;
				Vector3 phalanx(phalanx_pos[0], phalanx_pos[1], phalanx_pos[2]);

				// Offset the palm half-way towards the phalanx joint.
				palm_transform.origin = (palm_transform.origin + phalanx) / 2.0;

				// Set the palm joint and the pose.
				hand_tracker->set_hand_joint_transform(XRTracker::HAND_JOINT_PALM, palm_transform);
				hand_tracker->set_pose("default", palm_transform, Vector3(), Vector3());
			}

		} else if (hand_tracker.is_valid()) {
			hand_tracker->set_has_tracking_data(false);
			hand_tracker->invalidate_pose("default");
		}
	}
}

void WebXRInterface::_on_input_event(int p_event_type, int p_input_source_id) {
	// Get the latest data for this input source. For transient input sources,
	// we may not have any data at all yet!
	_update_input_source(p_input_source_id);

	if (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART || p_event_type == WEBXR_INPUT_EVENT_SELECTEND) {
		const InputSource &input_source = input_sources[p_input_source_id];
		if (input_source.target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
			int touch_index = input_source.touch_index;
			if (touch_index >= 0 && touch_index < 5) {
				touches[touch_index].is_touching = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				InputEvent event = InputEvent::make_screen_touch();
				event.touch.index = touch_index;
				event.touch.position = touches[touch_index].position;
				event.pressed = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				Input::parse_input_event(event);
			}
		}
	}

	(void)p_event_type;
	(void)p_input_source_id;
}

Vector2 WebXRInterface::_get_screen_position_from_joy_vector(const Vector2 &p_joy_vector) {
	if (!SceneTree::is_initialized()) {
		return Vector2();
	}

	Window *viewport = SceneTree::get_root();
	if (!viewport) {
		return Vector2();
	}

	Vector2 position_percentage((p_joy_vector.x + 1.0f) / 2.0f, ((p_joy_vector.y) + 1.0f) / 2.0f);
	Vector2 position = (Size2)viewport->get_size() * position_percentage;

	return position;
}

WebXRInterface::WebXRInterface() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
}

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (initialized) {
		uninitialize();
	};
}

#else // !WEB_ENABLED

void WebXRInterface::is_session_supported(const String &p_session_mode) {}
void WebXRInterface::set_session_mode(const String &p_session_mode) {}
String WebXRInterface::get_session_mode() const { return String(); }
void WebXRInterface::set_required_features(const String &p_required_features) {}
String WebXRInterface::get_required_features() const { return String(); }
void WebXRInterface::set_optional_features(const String &p_optional_features) {}
String WebXRInterface::get_optional_features() const { return String(); }
void WebXRInterface::set_requested_reference_space_types(const String &p_requested_reference_space_types) {}
String WebXRInterface::get_requested_reference_space_types() const { return String(); }
String WebXRInterface::get_reference_space_type() const { return String(); }
String WebXRInterface::get_enabled_features() const { return String(); }
bool WebXRInterface::is_input_source_active(int p_input_source_id) const { return false; }
Ref<XRTracker> WebXRInterface::get_input_source_tracker(int p_input_source_id) const { return Ref<XRTracker>(); }
WebXRInterface::TargetRayMode WebXRInterface::get_input_source_target_ray_mode(int p_input_source_id) const { return TARGET_RAY_MODE_UNKNOWN; }
String WebXRInterface::get_visibility_state() const { return String(); }
float WebXRInterface::get_display_refresh_rate() const { return 0.0f; }
void WebXRInterface::set_display_refresh_rate(float p_refresh_rate) {}
Vector<float> WebXRInterface::get_available_display_refresh_rates() const { return Vector<float>(); }
Vector<XRInterface::EnvironmentBlendMode> WebXRInterface::get_supported_environment_blend_modes() { return Vector<XRInterface::EnvironmentBlendMode>(); }
HashMap<String, String> WebXRInterface::get_system_info() { return HashMap<String, String>(); }

String WebXRInterface::get_name() const { return "WebXR"; }
uint32_t WebXRInterface::get_capabilities() const { return 0; }
bool WebXRInterface::is_initialized() const { return false; }
bool WebXRInterface::initialize() { return false; }
void WebXRInterface::uninitialize() {}

#endif // WEB_ENABLED

#ifdef WEB_ENABLED
#endif


#ifdef WEB_ENABLED
static Ref<WebXRInterface> webxr;
#endif

void initialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
#ifdef WEB_ENABLED
	if (XRServer::is_initialized()) {
		webxr.instantiate();
		XRServer::add_interface(webxr);
	}
#endif
}

void uninitialize_webxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef WEB_ENABLED
	if (webxr.is_valid()) {
		// uninitialize our interface if it is initialized
		if (webxr->is_initialized()) {
			webxr->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			XRServer::remove_interface(webxr);
		}

		// and release
		webxr.unref();
	}
#endif
}
#endif // defined(MODULE_WEBXR_ENABLED)

// === Merged from modules/camera/register_types.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_CAMERA_ENABLED)
#include "modules/register_module_types.h"

#if defined(LINUXBSD_ENABLED)
// --- Inlined/Folded from modules/camera/camera_linux.h ---
#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/camera/camera_server.h"


#include "core/io/image.h"
#include "core/templates/vector.h"

class CameraFeed;

struct StreamingBuffer
{
	void* start = nullptr;
	size_t length = 0;
};

class BufferDecoder
{
protected:
	CameraFeed* camera_feed = nullptr;
	Ref<Image> image;
	int width = 0;
	int height = 0;

public:
	virtual void decode(StreamingBuffer p_buffer);

	BufferDecoder() = default;
	BufferDecoder(CameraFeed* p_camera_feed);

	virtual ~BufferDecoder();
};

class AbstractYuyvBufferDecoder : public BufferDecoder
{
protected:
	int* component_indexes = nullptr;

public:
	AbstractYuyvBufferDecoder(CameraFeed* p_camera_feed);
	~AbstractYuyvBufferDecoder();
};

class SeparateYuyvBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> y_image_data;
	Vector<uint8_t> cbcr_image_data;
	Ref<Image> y_image;
	Ref<Image> cbcr_image;

public:
	SeparateYuyvBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class YuyvToGrayscaleBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	YuyvToGrayscaleBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class YuyvToRgbBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	YuyvToRgbBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class CopyBufferDecoder final : public BufferDecoder
{
private:
	Vector<uint8_t> image_data;
	bool rgba = false;

public:
	CopyBufferDecoder(CameraFeed* p_camera_feed, bool p_rgba);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class JpegBufferDecoder final : public BufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	JpegBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

#include <linux/videodev2.h>
#include "servers/camera/camera_server.h"

class CameraFeedLinux final : public CameraFeed
{
private:
	SafeFlag exit_flag;
	Thread* thread = nullptr;
	String device_name;
	int file_descriptor = -1;
	StreamingBuffer* buffers = nullptr;
	unsigned int buffer_count = 0;
	BufferDecoder* buffer_decoder = nullptr;

	static void update_buffer_thread_func(void* p_func);

	void _update_buffer();
	void _query_device(const String& p_device_name);
	void _add_format(v4l2_fmtdesc description, v4l2_frmsize_discrete size, int frame_numerator,
		int frame_denominator);
	bool _request_buffers();
	bool _start_capturing();
	void _read_frame();
	void _stop_capturing();
	void _unmap_buffers(unsigned int p_count);
	BufferDecoder* _create_buffer_decoder();
	void _start_thread();

public:
	String get_device_name() const;
	bool activate_feed() override;
	void deactivate_feed() override;
	FeedFormat get_format() const override;

	CameraFeedLinux(const String& p_device_name);
	~CameraFeedLinux() override;
};
class CameraLinux final
{
private:
	struct Data
	{
		SafeFlag exit_flag;
		Thread camera_thread;
		Mutex camera_mutex;
	};

	static inline Data* data = nullptr;

	static void camera_thread_func(void* p_userdata);

	static void _update_devices();
	static bool _has_device(const String& p_device_name);
	static void _add_device(const String& p_device_name);
	static int _open_device(const String& p_device_name);
	static bool _is_active(const String& p_device_name);
	static bool _is_video_capture_device(int p_file_descriptor);
	static bool _can_query_format(int p_file_descriptor, int p_type);

public:
	static void initialize();
	static void finalize();

	static void set_monitoring_feeds(bool p_monitoring_feeds);
	static void handle_application_pause() {}
	static void handle_application_resume() {}
	static void handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	CameraLinux() = delete;
	CameraLinux(const CameraLinux&) = delete;
	CameraLinux& operator=(const CameraLinux&) = delete;
	~CameraLinux() = delete;
};
#endif
#if defined(WINDOWS_ENABLED)
// --- Inlined/Folded from modules/camera/camera_win.h ---
#include "servers/camera/camera_server.h"

class CameraWindows final
{
private:
	static void add_active_cameras();

public:
	static void initialize();
	static void finalize() {}

	static void set_monitoring_feeds(bool p_monitoring_feeds)
	{
		CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	}

	static void handle_application_pause() {}
	static void handle_application_resume() {}
	static void handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	CameraWindows() = delete;
	CameraWindows(const CameraWindows&) = delete;
	CameraWindows& operator=(const CameraWindows&) = delete;
	~CameraWindows() = delete;
};
#endif
#if defined(MACOS_ENABLED)
#include "servers/camera/camera_server.h"
#endif
#if defined(ANDROID_ENABLED)
// --- Inlined/Folded from modules/camera/camera_android.h ---
#include "core/os/mutex.h"
#include "core/templates/safe_refcount.h"
#include "servers/camera/camera_server.h"

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadataTags.h>
#include <media/NdkImageReader.h>

#include <optional>

enum class CameraFacing {
	BACK = 0,
	FRONT = 1,
};

struct CameraRotationParams {
	int sensor_orientation;
	CameraFacing camera_facing;
	int display_rotation;
};

class CameraFeedAndroid : public CameraFeed {

private:
	String camera_id;
	int32_t orientation;
	Ref<Image> image_y;
	Ref<Image> image_uv;
	Vector<uint8_t> data_y;
	Vector<uint8_t> data_uv;
	// Scratch buffers to avoid reallocations when compacting stride-aligned planes.
	Vector<uint8_t> scratch_y;
	Vector<uint8_t> scratch_uv;

	ACameraManager *manager = nullptr;
	ACameraMetadata *metadata = nullptr;
	ACameraDevice *device = nullptr;
	AImageReader *reader = nullptr;
	ACameraCaptureSession *session = nullptr;
	ACaptureRequest *request = nullptr;
	ACaptureSessionOutput *session_output = nullptr;
	ACaptureSessionOutputContainer *session_output_container = nullptr;
	ACameraOutputTarget *camera_output_target = nullptr;
	Mutex callback_mutex;
	Semaphore session_closed_semaphore;
	SafeFlag is_deactivating;
	SafeFlag session_close_pending;
	SafeFlag device_error_occurred;
	bool was_active_before_pause = false;

	// Callback structures - must be instance members, not static, to ensure
	// correct 'this' pointer is captured for each camera feed instance.
	ACameraDevice_stateCallbacks device_callbacks = {};
	AImageReader_ImageListener image_listener = {};
	ACameraCaptureSession_stateCallbacks session_callbacks = {};

	void _add_formats();
	void _set_rotation();
	void refresh_camera_metadata();
	static std::optional<int> calculate_rotation(const CameraRotationParams &p_params);
	static int normalize_angle(int p_angle);
	static int get_display_rotation();
	static int get_app_orientation();
	static void compact_stride_inplace(uint8_t *data, size_t width, int height, size_t stride);

	static void onError(void *context, ACameraDevice *p_device, int error);
	static void onDisconnected(void *context, ACameraDevice *p_device);
	static void onImage(void *context, AImageReader *p_reader);
	static void onSessionReady(void *context, ACameraCaptureSession *session);
	static void onSessionActive(void *context, ACameraCaptureSession *session);
	static void onSessionClosed(void *context, ACameraCaptureSession *session);

protected:
public:
	bool activate_feed() override;
	void deactivate_feed() override;
	bool set_format(int p_index, const Dictionary &p_parameters) override;
	Array get_formats() const override;
	FeedFormat get_format() const override;
	void handle_pause();
	void handle_resume();
	void handle_rotation_change();

	CameraFeedAndroid(ACameraManager *manager, ACameraMetadata *metadata, const char *id,
			CameraFeed::FeedPosition position, int32_t orientation);
	~CameraFeedAndroid() override;
};

class CameraAndroid final
{
private:
	struct Data
	{
		ACameraManager* camera_manager = nullptr;
	};

	static inline Data* data = nullptr;

	static void update_feeds();
	static void remove_all_feeds();

public:
	static void initialize();
	static void finalize();

	static void set_monitoring_feeds(bool p_monitoring_feeds);
	static void handle_application_pause();
	static void handle_application_resume();
	static void handle_display_rotation_change(int p_orientation);

	CameraAndroid() = delete;
	CameraAndroid(const CameraAndroid&) = delete;
	CameraAndroid& operator=(const CameraAndroid&) = delete;
	~CameraAndroid() = delete;
};
#endif

void initialize_camera_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#if defined(LINUXBSD_ENABLED)
	CameraServer::bind<CameraLinux>();
#endif
#if defined(WINDOWS_ENABLED)
	CameraServer::bind<CameraWindows>();
#endif
#if defined(MACOS_ENABLED)
	CameraServer::bind<CameraApple>();
#endif
#if defined(ANDROID_ENABLED)
	CameraServer::bind<CameraAndroid>();
#endif
}

void uninitialize_camera_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}


// --- Merged from modules/camera/camera_linux.cpp ---
#if defined(LINUXBSD_ENABLED)
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>


void CameraLinux::camera_thread_func(void* p_userdata)
{
	(void)p_userdata;
	_update_devices();
}

void CameraLinux::_update_devices()
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	while (!data->exit_flag.is_set()) {
		{
			MutexLock lock(data->camera_mutex);

			for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
				Ref<CameraFeedLinux> feed = (Ref<CameraFeedLinux>)CameraServer::data->feeds[i];
				if (feed.is_null()) {
					continue;
				}
				String device_name = feed->get_device_name();
				if (!_is_active(device_name)) {
					CameraServer::remove_feed(feed);
				}
			}

			struct dirent** devices;
			int count = scandir("/dev", &devices, nullptr, alphasort);

			if (count != -1) {
				for (int i = 0; i < count; i++) {
					struct dirent* device = devices[i];
					if (strncmp(device->d_name, "video", 5) == 0) {
						String device_name = String("/dev/") + String(device->d_name);
						if (!_has_device(device_name)) {
							_add_device(device_name);
						}
					}
					free(device);
				}
			}

			free(devices);
		}
		usleep(1000000);
	}
}

bool CameraLinux::_has_device(const String& p_device_name)
{
	ERR_FAIL_NULL_V(CameraServer::data, false);

	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedLinux> feed = (Ref<CameraFeedLinux>)CameraServer::data->feeds[i];
		if (feed.is_null()) {
			continue;
		}
		if (feed->get_device_name() == p_device_name) {
			return true;
		}
	}
	return false;
}

void CameraLinux::_add_device(const String& p_device_name)
{
	int file_descriptor = _open_device(p_device_name);

	if (file_descriptor != -1) {
		if (_is_video_capture_device(file_descriptor)) {
			Ref<CameraFeedLinux> feed = memnew(CameraFeedLinux(p_device_name));
			CameraServer::add_feed(feed);
		}
	}

	close(file_descriptor);
}

int CameraLinux::_open_device(const String& p_device_name)
{
	struct stat s;

	if (stat(p_device_name.ascii().get_data(), &s) == -1) {
		return -1;
	}

	if (!S_ISCHR(s.st_mode)) {
		return -1;
	}

	return open(p_device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
}

// TODO any cheaper/cleaner way to check if file descriptor is invalid?
bool CameraLinux::_is_active(const String& p_device_name)
{
	struct v4l2_capability capability;
	bool result = false;
	int file_descriptor = _open_device(p_device_name);
	if (file_descriptor != -1 && ioctl(file_descriptor, VIDIOC_QUERYCAP, &capability) != -1) {
		result = true;
	}
	close(file_descriptor);
	return result;
}

bool CameraLinux::_is_video_capture_device(int p_file_descriptor)
{
	struct v4l2_capability capability;

	if (ioctl(p_file_descriptor, VIDIOC_QUERYCAP, &capability) == -1) {
		return false;
	}

	if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		return false;
	}

	if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
		return false;
	}

	return _can_query_format(p_file_descriptor, V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

bool CameraLinux::_can_query_format(int p_file_descriptor, int p_type)
{
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));
	format.type = p_type;

	return ioctl(p_file_descriptor, VIDIOC_G_FMT, &format) != -1;
}

void CameraLinux::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void CameraLinux::finalize()
{
	if (!data) {
		return;
	}
	data->exit_flag.set();
	if (data->camera_thread.is_started()) {
		data->camera_thread.wait_to_finish();
	}
	memdelete(data);
	data = nullptr;
}

void CameraLinux::set_monitoring_feeds(bool p_monitoring_feeds)
{
	ERR_FAIL_NULL(data);
	if (p_monitoring_feeds == CameraServer::is_monitoring_feeds()) {
		return;
	}

	CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	if (p_monitoring_feeds) {
		data->exit_flag.clear();
		data->camera_thread.start(CameraLinux::camera_thread_func, nullptr);
	}
	else {
		data->exit_flag.set();
		if (data->camera_thread.is_started()) {
			data->camera_thread.wait_to_finish();
		}
	}
}

void BufferDecoder::decode(StreamingBuffer p_buffer) {}

BufferDecoder::~BufferDecoder() {}

BufferDecoder::BufferDecoder(CameraFeed* p_camera_feed)
{
	camera_feed = p_camera_feed;
	width = camera_feed->get_format().width;
	height = camera_feed->get_format().height;
	image.instantiate();
}

AbstractYuyvBufferDecoder::AbstractYuyvBufferDecoder(CameraFeed* p_camera_feed)
	: BufferDecoder(p_camera_feed)
{
	switch (camera_feed->get_format().pixel_format) {
	case V4L2_PIX_FMT_YYUV:
		component_indexes = new int[4]{0, 1, 2, 3};
		break;
	case V4L2_PIX_FMT_YVYU:
		component_indexes = new int[4]{0, 2, 3, 1};
		break;
	case V4L2_PIX_FMT_UYVY:
		component_indexes = new int[4]{1, 3, 0, 2};
		break;
	case V4L2_PIX_FMT_VYUY:
		component_indexes = new int[4]{1, 3, 2, 0};
		break;
	default:
		component_indexes = new int[4]{0, 2, 1, 3};
	}
}

AbstractYuyvBufferDecoder::~AbstractYuyvBufferDecoder() { delete[] component_indexes; }

SeparateYuyvBufferDecoder::SeparateYuyvBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	y_image_data.resize(width * height);
	cbcr_image_data.resize(width * height);
	y_image.instantiate();
	cbcr_image.instantiate();
}

void SeparateYuyvBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* y_dst = (uint8_t*)y_image_data.ptrw();
	uint8_t* uv_dst = (uint8_t*)cbcr_image_data.ptrw();
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];
	uint8_t* u_src = src + component_indexes[2];
	uint8_t* v_src = src + component_indexes[3];

	for (int i = 0; i < width * height; i += 2) {
		*y_dst++ = *y0_src;
		*y_dst++ = *y1_src;
		*uv_dst++ = *u_src;
		*uv_dst++ = *v_src;

		y0_src += 4;
		y1_src += 4;
		u_src += 4;
		v_src += 4;
	}

	if (y_image.is_valid()) {
		y_image->set_data(width, height, false, Image::FORMAT_L8, y_image_data);
	}
	else {
		y_image.instantiate(width, height, false, Image::FORMAT_RGB8, y_image_data);
	}
	if (cbcr_image.is_valid()) {
		cbcr_image->set_data(width, height, false, Image::FORMAT_L8, cbcr_image_data);
	}
	else {
		cbcr_image.instantiate(width, height, false, Image::FORMAT_RGB8, cbcr_image_data);
	}

	camera_feed->set_ycbcr_images(y_image, cbcr_image);
}

YuyvToGrayscaleBufferDecoder::YuyvToGrayscaleBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	image_data.resize(width * height);
}

void YuyvToGrayscaleBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];

	for (int i = 0; i < width * height; i += 2) {
		*dst++ = *y0_src;
		*dst++ = *y1_src;

		y0_src += 4;
		y1_src += 4;
	}

	if (image.is_valid()) {
		image->set_data(width, height, false, Image::FORMAT_L8, image_data);
	}
	else {
		image.instantiate(width, height, false, Image::FORMAT_RGB8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

YuyvToRgbBufferDecoder::YuyvToRgbBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	image_data.resize(width * height * 3);
}

void YuyvToRgbBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];
	uint8_t* u_src = src + component_indexes[2];
	uint8_t* v_src = src + component_indexes[3];
	uint8_t* dst = (uint8_t*)image_data.ptrw();

	for (int i = 0; i < width * height; i += 2) {
		int u = *u_src;
		int v = *v_src;
		int u1 = (((u - 128) << 7) + (u - 128)) >> 6;
		int rg = (((u - 128) << 1) + (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
		int v1 = (((v - 128) << 1) + (v - 128)) >> 1;

		*dst++ = CLAMP(*y0_src + v1, 0, 255);
		*dst++ = CLAMP(*y0_src - rg, 0, 255);
		*dst++ = CLAMP(*y0_src + u1, 0, 255);

		*dst++ = CLAMP(*y1_src + v1, 0, 255);
		*dst++ = CLAMP(*y1_src - rg, 0, 255);
		*dst++ = CLAMP(*y1_src + u1, 0, 255);

		y0_src += 4;
		y1_src += 4;
		u_src += 4;
		v_src += 4;
	}

	if (image.is_valid()) {
		image->set_data(width, height, false, Image::FORMAT_RGB8, image_data);
	}
	else {
		image.instantiate(width, height, false, Image::FORMAT_RGB8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

CopyBufferDecoder::CopyBufferDecoder(CameraFeed* p_camera_feed, bool p_rgba)
	: BufferDecoder(p_camera_feed)
{
	rgba = p_rgba;
	image_data.resize(width * height * (rgba ? 4 : 2));
}

void CopyBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	memcpy(dst, p_buffer.start, p_buffer.length);

	if (image.is_valid()) {
		image->set_data(
			width, height, false, rgba ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8, image_data);
	}
	else {
		image.instantiate(
			width, height, false, rgba ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

JpegBufferDecoder::JpegBufferDecoder(CameraFeed* p_camera_feed) : BufferDecoder(p_camera_feed) {}

void JpegBufferDecoder::decode(StreamingBuffer p_buffer)
{
	image_data.resize(p_buffer.length);
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	memcpy(dst, p_buffer.start, p_buffer.length);
	if (image->load_jpg_from_buffer(image_data) == OK) {
		camera_feed->set_rgb_image(image);
	}
}

#include <sys/mman.h>
#include "servers/rendering/renderer.h"

void CameraFeedLinux::update_buffer_thread_func(void* p_func)
{
	if (p_func) {
		CameraFeedLinux* camera_feed_linux = (CameraFeedLinux*)p_func;
		camera_feed_linux->_update_buffer();
	}
}

void CameraFeedLinux::_update_buffer()
{
	while (!exit_flag.is_set()) {
		_read_frame();
		usleep(10000);
	}
}

void CameraFeedLinux::_query_device(const String& p_device_name)
{
	file_descriptor = open(p_device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
	ERR_FAIL_COND_MSG(file_descriptor == -1,
		vformat("Cannot open file descriptor for %s. Error: %d.", p_device_name, errno));

	struct v4l2_capability capability;
	if (ioctl(file_descriptor, VIDIOC_QUERYCAP, &capability) == -1) {
		ERR_FAIL_MSG(vformat("Cannot query device. Error: %d.", errno));
	}
	name = String((char*)capability.card);

	for (int index = 0;; index++) {
		struct v4l2_fmtdesc fmtdesc;
		memset(&fmtdesc, 0, sizeof(fmtdesc));
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmtdesc.index = index;

		if (ioctl(file_descriptor, VIDIOC_ENUM_FMT, &fmtdesc) == -1) {
			break;
		}

		for (int res_index = 0;; res_index++) {
			struct v4l2_frmsizeenum frmsizeenum;
			memset(&frmsizeenum, 0, sizeof(frmsizeenum));
			frmsizeenum.pixel_format = fmtdesc.pixelformat;
			frmsizeenum.index = res_index;

			if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) == -1) {
				break;
			}

			for (int framerate_index = 0;; framerate_index++) {
				struct v4l2_frmivalenum frmivalenum;
				memset(&frmivalenum, 0, sizeof(frmivalenum));
				frmivalenum.pixel_format = fmtdesc.pixelformat;
				frmivalenum.width = frmsizeenum.discrete.width;
				frmivalenum.height = frmsizeenum.discrete.height;
				frmivalenum.index = framerate_index;

				if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) == -1) {
					if (framerate_index == 0) {
						_add_format(fmtdesc, frmsizeenum.discrete, -1, 1);
					}
					break;
				}

				_add_format(fmtdesc, frmsizeenum.discrete, frmivalenum.discrete.numerator,
					frmivalenum.discrete.denominator);
			}
		}
	}

	close(file_descriptor);
}

void CameraFeedLinux::_add_format(v4l2_fmtdesc p_description, v4l2_frmsize_discrete p_size,
	int p_frame_numerator, int p_frame_denominator)
{
	FeedFormat feed_format;
	feed_format.width = p_size.width;
	feed_format.height = p_size.height;
	feed_format.format = String((char*)p_description.description);
	feed_format.frame_numerator = p_frame_numerator;
	feed_format.frame_denominator = p_frame_denominator;
	feed_format.pixel_format = p_description.pixelformat;
	print_verbose(vformat("%s %dx%d@%d/%dfps", (char*)p_description.description, p_size.width,
		p_size.height, p_frame_denominator, p_frame_numerator));
	formats.push_back(feed_format);
}

bool CameraFeedLinux::_request_buffers()
{
	struct v4l2_requestbuffers requestbuffers;

	memset(&requestbuffers, 0, sizeof(requestbuffers));
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;
	requestbuffers.count = 4;

	if (ioctl(file_descriptor, VIDIOC_REQBUFS, &requestbuffers) == -1) {
		ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_REQBUFS) error: %d.", errno));
	}

	ERR_FAIL_COND_V_MSG(requestbuffers.count < 2, false, "Not enough buffers granted.");

	buffer_count = requestbuffers.count;
	buffers = new StreamingBuffer[buffer_count];

	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = requestbuffers.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QUERYBUF, &buffer) == -1) {
			delete[] buffers;
			ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_QUERYBUF) error: %d.", errno));
		}

		buffers[i].length = buffer.length;
		buffers[i].start = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
			file_descriptor, buffer.m.offset);

		if (buffers[i].start == MAP_FAILED) {
			_unmap_buffers(i);
			delete[] buffers;
			ERR_FAIL_V_MSG(false, "Mapping buffers failed.");
		}
	}

	return true;
}

bool CameraFeedLinux::_start_capturing()
{
	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
			ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_QBUF) error: %d.", errno));
		}
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMON, &type) == -1) {
		ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_STREAMON) error: %d.", errno));
	}

	return true;
}

void CameraFeedLinux::_read_frame()
{
	struct v4l2_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (ioctl(file_descriptor, VIDIOC_DQBUF, &buffer) == -1) {
		if (errno != EAGAIN) {
			print_error(vformat("ioctl(VIDIOC_DQBUF) error: %d.", errno));
			exit_flag.set();
		}
		return;
	}

	buffer_decoder->decode(buffers[buffer.index]);

	if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
		print_error(vformat("ioctl(VIDIOC_QBUF) error: %d.", errno));
	}
}

void CameraFeedLinux::_stop_capturing()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMOFF, &type) == -1) {
		print_error(vformat("ioctl(VIDIOC_STREAMOFF) error: %d.", errno));
	}
}

void CameraFeedLinux::_unmap_buffers(unsigned int p_count)
{
	for (unsigned int i = 0; i < p_count; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}
}

void CameraFeedLinux::_start_thread()
{
	exit_flag.clear();
	thread = memnew(Thread);
	thread->start(CameraFeedLinux::update_buffer_thread_func, this);
}

String CameraFeedLinux::get_device_name() const { return device_name; }

bool CameraFeedLinux::activate_feed()
{
	ERR_FAIL_COND_V_MSG(
		selected_format == -1, false, "CameraFeed format needs to be set before activating.");
	file_descriptor = open(device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
	if (_request_buffers() && _start_capturing()) {
		buffer_decoder = _create_buffer_decoder();
		_start_thread();
		return true;
	}
	ERR_FAIL_V_MSG(false, "Could not activate feed.");
}

CameraFeed::FeedFormat CameraFeedLinux::get_format() const
{
	FeedFormat feed_format = {};
	return selected_format == -1 ? feed_format : formats[selected_format];
}

CameraFeedLinux::CameraFeedLinux(const String& p_device_name)
{
	device_name = p_device_name;
	_query_device(device_name);
}

CameraFeedLinux::~CameraFeedLinux()
{
	if (is_active()) {
		deactivate_feed();
	}
}

void CameraFeedLinux::deactivate_feed() {}

BufferDecoder* CameraFeedLinux::_create_buffer_decoder()
{
	BufferDecoder bd = BufferDecoder();
	return &bd;
}
#endif // defined(LINUXBSD_ENABLED)


// --- Merged from modules/camera/camera_win.cpp ---
#if defined(WINDOWS_ENABLED)


#include "servers/camera/camera_server.h"

///@TODO sorry guys, I got about 80% through implementing this using DirectShow only
// to find out Microsoft deprecated half the API and its replacement is as confusing
// as they could make it. Joey suggested looking into libuvc which offers a more direct
// route to webcams over USB and this is very promising but it wouldn't compile on
// windows for me...I've gutted the classes I implemented DirectShow in just to have
// a skeleton for someone to work on, mail me for more details or if you want a copy....

//////////////////////////////////////////////////////////////////////////
// CameraFeedWindows - Subclass for our camera feed on windows

/// @TODO need to implement this

class CameraFeedWindows : public CameraFeed {
private:
protected:
public:
	CameraFeedWindows();
	virtual ~CameraFeedWindows();

	bool activate_feed();
	void deactivate_feed();
};

CameraFeedWindows::CameraFeedWindows() {
	///@TODO implement this, should store information about our available camera
}

CameraFeedWindows::~CameraFeedWindows() {
	// make sure we stop recording if we are!
	if (is_active()) {
		deactivate_feed();
	};

	///@TODO free up anything used by this
}

bool CameraFeedWindows::activate_feed() {
	///@TODO this should activate our camera and start the process of capturing frames

	return true;
}

///@TODO we should probably have a callback method here that is being called by the
// camera API which provides frames and call back into the CameraServer to update our texture

void CameraFeedWindows::deactivate_feed() {
	///@TODO this should deactivate our camera and stop the process of capturing frames
}

//////////////////////////////////////////////////////////////////////////
// CameraWindows - Subclass for our camera server on windows

void CameraWindows::add_active_cameras() {
	///@TODO scan through any active cameras and create CameraFeedWindows objects for them
}

void CameraWindows::initialize() {
	// Find cameras active right now
	add_active_cameras();

	// need to add something that will react to devices being connected/removed...
}
#endif // defined(WINDOWS_ENABLED)


// --- Merged from modules/camera/camera_android.cpp ---
#if defined(ANDROID_ENABLED)


#include "core/os/os.h"

#include "platform/android/os_android.h"

// Scope guard to ensure AImage instances are always deleted.
class ScopedAImage {
	AImage *image = nullptr;

public:
	ScopedAImage() = default;
	~ScopedAImage() {
		reset();
	}

	ScopedAImage(const ScopedAImage &) = delete;
	ScopedAImage &operator=(const ScopedAImage &) = delete;

	AImage **out() {
		return &image;
	}

	AImage *get() const {
		return image;
	}

	void reset(AImage *p_image = nullptr) {
		if (image != nullptr) {
			AImage_delete(image);
		}
		image = p_image;
	}
};

//////////////////////////////////////////////////////////////////////////
// Helper functions
//
// The following code enables you to view the contents of a media type while
// debugging.

#ifndef IF_EQUAL_RETURN
#define MAKE_FORMAT_CONST(suffix) AIMAGE_FORMAT_##suffix
#define IF_EQUAL_RETURN(param, val) \
	if (MAKE_FORMAT_CONST(val) == param) \
	return #val
#endif

String GetFormatName(const int32_t &format) {
	IF_EQUAL_RETURN(format, YUV_420_888);
	IF_EQUAL_RETURN(format, RGB_888);
	IF_EQUAL_RETURN(format, RGBA_8888);

	return "Unsupported";
}

//////////////////////////////////////////////////////////////////////////
// CameraFeedAndroid - Subclass for our camera feed on Android

CameraFeedAndroid::CameraFeedAndroid(ACameraManager *manager, ACameraMetadata *metadata, const char *id,
		CameraFeed::FeedPosition position, int32_t orientation) {
	this->manager = manager;
	this->metadata = metadata;
	this->orientation = orientation;
	_add_formats();
	camera_id = id;
	set_position(position);

	// Position
	switch (position) {
		case CameraFeed::FEED_BACK:
			name = vformat("%s | BACK", id);
			break;
		case CameraFeed::FEED_FRONT:
			name = vformat("%s | FRONT", id);
			break;
		default:
			name = vformat("%s", id);
			break;
	}

	image_y.instantiate();
	image_uv.instantiate();
}

CameraFeedAndroid::~CameraFeedAndroid() {
	if (is_active()) {
		deactivate_feed();
	}
	if (metadata != nullptr) {
		ACameraMetadata_free(metadata);
	}
}

void CameraFeedAndroid::refresh_camera_metadata() {
	ERR_FAIL_NULL_MSG(manager, vformat("Camera %s: Cannot refresh metadata, manager is null.", camera_id));

	if (metadata != nullptr) {
		ACameraMetadata_free(metadata);
		metadata = nullptr;
	}

	camera_status_t status = ACameraManager_getCameraCharacteristics(manager, camera_id.utf8().get_data(), &metadata);
	if (status != ACAMERA_OK || metadata == nullptr) {
		ERR_FAIL_MSG(vformat("Camera %s: Failed to refresh metadata (status: %d).", camera_id, status));
	}

	ACameraMetadata_const_entry orientation_entry;
	status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation_entry);
	if (status == ACAMERA_OK) {
		orientation = orientation_entry.data.i32[0];
		print_verbose(vformat("Camera %s: Orientation updated to %d.", camera_id, orientation));
	} else {
		ERR_PRINT(vformat("Camera %s: Failed to get sensor orientation after refresh (status: %d).", camera_id, status));
	}

	formats.clear();
	_add_formats();

	print_verbose(vformat("Camera %s: Metadata refreshed successfully.", camera_id));
}

void CameraFeedAndroid::_set_rotation() {
	if (!metadata) {
		print_verbose(vformat("Camera %s: Metadata is null in _set_rotation, attempting refresh.", camera_id));
		refresh_camera_metadata();
	}

	float image_rotation = 0.0f;
	std::optional<int> result;

	if (metadata) {
		CameraRotationParams params;
		params.sensor_orientation = orientation;
		params.camera_facing = (position == CameraFeed::FEED_FRONT) ? CameraFacing::FRONT : CameraFacing::BACK;
		params.display_rotation = get_app_orientation();

		result = calculate_rotation(params);
	} else {
		ERR_PRINT(vformat("Camera %s: Cannot update rotation, metadata unavailable after refresh, using fallback.", camera_id));
	}

	if (result.has_value()) {
		image_rotation = static_cast<float>(result.value());
	} else {
		int display_rotation = DisplayServer::driver_as<DisplayServerAndroid>()->get_display_rotation();
		switch (display_rotation) {
			case 90:
				display_rotation = 270;
				break;
			case 270:
				display_rotation = 90;
				break;
			default:
				break;
		}

		int sign = position == CameraFeed::FEED_FRONT ? 1 : -1;
		image_rotation = (orientation - display_rotation * sign + 360) % 360;
	}

	transform = Transform2D();
	transform = transform.rotated(Math::deg_to_rad(image_rotation));
}

void CameraFeedAndroid::_add_formats() {
	// Get supported formats
	ACameraMetadata_const_entry formats;
	camera_status_t status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &formats);

	if (status == ACAMERA_OK) {
		for (uint32_t f = 0; f < formats.count; f += 4) {
			// Only support output streams
			int32_t input = formats.data.i32[f + 3];
			if (input) {
				continue;
			}

			// Get format and resolution
			int32_t format = formats.data.i32[f + 0];
			if (format == AIMAGE_FORMAT_YUV_420_888 ||
					format == AIMAGE_FORMAT_RGBA_8888 ||
					format == AIMAGE_FORMAT_RGB_888) {
				CameraFeed::FeedFormat feed_format;
				feed_format.width = formats.data.i32[f + 1];
				feed_format.height = formats.data.i32[f + 2];
				feed_format.format = GetFormatName(format);
				feed_format.pixel_format = format;
				this->formats.append(feed_format);
			}
		}
	}
}

bool CameraFeedAndroid::activate_feed() {
	ERR_FAIL_COND_V_MSG(formats.is_empty(), false, "No camera formats available.");
	ERR_FAIL_INDEX_V_MSG(selected_format, formats.size(), false,
			vformat("CameraFeed format needs to be set before activating. Selected format index: %d (formats size: %d)", selected_format, formats.size()));
	if (is_active()) {
		deactivate_feed();
	};

	// Clear deactivation and error flags before starting activation.
	is_deactivating.clear();
	device_error_occurred.clear();

	// Request permission
	if (!OS::request_permission("CAMERA")) {
		return false;
	}

	// Open device
	device_callbacks = {
		.context = this,
		.onDisconnected = onDisconnected,
		.onError = onError,
	};
	camera_status_t c_status = ACameraManager_openCamera(manager, camera_id.utf8().get_data(), &device_callbacks, &device);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to open camera (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create image reader
	const FeedFormat &feed_format = formats[selected_format];
	media_status_t m_status = AImageReader_new(feed_format.width, feed_format.height, feed_format.pixel_format, 1, &reader);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to create image reader (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Get image listener
	image_listener = {
		.context = this,
		.onImageAvailable = onImage,
	};
	m_status = AImageReader_setImageListener(reader, &image_listener);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to set image listener (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Get image surface
	ANativeWindow *surface;
	m_status = AImageReader_getWindow(reader, &surface);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to get image surface (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Prepare session outputs
	c_status = ACaptureSessionOutput_create(surface, &session_output);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create session output (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureSessionOutputContainer_create(&session_output_container);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create session output container (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureSessionOutputContainer_add(session_output_container, session_output);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to add session output to container (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create capture session
	session_callbacks = {
		.context = this,
		.onClosed = onSessionClosed,
		.onReady = onSessionReady,
		.onActive = onSessionActive
	};
	c_status = ACameraDevice_createCaptureSession(device, session_output_container, &session_callbacks, &session);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create capture session (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create capture request
	c_status = ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &request);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create capture request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Set capture target
	c_status = ACameraOutputTarget_create(surface, &camera_output_target);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create camera output target (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureRequest_addTarget(request, camera_output_target);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to add target to capture request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Start capture
	c_status = ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &request, nullptr);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to start repeating request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	return true;
}

bool CameraFeedAndroid::set_format(int p_index, const Dictionary &p_parameters) {
	ERR_FAIL_COND_V_MSG(active, false, "Feed is active.");
	ERR_FAIL_INDEX_V_MSG(p_index, formats.size(), false, "Invalid format index.");

	selected_format = p_index;

	// Reset base dimensions to force texture recreation on next frame.
	// This ensures proper handling when switching between different resolutions.
	base_width = 0;
	base_height = 0;

	return true;
}

Array CameraFeedAndroid::get_formats() const {
	Array result;
	for (const FeedFormat &feed_format : formats) {
		Dictionary dictionary;
		dictionary["width"] = feed_format.width;
		dictionary["height"] = feed_format.height;
		dictionary["format"] = feed_format.format;
		result.push_back(dictionary);
	}
	return result;
}

CameraFeed::FeedFormat CameraFeedAndroid::get_format() const {
	CameraFeed::FeedFormat feed_format = {};
	ERR_FAIL_INDEX_V_MSG(selected_format, formats.size(), feed_format,
			vformat("Invalid format index: %d (formats size: %d)", selected_format, formats.size()));
	return formats[selected_format];
}

void CameraFeedAndroid::handle_pause() {
	if (is_active()) {
		was_active_before_pause = true;
		print_verbose(vformat("Camera %s: Pausing (was active).", camera_id));
		deactivate_feed();
	} else {
		was_active_before_pause = false;
	}
}

void CameraFeedAndroid::handle_resume() {
	if (was_active_before_pause) {
		print_verbose(vformat("Camera %s: Resuming.", camera_id));
		activate_feed();
		was_active_before_pause = false;
	}
}

void CameraFeedAndroid::handle_rotation_change() {
	if (!is_active()) {
		return;
	}

	print_verbose(vformat("Camera %s: Handling rotation change.", camera_id));
	refresh_camera_metadata();
	_set_rotation();
}

// In-place stride compaction (handles row padding).
void CameraFeedAndroid::compact_stride_inplace(uint8_t *data, size_t width, int height, size_t stride) {
	if (stride <= width) {
		return;
	}

	uint8_t *src_row = data + stride;
	uint8_t *dst_row = data + width;

	for (int y = 1; y < height; y++) {
		memmove(dst_row, src_row, width);
		src_row += stride;
		dst_row += width;
	}
}

void CameraFeedAndroid::onImage(void *context, AImageReader *p_reader) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);

	// Check deactivation flag before acquiring mutex to avoid using resources
	// that may be deleted during deactivate_feed(). This is a racy check but
	// safe because we only transition is_deactivating from false->true during
	// deactivation, and back to false only at the start of activation.
	if (feed->is_deactivating.is_set()) {
		// Don't try to acquire images - the reader may be deleted.
		// Android will clean up pending images when the reader is deleted.
		return;
	}

	MutexLock lock(feed->callback_mutex);

	// Re-check after acquiring mutex in case deactivation started while waiting.
	if (feed->is_deactivating.is_set()) {
		return;
	}

	// If feed is not active, we must still acquire and discard the image to
	// free the buffer slot. Otherwise, with maxImages=1, the buffer stays full
	// and no new frames will arrive (onImage won't be called again).
	// This can happen when a frame arrives between activate_feed() returning
	// and active=true being set in the base class.
	if (!feed->is_active()) {
		AImage *pending_image = nullptr;
		if (AImageReader_acquireNextImage(p_reader, &pending_image) == AMEDIA_OK && pending_image != nullptr) {
			AImage_delete(pending_image);
		}
		return;
	}

	Vector<uint8_t> data_y;
	Vector<uint8_t> data_uv;

	// Get image
	ScopedAImage image_guard;
	media_status_t status = AImageReader_acquireNextImage(p_reader, image_guard.out());
	ERR_FAIL_COND(status != AMEDIA_OK);
	AImage *image = image_guard.get();

	// Get image data
	uint8_t *data = nullptr;
	int len = 0;
	int32_t pixel_stride, row_stride;
	FeedFormat format = feed->get_format();
	int width = format.width;
	int height = format.height;

	switch (format.pixel_format) {
		case AIMAGE_FORMAT_YUV_420_888: {
			int32_t y_row_stride;
			AImage_getPlaneRowStride(image, 0, &y_row_stride);
			AImage_getPlaneData(image, 0, &data, &len);

			if (len <= 0) {
				return;
			}

			int64_t y_size = Image::get_image_data_size(width, height, Image::FORMAT_R8, false);

			if (y_row_stride == width && len == y_size) {
				data_y.resize(y_size);
				memcpy(data_y.ptrw(), data, y_size);
			} else {
				// Validate buffer size before compaction to prevent out-of-bounds read.
				int64_t required_y_len = (int64_t)y_row_stride * (height - 1) + width;
				if (len < required_y_len) {
					return;
				}
				if (feed->scratch_y.size() < len) {
					feed->scratch_y.resize(len);
				}
				memcpy(feed->scratch_y.ptrw(), data, len);
				CameraFeedAndroid::compact_stride_inplace(feed->scratch_y.ptrw(), width, height, y_row_stride);
				data_y.resize(y_size);
				memcpy(data_y.ptrw(), feed->scratch_y.ptr(), y_size);
			}

			AImage_getPlanePixelStride(image, 1, &pixel_stride);
			AImage_getPlaneRowStride(image, 1, &row_stride);
			AImage_getPlaneData(image, 1, &data, &len);

			if (len <= 0) {
				return;
			}

			int64_t uv_size = Image::get_image_data_size(width / 2, height / 2, Image::FORMAT_RG8, false);

			int uv_width = width / 2;
			int uv_height = height / 2;

			uint8_t *data_v = nullptr;
			int32_t v_pixel_stride = 0;
			int32_t v_row_stride = 0;
			int len_v = 0;

			if (pixel_stride != 2) {
				AImage_getPlanePixelStride(image, 2, &v_pixel_stride);
				AImage_getPlaneRowStride(image, 2, &v_row_stride);
				AImage_getPlaneData(image, 2, &data_v, &len_v);
				if (len_v <= 0) {
					return;
				}
			}

			if (pixel_stride == 2 && row_stride == uv_width * 2 && len == uv_size) {
				data_uv.resize(uv_size);
				memcpy(data_uv.ptrw(), data, uv_size);
			} else if (pixel_stride == 2) {
				// Allow 1-2 byte tolerance for UV buffer (some devices omit final bytes).
				int64_t required_uv_len = (int64_t)row_stride * (uv_height - 1) + uv_width * 2;
				const int64_t UV_TOLERANCE = 2;
				if (len < required_uv_len - UV_TOLERANCE) {
					return;
				}

				if (feed->scratch_uv.size() < required_uv_len) {
					feed->scratch_uv.resize(required_uv_len);
				}
				if (len < required_uv_len) {
					memset(feed->scratch_uv.ptrw() + len, 128, required_uv_len - len);
				}
				memcpy(feed->scratch_uv.ptrw(), data, len);
				if (row_stride != uv_width * 2) {
					CameraFeedAndroid::compact_stride_inplace(feed->scratch_uv.ptrw(), uv_width * 2, uv_height, row_stride);
				}
				data_uv.resize(uv_size);
				memcpy(data_uv.ptrw(), feed->scratch_uv.ptr(), uv_size);
			} else {
				if (data_v && len_v > 0) {
					data_uv.resize(uv_size);
					uint8_t *dst = data_uv.ptrw();
					uint8_t *src_u = data;
					uint8_t *src_v = data_v;
					for (int row = 0; row < uv_height; row++) {
						for (int col = 0; col < uv_width; col++) {
							dst[col * 2] = src_u[col * pixel_stride];
							dst[col * 2 + 1] = src_v[col * v_pixel_stride];
						}
						dst += uv_width * 2;
						src_u += row_stride;
						src_v += v_row_stride;
					}
				} else {
					data_uv.resize(uv_size);
					memset(data_uv.ptrw(), 128, uv_size);
				}
			}

			// Defer to main thread to avoid race conditions with Renderer.
			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_R8, data_y);

			feed->image_uv.instantiate();
			feed->image_uv->initialize_data(width / 2, height / 2, false, Image::FORMAT_RG8, data_uv);

			feed->call_deferred("set_ycbcr_images", feed->image_y, feed->image_uv);
			break;
		}
		case AIMAGE_FORMAT_RGBA_8888: {
			AImage_getPlaneData(image, 0, &data, &len);
			if (len <= 0) {
				return;
			}
			int64_t size = Image::get_image_data_size(width, height, Image::FORMAT_RGBA8, false);
			data_y.resize(len > size ? len : size);
			memcpy(data_y.ptrw(), data, len);

			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_RGBA8, data_y);

			feed->call_deferred("set_rgb_image", feed->image_y);
			break;
		}
		case AIMAGE_FORMAT_RGB_888: {
			AImage_getPlaneData(image, 0, &data, &len);
			if (len <= 0) {
				return;
			}
			int64_t size = Image::get_image_data_size(width, height, Image::FORMAT_RGB8, false);
			data_y.resize(len > size ? len : size);
			memcpy(data_y.ptrw(), data, len);

			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_RGB8, data_y);

			feed->call_deferred("set_rgb_image", feed->image_y);
			break;
		}
		default:
			return;
	}

	if (!feed->formats.is_empty()) {
		if (feed->metadata != nullptr) {
			feed->_set_rotation();
		} else {
			print_verbose(vformat("Camera %s: Metadata invalidated in onImage, attempting refresh.", feed->camera_id));
			feed->refresh_camera_metadata();
			if (feed->metadata != nullptr && !feed->formats.is_empty()) {
				feed->_set_rotation();
			}
		}
	}

	// Release image happens automatically via ScopedAImage.
}

void CameraFeedAndroid::onSessionReady(void *context, ACameraCaptureSession *session) {
	print_verbose("Capture session ready");
}

void CameraFeedAndroid::onSessionActive(void *context, ACameraCaptureSession *session) {
	print_verbose("Capture session active");
}

void CameraFeedAndroid::onSessionClosed(void *context, ACameraCaptureSession *p_session) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	// Only post if deactivate_feed() is waiting for us. This prevents
	// spurious posts from error-triggered session closes that could
	// desynchronize the semaphore count.
	if (feed->session_close_pending.is_set()) {
		feed->session_closed_semaphore.post();
	}
}

void CameraFeedAndroid::deactivate_feed() {
	// Signal that deactivation is in progress. This prevents onImage callbacks
	// from using resources that may be deleted during cleanup.
	is_deactivating.set();

	// First, remove image listener to prevent new callbacks.
	if (reader != nullptr) {
		AImageReader_setImageListener(reader, nullptr);
	}

	// Stop and close capture session.
	// Must wait for session to fully close before closing device.
	if (session != nullptr) {
		ACameraCaptureSession_stopRepeating(session);

		// If an error occurred, the session may have already been closed by
		// Android. In that case, ACameraCaptureSession_close() may not trigger
		// onSessionClosed, so we skip waiting to avoid a deadlock.
		bool skip_wait = device_error_occurred.is_set();

		if (!skip_wait) {
			// Set flag before closing to indicate we're waiting for the callback.
			// This ensures we only wait for the post from THIS close operation.
			session_close_pending.set();
		}

		ACameraCaptureSession_close(session);

		if (!skip_wait) {
			// Wait for onSessionClosed callback to ensure session is fully closed
			// before proceeding to close the device.
			session_closed_semaphore.wait();
			session_close_pending.clear();
		}

		session = nullptr;
	}

	// Now safe to acquire lock and clean up resources.
	// No new callbacks will be triggered after this point.
	MutexLock lock(callback_mutex);

	if (device != nullptr) {
		ACameraDevice_close(device);
		device = nullptr;
	}

	if (reader != nullptr) {
		AImageReader_delete(reader);
		reader = nullptr;
	}

	if (request != nullptr) {
		ACaptureRequest_free(request);
		request = nullptr;
	}

	if (camera_output_target != nullptr) {
		ACameraOutputTarget_free(camera_output_target);
		camera_output_target = nullptr;
	}

	if (session_output_container != nullptr) {
		ACaptureSessionOutputContainer_free(session_output_container);
		session_output_container = nullptr;
	}

	if (session_output != nullptr) {
		ACaptureSessionOutput_free(session_output);
		session_output = nullptr;
	}
}

void CameraFeedAndroid::onError(void *context, ACameraDevice *p_device, int error) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	print_error(vformat("Camera %s error: %d", feed->camera_id, error));
	// Mark that an error occurred. This signals to deactivate_feed() that
	// the session may have been closed by Android, so we shouldn't wait
	// for onSessionClosed.
	feed->device_error_occurred.set();
	onDisconnected(context, p_device);
}

void CameraFeedAndroid::onDisconnected(void *context, ACameraDevice *p_device) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	// Defer to main thread to avoid reentrancy issues when called from
	// ACameraDevice_close() during deactivate_feed().
	feed->call_deferred("set_active", false);
}

//////////////////////////////////////////////////////////////////////////
// CameraAndroid - Subclass for our camera server on Android

void CameraAndroid::initialize() {
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void CameraAndroid::finalize() {
	if (!data) {
		return;
	}
	remove_all_feeds();
	memdelete(data);
	data = nullptr;
}

void CameraAndroid::update_feeds() {
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	ACameraIdList *cameraIds = nullptr;
	camera_status_t c_status = ACameraManager_getCameraIdList(data->camera_manager, &cameraIds);
	ERR_FAIL_COND(c_status != ACAMERA_OK);

	// Deactivate all feeds before removing to ensure proper cleanup.
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid() && feed->is_active()) {
			feed->deactivate_feed();
		}
	}

	// remove existing devices
	for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
		CameraServer::remove_feed(CameraServer::data->feeds[i]);
	}

	for (int c = 0; c < cameraIds->numCameras; ++c) {
		const char *id = cameraIds->cameraIds[c];
		ACameraMetadata *metadata = nullptr;
		ACameraManager_getCameraCharacteristics(data->camera_manager, id, &metadata);
		if (!metadata) {
			continue;
		}

		// Get sensor orientation
		ACameraMetadata_const_entry orientation;
		c_status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation);
		int32_t cameraOrientation;
		if (c_status == ACAMERA_OK) {
			cameraOrientation = orientation.data.i32[0];
		} else {
			cameraOrientation = 0;
			print_error(vformat("Unable to get sensor orientation: %s", id));
		}

		// Get position
		ACameraMetadata_const_entry lensInfo;
		CameraFeed::FeedPosition position = CameraFeed::FEED_UNSPECIFIED;
		camera_status_t status;
		status = ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &lensInfo);
		if (status != ACAMERA_OK) {
			ACameraMetadata_free(metadata);
			continue;
		}
		uint8_t lens_facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(lensInfo.data.u8[0]);
		if (lens_facing == ACAMERA_LENS_FACING_FRONT) {
			position = CameraFeed::FEED_FRONT;
		} else if (lens_facing == ACAMERA_LENS_FACING_BACK) {
			position = CameraFeed::FEED_BACK;
		} else {
			ACameraMetadata_free(metadata);
			continue;
		}

		Ref<CameraFeedAndroid> feed = memnew(CameraFeedAndroid(data->camera_manager, metadata, id, position, cameraOrientation));
		CameraServer::add_feed(feed);
	}

	ACameraManager_deleteCameraIdList(cameraIds);
}

void CameraAndroid::remove_all_feeds() {
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	// Deactivate all feeds before removing to ensure proper cleanup.
	// This prevents "Device is closed but session is not notified" warnings
	// that can occur if feeds are destroyed while still active.
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid() && feed->is_active()) {
			feed->deactivate_feed();
		}
	}

	// remove existing devices
	for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
		CameraServer::remove_feed(CameraServer::data->feeds[i]);
	}

	if (data->camera_manager != nullptr) {
		ACameraManager_delete(data->camera_manager);
		data->camera_manager = nullptr;
	}
}

void CameraAndroid::set_monitoring_feeds(bool p_monitoring_feeds) {
	ERR_FAIL_NULL(data);
	if (p_monitoring_feeds == CameraServer::is_monitoring_feeds()) {
		return;
	}

	CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	if (p_monitoring_feeds) {
		if (data->camera_manager == nullptr) {
			data->camera_manager = ACameraManager_create();
		}

		// Update feeds
		update_feeds();
	} else {
		remove_all_feeds();
	}
}

void CameraAndroid::handle_application_pause() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_pause();
		}
	}
}

void CameraAndroid::handle_application_resume() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_resume();
		}
	}
}

void CameraAndroid::handle_display_rotation_change(int) {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_rotation_change();
		}
	}
}

std::optional<int> CameraFeedAndroid::calculate_rotation(const CameraRotationParams &p_params) {
	if (p_params.sensor_orientation < 0 || p_params.sensor_orientation > 270 || p_params.sensor_orientation % 90 != 0) {
		return std::nullopt;
	}

	int rotation_angle = p_params.sensor_orientation - p_params.display_rotation;
	return normalize_angle(rotation_angle);
}

int CameraFeedAndroid::normalize_angle(int p_angle) {
	while (p_angle < 0) {
		p_angle += 360;
	}
	return p_angle % 360;
}

int CameraFeedAndroid::get_display_rotation() {
	return DisplayServer::driver_as<DisplayServerAndroid>()->get_display_rotation();
}

int CameraFeedAndroid::get_app_orientation() {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	int orientation = godot_io_java->get_screen_orientation();
	switch (orientation) {
		case 0: // SCREEN_LANDSCAPE
			return 90;
		case 1: // SCREEN_PORTRAIT
			return 0;
		case 2: // SCREEN_REVERSE_LANDSCAPE
			return 270;
		case 3: // SCREEN_REVERSE_PORTRAIT
			return 180;
		case 4: // SCREEN_SENSOR_LANDSCAPE
		case 5: // SCREEN_SENSOR_PORTRAIT
		case 6: // SCREEN_SENSOR
		default:
			return get_display_rotation();
	}
}
#endif // defined(ANDROID_ENABLED)
#endif // defined(MODULE_CAMERA_ENABLED)


// --- Merged from modules/glslang/register_types.cpp ---
#if defined(MODULE_GLSLANG_ENABLED)
#include "core/config/engine.h"
#include "servers/rendering/rendering_device_commons.h"

#ifdef D3D12_ENABLED
#include "core/os/os.h"
#endif

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wshadow")

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

VLTR_GCC_WARNING_POP

Vector<uint8_t> compile_glslang_shader(RenderingDeviceCommons::ShaderStage p_stage,
	const String& p_source_code, RenderingDeviceCommons::ShaderLanguageVersion p_language_version,
	RenderingDeviceCommons::ShaderSpirvVersion p_spirv_version, String* r_error)
{
	Vector<uint8_t> ret;
	EShLanguage stages[RenderingDeviceCommons::SHADER_STAGE_MAX] = {
		EShLangVertex,
		EShLangFragment,
		EShLangTessControl,
		EShLangTessEvaluation,
		EShLangCompute,
		EShLangRayGen,
		EShLangAnyHit,
		EShLangClosestHit,
		EShLangMiss,
		EShLangIntersect,
	};

	int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100

	// The enum values can be converted directly.
	glslang::EShTargetClientVersion ClientVersion =
		(glslang::EShTargetClientVersion)p_language_version;
	glslang::EShTargetLanguageVersion TargetVersion =
		(glslang::EShTargetLanguageVersion)p_spirv_version;

	glslang::TShader shader(stages[p_stage]);
	CharString cs = p_source_code.utf8();
	const char* cs_strings = cs.get_data();
	std::string preamble = "";

	shader.setStrings(&cs_strings, 1);
	shader.setEnvInput(glslang::EShSourceGlsl, stages[p_stage], glslang::EShClientVulkan,
		ClientInputSemanticsVersion);
	shader.setEnvClient(glslang::EShClientVulkan, ClientVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

	if (!preamble.empty()) {
		shader.setPreamble(preamble.c_str());
	}

	bool generate_spirv_debug_info = Engine::is_generate_spirv_debug_info_enabled();
#ifdef D3D12_ENABLED
	if (OS::get_current_rendering_driver_name() == "d3d12") {
		// SPIRV to DXIL conversion does not support debug info.
		generate_spirv_debug_info = false;
	}
#endif

	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	if (generate_spirv_debug_info) {
		messages = (EShMessages)(messages | EShMsgDebugInfo);
	}
	const int DefaultVersion = 100;

	// parse
	if (!shader.parse(GetDefaultResources(), DefaultVersion, false, messages)) {
		if (r_error) {
			(*r_error) = "Failed parse:\n";
			(*r_error) += shader.getInfoLog();
			(*r_error) += "\n";
			(*r_error) += shader.getInfoDebugLog();
		}
		return ret;
	}

	// link
	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages)) {
		if (r_error) {
			(*r_error) = "Failed link:\n";
			(*r_error) += program.getInfoLog();
			(*r_error) += "\n";
			(*r_error) += program.getInfoDebugLog();
		}

		return ret;
	}

	std::vector<uint32_t> SpirV;
	spv::SpvBuildLogger logger;
	glslang::SpvOptions spvOptions;

	if (generate_spirv_debug_info) {
		spvOptions.generateDebugInfo = true;
		spvOptions.emitNonSemanticShaderDebugInfo = true;
		spvOptions.emitNonSemanticShaderDebugSource = true;
	}

	glslang::GlslangToSpv(*program.getIntermediate(stages[p_stage]), SpirV, &logger, &spvOptions);

	ret.resize(SpirV.size() * sizeof(uint32_t));
	{
		uint8_t* w = ret.ptrw();
		memcpy(w, &SpirV[0], SpirV.size() * sizeof(uint32_t));
	}

	return ret;
}

void initialize_glslang_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	// Initialize in case it's not initialized. This is done once per thread
	// and it's safe to call multiple times.
	glslang::InitializeProcess();
}

void uninitialize_glslang_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	glslang::FinalizeProcess();
}
#endif // defined(MODULE_GLSLANG_ENABLED)


// --- Merged from modules/raycast/raycast_occlusion_cull.cpp ---
#if defined(MODULE_RAYCAST_ENABLED)
// --- Inlined from modules/raycast/raycast_occlusion_cull.h ---
#include "servers/rendering/renderer_viewport.h"

struct Projection;

class RaycastOcclusionCull final
{
public:
	using HZBuffer = RendererSceneOcclusionCull::HZBuffer;
	struct Data;

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

private:
	static void _init_embree();
	static Vector2 _get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size);

public:
	static bool is_occluder(RID p_rid);
	static RID occluder_allocate();
	static void occluder_initialize(RID p_occluder);
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);
	static void free_occluder(RID p_occluder);

	static void add_scenario(RID p_scenario);
	static void remove_scenario(RID p_scenario);
	static void scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled);
	static void scenario_remove_instance(RID p_scenario, RID p_instance);

	static void add_buffer(RID p_buffer);
	static void remove_buffer(RID p_buffer);
	static HZBuffer* buffer_get_ptr(RID p_buffer);
	static void buffer_set_scenario(RID p_buffer, RID p_scenario);
	static void buffer_set_size(RID p_buffer, const Vector2i& p_size);
	static void buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal);

	static RID buffer_get_debug_texture(RID p_buffer);

	static void set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality);

	RaycastOcclusionCull() = delete;
	RaycastOcclusionCull(const RaycastOcclusionCull&) = delete;
	RaycastOcclusionCull& operator=(const RaycastOcclusionCull&) = delete;
	~RaycastOcclusionCull() = delete;
};

// --- End modules/raycast/raycast_occlusion_cull.h ---
#include "core/io/image.h"
#include "core/math/geometry_3d.h"
#include "modules/register_module_types.h"
#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/projection.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"

#include <embree4/rtcore.h>

#ifdef __SSE2__
#include <pmmintrin.h>
#endif

using CameraRayTile = RTCRayHit16;

struct RendererSceneOcclusionCull::HZBuffer::CameraRayThreadData
{
	int thread_count;
	float z_near;
	float z_far;
	Vector3 camera_dir;
	Vector3 camera_pos;
	Vector3 pixel_corner;
	Vector3 pixel_u_interp;
	Vector3 pixel_v_interp;
	bool camera_orthogonal;
	Size2i buffer_size;
};

namespace
{
struct InstanceID
{
	RID scenario;
	RID instance;

	static uint32_t hash(const InstanceID& p_ins)
	{
		uint32_t h = hash_murmur3_one_64(p_ins.scenario.get_id());
		return hash_fmix32(hash_murmur3_one_64(p_ins.instance.get_id(), h));
	}
	bool operator==(const InstanceID& rhs) const
	{
		return instance == rhs.instance && rhs.scenario == scenario;
	}

	InstanceID() {}
	InstanceID(RID s, RID i) : scenario(s), instance(i) {}
};

struct Occluder
{
	PackedVector3Array vertices;
	PackedInt32Array indices;
	HashSet<InstanceID, InstanceID> users;
};

struct OccluderInstance
{
	RID occluder;
	LocalVector<uint32_t> indices;
	LocalVector<float> xformed_vertices;
	Transform3D xform;
	bool enabled = true;
	bool removed = false;
};

struct Scenario
{
	struct RaycastThreadData
	{
		CameraRayTile* rays = nullptr;
		const uint32_t* masks;
	};

	struct TransformThreadData
	{
		uint32_t thread_count;
		uint32_t vertex_count;
		Transform3D xform;
		const Vector3* read;
		float* write = nullptr;
	};

	Thread* commit_thread = nullptr;
	bool commit_done = true;
	bool dirty = false;

	RTCScene ebr_scene[2] = {nullptr, nullptr};
	int current_scene_idx = 0;

	HashMap<RID, OccluderInstance> instances;
	HashSet<RID> dirty_instances;
	LocalVector<RID> dirty_instances_array;
	LocalVector<RID> removed_instances;

	void _update_dirty_instance(int p_idx, RID* p_instances) {}

	void _update_dirty_instance_thread(int p_idx, RID* p_instances)
	{
		_update_dirty_instance(p_idx, p_instances);
	}

	void _transform_vertices_range(
		const Vector3* p_read, float* p_write, const Transform3D& p_xform, int p_from, int p_to)
	{
		float* floats_w = p_write + 3 * p_from;
		for (int i = p_from; i < p_to; i++) {
			const Vector3 p = p_xform.xform(p_read[i]);
			floats_w[0] = p.x;
			floats_w[1] = p.y;
			floats_w[2] = p.z;
			floats_w += 3;
		}
	}

	void _transform_vertices_thread(uint32_t p_thread, TransformThreadData* p_data)
	{
		uint32_t vertex_total = p_data->vertex_count;
		uint32_t total_threads = p_data->thread_count;
		uint32_t from = p_thread * vertex_total / total_threads;
		uint32_t to = (p_thread + 1 == total_threads)
			? vertex_total
			: ((p_thread + 1) * vertex_total / total_threads);
		_transform_vertices_range(p_data->read, p_data->write, p_data->xform, from, to);
	}

	static void _commit_scene(void* p_ud)
	{
		Scenario* scenario = (Scenario*)p_ud;
		int commit_idx = 1 - (scenario->current_scene_idx);
		rtcCommitScene(scenario->ebr_scene[commit_idx]);
		scenario->commit_done = true;
	}

	void free()
	{
		if (commit_thread) {
			if (commit_thread->is_started()) {
				commit_thread->wait_to_finish();
			}
			memdelete(commit_thread);
			commit_thread = nullptr;
		}

		for (int i = 0; i < 2; i++) {
			if (ebr_scene[i]) {
				rtcReleaseScene(ebr_scene[i]);
				ebr_scene[i] = nullptr;
			}
		}
	}

	void update() {}

	void _raycast(uint32_t p_idx, const RaycastThreadData* p_raycast_data) const
	{
		RTCRayQueryContext context;
		rtcInitRayQueryContext(&context);
		RTCIntersectArguments args;
		rtcInitIntersectArguments(&args);
		args.flags = RTC_RAY_QUERY_FLAG_COHERENT;
		args.context = &context;
		rtcIntersect16((const int*)&p_raycast_data->masks[p_idx * RendererSceneOcclusionCull::HZBuffer::TILE_RAYS],
			ebr_scene[current_scene_idx], &p_raycast_data->rays[p_idx], &args);
	}

	void raycast(CameraRayTile*, unsigned int const*, unsigned int) const {}
};
} // namespace

struct RaycastOcclusionCull::Data
{
	RTCDevice ebr_device = nullptr;
	RID_PtrOwner<Occluder> occluder_owner;
	HashMap<RID, Scenario> scenarios;
	HashMap<RID, HZBuffer> buffers;
	RSE::ViewportOcclusionCullingBuildQuality build_quality =
		RSE::VIEWPORT_OCCLUSION_BUILD_QUALITY_MEDIUM;
	bool _jitter_enabled = false;
};

void RendererSceneOcclusionCull::HZBuffer::clear()
{
	_clear_mips();

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
		camera_rays_unaligned_buffer = nullptr;
		camera_rays = nullptr;
	}
	camera_ray_masks.clear();
	camera_rays_tile_count = 0;
	tile_grid_size = Size2i();
}

void RendererSceneOcclusionCull::HZBuffer::resize(const Size2i& p_size)
{
	if (p_size == Size2i()) {
		clear();
		return;
	}

	if (!sizes.is_empty() && p_size == sizes[0]) {
		return; // Size didn't change
	}

	_resize_mips(p_size);

	tile_grid_size =
		Size2i(Math::ceil(p_size.x / (float)TILE_SIZE), Math::ceil(p_size.y / (float)TILE_SIZE));
	camera_rays_tile_count = tile_grid_size.x * tile_grid_size.y;

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}

	const int alignment = 64; // Embree requires ray packets to be 64-aligned
	camera_rays_unaligned_buffer =
		(uint8_t*)memalloc(camera_rays_tile_count * sizeof(CameraRayTile) + alignment);
	camera_rays = (void*)(camera_rays_unaligned_buffer + alignment -
						  (((uint64_t)camera_rays_unaligned_buffer) % alignment));

	camera_ray_masks.resize(camera_rays_tile_count * TILE_RAYS);
	memset(camera_ray_masks.ptr(), ~0, camera_rays_tile_count * TILE_RAYS * sizeof(uint32_t));
}

void RendererSceneOcclusionCull::HZBuffer::_camera_rays_threaded(
	uint32_t p_thread, const CameraRayThreadData* p_data)
{
	uint32_t total_tiles = camera_rays_tile_count;
	uint32_t total_threads = p_data->thread_count;
	uint32_t from = p_thread * total_tiles / total_threads;
	uint32_t to = (p_thread + 1 == total_threads) ? total_tiles
												  : ((p_thread + 1) * total_tiles / total_threads);
	_generate_camera_rays(p_data, from, to);
}

void RendererSceneOcclusionCull::HZBuffer::_generate_camera_rays(
	const CameraRayThreadData* p_data, int p_from, int p_to)
{
	const Size2i& buffer_size = sizes[0];
	CameraRayTile* tiles = static_cast<CameraRayTile*>(camera_rays);

	for (int i = p_from; i < p_to; i++) {
		CameraRayTile& tile = tiles[i];
		int tile_x = (i % tile_grid_size.x) * TILE_SIZE;
		int tile_y = (i / tile_grid_size.x) * TILE_SIZE;

		for (int j = 0; j < TILE_RAYS; j++) {
			int x = tile_x + j % TILE_SIZE;
			int y = tile_y + j / TILE_SIZE;

			float u = (float(x) + 0.5f) / buffer_size.x;
			float v = (float(y) + 0.5f) / buffer_size.y;
			Vector3 pixel_pos =
				p_data->pixel_corner + u * p_data->pixel_u_interp + v * p_data->pixel_v_interp;

			tile.ray.tnear[j] = p_data->z_near;

			Vector3 dir;
			if (p_data->camera_orthogonal) {
				dir = p_data->camera_dir;
				tile.ray.org_x[j] = pixel_pos.x - dir.x * p_data->z_near;
				tile.ray.org_y[j] = pixel_pos.y - dir.y * p_data->z_near;
				tile.ray.org_z[j] = pixel_pos.z - dir.z * p_data->z_near;
			}
			else {
				dir = (pixel_pos - p_data->camera_pos).normalized();
				tile.ray.org_x[j] = p_data->camera_pos.x;
				tile.ray.org_y[j] = p_data->camera_pos.y;
				tile.ray.org_z[j] = p_data->camera_pos.z;
				tile.ray.tnear[j] /= dir.dot(p_data->camera_dir);
			}

			tile.ray.dir_x[j] = dir.x;
			tile.ray.dir_y[j] = dir.y;
			tile.ray.dir_z[j] = dir.z;

			tile.ray.tfar[j] = p_data->z_far;
			tile.ray.time[j] = 0.0f;

			tile.ray.flags[j] = 0;
			tile.ray.mask[j] = ~0U;
			tile.hit.geomID[j] = RTC_INVALID_GEOMETRY_ID;
		}
	}
}

void RendererSceneOcclusionCull::HZBuffer::sort_rays(const Vector3& p_camera_dir, bool p_orthogonal)
{
	ERR_FAIL_COND(is_empty());

	const CameraRayTile* tiles = static_cast<const CameraRayTile*>(camera_rays);
	Size2i buffer_size = sizes[0];
	for (int i = 0; i < tile_grid_size.y; i++) {
		for (int j = 0; j < tile_grid_size.x; j++) {
			for (int tile_i = 0; tile_i < TILE_SIZE; tile_i++) {
				for (int tile_j = 0; tile_j < TILE_SIZE; tile_j++) {
					int x = j * TILE_SIZE + tile_j;
					int y = i * TILE_SIZE + tile_i;
					if (x >= buffer_size.x || y >= buffer_size.y) {
						continue;
					}
					int k = tile_i * TILE_SIZE + tile_j;
					int tile_index = i * tile_grid_size.x + j;

					mips[0][y * buffer_size.x + x] = tiles[tile_index].ray.tfar[k];
				}
			}
		}
	}
}

void RendererSceneOcclusionCull::HZBuffer::update_camera_rays(
	const Transform3D&, const Vector3&, const Vector2&, real_t, bool)
{
}

RendererSceneOcclusionCull::HZBuffer::~HZBuffer()
{
	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}
}

bool RaycastOcclusionCull::is_occluder(RID p_rid)
{
	ERR_FAIL_NULL_V(data, false);
	return data->occluder_owner.owns(p_rid);
}

RID RaycastOcclusionCull::occluder_allocate()
{
	ERR_FAIL_NULL_V(data, RID());
	return data->occluder_owner.allocate_rid();
}

void RaycastOcclusionCull::occluder_initialize(RID p_occluder)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = memnew(Occluder);
	data->occluder_owner.initialize_rid(p_occluder, occluder);
}

void RaycastOcclusionCull::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->vertices = p_vertices;
	occluder->indices = p_indices;

	for (const InstanceID& E : occluder->users) {
		RID scenario_rid = E.scenario;
		RID instance_rid = E.instance;
		ERR_CONTINUE(!data->scenarios.has(scenario_rid));
		Scenario& scenario = data->scenarios[scenario_rid];
		ERR_CONTINUE(!scenario.instances.has(instance_rid));

		if (!scenario.dirty_instances.has(instance_rid)) {
			scenario.dirty_instances.insert(instance_rid);
			scenario.dirty_instances_array.push_back(instance_rid);
		}
	}
}

void RaycastOcclusionCull::free_occluder(RID p_occluder)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	memdelete(occluder);
	data->occluder_owner.free(p_occluder);
}

void RaycastOcclusionCull::add_scenario(RID p_scenario)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(data->scenarios.has(p_scenario));
	data->scenarios[p_scenario] = Scenario();
}

void RaycastOcclusionCull::remove_scenario(RID p_scenario)
{
	ERR_FAIL_NULL(data);
	Scenario* scenario = data->scenarios.getptr(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->free();
	data->scenarios.erase(p_scenario);
}

void RaycastOcclusionCull::scenario_set_instance(
	RID p_scenario, RID p_instance, RID p_occluder, const Transform3D& p_xform, bool p_enabled)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->scenarios.has(p_scenario));
	Scenario& scenario = data->scenarios[p_scenario];

	if (!scenario.instances.has(p_instance)) {
		scenario.instances[p_instance] = OccluderInstance();
	}

	OccluderInstance& instance = scenario.instances[p_instance];

	bool changed = false;

	if (instance.removed) {
		instance.removed = false;
		scenario.removed_instances.erase(p_instance);
		changed = true;
	}

	if (instance.occluder != p_occluder) {
		Occluder* old_occluder = data->occluder_owner.get_or_null(instance.occluder);
		if (old_occluder) {
			old_occluder->users.erase(InstanceID(p_scenario, p_instance));
		}

		instance.occluder = p_occluder;

		if (p_occluder.is_valid()) {
			Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
			ERR_FAIL_NULL(occluder);
			occluder->users.insert(InstanceID(p_scenario, p_instance));
		}
		changed = true;
	}

	if (instance.xform != p_xform) {
		scenario.instances[p_instance].xform = p_xform;
		changed = true;
	}

	if (instance.enabled != p_enabled) {
		instance.enabled = p_enabled;
		scenario.dirty = true;
	}

	if (changed && !scenario.dirty_instances.has(p_instance)) {
		scenario.dirty_instances.insert(p_instance);
		scenario.dirty_instances_array.push_back(p_instance);
		scenario.dirty = true;
	}
}

void RaycastOcclusionCull::scenario_remove_instance(RID p_scenario, RID p_instance)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->scenarios.has(p_scenario));
	Scenario& scenario = data->scenarios[p_scenario];

	if (scenario.instances.has(p_instance)) {
		OccluderInstance& instance = scenario.instances[p_instance];

		if (!instance.removed) {
			Occluder* occluder = data->occluder_owner.get_or_null(instance.occluder);
			if (occluder) {
				occluder->users.erase(InstanceID(p_scenario, p_instance));
			}

			scenario.removed_instances.push_back(p_instance);
			instance.removed = true;
		}
	}
}

void RaycastOcclusionCull::add_buffer(RID p_buffer)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(data->buffers.has(p_buffer));
	data->buffers[p_buffer];
}

void RaycastOcclusionCull::remove_buffer(RID p_buffer)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	data->buffers.erase(p_buffer);
}

void RaycastOcclusionCull::buffer_set_scenario(RID p_buffer, RID p_scenario)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	ERR_FAIL_COND(p_scenario.is_valid() && !data->scenarios.has(p_scenario));
	data->buffers[p_buffer].scenario_rid = p_scenario;
}

void RaycastOcclusionCull::buffer_set_size(RID p_buffer, const Vector2i& p_size)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	data->buffers[p_buffer].resize(p_size);
}

Vector2 RaycastOcclusionCull::_get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size)
{
	if (!data || !data->_jitter_enabled) {
		return Vector2();
	}

	if ((p_buffer_size.x <= 0) || (p_buffer_size.y <= 0)) {
		return Vector2();
	}

	int32_t frame = Engine::get_frames_drawn();
	frame %= 9;

	Vector2 jitter;

	switch (frame) {
	default:
		break;
	case 1: {
		jitter = Vector2(-1, -1);
	} break;
	case 2: {
		jitter = Vector2(1, -1);
	} break;
	case 3: {
		jitter = Vector2(-1, 1);
	} break;
	case 4: {
		jitter = Vector2(1, 1);
	} break;
	case 5: {
		jitter = Vector2(-0.5f, -0.5f);
	} break;
	case 6: {
		jitter = Vector2(0.5f, -0.5f);
	} break;
	case 7: {
		jitter = Vector2(-0.5f, 0.5f);
	} break;
	case 8: {
		jitter = Vector2(0.5f, 0.5f);
	} break;
	}
	Vector2 half_extents = p_viewport_rect.get_size() * 0.5;
	jitter *=
		Vector2(half_extents.x / (float)p_buffer_size.x, half_extents.y / (float)p_buffer_size.y);
	jitter *= 0.66f;

	return jitter;
}

static Rect2 _get_viewport_rect(const Projection& p_cam_projection)
{
	Size2 half_extents = p_cam_projection.get_viewport_half_extents();
	Point2 bottom_left =
		-half_extents *
		Vector2(p_cam_projection.columns[3][0] * p_cam_projection.columns[3][3] +
					p_cam_projection.columns[2][0] * p_cam_projection.columns[2][3] + 1,
			p_cam_projection.columns[3][1] * p_cam_projection.columns[3][3] +
				p_cam_projection.columns[2][1] * p_cam_projection.columns[2][3] + 1);
	return Rect2(bottom_left, 2 * half_extents);
}

void RaycastOcclusionCull::buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal)
{
	ERR_FAIL_NULL(data);
	if (!data->buffers.has(p_buffer)) {
		return;
	}

	HZBuffer& buffer = data->buffers[p_buffer];

	if (buffer.is_empty() || !data->scenarios.has(buffer.scenario_rid)) {
		return;
	}

	Scenario& scenario = data->scenarios[buffer.scenario_rid];
	scenario.update();

	Rect2 vp_rect = _get_viewport_rect(p_cam_projection);
	Vector2 bottom_left = vp_rect.position;
	bottom_left += _get_jitter(vp_rect, buffer.get_occlusion_buffer_size());
	Vector3 near_bottom_left =
		Vector3(bottom_left.x, bottom_left.y, -p_cam_projection.get_z_near());

	buffer.update_camera_rays(p_cam_transform, near_bottom_left, vp_rect.get_size(),
		p_cam_projection.get_z_far(), p_cam_orthogonal);

	scenario.raycast(static_cast<CameraRayTile*>(buffer.camera_rays),
		buffer.camera_ray_masks.ptr(), buffer.camera_rays_tile_count);
	buffer.sort_rays(-p_cam_transform.basis.get_column(2), p_cam_orthogonal);
	buffer.update_mips();
}

RaycastOcclusionCull::HZBuffer* RaycastOcclusionCull::buffer_get_ptr(RID p_buffer)
{
	ERR_FAIL_NULL_V(data, nullptr);
	if (!data->buffers.has(p_buffer)) {
		return nullptr;
	}
	return &data->buffers[p_buffer];
}

RID RaycastOcclusionCull::buffer_get_debug_texture(RID p_buffer)
{
	ERR_FAIL_NULL_V(data, RID());
	ERR_FAIL_COND_V(!data->buffers.has(p_buffer), RID());
	return data->buffers[p_buffer].get_debug_texture();
}

void RaycastOcclusionCull::set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality)
{
	ERR_FAIL_NULL(data);
	if (data->build_quality == p_quality) {
		return;
	}

	data->build_quality = p_quality;

	for (KeyValue<RID, Scenario>& K : data->scenarios) {
		K.value.dirty = true;
	}
}

void RaycastOcclusionCull::_init_embree()
{
	ERR_FAIL_NULL(data);
#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	String settings = vformat("threads=%d", MAX(1, OS::get_processor_count() - 2));
	data->ebr_device = rtcNewDevice(settings.utf8().ptr());
}

void RaycastOcclusionCull::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
	RendererSceneOcclusionCull::bind<RaycastOcclusionCull>();
}

void RaycastOcclusionCull::finalize()
{
	if (!data) {
		return;
	}

	RendererSceneOcclusionCull::reset();

	for (KeyValue<RID, Scenario>& K : data->scenarios) {
		K.value.free();
	}
	data->scenarios.clear();
	data->buffers.clear();

	if (data->ebr_device != nullptr) {
		rtcReleaseDevice(data->ebr_device);
		data->ebr_device = nullptr;
	}

	memdelete(data);
	data = nullptr;
}

#ifdef TOOLS_ENABLED




#ifdef __SSE2__
#endif

static float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
	return Math::lerp(Math::lerp(c00, c10, tx), Math::lerp(c01, c11, tx), ty);
}

struct LightmapRaycaster::EmbreeState
{
	struct AlphaTextureData
	{
		Vector<uint8_t> data;
		Vector2i size;

		uint8_t sample(float u, float v) const
		{
			float x = u * size.x;
			float y = v * size.y;
			int xi = (int)x;
			int yi = (int)y;

			uint8_t texels[4];

			for (int i = 0; i < 4; ++i) {
				int sample_x = CLAMP(xi + i % 2, 0, size.x - 1);
				int sample_y = CLAMP(yi + i / 2, 0, size.y - 1);
				texels[i] = data[sample_y * size.x + sample_x];
			}

			return Math::round(blerp(texels[0], texels[1], texels[2], texels[3], x - xi, y - yi));
		}
	};

	RTCDevice embree_device = nullptr;
	RTCScene embree_scene = nullptr;

	HashMap<unsigned int, AlphaTextureData> alpha_textures;
	HashSet<int> filter_meshes;

	static void filter_function(const struct RTCFilterFunctionNArguments* p_args)
	{
		RTCHit* hit = (RTCHit*)p_args->hit;

		unsigned int geomID = hit->geomID;
		float u = hit->u;
		float v = hit->v;

		EmbreeState* scene = (EmbreeState*)p_args->geometryUserPtr;
		RTCGeometry geom = rtcGetGeometry(scene->embree_scene, geomID);

		rtcInterpolate0(
			geom, hit->primID, hit->u, hit->v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &hit->u, 2);

		if (scene->alpha_textures.has(geomID)) {
			const AlphaTextureData& alpha_texture = scene->alpha_textures[geomID];

			if (alpha_texture.sample(hit->u, hit->v) < 128) {
				p_args->valid[0] = 0;
				return;
			}
		}

		rtcInterpolate0(
			geom, hit->primID, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &hit->Ng_x, 3);
	}
};

Ref<LightmapRaycaster> LightmapRaycaster::create()
{
	return Ref<LightmapRaycaster>(memnew(LightmapRaycaster));
}

bool LightmapRaycaster::intersect(Ray& r_ray)
{
	RTCRayQueryContext context;
	rtcInitRayQueryContext(&context);
	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.context = &context;
	rtcIntersect1(embree_state->embree_scene, (RTCRayHit*)&r_ray, &args);
	return r_ray.geomID != RTC_INVALID_GEOMETRY_ID;
}

void LightmapRaycaster::intersect(Vector<Ray>& r_rays)
{
	Ray* rays = r_rays.ptrw();
	for (int i = 0; i < r_rays.size(); ++i) {
		intersect(rays[i]);
	}
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id)
{
	if (p_alpha_texture.is_valid() && p_alpha_texture->get_size() != Vector2i()) {
		EmbreeState::AlphaTextureData tex;
		tex.size = p_alpha_texture->get_size();
		tex.data = p_alpha_texture->get_data();
		embree_state->alpha_textures.insert(p_id, tex);
	}
}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
	RTCGeometry embree_mesh =
		rtcNewGeometry(embree_state->embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	rtcSetGeometryVertexAttributeCount(embree_mesh, 2);

	int vertex_count = p_vertices.size();

	ERR_FAIL_COND(vertex_count % 3 != 0);
	ERR_FAIL_COND(vertex_count != p_uv2s.size());
	ERR_FAIL_COND(!p_normals.is_empty() && vertex_count != p_normals.size());

	Vector3* embree_vertices = (Vector3*)rtcSetNewGeometryBuffer(
		embree_mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
	memcpy(embree_vertices, p_vertices.ptr(), sizeof(Vector3) * vertex_count);

	Vector2* embree_light_uvs =
		(Vector2*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
			RTC_FORMAT_FLOAT2, sizeof(Vector2), vertex_count);
	memcpy(embree_light_uvs, p_uv2s.ptr(), sizeof(Vector2) * vertex_count);

	uint32_t* embree_triangles = (uint32_t*)rtcSetNewGeometryBuffer(embree_mesh,
		RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, vertex_count / 3);
	for (int i = 0; i < vertex_count; i++) {
		embree_triangles[i] = i;
	}

	if (!p_normals.is_empty()) {
		Vector3* embree_normals =
			(Vector3*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
				RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
		memcpy(embree_normals, p_normals.ptr(), sizeof(Vector3) * vertex_count);
	}

	rtcCommitGeometry(embree_mesh);
	rtcSetGeometryIntersectFilterFunction(embree_mesh, EmbreeState::filter_function);
	rtcSetGeometryUserData(embree_mesh, embree_state);
	rtcAttachGeometryByID(embree_state->embree_scene, embree_mesh, p_id);
	rtcReleaseGeometry(embree_mesh);
}

void LightmapRaycaster::commit() { rtcCommitScene(embree_state->embree_scene); }

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids)
{
	for (const int& E : p_mesh_ids) {
		rtcDisableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes = p_mesh_ids;
}

void LightmapRaycaster::clear_mesh_filter()
{
	for (const int& E : embree_state->filter_meshes) {
		rtcEnableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes.clear();
}

static void embree_lm_error_handler(void* p_user_data, RTCError p_code, const char* p_str)
{
	print_error("Embree error: " + String(p_str));
}

LightmapRaycaster::LightmapRaycaster()
{
	embree_state = memnew(EmbreeState);

#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	embree_state->embree_device = rtcNewDevice(nullptr);
	rtcSetDeviceErrorFunction(embree_state->embree_device, &embree_lm_error_handler, nullptr);
	embree_state->embree_scene = rtcNewScene(embree_state->embree_device);
}

LightmapRaycaster::~LightmapRaycaster()
{
	if (embree_state->embree_scene != nullptr) {
		rtcReleaseScene(embree_state->embree_scene);
	}

	if (embree_state->embree_device != nullptr) {
		rtcReleaseDevice(embree_state->embree_device);
	}

	memdelete(embree_state);
	embree_state = nullptr;
}

#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED


#ifdef __SSE2__
#endif

static RTCDevice static_embree_device = nullptr;

struct StaticRaycaster::EmbreeState
{
	RTCScene embree_scene = nullptr;
	HashSet<int> filter_meshes;
};

Ref<StaticRaycaster> StaticRaycaster::create()
{
	return Ref<StaticRaycaster>(memnew(StaticRaycaster));
}

void StaticRaycaster::free()
{
	if (static_embree_device) {
		rtcReleaseDevice(static_embree_device);
		static_embree_device = nullptr;
	}
}

bool StaticRaycaster::intersect(Ray& r_ray)
{
	RTCRayQueryContext context;
	rtcInitRayQueryContext(&context);
	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.context = &context;
	rtcIntersect1(embree_state->embree_scene, (RTCRayHit*)&r_ray, &args);
	return r_ray.geomID != RTC_INVALID_GEOMETRY_ID;
}

void StaticRaycaster::intersect(Vector<Ray>& r_rays)
{
	Ray* rays = r_rays.ptrw();
	for (int i = 0; i < r_rays.size(); ++i) {
		intersect(rays[i]);
	}
}

void StaticRaycaster::add_mesh(
	const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices, unsigned int p_id)
{
	RTCGeometry embree_mesh = rtcNewGeometry(static_embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	int vertex_count = p_vertices.size();

	Vector3* embree_vertices = (Vector3*)rtcSetNewGeometryBuffer(
		embree_mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
	memcpy(embree_vertices, p_vertices.ptr(), sizeof(Vector3) * vertex_count);

	if (p_indices.is_empty()) {
		ERR_FAIL_COND(vertex_count % 3 != 0);
		uint32_t* embree_triangles = (uint32_t*)rtcSetNewGeometryBuffer(embree_mesh,
			RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, vertex_count / 3);
		for (int i = 0; i < vertex_count; i++) {
			embree_triangles[i] = i;
		}
	}
	else {
		uint32_t* embree_triangles =
			(uint32_t*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_INDEX, 0,
				RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, p_indices.size() / 3);
		memcpy(embree_triangles, p_indices.ptr(), sizeof(uint32_t) * p_indices.size());
	}

	rtcCommitGeometry(embree_mesh);
	rtcAttachGeometryByID(embree_state->embree_scene, embree_mesh, p_id);
	rtcReleaseGeometry(embree_mesh);
}

void StaticRaycaster::commit() { rtcCommitScene(embree_state->embree_scene); }

void StaticRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids)
{
	for (const int& E : p_mesh_ids) {
		rtcDisableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes = p_mesh_ids;
}

void StaticRaycaster::clear_mesh_filter()
{
	for (const int& E : embree_state->filter_meshes) {
		rtcEnableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes.clear();
}

static void embree_error_handler(void* p_user_data, RTCError p_code, const char* p_str)
{
	print_error("Embree error: " + String(p_str));
}

StaticRaycaster::StaticRaycaster()
{
	embree_state = memnew(EmbreeState);

#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	if (!static_embree_device) {
		static_embree_device = rtcNewDevice(nullptr);
		rtcSetDeviceErrorFunction(static_embree_device, &embree_error_handler, nullptr);
	}

	embree_state->embree_scene = rtcNewScene(static_embree_device);
}

StaticRaycaster::~StaticRaycaster()
{
	if (embree_state->embree_scene != nullptr) {
		rtcReleaseScene(embree_state->embree_scene);
	}
	memdelete(embree_state);
	embree_state = nullptr;
}

#endif // TOOLS_ENABLED

void initialize_raycast_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	RaycastOcclusionCull::initialize();
}

void uninitialize_raycast_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	RaycastOcclusionCull::finalize();
#ifdef TOOLS_ENABLED
	StaticRaycaster::free();
#endif
}
#endif // defined(MODULE_RAYCAST_ENABLED)
