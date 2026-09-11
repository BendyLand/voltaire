/**************************************************************************/
/*  line_edit.cpp                                                         */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/alt_codes.h"
#include "line_edit.compat.inc"
#include "line_edit.h"
#include "scene/gui/label.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

void LineEdit::edit(bool p_hide_focus) { _edit(true, p_hide_focus); }

void LineEdit::_edit(bool p_show_virtual_keyboard, bool p_hide_focus)
{
	if (!is_inside_tree()) {
		return;
	}

	if (!has_focus()) {
		grab_focus(p_hide_focus);
		return;
	}

	if (!editable || editing) {
		return;
	}

	if (select_all_on_focus) {
		if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
			// Select all when the mouse button is up.
			pending_select_all_on_focus = true;
		}
		else {
			select_all();
		}
	}

	editing = true;
	_validate_caret_can_draw();

	if (p_show_virtual_keyboard && !pending_select_all_on_focus) {
		show_virtual_keyboard();
	}
	queue_redraw();
}

void LineEdit::unedit()
{
	if (!editing) {
		return;
	}

	editing = false;
	_validate_caret_can_draw();

	apply_ime();
	set_caret_column(caret_column); // Update scroll_offset.

	if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		DisplayServer::get_singleton()->virtual_keyboard_hide();
	}

	if (deselect_on_focus_loss_enabled && !selection.drag_attempt) {
		deselect();
	}
}

bool LineEdit::is_editing() const { return editing; }

void LineEdit::set_keep_editing_on_text_submit(bool p_enabled)
{
	keep_editing_on_text_submit = p_enabled;
}

bool LineEdit::is_editing_kept_on_text_submit() const { return keep_editing_on_text_submit; }

void LineEdit::_close_ime_window()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::get_singleton()->window_set_ime_position(Point2(), wid);
	DisplayServer::get_singleton()->window_set_ime_active(false, wid);
}

void LineEdit::_update_ime_window_position()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::get_singleton()->window_set_ime_active(true, wid);
	Point2 pos = Point2(get_caret_pixel_pos().x,
					 (get_size().y + theme_cache.font->get_height(theme_cache.font_size)) / 2) +
				 get_global_position();
	if (get_window()->get_embedder()) {
		pos += get_viewport()->get_popup_base_transform().get_origin();
	}
	// Take into account the window's transform.
	pos = get_window()->get_screen_transform().xform(pos);
	// The window will move to the updated position the next time the IME is updated, not
	// immediately.
	DisplayServer::get_singleton()->window_set_ime_position(pos, wid);
}

bool LineEdit::has_ime_text() const { return !ime_text.is_empty(); }

void LineEdit::cancel_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}
	ime_text = String();
	ime_selection = Vector2i();
	alt_start = false;
	alt_start_no_hold = false;
	_close_ime_window();
	_shape();
}

void LineEdit::apply_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}

	// Force apply the current IME text.
	if (alt_start || alt_start_no_hold) {
		cancel_ime();
		if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff && alt_code <= 0x10ffff)) {
			char32_t ucodestr[2] = {(char32_t)alt_code, 0};
			insert_text_at_caret(ucodestr);
		}
	}
	else {
		String insert_ime_text = ime_text;
		cancel_ime();
		insert_text_at_caret(insert_ime_text);
	}
}

void LineEdit::_swap_current_input_direction()
{
	if (input_direction == TEXT_DIRECTION_LTR) {
		input_direction = TEXT_DIRECTION_RTL;
	}
	else {
		input_direction = TEXT_DIRECTION_LTR;
	}
	set_caret_column(get_caret_column());
}

void LineEdit::_move_caret_left(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.begin);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Move to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() - 1);
		}
		else {
			set_caret_column(TS->shaped_text_prev_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_right(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.end);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Move to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() + 1);
		}
		else {
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_start(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(0);
	shift_selection_check_post(p_select);
}

void LineEdit::_move_caret_end(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(text.length());
	shift_selection_check_post(p_select);
}

void LineEdit::_backspace(bool p_word, bool p_all_to_left)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == 0) {
		return; // Nothing to do.
	}

	if (p_all_to_left) {
		text = text.substr(caret_column);
		_shape();
		set_caret_column(0);
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Delete to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(cc, caret_column);

		set_caret_column(cc);
	}
	else {
		delete_char();
	}
}

