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

List<float>* EditorPerformanceProfiler::get_monitor_data(const StringName& p_name)
{
	if (monitors.has(p_name)) {
		return &monitors[p_name].history;
	}
	return nullptr;
}


