/**************************************************************************/
/*  scene_shader_forward_mobile.cpp                                       */
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
#include "core/math/math_defs.h"
#include "scene_shader_forward_mobile.h"
#include "servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"

using namespace RendererSceneRenderImplementation;

bool SceneShaderForwardMobile::ShaderData::is_animated() const
{
	return (uses_fragment_time && uses_discard) || (uses_vertex_time && uses_vertex);
}

bool SceneShaderForwardMobile::ShaderData::casts_shadows() const
{
	bool has_read_screen_alpha = uses_screen_texture || uses_depth_texture || uses_normal_texture;
	bool has_base_alpha =
		(uses_alpha && (!uses_alpha_clip || uses_alpha_antialiasing)) || has_read_screen_alpha;
	bool has_alpha = has_base_alpha || uses_blend_alpha;

	return !has_alpha || (uses_depth_prepass_alpha && !(depth_draw == DEPTH_DRAW_DISABLED ||
														  depth_test != DEPTH_TEST_ENABLED));
}

RenderingServerTypes::ShaderNativeSourceCode
SceneShaderForwardMobile::ShaderData::get_native_source_code() const
{
	if (version.is_valid()) {
		MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
		return SceneShaderForwardMobile::singleton->shader.version_get_native_source_code(version);
	}
	else {
		return RenderingServerTypes::ShaderNativeSourceCode();
	}
}

Pair<ShaderRD*, RID> SceneShaderForwardMobile::ShaderData::get_native_shader_and_version() const
{
	if (version.is_valid()) {
		MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
		return {&SceneShaderForwardMobile::singleton->shader, version};
	}
	else {
		return {};
	}
}

