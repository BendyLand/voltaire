/**************************************************************************/
/*  theme_editor_plugin.cpp                                               */
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
#include "editor/doc/editor_help.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/gui/progress_dialog.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/check_button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/item_list.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/theme/theme_db.h"
#include "theme_editor_plugin.h"

void ThemeItemImportTree::_toggle_type_items(bool p_collapse)
{
	TreeItem* root = import_items_tree->get_root();
	if (!root) {
		return;
	}

	TreeItem* type_node = root->get_first_child();
	while (type_node) {
		type_node->set_collapsed(p_collapse);
		type_node = type_node->get_next();
	}
}

void ThemeItemImportTree::_filter_text_changed(const String& p_value) { _update_items_tree(); }

void ThemeItemImportTree::_tree_item_edited()
{
	if (updating_tree) {
		return;
	}

	TreeItem* edited_item = import_items_tree->get_edited();
	if (!edited_item) {
		return;
	}

	updating_tree = true;

	int edited_column = import_items_tree->get_edited_column();
	bool is_checked = edited_item->is_checked(edited_column);
	if (is_checked) {
		if (edited_column == IMPORT_ITEM_DATA) {
			edited_item->set_checked(IMPORT_ITEM, true);
			edited_item->propagate_check(IMPORT_ITEM);
		}
	}
	else {
		if (edited_column == IMPORT_ITEM) {
			edited_item->set_checked(IMPORT_ITEM_DATA, false);
			edited_item->propagate_check(IMPORT_ITEM_DATA);
		}
	}
	edited_item->propagate_check(edited_column);
	updating_tree = false;
}

void ThemeItemImportTree::_select_all_subitems(TreeItem* p_root_item, bool p_select_with_data)
{
	TreeItem* child_item = p_root_item->get_first_child();
	while (child_item) {
		child_item->set_checked(IMPORT_ITEM, true);
		if (p_select_with_data) {
			child_item->set_checked(IMPORT_ITEM_DATA, true);
		}
		_store_selected_item(child_item);

		_select_all_subitems(child_item, p_select_with_data);
		child_item = child_item->get_next();
	}
}

void ThemeItemImportTree::_deselect_all_subitems(TreeItem* p_root_item, bool p_deselect_completely)
{
	TreeItem* child_item = p_root_item->get_first_child();
	while (child_item) {
		child_item->set_checked(IMPORT_ITEM_DATA, false);
		if (p_deselect_completely) {
			child_item->set_checked(IMPORT_ITEM, false);
		}
		_store_selected_item(child_item);

		_deselect_all_subitems(child_item, p_deselect_completely);
		child_item = child_item->get_next();
	}
}

void ThemeItemImportTree::_select_all_items_pressed()
{
	if (updating_tree) {
		return;
	}

	updating_tree = true;

	TreeItem* root = import_items_tree->get_root();
	_select_all_subitems(root, false);

	updating_tree = false;
}

void ThemeItemImportTree::_select_full_items_pressed()
{
	if (updating_tree) {
		return;
	}

	updating_tree = true;

	TreeItem* root = import_items_tree->get_root();
	_select_all_subitems(root, true);

	updating_tree = false;
}

void ThemeItemImportTree::_deselect_all_items_pressed()
{
	if (updating_tree) {
		return;
	}

	updating_tree = true;

	TreeItem* root = import_items_tree->get_root();
	_deselect_all_subitems(root, true);

	updating_tree = false;
}

