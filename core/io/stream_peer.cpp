/**************************************************************************/
/*  stream_peer.cpp                                                       */
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

#include "core/io/marshalls.h"
#include "stream_peer.h"

Error StreamPeer::_put_data(const Vector<uint8_t>& p_data)
{
	int len = p_data.size();
	if (len == 0) {
		return OK;
	}
	const uint8_t* r = p_data.ptr();
	return put_data(&r[0], len);
}

void StreamPeer::set_big_endian(bool p_big_endian) { big_endian = p_big_endian; }

bool StreamPeer::is_big_endian_enabled() const { return big_endian; }

void StreamPeer::put_u8(uint8_t p_val) { put_data((const uint8_t*)&p_val, 1); }

void StreamPeer::put_8(int8_t p_val) { put_data((const uint8_t*)&p_val, 1); }

void StreamPeer::put_u16(uint16_t p_val)
{
	if (big_endian) {
		p_val = BSWAP16(p_val);
	}
	uint8_t buf[2];
	encode_uint16(p_val, buf);
	put_data(buf, 2);
}

void StreamPeer::put_16(int16_t p_val)
{
	if (big_endian) {
		p_val = BSWAP16(p_val);
	}
	uint8_t buf[2];
	encode_uint16(p_val, buf);
	put_data(buf, 2);
}

void StreamPeer::put_u32(uint32_t p_val)
{
	if (big_endian) {
		p_val = BSWAP32(p_val);
	}
	uint8_t buf[4];
	encode_uint32(p_val, buf);
	put_data(buf, 4);
}

void StreamPeer::put_32(int32_t p_val)
{
	if (big_endian) {
		p_val = BSWAP32(p_val);
	}
	uint8_t buf[4];
	encode_uint32(p_val, buf);
	put_data(buf, 4);
}

void StreamPeer::put_u64(uint64_t p_val)
{
	if (big_endian) {
		p_val = BSWAP64(p_val);
	}
	uint8_t buf[8];
	encode_uint64(p_val, buf);
	put_data(buf, 8);
}

void StreamPeer::put_64(int64_t p_val)
{
	if (big_endian) {
		p_val = BSWAP64(p_val);
	}
	uint8_t buf[8];
	encode_uint64(p_val, buf);
	put_data(buf, 8);
}

void StreamPeer::put_half(float p_val)
{
	uint8_t buf[2];

	encode_half(p_val, buf);
	uint16_t* p16 = (uint16_t*)buf;
	if (big_endian) {
		*p16 = BSWAP16(*p16);
	}

	put_data(buf, 2);
}

void StreamPeer::put_float(float p_val)
{
	uint8_t buf[4];

	encode_float(p_val, buf);
	if (big_endian) {
		uint32_t* p32 = (uint32_t*)buf;
		*p32 = BSWAP32(*p32);
	}

	put_data(buf, 4);
}

void StreamPeer::put_double(double p_val)
{
	uint8_t buf[8];

	encode_double(p_val, buf);
	if (big_endian) {
		uint64_t* p64 = (uint64_t*)buf;
		*p64 = BSWAP64(*p64);
	}

	put_data(buf, 8);
}

void StreamPeer::put_string(const String& p_string)
{
	CharString cs = p_string.ascii();
	put_u32(cs.length());
	put_data((const uint8_t*)cs.get_data(), cs.length());
}

void StreamPeer::put_utf8_string(const String& p_string)
{
	CharString cs = p_string.utf8();
	put_u32(cs.length());
	put_data((const uint8_t*)cs.get_data(), cs.length());
}

uint8_t StreamPeer::get_u8()
{
	uint8_t buf[1] = {};
	get_data(buf, 1);
	return buf[0];
}

int8_t StreamPeer::get_8()
{
	uint8_t buf[1] = {};
	get_data(buf, 1);
	return int8_t(buf[0]);
}

uint16_t StreamPeer::get_u16()
{
	uint8_t buf[2];
	get_data(buf, 2);

	uint16_t r = decode_uint16(buf);
	if (big_endian) {
		r = BSWAP16(r);
	}

	return r;
}

int16_t StreamPeer::get_16()
{
	uint8_t buf[2];
	get_data(buf, 2);

	uint16_t r = decode_uint16(buf);
	if (big_endian) {
		r = BSWAP16(r);
	}

	return int16_t(r);
}

uint32_t StreamPeer::get_u32()
{
	uint8_t buf[4];
	get_data(buf, 4);

	uint32_t r = decode_uint32(buf);
	if (big_endian) {
		r = BSWAP32(r);
	}

	return r;
}

int32_t StreamPeer::get_32()
{
	uint8_t buf[4];
	get_data(buf, 4);

	uint32_t r = decode_uint32(buf);
	if (big_endian) {
		r = BSWAP32(r);
	}

	return int32_t(r);
}

uint64_t StreamPeer::get_u64()
{
	uint8_t buf[8];
	get_data(buf, 8);

	uint64_t r = decode_uint64(buf);
	if (big_endian) {
		r = BSWAP64(r);
	}

	return r;
}

int64_t StreamPeer::get_64()
{
	uint8_t buf[8];
	get_data(buf, 8);

	uint64_t r = decode_uint64(buf);
	if (big_endian) {
		r = BSWAP64(r);
	}

	return int64_t(r);
}

float StreamPeer::get_half()
{
	uint8_t buf[2];
	get_data(buf, 2);

	if (big_endian) {
		uint16_t* p16 = (uint16_t*)buf;
		*p16 = BSWAP16(*p16);
	}

	return decode_half(buf);
}

float StreamPeer::get_float()
{
	uint8_t buf[4];
	get_data(buf, 4);

	if (big_endian) {
		uint32_t* p32 = (uint32_t*)buf;
		*p32 = BSWAP32(*p32);
	}

	return decode_float(buf);
}

double StreamPeer::get_double()
{
	uint8_t buf[8];
	get_data(buf, 8);

	if (big_endian) {
		uint64_t* p64 = (uint64_t*)buf;
		*p64 = BSWAP64(*p64);
	}

	return decode_double(buf);
}

String StreamPeer::get_string(int p_bytes)
{
	if (p_bytes < 0) {
		p_bytes = get_32();
	}
	ERR_FAIL_COND_V(p_bytes < 0, String());

	Vector<char> buf;
	Error err = buf.resize(p_bytes + 1);
	ERR_FAIL_COND_V(err != OK, String());
	err = get_data((uint8_t*)&buf[0], p_bytes);
	ERR_FAIL_COND_V(err != OK, String());
	buf.write[p_bytes] = 0;
	return buf.ptr();
}

String StreamPeer::get_utf8_string(int p_bytes)
{
	if (p_bytes < 0) {
		p_bytes = get_32();
	}
	ERR_FAIL_COND_V(p_bytes < 0, String());

	Vector<uint8_t> buf;
	Error err = buf.resize(p_bytes);
	ERR_FAIL_COND_V(err != OK, String());
	err = get_data(buf.ptrw(), p_bytes);
	ERR_FAIL_COND_V(err != OK, String());

	return String::utf8((const char*)buf.ptr(), buf.size());
}

Error StreamPeerExtension::get_data(uint8_t* r_buffer, int p_bytes)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_get_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::get_partial_data(uint8_t* r_buffer, int p_bytes, int& r_received)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_get_partial_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::put_data(const uint8_t* p_data, int p_bytes)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_put_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_put_partial_data is unimplemented!");
	return FAILED;
}

Error StreamPeerBuffer::put_data(const uint8_t* p_data, int p_bytes)
{
	if (p_bytes <= 0 || !p_data) {
		return OK;
	}

	if (pointer + p_bytes > data.size()) {
		data.resize(pointer + p_bytes);
	}

	uint8_t* w = data.ptrw();
	memcpy(&w[pointer], p_data, p_bytes);

	pointer += p_bytes;
	return OK;
}

Error StreamPeerBuffer::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	r_sent = p_bytes;
	return put_data(p_data, p_bytes);
}

Error StreamPeerBuffer::get_data(uint8_t* p_buffer, int p_bytes)
{
	int recv;
	get_partial_data(p_buffer, p_bytes, recv);
	if (recv != p_bytes) {
		return ERR_INVALID_PARAMETER;
	}

	return OK;
}

Error StreamPeerBuffer::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	if (!p_bytes) {
		r_received = 0;
		return OK;
	}

	if (pointer + p_bytes > data.size()) {
		r_received = data.size() - pointer;
		if (r_received <= 0) {
			r_received = 0;
			return OK; // you got 0
		}
	}
	else {
		r_received = p_bytes;
	}

	const uint8_t* r = data.ptr();
	memcpy(p_buffer, r + pointer, r_received);

	pointer += r_received;
	// FIXME: return what? OK or ERR_*
	// return OK for now so we don't maybe return garbage
	return OK;
}

int StreamPeerBuffer::get_available_bytes() const { return data.size() - pointer; }

void StreamPeerBuffer::seek(int p_pos)
{
	ERR_FAIL_COND(p_pos < 0);
	ERR_FAIL_COND(p_pos > data.size());
	pointer = p_pos;
}

int StreamPeerBuffer::get_size() const { return data.size(); }

int StreamPeerBuffer::get_position() const { return pointer; }

void StreamPeerBuffer::resize(int p_size) { data.resize(p_size); }

void StreamPeerBuffer::set_data_array(const Vector<uint8_t>& p_data)
{
	data = p_data;
	pointer = 0;
}

const Vector<uint8_t>& StreamPeerBuffer::get_data_array() const { return data; }

void StreamPeerBuffer::clear()
{
	data.clear();
	pointer = 0;
}

Ref<StreamPeerBuffer> StreamPeerBuffer::duplicate() const
{
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->data = data;
	return spb;
}

