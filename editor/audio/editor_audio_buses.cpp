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
	case NOTIFICATION_READY: {
		update_bus();
		set_process(true);
	} break;

	case NOTIFICATION_DRAW: {
		if (is_master) {
			draw_style_box(get_theme_stylebox(SNAME("master"), SNAME("EditorAudioBus")).ptr(),
				Rect2(Vector2(), get_size()));
		}
		else if (has_focus()) {
			draw_style_box(get_theme_stylebox(SNAME("focus"), SNAME("EditorAudioBus")).ptr(),
				Rect2(Vector2(), get_size()));
		}
		else {
			draw_style_box(get_theme_stylebox(SNAME("normal"), SNAME("EditorAudioBus")).ptr(),
				Rect2(Vector2(), get_size()));
		}

		if (get_index() != 0 && hovering_drop) {
			Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			accent.a *= 0.7;
			draw_rect(Rect2(Point2(), get_size()), accent, false);
		}
	} break;

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

void EditorAudioBus::update_bus()
{
	if (updating_bus) {
		return;
	}

	updating_bus = true;

	int index = get_index();

	float db_value = AudioServer::get_singleton()->get_bus_volume_db(index);
	slider->set_value(_scaled_db_to_normalized_volume(db_value));
	track_name->set_text(AudioServer::get_singleton()->get_bus_name(index));
	if (is_master) {
		track_name->set_editable(false);
	}

	solo->set_pressed(AudioServer::get_singleton()->is_bus_solo(index));
	mute->set_pressed(AudioServer::get_singleton()->is_bus_mute(index));
	bypass->set_pressed(AudioServer::get_singleton()->is_bus_bypassing_effects(index));
	// effects..
	effects->clear();

	TreeItem* root = effects->create_item();
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_effect_count(index); i++) {
		Ref<AudioEffect> afx = AudioServer::get_singleton()->get_bus_effect(index, i);

		TreeItem* fx = effects->create_item(root);
		fx->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		fx->set_editable(0, true);
		fx->set_checked(0, AudioServer::get_singleton()->is_bus_effect_enabled(index, i));
		fx->set_text(0, afx->get_name());
	}

	TreeItem* add = effects->create_item(root);
	add->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);
	add->set_editable(0, true);
	add->set_selectable(0, false);
	add->set_text(0, TTR("Add Effect"));

	update_send();

	updating_bus = false;
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