void LineEdit::_delete(bool p_word, bool p_all_to_right)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == text.length()) {
		return; // Nothing to do.
	}

	if (p_all_to_right) {
		text = text.substr(0, caret_column);
		_shape();
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;
		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Delete to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(caret_column, cc);
		set_caret_column(caret_column);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(caret_column + 1);
			delete_char();
		}
		else {
			int cc = caret_column;
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, caret_column));
			delete_text(cc, caret_column);
		}
	}
}

Point2 LineEdit::_get_right_icon_size(Ref<Texture2D> p_right_icon) const
{
	Size2 icon_size;

	if (p_right_icon.is_null()) {
		return icon_size;
	}

	switch (icon_expand_mode) {
	default:
	case LineEdit::EXPAND_MODE_ORIGINAL_SIZE:
		icon_size = p_right_icon->get_size();
		break;
	case LineEdit::EXPAND_MODE_FIT_TO_TEXT: {
		real_t height = theme_cache.font->get_height(theme_cache.font_size);
		icon_size = Size2(height, height);
	} break;
	case LineEdit::EXPAND_MODE_FIT_TO_LINE_EDIT: {
		icon_size = p_right_icon->get_size();
		Point2 size = get_size();
		float icon_width = icon_size.width * size.height / icon_size.height;
		float icon_height = size.height;

		if (icon_width > size.width) {
			icon_width = size.width;
			icon_height = icon_size.height * icon_width / icon_size.width;
		}

		icon_size = Size2(icon_width, icon_height) * right_icon_scale;
	} break;
	}

	return icon_size;
}

void LineEdit::unhandled_key_input(const Ref<InputEvent>& p_event)
{
	// Return to prevent editing if just focused.
	if (!editing) {
		return;
	}

	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		if (!k->is_pressed()) {
			return;
		}
		// Handle Unicode (with modifiers active, process after shortcuts).
		if (has_focus() && editable && (k->get_unicode() >= 32)) {
			selection_delete();
			char32_t ucodestr[2] = {(char32_t)k->get_unicode(), 0};
			int prev_len = text.length();
			insert_text_at_caret(ucodestr);
			if (text.length() != prev_len) {
				_text_changed();
			}
			accept_event();
		}
	}
}

void LineEdit::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (alignment == p_alignment) {
		return;
	}

	alignment = p_alignment;
	_shape();
	queue_redraw();
}

HorizontalAlignment LineEdit::get_horizontal_alignment() const { return alignment; }

Control::CursorShape LineEdit::get_cursor_shape(const Point2& p_pos) const
{
	if ((!text.is_empty() && is_editable() && _is_over_clear_button(p_pos)) ||
		(!is_editable() && (!is_selecting_enabled() || text.is_empty()))) {
		return CURSOR_ARROW;
	}
	return Control::get_cursor_shape(p_pos);
}

bool LineEdit::_is_over_clear_button(const Point2& p_pos) const
{
	if (!clear_button_enabled || !has_point(p_pos)) {
		return false;
	}
	Ref<Texture2D> icon = theme_cache.clear_icon;
	return is_layout_rtl() ? p_pos.x < theme_cache.normal->get_margin(SIDE_LEFT) + icon->get_width()
						   : p_pos.x > get_size().width - icon->get_width() -
										   theme_cache.normal->get_margin(SIDE_RIGHT);
}

void LineEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void LineEdit::copy_text()
{
	if (selection.enabled && !pass) {
		DisplayServer::get_singleton()->clipboard_set(get_selected_text());
	}
}

void LineEdit::cut_text()
{
	if (editable && selection.enabled && !pass) {
		DisplayServer::get_singleton()->clipboard_set(get_selected_text());
		selection_delete();
	}
}

bool LineEdit::has_undo() const
{
	if (undo_stack_pos == nullptr) {
		return undo_stack.size() > 1;
	}
	return undo_stack_pos != undo_stack.front();
}

bool LineEdit::has_redo() const
{
	return undo_stack_pos != nullptr && undo_stack_pos != undo_stack.back();
}

