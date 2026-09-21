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

void EditorLog::_clear_request()
{
	log->clear();
	messages.clear();
	_reset_message_counts();
	_set_dock_tab_icon(Ref<Texture2D>());
}

void EditorLog::clear() { _clear_request(); }

void EditorLog::_set_dock_tab_icon(Ref<Texture2D> p_icon)
{
	set_dock_icon(p_icon);
	set_force_show_icon(p_icon.is_valid());
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


