/**************************************************************************/
/*  script_text_editor.cpp                                                */
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
#include "core/io/dir_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/math/expression.h"
#include "core/os/keyboard.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/style_box_flat.h"
#include "script_text_editor.h"
#include "servers/rendering/rendering_server.h"

ConnectionInfoDialog::ConnectionInfoDialog()
{
	set_title(TTRC("Connections to method:"));

	VBoxContainer* vbc = memnew(VBoxContainer);
	vbc->set_anchor_and_offset(SIDE_LEFT, Control::ANCHOR_BEGIN, 8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_TOP, Control::ANCHOR_BEGIN, 8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_RIGHT, Control::ANCHOR_END, -8 * EDSCALE);
	vbc->set_anchor_and_offset(SIDE_BOTTOM, Control::ANCHOR_END, -8 * EDSCALE);
	add_child(vbc);

	method = memnew(Label);
	method->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	method->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	method->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	vbc->add_child(method);

	tree = memnew(Tree);
	tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tree->set_theme_type_variation("TreeTable");
	tree->set_hide_folding(true);
	tree->set_columns(3);
	tree->set_hide_root(true);
	tree->set_column_titles_visible(true);
	tree->set_column_title(0, TTRC("Source"));
	tree->set_column_title(1, TTRC("Signal"));
	tree->set_column_title(2, TTRC("Target"));
	vbc->add_child(tree);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_allow_rmb_select(true);
}

////////////////////////////////////////////////////////////////////////////////

void ScriptTextEditor::EditMenusSTE::_update_breakpoint_list()
{
	breakpoints_menu->clear();
	breakpoints_menu->reset_size();

	breakpoints_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_breakpoint"), DEBUG_TOGGLE_BREAKPOINT);
	breakpoints_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/remove_all_breakpoints"), DEBUG_REMOVE_ALL_BREAKPOINTS);
	breakpoints_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/goto_next_breakpoint"), DEBUG_GOTO_NEXT_BREAKPOINT);
	breakpoints_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/goto_previous_breakpoint"), DEBUG_GOTO_PREV_BREAKPOINT);

	TextEditorBase* script_text_editor = _get_active_editor();
	if (script_text_editor == nullptr) {
		return;
	}

	PackedInt32Array breakpoint_list =
		script_text_editor->get_code_editor()->get_text_editor()->get_breakpointed_lines();
	if (breakpoint_list.is_empty()) {
		return;
	}

	breakpoints_menu->add_separator();

	for (int i = 0; i < breakpoint_list.size(); i++) {
		// Strip edges to remove spaces or tabs.
		// Also replace any tabs by spaces, since we can't print tabs in the menu.
		String line = script_text_editor->get_code_editor()
						  ->get_text_editor()
						  ->get_line(breakpoint_list[i])
						  .replace("\t", "  ")
						  .strip_edges();

		// Limit the size of the line if too big.
		if (line.length() > 50) {
			line = line.substr(0, 50);
		}

		breakpoints_menu->add_item(String::num_int64(breakpoint_list[i] + 1) + " - `" + line + "`");
	}
}

////////////////////////////////////////////////////////////////////////////////

void ScriptTextEditor::_show_errors_panel(bool p_show) { errors_panel->set_visible(p_show); }

void ScriptTextEditor::_show_warnings_panel(bool p_show) { warnings_panel->set_visible(p_show); }

void ScriptTextEditor::_on_mouse_exited() { drag_info_label->hide(); }