// ==========================================================================
// stream_peer_socket.cpp
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

#include "stream_peer_socket.h"

#include "core/os/os.h"

void StreamPeerSocket::accept_socket(Ref<NetSocket> p_sock, const NetSocket::Address& p_addr)
{
	_sock = p_sock;
	_sock->set_blocking_enabled(false);

	timeout = 30;
	status = STATUS_CONNECTED;
	peer_address = p_addr;
	if (p_addr.is_unix()) {
		domain = DOMAIN_UDS;
		peer_path = String(p_addr.get_path().get_data());
	}
	else if (p_addr.is_inet()) {
		domain = DOMAIN_TCP;
	}
}

Error StreamPeerSocket::bind(int p_port, const IPAddress& p_host)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V_MSG(p_port < 0 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The local port number must be between 0 and 65535 (inclusive).");

	domain = DOMAIN_TCP;
	IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	if (p_host.is_wildcard()) {
		ip_type = IP::TYPE_ANY;
	}
	Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);
	if (err != OK) {
		return err;
	}
	_sock->set_blocking_enabled(false);
	NetSocket::Address addr(p_host, p_port);
	return _sock->bind(addr);
}

Error StreamPeerSocket::connect_to_host(const IPAddress& p_host, int p_port)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(status != STATUS_NONE, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(!p_host.is_valid(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The remote port number must be between 1 and 65535 (inclusive).");

	domain = DOMAIN_TCP;
	if (!_sock->is_open()) {
		IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);
		if (err != OK) {
			return err;
		}
		_sock->set_blocking_enabled(false);
	}

	timeout = 30;

	NetSocket::Address addr(p_host, p_port);
	Error err = _sock->connect_to_host(addr);

	if (err == OK) {
		status = STATUS_CONNECTED;
	}
	else if (err == ERR_BUSY) {
		status = STATUS_CONNECTING;
	}
	else {
		ERR_PRINT("Connection to remote host failed!");
		disconnect_from_host();
		return FAILED;
	}

	peer_address = addr;

	return OK;
}

void StreamPeerSocket::set_no_delay(bool p_enabled)
{
	ERR_FAIL_COND(_sock.is_null() || !_sock->is_open());
	_sock->set_tcp_no_delay_enabled(p_enabled);
}

IPAddress StreamPeerSocket::get_connected_host() const { return peer_address.ip(); }

int StreamPeerSocket::get_connected_port() const { return peer_address.port(); }

int StreamPeerSocket::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

Error StreamPeerSocket::bind(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);

	domain = DOMAIN_UDS;
	IP::Type ip_type = IP::TYPE_NONE;
	Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
	if (err != OK) {
		return err;
	}
	_sock->set_blocking_enabled(false);
	NetSocket::Address addr(p_path);
	return _sock->bind(addr);
}

Error StreamPeerSocket::connect_to_path(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	domain = DOMAIN_UDS;
	if (!_sock->is_open()) {
		IP::Type ip_type = IP::TYPE_NONE;
		Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
		if (err != OK) {
			return err;
		}
		_sock->set_blocking_enabled(false);
	}

	timeout = 30;
	NetSocket::Address addr(p_path);
	Error err = _sock->connect_to_host(addr);

	if (err == OK) {
		status = STATUS_CONNECTED;
	}
	else if (err == ERR_BUSY) {
		status = STATUS_CONNECTING;
	}
	else {
		ERR_PRINT("Connection to remote host failed!");
		disconnect_from_host();
		return FAILED;
	}

	peer_address = addr;
	peer_path = p_path;

	return OK;
}

String StreamPeerSocket::get_connected_path() const
{
	return String(peer_address.get_path().get_data());
}

Error StreamPeerSocket::poll()
{
	if (status == STATUS_CONNECTED) {
		Error err;
		err = _sock->poll(NetSocket::POLL_TYPE_IN, 0);
		if (err == OK) {
			// FIN received
			if (_sock->get_available_bytes() == 0) {
				disconnect_from_host();
				return OK;
			}
		}
		// Also poll write
		err = _sock->poll(NetSocket::POLL_TYPE_IN_OUT, 0);
		if (err != OK && err != ERR_BUSY) {
			// Got an error
			disconnect_from_host();
			status = STATUS_ERROR;
			return err;
		}
		return OK;
	}
	else if (status != STATUS_CONNECTING) {
		return OK;
	}

	Error err = _sock->connect_to_host(peer_address);

	if (err == OK) {
		status = STATUS_CONNECTED;
		return OK;
	}
	else if (err == ERR_BUSY) {
		// Check for connect timeout
		if (OS::get_ticks_msec() > timeout) {
			disconnect_from_host();
			status = STATUS_ERROR;
			return ERR_CONNECTION_ERROR;
		}
		// Still trying to connect
		return OK;
	}

	disconnect_from_host();
	status = STATUS_ERROR;
	return ERR_CONNECTION_ERROR;
}

Error StreamPeerSocket::write(const uint8_t* p_data, int p_bytes, int& r_sent, bool p_block)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);

	if (status != STATUS_CONNECTED) {
		return FAILED;
	}

	Error err;
	int data_to_send = p_bytes;
	const uint8_t* offset = p_data;
	int total_sent = 0;

	while (data_to_send) {
		int sent_amount = 0;
		err = _sock->send(offset, data_to_send, sent_amount);

		if (err != OK) {
			if (err != ERR_BUSY) {
				disconnect_from_host();
				return FAILED;
			}

			if (!p_block) {
				r_sent = total_sent;
				return OK;
			}

			// Block and wait for the socket to accept more data
			err = _sock->poll(NetSocket::POLL_TYPE_OUT, -1);
			if (err != OK) {
				disconnect_from_host();
				return FAILED;
			}
		}
		else {
			data_to_send -= sent_amount;
			offset += sent_amount;
			total_sent += sent_amount;
		}
	}

	r_sent = total_sent;

	return OK;
}

Error StreamPeerSocket::read(uint8_t* p_buffer, int p_bytes, int& r_received, bool p_block)
{
	if (status != STATUS_CONNECTED) {
		return FAILED;
	}

	Error err;
	int to_read = p_bytes;
	int total_read = 0;
	r_received = 0;

	while (to_read) {
		int read = 0;
		err = _sock->recv(p_buffer + total_read, to_read, read);

		if (err != OK) {
			if (err != ERR_BUSY) {
				disconnect_from_host();
				return FAILED;
			}

			if (!p_block) {
				r_received = total_read;
				return OK;
			}

			err = _sock->poll(NetSocket::POLL_TYPE_IN, -1);

			if (err != OK) {
				disconnect_from_host();
				return FAILED;
			}
		}
		else if (read == 0) {
			disconnect_from_host();
			r_received = total_read;
			return ERR_FILE_EOF;
		}
		else {
			to_read -= read;
			total_read += read;

			if (!p_block) {
				r_received = total_read;
				return OK;
			}
		}
	}

	r_received = total_read;

	return OK;
}

StreamPeerSocket::Status StreamPeerSocket::get_status() const { return status; }

void StreamPeerSocket::disconnect_from_host()
{
	if (_sock.is_valid() && _sock->is_open()) {
		_sock->close();
	}

	timeout = 0;
	status = STATUS_NONE;
	peer_address = NetSocket::Address();
	peer_path = String();
}

Error StreamPeerSocket::wait(NetSocket::PollType p_type, int p_timeout)
{
	ERR_FAIL_COND_V(_sock.is_null() || !_sock->is_open(), ERR_UNAVAILABLE);
	return _sock->poll(p_type, p_timeout);
}

Error StreamPeerSocket::put_data(const uint8_t* p_data, int p_bytes)
{
	int total;
	return write(p_data, p_bytes, total, true);
}

Error StreamPeerSocket::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	return write(p_data, p_bytes, r_sent, false);
}

Error StreamPeerSocket::get_data(uint8_t* p_buffer, int p_bytes)
{
	int total;
	return read(p_buffer, p_bytes, total, true);
}

Error StreamPeerSocket::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	return read(p_buffer, p_bytes, r_received, false);
}

int StreamPeerSocket::get_available_bytes() const
{
	ERR_FAIL_COND_V(_sock.is_null(), -1);
	return _sock->get_available_bytes();
}

StreamPeerSocket::StreamPeerSocket(Domain p_domain)
	: domain(p_domain), _sock(NetSocket::create())
{
}

StreamPeerSocket::~StreamPeerSocket() { disconnect_from_host(); }

// ==========================================================================
// stream_peer_gzip.cpp
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

#include <zlib.h>
#include "core/io/stream_peer.h"
#include "core/io/zip_io.h"

StreamPeerGZIP::~StreamPeerGZIP() { _close(); }

void StreamPeerGZIP::_close()
{
	if (ctx) {
		z_stream* strm = (z_stream*)ctx;
		if (compressing) {
			deflateEnd(strm);
		}
		else {
			inflateEnd(strm);
		}
		memfree(strm);
		ctx = nullptr;
	}
}

void StreamPeerGZIP::clear()
{
	_close();
	rb.clear();
	buffer.clear();
}

Error StreamPeerGZIP::start_compression(bool p_is_deflate, int p_buffer_size)
{
	return _start(true, p_is_deflate, p_buffer_size);
}

Error StreamPeerGZIP::start_decompression(bool p_is_deflate, int p_buffer_size)
{
	return _start(false, p_is_deflate, p_buffer_size);
}

