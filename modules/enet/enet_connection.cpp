/**************************************************************************/
/*  enet_connection.cpp                                                   */
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

#include "enet_connection.h"
#include "register_types.h"

#include "core/error/error_macros.h"
#include "core/io/compression.h"
#include "core/io/ip.h"

// ENetPacketPeer

void ENetPacketPeer::peer_disconnect(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect(peer, p_data);
}

void ENetPacketPeer::peer_disconnect_later(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect_later(peer, p_data);
}

void ENetPacketPeer::peer_disconnect_now(int p_data)
{
	ERR_FAIL_NULL(peer);
	enet_peer_disconnect_now(peer, p_data);
	_on_disconnect();
}

void ENetPacketPeer::ping()
{
	ERR_FAIL_NULL(peer);
	enet_peer_ping(peer);
}

void ENetPacketPeer::ping_interval(int p_interval)
{
	ERR_FAIL_NULL(peer);
	enet_peer_ping_interval(peer, p_interval);
}

int ENetPacketPeer::send(uint8_t p_channel, ENetPacket* p_packet)
{
	ERR_FAIL_NULL_V(peer, -1);
	ERR_FAIL_NULL_V(p_packet, -1);
	ERR_FAIL_COND_V_MSG(p_channel >= peer->channelCount, -1,
		vformat("Unable to send packet on channel %d, max channels: %d", p_channel,
			(int)peer->channelCount));
	return enet_peer_send(peer, p_channel, p_packet);
}

void ENetPacketPeer::reset()
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	enet_peer_reset(peer);
	_on_disconnect();
}

void ENetPacketPeer::throttle_configure(int p_interval, int p_acceleration, int p_deceleration)
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	enet_peer_throttle_configure(peer, p_interval, p_acceleration, p_deceleration);
}

void ENetPacketPeer::set_timeout(int p_timeout, int p_timeout_min, int p_timeout_max)
{
	ERR_FAIL_NULL_MSG(peer, "Peer not connected.");
	ERR_FAIL_COND_MSG(p_timeout > p_timeout_min || p_timeout_min > p_timeout_max,
		"Timeout limit must be less than minimum timeout, which itself must be less than maximum "
		"timeout");
	enet_peer_timeout(peer, p_timeout, p_timeout_min, p_timeout_max);
}

int ENetPacketPeer::get_max_packet_size() const { return 1 << 24; }

int ENetPacketPeer::get_available_packet_count() const { return packet_queue.size(); }

Error ENetPacketPeer::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_NULL_V(peer, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(packet_queue.is_empty(), ERR_UNAVAILABLE);
	if (last_packet) {
		enet_packet_destroy(last_packet);
		last_packet = nullptr;
	}
	last_packet = packet_queue.front()->get();
	packet_queue.pop_front();
	*r_buffer = (const uint8_t*)(last_packet->data);
	r_buffer_size = last_packet->dataLength;
	return OK;
}

Error ENetPacketPeer::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_NULL_V(peer, ERR_UNCONFIGURED);
	ENetPacket* packet = enet_packet_create(p_buffer, p_buffer_size, ENET_PACKET_FLAG_RELIABLE);
	return send(0, packet) < 0 ? FAILED : OK;
}

int ENetPacketPeer::get_remote_port() const
{
	ERR_FAIL_NULL_V(peer, 0);
	return peer->address.port;
}

bool ENetPacketPeer::is_active() const { return peer != nullptr; }