String ScriptTextEditor::_picker_color_stringify(const Color& p_color, COLOR_MODE p_mode)
{
	String result;
	String fname;
	Vector<String> str_params;
	switch (p_mode) {
	case ScriptTextEditor::MODE_STRING: {
		str_params.push_back("\"" + p_color.to_html() + "\"");
	} break;
	case ScriptTextEditor::MODE_HEX: {
		str_params.push_back("0x" + p_color.to_html());
	} break;
	case ScriptTextEditor::MODE_RGB: {
		str_params = {String::num(p_color.r, 3), String::num(p_color.g, 3),
			String::num(p_color.b, 3), String::num(p_color.a, 3)};
	} break;
	case ScriptTextEditor::MODE_HSV: {
		str_params = {String::num(p_color.get_h(), 3), String::num(p_color.get_s(), 3),
			String::num(p_color.get_v(), 3), String::num(p_color.a, 3)};
		fname = ".from_hsv";
	} break;
	case ScriptTextEditor::MODE_OKHSL: {
		str_params = {String::num(p_color.get_ok_hsl_h(), 3),
			String::num(p_color.get_ok_hsl_s(), 3), String::num(p_color.get_ok_hsl_l(), 3),
			String::num(p_color.a, 3)};
		fname = ".from_ok_hsl";
	} break;
	case ScriptTextEditor::MODE_RGB8: {
		str_params = {itos(p_color.get_r8()), itos(p_color.get_g8()), itos(p_color.get_b8()),
			itos(p_color.get_a8())};
		fname = ".from_rgba8";
	} break;
	default: {
	} break;
	}
	result = "Color" + fname + "(" + String(", ").join(str_params) + ")";
	return result;
}

void ScriptTextEditor::_picker_color_changed(const Color& p_color)
{
	_update_color_constructor_options();
	_update_color_text();
}

void ScriptTextEditor::_update_color_constructor_options()
{
	int item_count = inline_color_options->get_item_count();
	// Update or add each constructor as an option.
	for (int i = 0; i < MODE_MAX; i++) {
		String option_text =
			_picker_color_stringify(inline_color_picker->get_pick_color(), (COLOR_MODE)i);
		if (i >= item_count) {
			inline_color_options->add_item(option_text);
		}
		else {
			inline_color_options->set_item_text(i, option_text);
		}
	}
}

void ScriptTextEditor::_update_color_text()
{
	if (inline_color_line < 0) {
		return;
	}
	String result = inline_color_options->get_item_text(inline_color_options->get_selected_id());
	code_editor->get_text_editor()->begin_complex_operation();
	code_editor->get_text_editor()->remove_text(
		inline_color_line, inline_color_start, inline_color_line, inline_color_end + 1);
	inline_color_end = inline_color_start + result.size() - 2;
	code_editor->get_text_editor()->insert_text(result, inline_color_line, inline_color_start);
	code_editor->get_text_editor()->end_complex_operation();
}

void ScriptTextEditor::store_previous_state() { return code_editor->store_previous_state(); }

String ScriptTextEditor::_get_absolute_path(const String& rel_path)
{
	String base_path = edited_res->get_path().get_base_dir();
	String path = base_path.path_join(rel_path);
	return path.replace("///", "//").simplify_path();
}

void ScriptTextEditor::_goto_line(int p_line)
{
	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	goto_line(p_line);
	ScriptEditorNavigationMarker::get_singleton()->locate_end();
}

void ScriptTextEditor::_update_gutter_indexes()
{
	for (int i = 0; i < code_editor->get_text_editor()->get_gutter_count(); i++) {
		if (code_editor->get_text_editor()->get_gutter_name(i) == "connection_gutter") {
			connection_gutter = i;
			continue;
		}

		if (code_editor->get_text_editor()->get_gutter_name(i) == "line_numbers") {
			line_number_gutter = i;
			continue;
		}
	}
}

void ScriptTextEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (is_ready() && is_visible_in_tree()) {
			_update_errors();
			_update_warnings();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED:
		if (!editor_enabled) {
			break;
		}
		if (is_visible_in_tree()) {
			_update_warnings();
			_update_errors();
			_update_background_color();
		}
		[[fallthrough]];
	case NOTIFICATION_ENTER_TREE: {
		code_editor->get_text_editor()->set_gutter_width(
			connection_gutter, code_editor->get_text_editor()->get_line_height());
		Ref<Font> code_font = get_theme_font("font", "CodeEdit");
		inline_color_options->add_theme_font_override("font", code_font.ptr());
		inline_color_options->get_popup()->add_theme_font_override("font", code_font);
	} break;
	case NOTIFICATION_DRAG_END: {
		drag_info_label->hide();
	} break;
	}
}

