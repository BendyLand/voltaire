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

#include "editor_import_plugin.h"

#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "editor/file_system/editor_file_system.h"


void EditorImportPlugin::get_recognized_extensions(List<String> *p_extensions) const {
	Vector<String> extensions;
	for (int i = 0; i < extensions.size(); i++) {
		p_extensions->push_back(extensions[i]);
	}
}

void EditorImportPlugin::get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const {
	Array needed = { "name", "default_value" };
	TypedArray<Dictionary> options;
	for (int i = 0; i < options.size(); i++) {
		Dictionary d = options[i];
		ERR_FAIL_COND(!d.has_all(needed));
		String name = d["name"];
		Variant default_value = d["default_value"];

		PropertyHint hint = PROPERTY_HINT_NONE;
		if (d.has("property_hint")) {
			hint = (PropertyHint)d["property_hint"].operator int64_t();
		}

		String hint_string;
		if (d.has("hint_string")) {
			hint_string = d["hint_string"];
		}

		uint32_t usage = PROPERTY_USAGE_DEFAULT;
		if (d.has("usage")) {
			usage = d["usage"];
		}

		ImportOption option(PropertyInfo(default_value.get_type(), name, hint, hint_string, usage), default_value);
		r_options->push_back(option);
	}
}

bool EditorImportPlugin::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	Dictionary d;
	HashMap<StringName, Variant>::ConstIterator E = p_options.begin();
	while (E) {
		d[E->key] = E->value;
		++E;
	}
	return true;
}

Error EditorImportPlugin::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Dictionary options;
	TypedArray<String> platform_variants, gen_files;

	HashMap<StringName, Variant>::ConstIterator E = p_options.begin();
	while (E) {
		options[E->key] = E->value;
		++E;
	}

	Error err = OK;
	for (int i = 0; i < platform_variants.size(); i++) {
		r_platform_variants->push_back(platform_variants[i]);
	}
	for (int i = 0; i < gen_files.size(); i++) {
		r_gen_files->push_back(gen_files[i]);
	}
	return err;
}

bool EditorImportPlugin::can_import_threaded() const {
	return ResourceImporter::can_import_threaded();
}

Error EditorImportPlugin::_append_import_external_resource(const String &p_file, const Dictionary &p_custom_options, const String &p_custom_importer, Variant p_generator_parameters) {
	HashMap<StringName, Variant> options;
	for (const KeyValue<Variant, Variant> &kv : p_custom_options) {
		options.insert(kv.key, kv.value);
	}
	return append_import_external_resource(p_file, options, p_custom_importer, p_generator_parameters);
}

Error EditorImportPlugin::append_import_external_resource(const String &p_file, const HashMap<StringName, Variant> &p_custom_options, const String &p_custom_importer, Variant p_generator_parameters) {
	ERR_FAIL_COND_V_MSG(!EditorFileSystem::get_singleton()->is_importing(), ERR_INVALID_PARAMETER, "Can only append files to import during a current reimport process.");
	return EditorFileSystem::get_singleton()->reimport_append(p_file, p_custom_options, p_custom_importer, p_generator_parameters);
}

void EditorImportPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_import_external_resource", "path", "custom_options", "custom_importer", "generator_parameters"), &EditorImportPlugin::_append_import_external_resource, DEFVAL(Dictionary()), DEFVAL(String()), DEFVAL(Variant()));
}

String EditorImportPlugin::get_importer_name() const { return ""; }
String EditorImportPlugin::get_visible_name() const { return ""; }
String EditorImportPlugin::get_save_extension() const { return ""; }
String EditorImportPlugin::get_resource_type() const { return ""; }
int EditorImportPlugin::get_preset_count() const { return 0; }
float EditorImportPlugin::get_priority() const {
	return 1.0;
}

int EditorImportPlugin::get_import_order() const {
	return 0;
}

int EditorImportPlugin::get_format_version() const {
	return 0;
}

String EditorImportPlugin::get_preset_name(int p_idx) const {
	return String();
}
