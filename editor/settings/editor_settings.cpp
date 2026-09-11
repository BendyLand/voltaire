/**************************************************************************/
/*  editor_settings.cpp                                                   */
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
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/certs_compressed.gen.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/ip.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "core/templates/rb_set.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_paths.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/project_manager/engine_update_label.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation.h"
#include "editor_settings.h"
#include "main/main.h"
#include "modules/regex/regex.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/file_dialog.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/animation.h"
#include "servers/display/display_server.h"

// PRIVATE METHODS

Ref<EditorSettings> EditorSettings::singleton = nullptr;

// Properties

struct _EVCSort
{
	String name;
	int order = 0;
	bool basic = false;
	bool save = false;
	bool restart_if_changed = false;

	bool operator<(const _EVCSort& p_vcs) const { return order < p_vcs.order; }
};

// Default configs

void EditorSettings::_set_initialized() { initialized = true; }

static LocalVector<String> _get_skipped_locales()
{
	// Skip locales if Text server lack required features.
	LocalVector<String> locales_to_skip;
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT) ||
		!TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("ar"); // Arabic.
		locales_to_skip.push_back("fa"); // Persian.
		locales_to_skip.push_back("ur"); // Urdu.
	}
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT)) {
		locales_to_skip.push_back("he"); // Hebrew.
	}
	if (!TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("bn"); // Bengali.
		locales_to_skip.push_back("hi"); // Hindi.
		locales_to_skip.push_back("ml"); // Malayalam.
		locales_to_skip.push_back("si"); // Sinhala.
		locales_to_skip.push_back("ta"); // Tamil.
		locales_to_skip.push_back("te"); // Telugu.
	}
	return locales_to_skip;
}

const String EditorSettings::_get_project_metadata_path() const
{
	return EditorPaths::get_singleton()->get_project_settings_dir().path_join(
		"project_metadata.cfg");
}

