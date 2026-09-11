/**************************************************************************/
/*  debug_adapter_types.h                                                 */
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

#include "core/io/file_access.h"

namespace DAP
{

enum ErrorType
{
	UNKNOWN,
	WRONG_PATH,
	NOT_RUNNING,
	TIMEOUT,
	UNKNOWN_PLATFORM,
	MISSING_DEVICE
};

struct Checksum
{
	String algorithm;
	String checksum;
};

struct Source
{
public:
	String name;
	String path;

	void compute_checksums()
	{
		ERR_FAIL_COND(path.is_empty());

		// MD5
		Checksum md5;
		md5.algorithm = "MD5";
		md5.checksum = FileAccess::get_md5(path);

		// SHA-256
		Checksum sha256;
		sha256.algorithm = "SHA256";
		sha256.checksum = FileAccess::get_sha256(path);
	}
};

struct Breakpoint
{
	int id = 0;
	bool verified = false;
	const Source* source = nullptr;
	int line = 0;

	Breakpoint() = default; // Empty constructor is invalid, but is necessary because Godot's
							// collections don't support rvalues.

	Breakpoint(const Source& p_source) : source(&p_source) {}

	bool operator==(const Breakpoint& p_other) const
	{
		return source == p_other.source && line == p_other.line;
	}
};

struct BreakpointLocation
{
	int line = 0;
	int endLine = -1;
};

struct Capabilities
{
	bool supportsConfigurationDoneRequest = true;
	bool supportsEvaluateForHovers = true;
	bool supportsSetVariable = true;
	String supportedChecksumAlgorithms[2] = {"MD5", "SHA256"};
	bool supportsRestartRequest = true;
	bool supportsValueFormattingOptions = true;
	bool supportTerminateDebuggee = true;
	bool supportSuspendDebuggee = true;
	bool supportsTerminateRequest = true;
	bool supportsBreakpointLocationsRequest = true;
};

struct Message
{
	int id = 0;
	String format;
	bool sendTelemetry = false; // Just in case :)
	bool showUser = true;
};

struct Scope
{
	String name;
	String presentationHint;
	int variablesReference = 0;
	bool expensive = false;
};

struct SourceBreakpoint
{
	int line = 0;
};

struct StackFrame
{
	int id = 0;
	String name;
	const Source* source = nullptr;
	int line = 0;
	int column = 0;

	StackFrame() = default; // Empty constructor is invalid, but is necessary because Godot's
							// collections don't support rvalues.

	StackFrame(const Source& p_source) : source(&p_source) {}

	static uint32_t hash(const StackFrame& p_frame) { return hash_murmur3_one_32(p_frame.id); }
};

struct Thread
{
	int id = 0;
	String name;
};

struct Variable
{
	String name;
	String value;
	String type;
	int variablesReference = 0;
};
} // namespace DAP


