/**************************************************************************/
/*  socket_server.h                                                       */
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

#include "core/io/ip_address.h"
#include "core/io/net_socket.h"
#include "core/io/stream_peer_socket.h"

class SocketServer final : public RefCounted
{
private:
	enum
	{
		MAX_PENDING_CONNECTIONS = 8,
	};

	StreamPeerSocket::Domain domain = StreamPeerSocket::DOMAIN_TCP;
	Ref<NetSocket> _sock;

	Error _listen(const NetSocket::Address& p_addr);

public:
	Error listen(uint16_t p_port, const IPAddress& p_bind_address = IPAddress("*"));
	Error listen_path(const String& p_path);
	Error listen(const String& p_path) { return listen_path(p_path); }

	int get_local_port() const;

	bool is_listening() const;
	bool is_connection_available() const;
	Ref<StreamPeerSocket> take_connection();
	Ref<StreamPeerSocket> take_socket_connection() { return take_connection(); }

	void stop(); // Stop listening

	SocketServer();
	~SocketServer() override;
};
