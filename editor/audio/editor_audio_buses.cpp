/**************************************************************************/
/*  editor_audio_buses.cpp                                                */
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
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_audio_buses.h"
#include "scene/gui/box_container.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/resources/font.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/style_box_flat.h"
#include "servers/audio/audio_server.h"

void EditorAudioBus::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		for (int i = 0; i < CHANNELS_MAX; i++) {
			channel[i].peak_l = -100;
			channel[i].peak_r = -100;
		}

		for (int i = 0; i < cc; i++) {
			if (AudioServer::get_singleton()->is_bus_channel_active(get_index(), i)) {
				channel[i].prev_active = false;
			}
			else {
				channel[i].prev_active = true;
			}
		}

		set_process(is_visible_in_tree());
	} break;
	}
}

float EditorAudioBus::_normalized_volume_to_scaled_db(float normalized)
{
	/* There are three different formulas for the conversion from normalized
	 * values to relative decibal values.
	 * One formula is an exponential graph which intends to counteract
	 * the logarithmic nature of human hearing. This is an approximation
	 * of the behavior of a 'logarithmic potentiometer' found on most
	 * musical instruments and also emulated in popular software.
	 * The other two equations are hand-tuned linear tapers that intend to
	 * try to ease the exponential equation in areas where it makes sense.*/

	if (normalized > 0.6f) {
		return 22.22f * normalized - 16.2f;
	}
	else if (normalized < 0.05f) {
		return 830.72 * normalized - 80.0f;
	}
	else {
		return 45.0f * Math::pow(normalized - 1.0, 3);
	}
}

float EditorAudioBus::_scaled_db_to_normalized_volume(float db)
{
	/* Inversion of equations found in _normalized_volume_to_scaled_db.
	 * IMPORTANT: If one function changes, the other must change to reflect it. */
	if (db > -2.88) {
		return (db + 16.2f) / 22.22f;
	}
	else if (db < -38.602f) {
		return (db + 80.00f) / 830.72f;
	}
	else {
		if (db < 0.0) {
			/* To accommodate for NaN on negative numbers for root, we will mirror the
			 * results of the positive db range in order to get the desired numerical
			 * value on the negative side. */
			float positive_x = Math::pow(Math::abs(db) / 45.0f, 1.0f / 3.0f) + 1.0f;
			Vector2 translation = Vector2(1.0f, 0.0f) - Vector2(positive_x, Math::abs(db));
			Vector2 reflected_position = Vector2(1.0, 0.0f) + translation;
			return reflected_position.x;
		}
		else {
			return Math::pow(db / 45.0f, 1.0f / 3.0f) + 1.0f;
		}
	}
}

void EditorAudioBus::_enable_indicator_fall()
{
	for (int i = 0; i < cc; i++) {
		if (channel[i].peak_timer_l->get_time_left() <= 0.0 && !channel[i].indicator_fall_l &&
			Math::snapped(channel[i].peak_indicator_l->get_position().height, 0.0001) <
				peak_indicator_range) {
			channel[i].indicator_fall_l = true;
		}
		if (channel[i].peak_timer_r->get_time_left() <= 0.0 && !channel[i].indicator_fall_r &&
			Math::snapped(channel[i].peak_indicator_r->get_position().height, 0.0001) <
				peak_indicator_range) {
			channel[i].indicator_fall_r = true;
		}
	}
}

void EditorAudioBus::_effect_rmb(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem* item = effects->get_selected();
	if (!item) {
		return;
	}

	delete_effect_popup->set_position(get_screen_position() + get_local_mouse_position());
	delete_effect_popup->reset_size();
	delete_effect_popup->popup();
}

void EditorAudioBuses::_update_file_label_size()
{
	int label_min_width = file->get_minimum_size().x + file->get_character_bounds(0).size.x;
	file->set_custom_minimum_size(Size2(label_min_width, 0));
}

EditorAudioBuses* EditorAudioBuses::register_editor()
{
	EditorAudioBuses* audio_buses = memnew(EditorAudioBuses);
	EditorDockManager::get_singleton()->add_dock(audio_buses);
	return audio_buses;
}

void EditorAudioBuses::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		_rebuild_buses();
	} break;

	case NOTIFICATION_DRAG_END: {
		if (drop_end) {
			bus_hb->remove_child(drop_end);
			drop_end->queue_free();
			drop_end = nullptr;
		}
	} break;
	}
}

void EditorAudioBuses::_save_as_layout()
{
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_title(TTR("Save Audio Bus Layout As..."));
	file_dialog->set_current_path(ResourceUID::ensure_path(edited_path));
	file_dialog->popup_file_dialog();
	new_layout = false;
}

void EditorAudioBuses::_new_layout()
{
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_title(TTR("Location for New Layout..."));
	file_dialog->set_current_path("new_bus_layout.tres");
	file_dialog->popup_file_dialog();
	new_layout = true;
}

void EditorAudioBuses::_menu_option(int p_option)
{
	switch ((MenuOption)p_option) {
	case MenuOption::LOAD: {
		_load_layout();
	} break;

	case MenuOption::SAVE_AS: {
		_save_as_layout();
	} break;

	case MenuOption::LOAD_DEFAULT: {
		_load_default_layout();
	} break;

	case MenuOption::CREATE: {
		_new_layout();
	} break;
	}
}

void EditorAudioBuses::_load_layout()
{
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_title(TTR("Open Audio Bus Layout"));
	file_dialog->set_current_path(ResourceUID::ensure_path(edited_path));
	file_dialog->popup_file_dialog();
	new_layout = false;
}

AudioBusesEditorPlugin::AudioBusesEditorPlugin(EditorAudioBuses* p_node)
{
	audio_bus_editor = p_node;
}

void EditorAudioMeterNotches::add_notch(
	float p_normalized_offset, float p_db_value, bool p_render_value)
{
	notches.push_back(AudioNotch(p_normalized_offset, p_db_value, p_render_value));
}

Size2 EditorAudioMeterNotches::get_minimum_size() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	float font_height = font->get_height(font_size);

	float width = 0;
	float height = 0;

	for (const EditorAudioMeterNotches::AudioNotch& notch : notches) {
		if (notch.render_db_value) {
			char sign = notch.db_value >= 0 ? '+' : '-';
			width = MAX(width,
				font->get_string_size(sign + String::num(Math::abs(notch.db_value), 0) + " dB",
						HORIZONTAL_ALIGNMENT_LEFT, -1, font_size)
					.x);
			height += font_height;
		}
	}
	width += line_length + label_space;

	return Size2(width, height);
}

void EditorAudioMeterNotches::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		_draw_audio_notches();
	} break;
	}
}

void EditorAudioMeterNotches::_draw_audio_notches()
{
	float font_height = theme_cache.font->get_height(theme_cache.font_size);

	for (const AudioNotch& n : notches) {
		draw_line(Vector2(0, (1.0f - n.relative_position) * get_size().y),
			Vector2(line_length * EDSCALE, (1.0f - n.relative_position) * get_size().y),
			theme_cache.notch_color, Math::round(EDSCALE));

		if (n.render_db_value) {
			char sign = n.db_value >= 0 ? '+' : '-';
			draw_string(theme_cache.font.ptr(),
				Vector2((line_length + label_space) * EDSCALE,
					(1.0f - n.relative_position) * get_size().y + (font_height / 4)),
				sign + String::num(Math::abs(n.db_value), 0) + " dB", HORIZONTAL_ALIGNMENT_LEFT, -1,
				theme_cache.font_size, theme_cache.notch_color);
		}
	}
}


