/**************************************************************************/
/*  script_language_extension.cpp                                         */
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

#include "script_language_extension.h"

#include "core/object/class_db.h"

ScriptLanguageExtension::ScriptLanguageExtension() {
#ifdef TOOLS_ENABLED
	editor_adapter = memnew(EditorAdapter(this));
#endif // TOOLS_ENABLED
}

ScriptLanguageExtension::~ScriptLanguageExtension() {
#ifdef TOOLS_ENABLED
	memdelete(editor_adapter);
#endif // TOOLS_ENABLED
}

void ScriptExtension::_bind_methods() {
	VLTRVIRTUAL_BIND(_editor_can_reload_from_file);
	VLTRVIRTUAL_BIND(_placeholder_erased, "placeholder");

	VLTRVIRTUAL_BIND(_can_instantiate);
	VLTRVIRTUAL_BIND(_get_base_script);
	VLTRVIRTUAL_BIND(_get_global_name);
	VLTRVIRTUAL_BIND(_inherits_script, "script");

	VLTRVIRTUAL_BIND(_get_instance_base_type);
	VLTRVIRTUAL_BIND(_instance_create, "for_object");
	VLTRVIRTUAL_BIND(_placeholder_instance_create, "for_object");

	VLTRVIRTUAL_BIND(_has_source_code);
	VLTRVIRTUAL_BIND(_get_source_code);

	VLTRVIRTUAL_BIND(_set_source_code, "code");
	VLTRVIRTUAL_BIND(_reload, "keep_state");

	VLTRVIRTUAL_BIND(_get_doc_class_name);
	VLTRVIRTUAL_BIND(_get_documentation);
	VLTRVIRTUAL_BIND(_get_class_icon_path);

	VLTRVIRTUAL_BIND(_has_method, "method");
	VLTRVIRTUAL_BIND(_has_static_method, "method");

	VLTRVIRTUAL_BIND(_get_script_method_argument_count, "method");

	VLTRVIRTUAL_BIND(_get_method_info, "method");

	VLTRVIRTUAL_BIND(_is_tool);
	VLTRVIRTUAL_BIND(_is_valid);
	VLTRVIRTUAL_BIND(_is_abstract);
	VLTRVIRTUAL_BIND(_get_language);

	VLTRVIRTUAL_BIND(_has_script_signal, "signal");
	VLTRVIRTUAL_BIND(_get_script_signal_list);

	VLTRVIRTUAL_BIND(_has_property_default_value, "property");
	VLTRVIRTUAL_BIND(_get_property_default_value, "property");

	VLTRVIRTUAL_BIND(_update_exports);
	VLTRVIRTUAL_BIND(_get_script_method_list);
	VLTRVIRTUAL_BIND(_get_script_property_list);

	VLTRVIRTUAL_BIND(_get_member_line, "member");

	VLTRVIRTUAL_BIND(_get_constants);
	VLTRVIRTUAL_BIND(_get_members);
	VLTRVIRTUAL_BIND(_is_placeholder_fallback_enabled);

	VLTRVIRTUAL_BIND(_get_rpc_config);

#ifndef DISABLE_DEPRECATED
	VLTRVIRTUAL_BIND(_instance_has, "object");
#endif // !DISABLE_DEPRECATED
}

