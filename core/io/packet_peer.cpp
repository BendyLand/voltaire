/**************************************************************************/
/*  packet_peer.cpp                                                       */
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

#include "core/config/project_settings.h"
#include "core/io/marshalls.h"
#include "packet_peer.h"

void PacketPeer::set_encode_buffer_max_size(int p_max_size)
{
	ERR_FAIL_COND_MSG(p_max_size < 1024, "Max encode buffer must be at least 1024 bytes");
	ERR_FAIL_COND_MSG(p_max_size > 256 * 1024 * 1024, "Max encode buffer cannot exceed 256 MiB");
	encode_buffer_max_size = Math::next_power_of_2((uint32_t)p_max_size);
	encode_buffer.clear();
}

int PacketPeer::get_encode_buffer_max_size() const { return encode_buffer_max_size; }

Error PacketPeer::get_packet_buffer(Vector<uint8_t>& r_buffer)
{
	const uint8_t* buffer;
	int buffer_size;
	Error err = get_packet(&buffer, buffer_size);
	if (err) {
		return err;
	}

	r_buffer.resize(buffer_size);
	if (buffer_size == 0) {
		return OK;
	}

	uint8_t* w = r_buffer.ptrw();
	for (int i = 0; i < buffer_size; i++) {
		w[i] = buffer[i];
	}

	return OK;
}

Error PacketPeer::put_packet_buffer(const Vector<uint8_t>& p_buffer)
{
	int len = p_buffer.size();
	if (len == 0) {
		return OK;
	}

	const uint8_t* r = p_buffer.ptr();
	return put_packet(&r[0], len);
}

Error PacketPeer::_put_packet(const Vector<uint8_t>& p_buffer)
{
	return put_packet_buffer(p_buffer);
}

Vector<uint8_t> PacketPeer::_get_packet()
{
	Vector<uint8_t> raw;
	last_get_error = get_packet_buffer(raw);
	return raw;
}

Error PacketPeer::_get_packet_error() const { return last_get_error; }

Error PacketPeerExtension::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	WARN_PRINT_ONCE("PacketPeerExtension::_get_packet_native is unimplemented!");
	return FAILED;
}

Error PacketPeerExtension::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	WARN_PRINT_ONCE("PacketPeerExtension::_put_packet_native is unimplemented!");
	return FAILED;
}

Error PacketPeerStream::_poll_buffer() const
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);

	int read = 0;
	ERR_FAIL_COND_V(input_buffer.size() < ring_buffer.space_left(), ERR_UNAVAILABLE);
	Error err = peer->get_partial_data(input_buffer.ptrw(), ring_buffer.space_left(), read);
	if (err) {
		return err;
	}
	if (read == 0) {
		return OK;
	}

	int w = ring_buffer.write(&input_buffer[0], read);
	ERR_FAIL_COND_V(w != read, ERR_BUG);

	return OK;
}

int PacketPeerStream::get_available_packet_count() const
{
	_poll_buffer();

	uint32_t remaining = ring_buffer.data_left();

	int ofs = 0;
	int count = 0;

	while (remaining >= 4) {
		uint8_t lbuf[4];
		ring_buffer.copy(lbuf, ofs, 4);
		uint32_t len = decode_uint32(lbuf);
		remaining -= 4;
		ofs += 4;
		if (len > remaining) {
			break;
		}
		remaining -= len;
		ofs += len;
		count++;
	}

	return count;
}

Error PacketPeerStream::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	_poll_buffer();

	int remaining = ring_buffer.data_left();
	ERR_FAIL_COND_V(remaining < 4, ERR_UNAVAILABLE);
	uint8_t lbuf[4];
	ring_buffer.copy(lbuf, 0, 4);
	remaining -= 4;
	uint32_t len = decode_uint32(lbuf);
	ERR_FAIL_COND_V(remaining < (int)len, ERR_UNAVAILABLE);

	ERR_FAIL_COND_V(input_buffer.size() < (int)len, ERR_UNAVAILABLE);
	ring_buffer.read(lbuf, 4);					// get rid of first 4 bytes
	ring_buffer.read(input_buffer.ptrw(), len); // read packet

	*r_buffer = &input_buffer[0];
	r_buffer_size = len;
	return OK;
}

