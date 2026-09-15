/**************************************************************************/
/*  window_wrapper.h                                                      */
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

#include "core/math/rect2.h"
#include "scene/gui/button.h"
#include "scene/gui/margin_container.h"

class HBoxContainer;
class Popup;
class Window;

class WindowWrapper : public MarginContainer
{
	Control* wrapped_control = nullptr;
	MarginContainer* margins = nullptr;
	Window* window = nullptr;

	Panel* window_background = nullptr;

	Ref<Shortcut> enable_shortcut;

	bool override_close_request = false;

	Rect2 _get_default_window_rect() const;
	Node* _get_wrapped_control_parent() const;

protected:
	void _notification(int p_what);

public:
	void set_wrapped_control(
		Control* p_control, const Ref<Shortcut>& p_enable_shortcut = Ref<Shortcut>());
	Control* get_wrapped_control() const;

	bool is_window_available() const;

	bool get_window_enabled() const;

	Rect2i get_window_rect() const;
	int get_window_screen() const;

	void set_window_title(const String& p_title);
	void set_margins_enabled(bool p_enabled);
	Size2 get_margins_size();
	Size2 get_margins_top_left();
	void grab_window_focus();

	void set_override_close_request(bool p_enabled);

	WindowWrapper() = default;
	~WindowWrapper() = default;
};

class ScreenSelect : public Button
{
	Popup* popup = nullptr;
	HBoxContainer* screen_list = nullptr;

	void _show_popup();

public:
	ScreenSelect() = default;
};


