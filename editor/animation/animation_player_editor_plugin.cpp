/**************************************************************************/
/*  animation_player_editor_plugin.cpp                                    */
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

#include "animation_player_editor_plugin.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/animation/animation_tree_editor_plugin.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"	// For onion skinning.
#include "editor/scene/canvas_item_editor_plugin.h" // For onion skinning.
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/animation/animation_tree.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/animation.h"
#include "scene/resources/image_texture.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

///////////////////////////////////

void AnimationPlayerEditor::_play_pressed()
{
	String current = _get_current();

	if (!current.is_empty()) {
		if (current == player->get_assigned_animation()) {
			player->stop(); // So it won't blend with itself.
		}
		ERR_FAIL_COND_EDMSG(!_validate_tracks(player->get_animation(current)),
			"Animation tracks may have any invalid key, abort playing.");
		PackedStringArray markers = track_editor->get_selected_section();
		if (markers.size() == 2) {
			StringName start_marker = markers[0];
			StringName end_marker = markers[1];
			player->play_section_with_markers(current, start_marker, end_marker);
		}
		else {
			player->play(current);
		}
	}

	// unstop
	stop->set_button_icon(pause_icon);
}

void AnimationPlayerEditor::_play_from_pressed()
{
	String current = _get_current();

	if (!current.is_empty()) {
		if (!player->is_valid()) {
			_play_pressed(); // Fallback.
			return;
		}
		double time = player->get_current_animation_position();
		if (current == player->get_assigned_animation() && player->is_playing()) {
			player->clear_caches(); // So it won't blend with itself.
		}
		ERR_FAIL_COND_EDMSG(!_validate_tracks(player->get_animation(current)),
			"Animation tracks may have any invalid key, abort playing.");
		player->seek_internal(time, true, true, true);
		PackedStringArray markers = track_editor->get_selected_section();
		if (markers.size() == 2) {
			StringName start_marker = markers[0];
			StringName end_marker = markers[1];
			player->play_section_with_markers(current, start_marker, end_marker);
		}
		else {
			player->play(current);
		}
	}

	// unstop
	stop->set_button_icon(pause_icon);
}

String AnimationPlayerEditor::_get_current() const
{
	String current;
	if (animation->get_selected() >= 0 && animation->get_selected() < animation->get_item_count() &&
		!animation->is_item_separator(animation->get_selected())) {
		current = animation->get_item_text(animation->get_selected());
	}
	return current;
}

void AnimationPlayerEditor::_play_bw_pressed()
{
	String current = _get_current();
	if (!current.is_empty()) {
		if (current == player->get_assigned_animation()) {
			player->stop(); // So it won't blend with itself.
		}
		ERR_FAIL_COND_EDMSG(!_validate_tracks(player->get_animation(current)),
			"Animation tracks may have any invalid key, abort playing.");
		PackedStringArray markers = track_editor->get_selected_section();
		if (markers.size() == 2) {
			StringName start_marker = markers[0];
			StringName end_marker = markers[1];
			player->play_section_with_markers_backwards(current, start_marker, end_marker);
		}
		else {
			player->play_backwards(current);
		}
	}

	// unstop
	stop->set_button_icon(pause_icon);
}

void AnimationPlayerEditor::_play_bw_from_pressed()
{
	String current = _get_current();

	if (!current.is_empty()) {
		if (!player->is_valid()) {
			_play_bw_pressed(); // Fallback.
			return;
		}
		double time = player->get_current_animation_position();
		if (current == player->get_assigned_animation() && player->is_playing()) {
			player->clear_caches(); // So it won't blend with itself.
		}
		ERR_FAIL_COND_EDMSG(!_validate_tracks(player->get_animation(current)),
			"Animation tracks may have any invalid key, abort playing.");
		player->seek_internal(time, true, true, true);
		PackedStringArray markers = track_editor->get_selected_section();
		if (markers.size() == 2) {
			StringName start_marker = markers[0];
			StringName end_marker = markers[1];
			player->play_section_with_markers_backwards(current, start_marker, end_marker);
		}
		else {
			player->play_backwards(current);
		}
	}

	// unstop
	stop->set_button_icon(pause_icon);
}

