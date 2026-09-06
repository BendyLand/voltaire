/**************************************************************************/
/*  multiplayer_synchronizer.cpp                                          */
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

#include "core/config/engine.h"
#include "multiplayer_synchronizer.h"
#include "scene/main/multiplayer_api.h"

void MultiplayerSynchronizer::reset()
{
	net_id = 0;
	last_sync_usec = 0;
	last_inbound_sync = 0;
	last_watch_usec = 0;
	sync_started = false;
	watchers.clear();
}

uint32_t MultiplayerSynchronizer::get_net_id() const { return net_id; }

void MultiplayerSynchronizer::set_net_id(uint32_t p_net_id) { net_id = p_net_id; }

bool MultiplayerSynchronizer::update_outbound_sync_time(uint64_t p_usec)
{
	if (last_sync_usec == p_usec) {
		// last_sync_usec has been updated in this frame.
		return true;
	}
	if (p_usec < last_sync_usec + sync_interval_usec) {
		// Too soon, should skip this synchronization frame.
		return false;
	}
	last_sync_usec = p_usec;
	return true;
}

bool MultiplayerSynchronizer::update_inbound_sync_time(uint16_t p_network_time)
{
	if (!sync_started) {
		sync_started = true;
	}
	else if (p_network_time <= last_inbound_sync && last_inbound_sync - p_network_time < 32767) {
		return false;
	}
	last_inbound_sync = p_network_time;
	return true;
}

PackedStringArray MultiplayerSynchronizer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (root_path.is_empty() || !has_node(root_path)) {
		warnings.push_back(
			RTR("A valid NodePath must be set in the \"Root Path\" property in order for "
				"MultiplayerSynchronizer to be able to synchronize properties."));
	}

	return warnings;
}

bool MultiplayerSynchronizer::is_visibility_public() const { return peer_visibility.has(0); }

void MultiplayerSynchronizer::set_visibility_public(bool p_visible)
{
	set_visibility_for(0, p_visible);
}

void MultiplayerSynchronizer::set_visibility_for(int p_peer, bool p_visible)
{
	if (peer_visibility.has(p_peer) == p_visible) {
		return;
	}
	if (p_visible) {
		peer_visibility.insert(p_peer);
	}
	else {
		peer_visibility.erase(p_peer);
	}
	update_visibility(p_peer);
}

bool MultiplayerSynchronizer::get_visibility_for(int p_peer) const
{
	return peer_visibility.has(p_peer);
}

void MultiplayerSynchronizer::set_visibility_update_mode(VisibilityUpdateMode p_mode)
{
	visibility_update_mode = p_mode;
	_update_process();
}

MultiplayerSynchronizer::VisibilityUpdateMode
MultiplayerSynchronizer::get_visibility_update_mode() const
{
	return visibility_update_mode;
}

void MultiplayerSynchronizer::_bind_methods() {}

void MultiplayerSynchronizer::_notification(int p_what)
{
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif
	if (root_path.is_empty()) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_start();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_stop();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS:
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		update_visibility(0);
	} break;
	}
}

void MultiplayerSynchronizer::set_replication_interval(double p_interval)
{
	ERR_FAIL_COND_MSG(
		p_interval < 0, "Interval must be greater or equal to 0 (where 0 means default)");
	sync_interval_usec = uint64_t(p_interval * 1000 * 1000);
}

double MultiplayerSynchronizer::get_replication_interval() const
{
	return double(sync_interval_usec) / 1000.0 / 1000.0;
}

void MultiplayerSynchronizer::set_delta_interval(double p_interval)
{
	ERR_FAIL_COND_MSG(
		p_interval < 0, "Interval must be greater or equal to 0 (where 0 means default)");
	delta_interval_usec = uint64_t(p_interval * 1000 * 1000);
}

double MultiplayerSynchronizer::get_delta_interval() const
{
	return double(delta_interval_usec) / 1000.0 / 1000.0;
}

void MultiplayerSynchronizer::set_replication_config(Ref<SceneReplicationConfig> p_config)
{
	replication_config = p_config;
}

Ref<SceneReplicationConfig> MultiplayerSynchronizer::get_replication_config()
{
	return replication_config;
}

void MultiplayerSynchronizer::set_root_path(const NodePath& p_path)
{
	if (p_path == root_path) {
		return;
	}
	_stop();
	root_path = p_path;
	_start();
	update_configuration_warnings();
}

NodePath MultiplayerSynchronizer::get_root_path() const { return root_path; }

void MultiplayerSynchronizer::set_multiplayer_authority(int p_peer_id, bool p_recursive)
{
	if (get_multiplayer_authority() == p_peer_id) {
		return;
	}
	_stop();
	Node::set_multiplayer_authority(p_peer_id, p_recursive);
	_start();
}

List<NodePath> MultiplayerSynchronizer::get_delta_properties(uint64_t p_indexes)
{
	List<NodePath> out;
	ERR_FAIL_COND_V(replication_config.is_null(), out);
	const List<NodePath> watch_props(replication_config->get_watch_properties());
	int idx = 0;
	for (const NodePath& prop : watch_props) {
		if ((p_indexes & (1ULL << idx++)) == 0) {
			continue;
		}
		out.push_back(prop);
	}
	return out;
}

SceneReplicationConfig* MultiplayerSynchronizer::get_replication_config_ptr() const
{
	return replication_config.ptr();
}

MultiplayerSynchronizer::MultiplayerSynchronizer()
{
	// Publicly visible by default.
	peer_visibility.insert(0);
}


