/**************************************************************************/
/*  stream_peer_tls.cpp                                                   */
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
