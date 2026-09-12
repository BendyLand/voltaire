/**************************************************************************/
/*  sprite_frames_editor_plugin.cpp                                       */
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

#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/gui/center_container.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "sprite_frames_editor_plugin.h"

static void _draw_shadowed_line(Control* p_control, const Point2& p_from, const Size2& p_size,
	const Size2& p_shadow_offset, Color p_color, Color p_shadow_color)
{
	p_control->draw_line(p_from, p_from + p_size, p_color);
	p_control->draw_line(
		p_from + p_shadow_offset, p_from + p_size + p_shadow_offset, p_shadow_color);
}

void SpriteFramesEditor::_open_sprite_sheet()
{
	file_split_sheet->clear_filters();
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("Texture2D", &extensions);
	for (const String& extension : extensions) {
		file_split_sheet->add_filter("*." + extension);
	}

	file_split_sheet->popup_file_dialog();
}

int SpriteFramesEditor::_sheet_preview_position_to_frame_index(const Point2& p_position)
{
	const Size2i offset = _get_offset();
	const Size2i frame_size = _get_frame_size();
	const Size2i separation = _get_separation();
	const Size2i block_size = frame_size + separation;
	const Point2i position = p_position / sheet_zoom - offset;

	if (position.x < 0 || position.y < 0) {
		return -1; // Out of bounds.
	}

	if (position.x % block_size.x >= frame_size.x || position.y % block_size.y >= frame_size.y) {
		return -1; // Gap between frames.
	}

	const Point2i frame = position / block_size;
	const Size2i frame_count = _get_frame_count();
	if (frame.x >= frame_count.x || frame.y >= frame_count.y) {
		return -1; // Out of bounds.
	}

	return frame_count.x * frame.y + frame.x;
}

void SpriteFramesEditor::_sheet_update_zoom_label()
{
	String zoom_text;
	// The zoom level displayed is relative to the editor scale
	// (like in most image editors). Its lower bound is clamped to 1 as some people
	// lower the editor scale to increase the available real estate,
	// even if their display doesn't have a particularly low DPI.
	TranslationServer* translation_server = TranslationServer::get_singleton();
	String locale = translation_server->get_tool_locale();
	if (sheet_zoom >= 10) {
		zoom_text = translation_server->format_number(
			rtos(Math::round((sheet_zoom / MAX(1, EDSCALE)) * 100)), locale);
	}
	else {
		// 2 decimal places if the zoom is below 10%, 1 decimal place if it's below 1000%.
		zoom_text = translation_server->format_number(
			rtos(Math::snapped(
				(sheet_zoom / MAX(1, EDSCALE)) * 100, (sheet_zoom >= 0.1) ? 0.1 : 0.01)),
			locale);
	}
	zoom_text += " " + translation_server->get_percent_sign(locale);
	split_sheet_zoom_reset->set_text(zoom_text);
}

void SpriteFramesEditor::_sheet_zoom_on_position(float p_zoom, const Vector2& p_position)
{
	const float old_zoom = sheet_zoom;
	sheet_zoom = CLAMP(sheet_zoom * p_zoom, min_sheet_zoom, max_sheet_zoom);

	const Size2 texture_size = split_sheet_preview->get_texture()->get_size();
	split_sheet_preview->set_custom_minimum_size(texture_size * sheet_zoom);

	Vector2 offset =
		Vector2(split_sheet_scroll->get_h_scroll(), split_sheet_scroll->get_v_scroll());
	offset = (offset + p_position) / old_zoom * sheet_zoom - p_position;
	split_sheet_scroll->set_h_scroll(offset.x);
	split_sheet_scroll->set_v_scroll(offset.y);

	_sheet_update_zoom_label();
}

void SpriteFramesEditor::_sheet_zoom_in() { _sheet_zoom_on_position(scale_ratio, Vector2()); }

void SpriteFramesEditor::_sheet_zoom_out() { _sheet_zoom_on_position(1 / scale_ratio, Vector2()); }

void SpriteFramesEditor::_sheet_zoom_reset()
{
	// Default the zoom to match the editor scale, but don't dezoom on editor scales below 100% to
	// prevent pixel art from looking bad.
	sheet_zoom = MAX(1.0f, EDSCALE);
	Size2 texture_size = split_sheet_preview->get_texture()->get_size();
	split_sheet_preview->set_custom_minimum_size(texture_size * sheet_zoom);

	_sheet_update_zoom_label();
}

