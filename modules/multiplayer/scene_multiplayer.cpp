/**************************************************************************/
/*  scene_multiplayer.cpp                                                 */
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

#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "scene_multiplayer.h"

void SceneMultiplayer::set_root_path(const NodePath& p_path)
{
	ERR_FAIL_COND_MSG(!p_path.is_absolute() && !p_path.is_empty(),
		"SceneMultiplayer root path must be absolute.");
	root_path = p_path;
}

NodePath SceneMultiplayer::get_root_path() const { return root_path; }

Ref<MultiplayerPeer> SceneMultiplayer::get_multiplayer_peer() { return multiplayer_peer; }

#ifdef DEBUG_ENABLED
_FORCE_INLINE_ Error SceneMultiplayer::_send(const uint8_t* p_packet, int p_packet_len)
{
	return multiplayer_peer->put_packet(p_packet, p_packet_len);
}
#endif

Error SceneMultiplayer::send_command(int p_to, const uint8_t* p_packet, int p_packet_len)
{
	if (server_relay && get_unique_id() != 1 && p_to != 1 &&
		multiplayer_peer->is_server_relay_supported()) {
		// Send relay packet.
		relay_buffer->seek(0);
		relay_buffer->put_u8(NETWORK_COMMAND_SYS);
		relay_buffer->put_u8(SYS_COMMAND_RELAY);
		relay_buffer->put_32(p_to); // Set the destination.
		relay_buffer->put_data(p_packet, p_packet_len);
		multiplayer_peer->set_target_peer(1);
		const Vector<uint8_t> data = relay_buffer->get_data_array();
		return _send(data.ptr(), relay_buffer->get_position());
	}
	if (p_to > 0) {
		ERR_FAIL_COND_V(!connected_peers.has(p_to), ERR_BUG);
		multiplayer_peer->set_target_peer(p_to);
		return _send(p_packet, p_packet_len);
	}
	else {
		for (const int& pid : connected_peers) {
			if (p_to && pid == -p_to) {
				continue;
			}
			multiplayer_peer->set_target_peer(pid);
			_send(p_packet, p_packet_len);
		}
		return OK;
	}
}

