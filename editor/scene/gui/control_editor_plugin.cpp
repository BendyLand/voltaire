/**************************************************************************/
/*  control_editor_plugin.cpp                                             */
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

#include "control_editor_plugin.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"

// Inspector controls.

void ControlPositioningWarning::_update_toggler()
{
	Ref<Texture2D> arrow;
	if (hint_label->is_visible()) {
		arrow = get_theme_icon(SNAME("arrow"), SNAME("Tree"));
		set_tooltip_text(TTR("Collapse positioning hint."));
	}
	else {
		if (is_layout_rtl()) {
			arrow = get_theme_icon(SNAME("arrow_collapsed"), SNAME("Tree"));
		}
		else {
			arrow = get_theme_icon(SNAME("arrow_collapsed_mirrored"), SNAME("Tree"));
		}
		set_tooltip_text(TTR("Expand positioning hint."));
	}

	hint_icon->set_texture(arrow);
}

void ControlPositioningWarning::set_control(Control* p_node)
{
	control_node = p_node;
	_update_warning();
}

void ControlPositioningWarning::gui_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		bool state = !hint_label->is_visible();

		hint_filler_left->set_visible(state);
		hint_label->set_visible(state);
		hint_filler_right->set_visible(state);

		_update_toggler();
	}
}

void ControlPositioningWarning::_notification(int p_notification)
{
	switch (p_notification) {
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_TRANSLATION_CHANGED:
	case NOTIFICATION_THEME_CHANGED:
		_update_warning();
		_update_toggler();
		break;
	}
}

ControlPositioningWarning::ControlPositioningWarning()
{
	set_mouse_filter(MOUSE_FILTER_STOP);

	bg_panel = memnew(PanelContainer);
	bg_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(bg_panel);

	grid = memnew(GridContainer);
	grid->set_columns(3);
	bg_panel->add_child(grid);

	title_icon = memnew(TextureRect);
	title_icon->set_stretch_mode(TextureRect::StretchMode::STRETCH_KEEP_CENTERED);
	grid->add_child(title_icon);

	title_label = memnew(Label);
	title_label->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_WORD);
	title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_label->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);
	grid->add_child(title_label);

	hint_icon = memnew(TextureRect);
	hint_icon->set_stretch_mode(TextureRect::StretchMode::STRETCH_KEEP_CENTERED);
	grid->add_child(hint_icon);

	// Filler.
	hint_filler_left = memnew(Control);
	hint_filler_left->hide();
	grid->add_child(hint_filler_left);

	hint_label = memnew(Label);
	hint_label->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_WORD);
	hint_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hint_label->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);
	hint_label->hide();
	grid->add_child(hint_label);

	// Filler.
	hint_filler_right = memnew(Control);
	hint_filler_right->hide();
	grid->add_child(hint_filler_right);
}

void EditorPropertyAnchorsPreset::_set_read_only(bool p_read_only)
{
	options->set_disabled(p_read_only);
}

void EditorPropertySizeFlags::_set_read_only(bool p_read_only)
{
	for (CheckBox* check : flag_checks) {
		check->set_disabled(p_read_only);
	}
	flag_presets->set_disabled(p_read_only);
}

void EditorPropertySizeFlags::_preset_selected(int p_which)
{
	int preset = flag_presets->get_item_id(p_which);
	if (preset == SIZE_FLAGS_PRESET_CUSTOM) {
		flag_options->set_visible(true);
		return;
	}
	flag_options->set_visible(false);

	uint32_t value = 0;
	switch (preset) {
	case SIZE_FLAGS_PRESET_FILL:
		value = Control::SIZE_FILL;
		break;
	case SIZE_FLAGS_PRESET_SHRINK_BEGIN:
		value = Control::SIZE_SHRINK_BEGIN;
		break;
	case SIZE_FLAGS_PRESET_SHRINK_CENTER:
		value = Control::SIZE_SHRINK_CENTER;
		break;
	case SIZE_FLAGS_PRESET_SHRINK_END:
		value = Control::SIZE_SHRINK_END;
		break;
	}

	bool is_expand = flag_expand->is_visible() && flag_expand->is_pressed();
	if (is_expand) {
		value |= Control::SIZE_EXPAND;
	}
}

Size2 ControlEditorPopupButton::get_minimum_size() const
{
	Vector2 base_size = Vector2(26, 26) * EDSCALE;

	if (arrow_icon.is_null()) {
		return base_size;
	}

	Vector2 final_size;
	final_size.x = base_size.x + arrow_icon->get_width();
	final_size.y = MAX(base_size.y, arrow_icon->get_height());

	return final_size;
}

