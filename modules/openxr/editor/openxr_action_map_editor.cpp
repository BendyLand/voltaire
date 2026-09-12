/**************************************************************************/
/*  openxr_action_map_editor.cpp                                          */
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

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "openxr_action_map_editor.h"
#include "scene/gui/separator.h"

HashMap<String, String> OpenXRActionMapEditor::interaction_profile_editors;
HashMap<String, String> OpenXRActionMapEditor::binding_modifier_editors;

OpenXRInteractionProfileEditorBase* OpenXRActionMapEditor::_add_interaction_profile_editor(
	const Ref<OpenXRInteractionProfile>& p_interaction_profile)
{
	ERR_FAIL_COND_V(p_interaction_profile.is_null(), nullptr);

	String profile_path = p_interaction_profile->get_interaction_profile_path();

	// need to instance the correct editor for our profile
	OpenXRInteractionProfileEditorBase* new_profile_editor = nullptr;
	if (interaction_profile_editors.has(profile_path)) {
		new_profile_editor = memnew(OpenXRInteractionProfileEditorBase);
		if (!new_profile_editor) {
			WARN_PRINT("Interaction profile editor type mismatch for " + profile_path);
			memfree(new_profile_editor);
		}
	}
	if (!new_profile_editor) {
		// instance generic editor
		new_profile_editor = memnew(OpenXRInteractionProfileEditor);
	}

	// now add it in..
	ERR_FAIL_NULL_V(new_profile_editor, nullptr);
	new_profile_editor->setup(action_map, p_interaction_profile);
	tabs->add_child(new_profile_editor);
	new_profile_editor->add_theme_style_override(
		SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
	tabs->set_tab_button_icon(
		tabs->get_tab_count() - 1, get_theme_icon(SNAME("close"), SNAME("TabBar")));

	if (!new_profile_editor->tooltip.is_empty()) {
		tabs->set_tab_tooltip(tabs->get_tab_count() - 1, new_profile_editor->tooltip);
	}

	return new_profile_editor;
}

void OpenXRActionMapEditor::_set_focus_on_action_set(OpenXRActionSetEditor* p_action_set_editor)
{
	// Scroll down to our new entry
	actionsets_scroll->ensure_control_visible(p_action_set_editor);

	// Set focus on this entry
	p_action_set_editor->set_focus_on_entry();
}

void OpenXRActionMapEditor::_on_add_interaction_profile()
{
	ERR_FAIL_COND(action_map.is_null());

	PackedStringArray already_selected;

	for (int i = 0; i < action_map->get_interaction_profile_count(); i++) {
		already_selected.push_back(
			action_map->get_interaction_profile(i)->get_interaction_profile_path());
	}

	select_interaction_profile_dialog->open(already_selected);
}

void OpenXRActionMapEditor::_load_action_map(const String& p_path, bool p_create_new_if_missing)
{
	Error err = OK;
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (da->file_exists(p_path)) {
		action_map = ResourceLoader::load(p_path, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err);
		if (err != OK) {
			EditorNode::get_singleton()->show_warning(
				vformat(TTR("Error loading %s: %s."), edited_path, TTR(error_names[err])));

			edited_path = "";
			header_label->set_text("");
			return;
		}
	}
	else if (p_create_new_if_missing) {
		action_map.instantiate();
		action_map->create_default_action_sets();

		// Save it immediately
		err = ResourceSaver::save(action_map.ptr(), p_path);
		if (err != OK) {
			// Show warning but continue.
			EditorNode::get_singleton()->show_warning(
				vformat(TTR("Error saving file %s: %s"), p_path, TTR(error_names[err])));
		}
	}

	edited_path = p_path;
	header_label->set_text(TTR("OpenXR Action map:") + " " + edited_path.get_file());
}

void OpenXRActionMapEditor::_on_save_action_map()
{
	Error err = ResourceSaver::save(action_map.ptr(), edited_path);
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Error saving file %s: %s"), edited_path, TTR(error_names[err])));
		return;
	}

	// TODO should clear undo/redo history

	// out with the old
	_clear_action_map();

	_create_action_sets();
	_create_interaction_profiles();
}

void OpenXRActionMapEditor::_on_reset_to_default_layout()
{
	// TODO should clear undo/redo history

	// out with the old
	_clear_action_map();

	// create a new one
	action_map.unref();
	action_map.instantiate();
	action_map->create_default_action_sets();

	_create_action_sets();
	_create_interaction_profiles();
}

void OpenXRActionMapEditor::_do_add_action_set_editor(OpenXRActionSetEditor* p_action_set_editor)
{
	Ref<OpenXRActionSet> action_set = p_action_set_editor->get_action_set();
	ERR_FAIL_COND(action_set.is_null());

	action_map->add_action_set(action_set);
	actionsets_vb->add_child(p_action_set_editor);
}

void OpenXRActionMapEditor::_do_remove_action_set_editor(OpenXRActionSetEditor* p_action_set_editor)
{
	Ref<OpenXRActionSet> action_set = p_action_set_editor->get_action_set();
	ERR_FAIL_COND(action_set.is_null());

	actionsets_vb->remove_child(p_action_set_editor);
	action_map->remove_action_set(action_set);
}

void OpenXRActionMapEditor::_do_add_interaction_profile_editor(
	OpenXRInteractionProfileEditorBase* p_interaction_profile_editor)
{
	Ref<OpenXRInteractionProfile> interaction_profile =
		p_interaction_profile_editor->get_interaction_profile();
	ERR_FAIL_COND(interaction_profile.is_null());

	action_map->add_interaction_profile(interaction_profile);
	tabs->add_child(p_interaction_profile_editor);
	tabs->set_tab_button_icon(
		tabs->get_tab_count() - 1, get_theme_icon(SNAME("close"), SNAME("TabBar")));

	tabs->set_current_tab(tabs->get_tab_count() - 1);
}

void OpenXRActionMapEditor::_do_remove_interaction_profile_editor(
	OpenXRInteractionProfileEditorBase* p_interaction_profile_editor)
{
	Ref<OpenXRInteractionProfile> interaction_profile =
		p_interaction_profile_editor->get_interaction_profile();
	ERR_FAIL_COND(interaction_profile.is_null());

	tabs->remove_child(p_interaction_profile_editor);
	action_map->remove_interaction_profile(interaction_profile);
}

void OpenXRActionMapEditor::open_action_map(const String& p_path)
{
	make_visible();

	// out with the old...
	_clear_action_map();

	// now load in our new action map
	_load_action_map(p_path);

	_create_action_sets();
	_create_interaction_profiles();
}

void OpenXRActionMapEditor::register_interaction_profile_editor(
	const String& p_for_path, const String& p_editor_class)
{
	interaction_profile_editors[p_for_path] = p_editor_class;
}

void OpenXRActionMapEditor::register_binding_modifier_editor(
	const String& p_binding_modifier_class, const String& p_editor_class)
{
	binding_modifier_editors[p_binding_modifier_class] = p_editor_class;
}


