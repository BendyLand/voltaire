/**************************************************************************/
/*  script_editor_plugin.cpp                                              */
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
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/fuzzy_search.h"
#include "core/version.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/doc/editor_help.h"
#include "editor/doc/editor_help_search.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_interface.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/code_editor.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/gui/window_wrapper.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/find_in_files.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/script/text_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_editor_plugin.h"
#include "editor/shader/text_shader_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/separator.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "script_editor_plugin.h"
#include "servers/display/display_server.h"

void ScriptEditorQuickOpen::popup_dialog(const Vector<String>& p_functions, bool p_dontclear)
{
	popup_centered_ratio(0.6);
	if (p_dontclear) {
		search_box->select_all();
	}
	else {
		search_box->clear();
	}
	search_box->grab_focus();
	functions = p_functions;
	_update_search();
}

void ScriptEditorQuickOpen::_text_changed(const String& p_newtext) { _update_search(); }

void ScriptEditorQuickOpen::_update_search()
{
	search_options->clear();
	TreeItem* root = search_options->create_item();

	for (int i = 0; i < functions.size(); i++) {
		String file = functions[i];
		if ((search_box->get_text().is_empty() || file.containsn(search_box->get_text()))) {
			TreeItem* ti = search_options->create_item(root);
			ti->set_text(0, file);
			if (root->get_first_child() == ti) {
				ti->select(0);
			}
		}
	}

	get_ok_button()->set_disabled(root->get_first_child() == nullptr);
}

void ScriptEditorQuickOpen::_confirmed()
{
	TreeItem* ti = search_options->get_selected();
	if (!ti) {
		return;
	}
	int line = ti->get_text(0).get_slicec(':', 1).to_int();

	hide();
}

ScriptEditorQuickOpen::ScriptEditorQuickOpen()
{
	set_ok_button_text(TTRC("Open"));
	get_ok_button()->set_disabled(true);
	set_hide_on_ok(false);

	VBoxContainer* vbc = memnew(VBoxContainer);
	add_child(vbc);

	search_box = memnew(FilterLineEdit);
	vbc->add_margin_child(TTRC("Search:"), search_box);
	register_text_enter(search_box);

	search_options = memnew(Tree);
	search_box->set_forward_control(search_options);
	search_options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	search_options->set_hide_root(true);
	search_options->set_hide_folding(true);
	search_options->add_theme_constant_override("draw_guides", 1);
	vbc->add_margin_child(TTRC("Matches:"), search_options, true);
}

/////////////////////////////////

void DocumentOutline::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		sort_button->set_button_icon(get_editor_theme_icon(SNAME("Sort")));

		update_visibility();
	} break;
	}
}

/////////////////////////////////

ScriptEditor* ScriptEditor::script_editor = nullptr;

/*** SCRIPT EDITOR ******/

void ScriptEditor::_update_history_arrows()
{
	script_back->set_disabled(history_pos <= 0);
	script_forward->set_disabled(history_pos >= history.size() - 1);
}

// Compress the history and remove duplicate patterns.
// Example 1: If the history is ...ABAB..., it will be compressed to ...AB....
// Example 2: If the history is ...ABCABC..., it will be compressed to ...ABC....

void ScriptEditor::_show_error_dialog(const String& p_path)
{
	error_dialog->set_text(
		vformat(TTR("Can't open '%s'. The file could have been moved or deleted."), p_path));
	error_dialog->popup_centered();
}

void ScriptEditor::_close_current_tab(bool p_save)
{
	_close_tab(tab_container->get_current_tab(), p_save);
}

void ScriptEditor::_copy_script_path()
{
	if (ScriptEditorBase* seb = _get_current_editor()) {
		Ref<Resource> scr = seb->get_edited_resource();
		DisplayServer::get_singleton()->clipboard_set(scr->get_path());
	}
}

void ScriptEditor::_copy_script_uid()
{
	if (ScriptEditorBase* seb = _get_current_editor()) {
		Ref<Resource> scr = seb->get_edited_resource();
		ResourceUID::ID uid = ResourceLoader::get_resource_uid(scr->get_path());
		DisplayServer::get_singleton()->clipboard_set(
			ResourceUID::get_singleton()->id_to_text(uid));
	}
}

