/**************************************************************************/
/*  editor_properties.cpp                                                 */
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
#include "core/input/input_map.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/editor_variant_type_selectors.h"
#include "editor/inspector/editor_properties_array_dict.h"
#include "editor/inspector/editor_properties_vector.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/property_selector.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor_properties.h"
#include "modules/modules_enabled.gen.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/3d/fog_volume.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/mesh.h"
#include "scene/resources/sky.h"
#include "servers/display/display_server.h"

#ifdef MODULE_VISUAL_SHADER_ENABLED
#include "modules/visual_shader/vs_nodes/visual_shader_nodes.h"
#endif // MODULE_VISUAL_SHADER_ENABLED

///////////////////// NIL /////////////////////////

void EditorPropertyNil::update_property() {}

EditorPropertyNil::EditorPropertyNil()
{
	Label* prop_label = memnew(Label);
	prop_label->set_text("<null>");
	add_child(prop_label);
}

//////////////////// VARIANT ///////////////////////

void EditorPropertyVariant::_set_read_only(bool p_read_only)
{
	edit_button->set_disabled(p_read_only);
	if (sub_property) {
		sub_property->set_read_only(p_read_only);
	}
}

void EditorPropertyVariant::_notification(int p_what)
{
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		edit_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
	}
}

///////////////////// TEXT /////////////////////////

void EditorPropertyText::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_update_theme();
	} break;
	}
}

void EditorPropertyText::_set_read_only(bool p_read_only) { text->set_editable(!p_read_only); }

void EditorPropertyText::_update_theme()
{
	Ref<Font> font;
	int font_size;

	if (monospaced) {
		font = get_theme_font(SNAME("source"), EditorStringName(EditorFonts));
		font_size = get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts));
	}
	else {
		font = get_theme_font(SceneStringName(font), SNAME("LineEdit"));
		font_size = get_theme_font_size(SceneStringName(font_size), SNAME("LineEdit"));
	}

	text->add_theme_font_override(SceneStringName(font), font.ptr());
	text->add_theme_font_size_override(SceneStringName(font_size), font_size);
}

void EditorPropertyText::_text_submitted(const String& p_string)
{
	if (updating) {
		return;
	}

	if (text->has_focus()) {
		_text_changed(p_string);
	}
}

void EditorPropertyText::set_string_name(bool p_enabled)
{
	string_name = p_enabled;
	if (p_enabled) {
		Label* prefix = memnew(Label("&"));
		prefix->set_tooltip_text("StringName");
		prefix->set_mouse_filter(MOUSE_FILTER_STOP);
		text->get_parent()->add_child(prefix);
		text->get_parent()->move_child(prefix, 0);
	}
}

void EditorPropertyText::set_secret(bool p_enabled) { text->set_secret(p_enabled); }

void EditorPropertyText::set_placeholder(const String& p_string)
{
	text->set_placeholder(p_string);
}

void EditorPropertyText::set_monospaced(bool p_monospaced)
{
	if (p_monospaced == monospaced) {
		return;
	}
	monospaced = p_monospaced;
	_update_theme();
}

///////////////////// MULTILINE TEXT /////////////////////////

void EditorPropertyMultilineText::_set_read_only(bool p_read_only)
{
	text->set_editable(!p_read_only);
	open_big_text->set_disabled(p_read_only);
}

void EditorPropertyMultilineText::_update_theme()
{
	Ref<Texture2D> df = get_editor_theme_icon(SNAME("DistractionFree"));
	open_big_text->set_button_icon(df);

	Ref<Font> font;
	int font_size;
	if (expression) {
		font = get_theme_font(SNAME("expression"), EditorStringName(EditorFonts));
		font_size = get_theme_font_size(SNAME("expression_size"), EditorStringName(EditorFonts));
	}
	else {
		// Non expression.
		if (monospaced) {
			font = get_theme_font(SNAME("source"), EditorStringName(EditorFonts));
			font_size = get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts));
		}
		else {
			font = get_theme_font(SceneStringName(font), SNAME("TextEdit"));
			font_size = get_theme_font_size(SceneStringName(font_size), SNAME("TextEdit"));
		}
	}
	text->add_theme_font_override(SceneStringName(font), font.ptr());
	text->add_theme_font_size_override(SceneStringName(font_size), font_size);
	text->set_line_wrapping_mode(wrap_lines ? TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY
											: TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	if (big_text) {
		big_text->add_theme_font_override(SceneStringName(font), font.ptr());
		big_text->add_theme_font_size_override(SceneStringName(font_size), font_size);
		big_text->set_line_wrapping_mode(wrap_lines
											 ? TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY
											 : TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	}

	text->set_custom_minimum_size(Vector2(0, font->get_height(font_size) * 6));
}

void EditorPropertyMultilineText::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_update_theme();
	} break;
	}
}

