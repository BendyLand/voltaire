/**************************************************************************/
/*  editor_properties_array_dict.h                                        */
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
#include "editor/translations/editor_locale_dialog.h"

class Button;
class EditorSpinSlider;
class EditorVariantTypePopupMenu;
class MarginContainer;

class EditorPropertyArrayObject : public RefCounted
{
public:
	enum
	{
		NOT_CHANGING_TYPE = -1,
	};
};

class EditorPropertyDictionaryObject : public RefCounted
{
public:
	enum
	{
		NOT_CHANGING_TYPE = -3,
		NEW_KEY_INDEX,
		NEW_VALUE_INDEX,
	};

	String get_label_for_index(int p_index);
	String get_property_name_for_index(int p_index);
	String get_key_name_for_index(int p_index);
};

class EditorPropertyArray : public EditorProperty
{
	struct Slot
	{
		Ref<EditorPropertyArrayObject> object;
		HBoxContainer* container = nullptr;
		int index = -1;
		bool as_id = false;
		EditorProperty* prop = nullptr;
		Button* reorder_button = nullptr;
		Button* edit_button = nullptr;
		Button* remove_button = nullptr;
		void set_index(int p_idx)
		{
			String prop_name = "indices/" + itos(p_idx);
			prop->set_label(itos(p_idx));
			index = p_idx;
		}
	};

	EditorVariantTypePopupMenu* change_type = nullptr;

	bool preview_value = false;
	int page_length = 20;
	int page_index = 0;
	int changing_type_index = EditorPropertyArrayObject::NOT_CHANGING_TYPE;
	Button* edit = nullptr;
	PanelContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;
	EditorSpinSlider* size_slider = nullptr;
	Button* button_add_item = nullptr;
	EditorPaginator* paginator = nullptr;
	String subtype_hint_string;
	LocalVector<Slot> slots;

	Slot reorder_slot;
	int reorder_to_index = -1;
	float reorder_mouse_y_delta = 0.0f;
	void _update_slots_size();

	void _page_changed(int p_page);

	void _reorder_button_gui_input(const Ref<InputEvent>& p_event);
	void _reorder_button_down(int p_index);
	void _reorder_button_up();
	void _create_new_property_slot();

	void _resource_selected(const String& p_path, Ref<Resource> p_resource);

	Node* get_base_node();

protected:
	Ref<EditorPropertyArrayObject> object;

	bool updating = false;
	bool dropping = false;

	void _notification(int p_what);

	virtual void _add_element();
	virtual void _length_changed(double p_page);
	virtual void _edit_pressed();
	virtual void _change_type_menu(int p_index);

	virtual void _remove_pressed(int p_index);

	virtual void _button_draw();
	virtual void _button_add_item_draw();

public:
	void set_preview_value(bool p_preview_value);
	virtual void update_property() override;
	virtual bool is_colored(ColorationMode p_mode) override;
	EditorPropertyArray();
};

class EditorPropertyDictionary : public EditorProperty
{
	struct Slot
	{
		Ref<EditorPropertyDictionaryObject> object;
		HBoxContainer* container = nullptr;
		int index = -1;
		bool as_id = false;
		bool key_as_id = false;
		EditorProperty* prop = nullptr;
		EditorProperty* prop_key = nullptr;
		Button* edit_button = nullptr;
		Button* remove_button = nullptr;
		String prop_name;
		String key_name;
	};

	EditorVariantTypePopupMenu* change_type = nullptr;
	bool updating = false;

	bool preview_value = false;
	Ref<EditorPropertyDictionaryObject> object;
	int page_length = 20;
	int page_index = 0;
	int changing_type_index = EditorPropertyDictionaryObject::NOT_CHANGING_TYPE;
	Button* edit = nullptr;
	PanelContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;
	PanelContainer* add_panel = nullptr;
	Button* button_add_item = nullptr;
	EditorPaginator* paginator = nullptr;
	LocalVector<Slot> slots;
	void _create_new_property_slot(int p_idx);

	void _page_changed(int p_page);
	void _edit_pressed();
	void _resource_selected(const String& p_path, Ref<Resource> p_resource);
	void _change_type_menu(int p_index);

	void _add_key_value();
	void _remove_pressed(int p_slot_index);

	String key_subtype_hint_string;
	String value_subtype_hint_string;

protected:
	void _notification(int p_what);

public:
	void set_preview_value(bool p_preview_value);
	virtual void update_property() override;
	virtual bool is_colored(ColorationMode p_mode) override;
	EditorPropertyDictionary();
};

class EditorPropertyLocalizableString : public EditorProperty
{
	EditorLocaleDialog* locale_select = nullptr;

	bool updating;

	Ref<EditorPropertyDictionaryObject> object;
	int page_length = 20;
	int page_index = 0;
	Button* edit = nullptr;
	MarginContainer* container = nullptr;
	VBoxContainer* property_vbox = nullptr;
	EditorSpinSlider* size_slider = nullptr;
	Button* button_add_item = nullptr;
	EditorPaginator* paginator = nullptr;

	void _page_changed(int p_page);
	void _edit_pressed();

	void _add_locale_popup();
	void _add_locale(const String& p_locale);

public:
	virtual void update_property() override;
	EditorPropertyLocalizableString();
};


