/**************************************************************************/
/*  debug_adapter_protocol.cpp                                            */
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

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "debug_adapter_protocol.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"

void DebugAdapterProtocol::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void DebugAdapterProtocol::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

Error DAPeer::handle_data()
{
	int read = 0;
	// Read headers
	if (!has_header) {
		if (!connection->get_available_bytes()) {
			return OK;
		}
		while (true) {
			if (req_pos >= DAP_MAX_BUFFER_SIZE) {
				req_pos = 0;
				ERR_FAIL_V_MSG(ERR_OUT_OF_MEMORY, "Response header too big");
			}
			Error err = connection->get_partial_data(&req_buf[req_pos], 1, read);
			if (err != OK) {
				return FAILED;
			}
			else if (read != 1) { // Busy, wait until next poll
				return ERR_BUSY;
			}
			char* r = (char*)req_buf;
			int l = req_pos;

			// End of headers
			if (l > 3 && r[l] == '\n' && r[l - 1] == '\r' && r[l - 2] == '\n' && r[l - 3] == '\r') {
				r[l - 3] = '\0'; // Null terminate to read string
				String header = String::utf8(r);
				content_length = header.substr(16).to_int();
				has_header = true;
				req_pos = 0;
				break;
			}
			req_pos++;
		}
	}
	if (has_header) {
		while (req_pos < content_length) {
			if (content_length >= DAP_MAX_BUFFER_SIZE) {
				req_pos = 0;
				has_header = false;
				ERR_FAIL_COND_V_MSG(
					req_pos >= DAP_MAX_BUFFER_SIZE, ERR_OUT_OF_MEMORY, "Response content too big");
			}
			Error err =
				connection->get_partial_data(&req_buf[req_pos], content_length - req_pos, read);
			if (err != OK) {
				return FAILED;
			}
			else if (read < content_length - req_pos) {
				return ERR_BUSY;
			}
			req_pos += read;
		}

		// Parse data
		String msg = String::utf8((const char*)req_buf, req_pos);

		// Apply a timestamp if it there's none yet
		if (!timestamp) {
			timestamp = OS::get_ticks_msec();
		}

		// Response
		if (DebugAdapterProtocol::process_message(msg)) {
			// Reset to read again
			req_pos = 0;
			has_header = false;
			timestamp = 0;
		}
	}
	return OK;
}

void DebugAdapterProtocol::reset_current_info()
{
	ERR_FAIL_NULL(data);
	data->_current_request = "";
	data->_current_peer.unref();
}

void DebugAdapterProtocol::reset_ids()
{
	ERR_FAIL_NULL(data);
	data->breakpoint_id = 0;
	data->breakpoint_list.clear();
	data->breakpoint_source_list.clear();

	reset_stack_info();
}

void DebugAdapterProtocol::reset_stack_info()
{
	ERR_FAIL_NULL(data);
	data->stackframe_id = 0;
	data->variable_id = 1;

	data->stackframe_list.clear();
	data->scope_list.clear();
}

bool DebugAdapterProtocol::request_remote_evaluate(const String& p_eval, int p_stack_frame)
{
	ERR_FAIL_NULL_V(data, false);
	// If the eval is already on the pending list, we don't need to request it again
	if (data->eval_pending_list.has(p_eval)) {
		return false;
	}

	EditorDebuggerNode::get_default_debugger()->request_remote_evaluate(
		p_eval, p_stack_frame);
	data->eval_pending_list.insert(p_eval);

	return true;
}

const DAP::Source& DebugAdapterProtocol::fetch_source(const String& p_path)
{
	static DAP::Source empty_source;
	ERR_FAIL_NULL_V(data, empty_source);

	const String& global_path = ProjectSettings::globalize_path(p_path);

	HashMap<String, DAP::Source>::Iterator E = data->breakpoint_source_list.find(global_path);
	if (E != data->breakpoint_source_list.end()) {
		return E->value;
	}
	DAP::Source& added_source =
		data->breakpoint_source_list.insert(global_path, DAP::Source())->value;
	added_source.name = global_path.get_file();
	added_source.path = global_path;
	added_source.compute_checksums();

	return added_source;
}

void DebugAdapterProtocol::update_source(const String& p_path)
{
	ERR_FAIL_NULL(data);
	const String& global_path = ProjectSettings::globalize_path(p_path);

	HashMap<String, DAP::Source>::Iterator E = data->breakpoint_source_list.find(global_path);
	if (E != data->breakpoint_source_list.end()) {
		E->value.compute_checksums();
	}
}

void DebugAdapterProtocol::on_debug_paused()
{
	if (EditorRunBar::get_pause_button()->is_pressed()) {
		notify_stopped_paused();
	}
	else {
		notify_continued();
	}
}

void DebugAdapterProtocol::on_debug_stopped()
{
	notify_exited();
	notify_terminated();
	reset_ids();
}

void DebugAdapterProtocol::on_debug_breaked(const bool& p_reallydid, const bool& p_can_debug,
	const String& p_reason, const bool& p_has_stackdump)
{
	ERR_FAIL_NULL(data);
	if (!p_reallydid) {
		notify_continued();
		return;
	}

	if (p_reason == "Breakpoint") {
		if (data->_stepping) {
			notify_stopped_step();
			data->_stepping = false;
		}
		else {
			data->_processing_breakpoint =
				true; // Wait for stack_dump to find where the breakpoint happened
		}
	}
	else {
		notify_stopped_exception(p_reason);
	}

	data->_processing_stackdump = p_has_stackdump;
}

void DebugAdapterProtocol::on_debug_breakpoint_toggled(
	const String& p_path, const int& p_line, const bool& p_enabled)
{
	ERR_FAIL_NULL(data);
	DAP::Breakpoint breakpoint(fetch_source(p_path));
	breakpoint.verified = true;
	breakpoint.line = p_line;

	if (p_enabled) {
		// Add the breakpoint
		breakpoint.id = data->breakpoint_id++;
		data->breakpoint_list.push_back(breakpoint);
	}
	else {
		// Remove the breakpoint
		List<DAP::Breakpoint>::Element* E = data->breakpoint_list.find(breakpoint);
		if (E) {
			breakpoint.id = E->get().id;
			data->breakpoint_list.erase(E);
		}
	}

	notify_breakpoint(breakpoint, p_enabled);
}

void DebugAdapterProtocol::stop()
{
	ERR_FAIL_NULL(data);
	for (const Ref<DAPeer>& peer : data->clients) {
		peer->connection->disconnect_from_host();
	}

	data->clients.clear();
	if (data->server.is_valid()) {
		data->server->stop();
	}
	data->_initialized = false;
}

void DebugAdapterProtocol::notify_continued() {}

bool DebugAdapterProtocol::process_message(const String& p_message)
{
	(void)p_message;
	return false;
}

void DebugAdapterProtocol::notify_terminated() {}

void DebugAdapterProtocol::notify_stopped_step() {}

void DebugAdapterProtocol::notify_stopped_paused() {}

void DebugAdapterProtocol::notify_stopped_exception(const String& p_error) {}

void DebugAdapterProtocol::notify_exited(const int& p_exit_code) {}

void DebugAdapterProtocol::notify_breakpoint( const DAP::Breakpoint& p_breakpoint, const bool& p_verified) { }