void EditorPropertyMultilineText::EditorPropertyMultilineText::set_monospaced(bool p_monospaced)
{
	if (p_monospaced == monospaced) {
		return;
	}
	monospaced = p_monospaced;
	_update_theme();
}

bool EditorPropertyMultilineText::EditorPropertyMultilineText::get_monospaced()
{
	return monospaced;
}

void EditorPropertyMultilineText::EditorPropertyMultilineText::set_wrap_lines(bool p_wrap_lines)
{
	if (p_wrap_lines == wrap_lines) {
		return;
	}
	wrap_lines = p_wrap_lines;
	_update_theme();
}

bool EditorPropertyMultilineText::EditorPropertyMultilineText::get_wrap_lines()
{
	return wrap_lines;
}

///////////////////// TEXT ENUM /////////////////////////

void EditorPropertyTextEnum::_set_read_only(bool p_read_only)
{
	option_button->set_disabled(p_read_only);
	edit_button->set_disabled(p_read_only);
}

void EditorPropertyTextEnum::_edit_custom_value()
{
	default_layout->hide();
	edit_custom_layout->show();
	custom_value_edit->grab_focus(true);
}

void EditorPropertyTextEnum::_custom_value_submitted(const String& p_value)
{
	edit_custom_layout->hide();
	default_layout->show();

	_emit_changed_value(p_value.strip_edges());
}

void EditorPropertyTextEnum::_custom_value_accepted()
{
	String new_value = custom_value_edit->get_text().strip_edges();
	_custom_value_submitted(new_value);
}

void EditorPropertyTextEnum::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		edit_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
		accept_button->set_button_icon(get_editor_theme_icon(SNAME("ImportCheck")));
		cancel_button->set_button_icon(get_editor_theme_icon(SNAME("ImportFail")));
	} break;
	}
}

//////////////////// LOCALE ////////////////////////

void EditorPropertyLocale::setup(const String& p_hint_text) {}

void EditorPropertyLocale::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		locale_edit->set_button_icon(get_editor_theme_icon(SNAME("Translation")));
	} break;
	}
}

void EditorPropertyLocale::_locale_focus_exited() { _locale_selected(locale->get_text()); }

///////////////////// PATH /////////////////////////

void EditorPropertyPath::_set_read_only(bool p_read_only)
{
	path->set_editable(!p_read_only);
	path_edit->set_disabled(p_read_only);
}

void EditorPropertyPath::setup(
	const Vector<String>& p_extensions, bool p_folder, bool p_global, bool p_enable_uid)
{
	extensions = p_extensions;
	folder = p_folder;
	global = p_global;
	enable_uid = p_enable_uid;
}

void EditorPropertyPath::set_save_mode() { save_mode = true; }

void EditorPropertyPath::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (folder) {
			path_edit->set_button_icon(get_editor_theme_icon(SNAME("FolderBrowse")));
		}
		else {
			path_edit->set_button_icon(get_editor_theme_icon(SNAME("FileBrowse")));
		}
		_update_uid_icon();
	} break;
	}
}

void EditorPropertyPath::_path_focus_exited() { _path_selected(path->get_text()); }

void EditorPropertyPath::_toggle_uid_display()
{
	display_uid = !display_uid;
	_update_uid_icon();
	update_property();
}

void EditorPropertyPath::_update_uid_icon()
{
	toggle_uid->set_button_icon(
		get_editor_theme_icon(display_uid ? SNAME("UID") : SNAME("NodePath")));
}

///////////////////// CLASS NAME /////////////////////////

void EditorPropertyClassName::_set_read_only(bool p_read_only)
{
	property->set_disabled(p_read_only);
}

void EditorPropertyClassName::setup(const String& p_base_type, const String& p_selected_type)
{
	base_type = p_base_type;
	dialog->set_base_type(base_type);
	selected_type = p_selected_type;
	property->set_text(selected_type);
}

///////////////////// CHECK /////////////////////////

void EditorPropertyCheck::_set_read_only(bool p_read_only) { checkbox->set_disabled(p_read_only); }

