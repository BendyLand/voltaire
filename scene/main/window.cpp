/**************************************************************************/
/*  window.cpp                                                            */
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

#include "window.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/os.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "scene/theme/theme_owner.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_enums.h"

// Editor integration.

int Window::root_layout_direction = 0;

void Window::set_root_layout_direction(int p_root_dir) { root_layout_direction = p_root_dir; }

// Dynamic properties.

Window* Window::focused_window = nullptr;

String Window::get_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return title;
}

String Window::get_displayed_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return displayed_title;
}

Window::WindowInitialPosition Window::get_initial_position() const
{
	ERR_READ_THREAD_GUARD_V(WINDOW_INITIAL_POSITION_ABSOLUTE);
	return initial_position;
}

void Window::set_current_screen(int p_screen)
{
	ERR_MAIN_THREAD_GUARD;

	current_screen = p_screen;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::get_singleton()->window_set_current_screen(p_screen, window_id);
}

int Window::get_current_screen() const
{
	ERR_READ_THREAD_GUARD_V(0);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		current_screen = DisplayServer::get_singleton()->window_get_current_screen(window_id);
	}
	return current_screen;
}

void Window::set_position(const Point2i& p_position)
{
	ERR_MAIN_THREAD_GUARD;

	position = p_position;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_position(p_position, window_id);
	}
}

Point2i Window::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());

	return position;
}

void Window::move_to_center()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		int parent_screen =
			DisplayServer::get_singleton()->window_get_current_screen(get_window_id());
		parent_rect.position = DisplayServer::get_singleton()->screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::get_singleton()->screen_get_size(parent_screen);
	}

	if (parent_rect != Rect2()) {
		set_position(parent_rect.position + (parent_rect.size - get_size()) / 2);
	}
}

void Window::set_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif

	size = p_size;
	_update_window_size();
	_settings_changed();
}

Size2i Window::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return size;
}

void Window::reset_size()
{
	ERR_MAIN_THREAD_GUARD;
	set_size(Size2i());
}

Point2i Window::get_position_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_get_position_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_offset;
		if (theme_cache.embedded_border.is_valid()) {
			border_offset = theme_cache.embedded_border->get_offset();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_offset = border_offset.max(theme_cache.embedded_unfocused_border->get_offset());
		}
		return position - border_offset;
	}
	return position;
}

Size2i Window::get_size_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_get_size_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_size;
		if (theme_cache.embedded_border.is_valid()) {
			border_size = theme_cache.embedded_border->get_minimum_size();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_size =
				border_size.max(theme_cache.embedded_unfocused_border->get_minimum_size());
		}
		return size + border_size;
	}
	return size;
}

Size2i Window::_clamp_limit_size(const Size2i& p_limit_size)
{
	// Force window limits to respect size limitations of rendering server.
	Size2i max_window_size = RS::get_singleton()->get_maximum_viewport_size();
	if (max_window_size != Size2i()) {
		return p_limit_size.clamp(Vector2i(), max_window_size);
	}
	else {
		return p_limit_size.maxi(0);
	}
}

void Window::_validate_limit_size()
{
	// When max_size is invalid, max_size_used falls back to respect size limitations of rendering
	// server.
	bool max_size_valid =
		(max_size.x > 0 || max_size.y > 0) && max_size.x >= min_size.x && max_size.y >= min_size.y;
	max_size_used = max_size_valid ? max_size : RS::get_singleton()->get_maximum_viewport_size();
}

void Window::set_max_size(const Size2i& p_max_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i max_size_clamped = _clamp_limit_size(p_max_size);
	if (max_size == max_size_clamped) {
		return;
	}
	max_size = max_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_max_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return max_size;
}

void Window::set_min_size(const Size2i& p_min_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i min_size_clamped = _clamp_limit_size(p_min_size);
	if (min_size == min_size_clamped) {
		return;
	}
	min_size = min_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_min_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return min_size;
}

void Window::set_mode(Mode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	mode = p_mode;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_mode(
			DisplayServerEnums::WindowMode(p_mode), window_id);
	}
}

Window::Mode Window::get_mode() const
{
	ERR_READ_THREAD_GUARD_V(MODE_WINDOWED);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		mode = (Mode)DisplayServer::get_singleton()->window_get_mode(window_id);
	}
	return mode;
}

