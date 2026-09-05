/**************************************************************************/
/*  websocket_peer.cpp                                                    */
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

#include "websocket_peer.h"

WebSocketPeer* (*WebSocketPeer::_create)(bool p_notify_postinitialize) = nullptr;

WebSocketPeer::WebSocketPeer() {}

WebSocketPeer::~WebSocketPeer() {}

void WebSocketPeer::_bind_methods() {}

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