void AnimationPlayerEditor::_animation_new()
{
	int count = 1;
	String base = "new_animation";
	String current_library_name = "";
	if (animation->has_selectable_items()) {
		String current_animation_name = animation->get_item_text(animation->get_selected());
		Ref<Animation> current_animation = player->get_animation(current_animation_name);
		if (current_animation.is_valid()) {
			current_library_name = player->find_animation_library(current_animation);
		}
	}
	String attempt_prefix = (current_library_name == "") ? "" : current_library_name + "/";
	while (true) {
		String attempt = base;
		if (count > 1) {
			attempt += vformat("_%d", count);
		}
		if (player->has_animation(attempt_prefix + attempt)) {
			count++;
			continue;
		}
		base = attempt;
		break;
	}

	_update_name_dialog_library_dropdown();

	name_dialog_op = TOOL_NEW_ANIM;
	name_dialog->set_title(TTR("Create New Animation"));
	name_dialog->popup_centered(Size2(300, 90));
	name_title->set_text(TTR("New Animation Name:"));
	name->set_text(base);
	name->select_all();
	name->grab_focus();
}

void AnimationPlayerEditor::_animation_rename()
{
	if (!animation->has_selectable_items()) {
		return;
	}
	int selected = animation->get_selected();
	String selected_name = animation->get_item_text(selected);

	// Remove library prefix if present.
	if (selected_name.contains_char('/')) {
		selected_name = selected_name.get_slicec('/', 1);
	}

	name_dialog->set_title(TTR("Rename Animation"));
	name_title->set_text(TTR("Change Animation Name:"));
	name->set_text(selected_name);
	name_dialog_op = TOOL_RENAME_ANIM;
	name_dialog->popup_centered(Size2(300, 90));
	name->select_all();
	name->grab_focus();
	library->hide();
}

void AnimationPlayerEditor::_animation_remove()
{
	if (!animation->has_selectable_items()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());

	delete_dialog->set_text(vformat(TTR("Delete Animation '%s'?"), current));
	delete_dialog->popup_centered();
}

float AnimationPlayerEditor::_get_editor_step() const
{
	const StringName current = player->get_assigned_animation();
	const Ref<Animation> anim = player->get_animation(current);
	ERR_FAIL_COND_V(anim.is_null(), 0.0);

	float step = track_editor->get_snap_unit();

	// Use more precise snapping when holding Shift
	return Input::get_singleton()->is_key_pressed(Key::SHIFT) ? step * 0.25 : step;
}

void AnimationPlayerEditor::_edit_animation_blend()
{
	if (updating_blends || !animation->has_selectable_items()) {
		return;
	}

	blend_editor.dialog->popup_centered(Size2(400, 400) * EDSCALE);
	_update_animation_blend();
}

void AnimationPlayerEditor::_update_animation_blend()
{
	if (updating_blends || !animation->has_selectable_items()) {
		return;
	}

	blend_editor.tree->clear();

	StringName current = animation->get_item_text(animation->get_selected());

	TreeItem* root = blend_editor.tree->create_item();
	updating_blends = true;

	int i = 0;
	bool anim_found = false;
	blend_editor.next->clear();
	blend_editor.next->add_item("", i);

	for (const StringName& to : player->get_sorted_animation_list()) {
		TreeItem* blend = blend_editor.tree->create_item(root);
		blend->set_editable(0, false);
		blend->set_editable(1, true);
		blend->set_text(0, to);
		blend->set_cell_mode(1, TreeItem::CELL_MODE_RANGE);
		blend->set_range_config(1, 0, 3600, 0.001);
		blend->set_range(1, player->get_blend_time(current, to));

		i++;
		blend_editor.next->add_item(to, i);
		if (to == player->animation_get_next(current)) {
			blend_editor.next->select(i);
			anim_found = true;
		}
	}

	// make sure we reset it else it becomes out of sync and could contain a deleted animation
	if (!anim_found) {
		blend_editor.next->select(0);
		player->animation_set_next(current, blend_editor.next->get_item_text(0));
	}

	updating_blends = false;
}