void ScriptLanguageExtension::_bind_methods() {
	VLTRVIRTUAL_BIND(_get_name);
	VLTRVIRTUAL_BIND(_init);
	VLTRVIRTUAL_BIND(_get_type);
	VLTRVIRTUAL_BIND(_get_extension);
	VLTRVIRTUAL_BIND(_finish);

	VLTRVIRTUAL_BIND(_get_reserved_words);
	VLTRVIRTUAL_BIND(_is_control_flow_keyword, "keyword");
	VLTRVIRTUAL_BIND(_get_comment_delimiters);
	VLTRVIRTUAL_BIND(_get_doc_comment_delimiters);
	VLTRVIRTUAL_BIND(_get_string_delimiters);
	VLTRVIRTUAL_BIND(_make_template, "template", "class_name", "base_class_name");
	VLTRVIRTUAL_BIND(_get_built_in_templates, "object");
	VLTRVIRTUAL_BIND(_is_using_templates);
	VLTRVIRTUAL_BIND(_validate, "script", "path", "validate_functions", "validate_errors", "validate_warnings", "validate_safe_lines");

	VLTRVIRTUAL_BIND(_validate_path, "path");
#ifndef DISABLE_DEPRECATED
	VLTRVIRTUAL_BIND(_create_script);
	VLTRVIRTUAL_BIND(_has_named_classes);
#endif
	VLTRVIRTUAL_BIND(_supports_builtin_mode);
	VLTRVIRTUAL_BIND(_supports_documentation);
	VLTRVIRTUAL_BIND(_can_inherit_from_file);
	VLTRVIRTUAL_BIND(_find_function, "function", "code");
	VLTRVIRTUAL_BIND(_make_function, "class_name", "function_name", "function_args");
	VLTRVIRTUAL_BIND(_can_make_function);
	VLTRVIRTUAL_BIND(_open_in_external_editor, "script", "line", "column");
	VLTRVIRTUAL_BIND(_overrides_external_editor);
	VLTRVIRTUAL_BIND(_preferred_file_name_casing);

	VLTRVIRTUAL_BIND(_complete_code, "code", "path", "owner");
	VLTRVIRTUAL_BIND(_lookup_code, "code", "symbol", "path", "owner");
	VLTRVIRTUAL_BIND(_auto_indent_code, "code", "from_line", "to_line");

	VLTRVIRTUAL_BIND(_add_global_constant, "name", "value");
	VLTRVIRTUAL_BIND(_add_named_global_constant, "name", "value");
	VLTRVIRTUAL_BIND(_remove_named_global_constant, "name");

	VLTRVIRTUAL_BIND(_thread_enter);
	VLTRVIRTUAL_BIND(_thread_exit);
	VLTRVIRTUAL_BIND(_debug_get_error);
	VLTRVIRTUAL_BIND(_debug_get_stack_level_count);

	VLTRVIRTUAL_BIND(_debug_get_stack_level_line, "level");
	VLTRVIRTUAL_BIND(_debug_get_stack_level_function, "level");
	VLTRVIRTUAL_BIND(_debug_get_stack_level_source, "level");
	VLTRVIRTUAL_BIND(_debug_get_stack_level_locals, "level", "max_subitems", "max_depth");
	VLTRVIRTUAL_BIND(_debug_get_stack_level_members, "level", "max_subitems", "max_depth");
	VLTRVIRTUAL_BIND(_debug_get_stack_level_instance, "level");
	VLTRVIRTUAL_BIND(_debug_get_globals, "max_subitems", "max_depth");
	VLTRVIRTUAL_BIND(_debug_parse_stack_level_expression, "level", "expression", "max_subitems", "max_depth");

	VLTRVIRTUAL_BIND(_debug_get_current_stack_info);

	VLTRVIRTUAL_BIND(_reload_all_scripts);
	VLTRVIRTUAL_BIND(_reload_scripts, "scripts", "soft_reload");
	VLTRVIRTUAL_BIND(_reload_tool_script, "script", "soft_reload");

	VLTRVIRTUAL_BIND(_get_recognized_extensions);
	VLTRVIRTUAL_BIND(_get_public_functions);
	VLTRVIRTUAL_BIND(_get_public_constants);
	VLTRVIRTUAL_BIND(_get_public_annotations);

	VLTRVIRTUAL_BIND(_profiling_start);
	VLTRVIRTUAL_BIND(_profiling_stop);
	VLTRVIRTUAL_BIND(_profiling_set_save_native_calls, "enable");

	VLTRVIRTUAL_BIND(_profiling_get_accumulated_data, "info_array", "info_max");
	VLTRVIRTUAL_BIND(_profiling_get_frame_data, "info_array", "info_max");

	VLTRVIRTUAL_BIND(_frame);

	VLTRVIRTUAL_BIND(_handles_global_class_type, "type");
	VLTRVIRTUAL_BIND(_get_global_class_name, "path");

	BIND_ENUM_CONSTANT(LOOKUP_RESULT_SCRIPT_LOCATION);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_CONSTANT);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_PROPERTY);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_METHOD);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_SIGNAL);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_ENUM);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_TBD_GLOBALSCOPE); // Deprecated.
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_CLASS_ANNOTATION);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_LOCAL_CONSTANT);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_LOCAL_VARIABLE);
	BIND_ENUM_CONSTANT(LOOKUP_RESULT_MAX);

	BIND_ENUM_CONSTANT(LOCATION_LOCAL);
	BIND_ENUM_CONSTANT(LOCATION_PARENT_MASK);
	BIND_ENUM_CONSTANT(LOCATION_OTHER_USER_CODE);
	BIND_ENUM_CONSTANT(LOCATION_OTHER);

	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_CLASS);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_FUNCTION);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_SIGNAL);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_VARIABLE);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_MEMBER);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_ENUM);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_CONSTANT);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_NODE_PATH);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_FILE_PATH);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_PLAIN_TEXT);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_KEYWORD);
	BIND_ENUM_CONSTANT(CODE_COMPLETION_KIND_MAX);
}
