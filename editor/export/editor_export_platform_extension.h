/**************************************************************************/
/*  editor_export_platform_extension.h                                    */
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

#pragma once

#include "core/object/gdvirtual.gen.h"
#include "core/variant/typed_array.h"
#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_preset.h"

class ImageTexture;

class EditorExportPlatformExtension : public EditorExportPlatform {
	VLTRCLASS(EditorExportPlatformExtension, EditorExportPlatform);

	mutable String config_error;
	mutable bool config_missing_templates = false;

protected:
	static void _bind_methods();

public:
	virtual void get_preset_features(const Ref<EditorExportPreset> &p_preset, List<String> *r_features) const override;
	virtual bool is_executable(const String &p_path) const override;
	virtual void get_export_options(List<ExportOption> *r_options) const override;
	virtual bool should_update_export_options() override;
	virtual bool get_export_option_visibility(const EditorExportPreset *p_preset, const String &p_option) const override;
	virtual String get_export_option_warning(const EditorExportPreset *p_preset, const StringName &p_name) const override;
	virtual String get_os_name() const override;
	virtual String get_name() const override;
	virtual Ref<Texture2D> get_logo() const override;
	virtual bool poll_export() override;
	virtual int get_options_count() const override;
	virtual String get_options_tooltip() const override;
	virtual Ref<Texture2D> get_option_icon(int p_index) const override;
	virtual String get_option_label(int p_device) const override;
	virtual String get_option_tooltip(int p_device) const override;
	virtual String get_device_architecture(int p_device) const override;
	virtual void cleanup() override;
	virtual Error run(const Ref<EditorExportPreset> &p_preset, int p_device, BitField<EditorExportPlatform::DebugFlags> p_debug_flags) override;
	virtual Ref<Texture2D> get_run_icon() const override;
	void set_config_error(const String &p_error) const {
		config_error = p_error;
	}
	String get_config_error() const {
		return config_error;
	}

	void set_config_missing_templates(bool p_missing_templates) const {
		config_missing_templates = p_missing_templates;
	}
	bool get_config_missing_templates() const {
		return config_missing_templates;
	}

	virtual bool can_export(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error) const override;
	virtual List<String> get_binary_extensions(const Ref<EditorExportPreset> &p_preset) const override;
	virtual Error export_project(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags = 0, bool p_notify = true) override;
	virtual Error export_pack(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags = 0) override;
	virtual Error export_zip(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags = 0) override;
	virtual Error export_pack_patch(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, const Vector<String> &p_patches = Vector<String>(), BitField<EditorExportPlatform::DebugFlags> p_flags = 0) override;
	virtual Error export_zip_patch(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, const Vector<String> &p_patches = Vector<String>(), BitField<EditorExportPlatform::DebugFlags> p_flags = 0) override;
	virtual void get_platform_features(List<String> *r_features) const override;
	virtual String get_debug_protocol() const override;
	virtual void initialize() override;
	EditorExportPlatformExtension();
	~EditorExportPlatformExtension();
};