Error StreamPeerGZIP::_start(bool p_compress, bool p_is_deflate, int p_buffer_size)
{
	ERR_FAIL_COND_V(ctx != nullptr, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V_MSG(p_buffer_size <= 0, ERR_INVALID_PARAMETER,
		"Invalid buffer size. It should be a positive integer.");
	clear();
	compressing = p_compress;
	rb.resize(Math::nearest_shift(uint32_t(p_buffer_size - 1)));
	buffer.resize(1024);

	// Create ctx.
	ctx = memalloc(sizeof(z_stream));
	z_stream& strm = *(z_stream*)ctx;
	strm.next_in = Z_NULL;
	strm.avail_in = 0;
	strm.zalloc = zipio_alloc;
	strm.zfree = zipio_free;
	strm.opaque = Z_NULL;
	int window_bits = p_is_deflate ? 15 : (15 + 16);
	int err = Z_OK;
	int level = Z_DEFAULT_COMPRESSION;
	if (compressing) {
		err = deflateInit2(&strm, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
	}
	else {
		err = inflateInit2(&strm, window_bits);
	}
	ERR_FAIL_COND_V(err != Z_OK, FAILED);
	return OK;
}

Error StreamPeerGZIP::_process(uint8_t* p_dst, int p_dst_size, const uint8_t* p_src, int p_src_size,
	int& r_consumed, int& r_out, bool p_close)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	z_stream& strm = *(z_stream*)ctx;
	strm.avail_in = p_src_size;
	strm.avail_out = p_dst_size;
	strm.next_in = (Bytef*)p_src;
	strm.next_out = (Bytef*)p_dst;
	int flush = p_close ? Z_FINISH : Z_NO_FLUSH;
	if (compressing) {
		int err = deflate(&strm, flush);
		ERR_FAIL_COND_V(err != (p_close ? Z_STREAM_END : Z_OK), FAILED);
	}
	else {
		int err = inflate(&strm, flush);
		ERR_FAIL_COND_V(err != Z_OK && err != Z_STREAM_END, FAILED);
	}
	r_out = p_dst_size - strm.avail_out;
	r_consumed = p_src_size - strm.avail_in;
	return OK;
}

Error StreamPeerGZIP::put_data(const uint8_t* p_data, int p_bytes)
{
	int wrote = 0;
	Error err = put_partial_data(p_data, p_bytes, wrote);
	if (err != OK) {
		return err;
	}
	ERR_FAIL_COND_V(p_bytes != wrote, ERR_OUT_OF_MEMORY);
	return OK;
}

Error StreamPeerGZIP::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);

	// Ensure we have enough space in temporary buffer.
	if (buffer.size() < p_bytes) {
		buffer.resize(p_bytes);
	}

	r_sent = 0;
	while (r_sent < p_bytes && rb.space_left() > 1024) { // Keep the ring buffer size meaningful.
		int sent = 0;
		int to_write = 0;
		// Compress or decompress
		Error err = _process(buffer.ptrw(), MIN(buffer.size(), rb.space_left()), p_data + r_sent,
			p_bytes - r_sent, sent, to_write);
		if (err != OK) {
			return err;
		}
		// When decompressing, we might need to do another round.
		r_sent += sent;

		// We can't write more than this buffer is full.
		if (sent == 0 && to_write == 0) {
			return OK;
		}
		if (to_write) {
			// Copy to ring buffer.
			int wrote = rb.write(buffer.ptr(), to_write);
			ERR_FAIL_COND_V(wrote != to_write, ERR_BUG);
		}
	}
	return OK;
}

Error StreamPeerGZIP::get_data(uint8_t* p_buffer, int p_bytes)
{
	int received = 0;
	Error err = get_partial_data(p_buffer, p_bytes, received);
	if (err != OK) {
		return err;
	}
	ERR_FAIL_COND_V(p_bytes != received, ERR_UNAVAILABLE);
	return OK;
}

Error StreamPeerGZIP::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);

	r_received = MIN(p_bytes, rb.data_left());
	if (r_received == 0) {
		return OK;
	}
	int received = rb.read(p_buffer, r_received);
	ERR_FAIL_COND_V(received != r_received, ERR_BUG);
	return OK;
}

int StreamPeerGZIP::get_available_bytes() const { return rb.data_left(); }

Error StreamPeerGZIP::finish()
{
	ERR_FAIL_COND_V(!ctx || !compressing, ERR_UNAVAILABLE);
	// Ensure we have enough space in temporary buffer.
	if (buffer.size() < get_available_bytes()) {
		buffer.resize(
			get_available_bytes()); // get_available_bytes() is what we can store in RingBuffer.
	}
	int consumed = 0;
	int to_write = 0;
	Error err =
		_process(buffer.ptrw(), buffer.size(), nullptr, 0, consumed, to_write, true); // compress
	if (err != OK) {
		return err;
	}
	int wrote = rb.write(buffer.ptr(), to_write);
	ERR_FAIL_COND_V(wrote != to_write, ERR_OUT_OF_MEMORY);
	return OK;
}

// ==========================================================================
// stream_peer_tls.cpp
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

#include "stream_peer_tls.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

void StreamPeerTLS::poll() {}
Error StreamPeerTLS::accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
Error StreamPeerTLS::connect_to_stream(
	Ref<StreamPeer> p_base, const String& p_common_name, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
StreamPeerTLS::Status StreamPeerTLS::get_status() const { return STATUS_DISCONNECTED; }
Ref<StreamPeer> StreamPeerTLS::get_stream() const { return Ref<StreamPeer>(); }
void StreamPeerTLS::disconnect_from_stream() {}

Error StreamPeerTLS::put_data(const uint8_t* p_data, int p_bytes) { return ERR_UNAVAILABLE; }
Error StreamPeerTLS::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	r_sent = 0;
	return ERR_UNAVAILABLE;
}
Error StreamPeerTLS::get_data(uint8_t* p_buffer, int p_bytes) { return ERR_UNAVAILABLE; }
Error StreamPeerTLS::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	r_received = 0;
	return ERR_UNAVAILABLE;
}
int StreamPeerTLS::get_available_bytes() const { return 0; }

StreamPeerTLS* StreamPeerTLS::create(bool p_notify_postinitialize) { return nullptr; }
bool StreamPeerTLS::is_available() { return false; }

StreamPeerTLS::StreamPeerTLS() {}
StreamPeerTLS::~StreamPeerTLS() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// socket_server.cpp
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

#include "socket_server.h"

Error SocketServer::_listen(const NetSocket::Address& p_addr)
{
	DEV_ASSERT(_sock.is_valid());
	DEV_ASSERT(_sock->is_open());

	_sock->set_blocking_enabled(false);
	Error err = _sock->bind(p_addr);

	if (err != OK) {
		_sock->close();
		return ERR_ALREADY_IN_USE;
	}

	err = _sock->listen(MAX_PENDING_CONNECTIONS);

	if (err != OK) {
		_sock->close();
		return FAILED;
	}
	return OK;
}

Error SocketServer::listen(uint16_t p_port, const IPAddress& p_bind_address)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);

	domain = StreamPeerSocket::DOMAIN_TCP;
	IP::Type ip_type = IP::TYPE_ANY;

	// If the bind address is valid use its type as the socket type
	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);

	ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

	_sock->set_reuse_address_enabled(true);

	return _listen(NetSocket::Address(p_bind_address, p_port));
}

Error SocketServer::listen_path(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	domain = StreamPeerSocket::DOMAIN_UDS;
	IP::Type ip_type = IP::TYPE_NONE;
	Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
	ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

	return _listen(NetSocket::Address(p_path));
}

int SocketServer::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

bool SocketServer::is_listening() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	return _sock->is_open();
}

bool SocketServer::is_connection_available() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	if (!_sock->is_open()) {
		return false;
	}

	Error err = _sock->poll(NetSocket::POLL_TYPE_IN, 0);
	return (err == OK);
}

Ref<StreamPeerSocket> SocketServer::take_connection()
{
	Ref<StreamPeerSocket> conn;
	if (!is_connection_available()) {
		return conn;
	}

	NetSocket::Address addr;
	Ref<NetSocket> ns = _sock->accept(addr);
	if (ns.is_null()) {
		return conn;
	}

	conn.instantiate(domain);
	conn->accept_socket(ns, addr);
	return conn;
}

void SocketServer::stop()
{
	if (_sock.is_valid()) {
		_sock->close();
	}
}

SocketServer::SocketServer() : _sock(NetSocket::create()) {}

SocketServer::~SocketServer() { stop(); }

// ============================================================================
// Merged from core/io/http_client.cpp
// ============================================================================

#include "http_client.h"

#ifndef WEB_ENABLED
#include "core/io/stream_peer_socket.h"
#include "core/io/stream_peer_tls.h"
#include "core/os/os.h"
#include "core/version.h"
#endif

const char* HTTPClient::_methods[METHOD_MAX] = {
	"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE", "CONNECT", "PATCH"};

Ref<HTTPClient> HTTPClient::create(bool p_notify_postinitialize)
{
#ifdef WEB_ENABLED
	if (_create) {
		return Ref<HTTPClient>(_create(p_notify_postinitialize));
	}
	return Ref<HTTPClient>();
#else
	(void)p_notify_postinitialize;
	return Ref<HTTPClient>(memnew(HTTPClient));
#endif
}

Error HTTPClient::_request_raw(Method p_method, const String& p_url,
	const Vector<String>& p_headers, const Vector<uint8_t>& p_body)
{
	int size = p_body.size();
	return request(p_method, p_url, p_headers, size > 0 ? p_body.ptr() : nullptr, size);
}

Error HTTPClient::_request(
	Method p_method, const String& p_url, const Vector<String>& p_headers, const String& p_body)
{
	CharString body_utf8 = p_body.utf8();
	int size = body_utf8.length();
	return request(p_method, p_url, p_headers,
		size > 0 ? (const uint8_t*)body_utf8.get_data() : nullptr, size);
}

