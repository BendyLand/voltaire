/**************************************************************************/
/*  editor_toaster.cpp                                                    */
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

#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_toaster.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"

EditorToaster* EditorToaster::singleton = nullptr;

// This is kind of a workaround because it's hard to keep the VBox anchored to the bottom.
void EditorToaster::_update_vbox_position()
{
	vbox_container->set_size(Vector2());

	Point2 pos = get_global_position();
	Size2 vbox_size = vbox_container->get_size();
	pos.y -= vbox_size.y + 5 * EDSCALE;
	if (!is_layout_rtl()) {
		pos.x = pos.x - vbox_size.x + get_size().x;
	}

	vbox_container->set_position(pos);
}

void EditorToaster::_update_disable_notifications_button()
{
	bool any_visible = false;
	for (KeyValue<Control*, Toast> element : toasts) {
		if (element.key->is_visible()) {
			any_visible = true;
			break;
		}
	}

	if (!any_visible || !vbox_container->is_visible()) {
		disable_notifications_panel->hide();
	}
	else {
		disable_notifications_panel->show();

		Point2 pos = get_global_position();
		int sep = 5 * EDSCALE;
		Size2 disable_panel_size = disable_notifications_panel->get_minimum_size();
		pos.y -= disable_panel_size.y + sep;
		if (is_layout_rtl()) {
			pos.x = pos.x - disable_panel_size.x - sep;
		}
		else {
			pos.x += get_size().x + sep;
		}

		disable_notifications_panel->set_position(pos);
	}
}

void EditorToaster::_draw_button()
{
	bool has_one = false;
	Severity highest_severity = SEVERITY_INFO;
	for (const KeyValue<Control*, Toast>& element : toasts) {
		if (!element.key->is_visible()) {
			continue;
		}
		has_one = true;
		if (element.value.severity > highest_severity) {
			highest_severity = element.value.severity;
		}
	}

	if (!has_one) {
		return;
	}

	Color color;
	real_t button_radius = main_button->get_size().x / 8;
	switch (highest_severity) {
	case SEVERITY_INFO:
		color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		break;
	case SEVERITY_WARNING:
		color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
		break;
	case SEVERITY_ERROR:
		color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
		break;
	default:
		break;
	}
	main_button->draw_circle(Vector2(button_radius * 2, button_radius * 2), button_radius, color);
}

void EditorToaster::_draw_progress(Control* panel)
{
	if (toasts.has(panel) && toasts[panel].remaining_time > 0 && toasts[panel].duration > 0) {
		Ref<StyleBoxFlat> stylebox;
		switch (toasts[panel].severity) {
		case SEVERITY_INFO:
			stylebox = info_panel_style_progress;
			break;
		case SEVERITY_WARNING:
			stylebox = warning_panel_style_progress;
			break;
		case SEVERITY_ERROR:
			stylebox = error_panel_style_progress;
			break;
		default:
			break;
		}

		Size2 size = panel->get_size();
		Size2 progress = size;
		progress.width *=
			MIN(1, Math::remap(toasts[panel].remaining_time, 0, toasts[panel].duration, 0, 2));
		if (is_layout_rtl()) {
			panel->draw_style_box(stylebox.ptr(), Rect2(size - progress, progress));
		}
		else {
			panel->draw_style_box(stylebox.ptr(), Rect2(Vector2(), progress));
		}
	}
}

void EditorToaster::_set_notifications_enabled(bool p_enabled)
{
	vbox_container->set_visible(p_enabled);
	if (p_enabled) {
		main_button->set_button_icon(get_editor_theme_icon(SNAME("Notification")));
	}
	else {
		main_button->set_button_icon(get_editor_theme_icon(SNAME("NotificationDisabled")));
	}
	_update_disable_notifications_button();
}

void EditorToaster::_popup_str(
	const String& p_message, Severity p_severity, const String& p_tooltip)
{
	is_processing_error = true;
	// Check if we already have a popup with the given message.
	Control* control = nullptr;
	for (KeyValue<Control*, Toast> element : toasts) {
		if (element.value.message == p_message && element.value.severity == p_severity &&
			element.value.tooltip == p_tooltip) {
			control = element.key;
			break;
		}
	}

	// Create a new message if needed.
	if (control == nullptr) {
		HBoxContainer* hb = memnew(HBoxContainer);
		hb->add_theme_constant_override("separation", 0);

		Label* label = memnew(Label);
		label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		label->set_focus_mode(FOCUS_ACCESSIBILITY);
		hb->add_child(label);

		Label* count_label = memnew(Label);
		count_label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		hb->add_child(count_label);

		control = popup(hb, p_severity, default_message_duration, p_tooltip);

		Toast& toast = toasts[control];
		toast.message = p_message;
		toast.tooltip = p_tooltip;
		toast.count = 1;
		toast.message_label = label;
		toast.message_count_label = count_label;
	}
	else {
		Toast& toast = toasts[control];
		if (toast.popped) {
			toast.count += 1;
		}
		else {
			toast.count = 1;
		}
		toast.remaining_time = toast.duration;
		toast.popped = true;
		control->show();
		vbox_container->move_child(control, vbox_container->get_child_count());
		_auto_hide_or_free_toasts();
		_update_vbox_position();
		_update_disable_notifications_button();
		main_button->queue_redraw();
	}

	// Retrieve the label back, then update the text.
	Label* message_label = toasts[control].message_label;
	ERR_FAIL_NULL(message_label);
	message_label->set_text(p_message);
	message_label->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	message_label->set_custom_minimum_size(Size2());

	Size2i size = message_label->get_combined_minimum_size();
	int limit_width = get_viewport_rect().size.x / 2; // Limit label size to half the viewport size.
	if (size.x > limit_width) {
		message_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		message_label->set_custom_minimum_size(Size2(limit_width, 0));
	}

	// Retrieve the count label back, then update the text.
	Label* message_count_label = toasts[control].message_count_label;
	if (toasts[control].count == 1) {
		message_count_label->hide();
	}
	else {
		message_count_label->set_text(vformat("(%d)", toasts[control].count));
		message_count_label->show();
	}

	vbox_container->reset_size();

	is_processing_error = false;
	set_process_internal(true);
}

void EditorToaster::_toast_theme_changed(Control* p_control)
{
	ERR_FAIL_COND(!toasts.has(p_control));

	Toast& toast = toasts[p_control];
	if (toast.close_button) {
		toast.close_button->set_button_icon(get_editor_theme_icon(SNAME("Close")));
	}
	if (toast.copy_button) {
		toast.copy_button->set_button_icon(get_editor_theme_icon(SNAME("ActionCopy")));
	}
}

void EditorToaster::close(Control* p_control)
{
	ERR_FAIL_COND(!toasts.has(p_control));
	toasts[p_control].remaining_time = -1.0;
	toasts[p_control].popped = false;
}

void EditorToaster::instant_close(Control* p_control)
{
	close(p_control);
	p_control->set_modulate(Color(1, 1, 1, 0));
}

void EditorToaster::copy(Control* p_control)
{
	ERR_FAIL_COND(!toasts.has(p_control));
	DisplayServer::get_singleton()->clipboard_set(toasts[p_control].message);
}

void EditorToaster::_bind_methods() {}

EditorToaster* EditorToaster::get_singleton() { return singleton; }

EditorToaster::~EditorToaster()
{
	singleton = nullptr;
	remove_error_handler(&eh);
}


