/**************************************************************************/
/*  editor_export.h                                                       */
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

#pragma once

#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_plugin.h"

class Timer;

class EditorExport : public Node
{
	Vector<Ref<EditorExportPlatform>> export_platforms;
	Vector<Ref<EditorExportPreset>> export_presets;
	Vector<Ref<EditorExportPlugin>> export_plugins;
	HashMap<Ref<EditorExportPlatform>, Ref<EditorExportPreset>> runnable_presets;

	static inline const String RUNNABLE_SECTION_NAME = "runnable_presets";

	static inline String _export_presets_updated;
	static inline String _export_presets_runnable_updated;

	Timer* save_timer = nullptr;
	bool block_save = false;
	bool should_update_presets = false;
	bool should_reload_presets = false;



	void _save();

protected:
	friend class EditorExportPreset;
	void emit_presets_runnable_changed();

	void _notification(int p_what);

public:
	static inline EditorExport* instance = nullptr;
	_FORCE_INLINE_ static bool is_initialized() { return instance != nullptr; }

	// Encodes a platform/device pair into a single menu/device id and decodes it back.
	static int encode_platform_device_id(int p_platform_idx, int p_device_idx)
	{
		return p_platform_idx * 10000 + p_device_idx;
	}

	static int decode_platform_from_id(int p_id) { return p_id / 10000; }

	static int decode_device_from_id(int p_id) { return p_id % 10000; }

	void _impl_add_export_platform(const Ref<EditorExportPlatform>& p_platform);
	static void add_export_platform(const Ref<EditorExportPlatform>& p_platform);
	int _impl_get_export_platform_count() const;
	static int get_export_platform_count();
	int get_export_platform_index_by_name(const String& p_name);
	Ref<EditorExportPlatform> _impl_get_export_platform(int p_idx);
	static Ref<EditorExportPlatform> get_export_platform(int p_idx);
	void _impl_remove_export_platform(const Ref<EditorExportPlatform>& p_platform);
	static void remove_export_platform(const Ref<EditorExportPlatform>& p_platform);

	void add_export_preset(const Ref<EditorExportPreset>& p_preset, int p_at_pos = -1);
	int _impl_get_export_preset_count() const;
	static int get_export_preset_count();
	Ref<EditorExportPreset> _impl_get_export_preset(int p_idx);
	static Ref<EditorExportPreset> get_export_preset(int p_idx);

	void _impl_add_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	static void add_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	void _impl_remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	static void remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	Vector<Ref<EditorExportPlugin>> _impl_get_export_plugins();
	static Vector<Ref<EditorExportPlugin>> get_export_plugins();

	Ref<EditorExportPreset> _impl_get_runnable_preset_for_platform(const Ref<EditorExportPlatform>& p_for_platform) const;
	static Ref<EditorExportPreset> get_runnable_preset_for_platform(const Ref<EditorExportPlatform>& p_for_platform);

	void load_config();
	void update_export_presets();
	bool poll_export_platforms();
	bool has_preset_with_name(const String& p_name, int p_exclude_index = -1) const;

	EditorExport();
};