Error HTTPClient::verify_headers(const Vector<String>& p_headers)
{
	for (int i = 0; i < p_headers.size(); i++) {
		String sanitized = p_headers[i].strip_edges();
		ERR_FAIL_COND_V_MSG(sanitized.is_empty(), ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: empty.", i));
		ERR_FAIL_COND_V_MSG(sanitized.find_char(':') < 1, ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: String must contain header-value pair, "
					"delimited by ':', but was: '%s'.",
				i, p_headers[i]));
	}

	return OK;
}

Vector<String> HTTPClient::_get_response_headers()
{
	List<String> rh;
	get_response_headers(&rh);
	Vector<String> ret;
	ret.resize(rh.size());
	int idx = 0;
	for (const String& E : rh) {
		ret.set(idx++, E);
	}

	return ret;
}

#ifdef WEB_ENABLED

void HTTPClient::set_http_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTP proxy feature is not available");
}

void HTTPClient::set_https_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTPS proxy feature is not available");
}

#else

Error HTTPClient::connect_to_host(const String& p_host, int p_port, Ref<TLSOptions> p_options)
{
	close();

	conn_port = p_port;
	conn_host = p_host;
	tls_options = p_options;

	String host_lower = conn_host.to_lower();
	if (host_lower.begins_with("http://")) {
		conn_host = conn_host.substr(7);
		tls_options.unref();
	}
	else if (host_lower.begins_with("https://")) {
		if (tls_options.is_null()) {
			tls_options = TLSOptions::client();
		}
		conn_host = conn_host.substr(8);
	}

	ERR_FAIL_COND_V(tls_options.is_valid() && tls_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(tls_options.is_valid() && !StreamPeerTLS::is_available(), ERR_UNAVAILABLE,
		"HTTPS is not available in this build.");
	ERR_FAIL_COND_V(conn_host.length() < HOST_MIN_LEN, ERR_INVALID_PARAMETER);

	if (conn_port < 0) {
		if (tls_options.is_valid()) {
			conn_port = PORT_HTTPS;
		}
		else {
			conn_port = PORT_HTTP;
		}
	}

	connection = tcp_connection;

	if (tls_options.is_valid() && https_proxy_port != -1) {
		proxy_client.instantiate(); // Needs proxy negotiation.
		server_host = https_proxy_host;
		server_port = https_proxy_port;
	}
	else if (tls_options.is_null() && http_proxy_port != -1) {
		server_host = http_proxy_host;
		server_port = http_proxy_port;
	}
	else {
		server_host = conn_host;
		server_port = conn_port;
	}

	if (server_host.is_valid_ip_address()) {
		// Host contains valid IP.
		Error err = tcp_connection->connect_to_host(IPAddress(server_host), server_port);
		if (err) {
			status = STATUS_CANT_CONNECT;
			return err;
		}

		status = STATUS_CONNECTING;
	}
	else {
		// Host contains hostname and needs to be resolved to IP.
		resolving = IP::resolve_hostname_queue_item(server_host);
		if (resolving == IP::RESOLVER_INVALID_ID) {
			status = STATUS_CANT_RESOLVE;
			return ERR_CANT_RESOLVE;
		}
		status = STATUS_RESOLVING;
	}

	return OK;
}

void HTTPClient::set_connection(const Ref<StreamPeer>& p_connection)
{
	ERR_FAIL_COND_MSG(
		p_connection.is_null(), "Connection is not a reference to a valid StreamPeer object.");

	if (connection == p_connection) {
		return;
	}

	close();
	connection = p_connection;
	status = STATUS_CONNECTED;
}

Ref<StreamPeer> HTTPClient::get_connection() const { return connection; }

static bool _check_request_url(HTTPClient::Method p_method, const String& p_url)
{
	switch (p_method) {
	case HTTPClient::METHOD_CONNECT: {
		// Authority in host:port format, as in RFC7231.
		int pos = p_url.find_char(':');
		return 0 < pos && pos < p_url.length() - 1;
	}
	case HTTPClient::METHOD_OPTIONS: {
		if (p_url == "*") {
			return true;
		}
		[[fallthrough]];
	}
	default:
		// Absolute path or absolute URL.
		return p_url.begins_with("/") || p_url.begins_with("http://") ||
			   p_url.begins_with("https://");
	}
}

Error HTTPClient::request(Method p_method, const String& p_url, const Vector<String>& p_headers,
	const uint8_t* p_body, int p_body_size)
{
	ERR_FAIL_INDEX_V(p_method, METHOD_MAX, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(!_check_request_url(p_method, p_url), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(status != STATUS_CONNECTED, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(connection.is_null(), ERR_INVALID_DATA);

	Error err = verify_headers(p_headers);
	if (err) {
		return err;
	}

	String uri = p_url;
	if (tls_options.is_null() && http_proxy_port != -1) {
		uri = vformat("http://%s:%d%s", conn_host, conn_port, p_url);
	}

	String req_str = String(_methods[p_method]) + " " + uri + " HTTP/1.1\r\n";
	bool add_host = true;
	bool add_clen = p_body_size > 0;
	bool add_uagent = true;
	bool add_accept = true;
	for (int i = 0; i < p_headers.size(); i++) {
		req_str += p_headers[i] + "\r\n";
		if (add_host && p_headers[i].findn("Host:") == 0) {
			add_host = false;
		}
		if (add_clen && p_headers[i].findn("Content-Length:") == 0) {
			add_clen = false;
		}
		if (add_uagent && p_headers[i].findn("User-Agent:") == 0) {
			add_uagent = false;
		}
		if (add_accept && p_headers[i].findn("Accept:") == 0) {
			add_accept = false;
		}
	}
	if (add_host) {
		if ((tls_options.is_valid() && conn_port == PORT_HTTPS) ||
			(tls_options.is_null() && conn_port == PORT_HTTP)) {
			// Don't append the standard ports.
			req_str += "Host: " + conn_host + "\r\n";
		}
		else {
			req_str += "Host: " + conn_host + ":" + itos(conn_port) + "\r\n";
		}
	}
	if (add_clen) {
		req_str += "Content-Length: " + itos(p_body_size) + "\r\n";
	}
	if (add_uagent) {
		req_str += "User-Agent: GodotEngine/" + String(VLTR_VERSION_FULL_BUILD) + " (" +
				   OS::get_name() + ")\r\n";
	}
	if (add_accept) {
		req_str += "Accept: */*\r\n";
	}
	req_str += "\r\n";
	CharString cs = req_str.utf8();

	request_buffer->clear();
	request_buffer->put_data((const uint8_t*)cs.get_data(), cs.length());
	if (p_body_size > 0) {
		request_buffer->put_data(p_body, p_body_size);
	}
	request_buffer->seek(0);

	status = STATUS_REQUESTING;
	head_request = p_method == METHOD_HEAD;

	return OK;
}

bool HTTPClient::has_response() const { return response_headers.size() != 0; }

bool HTTPClient::is_response_chunked() const { return chunked; }

int HTTPClient::get_response_code() const { return response_num; }

Error HTTPClient::get_response_headers(List<String>* r_response)
{
	if (!response_headers.size()) {
		return ERR_INVALID_PARAMETER;
	}

	for (int i = 0; i < response_headers.size(); i++) {
		r_response->push_back(response_headers[i]);
	}

	response_headers.clear();

	return OK;
}

void HTTPClient::close()
{
	if (tcp_connection->get_status() != StreamPeerSocket::STATUS_NONE) {
		tcp_connection->disconnect_from_host();
	}

	connection.unref();
	proxy_client.unref();
	status = STATUS_DISCONNECTED;
	head_request = false;
	if (resolving != IP::RESOLVER_INVALID_ID) {
		IP::erase_resolve_item(resolving);
		resolving = IP::RESOLVER_INVALID_ID;
	}
	response_headers.clear();
	response_str.clear();
	request_buffer->clear();
	body_size = -1;
	body_left = 0;
	chunk_left = 0;
	chunk_trailer_part = false;
	read_until_eof = false;
	response_num = 0;
	handshaking = false;
}

Error HTTPClient::poll()
{
	if (tcp_connection.is_valid()) {
		tcp_connection->poll();
	}
	switch (status) {
	case STATUS_RESOLVING: {
		ERR_FAIL_COND_V(resolving == IP::RESOLVER_INVALID_ID, ERR_BUG);

		IP::ResolverStatus rstatus = IP::get_resolve_item_status(resolving);
		switch (rstatus) {
		case IP::RESOLVER_STATUS_WAITING:
			return OK; // Still resolving.

		case IP::RESOLVER_STATUS_DONE: {
			IP::erase_resolve_item(resolving);
			resolving = IP::RESOLVER_INVALID_ID;

			Error err = ERR_BUG; // Should be at least one entry.
			if (err) {
				status = STATUS_CANT_CONNECT;
				return err;
			}

			status = STATUS_CONNECTING;
		} break;
		case IP::RESOLVER_STATUS_NONE:
		case IP::RESOLVER_STATUS_ERROR: {
			IP::erase_resolve_item(resolving);
			resolving = IP::RESOLVER_INVALID_ID;
			close();
			status = STATUS_CANT_RESOLVE;
			return ERR_CANT_RESOLVE;
		} break;
		}
	} break;
	case STATUS_CONNECTING: {
		StreamPeerSocket::Status s = tcp_connection->get_status();
		switch (s) {
		case StreamPeerSocket::STATUS_CONNECTING: {
			return OK;
		} break;
		case StreamPeerSocket::STATUS_CONNECTED: {
			if (tls_options.is_valid() && proxy_client.is_valid()) {
				Error err = proxy_client->poll();
				if (err == ERR_UNCONFIGURED) {
					proxy_client->set_connection(tcp_connection);
					const Vector<String> headers;
					err = proxy_client->request(METHOD_CONNECT,
						vformat("%s:%d", conn_host, conn_port), headers, nullptr, 0);
					if (err != OK) {
						status = STATUS_CANT_CONNECT;
						return err;
					}
				}
				else if (err != OK) {
					status = STATUS_CANT_CONNECT;
					return err;
				}
				switch (proxy_client->get_status()) {
				case STATUS_REQUESTING: {
					return OK;
				} break;
				case STATUS_BODY: {
					proxy_client->read_response_body_chunk();
					return OK;
				} break;
				case STATUS_CONNECTED: {
					if (proxy_client->get_response_code() != RESPONSE_OK) {
						status = STATUS_CANT_CONNECT;
						return ERR_CANT_CONNECT;
					}
					proxy_client.unref();
					return OK;
				}
				case STATUS_DISCONNECTED:
				case STATUS_RESOLVING:
				case STATUS_CONNECTING: {
					status = STATUS_CANT_CONNECT;
					ERR_FAIL_V(ERR_BUG);
				} break;
				default: {
					status = STATUS_CANT_CONNECT;
					return ERR_CANT_CONNECT;
				} break;
				}
			}
			else if (tls_options.is_valid()) {
				Ref<StreamPeerTLS> tls_conn;
				if (!handshaking) {
					// Connect the StreamPeerTLS and start handshaking.
					tls_conn = Ref<StreamPeerTLS>(StreamPeerTLS::create());
					Error err = tls_conn->connect_to_stream(tcp_connection, conn_host, tls_options);
					if (err != OK) {
						close();
						status = STATUS_TLS_HANDSHAKE_ERROR;
						return ERR_CANT_CONNECT;
					}
					connection = tls_conn;
					handshaking = true;
				}
				else {
					// We are already handshaking, which means we can use your already active TLS
					// connection.
					tls_conn = static_cast<Ref<StreamPeerTLS>>(connection);
					if (tls_conn.is_null()) {
						close();
						status = STATUS_TLS_HANDSHAKE_ERROR;
						return ERR_CANT_CONNECT;
					}

					tls_conn->poll(); // Try to finish the handshake.
				}

				if (tls_conn->get_status() == StreamPeerTLS::STATUS_CONNECTED) {
					// Handshake has been successful.
					handshaking = false;
					status = STATUS_CONNECTED;
					return OK;
				}
				else if (tls_conn->get_status() != StreamPeerTLS::STATUS_HANDSHAKING) {
					// Handshake has failed.
					close();
					status = STATUS_TLS_HANDSHAKE_ERROR;
					return ERR_CANT_CONNECT;
				}
				// ... we will need to poll more for handshake to finish.
			}
			else {
				status = STATUS_CONNECTED;
			}
			return OK;
		} break;
		case StreamPeerSocket::STATUS_ERROR:
		case StreamPeerSocket::STATUS_NONE: {
			Error err = ERR_CANT_CONNECT;
			close();
			status = STATUS_CANT_CONNECT;
			return err;
		} break;
		}
	} break;
	case STATUS_BODY:
	case STATUS_CONNECTED: {
		// Check if we are still connected.
		if (tls_options.is_valid()) {
			Ref<StreamPeerTLS> tmp = connection;
			tmp->poll();
			if (tmp->get_status() != StreamPeerTLS::STATUS_CONNECTED) {
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
		}
		else if (tcp_connection->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			status = STATUS_CONNECTION_ERROR;
			return ERR_CONNECTION_ERROR;
		}
		// Connection established, requests can now be made.
		return OK;
	} break;
	case STATUS_REQUESTING: {
		if (request_buffer->get_available_bytes()) {
			int avail = request_buffer->get_available_bytes();
			int pos = request_buffer->get_position();
			const Vector<uint8_t> data = request_buffer->get_data_array();
			int wrote = 0;
			Error err;
			if (blocking) {
				err = connection->put_data(data.ptr() + pos, avail);
				wrote += avail;
			}
			else {
				err = connection->put_partial_data(data.ptr() + pos, avail, wrote);
			}
			if (err != OK) {
				close();
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
			pos += wrote;
			request_buffer->seek(pos);
			if (avail - wrote > 0) {
				return OK;
			}
			request_buffer->clear();
		}
		while (true) {
			uint8_t byte;
			int rec = 0;
			Error err = _get_http_data(&byte, 1, rec);
			if (err != OK) {
				close();
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}

			if (rec == 0) {
				return OK; // Still requesting, keep trying!
			}

			response_str.push_back(byte);
			int rs = response_str.size();
			if ((rs >= 2 && response_str[rs - 2] == '\n' && response_str[rs - 1] == '\n') ||
				(rs >= 4 && response_str[rs - 4] == '\r' && response_str[rs - 3] == '\n' &&
					response_str[rs - 2] == '\r' && response_str[rs - 1] == '\n')) {
				// End of response, parse.
				response_str.push_back(0);
				String response =
					String::utf8((const char*)response_str.ptr(), response_str.size());
				Vector<String> responses = response.split("\n");
				body_size = -1;
				chunked = false;
				body_left = 0;
				chunk_left = 0;
				chunk_trailer_part = false;
				read_until_eof = false;
				response_str.clear();
				response_headers.clear();
				response_num = RESPONSE_OK;

				// Per the HTTP 1.1 spec, keep-alive is the default.
				// Not following that specification breaks standard implementations.
				// Broken web servers should be fixed.
				bool keep_alive = true;

				for (int i = 0; i < responses.size(); i++) {
					String header = responses[i].strip_edges();
					String s = header.to_lower();
					if (s.length() == 0) {
						continue;
					}
					if (s.begins_with("content-length:")) {
						body_size = s.substr(s.find_char(':') + 1).strip_edges().to_int();
						body_left = body_size;
					}
					else if (s.begins_with("transfer-encoding:")) {
						String encoding = header.substr(header.find_char(':') + 1).strip_edges();
						if (encoding == "chunked") {
							chunked = true;
						}
					}
					else if (s.begins_with("connection: close")) {
						keep_alive = false;
					}

					if (i == 0 && responses[i].begins_with("HTTP")) {
						String num = responses[i].get_slicec(' ', 1);
						response_num = num.to_int();
					}
					else {
						response_headers.push_back(header);
					}
				}

				// This is a HEAD request, we won't receive anything.
				if (head_request) {
					body_size = 0;
					body_left = 0;
				}

				if (body_size != -1 || chunked) {
					status = STATUS_BODY;
				}
				else if (!keep_alive) {
					read_until_eof = true;
					status = STATUS_BODY;
				}
				else {
					status = STATUS_CONNECTED;
				}
				return OK;
			}
		}
	} break;
	case STATUS_DISCONNECTED: {
		return ERR_UNCONFIGURED;
	} break;
	case STATUS_CONNECTION_ERROR:
	case STATUS_TLS_HANDSHAKE_ERROR: {
		return ERR_CONNECTION_ERROR;
	} break;
	case STATUS_CANT_CONNECT: {
		return ERR_CANT_CONNECT;
	} break;
	case STATUS_CANT_RESOLVE: {
		return ERR_CANT_RESOLVE;
	} break;
	}

	return OK;
}

int64_t HTTPClient::get_response_body_length() const { return body_size; }

Vector<uint8_t> HTTPClient::read_response_body_chunk()
{
	ERR_FAIL_COND_V(status != STATUS_BODY, Vector<uint8_t>());

	Vector<uint8_t> ret;
	Error err = OK;

	if (chunked) {
		while (true) {
			if (chunk_trailer_part) {
				// We need to consume the trailer part too or keep-alive will break.
				uint8_t b;
				int rec = 0;
				err = _get_http_data(&b, 1, rec);

				if (rec == 0) {
					break;
				}

				chunk.push_back(b);
				int cs = chunk.size();
				if ((cs >= 2 && chunk[cs - 2] == '\r' && chunk[cs - 1] == '\n')) {
					if (cs == 2) {
						// Finally over.
						chunk_trailer_part = false;
						status = STATUS_CONNECTED;
						chunk.clear();
						break;
					}
					else {
						// We do not process nor return the trailer data.
						chunk.clear();
					}
				}
			}
			else if (chunk_left == 0) {
				// Reading length.
				uint8_t b;
				int rec = 0;
				err = _get_http_data(&b, 1, rec);

				if (rec == 0) {
					break;
				}

				chunk.push_back(b);

				if (chunk.size() > 32) {
					ERR_PRINT("HTTP Invalid chunk hex len");
					status = STATUS_CONNECTION_ERROR;
					break;
				}

				if (chunk.size() > 2 && chunk[chunk.size() - 2] == '\r' &&
					chunk[chunk.size() - 1] == '\n') {
					int len = 0;
					for (int i = 0; i < chunk.size() - 2; i++) {
						char c = chunk[i];
						int v = 0;
						if (is_digit(c)) {
							v = c - '0';
						}
						else if (c >= 'a' && c <= 'f') {
							v = c - 'a' + 10;
						}
						else if (c >= 'A' && c <= 'F') {
							v = c - 'A' + 10;
						}
						else {
							ERR_PRINT("HTTP Chunk len not in hex!!");
							status = STATUS_CONNECTION_ERROR;
							break;
						}
						len <<= 4;
						len |= v;
						if (len > (1 << 24)) {
							ERR_PRINT("HTTP Chunk too big!! >16mb");
							status = STATUS_CONNECTION_ERROR;
							break;
						}
					}

					if (len == 0) {
						// End reached!
						chunk_trailer_part = true;
						chunk.clear();
						break;
					}

					chunk_left = len + 2;
					chunk.resize(chunk_left);
				}
			}
			else {
				int rec = 0;
				err = _get_http_data(&chunk.write[chunk.size() - chunk_left], chunk_left, rec);
				if (rec == 0) {
					break;
				}
				chunk_left -= rec;

				if (chunk_left == 0) {
					const int chunk_size = chunk.size();
					if (chunk[chunk_size - 2] != '\r' || chunk[chunk_size - 1] != '\n') {
						ERR_PRINT("HTTP Invalid chunk terminator (not \\r\\n)");
						status = STATUS_CONNECTION_ERROR;
						break;
					}

					ret.resize(chunk_size - 2);
					uint8_t* w = ret.ptrw();
					memcpy(w, chunk.ptr(), chunk_size - 2);
					chunk.clear();
				}

				break;
			}
		}
	}
	else {
		int to_read = !read_until_eof ? MIN(body_left, read_chunk_size) : read_chunk_size;
		ret.resize(to_read);
		int _offset = 0;
		while (to_read > 0) {
			int rec = 0;
			{
				uint8_t* w = ret.ptrw();
				err = _get_http_data(w + _offset, to_read, rec);
			}
			if (rec <= 0) { // Ended up reading less.
				ret.resize(_offset);
				break;
			}
			else {
				_offset += rec;
				to_read -= rec;
				if (!read_until_eof) {
					body_left -= rec;
				}
			}
			if (err != OK) {
				ret.resize(_offset);
				break;
			}
		}
	}

	if (err != OK) {
		close();

		if (err == ERR_FILE_EOF) {
			status = STATUS_DISCONNECTED; // Server disconnected.
		}
		else {
			status = STATUS_CONNECTION_ERROR;
		}
	}
	else if (body_left == 0 && !chunked && !read_until_eof) {
		status = STATUS_CONNECTED;
	}

	return ret;
}

HTTPClient::Status HTTPClient::get_status() const { return status; }

void HTTPClient::set_blocking_mode(bool p_enable) { blocking = p_enable; }

bool HTTPClient::is_blocking_mode_enabled() const { return blocking; }

Error HTTPClient::_get_http_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	if (blocking) {
		// We can't use StreamPeer.get_data, since when reaching EOF we will get an
		// error without knowing how many bytes we received.
		Error err = ERR_FILE_EOF;
		int read = 0;
		int left = p_bytes;
		r_received = 0;
		while (left > 0) {
			err = connection->get_partial_data(p_buffer + r_received, left, read);
			if (err == OK) {
				r_received += read;
			}
			else if (err == ERR_FILE_EOF) {
				r_received += read;
				return err;
			}
			else {
				return err;
			}
			left -= read;
		}
		return err;
	}
	else {
		return connection->get_partial_data(p_buffer, p_bytes, r_received);
	}
}

void HTTPClient::set_read_chunk_size(int p_size)
{
	ERR_FAIL_COND(p_size < 256 || p_size > (1 << 24));
	read_chunk_size = p_size;
}

int HTTPClient::get_read_chunk_size() const { return read_chunk_size; }

void HTTPClient::set_http_proxy(const String& p_host, int p_port)
{
	if (p_host.is_empty() || p_port == -1) {
		http_proxy_host = "";
		http_proxy_port = -1;
	}
	else {
		http_proxy_host = p_host;
		http_proxy_port = p_port;
	}
}

void HTTPClient::set_https_proxy(const String& p_host, int p_port)
{
	if (p_host.is_empty() || p_port == -1) {
		https_proxy_host = "";
		https_proxy_port = -1;
	}
	else {
		https_proxy_host = p_host;
		https_proxy_port = p_port;
	}
}

HTTPClient::HTTPClient()
{
	tcp_connection.instantiate();
	request_buffer.instantiate();
}

HTTPClient::~HTTPClient() {}

#endif // WEB_ENABLED

// ============================================================================
// Merged from core/io/ip.cpp
// ============================================================================

#include "core/os/semaphore.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "ip.h"

/************* RESOLVER ******************/

struct _IP_ResolverPrivate
{
	struct QueueItem
	{
		SafeNumeric<IP::ResolverStatus> status;

		List<IPAddress> response;

		String hostname;
		IP::Type type;

		void clear()
		{
			status.set(IP::RESOLVER_STATUS_NONE);
			response.clear();
			type = IP::TYPE_NONE;
			hostname = "";
		}

		QueueItem() { clear(); }
	};

	QueueItem queue[IP::RESOLVER_MAX_QUERIES];

	IP::ResolverID find_empty_id() const
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() == IP::RESOLVER_STATUS_NONE) {
				return i;
			}
		}
		return IP::RESOLVER_INVALID_ID;
	}

	Mutex mutex;
	Semaphore sem;

	Thread thread;
	SafeFlag thread_abort;

	void resolve_queues()
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}

			MutexLock lock(mutex);
			List<IPAddress> response;
			String hostname = queue[i].hostname;
			IP::Type type = queue[i].type;
			lock.temp_unlock();

			// We should not lock while resolving the hostname,
			// only when modifying the queue.
			IP::_resolve_hostname(response, hostname, type);

			lock.temp_relock();
			// Could have been completed by another function, or deleted.
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (response.size()) {
				String key = get_cache_key(hostname, type);
				cache[key] = response;
			}
			queue[i].response = response;
			queue[i].status.set(
				response.is_empty() ? IP::RESOLVER_STATUS_ERROR : IP::RESOLVER_STATUS_DONE);
		}
	}

	static void _thread_function(void* p_self)
	{
		_IP_ResolverPrivate* ipr = static_cast<_IP_ResolverPrivate*>(p_self);

		while (!ipr->thread_abort.is_set()) {
			ipr->sem.wait();
			ipr->resolve_queues();
		}
	}

	HashMap<String, List<IPAddress>> cache;

	static String get_cache_key(const String& p_hostname, IP::Type p_type)
	{
		return itos(p_type) + p_hostname;
	}
};

