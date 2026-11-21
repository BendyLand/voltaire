/**************************************************************************/
/*  stream_peer_tls.h                                                     */
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
#include "core/io/stream_peer.h"

class StreamPeerTLS : public StreamPeer
{
public:
	struct Data;

private:
	Data* data = nullptr;

public:
	enum Status
	{
		STATUS_DISCONNECTED,
		STATUS_HANDSHAKING,
		STATUS_CONNECTED,
		STATUS_ERROR,
		STATUS_ERROR_HOSTNAME_MISMATCH
	};

	void poll();
	Error accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options);
	Error connect_to_stream(Ref<StreamPeer> p_base, const String& p_common_name,
		Ref<TLSOptions> p_options = Ref<TLSOptions>());
	Status get_status() const;
	Ref<StreamPeer> get_stream() const;

	void disconnect_from_stream();

	virtual Error put_data(const uint8_t* p_data, int p_bytes) override;
	virtual Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;

	virtual Error get_data(uint8_t* p_buffer, int p_bytes) override;
	virtual Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;

	virtual int get_available_bytes() const override;

	static StreamPeerTLS* create(bool p_notify_postinitialize = true);
	static bool is_available();

	StreamPeerTLS();
	~StreamPeerTLS();
};