void LineEdit::undo()
{
	if (!editable) {
		return;
	}

	if (!has_undo()) {
		return;
	}

	if (undo_stack_pos == nullptr) {
		undo_stack_pos = undo_stack.back();
	}

	deselect();

	undo_stack_pos = undo_stack_pos->prev();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::redo()
{
	if (!editable) {
		return;
	}

	if (!has_redo()) {
		return;
	}

	deselect();

	undo_stack_pos = undo_stack_pos->next();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::shift_selection_check_pre(bool p_shift)
{
	if (!selection.enabled && p_shift) {
		selection.start_column = caret_column;
	}
	if (!p_shift) {
		deselect();
	}
}

void LineEdit::shift_selection_check_post(bool p_shift)
{
	if (p_shift) {
		selection_fill_at_caret();
	}
}

void LineEdit::set_caret_at_pixel_pos(int p_x)
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	int ofs = std::ceil(TS->shaped_text_hit_test_position(text_rid, p_x - x_ofs - scroll_offset));
	if (ofs == -1) {
		return;
	}
	if (!caret_mid_grapheme_enabled) {
		ofs = TS->shaped_text_closest_character_pos(text_rid, ofs);
	}
	set_caret_column(ofs);
}

Vector2 LineEdit::get_caret_pixel_pos()
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	Vector2 ret;
	CaretInfo caret;
	// Get position of the start of caret.
	if (!ime_text.is_empty() && ime_selection.x != 0) {
		caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_selection.x);
	}
	else {
		caret = TS->shaped_text_get_carets(text_rid, caret_column);
	}

	if ((caret.l_caret != Rect2() && (caret.l_dir == TextServer::DIRECTION_AUTO ||
										 caret.l_dir == (TextServer::Direction)input_direction)) ||
		(caret.t_caret == Rect2())) {
		ret.x = x_ofs + caret.l_caret.position.x + scroll_offset;
	}
	else {
		ret.x = x_ofs + caret.t_caret.position.x + scroll_offset;
	}

	// Get position of the end of caret.
	if (!ime_text.is_empty()) {
		if (ime_selection.y != 0) {
			caret = TS->shaped_text_get_carets(
				text_rid, caret_column + ime_selection.x + ime_selection.y);
		}
		else {
			caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_text.size());
		}
		if ((caret.l_caret != Rect2() &&
				(caret.l_dir == TextServer::DIRECTION_AUTO ||
					caret.l_dir == (TextServer::Direction)input_direction)) ||
			(caret.t_caret == Rect2())) {
			ret.y = x_ofs + caret.l_caret.position.x + scroll_offset;
		}
		else {
			ret.y = x_ofs + caret.t_caret.position.x + scroll_offset;
		}
	}
	else {
		ret.y = ret.x;
	}

	return ret;
}

void LineEdit::set_caret_mid_grapheme_enabled(const bool p_enabled)
{
	caret_mid_grapheme_enabled = p_enabled;
}

bool LineEdit::is_caret_mid_grapheme_enabled() const { return caret_mid_grapheme_enabled; }

bool LineEdit::is_caret_blink_enabled() const { return caret_blink_enabled; }

bool LineEdit::is_caret_force_displayed() const { return caret_force_displayed; }

void LineEdit::set_caret_force_displayed(const bool p_enabled)
{
	if (caret_force_displayed == p_enabled) {
		return;
	}

	caret_force_displayed = p_enabled;
	_validate_caret_can_draw();

	queue_redraw();
}

float LineEdit::get_caret_blink_interval() const { return caret_blink_interval; }

void LineEdit::set_caret_blink_interval(const float p_interval)
{
	ERR_FAIL_COND(p_interval <= 0);
	caret_blink_interval = p_interval;
}

void LineEdit::_reset_caret_blink_timer()
{
	if (caret_blink_enabled) {
		draw_caret = true;
		if (caret_can_draw) {
			caret_blink_timer = 0.0;
			queue_redraw();
		}
	}
}

void LineEdit::_toggle_draw_caret()
{
	draw_caret = !draw_caret;
	if (is_visible_in_tree() && caret_can_draw) {
		queue_redraw();
	}
}

