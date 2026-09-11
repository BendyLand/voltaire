/**************************************************************************/
/*  editor_inspector.cpp                                                  */
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

#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/doc/doc_tools.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_interface.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/add_metadata_dialog.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_inspector.compat.inc"
#include "editor_inspector.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/property_utils.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_flat.h"
#include "scene/scene_string_names.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

void EditorInspectorActionButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		set_button_icon(get_editor_theme_icon(icon_name));
	} break;
	}
}

EditorInspectorActionButton::EditorInspectorActionButton(
	const String& p_text, const StringName& p_icon_name)
{
	icon_name = p_icon_name;
	set_text(p_text);
	set_theme_type_variation(SNAME("InspectorActionButton"));
	set_h_size_flags(SIZE_SHRINK_CENTER);
}

bool EditorInspector::_property_path_matches(const String& p_property_path, const String& p_filter,
	EditorPropertyNameProcessor::Style p_style)
{
	if (p_property_path.containsn(p_filter)) {
		return true;
	}

	const Vector<String> prop_sections = p_property_path.split("/");
	for (int i = 0; i < prop_sections.size(); i++) {
		if (p_filter.is_subsequence_ofn(EditorPropertyNameProcessor::get_singleton()->process_name(
				prop_sections[i], p_style, p_property_path))) {
			return true;
		}
	}
	return false;
}

String EditorProperty::get_tooltip_string(const String& p_string) const
{
	// Trim to 100 characters to prevent the tooltip from being too long.
	constexpr int TOOLTIP_MAX_LENGTH = 100;
	return p_string.left(TOOLTIP_MAX_LENGTH).strip_edges() +
		   String((p_string.length() > TOOLTIP_MAX_LENGTH) ? "..." : "");
}

Size2 EditorProperty::get_minimum_size() const
{
	if (theme_cache.font.is_null()) {
		// Too early.
		return Vector2();
	}

	Size2 ms = Size2(0, theme_cache.inspector_property_height);
	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i));
		if (!c) {
			continue;
		}
		if (c == bottom_editor) {
			continue;
		}
		if (c == left_container) {
			continue;
		}
		if (c == right_container) {
			continue;
		}

		Size2 minsize = c->get_combined_minimum_size();
		ms = ms.max(minsize);
	}

	if (!label.is_empty()) {
		ms.width += theme_cache.font_offset + theme_cache.horizontal_separation;
	}

	// Always take the revert and pin values into account, since their state can be changed at whim
	// and we don't want to update the min width every time this happens.
	{
		if (!is_read_only()) {
			ms.width += theme_cache.revert_icon->get_width() + theme_cache.padding +
						theme_cache.horizontal_separation;
		}

		ms.width += theme_cache.pin_icon->get_width() + theme_cache.horizontal_separation;
	}

	if (keying) {
		ms.width += theme_cache.key_icon->get_width() + theme_cache.padding +
					theme_cache.horizontal_separation;
	}

	if (deletable) {
		ms.width += theme_cache.delete_icon->get_width() + theme_cache.padding +
					theme_cache.horizontal_separation;
	}

	if (checkable) {
		ms.width += theme_cache.checked_icon->get_width() + theme_cache.padding +
					theme_cache.horizontal_separation;
	}

	Size2 ls = left_container->get_combined_minimum_size();
	ms.width += ls.x;
	ms.height = MAX(ms.height, ls.y);

	Size2 rs = right_container->get_combined_minimum_size();
	ms.width += rs.x;
	ms.height = MAX(ms.height, rs.y);

	if (bottom_editor != nullptr && bottom_editor->is_visible()) {
		ms.height += label.is_empty() ? 0 : _get_v_separation();
		Size2 bems = bottom_editor->get_combined_minimum_size();
		ms.height += bems.height;
		ms.width = MAX(ms.width, bems.width);
	}

	return ms;
}

void EditorProperty::set_label(const String& p_label)
{
	label = p_label;
	queue_redraw();
}

void EditorProperty::set_doc_path(const String& p_doc_path) { doc_path = p_doc_path; }

void EditorProperty::set_internal(bool p_internal) { internal = p_internal; }

void EditorProperty::set_read_only(bool p_read_only)
{
	read_only = p_read_only;
	_set_read_only(p_read_only);
}

bool EditorProperty::is_read_only() const { return read_only; }

StringName EditorProperty::_get_revert_property() const { return property; }