void Window::set_fullscreen_shortcut_enabled(bool p_enabled)
{
	fullscreen_shortcut_enabled = p_enabled;
}

bool Window::is_fullscreen_shortcut_enabled() const { return fullscreen_shortcut_enabled; }

void Window::set_flag(Flags p_flag, bool p_enabled)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags[p_flag] = p_enabled;

	if (p_flag == FLAG_TRANSPARENT) {
		set_transparent_background(p_enabled);
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			DisplayServer::get_singleton()->window_set_flag(
				DisplayServerEnums::WindowFlags(p_flag), p_enabled, window_id);
		}
	}
}

bool Window::get_flag(Flags p_flag) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			flags[p_flag] = DisplayServer::get_singleton()->window_get_flag(
				DisplayServerEnums::WindowFlags(p_flag), window_id);
		}
	}
	return flags[p_flag];
}

bool Window::is_popup() const
{
	return (get_flag(Window::FLAG_POPUP) && get_flag(Window::FLAG_NO_FOCUS)) ||
		   get_flag(Window::FLAG_MOUSE_PASSTHROUGH);
}

void Window::set_hdr_output_requested(bool p_requested)
{
	ERR_MAIN_THREAD_GUARD;

	hdr_output_requested = p_requested;

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_request_hdr_output(hdr_output_requested, window_id);
	}

	_update_viewport_for_hdr_output();
}

bool Window::is_hdr_output_requested() const
{
	ERR_READ_THREAD_GUARD_V(false);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		hdr_output_requested =
			DisplayServer::get_singleton()->window_is_hdr_output_requested(window_id);
	}

	return hdr_output_requested;
}

float Window::get_output_max_linear_value() const
{
	ERR_READ_THREAD_GUARD_V(1.0f);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_get_output_max_linear_value(window_id);
	}

	return 1.0f;
}

bool Window::is_maximize_allowed() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_is_maximize_allowed(window_id);
	}
	return true;
}

void Window::request_attention()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_request_attention(window_id);
	}
}

void Window::set_taskbar_progress_value(float p_value)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_taskbar_progress_value(p_value, window_id);
	}
}

void Window::set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_taskbar_progress_state(p_state);
	}
}

#ifndef DISABLE_DEPRECATED
void Window::move_to_foreground()
{
	WARN_DEPRECATED_MSG(
		R"*(The "move_to_foreground()" method is deprecated, use "grab_focus()" instead.)*");
	grab_focus();
}
#endif // DISABLE_DEPRECATED

bool Window::can_draw() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (!is_inside_tree()) {
		return false;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_can_draw(window_id);
	}

	return visible;
}

void Window::set_ime_active(bool p_active)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_ime_active(p_active, window_id);
	}
}

void Window::set_ime_position(const Point2i& p_pos)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_ime_position(p_pos, window_id);
	}
}

bool Window::is_embedded() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return get_embedder() != nullptr;
}

bool Window::is_in_edited_scene_root() const
{
	ERR_READ_THREAD_GUARD_V(false);
#ifdef TOOLS_ENABLED
	return is_part_of_edited_scene();
#else
	return false;
#endif
}

void Window::_update_from_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);
	mode = (Mode)DisplayServer::get_singleton()->window_get_mode(window_id);
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = DisplayServer::get_singleton()->window_get_flag(
			DisplayServerEnums::WindowFlags(i), window_id);
	}
}

void Window::_clear_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);

	bool had_focus = has_focus();

	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		float win_scale = DisplayServer::get_singleton()->window_get_scale(window_id);

		Size2i adjusted_size = Size2i(size.width / win_scale, size.height / win_scale);
		Size2i adjusted_pos = Size2i(position.x / win_scale, position.y / win_scale);

		_rect_changed_callback(Rect2i(adjusted_pos, adjusted_size));
	}

	if (transient_parent && transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_set_transient(
			window_id, DisplayServerEnums::INVALID_WINDOW_ID);
	}

	for (const Window* E : transient_children) {
		if (E->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::get_singleton()->window_set_transient(
				E->window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
	}

	_update_from_window();

	DisplayServer::get_singleton()->delete_sub_window(window_id);
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;

	// If closing window was focused and has a parent, return focus.
	if (had_focus && transient_parent) {
		transient_parent->grab_focus();
	}

	_update_viewport_size();
	RS::get_singleton()->viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);

	if (transient && transient_to_focused) {
		_clear_transient();
	}
}

