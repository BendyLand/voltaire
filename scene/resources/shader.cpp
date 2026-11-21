/**************************************************************************/
/*  shader.cpp                                                            */
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

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_preprocessor.h"
#include "shader.h"

#ifdef TOOLS_ENABLED
#include "modules/modules_enabled.gen.h" // For regex.
#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif
#endif

Shader::Mode Shader::get_mode() const { return mode; }

void Shader::_check_shader_rid() const
{
	MutexLock lock(shader_rid_mutex);
	if (shader_rid.is_null() && !preprocessed_code.is_empty()) {
		shader_rid = Renderer::shader_create_from_code(
			preprocessed_code, get_path());
		preprocessed_code = String();
	}
}

void Shader::_dependency_changed()
{
	// Preprocess and compile the code again because a dependency has changed. It also calls
	// emit_changed() for us.
	_recompile();
}

void Shader::_recompile() { set_code(get_code()); }

void Shader::set_path(const String& p_path, bool p_take_over)
{
	Resource::set_path(p_path, p_take_over);

	if (shader_rid.is_valid()) {
		Renderer::shader_set_path_hint(shader_rid, p_path);
	}
}

void Shader::set_include_path(const String& p_path)
{
	// Used only if the shader does not have a resource path set,
	// for example during loading stage or when created by code.
	include_path = p_path;
}


void Shader::set_code(const String& p_code)
{
	code = p_code;
}

String Shader::get_code() const
{
	_update_shader();
	return code;
}

RID Shader::get_rid() const
{
	_update_shader();
	_check_shader_rid();

	return shader_rid;
}

void Shader::set_default_texture_parameter(
	const String& p_name, const Ref<Texture>& p_texture, int p_index)
{
	_check_shader_rid();

	if (p_texture.is_valid()) {
		if (!default_textures.has(p_name)) {
			default_textures[p_name] = HashMap<int, Ref<Texture>>();
		}
		default_textures[p_name][p_index] = p_texture;
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, p_texture->get_rid(), p_index);
	}
	else {
		if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
			default_textures[p_name].erase(p_index);

			if (default_textures[p_name].is_empty()) {
				default_textures.erase(p_name);
			}
		}
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, RID(), p_index);
	}

	emit_changed();
}

Ref<Texture> Shader::get_default_texture_parameter(const String& p_name, int p_index) const
{
	if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
		return default_textures[p_name][p_index];
	}
	return Ref<Texture>();
}

void Shader::get_default_texture_parameter_list(List<String>* r_textures) const
{
	for (const KeyValue<String, HashMap<int, Ref<Texture>>>& E : default_textures) {
		r_textures->push_back(E.key);
	}
}

bool Shader::is_text_shader() const { return true; }

void Shader::_update_shader() const
{
	// Base implementation does nothing.
}

Shader::Shader()
{
	// Shader RID will be empty until it is required.
}

Shader::~Shader()
{
	if (shader_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(shader_rid);
	}
}

// --- shader.compat.inc ---

#ifndef DISABLE_DEPRECATED
void Shader::_set_default_texture_parameter_bind_compat_95126(
	const String& p_name, const Ref<Texture2D>& p_texture, int p_index)
{
	set_default_texture_parameter(p_name, p_texture, p_index);
}

Ref<Texture2D> Shader::_get_default_texture_parameter_bind_compat_95126(
	const String& p_name, int p_index) const
{
	return get_default_texture_parameter(p_name, p_index);
}

void Shader::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- shader_include.cpp ---

#include "servers/rendering/shader_preprocessor.h"
#include "shader_include.h"

void ShaderInclude::_dependency_changed() { emit_changed(); }

String ShaderInclude::get_code() const { return code; }

void ShaderInclude::set_include_path(const String& p_path) { include_path = p_path; }

void set_code(const String& p_text) {}

void ShaderInclude::set_code(const String& p_code) {}

// --- shader_resource_format.cpp ---

#include "shader_resource_format.h"

#include "core/io/file_access.h"
#include "scene/resources/shader.h"
#include "scene/resources/shader_include.h"

Ref<Resource> ResourceFormatLoaderShader::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	(void)p_original_path;
	(void)p_use_sub_threads;
	(void)r_progress;
	(void)p_cache_mode;

	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	const bool is_include = p_path.has_extension("gdshaderinc");

	Error error = OK;
	Vector<uint8_t> buffer = FileAccess::get_file_as_bytes(p_path, &error);
	ERR_FAIL_COND_V_MSG(error, nullptr,
		(is_include ? "Cannot load shader include: " : "Cannot load shader: ") + p_path);

	String str;
	if (buffer.size() > 0) {
		error = str.append_utf8((const char*)buffer.ptr(), buffer.size());
		ERR_FAIL_COND_V_MSG(error, nullptr,
			(is_include ? "Cannot parse shader include: " : "Cannot parse shader: ") + p_path);
	}

	if (is_include) {
		Ref<ShaderInclude> shader_inc;
		shader_inc.instantiate();

		shader_inc->set_include_path(p_path);
		shader_inc->set_code(str);

		if (r_error) {
			*r_error = OK;
		}

		return shader_inc;
	}

	Ref<Shader> shader;
	shader.instantiate();

	shader->set_include_path(p_path);
	shader->set_code(str);

	if (r_error) {
		*r_error = OK;
	}

	return shader;
}

void ResourceFormatLoaderShader::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("gdshader");
	p_extensions->push_back("shader");
	p_extensions->push_back("gdshaderinc");
}

void ResourceFormatLoaderShader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}
	if (p_type == "Shader") {
		p_extensions->push_back("gdshader");
		p_extensions->push_back("shader");
	}
	else if (p_type == "ShaderInclude") {
		p_extensions->push_back("gdshaderinc");
	}
}

bool ResourceFormatLoaderShader::handles_type(const String& p_type) const
{
	return (p_type == "Shader" || p_type == "ShaderInclude");
}

String ResourceFormatLoaderShader::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("gdshader") || p_path.has_extension("shader")) {
		return "Shader";
	}
	if (p_path.has_extension("gdshaderinc")) {
		return "ShaderInclude";
	}
	return "";
}

Error ResourceFormatSaverShader::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	(void)p_flags;

	Ref<Shader> shader = p_resource;
	Ref<ShaderInclude> shader_inc = p_resource;
	ERR_FAIL_COND_V(shader.is_null() && shader_inc.is_null(), ERR_INVALID_PARAMETER);

	String source = shader.is_valid() ? shader->get_code() : shader_inc->get_code();

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err, err,
		(shader_inc.is_valid() ? "Cannot save shader include '" : "Cannot save shader '") + p_path +
			"'.");

	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

bool ResourceFormatSaverShader::recognize(const Ref<Resource>& p_resource) const
{
	if (p_resource.is_null()) {
		return false;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid()) {
		return shader->is_text_shader();
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	return shader_inc.is_valid();
}

void ResourceFormatSaverShader::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	if (p_resource.is_null()) {
		return;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid() && shader->is_text_shader()) {
		p_extensions->push_back("gdshader");
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	if (shader_inc.is_valid()) {
		p_extensions->push_back("gdshaderinc");
	}
}
