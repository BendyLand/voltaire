/**************************************************************************/
/*  openxr_action_set_editor.cpp                                          */
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

#include "editor/editor_string_names.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/themes/editor_scale.h"
#include "openxr_action_editor.h"
#include "openxr_action_set_editor.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"

void OpenXRActionSetEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_theme_changed();
		panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("TabContainer")).ptr());
	} break;
	}
}

void OpenXRActionSetEditor::_do_set_localized_name(const String& p_new_text)
{
	action_set->set_localized_name(p_new_text);
	action_set_localized_name->set_text(p_new_text);
}

void OpenXRActionSetEditor::_do_set_priority(int64_t p_value)
{
	action_set->set_priority(p_value);
	action_set_priority->set_value_no_signal(p_value);
}

void OpenXRActionSetEditor::_do_add_action_editor(OpenXRActionEditor* p_action_editor)
{
	Ref<OpenXRAction> action = p_action_editor->get_action();
	ERR_FAIL_COND(action.is_null());

	action_set->add_action(action);
	actions_vb->add_child(p_action_editor);
}

void OpenXRActionSetEditor::_do_remove_action_editor(OpenXRActionEditor* p_action_editor)
{
	Ref<OpenXRAction> action = p_action_editor->get_action();
	ERR_FAIL_COND(action.is_null());

	actions_vb->remove_child(p_action_editor);
	action_set->remove_action(action);
}

void OpenXRActionSetEditor::set_focus_on_entry()
{
	ERR_FAIL_NULL(action_set_name);
	action_set_name->grab_focus();
}


