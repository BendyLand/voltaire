/**************************************************************************/
/*  editor_log.h                                                          */
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

#pragma once

#include "core/os/thread.h"
#include "editor/docks/editor_dock.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/rich_text_label.h"

class Timer;
class UndoRedo;

class EditorLog : public EditorDock
{
public:
	enum MessageType
	{
		MSG_TYPE_STD,
		MSG_TYPE_ERROR,
		MSG_TYPE_STD_RICH,
		MSG_TYPE_WARNING,
		MSG_TYPE_EDITOR,
	};

private:
	struct LogMessage
	{
		String text;
		MessageType type;
		int count = 1;
		bool clear = true;

		LogMessage() = default;

		LogMessage(const String& p_text, MessageType p_type, bool p_clear)
			: text(p_text), type(p_type), clear(p_clear)
		{
		}
	};

	struct
	{
		Color error_color;
		Ref<Texture2D> error_icon;

		Color warning_color;
		Ref<Texture2D> warning_icon;

		Color message_color;
	} theme_cache;

	// Encapsulates all data and functionality regarding filters.
	struct LogFilter
	{
	private:
		// Force usage of set method since it has functionality built-in.
		int message_count = 0;
		bool active = true;

	public:
		MessageType type;
		Button* toggle_button = nullptr;

		int get_message_count() { return message_count; }

		void set_message_count(int p_count)
		{
			message_count = p_count;
			toggle_button->set_text(itos(message_count));
		}

		bool is_active() { return active; }

		LogFilter(MessageType p_type) : type(p_type) {}
	};

	int line_limit = 10000;

	Vector<LogMessage> messages;
	// Maps MessageTypes to LogFilters for convenient access and storage (don't need 1 member per
	// filter).
	HashMap<MessageType, LogFilter*> type_filter_map;

	RichTextLabel* log = nullptr;

	Button* clear_button = nullptr;

	Button* collapse_button = nullptr;
	bool collapse = false;

	LineEdit* search_box = nullptr;

	// Reusable RichTextLabel for BBCode parsing during search
	RichTextLabel* bbcode_parser = nullptr;

	bool is_loading_state =
		false; // Used to disable saving requests while loading (some signals from buttons will try
			   // to trigger a save, which happens during loading).
	Timer* save_state_timer = nullptr;

	static void _error_handler(void* p_self, const char* p_func, const char* p_file, int p_line,
		const char* p_error, const char* p_errorexp, bool p_editor_notify, ErrorHandlerType p_type);

	ErrorHandlerList eh;

	void _clear_request();

	bool _check_display_message(LogMessage& p_message);

	void _reset_message_counts();
	void _set_dock_tab_icon(Ref<Texture2D> p_icon);

public:
	void deinit();

	void clear();

	EditorLog() = default;
	~EditorLog();
};


