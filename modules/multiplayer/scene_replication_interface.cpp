/**************************************************************************/
/*  scene_replication_interface.cpp                                       */
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
#include "core/os/os.h"
#include "scene/main/node.h"
#include "scene_multiplayer.h"
#include "scene_replication_interface.h"

#define MAKE_ROOM(m_amount)                                                                        \
	if (packet_cache.size() < m_amount)                                                            \
		packet_cache.resize(m_amount);

bool SceneReplicationInterface::_has_authority(const Node* p_node)
{
	return multiplayer->has_multiplayer_peer() &&
		   p_node->get_multiplayer_authority() == multiplayer->get_unique_id();
}

Error SceneReplicationInterface::_send_raw(
	const uint8_t* p_buffer, int p_size, int p_peer, bool p_reliable)
{
	ERR_FAIL_COND_V(!p_buffer || p_size < 1, ERR_INVALID_PARAMETER);

	Ref<MultiplayerPeer> peer = multiplayer->get_multiplayer_peer();
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	peer->set_transfer_channel(0);
	peer->set_transfer_mode(p_reliable ? MultiplayerPeer::TRANSFER_MODE_RELIABLE
									   : MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);
	return multiplayer->send_command(p_peer, p_buffer, p_size);
}

void SceneReplicationInterface::set_max_sync_packet_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 128, "Sync maximum packet size must be at least 128 bytes.");
	sync_mtu = p_size;
}

int SceneReplicationInterface::get_max_sync_packet_size() const { return sync_mtu; }

void SceneReplicationInterface::set_max_delta_packet_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 128, "Sync maximum packet size must be at least 128 bytes.");
	delta_mtu = p_size;
}

int SceneReplicationInterface::get_max_delta_packet_size() const { return delta_mtu; }