Control* ScriptTextEditor::get_edit_menu()
{
	if (!edit_menus) {
		edit_menus = memnew(EditMenusSTE);
	}
	return edit_menus;
}

PackedInt32Array ScriptTextEditor::get_breakpoints()
{
	return code_editor->get_text_editor()->get_breakpointed_lines();
}

void ScriptTextEditor::set_breakpoint(int p_line, bool p_enabled)
{
	code_editor->get_text_editor()->set_line_as_breakpoint(p_line, p_enabled);
}

void ScriptTextEditor::clear_breakpoints()
{
	code_editor->get_text_editor()->clear_breakpointed_lines();
}

void ScriptTextEditor::_color_changed(const Color& p_color)
{
	String new_args;
	const int decimals = 3;
	if (p_color.a == 1.0f) {
		new_args = String("(" + String::num(p_color.r, decimals) + ", " +
						  String::num(p_color.g, decimals) + ", " +
						  String::num(p_color.b, decimals) + ")");
	}
	else {
		new_args =
			String("(" + String::num(p_color.r, decimals) + ", " +
				   String::num(p_color.g, decimals) + ", " + String::num(p_color.b, decimals) +
				   ", " + String::num(p_color.a, decimals) + ")");
	}

	String line = code_editor->get_text_editor()->get_line(color_position.x);
	String line_with_replaced_args =
		line.substr(0, color_position.y) +
		line.substr(color_position.y, color_position.z - color_position.y)
			.replace(color_args, new_args) +
		line.substr(color_position.z);

	color_args = new_args;
	code_editor->get_text_editor()->begin_complex_operation();
	code_editor->get_text_editor()->set_line(color_position.x, line_with_replaced_args);
	code_editor->get_text_editor()->end_complex_operation();
}

void ScriptTextEditor::_make_context_menu(bool p_selection, bool p_color, bool p_foldable,
	bool p_open_docs, bool p_goto_definition, const Vector2& p_position)
{
	TextEditorBase::_make_context_menu(p_selection, p_foldable, p_position, false);
	context_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
	_popup_move_item(EDIT_UNINDENT, context_menu);

	if (p_selection) {
		context_menu->add_shortcut(
			ED_GET_SHORTCUT("script_text_editor/evaluate_selection"), EDIT_EVALUATE);
		_popup_move_item(EDIT_TO_LOWERCASE, context_menu);
		context_menu->add_shortcut(
			ED_GET_SHORTCUT("script_text_editor/create_code_region"), EDIT_CREATE_CODE_REGION);
		_popup_move_item(EDIT_EVALUATE, context_menu);
	}

	if (p_color || p_open_docs || p_goto_definition) {
		context_menu->add_separator();
		if (p_open_docs) {
			context_menu->add_shortcut(
				ED_GET_SHORTCUT("script_text_editor/goto_symbol"), LOOKUP_SYMBOL);
		}
		if (p_color) {
			context_menu->add_item(TTRC("Pick Color"), EDIT_PICK_COLOR);
		}
	}

	const PackedStringArray paths = {String(code_editor->get_text_editor()->get_path())};
	EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(
		context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR_CODE, paths);

	_show_context_menu(p_position);
}

