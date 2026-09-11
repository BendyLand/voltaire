/**************************************************************************/
/*  navigation_region_3d_editor_plugin.cpp                                */
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
#include "editor/editor_string_names.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "navigation_region_3d_editor_plugin.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_3d/navigation_server_3d.h"

void NavigationRegion3DEditor::_on_navmesh_multibake_canceled()
{
	if (bake_in_process) {
		region_baking_canceled = true;
		return;
	}

	multibake_dialog->set_visible(false);
	regions_to_bake.clear();
	regions_with_navmesh_to_bake.clear();
	processed_regions_to_bake_count = 0;
	processed_regions_to_bake_count_max = 0;
	region_baking_canceled = false;
	currently_baking_region = nullptr;
	bake_in_process = false;
}

void NavigationRegion3DEditor::_clear_pressed()
{
	button_bake->set_pressed(false);
	bake_info->set_text("");

	if (!selected_regions.is_empty()) {
		for (NavigationRegion3D* region : selected_regions) {
			if (region->get_navigation_mesh().is_valid()) {
				region->get_navigation_mesh()->clear();
				region->update_gizmos();
			}
		}
	}
}

void NavigationRegion3DEditor::edit(LocalVector<NavigationRegion3D*> p_regions)
{
	if (p_regions.is_empty()) {
		return;
	}

	selected_regions = p_regions;
}

NavigationRegion3DEditorPlugin::NavigationRegion3DEditorPlugin()
{
	navigation_region_editor = memnew(NavigationRegion3DEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(navigation_region_editor);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, navigation_region_editor->bake_hbox);
	navigation_region_editor->hide();
	navigation_region_editor->bake_hbox->hide();

	gizmo_plugin.instantiate();
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}