void Window::_rect_changed_callback(const Rect2i& p_callback)
{
	// we must always accept this as the truth
	if (size == p_callback.size && position == p_callback.position) {
		return;
	}

	if (position != p_callback.position) {
		position = p_callback.position;
		_propagate_window_notification(this, NOTIFICATION_WM_POSITION_CHANGED);
	}

	if (size != p_callback.size) {
		size = p_callback.size;
		_update_viewport_size();
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
		!DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		Vector2 sz_out =
			DisplayServer::get_singleton()->window_get_size_with_decorations(window_id);
		Vector2 pos_out =
			DisplayServer::get_singleton()->window_get_position_with_decorations(window_id);
		Vector2 sz_in = DisplayServer::get_singleton()->window_get_size(window_id);
		Vector2 pos_in = DisplayServer::get_singleton()->window_get_position(window_id);
		AccessibilityServer::get_singleton()->set_window_rect(
			window_id, Rect2(pos_out, sz_out), Rect2(pos_in, sz_in));
	}
	queue_accessibility_update();
}

void Window::update_mouse_cursor_state()
{
	ERR_MAIN_THREAD_GUARD;
	// Update states based on mouse cursor position.
	// This includes updated mouse_enter or mouse_exit signals or the current mouse cursor shape.
	// These details are set in Viewport::_gui_input_event. To instantly
	// see the changes in the viewport, we need to trigger a mouse motion event.
	// This function should be called whenever scene tree changes affect the mouse cursor.
	Ref<InputEventMouseMotion> mm;
	Vector2 pos = get_mouse_position();
	Transform2D xform = get_global_canvas_transform().affine_inverse();
	mm.instantiate();
	mm->set_position(pos);
	mm->set_global_position(xform.xform(pos));
	mm->set_device(InputEvent::DEVICE_ID_INTERNAL);
	push_input(mm.ptr(), true);
}

void Window::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void Window::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

void Window::_accessibility_activate()
{
	_accessibility_notify_enter(this);
	if (!get_embedder()) {
		AccessibilityServer::get_singleton()->window_activation_completed(get_window_id());
	}
}

void Window::_accessibility_deactivate()
{
	_accessibility_notify_exit(this);
	if (!get_embedder()) {
		AccessibilityServer::get_singleton()->window_deactivation_completed(get_window_id());
	}
}

void Window::_clear_transient()
{
	if (transient_parent) {
		if (transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
			window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::get_singleton()->window_set_transient(
				window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
		transient_parent->transient_children.erase(this);
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
		transient_parent = nullptr;
	}
}

void Window::_set_transient_exclusive_child(bool p_clear_invalid)
{
	if (exclusive && visible && is_inside_tree()) {
		if (!is_in_edited_scene_root()) {
			// Transient parent has another exclusive child.
			if (transient_parent->exclusive_child && transient_parent->exclusive_child != this) {
				ERR_PRINT(vformat("Attempting to make child window exclusive, but the parent "
								  "window already has another exclusive child. This window, "
								  "parent window, and current exclusive child window"));
			}
			transient_parent->exclusive_child = this;
		}
	}
	else if (p_clear_invalid) {
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
	}
}

void Window::set_transient(bool p_transient)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient == p_transient) {
		return;
	}

	transient = p_transient;

	if (!is_inside_tree()) {
		return;
	}

	if (transient) {
		if (!transient_to_focused) {
			_make_transient();
		}
	}
	else {
		_clear_transient();
	}
}

bool Window::is_transient() const { return transient; }

void Window::set_transient_to_focused(bool p_transient_to_focused)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient_to_focused == p_transient_to_focused) {
		return;
	}

	transient_to_focused = p_transient_to_focused;
}

bool Window::is_transient_to_focused() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return transient_to_focused;
}