void SpriteFramesEditor::_sheet_zoom_fit()
{
	const float margin_percentage = 0.1f;
	const float max_margin = 64.0f;
	const Size2 margin = (margin_percentage * split_sheet_scroll->get_size()).minf(max_margin);
	const Size2 display_area_size = split_sheet_scroll->get_size() - margin;
	const Size2 texture_size = split_sheet_preview->get_texture()->get_size();
	const Vector2 texture_ratio = display_area_size / texture_size;
	float texture_fit_zoom = MIN(texture_ratio.x, texture_ratio.y);

	// Quantize the zoom level to avoid subpixel rendering
	if (texture_fit_zoom > 1.0) {
		texture_fit_zoom = Math::floor(texture_fit_zoom);
	}
	else if (!Math::is_zero_approx(texture_fit_zoom)) {
		texture_fit_zoom = 1.0f / Math::ceil(1.0f / texture_fit_zoom);
	}

	sheet_zoom = CLAMP(texture_fit_zoom, min_sheet_zoom, max_sheet_zoom);
	split_sheet_preview->set_custom_minimum_size(texture_size * sheet_zoom);

	_sheet_update_zoom_label();
}

void SpriteFramesEditor::_sheet_sort_frames()
{
	if (!frames_need_sort) {
		return;
	}
	frames_need_sort = false;
	frames_ordered.resize(frames_selected.size());
	if (frames_selected.is_empty()) {
		return;
	}

	const Size2i frame_count = _get_frame_count();
	const int frame_order = split_sheet_order->get_selected_id();
	int index = 0;

	// Fill based on order.
	for (const KeyValue<int, int>& from_pair : frames_selected) {
		const int idx = from_pair.key;

		const int selection_order = from_pair.value;

		// Default to using selection order.
		int order_by = selection_order;

		// Extract coordinates for sorting.
		const int pos_frame_x = idx % frame_count.x;
		const int pos_frame_y = idx / frame_count.x;

		const int neg_frame_x = frame_count.x - (pos_frame_x + 1);
		const int neg_frame_y = frame_count.y - (pos_frame_y + 1);

		switch (frame_order) {
		case FRAME_ORDER_LEFT_RIGHT_TOP_BOTTOM: {
			order_by = frame_count.x * pos_frame_y + pos_frame_x;
		} break;

		case FRAME_ORDER_LEFT_RIGHT_BOTTOM_TOP: {
			order_by = frame_count.x * neg_frame_y + pos_frame_x;
		} break;

		case FRAME_ORDER_RIGHT_LEFT_TOP_BOTTOM: {
			order_by = frame_count.x * pos_frame_y + neg_frame_x;
		} break;

		case FRAME_ORDER_RIGHT_LEFT_BOTTOM_TOP: {
			order_by = frame_count.x * neg_frame_y + neg_frame_x;
		} break;

		case FRAME_ORDER_TOP_BOTTOM_LEFT_RIGHT: {
			order_by = pos_frame_y + frame_count.y * pos_frame_x;
		} break;

		case FRAME_ORDER_TOP_BOTTOM_RIGHT_LEFT: {
			order_by = pos_frame_y + frame_count.y * neg_frame_x;
		} break;

		case FRAME_ORDER_BOTTOM_TOP_LEFT_RIGHT: {
			order_by = neg_frame_y + frame_count.y * pos_frame_x;
		} break;

		case FRAME_ORDER_BOTTOM_TOP_RIGHT_LEFT: {
			order_by = neg_frame_y + frame_count.y * neg_frame_x;
		} break;
		}

		// Assign in vector.
		frames_ordered.set(index, Pair<int, int>(order_by, idx));
		index++;
	}

	// Sort frames.
	frames_ordered.sort_custom<PairSort<int, int>>();
}