void ControlEditorPopupButton::toggled(bool p_pressed)
{
	if (!p_pressed) {
		return;
	}

	Size2 size = get_size() * get_viewport()->get_canvas_transform().get_scale();

	popup_panel->set_size(Size2(size.width, 0));
	Point2 gp = get_screen_position();
	gp.y += size.y;
	if (is_layout_rtl()) {
		gp.x += size.width - popup_panel->get_size().width;
	}
	popup_panel->set_position(gp);

	popup_panel->popup();
}

void ControlEditorPopupButton::_popup_visibility_changed(bool p_visible) { set_pressed(p_visible); }

void ControlEditorPopupButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		arrow_icon = get_theme_icon("select_arrow", "Tree");
	} break;

	case NOTIFICATION_DRAW: {
		if (arrow_icon.is_valid()) {
			Vector2 arrow_pos = Point2(26, 0) * EDSCALE;
			if (is_layout_rtl()) {
				arrow_pos.x = get_size().x - arrow_pos.x - arrow_icon->get_width();
			}
			arrow_pos.y = get_size().y / 2 - arrow_icon->get_height() / 2;
			draw_texture(arrow_icon.ptr(), arrow_pos);
		}
	} break;

	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		popup_panel->set_layout_direction((Window::LayoutDirection)get_layout_direction());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible_in_tree()) {
			popup_panel->hide();
		}
	} break;
	}
}

void ControlEditorPresetPicker::_add_separator(BoxContainer* p_box, Separator* p_separator)
{
	p_separator->add_theme_constant_override("separation", grid_separation);
	p_separator->set_custom_minimum_size(Size2i(1, 1));
	p_box->add_child(p_separator);
}

void ControlEditorPresetPicker::_update_preset_button_state(int p_preset)
{
	for (KeyValue<int, Button*>& E : preset_buttons) {
		Button* button = E.value;

		if (!button) {
			continue;
		}

		button->begin_bulk_theme_override();

		if (E.key == p_preset) {
			const Color pressed_color = get_theme_color(SNAME("icon_pressed_color"), "Button");
			button->add_theme_color_override(SNAME("icon_normal_color"), pressed_color);
			button->add_theme_color_override(SNAME("icon_hover_color"), pressed_color);
		}
		else {
			button->remove_theme_color_override(SNAME("icon_normal_color"));
			button->remove_theme_color_override(SNAME("icon_hover_color"));
		}

		button->end_bulk_theme_override();
	}
}

