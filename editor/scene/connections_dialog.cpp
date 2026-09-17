/**************************************************************************/
/*  connections_dialog.cpp                                                */
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

#include "connections_dialog.h"
#include "core/config/project_settings.h"
#include "core/templates/hash_set.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_variant_type_selectors.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/spin_box.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"

void ConnectDialog::_cancel_pressed() { hide(); }

void ConnectDialog::_item_activated()
{
	_ok_pressed(); // From AcceptDialog.
}

ConnectDialog::ConnectionData ConnectDialog::get_source_connection_data() const
{
	return source_connection_data;
}

StringName ConnectDialog::get_signal_name() const { return signal; }

PackedStringArray ConnectDialog::get_signal_args() const { return signal_args; }

NodePath ConnectDialog::get_dst_path() const { return dst_path; }

void ConnectDialog::set_dst_node(Node* p_node) { tree->set_selected(p_node); }

StringName ConnectDialog::get_dst_method_name() const
{
	String txt = dst_method->get_text();
	if (txt.contains_char('(')) {
		txt = txt.left(txt.find_char('(')).strip_edges();
	}
	return txt;
}

void ConnectDialog::set_dst_method(const StringName& p_method) { dst_method->set_text(p_method); }

int ConnectDialog::get_unbinds() const { return int(unbind_count->get_value()); }

bool ConnectDialog::get_deferred() const { return deferred->is_pressed(); }

bool ConnectDialog::get_one_shot() const { return one_shot->is_pressed(); }

bool ConnectDialog::get_append_source() const
{
	return !append_source->is_disabled() && append_source->is_pressed();
}

/*
 * Returns true if ConnectDialog is being used to edit an existing connection.
 */
bool ConnectDialog::is_editing() const { return edit_mode; }

ConnectDialog::~ConnectDialog() {}

void ConnectionsDock::_filter_changed(const String& p_text) { update_tree(); }

void ConnectionsDock::_handle_class_menu_option(int p_option)
{
	switch (p_option) {
	case CLASS_MENU_OPEN_DOCS:
		ScriptEditor::get_singleton()->goto_help("class:" + class_menu_doc_class_name);
		EditorNode::get_singleton()->get_editor_main_screen()->select(
			EditorMainScreen::EDITOR_SCRIPT);
		break;
	}
}

void ConnectionsDock::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editors")) {
			update_tree();
		}
	} break;
	}
}



void ConnectionsDock::update_tree() {}

ConnectionsDock::ConnectionsDock() {}

void ConnectDialog::ok_pressed() {}


