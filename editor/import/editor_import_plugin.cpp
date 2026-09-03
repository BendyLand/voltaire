/**************************************************************************/
/*  editor_import_plugin.cpp                                              */
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

#include "editor/file_system/editor_file_system.h"
#include "editor_import_plugin.h"

bool EditorImportPlugin::can_import_threaded() const
{
	return ResourceImporter::can_import_threaded();
}

String EditorImportPlugin::get_importer_name() const { return ""; }

String EditorImportPlugin::get_visible_name() const { return ""; }

String EditorImportPlugin::get_save_extension() const { return ""; }

String EditorImportPlugin::get_resource_type() const { return ""; }

int EditorImportPlugin::get_preset_count() const { return 0; }

float EditorImportPlugin::get_priority() const { return 1.0; }

int EditorImportPlugin::get_import_order() const { return 0; }

int EditorImportPlugin::get_format_version() const { return 0; }

String EditorImportPlugin::get_preset_name(int p_idx) const { return String(); }