void Window::set_exclusive(bool p_exclusive)
{
	ERR_MAIN_THREAD_GUARD;
	if (exclusive == p_exclusive) {
		return;
	}

	exclusive = p_exclusive;

	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (is_in_edited_scene_root()) {
			DisplayServer::get_singleton()->window_set_exclusive(window_id, false);
		}
		else {
			DisplayServer::get_singleton()->window_set_exclusive(window_id, exclusive);
		}
	}

	if (transient_parent) {
		_set_transient_exclusive_child(true);
	}
}

bool Window::is_exclusive() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return exclusive;
}

bool Window::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible;
}

Size2i Window::_clamp_window_size(const Size2i& p_size)
{
	Size2i window_size_clamped = p_size;
	Size2 minsize = get_clamped_minimum_size();
	window_size_clamped = window_size_clamped.max(minsize);

	if (max_size_used != Size2i()) {
		window_size_clamped = window_size_clamped.min(max_size_used);
	}

	return window_size_clamped;
}

void Window::_update_window_size()
{
	Size2i size_limit = get_clamped_minimum_size();
	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID && keep_title_visible) {
		Size2i title_size =
			DisplayServer::get_singleton()->window_get_title_size(displayed_title, window_id);
		size_limit = size_limit.max(title_size);
	}

	size = size.max(size_limit);

	bool reset_min_first = false;

	if (max_size_used != Size2i()) {
		// Force window size to respect size limitations of max_size_used.
		size = size.min(max_size_used);

		if (size_limit.x > max_size_used.x) {
			size_limit.x = max_size_used.x;
			reset_min_first = true;
		}
		if (size_limit.y > max_size_used.y) {
			size_limit.y = max_size_used.y;
			reset_min_first = true;
		}
	}

	if (embedder) {
		size = size.maxi(1);

		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		// When main window embedded in the editor, we can't resize the main window.
		if (window_id != DisplayServerEnums::MAIN_WINDOW_ID ||
			!Engine::get_singleton()->is_embedded_in_editor()) {
			if (reset_min_first && wrap_controls) {
				// Avoid an error if setting max_size to a value between min_size and the previous
				// size_limit.
				DisplayServer::get_singleton()->window_set_min_size(Size2i(), window_id);
			}

			DisplayServer::get_singleton()->window_set_max_size(max_size_used, window_id);
			DisplayServer::get_singleton()->window_set_min_size(size_limit, window_id);
			DisplayServer::get_singleton()->window_set_size(size, window_id);
		}
		else if (Engine::get_singleton()->is_embedded_in_editor()) {
			size = DisplayServer::get_singleton()->window_get_size(window_id); // Reset size.
		}
	}

	// update the viewport
	_update_viewport_size();
}

void Window::set_force_native(bool p_force_native)
{
	if (force_native == p_force_native) {
		return;
	}
	if (is_visible() && !is_in_edited_scene_root()) {
		ERR_FAIL_MSG("Can't change \"force_native\" while a window is displayed. Consider hiding "
					 "window before changing this value.");
	}
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		return;
	}
	force_native = p_force_native;
	if (!is_in_edited_scene_root() && is_inside_tree() &&
		get_tree()->get_root()->is_embedding_subwindows()) {
		set_embedding_subwindows(force_native);
	}
}

bool Window::get_force_native() const { return force_native; }

Viewport* Window::get_embedder() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (force_native &&
		DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS) &&
		!is_in_edited_scene_root()) {
		return nullptr;
	}

	Viewport* vp = get_parent_viewport();

	while (vp) {
		if (vp->is_embedding_subwindows()) {
			return vp;
		}

		if (vp->get_parent()) {
			vp = vp->get_parent()->get_viewport();
		}
		else {
			vp = nullptr;
		}
	}
	return nullptr;
}

RID Window::get_accessibility_element() const
{
	if (!visible || is_part_of_edited_scene()) {
		return RID();
	}
	if (get_embedder() || is_popup()) {
		return Node::get_accessibility_element();
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return AccessibilityServer::get_singleton()->get_window_root(window_id);
	}
	else {
		return RID();
	}
}

RID Window::get_focused_accessibility_element() const
{
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		if (get_child_count() > 0) {
			return get_child(0)->get_focused_accessibility_element(); // Try scene tree root node.
		}
	}
	return Node::get_focused_accessibility_element();
}