void ThemeItemImportTree::_select_all_data_type_pressed(int p_data_type)
{
	ERR_FAIL_INDEX_MSG(p_data_type, Theme::DATA_TYPE_MAX, "Theme item data type is out of bounds.");

	if (updating_tree) {
		return;
	}

	Theme::DataType data_type = (Theme::DataType)p_data_type;
	List<TreeItem*>* item_list = nullptr;

	switch (data_type) {
	case Theme::DATA_TYPE_COLOR:
		item_list = &tree_color_items;
		break;

	case Theme::DATA_TYPE_CONSTANT:
		item_list = &tree_constant_items;
		break;

	case Theme::DATA_TYPE_FONT:
		item_list = &tree_font_items;
		break;

	case Theme::DATA_TYPE_FONT_SIZE:
		item_list = &tree_font_size_items;
		break;

	case Theme::DATA_TYPE_ICON:
		item_list = &tree_icon_items;
		break;

	case Theme::DATA_TYPE_STYLEBOX:
		item_list = &tree_stylebox_items;
		break;

	case Theme::DATA_TYPE_MAX:
		return; // Can't happen, but silences warning.
	}

	updating_tree = true;

	for (List<TreeItem*>::Element* E = item_list->front(); E; E = E->next()) {
		TreeItem* child_item = E->get();
		if (!child_item) {
			continue;
		}

		child_item->set_checked(IMPORT_ITEM, true);
		child_item->propagate_check(IMPORT_ITEM, false);
		_store_selected_item(child_item);
	}

	updating_tree = false;
}

void ThemeItemImportTree::_select_full_data_type_pressed(int p_data_type)
{
	ERR_FAIL_INDEX_MSG(p_data_type, Theme::DATA_TYPE_MAX, "Theme item data type is out of bounds.");

	if (updating_tree) {
		return;
	}

	Theme::DataType data_type = (Theme::DataType)p_data_type;
	List<TreeItem*>* item_list = nullptr;

	switch (data_type) {
	case Theme::DATA_TYPE_COLOR:
		item_list = &tree_color_items;
		break;

	case Theme::DATA_TYPE_CONSTANT:
		item_list = &tree_constant_items;
		break;

	case Theme::DATA_TYPE_FONT:
		item_list = &tree_font_items;
		break;

	case Theme::DATA_TYPE_FONT_SIZE:
		item_list = &tree_font_size_items;
		break;

	case Theme::DATA_TYPE_ICON:
		item_list = &tree_icon_items;
		break;

	case Theme::DATA_TYPE_STYLEBOX:
		item_list = &tree_stylebox_items;
		break;

	case Theme::DATA_TYPE_MAX:
		return; // Can't happen, but silences warning.
	}

	updating_tree = true;

	for (List<TreeItem*>::Element* E = item_list->front(); E; E = E->next()) {
		TreeItem* child_item = E->get();
		if (!child_item) {
			continue;
		}

		child_item->set_checked(IMPORT_ITEM, true);
		child_item->set_checked(IMPORT_ITEM_DATA, true);
		child_item->propagate_check(IMPORT_ITEM, false);
		child_item->propagate_check(IMPORT_ITEM_DATA, false);
		_store_selected_item(child_item);
	}

	updating_tree = false;
}

void ThemeItemImportTree::_deselect_all_data_type_pressed(int p_data_type)
{
	ERR_FAIL_INDEX_MSG(p_data_type, Theme::DATA_TYPE_MAX, "Theme item data type is out of bounds.");

	if (updating_tree) {
		return;
	}

	Theme::DataType data_type = (Theme::DataType)p_data_type;
	List<TreeItem*>* item_list = nullptr;

	switch (data_type) {
	case Theme::DATA_TYPE_COLOR:
		item_list = &tree_color_items;
		break;

	case Theme::DATA_TYPE_CONSTANT:
		item_list = &tree_constant_items;
		break;

	case Theme::DATA_TYPE_FONT:
		item_list = &tree_font_items;
		break;

	case Theme::DATA_TYPE_FONT_SIZE:
		item_list = &tree_font_size_items;
		break;

	case Theme::DATA_TYPE_ICON:
		item_list = &tree_icon_items;
		break;

	case Theme::DATA_TYPE_STYLEBOX:
		item_list = &tree_stylebox_items;
		break;

	case Theme::DATA_TYPE_MAX:
		return; // Can't happen, but silences warning.
	}

	updating_tree = true;

	for (List<TreeItem*>::Element* E = item_list->front(); E; E = E->next()) {
		TreeItem* child_item = E->get();
		if (!child_item) {
			continue;
		}

		child_item->set_checked(IMPORT_ITEM, false);
		child_item->set_checked(IMPORT_ITEM_DATA, false);
		child_item->propagate_check(IMPORT_ITEM, false);
		child_item->propagate_check(IMPORT_ITEM_DATA, false);
		_store_selected_item(child_item);
	}

	updating_tree = false;
}

