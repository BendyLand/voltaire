/**************************************************************************/
/*  editor_debugger_tree.cpp                                              */
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

#include "core/io/resource_saver.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/settings/editor_settings.h"
#include "editor_debugger_tree.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

EditorDebuggerTree::EditorDebuggerTree()
{
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_allow_rmb_select(true);
	set_select_mode(SELECT_MULTI);

	// Popup
	item_menu = memnew(PopupMenu);
	add_child(item_menu);

	// File Dialog
	file_dialog = memnew(EditorFileDialog);
	add_child(file_dialog);

	accept = memnew(AcceptDialog);
	accept->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	add_child(accept);
}

void EditorDebuggerTree::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		update_icon_max_width();
	} break;
	}
}


void EditorDebuggerTree::_scene_tree_selected()
{
	TreeItem* item = get_selected();
	if (!item) {
		return;
	}

	if (!notify_selection_queued) {
		notify_selection_queued = true;
	}
}

void EditorDebuggerTree::clear_selection()
{
	if (!updating_scene_tree) {
		// Request a tree refresh.
		EditorDebuggerNode::get_singleton()->request_remote_tree();
	}
	// Set the value immediately, so no update flooding happens and causes a crash.
	updating_scene_tree = true;
}

void EditorDebuggerTree::update_icon_max_width()
{
	add_theme_constant_override(
		"icon_max_width", get_theme_constant("class_icon_size", EditorStringName(Editor)));
}

void EditorDebuggerTree::_item_menu_id_pressed(int p_option)
{
	switch (p_option) {
	case ITEM_MENU_SAVE_REMOTE_NODE: {
		file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
		file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);

		List<String> extensions;
		Ref<PackedScene> sd = memnew(PackedScene);
		ResourceSaver::get_recognized_extensions(sd, &extensions);
		file_dialog->clear_filters();
		for (const String& extension : extensions) {
			file_dialog->add_filter("*." + extension, extension.to_upper());
		}

		String filename =
			get_selected_path().get_file() + "." + extensions.front()->get().to_lower();
		file_dialog->set_current_path(filename);
		file_dialog->popup_file_dialog();
	} break;
	case ITEM_MENU_COPY_NODE_PATH: {
		String text = get_selected_path();
		if (text.is_empty()) {
			return;
		}
		// Keep full remote path but strip the "/root" prefix for user-facing copy.
		if (text == "/root") {
			text = ".";
		}
		else if (text.begins_with("/root/")) {
			text = text.substr(String("/root/").length());
		}
		DisplayServer::get_singleton()->clipboard_set(text);
	} break;
	case ITEM_MENU_EXPAND_COLLAPSE: {
		TreeItem* s_item = get_selected();

		if (!s_item) {
			s_item = get_root();
			if (!s_item) {
				break;
			}
		}

		bool collapsed = s_item->is_any_collapsed();
		s_item->set_collapsed_recursive(!collapsed);

		ensure_cursor_is_visible();
	}
	}
}


