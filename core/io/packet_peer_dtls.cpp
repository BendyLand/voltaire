/**************************************************************************/
/*  packet_peer_dtls.cpp                                                  */
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
