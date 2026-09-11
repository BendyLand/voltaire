/**************************************************************************/
/*  animation_blend_space_1d_editor.cpp                                   */
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

#include "animation_blend_space_1d_editor.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/main/timer.h"

StringName AnimationNodeBlendSpace1DEditor::get_blend_position_path() const
{
	StringName path = AnimationTreeEditor::get_singleton()->get_base_path() + "blend_position";
	return path;
}

void AnimationNodeBlendSpace1DEditor::_update_space()
{
	// edge case when undoing action after editor has changed
	if (updating || blend_space.is_null()) {
		return;
	}

	updating = true;

	max_value->set_value(blend_space->get_max_space());
	min_value->set_value(blend_space->get_min_space());

	sync->select(blend_space->get_sync_mode());
	cyclic_length_value->set_value(blend_space->get_cyclic_length());
	cyclic_length_value->set_visible(
		blend_space->get_sync_mode() == AnimationNodeBlendSpace1D::SYNC_MODE_CYCLIC_CONSTANT);

	interpolation->select(blend_space->get_blend_mode());

	label_value->set_text(blend_space->get_value_label());

	snap_value->set_value(blend_space->get_snap());

	blend_space_draw->queue_redraw();

	updating = false;
}

void AnimationNodeBlendSpace1DEditor::_snap_toggled() { blend_space_draw->queue_redraw(); }

String AnimationNodeBlendSpace1DEditor::_get_safe_name(
	const Ref<AnimationNodeBlendSpace1D>& p_blend_space, const String& p_name)
{
	String final_name = p_name;

	// Append a number suffix if there's a naming conflict.
	int suffix = 1;
	while (p_blend_space->find_blend_point_by_name(final_name) != -1) {
		suffix++;
		final_name = p_name + " " + itos(suffix);
	}

	return final_name;
}

void AnimationNodeBlendSpace1DEditor::_tool_switch(int p_tool)
{
	if (p_tool == 0) {
		tool_erase->show();
		tool_erase_sep->show();
	}
	else {
		tool_erase->hide();
		tool_erase_sep->hide();
	}

	_update_tool_erase();
	blend_space_draw->queue_redraw();
}

void AnimationNodeBlendSpace1DEditor::_update_edited_point_pos()
{
	if (updating || blend_space.is_null()) {
		return;
	}

	if (selected_point >= 0 && selected_point < blend_space->get_blend_point_count()) {
		float pos = blend_space->get_blend_point_position(selected_point);

		if (dragging_selected) {
			pos += drag_ofs.x;

			if (snap->is_pressed()) {
				pos = Math::snapped(pos, blend_space->get_snap());
			}
			pos = CLAMP(pos, blend_space->get_min_space(), blend_space->get_max_space());
		}

		updating = true;
		edit_value->set_value(pos);
		index_edit->set_max(blend_space->get_blend_point_count() - 1);
		index_edit->set_value(selected_point);
		index_edit->set_editable(blend_space->get_blend_point_count() > 1 && !read_only);
		updating = false;
	}
}

void AnimationNodeBlendSpace1DEditor::_update_edited_point_name()
{
	if (updating) {
		return;
	}
}

void AnimationNodeBlendSpace1DEditor::_update_tool_erase()
{
	bool point_valid = selected_point >= 0 && selected_point < blend_space->get_blend_point_count();
	tool_erase->set_disabled(!point_valid || read_only);

	if (point_valid) {
		Ref<AnimationNode> an = blend_space->get_blend_point_node(selected_point);

		if (AnimationTreeEditor::get_singleton()->can_edit(an)) {
			open_editor->show();
			open_editor_sep->show();
		}
		else {
			open_editor->hide();
			open_editor_sep->hide();
		}

		if (!read_only) {
			edit_hb->show();
		}
		else {
			edit_hb->hide();
		}
	}
	else {
		edit_hb->hide();
	}
}

void AnimationNodeBlendSpace1DEditor::_set_selected_point(int p_index)
{
	selected_point = p_index;
	if (blend_space.is_null()) {
		return;
	}
	_update_tool_erase();
	if (p_index != -1) {
		_update_edited_point_pos();
		Ref<AnimationNode> node = blend_space->get_blend_point_node(p_index);
	}
}

void AnimationNodeBlendSpace1DEditor::_open_editor()
{
	if (selected_point >= 0 && selected_point < blend_space->get_blend_point_count()) {
		Ref<AnimationNode> an = blend_space->get_blend_point_node(selected_point);
		ERR_FAIL_COND(an.is_null());
		AnimationTreeEditor::get_singleton()->enter_editor(
			blend_space->get_blend_point_name(selected_point));
	}
}