String Window::_get_accessibility_name() const
{
	if (accessibility_name.is_empty()) {
		return displayed_title;
	}
	else {
		return accessibility_name;
	}
}

Transform2D Window::get_accessibility_transform() const
{
	if (is_inside_tree() && get_parent()) {
		Transform2D parent_tr = get_parent()->get_accessibility_transform();
		Transform2D window_tr;
		if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
			window_tr.set_origin(position);
		}
		else {
			Window* np = get_non_popup_window();
			if (np) {
				window_tr.set_origin(get_position() - np->get_position());
			}
		}
		window_tr.set_scale(Vector2(1.f, 1.f) * get_content_scale_factor());
		return parent_tr.affine_inverse() * window_tr;
	}
	else {
		return Transform2D();
	}
}

void Window::set_content_scale_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_size.x < 0);
	ERR_FAIL_COND(p_size.y < 0);
	content_scale_size = p_size;
	_update_viewport_size();
}

Size2i Window::get_content_scale_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return content_scale_size;
}

void Window::set_content_scale_mode(ContentScaleMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	content_scale_mode = p_mode;
	_update_viewport_size();
}

Window::ContentScaleMode Window::get_content_scale_mode() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_MODE_DISABLED);
	return content_scale_mode;
}

void Window::set_content_scale_aspect(ContentScaleAspect p_aspect)
{
	ERR_MAIN_THREAD_GUARD;

	content_scale_aspect = p_aspect;
	_update_viewport_size();
}

Window::ContentScaleAspect Window::get_content_scale_aspect() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_ASPECT_IGNORE);
	return content_scale_aspect;
}

void Window::set_content_scale_stretch(ContentScaleStretch p_stretch)
{
	content_scale_stretch = p_stretch;
	_update_viewport_size();
}

Window::ContentScaleStretch Window::get_content_scale_stretch() const
{
	return content_scale_stretch;
}

void Window::set_keep_title_visible(bool p_title_visible)
{
	if (keep_title_visible == p_title_visible) {
		return;
	}
	keep_title_visible = p_title_visible;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		_update_window_size();
	}
}

bool Window::get_keep_title_visible() const { return keep_title_visible; }

void Window::set_content_scale_factor(real_t p_factor)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_factor <= 0);
	content_scale_factor = p_factor;
	_update_viewport_size();
}

real_t Window::get_content_scale_factor() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return content_scale_factor;
}

void Window::set_nonclient_area(const Rect2i& p_rect)
{
	ERR_MAIN_THREAD_GUARD;
	nonclient_area = p_rect;
}

Rect2i Window::get_nonclient_area() const { return nonclient_area; }

DisplayServerEnums::WindowID Window::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	if (get_embedder()) {
#ifdef TOOLS_ENABLED
		if (is_part_of_edited_scene()) {
			return DisplayServerEnums::MAIN_WINDOW_ID;
		}
#endif
		return parent->get_window_id();
	}
	return window_id;
}

void Window::set_mouse_passthrough_polygon(const Vector<Vector2>& p_region)
{
	ERR_MAIN_THREAD_GUARD;
	mpath = p_region;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::get_singleton()->window_set_mouse_passthrough(mpath, window_id);
}

Vector<Vector2> Window::get_mouse_passthrough_polygon() const { return mpath; }

void Window::set_wrap_controls(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	wrap_controls = p_enable;

	if (!is_inside_tree()) {
		return;
	}

	if (updating_child_controls) {
		_update_child_controls();
	}
	else {
		_update_window_size();
	}
}

bool Window::is_wrapping_controls() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return wrap_controls;
}

void Window::_update_viewport_for_hdr_output()
{
	// If HDR output is enabled, we need to enable HDR 2D rendering as well.
	// This is required to get the correct dynamic range for the final output.
	// We only need to do this if the viewport is not already set up for HDR 2D rendering.

	if (!is_using_hdr_2d()) {
		RS::get_singleton()->viewport_set_use_hdr_2d(viewport, hdr_output_requested);
	}
}

void Window::_update_child_controls()
{
	if (!updating_child_controls) {
		return;
	}

	_update_window_size();

	updating_child_controls = false;
}

bool Window::_can_consume_input_events() const { return exclusive_child == nullptr; }

