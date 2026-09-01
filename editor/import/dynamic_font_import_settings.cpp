/**************************************************************************/
/*  dynamic_font_import_settings.cpp                                      */
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
#include "core/os/os.h"
#include "core/string/translation.h"
#include "dynamic_font_import_settings.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/import/unicode_ranges.inc"
#include "editor/inspector/editor_inspector.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/translations/editor_locale_dialog.h"
#include "scene/gui/split_container.h"

/*************************************************************************/
/* Settings data                                                         */
/*************************************************************************/

Ref<FontFile> DynamicFontImportSettingsData::get_font() const { return fd; }

/*************************************************************************/
/* Glyph ranges                                                          */
/*************************************************************************/

/*************************************************************************/
/* Page 1 callbacks: Rendering Options                                   */
/*************************************************************************/

/*************************************************************************/
/* Page 2 callbacks: Configurations                                      */
/*************************************************************************/

void DynamicFontImportSettingsDialog::_variation_changed(const String& p_edited_property)
{
	_variations_validate();
}

/*************************************************************************/
/* Page 2.1 callbacks: Text to select glyphs                             */
/*************************************************************************/

/*************************************************************************/
/* Page 2.2 callbacks: Character map                                     */
/*************************************************************************/

/*************************************************************************/
/* Common                                                                */
/*************************************************************************/

DynamicFontImportSettingsDialog* DynamicFontImportSettingsDialog::singleton = nullptr;

String DynamicFontImportSettingsDialog::_pad_zeros(const String& p_hex) const
{
	int len = CLAMP(5 - p_hex.length(), 0, 5);
	return String("0").repeat(len) + p_hex;
}

void DynamicFontImportSettingsDialog::_locale_edited()
{
	TreeItem* item = locale_tree->get_selected();
	ERR_FAIL_NULL(item);
	item->set_checked(0, !item->is_checked(0));
}

DynamicFontImportSettingsDialog* DynamicFontImportSettingsDialog::get_singleton()
{
	return singleton;
}


