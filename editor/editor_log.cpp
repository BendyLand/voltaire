/**************************************************************************/
/*  editor_log.cpp                                                        */
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
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_log.h"
#include "scene/gui/box_container.h"
#include "scene/gui/flow_container.h"
#include "scene/main/timer.h"
#include "scene/resources/font.h"
#include "servers/display/display_server.h"

void EditorLog::_update_theme()
{
	const Ref<Font> normal_font =
		get_theme_font(SNAME("output_source"), EditorStringName(EditorFonts));
	if (normal_font.is_valid()) {
		log->add_theme_font_override("normal_font", normal_font.ptr());
	}

	const Ref<Font> bold_font =
		get_theme_font(SNAME("output_source_bold"), EditorStringName(EditorFonts));
	if (bold_font.is_valid()) {
		log->add_theme_font_override("bold_font", bold_font.ptr());
	}

	const Ref<Font> italics_font =
		get_theme_font(SNAME("output_source_italic"), EditorStringName(EditorFonts));
	if (italics_font.is_valid()) {
		log->add_theme_font_override("italics_font", italics_font.ptr());
	}

	const Ref<Font> bold_italics_font =
		get_theme_font(SNAME("output_source_bold_italic"), EditorStringName(EditorFonts));
	if (bold_italics_font.is_valid()) {
		log->add_theme_font_override("bold_italics_font", bold_italics_font.ptr());
	}

	const Ref<Font> mono_font =
		get_theme_font(SNAME("output_source_mono"), EditorStringName(EditorFonts));
	if (mono_font.is_valid()) {
		log->add_theme_font_override("mono_font", mono_font.ptr());
	}

	// Disable padding for highlighted background/foreground to prevent highlights from overlapping
	// on close lines. This also better matches terminal output, which does not use any form of
	// padding.
	log->add_theme_constant_override("text_highlight_h_padding", 0);
	log->add_theme_constant_override("text_highlight_v_padding", 0);

	const int font_size =
		get_theme_font_size(SNAME("output_source_size"), EditorStringName(EditorFonts));
	log->begin_bulk_theme_override();
	log->add_theme_font_size_override("normal_font_size", font_size);
	log->add_theme_font_size_override("bold_font_size", font_size);
	log->add_theme_font_size_override("italics_font_size", font_size);
	log->add_theme_font_size_override("mono_font_size", font_size);
	log->end_bulk_theme_override();

	const String wide_text = "MM";

	Button* button = type_filter_map[MSG_TYPE_STD]->toggle_button;
	button->set_button_icon(get_editor_theme_icon(SNAME("Popup")));
	button->set_custom_minimum_size(
		Vector2(button->get_minimum_size_for_text_and_icon(wide_text, button->get_button_icon()).x *
					EDSCALE,
			0));
	button = type_filter_map[MSG_TYPE_ERROR]->toggle_button;
	button->set_button_icon(get_editor_theme_icon(SNAME("StatusError")));
	button->set_custom_minimum_size(
		Vector2(button->get_minimum_size_for_text_and_icon(wide_text, button->get_button_icon()).x *
					EDSCALE,
			0));
	button = type_filter_map[MSG_TYPE_WARNING]->toggle_button;
	button->set_button_icon(get_editor_theme_icon(SNAME("StatusWarning")));
	button->set_custom_minimum_size(
		Vector2(button->get_minimum_size_for_text_and_icon(wide_text, button->get_button_icon()).x *
					EDSCALE,
			0));
	button = type_filter_map[MSG_TYPE_EDITOR]->toggle_button;
	button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
	button->set_custom_minimum_size(
		Vector2(button->get_minimum_size_for_text_and_icon(wide_text, button->get_button_icon()).x *
					EDSCALE,
			0));

	clear_button->set_button_icon(get_editor_theme_icon(SNAME("Clear")));
	collapse_button->set_button_icon(get_editor_theme_icon(SNAME("CombineLines")));
	search_box->set_right_icon(get_editor_theme_icon(SNAME("Search")));

	theme_cache.error_color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	theme_cache.error_icon = get_editor_theme_icon(SNAME("Error"));
	theme_cache.warning_color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
	theme_cache.warning_icon = get_editor_theme_icon(SNAME("Warning"));
	theme_cache.message_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor)) *
		Color(1, 1, 1, 0.6);
}

