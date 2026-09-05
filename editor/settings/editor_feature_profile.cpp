/**************************************************************************/
/*  editor_feature_profile.cpp                                            */
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

#include "core/io/dir_access.h"
#include "core/io/json.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_feature_profile.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/separator.h"

const char* EditorFeatureProfile::feature_names[FEATURE_MAX] = {
	TTRC("3D Editor"),
	TTRC("Script Editor"),
	TTRC("Asset Store"),
	TTRC("Scene Tree Editing"),
#ifndef DISABLE_DEPRECATED
	TTRC("Node Dock (deprecated)"),
#endif
	TTRC("FileSystem Dock"),
	TTRC("Import Dock"),
	TTRC("History Dock"),
	TTRC("Game View"),
	TTRC("Signals Dock"),
	TTRC("Groups Dock"),
};

const char* EditorFeatureProfile::feature_descriptions[FEATURE_MAX] = {
	TTRC("Allows to view and edit 3D scenes."),
	TTRC("Allows to edit scripts using the integrated script editor."),
	TTRC("Provides built-in access to the Asset Store."),
	TTRC("Allows editing the node hierarchy in the Scene dock."),
#ifndef DISABLE_DEPRECATED
	TTRC("Allows to work with signals and groups of the node selected in the Scene dock."),
#endif
	TTRC("Allows to browse the local file system via a dedicated dock."),
	TTRC("Allows to configure import settings for individual assets. Requires the FileSystem dock "
		 "to function."),
	TTRC("Provides an overview of the editor's and each scene's undo history."),
	TTRC("Provides tools for selecting and debugging nodes at runtime."),
	TTRC("Allows to work with signals of the node selected in the Scene dock."),
	TTRC("Allows to manage groups of the node selected in the Scene dock."),
};

const char* EditorFeatureProfile::feature_identifiers[FEATURE_MAX] = {
	"3d",
	"script",
	"asset_lib",
	"scene_tree",
#ifndef DISABLE_DEPRECATED
	"node_dock",
#endif
	"filesystem_dock",
	"import_dock",
	"history_dock",
	"game",
	"signals_dock",
	"groups_dock",
};

void EditorFeatureProfile::set_disable_class(const StringName& p_class, bool p_disabled)
{
	if (p_disabled) {
		disabled_classes.insert(p_class);
	}
	else {
		disabled_classes.erase(p_class);
	}
}

bool EditorFeatureProfile::is_class_disabled(const StringName& p_class) const
{
	if (p_class == StringName()) {
		return false;
	}
	return disabled_classes.has(p_class);
}

void EditorFeatureProfile::set_disable_class_editor(const StringName& p_class, bool p_disabled)
{
	if (p_disabled) {
		disabled_editors.insert(p_class);
	}
	else {
		disabled_editors.erase(p_class);
	}
}

bool EditorFeatureProfile::is_class_editor_disabled(const StringName& p_class) const
{
	if (p_class == StringName()) {
		return false;
	}
	return disabled_editors.has(p_class);
}

void EditorFeatureProfile::set_disable_class_property(
	const StringName& p_class, const StringName& p_property, bool p_disabled)
{
	if (p_disabled) {
		if (!disabled_properties.has(p_class)) {
			disabled_properties[p_class] = HashSet<StringName>();
		}

		disabled_properties[p_class].insert(p_property);
	}
	else {
		ERR_FAIL_COND(!disabled_properties.has(p_class));
		disabled_properties[p_class].erase(p_property);
		if (disabled_properties[p_class].is_empty()) {
			disabled_properties.erase(p_class);
		}
	}
}

bool EditorFeatureProfile::is_class_property_disabled(
	const StringName& p_class, const StringName& p_property) const
{
	if (!disabled_properties.has(p_class)) {
		return false;
	}

	if (!disabled_properties[p_class].has(p_property)) {
		return false;
	}

	return true;
}

bool EditorFeatureProfile::has_class_properties_disabled(const StringName& p_class) const
{
	return disabled_properties.has(p_class);
}

void EditorFeatureProfile::set_item_collapsed(const StringName& p_class, bool p_collapsed)
{
	if (p_collapsed) {
		collapsed_classes.insert(p_class);
	}
	else {
		collapsed_classes.erase(p_class);
	}
}

bool EditorFeatureProfile::is_item_collapsed(const StringName& p_class) const
{
	return collapsed_classes.has(p_class);
}

void EditorFeatureProfile::set_disable_feature(Feature p_feature, bool p_disable)
{
	ERR_FAIL_INDEX(p_feature, FEATURE_MAX);
	features_disabled[p_feature] = p_disable;
}

bool EditorFeatureProfile::is_feature_disabled(Feature p_feature) const
{
	ERR_FAIL_INDEX_V(p_feature, FEATURE_MAX, false);
	return features_disabled[p_feature];
}

String EditorFeatureProfile::get_feature_name(Feature p_feature)
{
	ERR_FAIL_INDEX_V(p_feature, FEATURE_MAX, String());
	return feature_names[p_feature];
}

String EditorFeatureProfile::get_feature_description(Feature p_feature)
{
	ERR_FAIL_INDEX_V(p_feature, FEATURE_MAX, String());
	return feature_descriptions[p_feature];
}

EditorFeatureProfile::EditorFeatureProfile()
{
	for (int i = 0; i < FEATURE_MAX; i++) {
		features_disabled[i] = false;
	}
}

//////////////////////////

