/**************************************************************************/
/*  fog.cpp                                                               */
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

#include "fog.h"
#include "servers/rendering/renderer_rd/cluster_builder_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_server_default.h"

using namespace RendererRD;

Fog* Fog::singleton = nullptr;

Fog::Fog() { singleton = this; }

Fog::~Fog() { singleton = nullptr; }

int Fog::_get_fog_shader_group()
{
	RenderingDevice* rd = RD::get_singleton();
	bool use_32_bit_atomics = rd->has_feature(RD::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = rd->has_feature(RD::SUPPORTS_VULKAN_MEMORY_MODEL);
	if (use_vulkan_memory_model) {
		return use_32_bit_atomics
				   ? VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL
				   : VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL_NO_ATOMICS;
	}
	else {
		return use_32_bit_atomics ? VolumetricFogShader::SHADER_GROUP_BASE
								  : VolumetricFogShader::SHADER_GROUP_NO_ATOMICS;
	}
}

int Fog::_get_fog_variant()
{
	RenderingDevice* rd = RD::get_singleton();
	bool use_32_bit_atomics = rd->has_feature(RD::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = rd->has_feature(RD::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? 2 : 0) + (use_32_bit_atomics ? 0 : 1);
}

int Fog::_get_fog_process_variant(int p_idx)
{
	RenderingDevice* rd = RD::get_singleton();
	bool use_32_bit_atomics = rd->has_feature(RD::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = rd->has_feature(RD::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? (VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX * 2)
									: 0) +
		   (use_32_bit_atomics ? 0 : VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX) +
		   p_idx;
}

/* FOG VOLUMES */

RID Fog::fog_volume_allocate() { return fog_volume_owner.allocate_rid(); }

void Fog::fog_volume_initialize(RID p_rid) { fog_volume_owner.initialize_rid(p_rid, FogVolume()); }

void Fog::fog_volume_free(RID p_rid)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_rid);
	fog_volume->dependency.deleted_notify(p_rid);
	fog_volume_owner.free(p_rid);
}

Dependency* Fog::fog_volume_get_dependency(RID p_fog_volume) const
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, nullptr);

	return &fog_volume->dependency;
}

void Fog::fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	if (p_shape == fog_volume->shape) {
		return;
	}

	fog_volume->shape = p_shape;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_size(RID p_fog_volume, const Vector3& p_size)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	fog_volume->size = p_size;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_material(RID p_fog_volume, RID p_material)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);
	fog_volume->material = p_material;
}

RID Fog::fog_volume_get_material(RID p_fog_volume) const
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RID());

	return fog_volume->material;
}

RSE::FogVolumeShape Fog::fog_volume_get_shape(RID p_fog_volume) const
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RSE::FOG_VOLUME_SHAPE_BOX);

	return fog_volume->shape;
}

AABB Fog::fog_volume_get_aabb(RID p_fog_volume) const
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, AABB());

	switch (fog_volume->shape) {
	case RSE::FOG_VOLUME_SHAPE_ELLIPSOID:
	case RSE::FOG_VOLUME_SHAPE_CONE:
	case RSE::FOG_VOLUME_SHAPE_CYLINDER:
	case RSE::FOG_VOLUME_SHAPE_BOX: {
		AABB aabb;
		aabb.position = -fog_volume->size / 2;
		aabb.size = fog_volume->size;
		return aabb;
	}
	default: {
		// Need some size otherwise will get culled
		return AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	}
	}
}

Vector3 Fog::fog_volume_get_size(RID p_fog_volume) const
{
	const FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, Vector3());
	return fog_volume->size;
}

