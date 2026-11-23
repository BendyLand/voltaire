/**************************************************************************/
/*  test_scene_multiplayer.h                                              */
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

#include "scene/main/scene_tree.h"

#include "core/os/os.h"
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestSceneMultiplayer {

TEST_CASE("[Multiplayer][MultiplayerAPI] Defaults") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	REQUIRE(multiplayer->has_multiplayer_peer());
	Ref<MultiplayerPeer> multiplayer_peer = multiplayer->get_multiplayer_peer();
	REQUIRE_MESSAGE(dynamic_cast<OfflineMultiplayerPeer *>(multiplayer_peer.ptr()) != nullptr, "By default it must be an OfflineMultiplayerPeer instance.");
	CHECK_EQ(multiplayer->poll(), Error::OK);
	CHECK_EQ(multiplayer->get_unique_id(), MultiplayerPeer::TARGET_PEER_SERVER);
	CHECK_EQ(multiplayer->get_peer_ids(), Vector<int>());
	CHECK_EQ(multiplayer->get_remote_sender_id(), 0);
	CHECK_EQ(multiplayer->get_root_path(), NodePath());
	CHECK(multiplayer->get_connected_peers().is_empty());
	CHECK_FALSE(multiplayer->is_refusing_new_connections());
	CHECK_FALSE(multiplayer->is_object_decoding_allowed());
	CHECK(multiplayer->is_server_relay_enabled());
	CHECK_EQ(multiplayer->get_max_sync_packet_size(), 1350);
	CHECK_EQ(multiplayer->get_max_delta_packet_size(), 65535);
	CHECK(multiplayer->is_server());
}

TEST_CASE("[Multiplayer][MultiplayerAPI][SceneTree] SceneTree has an OfflineMultiplayerPeer by default") {
	Ref<MultiplayerAPI> multiplayer = SceneTree::get_multiplayer();
	REQUIRE(multiplayer->has_multiplayer_peer());

	Ref<MultiplayerPeer> multiplayer_peer = multiplayer->get_multiplayer_peer();
	REQUIRE_MESSAGE(dynamic_cast<OfflineMultiplayerPeer *>(multiplayer_peer.ptr()) != nullptr, "By default it must be an OfflineMultiplayerPeer instance.");
}

TEST_CASE("[Multiplayer][MultiplayerAPI] Root Path") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	SUBCASE("Is set") {
		NodePath foo_path("/Foo");
		multiplayer->set_root_path(foo_path);

		CHECK_EQ(multiplayer->get_root_path(), foo_path);
	}

	SUBCASE("Allows empty path") {
		multiplayer->set_root_path(NodePath());
		CHECK_EQ(multiplayer->get_root_path(), NodePath());
	}

	SUBCASE("Fails when path is relative") {
		NodePath foo_path("Foo");
		ERR_PRINT_OFF;
		multiplayer->set_root_path(foo_path);
		ERR_PRINT_ON;

		CHECK_EQ(multiplayer->get_root_path(), NodePath());
	}
}

TEST_CASE("[Multiplayer][MultiplayerAPI] Authentication validation") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	SUBCASE("Send auth fails when there is no MultiplayerPeer configured") {
		multiplayer->set_multiplayer_peer(nullptr);

		ERR_PRINT_OFF;
		CHECK_EQ(multiplayer->send_auth(42, Vector<uint8_t>()), Error::ERR_UNCONFIGURED);
		CHECK_EQ(multiplayer->complete_auth(42), Error::ERR_UNCONFIGURED);
		ERR_PRINT_ON;
	}

	SUBCASE("Send/complete auth fails when peer is not pending") {
		ERR_PRINT_OFF;
		CHECK_EQ(multiplayer->send_auth(42, String("It's me").to_ascii_buffer()), Error::ERR_INVALID_PARAMETER);
		CHECK_EQ(multiplayer->complete_auth(42), Error::ERR_INVALID_PARAMETER);
		ERR_PRINT_ON;
	}
}

} // namespace TestSceneMultiplayer

// --- Folded from modules/multiplayer/tests/test_multiplayer_spawner.h ---
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestMultiplayerSpawner
{
class Wasp : public Node
{
	int _size = 0;

public:
	int get_size() const { return _size; }

	void set_size(int p_size) { _size = p_size; }

	Wasp()
	{
		set_name("Wasp");
		set_scene_file_path("wasp.tscn");
	}
};

class SpawnWasps
{
public:
	Wasp* create_wasps(int p_size)
	{
		Wasp* wasp = memnew(Wasp);
		wasp->set_size(p_size);
		return wasp;
	}

	int echo_size(int p_size) { return p_size; }
};

} // namespace TestMultiplayerSpawner
// --- End modules/multiplayer/tests/test_multiplayer_spawner.h ---