///////////////////// ENUM /////////////////////////

void EditorPropertyEnum::_set_read_only(bool p_read_only) { options->set_disabled(p_read_only); }

void EditorPropertyEnum::set_option_button_clip(bool p_enable) { options->set_clip_text(p_enable); }

OptionButton* EditorPropertyEnum::get_option_button() { return options; }

///////////////////// FLAGS /////////////////////////

void EditorPropertyFlags::_set_read_only(bool p_read_only)
{
	for (CheckBox* check : flags) {
		check->set_disabled(p_read_only);
	}
}

EditorPropertyFlags::EditorPropertyFlags()
{
	vbox = memnew(VBoxContainer);
	vbox->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	add_child(vbox);
}

///////////////////// LAYERS /////////////////////////

void EditorPropertyLayersGrid::_rename_pressed(int p_menu)
{
	// Show rename popup for active layer.
	ERR_FAIL_INDEX(renamed_layer_index, names.size());
	String name = names[renamed_layer_index];
	rename_dialog->set_title(vformat(TTR("Renaming Layer %d:"), renamed_layer_index + 1));
	rename_dialog_text->set_text(name);
	// Indicate that leaving it blank reverts back to "Layer [Number]".
	rename_dialog_text->set_placeholder(vformat(TTR("Layer %d"), renamed_layer_index + 1));
	rename_dialog_text->select(0, name.length());
	rename_dialog->popup_centered(Size2(300, 80) * EDSCALE);
	rename_dialog_text->grab_focus();
}

Size2 EditorPropertyLayersGrid::get_grid_size() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	return Vector2(0, font->get_height(font_size) * 3);
}

void EditorPropertyLayersGrid::set_read_only(bool p_read_only) { read_only = p_read_only; }

Size2 EditorPropertyLayersGrid::get_minimum_size() const
{
	Size2 min_size = get_grid_size();

	// Add extra rows when expanded.
	if (expanded) {
		const int bsize = (min_size.height * 80 / 100) / 2;
		for (int i = 0; i < expansion_rows; ++i) {
			min_size.y += 2 * (bsize + 1) + 3;
		}
	}

	return min_size;
}

void EditorPropertyLayersGrid::_update_hovered(const Vector2& p_position)
{
	bool expand_was_hovered = expand_hovered;
	expand_hovered = expand_rect.has_point(p_position);
	if (expand_hovered != expand_was_hovered) {
		queue_redraw();
	}

	if (!expand_hovered) {
		for (int i = 0; i < flag_rects.size(); i++) {
			if (flag_rects[i].has_point(p_position)) {
				// Used to highlight the hovered flag in the layers grid.
				hovered_index = i;
				queue_redraw();
				return;
			}
		}
	}

	// Remove highlight when no square is hovered.
	if (hovered_index != HOVERED_INDEX_NONE) {
		hovered_index = HOVERED_INDEX_NONE;
		queue_redraw();
	}
}

void EditorPropertyLayersGrid::_on_hover_exit()
{
	if (expand_hovered) {
		expand_hovered = false;
		queue_redraw();
	}
	if (hovered_index != HOVERED_INDEX_NONE) {
		hovered_index = HOVERED_INDEX_NONE;
		queue_redraw();
	}
	if (dragging) {
		dragging = false;
	}
}