void ThemeItemImportTree::set_edited_theme(const Ref<Theme>& p_theme) { edited_theme = p_theme; }

void ThemeItemImportTree::set_base_theme(const Ref<Theme>& p_theme) { base_theme = p_theme; }

bool ThemeItemImportTree::has_selected_items() const { return (selected_items.size() > 0); }

void ThemeItemEditorDialog::_close_dialog() { hide(); }

void ThemeItemEditorDialog::_edited_type_selected()
{
	TreeItem* selected_item = edit_type_list->get_selected();
	String selected_type = selected_item->get_text(0);
	_update_edit_item_tree(selected_type);
}

void ThemeItemEditorDialog::_edit_theme_item_gui_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		if (!k->is_pressed()) {
			return;
		}

		if (k->is_action_pressed(SNAME("ui_text_submit"), false, true)) {
			_confirm_edit_theme_item();
			edit_theme_item_dialog->hide();
			edit_theme_item_dialog->set_input_as_handled();
		}
		else if (k->is_action_pressed(SNAME("ui_cancel"), false, true)) {
			edit_theme_item_dialog->hide();
			edit_theme_item_dialog->set_input_as_handled();
		}
	}
}

void ThemeItemEditorDialog::_open_select_another_theme()
{
	import_another_theme_dialog->popup_file_dialog();
}

void ThemeItemEditorDialog::set_edited_theme(const Ref<Theme>& p_theme) { edited_theme = p_theme; }

void ThemeTypeDialog::_add_type_options_cbk(int p_index)
{
	add_type_filter->set_text(add_type_options->get_item_text(p_index));
	add_type_filter->set_caret_column(add_type_filter->get_text().length());
}

void ThemeTypeDialog::set_edited_theme(const Ref<Theme>& p_theme) { edited_theme = p_theme; }

void ThemeTypeDialog::set_include_own_types(bool p_enable) { include_own_types = p_enable; }

HashMap<StringName, bool> ThemeTypeEditor::_get_type_items(
	String p_type_name, Theme::DataType p_type, bool p_include_default)
{
	HashMap<StringName, bool> items;
	List<StringName> names;

	if (p_include_default) {
		names.clear();
		String default_type;

		{
			const StringName variation_base = edited_theme->get_type_variation_base(p_type_name);
			if (variation_base != StringName()) {
				default_type = variation_base;
			}
		}

		if (default_type.is_empty()) {
			// If variation base was not found in the edited theme, look in the default theme.
			const StringName variation_base =
				ThemeDB::get_singleton()->get_default_theme()->get_type_variation_base(p_type_name);
			if (variation_base != StringName()) {
				default_type = variation_base;
			}
		}

		if (default_type.is_empty()) {
			default_type = p_type_name;
		}

		List<ThemeDB::ThemeItemBind> theme_binds;
		ThemeDB::get_singleton()->get_class_items(default_type, &theme_binds, true, p_type);
		for (const ThemeDB::ThemeItemBind& E : theme_binds) {
			names.push_back(E.item_name);
		}

		names.sort_custom<StringName::AlphCompare>();
		for (const StringName& E : names) {
			items[E] = false;
		}
	}

	{
		names.clear();
		edited_theme->get_theme_item_list(p_type, p_type_name, &names);
		names.sort_custom<StringName::AlphCompare>();
		for (const StringName& E : names) {
			items[E] = true;
		}
	}

	List<StringName> keys;
	for (const KeyValue<StringName, bool>& E : items) {
		keys.push_back(E.key);
	}
	keys.sort_custom<StringName::AlphCompare>();

	HashMap<StringName, bool> ordered_items;
	for (const StringName& E : keys) {
		ordered_items[E] = items[E];
	}

	return ordered_items;
}

void ThemeTypeEditor::_add_focusable(Control* p_control) { focusables.append(p_control); }

