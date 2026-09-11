/**************************************************************************/
/*  color_picker.cpp                                                      */
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

#include "color_picker.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/expression.h"
#include "scene/gui/color_mode.h"
#include "scene/gui/color_picker_shape.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/link_button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/color_palette.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_texture.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

#ifdef MACOS_ENABLED
#include "core/os/os.h"
#endif

static inline bool is_color_overbright(const Color& color)
{
	return (color.r > 1.0) || (color.g > 1.0) || (color.b > 1.0);
}

static inline bool is_color_valid_hex(const Color& color)
{
	return !is_color_overbright(color) && color.r >= 0 && color.g >= 0 && color.b >= 0;
}

static inline String color_to_string(
	const Color& color, bool show_alpha = true, bool force_value_format = false)
{
	if (!force_value_format && !is_color_overbright(color)) {
		return "#" + color.to_html(show_alpha);
	}
	String t = "(" + String::num(color.r, 3) + ", " + String::num(color.g, 3) + ", " +
			   String::num(color.b, 3);
	if (show_alpha) {
		t += ", " + String::num(color.a, 3) + ")";
	}
	else {
		t += ")";
	}
	return t;
}

void ColorPicker::_update_theme_item_cache()
{
	VBoxContainer::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void ColorPicker::set_focus_on_picker_shape() { shapes[get_current_shape_index()]->grab_focus(); }

void ColorPicker::_update_controls()
{
	int mode_sliders_count = modes[current_mode]->get_slider_count();

	for (int i = current_slider_count; i < mode_sliders_count; i++) {
		sliders[i]->show();
		labels[i]->show();
		values[i]->show();
	}
	for (int i = mode_sliders_count; i < current_slider_count; i++) {
		sliders[i]->hide();
		labels[i]->hide();
		values[i]->hide();
	}
	current_slider_count = mode_sliders_count;

	for (int i = 0; i < current_slider_count; i++) {
		labels[i]->set_text(modes[current_mode]->get_slider_label(i));
		sliders[i]->set_accessibility_name(modes[current_mode]->get_slider_label(i));
		values[i]->set_accessibility_name(modes[current_mode]->get_slider_label(i));
	}
	alpha_label->set_text("A");
	alpha_slider->set_accessibility_name(ETR("Alpha"));
	alpha_value->set_accessibility_name(ETR("Alpha"));

	intensity_label->set_text("I");
	intensity_slider->set_accessibility_name(ETR("Intensity"));
	intensity_value->set_accessibility_name(ETR("Intensity"));

	alpha_value->set_visible(edit_alpha);
	alpha_slider->set_visible(edit_alpha);
	alpha_label->set_visible(edit_alpha);

	intensity_value->set_visible(edit_intensity);
	intensity_slider->set_visible(edit_intensity);
	intensity_label->set_visible(edit_intensity);

	int i = 0;
	for (ColorPickerShape* shape : shapes) {
		bool is_active = get_current_shape_index() == i;
		i++;

		if (!shape->is_initialized) {
			if (is_active) {
				// Controls are initialized on demand, because ColorPicker does not need them all at
				// once.
				shape->initialize_controls();
			}
			else {
				continue;
			}
		}

		for (Control* control : shape->controls) {
			control->set_visible(is_active);
		}
	}
	btn_shape->set_visible(current_shape != SHAPE_NONE);
}

void ColorPicker::_set_pick_color(
	const Color& p_color, bool p_update_sliders, bool p_calc_intensity)
{
	if (text_changed) {
		add_recent_preset(color);
		text_changed = false;
	}

	color = p_color;
	if (p_calc_intensity) {
		_copy_color_to_normalized_and_intensity();
	}
	_copy_normalized_to_hsv_okhsl();

	if (!is_inside_tree()) {
		return;
	}

	_update_color(p_update_sliders);
}

void ColorPicker::set_pick_color(const Color& p_color)
{
	_set_pick_color(p_color, true, true); // Because setters can't have more arguments.
}

void ColorPicker::set_old_color(const Color& p_color) { old_color = p_color; }

void ColorPicker::set_display_old_color(bool p_enabled) { display_old_color = p_enabled; }

bool ColorPicker::is_displaying_old_color() const { return display_old_color; }

bool ColorPicker::is_editing_alpha() const { return edit_alpha; }

bool ColorPicker::is_editing_intensity() const { return edit_intensity; }

void ColorPicker::_slider_drag_started() { currently_dragging = true; }

void ColorPicker::add_mode(ColorMode* p_mode) { modes.push_back(p_mode); }

void ColorPicker::add_shape(ColorPickerShape* p_shape) { shapes.push_back(p_shape); }

HSlider* ColorPicker::get_slider(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, MODE_MAX, nullptr);
	return sliders[p_idx];
}

