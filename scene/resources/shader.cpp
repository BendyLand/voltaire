/**************************************************************************/
/*  shader.cpp                                                            */
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

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_preprocessor.h"
#include "shader.compat.inc"
#include "shader.h"

#ifdef TOOLS_ENABLED
#include "editor/doc/editor_help.h"
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
		shader_rid = RenderingServer::get_singleton()->shader_create_from_code(
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
		RS::get_singleton()->shader_set_path_hint(shader_rid, p_path);
	}
}

void Shader::set_include_path(const String& p_path)
{
	// Used only if the shader does not have a resource path set,
	// for example during loading stage or when created by code.
	include_path = p_path;
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
	const StringName& p_name, const Ref<Texture>& p_texture, int p_index)
{
	_check_shader_rid();

	if (p_texture.is_valid()) {
		if (!default_textures.has(p_name)) {
			default_textures[p_name] = HashMap<int, Ref<Texture>>();
		}
		default_textures[p_name][p_index] = p_texture;
		RS::get_singleton()->shader_set_default_texture_parameter(
			shader_rid, p_name, p_texture->get_rid(), p_index);
	}
	else {
		if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
			default_textures[p_name].erase(p_index);

			if (default_textures[p_name].is_empty()) {
				default_textures.erase(p_name);
			}
		}
		RS::get_singleton()->shader_set_default_texture_parameter(
			shader_rid, p_name, RID(), p_index);
	}

	emit_changed();
}

Ref<Texture> Shader::get_default_texture_parameter(const StringName& p_name, int p_index) const
{
	if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
		return default_textures[p_name][p_index];
	}
	return Ref<Texture2D>();
}

void Shader::get_default_texture_parameter_list(List<StringName>* r_textures) const
{
	for (const KeyValue<StringName, HashMap<int, Ref<Texture>>>& E : default_textures) {
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
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RenderingServer::get_singleton()->free_rid(shader_rid);
	}
}


