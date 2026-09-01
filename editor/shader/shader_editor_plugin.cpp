/**************************************************************************/
/*  shader_editor_plugin.cpp                                              */
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

#include "core/io/resource_loader.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/window_wrapper.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_create_dialog.h"
#include "editor/shader/text_shader_editor.h"
#include "editor/shader/text_shader_language_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/item_list.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "servers/display/display_server.h"
#include "shader_editor_plugin.h"

Ref<Resource> ShaderEditorPlugin::_get_current_shader()
{
	int index = shader_tabs->get_current_tab();
	ERR_FAIL_INDEX_V(index, shader_tabs->get_tab_count(), Ref<Resource>());
	if (edited_shaders[index].shader.is_valid()) {
		return edited_shaders[index].shader;
	}
	else {
		return edited_shaders[index].shader_inc;
	}
}

void ShaderEditorPlugin::_move_shader_tab(int p_from, int p_to)
{
	if (p_from == p_to) {
		return;
	}
	EditedShader es = edited_shaders[p_from];
	edited_shaders.remove_at(p_from);
	edited_shaders.insert(p_to, es);
	shader_tabs->move_child(shader_tabs->get_tab_control(p_from), p_to);
	_update_shader_list();
}

ShaderEditor* ShaderEditorPlugin::get_shader_editor(const Ref<Shader>& p_for_shader)
{
	for (EditedShader& edited_shader : edited_shaders) {
		if (edited_shader.shader == p_for_shader) {
			return edited_shader.shader_editor;
		}
	}
	return nullptr;
}

String ShaderEditorPlugin::get_unsaved_status(const String& p_for_scene) const
{
	// TODO: This should also include visual shaders and shader includes, but save_external_data()
	// doesn't seem to save them...
	PackedStringArray unsaved_shaders;
	for (uint32_t i = 0; i < edited_shaders.size(); i++) {
		if (edited_shaders[i].shader_editor) {
			if (edited_shaders[i].shader_editor->is_unsaved()) {
				if (unsaved_shaders.is_empty()) {
					unsaved_shaders.append(
						TTR("Save changes to the following shaders(s) before quitting?"));
				}
				unsaved_shaders.append(edited_shaders[i].name.trim_suffix("(*)"));
			}
		}
	}

	if (!p_for_scene.is_empty()) {
		PackedStringArray unsaved_built_in_shaders;

		const String scene_file = p_for_scene.get_file();
		for (const String& E : unsaved_shaders) {
			if (!E.is_resource_file() && E.contains(scene_file)) {
				if (unsaved_built_in_shaders.is_empty()) {
					unsaved_built_in_shaders.append(
						TTR("There are unsaved changes in the following built-in shaders(s):"));
				}
				unsaved_built_in_shaders.append(E);
			}
		}

		if (!unsaved_built_in_shaders.is_empty()) {
			return String("\n").join(unsaved_built_in_shaders);
		}
		return String();
	}

	return String("\n").join(unsaved_shaders);
}

void ShaderEditorPlugin::save_external_data()
{
	for (EditedShader& edited_shader : edited_shaders) {
		if (edited_shader.shader_editor && edited_shader.shader_editor->is_unsaved()) {
			edited_shader.shader_editor->save_external_data();
		}
	}
	_update_shader_list();
}

void ShaderEditorPlugin::apply_changes()
{
	for (EditedShader& edited_shader : edited_shaders) {
		if (edited_shader.shader_editor) {
			edited_shader.shader_editor->apply_shaders();
		}
	}
}

void ShaderEditorPlugin::_shader_list_clicked(
	int p_item, Vector2 p_local_mouse_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index == MouseButton::MIDDLE) {
		_close_shader(p_item);
	}
	if (p_mouse_button_index == MouseButton::RIGHT) {
		_make_script_list_context_menu();
	}
}

