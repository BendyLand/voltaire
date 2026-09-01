/**************************************************************************/
/*  embedded_process.cpp                                                  */
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
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "embedded_process.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

void EmbeddedProcessBase::_draw()
{
	if (is_embedding_completed()) {
		Rect2 r = get_adjusted_embedded_window_rect(get_rect());
#ifndef MACOS_ENABLED
		r.position -= get_window()->get_position();
#endif
		if (transp_enabled) {
			draw_texture_rect(checkerboard.ptr(), r, true);
		}
		else {
			draw_rect(r, clear_color, true);
		}
	}
	if (is_process_focused() && focus_style_box.is_valid()) {
		Size2 size = get_size();
		Rect2 r = Rect2(Point2(), size);
		focus_style_box->draw(get_canvas_item(), r);
	}
}

void EmbeddedProcessBase::set_window_size(const Size2i& p_window_size)
{
	if (window_size != p_window_size) {
		window_size = p_window_size;
		queue_update_embedded_process();
		queue_redraw();
	}
}

void EmbeddedProcessBase::set_keep_aspect(bool p_keep_aspect)
{
	if (keep_aspect != p_keep_aspect) {
		keep_aspect = p_keep_aspect;
		queue_update_embedded_process();
		queue_redraw();
	}
}

Rect2i EmbeddedProcessBase::get_screen_embedded_window_rect() const
{
	return get_adjusted_embedded_window_rect(get_global_rect());
}

int EmbeddedProcessBase::get_margin_size(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);

	switch (p_side) {
	case SIDE_LEFT:
		return margin_top_left.x;
	case SIDE_RIGHT:
		return margin_bottom_right.x;
	case SIDE_TOP:
		return margin_top_left.y;
	case SIDE_BOTTOM:
		return margin_bottom_right.y;
	}

	return 0;
}

Size2 EmbeddedProcessBase::get_margins_size() const
{
	return margin_top_left + margin_bottom_right;
}

EmbeddedProcessBase::EmbeddedProcessBase() { set_focus_mode(FOCUS_ALL); }

EmbeddedProcessBase::~EmbeddedProcessBase() {}

Rect2i EmbeddedProcess::get_adjusted_embedded_window_rect(const Rect2i& p_rect) const
{
	Rect2i control_rect =
		Rect2i(p_rect.position + margin_top_left, (p_rect.size - get_margins_size()).maxi(1));
	if (window) {
		control_rect.position += window->get_position();
	}
	if (window_size != Size2i()) {
		Rect2i desired_rect;
		if (!keep_aspect && control_rect.size.x >= window_size.x &&
			control_rect.size.y >= window_size.y) {
			// Fixed at the desired size.
			desired_rect.size = window_size;
		}
		else {
			float ratio = MIN((float)control_rect.size.x / window_size.x,
				(float)control_rect.size.y / window_size.y);
			desired_rect.size = Size2i(window_size.x * ratio, window_size.y * ratio).maxi(1);
		}
		desired_rect.position =
			Size2i(control_rect.position.x + ((control_rect.size.x - desired_rect.size.x) / 2),
				control_rect.position.y + ((control_rect.size.y - desired_rect.size.y) / 2));
		return desired_rect;
	}
	else {
		// Stretch, use all the control area.
		return control_rect;
	}
}

bool EmbeddedProcess::is_embedding_in_progress() const { return !timer_embedding->is_stopped(); }

bool EmbeddedProcess::is_embedding_completed() const { return embedding_completed; }

bool EmbeddedProcess::is_process_focused() const
{
	return focused_process_id == current_process_id && has_focus();
}

int EmbeddedProcess::get_embedded_pid() const { return current_process_id; }

