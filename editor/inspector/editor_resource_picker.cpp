/**************************************************************************/
/*  editor_resource_picker.cpp                                            */
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

#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_resource_picker.h"
#include "scene/gui/button.h"
#include "scene/gui/texture_rect.h"
#include "scene/property_utils.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/rendering_server.h"

void EditorResourcePicker::_on_unique_button_pressed()
{
	if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
		_edit_menu_cbk(OBJ_MENU_MAKE_UNIQUE);
	}
	else if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::RIGHT) &&
			   _is_uniqueness_enabled(true)) {
		_edit_menu_cbk(OBJ_MENU_MAKE_UNIQUE_RECURSIVE);
	}
}

static bool _should_hide_type(const StringName& p_type)
{
	if (p_type == SNAME("MissingResource")) {
		return true;
	}

	return false;
}

void EditorResourcePicker::set_assign_button_min_size(const Size2i& p_size)
{
	assign_button_min_size = p_size;
	assign_button->set_custom_minimum_size(assign_button_min_size);
}

String EditorResourcePicker::get_base_type() const { return base_type; }

Vector<String> EditorResourcePicker::get_allowed_types() const
{
	_ensure_allowed_types();
	HashSet<StringName> allowed_types(allowed_types_without_convert);

	Vector<String> types;
	types.resize(allowed_types.size());

	int i = 0;
	String* w = types.ptrw();
	for (const StringName& E : allowed_types) {
		w[i] = E;
		i++;
	}

	return types;
}

void EditorResourcePicker::set_edited_resource_no_check(Ref<Resource> p_resource)
{
	edited_resource = p_resource;
	_update_resource();
}

Ref<Resource> EditorResourcePicker::get_edited_resource() { return edited_resource; }

bool EditorResourcePicker::is_toggle_mode() const { return assign_button->is_toggle_mode(); }

bool EditorResourcePicker::is_toggle_pressed() const { return assign_button->is_pressed(); }

bool EditorResourcePicker::is_editable() const { return editable; }

void EditorScriptPicker::set_script_owner(Node* p_owner) { script_owner = p_owner; }

Node* EditorScriptPicker::get_script_owner() const { return script_owner; }

void EditorShaderPicker::set_edited_material(ShaderMaterial* p_material)
{
	edited_material = p_material;
}

ShaderMaterial* EditorShaderPicker::get_edited_material() const { return edited_material; }

void EditorShaderPicker::set_preferred_mode(int p_mode) { preferred_mode = p_mode; }

void EditorAudioStreamPicker::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY:
	case NOTIFICATION_THEME_CHANGED: {
		_update_resource();
	} break;
	}
}

bool EditorResourcePicker::handle_menu_selected(int p_idx) { return true; }