IPAddress IP::resolve_hostname(const String& p_hostname, IP::Type p_type)
{
	const Vector<String> addresses = resolve_hostname_addresses(p_hostname, p_type);
	return addresses.size() ? (IPAddress)addresses[0] : IPAddress();
}

Vector<String> IP::resolve_hostname_addresses(const String& p_hostname, Type p_type)
{
	ERR_FAIL_NULL_V(data, Vector<String>());
	_IP_ResolverPrivate* resolver = data->resolver;

	List<IPAddress> res;
	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);

	{
		MutexLock lock(resolver->mutex);
		if (resolver->cache.has(key)) {
			res = resolver->cache[key];
		}
		else {
			// This should be run unlocked so the resolver thread can keep resolving
			// other requests.
			lock.temp_unlock();
			_resolve_hostname(res, p_hostname, p_type);
			lock.temp_relock();
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (res.size()) {
				resolver->cache[key] = res;
			}
		}
	}

	Vector<String> result;
	for (const IPAddress& E : res) {
		result.push_back(String(E));
	}
	return result;
}

IP::ResolverID IP::resolve_hostname_queue_item(const String& p_hostname, IP::Type p_type)
{
	ERR_FAIL_NULL_V(data, RESOLVER_INVALID_ID);
	_IP_ResolverPrivate* resolver = data->resolver;

	MutexLock lock(resolver->mutex);

	ResolverID id = resolver->find_empty_id();

	if (id == RESOLVER_INVALID_ID) {
		WARN_PRINT("Out of resolver queries");
		return id;
	}

	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);
	resolver->queue[id].hostname = p_hostname;
	resolver->queue[id].type = p_type;
	if (resolver->cache.has(key)) {
		resolver->queue[id].response = resolver->cache[key];
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_DONE);
	}
	else {
		resolver->queue[id].response = List<IPAddress>();
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_WAITING);
		if (resolver->thread.is_started()) {
			resolver->sem.post();
		}
		else {
			resolver->resolve_queues();
		}
	}

	return id;
}

