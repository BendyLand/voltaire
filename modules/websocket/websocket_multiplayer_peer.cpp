/**************************************************************************/
/*  websocket_multiplayer_peer.cpp                                        */
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

#include "core/io/stream_peer_tls.h"
#include "core/os/os.h"
#include "websocket_multiplayer_peer.h"

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


//
// PacketPeer
//
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

//
// MultiplayerPeer
//
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
	pending.time = OS::get_singleton()->get_ticks_msec();
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