void EditorPropertyLayersGrid::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		Size2 grid_size = get_grid_size();
		grid_size.x = get_size().x;

		flag_rects.clear();

		int prev_expansion_rows = expansion_rows;
		expansion_rows = 0;

		const int bsize = (grid_size.height * 80 / 100) / 2;
		const int h = bsize * 2 + 1;

		Color color = get_theme_color(
			read_only ? SNAME("highlight_disabled_color") : SNAME("highlight_color"),
			EditorStringName(Editor));

		Color text_color =
			get_theme_color(read_only ? SNAME("font_disabled_color") : SceneStringName(font_color),
				EditorStringName(Editor));
		text_color.a *= 0.5;

		Color text_color_on =
			get_theme_color(read_only ? SNAME("font_disabled_color") : SNAME("font_hover_color"),
				EditorStringName(Editor));
		text_color_on.a *= 0.7;

		const int vofs = (grid_size.height - h) / 2;

		uint32_t layer_index = 0;

		Point2 arrow_pos;

		Point2 block_ofs(4, vofs);

		while (true) {
			Point2 ofs = block_ofs;

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < layer_group_size; j++) {
					const bool on = value & (1u << layer_index);
					Rect2 rect2 = Rect2(ofs, Size2(bsize, bsize));

					color.a = on ? 0.6 : 0.2;
					if (layer_index == hovered_index) {
						// Add visual feedback when hovering a flag.
						color.a += 0.15;
					}

					draw_rect(rect2, color);
					flag_rects.push_back(rect2);

					Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
					int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
					Vector2 offset;
					offset.y = rect2.size.y * 0.75;

					draw_string(font.ptr(), rect2.position + offset, itos(layer_index + 1),
						HORIZONTAL_ALIGNMENT_CENTER, rect2.size.x, font_size,
						on ? text_color_on : text_color);

					ofs.x += bsize + 1;

					++layer_index;
				}

				ofs.x = block_ofs.x;
				ofs.y += bsize + 1;
			}

			if (layer_index >= layer_count) {
				if (!flag_rects.is_empty() && (expansion_rows == 0)) {
					const Rect2& last_rect = flag_rects[flag_rects.size() - 1];
					arrow_pos = last_rect.get_end();
				}
				break;
			}

			int block_size_x = layer_group_size * (bsize + 1);
			block_ofs.x += block_size_x + 3;

			if (block_ofs.x + block_size_x + 12 > grid_size.width) {
				// Keep last valid cell position for the expansion icon.
				if (!flag_rects.is_empty() && (expansion_rows == 0)) {
					const Rect2& last_rect = flag_rects[flag_rects.size() - 1];
					arrow_pos = last_rect.get_end();
				}
				++expansion_rows;

				if (expanded) {
					// Expand grid to next line.
					block_ofs.x = 4;
					block_ofs.y += 2 * (bsize + 1) + 3;
				}
				else {
					// Skip remaining blocks.
					break;
				}
			}
		}

		if ((expansion_rows != prev_expansion_rows) && expanded) {
			update_minimum_size();
		}

		if ((expansion_rows == 0) && (layer_index == layer_count)) {
			// Whole grid was drawn, no need for expansion icon.
			break;
		}

		Ref<Texture2D> arrow = get_theme_icon(SNAME("arrow"), SNAME("Tree"));
		ERR_FAIL_COND(arrow.is_null());

		Color arrow_color = get_theme_color(SNAME("highlight_color"), EditorStringName(Editor));
		arrow_color.a = expand_hovered ? 1.0 : 0.6;

		arrow_pos.x += 2.0;
		arrow_pos.y -= arrow->get_height();

		Rect2 arrow_draw_rect(arrow_pos, arrow->get_size());
		expand_rect = arrow_draw_rect;
		if (expanded) {
			arrow_draw_rect.size.y *= -1.0; // Flip arrow vertically when expanded.
		}

		RID ci = get_canvas_item();
		arrow->draw_rect(ci, arrow_draw_rect, false, arrow_color);

	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		_on_hover_exit();
	} break;
	}
}

void EditorPropertyLayersGrid::set_flag(uint32_t p_flag)
{
	value = p_flag;
	queue_redraw();
}

void EditorPropertyLayersGrid::_bind_methods() {}

void EditorPropertyLayers::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		button->set_texture_normal(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		button->set_texture_pressed(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		button->set_texture_disabled(get_editor_theme_icon(SNAME("GuiTabMenu")));
	} break;
	}
}

void EditorPropertyLayers::_set_read_only(bool p_read_only)
{
	button->set_disabled(p_read_only);
	grid->set_read_only(p_read_only);
}

void EditorPropertyLayers::_button_pressed()
{
	int layer_count = grid->layer_count;
	layers->clear();
	for (int i = 0; i < layer_count; i++) {
		const String name = get_layer_name(i);
		if (name.is_empty()) {
			continue;
		}
		layers->add_check_item(name, i);
		int idx = layers->get_item_index(i);
		layers->set_item_checked(idx, grid->value & (1u << i));
	}

	if (layers->get_item_count() == 0) {
		layers->add_item(TTR("No Named Layers"));
		layers->set_item_disabled(0, true);
	}
	layers->add_separator();
	layers->add_icon_item(
		get_editor_theme_icon("Edit"), TTR("Edit Layer Names"), grid->layer_count);

	Rect2 gp = button->get_screen_rect();
	layers->reset_size();
	Vector2 popup_pos = gp.position - Vector2(layers->get_contents_minimum_size().x, 0);
	layers->set_position(popup_pos);
	layers->popup();
}