void AnimationNodeBlendSpace1DEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("GraphBlendSpace")).ptr());
		tool_blend->set_button_icon(get_editor_theme_icon(SNAME("EditPivot")));
		tool_select->set_button_icon(get_editor_theme_icon(SNAME("ToolSelect")));
		tool_create->set_button_icon(get_editor_theme_icon(SNAME("EditKey")));
		tool_erase->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
		snap->set_button_icon(get_editor_theme_icon(SNAME("SnapGrid")));
		open_editor->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
		interpolation->clear();
		interpolation->add_icon_item(
			get_editor_theme_icon(SNAME("TrackContinuous")), TTR("Continuous"), 0);
		interpolation->add_icon_item(
			get_editor_theme_icon(SNAME("TrackDiscrete")), TTR("Discrete"), 1);
		interpolation->add_icon_item(
			get_editor_theme_icon(SNAME("TrackCapture")), TTR("Capture"), 2);
	} break;
	}
}

void AnimationNodeBlendSpace1DEditor::_bind_methods() {}

bool AnimationNodeBlendSpace1DEditor::can_edit(const Ref<AnimationNode>& p_node)
{
	Ref<AnimationNodeBlendSpace1D> b1d = p_node;
	return b1d.is_valid();
}

void AnimationNodeBlendSpace1DEditor::edit(const Ref<AnimationNode>& p_node)
{
	blend_space = p_node;
	read_only = false;

	if (blend_space.is_valid()) {
		read_only = EditorNode::get_singleton()->is_resource_read_only(blend_space);

		_update_space();
	}

	tool_create->set_disabled(read_only);
	edit_value->set_editable(!read_only);
	index_edit->set_editable(!read_only);
	label_value->set_editable(!read_only);
	min_value->set_editable(!read_only);
	max_value->set_editable(!read_only);
	sync->set_disabled(read_only);
	cyclic_length_value->set_editable(!read_only);
	interpolation->set_disabled(read_only);
}

