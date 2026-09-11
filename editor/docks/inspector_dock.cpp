/**************************************************************************/
/*  inspector_dock.cpp                                                    */
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
#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_object_selector.h"
#include "editor/gui/editor_toaster.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "inspector_dock.h"
#include "scene/gui/box_container.h"

void InspectorDock::_prepare_menu()
{
	PopupMenu* menu = object_menu->get_popup();
	for (int i = EditorPropertyNameProcessor::STYLE_RAW;
		 i <= EditorPropertyNameProcessor::STYLE_LOCALIZED; i++) {
		menu->set_item_checked(
			menu->get_item_index(PROPERTY_NAME_STYLE_RAW + i), i == property_name_style);
	}
}

void InspectorDock::_menu_option(int p_option) { _menu_option_confirm(p_option, false); }

void InspectorDock::_menu_confirm_current() { _menu_option_confirm(current_option, true); }

void InspectorDock::_new_resource() { new_resource_dialog->popup_create(true); }

void InspectorDock::_unref_resource()
{
	Ref<Resource> current_res = _get_current_resource();
	ERR_FAIL_COND(current_res.is_null());
	current_res->set_path("");
	EditorNode::get_singleton()->edit_current();
}

void InspectorDock::_copy_resource()
{
	Ref<Resource> current_res = _get_current_resource();
	ERR_FAIL_COND(current_res.is_null());
	EditorSettings::get_singleton()->set_resource_clipboard(current_res);
}

void InspectorDock::_prepare_resource_extra_popup()
{
	Ref<Resource> r = EditorSettings::get_singleton()->get_resource_clipboard();
	PopupMenu* popup = resource_extra_button->get_popup();
	popup->set_item_disabled(popup->get_item_index(RESOURCE_EDIT_CLIPBOARD), r.is_null());

	Ref<Resource> current_res = _get_current_resource();
	popup->set_item_disabled(popup->get_item_index(RESOURCE_SHOW_IN_FILESYSTEM),
		current_res.is_null() || current_res->is_built_in());
}

void InspectorDock::_menu_collapseall() { inspector->collapse_all_folding(); }

void InspectorDock::_menu_expandall() { inspector->expand_all_folding(); }

void InspectorDock::_menu_expand_revertable() { inspector->expand_revertable(); }

void InspectorDock::_info_pressed() { info_dialog->popup_centered(); }

Container* InspectorDock::get_addon_area() { return this; }

void InspectorDock::_bind_methods() {}

void InspectorDock::edit_resource(const Ref<Resource>& p_resource)
{
	_resource_selected(p_resource, "");
}

void InspectorDock::open_resource(const String& p_type) { _load_resource(p_type); }

void InspectorDock::set_info(
	const String& p_button_text, const String& p_message, bool p_is_warning)
{
	info->hide();
	info_is_warning = p_is_warning;

	if (info_is_warning) {
		info->set_button_icon(get_editor_theme_icon(SNAME("NodeWarning")));
		info->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
	}
	else {
		info->set_button_icon(get_editor_theme_icon(SNAME("NodeInfo")));
		info->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SceneStringName(font_color), EditorStringName(Editor)));
	}

	if (!p_button_text.is_empty() && !p_message.is_empty()) {
		info->show();
		info->set_text(p_button_text);
		info_dialog->set_text(p_message);
	}
}

void InspectorDock::clear() {}

void InspectorDock::go_back() { _edit_back(); }

EditorPropertyNameProcessor::Style InspectorDock::get_property_name_style() const
{
	return property_name_style;
}

InspectorDock::~InspectorDock() { singleton = nullptr; }