void EditorPropertyLayers::_menu_pressed(int p_menu)
{
	if (uint32_t(p_menu) == grid->layer_count) {
		ProjectSettingsEditor::get_singleton()->popup_project_settings(true);
		ProjectSettingsEditor::get_singleton()->set_general_page(basename);
	}
	else {
		grid->value ^= 1u << p_menu;
		grid->queue_redraw();
		layers->set_item_checked(layers->get_item_index(p_menu), grid->value & (1u << p_menu));
		_grid_changed(grid->value);
	}
}

void EditorPropertyLayers::_refresh_names() { setup(layer_type); }

///////////////////// INT /////////////////////////

void EditorPropertyInteger::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyInteger::set_deferred_drag_mode_enabled(bool p_enabled)
{
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	spin->set_deferred_drag_mode_enabled(p_enabled);
}

void EditorPropertyInteger::setup(const EditorPropertyRangeHint& p_range_hint)
{
	spin->set_min(p_range_hint.min);
	spin->set_max(p_range_hint.max);
	spin->set_step(Math::round(p_range_hint.step));
	if (p_range_hint.hide_control) {
		spin->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
	}
	else {
		spin->set_control_state(p_range_hint.prefer_slider
									? EditorSpinSlider::CONTROL_STATE_PREFER_SLIDER
									: EditorSpinSlider::CONTROL_STATE_DEFAULT);
	}
	spin->set_allow_greater(p_range_hint.or_greater);
	spin->set_allow_lesser(p_range_hint.or_less);
	spin->set_suffix(p_range_hint.suffix);
}

///////////////////// OBJECT ID /////////////////////////

void EditorPropertyObjectID::_set_read_only(bool p_read_only) { edit->set_disabled(p_read_only); }

void EditorPropertyObjectID::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		edit->add_theme_constant_override("icon_max_width",
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));
	} break;
	}
}

void EditorPropertyObjectID::setup(const String& p_base_type) { base_type = p_base_type; }

///////////////////// SIGNAL /////////////////////////

///////////////////// CALLABLE /////////////////////////

EditorPropertyCallable::EditorPropertyCallable()
{
	edit = memnew(Button);
	edit->set_theme_type_variation(SNAME("EditorInspectorButton"));
	edit->set_accessibility_name(TTRC("Edit"));
	add_child(edit);
	add_focusable(edit);
}

///////////////////// FLOAT /////////////////////////

void EditorPropertyFloat::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyFloat::set_deferred_drag_mode_enabled(bool p_enabled)
{
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	spin->set_deferred_drag_mode_enabled(p_enabled);
}

void EditorPropertyFloat::setup(const EditorPropertyRangeHint& p_range_hint)
{
	radians_as_degrees = p_range_hint.radians_as_degrees;
	spin->set_min(p_range_hint.min);
	spin->set_max(p_range_hint.max);
	spin->set_step(p_range_hint.step);
	if (p_range_hint.hide_control) {
		spin->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
	}
	spin->set_exp_ratio(p_range_hint.exp_range);
	spin->set_allow_greater(p_range_hint.or_greater);
	spin->set_allow_lesser(p_range_hint.or_less);
	spin->set_suffix(p_range_hint.suffix);
}

///////////////////// EASING /////////////////////////

void EditorPropertyEasing::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyEasing::_spin_focus_exited()
{
	spin->hide();
	// Ensure the easing doesn't appear as being dragged
	dragging = false;
	easing_draw->queue_redraw();
}

void EditorPropertyEasing::setup(bool p_positive_only, bool p_flip)
{
	flip = p_flip;
	positive_only = p_positive_only;

	// Names need translation context, so they are set in NOTIFICATION_TRANSLATION_CHANGED.
	preset->add_item("", EASING_LINEAR);
	preset->add_item("", EASING_IN);
	preset->add_item("", EASING_OUT);
	preset->add_item("", EASING_ZERO);
	if (!positive_only) {
		preset->add_item("", EASING_IN_OUT);
		preset->add_item("", EASING_OUT_IN);
	}
}