EditorAudioBus::EditorAudioBus(EditorAudioBuses* p_buses, bool p_is_master)
{
	buses = p_buses;
	is_master = p_is_master;

	set_tooltip_text(TTR("Drag & drop to rearrange."));

	VBoxContainer* vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", 4 * EDSCALE);
	add_child(vb);

	set_v_size_flags(SIZE_EXPAND_FILL);

	track_name = memnew(LineEdit);
	track_name->set_accessibility_name(TTRC("Track Name"));

	HBoxContainer* hbc = memnew(HBoxContainer);
	vb->add_child(hbc);
	solo = memnew(Button);
	solo->set_theme_type_variation(SceneStringName(FlatButton));
	solo->set_toggle_mode(true);
	solo->set_tooltip_text(TTR("Solo"));
	solo->set_focus_mode(FOCUS_ACCESSIBILITY);
	hbc->add_child(solo);
	mute = memnew(Button);
	mute->set_theme_type_variation(SceneStringName(FlatButton));
	mute->set_toggle_mode(true);
	mute->set_tooltip_text(TTR("Mute"));
	mute->set_focus_mode(FOCUS_ACCESSIBILITY);
	hbc->add_child(mute);
	bypass = memnew(Button);
	bypass->set_theme_type_variation(SceneStringName(FlatButton));
	bypass->set_toggle_mode(true);
	bypass->set_tooltip_text(TTR("Bypass"));
	bypass->set_focus_mode(FOCUS_ACCESSIBILITY);
	hbc->add_child(bypass);
	hbc->add_spacer();

	Ref<StyleBoxEmpty> sbempty = memnew(StyleBoxEmpty);
	for (int i = 0; i < hbc->get_child_count(); i++) {
		Ref<StyleBoxFlat> sbflat = memnew(StyleBoxFlat);
		sbflat->set_content_margin_all(0);
		sbflat->set_bg_color(Color(1, 1, 1, 0));
		sbflat->set_border_width(Side::SIDE_BOTTOM, Math::round(3 * EDSCALE));
	}

	HSeparator* separator = memnew(HSeparator);
	separator->set_mouse_filter(MOUSE_FILTER_PASS);
	vb->add_child(separator);

	Control* spacer_top = memnew(Control);
	spacer_top->set_custom_minimum_size(Size2(0, 6 * EDSCALE));
	vb->add_child(spacer_top);

	HBoxContainer* hb = memnew(HBoxContainer);
	vb->add_child(hb);

	Control* spacer_bottom = memnew(Control);
	spacer_bottom->set_custom_minimum_size(Size2(0, 2 * EDSCALE));
	vb->add_child(spacer_bottom);

	slider = memnew(VSlider);
	slider->set_min(0.0);
	slider->set_max(1.0);
	slider->set_step(0.0001);
	slider->set_clip_contents(false);
	slider->set_accessibility_name(TTRC("Volume"));

	audio_value_preview_box = memnew(Panel);
	slider->add_child(audio_value_preview_box);
	audio_value_preview_box->set_as_top_level(true);
	audio_value_preview_box->set_mouse_filter(MOUSE_FILTER_PASS);

	HBoxContainer* audioprev_hbc = memnew(HBoxContainer);
	audioprev_hbc->set_v_size_flags(SIZE_EXPAND_FILL);
	audioprev_hbc->set_h_size_flags(SIZE_EXPAND_FILL);
	audio_value_preview_box->add_child(audioprev_hbc);

	audio_value_preview_label = memnew(Label);
	audio_value_preview_label->set_focus_mode(FOCUS_ACCESSIBILITY);
	audio_value_preview_label->set_v_size_flags(SIZE_EXPAND_FILL);
	audio_value_preview_label->set_h_size_flags(SIZE_EXPAND_FILL);
	audio_value_preview_label->set_mouse_filter(MOUSE_FILTER_PASS);
	audioprev_hbc->add_child(audio_value_preview_label);

	preview_timer = memnew(Timer);
	preview_timer->set_wait_time(0.8f);
	preview_timer->set_one_shot(true);
	add_child(preview_timer);

	hb->add_child(slider);

	active_bus_texture = memnew(GradientTexture2D);
	active_gradient = memnew(Gradient);
	active_gradient->set_offsets(gradient_offsets);
	active_gradient->set_colors(active_gradient_colors);
	active_bus_texture->set_gradient(active_gradient);
	active_bus_texture->set_width(vu_width * EDSCALE);
	active_bus_texture->set_height(vu_height * EDSCALE);
	active_bus_texture->set_fill_from(Vector2(0.0, 1.0));
	active_bus_texture->set_fill_to(Vector2(0.0, 0.0));
	inactive_bus_texture = memnew(GradientTexture2D);
	inactive_gradient = memnew(Gradient);
	inactive_gradient->set_offsets(gradient_offsets);
	inactive_gradient->set_colors(inactive_gradient_colors);
	inactive_bus_texture->set_gradient(inactive_gradient);
	inactive_bus_texture->set_width(vu_width * EDSCALE);
	inactive_bus_texture->set_height(vu_height * EDSCALE);
	inactive_bus_texture->set_fill_from(Vector2(0.0, 1.0));
	inactive_bus_texture->set_fill_to(Vector2(0.0, 0.0));

	peak_indicator_stylebox_l = memnew(StyleBoxFlat);
	peak_indicator_stylebox_l->set_bg_color(Color(1.0, 1.0, 1.0, 0.75));
	peak_indicator_stylebox_r = memnew(StyleBoxFlat);
	peak_indicator_stylebox_r->set_bg_color(Color(1.0, 1.0, 1.0, 0.75));

	peak_indicator_range = vu_height * EDSCALE - 2.0;

	cc = 0;
	for (int i = 0; i < CHANNELS_MAX; i++) {
		channel[i].vu_l = memnew(TextureProgressBar);
		channel[i].vu_l->set_fill_mode(TextureProgressBar::FILL_BOTTOM_TO_TOP);
		channel[i].vu_l->set_custom_minimum_size(Size2(vu_width, vu_height) * EDSCALE);
		channel[i].vu_l->set_progress_texture(active_bus_texture);
		channel[i].vu_l->set_under_texture(active_bus_texture);
		channel[i].vu_l->set_over_texture(inactive_bus_texture);
		hb->add_child(channel[i].vu_l);
		channel[i].vu_l->set_min(0);
		channel[i].vu_l->set_max(1);
		channel[i].vu_l->set_step(0.0001);
		channel[i].vu_l->set_accessibility_name(vformat(TTR("Channel %d, Left VU"), i));

		channel[i].peak_indicator_l = memnew(Panel);
		channel[i].peak_indicator_l->set_custom_minimum_size(Size2(vu_width * EDSCALE, 2.0));
		channel[i].peak_indicator_l->add_theme_style_override(
			SceneStringName(panel), peak_indicator_stylebox_l.ptr());
		channel[i].vu_l->add_child(channel[i].peak_indicator_l);
		channel[i].peak_indicator_l->set_position(Point2(0.0, peak_indicator_range));
		channel[i].peak_timer_l = memnew(Timer);
		channel[i].peak_timer_l->set_wait_time(1.5f);
		channel[i].peak_timer_l->set_one_shot(true);
		channel[i].peak_timer_l->set_paused(true);
		channel[i].peak_indicator_l->add_child(channel[i].peak_timer_l);

		channel[i].vu_r = memnew(TextureProgressBar);
		channel[i].vu_r->set_fill_mode(TextureProgressBar::FILL_BOTTOM_TO_TOP);
		channel[i].vu_r->set_custom_minimum_size(Size2(vu_width, vu_height) * EDSCALE);
		channel[i].vu_r->set_progress_texture(active_bus_texture);
		channel[i].vu_r->set_under_texture(active_bus_texture);
		channel[i].vu_r->set_over_texture(inactive_bus_texture);
		hb->add_child(channel[i].vu_r);
		channel[i].vu_r->set_min(0);
		channel[i].vu_r->set_max(1);
		channel[i].vu_r->set_step(0.0001);
		channel[i].vu_r->set_accessibility_name(vformat(TTR("Channel %d, Right VU"), i));

		channel[i].peak_indicator_r = memnew(Panel);
		channel[i].peak_indicator_r->set_custom_minimum_size(Size2(vu_width * EDSCALE, 2.0));
		channel[i].peak_indicator_r->add_theme_style_override(
			SceneStringName(panel), peak_indicator_stylebox_r.ptr());
		channel[i].vu_r->add_child(channel[i].peak_indicator_r);
		channel[i].peak_indicator_r->set_position(Point2(0.0, peak_indicator_range));
		channel[i].peak_timer_r = memnew(Timer);
		channel[i].peak_timer_r->set_wait_time(1.5f);
		channel[i].peak_timer_r->set_one_shot(true);
		channel[i].peak_timer_r->set_paused(true);
		channel[i].peak_indicator_r->add_child(channel[i].peak_timer_r);

		channel[i].peak_l = -100.0f;
		channel[i].peak_r = -100.0f;
	}

	EditorAudioMeterNotches* scale = memnew(EditorAudioMeterNotches);

	for (float db = 6.0f; db >= -80.0f; db -= 6.0f) {
		bool renderNotch = (db >= -6.0f || db == -24.0f || db == -72.0f);
		scale->add_notch(_scaled_db_to_normalized_volume(db), db, renderNotch);
	}
	scale->set_mouse_filter(MOUSE_FILTER_PASS);
	hb->add_child(scale);

	effects = memnew(Tree);
	effects->set_accessibility_name(TTRC("Effects"));
	effects->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	effects->set_hide_root(true);
	effects->set_custom_minimum_size(Size2(0, 80) * EDSCALE);
	effects->set_hide_folding(true);
	effects->set_v_size_flags(SIZE_EXPAND_FILL);
	vb->add_child(effects);
	effects->set_edit_checkbox_cell_only_when_checkbox_is_pressed(true);
	effects->set_allow_rmb_select(true);
	effects->set_focus_mode(FOCUS_CLICK);
	effects->set_allow_reselect(true);
	effects->set_theme_type_variation("EditorAudioBusEffectsTree");

	send = memnew(OptionButton);
	send->set_accessibility_name(TTRC("Send"));
	send->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	send->set_clip_text(true);
	send->set_fit_to_longest_item(false);

	set_focus_mode(FOCUS_CLICK);

	effect_options = memnew(PopupMenu);
	effect_options->set_auto_translate_mode(
		AUTO_TRANSLATE_MODE_DISABLED); // Don't translate class names.
	add_child(effect_options);
	LocalVector<StringName> effect_list;
#ifndef DISABLE_DEPRECATED
	effect_list.erase("AudioEffectLimiter");
#endif
	// TODO Godot 5.0: AudioEffectEQ and AudioEffectFilter should be abstract
	effect_list.erase("AudioEffectEQ"); // Base classes, shouldn't be used directly.
	effect_list.erase("AudioEffectFilter");
	effect_list.sort_custom<StringName::AlphCompare>();
	for (const StringName& E : effect_list) {
		String name = E.string().replace("AudioEffect", "");
		effect_options->add_item(name);
	}

	bus_options = memnew(MenuButton);
	bus_options->set_shortcut_context(this);
	bus_options->set_h_size_flags(SIZE_SHRINK_END);
	bus_options->set_anchor(SIDE_RIGHT, 0.0);
	bus_options->set_tooltip_text(TTR("Bus Options"));
	hbc->add_child(bus_options);

	bus_popup = bus_options->get_popup();
	bus_popup->add_shortcut(ED_SHORTCUT("audio_bus_editor/duplicate_selected_bus",
		TTRC("Duplicate Bus"), KeyModifierMask::CMD_OR_CTRL | Key::D));
	bus_popup->add_shortcut(
		ED_SHORTCUT("audio_bus_editor/delete_selected_bus", TTRC("Delete Bus"), Key::KEY_DELETE));
	bus_popup->set_item_disabled(1, is_master);
	bus_popup->add_item(TTR("Reset Volume"));

	delete_effect_popup = memnew(PopupMenu);
	delete_effect_popup->add_item(TTR("Delete Effect"));
	add_child(delete_effect_popup);
}