Vector<float> ColorPicker::get_active_slider_values()
{
	Vector<float> cur_values;
	for (int i = 0; i < current_slider_count; i++) {
		cur_values.push_back(sliders[i]->get_value());
	}
	cur_values.push_back(alpha_slider->get_value());
	return cur_values;
}

void ColorPicker::_copy_normalized_to_hsv_okhsl()
{
	if (!okhsl_cached) {
		ok_hsl_h = color_normalized.get_ok_hsl_h();
		ok_hsl_s = color_normalized.get_ok_hsl_s();
		ok_hsl_l = color_normalized.get_ok_hsl_l();
	}
	if (!hsv_cached) {
		h = color_normalized.get_h();
		s = color_normalized.get_s();
		v = color_normalized.get_v();
	}
	hsv_cached = false;
	okhsl_cached = false;
}

void ColorPicker::_copy_hsv_okhsl_to_normalized()
{
	if (current_shape != SHAPE_NONE && shapes[get_current_shape_index()]->is_ok_hsl()) {
		color_normalized.set_ok_hsl(ok_hsl_h, ok_hsl_s, ok_hsl_l, color_normalized.a);
	}
	else {
		color_normalized.set_hsv(h, s, v, color_normalized.a);
	}
}

Color ColorPicker::_color_apply_intensity(const Color& col) const
{
	if (intensity == 0.0f) {
		return col;
	}
	Color linear_color = col.srgb_to_linear();
	Color result;
	float multiplier = Math::pow(2, intensity);
	for (int i = 0; i < 3; i++) {
		result[i] = linear_color[i] * multiplier;
	}
	result.a = col.a;
	return result.linear_to_srgb();
}

void ColorPicker::_normalized_apply_intensity_to_color()
{
	color = _color_apply_intensity(color_normalized);
}

void ColorPicker::_copy_color_to_normalized_and_intensity()
{
	Color linear_color = color.srgb_to_linear();
	float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));
	for (int i = 0; i < 3; i++) {
		color_normalized[i] = linear_color[i] / multiplier;
	}
	color_normalized.a = linear_color.a;
	color_normalized = color_normalized.linear_to_srgb();
	intensity = Math::log2(multiplier);
}

