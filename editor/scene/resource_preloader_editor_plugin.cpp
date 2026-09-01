/**************************************************************************/
/*  resource_preloader_editor_plugin.cpp                                  */
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
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "resource_preloader_editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/tree.h"
#include "scene/main/resource_preloader.h"

void ResourcePreloaderEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (preloader) {
			_update_library();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		load->set_button_icon(get_editor_theme_icon(SNAME("Folder")));
	} break;
	}
}

void ResourcePreloaderEditor::_load_pressed()
{
	loading_scene = false;

	file->clear_filters();
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("", &extensions);
	for (const String& extension : extensions) {
		file->add_filter("*." + extension);
	}

	file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILES);
	file->popup_file_dialog();
}

void ResourcePreloaderEditor::_search_text_changed(const String& p_new_text) const
{
	TreeItem* root = tree->get_root();

	if (root == nullptr) {
		return;
	}

	if (p_new_text.is_empty()) {
		for (TreeItem* child = root->get_first_child(); child; child = child->get_next()) {
			child->set_visible(true);
		}
		return;
	}

	for (TreeItem* child = root->get_first_child(); child; child = child->get_next()) {
		child->set_visible(child->get_text(0).containsn(p_new_text));
	}
}

void ResourcePreloaderEditor::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (p_layout == EditorDock::DOCK_LAYOUT_HORIZONTAL && p_slot != EditorDock::DOCK_SLOT_BOTTOM) {
		mc->set_theme_type_variation("NoBorderHorizontal");
		tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTH);
	}
	else {
		mc->set_theme_type_variation("NoBorderHorizontalBottom");
		tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_TOP);
	}
}

ResourcePreloaderEditorPlugin::ResourcePreloaderEditorPlugin()
{
	preloader_editor = memnew(ResourcePreloaderEditor);
	EditorDockManager::get_singleton()->add_dock(preloader_editor);
	preloader_editor->close();
}


