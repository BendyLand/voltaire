/**************************************************************************/
/*  editor_visual_profiler.cpp                                            */
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

#include "core/io/image.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_visual_profiler.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/resources/image_texture.h"

void EditorVisualProfiler::add_frame_metric(const Metric& p_metric)
{
	++last_metric;
	if (last_metric >= frame_metrics.size()) {
		last_metric = 0;
	}

	frame_metrics.write[last_metric] = p_metric;

	List<String> stack;
	for (int i = 0; i < frame_metrics[last_metric].areas.size(); i++) {
		String name = frame_metrics[last_metric].areas[i].name;
		frame_metrics.write[last_metric].areas.write[i].color_cache =
			_get_color_from_signature(name);
		String full_name;

		if (name[0] == '<') {
			stack.pop_back();
		}

		if (stack.size()) {
			full_name = stack.back()->get() + name;
		}
		else {
			full_name = name;
		}

		if (name[0] == '>') {
			stack.push_back(full_name + "/");
		}

		frame_metrics.write[last_metric].areas.write[i].fullpath_cache = full_name;
	}

	updating_frame = true;
	clear_button->set_disabled(false);
	cursor_metric_edit->set_max(frame_metrics[last_metric].frame_number);
	cursor_metric_edit->set_min(
		MAX(int64_t(frame_metrics[last_metric].frame_number) - frame_metrics.size(), 0));

	if (!seeking) {
		cursor_metric_edit->set_value(frame_metrics[last_metric].frame_number);
		if (hover_metric != -1) {
			hover_metric++;
			if (hover_metric >= frame_metrics.size()) {
				hover_metric = 0;
			}
		}
	}
	updating_frame = false;

	if (frame_delay->is_stopped()) {
		frame_delay->set_wait_time(0.1);
		frame_delay->start();
	}

	if (plot_delay->is_stopped()) {
		plot_delay->set_wait_time(0.1);
		plot_delay->start();
	}
}

void EditorVisualProfiler::clear()
{
	frame_metrics.clear();
	last_metric = -1;
	variables->clear();
	// activate->set_pressed(false);
	category_folding.clear();

	updating_frame = true;
	cursor_metric_edit->set_min(0);
	cursor_metric_edit->set_max(0);
	cursor_metric_edit->set_value(0);
	updating_frame = false;
	hover_metric = -1;
	seeking = false;
}

Color EditorVisualProfiler::_get_color_from_signature(const StringName& p_signature) const
{
	Color bc = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	double rot = Math::abs(double(p_signature.hash()) / double(0x7FFFFFFF));
	Color c;
	c.set_hsv(rot, bc.get_s(), bc.get_v());
	return c.lerp(get_theme_color(SNAME("base_color"), EditorStringName(Editor)), 0.07);
}

void EditorVisualProfiler::_item_selected()
{
	if (updating_frame) {
		return;
	}

	TreeItem* item = variables->get_selected();
	if (!item) {
		return;
	}
	_update_plot();
}

