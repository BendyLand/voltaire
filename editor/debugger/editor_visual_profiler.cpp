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

void EditorVisualProfiler::_autostart_toggled(bool p_toggled_on)
{
	EditorRunBar::get_singleton()->update_profiler_autostart_indicator();
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



void EditorVisualProfiler::_update_plot() {}