void EditorLog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_update_theme();
		_load_state();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (!save_state_timer->is_stopped()) {
			_save_state();
			save_state_timer->stop();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		_update_theme();
		_rebuild_log();
	} break;
	}
}

void EditorLog::_set_collapse(bool p_collapse)
{
	collapse = p_collapse;
	_start_state_save_timer();
	_rebuild_log();
}

void EditorLog::_start_state_save_timer()
{
	if (!is_loading_state) {
		save_state_timer->start();
	}
}

void EditorLog::_meta_clicked(const String& p_meta)
{
	if (!p_meta.contains_char(':')) {
		return;
	}
	const PackedStringArray parts = p_meta.rsplit(":", true, 1);
	String path = parts[0];
	const int line = parts[1].to_int() - 1;

	if (path.begins_with("res://")) {
		if (ResourceLoader::exists(path)) {
			const Ref<Resource> res = ResourceLoader::load(path);
			ScriptEditor::get_singleton()->edit(res, line, 0);
			InspectorDock::get_singleton()->edit_resource(res);
		}
	}
	else if (path.has_extension("cpp") || path.has_extension("h") || path.has_extension("mm") ||
			   path.has_extension("hpp")) {
		// Godot source file. Try to open it in external editor.
		if (path.begins_with("./") || path.begins_with(".\\")) {
			// Relative path. Convert to absolute, using executable path as reference.
			path = path.trim_prefix("./").trim_prefix(".\\");
			const String absolute_path =
				OS::get_singleton()->get_executable_path().get_base_dir().get_base_dir().path_join(
					path);
			if (FileAccess::exists(absolute_path)) {
				path = absolute_path;
			}
		}

		if (!FileAccess::exists(path)) {
			// The file does not exist. Try on GitHub instead.
			String branch = "master";
			if (str_compare(VLTR_VERSION_BUILD, "official") == 0) {
				// In official builds it's safe to use specific commit hash, so the line number is
				// more accurate.
				branch = VLTR_VERSION_HASH;
			}
			OS::get_singleton()->shell_open(vformat(
				"https://github.com/godotengine/godot/blob/%s/%s#L%d", branch, path, line + 1));
			return;
		}

		if (!ScriptEditorPlugin::open_in_external_editor(path, line, -1, true)) {
			OS::get_singleton()->shell_open(path);
		}
	}
	else {
		OS::get_singleton()->shell_open(p_meta);
	}
}

void EditorLog::_clear_request()
{
	log->clear();
	messages.clear();
	_reset_message_counts();
	_set_dock_tab_icon(Ref<Texture2D>());
}

void EditorLog::clear() { _clear_request(); }

void EditorLog::_process_message(const String& p_msg, MessageType p_type, bool p_clear)
{
	if (messages.size() > 0 && messages[messages.size() - 1].text == p_msg &&
		messages[messages.size() - 1].type == p_type) {
		// If previous message is the same as the new one, increase previous count rather than
		// adding another instance to the messages list.
		LogMessage& previous = messages.write[messages.size() - 1];
		previous.count++;

		_add_log_line(previous, collapse);
	}
	else {
		// Different message to the previous one received.
		LogMessage message(p_msg, p_type, p_clear);
		_add_log_line(message);
		messages.push_back(message);
	}

	type_filter_map[p_type]->set_message_count(type_filter_map[p_type]->get_message_count() + 1);
}

