/**************************************************************************/
/*  stream_peer_socket.cpp                                                */
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
