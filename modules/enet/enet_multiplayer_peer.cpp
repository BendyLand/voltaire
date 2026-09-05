/**************************************************************************/
/*  enet_multiplayer_peer.cpp                                             */
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

#include "enet_multiplayer_peer.h"

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