void AnimationPlayerEditor::_animation_resource_edit()
{
	String current = _get_current();
	if (current != String()) {
		Ref<Animation> anim = player->get_animation(current);
		EditorNode::get_singleton()->edit_resource(anim);
	}
}

void AnimationPlayerEditor::_scale_changed(const String& p_scale)
{
	player->set_speed_scale(p_scale.to_float());
}

void AnimationPlayerEditor::_update_animation()
{
	// the purpose of _update_animation is to reflect the current state
	// of the animation player in the current editor..

	updating = true;

	if (player->is_playing()) {
		stop->set_button_icon(pause_icon);
	}
	else {
		stop->set_button_icon(stop_icon);
	}

	scale->set_text(String::num(player->get_speed_scale(), 2));
	String current = player->get_assigned_animation();

	for (int i = 0; i < animation->get_item_count(); i++) {
		if (animation->get_item_text(i) == current) {
			animation->select(i);
			break;
		}
	}

	updating = false;
}

void AnimationPlayerEditor::_set_controls_disabled(bool p_disabled)
{
	frame->set_editable(!p_disabled);

	stop->set_disabled(p_disabled);
	play->set_disabled(p_disabled);
	play_bw->set_disabled(p_disabled);
	play_bw_from->set_disabled(p_disabled);
	play_from->set_disabled(p_disabled);
	animation->set_disabled(p_disabled);
	autoplay->set_disabled(p_disabled);
	onion_toggle->set_disabled(p_disabled);
	onion_skinning->set_disabled(p_disabled);
}

void AnimationPlayerEditor::_update_animation_list_icons()
{
	for (int i = 0; i < animation->get_item_count(); i++) {
		String anim_name = animation->get_item_text(i);
		if (animation->is_item_disabled(i) || animation->is_item_separator(i)) {
			continue;
		}

		Ref<Texture2D> icon;
		if (anim_name == player->get_autoplay()) {
			if (anim_name == SceneStringName(RESET)) {
				icon = autoplay_reset_icon;
			}
			else {
				icon = autoplay_icon;
			}
		}
		else if (anim_name == SceneStringName(RESET)) {
			icon = reset_icon;
		}

		animation->set_item_icon(i, icon);
	}
}

void AnimationPlayerEditor::_update_name_dialog_library_dropdown()
{
	StringName current_library_name;
	if (animation->has_selectable_items()) {
		String current_animation_name = animation->get_item_text(animation->get_selected());
		Ref<Animation> current_animation = player->get_animation(current_animation_name);
		if (current_animation.is_valid()) {
			current_library_name = player->find_animation_library(current_animation);
		}
	}

	LocalVector<StringName> libraries;
	player->get_animation_library_list(&libraries);
	library->clear();

	int valid_library_count = 0;

	// When [Global] isn't present, but other libraries are, add option of creating [Global].
	int index_offset = 0;
	if (!player->has_animation_library(StringName())) {
		library->add_item(String(TTR("[Global] (create)")));
		if (!libraries.is_empty()) {
			index_offset = 1;
		}
		valid_library_count++;
	}

	int current_lib_id = index_offset; // Don't default to [Global] if it doesn't exist yet.
	for (const StringName& library_name : libraries) {
		if (!EditorNode::get_singleton()->is_resource_read_only(
				player->get_animation_library(library_name))) {
			library->add_item(
				(library_name == StringName()) ? String(TTR("[Global]")) : String(library_name));
			// Default to duplicating into same library.
			if (library_name == current_library_name) {
				current_library_name = library_name;
				current_lib_id = valid_library_count;
			}
			valid_library_count++;
		}
	}

	// If our library name is empty, but we have valid libraries, we can check here to auto assign
	// the first one which isn't a read-only library.
	bool auto_assigning_non_global_library = false;
	if (current_library_name == StringName() && valid_library_count > 0) {
		for (const StringName& library_name : libraries) {
			if (!EditorNode::get_singleton()->is_resource_read_only(
					player->get_animation_library(library_name))) {
				current_library_name = library_name;
				current_lib_id = 0;
				if (library_name != StringName()) {
					auto_assigning_non_global_library = true;
				}
				break;
			}
		}
	}

	if (library->get_item_count() > 0) {
		library->select(current_lib_id);
		if (library->get_item_count() > 1 || auto_assigning_non_global_library) {
			library->show();
			library->set_disabled(
				auto_assigning_non_global_library && library->get_item_count() == 1);
		}
		else {
			library->hide();
		}
	}
}

