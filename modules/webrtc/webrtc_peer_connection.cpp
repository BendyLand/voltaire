/**************************************************************************/
/*  webrtc_peer_connection.cpp                                            */
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
// --- Inlined from modules/webrtc/webrtc_peer_connection.h ---
#include "core/io/packet_peer.h"
#include "core/templates/ring_buffer.h"
#include "scene/main/multiplayer_peer.h"

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
#include "register_types.h"

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