void EditorAudioBusDrop::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (hovering_drop) {
			Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			accent.a *= 0.7;
			draw_rect(Rect2(Point2(), get_size()), accent, false);
		}
	} break;
	}
}

void EditorAudioBuses::_update_file_label()
{
	const String filename = ResourceUID::ensure_path(edited_path).get_file();
	file->set_text(filename);
	file->set_tooltip_text(filename);

	if (is_visible_in_tree()) {
		_update_file_label_size();
	}
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
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible_in_tree()) {
			_update_file_label();
		}
	} break;
	}
}

void EditorAudioBuses::_select_layout()
{
	FileSystemDock::get_singleton()->navigate_to_path(ResourceUID::ensure_path(edited_path));
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

EditorAudioBuses::EditorAudioBuses()
{
	set_name(TTRC("Audio"));
	set_icon_name("AudioStreamPlayer");
	set_dock_shortcut(ED_SHORTCUT_AND_COMMAND("bottom_panels/toggle_audio_bottom_panel",
		TTRC("Toggle Audio Dock"), KeyModifierMask::ALT | Key::A));
	set_default_slot(EditorDock::DOCK_SLOT_BOTTOM);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	set_custom_minimum_size(Vector2(150, 150) * EDSCALE);

	VBoxContainer* main_vb = memnew(VBoxContainer);
	add_child(main_vb);

	top_hb = memnew(HBoxContainer);
	main_vb->add_child(top_hb);

	Label* layout_label = memnew(Label(TTRC("Layout:")));
	top_hb->add_child(layout_label);

	file = memnew(Label);
	file->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	file->set_mouse_filter(MOUSE_FILTER_PASS);
	file->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	file->set_h_size_flags(SIZE_EXPAND_FILL);
	top_hb->add_child(file);

	menu = memnew(MenuButton);
	menu->get_popup()->add_item(TTRC("New Layout..."), (int)MenuOption::CREATE);
	menu->get_popup()->add_item(TTRC("Load..."), (int)MenuOption::LOAD);
	menu->get_popup()->add_item(TTRC("Load Default Layout"), (int)MenuOption::LOAD_DEFAULT);
	menu->get_popup()->add_item(TTRC("Save As..."), (int)MenuOption::SAVE_AS);
	top_hb->add_child(menu);

	bus_mc = memnew(MarginContainer);
	bus_mc->set_theme_type_variation("NoBorderPanel");
	bus_mc->set_v_size_flags(SIZE_EXPAND_FILL);
	main_vb->add_child(bus_mc);

	bus_scroll = memnew(ScrollContainer);
	bus_scroll->set_custom_minimum_size(Size2(0, 40 * EDSCALE));
	bus_mc->add_child(bus_scroll);

	HBoxContainer* bus_parent_hb = memnew(HBoxContainer);
	bus_parent_hb->set_v_size_flags(SIZE_EXPAND_FILL);
	bus_scroll->add_child(bus_parent_hb);

	bus_hb = memnew(HBoxContainer);
	bus_parent_hb->add_child(bus_hb);

	PanelContainer* add_bus_container = memnew(PanelContainer);
	add_bus_container->set_theme_type_variation("EditorAudioBusAddBusPanel");
	add_bus_container->set_custom_minimum_size(Vector2(144 * EDSCALE, 0));
	bus_parent_hb->add_child(add_bus_container);

	add = memnew(Button);
	add->set_text(TTRC("Add Bus"));
	add->set_tooltip_text(TTRC("Add a new Audio Bus to this layout."));
	add->set_h_size_flags(SIZE_SHRINK_CENTER);
	add->set_v_size_flags(SIZE_SHRINK_CENTER);
	add_bus_container->add_child(add);

	save_timer = memnew(Timer);
	save_timer->set_wait_time(0.8);
	save_timer->set_one_shot(true);
	main_vb->add_child(save_timer);

	set_v_size_flags(SIZE_EXPAND_FILL);

	file_dialog = memnew(EditorFileDialog);
	List<String> ext;
	ResourceLoader::get_recognized_extensions_for_type("AudioBusLayout", &ext);
	for (const String& E : ext) {
		file_dialog->add_filter("*." + E, TTR("Audio Bus Layout"));
	}
	add_child(file_dialog);
	set_process(true);
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

void EditorAudioMeterNotches::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.notch_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor));

	theme_cache.font = get_theme_font(SceneStringName(font), SNAME("Label"));
	theme_cache.font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
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