void SceneShaderForwardMobile::ShaderData::_create_pipeline(PipelineKey p_pipeline_key)
{
#if PRINT_PIPELINE_COMPILATION_KEYS
	print_line("HASH:", p_pipeline_key.hash(), "VERSION:", version,
		"VERTEX:", p_pipeline_key.vertex_format_id,
		"FRAMEBUFFER:", p_pipeline_key.framebuffer_format_id, "CULL:", p_pipeline_key.cull_mode,
		"PRIMITIVE:", p_pipeline_key.primitive_type, "VERSION:", p_pipeline_key.version,
		"SPEC PACKED #0:", p_pipeline_key.shader_specialization.packed_0,
		"SPEC PACKED #1:", p_pipeline_key.shader_specialization.packed_1,
		"SPEC PACKED #2:", p_pipeline_key.shader_specialization.packed_2,
		"RENDER PASS:", p_pipeline_key.render_pass, "WIREFRAME:", p_pipeline_key.wireframe);
#endif

	RD::PipelineColorBlendState::Attachment blend_attachment =
		blend_mode_to_blend_attachment(BlendMode(blend_mode));
	RD::PipelineColorBlendState blend_state_blend;
	blend_state_blend.attachments.push_back(blend_attachment);
	RD::PipelineColorBlendState blend_state_opaque =
		RD::PipelineColorBlendState::create_disabled(1);
	RD::PipelineColorBlendState blend_state_opaque_specular =
		RD::PipelineColorBlendState::create_disabled(2);
	RD::PipelineColorBlendState blend_state_depth_normal_roughness =
		RD::PipelineColorBlendState::create_disabled(1);
	RD::PipelineColorBlendState blend_state_depth_normal_roughness_giprobe =
		RD::PipelineColorBlendState::create_disabled(2);

	// update pipelines

	RD::PipelineDepthStencilState depth_stencil_state;

	if (depth_test != DEPTH_TEST_DISABLED) {
		depth_stencil_state.enable_depth_test = true;
		depth_stencil_state.enable_depth_write = depth_draw != DEPTH_DRAW_DISABLED ? true : false;
		depth_stencil_state.depth_compare_operator = RD::COMPARE_OP_GREATER_OR_EQUAL;

		if (depth_test == DEPTH_TEST_ENABLED_INVERTED) {
			depth_stencil_state.depth_compare_operator = RD::COMPARE_OP_LESS;
		}
	}

	RD::RenderPrimitive primitive_rd_table[RSE::PRIMITIVE_MAX] = {
		RD::RENDER_PRIMITIVE_POINTS,
		RD::RENDER_PRIMITIVE_LINES,
		RD::RENDER_PRIMITIVE_LINESTRIPS,
		RD::RENDER_PRIMITIVE_TRIANGLES,
		RD::RENDER_PRIMITIVE_TRIANGLE_STRIPS,
	};

	depth_stencil_state.enable_stencil = stencil_enabled;
	if (stencil_enabled) {
		static const RD::CompareOperator stencil_compare_rd_table[STENCIL_COMPARE_MAX] = {
			RD::COMPARE_OP_LESS,
			RD::COMPARE_OP_EQUAL,
			RD::COMPARE_OP_LESS_OR_EQUAL,
			RD::COMPARE_OP_GREATER,
			RD::COMPARE_OP_NOT_EQUAL,
			RD::COMPARE_OP_GREATER_OR_EQUAL,
			RD::COMPARE_OP_ALWAYS,
		};

		uint32_t stencil_mask = 255;

		RD::PipelineDepthStencilState::StencilOperationState op;
		op.fail = RD::STENCIL_OP_KEEP;
		op.pass = RD::STENCIL_OP_KEEP;
		op.depth_fail = RD::STENCIL_OP_KEEP;
		op.compare = stencil_compare_rd_table[stencil_compare];
		op.compare_mask = 0;
		op.write_mask = 0;
		op.reference = stencil_reference;

		if (stencil_flags & STENCIL_FLAG_READ) {
			op.compare_mask = stencil_mask;
		}

		if (stencil_flags & STENCIL_FLAG_WRITE) {
			op.pass = RD::STENCIL_OP_REPLACE;
			op.write_mask = stencil_mask;
		}

		if (stencil_flags & STENCIL_FLAG_WRITE_DEPTH_FAIL) {
			op.depth_fail = RD::STENCIL_OP_REPLACE;
			op.write_mask = stencil_mask;
		}

		depth_stencil_state.front_op = op;
		depth_stencil_state.back_op = op;
	}

	bool emulate_point_size_flag =
		uses_point_size && SceneShaderForwardMobile::singleton->emulate_point_size;

	RD::RenderPrimitive primitive_rd;
	if (uses_point_size) {
		primitive_rd =
			emulate_point_size_flag ? RD::RENDER_PRIMITIVE_TRIANGLES : RD::RENDER_PRIMITIVE_POINTS;
	}
	else {
		primitive_rd = primitive_rd_table[p_pipeline_key.primitive_type];
	}

	RD::PipelineRasterizationState raster_state;
	raster_state.cull_mode = p_pipeline_key.cull_mode;
	raster_state.wireframe = wireframe || p_pipeline_key.wireframe;

	RD::PipelineMultisampleState multisample_state;
	multisample_state.sample_count = RD::get_singleton()->framebuffer_format_get_texture_samples(
		p_pipeline_key.framebuffer_format_id, 0);

	RD::PipelineColorBlendState blend_state;
	if (uses_alpha || uses_blend_alpha) {
		// These flags should only go through if we have some form of MSAA.
		if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE) {
			multisample_state.enable_alpha_to_coverage = true;
		}
		else if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE) {
			multisample_state.enable_alpha_to_coverage = true;
			multisample_state.enable_alpha_to_one = true;
		}

		if (p_pipeline_key.version == SHADER_VERSION_COLOR_PASS ||
			p_pipeline_key.version == SHADER_VERSION_COLOR_PASS_MULTIVIEW ||
			p_pipeline_key.version == SHADER_VERSION_LIGHTMAP_COLOR_PASS ||
			p_pipeline_key.version == SHADER_VERSION_LIGHTMAP_COLOR_PASS_MULTIVIEW ||
			p_pipeline_key.version == SHADER_VERSION_MOTION_VECTORS_MULTIVIEW) {
			blend_state = blend_state_blend;
			if (depth_draw == DEPTH_DRAW_OPAQUE && !uses_alpha_clip) {
				// Alpha does not write to depth.
				depth_stencil_state.enable_depth_write = false;
			}
		}
		else if (p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS ||
				   p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS_MULTIVIEW ||
				   p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS_DP) {
			// Contains nothing.
		}
		else if (p_pipeline_key.version == SHADER_VERSION_DEPTH_PASS_WITH_MATERIAL) {
			// Writes to normal and roughness in opaque way.
			blend_state = RD::PipelineColorBlendState::create_disabled(5);
		}
		else {
			// Do not use this version (error case).
		}
	}
	else {
		if (p_pipeline_key.version == SHADER_VERSION_COLOR_PASS ||
			p_pipeline_key.version == SHADER_VERSION_COLOR_PASS_MULTIVIEW ||
			p_pipeline_key.version == SHADER_VERSION_LIGHTMAP_COLOR_PASS ||
			p_pipeline_key.version == SHADER_VERSION_LIGHTMAP_COLOR_PASS_MULTIVIEW ||
			p_pipeline_key.version == SHADER_VERSION_MOTION_VECTORS_MULTIVIEW) {
			blend_state = blend_state_opaque;
		}
		else if (p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS ||
				   p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS_MULTIVIEW ||
				   p_pipeline_key.version == SHADER_VERSION_SHADOW_PASS_DP) {
			// Contains nothing.
		}
		else if (p_pipeline_key.version == SHADER_VERSION_DEPTH_PASS_WITH_MATERIAL) {
			// Writes to normal and roughness in opaque way.
			blend_state = RD::PipelineColorBlendState::create_disabled(5);
		}
		else {
			// Unknown pipeline version.
		}
	}

	// Convert the specialization from the key to pipeline specialization constants.
	Vector<RD::PipelineSpecializationConstant> specialization_constants;
	RD::PipelineSpecializationConstant sc;
	sc.constant_id = 0;
	sc.int_value = p_pipeline_key.shader_specialization.packed_0;
	sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
	specialization_constants.push_back(sc);

	sc.constant_id = 1;
	sc.int_value = p_pipeline_key.shader_specialization.packed_1;
	sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
	specialization_constants.push_back(sc);

	sc.constant_id = 2;
	sc.float_value = p_pipeline_key.shader_specialization.packed_2;
	sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT;
	specialization_constants.push_back(sc);

	sc = {}; // Sanitize value bits. "bool_value" only assigns 8 bits and keeps the remaining bits
			 // intact.
	sc.constant_id = 3;
	sc.bool_value = emulate_point_size_flag;
	sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
	specialization_constants.push_back(sc);

	RID shader_rid = get_shader_variant(p_pipeline_key.version, p_pipeline_key.ubershader);
	ERR_FAIL_COND(shader_rid.is_null());

	RID pipeline = RD::get_singleton()->render_pipeline_create(shader_rid,
		p_pipeline_key.framebuffer_format_id, p_pipeline_key.vertex_format_id, primitive_rd,
		raster_state, multisample_state, depth_stencil_state, blend_state, 0,
		p_pipeline_key.render_pass, specialization_constants);

	// Don't print error when it's expected.
	if (unlikely(pipeline.is_null() && RD::get_singleton()
										   ->get_driver_workarounds()
										   .dont_print_on_render_pipeline_creation_failure)) {
		return;
	}

	ERR_FAIL_COND(pipeline.is_null());

	pipeline_hash_map.add_compiled_pipeline(p_pipeline_key.hash(), pipeline);
}

