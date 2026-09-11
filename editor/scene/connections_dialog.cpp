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
#include "core/templates/mem_unique_ptr.h"
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


void ConnectDialog::_open_method_popup()
{
	method_popup->popup_centered();
	method_search->clear();
	method_search->grab_focus();
}

/*
 * Enables or disables the connect button. The connect button is enabled if a
 * node is selected and valid in the selected mode.
 */
void ConnectDialog::_update_ok_enabled()
{
	Node* target = tree->get_selected();

	if (target == nullptr) {
		get_ok_button()->set_disabled(true);
		return;
	}

	if (dst_method->get_text().is_empty()) {
		get_ok_button()->set_disabled(true);
		return;
	}

	get_ok_button()->set_disabled(false);
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

/*
 * Initialize ConnectDialog and populate fields with expected data.
 * If creating a connection from scratch, sensible defaults are used.
 * If editing an existing connection, previous data is retained.
 */

ConnectDialog::~ConnectDialog() {}

//////////////////////////////////////////

Control* ConnectionsDockTree::make_custom_tooltip(const String& p_text) const
{
	// If it's not a doc tooltip, fallback to the default one.
	if (p_text.is_empty() || p_text.contains(" :: ")) {
		return nullptr;
	}

	return EditorHelpBitTooltip::make_tooltip(const_cast<ConnectionsDockTree*>(this), p_text);
}

void ConnectionsDock::_filter_changed(const String& p_text) { update_tree(); }

void ConnectionsDock::_tree_item_activated()
{ // "Activation" on double-click.
	TreeItem* item = tree->get_selected();
	if (!item) {
		return;
	}

	if (_get_item_type(*item) == TREE_ITEM_TYPE_SIGNAL) {
		_open_connection_dialog(*item);
	}
	else if (_get_item_type(*item) == TREE_ITEM_TYPE_CONNECTION) {
		_go_to_method(*item);
	}
}

ConnectionsDock::TreeItemType ConnectionsDock::_get_item_type(const TreeItem& p_item) const
{
	if (&p_item == tree->get_root()) {
		return TREE_ITEM_TYPE_ROOT;
	}
	else if (p_item.get_parent() == tree->get_root()) {
		return TREE_ITEM_TYPE_CLASS;
	}
	else if (p_item.get_parent()->get_parent() == tree->get_root()) {
		return TREE_ITEM_TYPE_SIGNAL;
	}
	else {
		return TREE_ITEM_TYPE_CONNECTION;
	}
}

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

void ConnectionsDock::_class_menu_about_to_popup()
{
	class_menu->set_item_disabled(
		class_menu->get_item_index(CLASS_MENU_OPEN_DOCS), class_menu_doc_class_name.is_empty());
}

void ConnectionsDock::_close() { hide(); }

void ConnectionsDock::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		search_box->set_right_icon(get_editor_theme_icon(SNAME("Search")));

		class_menu->set_item_icon(
			class_menu->get_item_index(CLASS_MENU_OPEN_DOCS), get_editor_theme_icon(SNAME("Help")));

		signal_menu->set_item_icon(signal_menu->get_item_index(SIGNAL_MENU_CONNECT),
			get_editor_theme_icon(SNAME("Instance")));
		signal_menu->set_item_icon(signal_menu->get_item_index(SIGNAL_MENU_DISCONNECT_ALL),
			get_editor_theme_icon(SNAME("Unlinked")));
		signal_menu->set_item_icon(signal_menu->get_item_index(SIGNAL_MENU_COPY_NAME),
			get_editor_theme_icon(SNAME("ActionCopy")));
		signal_menu->set_item_icon(signal_menu->get_item_index(SIGNAL_MENU_OPEN_DOCS),
			get_editor_theme_icon(SNAME("Help")));

		slot_menu->set_item_icon(
			slot_menu->get_item_index(SLOT_MENU_EDIT), get_editor_theme_icon(SNAME("Edit")));
		slot_menu->set_item_icon(slot_menu->get_item_index(SLOT_MENU_GO_TO_METHOD),
			get_editor_theme_icon(SNAME("ArrowRight")));
		slot_menu->set_item_icon(slot_menu->get_item_index(SLOT_MENU_DISCONNECT),
			get_editor_theme_icon(SNAME("Unlinked")));

		tree->add_theme_constant_override("icon_max_width",
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));

		update_tree();
	} break;

	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editors")) {
			update_tree();
		}
	} break;
	}
}


