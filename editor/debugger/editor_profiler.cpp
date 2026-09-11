/**************************************************************************/
/*  editor_profiler.cpp                                                   */
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
#include "editor_profiler.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/resources/image_texture.h"

void EditorProfiler::_make_metric_ptrs(Metric& m)
{
	for (int i = 0; i < m.categories.size(); i++) {
		m.category_ptrs[m.categories[i].signature] = &m.categories.write[i];
		for (int j = 0; j < m.categories[i].items.size(); j++) {
			m.item_ptrs[m.categories[i].items[j].signature] = &m.categories.write[i].items.write[j];
		}
	}
}

const EditorProfiler::Metric& EditorProfiler::_get_frame_metric(int index) const
{
	return frame_metrics[(frame_metrics.size() + last_metric - (total_metrics - 1) + index) %
						 frame_metrics.size()];
}

void EditorProfiler::clear()
{
	frame_metrics.clear();
	total_metrics = 0;
	last_metric = -1;
	variables->clear();
	plot_sigs.clear();
	plot_sigs.insert("physics_frame_time");
	plot_sigs.insert("category_frame_time");

	updating_frame = true;
	cursor_metric_edit->set_min(0);
	cursor_metric_edit->set_max(
		100); // Doesn't make much sense, but we can't have min == max. Doesn't hurt.
	cursor_metric_edit->set_value(0);
	cursor_metric_edit->set_editable(false);
	updating_frame = false;
	hover_metric = -1;
	seeking = false;

	// Ensure button text (start, stop) is correct
	_update_button_text();
}

Color EditorProfiler::_get_color_from_signature(const StringName& p_signature) const
{
	Color bc = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	double rot = Math::abs(double(p_signature.hash()) / double(0x7FFFFFFF));
	Color c;
	c.set_hsv(rot, bc.get_s(), bc.get_v());
	return c.lerp(get_theme_color(SNAME("base_color"), EditorStringName(Editor)), 0.07);
}

int EditorProfiler::_get_zoom_left_border() const
{
	const int max_profiles_shown = frame_metrics.size() / Math::exp(graph_zoom);
	return CLAMP(
		zoom_center - max_profiles_shown / 2, 0, frame_metrics.size() - max_profiles_shown);
}

void EditorProfiler::_item_edited()
{
	if (updating_frame) {
		return;
	}

	TreeItem* item = variables->get_edited();
	if (!item) {
		return;
	}

	if (!frame_delay->is_processing()) {
		frame_delay->set_wait_time(0.1);
		frame_delay->start();
	}

	_update_plot();
}

void EditorProfiler::_update_frame()
{
	int cursor_metric = cursor_metric_edit->get_value() - _get_frame_metric(0).frame_number;

	updating_frame = true;
	variables->clear();

	TreeItem* root = variables->create_item();
	const Metric& m = _get_frame_metric(cursor_metric);

	int dtime = display_time->get_selected();

	for (int i = 0; i < m.categories.size(); i++) {
		TreeItem* category = variables->create_item(root);
		category->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		category->set_editable(0, true);
		category->set_text(0, String(m.categories[i].name));
		category->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
		category->set_text(1, _get_time_as_text(m, m.categories[i].total_time, 1));

		if (collapsed_categories.has(m.categories[i].signature)) {
			category->set_collapsed(true);
		}

		if (plot_sigs.has(m.categories[i].signature)) {
			category->set_checked(0, true);
			category->set_custom_color(0, _get_color_from_signature(m.categories[i].signature));
		}

		for (int j = 0; j < m.categories[i].items.size(); j++) {
			const Metric::Category::Item& it = m.categories[i].items[j];

			if (it.internal == it.total && !display_internal_profiles->is_pressed() &&
				m.categories[i].name == "Script Functions") {
				continue;
			}
			TreeItem* item = variables->create_item(category);
			item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
			item->set_editable(0, true);
			item->set_text(0, it.name);
			item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
			item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_RIGHT);
			item->set_tooltip_text(0, it.name + "\n" + it.script + ":" + itos(it.line));

			float time = dtime == DISPLAY_SELF_TIME ? it.self : it.total;
			if (dtime == DISPLAY_SELF_TIME && !display_internal_profiles->is_pressed()) {
				time += it.internal;
			}

			item->set_text(1, _get_time_as_text(m, time, it.calls));

			item->set_text(2, itos(it.calls));

			if (plot_sigs.has(it.signature)) {
				item->set_checked(0, true);
				item->set_custom_color(0, _get_color_from_signature(it.signature));
			}
		}
	}

	updating_frame = false;
}

