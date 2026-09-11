/**************************************************************************/
/*  quick_settings_dialog.cpp                                             */
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

#include "editor/doc/editor_help.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_properties.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/themes/editor_scale.h"
#include "quick_settings_dialog.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "servers/display/display_server.h"

void QuickSettingsDialog::_add_setting_control(
	const String& p_text, const String& p_setting, Control* p_control)
{
	HBoxContainer* container = memnew(HBoxContainer);
	settings_list->add_child(container);

	Label* label = memnew(SettingLabel(p_text, p_setting));
	label->set_text(p_text);
	label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	label->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	container->add_child(label);

	p_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	container->add_child(p_control);
}

void QuickSettingsDialog::update_size_limits(const Size2& p_max_popup_size)
{
#ifndef ANDROID_ENABLED
	language_option_button->get_popup()->set_max_size(p_max_popup_size);
#endif
}

void QuickSettingsDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		settings_list_panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("quick_settings_panel"), SNAME("ProjectManager")).ptr());

		restart_required_label->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		custom_theme_label->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("font_placeholder_color"), EditorStringName(Editor)));
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
			_update_current_values();
		}
	} break;
	}
}

Control* SettingLabel::make_custom_tooltip(const String& p_text) const
{
	return EditorHelpBitTooltip::make_tooltip(const_cast<SettingLabel*>(this),
		vformat("property|EditorSettings|%s", setting_name), String());
}

SettingLabel::SettingLabel(const String& p_text, const String& p_setting) : Label(p_text)
{
	setting_name = p_setting;
}


