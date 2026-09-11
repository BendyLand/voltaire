/**************************************************************************/
/*  remote_debugger_peer_websocket.cpp                                    */
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

#include "core/config/project_settings.h"
#include "remote_debugger_peer_websocket.h"

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


