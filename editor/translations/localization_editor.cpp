/**************************************************************************/
/*  localization_editor.cpp                                               */
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
#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/translations/editor_translation_parser.h"
#include "editor/translations/template_generator.h"
#include "localization_editor.h"
#include "scene/gui/control.h"
#include "scene/gui/tab_container.h"

void LocalizationEditor::add_translation(const String& p_translation)
{
	PackedStringArray translations;
	translations.push_back(p_translation);
	_translation_add(translations);
}

void LocalizationEditor::_translation_file_open() { translation_file_open->popup_file_dialog(); }

void LocalizationEditor::_translation_res_file_open()
{
	translation_res_file_open_dialog->popup_file_dialog();
}

void LocalizationEditor::_translation_res_option_file_open()
{
	translation_res_option_file_open_dialog->popup_file_dialog();
}

void LocalizationEditor::_translation_res_option_popup(bool p_arrow_clicked)
{
	TreeItem* ed = translation_remap_options->get_edited();
	ERR_FAIL_NULL(ed);

	locale_select->set_locale(ed->get_tooltip_text(1));
	locale_select->popup_locale_dialog();
}

void LocalizationEditor::_translation_res_option_selected(const String& p_locale)
{
	TreeItem* ed = translation_remap_options->get_edited();
	ERR_FAIL_NULL(ed);

	ed->set_text(1, TranslationServer::get_singleton()->get_locale_name(p_locale));
	ed->set_tooltip_text(1, p_locale);

	LocalizationEditor::_translation_res_option_changed();
}

void LocalizationEditor::_template_source_file_open()
{
	template_source_open_dialog->popup_file_dialog();
}

void LocalizationEditor::_template_generate_open()
{
	template_generate_dialog->popup_file_dialog();
}

void LocalizationEditor::_template_generate_command()
{
	const String current_path = template_generate_dialog->get_current_path();
	if (!current_path.is_empty() && current_path.get_file().is_valid_filename()) {
		_template_generate(current_path);
		EditorToaster::get_singleton()->popup_str(TTR("Template generated."));
	}
	else {
		ProjectSettingsEditor::get_singleton()->popup_centered();
		_template_generate_open();
	}
}

void LocalizationEditor::_update_template_source_file_extensions()
{
	template_source_open_dialog->clear_filters();
	List<String> translation_parse_file_extensions;
	EditorTranslationParser::get_singleton()->get_recognized_extensions(
		&translation_parse_file_extensions);
	for (const String& E : translation_parse_file_extensions) {
		template_source_open_dialog->add_filter("*." + E);
	}
}


