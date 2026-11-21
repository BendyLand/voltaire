/**************************************************************************/
/*  shader_baker_export_plugin.h                                          */
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

#include "core/os/condition_variable.h"
#include "core/templates/rb_set.h"
#include "editor/export/editor_export_plugin.h"

class ShaderRD;
class RenderingShaderContainerFormat;

class ShaderBakerExportPluginPlatform final : public RefCounted
{
public:
	enum Driver
	{
		DRIVER_VULKAN,
		DRIVER_D3D12,
		DRIVER_METAL,
	};

private:
	Driver driver;
#ifdef D3D12_ENABLED
	void* lib_d3d12 = nullptr;
#endif

public:
	explicit ShaderBakerExportPluginPlatform(Driver p_driver) : driver(p_driver) {}
	~ShaderBakerExportPluginPlatform() override;

	Driver get_driver() const { return driver; }
	RenderingShaderContainerFormat* create_shader_container_format(
		const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset);
	bool matches_driver(const String& p_driver) const;
};

class ShaderBakerExportPlugin final : public EditorExportPlugin
{
protected:
	struct WorkItem
	{
		String cache_path;
		String shader_name;
		Vector<String> stage_sources;
		Vector<uint64_t> dynamic_buffers;
		int64_t variant = 0;
	};

	struct WorkResult
	{
		Vector<PackedByteArray> variant_data;
	};

	struct ShaderGroupItem
	{
		String cache_path;
		LocalVector<int> variants;
	};

	String shader_cache_platform_name;
	String shader_cache_renderer_name;
	String shader_cache_export_path;
	RBSet<String> shader_paths_processed;
	HashMap<String, WorkResult> shader_work_results;
	Mutex shader_work_results_mutex;
	LocalVector<ShaderGroupItem> shader_group_items;
	RenderingShaderContainerFormat* shader_container_format = nullptr;
	String shader_container_driver;
	Vector<Ref<ShaderBakerExportPluginPlatform>> platforms;
	uint64_t customization_configuration_hash = 0;
	uint32_t tasks_processed = 0;
	uint32_t tasks_total = 0;
	std::atomic<bool> tasks_cancelled;
	BinaryMutex tasks_mutex;
	ConditionVariable tasks_condition;

	String get_name() const override;
	bool _is_active(const Vector<String>& p_features) const;
	bool _initialize_container_format(
		const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset);
	void _cleanup_container_format();
	bool _initialize_cache_directory();
	bool _begin_customize_resources(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;
	bool _begin_customize_scenes(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;
	void _end_customize_resources() override;
	Ref<Resource> _customize_resource(
		const Ref<Resource>& p_resource, const String& p_path) override;
	Node* _customize_scene(Node* p_root, const String& p_path) override;
	uint64_t _get_customization_configuration_hash() const override;
	void _customize_shader_version(ShaderRD* p_shader, RID p_version);
	void _process_work_item(WorkItem p_work_item);

public:
	ShaderBakerExportPlugin();
	~ShaderBakerExportPlugin() override;
	void add_platform(Ref<ShaderBakerExportPluginPlatform> p_platform);
	void remove_platform(Ref<ShaderBakerExportPluginPlatform> p_platform);
};