#ifndef DISABLE_DEPRECATED
void EditorSettings::_handle_setting_compatibility()
{
	// Remove deprecated settings.
	erase("interface/theme/preset");
	erase("network/connection/engine_version_update_mode");
	erase("run/output/always_open_output_on_play");
	erase("run/output/always_close_output_on_stop");
	erase("text_editor/theme/line_spacing"); // See GH-106137.
	erase("interface/editors/show_scene_tree_root_selection");
	erase("asset_library/available_urls"); // Workaround bugged settings treating the previous
										   // default as a modified value (see GH-118755).

	// Handle renamed settings.
	_rename_setting(
		"interface/editor/editor_language", "interface/editor/localization/editor_language");
	_rename_setting(
		"interface/editor/localize_settings", "interface/editor/localization/localize_settings");
	_rename_setting("interface/editor/dock_tab_style", "interface/editor/docks/dock_tab_style");
	_rename_setting(
		"interface/editor/bottom_dock_tab_style", "interface/editor/docks/bottom_dock_tab_style");
	_rename_setting("interface/editor/ui_layout_direction",
		"interface/editor/localization/ui_layout_direction");
	_rename_setting("interface/editor/display_scale", "interface/editor/appearance/display_scale");
	_rename_setting("interface/editor/custom_display_scale",
		"interface/editor/appearance/custom_display_scale");
	_rename_setting("interface/editor/editor_screen", "interface/editor/appearance/editor_screen");
	_rename_setting("interface/editor/tablet_driver", "interface/editor/input/tablet_driver");
	_rename_setting("interface/editor/project_manager_screen",
		"interface/editor/appearance/project_manager_screen");
	_rename_setting(
		"interface/editor/use_embedded_menu", "interface/editor/appearance/use_embedded_menu");
	_rename_setting("interface/editor/use_native_file_dialogs",
		"interface/editor/appearance/use_native_file_dialogs");
	_rename_setting(
		"interface/editor/expand_to_title", "interface/editor/appearance/expand_to_title");
	_rename_setting("interface/editor/main_font_size", "interface/editor/fonts/main_font_size");
	_rename_setting("interface/editor/code_font_size", "interface/editor/fonts/code_font_size");
	_rename_setting("interface/editor/main_font_custom_opentype_features",
		"interface/editor/fonts/main_font_custom_opentype_features");
	_rename_setting("interface/editor/code_font_contextual_ligatures",
		"interface/editor/fonts/code_font_contextual_ligatures");
	_rename_setting("interface/editor/code_font_custom_opentype_features",
		"interface/editor/fonts/code_font_custom_opentype_features");
	_rename_setting("interface/editor/code_font_custom_variations",
		"interface/editor/fonts/code_font_custom_variations");
	_rename_setting(
		"interface/editor/font_antialiasing", "interface/editor/fonts/font_antialiasing");
	_rename_setting("interface/editor/font_hinting", "interface/editor/fonts/font_hinting");
	_rename_setting("interface/editor/font_subpixel_positioning",
		"interface/editor/fonts/font_subpixel_positioning");
	_rename_setting("interface/editor/font_disable_embedded_bitmaps",
		"interface/editor/fonts/font_disable_embedded_bitmaps");
	_rename_setting("interface/editor/font_allow_msdf", "interface/editor/fonts/font_allow_msdf");
	_rename_setting("interface/editor/main_font", "interface/editor/fonts/main_font");
	_rename_setting("interface/editor/main_font_bold", "interface/editor/fonts/main_font_bold");
	_rename_setting("interface/editor/code_font", "interface/editor/fonts/code_font");
	_rename_setting("interface/editor/dragging_hover_wait_seconds",
		"interface/editor/timers/dragging_hover_wait_seconds");
	_rename_setting("interface/editor/separate_distraction_mode",
		"interface/editor/behavior/separate_distraction_mode");
	_rename_setting("interface/editor/automatically_open_screenshots",
		"interface/editor/behavior/automatically_open_screenshots");
	_rename_setting(
		"interface/editor/single_window_mode", "interface/editor/display/single_window_mode");
	_rename_setting("interface/editor/mouse_extra_buttons_navigate_history",
		"interface/editor/input/mouse_extra_buttons_navigate_history");
	_rename_setting("interface/editor/save_each_scene_on_quit",
		"interface/editor/behavior/save_each_scene_on_quit");
	_rename_setting(
		"interface/editor/save_on_focus_loss", "interface/editor/behavior/save_on_focus_loss");
	_rename_setting("interface/editor/accept_dialog_cancel_ok_buttons",
		"interface/editor/appearance/accept_dialog_cancel_ok_buttons");
	_rename_setting("interface/editor/show_internal_errors_in_toast_notifications",
		"interface/editor/behavior/show_internal_errors_in_toast_notifications");
	_rename_setting(
		"interface/editor/show_update_spinner", "interface/editor/appearance/show_update_spinner");
	_rename_setting("interface/editor/keep_screen_on", "interface/editor/display/keep_screen_on");
	_rename_setting("interface/editor/low_processor_mode_sleep_usec",
		"interface/editor/timers/low_processor_mode_sleep_usec");
	_rename_setting("interface/editor/unfocused_low_processor_mode_sleep_usec",
		"interface/editor/timers/unfocused_low_processor_mode_sleep_usec");
	_rename_setting("interface/editor/import_resources_when_unfocused",
		"interface/editor/behavior/import_resources_when_unfocused");
	_rename_setting("interface/editor/vsync_mode", "interface/editor/display/vsync_mode");
	_rename_setting(
		"interface/editor/update_continuously", "interface/editor/display/update_continuously");
	_rename_setting(
		"interface/editor/collapse_main_menu", "interface/editor/appearance/collapse_main_menu");
	_rename_setting("asset_library/use_threads", "asset_store/use_threads");
	_rename_setting("interface/editors/derive_script_globals_by_name",
		"docks/scene_tree/derive_script_globals_by_name");

	// Handle renamed shortcuts.
	_rename_shortcut("editor/editor_assetlib", "editor/editor_asset_store");
}

void EditorSettings::_rename_shortcut(const String& p_old_path, const String& p_new_path)
{
	if (!shortcuts.has(p_old_path)) {
		return;
	}
	if (!shortcuts.has(p_new_path)) {
		shortcuts[p_new_path] = shortcuts[p_old_path];
	}
	shortcuts.erase(p_old_path);
}
#endif

// PUBLIC METHODS

EditorSettings* EditorSettings::get_singleton() { return singleton.ptr(); }

String EditorSettings::get_existing_settings_path()
{
	const String config_dir = EditorPaths::get_singleton()->get_config_dir();
	int minor = VLTR_VERSION_MINOR;
	String filename;

	do {
		if (VLTR_VERSION_MAJOR == 4 && minor < 3) {
			// Minor version is used since 4.3, so special case to load older settings.
			filename = vformat("editor_settings-%d.tres", VLTR_VERSION_MAJOR);
			minor = -1;
		}
		else {
			filename = vformat("editor_settings-%d.%d.tres", VLTR_VERSION_MAJOR, minor);
			minor--;
		}
	} while (minor >= 0 && !FileAccess::exists(config_dir.path_join(filename)));
	return config_dir.path_join(filename);
}

