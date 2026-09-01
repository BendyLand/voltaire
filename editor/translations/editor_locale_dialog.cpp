/**************************************************************************/
/*  editor_locale_dialog.cpp                                              */
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
#include "core/string/translation_server.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "editor_locale_dialog.h"
#include "scene/gui/check_button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/tree.h"

void EditorLocaleDialog::_notification(int p_what)
{
	if (p_what == NOTIFICATION_TRANSLATION_CHANGED) {
		// TRANSLATORS: This is the label for a list of writing systems.
		script_label1->set_text(TTR("Script:", "Locale"));
		// TRANSLATORS: This refers to a writing system.
		script_label2->set_text(TTR("Script", "Locale"));

		script_list->set_accessibility_name(TTR("Script", "Locale"));
		script_code->set_accessibility_name(TTR("Script", "Locale"));
	}
}

void EditorLocaleDialog::_bind_methods() {}

void EditorLocaleDialog::ok_pressed()
{
	if (edit_filters->is_pressed()) {
		return; // Do not update, if in filter edit mode.
	}

	String locale;
	if (lang_code->get_text().is_empty()) {
		return; // Language code is required.
	}
	locale = lang_code->get_text();

	if (!script_code->get_text().is_empty()) {
		locale += "_" + script_code->get_text();
	}
	if (!country_code->get_text().is_empty()) {
		locale += "_" + country_code->get_text();
	}
	if (!variant_code->get_text().is_empty()) {
		locale += "_" + variant_code->get_text();
	}
	hide();
}

void EditorLocaleDialog::_toggle_advanced(bool p_checked)
{
	if (!p_checked) {
		script_code->set_text("");
		variant_code->set_text("");
	}
	_update_tree();
}

void EditorLocaleDialog::_post_popup()
{
	ConfirmationDialog::_post_popup();

	if (!locale_set) {
		lang_code->set_text("");
		script_code->set_text("");
		country_code->set_text("");
		variant_code->set_text("");
	}
	edit_filters->set_pressed(false);
	_update_tree();
}

void EditorLocaleDialog::_edit_filters(bool p_checked) { _update_tree(); }

void EditorLocaleDialog::set_locale(const String& p_locale)
{
	const String& locale = TranslationServer::get_singleton()->standardize_locale(p_locale);
	if (locale.is_empty()) {
		locale_set = false;

		lang_code->set_text("");
		script_code->set_text("");
		country_code->set_text("");
		variant_code->set_text("");
	}
	else {
		locale_set = true;

		Vector<String> locale_elements = p_locale.split("_");
		lang_code->set_text(locale_elements[0]);
		if (locale_elements.size() >= 2) {
			if (locale_elements[1].length() == 4 && is_ascii_upper_case(locale_elements[1][0]) &&
				is_ascii_lower_case(locale_elements[1][1]) &&
				is_ascii_lower_case(locale_elements[1][2]) &&
				is_ascii_lower_case(locale_elements[1][3])) {
				script_code->set_text(locale_elements[1]);
				advanced->set_pressed(true);
			}
			if (locale_elements[1].length() == 2 && is_ascii_upper_case(locale_elements[1][0]) &&
				is_ascii_upper_case(locale_elements[1][1])) {
				country_code->set_text(locale_elements[1]);
			}
		}
		if (locale_elements.size() >= 3) {
			if (locale_elements[2].length() == 2 && is_ascii_upper_case(locale_elements[2][0]) &&
				is_ascii_upper_case(locale_elements[2][1])) {
				country_code->set_text(locale_elements[2]);
			}
			else {
				variant_code->set_text(locale_elements[2].to_lower());
				advanced->set_pressed(true);
			}
		}
		if (locale_elements.size() >= 4) {
			variant_code->set_text(locale_elements[3].to_lower());
			advanced->set_pressed(true);
		}
	}
}

void EditorLocaleDialog::popup_locale_dialog()
{
	popup_centered_clamped(Size2(1050, 700) * EDSCALE, 0.8);
}