void SpriteFramesEditor::_sheet_spin_changed(double p_value, int p_dominant_param)
{
	if (updating_split_settings) {
		return;
	}
	updating_split_settings = true;

	if (p_dominant_param != PARAM_USE_CURRENT) {
		dominant_param = p_dominant_param;
	}

	const Size2i texture_size = split_sheet_preview->get_texture()->get_size();
	const Size2i size = texture_size - _get_offset();

	switch (dominant_param) {
	case PARAM_SIZE: {
		const Size2i frame_size = _get_frame_size();

		const Size2i offset_max = texture_size - frame_size;
		split_sheet_offset_x->set_max(offset_max.x);
		split_sheet_offset_y->set_max(offset_max.y);

		const Size2i sep_max = size - frame_size * 2;
		split_sheet_sep_x->set_max(sep_max.x);
		split_sheet_sep_y->set_max(sep_max.y);

		const Size2i separation = _get_separation();
		const Size2i count = (size + separation) / (frame_size + separation);
		split_sheet_h->set_value(count.x);
		split_sheet_v->set_value(count.y);
	} break;

	case PARAM_FRAME_COUNT: {
		const Size2i count = _get_frame_count();

		const Size2i offset_max = texture_size - count;
		split_sheet_offset_x->set_max(offset_max.x);
		split_sheet_offset_y->set_max(offset_max.y);

		const Size2i gap_count = count - Size2i(1, 1);
		split_sheet_sep_x->set_max(gap_count.x == 0 ? size.x : (size.x - count.x) / gap_count.x);
		split_sheet_sep_y->set_max(gap_count.y == 0 ? size.y : (size.y - count.y) / gap_count.y);

		const Size2i separation = _get_separation();
		const Size2i frame_size = (size - separation * gap_count) / count;
		split_sheet_size_x->set_value(frame_size.x);
		split_sheet_size_y->set_value(frame_size.y);
	} break;
	}

	updating_split_settings = false;

	frames_selected.clear();
	selected_count = 0;
	last_frame_selected = -1;
}

void SpriteFramesEditor::_auto_slice_sprite_sheet()
{
	if (updating_split_settings) {
		return;
	}
	updating_split_settings = true;

	const Size2i size = split_sheet_preview->get_texture()->get_size();

	const Size2i split_sheet = _estimate_sprite_sheet_size(split_sheet_preview->get_texture());
	split_sheet_h->set_value(split_sheet.x);
	split_sheet_v->set_value(split_sheet.y);
	split_sheet_size_x->set_value(size.x / split_sheet.x);
	split_sheet_size_y->set_value(size.y / split_sheet.y);
	split_sheet_sep_x->set_value(0);
	split_sheet_sep_y->set_value(0);
	split_sheet_offset_x->set_value(0);
	split_sheet_offset_y->set_value(0);

	updating_split_settings = false;

	frames_selected.clear();
	selected_count = 0;
	last_frame_selected = -1;
}

bool SpriteFramesEditor::_matches_background_color(
	const Color& p_background_color, const Color& p_pixel_color)
{
	if ((p_background_color.a == 0 && p_pixel_color.a == 0) ||
		p_background_color.is_equal_approx(p_pixel_color)) {
		return true;
	}

	Color d = p_background_color - p_pixel_color;
	// 0.04f is the threshold for how much a colour can deviate from background colour and still be
	// considered a match. Arrived at through experimentation, can be tweaked.
	return (d.r * d.r) + (d.g * d.g) + (d.b * d.b) + (d.a * d.a) < 0.04f;
}

Size2i SpriteFramesEditor::_estimate_sprite_sheet_size(const Ref<Texture2D> p_texture)
{
	Ref<Image> image = p_texture->get_image();
	if (image->is_compressed()) {
		image = image->duplicate();
		ERR_FAIL_COND_V(image->decompress() != OK, p_texture->get_size());
	}
	Size2i size = image->get_size();

	Color assumed_background_color = image->get_pixel(0, 0);
	Size2i sheet_size;

	bool previous_line_background = true;
	for (int x = 0; x < size.x; x++) {
		int y = 0;
		while (y < size.y &&
			   _matches_background_color(assumed_background_color, image->get_pixel(x, y))) {
			y++;
		}
		bool current_line_background = (y == size.y);
		if (previous_line_background && !current_line_background) {
			sheet_size.x++;
		}
		previous_line_background = current_line_background;
	}

	previous_line_background = true;
	for (int y = 0; y < size.y; y++) {
		int x = 0;
		while (x < size.x &&
			   _matches_background_color(assumed_background_color, image->get_pixel(x, y))) {
			x++;
		}
		bool current_line_background = (x == size.x);
		if (previous_line_background && !current_line_background) {
			sheet_size.y++;
		}
		previous_line_background = current_line_background;
	}

	if (sheet_size == Size2i(0, 0) || sheet_size == Size2i(1, 1)) {
		sheet_size = Size2i(4, 4);
	}

	return sheet_size;
}

Size2i SpriteFramesEditor::_get_frame_count() const
{
	return Size2i(split_sheet_h->get_value(), split_sheet_v->get_value());
}