void Window::_window_input_text(const String& p_text, bool p_emit_signal)
{
	_push_text_input(p_text, p_emit_signal);
}

Viewport* Window::get_parent_viewport() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (get_parent()) {
		return get_parent()->get_viewport();
	}
	else {
		return nullptr;
	}
}

Window* Window::get_non_popup_window() const
{
	Window* w = const_cast<Window*>(this);
	while (w && w->is_popup()) {
		w = w->get_parent_visible_window();
	}
	return w;
}

void Window::popup_on_parent(const Rect2i& p_parent_rect)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(
		window_id == DisplayServerEnums::MAIN_WINDOW_ID, "Can't popup the main window.");

	if (!is_embedded()) {
		Window* window = get_parent_visible_window();

		if (!window) {
			popup(p_parent_rect);
		}
		else {
			popup(Rect2i(window->get_position() + p_parent_rect.position, p_parent_rect.size));
		}
	}
	else {
		popup(p_parent_rect);
	}
}

bool Window::_try_parent_dialog(Node* p_from_node)
{
	ERR_FAIL_NULL_V(p_from_node, false);
	ERR_FAIL_COND_V_MSG(is_inside_tree(), false,
		"Attempting to parent and popup a dialog that already has a parent.");

	Window* w = p_from_node->get_last_exclusive_window();
	if (w && w != this) {
		w->add_child(this);
		return true;
	}
	return false;
}

void Window::popup_exclusive(Node* p_from_node, const Rect2i& p_screen_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup(p_screen_rect);
	}
}

void Window::popup_exclusive_on_parent(Node* p_from_node, const Rect2i& p_parent_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_on_parent(p_parent_rect);
	}
}

void Window::popup_exclusive_centered(Node* p_from_node, const Size2i& p_minsize)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered(p_minsize);
	}
}

void Window::popup_exclusive_centered_ratio(Node* p_from_node, float p_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_ratio(p_ratio);
	}
}

void Window::popup_exclusive_centered_clamped(
	Node* p_from_node, const Size2i& p_size, float p_fallback_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_clamped(p_size, p_fallback_ratio);
	}
}

Rect2i Window::fit_rect_in_parent(Rect2i p_rect, const Rect2i& p_parent_rect) const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	Size2i limit = p_parent_rect.size;
	if (p_rect.position.x + p_rect.size.x > limit.x) {
		p_rect.position.x = limit.x - p_rect.size.x;
	}
	if (p_rect.position.y + p_rect.size.y > limit.y) {
		p_rect.position.y = limit.y - p_rect.size.y;
	}

	if (p_rect.position.x < 0) {
		p_rect.position.x = 0;
	}

	int title_height = get_flag(Window::FLAG_BORDERLESS) ? 0 : theme_cache.title_height;

	if (p_rect.position.y < title_height) {
		p_rect.position.y = title_height;
	}

	return p_rect;
}

Size2 Window::get_contents_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return _get_contents_minimum_size();
}

Size2 Window::get_clamped_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!wrap_controls) {
		return min_size;
	}

	return min_size.max(get_contents_minimum_size() * get_content_scale_factor());
}

void Window::grab_focus()
{
	ERR_MAIN_THREAD_GUARD;
	if (embedder) {
		embedder->_sub_window_grab_focus(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_move_to_foreground(window_id);
	}
}

bool Window::has_focus() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_is_focused(window_id);
	}
	return focused;
}

bool Window::has_focus_or_active_popup() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::get_singleton()->window_is_focused(window_id) ||
			   (DisplayServer::get_singleton()->window_get_active_popup() == window_id);
	}
	return focused;
}

void Window::start_drag()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_start_drag(window_id);
	}
	else if (embedder) {
		embedder->_window_start_drag(this);
	}
}

void Window::start_resize(DisplayServerEnums::WindowResizeEdge p_edge)
{
	ERR_MAIN_THREAD_GUARD;
	if (get_flag(FLAG_RESIZE_DISABLED)) {
		return;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::get_singleton()->window_start_resize(p_edge, window_id);
	}
	else if (embedder) {
		switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_RIGHT, this);
		} break;
		default:
			break;
		}
	}
}