Fog::FogMaterialData::~FogMaterialData() { free_parameters_uniform_set(uniform_set); }

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_func()
{
	FogShaderData* shader_data = memnew(FogShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_funcs()
{
	return Fog::get_singleton()->_create_fog_shader_func();
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_func(FogShaderData* p_shader)
{
	FogMaterialData* material_data = memnew(FogMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_funcs(
	RendererRD::MaterialStorage::ShaderData* p_shader)
{
	return Fog::get_singleton()->_create_fog_material_func(static_cast<FogShaderData*>(p_shader));
}

RID Fog::fog_volume_instance_create(RID p_fog_volume)
{
	FogVolumeInstance fvi;
	fvi.volume = p_fog_volume;
	return fog_volume_instance_owner.make_rid(fvi);
}

void Fog::fog_instance_free(RID p_rid) { fog_volume_instance_owner.free(p_rid); }

void Fog::free_fog_shader()
{
	MaterialStorage* material_storage = MaterialStorage::get_singleton();
	for (int i = 0; i < VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX; i++) {
		volumetric_fog.process_pipelines[i].free();
	}
	if (volumetric_fog.process_shader_version.is_valid()) {
		volumetric_fog.process_shader.version_free(volumetric_fog.process_shader_version);
	}
	if (volumetric_fog.volume_ubo.is_valid()) {
		RD::get_singleton()->free_rid(volumetric_fog.volume_ubo);
	}
	if (volumetric_fog.params_ubo.is_valid()) {
		RD::get_singleton()->free_rid(volumetric_fog.params_ubo);
	}
	if (volumetric_fog.default_shader.is_valid()) {
		material_storage->shader_free(volumetric_fog.default_shader);
	}
	if (volumetric_fog.default_material.is_valid()) {
		material_storage->material_free(volumetric_fog.default_material);
	}
}

void Fog::FogShaderData::set_code(const String& p_code)
{
	// compile

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	if (code.is_empty()) {
		return; // just invalid, but no error
	}

	ShaderCompiler::GeneratedCode gen_code;
	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["fog"] = ShaderCompiler::STAGE_COMPUTE;

	uses_time = false;

	actions.usage_flag_pointers["TIME"] = &uses_time;

	actions.uniforms = &uniforms;

	Fog* fog_singleton = Fog::get_singleton();

	Error err = fog_singleton->volumetric_fog.compiler.compile(
		RSE::SHADER_FOG, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Fog shader compilation failed.");

	if (version.is_null()) {
		version = fog_singleton->volumetric_fog.shader.version_create();
	}
	else {
		pipeline.free();
	}

	fog_singleton->volumetric_fog.shader.version_set_compute_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_COMPUTE], gen_code.defines);
	ERR_FAIL_COND(!fog_singleton->volumetric_fog.shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool Fog::FogShaderData::is_animated() const { return false; }

bool Fog::FogShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode Fog::FogShaderData::get_native_source_code() const
{
	Fog* fog_singleton = Fog::get_singleton();

	return fog_singleton->volumetric_fog.shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> Fog::FogShaderData::get_native_shader_and_version() const
{
	Fog* fog_singleton = Fog::get_singleton();
	return {&fog_singleton->volumetric_fog.shader, version};
}

Fog::FogShaderData::~FogShaderData()
{
	pipeline.free();

	Fog* fog_singleton = Fog::get_singleton();
	ERR_FAIL_NULL(fog_singleton);
	if (version.is_valid()) {
		fog_singleton->volumetric_fog.shader.version_free(version);
	}
}

bool Fog::VolumetricFog::sync_gi_dependent_sets_validity(bool p_ensure_freed)
{
	bool null = gi_dependent_sets.process_uniform_set_density.is_null();
	bool valid = !null && RD::get_singleton()->uniform_set_is_valid(
							  gi_dependent_sets.process_uniform_set_density);

#ifdef DEV_ENABLED
	// It's all-or-nothing, or something else has changed that requires dev attention.
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set.is_null());
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set2.is_null());
	DEV_ASSERT(
		valid == RD::get_singleton()->uniform_set_is_valid(gi_dependent_sets.process_uniform_set));
	DEV_ASSERT(
		valid == RD::get_singleton()->uniform_set_is_valid(gi_dependent_sets.process_uniform_set2));
#endif

	if (valid) {
		if (p_ensure_freed) {
			RD::get_singleton()->free_rid(gi_dependent_sets.process_uniform_set_density);
			RD::get_singleton()->free_rid(gi_dependent_sets.process_uniform_set);
			RD::get_singleton()->free_rid(gi_dependent_sets.process_uniform_set2);
			valid = false;
		}
	}

	if (!valid && !null) {
		gi_dependent_sets = {};
	}

	return valid;
}

void Fog::VolumetricFog::init(const Vector3i& fog_size, RID p_sky_shader)
{
	width = fog_size.x;
	height = fog_size.y;
	depth = fog_size.z;
	atomic_type = RD::get_singleton()->has_feature(RD::SUPPORTS_IMAGE_ATOMIC_32_BIT)
					  ? RD::UNIFORM_TYPE_IMAGE
					  : RD::UNIFORM_TYPE_STORAGE_BUFFER;

	RD::TextureFormat tf;
	tf.format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
	tf.width = fog_size.x;
	tf.height = fog_size.y;
	tf.depth = fog_size.z;
	tf.texture_type = RD::TEXTURE_TYPE_3D;
	tf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

	light_density_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
	RD::get_singleton()->set_resource_name(light_density_map, "Fog light-density map");

	tf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT |
					RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;

	prev_light_density_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
	RD::get_singleton()->set_resource_name(
		prev_light_density_map, "Fog previous light-density map");
	RD::get_singleton()->texture_clear(prev_light_density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);

	tf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;

	fog_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
	RD::get_singleton()->set_resource_name(fog_map, "Fog map");

	if (atomic_type == RD::UNIFORM_TYPE_STORAGE_BUFFER) {
		Vector<uint8_t> dm;
		dm.resize_initialized(fog_size.x * fog_size.y * fog_size.z * 4);

		density_map = RD::get_singleton()->storage_buffer_create(dm.size(), dm);
		RD::get_singleton()->set_resource_name(density_map, "Fog density map");
		light_map = RD::get_singleton()->storage_buffer_create(dm.size(), dm);
		RD::get_singleton()->set_resource_name(light_map, "Fog light map");
		emissive_map = RD::get_singleton()->storage_buffer_create(dm.size(), dm);
		RD::get_singleton()->set_resource_name(emissive_map, "Fog emissive map");
	}
	else {
		tf.format = RD::DATA_FORMAT_R32_UINT;
		tf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT |
						RD::TEXTURE_USAGE_STORAGE_ATOMIC_BIT;
		density_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
		RD::get_singleton()->set_resource_name(density_map, "Fog density map");
		RD::get_singleton()->texture_clear(density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		light_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
		RD::get_singleton()->set_resource_name(light_map, "Fog light map");
		RD::get_singleton()->texture_clear(light_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		emissive_map = RD::get_singleton()->texture_create(tf, RD::TextureView());
		RD::get_singleton()->set_resource_name(emissive_map, "Fog emissive map");
		RD::get_singleton()->texture_clear(emissive_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
	}

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.append_id(fog_map);
		uniforms.push_back(u);
	}

	sky_uniform_set = RD::get_singleton()->uniform_set_create(
		uniforms, p_sky_shader, RendererRD::SkyRD::SKY_SET_FOG);
}

Fog::VolumetricFog::~VolumetricFog()
{
	RD::get_singleton()->free_rid(prev_light_density_map);
	RD::get_singleton()->free_rid(light_density_map);
	RD::get_singleton()->free_rid(fog_map);
	RD::get_singleton()->free_rid(density_map);
	RD::get_singleton()->free_rid(light_map);
	RD::get_singleton()->free_rid(emissive_map);

	if (fog_uniform_set.is_valid() && RD::get_singleton()->uniform_set_is_valid(fog_uniform_set)) {
		RD::get_singleton()->free_rid(fog_uniform_set);
	}
	if (copy_uniform_set.is_valid() &&
		RD::get_singleton()->uniform_set_is_valid(copy_uniform_set)) {
		RD::get_singleton()->free_rid(copy_uniform_set);
	}

	sync_gi_dependent_sets_validity(true);

	if (sdfgi_uniform_set.is_valid() &&
		RD::get_singleton()->uniform_set_is_valid(sdfgi_uniform_set)) {
		RD::get_singleton()->free_rid(sdfgi_uniform_set);
	}
	if (sky_uniform_set.is_valid() && RD::get_singleton()->uniform_set_is_valid(sky_uniform_set)) {
		RD::get_singleton()->free_rid(sky_uniform_set);
	}
}

Vector3i Fog::_point_get_position_in_froxel_volume(const Vector3& p_point, float fog_end,
	const Vector2& fog_near_size, const Vector2& fog_far_size, float volumetric_fog_detail_spread,
	const Vector3& fog_size, const Transform3D& p_cam_transform)
{
	Vector3 view_position = p_cam_transform.affine_inverse().xform(p_point);
	view_position.z = MIN(view_position.z, -0.01); // Clamp to the front of camera
	Vector3 fog_position = Vector3(0, 0, 0);

	view_position.y = -view_position.y;
	fog_position.z = -view_position.z / fog_end;
	fog_position.x =
		(view_position.x /
			(2 * (fog_near_size.x * (1.0 - fog_position.z) + fog_far_size.x * fog_position.z))) +
		0.5;
	fog_position.y =
		(view_position.y /
			(2 * (fog_near_size.y * (1.0 - fog_position.z) + fog_far_size.y * fog_position.z))) +
		0.5;
	fog_position.z = Math::pow(float(fog_position.z), float(1.0 / volumetric_fog_detail_spread));
	fog_position = fog_position * fog_size - Vector3(0.5, 0.5, 0.5);

	fog_position = fog_position.clamp(Vector3(), fog_size);

	return Vector3i(fog_position);
}