void AnimationPlayerEditor::_update_playback_tooltips()
{
	stop->set_tooltip_text(TTR("Pause/Stop Animation") + " (" +
						   ED_GET_SHORTCUT("animation_editor/stop_animation")->get_as_text() + ")");
	play->set_tooltip_text(
		TTR("Play Animation from Start") + " (" +
		ED_GET_SHORTCUT("animation_editor/play_animation_from_start")->get_as_text() + ")");
	play_from->set_tooltip_text(TTR("Play Animation") + " (" +
								ED_GET_SHORTCUT("animation_editor/play_animation")->get_as_text() +
								")");
	play_bw_from->set_tooltip_text(
		TTR("Play Animation Backwards") + " (" +
		ED_GET_SHORTCUT("animation_editor/play_animation_backwards")->get_as_text() + ")");
	play_bw->set_tooltip_text(
		TTR("Play Animation Backwards from End") + " (" +
		ED_GET_SHORTCUT("animation_editor/play_animation_from_end")->get_as_text() + ")");
}

void AnimationPlayerEditor::_ensure_dummy_player()
{
	bool dummy_exists = is_dummy && player && original_node;
	if (dummy_exists) {
		if (is_visible()) {
			player->set_active(true);
			original_node->set_editing(true);
		}
		else {
			player->set_active(false);
			original_node->set_editing(false);
		}
	}

	int selected = animation->get_selected();
	autoplay->set_disabled(
		selected != -1 ? (animation->get_item_text(selected).is_empty() ? true : dummy_exists)
					   : true);

	// Show warning.
	if (track_editor) {
		track_editor->show_dummy_player_warning(dummy_exists);
	}
}

void AnimationPlayerEditor::forward_force_draw_over_viewport(Control* p_overlay)
{
	if (!onion.can_overlay) {
		return;
	}

	// Can happen on viewport resize, at least.
	if (!_are_onion_layers_valid()) {
		return;
	}

	RID ci = p_overlay->get_canvas_item();
	Rect2 src_rect = p_overlay->get_global_rect();
	// Re-flip since captures are already flipped.
	src_rect.position.y = onion.capture_size.y - (src_rect.position.y + src_rect.size.y);
	src_rect.size.y *= -1;

	Rect2 dst_rect = Rect2(Point2(), p_overlay->get_size());

	float alpha_step = 1.0 / (onion.steps + 1);

	uint32_t capture_idx = 0;
	if (onion.past) {
		float alpha = 0.0f;
		do {
			alpha += alpha_step;

			if (onion.captures_valid[capture_idx]) {
				RS::get_singleton()->canvas_item_add_texture_rect_region(ci, dst_rect,
					RS::get_singleton()->viewport_get_texture(onion.captures[capture_idx]),
					src_rect, Color(1, 1, 1, alpha));
			}

			capture_idx++;
		} while (capture_idx < onion.steps);
	}
	if (onion.future) {
		float alpha = 1.0f;
		uint32_t base_cidx = capture_idx;
		do {
			alpha -= alpha_step;

			if (onion.captures_valid[capture_idx]) {
				RS::get_singleton()->canvas_item_add_texture_rect_region(ci, dst_rect,
					RS::get_singleton()->viewport_get_texture(onion.captures[capture_idx]),
					src_rect, Color(1, 1, 1, alpha));
			}

			capture_idx++;
		} while (
			capture_idx <
			base_cidx + onion.steps); // In case there's the present capture at the end, skip it.
	}
}