Rect2i Window::get_usable_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	Rect2i parent_rect;
	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		const Window* w = is_visible() ? this : get_parent_visible_window();
		// find a parent that can contain us
		ERR_FAIL_NULL_V(w, Rect2());

		parent_rect = DisplayServer::get_singleton()->screen_get_usable_rect(
			DisplayServer::get_singleton()->window_get_current_screen(w->get_window_id()));
	}
	return parent_rect;
}

void Window::set_accessibility_name(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	if (accessibility_name != p_name) {
		accessibility_name = p_name;
		queue_accessibility_update();
		update_configuration_warnings();
	}
}

void Window::set_accessibility_description(const String& p_description)
{
	ERR_MAIN_THREAD_GUARD;
	if (accessibility_description != p_description) {
		accessibility_description = p_description;
		queue_accessibility_update();
	}
}

void Window::accessibility_announcement(const String& p_announcement)
{
	ERR_MAIN_THREAD_GUARD;
	announcement = p_announcement;
	queue_accessibility_update();
}

void Window::add_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

void Window::remove_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

// Theming.

void Window::set_theme_owner_node(Node* p_node)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_node(p_node);
}

Node* Window::get_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return theme_owner->get_owner_node();
}

bool Window::has_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return theme_owner->has_owner_node();
}

void Window::set_theme_context(ThemeContext* p_context, bool p_propagate)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_context(p_context, p_propagate);
}

Ref<Theme> Window::get_theme() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Theme>());
	return theme;
}

void Window::_theme_changed()
{
	if (is_inside_tree()) {
		theme_owner->propagate_theme_changed(this, this, true, false);
	}
}

void Window::_invalidate_theme_cache()
{
	theme_icon_cache.clear();
	theme_style_cache.clear();
	theme_font_cache.clear();
	theme_font_size_cache.clear();
	theme_color_cache.clear();
	theme_constant_cache.clear();
}

void Window::_update_embedded_window()
{
	if (!updating_embedded_window) {
		return;
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	};

	updating_embedded_window = false;
}

StringName Window::get_theme_type_variation() const
{
	ERR_READ_THREAD_GUARD_V(StringName());
	return theme_type_variation;
}

/// Theme property lookup.

#ifdef TOOLS_ENABLED
Ref<Texture2D> Window::get_editor_theme_icon(const StringName& p_name) const
{
	return get_theme_icon(p_name, SNAME("EditorIcons"));
}
#endif

void Window::add_theme_font_size_override(const StringName& p_name, int p_font_size)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override[p_name] = p_font_size;
	_notify_theme_override_changed();
}

void Window::add_theme_color_override(const StringName& p_name, const Color& p_color)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override[p_name] = p_color;
	_notify_theme_override_changed();
}

void Window::add_theme_constant_override(const StringName& p_name, int p_constant)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override[p_name] = p_constant;
	_notify_theme_override_changed();
}

void Window::remove_theme_font_size_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override.erase(p_name);
	_notify_theme_override_changed();
}

void Window::remove_theme_color_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override.erase(p_name);
	_notify_theme_override_changed();
}

void Window::remove_theme_constant_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override.erase(p_name);
	_notify_theme_override_changed();
}

bool Window::has_theme_icon_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Texture2D>* tex = theme_icon_override.getptr(p_name);
	return tex != nullptr;
}

bool Window::has_theme_stylebox_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<StyleBox>* style = theme_style_override.getptr(p_name);
	return style != nullptr;
}

bool Window::has_theme_font_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Font>* font = theme_font_override.getptr(p_name);
	return font != nullptr;
}

bool Window::has_theme_font_size_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* font_size = theme_font_size_override.getptr(p_name);
	return font_size != nullptr;
}

bool Window::has_theme_color_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Color* color = theme_color_override.getptr(p_name);
	return color != nullptr;
}

bool Window::has_theme_constant_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* constant = theme_constant_override.getptr(p_name);
	return constant != nullptr;
}

/// Default theme properties.

float Window::get_theme_default_base_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_base_scale();
}

Ref<Font> Window::get_theme_default_font() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Font>());
	return theme_owner->get_theme_default_font();
}

int Window::get_theme_default_font_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_font_size();
}

/// Bulk actions.

void Window::begin_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	bulk_theme_override = true;
}