void AnchorPresetPicker::_notification(int p_notification)
{
	switch (p_notification) {
	case NOTIFICATION_THEME_CHANGED: {
		preset_buttons[PRESET_TOP_LEFT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignTopLeft")));
		preset_buttons[PRESET_CENTER_TOP]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignCenterTop")));
		preset_buttons[PRESET_TOP_RIGHT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignTopRight")));

		preset_buttons[PRESET_CENTER_LEFT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignCenterLeft")));
		preset_buttons[PRESET_CENTER]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignCenter")));
		preset_buttons[PRESET_CENTER_RIGHT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignCenterRight")));

		preset_buttons[PRESET_BOTTOM_LEFT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignBottomLeft")));
		preset_buttons[PRESET_CENTER_BOTTOM]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignCenterBottom")));
		preset_buttons[PRESET_BOTTOM_RIGHT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignBottomRight")));

		preset_buttons[PRESET_TOP_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignTopWide")));
		preset_buttons[PRESET_HCENTER_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignHCenterWide")));
		preset_buttons[PRESET_BOTTOM_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignBottomWide")));

		preset_buttons[PRESET_LEFT_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignLeftWide")));
		preset_buttons[PRESET_VCENTER_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignVCenterWide")));
		preset_buttons[PRESET_RIGHT_WIDE]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignRightWide")));

		preset_buttons[PRESET_FULL_RECT]->set_button_icon(
			get_editor_theme_icon(SNAME("ControlAlignFullRect")));
	} break;
	}
}

void AnchorPresetPicker::set_selected_preset(int p_preset)
{
	_update_preset_button_state(p_preset);
}

void AnchorPresetPicker::_bind_methods() {}

AnchorPresetPicker::AnchorPresetPicker()
{
	VBoxContainer* main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", grid_separation);
	add_child(main_vb);

	HBoxContainer* top_row = memnew(HBoxContainer);
	top_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	top_row->add_theme_constant_override("separation", grid_separation);
	main_vb->add_child(top_row);

	_add_row_button(top_row, PRESET_TOP_LEFT, TTRC("Top Left"));
	_add_row_button(top_row, PRESET_CENTER_TOP, TTRC("Center Top"));
	_add_row_button(top_row, PRESET_TOP_RIGHT, TTRC("Top Right"));
	_add_separator(top_row, memnew(VSeparator));
	_add_row_button(top_row, PRESET_TOP_WIDE, TTRC("Top Wide"));

	HBoxContainer* mid_row = memnew(HBoxContainer);
	mid_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	mid_row->add_theme_constant_override("separation", grid_separation);
	main_vb->add_child(mid_row);

	_add_row_button(mid_row, PRESET_CENTER_LEFT, TTRC("Center Left"));
	_add_row_button(mid_row, PRESET_CENTER, TTRC("Center"));
	_add_row_button(mid_row, PRESET_CENTER_RIGHT, TTRC("Center Right"));
	_add_separator(mid_row, memnew(VSeparator));
	_add_row_button(mid_row, PRESET_HCENTER_WIDE, TTRC("HCenter Wide"));

	HBoxContainer* bot_row = memnew(HBoxContainer);
	bot_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	bot_row->add_theme_constant_override("separation", grid_separation);
	main_vb->add_child(bot_row);

	_add_row_button(bot_row, PRESET_BOTTOM_LEFT, TTRC("Bottom Left"));
	_add_row_button(bot_row, PRESET_CENTER_BOTTOM, TTRC("Center Bottom"));
	_add_row_button(bot_row, PRESET_BOTTOM_RIGHT, TTRC("Bottom Right"));
	_add_separator(bot_row, memnew(VSeparator));
	_add_row_button(bot_row, PRESET_BOTTOM_WIDE, TTRC("Bottom Wide"));

	_add_separator(main_vb, memnew(HSeparator));

	HBoxContainer* extra_row = memnew(HBoxContainer);
	extra_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	extra_row->add_theme_constant_override("separation", grid_separation);
	main_vb->add_child(extra_row);

	_add_row_button(extra_row, PRESET_LEFT_WIDE, TTRC("Left Wide"));
	_add_row_button(extra_row, PRESET_VCENTER_WIDE, TTRC("VCenter Wide"));
	_add_row_button(extra_row, PRESET_RIGHT_WIDE, TTRC("Right Wide"));
	_add_separator(extra_row, memnew(VSeparator));
	_add_row_button(extra_row, PRESET_FULL_RECT, TTRC("Full Rect"));
}

void SizeFlagPresetPicker::set_allowed_flags(Vector<SizeFlags>& p_flags)
{
	preset_buttons[SIZE_SHRINK_BEGIN]->set_disabled(!p_flags.has(SIZE_SHRINK_BEGIN));
	preset_buttons[SIZE_SHRINK_CENTER]->set_disabled(!p_flags.has(SIZE_SHRINK_CENTER));
	preset_buttons[SIZE_SHRINK_END]->set_disabled(!p_flags.has(SIZE_SHRINK_END));
	preset_buttons[SIZE_FILL]->set_disabled(!p_flags.has(SIZE_FILL));

	expand_button->set_disabled(!p_flags.has(SIZE_EXPAND));
	if (p_flags.has(SIZE_EXPAND)) {
		expand_button->set_tooltip_text(
			TTR("Enable to also set the Expand flag.\nDisable to only set Shrink/Fill flags."));
	}
	else {
		expand_button->set_pressed(false);
		expand_button->set_tooltip_text(
			TTR("Some parents of the selected nodes do not support the Expand flag."));
	}
}

void SizeFlagPresetPicker::set_selected_preset(int p_preset)
{
	_update_preset_button_state(p_preset);
}

void SizeFlagPresetPicker::set_expand_flag(bool p_expand) { expand_button->set_pressed(p_expand); }

void SizeFlagPresetPicker::_notification(int p_notification)
{
	switch (p_notification) {
	case NOTIFICATION_THEME_CHANGED: {
		if (vertical) {
			preset_buttons[SIZE_SHRINK_BEGIN]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenterTop")));
			preset_buttons[SIZE_SHRINK_CENTER]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenter")));
			preset_buttons[SIZE_SHRINK_END]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenterBottom")));

			preset_buttons[SIZE_FILL]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignVCenterWide")));
		}
		else {
			preset_buttons[SIZE_SHRINK_BEGIN]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenterLeft")));
			preset_buttons[SIZE_SHRINK_CENTER]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenter")));
			preset_buttons[SIZE_SHRINK_END]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignCenterRight")));

			preset_buttons[SIZE_FILL]->set_button_icon(
				get_editor_theme_icon(SNAME("ControlAlignHCenterWide")));
		}
	} break;
	}
}

// Toolbar.

Vector2 ControlEditorToolbar::_position_to_anchor(const Control* p_control, Vector2 position)
{
	ERR_FAIL_NULL_V(p_control, Vector2());

	Rect2 parent_rect = p_control->get_parent_anchorable_rect();

	Vector2 output;
	if (p_control->is_layout_rtl()) {
		output.x = (parent_rect.size.x == 0)
					   ? 0.0
					   : (parent_rect.size.x - p_control->get_transform().xform(position).x -
							 parent_rect.position.x) /
							 parent_rect.size.x;
	}
	else {
		output.x = (parent_rect.size.x == 0)
					   ? 0.0
					   : (p_control->get_transform().xform(position).x - parent_rect.position.x) /
							 parent_rect.size.x;
	}
	output.y = (parent_rect.size.y == 0)
				   ? 0.0
				   : (p_control->get_transform().xform(position).y - parent_rect.position.y) /
						 parent_rect.size.y;
	return output;
}

void ControlEditorToolbar::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		anchors_button->set_button_icon(get_editor_theme_icon(SNAME("ControlLayout")));
		anchor_mode_button->set_button_icon(get_editor_theme_icon(SNAME("Anchor")));
		containers_button->set_button_icon(get_editor_theme_icon(SNAME("ContainerLayout")));
	} break;
	}
}

ControlEditorToolbar* ControlEditorToolbar::singleton = nullptr;

// Editor plugin.

void ControlOffsetTransformPreview::forward_canvas_draw_over_viewport(Control* p_overlay) const
{
	if (!selected_control || !selected_control->is_offset_transform_enabled() ||
		selected_control->is_offset_transform_visual_only()) {
		return;
	}

	Point2 top_left = Point2();
	Point2 bottom_right = selected_control->get_size();
	Point2 top_right = Point2(bottom_right.x, top_left.y);
	Point2 bottom_left = Point2(top_left.x, bottom_right.y);

	Transform2D control_transform_without_offset =
		selected_control->get_global_transform() *
		selected_control->get_offset_transform().affine_inverse();
	top_left = control_transform_without_offset.xform(top_left);
	bottom_right = control_transform_without_offset.xform(bottom_right);
	top_right = control_transform_without_offset.xform(top_right);
	bottom_left = control_transform_without_offset.xform(bottom_left);

	Transform2D canvas_transform = CanvasItemEditor::get_singleton()->get_canvas_transform();
	top_left = canvas_transform.xform(top_left);
	bottom_right = canvas_transform.xform(bottom_right);
	top_right = canvas_transform.xform(top_right);
	bottom_left = canvas_transform.xform(bottom_left);

	Color color = Color(0.5, 0.5, 0.5, 0.75);
	p_overlay->draw_dashed_line(top_left, top_right, color, 5.0, 4.0);
	p_overlay->draw_dashed_line(top_right, bottom_right, color, 5.0, 4.0);
	p_overlay->draw_dashed_line(bottom_right, bottom_left, color, 5.0, 4.0);
	p_overlay->draw_dashed_line(bottom_left, top_left, color, 5.0, 4.0);
}

ControlOffsetTransformPreview::ControlOffsetTransformPreview(EditorPlugin* p_plugin)
{
	plugin = p_plugin;
}

void ControlEditorPlugin::forward_canvas_draw_over_viewport(Control* p_overlay)
{
	offset_transform_preview->forward_canvas_draw_over_viewport(p_overlay);
}

ControlEditorPlugin::ControlEditorPlugin()
{
	toolbar = memnew(ControlEditorToolbar);
	toolbar->hide();
	add_control_to_container(CONTAINER_CANVAS_EDITOR_MENU, toolbar);

	offset_transform_preview = memnew(ControlOffsetTransformPreview(this));
	EditorNode::get_singleton()->get_gui_base()->add_child(offset_transform_preview);

	Ref<EditorInspectorPluginControl> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}