void AnimationPlayerEditor::_animation_duplicate()
{
	if (!animation->has_selectable_items()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());
	Ref<Animation> anim = player->get_animation(current);
	if (anim.is_null()) {
		return;
	}

	int count = 2;
	String new_name = current;
	PackedStringArray split = new_name.split("_");
	int last_index = split.size() - 1;
	if (last_index > 0 && split[last_index].is_valid_int() && split[last_index].to_int() >= 0) {
		count = split[last_index].to_int();
		split.remove_at(last_index);
		new_name = String("_").join(split);
	}
	while (true) {
		String attempt = new_name;
		attempt += vformat("_%d", count);
		if (player->has_animation(attempt)) {
			count++;
			continue;
		}
		new_name = attempt;
		break;
	}

	if (new_name.contains_char('/')) {
		// Discard library prefix.
		new_name = new_name.get_slicec('/', 1);
	}

	_update_name_dialog_library_dropdown();

	name_dialog_op = TOOL_DUPLICATE_ANIM;
	name_dialog->set_title(TTR("Duplicate Animation"));
	// TRANSLATORS: This is a label for the new name field in the "Duplicate Animation" dialog.
	name_title->set_text(TTR("Duplicated Animation Name:"));
	name->set_text(new_name);
	name_dialog->popup_centered(Size2(300, 90));
	name->select_all();
	name->grab_focus();
}

void AnimationPlayerEditor::_animation_finished(const String& p_name) { finishing = true; }

void AnimationPlayerEditor::_animation_key_editor_anim_len_changed(float p_len)
{
	frame->set_max(p_len);
}

void AnimationPlayerEditor::_animation_update_key_frame()
{
	if (player) {
		player->advance(0);
	}
}

void AnimationPlayerEditor::_onion_skinning_menu(int p_option)
{
	PopupMenu* menu = onion_skinning->get_popup();
	int idx = menu->get_item_index(p_option);

	switch (p_option) {
	case ONION_SKINNING_ENABLE: {
		onion.enabled = !onion.enabled;

		if (onion.enabled) {
			if (get_player() && !get_player()->has_animation(SceneStringName(RESET))) {
				EditorNode::get_singleton()->show_warning(
					TTR("Onion skinning requires a RESET animation."));
			}
			_start_onion_skinning(); // It will check for RESET animation anyway.
		}
		else {
			_stop_onion_skinning();
		}

	} break;
	case ONION_SKINNING_PAST: {
		// Ensure at least one of past/future is checked.
		onion.past = onion.future ? !onion.past : true;
		menu->set_item_checked(idx, onion.past);
	} break;
	case ONION_SKINNING_FUTURE: {
		// Ensure at least one of past/future is checked.
		onion.future = onion.past ? !onion.future : true;
		menu->set_item_checked(idx, onion.future);
	} break;
	case ONION_SKINNING_1_STEP: // Fall-through.
	case ONION_SKINNING_2_STEPS:
	case ONION_SKINNING_3_STEPS: {
		onion.steps = (p_option - ONION_SKINNING_1_STEP) + 1;
		int one_frame_idx = menu->get_item_index(ONION_SKINNING_1_STEP);
		for (int i = 0; i <= ONION_SKINNING_LAST_STEPS_OPTION - ONION_SKINNING_1_STEP; i++) {
			menu->set_item_checked(one_frame_idx + i, (int)onion.steps == i + 1);
		}
	} break;
	case ONION_SKINNING_DIFFERENCES_ONLY: {
		onion.differences_only = !onion.differences_only;
		menu->set_item_checked(idx, onion.differences_only);
	} break;
	case ONION_SKINNING_FORCE_WHITE_MODULATE: {
		onion.force_white_modulate = !onion.force_white_modulate;
		menu->set_item_checked(idx, onion.force_white_modulate);
	} break;
	case ONION_SKINNING_INCLUDE_GIZMOS: {
		onion.include_gizmos = !onion.include_gizmos;
		menu->set_item_checked(idx, onion.include_gizmos);
	} break;
	}
}

void AnimationPlayerEditor::_editor_visibility_changed()
{
	if (is_visible() && animation->has_selectable_items()) {
		_start_onion_skinning();
	}
}

bool AnimationPlayerEditor::_are_onion_layers_valid()
{
	ERR_FAIL_COND_V(!onion.past && !onion.future, false);

	Size2 capture_size =
		DisplayServer::get_singleton()->window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
	return onion.captures.size() == onion.get_capture_count() && onion.capture_size == capture_size;
}

