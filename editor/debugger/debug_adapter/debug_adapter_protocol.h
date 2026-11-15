/**************************************************************************/
/*  debug_adapter_protocol.h                                              */
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

#pragma once

#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "editor/debugger/debug_adapter/debug_adapter_types.h"

#define DAP_MAX_BUFFER_SIZE 4194304 // 4MB
#define DAP_MAX_CLIENTS 8

struct DAPeer : RefCounted
{
	Ref<StreamPeerTCP> connection;

	uint8_t req_buf[DAP_MAX_BUFFER_SIZE];
	int req_pos = 0;
	bool has_header = false;
	int content_length = 0;
	int seq = 0;
	uint64_t timestamp = 0;

	// Client specific info
	bool linesStartAt1 = false;
	bool columnsStartAt1 = false;
	bool supportsVariableType = false;
	bool supportsInvalidatedEvent = false;
	bool supportsCustomData = false;

	// Internal client info
	bool attached = false;

	Error handle_data();
	Error send_data();
};

class DebugAdapterProtocol final
{
	friend class DebugAdapterParser;
	friend class DebugAdapterServer;

	using DAPVarID = int;
	using DAPStackFrameID = int;

private:
	struct Data
	{
		List<Ref<DAPeer>> clients;
		Ref<TCPServer> server;

		bool _initialized = false;
		bool _processing_breakpoint = false;
		bool _stepping = false;
		bool _processing_stackdump = false;
		int _remaining_vars = 0;
		int _current_frame = 0;
		uint64_t _request_timeout = 5000;
		bool _sync_breakpoints = false;

		String _current_request;
		Ref<DAPeer> _current_peer;

		int breakpoint_id = 0;
		int stackframe_id = 0;
		DAPVarID variable_id = 0;
		List<DAP::Breakpoint> breakpoint_list;
		HashMap<String, DAP::Source> breakpoint_source_list;
		List<DAP::StackFrame> stackframe_list;
		HashMap<DAPStackFrameID, Vector<int>> scope_list;
		HashMap<String, DAP::Variable> eval_list;
		HashSet<String> eval_pending_list;
	};

	static inline Data* data = nullptr;

	static void on_debug_paused();
	static void on_debug_stopped();
	static void on_debug_output(const String& p_message, int p_type);
	static void on_debug_breaked(const bool& p_reallydid, const bool& p_can_debug,
		const String& p_reason, const bool& p_has_stackdump);
	static void on_debug_breakpoint_toggled(
		const String& p_path, const int& p_line, const bool& p_enabled);
	static void on_debug_stack_frame_vars(const int& p_size);

	static void reset_current_info();
	static void reset_ids();
	static void reset_stack_info();

	static bool request_remote_evaluate(const String& p_eval, int p_stack_frame);

	static const DAP::Source& fetch_source(const String& p_path);
	static void update_source(const String& p_path);

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	_FORCE_INLINE_ static bool is_active()
	{
		return data && data->_initialized && data->clients.size() > 0;
	}

	static bool process_message(const String& p_text);

	static String get_current_request() { return data ? data->_current_request : String(); }

	static Ref<DAPeer> get_current_peer() { return data ? data->_current_peer : Ref<DAPeer>(); }

	static void notify_initialized();
	static void notify_process();
	static void notify_terminated();
	static void notify_exited(const int& p_exitcode = 0);
	static void notify_stopped_paused();
	static void notify_stopped_exception(const String& p_error);
	static void notify_stopped_breakpoint(const int& p_id);
	static void notify_stopped_step();
	static void notify_continued();
	static void notify_breakpoint(const DAP::Breakpoint& p_breakpoint, const bool& p_enabled);

	static Error start(int p_port, const IPAddress& p_bind_ip);
	static void stop();

	DebugAdapterProtocol() = delete;
	DebugAdapterProtocol(const DebugAdapterProtocol&) = delete;
	DebugAdapterProtocol& operator=(const DebugAdapterProtocol&) = delete;
	~DebugAdapterProtocol() = delete;
};