void EditorFeatureProfileManager::_profile_action(int p_action)
{
	switch (p_action) {
	case PROFILE_CLEAR: {
		set_current_profile("", false);
	} break;
	case PROFILE_SET: {
		String selected = _get_selected_profile();
		ERR_FAIL_COND(selected.is_empty());
		if (selected == current_profile) {
			return; // Nothing to do here.
		}
		set_current_profile(selected, false);
	} break;
	case PROFILE_IMPORT: {
		import_profiles->popup_file_dialog();
	} break;
	case PROFILE_EXPORT: {
		export_profile->popup_file_dialog();
		export_profile->set_current_file(_get_selected_profile() + ".profile");
	} break;
	case PROFILE_NEW: {
		new_profile_dialog->popup_centered(Size2(240, 60) * EDSCALE);
		new_profile_name->clear();
		new_profile_name->grab_focus();
	} break;
	case PROFILE_ERASE: {
		String selected = _get_selected_profile();
		ERR_FAIL_COND(selected.is_empty());

		erase_profile_dialog->set_text(
			vformat(TTR("Remove currently selected profile, '%s'? Cannot be undone."), selected));
		erase_profile_dialog->popup_centered(Size2(240, 60) * EDSCALE);
	} break;
	}
}

void EditorFeatureProfileManager::_erase_selected_profile()
{
	String selected = _get_selected_profile();
	ERR_FAIL_COND(selected.is_empty());
	Ref<DirAccess> da = DirAccess::open(EditorPaths::get_singleton()->get_feature_profiles_dir());
	ERR_FAIL_COND_MSG(da.is_null(), "Cannot open directory '" +
										EditorPaths::get_singleton()->get_feature_profiles_dir() +
										"'.");

	da->remove(selected + ".profile");
	if (selected == current_profile) {
		_profile_action(PROFILE_CLEAR);
	}
	else {
		_update_profile_list();
	}
}

void EditorFeatureProfileManager::_create_new_profile()
{
	String name = new_profile_name->get_text().strip_edges();
	if (!name.is_valid_filename() || name.contains_char('.')) {
		EditorNode::get_singleton()->show_warning(
			TTR("Profile must be a valid filename and must not contain '.'"));
		return;
	}
	String file =
		EditorPaths::get_singleton()->get_feature_profiles_dir().path_join(name + ".profile");
	if (FileAccess::exists(file)) {
		EditorNode::get_singleton()->show_warning(TTR("Profile with this name already exists."));
		return;
	}

	Ref<EditorFeatureProfile> new_profile;
	new_profile.instantiate();
	new_profile->save_to_file(file);

	_update_profile_list(name);
	// The newly created profile is the first one, make it the current profile automatically.
	if (profile_list->get_item_count() == 1) {
		_profile_action(PROFILE_SET);
	}
}

void EditorFeatureProfileManager::_profile_selected(int p_what) { _update_selected_profile(); }

void EditorFeatureProfileManager::_hide_requested()
{
	_cancel_pressed(); // From AcceptDialog.
}

void EditorFeatureProfileManager::_import_profiles(const Vector<String>& p_paths)
{
	// test it first
	for (int i = 0; i < p_paths.size(); i++) {
		Ref<EditorFeatureProfile> profile;
		profile.instantiate();
		Error err = profile->load_from_file(p_paths[i]);
		String basefile = p_paths[i].get_file();
		if (err != OK) {
			EditorNode::get_singleton()->show_warning(
				vformat(TTR("File '%s' format is invalid, import aborted."), basefile));
			return;
		}

		String dst_file =
			EditorPaths::get_singleton()->get_feature_profiles_dir().path_join(basefile);

		if (FileAccess::exists(dst_file)) {
			EditorNode::get_singleton()->show_warning(
				vformat(TTR("Profile '%s' already exists. Remove it first before importing, import "
							"aborted."),
					basefile.get_basename()));
			return;
		}
	}

	// do it second
	for (int i = 0; i < p_paths.size(); i++) {
		Ref<EditorFeatureProfile> profile;
		profile.instantiate();
		Error err = profile->load_from_file(p_paths[i]);
		ERR_CONTINUE(err != OK);
		String basefile = p_paths[i].get_file();
		String dst_file =
			EditorPaths::get_singleton()->get_feature_profiles_dir().path_join(basefile);
		profile->save_to_file(dst_file);
	}

	_update_profile_list();
	// The newly imported profile is the first one, make it the current profile automatically.
	if (profile_list->get_item_count() == 1) {
		_profile_action(PROFILE_SET);
	}
}

void EditorFeatureProfileManager::_export_profile(const String& p_path)
{
	ERR_FAIL_COND(edited.is_null());
	Error err = edited->save_to_file(p_path);
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Error saving profile to path: '%s'."), p_path));
	}
}

void EditorFeatureProfileManager::_save_and_update()
{
	String edited_path = _get_selected_profile();
	ERR_FAIL_COND(edited_path.is_empty());
	ERR_FAIL_COND(edited.is_null());

	edited->save_to_file(EditorPaths::get_singleton()->get_feature_profiles_dir().path_join(
		edited_path + ".profile"));

	if (edited == current) {
		update_timer->start();
	}
}

void EditorFeatureProfileManager::notify_changed() { _emit_current_profile_changed(); }

Ref<EditorFeatureProfile> EditorFeatureProfileManager::get_current_profile() { return current; }

String EditorFeatureProfileManager::get_current_profile_name() const { return current_profile; }

EditorFeatureProfileManager* EditorFeatureProfileManager::singleton = nullptr;