void ScriptTextEditor::register_editor()
{
	ED_SHORTCUT("script_text_editor/move_up", TTRC("Move Up"), KeyModifierMask::ALT | Key::UP);
	ED_SHORTCUT(
		"script_text_editor/move_down", TTRC("Move Down"), KeyModifierMask::ALT | Key::DOWN);
	ED_SHORTCUT("script_text_editor/delete_line", TTRC("Delete Line"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::K);
	ED_SHORTCUT("script_text_editor/join_lines", TTRC("Join Lines"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::J);

	// Leave these at zero, same can be accomplished with tab/shift-tab, including selection.
	// The next/previous in history shortcut in this case makes a lot more sense.

	ED_SHORTCUT("script_text_editor/indent", TTRC("Indent"), Key::NONE);
	ED_SHORTCUT("script_text_editor/unindent", TTRC("Unindent"), KeyModifierMask::SHIFT | Key::TAB);
	ED_SHORTCUT_ARRAY("script_text_editor/toggle_comment", TTRC("Toggle Comment"),
		{int32_t(KeyModifierMask::CMD_OR_CTRL | Key::K),
			int32_t(KeyModifierMask::CMD_OR_CTRL | Key::SLASH),
			int32_t(KeyModifierMask::CMD_OR_CTRL | Key::KP_DIVIDE),
			int32_t(KeyModifierMask::CMD_OR_CTRL | Key::NUMBERSIGN)});
	ED_SHORTCUT("script_text_editor/toggle_fold_line", TTRC("Fold/Unfold Line"),
		KeyModifierMask::ALT | Key::F);
	ED_SHORTCUT_OVERRIDE("script_text_editor/toggle_fold_line", "macos",
		KeyModifierMask::CTRL | KeyModifierMask::META | Key::F);
	ED_SHORTCUT("script_text_editor/fold_all_lines", TTRC("Fold All Lines"), Key::NONE);
	ED_SHORTCUT("script_text_editor/create_code_region", TTRC("Create Code Region"),
		KeyModifierMask::ALT | Key::R);
	ED_SHORTCUT("script_text_editor/unfold_all_lines", TTRC("Unfold All Lines"), Key::NONE);
	ED_SHORTCUT("script_text_editor/duplicate_selection", TTRC("Duplicate Selection"),
		KeyModifierMask::SHIFT | KeyModifierMask::CTRL | Key::D);
	ED_SHORTCUT_OVERRIDE("script_text_editor/duplicate_selection", "macos",
		KeyModifierMask::SHIFT | KeyModifierMask::META | Key::C);
	ED_SHORTCUT("script_text_editor/duplicate_lines", TTRC("Duplicate Lines"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::DOWN);
	ED_SHORTCUT_OVERRIDE("script_text_editor/duplicate_lines", "macos",
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::DOWN);
	ED_SHORTCUT("script_text_editor/evaluate_selection", TTRC("Evaluate Selection"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::E);
	ED_SHORTCUT("script_text_editor/toggle_word_wrap", TTRC("Toggle Word Wrap"),
		KeyModifierMask::ALT | Key::Z);
	ED_SHORTCUT("script_text_editor/trim_trailing_whitespace", TTRC("Trim Trailing Whitespace"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::T);
	ED_SHORTCUT("script_text_editor/trim_final_newlines", TTRC("Trim Final Newlines"), Key::NONE);
	ED_SHORTCUT("script_text_editor/convert_indent_to_spaces", TTRC("Convert Indent to Spaces"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::Y);
	ED_SHORTCUT("script_text_editor/convert_indent_to_tabs", TTRC("Convert Indent to Tabs"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::I);
	ED_SHORTCUT("script_text_editor/auto_indent", TTRC("Auto Indent"),
		KeyModifierMask::CMD_OR_CTRL | Key::I);

	ED_SHORTCUT_AND_COMMAND(
		"script_text_editor/find", TTRC("Find..."), KeyModifierMask::CMD_OR_CTRL | Key::F);

	ED_SHORTCUT("script_text_editor/find_next", TTRC("Find Next"), Key::F3);
	ED_SHORTCUT_OVERRIDE("script_text_editor/find_next", "macos", KeyModifierMask::META | Key::G);

	ED_SHORTCUT("script_text_editor/find_previous", TTRC("Find Previous"),
		KeyModifierMask::SHIFT | Key::F3);
	ED_SHORTCUT_OVERRIDE("script_text_editor/find_previous", "macos",
		KeyModifierMask::META | KeyModifierMask::SHIFT | Key::G);

	ED_SHORTCUT_AND_COMMAND(
		"script_text_editor/replace", TTRC("Replace..."), KeyModifierMask::CTRL | Key::R);
	ED_SHORTCUT_OVERRIDE("script_text_editor/replace", "macos",
		KeyModifierMask::ALT | KeyModifierMask::META | Key::F);

	ED_SHORTCUT("script_text_editor/replace_in_files", TTRC("Replace in Files..."),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::R);

	ED_SHORTCUT("script_text_editor/show_tooltip", TTRC("Show Tooltip"),
		KeyModifierMask::ALT | Key::SLASH, true);
	ED_SHORTCUT("script_text_editor/contextual_help", TTRC("Contextual Help"),
		KeyModifierMask::ALT | Key::F1);
	ED_SHORTCUT_OVERRIDE("script_text_editor/contextual_help", "macos",
		KeyModifierMask::ALT | KeyModifierMask::SHIFT | Key::SPACE);

	ED_SHORTCUT("script_text_editor/toggle_bookmark", TTRC("Toggle Bookmark"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::B);

	ED_SHORTCUT("script_text_editor/goto_next_bookmark", TTRC("Go to Next Bookmark"),
		KeyModifierMask::CMD_OR_CTRL | Key::B);
	ED_SHORTCUT_OVERRIDE("script_text_editor/goto_next_bookmark", "macos",
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | KeyModifierMask::ALT | Key::B);

	ED_SHORTCUT("script_text_editor/goto_previous_bookmark", TTRC("Go to Previous Bookmark"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::B);
	ED_SHORTCUT("script_text_editor/remove_all_bookmarks", TTRC("Remove All Bookmarks"), Key::NONE);

	ED_SHORTCUT("script_text_editor/goto_function", TTRC("Go to Function..."),
		KeyModifierMask::ALT | KeyModifierMask::CTRL | Key::F);
	ED_SHORTCUT_OVERRIDE("script_text_editor/goto_function", "macos",
		KeyModifierMask::CTRL | KeyModifierMask::META | Key::J);

	ED_SHORTCUT("script_text_editor/goto_line", TTRC("Go to Line..."),
		KeyModifierMask::CMD_OR_CTRL | Key::G);
	ED_SHORTCUT_OVERRIDE(
		"script_text_editor/goto_line", "macos", KeyModifierMask::CMD_OR_CTRL | Key::L);
	ED_SHORTCUT("script_text_editor/goto_symbol", TTRC("Lookup Symbol"));

	ED_SHORTCUT("script_text_editor/toggle_breakpoint", TTRC("Toggle Breakpoint"), Key::F9);
	ED_SHORTCUT_OVERRIDE("script_text_editor/toggle_breakpoint", "macos",
		KeyModifierMask::META | KeyModifierMask::SHIFT | Key::B);

	ED_SHORTCUT("script_text_editor/remove_all_breakpoints", TTRC("Remove All Breakpoints"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F9);
	// Using Control for these shortcuts even on macOS because Command+Comma is taken for opening
	// Editor Settings.
	ED_SHORTCUT("script_text_editor/goto_next_breakpoint", TTRC("Go to Next Breakpoint"),
		KeyModifierMask::CTRL | Key::PERIOD);
	ED_SHORTCUT("script_text_editor/goto_previous_breakpoint", TTRC("Go to Previous Breakpoint"),
		KeyModifierMask::CTRL | Key::COMMA);

	ScriptEditor::register_create_script_editor_function(create_editor);
}

ScriptTextEditor::~ScriptTextEditor()
{
	if (!editor_enabled) {
		memdelete(errors_panel);
		memdelete(color_panel);
		memdelete(connection_info_dialog);
	}
}