String EditorSettings::get_newest_settings_path()
{
	const String config_file_name =
		vformat("editor_settings-%d.%d.tres", VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR);
	return EditorPaths::get_singleton()->get_config_dir().path_join(config_file_name);
}

void EditorSettings::init_shortcuts()
{
	ED_SHORTCUT("editor/open_search", TTRC("Focus Search/Filter Bar"),
		KeyModifierMask::CMD_OR_CTRL | Key::F);
}

void EditorSettings::save()
{
	//_THREAD_SAFE_METHOD_

	if (!singleton.ptr()) {
		return;
	}

	Error err = ResourceSaver::save(singleton.ptr());

	if (err != OK) {
		ERR_PRINT("Error saving editor settings to " + singleton->get_path());
	}
	else {
		singleton->changed_settings.clear();
		print_verbose("EditorSettings: Save OK!");
	}
}

PackedStringArray EditorSettings::get_changed_settings() const
{
	PackedStringArray arr;
	for (const String& setting : changed_settings) {
		arr.push_back(setting);
	}

	return arr;
}

bool EditorSettings::check_changed_settings_in_group(const String& p_setting_prefix) const
{
	for (const String& setting : changed_settings) {
		if (setting.begins_with(p_setting_prefix)) {
			return true;
		}
	}

	return false;
}

void EditorSettings::mark_setting_changed(const String& p_setting)
{
	changed_settings.insert(p_setting);
}

void EditorSettings::destroy()
{
	if (!singleton.ptr()) {
		return;
	}
	save();
	singleton = Ref<EditorSettings>();
}

void EditorSettings::set_optimize_save(bool p_optimize) { optimize_save = p_optimize; }

// Properties

// Metadata

void EditorSettings::save_project_metadata()
{
	if (!project_metadata_dirty) {
		return;
	}
	const String path = _get_project_metadata_path();
	Error err = project_metadata->save(path);
	ERR_FAIL_COND_MSG(err != OK, "Cannot save project metadata to file '" + path + "'.");
	project_metadata_dirty = false;
}

void EditorSettings::set_favorites_bind(const Vector<String>& p_favorites)
{
	favorites = p_favorites;
	String favorites_file;
	if (Engine::get_singleton()->is_project_manager_hint()) {
		favorites_file = EditorPaths::get_singleton()->get_config_dir().path_join("favorite_dirs");
	}
	else {
		favorites_file =
			EditorPaths::get_singleton()->get_project_settings_dir().path_join("favorites");
	}
	Ref<FileAccess> f = FileAccess::open(favorites_file, FileAccess::WRITE);
	if (f.is_valid()) {
		for (int i = 0; i < favorites.size(); i++) {
			f->store_line(favorites[i]);
		}
	}
}

Vector<String> EditorSettings::get_favorites() const { return favorites; }

Vector<String> EditorSettings::get_favorite_folders() const
{
	Vector<String> folder_favorites;
	folder_favorites.resize(favorites.size());
	String* folder_write = folder_favorites.ptrw();

	int i = 0;
	for (const String& fav : favorites) {
		if (fav.ends_with("/")) {
			folder_write[i] = fav;
			i++;
		}
	}
	folder_favorites.resize(i);
	return folder_favorites;
}

HashMap<String, PackedStringArray> EditorSettings::get_favorite_properties() const
{
	return HashMap<String, PackedStringArray>(favorite_properties);
}

void EditorSettings::set_recent_dirs(const Vector<String>& p_recent_dirs, bool p_update_file_dialog)
{
	if (p_update_file_dialog) {
		FileDialog::set_recent_list(p_recent_dirs);
	}
	set_recent_dirs_bind(p_recent_dirs);
}

void EditorSettings::set_recent_dirs_bind(const Vector<String>& p_recent_dirs)
{
	recent_dirs = p_recent_dirs;
	String recent_dirs_file;
	if (Engine::get_singleton()->is_project_manager_hint()) {
		recent_dirs_file = EditorPaths::get_singleton()->get_config_dir().path_join("recent_dirs");
	}
	else {
		recent_dirs_file =
			EditorPaths::get_singleton()->get_project_settings_dir().path_join("recent_dirs");
	}
	Ref<FileAccess> f = FileAccess::open(recent_dirs_file, FileAccess::WRITE);
	if (f.is_valid()) {
		for (int i = 0; i < recent_dirs.size(); i++) {
			f->store_line(recent_dirs[i]);
		}
	}
}

