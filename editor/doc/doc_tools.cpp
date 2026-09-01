/**************************************************************************/
/*  doc_tools.cpp                                                         */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/core_constants.h"
#include "core/io/compression.h"
#include "core/io/dir_access.h"
#include "core/io/resource_importer.h"
#include "core/io/xml_parser.h"
#include "core/string/translation_server.h"
#include "doc_tools.h"
#include "editor/export/editor_export_platform.h"
#include "editor/settings/editor_settings.h"
#include "scene/property_list_helper.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"

static String _get_indent(const String& p_text)
{
	String indent;
	bool has_text = false;
	int line_start = 0;

	for (int i = 0; i < p_text.length(); i++) {
		const char32_t c = p_text[i];
		if (c == '\n') {
			line_start = i + 1;
		}
		else if (c > 32) {
			has_text = true;
			indent = p_text.substr(line_start, i - line_start);
			break; // Indentation of the first line that has text.
		}
	}
	if (!has_text) {
		return p_text;
	}
	return indent;
}

static String _translate_doc_string(const String& p_text)
{
	const String indent = _get_indent(p_text);
	const String message = p_text.dedent().strip_edges();
	const String translated =
		TranslationServer::get_singleton()->get_doc_domain()->translate(message, StringName());
	// No need to restore stripped edges because they'll be stripped again later.
	return translated.indent(indent);
}

Error DocTools::load_classes(const String& p_dir)
{
	Error err;
	Ref<DirAccess> da = DirAccess::open(p_dir, &err);
	if (da.is_null()) {
		return err;
	}

	da->list_dir_begin();
	String path;
	path = da->get_next();
	while (!path.is_empty()) {
		if (!da->current_is_dir() && path.ends_with("xml")) {
			Ref<XMLParser> parser = memnew(XMLParser);
			Error err2 = parser->open(p_dir.path_join(path));
			if (err2) {
				return err2;
			}

			_load(parser);
		}
		path = da->get_next();
	}

	da->list_dir_end();

	return OK;
}

Error DocTools::erase_classes(const String& p_dir)
{
	Error err;
	Ref<DirAccess> da = DirAccess::open(p_dir, &err);
	if (da.is_null()) {
		return err;
	}

	List<String> to_erase;

	da->list_dir_begin();
	String path;
	path = da->get_next();
	while (!path.is_empty()) {
		if (!da->current_is_dir() && path.ends_with("xml")) {
			to_erase.push_back(path);
		}
		path = da->get_next();
	}
	da->list_dir_end();

	while (to_erase.size()) {
		da->remove(to_erase.front()->get());
		to_erase.pop_front();
	}

	return OK;
}

static void _write_string(Ref<FileAccess> f, int p_tablevel, const String& p_string)
{
	if (p_string.is_empty()) {
		return;
	}
	String tab = String("\t").repeat(p_tablevel);
	f->store_string(tab + p_string + "\n");
}

Error DocTools::load_compressed(
	const uint8_t* p_data, int64_t p_compressed_size, int64_t p_uncompressed_size)
{
	Vector<uint8_t> data;
	data.resize(p_uncompressed_size);
	const int64_t ret = Compression::decompress(
		data.ptrw(), p_uncompressed_size, p_data, p_compressed_size, Compression::MODE_DEFLATE);
	ERR_FAIL_COND_V_MSG(ret == -1, ERR_FILE_CORRUPT, "Compressed file is corrupt.");

	Ref<XMLParser> parser = memnew(XMLParser);
	Error err = parser->open_buffer(data);
	if (err) {
		return err;
	}

	_load(parser);

	return OK;
}

Error DocTools::load_xml(const uint8_t* p_data, int64_t p_size)
{
	Ref<XMLParser> parser = memnew(XMLParser);
	Error err = parser->_open_buffer(p_data, p_size);
	if (err) {
		return err;
	}

	_load(parser);

	return OK;
}