void SceneMultiplayer::_process_sys(int p_from, const uint8_t* p_packet, int p_packet_len,
	MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	ERR_FAIL_COND_MSG(p_packet_len < SYS_CMD_SIZE, "Invalid packet received. Size too small.");
	uint8_t sys_cmd_type = p_packet[1];
	int32_t peer = int32_t(decode_uint32(&p_packet[2]));
	switch (sys_cmd_type) {
	case SYS_COMMAND_ADD_PEER: {
		ERR_FAIL_COND(!server_relay || !multiplayer_peer->is_server_relay_supported() ||
					  get_unique_id() == 1 || p_from != 1);
		_admit_peer(peer); // Relayed peers are automatically accepted.
	} break;
	case SYS_COMMAND_DEL_PEER: {
		ERR_FAIL_COND(!server_relay || !multiplayer_peer->is_server_relay_supported() ||
					  get_unique_id() == 1 || p_from != 1);
		_del_peer(peer);
	} break;
	case SYS_COMMAND_RELAY: {
		ERR_FAIL_COND(!server_relay || !multiplayer_peer->is_server_relay_supported());
		ERR_FAIL_COND(p_packet_len < SYS_CMD_SIZE + 1);
		const uint8_t* packet = p_packet + SYS_CMD_SIZE;
		int len = p_packet_len - SYS_CMD_SIZE;
		bool should_process = false;
		if (get_unique_id() == 1) { // I am the server.
			// The requested target might have disconnected while the packet was in transit.
			if (unlikely(peer > 0 && !connected_peers.has(peer))) {
				return;
			}
			// Send relay packet.
			relay_buffer->seek(0);
			relay_buffer->put_u8(NETWORK_COMMAND_SYS);
			relay_buffer->put_u8(SYS_COMMAND_RELAY);
			relay_buffer->put_32(p_from); // Set the source.
			relay_buffer->put_data(packet, len);
			const Vector<uint8_t> data = relay_buffer->get_data_array();
			multiplayer_peer->set_transfer_mode(p_mode);
			multiplayer_peer->set_transfer_channel(p_channel);
			if (peer > 0) {
				// Single destination.
				multiplayer_peer->set_target_peer(peer);
				_send(data.ptr(), relay_buffer->get_position());
			}
			else {
				// Multiple destinations.
				for (const int& P : connected_peers) {
					// Not to sender, nor excluded.
					if (P == p_from || P == -peer) {
						continue;
					}
					multiplayer_peer->set_target_peer(P);
					_send(data.ptr(), relay_buffer->get_position());
				}
				if (peer != -1) {
					// The server is one of the targets, process the packet with sender as source.
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
			remote_sender_id = peer;
			_process_packet(peer, packet, len);
			remote_sender_id = 0;
		}
	} break;
	default: {
		ERR_FAIL();
	}
	}
}

Error SceneMultiplayer::send_bytes(
	Vector<uint8_t> p_data, int p_to, MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	ERR_FAIL_COND_V_MSG(p_data.is_empty(), ERR_INVALID_DATA, "Trying to send an empty raw packet.");
	ERR_FAIL_COND_V_MSG(multiplayer_peer.is_null(), ERR_UNCONFIGURED,
		"Trying to send a raw packet while no multiplayer peer is active.");
	ERR_FAIL_COND_V_MSG(
		multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED,
		"Trying to send a raw packet via a multiplayer peer which is not connected.");

	if (packet_cache.size() < p_data.size() + 1) {
		packet_cache.resize(p_data.size() + 1);
	}

	const uint8_t* r = p_data.ptr();
	packet_cache.write[0] = NETWORK_COMMAND_RAW;
	memcpy(&packet_cache.write[1], &r[0], p_data.size());

	multiplayer_peer->set_transfer_channel(p_channel);
	multiplayer_peer->set_transfer_mode(p_mode);
	return send_command(p_to, packet_cache.ptr(), p_data.size() + 1);
}

Error SceneMultiplayer::send_auth(int p_to, Vector<uint8_t> p_data)
{
	ERR_FAIL_COND_V(multiplayer_peer.is_null() || multiplayer_peer->get_connection_status() !=
													  MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!pending_peers.has(p_to), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_data.is_empty(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(pending_peers[p_to].local, ERR_FILE_CANT_WRITE,
		"The authentication session was previously marked as completed, no more authentication "
		"data can be sent.");
	ERR_FAIL_COND_V_MSG(pending_peers[p_to].remote, ERR_FILE_CANT_WRITE,
		"The remote peer notified that the authentication session was completed, no more "
		"authentication data can be sent.");

	if (packet_cache.size() < p_data.size() + 2) {
		packet_cache.resize(p_data.size() + 2);
	}

	packet_cache.write[0] = NETWORK_COMMAND_SYS;
	packet_cache.write[1] = SYS_COMMAND_AUTH;
	memcpy(&packet_cache.write[2], p_data.ptr(), p_data.size());

	multiplayer_peer->set_target_peer(p_to);
	multiplayer_peer->set_transfer_channel(0);
	multiplayer_peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	return _send(packet_cache.ptr(), p_data.size() + 2);
}

Error SceneMultiplayer::complete_auth(int p_peer)
{
	ERR_FAIL_COND_V(multiplayer_peer.is_null() || multiplayer_peer->get_connection_status() !=
													  MultiplayerPeer::CONNECTION_CONNECTED,
		ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!pending_peers.has(p_peer), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(pending_peers[p_peer].local, ERR_FILE_CANT_WRITE,
		"The authentication session was already marked as completed.");
	pending_peers[p_peer].local = true;

	// Notify the remote peer that the authentication has completed.
	uint8_t buf[2] = {NETWORK_COMMAND_SYS, SYS_COMMAND_AUTH};
	multiplayer_peer->set_target_peer(p_peer);
	multiplayer_peer->set_transfer_channel(0);
	multiplayer_peer->set_transfer_mode(MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	Error err = _send(buf, 2);

	// The remote peer already reported the authentication as completed, so admit the peer.
	// May generate new packets, so it must happen after sending confirmation.
	if (pending_peers[p_peer].remote) {
		pending_peers.erase(p_peer);
		_admit_peer(p_peer);
	}
	return err;
}

void SceneMultiplayer::set_auth_timeout(double p_timeout)
{
	ERR_FAIL_COND_MSG(
		p_timeout < 0, "Timeout must be greater or equal to 0 (where 0 means no timeout)");
	auth_timeout = uint64_t(p_timeout * 1000);
}

double SceneMultiplayer::get_auth_timeout() const { return double(auth_timeout) / 1000.0; }

int SceneMultiplayer::get_unique_id()
{
	ERR_FAIL_COND_V_MSG(
		multiplayer_peer.is_null(), 0, "No multiplayer peer is assigned. Unable to get unique ID.");
	return multiplayer_peer->get_unique_id();
}

void SceneMultiplayer::set_refuse_new_connections(bool p_refuse)
{
	ERR_FAIL_COND_MSG(multiplayer_peer.is_null(),
		"No multiplayer peer is assigned. Unable to set 'refuse_new_connections'.");
	multiplayer_peer->set_refuse_new_connections(p_refuse);
}

bool SceneMultiplayer::is_refusing_new_connections() const
{
	ERR_FAIL_COND_V_MSG(multiplayer_peer.is_null(), false,
		"No multiplayer peer is assigned. Unable to get 'refuse_new_connections'.");
	return multiplayer_peer->is_refusing_new_connections();
}

Vector<int> SceneMultiplayer::get_peer_ids()
{
	ERR_FAIL_COND_V_MSG(multiplayer_peer.is_null(), Vector<int>(),
		"No multiplayer peer is assigned. Assume no peers are connected.");

	Vector<int> ret;
	for (const int& E : connected_peers) {
		ret.push_back(E);
	}

	return ret;
}

Vector<int> SceneMultiplayer::get_authenticating_peer_ids()
{
	Vector<int> out;
	out.resize(pending_peers.size());
	int idx = 0;
	for (const KeyValue<int, PendingPeer>& E : pending_peers) {
		out.write[idx++] = E.key;
	}
	return out;
}

void SceneMultiplayer::set_allow_object_decoding(bool p_enable)
{
	allow_object_decoding = p_enable;
}

bool SceneMultiplayer::is_object_decoding_allowed() const { return allow_object_decoding; }

void SceneMultiplayer::set_server_relay_enabled(bool p_enabled) { server_relay = p_enabled; }

bool SceneMultiplayer::is_server_relay_enabled() const { return server_relay; }

void SceneMultiplayer::set_max_sync_packet_size(int p_size)
{
	replicator->set_max_sync_packet_size(p_size);
}

int SceneMultiplayer::get_max_sync_packet_size() const
{
	return replicator->get_max_sync_packet_size();
}

void SceneMultiplayer::set_max_delta_packet_size(int p_size)
{
	replicator->set_max_delta_packet_size(p_size);
}

int SceneMultiplayer::get_max_delta_packet_size() const
{
	return replicator->get_max_delta_packet_size();
}

void SceneMultiplayer::_bind_methods() {}

SceneMultiplayer::SceneMultiplayer()
{
	relay_buffer.instantiate();
	cache.instantiate(this);
	replicator.instantiate(this, cache.ptr());
	rpc.instantiate(this, cache.ptr(), replicator.ptr());
	set_multiplayer_peer(Ref<OfflineMultiplayerPeer>(memnew(OfflineMultiplayerPeer)));
}

SceneMultiplayer::~SceneMultiplayer()
{
	clear();
	// Ensure unref in reverse order for safety (we shouldn't use those pointers in the
	// deconstructors anyway).
	rpc.unref();
	replicator.unref();
	cache.unref();
}