IP::ResolverStatus IP::get_resolve_item_status(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IP::RESOLVER_STATUS_NONE);
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IP::RESOLVER_STATUS_NONE,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	IP::ResolverStatus res = data->resolver->queue[p_id].status.get();
	if (res == IP::RESOLVER_STATUS_NONE) {
		ERR_PRINT("Condition status == IP::RESOLVER_STATUS_NONE");
		return IP::RESOLVER_STATUS_NONE;
	}
	return res;
}

IPAddress IP::get_resolve_item_address(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IPAddress());
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IPAddress(),
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (resolver->queue[p_id].status.get() != IP::RESOLVER_STATUS_DONE) {
		ERR_PRINT(vformat("Resolve of '%s' didn't complete yet.", resolver->queue[p_id].hostname));
		return IPAddress();
	}

	List<IPAddress> res(resolver->queue[p_id].response);

	for (const IPAddress& E : res) {
		if (E.is_valid()) {
			return E;
		}
	}
	return IPAddress();
}

void IP::erase_resolve_item(ResolverID p_id)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_INDEX_MSG(p_id, IP::RESOLVER_MAX_QUERIES,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	data->resolver->queue[p_id].status.set(IP::RESOLVER_STATUS_NONE);
}

void IP::clear_cache(const String& p_hostname)
{
	ERR_FAIL_NULL(data);
	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (p_hostname.is_empty()) {
		resolver->cache.clear();
	}
	else {
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_NONE));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV4));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV6));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_ANY));
	}
}

Vector<String> IP::_get_local_addresses()
{
	Vector<String> addresses;
	List<IPAddress> ip_addresses;
	get_local_addresses(&ip_addresses);
	for (const IPAddress& E : ip_addresses) {
		addresses.push_back(String(E));
	}

	return addresses;
}

void IP::get_local_addresses(List<IPAddress>* r_addresses)
{
	HashMap<String, Interface_Info> interfaces;
	get_local_interfaces(&interfaces);
	for (const KeyValue<String, Interface_Info>& E : interfaces) {
		for (const IPAddress& F : E.value.ip_addresses) {
			r_addresses->push_front(F);
		}
	}
}

void IP::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->resolver = memnew(_IP_ResolverPrivate);
	data->resolver->thread_abort.clear();
	data->resolver->thread.start(_IP_ResolverPrivate::_thread_function, data->resolver);
}

