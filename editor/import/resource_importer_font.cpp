/**************************************************************************/
/*  resource_importer_font.cpp                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "resource_importer_font.h"

#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/font.h"
#include "servers/text/text_server.h"

ResourceImporterFont::ResourceImporterFont(Mode p_mode) :
	mode(p_mode)
{
}

String ResourceImporterFont::get_importer_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "font_data_dynamic";
	case MODE_BMFONT:
		return "font_data_bmfont";
	case MODE_IMAGEFONT:
		return "font_data_image";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterFont::get_visible_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "Font Data (Dynamic Font)";
	case MODE_BMFONT:
		return "Font Data (AngelCode BMFont)";
	case MODE_IMAGEFONT:
		return "Font Data (Image Font)";
	}
	ERR_FAIL_V(String());
}

void ResourceImporterFont::get_recognized_extensions(List<String>* p_extensions) const
{
	if (!p_extensions) {
		return;
	}
	switch (mode) {
	case MODE_DYNAMIC: {
		p_extensions->push_back("ttf");
		p_extensions->push_back("ttc");
		p_extensions->push_back("otf");
		p_extensions->push_back("otc");
		p_extensions->push_back("woff");
		p_extensions->push_back("woff2");
		p_extensions->push_back("pfb");
		p_extensions->push_back("pfm");
	} break;
	case MODE_BMFONT: {
		p_extensions->push_back("font");
		p_extensions->push_back("fnt");
	} break;
	case MODE_IMAGEFONT: {
		ImageLoader::get_recognized_extensions(p_extensions);
	} break;
	}
}

String ResourceImporterFont::get_save_extension() const { return "fontdata"; }

String ResourceImporterFont::get_resource_type() const { return "FontFile"; }

void ResourceImporterFont::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	if (mode == MODE_DYNAMIC) {
		Ref<FontFile> font = ResourceLoader::load(p_path);
		if (font.is_valid() && font->is_multichannel_signed_distance_field()) {
			r_dependencies->insert("module_msdfgen_enabled");
		}
	}
}

int ResourceImporterFont::get_preset_count() const
{
	if (mode == MODE_DYNAMIC) {
		return PRESET_MAX;
	}
	return 0;
}

String ResourceImporterFont::get_preset_name(int p_idx) const
{
	if (mode == MODE_DYNAMIC) {
		switch (p_idx) {
		case PRESET_DYNAMIC:
			return TTR("Dynamically rendered TrueType/OpenType font");
		case PRESET_MSDF:
			return TTR("Prerendered multichannel(+true) signed distance field");
		default:
			return String();
		}
	}
	return String();
}

bool ResourceImporterFont::has_advanced_options() const { return false; }

void ResourceImporterFont::show_advanced_options(const String& p_path) { (void)p_path; }

void ResourceImporterFont::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
	(void)p_path;
	(void)r_options;
	(void)p_preset;
}