Vector<String> EditorSettings::get_recent_dirs() const { return recent_dirs; }

HashMap<StringName, Color> EditorSettings::get_godot2_text_editor_theme()
{
	// Godot 2 is only a dark theme; it doesn't have a light theme counterpart.
	HashMap<StringName, Color> colors;
	colors["text_editor/theme/highlighting/symbol_color"] = Color(0.73, 0.87, 1.0);
	colors["text_editor/theme/highlighting/keyword_color"] = Color(1.0, 1.0, 0.7);
	colors["text_editor/theme/highlighting/control_flow_keyword_color"] = Color(1.0, 0.85, 0.7);
	colors["text_editor/theme/highlighting/base_type_color"] = Color(0.64, 1.0, 0.83);
	colors["text_editor/theme/highlighting/engine_type_color"] = Color(0.51, 0.83, 1.0);
	colors["text_editor/theme/highlighting/user_type_color"] = Color(0.42, 0.67, 0.93);
	colors["text_editor/theme/highlighting/comment_color"] = Color(0.4, 0.4, 0.4);
	colors["text_editor/theme/highlighting/doc_comment_color"] = Color(0.5, 0.6, 0.7);
	colors["text_editor/theme/highlighting/string_color"] = Color(0.94, 0.43, 0.75);
	colors["text_editor/theme/highlighting/string_placeholder_color"] = Color(1, 0.75, 0.4);
	colors["text_editor/theme/highlighting/background_color"] = Color(0.13, 0.12, 0.15);
	colors["text_editor/theme/highlighting/completion_background_color"] = Color(0.17, 0.16, 0.2);
	colors["text_editor/theme/highlighting/completion_selected_color"] = Color(0.26, 0.26, 0.27);
	colors["text_editor/theme/highlighting/completion_existing_color"] =
		Color(0.87, 0.87, 0.87, 0.13);
	colors["text_editor/theme/highlighting/completion_scroll_color"] = Color(1, 1, 1, 0.29);
	colors["text_editor/theme/highlighting/completion_scroll_hovered_color"] = Color(1, 1, 1, 0.4);
	colors["text_editor/theme/highlighting/completion_font_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/text_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/line_number_color"] = Color(0.67, 0.67, 0.67, 0.4);
	colors["text_editor/theme/highlighting/safe_line_number_color"] = Color(0.67, 0.78, 0.67, 0.6);
	colors["text_editor/theme/highlighting/caret_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/caret_background_color"] = Color(0, 0, 0);
	colors["text_editor/theme/highlighting/text_selected_color"] = Color(0, 0, 0, 0);
	colors["text_editor/theme/highlighting/selection_color"] = Color(0.41, 0.61, 0.91, 0.35);
	colors["text_editor/theme/highlighting/brace_mismatch_color"] = Color(1, 0.2, 0.2);
	colors["text_editor/theme/highlighting/current_line_color"] = Color(0.3, 0.5, 0.8, 0.15);
	colors["text_editor/theme/highlighting/line_length_guideline_color"] =
		Color(0.3, 0.5, 0.8, 0.1);
	colors["text_editor/theme/highlighting/word_highlighted_color"] = Color(0.8, 0.9, 0.9, 0.15);
	colors["text_editor/theme/highlighting/number_color"] = Color(0.92, 0.58, 0.2);
	colors["text_editor/theme/highlighting/function_color"] = Color(0.4, 0.64, 0.81);
	colors["text_editor/theme/highlighting/member_variable_color"] = Color(0.9, 0.31, 0.35);
	colors["text_editor/theme/highlighting/mark_color"] = Color(1.0, 0.4, 0.4, 0.4);
	colors["text_editor/theme/highlighting/warning_color"] = Color(1.0, 0.8, 0.4, 0.1);
	colors["text_editor/theme/highlighting/bookmark_color"] = Color(0.08, 0.49, 0.98);
	colors["text_editor/theme/highlighting/breakpoint_color"] = Color(0.9, 0.29, 0.3);
	colors["text_editor/theme/highlighting/executing_line_color"] = Color(0.98, 0.89, 0.27);
	colors["text_editor/theme/highlighting/code_folding_color"] = Color(0.8, 0.8, 0.8, 0.8);
	colors["text_editor/theme/highlighting/folded_code_region_color"] =
		Color(0.68, 0.46, 0.77, 0.2);
	colors["text_editor/theme/highlighting/search_result_color"] = Color(0.05, 0.25, 0.05, 1);
	colors["text_editor/theme/highlighting/search_result_border_color"] =
		Color(0.41, 0.61, 0.91, 0.38);
	colors["text_editor/theme/highlighting/gdscript/function_definition_color"] =
		Color(0.4, 0.9, 1.0);

	colors["text_editor/theme/highlighting/gdscript/global_function_color"] =
		Color(0.64, 0.64, 0.96);
	colors["text_editor/theme/highlighting/gdscript/node_path_color"] = Color(0.72, 0.77, 0.49);
	colors["text_editor/theme/highlighting/gdscript/node_reference_color"] =
		Color(0.39, 0.76, 0.35);
	colors["text_editor/theme/highlighting/gdscript/annotation_color"] = Color(1.0, 0.7, 0.45);
	colors["text_editor/theme/highlighting/gdscript/string_name_color"] = Color(1.0, 0.76, 0.65);
	colors["text_editor/theme/highlighting/comment_markers/critical_color"] =
		Color(0.77, 0.35, 0.35);
	colors["text_editor/theme/highlighting/comment_markers/warning_color"] =
		Color(0.72, 0.61, 0.48);
	colors["text_editor/theme/highlighting/comment_markers/notice_color"] = Color(0.56, 0.67, 0.51);
	return colors;
}