void IP::finalize()
{
	if (data == nullptr) {
		return;
	}
	if (data->resolver != nullptr) {
		data->resolver->thread_abort.set();
		data->resolver->sem.post();
		if (data->resolver->thread.is_started()) {
			data->resolver->thread.wait_to_finish();
		}
		memdelete(data->resolver);
		data->resolver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void IP::reset_backend()
{
	_resolve_hostname = &_default_resolve_hostname;
	get_local_interfaces = &_default_get_local_interfaces;
}

// ==========================================================================
// ip_address.cpp
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

#include "ip_address.h"
/*
IPAddress::operator Variant() const {
	return operator String();
}*/

IPAddress::operator String() const {
	if (wildcard) {
		return "*";
	}

	if (!valid) {
		return "";
	}

	if (is_ipv4()) {
		// IPv4 address mapped to IPv6.
		return itos(field8[12]) + "." + itos(field8[13]) + "." + itos(field8[14]) + "." + itos(field8[15]);
	}
	String ret;
	for (int i = 0; i < 8; i++) {
		if (i > 0) {
			ret = ret + ":";
		}
		uint16_t num = (field8[i * 2] << 8) + field8[i * 2 + 1];
		ret = ret + String::num_int64(num, 16);
	}

	return ret;
}

bool IPAddress::_parse_ipv6(const String &p_string, IPAddress &r_ip) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	int shift = -1;
	for (int i = 0; i < len; i++) {
		for (int j = i; j < len; j++) {
			char32_t c = buf[j];
			if (c == ':') {
				if (j + 1 == len) {
					return false; // Can't end with a column (unless part of shortening).
				}
				if (buf[j + 1] == ':') {
					if (shift > -1) {
						return false; // Only one shortening allowed.
					} else if (j == 0) {
						shift = cur;
					} else {
						shift = cur + 1;
					}
					j++;
				} else if (i == j) {
					return false; // Stray column.
				}
				i = j;
				break;
			}
			if (j - i > 3) {
				return false;
			}
			if (c >= '0' && c <= '9') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= c - '0';
			} else if (c >= 'a' && c <= 'f') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'a');
			} else if (c >= 'A' && c <= 'F') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'A');
			} else if (c == '.') {
				// IPv4 mapped IPv6 (e.g. "::FFFF:127.0.0.1").
				if (cur < 1 || r_ip.field16[cur - 1] != 0xFFFF) {
					return false; // IPv6 part must end with FFFF.
				}
				if (shift < 0 && cur != 6) {
					return false; // Needs 5 zeros, and FFFF "0:0:0:0:0:FFFF:127.0.0.1".
				}
				// Only empty bytes allowed before FFFF.
				r_ip.field16[cur] = 0;
				r_ip.field16[cur - 1] = 0;
				while (cur > 0) {
					cur--;
					if (r_ip.field16[cur] != 0) {
						return false;
					}
				}
				r_ip.field16[5] = 0xFFFF;
				return _parse_ipv4(p_string, i, &r_ip.field8[12]);
			} else {
				return false;
			}
			if (j + 1 == len) {
				i = j;
			}
		}
		r_ip.field16[cur] = BSWAP16(r_ip.field16[cur]);
		cur += 1;
		if (cur > 8 || (cur == 8 && i + 1 != len)) {
			return false;
		}
	}
	if (shift < 0) {
		return cur == 8; // Should have parsed 8 16-bits ints.
	} else if (shift > 7) {
		return false; // Can't shorten more than this.
	} else if (shift == cur || cur == 8) {
		return true; // Nothing to do (fields are assumed zeroized).
	}
	// Shift bytes.
	int pad = 8 - cur;
	int blank_end = shift + pad;
	for (int i = 7; i > shift; i--) {
		if (i < blank_end) {
			r_ip.field16[i] = 0;
		} else {
			r_ip.field16[i] = r_ip.field16[i - pad];
			r_ip.field16[i - pad] = 0;
		}
	}
	return true;
}

bool IPAddress::_parse_ipv4(const String &p_string, int p_start, uint8_t *r_dest) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	uint16_t next = 0;
	bool parsed = false;
	for (int i = p_start; i < len; i++) {
		char32_t c = buf[i];
		if (c == '.') {
			if (!parsed) {
				return false;
			}
			parsed = false;
			r_dest[cur] = next;
			next = 0;
			cur++;
			if (cur > 3) {
				return false;
			}
		} else if (c >= '0' && c <= '9') {
			parsed = true;
			next *= 10;
			next += c - '0';
			if (next > 255) {
				return false;
			}
		} else {
			return false; // Invalid char.
		}
	}
	if (!parsed) {
		return false;
	}
	r_dest[cur] = next;
	return parsed && cur == 3;
}

void IPAddress::clear() {
	memset(&field8[0], 0, sizeof(field8));
	valid = false;
	wildcard = false;
}

bool IPAddress::is_ipv4() const {
	return (field32[0] == 0 && field32[1] == 0 && field16[4] == 0 && field16[5] == 0xffff);
}

const uint8_t *IPAddress::get_ipv4() const {
	ERR_FAIL_COND_V_MSG(!is_ipv4(), &(field8[12]), "IPv4 requested, but current IP is IPv6."); // Not the correct IPv4 (it's an IPv6), but we don't want to return a null pointer risking an engine crash.
	return &(field8[12]);
}

void IPAddress::set_ipv4(const uint8_t *p_ip) {
	clear();
	valid = true;
	field16[5] = 0xffff;
	field32[3] = *((const uint32_t *)p_ip);
}

const uint8_t *IPAddress::get_ipv6() const {
	return field8;
}

void IPAddress::set_ipv6(const uint8_t *p_buf) {
	clear();
	valid = true;
	for (int i = 0; i < 16; i++) {
		field8[i] = p_buf[i];
	}
}

bool IPAddress::is_valid_ip_address(const String &p_string) {
	IPAddress addr;
	if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		return _parse_ipv6(p_string, addr);
	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) { // Try IPv4.
		return _parse_ipv4(p_string, 0, &addr.field8[12]);
	}
	return false;
}

IPAddress::IPAddress(const String &p_string) {
	clear();

	if (p_string == "*") {
		// Wildcard (not a valid IP).
		wildcard = true;

	} else if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		// IPv6.
		valid = _parse_ipv6(p_string, *this);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv6 address: " + p_string);

	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) {
		// IPv4 (mapped to IPv6 internally).
		field16[5] = 0xffff;
		valid = _parse_ipv4(p_string, 0, &field8[12]);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv4 address: " + p_string);

	} else {
		ERR_PRINT("Invalid IP address.");
	}
}

_FORCE_INLINE_ static void _32_to_buf(uint8_t *p_dst, uint32_t p_n) {
	p_dst[0] = (p_n >> 24) & 0xff;
	p_dst[1] = (p_n >> 16) & 0xff;
	p_dst[2] = (p_n >> 8) & 0xff;
	p_dst[3] = (p_n >> 0) & 0xff;
}

IPAddress::IPAddress(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d, bool p_is_v6) {
	clear();
	valid = true;
	if (!p_is_v6) {
		// Mapped to IPv6.
		field16[5] = 0xffff;
		field8[12] = p_a;
		field8[13] = p_b;
		field8[14] = p_c;
		field8[15] = p_d;
	} else {
		_32_to_buf(&field8[0], p_a);
		_32_to_buf(&field8[4], p_b);
		_32_to_buf(&field8[8], p_c);
		_32_to_buf(&field8[12], p_d);
	}
}

// ==========================================================================
// net_socket.cpp
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

#include "net_socket.h"

Ref<NetSocket> (*NetSocket::_create)() = nullptr;

Ref<NetSocket> NetSocket::create() {
	if (_create) {
		return _create();
	}

	ERR_PRINT("Unable to create network socket, platform not supported");
	return nullptr;
}

// ==========================================================================
// remote_filesystem_client.cpp
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

#include "core/os/os.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer_socket.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"

#define FILESYSTEM_CACHE_VERSION 1
#define FILESYSTEM_PROTOCOL_VERSION 1
#define PASSWORD_LENGTH 32

#define FILES_SUBFOLDER "remote_filesystem_files"
#define FILES_CACHE_FILE "remote_filesystem.cache"

Vector<RemoteFilesystemClient::FileCache> RemoteFilesystemClient::_load_cache_file() {
	Ref<FileAccess> fa = FileAccess::open(cache_path.path_join(FILES_CACHE_FILE), FileAccess::READ);
	if (fa.is_null()) {
		return Vector<FileCache>(); // No cache, return empty
	}

	int version = fa->get_line().to_int();
	if (version != FILESYSTEM_CACHE_VERSION) {
		return Vector<FileCache>(); // Version mismatch, ignore everything.
	}

	String file_path = cache_path.path_join(FILES_SUBFOLDER);

	Vector<FileCache> file_cache;

	while (!fa->eof_reached()) {
		String l = fa->get_line();
		Vector<String> fields = l.split("::");
		if (fields.size() != 3) {
			break;
		}
		FileCache fc;
		fc.path = fields[0];
		fc.server_modified_time = fields[1].to_int();
		fc.modified_time = fields[2].to_int();

		String full_path = file_path.path_join(fc.path);
		if (!FileAccess::exists(full_path)) {
			continue; // File is gone.
		}

		if (FileAccess::get_modified_time(full_path) != fc.modified_time) {
			DirAccess::remove_absolute(full_path); // Take the chance to remove this file and assume we no longer have it.
			continue;
		}

		file_cache.push_back(fc);
	}

	return file_cache;
}

