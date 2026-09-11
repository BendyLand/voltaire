/**************************************************************************/
/*  resource_importer_csv_translation.cpp                                 */
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

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/string/optimized_translation.h"
#include "core/string/translation_server.h"
#include "resource_importer_csv_translation.h"

String ResourceImporterCSVTranslation::get_importer_name() const { return "csv_translation"; }

String ResourceImporterCSVTranslation::get_visible_name() const { return "CSV Translation"; }

void ResourceImporterCSVTranslation::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("csv");
}

String ResourceImporterCSVTranslation::get_save_extension() const
{
	return ""; // does not save a single resource
}

String ResourceImporterCSVTranslation::get_resource_type() const { return "Translation"; }

int ResourceImporterCSVTranslation::get_preset_count() const { return 0; }

String ResourceImporterCSVTranslation::get_preset_name(int p_idx) const { return ""; }


