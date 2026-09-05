/**************************************************************************/
/*  shader_globals_editor.cpp                                             */
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
#include "core/templates/mem_unique_ptr.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_inspector.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/shader_language.h"
#include "shader_globals_editor.h"

static const char* global_var_type_names[RSE::GLOBAL_VAR_TYPE_MAX] = {
	"bool",
	"bvec2",
	"bvec3",
	"bvec4",
	"int",
	"ivec2",
	"ivec3",
	"ivec4",
	"rect2i",
	"uint",
	"uvec2",
	"uvec3",
	"uvec4",
	"float",
	"vec2",
	"vec3",
	"vec4",
	"color",
	"rect2",
	"mat2",
	"mat3",
	"mat4",
	"transform_2d",
	"transform",
	"sampler2D",
	"sampler2DArray",
	"sampler3D",
	"samplerCube",
	"samplerExternalOES",
};

class ShaderGlobalsEditorInterface
{
	void _var_changed() {}

public:
	bool block_update = false;

	ShaderGlobalsEditorInterface() {}
};

String ShaderGlobalsEditor::_check_new_variable_name(const String& p_variable_name)
{
	if (p_variable_name.is_empty()) {
		return TTRC("Name cannot be empty.");
	}

	if (!p_variable_name.is_valid_ascii_identifier()) {
		return TTRC("Name must be a valid identifier.");
	}

	return "";
}

LineEdit* ShaderGlobalsEditor::get_name_box() const { return variable_name; }

void ShaderGlobalsEditor::_variable_name_text_changed(const String& p_variable_name)
{
	const String& warning = _check_new_variable_name(p_variable_name.strip_edges());
	variable_add->set_tooltip_text(warning);
	variable_add->set_disabled(!warning.is_empty());
}

ShaderGlobalsEditor::~ShaderGlobalsEditor() { memdelete(interface); }