void EditorProperty::set_draw_label(bool p_draw_label)
{
	draw_label = p_draw_label;
	queue_redraw();
	queue_sort();
}

bool EditorProperty::is_draw_label() const { return draw_label; }

void EditorProperty::set_draw_background(bool p_draw_background)
{
	draw_background = p_draw_background;
	queue_redraw();
}

bool EditorProperty::is_draw_background() const { return draw_background; }

void EditorProperty::set_checkable(bool p_checkable)
{
	checkable = p_checkable;
	queue_redraw();
	queue_sort();
}

bool EditorProperty::is_checkable() const { return checkable; }

void EditorProperty::set_checked(bool p_checked)
{
	checked = p_checked;
	queue_redraw();
}

bool EditorProperty::is_checked() const { return checked; }

void EditorProperty::set_draw_warning(bool p_draw_warning)
{
	draw_warning = p_draw_warning;
	queue_redraw();
}

void EditorProperty::set_keying(bool p_keying)
{
	keying = p_keying;
	queue_redraw();
	queue_sort();
}

void EditorProperty::set_deletable(bool p_deletable)
{
	deletable = p_deletable;
	queue_redraw();
	queue_sort();
}

bool EditorProperty::is_deletable() const { return deletable; }

bool EditorProperty::is_keying() const { return keying; }

bool EditorProperty::is_draw_warning() const { return draw_warning; }

void EditorProperty::grab_focus(int p_focusable)
{
	if (focusables.is_empty()) {
		return;
	}

	if (p_focusable >= 0) {
		ERR_FAIL_INDEX(p_focusable, focusables.size());
		focusables[p_focusable]->grab_focus(true);
	}
	else {
		focusables[0]->grab_focus(true);
	}
}

void EditorProperty::deselect()
{
	selected = false;
	selected_focusable = -1;
	queue_redraw();
}

bool EditorProperty::is_selected() const { return selected; }

void EditorProperty::add_inline_control(Control* p_control, InlineControlSide p_side)
{
	Node* parent = p_control->get_parent();
	if (parent != nullptr) {
		parent->remove_child(p_control);
	}
	if (p_side == INLINE_CONTROL_LEFT) {
		left_container->add_child(p_control);
	}
	else {
		right_container->add_child(p_control);
	}
}

void EditorProperty::set_label_overlayed(bool p_overlay) { label_overlayed = p_overlay; }

const Color* EditorProperty::_get_property_colors()
{
	static Color c[4];
	c[0] = theme_cache.property_color_x;
	c[1] = theme_cache.property_color_y;
	c[2] = theme_cache.property_color_z;
	c[3] = theme_cache.property_color_w;
	return c;
}

void EditorProperty::set_label_reference(Control* p_control) { label_reference = p_control; }

HBoxContainer* EditorProperty::get_inline_container(InlineControlSide p_side)
{
	if (p_side == INLINE_CONTROL_LEFT) {
		return left_container;
	}
	else {
		return right_container;
	}
}

void EditorProperty::set_bottom_editor(Control* p_control)
{
	bottom_editor = p_control;
	if (has_borders) {
		_update_property_bg();
	}
}

void EditorProperty::set_deferred_drag_mode_enabled(bool p_enabled)
{
	deferred_drag_mode = p_enabled;
}

bool EditorProperty::is_deferred_drag_mode_enabled() const { return deferred_drag_mode; }

void EditorProperty::set_use_folding(bool p_use_folding) { use_folding = p_use_folding; }

bool EditorProperty::is_using_folding() const { return use_folding; }

void EditorProperty::expand_all_folding() {}

void EditorProperty::collapse_all_folding() {}

void EditorProperty::expand_revertable() {}

void EditorProperty::set_selectable(bool p_selectable) { selectable = p_selectable; }

bool EditorProperty::is_selectable() const { return selectable; }

void EditorProperty::set_name_split_ratio(float p_ratio) { split_ratio = p_ratio; }

float EditorProperty::get_name_split_ratio() const { return split_ratio; }

void EditorProperty::set_name_fixed_size(float p_size)
{
	if (name_fixed_size == p_size) {
		return;
	}
	name_fixed_size = p_size;
	queue_sort();
}

void EditorProperty::set_favoritable(bool p_favoritable) { can_favorite = p_favoritable; }

bool EditorProperty::is_favoritable() const { return can_favorite; }

