/**************************************************************************/
/*  project_export.cpp                                                    */
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

#include <zstd.h>
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_properties.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "project_export.h"
#include "scene/gui/check_button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/link_button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "servers/display/display_server.h"

void ProjectExportDialog::_update_current_preset() { _edit_preset(presets->get_current()); }

void ProjectExportDialog::_update_feature_list()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	List<String> features_list;

	current->get_platform()->get_platform_features(&features_list);
	current->get_platform()->get_preset_features(current, &features_list);

	String custom = current->get_custom_features();
	Vector<String> custom_list = custom.split(",");
	for (int i = 0; i < custom_list.size(); i++) {
		String f = custom_list[i].strip_edges();
		if (!f.is_empty()) {
			features_list.push_back(f);
		}
	}

	feature_set.clear();
	for (const String& E : features_list) {
		feature_set.insert(E);
	}

#ifdef REAL_T_IS_DOUBLE
	feature_set.insert("double");
#else
	feature_set.insert("single");
#endif // REAL_T_IS_DOUBLE

	custom_feature_display->clear();
	String text;
	bool first = true;
	for (const String& E : feature_set) {
		if (!first) {
			text += ", ";
		}
		else {
			first = false;
		}
		text += E;
	}
	custom_feature_display->add_text(text);
}

void ProjectExportDialog::_tab_changed(int) { _update_feature_list(); }

void ProjectExportDialog::_update_parameters(const String& p_edited_property)
{
	_update_current_preset();
}

String ProjectExportDialog::get_export_path()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND_V(current.is_null(), String(""));

	return current->get_export_path();
}

Ref<EditorExportPreset> ProjectExportDialog::get_current_preset() const
{
	return EditorExport::get_singleton()->get_export_preset(presets->get_current());
}

void ProjectExportDialog::_open_key_help_link()
{
	OS::get_singleton()->shell_open(
		vformat("%s/engine_details/development/compiling/compiling_with_script_encryption_key.html",
			VLTR_VERSION_DOCS_URL));
}

bool ProjectExportDialog::_validate_script_encryption_key(const String& p_key)
{
	bool is_valid = false;

	if (!p_key.is_empty() && p_key.is_valid_hex_number(false) && p_key.length() == 64) {
		is_valid = true;
	}
	return is_valid;
}

String ProjectExportDialog::_get_resource_export_header(
	EditorExportPreset::ExportFilter p_filter) const
{
	switch (p_filter) {
	case EditorExportPreset::EXCLUDE_SELECTED_RESOURCES:
		return TTRC("Resources to exclude:");
	case EditorExportPreset::EXPORT_CUSTOMIZED:
		return TTRC("Resources to override export behavior:");
	default:
		return TTRC("Resources to export:");
	}
}

void ProjectExportDialog::_tree_popup_edited(bool p_arrow_clicked)
{
	Rect2 bounds = include_files->get_custom_popup_rect();
	bounds.position += get_global_canvas_transform().get_origin();
	bounds.size *= get_global_canvas_transform().get_scale();
	if (!is_embedding_subwindows()) {
		bounds.position += get_position();
	}
	file_mode_popup->popup(bounds);
}

void ProjectExportDialog::_patch_add_pack_pressed()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	patch_index = current->get_patches().size();
	patch_dialog->popup_file_dialog();
}

void ProjectExportDialog::_export_pck_zip()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	String dir = current->get_export_path().get_base_dir();
	export_pck_zip->set_current_dir(dir);

	export_pck_zip->popup_file_dialog();
}

void ProjectExportDialog::_export_project()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());
	Ref<EditorExportPlatform> platform = current->get_platform();
	ERR_FAIL_COND(platform.is_null());

	export_project->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	export_project->clear_filters();

	List<String> extension_list = platform->get_binary_extensions(current);
	for (const String& extension : extension_list) {
		// TRANSLATORS: This is the name of a project export file format. %s will be replaced by the
		// platform name.
		export_project->add_filter(
			"*." + extension, vformat(TTR("%s Export"), platform->get_name()));
	}

	String path = current->get_export_path();
	if (!path.is_empty()) {
		if (extension_list.find(path.get_extension()) == nullptr && extension_list.size() >= 1) {
			path = path.get_basename() + "." + extension_list.front()->get();
		}
		export_project->set_current_path(path);
	}
	else {
		if (extension_list.size() >= 1) {
			export_project->set_current_file(
				default_filename + "." + extension_list.front()->get());
		}
		else {
			export_project->set_current_file(default_filename);
		}
	}
	export_project->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	export_project->popup_file_dialog();
}

void ProjectExportDialog::_export_all_dialog_action(const String& p_str)
{
	export_all_dialog->hide();

	_export_all(p_str != "release");
}

void ProjectExportDialog::_export_all(bool p_debug)
{
	exporting = true;
	bool show_dialog = false;

	{ // Scope for the editor progress, we must free it before showing the dialog at the end.
		String export_target = p_debug ? TTR("Debug") : TTR("Release");
		EditorProgress ep("exportall", TTR("Exporting All") + " " + export_target,
			EditorExport::get_singleton()->get_export_preset_count(), true);

		result_dialog_log->clear();
		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {
			Ref<EditorExportPreset> preset = EditorExport::get_singleton()->get_export_preset(i);
			if (preset.is_null()) {
				exporting = false;
				ERR_FAIL_MSG("Failed to start the export: one of the presets is invalid.");
			}

			Ref<EditorExportPlatform> platform = preset->get_platform();
			if (platform.is_null()) {
				exporting = false;
				ERR_FAIL_MSG(
					"Failed to start the export: one of the presets has no valid platform.");
			}

			ep.step(preset->get_name(), i);

			platform->clear_messages();
			preset->update_value_overrides();
			Error err = platform->export_project(preset, p_debug, preset->get_export_path(), 0);
			if (err == ERR_SKIP) {
				exporting = false;
				return;
			}
			bool has_messages = platform->fill_log_messages(result_dialog_log, err);
			show_dialog = show_dialog || has_messages;
		}
	}

	if (show_dialog) {
		result_dialog->popup_centered_ratio(0.5);
	}

	exporting = false;
}


