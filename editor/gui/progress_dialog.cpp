/**************************************************************************/
/*  progress_dialog.cpp                                                   */
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

#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "main/main.h"
#include "progress_dialog.h"
#include "scene/gui/panel_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"

void BackgroundProgress::_add_task(const String& p_task, const String& p_label, int p_steps)
{
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_MSG(tasks.has(p_task), "Task '" + p_task + "' already exists.");
	BackgroundProgress::Task t;
	t.hb = memnew(HBoxContainer);
	Label* l = memnew(Label);
	l->set_text(p_label + " ");
	t.hb->add_child(l);
	t.progress = memnew(ProgressBar);
	t.progress->set_theme_type_variation("PopupProgressBar");
	t.progress->set_max(p_steps);
	t.progress->set_value(p_steps);
	Control* ec = memnew(Control);
	ec->set_h_size_flags(SIZE_EXPAND_FILL);
	ec->set_v_size_flags(SIZE_EXPAND_FILL);
	t.progress->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	ec->add_child(t.progress);
	ec->set_custom_minimum_size(Size2(80, 5) * EDSCALE);
	t.hb->add_child(ec);

	add_child(t.hb);

	tasks[p_task] = t;
}

void BackgroundProgress::_update()
{
	_THREAD_SAFE_METHOD_

	for (const KeyValue<String, int>& E : updates) {
		if (tasks.has(E.key)) {
			_task_step(E.key, E.value);
		}
	}

	updates.clear();
}

void BackgroundProgress::_task_step(const String& p_task, int p_step)
{
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!tasks.has(p_task));

	Task& t = tasks[p_task];
	if (p_step < 0) {
		t.progress->set_value(t.progress->get_value() + 1);
	}
	else {
		t.progress->set_value(p_step);
	}
}

void BackgroundProgress::_end_task(const String& p_task)
{
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!tasks.has(p_task));
	Task& t = tasks[p_task];

	memdelete(t.hb);
	tasks.erase(p_task);
}

////////////////////////////////////////////////

ProgressDialog* ProgressDialog::singleton = nullptr;

void ProgressDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		Ref<StyleBox> style = main->get_theme_stylebox(SceneStringName(panel), SNAME("PopupMenu"));
		main_border_size = style->get_minimum_size();
		main->set_offset(SIDE_LEFT, style->get_margin(SIDE_LEFT));
		main->set_offset(SIDE_RIGHT, -style->get_margin(SIDE_RIGHT));
		main->set_offset(SIDE_TOP, style->get_margin(SIDE_TOP));
		main->set_offset(SIDE_BOTTOM, -style->get_margin(SIDE_BOTTOM));

		center_panel->add_theme_style_override(
			SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), "PopupPanel").ptr());
	} break;
	}
}

void ProgressDialog::_update_ui()
{
	// Run main loop for two frames.
	if (is_inside_tree()) {
		DisplayServer::get_singleton()->process_events();
		Main::iteration();
	}
}

void ProgressDialog::_reparent_and_show()
{
	Window* current_window = SceneTree::get_singleton()->get_root()->get_last_exclusive_window();
	ERR_FAIL_NULL(current_window);
	reparent(current_window);

	// Ensures that events are properly released before the dialog blocks input.
	bool window_is_input_disabled = current_window->is_input_disabled();
	current_window->set_disable_input(!window_is_input_disabled);
	current_window->set_disable_input(window_is_input_disabled);

	show();
}

bool ProgressDialog::task_step(
	const String& p_task, const String& p_state, int p_step, bool p_force_redraw)
{
	ERR_FAIL_COND_V(!tasks.has(p_task), canceled);

	Task& t = tasks[p_task];
	if (!p_force_redraw) {
		uint64_t tus = OS::get_singleton()->get_ticks_usec();
		if (tus - t.last_progress_tick < 200000) { // 200ms
			return canceled;
		}
	}
	if (p_step < 0) {
		t.progress->set_value(t.progress->get_value() + 1);
	}
	else {
		t.progress->set_value(p_step);
	}

	t.state->set_text(p_state);
	t.last_progress_tick = OS::get_singleton()->get_ticks_usec();
	_update_ui();

	return canceled;
}

void ProgressDialog::_cancel_pressed() { canceled = true; }

ProgressDialog::~ProgressDialog() { singleton = nullptr; }