////////////////////////////////////////////////
////////////////////////////////////////////////

void EditorInspectorPlugin::add_custom_control(Control* control)
{
	AddedEditor ae;
	ae.property_editor = control;
	added_editors.push_back(ae);
}

void EditorInspectorPlugin::add_property_editor(
	const String& p_for_property, Control* p_prop, bool p_add_to_end, const String& p_label)
{
	AddedEditor ae;
	ae.properties.push_back(p_for_property);
	ae.property_editor = p_prop;
	ae.add_to_end = p_add_to_end;
	ae.label = p_label;
	added_editors.push_back(ae);
}

void EditorInspectorPlugin::add_property_editor_for_multiple_properties(
	const String& p_label, const Vector<String>& p_properties, Control* p_prop)
{
	AddedEditor ae;
	ae.properties = p_properties;
	ae.property_editor = p_prop;
	ae.label = p_label;
	added_editors.push_back(ae);
}

////////////////////////////////////////////////
////////////////////////////////////////////////

Control* EditorInspectorCategory::make_custom_tooltip(const String& p_text) const
{
	// If it's not a doc tooltip, fallback to the default one.
	if (doc_class_name.is_empty()) {
		return nullptr;
	}

	return EditorHelpBitTooltip::make_tooltip(const_cast<EditorInspectorCategory*>(this), p_text);
}

void EditorInspectorCategory::set_as_favorite()
{
	is_favorite = true;
	_update_icon();
}

void EditorInspectorCategory::set_doc_class_name(const String& p_name) { doc_class_name = p_name; }

void EditorInspectorCategory::set_color_level(int p_color_level)
{
	ERR_FAIL_COND(p_color_level < -1 || p_color_level > 16);
	color_level = p_color_level;
	update_minimum_size();
	queue_redraw();
}

Size2 EditorInspectorCategory::get_minimum_size() const
{
	Size2 ms;
	if (theme_cache.bold_font.is_valid()) {
		ms.height = theme_cache.bold_font->get_height(theme_cache.bold_font_size);
	}
	if (icon.is_valid()) {
		ms.height = MAX(theme_cache.class_icon_size, ms.height);
	}
	ms.height += theme_cache.vertical_separation;

	Ref<StyleBox> bg;
	if (color_level == -1) {
		bg = theme_cache.background;
	}
	else if (color_level == 0) {
		bg = theme_cache.sub_inspector_background;
	}
	else {
		bg = theme_cache.sub_inspector_color_background[color_level];
	}

	if (bg.is_valid()) {
		ms.height += bg->get_content_margin(SIDE_TOP) + bg->get_content_margin(SIDE_BOTTOM);
	}

	return ms;
}

void EditorInspectorCategory::_theme_changed()
{
	// This needs to be done via the signal, as it's fired before the minimum since is updated.
	EditorInspector::initialize_category_theme(theme_cache, this);
	menu_icon_dirty = true;
	_update_icon();
}

EditorInspectorCategory::EditorInspectorCategory() { set_focus_mode(FOCUS_ACCESSIBILITY); }

////////////////////////////////////////////////
////////////////////////////////////////////////

void EditorInspectorSection::_test_unfold()
{
	if (!vbox_added) {
		add_child(vbox);
		move_child(vbox, 0);
		vbox_added = true;
	}
}

Ref<Texture2D> EditorInspectorSection::_get_checkbox()
{
	Ref<Texture2D> checkbox;

	if (checkable) {
		if (checked) {
			checkbox = theme_cache.icon_gui_checked;
		}
		else {
			checkbox = theme_cache.icon_gui_unchecked;
		}
	}

	return checkbox;
}

int EditorInspectorSection::_get_header_height()
{
	int header_height = theme_cache.bold_font->get_height(theme_cache.bold_font_size);
	Ref<Texture2D> arrow = _get_arrow();
	if (arrow.is_valid()) {
		header_height = MAX(header_height, arrow->get_height());
	}
	header_height += theme_cache.vertical_separation;

	return header_height;
}