void ScriptEditor::_close_other_tabs()
{
	int current_idx = tab_container->get_current_tab();
	for (int i = tab_container->get_tab_count() - 1; i >= 0; i--) {
		if (i != current_idx) {
			script_close_queue.push_back(i);
		}
	}
	_queue_close_tabs();
}

void ScriptEditor::_close_tabs_below()
{
	int current_idx = tab_container->get_current_tab();
	for (int i = tab_container->get_tab_count() - 1; i > current_idx; i--) {
		script_close_queue.push_back(i);
	}
	_go_to_tab(current_idx, true);
	_queue_close_tabs();
}

void ScriptEditor::_close_all_tabs()
{
	for (int i = tab_container->get_tab_count() - 1; i >= 0; i--) {
		script_close_queue.push_back(i);
	}
	_queue_close_tabs();
}

void ScriptEditor::_ask_close_current_unsaved_tab(ScriptEditorBase* current)
{
	erase_tab_confirm->set_text(
		TTR("Close and save changes?") + "\n\"" + current->get_name() + "\"");
	erase_tab_confirm->popup_centered();
}

void ScriptEditor::_scene_saved_callback(const String& p_path)
{
	// If scene was saved, mark all built-in scripts from that scene as saved.
	_mark_built_in_scripts_as_saved(p_path);
}

void ScriptEditor::_live_auto_reload_running_scripts()
{
	pending_auto_reload = false;
	EditorDebuggerNode::get_singleton()->reload_scripts(script_paths_to_reload);
	script_paths_to_reload.clear();
}

bool ScriptEditor::_script_exists(const String& p_path) const
{
	if (p_path.is_empty()) {
		return false;
	}
	else if (p_path.is_resource_file()) {
		return FileAccess::exists(p_path);
	}
	else {
		return FileAccess::exists(p_path.get_slice("::", 0));
	}
}

bool ScriptEditor::is_files_panel_toggled() { return list_split->is_visible(); }

List<String> ScriptEditor::_get_recognized_extensions()
{
	List<String> extensions;
	for (const String type : {"Script", "JSON"}) {
		ResourceLoader::get_recognized_extensions_for_type(type, &extensions);
	}
	return extensions;
}

void ScriptEditor::_theme_option(int p_option)
{
	switch (p_option) {
	case THEME_IMPORT: {
		file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
		file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
		file_dialog_option = THEME_IMPORT;
		file_dialog->clear_filters();
		file_dialog->add_filter("*.tet");
		file_dialog->set_title(TTRC("Import Theme"));
		file_dialog->popup_file_dialog();
	} break;
	case THEME_RELOAD: {
		EditorSettings::get_singleton()->mark_setting_changed("text_editor/theme/color_theme");
		EditorSettings::get_singleton()->notify_changes();
	} break;
	case THEME_SAVE_AS: {
		ScriptEditor::_show_save_theme_as_dialog();
	} break;
	}
}

void ScriptEditor::_prepare_file_menu()
{
	PopupMenu* menu = file_menu->get_popup();
	ScriptEditorBase* editor = _get_current_editor();
	const Ref<Resource> res = editor ? editor->get_edited_resource() : Ref<Resource>();

	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_REOPEN_CLOSED), previous_scripts.is_empty());

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE), res.is_null());
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE_AS), res.is_null());
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE_ALL), !_has_script_tab());

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SOFT_RELOAD_TOOL), res.is_null());
	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_COPY_PATH), res.is_null() || res->get_path().is_empty());
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_COPY_UID),
		res.is_null() ||
			ResourceLoader::get_resource_uid(res->get_path()) == ResourceUID::INVALID_ID);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SHOW_IN_FILE_SYSTEM), res.is_null());

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_HISTORY_PREV), history_pos <= 0);
	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_HISTORY_NEXT), history_pos >= history.size() - 1);

	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_CLOSE), tab_container->get_tab_count() < 1);
	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_CLOSE_ALL), tab_container->get_tab_count() < 1);
	menu->set_item_disabled(
		menu->get_item_index(FILE_MENU_CLOSE_OTHER_TABS), tab_container->get_tab_count() <= 1);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE_TABS_BELOW),
		tab_container->get_current_tab() >= tab_container->get_tab_count() - 1);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE_DOCS), !_has_docs_tab());

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_RUN), res.is_null());
}