void EditorLog::add_message(const String& p_msg, MessageType p_type)
{
	// Make text split by new lines their own message.
	// See #41321 for reasoning. At time of writing, multiple print()'s in running projects
	// get grouped together and sent to the editor log as one message. This can mess with the
	// search functionality (see the comments on the PR above for more details). This behavior
	// also matches that of other IDE's.
	Vector<String> lines = p_msg.split("\n", true);
	int line_count = lines.size();

	for (int i = 0; i < line_count; i++) {
		_process_message(lines[i], p_type, i == line_count - 1);
	}
}

void EditorLog::_set_dock_tab_icon(Ref<Texture2D> p_icon)
{
	set_dock_icon(p_icon);
	set_force_show_icon(p_icon.is_valid());
}

void EditorLog::_undo_redo_cbk(void* p_self, const String& p_name)
{
	EditorLog* self = static_cast<EditorLog*>(p_self);
	self->add_message(p_name, EditorLog::MSG_TYPE_EDITOR);
}

void EditorLog::_rebuild_log()
{
	if (messages.is_empty()) {
		return;
	}

	log->clear();

	int line_count = 0;
	int start_message_index = 0;
	int initial_skip = 0;

	// Search backward for starting place.
	for (start_message_index = messages.size() - 1; start_message_index >= 0;
		 start_message_index--) {
		LogMessage msg = messages[start_message_index];
		if (collapse) {
			if (_check_display_message(msg)) {
				line_count++;
			}
		}
		else {
			// If not collapsing, log each instance on a line.
			for (int i = 0; i < msg.count; i++) {
				if (_check_display_message(msg)) {
					line_count++;
				}
			}
		}
		if (line_count >= line_limit) {
			initial_skip = line_count - line_limit;
			break;
		}
		if (start_message_index == 0) {
			break;
		}
	}

	for (int msg_idx = start_message_index; msg_idx < messages.size(); msg_idx++) {
		LogMessage msg = messages[msg_idx];

		if (collapse) {
			// If collapsing, only log one instance of the message.
			_add_log_line(msg);
		}
		else {
			// If not collapsing, log each instance on a line.
			for (int i = initial_skip; i < msg.count; i++) {
				initial_skip = 0;
				_add_log_line(msg);
			}
		}
	}
}

bool EditorLog::_check_display_message(LogMessage& p_message)
{
	bool filter_active = type_filter_map[p_message.type]->is_active();
	String search_text = search_box->get_text();

	if (search_text.is_empty()) {
		return filter_active;
	}

	bool search_match = p_message.text.containsn(search_text);

	// If not found and message contains BBCode tags, also check the parsed text
	if (!search_match && p_message.text.contains_char('[')) {
		// Lazy initialize the BBCode parser
		if (!bbcode_parser) {
			bbcode_parser = memnew(RichTextLabel);
			bbcode_parser->set_use_bbcode(true);
		}

		// Ensure clean state for each message
		bbcode_parser->clear();
		bbcode_parser->parse_bbcode(p_message.text);
		String parsed_text = bbcode_parser->get_parsed_text();
		search_match = parsed_text.containsn(search_text);
	}

	return filter_active && search_match;
}

void EditorLog::_set_filter_active(bool p_active, MessageType p_message_type)
{
	type_filter_map[p_message_type]->set_active(p_active);
	_start_state_save_timer();
	_rebuild_log();
}

void EditorLog::_search_changed(const String& p_text) { _rebuild_log(); }

void EditorLog::_reset_message_counts()
{
	for (const KeyValue<MessageType, LogFilter*>& E : type_filter_map) {
		E.value->set_message_count(0);
	}
}

void EditorLog::deinit() { remove_error_handler(&eh); }

EditorLog::~EditorLog()
{
	memdelete(bbcode_parser);

	for (const KeyValue<MessageType, LogFilter*>& E : type_filter_map) {
		// MSG_TYPE_STD_RICH is connected to the std_filter button, so we do this
		// to avoid it from being deleted twice, causing a crash on closing.
		if (E.key != MSG_TYPE_STD_RICH) {
			memdelete(E.value);
		}
	}
}


