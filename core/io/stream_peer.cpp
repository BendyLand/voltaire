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
#include "core/io/stream_peer_gzip.h"
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