void ScriptEditor::_file_menu_closed()
{
	PopupMenu* menu = file_menu->get_popup();

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_REOPEN_CLOSED), false);

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE_AS), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SAVE_ALL), false);

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SOFT_RELOAD_TOOL), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_COPY_PATH), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_SHOW_IN_FILE_SYSTEM), false);

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_HISTORY_PREV), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_HISTORY_NEXT), false);

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE_ALL), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE_OTHER_TABS), false);
	menu->set_item_disabled(menu->get_item_index(FILE_MENU_CLOSE_DOCS), false);

	menu->set_item_disabled(menu->get_item_index(FILE_MENU_RUN), false);
}

void ScriptEditor::_tab_changed(int p_which) { ensure_select_current(); }

Vector<String> ScriptEditor::_get_breakpoints()
{
	List<String> bpoints_list;
	get_breakpoints(&bpoints_list);

	Vector<String> ret;
	for (const String& E : bpoints_list) {
		ret.push_back(E);
	}

	return ret;
}

bool ScriptEditor::is_editor_floating() { return is_floating; }

void ScriptEditor::_connect_to_scene()
{
	if (!highlight_scene_scripts) {
		return;
	}
	Node* edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (!edited_scene) {
		return;
	}
	_connect_to_scene_recursive(edited_scene, edited_scene);
}

struct _ScriptEditorItemData
{
	String name;
	String sort_key;
	Ref<Texture2D> icon;
	bool tool = false;
	int index = 0;
	String tooltip;
	bool used = false;
	int category = 0;
	Node* ref = nullptr;
	String path;

	bool operator<(const _ScriptEditorItemData& id) const
	{
		if (category == id.category) {
			if (sort_key == id.sort_key) {
				return index < id.index;
			}
			else {
				return sort_key.filenocasecmp_to(id.sort_key) < 0;
			}
		}
		else {
			return category < id.category;
		}
	}
};

Control* ScriptEditor::get_active_editor() const
{
	return tab_container->get_current_tab_control();
}

void ScriptEditor::open_find_in_files_dialog(const String& p_initial_text, bool p_replace)
{
	find_in_files->open_dialog(p_initial_text, p_replace);
}

void ScriptEditor::open_script_create_dialog(const String& p_base_name, const String& p_base_path)
{
	_menu_option(FILE_MENU_NEW_SCRIPT);
	script_create_dialog->config(p_base_name, p_base_path);
}

void ScriptEditor::open_text_file_create_dialog(
	const String& p_base_path, const String& p_base_name)
{
	_menu_option(FILE_MENU_NEW_TEXTFILE);
	file_dialog->set_current_dir(p_base_path);
	file_dialog->set_current_file(p_base_name);
}

Ref<Resource> ScriptEditor::open_file(const String& p_file)
{
	if (_get_recognized_extensions().find(p_file.get_extension())) {
		Ref<Resource> scr = ResourceLoader::load(p_file);
		if (scr.is_null()) {
			EditorNode::get_singleton()->show_warning(
				TTR("Could not load file at:") + "\n\n" + p_file, TTR("Error!"));
			return Ref<Resource>();
		}

		edit(scr);
		return scr;
	}

	Error error;
	Ref<TextFile> text_file = _load_text_file(p_file, &error);
	if (error != OK) {
		EditorNode::get_singleton()->show_warning(
			TTR("Could not load file at:") + "\n\n" + p_file, TTR("Error!"));
		return Ref<Resource>();
	}

	if (text_file.is_valid()) {
		edit(text_file);
		return text_file;
	}
	return Ref<Resource>();
}