Error RemoteFilesystemClient::_store_file(const String &p_path, const LocalVector<uint8_t> &p_file, uint64_t &r_modified_time) {
	r_modified_time = 0;
	String full_path = cache_path.path_join(FILES_SUBFOLDER).path_join(p_path);
	String base_file_dir = full_path.get_base_dir();

	if (!validated_directories.has(base_file_dir)) {
		// Verify that path exists before writing file, but only verify once for performance.
		DirAccess::make_dir_recursive_absolute(base_file_dir);
		validated_directories.insert(base_file_dir);
	}

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open file for writing to remote filesystem cache: '%s'.", p_path));
	f->store_buffer(p_file.ptr(), p_file.size());
	Error err = f->get_error();
	if (err) {
		return err;
	}
	f.unref(); // Unref to ensure file is not locked and modified time can be obtained.

	r_modified_time = FileAccess::get_modified_time(full_path);
	return OK;
}

Error RemoteFilesystemClient::_remove_file(const String &p_path) {
	return DirAccess::remove_absolute(cache_path.path_join(FILES_SUBFOLDER).path_join(p_path));
}
Error RemoteFilesystemClient::_store_cache_file(const Vector<FileCache> &p_cache) {
	String full_path = cache_path.path_join(FILES_CACHE_FILE);
	String base_file_dir = full_path.get_base_dir();
	Error err = DirAccess::make_dir_recursive_absolute(base_file_dir);
	ERR_FAIL_COND_V_MSG(err != OK && err != ERR_ALREADY_EXISTS, err, vformat("Unable to create base directory to store cache file: '%s'.", base_file_dir));

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open the remote cache file for writing: '%s'.", full_path));
	f->store_line(itos(FILESYSTEM_CACHE_VERSION));
	for (int i = 0; i < p_cache.size(); i++) {
		String l = p_cache[i].path + "::" + itos(p_cache[i].server_modified_time) + "::" + itos(p_cache[i].modified_time);
		f->store_line(l);
	}
	return OK;
}

Error RemoteFilesystemClient::synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	Error err = _synchronize_with_server(p_host, p_port, p_password, r_cache_path);
	// Ensure no memory is kept
	validated_directories.reset();
	cache_path = String();
	return err;
}

void RemoteFilesystemClient::_update_cache_path(String &r_cache_path) {
	r_cache_path = cache_path.path_join(FILES_SUBFOLDER);
}

Error RemoteFilesystemClient::_synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	cache_path = r_cache_path;
	{
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		dir->change_dir(cache_path);
		cache_path = dir->get_current_dir();
	}

	Ref<StreamPeerSocket> tcp_client;
	tcp_client.instantiate();

	IPAddress ip = p_host.is_valid_ip_address() ? IPAddress(p_host) : IP::resolve_hostname(p_host);
	ERR_FAIL_COND_V_MSG(!ip.is_valid(), ERR_INVALID_PARAMETER, vformat("Unable to resolve remote filesystem server hostname: '%s'.", p_host));
	print_verbose(vformat("Remote Filesystem: Connecting to host %s, port %d.", ip, p_port));
	Error err = tcp_client->connect_to_host(ip, p_port);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Unable to open connection to remote file server (%s, port %d) failed.", String(p_host), p_port));

	while (tcp_client->get_status() == StreamPeerSocket::STATUS_CONNECTING) {
		tcp_client->poll();
		OS::delay_usec(100);
	}

	if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		ERR_FAIL_V_MSG(ERR_CANT_CONNECT, vformat("Connection to remote file server (%s, port %d) failed.", String(p_host), p_port));
	}

	// Connection OK, now send the current file state.
	print_verbose("Remote Filesystem: Connection OK.");

	// Header (GRFS) - Godot Remote File System
	print_verbose("Remote Filesystem: Sending header");
	tcp_client->put_u8('G');
	tcp_client->put_u8('R');
	tcp_client->put_u8('F');
	tcp_client->put_u8('S');
	// Protocol version
	tcp_client->put_32(FILESYSTEM_PROTOCOL_VERSION);
	print_verbose("Remote Filesystem: Sending password");
	uint8_t password[PASSWORD_LENGTH]; // Send fixed size password, since it's easier and safe to validate.
	for (int i = 0; i < PASSWORD_LENGTH; i++) {
		if (i < p_password.length()) {
			password[i] = p_password[i];
		} else {
			password[i] = 0;
		}
	}
	tcp_client->put_data(password, PASSWORD_LENGTH);
	print_verbose("Remote Filesystem: Tags.");
	Vector<String> tags;
	{
		tags.push_back(OS::get_identifier());
		switch (OS::get_preferred_texture_format()) {
			case OS::PREFERRED_TEXTURE_FORMAT_S3TC_BPTC: {
				tags.push_back("bptc");
				tags.push_back("s3tc");
			} break;
			case OS::PREFERRED_TEXTURE_FORMAT_ETC2_ASTC: {
				tags.push_back("etc2");
				tags.push_back("astc");
			} break;
		}
	}

	tcp_client->put_32(tags.size());
	for (int i = 0; i < tags.size(); i++) {
		tcp_client->put_utf8_string(tags[i]);
	}
	// Size of compressed list of files
	print_verbose("Remote Filesystem: Sending file list");

	Vector<FileCache> file_cache = _load_cache_file();

	// Encode file cache to send it via network.
	Vector<uint8_t> file_cache_buffer;
	if (file_cache.size()) {
		StringBuilder sbuild;
		for (int64_t i = 0; i < file_cache.size(); i++) {
			sbuild.append(file_cache[i].path);
			sbuild.append("::");
			sbuild.append(itos(file_cache[i].server_modified_time));
			sbuild.append("\n");
		}
		String s = sbuild.as_string();
		CharString cs = s.utf8();
		file_cache_buffer.resize(Compression::get_max_compressed_buffer_size(cs.length(), Compression::MODE_ZSTD));
		const int64_t res_len = Compression::compress(file_cache_buffer.ptrw(), (const uint8_t *)cs.ptr(), cs.length(), Compression::MODE_ZSTD);
		file_cache_buffer.resize(res_len);

		tcp_client->put_32(cs.length()); // Size of buffer uncompressed
		tcp_client->put_32(file_cache_buffer.size()); // Size of buffer compressed
		tcp_client->put_data(file_cache_buffer.ptr(), file_cache_buffer.size()); // Buffer
	} else {
		tcp_client->put_32(0); // No file cache buffer
	}

	tcp_client->poll();
	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected after sending header.");

	uint32_t file_count = tcp_client->get_u32();

	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file list");

	LocalVector<uint8_t> file_buffer;

	Vector<FileCache> temp_file_cache;

	HashSet<String> files_processed;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = tcp_client->get_utf8_string();
		ERR_FAIL_COND_V_MSG(file == String(), ERR_CONNECTION_ERROR, "Invalid file name received from remote filesystem.");
		uint64_t server_modified_time = tcp_client->get_u64();
		ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file info.");

		FileCache fc;
		fc.path = file;
		fc.server_modified_time = server_modified_time;
		temp_file_cache.push_back(fc);

		files_processed.insert(file);
	}

	Vector<FileCache> new_file_cache;

	// Get the actual files. As a robustness measure, if the connection is interrupted here, any file not yet received will be considered removed.
	// Since the file changed anyway, this makes it the easiest way to keep robustness.

	bool server_disconnected = false;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = temp_file_cache[i].path;

		if (temp_file_cache[i].server_modified_time == 0 || server_disconnected) {
			// File was removed, or server disconnected before transferring it. Since it's no longer valid, remove anyway.
			_remove_file(file);
			continue;
		}

		uint64_t file_size = tcp_client->get_u64();
		file_buffer.resize(file_size);

		err = tcp_client->get_data(file_buffer.ptr(), file_size);
		if (err != OK) {
			ERR_PRINT(vformat("Error retrieving file from remote filesystem: '%s'.", file));
			server_disconnected = true;
		}

		if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			// Early disconnect, stop accepting files.
			server_disconnected = true;
		}

		if (server_disconnected) {
			// No more server, transfer is invalid, remove this file.
			_remove_file(file);
			continue;
		}

		uint64_t modified_time = 0;
		err = _store_file(file, file_buffer, modified_time);
		if (err != OK) {
			server_disconnected = true;
			continue;
		}
		FileCache fc = temp_file_cache[i];
		fc.modified_time = modified_time;
		new_file_cache.push_back(fc);
	}

	print_verbose("Remote Filesystem: Updating the cache file.");

	// Go through the list of local files read initially (file_cache) and see which ones are
	// unchanged (not sent again from the server).
	// These need to be re-saved in the new list (new_file_cache).

	for (int i = 0; i < file_cache.size(); i++) {
		if (files_processed.has(file_cache[i].path)) {
			continue; // This was either added or removed, so skip.
		}
		new_file_cache.push_back(file_cache[i]);
	}

	err = _store_cache_file(new_file_cache);
	ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CANT_OPEN, "Error writing the remote filesystem file cache.");

	print_verbose("Remote Filesystem: Update success.");

	_update_cache_path(r_cache_path);
	return OK;
}
#ifdef FILESYSTEM_CACHE_VERSION
#undef FILESYSTEM_CACHE_VERSION
#endif
#ifdef FILESYSTEM_PROTOCOL_VERSION
#undef FILESYSTEM_PROTOCOL_VERSION
#endif
#ifdef PASSWORD_LENGTH
#undef PASSWORD_LENGTH
#endif
#ifdef FILES_SUBFOLDER
#undef FILES_SUBFOLDER
#endif
#ifdef FILES_CACHE_FILE
#undef FILES_CACHE_FILE
#endif

// ============================================================================
// Merged from core/io/packet_peer.cpp
// ============================================================================

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
#include "core/io/packet_peer_udp.h"
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

#include "core/io/packet_peer_udp.h"

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

#include "core/io/packet_peer_dtls.h"

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