double ENetPacketPeer::get_statistic(PeerStatistic p_stat)
{
	ERR_FAIL_NULL_V(peer, 0);
	switch (p_stat) {
	case PEER_PACKET_LOSS:
		return peer->packetLoss;
	case PEER_PACKET_LOSS_VARIANCE:
		return peer->packetLossVariance;
	case PEER_PACKET_LOSS_EPOCH:
		return peer->packetLossEpoch;
	case PEER_ROUND_TRIP_TIME:
		return peer->roundTripTime;
	case PEER_ROUND_TRIP_TIME_VARIANCE:
		return peer->roundTripTimeVariance;
	case PEER_LAST_ROUND_TRIP_TIME:
		return peer->lastRoundTripTime;
	case PEER_LAST_ROUND_TRIP_TIME_VARIANCE:
		return peer->lastRoundTripTimeVariance;
	case PEER_PACKET_THROTTLE:
		return peer->packetThrottle;
	case PEER_PACKET_THROTTLE_LIMIT:
		return peer->packetThrottleLimit;
	case PEER_PACKET_THROTTLE_COUNTER:
		return peer->packetThrottleCounter;
	case PEER_PACKET_THROTTLE_EPOCH:
		return peer->packetThrottleEpoch;
	case PEER_PACKET_THROTTLE_ACCELERATION:
		return peer->packetThrottleAcceleration;
	case PEER_PACKET_THROTTLE_DECELERATION:
		return peer->packetThrottleDeceleration;
	case PEER_PACKET_THROTTLE_INTERVAL:
		return peer->packetThrottleInterval;
	}
	ERR_FAIL_V(0);
}

ENetPacketPeer::PeerState ENetPacketPeer::get_state() const
{
	if (!is_active()) {
		return STATE_DISCONNECTED;
	}
	return (PeerState)peer->state;
}

int ENetPacketPeer::get_channels() const
{
	ERR_FAIL_NULL_V_MSG(peer, 0, "The ENetConnection instance isn't currently active.");
	return peer->channelCount;
}

int ENetPacketPeer::get_packet_flags() const
{
	ERR_FAIL_COND_V(packet_queue.is_empty(), 0);
	return packet_queue.front()->get()->flags;
}

void ENetPacketPeer::_on_disconnect()
{
	if (peer) {
#ifdef GODOT_ENET
		enet_peer_socket_destroy(peer);
#endif
		peer->data = nullptr;
	}
	peer = nullptr;
}

void ENetPacketPeer::_queue_packet(ENetPacket* p_packet)
{
	ERR_FAIL_NULL(peer);
	packet_queue.push_back(p_packet);
}

Error ENetPacketPeer::_send(int p_channel, PackedByteArray p_packet, int p_flags)
{
	ERR_FAIL_NULL_V_MSG(peer, ERR_UNCONFIGURED, "Peer not connected.");
	ERR_FAIL_COND_V_MSG(p_channel < 0 || p_channel > (int)peer->channelCount, ERR_INVALID_PARAMETER,
		"Invalid channel");
	ERR_FAIL_COND_V_MSG(p_flags & ~FLAG_ALLOWED, ERR_INVALID_PARAMETER, "Invalid flags");
	ENetPacket* packet = enet_packet_create(p_packet.ptr(), p_packet.size(), p_flags);
	return send(p_channel, packet) == 0 ? OK : FAILED;
}

ENetPacketPeer::ENetPacketPeer(ENetPeer* p_peer)
{
	peer = p_peer;
	peer->data = this;
#ifdef GODOT_ENET
	enet_peer_socket_bind(peer);
#endif
}

ENetPacketPeer::~ENetPacketPeer()
{
	_on_disconnect();
	if (last_packet) {
		enet_packet_destroy(last_packet);
		last_packet = nullptr;
	}
	for (List<ENetPacket*>::Element* E = packet_queue.front(); E; E = E->next()) {
		enet_packet_destroy(E->get());
	}
	packet_queue.clear();
}

// ENetConnection

void ENetConnection::broadcast(enet_uint8 p_channel, ENetPacket* p_packet)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(p_channel >= host->channelLimit,
		vformat("Unable to send packet on channel %d, max channels: %d", p_channel,
			(int)host->channelLimit));
	enet_host_broadcast(host, p_channel, p_packet);
}