Error PacketPeerStream::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	Error err = _poll_buffer(); // won't hurt to poll here too

	if (err) {
		return err;
	}

	if (p_buffer_size == 0) {
		return OK;
	}

	ERR_FAIL_COND_V(p_buffer_size < 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_buffer_size + 4 > output_buffer.size(), ERR_INVALID_PARAMETER);

	encode_uint32(p_buffer_size, output_buffer.ptrw());
	uint8_t* dst = &output_buffer.write[4];
	for (int i = 0; i < p_buffer_size; i++) {
		dst[i] = p_buffer[i];
	}

	return peer->put_data(&output_buffer[0], p_buffer_size + 4);
}

int PacketPeerStream::get_max_packet_size() const { return output_buffer.size(); }

void PacketPeerStream::set_stream_peer(const Ref<StreamPeer>& p_peer)
{
	if (p_peer.ptr() != peer.ptr()) {
		ring_buffer.advance_read(ring_buffer.data_left()); // Reset the ring buffer.
	}

	peer = p_peer;
}

Ref<StreamPeer> PacketPeerStream::get_stream_peer() const { return peer; }

void PacketPeerStream::set_input_buffer_max_size(int p_max_size)
{
	ERR_FAIL_COND_MSG(p_max_size < 0, "Max size of input buffer size cannot be smaller than 0.");
	// WARNING: May lose packets.
	ERR_FAIL_COND_MSG(ring_buffer.data_left(), "Buffer in use, resizing would cause loss of data.");
	ring_buffer.resize(
		Math::nearest_shift(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4)) - 1);
	input_buffer.resize(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4));
}

int PacketPeerStream::get_input_buffer_max_size() const { return input_buffer.size() - 4; }

void PacketPeerStream::set_output_buffer_max_size(int p_max_size)
{
	output_buffer.resize(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4));
}

int PacketPeerStream::get_output_buffer_max_size() const { return output_buffer.size() - 4; }

PacketPeerStream::PacketPeerStream()
{
	// TODO: make a way to select dynamically based on available hardware
	int64_t rbsize = 16;

	ring_buffer.resize(rbsize);
	input_buffer.resize(int64_t(1) << rbsize);
	output_buffer.resize(int64_t(1) << rbsize);
}

// ==========================================================================
// packet_peer_udp.cpp
// ==========================================================================

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

#include "core/io/ip.h"
#include "core/io/udp_server.h"
#include "packet_peer_udp.h"

void PacketPeerUDP::set_blocking_mode(bool p_enable) { blocking = p_enable; }

void PacketPeerUDP::set_broadcast_enabled(bool p_enabled)
{
	ERR_FAIL_COND(udp_server);
	broadcast = p_enabled;
	if (_sock.is_valid() && _sock->is_open()) {
		_sock->set_broadcasting_enabled(p_enabled);
	}
}

Error PacketPeerUDP::join_multicast_group(IPAddress p_multi_address, const String& p_if_name)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!p_multi_address.is_valid(), ERR_INVALID_PARAMETER);

	if (!_sock->is_open()) {
		IP::Type ip_type = p_multi_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, err);
		_sock->set_blocking_enabled(false);
		_sock->set_broadcasting_enabled(broadcast);
	}
	return _sock->join_multicast_group(p_multi_address, p_if_name);
}

Error PacketPeerUDP::leave_multicast_group(IPAddress p_multi_address, const String& p_if_name)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!_sock->is_open(), ERR_UNCONFIGURED);
	return _sock->leave_multicast_group(p_multi_address, p_if_name);
}

String PacketPeerUDP::_get_packet_ip() const { return String(get_packet_address()); }

Error PacketPeerUDP::_set_dest_address(const String& p_address, int p_port)
{
	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
		ip = IP::resolve_hostname(p_address);
		if (!ip.is_valid()) {
			return ERR_CANT_RESOLVE;
		}
	}

	set_dest_address(ip, p_port);
	return OK;
}

int PacketPeerUDP::get_available_packet_count() const
{
	// TODO we should deprecate this, and expose poll instead!
	Error err = const_cast<PacketPeerUDP*>(this)->_poll();
	if (err != OK) {
		return -1;
	}

	return queue_count;
}