void LineEdit::_validate_caret_can_draw()
{
	if (caret_blink_enabled) {
		draw_caret = true;
		caret_blink_timer = 0.0;
	}
	caret_can_draw = (caret_force_displayed && !is_part_of_edited_scene()) ||
					 (editing && (window_has_focus || (menu && menu->has_focus())) && has_focus());
}

void LineEdit::delete_char()
{
	if (text.is_empty() || caret_column == 0) {
		return;
	}
	int delete_char_offset = 1;
	if (!caret_mid_grapheme_enabled && backspace_deletes_composite_character_enabled) {
		delete_char_offset = caret_column - get_previous_composite_character_column(caret_column);
	}
	text = text.left(caret_column - delete_char_offset) + text.substr(caret_column);
	_shape();

	set_caret_column(get_caret_column() - delete_char_offset);

	_text_changed();
}

void LineEdit::_set_text(String p_text, bool p_emit_signal)
{
	clear_internal();

	String previous_text = get_text();
	insert_text_at_caret(p_text);

	if (get_text() != previous_text) {
		_create_undo_state();
		if (p_emit_signal) {
			_text_changed();
		}
	}

	queue_redraw();
	caret_column = 0;
	scroll_offset = 0.0;
}

void LineEdit::set_text(String p_text) { _set_text(p_text); }

void LineEdit::set_text_with_selection(const String& p_text)
{
	Selection selection_copy = selection;

	clear_internal();

	String previous_text = get_text();
	insert_text_at_caret(p_text);

	if (get_text() != previous_text) {
		_create_undo_state();
	}

	int tlen = text.length();
	selection = selection_copy;
	selection.begin = MIN(selection.begin, tlen);
	selection.end = MIN(selection.end, tlen);
	selection.start_column = MIN(selection.start_column, tlen);

	queue_redraw();
}

void LineEdit::set_text_direction(Control::TextDirection p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		if (text_direction != TEXT_DIRECTION_AUTO && text_direction != TEXT_DIRECTION_INHERITED) {
			input_direction = text_direction;
		}
		_shape();

		if (menu_dir) {
			menu_dir->set_item_checked(menu_dir->get_item_index(MENU_DIR_INHERITED),
				text_direction == TEXT_DIRECTION_INHERITED);
			menu_dir->set_item_checked(
				menu_dir->get_item_index(MENU_DIR_AUTO), text_direction == TEXT_DIRECTION_AUTO);
			menu_dir->set_item_checked(
				menu_dir->get_item_index(MENU_DIR_LTR), text_direction == TEXT_DIRECTION_LTR);
			menu_dir->set_item_checked(
				menu_dir->get_item_index(MENU_DIR_RTL), text_direction == TEXT_DIRECTION_RTL);
		}
		queue_redraw();
	}
}

Control::TextDirection LineEdit::get_text_direction() const { return text_direction; }

void LineEdit::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		_shape();
		queue_redraw();
	}
}

String LineEdit::get_language() const { return language; }

void LineEdit::set_draw_control_chars(bool p_draw_control_chars)
{
	if (draw_control_chars != p_draw_control_chars) {
		draw_control_chars = p_draw_control_chars;
		if (menu && menu->get_item_index(MENU_DISPLAY_UCC) >= 0) {
			menu->set_item_checked(menu->get_item_index(MENU_DISPLAY_UCC), draw_control_chars);
		}
		_shape();
		queue_redraw();
	}
}

bool LineEdit::get_draw_control_chars() const { return draw_control_chars; }

void LineEdit::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		st_parser = p_parser;
		_shape();
		queue_redraw();
	}
}

TextServer::StructuredTextParser LineEdit::get_structured_text_bidi_override() const
{
	return st_parser;
}

void LineEdit::clear()
{
	bool was_empty = text.is_empty();
	clear_internal();
	_clear_redo();
	if (!was_empty) {
		_emit_text_change();
	}

	// This should reset virtual keyboard state if needed.
	if (editing) {
		show_virtual_keyboard();
	}
}

void LineEdit::show_virtual_keyboard()
{
	_update_ime_window_position();

	if (DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		if (selection.enabled) {
			DisplayServer::get_singleton()->virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				selection.begin, selection.end);
		}
		else {
			DisplayServer::get_singleton()->virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				caret_column);
		}
	}
}

