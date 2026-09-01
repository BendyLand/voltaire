/**************************************************************************/
/*  editor_debugger_server_websocket.cpp                                  */
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

#include "../remote_debugger_peer_websocket.h"
#include "core/os/os.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "editor_debugger_server_websocket.h"

void EditorDebuggerServerWebSocket::poll()
{
	if (pending_peer.is_null() && tcp_server->is_connection_available()) {
		Ref<WebSocketPeer> peer = Ref<WebSocketPeer>(WebSocketPeer::create());
		ERR_FAIL_COND(peer.is_null()); // Bug.

		Vector<String> ws_protocols;
		ws_protocols.push_back("binary"); // Compatibility for emscripten TCP-to-WebSocket.
		peer->set_supported_protocols(ws_protocols);

		Error err = peer->accept_stream(tcp_server->take_connection());
		if (err == OK) {
			pending_timer = OS::get_singleton()->get_ticks_msec();
			pending_peer = peer;
		}
	}
	if (pending_peer.is_valid() && pending_peer->get_ready_state() != WebSocketPeer::STATE_OPEN) {
		pending_peer->poll();
		WebSocketPeer::State ready_state = pending_peer->get_ready_state();
		if (ready_state != WebSocketPeer::STATE_CONNECTING &&
			ready_state != WebSocketPeer::STATE_OPEN) {
			pending_peer.unref(); // Failed.
		}
		if (ready_state == WebSocketPeer::STATE_CONNECTING &&
			OS::get_singleton()->get_ticks_msec() - pending_timer > 3000) {
			pending_peer.unref(); // Timeout.
		}
	}
}

String EditorDebuggerServerWebSocket::get_uri() const { return endpoint; }

void EditorDebuggerServerWebSocket::stop()
{
	pending_peer.unref();
	tcp_server->stop();
}

bool EditorDebuggerServerWebSocket::is_active() const { return tcp_server->is_listening(); }

bool EditorDebuggerServerWebSocket::is_connection_available() const
{
	return pending_peer.is_valid() && pending_peer->get_ready_state() == WebSocketPeer::STATE_OPEN;
}

EditorDebuggerServerWebSocket::EditorDebuggerServerWebSocket() { tcp_server.instantiate(); }

EditorDebuggerServerWebSocket::~EditorDebuggerServerWebSocket() { stop(); }

Ref<EditorDebuggerServer> EditorDebuggerServerWebSocket::create(const String& p_protocol)
{
	ERR_FAIL_COND_V(p_protocol != "ws://", nullptr);
	return memnew(EditorDebuggerServerWebSocket);
}