void AnimationPlayerEditor::_allocate_onion_layers()
{
	_free_onion_layers();

	int captures = onion.get_capture_count();
	Size2 capture_size =
		DisplayServer::get_singleton()->window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);

	onion.captures.resize(captures);
	onion.captures_valid.resize(captures);

	for (int i = 0; i < captures; i++) {
		bool is_present = onion.differences_only && i == captures - 1;

		// Each capture is a viewport with a canvas item attached that renders a full-size rect with
		// the contents of the main viewport.
		onion.captures[i] = RS::get_singleton()->viewport_create();

		RS::get_singleton()->viewport_set_size(
			onion.captures[i], capture_size.width, capture_size.height);
		RS::get_singleton()->viewport_set_update_mode(
			onion.captures[i], RSE::VIEWPORT_UPDATE_ALWAYS);
		RS::get_singleton()->viewport_set_transparent_background(onion.captures[i], !is_present);
		RS::get_singleton()->viewport_attach_canvas(onion.captures[i], onion.capture.canvas);
	}

	// Reset the capture canvas item to the current root viewport texture (defensive).
	RS::get_singleton()->canvas_item_clear(onion.capture.canvas_item);
	RS::get_singleton()->canvas_item_add_texture_rect(onion.capture.canvas_item,
		Rect2(Point2(), Point2(capture_size.x, -capture_size.y)),
		get_tree()->get_root()->get_texture()->get_rid());

	onion.capture_size = capture_size;
}

void AnimationPlayerEditor::_free_onion_layers()
{
	for (uint32_t i = 0; i < onion.captures.size(); i++) {
		if (onion.captures[i].is_valid()) {
			RS::get_singleton()->free_rid(onion.captures[i]);
		}
	}
	onion.captures.clear();
	onion.captures_valid.clear();
}

void AnimationPlayerEditor::_prepare_onion_layers_1()
{
	// This would be called per viewport and we want to act once only.
	int64_t cur_frame = get_tree()->get_frame();
	if (cur_frame == onion.last_frame) {
		return;
	}

	if (!onion.enabled || !is_visible() || !get_player() ||
		!get_player()->has_animation(SceneStringName(RESET))) {
		_stop_onion_skinning();
		return;
	}

	onion.last_frame = cur_frame;

	// Refresh viewports with no onion layers overlaid.
	onion.can_overlay = false;
	plugin->update_overlays();

	if (player->is_playing()) {
		return;
	}
}

void AnimationPlayerEditor::_prepare_onion_layers_2_step_capture(
	int p_step_offset, uint32_t p_capture_idx)
{
	DEV_ASSERT(p_step_offset != 0);
	DEV_ASSERT(onion.captures_valid[p_capture_idx]);

	RID root_vp = get_tree()->get_root()->get_viewport_rid();
	RS::get_singleton()->viewport_set_active(onion.captures[p_capture_idx], true);
	RS::get_singleton()->viewport_set_parent_viewport(root_vp, onion.captures[p_capture_idx]);
	RS::get_singleton()->draw(false);
	RS::get_singleton()->viewport_set_active(onion.captures[p_capture_idx], false);

	int last_step_offset = onion.future ? onion.steps : 0;
	if (p_step_offset < last_step_offset) {
		_prepare_onion_layers_2_step_prepare(p_step_offset + 1, p_capture_idx + 1);
	}
	else {
		_prepare_onion_layers_2_epilog();
	}
}