Error ENetConnection::create_host(
	int p_max_peers, int p_max_channels, int p_in_bandwidth, int p_out_bandwidth)
{
	return _create(nullptr, p_max_peers, p_max_channels, p_in_bandwidth, p_out_bandwidth);
}

void ENetConnection::destroy()
{
	ERR_FAIL_NULL_MSG(host, "Host already destroyed.");
	for (const Ref<ENetPacketPeer>& peer : peers) {
		peer->_on_disconnect();
	}
	peers.clear();
	enet_host_destroy(host);
	host = nullptr;
}

Ref<ENetPacketPeer> ENetConnection::connect_to_host(
	const String& p_address, int p_port, int p_channels, int p_data)
{
	Ref<ENetPacketPeer> out;
	ERR_FAIL_NULL_V_MSG(host, out, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V_MSG(peers.size(), out, "The ENetConnection is already connected to a peer.");
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, out,
		"The remote port number must be between 1 and 65535 (inclusive).");

	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
#ifdef GODOT_ENET
		ip = IP::resolve_hostname(p_address);
#else
		ip = IP::resolve_hostname(p_address, IP::TYPE_IPV4);
#endif
		ERR_FAIL_COND_V_MSG(
			!ip.is_valid(), out, "Couldn't resolve the server IP address or domain name.");
	}

	ENetAddress address;
#ifdef GODOT_ENET
	enet_address_set_ip(&address, ip.get_ipv6(), 16);
#else
	ERR_FAIL_COND_V_MSG(!ip.is_ipv4(), out,
		"Connecting to an IPv6 server isn't supported when using vanilla ENet. Recompile Godot "
		"with the bundled ENet library.");
	address.host = *(uint32_t*)ip.get_ipv4();
#endif
	address.port = p_port;

	// Initiate connection, allocating enough channels
	ENetPeer* peer = enet_host_connect(
		host, &address, p_channels > 0 ? p_channels : ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, p_data);

	if (peer == nullptr) {
		return nullptr;
	}
	out.instantiate(peer);
	peers.push_back(out);
	return out;
}

ENetConnection::EventType ENetConnection::_parse_event(const ENetEvent& p_event, Event& r_event)
{
	switch (p_event.type) {
	case ENET_EVENT_TYPE_CONNECT: {
		if (p_event.peer->data == nullptr) {
			Ref<ENetPacketPeer> pp = memnew(ENetPacketPeer(p_event.peer));
			peers.push_back(pp);
		}
		r_event.peer = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
		r_event.data = p_event.data;
		return EVENT_CONNECT;
	} break;
	case ENET_EVENT_TYPE_DISCONNECT: {
		// A peer disconnected.
		if (p_event.peer->data != nullptr) {
			Ref<ENetPacketPeer> pp = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
			pp->_on_disconnect();
			peers.erase(pp);
			r_event.peer = pp;
			r_event.data = p_event.data;
			return EVENT_DISCONNECT;
		}
		return EVENT_ERROR;
	} break;
	case ENET_EVENT_TYPE_RECEIVE: {
		// Packet received.
		if (p_event.peer->data != nullptr) {
			r_event.peer = Ref<ENetPacketPeer>((ENetPacketPeer*)p_event.peer->data);
			r_event.channel_id = p_event.channelID;
			r_event.packet = p_event.packet;
			return EVENT_RECEIVE;
		}
		return EVENT_ERROR;
	} break;
	case ENET_EVENT_TYPE_NONE:
		return EVENT_NONE;
	default:
		return EVENT_NONE;
	}
}