String LineEdit::get_text() const { return text; }

String LineEdit::get_placeholder() const { return placeholder; }

void LineEdit::set_caret_column(int p_column)
{
	if (p_column > (int)text.length()) {
		p_column = text.length();
	}

	if (p_column < 0) {
		p_column = 0;
	}

	caret_column = p_column;

	queue_accessibility_update();

	// Fit to window.

	if (!is_inside_tree()) {
		scroll_offset = 0.0;
		return;
	}

	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	int ofs_max = get_size().width - style->get_margin(SIDE_RIGHT);
	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				if (rtl) {
					x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
				}
				else {
					x_ofs = MAX(style->get_margin(SIDE_LEFT),
						x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
				}
			}
		}
		if (!rtl) {
			ofs_max -= right_icon_size.width;
		}
	}

	// Note: Use two coordinates to fit IME input range.
	Vector2 primary_caret_offset = get_caret_pixel_pos();

	if (MIN(primary_caret_offset.x, primary_caret_offset.y) <= x_ofs) {
		scroll_offset += x_ofs - MIN(primary_caret_offset.x, primary_caret_offset.y);
	}
	else if (MAX(primary_caret_offset.x, primary_caret_offset.y) >= ofs_max) {
		scroll_offset += ofs_max - MAX(primary_caret_offset.x, primary_caret_offset.y);
	}

	// Scroll to show as much text as possible
	if (text_width + scroll_offset + x_ofs < ofs_max) {
		scroll_offset = ofs_max - x_ofs - text_width;
	}

	scroll_offset = MIN(0, scroll_offset);

	queue_accessibility_update();
	queue_redraw();
}

int LineEdit::get_caret_column() const { return caret_column; }

int LineEdit::get_next_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == text.length()) {
		return p_column;
	}
	else {
		return TS->shaped_text_next_character_pos(text_rid, p_column);
	}
}

int LineEdit::get_previous_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == 0) {
		return 0;
	}
	else {
		return TS->shaped_text_prev_character_pos(text_rid, p_column);
	}
}

void LineEdit::set_scroll_offset(float p_pos)
{
	scroll_offset = p_pos;
	if (scroll_offset < 0.0) {
		scroll_offset = 0.0;
	}
}

float LineEdit::get_scroll_offset() const { return scroll_offset; }

void LineEdit::clear_internal()
{
	deselect();
	_clear_undo_stack();
	caret_column = 0;
	scroll_offset = 0.0;
	undo_text = "";
	text = "";
	_shape();
	queue_redraw();
}

Size2 LineEdit::get_minimum_size() const
{
	Ref<Font> font = theme_cache.font;
	int font_size = theme_cache.font_size;

	Size2 min_size;

	// Minimum size of text.
	// W is wider than M in most fonts, Using M may result in hiding the last digit when using float
	// values in SpinBox, ie. ColorPicker RAW values.
	float em_space_size = font->get_char_size('W', font_size).x;
	min_size.width = theme_cache.minimum_character_width * em_space_size;

	if (expand_to_text_length) {
		// Ensure some space for the caret when placed at the end.
		min_size.width = MAX(min_size.width, full_width + theme_cache.caret_width);
	}

	min_size.height = MAX(TS->shaped_text_get_size(text_rid).y, font->get_height(font_size));

	// Take icons into account.
	int icon_max_width = 0;
	if (right_icon.is_valid()) {
		Point2 right_icon_size = _get_right_icon_size(right_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = right_icon_size.width;
	}
	if (clear_button_enabled) {
		Point2 right_icon_size = _get_right_icon_size(theme_cache.clear_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = MAX(icon_max_width, right_icon_size.width);
	}
	min_size.width += icon_max_width;

	Size2 style_min_size =
		theme_cache.normal->get_minimum_size().max(theme_cache.read_only->get_minimum_size());
	return style_min_size + min_size;
}

void LineEdit::deselect()
{
	selection.begin = 0;
	selection.end = 0;
	selection.start_column = 0;
	selection.enabled = false;
	selection.creating = false;
	selection.double_click = false;
	queue_accessibility_update();
	queue_redraw();
}

bool LineEdit::has_selection() const { return selection.enabled; }

String LineEdit::get_selected_text()
{
	if (selection.enabled) {
		return text.substr(selection.begin, selection.end - selection.begin);
	}
	else {
		return String();
	}
}

int LineEdit::get_selection_from_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.begin;
}

int LineEdit::get_selection_to_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.end;
}