void ScriptEditor::_save_layout()
{
	if (restoring_layout) {
		return;
	}

	EditorNode::get_singleton()->save_editor_layout_delayed();
}

void ScriptEditor::_filesystem_changed() { _update_script_names(); }

void ScriptEditor::_autosave_scripts() { save_all_scripts(); }

void ScriptEditor::_split_dragged(float) { _save_layout(); }

void ScriptEditor::_script_list_clicked(
	int p_item, Vector2 p_local_mouse_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index == MouseButton::MIDDLE) {
		script_list->select(p_item);
		_script_selected(p_item);
		_menu_option(FILE_MENU_CLOSE);
	}

	if (p_mouse_button_index == MouseButton::RIGHT) {
		_make_script_list_context_menu();
	}
}

void ScriptEditor::_history_forward()
{
	if (history_pos < history.size() - 1) {
		ScriptEditorNavigationMarker::get_singleton()->traverse_begin();
		_update_history_pos(history_pos + 1);
		ScriptEditorNavigationMarker::get_singleton()->traverse_end();
	}
}

void ScriptEditor::_history_back()
{
	if (history_pos > 0) {
		ScriptEditorNavigationMarker::get_singleton()->traverse_begin();
		_update_history_pos(history_pos - 1);
		ScriptEditorNavigationMarker::get_singleton()->traverse_end();
	}
}

void ScriptEditor::_roll_back_to_pre_tab()
{
	Control* tselected = tab_container->get_current_tab_control();
	if (history_pos == -1 || history[history_pos].control != tselected) {
		return;
	}

	int pos = -1;
	for (int i = history_pos - 1; i >= 0; i--) {
		if (history[i].control != tselected) {
			pos = i;
			break;
		}
	}

	if (pos == -1) {
		history_pos = -1;
		history.clear();
	}
	else {
		_update_history_pos(pos);
		history.resize(history_pos + 1);
	}
}

void ScriptEditor::set_live_auto_reload_running_scripts(bool p_enabled)
{
	auto_reload_running_scripts = p_enabled;
}

void ScriptEditor::_calculate_script_name_button_ratio()
{
	const float total_width = script_name_button_hbox->get_size().width;
	if (total_width <= 0) {
		return;
	}

	// Make the ratios a fraction bigger, to avoid unnecessary trimming.
	const float extra_ratio = 4 / total_width;

	const float script_name_ratio = MIN(1, script_name_width / total_width + extra_ratio);
	script_name_button->set_stretch_ratio(script_name_ratio);

	float ratio_left = 1 - script_name_ratio;
	script_name_button_left_spacer->set_stretch_ratio(ratio_left / 2);
	script_name_button_right_spacer->set_stretch_ratio(ratio_left / 2);
}

void ScriptEditor::_help_search(const String& p_text) { help_search_dialog->popup_dialog(p_text); }

void ScriptEditor::register_syntax_highlighter(
	const Ref<EditorSyntaxHighlighter>& p_syntax_highlighter)
{
	ERR_FAIL_COND(p_syntax_highlighter.is_null());

	if (!syntax_highlighters.has(p_syntax_highlighter)) {
		syntax_highlighters.push_back(p_syntax_highlighter);
	}
}

void ScriptEditor::unregister_syntax_highlighter(
	const Ref<EditorSyntaxHighlighter>& p_syntax_highlighter)
{
	ERR_FAIL_COND(p_syntax_highlighter.is_null());

	syntax_highlighters.erase(p_syntax_highlighter);
}

int ScriptEditor::script_editor_func_count = 0;
CreateScriptEditorFunc ScriptEditor::script_editor_funcs[ScriptEditor::SCRIPT_EDITOR_FUNC_MAX];

void ScriptEditor::register_create_script_editor_function(CreateScriptEditorFunc p_func)
{
	ERR_FAIL_COND(script_editor_func_count == SCRIPT_EDITOR_FUNC_MAX);
	script_editor_funcs[script_editor_func_count++] = p_func;
}

void ScriptEditor::_script_changed() { SignalsDock::get_singleton()->update_lists(); }