RD::PolygonCullMode SceneShaderForwardMobile::ShaderData::get_cull_mode_from_cull_variant(
	CullVariant p_cull_variant)
{
	const RD::PolygonCullMode cull_mode_rd_table[CULL_VARIANT_MAX][3] = {
		{RD::POLYGON_CULL_DISABLED, RD::POLYGON_CULL_FRONT, RD::POLYGON_CULL_BACK},
		{RD::POLYGON_CULL_DISABLED, RD::POLYGON_CULL_BACK, RD::POLYGON_CULL_FRONT},
		{RD::POLYGON_CULL_DISABLED, RD::POLYGON_CULL_DISABLED, RD::POLYGON_CULL_DISABLED}};

	return cull_mode_rd_table[p_cull_variant][cull_mode];
}

void SceneShaderForwardMobile::ShaderData::_clear_vertex_input_mask_cache()
{
	for (uint32_t i = 0; i < VERTEX_INPUT_MASKS_SIZE; i++) {
		vertex_input_masks[i].store(0);
	}
}

uint64_t SceneShaderForwardMobile::ShaderData::get_vertex_input_mask(
	ShaderVersion p_shader_version, bool p_ubershader)
{
	// Vertex input masks require knowledge of the shader. Since querying the shader can be
	// expensive due to high contention and the necessary mutex, we cache the result instead. It is
	// intentional for the range of the input masks to be different than the versions available in
	// the shaders as it'll only ever use the regular variants or the FP16 ones.
	uint32_t input_mask_index = p_shader_version + (p_ubershader ? SHADER_VERSION_MAX : 0);
	uint64_t input_mask = vertex_input_masks[input_mask_index].load(std::memory_order_relaxed);
	if (input_mask == 0) {
		RID shader_rid = get_shader_variant(p_shader_version, p_ubershader);
		ERR_FAIL_COND_V(shader_rid.is_null(), 0);

		input_mask = RD::get_singleton()->shader_get_vertex_input_attribute_mask(shader_rid);
		vertex_input_masks[input_mask_index].store(input_mask, std::memory_order_relaxed);
	}

	return input_mask;
}

