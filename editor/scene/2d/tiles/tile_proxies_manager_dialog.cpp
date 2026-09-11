/**************************************************************************/
/*  tile_proxies_manager_dialog.cpp                                       */
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

#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_properties_vector.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "tile_proxies_manager_dialog.h"

void TileProxiesManagerDialog::_menu_id_pressed(int p_id)
{
	if (p_id == 0) {
		// Delete.
		_delete_selected_bindings();
	}
}

void TileProxiesManagerDialog::_update_enabled_property_editors()
{
	if (from.source_id == TileSet::INVALID_SOURCE) {
		from.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		to.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
		from.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		to.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		coords_from_property_editor->hide();
		coords_to_property_editor->hide();
		alternative_from_property_editor->hide();
		alternative_to_property_editor->hide();
	}
	else if (from.get_atlas_coords().x == -1 || from.get_atlas_coords().y == -1) {
		from.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		to.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
		coords_from_property_editor->show();
		coords_to_property_editor->show();
		alternative_from_property_editor->hide();
		alternative_to_property_editor->hide();
	}
	else {
		coords_from_property_editor->show();
		coords_to_property_editor->show();
		alternative_from_property_editor->show();
		alternative_to_property_editor->show();
	}

	source_from_property_editor->update_property();
	source_to_property_editor->update_property();
	coords_from_property_editor->update_property();
	coords_to_property_editor->update_property();
	alternative_from_property_editor->update_property();
	alternative_to_property_editor->update_property();
}

void TileProxiesManagerDialog::cancel_pressed()
{
	EditorUndoRedoManager* undo_redo = EditorUndoRedoManager::get_singleton();
	for (int i = 0; i < committed_actions_count; i++) {
		undo_redo->undo();
	}
	committed_actions_count = 0;
}

void TileProxiesManagerDialog::_bind_methods() {}

void TileProxiesManagerDialog::update_tile_set(Ref<TileSet> p_tile_set)
{
	ERR_FAIL_COND(p_tile_set.is_null());
	tile_set = p_tile_set;
	committed_actions_count = 0;
	_update_lists();
}


