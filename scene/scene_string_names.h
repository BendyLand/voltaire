/**************************************************************************/
/*  scene_string_names.h                                                  */
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

#pragma once

#include "core/string/ustring.h"

class SceneStrings {
	inline static SceneStrings *singleton = nullptr;

public:
	static void create() { singleton = memnew(SceneStrings); }
	static void free() {
		memdelete(singleton);
		singleton = nullptr;
	}

	_FORCE_INLINE_ static SceneStrings *get_singleton() { return singleton; }

	const String resized = "resized";
	const String draw = "draw";
	const String hidden = "hidden";
	const String visibility_changed = "visibility_changed";

	const String input_event = "input_event";
	const String gui_input = "gui_input";
	const String window_input = "window_input";
	const String nonclient_window_input = "nonclient_window_input";

	const String tree_entered = "tree_entered";
	const String tree_exiting = "tree_exiting";
	const String tree_exited = "tree_exited";
	const String ready = "ready";
	const String _ready = "_ready";

	const String item_rect_changed = "item_rect_changed";
	const String size_flags_changed = "size_flags_changed";
	const String maximum_size_changed = "maximum_size_changed";
	const String minimum_size_changed = "minimum_size_changed";
	const String sleeping_state_changed = "sleeping_state_changed";
	const String node_configuration_warning_changed = "node_configuration_warning_changed";
	const String update = "update";
	const String updated = "updated";

	const String line_separation = "line_separation";
	const String paragraph_separation = "paragraph_separation";
	const String font = "font";
	const String font_size = "font_size";
	const String font_color = "font_color";

	const String mouse_entered = "mouse_entered";
	const String mouse_exited = "mouse_exited";
	const String mouse_shape_entered = "mouse_shape_entered";
	const String mouse_shape_exited = "mouse_shape_exited";
	const String focus_entered = "focus_entered";
	const String focus_exited = "focus_exited";

	const String pre_sort_children = "pre_sort_children";
	const String sort_children = "sort_children";

	const String finished = "finished";
	const String animation_finished = "animation_finished";
	const String animation_changed = "animation_changed";
	const String animation_started = "animation_started";
	const String RESET = "RESET";

	const String pose_updated = "pose_updated";
	const String skeleton_updated = "skeleton_updated";
	const String bone_enabled_changed = "bone_enabled_changed";
	const String show_rest_only_changed = "show_rest_only_changed";

	const String body_shape_entered = "body_shape_entered";
	const String body_entered = "body_entered";
	const String body_shape_exited = "body_shape_exited";
	const String body_exited = "body_exited";

	const String area_shape_entered = "area_shape_entered";
	const String area_shape_exited = "area_shape_exited";

	const String screen_entered = "screen_entered";
	const String screen_exited = "screen_exited";

	const String _spatial_editor_group = "_spatial_editor_group";
	const String _request_gizmo = "_request_gizmo";

	const String offset = "offset";
	const String rotation_mode = "rotation_mode";
	const String rotate = "rotate";
	const String h_offset = "h_offset";
	const String v_offset = "v_offset";

	const String area_entered = "area_entered";
	const String area_exited = "area_exited";

	const String frame_changed = "frame_changed";
	const String texture_changed = "texture_changed";

	const String autoplay = "autoplay";
	const String blend_times = "blend_times";
	const String speed = "speed";

	const String default_ = "default"; // default would conflict with C++ keyword.
	const String output = "output";

	const String Master = "Master"; // Audio bus name.

	const String theme_changed = "theme_changed";
	const String shader = "shader";
	const String shader_overrides_group = "_shader_overrides_group_";
	const String shader_overrides_group_active = "_shader_overrides_group_active_";

	const String _custom_type_script = "_custom_type_script";

	const String pressed = "pressed";
	const String id_pressed = "id_pressed";
	const String toggled = "toggled";
	const String hover = "hover";

	const String panel = "panel";
	const String item_selected = "item_selected";
	const String confirmed = "confirmed";

	const String text_changed = "text_changed";
	const String text_submitted = "text_submitted";
	const String value_changed = "value_changed";

	const String Start = "Start";
	const String End = "End";
	const String state_started = "state_started";
	const String state_finished = "state_finished";

	const String FlatButton = "FlatButton";
};

#define SceneString(m_name) SceneStrings::get_singleton()->m_name
