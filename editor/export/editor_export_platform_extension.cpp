/**************************************************************************/
/*  editor_export_platform_extension.cpp                                  */
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

#include "editor_export_platform_extension.h"

#include "core/object/class_db.h"
#include "scene/resources/image_texture.h"

void EditorExportPlatformExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_config_error", "error_text"), &EditorExportPlatformExtension::set_config_error);
	ClassDB::bind_method(D_METHOD("get_config_error"), &EditorExportPlatformExtension::get_config_error);

	ClassDB::bind_method(D_METHOD("set_config_missing_templates", "missing_templates"), &EditorExportPlatformExtension::set_config_missing_templates);
	ClassDB::bind_method(D_METHOD("get_config_missing_templates"), &EditorExportPlatformExtension::get_config_missing_templates);
}

bool EditorExportPlatformExtension::can_export(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates, bool p_debug) const {
	config_error = r_error;
	config_missing_templates = r_missing_templates;
	return EditorExportPlatform::can_export(p_preset, r_error, r_missing_templates, p_debug);
}

bool EditorExportPlatformExtension::has_valid_export_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates, bool p_debug) const {
	bool ret = false;
	config_error = r_error;
	config_missing_templates = r_missing_templates;
	return ret;
}

bool EditorExportPlatformExtension::has_valid_project_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error) const {
	bool ret = false;
	config_error = r_error;
	return ret;
}

List<String> EditorExportPlatformExtension::get_binary_extensions(const Ref<EditorExportPreset> &p_preset) const {
	List<String> ret_list;
	Vector<String> ret;
	for (const String &E : ret) {
		ret_list.push_back(E);
	}
	return ret_list;
}

Error EditorExportPlatformExtension::export_project(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags, bool p_notify) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags, p_notify);
	Error ret = FAILED;
	return ret;
}

Error EditorExportPlatformExtension::export_pack(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	return save_pack(p_preset, p_debug, p_path);
}

Error EditorExportPlatformExtension::export_zip(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	return save_zip(p_preset, p_debug, p_path);
}

Error EditorExportPlatformExtension::export_pack_patch(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, const Vector<String> &p_patches, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	Error err = _load_patches(p_preset, p_patches.is_empty() ? p_preset->get_patches() : p_patches);
	if (err != OK) {
		return err;
	}
	err = save_pack_patch(p_preset, p_debug, p_path);
	_unload_patches();
	return err;
}

Error EditorExportPlatformExtension::export_zip_patch(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, const Vector<String> &p_patches, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);

	Error err = _load_patches(p_preset, p_patches.is_empty() ? p_preset->get_patches() : p_patches);
	if (err != OK) {
		return err;
	}
	err = save_zip_patch(p_preset, p_debug, p_path);
	_unload_patches();
	return err;
}

void EditorExportPlatformExtension::get_platform_features(List<String> *r_features) const {
	Vector<String> ret;
	for (const String &E : ret) {
		r_features->push_back(E);
	}
}

String EditorExportPlatformExtension::get_debug_protocol() const {
	return EditorExportPlatform::get_debug_protocol();
}

EditorExportPlatformExtension::EditorExportPlatformExtension() {
	//NOP
}

EditorExportPlatformExtension::~EditorExportPlatformExtension() {
	//NOP
}
