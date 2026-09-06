/**************************************************************************/
/*  multiplayer_spawner.cpp                                               */
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
#include "core/io/resource_loader.h"
#include "multiplayer_spawner.h"
#include "scene/main/multiplayer_api.h"

PackedStringArray MultiplayerSpawner::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (spawn_path.is_empty() || !has_node(spawn_path)) {
		warnings.push_back(RTR("A valid NodePath must be set in the \"Spawn Path\" property in "
							   "order for MultiplayerSpawner to be able to spawn Nodes."));
	}
	return warnings;
}

int MultiplayerSpawner::get_spawnable_scene_count() const { return spawnable_scenes.size(); }

String MultiplayerSpawner::get_spawnable_scene(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, (int)spawnable_scenes.size(), "");
	return spawnable_scenes[p_idx].path;
}

Vector<String> MultiplayerSpawner::_get_spawnable_scenes() const
{
	Vector<String> ss;
	ss.resize(spawnable_scenes.size());
	for (int i = 0; i < ss.size(); i++) {
		ss.write[i] = ResourceUID::path_to_uid(spawnable_scenes[i].path);
	}
	return ss;
}

void MultiplayerSpawner::_set_spawnable_scenes(const Vector<String>& p_scenes)
{
	clear_spawnable_scenes();
	for (int i = 0; i < p_scenes.size(); i++) {
		add_spawnable_scene(p_scenes[i]);
	}
}

NodePath MultiplayerSpawner::get_spawn_path() const { return spawn_path; }

void MultiplayerSpawner::set_spawn_path(const NodePath& p_path)
{
	spawn_path = p_path;
	_update_spawn_node();
	update_configuration_warnings();
}

int MultiplayerSpawner::find_spawnable_scene_index_from_path(const String& p_scene) const
{
	for (uint32_t i = 0; i < spawnable_scenes.size(); i++) {
		if (spawnable_scenes[i].path == p_scene) {
			return i;
		}
	}
	return INVALID_ID;
}