Size2 EditorInspectorSection::get_minimum_size() const
{
	Size2 ms;
	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i));
		if (!c) {
			continue;
		}
		Size2 minsize = c->get_combined_minimum_size();
		ms = ms.max(minsize);
	}

	if (theme_cache.font.is_valid()) {
		ms.height +=
			theme_cache.font->get_height(theme_cache.font_size) + theme_cache.vertical_separation;
		ms.width += theme_cache.inspector_margin;
	}

	if (indent_depth > 0 && theme_cache.indent_size > 0) {
		ms.width += indent_depth * theme_cache.indent_size;
	}
	if (indent_depth > 0 && theme_cache.indent_box.is_valid()) {
		ms.width += theme_cache.indent_box->get_margin(SIDE_LEFT) +
					theme_cache.indent_box->get_margin(SIDE_RIGHT);
	}

	return ms;
}

String EditorInspectorSection::get_section() const { return section; }

VBoxContainer* EditorInspectorSection::get_vbox() { return vbox; }

void EditorInspectorSection::set_bg_color(const Color& p_bg_color)
{
	bg_color = p_bg_color;
	queue_redraw();
}

void EditorInspectorSection::set_keying(bool p_keying)
{
	if (keying == (checkable && p_keying)) {
		return;
	}

	keying = checkable && p_keying;
	if (checkable) {
		queue_redraw();
	}
}

void EditorInspectorSection::reset_timer()
{
	if (dropping_for_unfold && !dropping_unfold_timer->is_stopped()) {
		dropping_unfold_timer->start();
	}
}

void EditorInspectorSection::set_checked(bool p_checked)
{
	if (checked == p_checked) {
		return;
	}

	checked = p_checked;
	if (!checkbox_only && checkable && !checked) {
		vbox->hide();
	}
	else if (!checkbox_only) {
		unfold();
	}

	queue_redraw();
}

bool EditorInspectorSection::has_revertable_properties() const
{
	return !revertable_properties.is_empty();
}

void EditorInspectorSection::property_can_revert_changed(const String& p_path, bool p_can_revert)
{
	bool had_revertable_properties = has_revertable_properties();
	if (p_can_revert) {
		revertable_properties.insert(p_path);
	}
	else {
		revertable_properties.erase(p_path);
	}
	if (has_revertable_properties() != had_revertable_properties) {
		queue_redraw();
	}
}

void EditorInspectorSection::_property_edited(const String& p_property)
{
	if (!related_enable_property.is_empty() && p_property == related_enable_property) {
		update_property();
	}
}

////////////////////////////////////////////////
////////////////////////////////////////////////

void EditorInspectorArray::_add_button_pressed() { _move_element(-1, -1); }

void EditorInspectorArray::_rmb_popup_id_pressed(int p_id)
{
	switch (p_id) {
	case OPTION_MOVE_UP:
		if (popup_array_index_pressed > 0) {
			_move_element(popup_array_index_pressed, popup_array_index_pressed - 1);
		}
		break;
	case OPTION_MOVE_DOWN:
		if (popup_array_index_pressed < count - 1) {
			_move_element(popup_array_index_pressed, popup_array_index_pressed + 2);
		}
		break;
	case OPTION_NEW_BEFORE:
		_move_element(-1, popup_array_index_pressed);
		break;
	case OPTION_NEW_AFTER:
		_move_element(-1, popup_array_index_pressed + 1);
		break;
	case OPTION_REMOVE:
		_move_element(popup_array_index_pressed, -1);
		break;
	case OPTION_CLEAR_ARRAY:
		_clear_array();
		break;
	case OPTION_RESIZE_ARRAY:
		new_size_spin_box->set_value(count);
		resize_dialog->get_ok_button()->set_disabled(true);
		resize_dialog->popup_centered(Size2(250, 0) * EDSCALE);
		new_size_spin_box->get_line_edit()->grab_focus();
		new_size_spin_box->get_line_edit()->select_all();
		break;
	default:
		break;
	}
}

void EditorInspectorArray::_vbox_visibility_changed()
{
	control_dropping->set_visible(vbox->is_visible_in_tree());
}

void EditorInspectorArray::_panel_draw(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)array_elements.size());

	Ref<StyleBox> style = get_theme_stylebox(SNAME("Focus"), EditorStringName(EditorStyles));
	if (style.is_null()) {
		return;
	}
	if (array_elements[p_index].panel->has_focus(true)) {
		array_elements[p_index].panel->draw_style_box(
			style.ptr(), Rect2(Vector2(), array_elements[p_index].panel->get_size()));
	}
}

void EditorInspectorArray::_panel_gui_focus(int p_index)
{
	array_elements[p_index].panel->queue_redraw();
	selected = p_index;
}