bool EditorSettings::is_default_text_editor_theme(const String& p_theme_name)
{
	return p_theme_name == "default" || p_theme_name == "godot 2" || p_theme_name == "custom";
}

Vector<String> EditorSettings::get_script_templates(
	const String& p_extension, const String& p_custom_path)
{
	Vector<String> templates;
	String template_dir = EditorPaths::get_singleton()->get_script_templates_dir();
	if (!p_custom_path.is_empty()) {
		template_dir = p_custom_path;
	}
	Ref<DirAccess> d = DirAccess::open(template_dir);
	if (d.is_valid()) {
		d->list_dir_begin();
		String file = d->get_next();
		while (!file.is_empty()) {
			if (file.get_extension() == p_extension) {
				templates.push_back(file.get_basename());
			}
			file = d->get_next();
		}
		d->list_dir_end();
	}
	return templates;
}

String EditorSettings::get_editor_layouts_config() const
{
	return EditorPaths::get_singleton()->get_config_dir().path_join("editor_layouts.cfg");
}

float EditorSettings::get_auto_display_scale()
{
#ifdef LINUXBSD_ENABLED
	if (DisplayServer::get_singleton()->get_name() == "Wayland") {
		float main_window_scale = DisplayServer::get_singleton()->screen_get_scale(
			DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);

		if (DisplayServer::get_singleton()->get_screen_count() == 1 ||
			Math::fract(main_window_scale) != 0) {
			// If we have a single screen or the screen of the window is fractional, all
			// bets are off. At this point, let's just return the current's window scale,
			// which is special-cased to the scale of `DisplayServerEnums::SCREEN_OF_MAIN_WINDOW`.
			return main_window_scale;
		}

		// If the above branch didn't fire, fractional scaling isn't going to work
		// properly anyways (we're need the ability to change the UI scale at runtime).
		// At this point it's more convenient to "supersample" like we do with other
		// platforms, hoping that the user is only using integer-scaled screens.
		return DisplayServer::get_singleton()->screen_get_max_scale();
	}
#endif

#if defined(MACOS_ENABLED) || defined(ANDROID_ENABLED)
	return DisplayServer::get_singleton()->screen_get_max_scale();
#else
	const int screen = DisplayServer::get_singleton()->window_get_current_screen();

	if (DisplayServer::get_singleton()->screen_get_size(screen) == Vector2i()) {
		// Invalid screen size, skip.
		return 1.0;
	}

#if defined(WINDOWS_ENABLED)
	return DisplayServer::get_singleton()->screen_get_dpi(screen) / 96.0;
#else
	// Use the smallest dimension to use a correct display scale on portrait displays.
	const int smallest_dimension = MIN(DisplayServer::get_singleton()->screen_get_size(screen).x,
		DisplayServer::get_singleton()->screen_get_size(screen).y);
	if (DisplayServer::get_singleton()->screen_get_dpi(screen) >= 192 &&
		smallest_dimension >= 1400) {
		// hiDPI display.
		return 2.0;
	}
	else if (smallest_dimension >= 1700) {
		// Likely a hiDPI display, but we aren't certain due to the returned DPI.
		// Use an intermediate scale to handle this situation.
		return 1.5;
	}
	else if (smallest_dimension <= 800) {
		// Small loDPI display. Use a smaller display scale so that editor elements fit more easily.
		// Icons won't look great, but this is better than having editor elements overflow from its
		// window.
		return 0.75;
	}
	return 1.0;
#endif // defined(WINDOWS_ENABLED)

#endif // defined(MACOS_ENABLED) || defined(ANDROID_ENABLED)
}

