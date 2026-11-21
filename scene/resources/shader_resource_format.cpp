/**************************************************************************/
/*  shader_resource_format.cpp                                            */
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
