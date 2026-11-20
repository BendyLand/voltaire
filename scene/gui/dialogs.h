/**************************************************************************/
/*  dialogs.h                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/main/window.h"

class LineEdit;

struct DialogChrome
{
	Window* parent_visible = nullptr;

	Panel* bg_panel = nullptr;
	Label* message_label = nullptr;
	Control* buttons_hbox = nullptr;
	Button* ok_button = nullptr;
	Button* cancel = nullptr;

	bool has_cancel_button = false;
	bool popped_up = false;
	String ok_text;
	String default_ok_text = "OK";

	bool hide_on_ok = true;
	bool close_on_escape = true;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
		int buttons_separation = 0;
		int buttons_min_width = 0;
		int buttons_min_height = 0;
	} theme_cache;

	inline static bool swap_cancel_ok = false;

	DialogChrome(bool p_with_cancel = false) : has_cancel_button(p_with_cancel) {}

	void init_controls(Window* p_owner);
	bool handle_escape_input(const InputEvent& p_event) const;
	bool should_cancel_on_parent_focus(const Window* p_owner) const;
	void post_popup() { popped_up = true; }
	void update_child_rects(Window* p_owner);
	void update_ok_text(Window* p_owner);
	void set_default_ok_text(Window* p_owner, const String& p_text);

	Label* get_label() const { return message_label; }
	Button* get_ok_button() const { return ok_button; }
	Button* get_cancel_button() const { return cancel; }

	void set_cancel_button_enabled(bool p_enabled) { has_cancel_button = p_enabled; }
	bool is_cancel_button_enabled() const { return has_cancel_button; }

	void set_cancel_button_text(const String& p_cancel_button_text);
	String get_cancel_button_text() const;

	void set_hide_on_ok(bool p_hide) { hide_on_ok = p_hide; }
	bool get_hide_on_ok() const { return hide_on_ok; }

	void set_close_on_escape(bool p_enable) { close_on_escape = p_enable; }
	bool get_close_on_escape() const { return close_on_escape; }

	void set_text(Window* p_owner, const String& p_text);
	String get_text() const;

	void set_autowrap(bool p_autowrap);
	bool has_autowrap() const;

	void set_ok_button_text(Window* p_owner, const String& p_ok_button_text);
	String get_ok_button_text() const { return ok_text; }
};

class Dialog : public Window
{
	DialogChrome chrome;

	void _parent_focused();
	void _text_submitted(const String& p_text);
	void _ok_pressed();
	void _cancel_pressed();
	void _update_child_rects() { chrome.update_child_rects(this); }

protected:
	virtual void _input_from_window(const InputEvent& p_event) override;
	virtual Size2 _get_contents_minimum_size() const override;
	virtual void _post_popup() override;

	void _notification(int p_what);

public:
	Label* get_label() { return chrome.get_label(); }

	Button* get_ok_button() { return chrome.get_ok_button(); }
	Button* get_cancel_button() { return chrome.get_cancel_button(); }

	void set_cancel_button_enabled(bool p_enabled) { chrome.set_cancel_button_enabled(p_enabled); }
	bool is_cancel_button_enabled() const { return chrome.is_cancel_button_enabled(); }

	void set_cancel_button_text(String p_cancel_button_text)
	{
		chrome.set_cancel_button_text(p_cancel_button_text);
	}
	String get_cancel_button_text() const { return chrome.get_cancel_button_text(); }

	static void set_swap_cancel_ok(bool p_swap);

	void set_hide_on_ok(bool p_hide) { chrome.set_hide_on_ok(p_hide); }
	bool get_hide_on_ok() const { return chrome.get_hide_on_ok(); }

	void set_close_on_escape(bool p_enable) { chrome.set_close_on_escape(p_enable); }
	bool get_close_on_escape() const { return chrome.get_close_on_escape(); }

	void set_text(String p_text) { chrome.set_text(this, p_text); }
	String get_text() const { return chrome.get_text(); }

	void set_autowrap(bool p_autowrap) { chrome.set_autowrap(p_autowrap); }
	bool has_autowrap() const { return chrome.has_autowrap(); }

	void set_ok_button_text(String p_ok_button_text)
	{
		chrome.set_ok_button_text(this, p_ok_button_text);
	}
	String get_ok_button_text() const { return chrome.get_ok_button_text(); }

	Dialog(bool p_with_cancel = false);
	~Dialog() = default;
};