bool SceneShaderForwardMobile::ShaderData::is_valid() const
{
	if (version.is_valid()) {
		MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
		ERR_FAIL_NULL_V(SceneShaderForwardMobile::singleton, false);
		return SceneShaderForwardMobile::singleton->shader.version_is_valid(version);
	}
	else {
		return false;
	}
}

SceneShaderForwardMobile::ShaderData::ShaderData() : shader_list_element(this)
{
	pipeline_hash_map.set_creation_object_and_function(this, &ShaderData::_create_pipeline);
	pipeline_hash_map.set_compilations(SceneShaderForwardMobile::singleton->pipeline_compilations,
		&SceneShaderForwardMobile::singleton_mutex);
}

RendererRD::MaterialStorage::ShaderData* SceneShaderForwardMobile::_create_shader_func()
{
	MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
	ShaderData* shader_data = memnew(ShaderData);
	singleton->shader_list.add(&shader_data->shader_list_element);
	return shader_data;
}

void SceneShaderForwardMobile::MaterialData::set_render_priority(int p_priority)
{
	priority = p_priority - RSE::MATERIAL_RENDER_PRIORITY_MIN; // 8 bits
}

void SceneShaderForwardMobile::MaterialData::set_next_pass(RID p_pass) { next_pass = p_pass; }

SceneShaderForwardMobile::MaterialData::~MaterialData()
{
	free_parameters_uniform_set(uniform_set);
}

RendererRD::MaterialStorage::MaterialData* SceneShaderForwardMobile::_create_material_func(
	ShaderData* p_shader)
{
	MaterialData* material_data = memnew(MaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

/* Scene Shader */

SceneShaderForwardMobile* SceneShaderForwardMobile::singleton = nullptr;
Mutex SceneShaderForwardMobile::singleton_mutex;

SceneShaderForwardMobile::SceneShaderForwardMobile()
{
	// there should be only one of these, contained within our RenderForwardMobile singleton.
	singleton = this;
}

uint32_t SceneShaderForwardMobile::get_pipeline_compilations(RSE::PipelineSource p_source)
{
	MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
	return pipeline_compilations[p_source];
}

void SceneShaderForwardMobile::enable_fp32_shader_group()
{
	shader.enable_group(SHADER_GROUP_FP32);

	if (is_multiview_shader_group_enabled()) {
		enable_multiview_shader_group();
	}
}

void SceneShaderForwardMobile::enable_fp16_shader_group()
{
	shader.enable_group(SHADER_GROUP_FP16);

	if (is_multiview_shader_group_enabled()) {
		enable_multiview_shader_group();
	}
}

void SceneShaderForwardMobile::enable_multiview_shader_group()
{
	if (shader.is_group_enabled(SHADER_GROUP_FP32)) {
		shader.enable_group(SHADER_GROUP_FP32_MULTIVIEW);
	}

	if (shader.is_group_enabled(SHADER_GROUP_FP16)) {
		shader.enable_group(SHADER_GROUP_FP16_MULTIVIEW);
	}
}

bool SceneShaderForwardMobile::is_multiview_shader_group_enabled() const
{
	return shader.is_group_enabled(SHADER_GROUP_FP32_MULTIVIEW) ||
		   shader.is_group_enabled(SHADER_GROUP_FP16_MULTIVIEW);
}

SceneShaderForwardMobile::~SceneShaderForwardMobile()
{
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();

	RD::get_singleton()->free_rid(default_vec4_xform_buffer);
	RD::get_singleton()->free_rid(shadow_sampler);

	material_storage->shader_free(overdraw_material_shader);
	material_storage->shader_free(default_shader);
	material_storage->shader_free(debug_shadow_splits_material_shader);

	material_storage->material_free(overdraw_material);
	material_storage->material_free(default_material);
	material_storage->material_free(debug_shadow_splits_material);
}