ENetConnection::EventType ENetConnection::service(int p_timeout, Event& r_event)
{
	ERR_FAIL_NULL_V_MSG(host, EVENT_ERROR, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(r_event.peer.is_valid(), EVENT_ERROR);

	// Drop peers that have already been disconnected.
	// NOTE: Forcibly disconnected peers (i.e. peers disconnected via
	// enet_peer_disconnect*) do not trigger DISCONNECTED events.
	List<Ref<ENetPacketPeer>>::Element* E = peers.front();
	while (E) {
		if (!E->get()->is_active()) {
			peers.erase(E->get());
		}
		E = E->next();
	}

	ENetEvent event;
	int ret = enet_host_service(host, &event, p_timeout);

	if (ret < 0) {
		return EVENT_ERROR;
	}
	else if (ret == 0) {
		return EVENT_NONE;
	}
	return _parse_event(event, r_event);
}

int ENetConnection::check_events(EventType& r_type, Event& r_event)
{
	ERR_FAIL_NULL_V_MSG(host, -1, "The ENetConnection instance isn't currently active.");
	ENetEvent event;
	int ret = enet_host_check_events(host, &event);
	if (ret < 0) {
		r_type = EVENT_ERROR;
		return ret;
	}
	r_type = _parse_event(event, r_event);
	return ret;
}

void ENetConnection::flush()
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_flush(host);
}

void ENetConnection::bandwidth_limit(int p_in_bandwidth, int p_out_bandwidth)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_bandwidth_limit(host, p_in_bandwidth, p_out_bandwidth);
}

void ENetConnection::channel_limit(int p_max_channels)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_channel_limit(host, p_max_channels);
}

void ENetConnection::bandwidth_throttle()
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_bandwidth_throttle(host);
}

void ENetConnection::compress(CompressionMode p_mode)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	Compressor::setup(host, p_mode);
}

double ENetConnection::pop_statistic(HostStatistic p_stat)
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	uint32_t* ptr = nullptr;
	switch (p_stat) {
	case HOST_TOTAL_SENT_DATA:
		ptr = &(host->totalSentData);
		break;
	case HOST_TOTAL_SENT_PACKETS:
		ptr = &(host->totalSentPackets);
		break;
	case HOST_TOTAL_RECEIVED_DATA:
		ptr = &(host->totalReceivedData);
		break;
	case HOST_TOTAL_RECEIVED_PACKETS:
		ptr = &(host->totalReceivedPackets);
		break;
	}
	ERR_FAIL_NULL_V_MSG(ptr, 0, "Invalid statistic: " + itos(p_stat) + ".");
	uint32_t ret = *ptr;
	*ptr = 0;
	return ret;
}

int ENetConnection::get_max_channels() const
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	return host->channelLimit;
}

int ENetConnection::get_local_port() const
{
	ERR_FAIL_NULL_V_MSG(host, 0, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V_MSG(!(host->socket), 0, "The ENetConnection instance isn't currently bound.");
	ENetAddress address;
	ERR_FAIL_COND_V_MSG(
		enet_socket_get_address(host->socket, &address), 0, "Unable to get socket address");
	return address.port;
}

void ENetConnection::get_peers(List<Ref<ENetPacketPeer>>& r_peers)
{
	for (const Ref<ENetPacketPeer>& I : peers) {
		r_peers.push_back(I);
	}
}

Error ENetConnection::dtls_server_setup(const Ref<TLSOptions>& p_options)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_V_MSG(
		host, ERR_UNCONFIGURED, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);
	return enet_host_dtls_server_setup(host, p_options.ptr()) ? FAILED : OK;
#else
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "ENet DTLS support not available in this build.");
#endif
}

void ENetConnection::refuse_new_connections(bool p_refuse)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	enet_host_refuse_new_connections(host, p_refuse);
#else
	ERR_FAIL_MSG("ENet DTLS support not available in this build.");
#endif
}

Error ENetConnection::dtls_client_setup(const String& p_hostname, const Ref<TLSOptions>& p_options)
{
#ifdef GODOT_ENET
	ERR_FAIL_NULL_V_MSG(
		host, ERR_UNCONFIGURED, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_V(p_options.is_null() || p_options->is_server(), ERR_INVALID_PARAMETER);
	return enet_host_dtls_client_setup(host, p_hostname.utf8().get_data(), p_options.ptr()) ? FAILED
																							: OK;
#else
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "ENet DTLS support not available in this build.");
#endif
}

