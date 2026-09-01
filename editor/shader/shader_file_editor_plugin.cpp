/**************************************************************************/
/*  shader_file_editor_plugin.cpp                                         */
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

#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/split_container.h"
#include "servers/rendering/rendering_device_binds.h"
#include "shader_file_editor_plugin.h"

/*** SHADER SCRIPT EDITOR ****/

void ShaderFileEditor::_update_version(
	const StringName& p_version_txt, const RD::ShaderStage p_stage)
{
}

void ShaderFileEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_WM_WINDOW_FOCUS_IN: {
		if (is_visible_in_tree() && shader_file.is_valid()) {
			_update_options();
		}
	} break;
	}
}

void ShaderFileEditor::_editor_settings_changed()
{
	if (is_visible_in_tree() && shader_file.is_valid()) {
		_update_options();
	}
}

void ShaderFileEditor::_shader_changed()
{
	if (is_visible_in_tree()) {
		_update_options();
	}
}

ShaderFileEditor* ShaderFileEditor::singleton = nullptr;

ShaderFileEditor::ShaderFileEditor()
{
	singleton = this;

	set_name(TTRC("ShaderFile"));
	set_icon_name("RDShaderFile");
	set_dock_shortcut(ED_SHORTCUT_AND_COMMAND(
		"bottom_panels/toggle_shader_file_bottom_panel", TTRC("Toggle ShaderFile Dock")));
	set_default_slot(EditorDock::DOCK_SLOT_BOTTOM);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	set_global(false);
	set_transient(true);
	set_custom_minimum_size(Size2(300, 200) * EDSCALE);

	HSplitContainer* main_hs = memnew(HSplitContainer);

	add_child(main_hs);

	versions = memnew(ItemList);
	versions->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	versions->set_custom_minimum_size(Size2i(100 * EDSCALE, 0));
	versions->set_theme_type_variation("TreeSecondary");
	main_hs->add_child(versions);

	VBoxContainer* main_vb = memnew(VBoxContainer);
	main_vb->set_h_size_flags(SIZE_EXPAND_FILL);
	main_hs->add_child(main_vb);

	static const char* stage_str[RD::SHADER_STAGE_MAX] = {
		"Vertex",
		"Fragment",
		"TessControl",
		"TessEval",
		"Compute",
		"Raygen",
		"AnyHit",
		"ClosestHit",
		"Miss",
		"Intersection",
	};

	stage_hb = memnew(HFlowContainer);
	main_vb->add_child(stage_hb);

	Ref<ButtonGroup> bg;
	bg.instantiate();
	for (int i = 0; i < RD::SHADER_STAGE_MAX; i++) {
		Button* button = memnew(Button(stage_str[i]));
		button->set_toggle_mode(true);
		button->set_focus_mode(FOCUS_ACCESSIBILITY);
		stage_hb->add_child(button);
		stages[i] = button;
		button->set_button_group(bg);
	}

	error_text = memnew(RichTextLabel);
	error_text->set_v_size_flags(SIZE_EXPAND_FILL);
	error_text->set_selection_enabled(true);
	error_text->set_context_menu_enabled(true);
	main_vb->add_child(error_text);
}

ShaderFileEditorPlugin::ShaderFileEditorPlugin()
{
	shader_editor = memnew(ShaderFileEditor);
	EditorDockManager::get_singleton()->add_dock(shader_editor);
	shader_editor->close();
}