void EditorProfiler::_internal_profiles_pressed() { _combo_changed(0); }

void EditorProfiler::_autostart_toggled(bool p_toggled_on)
{
	EditorRunBar::get_singleton()->update_profiler_autostart_indicator();
}

void EditorProfiler::_graph_tex_draw()
{
	if (total_metrics == 0) {
		return;
	}
	if (seeking) {
		int frame = cursor_metric_edit->get_value() - _get_frame_metric(0).frame_number;
		frame = frame - _get_zoom_left_border() + 1;
		int cur_x =
			(frame * graph->get_size().width * Math::exp(graph_zoom)) / frame_metrics.size();
		cur_x = CLAMP(cur_x, 0, graph->get_size().width);
		graph->draw_line(
			Vector2(cur_x, 0), Vector2(cur_x, graph->get_size().y), theme_cache.seek_line_color);
	}
	if (hover_metric > -1) {
		int cur_x = (2 * hover_metric + 1) * graph->get_size().x / (2 * frame_metrics.size()) + 1;
		graph->draw_line(Vector2(cur_x, 0), Vector2(cur_x, graph->get_size().y),
			theme_cache.seek_line_hover_color);
	}
}

void EditorProfiler::_combo_changed(int)
{
	_update_frame();
	_update_plot();
}

void EditorProfiler::set_profiling(bool p_pressed)
{
	activate->set_pressed(p_pressed);
	_update_button_text();
}

bool EditorProfiler::is_profiling() { return activate->is_pressed(); }

Vector<Vector<String>> EditorProfiler::get_data_as_csv() const
{
	Vector<Vector<String>> res;

	if (frame_metrics.is_empty()) {
		return res;
	}

	// Different metrics may contain different number of categories.
	HashSet<StringName> possible_signatures;
	for (int i = 0; i < frame_metrics.size(); i++) {
		const Metric& m = frame_metrics[i];
		if (!m.valid) {
			continue;
		}
		for (const KeyValue<StringName, Metric::Category*>& E : m.category_ptrs) {
			possible_signatures.insert(E.key);
		}
		for (const KeyValue<StringName, Metric::Category::Item*>& E : m.item_ptrs) {
			possible_signatures.insert(E.key);
		}
	}

	// Generate CSV header and cache indices.
	HashMap<StringName, int> sig_map;
	Vector<String> signatures;
	signatures.resize(possible_signatures.size());
	int sig_index = 0;
	for (const StringName& E : possible_signatures) {
		signatures.write[sig_index] = E;
		sig_map[E] = sig_index;
		sig_index++;
	}
	res.push_back(signatures);

	// values
	Vector<String> values;

	int index = last_metric;

	for (int i = 0; i < frame_metrics.size(); i++) {
		++index;

		if (index >= frame_metrics.size()) {
			index = 0;
		}

		const Metric& m = frame_metrics[index];

		if (!m.valid) {
			continue;
		}

		// Don't keep old values since there may be empty cells.
		values.clear();
		values.resize(possible_signatures.size());

		for (const KeyValue<StringName, Metric::Category*>& E : m.category_ptrs) {
			values.write[sig_map[E.key]] = String::num_real(E.value->total_time);
		}
		for (const KeyValue<StringName, Metric::Category::Item*>& E : m.item_ptrs) {
			values.write[sig_map[E.key]] = String::num_real(E.value->total);
		}

		res.push_back(values);
	}

	return res;
}


