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

void ProjectExportTextureFormatError::_on_fix_texture_format_pressed()
{
	export_dialog->hide();
	ProjectSettingsEditor* project_settings = EditorNode::get_singleton()->get_project_settings();
	project_settings->set_general_page("rendering/textures");
	project_settings->set_filter(setting_identifier);
	project_settings->popup_project_settings(false);
}

void ProjectExportTextureFormatError::_bind_methods() {}

void ProjectExportTextureFormatError::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		texture_format_error_label->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
	} break;
	}
}

void ProjectExportTextureFormatError::show_for_texture_format(
	const String& p_friendly_name, const String& p_setting_identifier)
{
	texture_format_error_label->set_text(vformat(
		TTR("Target platform requires '%s' texture compression. Enable 'Import %s' to fix."),
		p_friendly_name, p_friendly_name.replace_char('/', ' ')));
	setting_identifier = p_setting_identifier;
	show();
}









void ProjectExportDialog::_add_preset(int p_platform)
{
	Ref<EditorExportPreset> preset =
		EditorExport::get_singleton()->get_export_platform(p_platform)->create_preset();
	ERR_FAIL_COND(preset.is_null());

	String preset_name = EditorExport::get_singleton()->get_export_platform(p_platform)->get_name();
	int attempt = 1;
	while (true) {
		bool valid = true;

		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {
			Ref<EditorExportPreset> p = EditorExport::get_singleton()->get_export_preset(i);
			if (p->get_name() == preset_name) {
				valid = false;
				break;
			}
		}

		if (valid) {
			break;
		}

		attempt++;
		preset_name = EditorExport::get_singleton()->get_export_platform(p_platform)->get_name() +
					  " " + itos(attempt);
	}

	preset->set_name(preset_name);
	if (EditorExport::get_singleton()
			->get_runnable_preset_for_platform(preset->get_platform())
			.is_null()) {
		EditorExport::get_singleton()->set_runnable_preset(preset);
	}
	EditorExport::get_singleton()->add_export_preset(preset);
	_update_presets();
	_edit_preset(EditorExport::get_singleton()->get_export_preset_count() - 1);
}



void ProjectExportDialog::_update_current_preset() { _edit_preset(presets->get_current()); }

void ProjectExportDialog::_update_presets()
{
	updating = true;

	Ref<EditorExportPreset> current;
	if (presets->get_current() >= 0 && presets->get_current() < presets->get_item_count()) {
		current = get_current_preset();
	}

	int current_idx = -1;
	int preset_count = EditorExport::get_singleton()->get_export_preset_count();
	presets->clear();
	for (int i = 0; i < preset_count; i++) {
		Ref<EditorExportPreset> preset = EditorExport::get_singleton()->get_export_preset(i);
		if (preset == current) {
			current_idx = i;
		}
		else if (current.is_null()) {
			current_idx = i;
			_edit_preset(i);
		}

		String preset_name = preset->get_name();
		if (preset->is_runnable()) {
			preset_name += " (" + TTR("Runnable") + ")";
		}
		preset->update_files();
		presets->add_item(preset_name, preset->get_platform()->get_logo());
	}

	settings_vb->set_visible(current_idx != -1);
	empty_label->set_visible(current_idx == -1);

	if (current_idx != -1) {
		presets->select(current_idx);
	}

	updating = false;
}

void ProjectExportDialog::_update_export_all()
{
	bool can_export = EditorExport::get_singleton()->get_export_preset_count() > 0;

	for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {
		Ref<EditorExportPreset> preset = EditorExport::get_singleton()->get_export_preset(i);
		bool needs_templates;
		String error;
		if (preset->get_export_path().is_empty() ||
			!preset->get_platform()->can_export(preset, error, needs_templates)) {
			can_export = false;
			break;
		}
	}

	export_all_button->set_disabled(!can_export);

	if (can_export) {
		export_all_button->set_tooltip_text(
			TTRC("Export the project for all the presets defined."));
	}
	else {
		export_all_button->set_tooltip_text(
			TTRC("All presets must have an export path defined for Export All to work."));
	}
}



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

void ProjectExportDialog::_custom_features_changed(const String& p_text)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_custom_features(p_text);
	_update_feature_list();
}

void ProjectExportDialog::_tab_changed(int) { _update_feature_list(); }

void ProjectExportDialog::_update_parameters(const String& p_edited_property)
{
	_update_current_preset();
}