Error PacketPeerUDP::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	Error err = _poll();
	if (err != OK) {
		return err;
	}
	if (queue_count == 0) {
		return ERR_UNAVAILABLE;
	}

	/* Bogus GCC warning here:
	 * In member function 'int RingBuffer<T>::read(T*, int, bool) [with T = unsigned char]',
	 *     inlined from 'virtual Error PacketPeerUDP::get_packet(const uint8_t**, int&)' at
	 * core/io/packet_peer_udp.cpp:112:9, inlined from 'virtual Error
	 * PacketPeerUDP::get_packet(const uint8_t**, int&)' at core/io/packet_peer_udp.cpp:99:7: Error:
	 * ./core/ring_buffer.h:68:46: error: writing 1 byte into a region of size 0
	 * [-Werror=stringop-overflow=] 68 |                                 p_buf[dst++] = read[pos +
	 * i]; |                                 ~~~~~~~~~~~~~^~~~~~~
	 */
	VLTR_GCC_WARNING_PUSH
	VLTR_GCC_PRAGMA(
		GCC diagnostic warning "-Wstringop-overflow=0") // Can't "ignore" this for some reason.

	uint32_t size = 0;
	uint8_t ipv6[16] = {};
	rb.read(ipv6, 16, true);
	packet_ip.set_ipv6(ipv6);
	rb.read((uint8_t*)&packet_port, 4, true);
	rb.read((uint8_t*)&size, 4, true);
	rb.read(packet_buffer, size, true);
	--queue_count;
	*r_buffer = packet_buffer;
	r_buffer_size = size;

	VLTR_GCC_WARNING_POP

	return OK;
}

Error PacketPeerUDP::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!peer_addr.is_valid(), ERR_UNCONFIGURED);

	Error err;
	int sent = -1;

	if (!_sock->is_open()) {
		IP::Type ip_type = peer_addr.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, err);
		_sock->set_blocking_enabled(false);
		_sock->set_broadcasting_enabled(broadcast);
	}

	do {
		if (connected && !udp_server) {
			err = _sock->send(p_buffer, p_buffer_size, sent);
		}
		else {
			err = _sock->sendto(p_buffer, p_buffer_size, sent, peer_addr, peer_port);
		}
		if (err != OK) {
			if (err != ERR_BUSY) {
				return FAILED;
			}
			else if (!blocking) {
				return ERR_BUSY;
			}
			// Keep trying to send full packet
			continue;
		}
		return OK;

	} while (sent != p_buffer_size);

	return OK;
}

int PacketPeerUDP::get_max_packet_size() const
{
	return 512; // uhm maybe not
}

Error PacketPeerUDP::bind(int p_port, const IPAddress& p_bind_address, int p_recv_buffer_size)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 0 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The local port number must be between 0 and 65535 (inclusive).");

	Error err;
	IP::Type ip_type = IP::TYPE_ANY;

	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);

	if (err != OK) {
		return ERR_CANT_CREATE;
	}

	_sock->set_blocking_enabled(false);
	_sock->set_broadcasting_enabled(broadcast);
	NetSocket::Address addr(p_bind_address, p_port);
	err = _sock->bind(addr);

	if (err != OK) {
		_sock->close();
		return err;
	}
	rb.resize(Math::nearest_shift((uint32_t)p_recv_buffer_size));
	return OK;
}

Error PacketPeerUDP::connect_shared_socket(
	Ref<NetSocket> p_sock, IPAddress p_ip, uint16_t p_port, UDPServer* p_server)
{
	udp_server = p_server;
	connected = true;
	_sock = p_sock;
	peer_addr = p_ip;
	peer_port = p_port;
	packet_ip = peer_addr;
	packet_port = peer_port;
	return OK;
}

void PacketPeerUDP::disconnect_shared_socket()
{
	udp_server = nullptr;
	_sock = Ref<NetSocket>(NetSocket::create());
	close();
}

Error PacketPeerUDP::connect_to_host(const IPAddress& p_host, int p_port)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!p_host.is_valid(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The remote port number must be between 1 and 65535 (inclusive).");

	Error err;

	if (!_sock->is_open()) {
		IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_OPEN);
		_sock->set_blocking_enabled(false);
	}

	NetSocket::Address addr(p_host, p_port);
	err = _sock->connect_to_host(addr);

	// I see no reason why we should get ERR_BUSY (wouldblock/eagain) here.
	// This is UDP, so connect is only used to tell the OS to which socket
	// it should deliver packets when multiple are bound on the same address/port.
	if (err != OK) {
		close();
		ERR_FAIL_V_MSG(FAILED, "Unable to connect");
	}

	connected = true;

	peer_addr = p_host;
	peer_port = p_port;

	// Flush any packet we might still have in queue.
	rb.clear();
	return OK;
}

