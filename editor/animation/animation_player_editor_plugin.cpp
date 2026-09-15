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

String AnimationPlayerEditor::_get_current() const
{
	String current;
	if (animation->get_selected() >= 0 && animation->get_selected() < animation->get_item_count() &&
		!animation->is_item_separator(animation->get_selected())) {
		current = animation->get_item_text(animation->get_selected());
	}
	return current;
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


