/**************************************************************************/
/*  editor_network_profiler.cpp                                           */
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

#include "core/io/resource_loader.h"
#include "editor/editor_string_names.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_network_profiler.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/split_container.h"
#include "scene/main/timer.h"

void EditorNetworkProfiler::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		// TRANSLATORS: This is the label for the network profiler's incoming bandwidth.
		down_label->set_text(TTR("Down", "Network"));
		// TRANSLATORS: This is the label for the network profiler's outgoing bandwidth.
		up_label->set_text(TTR("Up", "Network"));

		set_bandwidth(incoming_bandwidth, outgoing_bandwidth);

		if (is_ready()) {
			refresh_rpc_data();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		if (activate->is_pressed()) {
			activate->set_button_icon(theme_cache.stop_icon);
		}
		else {
			activate->set_button_icon(theme_cache.play_icon);
		}
		clear_button->set_button_icon(theme_cache.clear_icon);

		incoming_bandwidth_text->set_right_icon(theme_cache.incoming_bandwidth_icon);
		outgoing_bandwidth_text->set_right_icon(theme_cache.outgoing_bandwidth_icon);

		// This needs to be done here to set the faded color when the profiler is first opened
		incoming_bandwidth_text->add_theme_color_override(
			"font_uneditable_color", theme_cache.incoming_bandwidth_color * Color(1, 1, 1, 0.5));
		outgoing_bandwidth_text->add_theme_color_override(
			"font_uneditable_color", theme_cache.outgoing_bandwidth_color * Color(1, 1, 1, 0.5));
	} break;
	}
}

void EditorNetworkProfiler::_update_theme_item_cache()
{
	VBoxContainer::_update_theme_item_cache();

	theme_cache.node_icon = get_theme_icon(SNAME("Node"), EditorStringName(EditorIcons));
	theme_cache.stop_icon = get_theme_icon(SNAME("Stop"), EditorStringName(EditorIcons));
	theme_cache.play_icon = get_theme_icon(SNAME("Play"), EditorStringName(EditorIcons));
	theme_cache.clear_icon = get_theme_icon(SNAME("Clear"), EditorStringName(EditorIcons));

	theme_cache.multiplayer_synchronizer_icon =
		get_theme_icon("MultiplayerSynchronizer", EditorStringName(EditorIcons));
	theme_cache.instance_options_icon =
		get_theme_icon(SNAME("InstanceOptions"), EditorStringName(EditorIcons));

	theme_cache.incoming_bandwidth_icon =
		get_theme_icon(SNAME("ArrowDown"), EditorStringName(EditorIcons));
	theme_cache.outgoing_bandwidth_icon =
		get_theme_icon(SNAME("ArrowUp"), EditorStringName(EditorIcons));

	theme_cache.incoming_bandwidth_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	theme_cache.outgoing_bandwidth_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
}

void EditorNetworkProfiler::_refresh()
{
	if (!dirty) {
		return;
	}
	dirty = false;
	refresh_rpc_data();
	refresh_replication_data();
}

void EditorNetworkProfiler::_activate_pressed()
{
	_update_button_text();

	if (activate->is_pressed()) {
		refresh_timer->start();
	}
	else {
		refresh_timer->stop();
	}
}

void EditorNetworkProfiler::_update_button_text()
{
	if (activate->is_pressed()) {
		activate->set_button_icon(theme_cache.stop_icon);
		activate->set_text(TTRC("Stop"));
	}
	else {
		activate->set_button_icon(theme_cache.play_icon);
		activate->set_text(TTRC("Start"));
	}
}

void EditorNetworkProfiler::stopped()
{
	activate->set_disabled(true);
	set_profiling(false);
	refresh_timer->stop();
}

void EditorNetworkProfiler::set_bandwidth(int p_incoming, int p_outgoing)
{
	incoming_bandwidth = p_incoming;
	outgoing_bandwidth = p_outgoing;

	incoming_bandwidth_text->set_text(vformat(TTR("%s/s"), String::humanize_size(p_incoming)));
	outgoing_bandwidth_text->set_text(vformat(TTR("%s/s"), String::humanize_size(p_outgoing)));

	// Make labels more prominent when the bandwidth is greater than 0 to attract user attention
	incoming_bandwidth_text->add_theme_color_override("font_uneditable_color",
		theme_cache.incoming_bandwidth_color * Color(1, 1, 1, p_incoming > 0 ? 1 : 0.5));
	outgoing_bandwidth_text->add_theme_color_override("font_uneditable_color",
		theme_cache.outgoing_bandwidth_color * Color(1, 1, 1, p_outgoing > 0 ? 1 : 0.5));
}

bool EditorNetworkProfiler::is_profiling() { return activate->is_pressed(); }


