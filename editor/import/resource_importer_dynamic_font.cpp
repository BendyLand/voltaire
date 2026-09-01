/**************************************************************************/
/*  resource_importer_dynamic_font.cpp                                    */
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
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/import/dynamic_font_import_settings.h"
#include "resource_importer_dynamic_font.h"
#include "scene/resources/font.h"
#include "servers/text/text_server.h"

String ResourceImporterDynamicFont::get_importer_name() const { return "font_data_dynamic"; }

String ResourceImporterDynamicFont::get_visible_name() const { return "Font Data (Dynamic Font)"; }

void ResourceImporterDynamicFont::get_recognized_extensions(List<String>* p_extensions) const
{
	if (p_extensions) {
		p_extensions->push_back("ttf");
		p_extensions->push_back("ttc");
		p_extensions->push_back("otf");
		p_extensions->push_back("otc");
		p_extensions->push_back("woff");
		p_extensions->push_back("woff2");
		p_extensions->push_back("pfb");
		p_extensions->push_back("pfm");
	}
}

String ResourceImporterDynamicFont::get_save_extension() const { return "fontdata"; }

String ResourceImporterDynamicFont::get_resource_type() const { return "FontFile"; }

void ResourceImporterDynamicFont::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	Ref<FontFile> font = ResourceLoader::load(p_path);
	if (font.is_valid() && font->is_multichannel_signed_distance_field()) {
		r_dependencies->insert("module_msdfgen_enabled");
	}
}



int ResourceImporterDynamicFont::get_preset_count() const { return PRESET_MAX; }

String ResourceImporterDynamicFont::get_preset_name(int p_idx) const
{
	switch (p_idx) {
	case PRESET_DYNAMIC:
		return TTR("Dynamically rendered TrueType/OpenType font");
	case PRESET_MSDF:
		return TTR("Prerendered multichannel(+true) signed distance field");
	default:
		return String();
	}
}



bool ResourceImporterDynamicFont::has_advanced_options() const { return true; }

void ResourceImporterDynamicFont::show_advanced_options(const String& p_path)
{
	DynamicFontImportSettingsDialog::get_singleton()->open_settings(p_path);
}