void ProjectExportDialog::_runnable_pressed()
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	if (runnable->is_pressed()) {
		EditorExport::get_singleton()->set_runnable_preset(current);
	}
	else {
		EditorExport::get_singleton()->unset_runnable_preset(current);
	}

	_update_presets();
}

void ProjectExportDialog::_name_changed(const String& p_string)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	int current_index = presets->get_current();

	String trimmed_name = p_string.strip_edges();
	if (trimmed_name.is_empty()) {
		ERR_PRINT_ED("Invalid preset name: preset name cannot be empty!");
		name->set_text(current->get_name());
		return;
	}

	if (EditorExport::get_singleton()->has_preset_with_name(trimmed_name, current_index)) {
		ERR_PRINT_ED(vformat(
			"Invalid preset name: a preset with the name '%s' already exists!", trimmed_name));
		name->set_text(current->get_name());
		return;
	}

	current->set_name(trimmed_name);
	_update_presets();
}

void ProjectExportDialog::_name_editing_finished()
{
	if (updating) {
		return;
	}

	_name_changed(name->get_text());
}

void ProjectExportDialog::set_export_path(const String& p_value)
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_export_path(p_value);
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



void ProjectExportDialog::_enc_filters_changed(const String& p_filters)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_enc_in_filter(enc_in_filters->get_text());
	current->set_enc_ex_filter(enc_ex_filters->get_text());

	updating_enc_filters = true;
	_update_current_preset();
	updating_enc_filters = false;
}

void ProjectExportDialog::_open_key_help_link()
{
	OS::get_singleton()->shell_open(
		vformat("%s/engine_details/development/compiling/compiling_with_script_encryption_key.html",
			VLTR_VERSION_DOCS_URL));
}

void ProjectExportDialog::_enc_pck_changed(bool p_pressed)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_enc_pck(p_pressed);
	enc_directory->set_disabled(!p_pressed);
	enc_in_filters->set_editable(p_pressed);
	enc_ex_filters->set_editable(p_pressed);
	script_key->set_editable(p_pressed);
	show_script_key->set_disabled(!p_pressed);
	if (!p_pressed) {
		show_script_key->set_pressed(false);
	}

	_update_current_preset();
}

void ProjectExportDialog::_seed_input_changed(const String& p_text)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_seed(seed_input->get_text().to_int());

	updating_seed = true;
	_update_current_preset();
	updating_seed = false;
}

void ProjectExportDialog::_enc_directory_changed(bool p_pressed)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_enc_directory(p_pressed);

	_update_current_preset();
}

void ProjectExportDialog::_script_encryption_key_changed(const String& p_key)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_script_encryption_key(p_key);

	updating_script_key = true;
	_update_current_preset();
	updating_script_key = false;
}

void ProjectExportDialog::_script_encryption_key_visibility_changed(bool p_visible)
{
	show_script_key->set_button_icon(get_editor_theme_icon(
		p_visible ? SNAME("GuiVisibilityVisible") : SNAME("GuiVisibilityHidden")));
	show_script_key->set_tooltip_text(
		p_visible ? TTRC("Hide encryption key") : TTRC("Show encryption key"));
	script_key->set_secret(!p_visible);
}

bool ProjectExportDialog::_validate_script_encryption_key(const String& p_key)
{
	bool is_valid = false;

	if (!p_key.is_empty() && p_key.is_valid_hex_number(false) && p_key.length() == 64) {
		is_valid = true;
	}
	return is_valid;
}

void ProjectExportDialog::_script_export_mode_changed(EditorExportPreset::ScriptExportMode p_mode)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_script_export_mode(p_mode);

	_update_current_preset();
}



void ProjectExportDialog::_delete_preset()
{
	Ref<EditorExportPreset> current = get_current_preset();
	if (current.is_null()) {
		return;
	}

	delete_confirm->set_text(vformat(TTR("Delete preset '%s'?"), current->get_name()));
	delete_confirm->popup_centered();
}

void ProjectExportDialog::_delete_preset_confirm()
{
	int idx = presets->get_current();
	EditorExport::get_singleton()->remove_export_preset(idx);
	_edit_preset(idx > 0 || presets->get_item_count() == 1 ? idx - 1 : 0);
	_update_presets();

	if (presets->get_item_count() == 0) {
		export_button->set_disabled(true);
		get_ok_button()->set_disabled(true);
	}

	// The Export All button might become enabled (if all other presets have an export path
	// defined), or it could be disabled (if there are no presets anymore).
	_update_export_all();
}







