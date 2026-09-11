/**************************************************************************/
/*  tile_set_scenes_collection_source_editor.cpp                          */
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
#include "editor/inspector/editor_resource_preview.h"
#include "editor/scene/2d/tiles/tile_set_editor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/split_container.h"
#include "tile_set_scenes_collection_source_editor.h"

int TileSetScenesCollectionSourceEditor::TileSetScenesCollectionProxyObject::get_id()
{
	return source_id;
}

void TileSetScenesCollectionSourceEditor::_tile_set_scenes_collection_source_changed()
{
	tile_set_scenes_collection_source_changed_needs_update = true;
}

void TileSetScenesCollectionSourceEditor::_scene_thumbnail_done(const String& p_path,
	const Ref<Texture2D>& p_preview, const Ref<Texture2D>& p_small_preview, int p_idx)
{
	if (p_idx >= 0 && p_idx < scene_tiles_list->get_item_count()) {
		scene_tiles_list->set_item_icon(p_idx, p_preview);
	}
}

void TileSetScenesCollectionSourceEditor::_update_action_buttons()
{
	Vector<int> selected_indices = scene_tiles_list->get_selected_items();
	scene_tile_delete_button->set_disabled(selected_indices.is_empty() || read_only);
}

void TileSetScenesCollectionSourceEditor::_update_all()
{
	_update_scenes_list();
	_update_action_buttons();
	_update_tile_inspector();
}

void TileSetScenesCollectionSourceEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		scenes_collection_source_inspector->add_custom_property_description(
			"TileSetScenesCollectionProxyObject", "id",
			TTRC("The tile's unique identifier within this TileSet. Each tile stores its source "
				 "ID, so changing one may make tiles invalid."));
		scenes_collection_source_inspector->add_custom_property_description(
			"TileSetScenesCollectionProxyObject", "name",
			TTRC("The human-readable name for the scene collection. Use a descriptive name here "
				 "for organizational purposes (such as \"obstacles\", \"decoration\", etc.)."));

		tile_inspector->add_custom_property_description("SceneTileProxyObject", "id",
			TTRC("ID of the scene tile in the collection. Each painted tile has associated ID, so "
				 "changing this property may cause your TileMaps to not display properly."));
		tile_inspector->add_custom_property_description("SceneTileProxyObject", "scene",
			TTRC("Absolute path to the scene associated with this tile."));
		tile_inspector->add_custom_property_description("SceneTileProxyObject",
			"display_placeholder",
			TTRC("If [code]true[/code], a placeholder marker will be displayed on top of the "
				 "scene's preview. The marker is displayed anyway if the scene has no valid "
				 "preview."));
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		_update_scenes_list();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		scene_tile_add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		scene_tile_delete_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
		_update_scenes_list();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (tile_set_scenes_collection_source_changed_needs_update) {
			read_only = false;
			// Add the listener again and check for read-only status.
			if (tile_set.is_valid()) {
				read_only = EditorNode::get_singleton()->is_resource_read_only(tile_set);
			}

			// Update everything.
			_update_source_inspector();
			_update_scenes_list();
			_update_action_buttons();
			_update_tile_inspector();
			tile_set_scenes_collection_source_changed_needs_update = false;
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		// Update things just in case.
		_update_scenes_list();
		_update_action_buttons();
	} break;
	}
}