Size2i SpriteFramesEditor::_get_frame_size() const
{
	return Size2i(split_sheet_size_x->get_value(), split_sheet_size_y->get_value());
}

Size2i SpriteFramesEditor::_get_offset() const
{
	return Size2i(split_sheet_offset_x->get_value(), split_sheet_offset_y->get_value());
}

Size2i SpriteFramesEditor::_get_separation() const
{
	return Size2i(split_sheet_sep_x->get_value(), split_sheet_sep_y->get_value());
}

void SpriteFramesEditor::_load_pressed()
{
	ERR_FAIL_COND(!frames->has_animation(edited_anim));
	loading_scene = false;

	file->clear_filters();
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("Texture2D", &extensions);
	for (const String& extension : extensions) {
		file->add_filter("*." + extension);
	}

	file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILES);
	file->popup_file_dialog();
}

void SpriteFramesEditor::_paste_pressed()
{
	ERR_FAIL_COND(!frames->has_animation(edited_anim));

	Ref<ClipboardSpriteFrames> clipboard_frames =
		EditorSettings::get_singleton()->get_resource_clipboard();
	if (clipboard_frames.is_valid()) {
		_paste_frame_array(clipboard_frames);
		return;
	}

	Ref<Texture2D> texture = EditorSettings::get_singleton()->get_resource_clipboard();
	if (texture.is_valid()) {
		_paste_texture(texture);
		return;
	}
}

void SpriteFramesEditor::_copy_pressed()
{
	ERR_FAIL_COND(!frames->has_animation(edited_anim));

	Vector<int> selected_items = frame_list->get_selected_items();

	if (selected_items.is_empty()) {
		return;
	}

	Ref<ClipboardSpriteFrames> clipboard_frames = memnew(ClipboardSpriteFrames);

	for (const int& frame_index : selected_items) {
		Ref<Texture2D> texture = frames->get_frame_texture(edited_anim, frame_index);
		if (texture.is_null()) {
			continue;
		}

		ClipboardSpriteFrames::Frame frame;
		frame.texture = texture;
		frame.duration = frames->get_frame_duration(edited_anim, frame_index);

		clipboard_frames->frames.push_back(frame);
	}
	EditorSettings::get_singleton()->set_resource_clipboard(clipboard_frames);
}

void SpriteFramesEditor::_animation_cut()
{
	if (!frames->has_animation(edited_anim)) {
		return;
	}

	// Copy animation to clipboard.
	Ref<ClipboardAnimation> clipboard_anim =
		ClipboardAnimation::from_sprite_frames(frames, edited_anim);
	EditorSettings::get_singleton()->set_resource_clipboard(clipboard_anim);

	// Remove animation with undo/redo (no confirmation dialog).
	_animation_remove_undo_redo(TTR("Cut Animation"), &clipboard_anim->frames);
}

void SpriteFramesEditor::_animation_copy()
{
	if (!frames->has_animation(edited_anim)) {
		return;
	}

	Ref<ClipboardAnimation> clipboard_anim =
		ClipboardAnimation::from_sprite_frames(frames, edited_anim);
	EditorSettings::get_singleton()->set_resource_clipboard(clipboard_anim);
}

void SpriteFramesEditor::_animation_remove()
{
	if (updating) {
		return;
	}

	if (!frames->has_animation(edited_anim)) {
		return;
	}

	delete_dialog->set_text(TTRC("Delete Animation?"));
	delete_dialog->popup_centered();
}

void SpriteFramesEditor::_animation_remove_confirmed()
{
	_animation_remove_undo_redo(TTR("Remove Animation"), nullptr);
}

void SpriteFramesEditor::_animation_search_text_changed(const String& p_text) { _update_library(); }

StringName SpriteFramesEditor::_find_next_animation()
{
	List<StringName> anim_names;
	frames->get_animation_list(&anim_names);
	anim_names.sort_custom<StringName::AlphCompare>();
	if (anim_names.size() >= 2) {
		if (edited_anim == anim_names.get(0)) {
			return anim_names.get(1);
		}
		else {
			return anim_names.get(0);
		}
	}
	else {
		return StringName();
	}
}

String SpriteFramesEditor::_generate_unique_animation_name(const String& p_base_name) const
{
	if (!frames->has_animation(p_base_name)) {
		return p_base_name;
	}

	int count = 2;
	String new_name = p_base_name;
	PackedStringArray split = new_name.split("_");
	int last_index = split.size() - 1;
	if (last_index > 0 && split[last_index].is_valid_int() && split[last_index].to_int() >= 0) {
		count = split[last_index].to_int();
		split.remove_at(last_index);
		new_name = String("_").join(split);
	}
	while (true) {
		String attempt = new_name;
		attempt += vformat("_%d", count);
		if (frames->has_animation(attempt)) {
			count++;
			continue;
		}
		new_name = attempt;
		break;
	}
	return new_name;
}