void LineEdit::selection_delete()
{
	if (selection.enabled) {
		delete_text(selection.begin, selection.end);
	}

	deselect();
}

void LineEdit::set_max_length(int p_max_length)
{
	ERR_FAIL_COND(p_max_length < 0);
	max_length = p_max_length;
	set_text(text);
}

int LineEdit::get_max_length() const { return max_length; }

void LineEdit::selection_fill_at_caret()
{
	if (!selecting_enabled) {
		return;
	}

	selection.begin = caret_column;
	selection.end = selection.start_column;

	if (selection.end < selection.begin) {
		int aux = selection.end;
		selection.end = selection.begin;
		selection.begin = aux;
	}

	selection.enabled = (selection.begin != selection.end);
	queue_accessibility_update();
}

void LineEdit::select_all()
{
	if (!selecting_enabled) {
		return;
	}

	if (text.is_empty()) {
		set_caret_column(0);
		return;
	}

	selection.begin = 0;
	selection.end = text.length();
	selection.enabled = true;
	queue_accessibility_update();
	queue_redraw();
}

bool LineEdit::is_editable() const { return editable; }

void LineEdit::set_secret(bool p_secret)
{
	if (pass == p_secret) {
		return;
	}

	pass = p_secret;
	_shape();
	set_caret_column(caret_column); // Update scroll_offset.
	queue_redraw();
}

bool LineEdit::is_secret() const { return pass; }