void AnimationPlayerEditor::_prepare_onion_layers_2_epilog()
{
	// Restore root viewport.
	RID root_vp = get_tree()->get_root()->get_viewport_rid();
	RS::get_singleton()->viewport_set_parent_viewport(root_vp, RID());
	RS::get_singleton()->viewport_attach_to_screen(
		root_vp, onion.temp.screen_rect, DisplayServerEnums::MAIN_WINDOW_ID);
	RS::get_singleton()->viewport_set_update_mode(root_vp, RSE::VIEWPORT_UPDATE_WHEN_VISIBLE);

	// Restore animation state.
	// Here we're combine the power of seeking back to the original position and
	// restoring the values backup. In most cases they will bring the same value back,
	// but there are cases handled by one that the other can't.
	// Namely:
	// - Seeking won't restore any values that may have been modified by the user
	//   in the node after the last time the AnimationPlayer updated it.
	// - Restoring the backup won't account for values that are not directly involved
	//   in the animation but a consequence of them (e.g., SkeletonModification2DLookAt).
	// FIXME: Since backup of values is based on the reset animation, only values
	//        backed by a proper reset animation will work correctly with onion
	//        skinning and the possibility to restore the values mentioned in the
	//        first point above is gone. Still good enough.
	player->seek_internal(onion.temp.anim_player_position, true, true, false);
	player->restore(onion.temp.anim_values_backup);

	// Update viewports with skin layers overlaid for the actual engine loop render.
	onion.can_overlay = true;
	plugin->update_overlays();
}

void AnimationPlayerEditor::_start_onion_skinning()
{
	if (get_player() && !get_player()->has_animation(SceneStringName(RESET))) {
		onion.enabled = false;
		onion_toggle->set_pressed_no_signal(false);
		return;
	}
}

void AnimationPlayerEditor::_stop_onion_skinning()
{
	_free_onion_layers();

	// Clean up.
	onion.can_overlay = false;
	plugin->update_overlays();
	onion.temp = {};
}

void AnimationPlayerEditor::_pin_pressed()
{
	SceneTreeDock::get_singleton()->get_tree_editor()->update_tree();
}

bool AnimationPlayerEditor::_validate_tracks(const Ref<Animation> p_anim)
{
	bool is_valid = true;
	if (p_anim.is_null()) {
		return true; // There is a problem outside of the animation track.
	}
	int len = p_anim->get_track_count();
	for (int i = 0; i < len; i++) {
		Animation::TrackType ttype = p_anim->track_get_type(i);
		if (ttype == Animation::TYPE_ROTATION_3D) {
			int key_len = p_anim->track_get_key_count(i);
			for (int j = 0; j < key_len; j++) {
				Quaternion q;
				p_anim->rotation_track_get_key(i, j, &q);
				ERR_BREAK_EDMSG(!q.is_normalized(),
					"AnimationPlayer: '" + player->get_name() + "', Animation: '" +
						player->get_current_animation() + "', 3D Rotation Track:  '" +
						String(p_anim->track_get_path(i)) +
						"' contains unnormalized Quaternion key.");
			}
		}
		else if (ttype == Animation::TYPE_VALUE) {
			int key_len = p_anim->track_get_key_count(i);
			if (key_len == 0) {
				continue;
			}
		}
	}
	return is_valid;
}

void AnimationPlayerEditor::_bind_methods() {}

AnimationPlayerEditor* AnimationPlayerEditor::singleton = nullptr;

AnimationPlayer* AnimationPlayerEditor::get_player() const { return player; }

AnimationMixer* AnimationPlayerEditor::get_editing_node() const { return original_node; }

AnimationPlayerEditor::~AnimationPlayerEditor()
{
	_free_onion_layers();
	RS::get_singleton()->free_rid(onion.capture.canvas);
	RS::get_singleton()->free_rid(onion.capture.canvas_item);
	onion.capture = {};
}

void AnimationPlayerEditorPlugin::_clear_dummy_player()
{
	if (!dummy_player) {
		return;
	}
	Node* parent = dummy_player->get_parent();
	dummy_player->queue_free();
	dummy_player = nullptr;
}

AnimationPlayerEditorPlugin::~AnimationPlayerEditorPlugin() {}

// AnimationTrackKeyEditEditorPlugin

AnimationTrackKeyEditEditorPlugin::AnimationTrackKeyEditEditorPlugin()
{
	atk_plugin = memnew(EditorInspectorPluginAnimationTrackKeyEdit);
	EditorInspector::add_inspector_plugin(atk_plugin);
}

AnimationMarkerKeyEditEditorPlugin::AnimationMarkerKeyEditEditorPlugin()
{
	amk_plugin = memnew(EditorInspectorPluginAnimationMarkerKeyEdit);
	EditorInspector::add_inspector_plugin(amk_plugin);
}


