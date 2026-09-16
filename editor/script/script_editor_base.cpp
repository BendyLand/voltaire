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

void TextEditorBase::EditMenus::_edit_option(int p_op)
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);
	script_text_editor->_edit_option(p_op);
}

void TextEditorBase::EditMenus::_change_syntax_highlighter(int p_idx)
{
	TextEditorBase* script_text_editor = _get_active_editor();
	ERR_FAIL_NULL(script_text_editor);
	ERR_FAIL_INDEX(p_idx, (int)script_text_editor->highlighters.size());
	script_text_editor->set_syntax_highlighter(script_text_editor->highlighters[p_idx]);
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