void ProjectExportDialog::_export_type_changed(int p_which)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	if (current.is_null()) {
		return;
	}

	EditorExportPreset::ExportFilter filter_type = (EditorExportPreset::ExportFilter)p_which;
	current->set_export_filter(filter_type);
	current->set_dedicated_server(filter_type == EditorExportPreset::EXPORT_CUSTOMIZED);
	server_strip_message->set_visible(filter_type == EditorExportPreset::EXPORT_CUSTOMIZED);

	// Default to stripping everything when first switching to server build.
	if (filter_type == EditorExportPreset::EXPORT_CUSTOMIZED &&
		current->get_customized_files_count() == 0) {
		current->set_file_export_mode("res://", EditorExportPreset::MODE_FILE_STRIP);
	}
	include_label->set_text(_get_resource_export_header(current->get_export_filter()));

	updating = true;
	_fill_resource_tree();
	updating = false;
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

void ProjectExportDialog::_filter_changed(const String& p_filter)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	if (current.is_null()) {
		return;
	}

	current->set_include_filter(include_filters->get_text());
	current->set_exclude_filter(exclude_filters->get_text());
}

void ProjectExportDialog::_fill_resource_tree()
{
	include_files->clear();
	include_label->hide();
	include_margin->hide();

	Ref<EditorExportPreset> current = get_current_preset();
	if (current.is_null()) {
		return;
	}

	EditorExportPreset::ExportFilter f = current->get_export_filter();

	if (f == EditorExportPreset::EXPORT_ALL_RESOURCES) {
		return;
	}

	TreeItem* root = include_files->create_item();

	if (f == EditorExportPreset::EXPORT_CUSTOMIZED) {
		include_files->set_columns(2);
		include_files->set_column_expand(1, false);
		include_files->set_column_custom_minimum_width(1, 250 * EDSCALE);
	}
	else {
		include_files->set_columns(1);
	}

	include_label->show();
	include_margin->show();

	_fill_tree(EditorFileSystem::get_singleton()->get_filesystem(), root, current, f);

	if (f == EditorExportPreset::EXPORT_CUSTOMIZED) {
		_propagate_file_export_mode(
			include_files->get_root(), EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED);
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



void ProjectExportDialog::_patch_delta_encoding_changed(bool p_pressed)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_patch_delta_encoding_enabled(p_pressed);

	_update_current_preset();
}

void ProjectExportDialog::_patch_delta_include_filter_changed(const String& p_filter)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_patch_delta_include_filter(patch_delta_include_filter->get_text());

	updating_patch_delta_filters = true;
	_update_current_preset();
	updating_patch_delta_filters = false;
}

void ProjectExportDialog::_patch_delta_exclude_filter_changed(const String& p_filter)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_patch_delta_exclude_filter(patch_delta_exclude_filter->get_text());

	updating_patch_delta_filters = true;
	_update_current_preset();
	updating_patch_delta_filters = false;
}

void ProjectExportDialog::_patch_delta_zstd_level_changed(double p_value)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_patch_delta_zstd_level((int)p_value);

	_update_current_preset();
}

void ProjectExportDialog::_patch_delta_min_reduction_changed(double p_value)
{
	if (updating) {
		return;
	}

	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	current->set_patch_delta_min_reduction(p_value / 100.0);

	_update_current_preset();
}





void ProjectExportDialog::_patch_file_selected(const String& p_path)
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	String relative_path =
		ProjectSettings::get_singleton()->get_resource_path().path_to_file(p_path);

	Vector<String> preset_patches = current->get_patches();
	if (patch_index >= preset_patches.size()) {
		current->add_patch(relative_path);
	}
	else {
		current->set_patch(patch_index, relative_path);
	}

	_update_current_preset();
}

void ProjectExportDialog::_patch_delete_confirmed()
{
	Ref<EditorExportPreset> current = get_current_preset();
	ERR_FAIL_COND(current.is_null());

	Vector<String> preset_patches = current->get_patches();
	if (patch_index < preset_patches.size()) {
		current->remove_patch(patch_index);
		_update_current_preset();
	}
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



void ProjectExportDialog::_open_export_template_manager()
{
	hide();
	EditorNode::get_singleton()->open_export_template_manager();
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



void ProjectExportDialog::_export_all_dialog()
{
	export_all_dialog->show();
	export_all_dialog->popup_centered(Size2(300, 80));
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


