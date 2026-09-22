/**************************************************************************/
/*  editor_expression_evaluator.cpp                                       */
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

#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_string_names.h"
#include "editor_expression_evaluator.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/line_edit.h"

void EditorExpressionEvaluator::set_editor_debugger(ScriptEditorDebugger* p_editor_debugger)
{
	editor_debugger = p_editor_debugger;
}

void EditorExpressionEvaluator::_line_edit_gui_input(const Ref<InputEvent>& p_event)
{
	if (!expression_input->is_editing()) {
		return;
	}

	const Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed()) {
		return;
	}

	if (k->is_action_pressed("ui_up", true)) {
		int size = expression_history.size();
		if (expression_index < size - 1) {
			expression_index++;

			const String& history_text =
				expression_history[expression_history.size() - expression_index - 1];
			expression_input->set_text(history_text);
			expression_input->set_caret_column(history_text.size());
		}
		accept_event();
	}
	else if (k->is_action_pressed("ui_down", true)) {
		if (expression_index > 0) {
			expression_index--;

			const String& history_text =
				expression_history[expression_history.size() - expression_index - 1];
			expression_input->set_text(history_text);
			expression_input->set_caret_column(history_text.size());
		}
		else if (expression_index == 0) {
			expression_index = -1;
			expression_input->clear();
		}
		accept_event();
	}
}

void EditorExpressionEvaluator::_evaluate()
{
	const String& expression = expression_input->get_text();
	if (expression.is_empty()) {
		return;
	}

	expression_history.erase(expression);
	expression_history.push_back(expression);
	expression_index = -1;

	editor_debugger->request_remote_evaluate(expression, editor_debugger->get_stack_script_frame());

	expression_input->clear();
}

void EditorExpressionEvaluator::_clear() { inspector->clear_stack_variables(); }

void EditorExpressionEvaluator::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		expression_input->add_theme_font_override(SceneStringName(font),
			get_theme_font(SNAME("expression"), EditorStringName(EditorFonts)).ptr());
		expression_input->add_theme_font_size_override(SceneStringName(font_size),
			get_theme_font_size(SNAME("expression_size"), EditorStringName(EditorFonts)));
	} break;
	}
}


