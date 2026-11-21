/**************************************************************************/
/*  websocket_peer.h                                                      */
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

#pragma once

#include "core/crypto/crypto.h"
#include "core/io/ip_address.h"
#include "core/io/packet_peer.h"
#include "core/io/socket_server.h"
#include "core/templates/list.h"
#include "core/templates/ring_buffer.h"
#include "scene/main/multiplayer_peer.h"

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
