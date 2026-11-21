/**************************************************************************/
/*  enet_connection.h                                                     */
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

#include <enet/enet.h>
#include "core/crypto/crypto.h"
#include "core/io/packet_peer.h"
#include "scene/main/multiplayer_peer.h"

template <typename T> class TypedArray;

class ENetConnection;

class ENetPacketPeer final : public PacketPeer
{
private:
	ENetPeer* peer = nullptr;
	List<ENetPacket*> packet_queue;
	ENetPacket* last_packet = nullptr;

	Error _send(int p_channel, PackedByteArray p_packet, int p_flags);

	friend class ENetConnection;
	// Internally used by ENetConnection during service, destroy, etc.
	void _on_disconnect();
	void _queue_packet(ENetPacket* p_packet);

public:
	enum
	{
		PACKET_THROTTLE_SCALE = ENET_PEER_PACKET_THROTTLE_SCALE,
		PACKET_LOSS_SCALE = ENET_PEER_PACKET_LOSS_SCALE,
	};

	enum
	{
		FLAG_RELIABLE = ENET_PACKET_FLAG_RELIABLE,
		FLAG_UNSEQUENCED = ENET_PACKET_FLAG_UNSEQUENCED,
		FLAG_UNRELIABLE_FRAGMENT = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
		FLAG_ALLOWED = ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_UNSEQUENCED |
					   ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
	};

	enum PeerState
	{
		STATE_DISCONNECTED = ENET_PEER_STATE_DISCONNECTED,
		STATE_CONNECTING = ENET_PEER_STATE_CONNECTING,
		STATE_ACKNOWLEDGING_CONNECT = ENET_PEER_STATE_ACKNOWLEDGING_CONNECT,
		STATE_CONNECTION_PENDING = ENET_PEER_STATE_CONNECTION_PENDING,
		STATE_CONNECTION_SUCCEEDED = ENET_PEER_STATE_CONNECTION_SUCCEEDED,
		STATE_CONNECTED = ENET_PEER_STATE_CONNECTED,
		STATE_DISCONNECT_LATER = ENET_PEER_STATE_DISCONNECT_LATER,
		STATE_DISCONNECTING = ENET_PEER_STATE_DISCONNECTING,
		STATE_ACKNOWLEDGING_DISCONNECT = ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT,
		STATE_ZOMBIE = ENET_PEER_STATE_ZOMBIE,
	};

	enum PeerStatistic
	{
		PEER_PACKET_LOSS,
		PEER_PACKET_LOSS_VARIANCE,
		PEER_PACKET_LOSS_EPOCH,
		PEER_ROUND_TRIP_TIME,
		PEER_ROUND_TRIP_TIME_VARIANCE,
		PEER_LAST_ROUND_TRIP_TIME,
		PEER_LAST_ROUND_TRIP_TIME_VARIANCE,
		PEER_PACKET_THROTTLE,
		PEER_PACKET_THROTTLE_LIMIT,
		PEER_PACKET_THROTTLE_COUNTER,
		PEER_PACKET_THROTTLE_EPOCH,
		PEER_PACKET_THROTTLE_ACCELERATION,
		PEER_PACKET_THROTTLE_DECELERATION,
		PEER_PACKET_THROTTLE_INTERVAL,
	};

	int get_max_packet_size() const override;
	int get_available_packet_count() const override;
	Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	void peer_disconnect(int p_data = 0);
	void peer_disconnect_later(int p_data = 0);
	void peer_disconnect_now(int p_data = 0);

	void ping();
	void ping_interval(int p_interval);
	void reset();
	int send(uint8_t p_channel, ENetPacket* p_packet);
	void throttle_configure(int interval, int acceleration, int deceleration);
	void set_timeout(int p_timeout, int p_timeout_min, int p_timeout_max);
	double get_statistic(PeerStatistic p_stat);
	PeerState get_state() const;
	int get_channels() const;
	int get_packet_flags() const;

	// Extras
	int get_remote_port() const;

	// Used by ENetMultiplayer (TODO use meta? If only they where Strings)
	bool is_active() const;

	ENetPacketPeer(ENetPeer* p_peer);
	~ENetPacketPeer();
};

class ENetConnection final : public RefCounted
{
public:
	enum CompressionMode
	{
		COMPRESS_NONE = 0,
		COMPRESS_RANGE_CODER,
		COMPRESS_FASTLZ,
		COMPRESS_ZLIB,
		COMPRESS_ZSTD,
	};

	enum HostStatistic
	{
		HOST_TOTAL_SENT_DATA,
		HOST_TOTAL_SENT_PACKETS,
		HOST_TOTAL_RECEIVED_DATA,
		HOST_TOTAL_RECEIVED_PACKETS,
	};

	enum EventType
	{
		EVENT_ERROR = -1,
		EVENT_NONE = 0,
		EVENT_CONNECT,
		EVENT_DISCONNECT,
		EVENT_RECEIVE,
	};

	struct Event
	{
		Ref<ENetPacketPeer> peer;
		enet_uint8 channel_id = 0;
		enet_uint32 data = 0;
		ENetPacket* packet = nullptr;
	};

private:
	ENetHost* host = nullptr;
	List<Ref<ENetPacketPeer>> peers;

