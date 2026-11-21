/**************************************************************************/
/*  embedded_debugger.mm                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "embedded_debugger.h"

#include "display_server_macos_embedded.h"

#include "core/debugger/engine_debugger.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/input/input.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"

void EmbeddedDebugger::initialize(DisplayServerMacOSEmbedded* p_ds)
{
#ifdef DEBUG_ENABLED
	if (EngineDebugger::is_active() && !data) {
		data = memnew(Data);
		data->ds = p_ds;
		_init_parse_message_handlers();
		EngineDebugger::register_message_capture(
			"embed", EngineDebugger::Capture(nullptr, EmbeddedDebugger::parse_message));
	}
#endif
}

void EmbeddedDebugger::deinitialize()
{
#ifdef DEBUG_ENABLED
	if (data) {
		memdelete(data);
		data = nullptr;
	}
#endif
}

#ifdef DEBUG_ENABLED
void EmbeddedDebugger::_init_parse_message_handlers()
{
	data->parse_message_handlers["window_size"] = &EmbeddedDebugger::_msg_window_size;
	data->parse_message_handlers["mouse_set_mode"] = &EmbeddedDebugger::_msg_mouse_set_mode;
	data->parse_message_handlers["event"] = &EmbeddedDebugger::_msg_event;
	data->parse_message_handlers["win_event"] = &EmbeddedDebugger::_msg_win_event;
	data->parse_message_handlers["notification"] = &EmbeddedDebugger::_msg_notification;
	data->parse_message_handlers["ime_update"] = &EmbeddedDebugger::_msg_ime_update;
	data->parse_message_handlers["ds_state"] = &EmbeddedDebugger::_msg_ds_state;
}

Error EmbeddedDebugger::_msg_window_size(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'window_size' message.");
	Size2i size = p_args[0];
	data->ds->_window_set_size(size);
	return OK;
}

Error EmbeddedDebugger::_msg_mouse_set_mode(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'mouse_set_mode' message.");
	DisplayServerEnums::MouseMode mode = p_args[0];
	data->ds->mouse_set_mode(mode);
	return OK;
}

Error EmbeddedDebugger::_msg_event(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'event' message.");
	if (!Input::is_initialized()) {
		// Ignore if we've received an event before the process has initialized.
		return OK;
	}

	PackedByteArray buf = p_args[0];
	Ref<InputEvent> event;
	decode_input_event(buf, event);

	// Events arrive in points (normalized by the editor). Scale to the
	// game's pixel coordinate space.
	float scale = data->ds->screen_get_max_scale();

	{
		Ref<InputEventMouse> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			e->set_global_position(e->get_global_position() * scale);
			Ref<InputEventMouseMotion> mm = e;
			if (mm.is_valid()) {
				mm->set_relative(mm->get_relative() * scale);
			}
			Input::set_mouse_position(e->get_position());
		}
	}

	{
		Ref<InputEventMagnifyGesture> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			Input::set_mouse_position(e->get_position());
		}
	}

	{
		Ref<InputEventPanGesture> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			Input::set_mouse_position(e->get_position());
		}
	}

	if (event.is_valid()) {
		Input::parse_input_event(event.ptr());
	}

	return OK;
}

Error EmbeddedDebugger::_msg_win_event(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'win_event' message.");
	DisplayServerEnums::WindowEvent win_event = p_args[0];
	data->ds->send_window_event_by_id(win_event, DisplayServerEnums::MAIN_WINDOW_ID);
	if (win_event == DisplayServerEnums::WindowEvent::WINDOW_EVENT_MOUSE_EXIT) {
		Input::release_pressed_events();
	}
	return OK;
}

Error EmbeddedDebugger::_msg_ime_update(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 2, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'ime_update' message.");
	String ime_text = p_args[0];
	Vector2i ime_selection = p_args[1];
	data->ds->update_im_text(ime_selection, ime_text);
	return OK;
}

Error EmbeddedDebugger::_msg_notification(const Array& p_args)
{
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'notification' message.");
	int notification = p_args[0];
	if (MainLoop::is_active()) {
		MainLoop::notification(notification);
	}
	return OK;
}

Error EmbeddedDebugger::_msg_ds_state(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'ds_state' message.");
	PackedByteArray buf = p_args[0];
	DisplayServerMacOSEmbeddedState state;
	state.deserialize(buf);
	data->ds->set_state(state);
	return OK;
}

Error EmbeddedDebugger::parse_message(
	void* p_user, const String& p_msg, const Array& p_args, bool& r_captured)
{
	r_captured = true;
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);

	ParseMessageFunc* fn_ptr = data->parse_message_handlers.getptr(p_msg);
	if (fn_ptr) {
		return (*fn_ptr)(p_args);
	}
	else {
		// Any other messages with this prefix should be ignored.
		WARN_PRINT("Unknown message: " + p_msg);
		return ERR_SKIP;
	}
}
#endif
