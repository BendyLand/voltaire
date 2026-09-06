/**************************************************************************/
/*  audio_stream_interactive_editor_plugin.cpp                            */
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

#include "../audio_stream_interactive.h"
#include "audio_stream_interactive_editor_plugin.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/check_box.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/style_box_flat.h"

void AudioStreamInteractiveTransitionEditor::_notification(int p_what)
{
	if (p_what == NOTIFICATION_READY || p_what == NOTIFICATION_THEME_CHANGED) {
		fade_mode->clear();
		fade_mode->add_icon_item(get_editor_theme_icon(SNAME("FadeDisabled")), TTR("Disabled"),
			AudioStreamInteractive::FADE_DISABLED);
		fade_mode->add_icon_item(get_editor_theme_icon(SNAME("FadeIn")), TTR("Fade-In"),
			AudioStreamInteractive::FADE_IN);
		fade_mode->add_icon_item(get_editor_theme_icon(SNAME("FadeOut")), TTR("Fade-Out"),
			AudioStreamInteractive::FADE_OUT);
		fade_mode->add_icon_item(get_editor_theme_icon(SNAME("FadeCross")), TTR("Cross-Fade"),
			AudioStreamInteractive::FADE_CROSS);
		fade_mode->add_icon_item(get_editor_theme_icon(SNAME("AutoPlay")), TTR("Automatic"),
			AudioStreamInteractive::FADE_AUTOMATIC);
	}
}

void AudioStreamInteractiveTransitionEditor::_update_transitions()
{
	if (!is_visible()) {
		return;
	}
	int clip_count = audio_stream_interactive->get_clip_count();
	Color font_color = tree->get_theme_color(SceneStringName(font_color), "Tree");
	Color font_color_default = font_color;
	font_color_default.a *= 0.5;
	Ref<Texture> fade_icons[5] = {get_editor_theme_icon(SNAME("FadeDisabled")),
		get_editor_theme_icon(SNAME("FadeIn")), get_editor_theme_icon(SNAME("FadeOut")),
		get_editor_theme_icon(SNAME("FadeCross")), get_editor_theme_icon(SNAME("AutoPlay"))};
	for (int i = 0; i <= clip_count; i++) {
		for (int j = 0; j <= clip_count; j++) {
			int from = i == clip_count ? AudioStreamInteractive::CLIP_ANY : i;
			int to = j == clip_count ? AudioStreamInteractive::CLIP_ANY : j;

			bool exists = audio_stream_interactive->has_transition(from, to);
			String tooltip;
			Ref<Texture> icon;
			if (!exists) {
				if (audio_stream_interactive->has_transition(
						AudioStreamInteractive::CLIP_ANY, to)) {
					from = AudioStreamInteractive::CLIP_ANY;
					tooltip = vformat(
						TTR(U"Using any clip → %s."), audio_stream_interactive->get_clip_name(to));
				}
				else if (audio_stream_interactive->has_transition(
							   from, AudioStreamInteractive::CLIP_ANY)) {
					to = AudioStreamInteractive::CLIP_ANY;
					tooltip = vformat(TTR(U"Using %s → Any clip."),
						audio_stream_interactive->get_clip_name(from));
				}
				else if (audio_stream_interactive->has_transition(
							   AudioStreamInteractive::CLIP_ANY,
							   AudioStreamInteractive::CLIP_ANY)) {
					from = to = AudioStreamInteractive::CLIP_ANY;
					tooltip = TTR(U"Using all clips → Any clip.");
				}
				else {
					tooltip = TTR("No transition available.");
				}
			}

			String from_time;
			String to_time;
			if (audio_stream_interactive->has_transition(from, to)) {
				icon = fade_icons[audio_stream_interactive->get_transition_fade_mode(from, to)];
				switch (audio_stream_interactive->get_transition_from_time(from, to)) {
				case AudioStreamInteractive::TRANSITION_FROM_TIME_IMMEDIATE: {
					from_time = TTR("Immediate");
				} break;
				case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BEAT: {
					from_time = TTR("Next Beat");
				} break;
				case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BAR: {
					from_time = TTR("Next Bar");
				} break;
				case AudioStreamInteractive::TRANSITION_FROM_TIME_END: {
					from_time = TTR("Clip End");
				} break;
				default: {
				}
				}

				switch (audio_stream_interactive->get_transition_to_time(from, to)) {
				case AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION: {
					to_time = TTR("Same", "Transition Time Position");
				} break;
				case AudioStreamInteractive::TRANSITION_TO_TIME_START: {
					to_time = TTR("Start", "Transition Time Position");
				} break;
				case AudioStreamInteractive::TRANSITION_TO_TIME_PREVIOUS_POSITION: {
					to_time = TTR("Prev", "Transition Time Position");
				} break;
				default: {
				}
				}
			}

			rows[j]->set_icon(i, icon);
			rows[j]->set_text(
				i, to_time.is_empty() ? from_time : vformat(U"%s ⮕ %s", from_time, to_time));
			rows[j]->set_tooltip_text(i, tooltip);
			if (exists) {
				rows[j]->set_custom_color(i, font_color);
				rows[j]->set_icon_modulate(i, Color(1, 1, 1, 1));
			}
			else {
				rows[j]->set_custom_color(i, font_color_default);
				rows[j]->set_icon_modulate(i, Color(1, 1, 1, 0.5));
			}
		}
	}
}

EditorInspectorPluginAudioStreamInteractive::EditorInspectorPluginAudioStreamInteractive()
{
	audio_stream_interactive_transition_editor = memnew(AudioStreamInteractiveTransitionEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(
		audio_stream_interactive_transition_editor);
}

AudioStreamInteractiveEditorPlugin::AudioStreamInteractiveEditorPlugin()
{
	Ref<EditorInspectorPluginAudioStreamInteractive> inspector_plugin;
	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);
}


