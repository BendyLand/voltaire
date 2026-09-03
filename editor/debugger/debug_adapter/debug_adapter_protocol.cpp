/**************************************************************************/
/*  debug_adapter_protocol.cpp                                            */
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
#include "core/io/json.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "debug_adapter_protocol.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"

DebugAdapterProtocol* DebugAdapterProtocol::singleton = nullptr;

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
			timestamp = OS::get_singleton()->get_ticks_msec();
		}

		// Response
		if (DebugAdapterProtocol::get_singleton()->process_message(msg)) {
			// Reset to read again
			req_pos = 0;
			has_header = false;
			timestamp = 0;
		}
	}
	return OK;
}

Error DebugAdapterProtocol::on_client_connected()
{
	ERR_FAIL_COND_V_MSG(clients.size() >= DAP_MAX_CLIENTS, FAILED, "Max client limits reached");

	Ref<StreamPeerTCP> tcp_peer = server->take_connection();
	ERR_FAIL_COND_V_MSG(tcp_peer.is_null(), FAILED, "Failed to take incoming DAP connection.");
	tcp_peer->set_no_delay(true);
	Ref<DAPeer> peer = memnew(DAPeer);
	peer->connection = tcp_peer;
	clients.push_back(peer);

	EditorDebuggerNode::get_singleton()->get_default_debugger()->set_move_to_foreground(false);
	EditorNode::get_log()->add_message("[DAP] Connection Taken", EditorLog::MSG_TYPE_EDITOR);
	return OK;
}

void DebugAdapterProtocol::on_client_disconnected(const Ref<DAPeer>& p_peer)
{
	clients.erase(p_peer);
	if (!clients.size()) {
		reset_ids();
		EditorDebuggerNode::get_singleton()->get_default_debugger()->set_move_to_foreground(true);
	}
	EditorNode::get_log()->add_message("[DAP] Disconnected", EditorLog::MSG_TYPE_EDITOR);
}

void DebugAdapterProtocol::reset_current_info()
{
	_current_request = "";
	_current_peer.unref();
}

void DebugAdapterProtocol::reset_ids()
{
	breakpoint_id = 0;
	breakpoint_list.clear();
	breakpoint_source_list.clear();

	reset_stack_info();
}

void DebugAdapterProtocol::reset_stack_info()
{
	stackframe_id = 0;
	variable_id = 1;

	stackframe_list.clear();
	scope_list.clear();
}

bool DebugAdapterProtocol::request_remote_evaluate(const String& p_eval, int p_stack_frame)
{
	// If the eval is already on the pending list, we don't need to request it again
	if (eval_pending_list.has(p_eval)) {
		return false;
	}

	EditorDebuggerNode::get_singleton()->get_default_debugger()->request_remote_evaluate(
		p_eval, p_stack_frame);
	eval_pending_list.insert(p_eval);

	return true;
}

const DAP::Source& DebugAdapterProtocol::fetch_source(const String& p_path)
{
	const String& global_path = ProjectSettings::get_singleton()->globalize_path(p_path);

	HashMap<String, DAP::Source>::Iterator E = breakpoint_source_list.find(global_path);
	if (E != breakpoint_source_list.end()) {
		return E->value;
	}
	DAP::Source& added_source = breakpoint_source_list.insert(global_path, DAP::Source())->value;
	added_source.name = global_path.get_file();
	added_source.path = global_path;
	added_source.compute_checksums();

	return added_source;
}

void DebugAdapterProtocol::update_source(const String& p_path)
{
	const String& global_path = ProjectSettings::get_singleton()->globalize_path(p_path);

	HashMap<String, DAP::Source>::Iterator E = breakpoint_source_list.find(global_path);
	if (E != breakpoint_source_list.end()) {
		E->value.compute_checksums();
	}
}

void DebugAdapterProtocol::on_debug_paused()
{
	if (EditorRunBar::get_singleton()->get_pause_button()->is_pressed()) {
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
	if (!p_reallydid) {
		notify_continued();
		return;
	}

	if (p_reason == "Breakpoint") {
		if (_stepping) {
			notify_stopped_step();
			_stepping = false;
		}
		else {
			_processing_breakpoint =
				true; // Wait for stack_dump to find where the breakpoint happened
		}
	}
	else {
		notify_stopped_exception(p_reason);
	}

	_processing_stackdump = p_has_stackdump;
}

void DebugAdapterProtocol::on_debug_breakpoint_toggled(
	const String& p_path, const int& p_line, const bool& p_enabled)
{
	DAP::Breakpoint breakpoint(fetch_source(p_path));
	breakpoint.verified = true;
	breakpoint.line = p_line;

	if (p_enabled) {
		// Add the breakpoint
		breakpoint.id = breakpoint_id++;
		breakpoint_list.push_back(breakpoint);
	}
	else {
		// Remove the breakpoint
		List<DAP::Breakpoint>::Element* E = breakpoint_list.find(breakpoint);
		if (E) {
			breakpoint.id = E->get().id;
			breakpoint_list.erase(E);
		}
	}

	notify_breakpoint(breakpoint, p_enabled);
}

void DebugAdapterProtocol::poll()
{
	if (server->is_connection_available()) {
		on_client_connected();
	}
	List<Ref<DAPeer>> to_delete;
	for (const Ref<DAPeer>& peer : clients) {
		peer->connection->poll();
		StreamPeerTCP::Status status = peer->connection->get_status();
		if (status == StreamPeerTCP::STATUS_NONE || status == StreamPeerTCP::STATUS_ERROR) {
			to_delete.push_back(peer);
		}
		else {
			_current_peer = peer;
			Error err = peer->handle_data();
			if (err != OK && err != ERR_BUSY) {
				to_delete.push_back(peer);
			}
			err = peer->send_data();
			if (err != OK && err != ERR_BUSY) {
				to_delete.push_back(peer);
			}
		}
	}

	for (const Ref<DAPeer>& peer : to_delete) {
		on_client_disconnected(peer);
	}
	to_delete.clear();
}

void DebugAdapterProtocol::stop()
{
	for (const Ref<DAPeer>& peer : clients) {
		peer->connection->disconnect_from_host();
	}

	clients.clear();
	server->stop();
	_initialized = false;
}

DebugAdapterProtocol::~DebugAdapterProtocol() {}


