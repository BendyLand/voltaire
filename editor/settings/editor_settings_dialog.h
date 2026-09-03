/**************************************************************************/
/*  editor_settings_dialog.h                                              */
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

#include "editor/inspector/editor_inspector.h"
#include "scene/gui/dialogs.h"

class CheckButton;
class EditorEventSearchBar;
class EventListenerLineEdit;
class InputEventConfigurationDialog;
class PanelContainer;
class SectionedInspector;
class TabContainer;
class TextureRect;
class Tree;
class TreeItem;

class EditorSettingsDialog : public AcceptDialog
{
	static inline EditorSettingsDialog* singleton = nullptr;

	TabContainer* tabs = nullptr;
	Control* tab_general = nullptr;
	Control* tab_shortcuts = nullptr;

	LineEdit* search_box = nullptr;
	CheckButton* advanced_switch = nullptr;
	SectionedInspector* inspector = nullptr;
	EditorEventSearchBar* shortcut_search_bar = nullptr;

	// Shortcuts
	enum ShortcutButton
	{
		SHORTCUT_ADD,
		SHORTCUT_EDIT,
		SHORTCUT_ERASE,
		SHORTCUT_REVERT
	};

	Tree* shortcuts = nullptr;

	InputEventConfigurationDialog* shortcut_editor = nullptr;

	bool is_editing_action = false;
	String current_edited_identifier;
	int current_event_index = -1;

	Timer* timer = nullptr;

	virtual void cancel_pressed() override;
	virtual void ok_pressed() override;

	void _settings_changed();
	void _settings_property_edited();
	void _settings_save();

	void _notification(int p_what);
	void _update_icons();

	void _event_config_confirmed();
	bool _is_in_project_manager() const;

	void _tabs_tab_changed(int p_tab);
	void _focus_current_search_box();

	void _advanced_toggled(bool p_button_pressed);

	void _update_dynamic_property_hints();
	String _get_shortcut_button_string(const String& p_shortcut_name);

	void _update_shortcuts();
	void _shortcut_cell_double_clicked();
	static void _set_shortcut_input(const String& p_name, Ref<InputEventKey>& p_event);

	static void _undo_redo_callback(void* p_self, const String& p_name);

	void _remove_setting_override(const String& p_setting);

	Label* restart_label = nullptr;
	TextureRect* restart_icon = nullptr;
	PanelContainer* restart_container = nullptr;
	Button* restart_close_button = nullptr;

	void _editor_restart_request();
	void _editor_restart();
	void _editor_restart_close();

protected:
	static void _bind_methods();

public:
	void popup_edit_settings();
	static void update_3d_navigation_preset();
	void set_current_section(const String& p_section);
	void set_advanced_mode_enabled(bool p_enabled);

	static EditorSettingsDialog* get_singleton() { return singleton; }

	EditorSettingsDialog();
	~EditorSettingsDialog();
};

class EditorSettingsPropertyWrapper : public EditorProperty
{
	String property;
	String hint_text;
	uint32_t usage;

	EditorProperty* editor_property = nullptr;

	HBoxContainer* override_container = nullptr;
	TextureRect* override_icon = nullptr;
	EditorProperty* override_editor_property = nullptr;
	Button* goto_button = nullptr;
	Button* remove_button = nullptr;

	void _setup_override_info();
	void _update_override();
	void _create_override();
	void _remove_override();

protected:
	void _notification(int p_what);

public:
	virtual void update_property() override;
};

class EditorSettingsInspectorPlugin : public EditorInspectorPlugin
{
public:
	SectionedInspector* inspector = nullptr;
};


