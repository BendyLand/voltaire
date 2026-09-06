/**************************************************************************/
/*  scene_rpc_interface.h                                                 */
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

#pragma once

#include "scene/main/multiplayer_api.h"

class SceneMultiplayer;
class SceneCacheInterface;
class SceneReplicationInterface;
class Node;

class SceneRPCInterface : public RefCounted
{
private:
	struct RPCConfig
	{
		StringName name;
		MultiplayerAPI::RPCMode rpc_mode = MultiplayerAPI::RPC_MODE_DISABLED;
		bool call_local = false;
		MultiplayerPeer::TransferMode transfer_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
		int channel = 0;

		bool operator==(const RPCConfig& p_other) const { return name == p_other.name; }
	};

	struct RPCConfigCache
	{
		HashMap<uint16_t, RPCConfig> configs;
		HashMap<StringName, uint16_t> ids;
	};

	struct SortRPCConfig
	{
		StringName::AlphCompare compare;

		bool operator()(const RPCConfig& p_a, const RPCConfig& p_b) const
		{
			return compare(p_a.name, p_b.name);
		}
	};

	enum NetworkNodeIdCompression
	{
		NETWORK_NODE_ID_COMPRESSION_8 = 0,
		NETWORK_NODE_ID_COMPRESSION_16,
		NETWORK_NODE_ID_COMPRESSION_32,
	};

	enum NetworkNameIdCompression
	{
		NETWORK_NAME_ID_COMPRESSION_8 = 0,
		NETWORK_NAME_ID_COMPRESSION_16,
	};

	SceneMultiplayer* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	SceneReplicationInterface* multiplayer_replicator = nullptr;

	Vector<uint8_t> packet_cache;

protected:
	void _process_rpc(Node* p_node, const uint16_t p_rpc_method_id, int p_from,
		const uint8_t* p_packet, int p_packet_len, int p_offset);

	Node* _process_get_node(
		int p_from, const uint8_t* p_packet, uint32_t p_node_target, int p_packet_len);

	const RPCConfigCache& _get_node_config(const Node* p_node);

public:
	void process_rpc(int p_from, const uint8_t* p_packet, int p_packet_len);

	SceneRPCInterface(SceneMultiplayer* p_multiplayer, SceneCacheInterface* p_cache,
		SceneReplicationInterface* p_replicator)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
		multiplayer_replicator = p_replicator;
	}
};


