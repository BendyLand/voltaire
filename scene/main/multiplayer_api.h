/**************************************************************************/
/*  multiplayer_api.h                                                     */
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

#include "core/templates/hash_set.h"
#include "core/types.h"
#include "scene/main/multiplayer_peer.h"

class MultiplayerAPI : public RefCounted
{
public:
	struct SceneData;

private:
	static String default_interface;
	SceneData* scene_data = nullptr;

	Error _send(const uint8_t* p_packet, int p_packet_len);
	void _process_packet(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_raw(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_sys(int p_from, const uint8_t* p_packet, int p_packet_len,
		MultiplayerPeer::TransferMode p_mode, int p_channel);
	void _add_peer(int p_id);
	void _admit_peer(int p_id);
	void _del_peer(int p_id);
	void _update_status();

public:
	enum RPCMode
	{
		RPC_MODE_DISABLED,	// No rpc for this method, calls to this will be blocked (default)
		RPC_MODE_ANY_PEER,	// Any peer can call this RPC
		RPC_MODE_AUTHORITY, // Only the node's multiplayer authority (server by default) can call
							// this RPC
	};

	enum NetworkCommands
	{
		NETWORK_COMMAND_REMOTE_CALL = 0,
		NETWORK_COMMAND_SIMPLIFY_PATH,
		NETWORK_COMMAND_CONFIRM_PATH,
		NETWORK_COMMAND_RAW,
		NETWORK_COMMAND_SPAWN,
		NETWORK_COMMAND_DESPAWN,
		NETWORK_COMMAND_SYNC,
		NETWORK_COMMAND_SYS,
	};

	enum SysCommands
	{
		SYS_COMMAND_AUTH,
		SYS_COMMAND_ADD_PEER,
		SYS_COMMAND_DEL_PEER,
		SYS_COMMAND_RELAY,
	};

	enum
	{
		SYS_CMD_SIZE = 6, // Command + sys command + peer_id (+ optional payload).
	};

	// For each command, the 4 MSB can contain custom flags, as defined by subsystems.
	enum
	{
		CMD_FLAG_0_SHIFT = 4,
		CMD_FLAG_1_SHIFT = 5,
		CMD_FLAG_2_SHIFT = 6,
		CMD_FLAG_3_SHIFT = 7,
	};

	// This is the mask that will be used to extract the command.
	enum
	{
		CMD_MASK = 7, // 0x7 -> 0b00000111
	};

	static Ref<MultiplayerAPI> create_default_interface();
	static void set_default_interface(const String& p_interface);
	static String get_default_interface();

	Error poll();
	void set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer);
	Ref<MultiplayerPeer> get_multiplayer_peer();
	int get_unique_id();
	Vector<int> get_peer_ids();
	int get_remote_sender_id() const;

	bool has_multiplayer_peer() { return get_multiplayer_peer().is_valid(); }
	bool is_server() { return get_unique_id() == MultiplayerPeer::TARGET_PEER_SERVER; }

	void clear();

	void set_root_path(const NodePath& p_path);
	NodePath get_root_path() const;

	void disconnect_peer(int p_id);

	Error send_auth(int p_to, Vector<uint8_t> p_bytes);
	Error complete_auth(int p_peer);
	void set_auth_timeout(double p_timeout);
	double get_auth_timeout() const;
	Vector<int> get_authenticating_peer_ids();

	Error send_command(int p_to, const uint8_t* p_packet, int p_packet_len);
	Error send_bytes(Vector<uint8_t> p_data, int p_to = MultiplayerPeer::TARGET_PEER_BROADCAST,
		MultiplayerPeer::TransferMode p_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE,
		int p_channel = 0);

	HashSet<int> get_connected_peers() const;
	void set_remote_sender_override(int p_id);

	void set_refuse_new_connections(bool p_refuse);
	bool is_refusing_new_connections() const;

	void set_allow_object_decoding(bool p_enable);
	bool is_object_decoding_allowed() const;

	void set_server_relay_enabled(bool p_enabled);
	bool is_server_relay_enabled() const;

	void set_max_sync_packet_size(int p_size);
	int get_max_sync_packet_size() const;

	void set_max_delta_packet_size(int p_size);
	int get_max_delta_packet_size() const;

	MultiplayerAPI();
	~MultiplayerAPI();
};