// Shortcuts

void EditorSettings::_add_shortcut_default(const String& p_path, const Ref<Shortcut>& p_shortcut)
{
	shortcuts[p_path] = p_shortcut;
}

void EditorSettings::remove_shortcut(const String& p_path) { shortcuts.erase(p_path); }

bool EditorSettings::is_shortcut(const String& p_path, const Ref<InputEvent>& p_event) const
{
	HashMap<String, Ref<Shortcut>>::ConstIterator E = shortcuts.find(p_path);
	ERR_FAIL_COND_V_MSG(!E, false, "Unknown Shortcut: " + p_path + ".");

	return E->value->matches_event(p_event);
}

bool EditorSettings::has_shortcut(const String& p_path) const
{
	return get_shortcut(p_path).is_valid();
}

Ref<Shortcut> EditorSettings::get_shortcut(const String& p_path) const
{
	HashMap<String, Ref<Shortcut>>::ConstIterator SC = shortcuts.find(p_path);
	if (SC) {
		return SC->value;
	}

	// If no shortcut with the provided name is found in the list, check the built-in shortcuts.
	// Use the first item in the action list for the shortcut event, since a shortcut can only have
	// 1 linked event.

	Ref<Shortcut> sc;
	HashMap<String, List<Ref<InputEvent>>>::ConstIterator builtin_override =
		builtin_action_overrides.find(p_path);
	if (builtin_override) {
		sc.instantiate();
		sc->set_events_list(&builtin_override->value);
		sc->set_name(InputMap::get_singleton()->get_builtin_display_name(p_path));
	}

	// If there was no override, check the default builtins to see if it has an InputEvent for the
	// provided name.
	if (sc.is_null()) {
		HashMap<String, List<Ref<InputEvent>>>::ConstIterator builtin_default =
			InputMap::get_singleton()->get_builtins_with_feature_overrides_applied().find(p_path);
		if (builtin_default) {
			sc.instantiate();
			sc->set_events_list(&builtin_default->value);
			sc->set_name(InputMap::get_singleton()->get_builtin_display_name(p_path));
		}
	}

	if (sc.is_valid()) {
		// Add the shortcut to the list.
		shortcuts[p_path] = sc;
		return sc;
	}

	return Ref<Shortcut>();
}

Vector<String> EditorSettings::_get_shortcut_list()
{
	List<String> shortcut_list;
	get_shortcut_list(&shortcut_list);
	Vector<String> ret;
	for (const String& shortcut : shortcut_list) {
		ret.push_back(shortcut);
	}
	return ret;
}

void EditorSettings::get_shortcut_list(List<String>* r_shortcuts)
{
	for (const KeyValue<String, Ref<Shortcut>>& E : shortcuts) {
		r_shortcuts->push_back(E.key);
	}
}

Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path)
{
	ERR_FAIL_NULL_V_MSG(
		EditorSettings::get_singleton(), nullptr, "EditorSettings not instantiated yet.");

	Ref<Shortcut> sc = EditorSettings::get_singleton()->get_shortcut(p_path);

	ERR_FAIL_COND_V_MSG(sc.is_null(), sc, "Used ED_GET_SHORTCUT with invalid shortcut: " + p_path);

	return sc;
}

void ED_SHORTCUT_OVERRIDE(
	const String& p_path, const String& p_feature, Key p_keycode, bool p_physical)
{
	if (!EditorSettings::get_singleton()) {
		return;
	}

	Ref<Shortcut> sc = EditorSettings::get_singleton()->get_shortcut(p_path);
	ERR_FAIL_COND_MSG(sc.is_null(), "Used ED_SHORTCUT_OVERRIDE with invalid shortcut: " + p_path);

	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);

	ED_SHORTCUT_OVERRIDE_ARRAY(p_path, p_feature, arr, p_physical);
}

Ref<Shortcut> ED_SHORTCUT(
	const String& p_path, const String& p_name, Key p_keycode, bool p_physical)
{
	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);
	return ED_SHORTCUT_ARRAY(p_path, p_name, arr, p_physical);
}


