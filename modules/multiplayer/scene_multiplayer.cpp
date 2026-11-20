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

#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "scene_multiplayer.h"

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