void EditorInspectorArray::_panel_gui_unfocus(int p_index)
{
	array_elements[p_index].panel->queue_redraw();
	if (selected == p_index) {
		selected = -1;
	}
}

void EditorInspectorArray::show_menu(int p_index, const Vector2& p_offset)
{
	popup_array_index_pressed = begin_array_index + p_index;
	rmb_popup->set_item_disabled(OPTION_MOVE_UP, popup_array_index_pressed == 0);
	rmb_popup->set_item_disabled(OPTION_MOVE_DOWN, popup_array_index_pressed == count - 1);
	rmb_popup->set_position(get_screen_position() + p_offset);
	rmb_popup->reset_size();
	rmb_popup->popup();
}

int EditorInspectorArray::_drop_position() const
{
	for (int i = 0; i < (int)array_elements.size(); i++) {
		const ArrayElement& ae = array_elements[i];

		Size2 size = ae.panel->get_size();
		Vector2 mp = ae.panel->get_local_mouse_position();

		if (Rect2(Vector2(), size).has_point(mp)) {
			if (mp.y < size.y / 2) {
				return i;
			}
			else {
				return i + 1;
			}
		}
	}
	return -1;
}

void EditorInspectorArray::_resize_dialog_confirmed()
{
	if (int(new_size_spin_box->get_value()) == count) {
		return;
	}

	resize_dialog->hide();
	_resize_array(int(new_size_spin_box->get_value()));
}

void EditorInspectorArray::_new_size_spin_box_value_changed(float p_value)
{
	resize_dialog->get_ok_button()->set_disabled(int(p_value) == count);
}

void EditorInspectorArray::_new_size_spin_box_text_submitted(const String& p_text)
{
	_resize_dialog_confirmed();
}

void EditorInspectorArray::_remove_item(int p_index) { _move_element(p_index, -1); }

ArrayPanelContainer::ArrayPanelContainer() { set_focus_mode(FOCUS_ACCESSIBILITY); }

VBoxContainer* EditorInspectorArray::get_vbox(int p_index)
{
	if (p_index >= begin_array_index && p_index < end_array_index) {
		return array_elements[p_index - begin_array_index].vbox;
	}
	else if (p_index < 0) {
		return vbox;
	}
	else {
		return nullptr;
	}
}

////////////////////////////////////////////////
////////////////////////////////////////////////

void EditorPaginator::update(int p_page, int p_max_page)
{
	page = p_page;
	max_page = p_max_page;

	// Update buttons.
	first_page_button->set_disabled(page == 0);
	prev_page_button->set_disabled(page == 0);
	next_page_button->set_disabled(page == max_page);
	last_page_button->set_disabled(page == max_page);

	// Update page number and page count.
	page_line_edit->set_text(vformat("%d", page + 1));
	page_count_label->set_text(vformat("/ %d", max_page + 1));
}

void EditorPaginator::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		first_page_button->set_button_icon(get_editor_theme_icon(SNAME("PageFirst")));
		prev_page_button->set_button_icon(get_editor_theme_icon(SNAME("PagePrevious")));
		next_page_button->set_button_icon(get_editor_theme_icon(SNAME("PageNext")));
		last_page_button->set_button_icon(get_editor_theme_icon(SNAME("PageLast")));
	} break;
	}
}

void EditorPaginator::_bind_methods() {}

////////////////////////////////////////////////
////////////////////////////////////////////////

Ref<EditorInspectorPlugin> EditorInspector::inspector_plugins[MAX_PLUGINS];
int EditorInspector::inspector_plugin_count = 0;