bool PacketPeerUDP::is_socket_connected() const { return connected; }

void PacketPeerUDP::close()
{
	if (udp_server) {
		udp_server->remove_peer(peer_addr, peer_port);
		udp_server = nullptr;
		_sock = Ref<NetSocket>(NetSocket::create());
	}
	else if (_sock.is_valid()) {
		_sock->close();
	}
	rb.resize(16);
	queue_count = 0;
	connected = false;
}

Error PacketPeerUDP::wait()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	return _sock->poll(NetSocket::POLL_TYPE_IN, -1);
}

Error PacketPeerUDP::_poll()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);

	if (!_sock->is_open()) {
		return FAILED;
	}
	if (udp_server) {
		return OK; // Handled by UDPServer.
	}

	Error err;
	int read;
	IPAddress ip;
	uint16_t port;

	while (true) {
		if (connected) {
			err = _sock->recv(recv_buffer, sizeof(recv_buffer), read);
			ip = peer_addr;
			port = peer_port;
		}
		else {
			err = _sock->recvfrom(recv_buffer, sizeof(recv_buffer), read, ip, port);
		}

		if (err != OK) {
			if (err == ERR_BUSY) {
				break;
			}
			return FAILED;
		}

		err = store_packet(ip, port, recv_buffer, read);
#ifdef TOOLS_ENABLED
		if (err != OK) {
			WARN_PRINT("Buffer full, dropping packets!");
		}
#endif
	}

	return OK;
}

Error PacketPeerUDP::store_packet(IPAddress p_ip, uint32_t p_port, uint8_t* p_buf, int p_buf_size)
{
	if (rb.space_left() < p_buf_size + 24) {
		return ERR_OUT_OF_MEMORY;
	}
	rb.write(p_ip.get_ipv6(), 16);
	rb.write((uint8_t*)&p_port, 4);
	rb.write((uint8_t*)&p_buf_size, 4);
	rb.write(p_buf, p_buf_size);
	++queue_count;
	return OK;
}

bool PacketPeerUDP::is_bound() const { return _sock.is_valid() && _sock->is_open(); }

IPAddress PacketPeerUDP::get_packet_address() const { return packet_ip; }

int PacketPeerUDP::get_packet_port() const { return packet_port; }

int PacketPeerUDP::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

void PacketPeerUDP::set_dest_address(const IPAddress& p_address, int p_port)
{
	ERR_FAIL_COND_MSG(connected, "Destination address cannot be set for connected sockets");
	peer_addr = p_address;
	peer_port = p_port;
}

PacketPeerUDP::PacketPeerUDP() : _sock(Ref<NetSocket>(NetSocket::create())) { rb.resize(16); }

PacketPeerUDP::~PacketPeerUDP() { close(); }

// ==========================================================================
// packet_peer_dtls.cpp
// ==========================================================================

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

#include "packet_peer_dtls.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

void PacketPeerDTLS::poll() {}
Error PacketPeerDTLS::connect_to_peer(
	Ref<PacketPeerUDP> p_base, const String& p_hostname, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
void PacketPeerDTLS::disconnect_from_peer() {}
PacketPeerDTLS::Status PacketPeerDTLS::get_status() const { return STATUS_DISCONNECTED; }

Error PacketPeerDTLS::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	r_buffer_size = 0;
	return ERR_UNAVAILABLE;
}
Error PacketPeerDTLS::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	return ERR_UNAVAILABLE;
}
int PacketPeerDTLS::get_available_packet_count() const { return 0; }
int PacketPeerDTLS::get_max_packet_size() const { return 0; }

PacketPeerDTLS* PacketPeerDTLS::create(bool p_notify_postinitialize) { return nullptr; }
bool PacketPeerDTLS::is_available() { return false; }

PacketPeerDTLS::PacketPeerDTLS() {}
PacketPeerDTLS::~PacketPeerDTLS() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// udp_server.cpp
// ==========================================================================

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

#include "udp_server.h"