void Window::end_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!bulk_theme_override);

	bulk_theme_override = false;
	_notify_theme_override_changed();
}

//

Rect2i Window::get_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2i());
	if (is_embedded()) {
		// viewport
		Node* n = get_parent();
		ERR_FAIL_NULL_V(n, Rect2i());
		Viewport* p = n->get_viewport();
		ERR_FAIL_NULL_V(p, Rect2i());

		return p->get_visible_rect();
	}
	else {
		int x = get_position().x;
		int closest_dist = 0x7FFFFFFF;
		Rect2i closest_rect;
		for (int i = 0; i < DisplayServer::get_singleton()->get_screen_count(); i++) {
			Rect2i s(DisplayServer::get_singleton()->screen_get_position(i),
				DisplayServer::get_singleton()->screen_get_size(i));
			int d;
			if (x >= s.position.x && x < s.size.x) {
				// contained
				closest_rect = s;
				break;
			}
			else if (x < s.position.x) {
				d = s.position.x - x;
			}
			else {
				d = x - (s.position.x + s.size.x);
			}

			if (d < closest_dist) {
				closest_dist = d;
				closest_rect = s;
			}
		}
		return closest_rect;
	}
}

void Window::set_clamp_to_embedder(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	clamp_to_embedder = p_enable;
}

bool Window::is_clamped_to_embedder() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return clamp_to_embedder;
}

void Window::set_unparent_when_invisible(bool p_unparent) { unparent_when_invisible = p_unparent; }

void Window::set_layout_direction(Window::LayoutDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_direction, LAYOUT_DIRECTION_MAX);

	layout_dir = p_direction;
	propagate_notification(Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
}

Window::LayoutDirection Window::get_layout_direction() const
{
	ERR_READ_THREAD_GUARD_V(LAYOUT_DIRECTION_INHERITED);
	return layout_dir;
}

#ifndef DISABLE_DEPRECATED

void Window::set_use_font_oversampling(bool p_oversampling)
{
	Viewport::set_use_oversampling(p_oversampling);
}

bool Window::is_using_font_oversampling() const { return Viewport::is_using_oversampling(); }

void Window::set_auto_translate(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	set_auto_translate_mode(p_enable ? AUTO_TRANSLATE_MODE_ALWAYS : AUTO_TRANSLATE_MODE_DISABLED);
}

bool Window::is_auto_translating() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return can_auto_translate();
}
#endif

Transform2D Window::get_final_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return window_transform * stretch_transform * global_canvas_transform;
}

Transform2D Window::get_screen_transform_internal(bool p_absolute_position) const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	Transform2D embedder_transform;
	if (get_embedder()) {
		embedder_transform.translate_local(get_position());
		embedder_transform =
			get_embedder()->get_screen_transform_internal(p_absolute_position) * embedder_transform;
	}
	else if (p_absolute_position) {
		embedder_transform.translate_local(get_position());
	}
	return embedder_transform * get_final_transform();
}

Transform2D Window::get_popup_base_transform_native() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS)) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform_native() * popup_base_transform;
	}
	return popup_base_transform;
}

Transform2D Window::get_popup_base_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (is_embedding_subwindows()) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform() * popup_base_transform;
	}
	return popup_base_transform;
}

Viewport* Window::get_section_root_viewport() const
{
	if (get_embedder()) {
		return get_embedder()->get_section_root_viewport();
	}
	if (is_inside_tree()) {
		// Native window.
		return SceneTree::get_singleton()->get_root();
	}
	Window* vp = const_cast<Window*>(this);
	return vp;
}

bool Window::is_attached_in_viewport() const { return get_embedder(); }

void Window::_mouse_leave_viewport()
{
	Viewport::_mouse_leave_viewport();
	if (is_embedded()) {
		mouse_in_window = false;
		_propagate_window_notification(this, NOTIFICATION_WM_MOUSE_EXIT);
	}
}

Window::Window()
{
	RenderingServer* rendering_server = RenderingServer::get_singleton();
	if (rendering_server) {
		max_size = rendering_server->get_maximum_viewport_size();
		max_size_used = max_size; // Update max_size_used.
	}

	theme_owner = memnew(ThemeOwner(this));
	RS::get_singleton()->viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);
}


