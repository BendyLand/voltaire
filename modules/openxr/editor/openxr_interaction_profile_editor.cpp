/**************************************************************************/
/*  openxr_interaction_profile_editor.cpp                                 */
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

#include "../openxr_api.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "openxr_interaction_profile_editor.h"

void OpenXRInteractionProfileEditorBase::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_update_interaction_profile();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		_theme_changed();
	} break;
	}
}

void OpenXRInteractionProfileEditorBase::_set_dirty() { is_dirty = true; }

void OpenXRInteractionProfileEditorBase::_update_interaction_profile()
{
	if (!is_dirty) {
		// no need to update
		return;
	}

	// Nothing to do here for now..

	// and we've updated it...
	is_dirty = false;
}

void OpenXRInteractionProfileEditorBase::_theme_changed()
{
	if (binding_modifiers_btn) {
		binding_modifiers_btn->set_button_icon(
			get_theme_icon(SNAME("Modifiers"), EditorStringName(EditorIcons)));
	}
}

void OpenXRInteractionProfileEditorBase::remove_all_for_action_set(
	const Ref<OpenXRActionSet>& p_action_set)
{
	// Note, don't need to remove bindings themselves as remove_all_for_action will be called for
	// each before this is called.

	// TODO update binding modifiers
}

void OpenXRInteractionProfileEditorBase::_on_open_binding_modifiers()
{
	binding_modifiers_dialog->popup_centered(Size2i(500, 400));
}

void OpenXRInteractionProfileEditorBase::setup(const Ref<OpenXRActionMap>& p_action_map,
	const Ref<OpenXRInteractionProfile>& p_interaction_profile)
{
	ERR_FAIL_NULL(binding_modifiers_dialog);
	binding_modifiers_dialog->setup(p_action_map, p_interaction_profile);

	action_map = p_action_map;
	interaction_profile = p_interaction_profile;
	String profile_path = interaction_profile->get_interaction_profile_path();
	String profile_name = profile_path;

	profile_def = OpenXRInteractionProfileMetadata::get_singleton()->get_profile(profile_path);
	if (profile_def != nullptr) {
		profile_name = profile_def->display_name;

		if (!profile_def->openxr_extension_names.is_empty()) {
			profile_name += "*";

			tooltip = vformat(TTR("Note: This interaction profile requires extension %s support."),
				profile_def->openxr_extension_names);
		}
	}

	set_name(profile_name);

	// Make sure it is updated when it enters the tree...
	is_dirty = true;
}

void OpenXRInteractionProfileEditor::select_action_for(const String& p_io_path)
{
	selecting_for_io_path = p_io_path;
	select_action_dialog->open();
}

void OpenXRInteractionProfileEditor::_update_interaction_profile()
{
	ERR_FAIL_NULL(profile_def);

	if (!is_dirty) {
		// no need to update
		return;
	}

	PackedStringArray requested_extensions = OpenXRAPI::get_all_requested_extensions(0);

	// out with the old...
	while (interaction_profile_hb->get_child_count() > 0) {
		memdelete(interaction_profile_hb->get_child(0));
	}

	// in with the new...

	// Determine toplevel paths
	Vector<String> top_level_paths;
	for (int i = 0; i < profile_def->io_paths.size(); i++) {
		const OpenXRInteractionProfileMetadata::IOPath* io_path = &profile_def->io_paths[i];

		if (!top_level_paths.has(io_path->top_level_path)) {
			top_level_paths.push_back(io_path->top_level_path);
		}
	}

	for (int i = 0; i < top_level_paths.size(); i++) {
		PanelContainer* panel = memnew(PanelContainer);
		panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		interaction_profile_hb->add_child(panel);
		panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("TabContainer")).ptr());

		VBoxContainer* container = memnew(VBoxContainer);
		panel->add_child(container);

		Label* label = memnew(Label);
		label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		label->set_text(OpenXRInteractionProfileMetadata::get_singleton()->get_top_level_name(
			top_level_paths[i]));
		container->add_child(label);

		for (int j = 0; j < profile_def->io_paths.size(); j++) {
			const OpenXRInteractionProfileMetadata::IOPath* io_path = &profile_def->io_paths[j];

			const Vector<String> extensions = io_path->openxr_extension_names.split(",", false);
			bool extension_is_requested =
				extensions.is_empty(); // If none, then yes we can use this.
			for (const String& extension : extensions) {
				extension_is_requested |= requested_extensions.has(extension);
			}

			if (io_path->top_level_path == top_level_paths[i] && extension_is_requested) {
				_add_io_path(container, io_path);
			}
		}
	}

	OpenXRInteractionProfileEditorBase::_update_interaction_profile();
}

OpenXRInteractionProfileEditor::OpenXRInteractionProfileEditor()
{
	interaction_profile_hb = memnew(HBoxContainer);
	interaction_profile_sc->add_child(interaction_profile_hb);
}


