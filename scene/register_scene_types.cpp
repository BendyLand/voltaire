/**************************************************************************/
/*  register_scene_types.cpp                                              */
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

#include "register_scene_types.h"

#include "scene/resources/compressed_texture_resource_format.h"
#include "scene/resources/resource_format_text.h"
#include "scene/resources/shader_resource_format.h"
#include "scene/scene_string_names.h"

static Ref<ResourceFormatSaverText> resource_saver_text;
static Ref<ResourceFormatLoaderText> resource_loader_text;

static Ref<ResourceFormatLoaderCompressedTexture> resource_loader_compressed_texture;

static Ref<ResourceFormatSaverShader> resource_saver_shader;
static Ref<ResourceFormatLoaderShader> resource_loader_shader;

void unregister_scene_types() { SceneStrings::finalize(); }

void register_scene_types() { SceneStrings::initialize(); }

void register_scene_singletons() {}

// ==========================================================================
// Merged from scene/property_list_helper.cpp
// ==========================================================================

#include "property_list_helper.h"

HashMap<String, Vector<PropertyListHelper*>> PropertyListHelper::base_helpers; // static

void PropertyListHelper::clear_base_helpers()
{ // static
	for (KeyValue<String, Vector<PropertyListHelper*>>& E : base_helpers) {
		for (PropertyListHelper* helper : E.value) {
			helper->clear();
		}
	}
	base_helpers.clear();
}

void PropertyListHelper::register_base_helper(
	const String& p_class_name, PropertyListHelper* p_helper)
{ // static
	base_helpers[p_class_name].push_back(p_helper);
}

Vector<PropertyListHelper*> PropertyListHelper::get_helpers_for_class(
	const String& p_class_name)
{
	if (base_helpers.has(p_class_name)) {
		return base_helpers[p_class_name];
	}
	else {
		return Vector<PropertyListHelper*>();
	}
}

void PropertyListHelper::set_prefix(const String& p_prefix) { prefix = p_prefix; }

#ifdef TOOLS_ENABLED

#endif

bool PropertyListHelper::property_can_revert(const String& p_property) const
{
	return is_property_valid(p_property);
}

bool PropertyListHelper::is_property_valid(const String& p_property, int* r_index) const
{
	return false;
}

void PropertyListHelper::clear() {}

// ==========================================================================
// Merged from scene/property_utils.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/templates/local_vector.h"
#include "property_utils.h"
#include "scene/resources/packed_scene.h"

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