void EditorVisualProfiler::_update_frame(bool p_focus_selected)
{
	int cursor_metric = _get_cursor_index();

	Ref<Texture> track_icon = get_editor_theme_icon(SNAME("TrackColor"));

	ERR_FAIL_INDEX(cursor_metric, frame_metrics.size());

	updating_frame = true;
	variables->clear();

	TreeItem* root = variables->create_item();
	const Metric& m = frame_metrics[cursor_metric];

	List<TreeItem*> stack;
	List<TreeItem*> categories;

	TreeItem* ensure_selected = nullptr;

	for (int i = 1; i < m.areas.size() - 1; i++) {
		TreeItem* parent = stack.size() ? stack.back()->get() : root;

		String name = m.areas[i].name;

		float cpu_time = m.areas[i].cpu_time;
		float gpu_time = m.areas[i].gpu_time;
		if (i < m.areas.size() - 1) {
			cpu_time = m.areas[i + 1].cpu_time - cpu_time;
			gpu_time = m.areas[i + 1].gpu_time - gpu_time;
		}

		if (name.begins_with(">")) {
			TreeItem* category = variables->create_item(parent);

			stack.push_back(category);
			categories.push_back(category);

			name = name.substr(1);

			category->set_text(0, name);

			if (category_folding.has(m.areas[i].fullpath_cache)) {
				category->set_collapsed(category_folding[m.areas[i].fullpath_cache]);
			}
			continue;
		}

		if (name.begins_with("<")) {
			stack.pop_back();
			continue;
		}
		TreeItem* category = variables->create_item(parent);

		category->set_icon(0, track_icon);
		category->set_icon_modulate(0, m.areas[i].color_cache);
		category->set_selectable(0, true);
		category->set_text(0, m.areas[i].name);
		category->set_text(1, _get_time_as_text(cpu_time));
		category->set_text(2, _get_time_as_text(gpu_time));

		if (selected_area == m.areas[i].fullpath_cache) {
			category->select(0);
			if (p_focus_selected) {
				ensure_selected = category;
			}
		}
	}

	if (ensure_selected) {
		// Make visible when it's collapsed.
		TreeItem* node = ensure_selected->get_parent();
		while (node) {
			node->set_collapsed(false);
			node = node->get_parent();
		}
		ensure_selected->select(0);
		variables->ensure_cursor_is_visible();
	}
	updating_frame = false;
}

void EditorVisualProfiler::_activate_pressed()
{
	if (activate->is_pressed()) {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Stop")));
		activate->set_text(TTRC("Stop"));
		_clear_pressed(); // always clear on start
		clear_button->set_disabled(false);
	}
	else {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Play")));
		activate->set_text(TTRC("Start"));
	}
}

void EditorVisualProfiler::_clear_pressed()
{
	clear_button->set_disabled(true);
	clear();
	_update_plot();
}

void EditorVisualProfiler::_autostart_toggled(bool p_toggled_on)
{
	EditorRunBar::get_singleton()->update_profiler_autostart_indicator();
}

void EditorVisualProfiler::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (is_ready()) {
			_update_frame();
		}
		[[fallthrough]];
	}
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_THEME_CHANGED: {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Play")));
		clear_button->set_button_icon(get_editor_theme_icon(SNAME("Clear")));
	} break;
	}
}