void LineEdit::set_secret_character(const String& p_string)
{
	String c = p_string;
	if (c.length() > 1) {
		WARN_PRINT("Secret character must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}
	if (secret_character == c) {
		return;
	}
	secret_character = c;
	_shape();
	set_caret_column(caret_column); // Update scroll_offset.
	queue_redraw();
}

String LineEdit::get_secret_character() const { return secret_character; }

void LineEdit::select(int p_from, int p_to)
{
	if (!selecting_enabled) {
		return;
	}

	if (p_from == 0 && p_to == 0) {
		deselect();
		return;
	}

	int len = text.length();
	if (p_from < 0) {
		p_from = 0;
	}
	if (p_from > len) {
		p_from = len;
	}
	if (p_to < 0 || p_to > len) {
		p_to = len;
	}

	if (p_from >= p_to) {
		return;
	}

	selection.enabled = true;
	selection.begin = p_from;
	selection.end = p_to;
	selection.creating = false;
	selection.double_click = false;
	queue_accessibility_update();
	queue_redraw();
}

bool LineEdit::is_text_field() const { return true; }

void LineEdit::set_context_menu_enabled(bool p_enable) { context_menu_enabled = p_enable; }

bool LineEdit::is_context_menu_enabled() { return context_menu_enabled; }

void LineEdit::show_emoji_and_symbol_picker()
{
	_update_ime_window_position();
	DisplayServer::get_singleton()->show_emoji_and_symbol_picker();
}

void LineEdit::set_emoji_menu_enabled(bool p_enabled)
{
	if (emoji_menu_enabled != p_enabled) {
		emoji_menu_enabled = p_enabled;
	}
}

bool LineEdit::is_emoji_menu_enabled() const { return emoji_menu_enabled; }

void LineEdit::set_backspace_deletes_composite_character_enabled(bool p_enabled)
{
	backspace_deletes_composite_character_enabled = p_enabled;
}

bool LineEdit::is_backspace_deletes_composite_character_enabled() const
{
	return backspace_deletes_composite_character_enabled;
}

bool LineEdit::is_menu_visible() const { return menu && menu->is_visible(); }

PopupMenu* LineEdit::get_menu() const
{
	if (!menu) {
		const_cast<LineEdit*>(this)->_generate_context_menu();
	}
	return menu;
}

void LineEdit::set_expand_to_text_length_enabled(bool p_enabled)
{
	expand_to_text_length = p_enabled;
	update_minimum_size();
	set_caret_column(caret_column);
}

bool LineEdit::is_expand_to_text_length_enabled() const { return expand_to_text_length; }

void LineEdit::set_clear_button_enabled(bool p_enabled)
{
	if (clear_button_enabled == p_enabled) {
		return;
	}
	clear_button_enabled = p_enabled;
	_fit_to_width();
	update_minimum_size();
	queue_redraw();
}

bool LineEdit::is_clear_button_enabled() const { return clear_button_enabled; }

void LineEdit::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool LineEdit::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

void LineEdit::set_virtual_keyboard_enabled(bool p_enable) { virtual_keyboard_enabled = p_enable; }

bool LineEdit::is_virtual_keyboard_enabled() const { return virtual_keyboard_enabled; }

void LineEdit::set_virtual_keyboard_show_on_focus(bool p_show_on_focus)
{
	virtual_keyboard_show_on_focus = p_show_on_focus;
}

bool LineEdit::get_virtual_keyboard_show_on_focus() const { return virtual_keyboard_show_on_focus; }

void LineEdit::set_virtual_keyboard_type(VirtualKeyboardType p_type)
{
	virtual_keyboard_type = p_type;
}

LineEdit::VirtualKeyboardType LineEdit::get_virtual_keyboard_type() const
{
	return virtual_keyboard_type;
}

void LineEdit::set_middle_mouse_paste_enabled(bool p_enabled)
{
	middle_mouse_paste_enabled = p_enabled;
}

bool LineEdit::is_middle_mouse_paste_enabled() const { return middle_mouse_paste_enabled; }

void LineEdit::set_selecting_enabled(bool p_enabled)
{
	if (selecting_enabled == p_enabled) {
		return;
	}

	selecting_enabled = p_enabled;

	if (!selecting_enabled) {
		deselect();
	}
}

bool LineEdit::is_selecting_enabled() const { return selecting_enabled; }

void LineEdit::set_deselect_on_focus_loss_enabled(const bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && selection.enabled && !has_focus()) {
		deselect();
	}
}

bool LineEdit::is_deselect_on_focus_loss_enabled() const { return deselect_on_focus_loss_enabled; }

void LineEdit::set_drag_and_drop_selection_enabled(const bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool LineEdit::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

void LineEdit::_texture_changed()
{
	_fit_to_width();
	update_minimum_size();
	queue_redraw();
}

Ref<Texture2D> LineEdit::get_right_icon() { return right_icon; }

LineEdit::ExpandMode LineEdit::get_icon_expand_mode() const { return icon_expand_mode; }

void LineEdit::set_right_icon_scale(float p_scale)
{
	if (right_icon_scale == p_scale) {
		return;
	}

	right_icon_scale = p_scale;
	queue_redraw();
	update_minimum_size();
}

float LineEdit::get_right_icon_scale() const { return right_icon_scale; }

void LineEdit::set_flat(bool p_enabled)
{
	if (flat != p_enabled) {
		flat = p_enabled;
		queue_redraw();
	}
}

bool LineEdit::is_flat() const { return flat; }

void LineEdit::set_select_all_on_focus(bool p_enabled) { select_all_on_focus = p_enabled; }

bool LineEdit::is_select_all_on_focus() const { return select_all_on_focus; }

void LineEdit::clear_pending_select_all_on_focus() { pending_select_all_on_focus = false; }

void LineEdit::_text_changed()
{
	_emit_text_change();
	_clear_redo();
}

PackedStringArray LineEdit::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (secret_character.length() > 1) {
		warnings.push_back("Secret Character property supports only one character. Extra "
						   "characters will be ignored.");
	}
	return warnings;
}

void LineEdit::_fit_to_width()
{
	if (alignment == HORIZONTAL_ALIGNMENT_FILL) {
		Ref<StyleBox> style = theme_cache.normal;
		int t_width =
			get_size().width - style->get_margin(SIDE_RIGHT) - style->get_margin(SIDE_LEFT);
		bool using_placeholder = text.is_empty() && ime_text.is_empty();
		bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
		if (right_icon.is_valid() || display_clear_icon) {
			Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
			Point2 right_icon_size = _get_right_icon_size(r_icon);
			t_width -= right_icon_size.width;
		}
		TS->shaped_text_fit_to_width(text_rid, MAX(t_width, full_width));
	}
}

void LineEdit::_clear_redo()
{
	_create_undo_state();
	if (undo_stack_pos == nullptr) {
		return;
	}

	undo_stack_pos = undo_stack_pos->next();
	while (undo_stack_pos) {
		List<TextOperation>::Element* elem = undo_stack_pos;
		undo_stack_pos = undo_stack_pos->next();
		undo_stack.erase(elem);
	}
	_create_undo_state();
}

void LineEdit::_clear_undo_stack()
{
	undo_stack.clear();
	undo_stack_pos = nullptr;
	_create_undo_state();
}

void LineEdit::_create_undo_state()
{
	TextOperation op;
	op.text = text;
	op.caret_column = caret_column;
	op.scroll_offset = scroll_offset;
	undo_stack.push_back(op);
}

Key LineEdit::_get_menu_action_accelerator(const String& p_action)
{
	const List<Ref<InputEvent>>* events = InputMap::get_singleton()->action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<Ref<InputEvent>>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const Ref<InputEventKey> event = first_event->get();
	if (event.is_null()) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero.
	if (event->get_physical_keycode() != Key::NONE) {
		return event->get_physical_keycode_with_modifiers();
	}
	else {
		return event->get_keycode_with_modifiers();
	}
}

void LineEdit::_update_context_menu()
{
	if (!menu) {
		_generate_context_menu();
	}

	int idx = -1;

#define MENU_ITEM_ACTION_DISABLED(m_menu, m_id, m_action, m_disabled)                              \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_ACTION(m_menu, m_id, m_action)                                                   \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
	}

#define MENU_ITEM_DISABLED(m_menu, m_id, m_disabled)                                               \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_CHECKED(m_menu, m_id, m_checked)                                                 \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_checked(idx, m_checked);                                                  \
	}

	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		MENU_ITEM_DISABLED(menu, MENU_EMOJI_AND_SYMBOL, !editable || !emoji_menu_enabled)
	}
	MENU_ITEM_ACTION_DISABLED(menu, MENU_CUT, "ui_cut", !editable)
	MENU_ITEM_ACTION(menu, MENU_COPY, "ui_copy")
	MENU_ITEM_ACTION_DISABLED(menu, MENU_PASTE, "ui_paste", !editable)
	MENU_ITEM_ACTION_DISABLED(menu, MENU_SELECT_ALL, "ui_text_select_all", !selecting_enabled)
	MENU_ITEM_DISABLED(menu, MENU_CLEAR, !editable)
	MENU_ITEM_ACTION_DISABLED(menu, MENU_UNDO, "ui_undo", !editable || !has_undo())
	MENU_ITEM_ACTION_DISABLED(menu, MENU_REDO, "ui_redo", !editable || !has_redo())
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_INHERITED, text_direction == TEXT_DIRECTION_INHERITED)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_AUTO, text_direction == TEXT_DIRECTION_AUTO)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_LTR, text_direction == TEXT_DIRECTION_LTR)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_RTL, text_direction == TEXT_DIRECTION_RTL)
	MENU_ITEM_CHECKED(menu, MENU_DISPLAY_UCC, draw_control_chars)
	MENU_ITEM_DISABLED(menu, MENU_SUBMENU_INSERT_UCC, !editable)

#undef MENU_ITEM_ACTION_DISABLED
#undef MENU_ITEM_ACTION
#undef MENU_ITEM_DISABLED
#undef MENU_ITEM_CHECKED
}

LineEdit::LineEdit(const String& p_placeholder)
{
	text_rid = TS->create_shaped_text();
	_create_undo_state();

	deselect();
	set_focus_mode(FOCUS_ALL);
	set_default_cursor_shape(CURSOR_IBEAM);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_process_unhandled_key_input(true);

	set_caret_blink_enabled(false);

	set_placeholder(p_placeholder);

	set_editable(
		true); // Initialize to opposite first, so we get past the early-out in set_editable.
}

LineEdit::~LineEdit() { TS->free_rid(text_rid); }


