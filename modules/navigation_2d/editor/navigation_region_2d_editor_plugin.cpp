/**************************************************************************/
/*  navigation_region_2d_editor_plugin.cpp                                */
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

#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/settings/editor_settings.h"
#include "navigation_region_2d_editor_plugin.h"
#include "scene/2d/navigation/navigation_region_2d.h"
#include "scene/gui/dialogs.h"
#include "scene/main/timer.h"

Ref<NavigationPolygon> NavigationRegion2DEditor::_ensure_navpoly() const
{
	Ref<NavigationPolygon> navpoly = node->get_navigation_polygon();
	if (navpoly.is_null()) {
		navpoly.instantiate();
		node->set_navigation_polygon(navpoly);
	}
	return navpoly;
}

Node2D* NavigationRegion2DEditor::_get_node() const { return node; }

int NavigationRegion2DEditor::_get_polygon_count() const
{
	Ref<NavigationPolygon> navpoly = node->get_navigation_polygon();
	if (navpoly.is_valid()) {
		return navpoly->get_outline_count();
	}
	else {
		return 0;
	}
}

bool NavigationRegion2DEditor::_has_resource() const
{
	return node && node->get_navigation_polygon().is_valid();
}

void NavigationRegion2DEditor::_bake_pressed()
{
	if (rebake_timer) {
		rebake_timer->stop();
	}
	button_bake->set_pressed(false);

	ERR_FAIL_NULL(node);
	Ref<NavigationPolygon> navigation_polygon = node->get_navigation_polygon();
	if (navigation_polygon.is_null()) {
		err_dialog->set_text(
			TTR("A NavigationPolygon resource must be set or created for this node to work."));
		err_dialog->popup_centered();
		return;
	}

	node->bake_navigation_polygon(true);

	node->queue_redraw();
}

void NavigationRegion2DEditor::_clear_pressed()
{
	if (rebake_timer) {
		rebake_timer->stop();
	}
	if (node) {
		if (node->get_navigation_polygon().is_valid()) {
			node->get_navigation_polygon()->clear();
			// Needed to update all the region internals.
			node->set_navigation_polygon(node->get_navigation_polygon());
		}
	}

	button_bake->set_pressed(false);
	bake_info->set_text("");

	if (node) {
		node->queue_redraw()
;
	}
}

void NavigationRegion2DEditor::_update_polygon_editing_state()
{
	if (!_get_node()) {
		return;
	}

	if (node != nullptr && node->get_navigation_polygon().is_valid()) {
		bake_hbox->show();
	}
	else {
		bake_hbox->hide();
	}
}

void NavigationRegion2DEditor::_rebake_timer_timeout()
{
	if (!node) {
		return;
	}
	Ref<NavigationPolygon> navigation_polygon = node->get_navigation_polygon();
	if (navigation_polygon.is_null()) {
		return;
	}

	node->bake_navigation_polygon(true);
	node->queue_redraw();
}

NavigationRegion2DEditorPlugin::NavigationRegion2DEditorPlugin()
	: AbstractPolygon2DEditorPlugin(memnew(NavigationRegion2DEditor), "NavigationRegion2D")
{
}