void EmbeddedProcess::embed_process(ProcessID p_pid)
{
	if (!window) {
		return;
	}

	ERR_FAIL_COND_MSG(
		!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_WINDOW_EMBEDDING),
		"Embedded process not supported by this display server.");

	if (current_process_id != 0) {
		// Stop embedding the last process.
		OS::get_singleton()->kill(current_process_id);
	}

	reset();

	current_process_id = p_pid;
	start_embedding_time = OS::get_singleton()->get_ticks_msec();
	embedding_grab_focus = has_focus();
	timer_update_embedded_process->start();
	set_process(true);
	set_notify_transform(true);

	// Attempt to embed the process, but if it has just started and the window is not ready yet,
	// we will retry in this case.
	_try_embed_process();
}

void EmbeddedProcess::reset()
{
	if (current_process_id != 0 && embedding_completed) {
		DisplayServer::get_singleton()->remove_embedded_process(current_process_id);
	}
	current_process_id = 0;
	embedding_completed = false;
	start_embedding_time = 0;
	embedding_grab_focus = false;
	reset_timers();
	set_process(false);
	set_notify_transform(false);
	queue_redraw();
}

void EmbeddedProcess::reset_timers()
{
	timer_embedding->stop();
	timer_update_embedded_process->stop();
}

void EmbeddedProcess::request_close()
{
	if (current_process_id != 0 && embedding_completed) {
		DisplayServer::get_singleton()->request_close_embedded_process(current_process_id);
	}
}

bool EmbeddedProcess::_is_embedded_process_updatable()
{
	return window && current_process_id != 0 && embedding_completed;
}

void EmbeddedProcess::queue_update_embedded_process() { updated_embedded_process_queued = true; }

void EmbeddedProcess::_timer_update_embedded_process_timeout()
{
	_check_focused_process_id();
	_check_mouse_over();

	if (!updated_embedded_process_queued) {
		// We need to detect when the control globally changes location or size on the screen.
		// NOTIFICATION_RESIZED and NOTIFICATION_WM_POSITION_CHANGED are not enough to detect
		// resized parent to siblings controls that can affect global position.
		Rect2i new_global_rect = get_global_rect();
		if (last_global_rect != new_global_rect) {
			last_global_rect = new_global_rect;
			queue_update_embedded_process();
		}
	}
}

void EmbeddedProcess::_update_embedded_process()
{
	if (!_is_embedded_process_updatable()) {
		return;
	}

	bool must_grab_focus = false;
	bool focus = has_focus();
	if (last_updated_embedded_process_focused != focus) {
		if (focus) {
			must_grab_focus = true;
		}
		last_updated_embedded_process_focused = focus;
	}

	DisplayServer::get_singleton()->embed_process(window->get_window_id(), current_process_id,
		get_screen_embedded_window_rect(), is_visible_in_tree(), must_grab_focus);
}

void EmbeddedProcess::_timer_embedding_timeout() { _try_embed_process(); }

void EmbeddedProcess::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_PROCESS: {
		if (updated_embedded_process_queued) {
			updated_embedded_process_queued = false;
			_update_embedded_process();
		}
	} break;
	case NOTIFICATION_RESIZED:
	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_WM_POSITION_CHANGED: {
		queue_update_embedded_process();
	} break;
	case NOTIFICATION_APPLICATION_FOCUS_IN: {
		application_has_focus = true;
		last_application_focus_time = OS::get_singleton()->get_ticks_msec();
	} break;
	case NOTIFICATION_APPLICATION_FOCUS_OUT: {
		application_has_focus = false;
	} break;
	case NOTIFICATION_FOCUS_ENTER: {
		queue_update_embedded_process();
	} break;
	}
}

Window* EmbeddedProcess::_get_current_modal_window()
{
	Vector<DisplayServerEnums::WindowID> wl = DisplayServer::get_singleton()->get_window_list();
	for (const DisplayServerEnums::WindowID& window_id : wl) {
		Window* w = Window::get_from_id(window_id);
		if (!w) {
			continue;
		}

		if (w->is_exclusive()) {
			return w;
		}
	}
	return nullptr;
}

EmbeddedProcess::~EmbeddedProcess()
{
	if (current_process_id != 0) {
		// Stop embedding the last process.
		OS::get_singleton()->kill(current_process_id);
		reset();
	}
}


