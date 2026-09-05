/**************************************************************************/
/*  script_editor_base.cpp                                                */
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

#include "core/io/json.h"
#include "editor/editor_node.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "script_editor_base.h"
#include "servers/display/display_server.h"

void ScriptEditorBase::_bind_methods() {}

String ScriptEditorBase::get_name()
{
	String name;

	name = edited_res->get_path().get_file();
	if (name.is_empty()) {
		// This appears for newly created built-in text_files before saving the scene.
		name = TTR("[unsaved]");
	}
	else if (edited_res->is_built_in()) {
		const String& text_file_name = edited_res->get_name();
		if (!text_file_name.is_empty()) {
			// If the built-in text_file has a custom resource name defined,
			// display the built-in text_file name as follows: `ResourceName (scene_file.tscn)`
			name = vformat("%s (%s)", text_file_name, name.get_slice("::", 0));
		}
	}

	if (is_unsaved()) {
		name += "(*)";
	}

	return name;
}

void ScriptEditorBase::tag_saved_version()
{
	edited_file_data.last_modified_time = FileAccess::get_modified_time(edited_file_data.path);
}

//// TextEditorBase

void TextEditorBase::EditMenus::_edit_option(int p_op)
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);
	script_text_editor->_edit_option(p_op);
}

void TextEditorBase::EditMenus::_prepare_edit_menu()
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);
	const CodeEdit* tx = script_text_editor->code_editor->get_text_editor();
	PopupMenu* popup = edit_menu->get_popup();
	popup->set_item_disabled(popup->get_item_index(EDIT_UNDO), !tx->has_undo());
	popup->set_item_disabled(popup->get_item_index(EDIT_REDO), !tx->has_redo());
}

void TextEditorBase::EditMenus::_update_highlighter_menu()
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);

	Ref<EditorSyntaxHighlighter> current_highlighter =
		script_text_editor->get_code_editor()->get_text_editor()->get_syntax_highlighter();
	highlighter_menu->clear();
	for (const Ref<EditorSyntaxHighlighter>& highlighter : script_text_editor->highlighters) {
		highlighter_menu->add_radio_check_item(highlighter->_get_name());
		highlighter_menu->set_item_checked(-1, highlighter == current_highlighter);
	}
}

void TextEditorBase::EditMenus::_change_syntax_highlighter(int p_idx)
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);
	ERR_FAIL_INDEX(p_idx, (int)script_text_editor->highlighters.size());
	script_text_editor->set_syntax_highlighter(script_text_editor->highlighters[p_idx]);
}

void TextEditorBase::_make_context_menu(
	bool p_selection, bool p_foldable, const Vector2& p_position, bool p_show)
{
	context_menu->clear();
	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		context_menu->add_item(TTRC("Emoji & Symbols"), EDIT_EMOJI_AND_SYMBOL);
		context_menu->add_separator();
	}
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_undo"), EDIT_UNDO);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_redo"), EDIT_REDO);
	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_cut"), EDIT_CUT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_copy"), EDIT_COPY);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_paste"), EDIT_PASTE);
	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_text_select_all"), EDIT_SELECT_ALL);
	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent"), EDIT_INDENT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/unindent"), EDIT_UNINDENT);
	context_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);

	if (p_selection) {
		context_menu->add_separator();
		context_menu->add_shortcut(
			ED_GET_SHORTCUT("script_text_editor/convert_to_uppercase"), EDIT_TO_UPPERCASE);
		context_menu->add_shortcut(
			ED_GET_SHORTCUT("script_text_editor/convert_to_lowercase"), EDIT_TO_LOWERCASE);
	}

	if (p_foldable) {
		context_menu->add_shortcut(
			ED_GET_SHORTCUT("script_text_editor/toggle_fold_line"), EDIT_TOGGLE_FOLD_LINE);
	}

	if (p_show) {
		_show_context_menu(p_position);
	}
}

void TextEditorBase::_show_context_menu(const Vector2& p_position)
{
	const CodeEdit* tx = code_editor->get_text_editor();
	context_menu->set_item_disabled(context_menu->get_item_index(EDIT_UNDO), !tx->has_undo());
	context_menu->set_item_disabled(context_menu->get_item_index(EDIT_REDO), !tx->has_redo());

	context_menu->set_position(get_screen_position() + p_position);
	context_menu->reset_size();
	context_menu->popup();
}

void TextEditorBase::_load_theme_settings()
{
	code_editor->get_text_editor()->get_syntax_highlighter()->update_cache();
}

void TextEditorBase::add_syntax_highlighter(Ref<EditorSyntaxHighlighter> p_highlighter)
{
	ERR_FAIL_COND(p_highlighter.is_null());

	highlighters.push_back(p_highlighter);
}

void TextEditorBase::set_syntax_highlighter(Ref<EditorSyntaxHighlighter> p_highlighter)
{
	ERR_FAIL_COND(p_highlighter.is_null());

	CodeEdit* te = code_editor->get_text_editor();
	p_highlighter->_set_edited_resource(edited_res);
	te->set_syntax_highlighter(p_highlighter);
}

bool TextEditorBase::is_unsaved()
{
	return code_editor->get_text_editor()->get_version() !=
			   code_editor->get_text_editor()->get_saved_version() ||
		   edited_res->get_path().is_empty(); // In memory.
}

void TextEditorBase::tag_saved_version()
{
	code_editor->get_text_editor()->tag_saved_version();
	ScriptEditorBase::tag_saved_version();
}

void TextEditorBase::enable_editor()
{
	if (editor_enabled) {
		return;
	}

	editor_enabled = true;

	_load_theme_settings();

	_validate_script();
}

TextEditorBase::~TextEditorBase() { highlighters.clear(); }

//// CodeEditorBase

CodeEditorBase::EditMenusCEB::EditMenusCEB()
{
	edit_menu->get_popup()->add_shortcut(
		ED_GET_SHORTCUT("ui_text_completion_query"), EDIT_COMPLETE);
	_popup_move_item(EDIT_TRIM_TRAILING_WHITESAPCE, edit_menu->get_popup(), false);
	edit_menu_line->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
}


