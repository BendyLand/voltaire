/**************************************************************************/
/*  material_storage.cpp                                                  */
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

#include "core/config/project_settings.h"
#include "servers/rendering/dummy/storage/material_storage.h"

void RendererDummy::MaterialStorage::global_shader_parameter_remove(const String& p_name)
{
	Data* d = _get_data();
	if (!d->global_shader_variables.has(p_name)) {
		return;
	}

	d->global_shader_variables.erase(p_name);
}

Vector<String> RendererDummy::MaterialStorage::global_shader_parameter_get_list()
{
	Data* d = _get_data();
	Vector<String> names;
	for (const KeyValue<String, RSE::GlobalShaderParameterType>& E : d->global_shader_variables) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

RID RendererDummy::MaterialStorage::shader_allocate()
{
	return _get_data()->shader_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	_get_data()->shader_owner.initialize_rid(p_rid, DummyShader());
}

void RendererDummy::MaterialStorage::shader_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyShader* shader = data->shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	data->shader_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	Data* d = _get_data();
	DummyShader* shader = d->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);
	if (p_code.is_empty()) {
		return;
	}

	String mode_string = ShaderLanguage::get_shader_type(p_code);

	RSE::ShaderMode new_mode;
	if (mode_string == "canvas_item") {
		new_mode = RSE::SHADER_CANVAS_ITEM;
	}
	else if (mode_string == "particles") {
		new_mode = RSE::SHADER_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_mode = RSE::SHADER_SPATIAL;
	}
	else if (mode_string == "sky") {
		new_mode = RSE::SHADER_SKY;
	}
	else if (mode_string == "fog") {
		new_mode = RSE::SHADER_FOG;
	}
	else if (mode_string == "texture_blit") {
		new_mode = RSE::SHADER_TEXTURE_BLIT;
	}
	else {
		new_mode = RSE::SHADER_MAX;
		ERR_FAIL_MSG("Shader type " + mode_string + " not supported in Dummy renderer.");
	}
	ShaderCompiler::IdentifierActions actions;
	actions.uniforms = &shader->uniforms;
	ShaderCompiler::GeneratedCode gen_code;

	Error err = d->dummy_compiler->compile(new_mode, p_code, &actions, "", gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");
}

RID RendererDummy::MaterialStorage::material_allocate()
{
	return _get_data()->material_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::material_initialize(RID p_rid)
{
	_get_data()->material_owner.initialize_rid(p_rid, DummyMaterial());
}

void RendererDummy::MaterialStorage::material_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyMaterial* material = data->material_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(material);

	data->material_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->shader = p_shader;
}

void RendererDummy::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->next_pass = p_next_material;
}

RSE::GlobalShaderParameterType RendererDummy::MaterialStorage::global_shader_parameter_get_type(
	const String& p_name)
{
	return RSE::GLOBAL_VAR_TYPE_MAX;
}

void RendererDummy::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void RendererDummy::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
}

void RendererDummy::MaterialStorage::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->dummy_compiler) {
		data->dummy_compiler = memnew(ShaderCompiler);
		ShaderCompiler::DefaultIdentifierActions actions;
		data->dummy_compiler->initialize(actions);
	}
}

void RendererDummy::MaterialStorage::finalize()
{
	if (!data) {
		return;
	}
	if (data->dummy_compiler) {
		memdelete(data->dummy_compiler);
		data->dummy_compiler = nullptr;
	}
	memdelete(data);
	data = nullptr;
}