	EventType _parse_event(const ENetEvent& p_event, Event& r_event);
	Error _create(ENetAddress* p_address, int p_max_peers, int p_max_channels, int p_in_bandwidth,
		int p_out_bandwidth);
	void _broadcast(int p_channel, PackedByteArray p_packet, int p_flags);

	class Compressor final
	{
	private:
		CompressionMode mode = COMPRESS_NONE;
		Vector<uint8_t> src_mem;
		Vector<uint8_t> dst_mem;
		ENetCompressor enet_compressor;

		Compressor(CompressionMode mode);

		static size_t enet_compress(void* context, const ENetBuffer* inBuffers,
			size_t inBufferCount, size_t inLimit, enet_uint8* outData, size_t outLimit);
		static size_t enet_decompress(void* context, const enet_uint8* inData, size_t inLimit,
			enet_uint8* outData, size_t outLimit);

		static void enet_compressor_destroy(void* context) { memdelete((Compressor*)context); }

	public:
		static void setup(ENetHost* p_host, CompressionMode p_mode);
	};

public:
	void broadcast(enet_uint8 p_channel, ENetPacket* p_packet);
	void socket_send(const String& p_address, int p_port, const PackedByteArray& p_packet);
	Error create_host(int p_max_peers = 32, int p_max_channels = 0, int p_in_bandwidth = 0,
		int p_out_bandwidth = 0);
	void destroy();
	Ref<ENetPacketPeer> connect_to_host(
		const String& p_address, int p_port, int p_channels, int p_data = 0);
	EventType service(int p_timeout, Event& r_event);
	int check_events(EventType& r_type, Event& r_event);
	void flush();
	void bandwidth_limit(int p_in_bandwidth = 0, int p_out_bandwidth = 0);
	void channel_limit(int p_max_channels);
	void bandwidth_throttle();
	void compress(CompressionMode p_mode);
	double pop_statistic(HostStatistic p_stat);
	int get_max_channels() const;

	// Extras
	void get_peers(List<Ref<ENetPacketPeer>>& r_peers);
	int get_local_port() const;

	// Godot additions
	Error dtls_server_setup(const Ref<TLSOptions>& p_options);
	Error dtls_client_setup(const String& p_hostname, const Ref<TLSOptions>& p_options);
	void refuse_new_connections(bool p_refuse);

	ENetConnection() {}
	~ENetConnection();
};

class ENetMultiplayerPeer final : public MultiplayerPeer
{
private:
	enum
	{
		SYSMSG_ADD_PEER,
		SYSMSG_REMOVE_PEER
	};

	enum
	{
		SYSCH_RELIABLE = 0,
		SYSCH_UNRELIABLE = 1,
		SYSCH_MAX = 2
	};

	enum Mode
	{
		MODE_NONE,
		MODE_SERVER,
		MODE_CLIENT,
		MODE_MESH,
	};

	Mode active_mode = MODE_NONE;

	uint32_t unique_id = 0;

	int target_peer = 0;

	ConnectionStatus connection_status = CONNECTION_DISCONNECTED;

	HashMap<int, Ref<ENetConnection>> hosts;
	HashMap<int, Ref<ENetPacketPeer>> peers;

	struct Packet
	{
		ENetPacket* packet = nullptr;
		int from = 0;
		int channel = 0;
		TransferMode transfer_mode = TRANSFER_MODE_RELIABLE;
	};

	List<Packet> incoming_packets;

	Packet current_packet;

	void _store_packet(int32_t p_source, ENetConnection::Event& p_event);
	void _pop_current_packet();
	void _disconnect_inactive_peers();
	void _destroy_unused(ENetPacket* p_packet);

	_FORCE_INLINE_ bool _is_active() const { return active_mode != MODE_NONE; }

public:
	virtual void set_target_peer(int p_peer) override;

	virtual int get_packet_peer() const override;
	virtual TransferMode get_packet_mode() const override;
	virtual int get_packet_channel() const override;

	virtual void poll() override;
	virtual void close() override;
	virtual void disconnect_peer(int p_peer, bool p_force = false) override;

	virtual bool is_server() const override;
	virtual bool is_server_relay_supported() const override;

	// Overridden so we can instrument the DTLSServer when needed.
	virtual void set_refuse_new_connections(bool p_enabled) override;

	virtual ConnectionStatus get_connection_status() const override;

	virtual int get_unique_id() const override;

	virtual int get_max_packet_size() const override;
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override;
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	Error create_server(int p_port, int p_max_clients = 32, int p_max_channels = 0,
		int p_in_bandwidth = 0, int p_out_bandwidth = 0);
	Error create_client(const String& p_address, int p_port, int p_channel_count = 0,
		int p_in_bandwidth = 0, int p_out_bandwidth = 0, int p_local_port = 0);
	Error create_mesh(int p_id);
	Error add_mesh_peer(int p_id, Ref<ENetConnection> p_host);

	Ref<ENetConnection> get_host() const;
	Ref<ENetPacketPeer> get_peer(int p_id) const;

	ENetMultiplayerPeer() {}
	~ENetMultiplayerPeer();
};
