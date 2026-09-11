/**************************************************************************/
/*  run_instances_dialog.cpp                                              */
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
#include "core/os/os.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "run_instances_dialog.h"
#include "scene/gui/check_box.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"
#include "scene/main/timer.h"

void RunInstancesDialog::_start_main_timer() { main_apply_timer->start(); }

void RunInstancesDialog::_start_instance_timer() { instance_apply_timer->start(); }

Vector<String> RunInstancesDialog::_split_cmdline_args(const String& p_arg_string) const
{
	Vector<String> split_args;
	int arg_start = 0;
	bool is_quoted = false;
	char32_t quote_char = '-';
	char32_t arg_char;
	int arg_length;
	for (int i = 0; i < p_arg_string.length(); i++) {
		arg_char = p_arg_string[i];
		if (arg_char == '\"' || arg_char == '\'') {
			if (i == 0 || p_arg_string[i - 1] != '\\') {
				if (is_quoted) {
					if (arg_char == quote_char) {
						is_quoted = false;
						quote_char = '-';
					}
				}
				else {
					is_quoted = true;
					quote_char = arg_char;
				}
			}
		}
		else if (!is_quoted && arg_char == ' ') {
			arg_length = i - arg_start;
			if (arg_length > 0) {
				split_args.push_back(p_arg_string.substr(arg_start, arg_length));
			}
			arg_start = i + 1;
		}
	}
	arg_length = p_arg_string.length() - arg_start;
	if (arg_length > 0) {
		split_args.push_back(p_arg_string.substr(arg_start, arg_length));
	}
	return split_args;
}

void RunInstancesDialog::popup_dialog() { popup_centered_clamped(Size2(1200, 600) * EDSCALE, 0.8); }

int RunInstancesDialog::get_instance_count() const
{
	if (enable_multiple_instances_checkbox->is_pressed()) {
		return instance_count->get_value();
	}
	else {
		return 1;
	}
}

void RunInstancesDialog::get_argument_list_for_instance(int p_idx, List<String>& r_list) const
{
	bool override_args = instances_data[p_idx].overrides_run_args();
	bool use_multiple_instances = enable_multiple_instances_checkbox->is_pressed();
	String raw_custom_args;

	if (use_multiple_instances) {
		if (override_args) {
			raw_custom_args = instances_data[p_idx].get_launch_arguments();
		}
		else {
			raw_custom_args =
				main_args_edit->get_text() + " " + instances_data[p_idx].get_launch_arguments();
		}
	}
	else {
		raw_custom_args = main_args_edit->get_text();
	}

	if (!raw_custom_args.is_empty()) {
		// Allow the user to specify a command to run, similar to Steam's launch options.
		// In this case, Godot will no longer be run directly; it's up to the underlying command
		// to run it. For instance, this can be used on Linux to force a running project
		// to use Optimus using `prime-run` or similar.
		// Example: `prime-run %command% --time-scale 0.5`
		const int placeholder_pos = raw_custom_args.find("%command%");

		Vector<String> custom_args;

		if (placeholder_pos != -1) {
			// Prepend executable-specific custom arguments.
			// If nothing is placed before `%command%`, behave as if no placeholder was specified.
			Vector<String> exec_args =
				_split_cmdline_args(raw_custom_args.substr(0, placeholder_pos));
			if (exec_args.size() > 0) {
				exec_args.remove_at(0);

				// Append the Godot executable name before we append executable arguments
				// (since the order is reversed when using `push_front()`).
				r_list.push_front(OS::get_singleton()->get_executable_path());
			}

			for (int i = exec_args.size() - 1; i >= 0; i--) {
				// Iterate backwards as we're pushing items in the reverse order.
				r_list.push_front(exec_args[i].replace(" ", "%20"));
			}

			// Append Godot-specific custom arguments.
			custom_args = _split_cmdline_args(
				raw_custom_args.substr(placeholder_pos + String("%command%").size()));
			for (int i = 0; i < custom_args.size(); i++) {
				r_list.push_back(custom_args[i].replace(" ", "%20"));
			}
		}
		else {
			// Append Godot-specific custom arguments.
			custom_args = _split_cmdline_args(raw_custom_args);
			for (int i = 0; i < custom_args.size(); i++) {
				r_list.push_back(custom_args[i].replace(" ", "%20"));
			}
		}
	}
}

void RunInstancesDialog::apply_custom_features(int p_instance_idx)
{
	const InstanceData& instance = instances_data[p_instance_idx];

	String raw_text;
	if (enable_multiple_instances_checkbox->is_pressed()) {
		if (instance.overrides_features()) {
			raw_text = instance.get_feature_tags();
		}
		else {
			raw_text = main_features_edit->get_text() + "," + instance.get_feature_tags();
		}
	}
	else {
		raw_text = main_features_edit->get_text();
	}

	const Vector<String> raw_list = raw_text.split(",");
	Vector<String> stripped_features;

	for (int i = 0; i < raw_list.size(); i++) {
		String f = raw_list[i].strip_edges();
		if (!f.is_empty()) {
			stripped_features.push_back(f);
		}
	}
	OS::get_singleton()->set_environment(
		"GODOT_EDITOR_CUSTOM_FEATURES", String(",").join(stripped_features));
}

bool RunInstancesDialog::InstanceData::overrides_run_args() const
{
	return item->is_checked(COLUMN_OVERRIDE_ARGS);
}

String RunInstancesDialog::InstanceData::get_launch_arguments() const
{
	return item->get_text(COLUMN_LAUNCH_ARGUMENTS);
}

bool RunInstancesDialog::InstanceData::overrides_features() const
{
	return item->is_checked(COLUMN_OVERRIDE_FEATURES);
}

String RunInstancesDialog::InstanceData::get_feature_tags() const
{
	return item->get_text(COLUMN_FEATURE_TAGS);
}


