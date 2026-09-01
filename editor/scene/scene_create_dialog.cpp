/**************************************************************************/
/*  scene_create_dialog.cpp                                               */
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

#include "core/io/dir_access.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/node_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/resources/packed_scene.h"
#include "scene_create_dialog.h"

void SceneCreateDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		select_node_button->set_button_icon(get_editor_theme_icon(SNAME("ClassList")));
		node_type_2d->set_button_icon(get_editor_theme_icon(SNAME("Node2D")));
		node_type_3d->set_button_icon(get_editor_theme_icon(SNAME("Node3D")));
		node_type_gui->set_button_icon(get_editor_theme_icon(SNAME("Control")));
		node_type_other->add_theme_icon_override(
			SNAME("icon"), get_editor_theme_icon(SNAME("Node")).ptr());
	} break;

	case NOTIFICATION_READY: {
		select_node_dialog->select_base();
	} break;
	}
}

void SceneCreateDialog::browse_types()
{
	select_node_dialog->popup_create(true);
	select_node_dialog->set_title(TTR("Pick Root Node Type"));
	select_node_dialog->set_ok_button_text(TTR("Pick"));
}

void SceneCreateDialog::on_type_picked()
{
	other_type_display->set_text(select_node_dialog->get_selected_type());
	if (node_type_other->is_pressed()) {
		validation_panel->update();
	}
	else {
		node_type_other->set_pressed(true); // Calls validation_panel->update() via group.
	}
}

String SceneCreateDialog::get_scene_path() const { return scene_name; }

String SceneCreateDialog::get_root_name() const { return root_name; }