void EditorPropertyEasing::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		preset->set_item_icon(
			preset->get_item_index(EASING_LINEAR), get_editor_theme_icon(SNAME("CurveLinear")));
		preset->set_item_icon(
			preset->get_item_index(EASING_IN), get_editor_theme_icon(SNAME("CurveIn")));
		preset->set_item_icon(
			preset->get_item_index(EASING_OUT), get_editor_theme_icon(SNAME("CurveOut")));
		preset->set_item_icon(
			preset->get_item_index(EASING_ZERO), get_editor_theme_icon(SNAME("CurveConstant")));
		if (!positive_only) {
			preset->set_item_icon(
				preset->get_item_index(EASING_IN_OUT), get_editor_theme_icon(SNAME("CurveInOut")));
			preset->set_item_icon(
				preset->get_item_index(EASING_OUT_IN), get_editor_theme_icon(SNAME("CurveOutIn")));
		}
		easing_draw->set_custom_minimum_size(Size2(0,
			get_theme_font(SceneStringName(font), SNAME("Label"))
					->get_height(get_theme_font_size(SceneStringName(font_size), SNAME("Label"))) *
				2));
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		preset->set_item_text(preset->get_item_index(EASING_LINEAR), TTR("Linear", "Ease Type"));
		preset->set_item_text(preset->get_item_index(EASING_IN), TTR("Ease In", "Ease Type"));
		preset->set_item_text(preset->get_item_index(EASING_OUT), TTR("Ease Out", "Ease Type"));
		preset->set_item_text(preset->get_item_index(EASING_ZERO), TTR("Zero", "Ease Type"));
		if (!positive_only) {
			preset->set_item_text(
				preset->get_item_index(EASING_IN_OUT), TTR("Ease In-Out", "Ease Type"));
			preset->set_item_text(
				preset->get_item_index(EASING_OUT_IN), TTR("Ease Out-In", "Ease Type"));
		}
	} break;
	}
}

///////////////////// RECT2 /////////////////////////

void EditorPropertyRect2::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyRect2::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 2]);
		}
	} break;
	}
}

void EditorPropertyRect2::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_range_hint.suffix);
	}
}

///////////////////// RECT2i /////////////////////////

void EditorPropertyRect2i::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyRect2i::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 2]);
		}
	} break;
	}
}

void EditorPropertyRect2i::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(1);
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_range_hint.suffix);
		spin[i]->set_editing_integer(true);
	}
}

///////////////////// PLANE /////////////////////////

void EditorPropertyPlane::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyPlane::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i]);
		}
	} break;
	}
}

void EditorPropertyPlane::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
	}
	spin[3]->set_suffix(p_range_hint.suffix);
}

///////////////////// QUATERNION /////////////////////////

void EditorPropertyQuaternion::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
	for (int i = 0; i < 3; i++) {
		euler[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyQuaternion::_edit_custom_value()
{
	if (edit_button->is_pressed()) {
		edit_custom_bc->show();
		for (int i = 0; i < 3; i++) {
			euler[i]->grab_focus();
		}
	}
	else {
		edit_custom_bc->hide();
		for (int i = 0; i < 4; i++) {
			spin[i]->grab_focus();
		}
	}
	update_property();
}

void EditorPropertyQuaternion::_custom_value_changed(double val)
{
	edit_euler.x = euler[0]->get_value();
	edit_euler.y = euler[1]->get_value();
	edit_euler.z = euler[2]->get_value();

	Vector3 v;
	v.x = Math::deg_to_rad(edit_euler.x);
	v.y = Math::deg_to_rad(edit_euler.y);
	v.z = Math::deg_to_rad(edit_euler.z);

	Quaternion temp_q = Quaternion::from_euler(v);
	spin[0]->set_value_no_signal(temp_q.x);
	spin[1]->set_value_no_signal(temp_q.y);
	spin[2]->set_value_no_signal(temp_q.z);
	spin[3]->set_value_no_signal(temp_q.w);
	_value_changed(-1, "");
}

bool EditorPropertyQuaternion::is_grabbing_euler()
{
	bool is_grabbing = false;
	for (int i = 0; i < 3; i++) {
		is_grabbing |= euler[i]->is_grabbing();
	}
	return is_grabbing;
}

void EditorPropertyQuaternion::_warning_pressed() { warning_dialog->popup_centered(); }

void EditorPropertyQuaternion::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i]);
		}
		for (int i = 0; i < 3; i++) {
			euler[i]->add_theme_color_override("label_color", colors[i]);
		}
		edit_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
		euler_label->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("property_color"), SNAME("EditorProperty")));
		warning->set_button_icon(get_editor_theme_icon(SNAME("NodeWarning")));
		warning->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
	} break;
	}
}