void ColorPicker::_reset_sliders_theme()
{
	Ref<StyleBoxFlat> style_box_flat(memnew(StyleBoxFlat));
	style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
	style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

	for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
		sliders[i]->begin_bulk_theme_override();
		sliders[i]->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_icon_override(
			SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_constant_override(
			SNAME("grabber_offset"), 8 * theme_cache.base_scale);
		if (!colorize_sliders) {
			sliders[i]->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
		}
		sliders[i]->end_bulk_theme_override();
	}

	alpha_slider->begin_bulk_theme_override();
	alpha_slider->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_icon_override(SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_constant_override(SNAME("grabber_offset"), 8 * theme_cache.base_scale);
	if (!colorize_sliders) {
		alpha_slider->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
	}
	alpha_slider->end_bulk_theme_override();
}

void ColorPicker::_text_copy_pressed()
{
	DisplayServer::get_singleton()->clipboard_set(c_text->get_text());
}

Color ColorPicker::get_pick_color() const { return color; }

Color ColorPicker::get_old_color() const { return old_color; }

ColorPicker::PickerShapeType ColorPicker::get_picker_shape() const { return current_shape; }

inline int ColorPicker::_get_preset_size()
{
	return (int(get_minimum_size().width) -
			   (preset_container->get_h_separation() * (PRESET_COLUMN_COUNT - 1))) /
		   PRESET_COLUMN_COUNT;
}

void ColorPicker::_load_palette()
{
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("ColorPalette", &extensions);

	file_dialog->set_title(ETR("Load Color Palette"));
	file_dialog->clear_filters();
	for (const String& K : extensions) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_current_file("");
	file_dialog->popup_centered_ratio();
}

void ColorPicker::_save_palette(bool p_is_save_as)
{
	if (!p_is_save_as && !palette_path.is_empty()) {
		file_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
		_palette_file_selected(palette_path);
		return;
	}
	else {
		List<String> extensions;
		ResourceLoader::get_recognized_extensions_for_type("ColorPalette", &extensions);

		file_dialog->set_title(ETR("Save Color Palette"));
		file_dialog->clear_filters();
		for (const String& K : extensions) {
			file_dialog->add_filter("*." + K);
		}

		file_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
		file_dialog->set_current_file("new_palette.tres");
		file_dialog->popup_centered_ratio();
	}
}

#ifdef TOOLS_ENABLED

GridContainer* ColorPicker::get_slider_container() { return slider_gc; }

#endif // ifdef TOOLS_ENABLED

void ColorPicker::_show_hide_preset(
	const bool& p_is_btn_pressed, Button* p_btn_preset, Container* p_preset_container)
{
	if (p_is_btn_pressed) {
		p_preset_container->show();
	}
	else {
		p_preset_container->hide();
	}
	_update_drop_down_arrow(p_is_btn_pressed, p_btn_preset);

	palette_name->hide();
	if (btn_preset->is_pressed() && !palette_name->get_text().is_empty()) {
		palette_name->show();
	}
}

void ColorPicker::_set_mode_popup_value(ColorModeType p_mode)
{
	ERR_FAIL_INDEX(p_mode, MODE_MAX + 1);

	if (p_mode == MODE_MAX) {
		set_colorize_sliders(!colorize_sliders);
	}
	else {
		set_color_mode(p_mode);
	}
}

PackedColorArray ColorPicker::get_presets() const
{
	PackedColorArray arr;
	arr.resize(presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = presets.begin(); itr != presets.end(); ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

PackedColorArray ColorPicker::get_recent_presets() const
{
	PackedColorArray arr;
	arr.resize(recent_presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = recent_presets.begin(); itr != recent_presets.end();
		 ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

ColorPicker::ColorModeType ColorPicker::get_color_mode() const { return current_mode; }

void ColorPicker::set_colorize_sliders(bool p_colorize_sliders)
{
	if (colorize_sliders == p_colorize_sliders) {
		return;
	}

	colorize_sliders = p_colorize_sliders;
	mode_popup->set_item_checked(MODE_MAX + 1, colorize_sliders);

	if (colorize_sliders) {
		Ref<StyleBoxEmpty> style_box_empty(memnew(StyleBoxEmpty));

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_empty.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_empty.ptr());
	}
	else {
		Ref<StyleBoxFlat> style_box_flat(memnew(StyleBoxFlat));
		style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
		style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_flat.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_flat.ptr());
	}
}

bool ColorPicker::is_colorizing_sliders() const { return colorize_sliders; }

void ColorPicker::set_deferred_mode(bool p_enabled) { deferred_mode_enabled = p_enabled; }

bool ColorPicker::is_deferred_mode() const { return deferred_mode_enabled; }

void ColorPicker::_sample_draw()
{
	// Covers the right half of the sample if the old color is being displayed,
	// or the whole sample if it's not being displayed.
	Rect2 rect_new;
	Rect2 rect_old;

	if (display_old_color) {
		rect_new = Rect2(Point2(sample->get_size().width * 0.5, 0),
			Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		// Draw both old and new colors for easier comparison (only if spawned from a
		// ColorPickerButton).
		rect_old = Rect2(
			Point2(), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		if (old_color.a < 1.0) {
			sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_old, true);
		}

		sample->draw_rect(rect_old, old_color);

		if (!old_color.is_equal_approx(color)) {
			// Draw a revert indicator to indicate that the old sample can be clicked to revert to
			// this old color. Adapt icon color to the background color (taking alpha checkerboard
			// into account) so that it's always visible.
			sample->draw_texture(theme_cache.sample_revert.ptr(),
				rect_old.size * 0.5 - theme_cache.sample_revert->get_size() * 0.5,
				Math::lerp(0.75f, old_color.get_luminance(), old_color.a) < 0.455
					? Color(1, 1, 1)
					: (Color(0.01, 0.01, 0.01)));

			sample->set_focus_mode(FOCUS_ALL);
		}
		else {
			sample->set_focus_mode(FOCUS_NONE);
		}

		if (is_color_overbright(color)) {
			// Draw an indicator to denote that the old color is "overbright" and can't be displayed
			// accurately in the preview.
			sample->draw_texture(theme_cache.overbright_indicator.ptr(), Point2());
		}
	}
	else {
		rect_new =
			Rect2(Point2(), Size2(sample->get_size().width, sample->get_size().height * 0.95));
	}

	if (color.a < 1.0) {
		sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_new, true);
	}

	sample->draw_rect(rect_new, color);

	if (display_old_color && !old_color.is_equal_approx(color) && sample->has_focus(true)) {
		RID ci = sample->get_canvas_item();
		theme_cache.sample_focus->draw(ci, rect_old);
	}

	if (is_color_overbright(color)) {
		// Draw an indicator to denote that the new color is "overbright" and can't be displayed
		// accurately in the preview.
		sample->draw_texture(
			theme_cache.overbright_indicator.ptr(), Point2(sample->get_size().width * 0.5, 0));
	}
}

void ColorPicker::_slider_draw(int p_which)
{
	if (colorize_sliders) {
		modes[current_mode]->slider_draw(p_which);
	}
}

void ColorPicker::_alpha_slider_draw()
{
	if (!colorize_sliders) {
		return;
	}
	Vector<Vector2> pos;
	pos.resize(4);
	Vector<Color> col;
	col.resize(4);
	Size2 size = alpha_slider->get_size();
	Color left_color;
	Color right_color;
	const real_t margin = 16 * theme_cache.base_scale;
	alpha_slider->draw_texture_rect(
		theme_cache.sample_bg.ptr(), Rect2(Point2(0, 0), Size2(size.x, margin)), true);

	left_color = color_normalized;
	left_color.a = 0;
	right_color = color_normalized;
	right_color.a = 1;

	col.set(0, left_color);
	col.set(1, right_color);
	col.set(2, right_color);
	col.set(3, left_color);
	pos.set(0, Vector2(0, 0));
	pos.set(1, Vector2(size.x, 0));
	pos.set(2, Vector2(size.x, margin));
	pos.set(3, Vector2(0, margin));

	alpha_slider->draw_polygon(pos, col);
}

void ColorPicker::_slider_or_spin_input(const Ref<InputEvent>& p_event)
{
	if (line_edit_mouse_release) {
		line_edit_mouse_release = false;
		return;
	}
	Ref<InputEventMouseButton> bev = p_event;
	if (bev.is_valid() && !bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
		add_recent_preset(color);
	}
}

void ColorPicker::_line_edit_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> bev = p_event;
	if (bev.is_valid() && !bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
		line_edit_mouse_release = true;
	}
}

void ColorPicker::_text_changed(const String&) { text_changed = true; }

void ColorPicker::_target_gui_input(const Ref<InputEvent>& p_event)
{
	const Ref<InputEventMouseButton> mouse_event = p_event;
	if (mouse_event.is_null()) {
		return;
	}
	if (mouse_event->get_button_index() == MouseButton::LEFT) {
		if (mouse_event->is_pressed()) {
			picker_window->hide();
			_pick_finished();
		}
	}
	else if (mouse_event->get_button_index() == MouseButton::RIGHT) {
		set_pick_color(pre_picking_color); // Cancel.
		is_picking_color = false;
		set_process_internal(false);
		picker_window->hide();
	}
	else {
		Window* w = picker_window->get_parent_visible_window();
		while (w) {
			Point2i win_mpos = w->get_mouse_position(); // Mouse position local to the window.
			Size2i win_size = w->get_size();
			if (win_mpos.x >= 0 && win_mpos.y >= 0 && win_mpos.x <= win_size.x &&
				win_mpos.y <= win_size.y) {
				// Mouse event inside window bounds, forward this event to the window.
				Ref<InputEventMouseButton> new_ev = p_event->duplicate();
				new_ev->set_position(win_mpos);
				new_ev->set_global_position(win_mpos);
				w->push_input(new_ev.ptr(), true);
				return;
			}
			w = w->get_parent_visible_window();
		}
	}
}

void ColorPicker::_update_menu_items()
{
	options_menu->clear();
	options_menu->reset_size();

	options_menu->add_icon_item(get_theme_icon(SNAME("save"), SNAME("FileDialog")), ETR("Save"),
		static_cast<int>(MenuOption::MENU_SAVE));
	options_menu->set_item_tooltip(-1, ETR("Save the current color palette to reuse later."));
	options_menu->set_item_disabled(-1, presets.is_empty());

	options_menu->add_icon_item(get_theme_icon(SNAME("save"), SNAME("FileDialog")), ETR("Save As"),
		static_cast<int>(MenuOption::MENU_SAVE_AS));
	options_menu->set_item_tooltip(
		-1, ETR("Save the current color palette as a new to reuse later."));
	options_menu->set_item_disabled(-1, palette_path.is_empty());

	options_menu->add_icon_item(get_theme_icon(SNAME("load"), SNAME("FileDialog")), ETR("Load"),
		static_cast<int>(MenuOption::MENU_LOAD));
	options_menu->set_item_tooltip(-1, ETR("Load existing color palette."));

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		options_menu->add_icon_item(get_theme_icon(SNAME("load"), SNAME("FileDialog")),
			TTRC("Quick Load"), static_cast<int>(MenuOption::MENU_QUICKLOAD));
		options_menu->set_item_tooltip(-1, TTRC("Load existing color palette."));
	}
#endif // TOOLS_ENABLED

	options_menu->add_icon_item(get_theme_icon(SNAME("clear"), SNAME("FileDialog")), ETR("Clear"),
		static_cast<int>(MenuOption::MENU_CLEAR));
	options_menu->set_item_tooltip(
		-1, ETR("Clear the currently loaded color palettes in the picker."));
	options_menu->set_item_disabled(-1, presets.is_empty());
}

void ColorPicker::_block_input_on_popup_show()
{
	if (!get_tree()->get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(true);
	}
}

void ColorPicker::_enable_input_on_popup_hide()
{
	if (!get_tree()->get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(false);
	}
}

void ColorPicker::_html_focus_exit()
{
	if (c_text->is_menu_visible()) {
		return;
	}

	if (is_visible_in_tree()) {
		_html_submitted(c_text->get_text());
	}
	else {
		_update_text_value();
	}
}

bool ColorPicker::are_swatches_enabled() const { return can_add_swatches; }

void ColorPicker::set_presets_visible(bool p_visible)
{
	if (presets_visible == p_visible) {
		return;
	}
	presets_visible = p_visible;
	swatches_vbc->set_visible(p_visible);
}

bool ColorPicker::are_presets_visible() const { return presets_visible; }

void ColorPicker::set_modes_visible(bool p_visible)
{
	if (color_modes_visible == p_visible) {
		return;
	}
	color_modes_visible = p_visible;
	mode_hbc->set_visible(p_visible);
}

bool ColorPicker::are_modes_visible() const { return color_modes_visible; }

void ColorPicker::set_sampler_visible(bool p_visible)
{
	if (sampler_visible == p_visible) {
		return;
	}
	sampler_visible = p_visible;
	sample_hbc->set_visible(p_visible);
#ifdef MACOS_ENABLED
	perm_hb->set_visible(p_visible && !OS::get_singleton()->get_granted_permissions().has(
										  "macos.permission.RECORD_SCREEN"));
#endif
}

bool ColorPicker::is_sampler_visible() const { return sampler_visible; }

void ColorPicker::set_sliders_visible(bool p_visible)
{
	if (sliders_visible == p_visible) {
		return;
	}
	sliders_visible = p_visible;
	slider_gc->set_visible(p_visible);
}

bool ColorPicker::are_sliders_visible() const { return sliders_visible; }

void ColorPicker::set_hex_visible(bool p_visible)
{
	if (hex_visible == p_visible) {
		return;
	}
	hex_visible = p_visible;
	hex_hbc->set_visible(p_visible);
}

bool ColorPicker::is_hex_visible() const { return hex_visible; }

void ColorPicker::_req_permission()
{
#ifdef MACOS_ENABLED
	OS::get_singleton()->request_permission("macos.permission.RECORD_SCREEN");
#endif
}

ColorPicker::~ColorPicker()
{
	for (ColorMode* mode : modes) {
		memdelete(mode);
	}
	for (ColorPickerShape* shape : shapes) {
		memdelete(shape);
	}
}

/////////////////

void ColorPickerPopupPanel::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (p_event->is_action_pressed(SNAME("ui_accept"), false, true)) {
		_close_pressed();
	}
	PopupPanel::_input_from_window(p_event);
}

