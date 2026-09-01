/**************************************************************************/
/*  font_config_plugin.h                                                  */
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

#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_properties_array_dict.h"
#include "editor/plugins/editor_plugin.h"

/*************************************************************************/

class EditorPropertyFontMetaOverride : public EditorProperty
{
	MarginContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;

	Button* edit = nullptr;
	PopupMenu* menu = nullptr;
	EditorLocaleDialog* locale_select = nullptr;

	Vector<String> script_codes;

	bool script_editor = false;
	bool updating = false;
	int page_length = 20;
	int page_index = 0;
	EditorPaginator* paginator = nullptr;

protected:
	void _edit_pressed();
	void _page_changed(int p_page);
	void _add_menu();
	void _add_script(int p_option);
	void _add_lang(const String& p_locale);

public:
	virtual void update_property() override;

	EditorPropertyFontMetaOverride(bool p_script);
};

/*************************************************************************/

class EditorPropertyOTVariation : public EditorProperty
{
	MarginContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;

	Button* edit = nullptr;

	bool updating = false;
	int page_length = 20;
	int page_index = 0;
	EditorPaginator* paginator = nullptr;

protected:
	void _edit_pressed();
	void _page_changed(int p_page);

public:
	virtual void update_property() override;

	EditorPropertyOTVariation();
};

/*************************************************************************/

class EditorPropertyOTFeatures : public EditorProperty
{
	enum FeatureGroups
	{
		FGRP_STYLISTIC_SET,
		FGRP_CHARACTER_VARIANT,
		FGRP_CAPITLS,
		FGRP_LIGATURES,
		FGRP_ALTERNATES,
		FGRP_EAL,
		FGRP_EAW,
		FGRP_NUMAL,
		FGRP_CUSTOM,
		FGRP_MAX,
	};

	MarginContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;

	Button* edit = nullptr;
	PopupMenu* menu = nullptr;
	PopupMenu* menu_sub[FGRP_MAX];
	String group_names[FGRP_MAX];

	bool updating = false;
	int page_length = 20;
	int page_index = 0;
	EditorPaginator* paginator = nullptr;

protected:
	void _edit_pressed();
	void _page_changed(int p_page);
	void _add_menu();
	void _add_feature(int p_option);

public:
	virtual void update_property() override;

	EditorPropertyOTFeatures();
};

/*************************************************************************/

class FontPreview : public Control
{
protected:
	void _notification(int p_what);

	Ref<Font> prev_font;

	void _preview_changed();

public:
	virtual Size2 get_minimum_size() const override;

	void set_data(const Ref<Font>& p_f);
};

/*************************************************************************/

class EditorPropertyFontNamesArray : public EditorPropertyArray
{
	PopupMenu* menu = nullptr;

protected:
	virtual void _add_element() override;

	void _add_font(int p_option);

public:
	EditorPropertyFontNamesArray();
};

/*************************************************************************/

class FontEditorPlugin : public EditorPlugin
{
public:
	FontEditorPlugin();

	virtual String get_plugin_name() const override { return "Font"; }
};