Error UDPServer::poll()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	if (!_sock->is_open()) {
		return ERR_UNCONFIGURED;
	}
	Error err;
	int read;
	IPAddress ip;
	uint16_t port;
	while (true) {
		err = _sock->recvfrom(recv_buffer, sizeof(recv_buffer), read, ip, port);
		if (err != OK) {
			if (err == ERR_BUSY) {
				break;
			}
			return FAILED;
		}
		Peer p;
		p.ip = ip;
		p.port = port;
		List<Peer>::Element* E = peers.find(p);
		if (!E) {
			E = pending.find(p);
		}
		if (E) {
			E->get().peer->store_packet(ip, port, recv_buffer, read);
		}
		else {
			if (pending.size() >= max_pending_connections) {
				// Drop connection.
				continue;
			}
			// It's a new peer, add it to the pending list.
			Peer peer;
			peer.ip = ip;
			peer.port = port;
			Ref<PacketPeerUDP> peer_ref = memnew(PacketPeerUDP);
			peer.peer = peer_ref.ptr();
			peer.peer->connect_shared_socket(_sock, ip, port, this);
			peer.peer->store_packet(ip, port, recv_buffer, read);
			pending.push_back(peer);

			// FIXME: peer.peer is intentionally leaked such that it can destruct
			//        when the client no longer needs it.
			//        The current system 'abuses' the refcount_init semantics, such
			//        that the first take_connection call takes ownership. A refactor
			//        with a better "transfer ownership" semantics is warranted to avoid
			//        accidental leaks.
			peer_ref->reference();
		}
	}
	return OK;
}

Error UDPServer::listen(uint16_t p_port, const IPAddress& p_bind_address)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);

	Error err;
	IP::Type ip_type = IP::TYPE_ANY;

	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);

	if (err != OK) {
		return ERR_CANT_CREATE;
	}

	_sock->set_blocking_enabled(false);
	_sock->set_reuse_address_enabled(true);
	NetSocket::Address addr(p_bind_address, p_port);
	err = _sock->bind(addr);

	if (err != OK) {
		stop();
		return err;
	}
	return OK;
}

int UDPServer::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

bool UDPServer::is_listening() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	return _sock->is_open();
}

bool UDPServer::is_connection_available() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	if (!_sock->is_open()) {
		return false;
	}

	return pending.size() > 0;
}

void UDPServer::set_max_pending_connections(int p_max)
{
	ERR_FAIL_COND_MSG(p_max < 0, "Max pending connections value must be a positive number (0 means "
								 "refuse new connections).");
	max_pending_connections = p_max;
	while (p_max > pending.size()) {
		List<Peer>::Element* E = pending.back();
		if (!E) {
			break;
		}
		memdelete(E->get().peer);
		pending.erase(E);
	}
}

int UDPServer::get_max_pending_connections() const { return max_pending_connections; }

Ref<PacketPeerUDP> UDPServer::take_connection()
{
	Ref<PacketPeerUDP> conn;
	if (!is_connection_available()) {
		return conn;
	}

	Peer peer = pending.front()->get();
	pending.pop_front();
	peers.push_back(peer);
	return peer.peer;
}

void UDPServer::remove_peer(IPAddress p_ip, int p_port)
{
	Peer peer;
	peer.ip = p_ip;
	peer.port = p_port;
	List<Peer>::Element* E = peers.find(peer);
	if (E) {
		peers.erase(E);
	}
}

void UDPServer::stop()
{
	if (_sock.is_valid()) {
		_sock->close();
	}
	List<Peer>::Element* E = peers.front();
	while (E) {
		E->get().peer->disconnect_shared_socket();
		// Don't delete peer; it's intentionally weakly owned.
		E = E->next();
	}
	E = pending.front();
	while (E) {
		E->get().peer->disconnect_shared_socket();
		memdelete(E->get().peer);
		E = E->next();
	}
	peers.clear();
	pending.clear();
}

UDPServer::UDPServer() : _sock(Ref<NetSocket>(NetSocket::create())) {}

UDPServer::~UDPServer() { stop(); }

// ==========================================================================
// dtls_server.cpp
// ==========================================================================

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

#include "dtls_server.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

bool DTLSServer::is_available() { return false; }
DTLSServer* DTLSServer::create(bool p_notify_postinitialize) { return nullptr; }

Error DTLSServer::setup(Ref<TLSOptions> p_options) { return ERR_UNAVAILABLE; }
void DTLSServer::stop() {}
Ref<PacketPeerDTLS> DTLSServer::take_connection(Ref<PacketPeerUDP> p_peer)
{
	return Ref<PacketPeerDTLS>();
}

DTLSServer::DTLSServer() {}
DTLSServer::~DTLSServer() {}

#endif // !MODULE_MBEDTLS_ENABLED
