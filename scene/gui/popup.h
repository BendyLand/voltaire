/**************************************************************************/
/*  popup.h                                                               */
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

#include "core/templates/local_vector.h"
#include "scene/main/window.h"

class Panel;

struct PopupTracker
{
	enum HideReason
	{
		HIDE_REASON_NONE,
		HIDE_REASON_CANCELED,  // E.g., because of rupture of UI flow (app unfocused). Includes
							   // closed programmatically.
		HIDE_REASON_UNFOCUSED, // E.g., user clicked outside.
	};

	LocalVector<Window*> visible_parents;
	bool popped_up = false;
	HideReason hide_reason = HIDE_REASON_NONE;

	void configure_window(Window* p_window);
	void initialize_visible_parents(Window* p_window);
	void deinitialize_visible_parents(Window* p_window);
	void post_popup();
	bool handle_cancel_input(const Window* p_window, const InputEvent& p_event);
	bool handle_parent_focused(const Window* p_window);
	void close_pressed(Window* p_window);
	static Rect2i adjust_popup_rect(const Window* p_window);
};

class Popup : public Window
{
public:
	using HideReason = PopupTracker::HideReason;
	static constexpr HideReason HIDE_REASON_NONE = PopupTracker::HIDE_REASON_NONE;
	static constexpr HideReason HIDE_REASON_CANCELED = PopupTracker::HIDE_REASON_CANCELED;
	static constexpr HideReason HIDE_REASON_UNFOCUSED = PopupTracker::HIDE_REASON_UNFOCUSED;

private:
	PopupTracker tracker;
	bool is_panel = false;
	Panel* panel = nullptr;
	bool close_on_accept = false;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
	} theme_cache;

	mutable Rect2i pre_popup_rect;

	void _update_shadow_offsets() const;
	void _update_child_rects() const;

protected:
	void _close_pressed();
	void _parent_focused();
	virtual Rect2i _popup_adjust_rect() const override;
	virtual void _input_from_window(const InputEvent& p_event) override;
	virtual void _post_popup() override;
	virtual Size2 _get_contents_minimum_size() const override;

	void _notification(int p_what);

public:
	HideReason get_hide_reason() const { return tracker.hide_reason; }

	void set_panel_enabled(bool p_enabled);
	bool is_panel_enabled() const { return is_panel; }
	Panel* get_panel() const { return panel; }

	void set_close_on_accept(bool p_enable) { close_on_accept = p_enable; }
	bool is_closing_on_accept() const { return close_on_accept; }

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
#endif

	Popup(bool p_panel = false);
	~Popup();
};
