/**************************************************************************/
/*  shader_baker_export_plugin.cpp                                        */
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

#include "shader_baker_export_plugin.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/string/string_builder.h"
#include "core/version.h"
#include "scene/3d/label_3d.h"
#include "scene/3d/sprite_3d.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_shader_container.h"

#ifdef VULKAN_ENABLED
#include "drivers/vulkan/rendering_shader_container_vulkan.h"
#endif

#ifdef D3D12_ENABLED
#include "drivers/d3d12/rendering_shader_container_d3d12.h"
#include <windows.h>
#endif

#ifdef METAL_ENABLED
#include "drivers/metal/rendering_shader_container_metal.h"
#endif

// Ensure that AlphaCut is the same between the two classes so we can share the code to detect
// transparency.
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_DISABLED, Label3D::ALPHA_CUT_DISABLED));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_DISCARD, Label3D::ALPHA_CUT_DISCARD));
static_assert(
	ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_OPAQUE_PREPASS, Label3D::ALPHA_CUT_OPAQUE_PREPASS));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_HASH, Label3D::ALPHA_CUT_HASH));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_MAX, Label3D::ALPHA_CUT_MAX));

ShaderBakerExportPluginPlatform::~ShaderBakerExportPluginPlatform()
{
#ifdef D3D12_ENABLED
	if (lib_d3d12 != nullptr) {
		FreeLibrary((HMODULE)(lib_d3d12));
	}
#endif
}

RenderingShaderContainerFormat* ShaderBakerExportPluginPlatform::create_shader_container_format(
	const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset)
{
	switch (driver) {
	case DRIVER_VULKAN: {
#ifdef VULKAN_ENABLED
		return memnew(RenderingShaderContainerFormatVulkan);
#else
		ERR_FAIL_V_MSG(nullptr, "Vulkan shader baker is not enabled in this build.");
#endif
	}
	case DRIVER_D3D12: {
#ifdef D3D12_ENABLED
		if (lib_d3d12 == nullptr) {
			lib_d3d12 = LoadLibraryW(L"D3D12.dll");
			ERR_FAIL_NULL_V_MSG(lib_d3d12, nullptr, "Unable to load D3D12.dll.");
		}
		RenderingShaderContainerFormatD3D12* shader_container_format_d3d12 =
			memnew(RenderingShaderContainerFormatD3D12);
		shader_container_format_d3d12->set_lib_d3d12(lib_d3d12);
		return shader_container_format_d3d12;
#else
		ERR_FAIL_V_MSG(nullptr, "D3D12 shader baker is not enabled in this build.");
#endif
	}
	case DRIVER_METAL: {
#ifdef METAL_ENABLED
		const String& os_name = p_platform->get_os_name();
		const MetalDeviceProfile* profile = nullptr;
		MinOsVersion min_os_version;

		if (os_name == U"macOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::macOS, MetalDeviceProfile::GPU::Apple7, min_os_version);
		}
		else if (os_name == U"iOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::iOS, MetalDeviceProfile::GPU::Apple7, min_os_version);
		}
		else if (os_name == U"visionOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::visionOS, MetalDeviceProfile::GPU::Apple8, min_os_version);
		}
		else {
			ERR_FAIL_V_MSG(nullptr, vformat("Unsupported platform: %s", os_name));
		}
		ERR_FAIL_NULL_V(profile, nullptr);
		return memnew(RenderingShaderContainerFormatMetal(profile, true));
#else
		ERR_FAIL_V_MSG(nullptr, "Metal shader baker is not enabled in this build.");
#endif
	}
	}
	return nullptr;
}

bool ShaderBakerExportPluginPlatform::matches_driver(const String& p_driver) const
{
	switch (driver) {
	case DRIVER_VULKAN:
		return p_driver == "vulkan";
	case DRIVER_D3D12:
		return p_driver == "d3d12";
	case DRIVER_METAL:
		return p_driver == "metal";
	}
	return false;
}

String ShaderBakerExportPlugin::get_name() const { return "ShaderBaker"; }

bool ShaderBakerExportPlugin::_is_active(const Vector<String>& p_features) const
{
	// Shader baker should only work when a RendererRD driver is active, as the embedded shaders
	// won't be found otherwise.
	return RendererSceneRenderRD::data != nullptr && p_features.has("shader_baker");
}