/////////////////

void ColorPickerButton::_about_to_popup()
{
	if (!get_tree()->get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(true);
	}
	set_pressed(true);
	if (picker) {
		picker->set_old_color(color);
	}
}

void ColorPickerButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::get_singleton()->update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
		AccessibilityServer::get_singleton()->update_set_popup_type(
			ae, AccessibilityServerEnums::AccessibilityPopupType::POPUP_DIALOG);
		AccessibilityServer::get_singleton()->update_set_color_value(ae, color);
	} break;

	case NOTIFICATION_DRAW: {
		const Rect2 r = Rect2(theme_cache.normal_style->get_offset(),
			get_size() - theme_cache.normal_style->get_minimum_size());
		draw_texture_rect(theme_cache.background_icon.ptr(), r, true);
		draw_rect(r, color);

		if (color.r > 1 || color.g > 1 || color.b > 1) {
			// Draw an indicator to denote that the color is "overbright" and can't be displayed
			// accurately in the preview
			draw_texture(
				theme_cache.overbright_indicator.ptr(), theme_cache.normal_style->get_offset());
		}
	} break;

	case NOTIFICATION_WM_CLOSE_REQUEST: {
		if (popup) {
			popup->hide();
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (popup && !is_visible_in_tree()) {
			popup->hide();
		}
	} break;
	}
}

