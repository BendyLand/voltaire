/**************************************************************************/
/*  control_editor_plugin.h                                               */
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
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/margin_container.h"

class CheckBox;
class CheckButton;
class EditorSelection;
class GridContainer;
class Label;
class OptionButton;
class PanelContainer;
class PopupPanel;
class Separator;
class TextureRect;

// Inspector controls.
class ControlPositioningWarning : public MarginContainer
{
	Control* control_node = nullptr;

	PanelContainer* bg_panel = nullptr;
	GridContainer* grid = nullptr;
	TextureRect* title_icon = nullptr;
	TextureRect* hint_icon = nullptr;
	Label* title_label = nullptr;
	Label* hint_label = nullptr;
	Control* hint_filler_left = nullptr;
	Control* hint_filler_right = nullptr;

public:
	ControlPositioningWarning() = default;
};

class EditorPropertyAnchorsPreset : public EditorProperty
{
	OptionButton* options = nullptr;

public:
	EditorPropertyAnchorsPreset() = default;
};

class EditorPropertySizeFlags : public EditorProperty
{
	enum FlagPreset
	{
		SIZE_FLAGS_PRESET_FILL,
		SIZE_FLAGS_PRESET_SHRINK_BEGIN,
		SIZE_FLAGS_PRESET_SHRINK_CENTER,
		SIZE_FLAGS_PRESET_SHRINK_END,
		SIZE_FLAGS_PRESET_CUSTOM,
	};

	OptionButton* flag_presets = nullptr;
	CheckBox* flag_expand = nullptr;
	VBoxContainer* flag_options = nullptr;
	Vector<CheckBox*> flag_checks;

	bool vertical = false;

	bool keep_selected_preset = false;

public:
	EditorPropertySizeFlags() = default;
};

class EditorInspectorPluginControl : public EditorInspectorPlugin
{
	bool inside_control_category = false;
};

// Toolbar controls.
class ControlEditorPopupButton : public Button
{
	Ref<Texture2D> arrow_icon;

	PopupPanel* popup_panel = nullptr;
	VBoxContainer* popup_vbox = nullptr;

protected:
	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const override;
	virtual void toggled(bool p_pressed);

	ControlEditorPopupButton() = default;
};

class ControlEditorPresetPicker : public MarginContainer
{
	virtual void _preset_button_pressed(const int p_preset) {}

protected:
	static constexpr int grid_separation = 0;
	HashMap<int, Button*> preset_buttons;

	void _add_separator(BoxContainer* p_box, Separator* p_separator);
};

class AnchorPresetPicker : public ControlEditorPresetPicker
{
public:
	AnchorPresetPicker() = default;
};

class SizeFlagPresetPicker : public ControlEditorPresetPicker
{
	CheckButton* expand_button = nullptr;

	bool vertical = false;

public:
	SizeFlagPresetPicker(bool p_vertical) : vertical(p_vertical) {}
};

class ControlEditorToolbar : public HBoxContainer
{
	EditorSelection* editor_selection = nullptr;

	ControlEditorPopupButton* anchors_button = nullptr;
	ControlEditorPopupButton* containers_button = nullptr;
	Button* anchor_mode_button = nullptr;
	CheckBox* reposition_button = nullptr;

	AnchorPresetPicker* anchors_picker = nullptr;

	SizeFlagPresetPicker* container_h_picker = nullptr;
	SizeFlagPresetPicker* container_v_picker = nullptr;

	bool anchors_mode = false;

	Vector2 _position_to_anchor(const Control* p_control, Vector2 position);

protected:
	static ControlEditorToolbar* singleton;

public:
	bool is_anchors_mode_enabled() { return anchors_mode; }

	static ControlEditorToolbar* get_singleton() { return singleton; }

	ControlEditorToolbar() = default;
};

class ControlOffsetTransformPreview : public Control
{
	friend class ControlEditorPlugin;

	EditorPlugin* plugin = nullptr;
	Control* selected_control = nullptr;

public:
	void forward_canvas_draw_over_viewport(Control* p_overlay) const;

	ControlOffsetTransformPreview(EditorPlugin* p_plugin);
};

class ControlEditorPlugin : public EditorPlugin
{
	ControlEditorToolbar* toolbar = nullptr;
	ControlOffsetTransformPreview* offset_transform_preview = nullptr;

public:
	virtual String get_plugin_name() const override { return "Control"; }

	virtual void forward_canvas_draw_over_viewport(Control* p_overlay) override;

	ControlEditorPlugin() = default;
};