void AnimationNodeBlendSpace1DEditor::_start_inline_edit(int p_point)
{
	if (editing_point != -1 || p_point < 0 || p_point >= blend_space->get_blend_point_count()) {
		return;
	}

	editing_point = p_point;
	_set_selected_point(p_point);

	inline_editor = memnew(LineEdit);
	blend_space_draw->add_child(inline_editor);

	inline_editor->add_theme_color_override(SceneStringName(font_color),
		get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
	inline_editor->add_theme_color_override("font_selected_color", Color::named("white"));
	inline_editor->add_theme_color_override(
		"selection_color", get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
	Ref<StyleBoxEmpty> empty_style = memnew(StyleBoxEmpty);
	empty_style->set_content_margin_all(0);
	inline_editor->add_theme_constant_override("minimum_character_width", 0);
	inline_editor->set_flat(true);

	inline_editor->set_text(blend_space->get_blend_point_name(p_point));
	inline_editor->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	inline_editor->set_expand_to_text_length_enabled(true);

	if (p_point < text_rects.size() && p_point < points.size()) {
		Rect2 text_rect = text_rects[p_point];

		inline_editor_point_x = points[p_point];

		float editor_width = text_rect.size.x;
		inline_editor->set_size(Vector2(editor_width, text_rect.size.y));

		const float pm = POINT_MARGIN * EDSCALE;
		const Size2 s = blend_space_draw->get_size() - Vector2(pm * 2, pm * 2);

		float editor_x = inline_editor_point_x - editor_width / 2.0;
		editor_x = CLAMP(editor_x, pm, pm + s.width - editor_width);
		inline_editor->set_position(Vector2(editor_x, text_rect.position.y - 1 * EDSCALE));
	}

	inline_editor->grab_focus();
	inline_editor->select_all();

	blend_space_draw->queue_redraw();
}

void AnimationNodeBlendSpace1DEditor::_cancel_inline_edit()
{
	if (inline_editor) {
		inline_editor->queue_free();
		inline_editor = nullptr;
	}
	editing_point = -1;
	blend_space_draw->queue_redraw();
}

void AnimationNodeBlendSpace1DEditor::_inline_editor_text_changed(const String& p_text)
{
	if (!inline_editor) {
		return;
	}

	Vector2 editor_size = inline_editor->get_size();
	inline_editor->set_size(Vector2(0, editor_size.y));

	const float pm = POINT_MARGIN * EDSCALE;
	const Size2 s = blend_space_draw->get_size() - Vector2(pm * 2, pm * 2);

	float editor_x = inline_editor_point_x - editor_size.x / 2.0;
	editor_x = CLAMP(editor_x, pm, pm + s.width - editor_size.x);

	inline_editor->set_position(Vector2(editor_x, inline_editor->get_position().y));
}

void AnimationNodeBlendSpace1DEditor::_index_edit_focus_entered()
{
	if (index_focus_cooldown_timer->is_stopped() == false) {
		index_focus_cooldown_timer->stop();
	}
	index_edit_has_focus = true;
	show_indices = true;
	blend_space_draw->queue_redraw();
}

void AnimationNodeBlendSpace1DEditor::_index_edit_focus_exited()
{
	index_edit_has_focus = false;
	index_focus_cooldown_timer->start();
}

void AnimationNodeBlendSpace1DEditor::_index_focus_cooldown_timeout()
{
	if (!index_edit_has_focus) {
		show_indices = false;
		blend_space_draw->queue_redraw();
	}
}

void AnimationNodeBlendSpace1DEditor::_show_indices_with_cooldown()
{
	if (index_focus_cooldown_timer->is_stopped() == false) {
		index_focus_cooldown_timer->stop();
	}
	show_indices = true;
	index_focus_cooldown_timer->start();
	blend_space_draw->queue_redraw();
}

AnimationNodeBlendSpace1DEditor* AnimationNodeBlendSpace1DEditor::singleton = nullptr;

AnimationNodeBlendSpace1DEditor::AnimationNodeBlendSpace1DEditor()
{
	singleton = this;

	HBoxContainer* top_hb = memnew(HBoxContainer);
	add_child(top_hb);

	Ref<ButtonGroup> bg;
	bg.instantiate();

	tool_select = memnew(Button);
	tool_select->set_theme_type_variation(SceneStringName(FlatButton));
	tool_select->set_toggle_mode(true);
	tool_select->set_button_group(bg);
	top_hb->add_child(tool_select);
	tool_select->set_pressed(true);
	tool_select->set_tooltip_text(
		TTR("Select and move points.\nRMB: Create point at position clicked.\nShift+LMB+Drag: Set "
			"the blending position within the space.\nScroll: Increment or decrement index."));

	tool_create = memnew(Button);
	tool_create->set_theme_type_variation(SceneStringName(FlatButton));
	tool_create->set_toggle_mode(true);
	tool_create->set_button_group(bg);
	top_hb->add_child(tool_create);
	tool_create->set_tooltip_text(TTR("Create points."));

	tool_blend = memnew(Button);
	tool_blend->set_theme_type_variation(SceneStringName(FlatButton));
	tool_blend->set_toggle_mode(true);
	tool_blend->set_button_group(bg);
	top_hb->add_child(tool_blend);
	tool_blend->set_tooltip_text(TTR("Set the blending position within the space."));

	tool_erase_sep = memnew(VSeparator);
	top_hb->add_child(tool_erase_sep);
	tool_erase = memnew(Button);
	tool_erase->set_theme_type_variation(SceneStringName(FlatButton));
	top_hb->add_child(tool_erase);
	tool_erase->set_tooltip_text(TTR("Erase points."));

	top_hb->add_child(memnew(VSeparator));

	snap = memnew(Button);
	snap->set_theme_type_variation(SceneStringName(FlatButton));
	snap->set_toggle_mode(true);
	top_hb->add_child(snap);
	snap->set_pressed(true);
	snap->set_tooltip_text(TTR("Enable snap and show grid."));

	snap_value = memnew(SpinBox);
	top_hb->add_child(snap_value);
	snap_value->set_min(0.01);
	snap_value->set_step(0.01);
	snap_value->set_max(1000);
	snap_value->set_accessibility_name(TTRC("Grid Step"));

	top_hb->add_child(memnew(VSeparator));
	top_hb->add_child(memnew(Label(TTR("Sync"))));
	sync = memnew(OptionButton);
	sync->add_item(TTR("None"));
	sync->add_item(TTR("Independent"));
	sync->add_item(TTR("Cyclic Mutable"));
	sync->add_item(TTR("Cyclic Constant"));
	top_hb->add_child(sync);

	cyclic_length_value = memnew(SpinBox);
	cyclic_length_value->set_min(0.0);
	cyclic_length_value->set_max(99.0);
	cyclic_length_value->set_step(0.001);
	cyclic_length_value->set_allow_greater(true);
	cyclic_length_value->set_suffix("s");
	cyclic_length_value->set_accessibility_name(TTRC("Cyclic Length"));
	cyclic_length_value->set_tooltip_text(
		TTR("Cycle length in seconds for cyclic sync. All animations are time-scaled to complete "
			"one cycle in this duration."));
	top_hb->add_child(cyclic_length_value);

	top_hb->add_child(memnew(VSeparator));

	top_hb->add_child(memnew(Label(TTR("Blend"))));
	interpolation = memnew(OptionButton);
	top_hb->add_child(interpolation);

	top_hb->add_spacer();

	edit_hb = memnew(HBoxContainer);
	top_hb->add_child(edit_hb);

	open_editor = memnew(Button);
	edit_hb->add_child(open_editor);
	open_editor->set_text(TTR("Open Editor"));
	open_editor_sep = memnew(VSeparator);
	edit_hb->add_child(open_editor_sep);

	edit_hb->add_child(memnew(Label(TTR("Index"))));
	index_edit = memnew(SpinBox);
	edit_hb->add_child(index_edit);
	index_edit->set_min(0);
	index_edit->set_step(1);
	index_edit->set_allow_greater(false);
	index_edit->set_allow_lesser(false);
	index_edit->set_accessibility_name(TTRC("Blend Point Index"));
	index_edit->set_tooltip_text(TTR("Index of the blend point.\nValues outside of the valid range "
									 "will be clamped to the nearest index."));
	edit_hb->add_child(memnew(VSeparator));

	edit_hb->add_child(memnew(Label(TTR("Position"))));
	edit_value = memnew(SpinBox);
	edit_hb->add_child(edit_value);
	edit_value->set_min(-ABS_MAX);
	edit_value->set_max(ABS_MAX);
	edit_value->set_step(STEP_UNIT);
	edit_value->set_accessibility_name(TTRC("Blend Value"));

	edit_hb->hide();
	open_editor->hide();
	open_editor_sep->hide();

	VBoxContainer* main_vb = memnew(VBoxContainer);
	add_child(main_vb);
	main_vb->set_v_size_flags(SIZE_EXPAND_FILL);

	panel = memnew(PanelContainer);
	panel->set_clip_contents(true);
	main_vb->add_child(panel);
	panel->set_h_size_flags(SIZE_EXPAND_FILL);
	panel->set_v_size_flags(SIZE_EXPAND_FILL);

	blend_space_draw = memnew(Control);
	blend_space_draw->set_focus_mode(FOCUS_ALL);

	blend_space_draw->set_anchors_preset(PRESET_FULL_RECT);

	panel->add_child(blend_space_draw);

	{
		HBoxContainer* bottom_hb = memnew(HBoxContainer);
		main_vb->add_child(bottom_hb);
		bottom_hb->set_h_size_flags(SIZE_EXPAND_FILL);

		min_value = memnew(SpinBox);
		min_value->set_min(-ABS_MAX);
		min_value->set_max(ABS_MAX - STEP_UNIT);
		min_value->set_step(STEP_UNIT);
		min_value->get_line_edit()->set_expand_to_text_length_enabled(true);
		min_value->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		min_value->set_accessibility_name(TTRC("Min"));

		max_value = memnew(SpinBox);
		max_value->set_min(-ABS_MAX + STEP_UNIT);
		max_value->set_max(ABS_MAX);
		max_value->set_step(STEP_UNIT);
		max_value->get_line_edit()->set_expand_to_text_length_enabled(true);
		max_value->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		max_value->set_accessibility_name(TTRC("Max"));

		label_value = memnew(LineEdit);
		label_value->set_expand_to_text_length_enabled(true);
		label_value->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		label_value->set_accessibility_name(TTRC("Value"));

		// now add

		bottom_hb->add_child(min_value);
		bottom_hb->add_spacer();
		bottom_hb->add_child(label_value);
		bottom_hb->add_spacer();
		bottom_hb->add_child(max_value);
	}

	menu = memnew(PopupMenu);
	add_child(menu);

	animations_menu = memnew(PopupMenu);
	animations_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	menu->add_child(animations_menu);

	open_file = memnew(EditorFileDialog);
	add_child(open_file);
	open_file->set_title(TTR("Open Animation Node"));
	open_file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);

	// Create timer for index focus cooldown (1.5 seconds).
	index_focus_cooldown_timer = memnew(Timer);
	add_child(index_focus_cooldown_timer);
	index_focus_cooldown_timer->set_wait_time(1.5);
	index_focus_cooldown_timer->set_one_shot(true);

	set_custom_minimum_size(Size2(0, 150 * EDSCALE));
}


