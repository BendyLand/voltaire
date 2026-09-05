/**************************************************************************/
/*  test_multiplayer_spawner.h                                            */
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

#include "../multiplayer_spawner.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestMultiplayerSpawner
{
class Wasp : public Node
{
	VLTRCLASS(Wasp, Node);

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
protected:
	static void _bind_methods()
	{
	}

public:
	Wasp* create_wasps(int p_size)
	{
		Wasp* wasp = memnew(Wasp);
		wasp->set_size(p_size);
		return wasp;
	}

	Wasp* create_wasps_error(const Variant** p_args, int p_argcount, Callable::CallError& r_error)
	{
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return nullptr;
	}

	int echo_size(int p_size) { return p_size; }
};

} // namespace TestMultiplayerSpawner


