/**************************************************************************/
/*  scene_rpc_interface.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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
#include "scene/main/multiplayer_api.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene_multiplayer.h"
#include "scene_rpc_interface.h"

// The RPC meta is composed by a single byte that contains (starting from the least significant
// bit):
// - `NetworkCommands` in the first four bits.
// - `NetworkNodeIdCompression` in the next 2 bits.
// - `NetworkNameIdCompression` in the next 1 bit.
// - `byte_only_or_no_args` in the next 1 bit.
#define NODE_ID_COMPRESSION_SHIFT SceneMultiplayer::CMD_FLAG_0_SHIFT
#define NAME_ID_COMPRESSION_SHIFT SceneMultiplayer::CMD_FLAG_2_SHIFT
#define BYTE_ONLY_OR_NO_ARGS_SHIFT SceneMultiplayer::CMD_FLAG_3_SHIFT

#define NODE_ID_COMPRESSION_FLAG                                                                   \
	((1 << NODE_ID_COMPRESSION_SHIFT) | (1 << (NODE_ID_COMPRESSION_SHIFT + 1)))
#define NAME_ID_COMPRESSION_FLAG (1 << NAME_ID_COMPRESSION_SHIFT)
#define BYTE_ONLY_OR_NO_ARGS_FLAG (1 << BYTE_ONLY_OR_NO_ARGS_SHIFT)

// Returns the packet size stripping the node path added when the node is not yet cached.
int get_packet_len(uint32_t p_node_target, int p_packet_len)
{
	if (p_node_target & 0x80000000) {
		int ofs = p_node_target & 0x7FFFFFFF;
		return p_packet_len - (p_packet_len - ofs);
	}
	else {
		return p_packet_len;
	}
}

void SceneRPCInterface::process_rpc(int p_from, const uint8_t* p_packet, int p_packet_len)
{
	// Extract packet meta
	int packet_min_size = 1;
	int name_id_offset = 1;
	ERR_FAIL_COND_MSG(p_packet_len < packet_min_size, "Invalid packet received. Size too small.");
	// Compute the meta size, which depends on the compression level.
	int node_id_compression = (p_packet[0] & NODE_ID_COMPRESSION_FLAG) >> NODE_ID_COMPRESSION_SHIFT;
	int name_id_compression = (p_packet[0] & NAME_ID_COMPRESSION_FLAG) >> NAME_ID_COMPRESSION_SHIFT;

	switch (node_id_compression) {
	case NETWORK_NODE_ID_COMPRESSION_8:
		packet_min_size += 1;
		name_id_offset += 1;
		break;
	case NETWORK_NODE_ID_COMPRESSION_16:
		packet_min_size += 2;
		name_id_offset += 2;
		break;
	case NETWORK_NODE_ID_COMPRESSION_32:
		packet_min_size += 4;
		name_id_offset += 4;
		break;
	default:
		ERR_FAIL_MSG("Was not possible to extract the node id compression mode.");
	}
	switch (name_id_compression) {
	case NETWORK_NAME_ID_COMPRESSION_8:
		packet_min_size += 1;
		break;
	case NETWORK_NAME_ID_COMPRESSION_16:
		packet_min_size += 2;
		break;
	default:
		ERR_FAIL_MSG("Was not possible to extract the name id compression mode.");
	}
	ERR_FAIL_COND_MSG(p_packet_len < packet_min_size, "Invalid packet received. Size too small.");

	uint32_t node_target = 0;
	switch (node_id_compression) {
	case NETWORK_NODE_ID_COMPRESSION_8:
		node_target = p_packet[1];
		break;
	case NETWORK_NODE_ID_COMPRESSION_16:
		node_target = decode_uint16(p_packet + 1);
		break;
	case NETWORK_NODE_ID_COMPRESSION_32:
		node_target = decode_uint32(p_packet + 1);
		break;
	default:
		// Unreachable, checked before.
		CRASH_NOW();
	}

	Node* node = _process_get_node(p_from, p_packet, node_target, p_packet_len);
	ERR_FAIL_NULL_MSG(node, "Invalid packet received. Requested node was not found.");

	uint16_t name_id = 0;
	switch (name_id_compression) {
	case NETWORK_NAME_ID_COMPRESSION_8:
		name_id = p_packet[name_id_offset];
		break;
	case NETWORK_NAME_ID_COMPRESSION_16:
		name_id = decode_uint16(p_packet + name_id_offset);
		break;
	default:
		// Unreachable, checked before.
		CRASH_NOW();
	}

	const int packet_len = get_packet_len(node_target, p_packet_len);
	_process_rpc(node, name_id, p_from, p_packet, packet_len, packet_min_size);
}

static String _get_rpc_mode_string(MultiplayerAPI::RPCMode p_mode)
{
	switch (p_mode) {
	case MultiplayerAPI::RPC_MODE_DISABLED:
		return "disabled";
	case MultiplayerAPI::RPC_MODE_ANY_PEER:
		return "any_peer";
	case MultiplayerAPI::RPC_MODE_AUTHORITY:
		return "authority";
	}
	ERR_FAIL_V_MSG(String(), "Invalid RPC mode.");
}