Color ColorPickerButton::get_pick_color() const { return color; }

void ColorPickerButton::set_edit_alpha(bool p_show)
{
	if (edit_alpha == p_show) {
		return;
	}
	edit_alpha = p_show;
	if (picker) {
		picker->set_edit_alpha(p_show);
	}
}

bool ColorPickerButton::is_editing_alpha() const { return edit_alpha; }

void ColorPickerButton::set_edit_intensity(bool p_show)
{
	if (edit_intensity == p_show) {
		return;
	}
	edit_intensity = p_show;
	if (picker) {
		picker->set_edit_intensity(p_show);
	}
}

bool ColorPickerButton::is_editing_intensity() const { return edit_intensity; }

ColorPicker* ColorPickerButton::get_picker()
{
	_update_picker();
	return picker;
}

PopupPanel* ColorPickerButton::get_popup()
{
	_update_picker();
	return popup;
}

ColorPickerButton::ColorPickerButton(const String& p_text) : Button(p_text)
{
	set_toggle_mode(true);
}

/////////////////

void ColorPresetButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::get_singleton()->update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
		AccessibilityServer::get_singleton()->update_set_color_value(ae, preset_color);
	} break;

	case NOTIFICATION_DRAW: {
		const Rect2 r = Rect2(Point2(0, 0), get_size());
		Ref<StyleBox> sb_raw = theme_cache.foreground_style->duplicate();
		Ref<StyleBoxFlat> sb_flat = sb_raw;
		Ref<StyleBoxTexture> sb_texture = sb_raw;

		if (sb_flat.is_valid()) {
			sb_flat->set_border_width(SIDE_BOTTOM, 2);
			if (get_draw_mode() == DRAW_PRESSED || get_draw_mode() == DRAW_HOVER_PRESSED) {
				sb_flat->set_border_color(Color(1, 1, 1, 1));
			}
			else {
				sb_flat->set_border_color(Color(0, 0, 0, 1));
			}

			if (preset_color.a < 1) {
				// Draw a background pattern when the color is transparent.
				sb_flat->set_bg_color(Color(1, 1, 1));
				sb_flat->draw(get_canvas_item(), r);

				Rect2 bg_texture_rect = r.grow_side(SIDE_LEFT, -sb_flat->get_margin(SIDE_LEFT));
				bg_texture_rect =
					bg_texture_rect.grow_side(SIDE_RIGHT, -sb_flat->get_margin(SIDE_RIGHT));
				bg_texture_rect =
					bg_texture_rect.grow_side(SIDE_TOP, -sb_flat->get_margin(SIDE_TOP));
				bg_texture_rect =
					bg_texture_rect.grow_side(SIDE_BOTTOM, -sb_flat->get_margin(SIDE_BOTTOM));

				draw_texture_rect(theme_cache.background_icon.ptr(), bg_texture_rect, true);
				sb_flat->set_bg_color(preset_color);
			}
			sb_flat->set_bg_color(preset_color);
			sb_flat->draw(get_canvas_item(), r);
		}
		else if (sb_texture.is_valid()) {
			if (preset_color.a < 1) {
				// Draw a background pattern when the color is transparent.
				bool use_tile_texture =
					(sb_texture->get_h_axis_stretch_mode() ==
						StyleBoxTexture::AxisStretchMode::AXIS_STRETCH_MODE_TILE) ||
					(sb_texture->get_h_axis_stretch_mode() ==
						StyleBoxTexture::AxisStretchMode::AXIS_STRETCH_MODE_TILE_FIT);
				draw_texture_rect(theme_cache.background_icon.ptr(), r, use_tile_texture);
			}
			sb_texture->set_modulate(preset_color);
			sb_texture->draw(get_canvas_item(), r);
		}
		else {
			WARN_PRINT("Unsupported StyleBox used for ColorPresetButton. Use StyleBoxFlat or "
					   "StyleBoxTexture instead.");
		}

		if (has_focus(true)) {
			RID ci = get_canvas_item();
			theme_cache.focus_style->draw(ci, Rect2(Point2(), get_size()));
		}

		if (is_color_overbright(preset_color)) {
			// Draw an indicator to denote that the color is "overbright" and can't be displayed
			// accurately in the preview
			draw_texture(theme_cache.overbright_indicator.ptr(), Vector2(0, 0));
		}

	} break;
	}
}

void ColorPresetButton::set_preset_color(const Color& p_color)
{
	preset_color = p_color;
	queue_accessibility_update();
}

Color ColorPresetButton::get_preset_color() const { return preset_color; }

ColorPresetButton::~ColorPresetButton() {}