void EditorInspector::initialize_category_theme(
	EditorInspectorCategory::ThemeCache& p_cache, Control* p_control)
{
	EditorInspector* parent_inspector = _get_control_parent_inspector(p_control);
	if (parent_inspector && parent_inspector != p_control) {
		p_cache = parent_inspector->category_theme_cache;
		return;
	}

	p_cache.horizontal_separation =
		p_control->get_theme_constant(SNAME("h_separation"), SNAME("Tree"));
	p_cache.vertical_separation =
		p_control->get_theme_constant(SNAME("separation"), SNAME("EditorPropertyContainer"));
	p_cache.class_icon_size =
		p_control->get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

	p_cache.font_color = p_control->get_theme_color(SceneStringName(font_color), SNAME("Tree"));

	p_cache.bold_font = p_control->get_theme_font(SNAME("bold"), EditorStringName(EditorFonts));
	p_cache.bold_font_size =
		p_control->get_theme_font_size(SNAME("bold_size"), EditorStringName(EditorFonts));

	p_cache.icon_copy = p_control->get_editor_theme_icon(SNAME("ActionCopy"));
	p_cache.icon_paste = p_control->get_editor_theme_icon(SNAME("ActionPaste"));

	p_cache.icon_favorites = p_control->get_editor_theme_icon(SNAME("Favorites"));
	p_cache.icon_unfavorite = p_control->get_editor_theme_icon(SNAME("Unfavorite"));
	p_cache.icon_help = p_control->get_editor_theme_icon(SNAME("Help"));

	p_cache.background =
		p_control->get_theme_stylebox(SNAME("bg"), SNAME("EditorInspectorCategory"));

	if (p_control == parent_inspector) {
		// Only initialize for the inspector, as stand-alone categories won't need it.
		p_cache.sub_inspector_background = p_control->get_theme_stylebox(
			"sub_inspector_category_bg", EditorStringName(EditorStyles));
		for (int i = 0; i <= 16; i++) {
			p_cache.sub_inspector_color_background[i] = p_control->get_theme_stylebox(
				"sub_inspector_color_category_bg" + itos(i), EditorStringName(EditorStyles));
		}
	}
}

void EditorInspector::add_inspector_plugin(const Ref<EditorInspectorPlugin>& p_plugin)
{
	ERR_FAIL_COND(inspector_plugin_count == MAX_PLUGINS);

	for (int i = 0; i < inspector_plugin_count; i++) {
		if (inspector_plugins[i] == p_plugin) {
			return; // already exists
		}
	}
	inspector_plugins[inspector_plugin_count++] = p_plugin;
}

void EditorInspector::remove_inspector_plugin(const Ref<EditorInspectorPlugin>& p_plugin)
{
	int idx = -1;
	for (int i = 0; i < inspector_plugin_count; i++) {
		if (inspector_plugins[i] == p_plugin) {
			idx = i;
			break;
		}
	}

	ERR_FAIL_COND_MSG(idx == -1, "Trying to remove nonexistent inspector plugin.");
	for (int i = idx; i < inspector_plugin_count - 1; i++) {
		inspector_plugins[i] = inspector_plugins[i + 1];
	}
	inspector_plugins[inspector_plugin_count - 1] = Ref<EditorInspectorPlugin>();

	inspector_plugin_count--;
}

void EditorInspector::cleanup_plugins()
{
	for (int i = 0; i < inspector_plugin_count; i++) {
		inspector_plugins[i].unref();
	}
	inspector_plugin_count = 0;
}

bool EditorInspector::is_main_editor_inspector() const
{
	return InspectorDock::get_singleton() && InspectorDock::get_inspector_singleton() == this;
}

String EditorInspector::get_selected_path() const { return property_selected; }

void EditorInspector::update_property(const String& p_prop)
{
	if (!editor_property_map.has(p_prop)) {
		return;
	}

	for (EditorProperty* E : editor_property_map[p_prop]) {
		E->update_property();
		E->update_editor_property_status();
		E->update_cache();
	}

	for (EditorInspectorSection* S : sections) {
		if (S->is_checkable()) {
			S->_property_edited(p_prop);
		}
	}
}

void EditorInspector::set_keying(bool p_active)
{
	if (keying == p_active) {
		return;
	}
	keying = p_active;
	_keying_changed();
}

void EditorInspector::_keying_changed()
{
	for (const KeyValue<StringName, List<EditorProperty*>>& F : editor_property_map) {
		for (EditorProperty* E : F.value) {
			if (E) {
				E->set_keying(keying);
			}
		}
	}

	for (EditorInspectorSection* S : sections) {
		S->set_keying(keying);
	}
}

void EditorInspector::set_read_only(bool p_read_only)
{
	if (p_read_only == read_only) {
		return;
	}
	read_only = p_read_only;
	update_tree();
}

EditorPropertyNameProcessor::Style EditorInspector::get_property_name_style() const
{
	return property_name_style;
}

void EditorInspector::set_property_name_style(EditorPropertyNameProcessor::Style p_style)
{
	if (property_name_style == p_style) {
		return;
	}
	property_name_style = p_style;
	update_tree();
}

