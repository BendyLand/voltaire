/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "core/object/class_db.h"
#include "register_types.h"
#include "visual_shader.h"
#include "vs_nodes/visual_shader_nodes.h"
#include "vs_nodes/visual_shader_particle_nodes.h"
#include "vs_nodes/visual_shader_sdf_nodes.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/visual_shader_editor_plugin.h"
#include "editor/visual_shader_language_plugin.h"

static void _editor_init()
{
	Ref<EditorInspectorVisualShaderModePlugin> visual_shader_mode_plugin;
	visual_shader_mode_plugin.instantiate();
	EditorInspector::add_inspector_plugin(visual_shader_mode_plugin);

	Ref<VisualShaderConversionPlugin> visual_shader_convert;
	visual_shader_convert.instantiate();
	EditorNode::get_singleton()->add_resource_conversion_plugin(visual_shader_convert);

	Ref<VisualShaderLanguagePlugin> visual_shader_lang;
	visual_shader_lang.instantiate();
	EditorShaderLanguagePlugin::register_shader_language(visual_shader_lang);
}
#endif // TOOLS_ENABLED

void initialize_visual_shader_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifdef TOOLS_ENABLED
	}
	else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorNode::add_init_callback(_editor_init);
#endif // TOOLS_ENABLED
	}
}

void uninitialize_visual_shader_module(ModuleInitializationLevel p_level) {}