void SpriteFramesEditor::_menu_selected(int p_id)
{
	switch (p_id) {
	case MENU_SHOW_IN_FILESYSTEM: {
		Ref<Texture2D> frame_texture = frames->get_frame_texture(edited_anim, right_clicked_frame);
		ERR_FAIL_COND(frame_texture.is_null());
		String path = frame_texture->get_path();
		// Check if the file is an atlas resource, if it is find the source texture.
		Ref<AtlasTexture> at = frame_texture;
		while (at.is_valid() && at->get_atlas().is_valid()) {
			path = at->get_atlas()->get_path();
			at = at->get_atlas();
		}
		FileSystemDock::get_singleton()->navigate_to_path(path);
	} break;
	}
}

void SpriteFramesEditor::_frame_list_item_selected(int p_index, bool p_selected)
{
	if (updating) {
		return;
	}

	selection = frame_list->get_selected_items();
	if (selection.is_empty() || !p_selected) {
		return;
	}

	updating = true;
	frame_duration->set_value(frames->get_frame_duration(edited_anim, selection[0]));
	updating = false;
}

void SpriteFramesEditor::_zoom_in()
{
	// Do not zoom in or out with no visible frames
	if (frames->get_frame_count(edited_anim) <= 0) {
		return;
	}
	if (thumbnail_zoom < max_thumbnail_zoom) {
		thumbnail_zoom *= scale_ratio;
		int thumbnail_size = (int)(thumbnail_default_size * thumbnail_zoom);
		frame_list->set_fixed_column_width(thumbnail_size * 3 / 2);
		frame_list->set_fixed_icon_size(Size2(thumbnail_size, thumbnail_size));
	}
}

void SpriteFramesEditor::_zoom_out()
{
	// Do not zoom in or out with no visible frames
	if (frames->get_frame_count(edited_anim) <= 0) {
		return;
	}
	if (thumbnail_zoom > min_thumbnail_zoom) {
		thumbnail_zoom /= scale_ratio;
		int thumbnail_size = (int)(thumbnail_default_size * thumbnail_zoom);
		frame_list->set_fixed_column_width(thumbnail_size * 3 / 2);
		frame_list->set_fixed_icon_size(Size2(thumbnail_size, thumbnail_size));
	}
}

void SpriteFramesEditor::_zoom_reset()
{
	thumbnail_zoom = MAX(1.0f, EDSCALE);
	frame_list->set_fixed_column_width(thumbnail_default_size * 3 / 2);
	frame_list->set_fixed_icon_size(Size2(thumbnail_default_size, thumbnail_default_size));
}

Ref<SpriteFrames> SpriteFramesEditor::get_sprite_frames() const { return frames; }

void SpriteFramesEditor::_node_removed(Node* p_node)
{
	if (animated_sprite) {
		if (animated_sprite != p_node) {
			return;
		}
		_remove_sprite_node();
	}
}

SpriteFramesEditorPlugin::SpriteFramesEditorPlugin()
{
	frames_editor = memnew(SpriteFramesEditor);
	frames_editor->set_custom_minimum_size(Size2(0, 300) * EDSCALE);
	EditorDockManager::get_singleton()->add_dock(frames_editor);
	frames_editor->close();
}

Ref<ClipboardAnimation> ClipboardAnimation::from_sprite_frames(
	const Ref<SpriteFrames>& p_frames, const String& p_anim)
{
	Ref<ClipboardAnimation> clipboard_anim;
	clipboard_anim.instantiate();
	clipboard_anim->name = p_anim;
	clipboard_anim->speed = p_frames->get_animation_speed(p_anim);
	clipboard_anim->loop = p_frames->get_animation_loop_mode(p_anim);

	int frame_count = p_frames->get_frame_count(p_anim);
	for (int i = 0; i < frame_count; ++i) {
		ClipboardSpriteFrames::Frame frame;
		frame.texture = p_frames->get_frame_texture(p_anim, i);
		frame.duration = p_frames->get_frame_duration(p_anim, i);
		clipboard_anim->frames.push_back(frame);
	}
	return clipboard_anim;
}


