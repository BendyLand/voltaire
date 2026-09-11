/**************************************************************************/
/*  audio_stream_import_settings.cpp                                      */
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

#include "audio_stream_import_settings.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/check_box.h"
#include "servers/rendering/rendering_server.h"

AudioStreamImportSettingsDialog* AudioStreamImportSettingsDialog::singleton = nullptr;

void AudioStreamImportSettingsDialog::_draw_preview()
{
	Rect2 rect = _preview->get_rect();
	Size2 rect_size = rect.size;
	int width = rect_size.width;

	Ref<AudioStreamPreview> preview =
		AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);
	float preview_offset = zoom_bar->get_value();
	float preview_len = zoom_bar->get_page();

	Ref<Font> beat_font = get_theme_font(SNAME("main"), EditorStringName(EditorFonts));
	int main_size = get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts));
	Vector<Vector2> points;
	points.resize(width * 2);
	Color color_active = get_theme_color(SNAME("contrast_color_2"), EditorStringName(Editor));
	Color color_inactive = color_active;
	color_inactive.a *= 0.5;
	Vector<Color> colors;
	colors.resize(width);

	float inactive_from = 1e20;
	float beat_size = 0;
	int last_beat = 0;
	if (stream->get_bpm() > 0) {
		beat_size = 60 / float(stream->get_bpm());
		int y_ofs = beat_font->get_height(main_size) + 4 * EDSCALE;
		rect.position.y += y_ofs;
		rect.size.y -= y_ofs;

		if (stream->get_beat_count() > 0) {
			last_beat = stream->get_beat_count();
			inactive_from = last_beat * beat_size;
		}
	}

	for (int i = 0; i < width; i++) {
		float ofs = preview_offset + i * preview_len / rect_size.width;
		float ofs_n = preview_offset + (i + 1) * preview_len / rect_size.width;
		float max = preview->get_max(ofs, ofs_n) * 0.5 + 0.5;
		float min = preview->get_min(ofs, ofs_n) * 0.5 + 0.5;

		int idx = i;
		points.write[idx * 2 + 0] = Vector2(i + 1, rect.position.y + min * rect.size.y);
		points.write[idx * 2 + 1] = Vector2(i + 1, rect.position.y + max * rect.size.y);

		colors.write[idx] = ofs > inactive_from ? color_inactive : color_active;
	}

	if (!points.is_empty()) {
		RS::get_singleton()->canvas_item_add_multiline(_preview->get_canvas_item(), points, colors);
	}

	if (beat_size) {
		Color beat_color = Color(1, 1, 1, 1);
		Color final_beat_color = beat_color;
		Color bar_color = beat_color;
		beat_color.a *= 0.4;
		bar_color.a *= 0.6;

		int prev_beat = 0; // Do not draw beat zero
		Color color_bg = color_active;
		color_bg.a *= 0.2;
		_preview->draw_rect(Rect2(0, 0, rect.size.width, rect.position.y), color_bg);
		int bar_beats = stream->get_bar_beats();

		int last_text_end_x = 0;
		for (int i = 0; i < width; i++) {
			float ofs = preview_offset + i * preview_len / rect_size.width;
			int beat = int(ofs / beat_size);
			if (beat != prev_beat) {
				String text = itos(beat);
				int text_w = beat_font->get_string_size(text).width;
				if (i - text_w / 2 > last_text_end_x + 2 * EDSCALE) {
					int x_ofs = i - text_w / 2;
					_preview->draw_string(beat_font.ptr(),
						Point2(x_ofs, 2 * EDSCALE + beat_font->get_ascent(main_size)), text,
						HORIZONTAL_ALIGNMENT_LEFT, rect.size.width - x_ofs, Font::DEFAULT_FONT_SIZE,
						color_active);
					last_text_end_x = i + text_w / 2;
				}

				if (beat == last_beat) {
					_preview->draw_rect(
						Rect2i(i, rect.position.y, 2, rect.size.height), final_beat_color);
					// Darken subsequent beats
					beat_color.a *= 0.3;
					color_active.a *= 0.3;
				}
				else {
					_preview->draw_rect(Rect2i(i, rect.position.y, 1, rect.size.height),
						(beat % bar_beats) == 0 ? bar_color : beat_color);
				}
				prev_beat = beat;
			}
		}
	}
}

void AudioStreamImportSettingsDialog::_draw_indicator()
{
	if (stream.is_null()) {
		return;
	}

	Rect2 rect = _preview->get_rect();

	Ref<Font> beat_font = get_theme_font(SNAME("main"), EditorStringName(EditorFonts));
	int main_size = get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts));

	if (stream->get_bpm() > 0) {
		int y_ofs = beat_font->get_height(main_size) + 4 * EDSCALE;
		rect.position.y += y_ofs;
		rect.size.height -= y_ofs;
	}

	_current_label->set_text(String::num(_current, 2).pad_decimals(2) + " /");

	float ofs_x = (_current - zoom_bar->get_value()) * rect.size.width / zoom_bar->get_page();
	if (ofs_x < 0 || ofs_x >= rect.size.width) {
		return;
	}

	const Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	_indicator->draw_line(Point2(ofs_x, rect.position.y),
		Point2(ofs_x, rect.position.y + rect.size.height), color, Math::round(2 * EDSCALE));
	_indicator->draw_texture(get_editor_theme_icon(SNAME("TimelineIndicator")).ptr(),
		Point2(ofs_x - get_editor_theme_icon(SNAME("TimelineIndicator"))->get_width() * 0.5,
			rect.position.y),
		color);

	if (stream->get_bpm() > 0 && _hovering_beat != -1) {
		// Draw hovered beat.
		float preview_offset = zoom_bar->get_value();
		float preview_len = zoom_bar->get_page();
		float beat_size = 60 / float(stream->get_bpm());
		int prev_beat = 0;
		for (int i = 0; i < rect.size.width; i++) {
			float ofs = preview_offset + i * preview_len / rect.size.width;
			int beat = int(ofs / beat_size);
			if (beat != prev_beat) {
				String text = itos(beat);
				int text_w = beat_font->get_string_size(text).width;
				if (i - text_w / 2 > 2 * EDSCALE && beat == _hovering_beat) {
					int x_ofs = i - text_w / 2;
					_indicator->draw_string(beat_font.ptr(),
						Point2(x_ofs, 2 * EDSCALE + beat_font->get_ascent(main_size)), text,
						HORIZONTAL_ALIGNMENT_LEFT, rect.size.width - x_ofs, Font::DEFAULT_FONT_SIZE,
						color);
					break;
				}
				prev_beat = beat;
			}
		}
	}
}

int AudioStreamImportSettingsDialog::_get_beat_at_pos(real_t p_x)
{
	float ofs_sec = zoom_bar->get_value() + p_x * zoom_bar->get_page() / _preview->get_size().width;
	ofs_sec = CLAMP(ofs_sec, 0, stream->get_length());
	float beat_size = 60 / float(stream->get_bpm());
	int beat = int(ofs_sec / beat_size + 0.5);

	if (beat * beat_size >
		stream->get_length() +
			0.001) { // Stream may end few audio frames before but may still want to use full loop.
		beat--;
	}
	return beat;
}

void AudioStreamImportSettingsDialog::_set_beat_len_to(real_t p_x)
{
	int beat = _get_beat_at_pos(p_x);
	if (beat < 1) {
		beat = 1; // Because 0 is disable.
	}
	updating_settings = true;
	beats_enabled->set_pressed(true);
	beats_edit->set_value(beat);
	updating_settings = false;
	_settings_changed();
}