void EditorVisualProfiler::_graph_tex_draw()
{
	if (last_metric < 0) {
		return;
	}

	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	const Color color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	Size2 graph_size = graph->get_size();

	if (seeking) {
		int max_frames = frame_metrics.size();

		int64_t first_visible_frame =
			static_cast<int64_t>(frame_metrics[last_metric].frame_number) - max_frames + 1;
		int frame = (cursor_metric_edit->get_value() - first_visible_frame);
		if (frame < 0) {
			frame = 0;
		}

		int half_width = graph_size.x / 2;
		int cur_x = frame * half_width / max_frames;

		graph->draw_line(Vector2(cur_x, 0), Vector2(cur_x, graph_size.y), color * Color(1, 1, 1));
		graph->draw_line(Vector2(cur_x + half_width, 0), Vector2(cur_x + half_width, graph_size.y),
			color * Color(1, 1, 1));
	}

	if (graph_height_cpu > 0) {
		int cpu_height = graph_limit * graph_size.y / graph_height_cpu;
		cpu_height = CLAMP(cpu_height, 0, graph_size.y - (font->get_ascent(font_size) + 2) * 2);
		int frame_y = graph_size.y - cpu_height - 1;

		int half_width = graph_size.x / 2;

		graph->draw_line(
			Vector2(0, frame_y), Vector2(half_width, frame_y), color * Color(1, 1, 1, 0.5));

		const String limit_str = String::num(graph_limit, 2) + " ms";
		graph->draw_string(font.ptr(),
			Vector2(
				half_width -
					font->get_string_size(limit_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x -
					2,
				frame_y - 2),
			limit_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color * Color(1, 1, 1, 0.75));
	}

	if (graph_height_gpu > 0) {
		int gpu_height = graph_limit * graph_size.y / graph_height_gpu;
		gpu_height = CLAMP(gpu_height, 0, graph_size.y - (font->get_ascent(font_size) + 2) * 2);
		int frame_y = graph_size.y - gpu_height - 1;

		int half_width = graph_size.x / 2;

		graph->draw_line(Vector2(half_width, frame_y), Vector2(graph_size.x, frame_y),
			color * Color(1, 1, 1, 0.5));

		const String limit_str = String::num(graph_limit, 2) + " ms";
		graph->draw_string(font.ptr(),
			Vector2(
				half_width * 2 -
					font->get_string_size(limit_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x -
					2,
				frame_y - 2),
			limit_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color * Color(1, 1, 1, 0.75));
	}

	graph->draw_string(font.ptr(),
		Vector2(font->get_string_size("X", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x,
			font->get_ascent(font_size) + 2),
		"CPU: " + cpu_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color * Color(1, 1, 1, 0.75));
	graph->draw_string(font.ptr(),
		Vector2(font->get_string_size("X", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x +
					graph_size.width / 2,
			font->get_ascent(font_size) + 2),
		"GPU: " + gpu_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color * Color(1, 1, 1, 0.75));
}

int EditorVisualProfiler::_get_cursor_index() const
{
	if (last_metric < 0) {
		return 0;
	}
	if (!frame_metrics[last_metric].valid) {
		return 0;
	}

	int diff = (frame_metrics[last_metric].frame_number - cursor_metric_edit->get_value());

	int idx = last_metric - diff;
	while (idx < 0) {
		idx += frame_metrics.size();
	}

	return idx;
}

void EditorVisualProfiler::_combo_changed(int)
{
	_update_frame();
	_update_plot();
}

void EditorVisualProfiler::_update_button_text()
{
	if (activate->is_pressed()) {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Stop")));
		activate->set_text(TTRC("Stop"));
	}
	else {
		activate->set_button_icon(get_editor_theme_icon(SNAME("Play")));
		activate->set_text(TTRC("Start"));
	}
}

void EditorVisualProfiler::set_enabled(bool p_enable) { activate->set_disabled(!p_enable); }

void EditorVisualProfiler::set_profiling(bool p_profiling)
{
	activate->set_pressed(p_profiling);
	_update_button_text();
}

bool EditorVisualProfiler::is_profiling() { return activate->is_pressed(); }

Vector<Vector<String>> EditorVisualProfiler::get_data_as_csv() const
{
	Vector<Vector<String>> res;
#if 0
	if (frame_metrics.is_empty()) {
		return res;
	}

	// signatures
	Vector<String> signatures;
	const Vector<EditorFrameProfiler::Metric::Category> &categories = frame_metrics[0].categories;

	for (int j = 0; j < categories.size(); j++) {
		const EditorFrameProfiler::Metric::Category &c = categories[j];
		signatures.push_back(c.signature);

		for (int k = 0; k < c.items.size(); k++) {
			signatures.push_back(c.items[k].signature);
		}
	}
	res.push_back(signatures);

	// values
	Vector<String> values;
	values.resize(signatures.size());

	int index = last_metric;

	for (int i = 0; i < frame_metrics.size(); i++) {
		++index;

		if (index >= frame_metrics.size()) {
			index = 0;
		}

		if (!frame_metrics[index].valid) {
			continue;
		}
		int it = 0;
		const Vector<EditorFrameProfiler::Metric::Category> &frame_cat = frame_metrics[index].categories;

		for (int j = 0; j < frame_cat.size(); j++) {
			const EditorFrameProfiler::Metric::Category &c = frame_cat[j];
			values.write[it++] = String::num_real(c.total_time);

			for (int k = 0; k < c.items.size(); k++) {
				values.write[it++] = String::num_real(c.items[k].total);
			}
		}
		res.push_back(values);
	}
#endif
	return res;
}

EditorVisualProfiler::EditorVisualProfiler()
{
	HBoxContainer* hb = memnew(HBoxContainer);
	hb->add_theme_constant_override(SNAME("separation"), 8 * EDSCALE);
	add_child(hb);

	FlowContainer* container = memnew(FlowContainer);
	container->set_h_size_flags(SIZE_EXPAND_FILL);
	container->add_theme_constant_override(SNAME("h_separation"), 8 * EDSCALE);
	container->add_theme_constant_override(SNAME("v_separation"), 2 * EDSCALE);
	hb->add_child(container);

	activate = memnew(Button);
	activate->set_toggle_mode(true);
	activate->set_disabled(true);
	activate->set_text(TTRC("Start"));
	container->add_child(activate);

	clear_button = memnew(Button);
	clear_button->set_text(TTRC("Clear"));
	clear_button->set_disabled(true);
	container->add_child(clear_button);

	CheckBox* autostart_checkbox = memnew(CheckBox);
	autostart_checkbox->set_text(TTRC("Autostart"));
	container->add_child(autostart_checkbox);

	HBoxContainer* hb_measure = memnew(HBoxContainer);
	hb_measure->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	container->add_child(hb_measure);

	hb_measure->add_child(memnew(Label(TTRC("Measure:"))));

	display_mode = memnew(OptionButton);
	display_mode->set_accessibility_name(TTRC("Measure:"));
	display_mode->add_item(TTRC("Frame Time (ms)"));
	display_mode->add_item(TTRC("Frame %"));

	hb_measure->add_child(display_mode);

	frame_relative = memnew(CheckBox(TTRC("Fit to Frame")));
	frame_relative->set_pressed(true);
	container->add_child(frame_relative);
	linked = memnew(CheckBox(TTRC("Linked")));
	linked->set_pressed(true);
	container->add_child(linked);

	HBoxContainer* hb_frame = memnew(HBoxContainer);
	hb_frame->add_theme_constant_override(SNAME("separation"), 2 * EDSCALE);
	hb_frame->set_v_size_flags(SIZE_SHRINK_BEGIN);
	hb->add_child(hb_frame);

	hb_frame->add_child(memnew(Label(TTRC("Frame #:"))));

	cursor_metric_edit = memnew(SpinBox);
	cursor_metric_edit->set_accessibility_name(TTRC("Frame #:"));
	cursor_metric_edit->set_h_size_flags(SIZE_FILL);
	hb_frame->add_child(cursor_metric_edit);

	h_split = memnew(HSplitContainer);
	add_child(h_split);
	h_split->set_v_size_flags(SIZE_EXPAND_FILL);

	variables = memnew(Tree);
	variables->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
	h_split->add_child(variables);
	variables->set_hide_root(true);
	variables->set_columns(3);
	variables->set_column_titles_visible(true);
	variables->set_column_title(0, TTRC("Name"));
	variables->set_column_expand(0, true);
	variables->set_column_clip_content(0, true);
	variables->set_column_custom_minimum_width(0, 60);
	variables->set_column_title(1, TTRC("CPU"));
	variables->set_column_expand(1, false);
	variables->set_column_clip_content(1, true);
	variables->set_column_custom_minimum_width(1, 75 * EDSCALE);
	variables->set_column_title(2, TTRC("GPU"));
	variables->set_column_expand(2, false);
	variables->set_column_clip_content(2, true);
	variables->set_column_custom_minimum_width(2, 75 * EDSCALE);
	variables->set_theme_type_variation("TreeSecondary");

	graph = memnew(TextureRect);
	graph->set_custom_minimum_size(Size2(250 * EDSCALE, 0));
	graph->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	graph->set_mouse_filter(MOUSE_FILTER_STOP);

	h_split->add_child(graph);
	graph->set_h_size_flags(SIZE_EXPAND_FILL);

	frame_delay = memnew(Timer);
	frame_delay->set_wait_time(0.1);
	frame_delay->set_one_shot(true);
	add_child(frame_delay);

	plot_delay = memnew(Timer);
	plot_delay->set_wait_time(0.1);
	plot_delay->set_one_shot(true);
	add_child(plot_delay);
}


