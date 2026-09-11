/**************************************************************************/
/*  animation_blend_space_2d_editor.cpp                                   */
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

#include "animation_blend_space_2d_editor.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/gui/button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"

bool AnimationNodeBlendSpace2DEditor::can_edit(const Ref<AnimationNode>& p_node)
{
	Ref<AnimationNodeBlendSpace2D> bs2d = p_node;
	return bs2d.is_valid();
}

StringName AnimationNodeBlendSpace2DEditor::get_blend_position_path() const
{
	StringName path = AnimationTreeEditor::get_singleton()->get_base_path() + "blend_position";
	return path;
}

String AnimationNodeBlendSpace2DEditor::_get_safe_name(
	const Ref<AnimationNodeBlendSpace2D>& p_blend_space, const String& p_name)
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

void AnimationNodeBlendSpace2DEditor::_update_edited_point_pos()
{
	if (updating || blend_space.is_null()) {
		return;
	}

	if (selected_point >= 0 && selected_point < blend_space->get_blend_point_count()) {
		Vector2 pos = blend_space->get_blend_point_position(selected_point);
		if (dragging_selected) {
			pos += drag_ofs;
			if (snap->is_pressed()) {
				pos = pos.snapped(blend_space->get_snap());
			}
			pos = pos.clamp(blend_space->get_min_space(), blend_space->get_max_space());
		}
		updating = true;
		edit_x->set_value(pos.x);
		edit_y->set_value(pos.y);
		index_edit->set_max(blend_space->get_blend_point_count() - 1);
		index_edit->set_value(selected_point);
		index_edit->set_editable(blend_space->get_blend_point_count() > 1 && !read_only);
		updating = false;
	}
}

void AnimationNodeBlendSpace2DEditor::_update_edited_point_name()
{
	if (updating) {
		return;
	}
}

void AnimationNodeBlendSpace2DEditor::_set_selected_point(int p_index)
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

void AnimationNodeBlendSpace2DEditor::_open_editor()
{
	if (selected_point >= 0 && selected_point < blend_space->get_blend_point_count()) {
		Ref<AnimationNode> an = blend_space->get_blend_point_node(selected_point);
		ERR_FAIL_COND(an.is_null());
		AnimationTreeEditor::get_singleton()->enter_editor(
			blend_space->get_blend_point_name(selected_point));
	}
}

void AnimationNodeBlendSpace2DEditor::_index_edit_focus_exited()
{
	index_edit_has_focus = false;
	index_focus_cooldown_timer->start();
}

void AnimationNodeBlendSpace2DEditor::_inline_editor_text_changed(const String& p_text)
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

AnimationNodeBlendSpace2DEditor* AnimationNodeBlendSpace2DEditor::singleton = nullptr;


