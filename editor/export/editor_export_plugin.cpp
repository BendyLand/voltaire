/**************************************************************************/
/*  editor_export_plugin.cpp                                              */
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
#include "editor/export/editor_export_platform.h"
#include "editor_export_plugin.h"

void EditorExportPlugin::set_export_base_path(const String& p_export_base_path)
{
	export_base_path = p_export_base_path;
}

const String& EditorExportPlugin::get_export_base_path() const { return export_base_path; }

void EditorExportPlugin::set_export_preset(const Ref<EditorExportPreset>& p_preset)
{
	if (p_preset.is_valid()) {
		export_preset = p_preset;
	}
}

Ref<EditorExportPreset> EditorExportPlugin::get_export_preset() const { return export_preset; }

Ref<EditorExportPlatform> EditorExportPlugin::get_export_platform() const
{
	if (export_preset.is_valid()) {
		return export_preset->get_platform();
	}
	else {
		return Ref<EditorExportPlatform>();
	}
}

void EditorExportPlugin::add_file(const String& p_path, const Vector<uint8_t>& p_file, bool p_remap)
{
	ExtraFile ef;
	ef.data = p_file;
	ef.path = p_path;
	ef.remap = p_remap;
	extra_files.push_back(ef);
}

void EditorExportPlugin::add_shared_object(
	const String& p_path, const Vector<String>& p_tags, const String& p_target)
{
	shared_objects.push_back(SharedObject(p_path, p_tags, p_target));
}

void EditorExportPlugin::_add_shared_object(const SharedObject& p_shared_object)
{
	shared_objects.push_back(p_shared_object);
}

void EditorExportPlugin::add_apple_embedded_platform_framework(const String& p_path)
{
	apple_embedded_platform_frameworks.push_back(p_path);
}

void EditorExportPlugin::add_apple_embedded_platform_embedded_framework(const String& p_path)
{
	apple_embedded_platform_embedded_frameworks.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_frameworks() const
{
	return apple_embedded_platform_frameworks;
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_embedded_frameworks() const
{
	return apple_embedded_platform_embedded_frameworks;
}

void EditorExportPlugin::add_apple_embedded_platform_plist_content(const String& p_plist_content)
{
	apple_embedded_platform_plist_content += p_plist_content + "\n";
}

String EditorExportPlugin::get_apple_embedded_platform_plist_content() const
{
	return apple_embedded_platform_plist_content;
}

void EditorExportPlugin::add_apple_embedded_platform_linker_flags(const String& p_flags)
{
	if (apple_embedded_platform_linker_flags.length() > 0) {
		apple_embedded_platform_linker_flags += ' ';
	}
	apple_embedded_platform_linker_flags += p_flags;
}

String EditorExportPlugin::get_apple_embedded_platform_linker_flags() const
{
	return apple_embedded_platform_linker_flags;
}

void EditorExportPlugin::add_apple_embedded_platform_bundle_file(const String& p_path)
{
	apple_embedded_platform_bundle_files.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_bundle_files() const
{
	return apple_embedded_platform_bundle_files;
}

void EditorExportPlugin::add_apple_embedded_platform_cpp_code(const String& p_code)
{
	apple_embedded_platform_cpp_code += p_code;
}

String EditorExportPlugin::get_apple_embedded_platform_cpp_code() const
{
	return apple_embedded_platform_cpp_code;
}

void EditorExportPlugin::add_macos_plugin_file(const String& p_path)
{
	macos_plugin_files.push_back(p_path);
}

const Vector<String>& EditorExportPlugin::get_macos_plugin_files() const
{
	return macos_plugin_files;
}

void EditorExportPlugin::add_apple_embedded_platform_project_static_lib(const String& p_path)
{
	apple_embedded_platform_project_static_libs.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_project_static_libs() const
{
	return apple_embedded_platform_project_static_libs;
}

// Customization

PackedStringArray EditorExportPlugin::get_export_features(
	const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const
{
	return _get_export_features(p_export_platform, p_debug);
}

void EditorExportPlugin::_export_file(
	const String& p_path, const String& p_type, const HashSet<String>& p_features)
{
}

void EditorExportPlugin::_export_begin(
	const HashSet<String>& p_features, bool p_debug, const String& p_path, int p_flags)
{
}

void EditorExportPlugin::_export_end() {}

void EditorExportPlugin::_end_generate_apple_embedded_project(
	const String& p_path, bool p_p_will_build_archive)
{
}

void EditorExportPlugin::end_generate_apple_embedded_project(
	const String& p_path, bool p_will_build_archive)
{
	_end_generate_apple_embedded_project(p_path, p_will_build_archive);
}

void EditorExportPlugin::skip() { skipped = true; }

void EditorExportPlugin::_bind_methods() {}

bool EditorExportPlugin::_begin_customize_resources(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_targets)
{
	return true;
}

Ref<Resource> EditorExportPlugin::_customize_resource(
	const Ref<Resource>& p_resource, const String& p_path)
{
	return nullptr;
}

bool EditorExportPlugin::_begin_customize_scenes(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_targets)
{
	return true;
}

Node* EditorExportPlugin::_customize_scene(Node* p_scene, const String& p_path) { return nullptr; }

uint64_t EditorExportPlugin::_get_customization_configuration_hash() const { return 0; }

void EditorExportPlugin::_end_customize_scenes() {}

void EditorExportPlugin::_end_customize_resources() {}

PackedStringArray EditorExportPlugin::_get_export_features(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return PackedStringArray();
}

bool EditorExportPlugin::_should_update_export_options(
	const Ref<EditorExportPlatform>& p_platform) const
{
	return false;
}

bool EditorExportPlugin::_get_export_option_visibility(
	const Ref<EditorExportPlatform>& p_platform, const String& p_option) const
{
	return true;
}

String EditorExportPlugin::_get_export_option_warning(
	const Ref<EditorExportPlatform>& p_platform, const String& p_option) const
{
	return String();
}

String EditorExportPlugin::get_name() const { return String(); }

bool EditorExportPlugin::supports_platform(const Ref<EditorExportPlatform>& p_platform) const
{
	return true;
}

Vector<String> EditorExportPlugin::get_android_dependencies(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

Vector<String> EditorExportPlugin::get_android_dependencies_maven_repos(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

Vector<String> EditorExportPlugin::get_android_libraries(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

String EditorExportPlugin::get_android_manifest_activity_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

String EditorExportPlugin::get_android_manifest_application_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

String EditorExportPlugin::get_android_manifest_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

PackedByteArray EditorExportPlugin::update_android_prebuilt_manifest(
	const Ref<EditorExportPlatform>& p_export_platform,
	const PackedByteArray& p_manifest_data) const
{
	return PackedByteArray();
}