void EditorInspector::set_use_settings_name_style(bool p_enable)
{
	if (use_settings_name_style == p_enable) {
		return;
	}
	use_settings_name_style = p_enable;
	if (use_settings_name_style) {
		set_property_name_style(EditorPropertyNameProcessor::get_singleton()->get_settings_style());
	}
}

void EditorInspector::set_autoclear(bool p_enable) { autoclear = p_enable; }

void EditorInspector::set_show_categories(bool p_show_standard, bool p_show_custom)
{
	show_standard_categories = p_show_standard;
	show_custom_categories = p_show_custom;
	update_tree();
}

void EditorInspector::set_category_color_level(int p_color_level)
{
	if (category_color_level != p_color_level) {
		category_color_level = p_color_level;
		update_tree();
	}
}

void EditorInspector::set_use_doc_hints(bool p_enable)
{
	use_doc_hints = p_enable;
	update_tree();
}

void EditorInspector::set_hide_script(bool p_hide)
{
	hide_script = p_hide;
	update_tree();
}

void EditorInspector::set_hide_metadata(bool p_hide)
{
	hide_metadata = p_hide;
	update_tree();
}

void EditorInspector::set_use_filter(bool p_use)
{
	use_filter = p_use;
	update_tree();
}

void EditorInspector::set_use_folding(bool p_use_folding, bool p_update_tree)
{
	use_folding = p_use_folding;

	if (p_update_tree) {
		update_tree();
	}
}

bool EditorInspector::is_using_folding() { return use_folding; }

void EditorInspector::collapse_all_folding()
{
	for (EditorInspectorSection* E : sections) {
		E->fold();
	}

	for (const KeyValue<StringName, List<EditorProperty*>>& F : editor_property_map) {
		for (EditorProperty* E : F.value) {
			E->collapse_all_folding();
		}
	}
}

void EditorInspector::expand_all_folding()
{
	for (EditorInspectorSection* E : sections) {
		E->unfold();
	}
	for (const KeyValue<StringName, List<EditorProperty*>>& F : editor_property_map) {
		for (EditorProperty* E : F.value) {
			E->expand_all_folding();
		}
	}
}

int EditorInspector::get_scroll_offset() const { return get_v_scroll(); }

void EditorInspector::set_use_wide_editors(bool p_enable) { wide_editors = p_enable; }

void EditorInspector::set_root_inspector(EditorInspector* p_root_inspector)
{
	root_inspector = p_root_inspector;
	// Only the root inspector should follow focus.
	set_follow_focus(false);
}

void EditorInspector::set_use_deletable_properties(bool p_enabled)
{
	deletable_properties = p_enabled;
}

void EditorInspector::_page_change_request(int p_new_page, const StringName& p_array_prefix)
{
	int prev_page = per_array_page.has(p_array_prefix) ? per_array_page[p_array_prefix] : 0;
	int new_page = MAX(0, p_new_page);
	if (new_page != prev_page) {
		per_array_page[p_array_prefix] = new_page;
		update_tree_pending = true;
	}
}

void EditorInspector::set_property_prefix(const String& p_prefix) { property_prefix = p_prefix; }

String EditorInspector::get_property_prefix() const { return property_prefix; }

void EditorInspector::add_custom_property_description(
	const String& p_class, const String& p_property, const String& p_description)
{
	const String key = vformat("property|%s|%s", p_class, p_property);
	custom_property_descriptions[key] = p_description;
}

String EditorInspector::get_custom_property_description(const String& p_property) const
{
	HashMap<String, String>::ConstIterator E = custom_property_descriptions.find(p_property);
	if (E) {
		return TTR(E->value);
	}
	return "";
}

void EditorInspector::remap_doc_property_class(
	const String& p_property_prefix, const String& p_class)
{
	doc_property_class_remaps[p_property_prefix] = p_class;
}

void EditorInspector::set_object_class(const String& p_class) { object_class = p_class; }

String EditorInspector::get_object_class() const { return object_class; }

void EditorInspector::_feature_profile_changed() { update_tree(); }

void EditorInspector::set_restrict_to_basic_settings(bool p_restrict)
{
	restrict_to_basic = p_restrict;
	update_tree();
}

void EditorInspector::_bind_methods() {}

void EditorProperty::_set_read_only(bool p_read_only) {}

void EditorProperty::update_property() {}


