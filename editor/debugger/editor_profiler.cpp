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

int EditorProfiler::_get_zoom_left_border() const
{
	const int max_profiles_shown = frame_metrics.size() / Math::exp(graph_zoom);
	return CLAMP(
		zoom_center - max_profiles_shown / 2, 0, frame_metrics.size() - max_profiles_shown);
}

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