void ScriptEditor::_set_script_zoom_factor(float p_zoom_factor)
{
	if (zoom_factor == p_zoom_factor) {
		return;
	}

	zoom_factor = p_zoom_factor;
}

void ScriptEditor::_update_code_editor_zoom_factor(CodeTextEditor* p_code_text_editor)
{
	if (p_code_text_editor && p_code_text_editor->is_visible_in_tree() &&
		zoom_factor != p_code_text_editor->get_zoom_factor()) {
		p_code_text_editor->set_zoom_factor(zoom_factor);
	}
}

void ScriptEditor::_window_changed(bool p_visible)
{
	make_floating->set_visible(!p_visible);
	is_floating = p_visible;
}

void ScriptEditor::_filter_scripts_text_changed(const String& p_newtext) { _update_script_names(); }

void ScriptEditor::_bind_methods() {}

ScriptEditor::~ScriptEditor()
{
	memdelete(find_in_files);
	ScriptEditorNavigationMarker::release_singleton();
}

void ScriptEditorPlugin::_focus_another_editor()
{
	if (window_wrapper->get_window_enabled()) {
		ERR_FAIL_COND(last_editor.is_empty());
		EditorInterface::get_singleton()->set_main_screen_editor(last_editor);
	}
}

void ScriptEditorPlugin::_save_last_editor(const String& p_editor)
{
	if (p_editor != get_plugin_name()) {
		last_editor = p_editor;
	}
}

void ScriptEditorPlugin::_window_visibility_changed(bool p_visible)
{
	_focus_another_editor();
	if (p_visible) {
		script_editor->add_theme_style_override(SceneStringName(panel),
			script_editor
				->get_theme_stylebox("ScriptEditorPanelFloating", EditorStringName(EditorStyles))
				.ptr());
	}
	else {
		script_editor->add_theme_style_override(SceneStringName(panel),
			script_editor->get_theme_stylebox("ScriptEditorPanel", EditorStringName(EditorStyles))
				.ptr());
	}
}

void ScriptEditorPlugin::selected_notify()
{
	script_editor->ensure_select_current();
	_focus_another_editor();
}

String ScriptEditorPlugin::get_unsaved_status(const String& p_for_scene) const
{
	const PackedStringArray unsaved_scripts = script_editor->get_unsaved_scripts();
	if (unsaved_scripts.is_empty()) {
		return String();
	}

	PackedStringArray message;
	if (!p_for_scene.is_empty()) {
		PackedStringArray unsaved_built_in_scripts;

		const String scene_file = p_for_scene.get_file();
		for (const String& E : unsaved_scripts) {
			if (!E.is_resource_file() && E.contains(scene_file)) {
				unsaved_built_in_scripts.append(E);
			}
		}

		if (unsaved_built_in_scripts.is_empty()) {
			return String();
		}
		else {
			message.resize(unsaved_built_in_scripts.size() + 1);
			message.write[0] =
				TTR("There are unsaved changes in the following built-in script(s):");

			int i = 1;
			for (const String& E : unsaved_built_in_scripts) {
				message.write[i] = E.trim_suffix("(*)");
				i++;
			}
			return String("\n").join(message);
		}
	}

	message.resize(unsaved_scripts.size() + 1);
	message.write[0] = TTR("Save changes to the following script(s) before quitting?");

	int i = 1;
	for (const String& E : unsaved_scripts) {
		message.write[i] = E.trim_suffix("(*)");
		i++;
	}
	return String("\n").join(message);
}

void ScriptEditorPlugin::save_external_data()
{
	if (!EditorNode::get_singleton()->is_exiting()) {
		script_editor->save_all_scripts();
	}
}

void ScriptEditorPlugin::apply_changes() { script_editor->apply_scripts(); }

void ScriptEditorPlugin::get_breakpoints(List<String>* p_breakpoints)
{
	script_editor->get_breakpoints(p_breakpoints);
}

void ScriptEditorPlugin::edited_scene_changed() { script_editor->edited_scene_changed(); }


