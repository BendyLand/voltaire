/**************************************************************************/
/*  multiplayer_api.cpp                                                   */
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

#include "multiplayer_api.h"

#include "core/io/marshalls.h"
#include "modules/modules_enabled.gen.h"

String MultiplayerAPI::default_interface;

void MultiplayerAPI::set_default_interface(const String& p_interface)
{
	default_interface = String(p_interface);
}

String MultiplayerAPI::get_default_interface() { return default_interface; }

Ref<MultiplayerAPI> MultiplayerAPI::create_default_interface()
{
	return Ref<MultiplayerAPI>(memnew(MultiplayerAPI));
}

#ifndef MODULE_MULTIPLAYER_ENABLED

MultiplayerAPI::MultiplayerAPI() {}

MultiplayerAPI::~MultiplayerAPI() {}

int MultiplayerAPI::get_unique_id() { return 0; }

Vector<int> MultiplayerAPI::get_peer_ids() { return Vector<int>(); }

int MultiplayerAPI::get_remote_sender_id() const { return 0; }

Error MultiplayerAPI::poll() { return OK; }

void MultiplayerAPI::set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer) {}

Ref<MultiplayerPeer> MultiplayerAPI::get_multiplayer_peer() { return Ref<MultiplayerPeer>(); }

void MultiplayerAPI::clear() {}

void MultiplayerAPI::set_root_path(const NodePath& p_path) {}

NodePath MultiplayerAPI::get_root_path() const { return NodePath(); }

void MultiplayerAPI::disconnect_peer(int p_id) {}

Error MultiplayerAPI::send_auth(int p_to, Vector<uint8_t> p_bytes) { return ERR_UNCONFIGURED; }

Error MultiplayerAPI::complete_auth(int p_peer) { return ERR_UNCONFIGURED; }

void MultiplayerAPI::set_auth_timeout(double p_timeout) {}

double MultiplayerAPI::get_auth_timeout() const { return 0.0; }

Vector<int> MultiplayerAPI::get_authenticating_peer_ids() { return Vector<int>(); }

Error MultiplayerAPI::send_command(int p_to, const uint8_t* p_packet, int p_packet_len)
{
	return ERR_UNCONFIGURED;
}

Error MultiplayerAPI::send_bytes(
	Vector<uint8_t> p_data, int p_to, MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	return ERR_UNCONFIGURED;
}

HashSet<int> MultiplayerAPI::get_connected_peers() const { return HashSet<int>(); }

void MultiplayerAPI::set_remote_sender_override(int p_id) {}

void MultiplayerAPI::set_refuse_new_connections(bool p_refuse) {}

bool MultiplayerAPI::is_refusing_new_connections() const { return false; }

void MultiplayerAPI::set_allow_object_decoding(bool p_enable) {}

bool MultiplayerAPI::is_object_decoding_allowed() const { return false; }

void MultiplayerAPI::set_server_relay_enabled(bool p_enabled) {}

bool MultiplayerAPI::is_server_relay_enabled() const { return false; }

void MultiplayerAPI::set_max_sync_packet_size(int p_size) {}

int MultiplayerAPI::get_max_sync_packet_size() const { return 0; }

void MultiplayerAPI::set_max_delta_packet_size(int p_size) {}

int MultiplayerAPI::get_max_delta_packet_size() const { return 0; }

#endif // MODULE_MULTIPLAYER_ENABLED