Error ENetConnection::_create(ENetAddress* p_address, int p_max_peers, int p_max_channels,
	int p_in_bandwidth, int p_out_bandwidth)
{
	ERR_FAIL_COND_V_MSG(
		host != nullptr, ERR_ALREADY_IN_USE, "The ENetConnection instance is already active.");
	ERR_FAIL_COND_V_MSG(p_max_peers < 1 || p_max_peers > 4095, ERR_INVALID_PARAMETER,
		"The number of clients must be set between 1 and 4095 (inclusive).");
	ERR_FAIL_COND_V_MSG(p_max_channels < 0 || p_max_channels > ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT,
		ERR_INVALID_PARAMETER,
		"Invalid channel count. Must be between 0 and 255 (0 means maximum, i.e. 255)");
	ERR_FAIL_COND_V_MSG(p_in_bandwidth < 0, ERR_INVALID_PARAMETER,
		"The incoming bandwidth limit must be greater than or equal to 0 (0 disables the limit).");
	ERR_FAIL_COND_V_MSG(p_out_bandwidth < 0, ERR_INVALID_PARAMETER,
		"The outgoing bandwidth limit must be greater than or equal to 0 (0 disables the limit).");

	host = enet_host_create(p_address /* the address to bind the server host to */,
		p_max_peers /* allow up to p_max_peers connections */,
		p_max_channels /* allow up to p_max_channel to be used */,
		p_in_bandwidth /* limit incoming bandwidth if > 0 */,
		p_out_bandwidth /* limit outgoing bandwidth if > 0 */);

	ERR_FAIL_NULL_V_MSG(host, ERR_CANT_CREATE, "Couldn't create an ENet host.");
	return OK;
}

void ENetConnection::_broadcast(int p_channel, PackedByteArray p_packet, int p_flags)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(p_channel < 0 || p_channel > (int)host->channelLimit, "Invalid channel");
	ERR_FAIL_COND_MSG(p_flags & ~ENetPacketPeer::FLAG_ALLOWED, "Invalid flags");
	ENetPacket* pkt = enet_packet_create(p_packet.ptr(), p_packet.size(), p_flags);
	broadcast(p_channel, pkt);
}

void ENetConnection::socket_send(
	const String& p_address, int p_port, const PackedByteArray& p_packet)
{
	ERR_FAIL_NULL_MSG(host, "The ENetConnection instance isn't currently active.");
	ERR_FAIL_COND_MSG(!(host->socket), "The ENetConnection instance isn't currently bound.");
	ERR_FAIL_COND_MSG(p_port < 1 || p_port > 65535,
		"The remote port number must be between 1 and 65535 (inclusive).");

	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
#ifdef GODOT_ENET
		ip = IP::resolve_hostname(p_address);
#else
		ip = IP::resolve_hostname(p_address, IP::TYPE_IPV4);
#endif
		ERR_FAIL_COND_MSG(!ip.is_valid(), "Couldn't resolve the server IP address or domain name.");
	}

	ENetAddress address;
#ifdef GODOT_ENET
	enet_address_set_ip(&address, ip.get_ipv6(), 16);
#else
	ERR_FAIL_COND_MSG(!ip.is_ipv4(),
		"Connecting to an IPv6 server isn't supported when using vanilla ENet. "
		"Recompile Godot with the bundled ENet library.");
	address.host = *(uint32_t*)ip.get_ipv4();
#endif
	address.port = p_port;

	ENetBuffer enet_buffers[1];
	enet_buffers[0].data = (void*)p_packet.ptr();
	enet_buffers[0].dataLength = p_packet.size();

	enet_socket_send(host->socket, &address, enet_buffers, 1);
}

ENetConnection::~ENetConnection()
{
	if (host) {
		destroy();
	}
}