void EditorPropertyQuaternion::setup(
	const EditorPropertyRangeHint& p_range_hint, bool p_hide_editor)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		// Quaternion is inherently unitless, however someone may want to use it as
		// a generic way to store 4 values, so we'll still respect the suffix.
		spin[i]->set_suffix(p_range_hint.suffix);
	}

	for (int i = 0; i < 3; i++) {
		euler[i]->set_min(-360);
		euler[i]->set_max(360);
		euler[i]->set_step(0.1);
		euler[i]->set_allow_greater(true);
		euler[i]->set_allow_lesser(true);
		euler[i]->set_suffix(U"\u00B0");
	}

	if (p_hide_editor) {
		edit_button->hide();
	}
}

///////////////////// AABB /////////////////////////

void EditorPropertyAABB::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyAABB::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 3]);
		}
	} break;
	}
}

void EditorPropertyAABB::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		spin[i]->set_suffix(p_range_hint.suffix);
	}
}

///////////////////// TRANSFORM2D /////////////////////////

void EditorPropertyTransform2D::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyTransform2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			// For Transform2D, use the 4th color (cyan) for the origin vector.
			if (i % 3 == 2) {
				spin[i]->add_theme_color_override("label_color", colors[3]);
			}
			else {
				spin[i]->add_theme_color_override("label_color", colors[i % 3]);
			}
		}
	} break;
	}
}

void EditorPropertyTransform2D::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		if (i % 3 == 2) {
			spin[i]->set_suffix(p_range_hint.suffix);
		}
	}
}

///////////////////// BASIS /////////////////////////

void EditorPropertyBasis::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 9; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyBasis::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 9; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 3]);
		}
	} break;
	}
}

void EditorPropertyBasis::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 9; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		// Basis is inherently unitless, however someone may want to use it as
		// a generic way to store 9 values, so we'll still respect the suffix.
		spin[i]->set_suffix(p_range_hint.suffix);
	}
}

///////////////////// TRANSFORM3D /////////////////////////

void EditorPropertyTransform3D::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 12; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyTransform3D::update_using_transform(Transform3D p_transform)
{
	spin[0]->set_value_no_signal(p_transform.basis[0][0]);
	spin[1]->set_value_no_signal(p_transform.basis[0][1]);
	spin[2]->set_value_no_signal(p_transform.basis[0][2]);
	spin[3]->set_value_no_signal(p_transform.origin[0]);
	spin[4]->set_value_no_signal(p_transform.basis[1][0]);
	spin[5]->set_value_no_signal(p_transform.basis[1][1]);
	spin[6]->set_value_no_signal(p_transform.basis[1][2]);
	spin[7]->set_value_no_signal(p_transform.origin[1]);
	spin[8]->set_value_no_signal(p_transform.basis[2][0]);
	spin[9]->set_value_no_signal(p_transform.basis[2][1]);
	spin[10]->set_value_no_signal(p_transform.basis[2][2]);
	spin[11]->set_value_no_signal(p_transform.origin[2]);
}

void EditorPropertyTransform3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 12; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 4]);
		}
	} break;
	}
}

void EditorPropertyTransform3D::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 12; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		if (i % 4 == 3) {
			spin[i]->set_suffix(p_range_hint.suffix);
		}
	}
}

///////////////////// PROJECTION /////////////////////////

void EditorPropertyProjection::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 12; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyProjection::update_using_transform(Projection p_transform)
{
	spin[0]->set_value_no_signal(p_transform.columns[0][0]);
	spin[1]->set_value_no_signal(p_transform.columns[0][1]);
	spin[2]->set_value_no_signal(p_transform.columns[0][2]);
	spin[3]->set_value_no_signal(p_transform.columns[0][3]);
	spin[4]->set_value_no_signal(p_transform.columns[1][0]);
	spin[5]->set_value_no_signal(p_transform.columns[1][1]);
	spin[6]->set_value_no_signal(p_transform.columns[1][2]);
	spin[7]->set_value_no_signal(p_transform.columns[1][3]);
	spin[8]->set_value_no_signal(p_transform.columns[2][0]);
	spin[9]->set_value_no_signal(p_transform.columns[2][1]);
	spin[10]->set_value_no_signal(p_transform.columns[2][2]);
	spin[11]->set_value_no_signal(p_transform.columns[2][3]);
	spin[12]->set_value_no_signal(p_transform.columns[3][0]);
	spin[13]->set_value_no_signal(p_transform.columns[3][1]);
	spin[14]->set_value_no_signal(p_transform.columns[3][2]);
	spin[15]->set_value_no_signal(p_transform.columns[3][3]);
}