void ShaderEditorPlugin::_setup_popup_menu(PopupMenuType p_type, PopupMenu* p_menu)
{
	if (p_type == FILE) {
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/new"), FILE_MENU_NEW);
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/new_include"), FILE_MENU_NEW_INCLUDE);
		p_menu->add_separator();
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/open"), FILE_MENU_OPEN);
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/open_include"), FILE_MENU_OPEN_INCLUDE);
	}

	if (p_type == FILE || p_type == CONTEXT_VALID_ITEM) {
		p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/save"), FILE_MENU_SAVE);
		p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/save_as"), FILE_MENU_SAVE_AS);
	}

	if (p_type == FILE) {
		p_menu->add_separator();
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/open_in_inspector"), FILE_MENU_INSPECT);
		p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/inspect_native_code"),
			FILE_MENU_INSPECT_NATIVE_SHADER_CODE);
		p_menu->add_separator();
		p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_file"), FILE_MENU_CLOSE);
		p_menu->add_separator();
		p_menu->add_shortcut(
			ED_GET_SHORTCUT("script_editor/toggle_files_panel"), FILE_MENU_TOGGLE_FILES_PANEL);
	}
	else {
		p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_file"), FILE_MENU_CLOSE);
		p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/close_all"), FILE_MENU_CLOSE_ALL);
		p_menu->add_shortcut(
			ED_GET_SHORTCUT("script_editor/close_other_tabs"), FILE_MENU_CLOSE_OTHER_TABS);
		if (p_type == CONTEXT_VALID_ITEM) {
			p_menu->add_separator();
			p_menu->add_shortcut(ED_GET_SHORTCUT("shader_editor/copy_path"), FILE_MENU_COPY_PATH);
			p_menu->add_shortcut(ED_GET_SHORTCUT("script_editor/show_in_file_system"),
				FILE_MENU_SHOW_IN_FILE_SYSTEM);
		}
	}
}

void ShaderEditorPlugin::_close_builtin_shaders_from_scene(const String& p_scene)
{
	for (uint32_t i = 0; i < edited_shaders.size();) {
		Ref<Shader>& shader = edited_shaders[i].shader;
		if (shader.is_valid()) {
			if (shader->is_built_in() && shader->get_path().begins_with(p_scene)) {
				_close_shader(i);
				continue;
			}
		}
		Ref<ShaderInclude>& include = edited_shaders[i].shader_inc;
		if (include.is_valid()) {
			if (include->is_built_in() && include->get_path().begins_with(p_scene)) {
				_close_shader(i);
				continue;
			}
		}
		i++;
	}
}

void ShaderEditorPlugin::_set_text_shader_zoom_factor(float p_zoom_factor)
{
	if (text_shader_zoom_factor == p_zoom_factor) {
		return;
	}

	text_shader_zoom_factor = p_zoom_factor;
}

void ShaderEditorPlugin::_update_shader_editor_zoom_factor(CodeTextEditor* p_shader_editor) const
{
	if (p_shader_editor && p_shader_editor->is_visible_in_tree() &&
		text_shader_zoom_factor != p_shader_editor->get_zoom_factor()) {
		p_shader_editor->set_zoom_factor(text_shader_zoom_factor);
	}
}

void ShaderEditorPlugin::_switch_to_editor(ShaderEditor* p_editor)
{
	ERR_FAIL_NULL(p_editor);
	if (file_menu->get_parent() != nullptr) {
		file_menu->get_parent()->remove_child(file_menu);
	}

	shader_tabs->show();
	p_editor->use_menu_bar(file_menu);
	file_menu->set_v_size_flags(Control::SIZE_EXPAND_FILL);
}

void ShaderEditorPlugin::_file_removed(const String& p_removed_file)
{
	for (uint32_t i = 0; i < edited_shaders.size(); i++) {
		if (edited_shaders[i].path == p_removed_file) {
			_close_shader(i);
			break;
		}
	}
}

void ShaderEditorPlugin::_set_file_specific_items_disabled(bool p_disabled)
{
	PopupMenu* file_popup_menu = file_menu->get_popup();
	file_popup_menu->set_item_disabled(file_popup_menu->get_item_index(FILE_MENU_SAVE), p_disabled);
	file_popup_menu->set_item_disabled(
		file_popup_menu->get_item_index(FILE_MENU_SAVE_AS), p_disabled);
	file_popup_menu->set_item_disabled(
		file_popup_menu->get_item_index(FILE_MENU_INSPECT), p_disabled);
	file_popup_menu->set_item_disabled(
		file_popup_menu->get_item_index(FILE_MENU_INSPECT_NATIVE_SHADER_CODE), p_disabled);
	file_popup_menu->set_item_disabled(
		file_popup_menu->get_item_index(FILE_MENU_CLOSE), p_disabled);
}

ShaderEditorPlugin::~ShaderEditorPlugin()
{
	EditorShaderLanguagePlugin::clear_registered_shader_languages();
	memdelete(file_menu);
}


