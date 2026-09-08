/**************************************************************************/
/*  property_utils.cpp                                                    */
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
#include "core/templates/local_vector.h"
#include "property_utils.h"
#include "scene/resources/packed_scene.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#endif // TOOLS_ENABLED

// Like SceneState::PackState, but using a raw pointer to avoid the cost of
// updating the reference count during the internal work of the functions below
namespace
{
struct _FastPackState
{
	SceneState* state = nullptr;
	int node = -1;
};
} // namespace

static bool _collect_inheritance_chain(const Ref<SceneState>& p_state, const NodePath& p_path,
	LocalVector<_FastPackState>& r_states_stack)
{
	bool found = false;

	LocalVector<_FastPackState> inheritance_states;

	Ref<SceneState> state = p_state;
	while (state.is_valid()) {
		int node = state->find_node_by_path(p_path);
		if (node >= 0) {
			// This one has state for this node
			inheritance_states.push_back({state.ptr(), node});
			found = true;
		}
		state = state->get_base_scene_state();
	}

	if (inheritance_states.size() > 0) {
		for (int i = inheritance_states.size() - 1; i >= 0; --i) {
			r_states_stack.push_back(inheritance_states[i]);
		}
	}

	return found;
}