void EditorPropertyProjection::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 16; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 4]);
		}
	} break;
	}
}

void EditorPropertyProjection::setup(const EditorPropertyRangeHint& p_range_hint)
{
	for (int i = 0; i < 16; i++) {
		spin[i]->set_min(p_range_hint.min);
		spin[i]->set_max(p_range_hint.max);
		spin[i]->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin[i]->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin[i]->set_allow_greater(true);
		spin[i]->set_allow_lesser(true);
		if (i % 4 == 3) {
			spin[i]->set_suffix(p_range_hint.suffix);
		}
	}
}

////////////// COLOR PICKER //////////////////////

void EditorPropertyColor::_set_read_only(bool p_read_only) { picker->set_disabled(p_read_only); }

void EditorPropertyColor::_popup_opening()
{
	if (EditorNode::get_singleton()) {
		EditorNode::get_singleton()->setup_color_picker(picker->get_picker());
	}
	last_color = picker->get_pick_color();
	was_checked = !is_checkable() || is_checked();
}

void EditorPropertyColor::setup(bool p_show_alpha) { picker->set_edit_alpha(p_show_alpha); }

void EditorPropertyColor::set_live_changes_enabled(bool p_enabled)
{
	live_changes_enabled = p_enabled;
}

////////////// NODE PATH //////////////////////

void EditorPropertyNodePath::_set_read_only(bool p_read_only)
{
	assign->set_disabled(p_read_only);
	menu->set_disabled(p_read_only);
}

void EditorPropertyNodePath::_assign_draw()
{
	if (dropping) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		assign->draw_rect(Rect2(Point2(), assign->get_size()), color, false);
	}
}

void EditorPropertyNodePath::_accept_text() { _text_submitted(edit->get_text()); }

void EditorPropertyNodePath::_text_submitted(const String& p_text)
{
	NodePath np = p_text;
	_node_selected(np, false);
	edit->hide();
	assign->show();
	menu->show();
}

void EditorPropertyNodePath::setup(
	const Vector<StringName>& p_valid_types, bool p_use_path_from_scene_root, bool p_editing_node)
{
	valid_types = p_valid_types;
	editing_node = p_editing_node;
	use_path_from_scene_root = p_use_path_from_scene_root;
}

///////////////////// RID /////////////////////////

EditorPropertyRID::EditorPropertyRID()
{
	label = memnew(Label);
	add_child(label);
}

////////////// RESOURCE //////////////////////

void EditorPropertyResource::_set_read_only(bool p_read_only)
{
	resource_picker->set_editable(!p_read_only);
}

void EditorPropertyResource::_resource_selected(const Ref<Resource>& p_resource, bool p_inspect)
{
	_select_resource(p_resource, p_inspect, false);
}

void EditorPropertyResource::_resource_expand_requested(
	const Ref<Resource>& p_resource, bool p_inspect)
{
	_select_resource(p_resource, p_inspect, true);
}

bool EditorPropertyResource::_should_stop_editing() const
{
	return !resource_picker->is_toggle_pressed();
}

void EditorPropertyResource::collapse_all_folding()
{
	if (sub_inspector) {
		sub_inspector->collapse_all_folding();
	}
}

void EditorPropertyResource::expand_all_folding()
{
	if (sub_inspector) {
		sub_inspector->expand_all_folding();
	}
}

void EditorPropertyResource::expand_revertable()
{
	if (sub_inspector) {
		sub_inspector->expand_revertable();
	}
}

void EditorPropertyResource::set_use_sub_inspector(bool p_enable) { use_sub_inspector = p_enable; }

void EditorPropertyResource::set_use_filter(bool p_use)
{
	use_filter = p_use;
	if (sub_inspector) {
		update_property();

	}
}

void EditorPropertyResource::set_keying(bool p_keying)
{
	EditorProperty::set_keying(p_keying);
	if (sub_inspector) {
		sub_inspector->set_keying(p_keying);
	}
}

void EditorPropertyResource::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_EXIT_TREE: {
		const EditorInspector* ei = get_parent_inspector();
		const EditorInspector* main_ei = InspectorDock::get_inspector_singleton();
		if (ei && main_ei && ei != main_ei && !main_ei->is_ancestor_of(ei)) {
			fold_resource();
		}
	} break;
	}
}


