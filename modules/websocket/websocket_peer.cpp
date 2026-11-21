/**************************************************************************/
/*  websocket_peer.cpp                                                    */
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

#include "websocket_peer.h"
#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/ip.h"
#include "core/io/stream_peer_tls.h"
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