void ShaderBakerExportPlugin::_cleanup_container_format()
{
	if (shader_container_format != nullptr) {
		memdelete(shader_container_format);
		shader_container_format = nullptr;
	}
}

bool ShaderBakerExportPlugin::_initialize_cache_directory()
{
	shader_cache_export_path = get_export_base_path()
								   .path_join("shader_baker")
								   .path_join(shader_cache_platform_name)
								   .path_join(shader_container_driver);

	if (!DirAccess::dir_exists_absolute(shader_cache_export_path)) {
		Error err = DirAccess::make_dir_recursive_absolute(shader_cache_export_path);
		ERR_FAIL_COND_V_MSG(err != OK, false, "Can't create shader cache folder for exporting.");
	}

	return true;
}

bool ShaderBakerExportPlugin::_begin_customize_scenes(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	if (!_is_active(p_features)) {
		return false;
	}

	if (shader_container_format == nullptr) {
		// Resource customization failed to initialize.
		return false;
	}

	return true;
}

Ref<Resource> ShaderBakerExportPlugin::_customize_resource(
	const Ref<Resource>& p_resource, const String& p_path)
{
	DEV_ASSERT(singleton != nullptr);

	Ref<Material> material = p_resource;
	if (material.is_valid()) {
		RID material_rid = material->get_rid();
		if (material_rid.is_valid()) {
			RendererRD::MaterialStorage::ShaderData* shader_data =
				RendererRD::MaterialStorage::material_get_shader_data(material_rid);
			if (shader_data != nullptr) {
				Pair<ShaderRD*, RID> shader_version_pair =
					shader_data->get_native_shader_and_version();
				if (shader_version_pair.first != nullptr) {
					_customize_shader_version(
						shader_version_pair.first, shader_version_pair.second);
				}
			}
		}
	}

	return Ref<Resource>();
}

uint64_t ShaderBakerExportPlugin::_get_customization_configuration_hash() const
{
	return customization_configuration_hash;
}

void ShaderBakerExportPlugin::_process_work_item(WorkItem p_work_item)
{
	if (!tasks_cancelled) {
		Vector<RDC::ShaderStageSPIRVData> spirv_data =
			ShaderRD::compile_stages(p_work_item.stage_sources, p_work_item.dynamic_buffers);
		if (unlikely(spirv_data.is_empty())) {
			ERR_PRINT("Unable to retrieve SPIR-V data for shader.");
		}
		else {
			Ref<RenderingShaderContainer> shader_container =
				shader_container_format->create_container();

			bool code_compiled =
				shader_container->set_code_from_spirv(p_work_item.shader_name, spirv_data);
			if (unlikely(!code_compiled)) {
				ERR_PRINT("Failed to compile code to native for SPIR-V.");
			}
			else {
				PackedByteArray shader_bytes = shader_container->to_bytes();
				{
					MutexLock lock(shader_work_results_mutex);
					shader_work_results[p_work_item.cache_path]
						.variant_data.ptrw()[p_work_item.variant] = shader_bytes;
				}
			}
		}
	}

	{
		MutexLock lock(tasks_mutex);
		tasks_processed++;
	}

	tasks_condition.notify_one();
}

ShaderBakerExportPlugin::ShaderBakerExportPlugin()
{
#ifdef VULKAN_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_VULKAN)));
#endif
#ifdef D3D12_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_D3D12)));
#endif
#ifdef METAL_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_METAL)));
#endif
}

ShaderBakerExportPlugin::~ShaderBakerExportPlugin() {}

void ShaderBakerExportPlugin::add_platform(Ref<ShaderBakerExportPluginPlatform> p_platform)
{
	platforms.push_back(p_platform);
}

void ShaderBakerExportPlugin::remove_platform(Ref<ShaderBakerExportPluginPlatform> p_platform)
{
	platforms.erase(p_platform);
}

bool ShaderBakerExportPlugin::_begin_customize_resources(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	return false;
}

Node* ShaderBakerExportPlugin::_customize_scene(Node* p_scene, const String& p_path)
{
	return p_scene;
}

void ShaderBakerExportPlugin::_end_customize_resources() {}

bool ShaderBakerExportPlugin::_initialize_container_format(
	const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset)
{
	return false;
}

void ShaderBakerExportPlugin::_customize_shader_version(ShaderRD* p_shader, RID p_version) {}