void ThemeTypeEditor::_list_type_selected(int p_index)
{
	edited_type = theme_type_list->get_item_text(p_index);
	_update_type_items();
}

void ThemeTypeEditor::_item_add_lineedit_cbk(String p_value, int p_data_type, Control* p_control)
{
	_item_add_cbk(p_data_type, p_control);
}

void ThemeTypeEditor::_item_rename_entered(
	String p_value, int p_data_type, String p_item_name, Control* p_control)
{
	_item_rename_confirmed(p_data_type, p_item_name, p_control);
}

void ThemeTypeEditor::_add_type_dialog_selected(const String p_type_name)
{
	if (add_type_mode == ADD_THEME_TYPE) {
		select_type(p_type_name);
	}
	else if (add_type_mode == ADD_VARIATION_BASE) {
		_type_variation_changed(p_type_name);
	}
}

void ThemeTypeEditor::select_type(String p_type_name)
{
	edited_type = p_type_name;
	bool type_exists = false;

	for (int i = 0; i < theme_type_list->get_item_count(); i++) {
		String type_name = theme_type_list->get_item_text(i);
		if (type_name == edited_type) {
			theme_type_list->select(i);
			type_exists = true;
			break;
		}
	}

	if (type_exists) {
		_update_type_items();
	}
	else {
		edited_theme->add_icon_type(edited_type);
		edited_theme->add_stylebox_type(edited_type);
		edited_theme->add_font_type(edited_type);
		edited_theme->add_font_size_type(edited_type);
		edited_theme->add_color_type(edited_type);
		edited_theme->add_constant_type(edited_type);

		_update_type_list();
	}
}

bool ThemeTypeEditor::is_stylebox_pinned(Ref<StyleBox> p_stylebox)
{
	return leading_stylebox.pinned && leading_stylebox.stylebox == p_stylebox;
}

Ref<Theme> ThemeEditor::get_edited_theme() { return theme; }

void ThemeEditor::_theme_edit_button_cbk()
{
	theme_edit_dialog->popup_centered_clamped(Size2(850, 700) * EDSCALE, 0.8);
}

void ThemeEditor::_add_preview_button_cbk() { preview_scene_dialog->popup_file_dialog(); }

void ThemeEditor::_remove_preview_tab_invalid(Node* p_tab_control)
{
	int tab_index = p_tab_control->get_index();
	_remove_preview_tab(tab_index);
}

void ThemeEditor::_preview_control_picked(String p_class_name)
{
	theme_type_editor->select_type(p_class_name);
}

void ThemeEditor::_preview_tabs_resized()
{
	const Size2 add_button_size =
		Size2(add_preview_button->get_size().x, preview_tabs->get_size().y);
	if (preview_tabs->get_offset_buttons_visible()) {
		// Move the add button to a fixed position.
		if (add_preview_button->get_parent() == preview_tabs) {
			add_preview_button->reparent(add_preview_button_ph);
			add_preview_button->set_rect(Rect2(Point2(), add_button_size));
		}
	}
	else {
		// Move the add button to be after the last tab.
		if (add_preview_button->get_parent() == add_preview_button_ph) {
			add_preview_button->reparent(preview_tabs);
		}

		Rect2 last_tab = preview_tabs->get_tab_rect(preview_tabs->get_tab_count() - 1);
		int hsep = preview_tabs->get_theme_constant(SNAME("h_separation"));
		if (preview_tabs->is_layout_rtl()) {
			add_preview_button->set_rect(
				Rect2(Point2(last_tab.position.x - add_button_size.x - hsep, last_tab.position.y),
					add_button_size));
		}
		else {
			add_preview_button->set_rect(
				Rect2(Point2(last_tab.position.x + last_tab.size.width + hsep, last_tab.position.y),
					add_button_size));
		}
	}
}

bool ThemeEditorPlugin::can_auto_hide() const { return theme_editor->theme.is_null(); }

void ThemeItemImportTree::_store_selected_item(TreeItem* p_tree_item) {}

void ThemeEditor::_remove_preview_tab(int p_tab) {}


