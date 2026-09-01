/**************************************************************************/
/*  editor_scene_tabs.cpp                                                 */
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

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_scene_tabs.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/texture_rect.h"

void EditorSceneTabs::_scene_tab_exit() { tab_preview_panel->hide(); }

void EditorSceneTabs::_scene_tab_input(const Ref<InputEvent>& p_input)
{
	Ref<InputEventMouseButton> mb = p_input;

	if (mb.is_valid()) {
		int tab_idx = scene_tabs->get_tab_idx_at_point(mb->get_position());
		if (tab_idx < 0 && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
			int tab_buttons = 0;
			if (scene_tabs->get_offset_buttons_visible()) {
				tab_buttons = get_theme_icon(SNAME("increment"), SNAME("TabBar"))->get_width() +
							  get_theme_icon(SNAME("decrement"), SNAME("TabBar"))->get_width();
			}

			if ((is_layout_rtl() && mb->get_position().x > tab_buttons) ||
				(!is_layout_rtl() &&
					mb->get_position().x < scene_tabs->get_size().width - tab_buttons)) {
				EditorNode::get_singleton()->trigger_menu_option(EditorNode::SCENE_NEW_SCENE, true);
			}
		}
		else if (mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
			// Context menu.
			_update_context_menu(tab_idx);

			scene_tabs_context_menu->set_position(
				scene_tabs->get_screen_position() + mb->get_position());
			scene_tabs_context_menu->reset_size();
			scene_tabs_context_menu->popup();
		}
	}
}

void EditorSceneTabs::unhandled_key_input(const Ref<InputEvent>& p_event)
{
	if (!tab_preview_panel->is_visible()) {
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		tab_preview_panel->hide();
	}
}

void EditorSceneTabs::_reposition_active_tab(int p_to_index)
{
	EditorNode::get_editor_data().move_edited_scene_to_index(p_to_index);
	update_scene_tabs();
}

int EditorSceneTabs::get_option_tab() const
{
	return last_hovered_tab >= 0 ? last_hovered_tab : scene_tabs->get_current_tab();
}

void EditorSceneTabs::_update_scene_list()
{
	PopupMenu* popup = scene_list->get_popup();
	popup->clear();

	for (int i = 0; i < scene_tabs->get_tab_count(); i++) {
		popup->add_item(scene_tabs->get_tab_title(i), i);
		popup->set_item_icon(i, scene_tabs->get_tab_icon(i));
	}
}

void EditorSceneTabs::_scene_tabs_resized()
{
	const Size2 add_button_size = Size2(scene_tab_add->get_size().x, scene_tabs->get_size().y);
	if (scene_tabs->get_offset_buttons_visible()) {
		// Move the add button to a fixed position.
		if (scene_tab_add->get_parent() == scene_tabs) {
			scene_tabs->remove_child(scene_tab_add);
			scene_tab_add_ph->add_child(scene_tab_add);
			scene_tab_add->set_rect(Rect2(Point2(), add_button_size));
		}
	}
	else {
		// Move the add button to be after the last tab.
		if (scene_tab_add->get_parent() == scene_tab_add_ph) {
			scene_tab_add_ph->remove_child(scene_tab_add);
			scene_tabs->add_child(scene_tab_add);
		}

		if (scene_tabs->get_tab_count() == 0) {
			scene_tab_add->set_rect(Rect2(Point2(), add_button_size));
			return;
		}

		Rect2 last_tab = scene_tabs->get_tab_rect(scene_tabs->get_tab_count() - 1);
		int hsep = scene_tabs->get_theme_constant(SNAME("h_separation"));
		if (scene_tabs->is_layout_rtl()) {
			scene_tab_add->set_rect(
				Rect2(Point2(last_tab.position.x - add_button_size.x - hsep, last_tab.position.y),
					add_button_size));
		}
		else {
			scene_tab_add->set_rect(
				Rect2(Point2(last_tab.position.x + last_tab.size.width + hsep, last_tab.position.y),
					add_button_size));
		}
	}
}

void EditorSceneTabs::_tab_preview_done(const String& p_path, const Ref<Texture2D>& p_preview,
	const Ref<Texture2D>& p_small_preview, int p_tab)
{
	if (p_preview.is_valid()) {
		tab_preview->set_texture(p_preview);

		Rect2 rect = scene_tabs->get_tab_rect(p_tab);
		rect.position += scene_tabs->get_global_position();
		tab_preview_panel->set_global_position(rect.position + Vector2(0, rect.size.height));
		tab_preview_panel->show();
	}
}

void EditorSceneTabs::add_extra_button(Button* p_button) { tabbar_container->add_child(p_button); }

void EditorSceneTabs::set_current_tab(int p_tab) { scene_tabs->set_current_tab(p_tab); }

int EditorSceneTabs::get_current_tab() const { return scene_tabs->get_current_tab(); }

void EditorSceneTabs::_project_settings_changed()
{
	if (ProjectSettings::get_singleton()->check_changed_settings_in_group(
			"application/run/main_scene")) {
		update_scene_tabs();
	}
}


