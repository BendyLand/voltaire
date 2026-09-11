/**************************************************************************/
/*  tile_set_editor.cpp                                                   */
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
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/scene/2d/tiles/tile_data_editors.h"
#include "editor/scene/2d/tiles/tiles_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "tile_set_editor.h"

TileSetEditor* TileSetEditor::singleton = nullptr;

void TileSetEditor::_sources_advanced_menu_id_pressed(int p_id_pressed)
{
	ERR_FAIL_COND(tile_set.is_null());

	switch (p_id_pressed) {
	case 0: {
		atlas_merging_dialog->update_tile_set(tile_set);
		atlas_merging_dialog->popup_centered_ratio(0.5);
	} break;
	case 1: {
		tile_proxies_manager_dialog->update_tile_set(tile_set);
		tile_proxies_manager_dialog->popup_centered_ratio(0.5);
	} break;
	}
}

void TileSetEditor::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (p_slot != EditorDock::DOCK_SLOT_BOTTOM) {
		patterns_mc->set_theme_type_variation("NoBorderHorizontalBottom");
		patterns_item_list->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_TOP);
	}
	else {
		patterns_mc->set_theme_type_variation("NoBorderHorizontal");
		patterns_item_list->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_BOTH);
	}
}

void TileSetEditor::_tile_set_changed() { tile_set_changed_needs_update = true; }

void TileSetEditor::_tab_changed(int p_tab_changed)
{
	split_container->set_visible(p_tab_changed == 0);
	patterns_mc->set_visible(p_tab_changed == 1);
}

void TileSetEditor::register_split(SplitContainer* p_split)
{
	disable_on_expand.push_back(p_split);
}