size_t ENetConnection::Compressor::enet_compress(void* context, const ENetBuffer* inBuffers,
	size_t inBufferCount, size_t inLimit, enet_uint8* outData, size_t outLimit)
{
	Compressor* compressor = (Compressor*)(context);

	if (size_t(compressor->src_mem.size()) < inLimit) {
		compressor->src_mem.resize(inLimit);
	}

	size_t total = inLimit;
	size_t ofs = 0;
	while (total) {
		for (size_t i = 0; i < inBufferCount; i++) {
			const size_t to_copy = MIN(total, inBuffers[i].dataLength);
			memcpy(&compressor->src_mem.write[ofs], inBuffers[i].data, to_copy);
			ofs += to_copy;
			total -= to_copy;
		}
	}

	Compression::Mode mode;

	switch (compressor->mode) {
	case COMPRESS_FASTLZ: {
		mode = Compression::MODE_FASTLZ;
	} break;
	case COMPRESS_ZLIB: {
		mode = Compression::MODE_DEFLATE;
	} break;
	case COMPRESS_ZSTD: {
		mode = Compression::MODE_ZSTD;
	} break;
	default: {
		ERR_FAIL_V_MSG(0, vformat("Invalid ENet compression mode: %d", compressor->mode));
	}
	}

	const int64_t req_size = Compression::get_max_compressed_buffer_size(ofs, mode);
	if (compressor->dst_mem.size() < req_size) {
		compressor->dst_mem.resize(req_size);
	}
	const int64_t ret =
		Compression::compress(compressor->dst_mem.ptrw(), compressor->src_mem.ptr(), ofs, mode);

	if (ret < 0) {
		return 0;
	}

	const size_t ret_size = size_t(ret);
	if (ret_size > outLimit) {
		return 0; // Do not bother
	}

	memcpy(outData, compressor->dst_mem.ptr(), ret_size);

	return ret;
}

size_t ENetConnection::Compressor::enet_decompress(
	void* context, const enet_uint8* inData, size_t inLimit, enet_uint8* outData, size_t outLimit)
{
	Compressor* compressor = (Compressor*)(context);
	int64_t ret = -1;
	switch (compressor->mode) {
	case COMPRESS_FASTLZ: {
		ret = Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_FASTLZ);
	} break;
	case COMPRESS_ZLIB: {
		ret =
			Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_DEFLATE);
	} break;
	case COMPRESS_ZSTD: {
		ret = Compression::decompress(outData, outLimit, inData, inLimit, Compression::MODE_ZSTD);
	} break;
	default: {
	}
	}
	if (ret < 0) {
		return 0;
	}
	else {
		return ret;
	}
}

void ENetConnection::Compressor::setup(ENetHost* p_host, CompressionMode p_mode)
{
	ERR_FAIL_NULL(p_host);
	switch (p_mode) {
	case COMPRESS_NONE: {
		enet_host_compress(p_host, nullptr);
	} break;
	case COMPRESS_RANGE_CODER: {
		enet_host_compress_with_range_coder(p_host);
	} break;
	case COMPRESS_FASTLZ:
	case COMPRESS_ZLIB:
	case COMPRESS_ZSTD: {
		Compressor* compressor = memnew(Compressor(p_mode));
		enet_host_compress(p_host, &(compressor->enet_compressor));
	} break;
	}
}

ENetConnection::Compressor::Compressor(CompressionMode p_mode)
{
	mode = p_mode;
	enet_compressor.context = this;
	enet_compressor.compress = enet_compress;
	enet_compressor.decompress = enet_decompress;
	enet_compressor.destroy = enet_compressor_destroy;
}

// ENetMultiplayerPeer

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

void ENetMultiplayerPeer::poll() {}

// Module registration

static bool enet_ok = false;

void initialize_enet_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (enet_initialize() != 0) {
		ERR_PRINT("ENet initialization failure");
	}
	else {
		enet_ok = true;
	}
}

void uninitialize_enet_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (enet_ok) {
		enet_deinitialize();
	}
}
