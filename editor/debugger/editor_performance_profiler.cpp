/**************************************************************************/
/*  editor_performance_profiler.cpp                                       */
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

#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_performance_profiler.h"
#include "main/performance.h"

EditorPerformanceProfiler::Monitor::Monitor(const String& p_name, const String& p_base,
	int p_frame_index, Performance::MonitorType p_type, TreeItem* p_item)
{
	type = p_type;
	item = p_item;
	frame_index = p_frame_index;
	name = p_name;
	base = p_base;
}

void EditorPerformanceProfiler::Monitor::reset()
{
	history.clear();
	max = 0.0f;
	if (item) {
		item->set_text(1, "");
		item->set_tooltip_text(1, "");
	}
}

String EditorPerformanceProfiler::_format_label(
	float p_value, Performance::MonitorType p_type) const
{
	const TranslationServer* ts = TranslationServer::get_singleton();

	switch (p_type) {
	case Performance::MONITOR_TYPE_MEMORY: {
		return String::humanize_size(p_value);
	}
	}
}

void EditorPerformanceProfiler::_update_monitor_value(Monitor* p_monitor, float p_value)
{
	TreeItem* item = p_monitor->item;
	ERR_FAIL_NULL(item);

	const String label = EditorPerformanceProfiler::_format_label(p_value, p_monitor->type);
	item->set_text(1, label);

	switch (p_monitor->type) {
	case Performance::MONITOR_TYPE_MEMORY:
	case Performance::MONITOR_TYPE_TIME: {
		item->set_tooltip_text(1, label);
	} break;

	default: {
		item->set_tooltip_text(1, label + " " + item->get_text(0));
	} break;
	}

	if (p_value > p_monitor->max) {
		p_monitor->max = p_value;
	}
}

void EditorPerformanceProfiler::_build_monitor_tree()
{
	HashSet<StringName> monitor_checked;
	for (KeyValue<StringName, Monitor>& E : monitors) {
		if (E.value.item && E.value.item->is_checked(0)) {
			monitor_checked.insert(E.key);
		}
	}

	base_map.clear();
	monitor_tree->get_root()->clear_children();

	for (KeyValue<StringName, Monitor>& E : monitors) {
		TreeItem* base = _get_monitor_base(E.value.base);
		TreeItem* item = _create_monitor_item(E.value.name, base);
		item->set_checked(0, monitor_checked.has(E.key));
		E.value.item = item;
		if (!E.value.history.is_empty()) {
			_update_monitor_value(&E.value, E.value.history.front()->get());
		}
	}
}

TreeItem* EditorPerformanceProfiler::_get_monitor_base(const StringName& p_base_name)
{
	if (base_map.has(p_base_name)) {
		return base_map[p_base_name];
	}

	TreeItem* base = monitor_tree->create_item(monitor_tree->get_root());
	base->set_text(0, EditorPropertyNameProcessor::get_singleton()->process_name(
						  p_base_name, EditorPropertyNameProcessor::get_settings_style()));
	base->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
	base->set_editable(0, false);
	base->set_selectable(0, false);
	base->set_expand_right(0, true);
	if (is_inside_tree()) {
		base->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	}
	base_map.insert(p_base_name, base);
	return base;
}

TreeItem* EditorPerformanceProfiler::_create_monitor_item(
	const StringName& p_monitor_name, TreeItem* p_base)
{
	TreeItem* item = monitor_tree->create_item(p_base);
	item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	item->set_editable(0, true);
	item->set_selectable(0, false);
	item->set_selectable(1, false);
	item->set_text(0, EditorPropertyNameProcessor::get_singleton()->process_name(
						  p_monitor_name, EditorPropertyNameProcessor::get_settings_style()));
	return item;
}

void EditorPerformanceProfiler::update_monitors(
	const Vector<StringName>& p_names, const PackedInt32Array& p_types)
{
	HashMap<StringName, int> names;
	for (int i = 0; i < p_names.size(); i++) {
		names.insert("custom:" + p_names[i], Performance::MONITOR_MAX + i);
	}

	{
		HashMap<StringName, Monitor>::Iterator E = monitors.begin();
		while (E != monitors.end()) {
			HashMap<StringName, Monitor>::Iterator N = E;
			++N;
			if (String(E->key).begins_with("custom:")) {
				if (!names.has(E->key)) {
					monitors.remove(E);
				}
				else {
					E->value.frame_index = names[E->key];
					names.erase(E->key);
				}
			}
			E = N;
		}
	}

	int index = 0;
	for (const KeyValue<StringName, int>& E : names) {
		String name = String(E.key).replace_first("custom:", "");
		String base = "Custom";
		if (name.get_slice_count("/") == 2) {
			base = name.get_slicec('/', 0);
			name = name.get_slicec('/', 1);
		}
		Performance::MonitorType type = Performance::MonitorType(p_types[index]);
		monitors.insert(E.key, Monitor(name, base, E.value, type, nullptr));
		index++;
	}

	_build_monitor_tree();
}

List<float>* EditorPerformanceProfiler::get_monitor_data(const StringName& p_name)
{
	if (monitors.has(p_name)) {
		return &monitors[p_name].history;
	}
	return nullptr;
}

EditorPerformanceProfiler::EditorPerformanceProfiler()
{
	set_name(TTRC("Monitors"));
	set_split_offset(340 * EDSCALE);

	monitor_tree = memnew(Tree);
	monitor_tree->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
	monitor_tree->set_columns(2);

	monitor_tree->set_column_title(0, TTRC("Monitor"));
	monitor_tree->set_column_expand(0, true);
	monitor_tree->set_column_title(1, TTRC("Value"));
	monitor_tree->set_column_custom_minimum_width(1, 100 * EDSCALE);
	monitor_tree->set_column_expand(1, false);
	monitor_tree->set_column_titles_visible(true);
	monitor_tree->create_item();
	monitor_tree->set_hide_root(true);
	monitor_tree->set_theme_type_variation("TreeSecondary");
	add_child(monitor_tree);

	monitor_draw = memnew(Control);
	monitor_draw->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
	monitor_draw->set_clip_contents(true);
	add_child(monitor_draw);

	info_message = memnew(Label);
	info_message->set_focus_mode(FOCUS_ACCESSIBILITY);
	info_message->set_text(TTRC("Pick one or more items from the list to display the graph."));
	info_message->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	info_message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	info_message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	info_message->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
	info_message->set_anchors_and_offsets_preset(
		PRESET_FULL_RECT, PRESET_MODE_KEEP_SIZE, 8 * EDSCALE);
	monitor_draw->add_child(info_message);

	for (int i = 0; i < Performance::MONITOR_MAX; i++) {
		const Performance::Monitor monitor = Performance::Monitor(i);
		const String path = Performance::get_singleton()->get_monitor_name(monitor);
		const String base = path.get_slicec('/', 0);
		const String name = path.get_slicec('/', 1);
		monitors.insert(
			path, Monitor(name, base, i, Performance::get_singleton()->get_monitor_type(monitor),
					  nullptr));
	}

	_build_monitor_tree();
}


