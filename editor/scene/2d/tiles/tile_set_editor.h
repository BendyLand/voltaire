/**************************************************************************/
/*  tile_set_editor.h                                                     */
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

#pragma once

#include "editor/docks/editor_dock.h"
#include "editor/scene/2d/tiles/atlas_merging_dialog.h"
#include "editor/scene/2d/tiles/tile_proxies_manager_dialog.h"
#include "editor/scene/2d/tiles/tile_set_atlas_source_editor.h"
#include "editor/scene/2d/tiles/tile_set_scenes_collection_source_editor.h"
#include "scene/gui/tab_bar.h"
#include "scene/resources/2d/tile_set.h"

class AcceptDialog;
class SpinBox;
class HBoxContainer;
class SplitContainer;
class EditorFileDialog;
class EditorInspectorPlugin;
class TileSetSourceItemList;

class TileSetEditor : public EditorDock
{
	static TileSetEditor* singleton;

private:
	bool read_only = false;

	Ref<TileSet> tile_set;
	bool tile_set_changed_needs_update = false;
	HSplitContainer* split_container = nullptr;

	HBoxContainer* tile_set_toolbar = nullptr;
	TabBar* tabs_bar = nullptr;

	Label* no_source_selected_label = nullptr;
	TileSetAtlasSourceEditor* tile_set_atlas_source_editor = nullptr;
	TileSetScenesCollectionSourceEditor* tile_set_scenes_collection_source_editor = nullptr;

	Button* sources_delete_button = nullptr;
	MenuButton* sources_add_button = nullptr;
	MenuButton* source_sort_button = nullptr;
	MenuButton* sources_advanced_menu_button = nullptr;
	TileSetSourceItemList* sources_list = nullptr;
	Ref<Texture2D> missing_texture_texture;
	void _sources_advanced_menu_id_pressed(int p_id_pressed);

	EditorFileDialog* texture_file_dialog = nullptr;
	AtlasMergingDialog* atlas_merging_dialog = nullptr;
	TileProxiesManagerDialog* tile_proxies_manager_dialog = nullptr;

	bool first_edit = true;

	MarginContainer* patterns_mc = nullptr;
	ItemList* patterns_item_list = nullptr;
	Label* patterns_help_label = nullptr;

	PanelContainer* expanded_area = nullptr;
	Control* expanded_editor = nullptr;
	LocalVector<SplitContainer*> disable_on_expand;

	void _tile_set_changed();

protected:
	virtual void update_layout(EditorDock::DockLayout p_layout, int p_slot) override;

public:
	void register_split(SplitContainer* p_split);

	TileSetEditor() = default;
};

class TileSourceInspectorPlugin : public EditorInspectorPlugin
{
	AcceptDialog* id_edit_dialog = nullptr;
	Label* id_label = nullptr;
	SpinBox* id_input = nullptr;
};


