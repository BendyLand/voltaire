/**************************************************************************/
/*  editor_vcs_interface.h                                                */
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

#include "core/string/ustring.h"

class EditorVCSInterface final
{
public:
	enum ChangeType
	{
		CHANGE_TYPE_NEW = 0,
		CHANGE_TYPE_MODIFIED = 1,
		CHANGE_TYPE_RENAMED = 2,
		CHANGE_TYPE_DELETED = 3,
		CHANGE_TYPE_TYPECHANGE = 4,
		CHANGE_TYPE_UNMERGED = 5
	};

	enum TreeArea
	{
		TREE_AREA_COMMIT = 0,
		TREE_AREA_STAGED = 1,
		TREE_AREA_UNSTAGED = 2
	};

	struct DiffLine
	{
		int new_line_no;
		int old_line_no;
		String content;
		String status;

		String old_text;
		String new_text;
	};

	struct DiffHunk
	{
		int new_start;
		int old_start;
		int new_lines;
		int old_lines;
	};

	struct DiffFile
	{
		String new_file;
		String old_file;
	};

	struct Commit
	{
		String author;
		String msg;
		String id;
		int64_t unix_timestamp;
		int64_t offset_minutes;
	};

	struct StatusFile
	{
		TreeArea area;
		ChangeType change_type;
		String file_path;
	};

	struct Data
	{
		bool active = false;
	};

	static inline Data* data = nullptr;

	static void finalize();
	static bool is_initialized() { return data != nullptr && data->active; }

	enum class VCSMetadata
	{
		NONE,
		GIT,
	};
	static void create_vcs_metadata_files(VCSMetadata p_vcs_metadata_type, String& p_dir);

	// Proxies to the editor for use
	static bool initialize(const String& p_project_path);
	static void set_credentials(const String& p_username, const String& p_password,
		const String& p_ssh_public_key_path, const String& p_ssh_private_key_path,
		const String& p_ssh_passphrase);
	static void stage_file(const String& p_file_path);
	static void unstage_file(const String& p_file_path);
	static void discard_file(const String& p_file_path);
	static void commit(const String& p_msg, bool p_amend);
	static bool allow_amends();
	static bool shut_down();
	static String get_vcs_name();
	static void create_branch(const String& p_branch_name);
	static void remove_branch(const String& p_branch_name);
	static void create_remote(const String& p_remote_name, const String& p_remote_url);
	static void remove_remote(const String& p_remote_name);
	static String get_current_branch_name();
	static bool checkout_branch(const String& p_branch_name);
	static void pull(const String& p_remote);
	static void push(const String& p_remote, bool p_force);
	static void fetch(const String& p_remote);
	static void popup_error(const String& p_msg);

	EditorVCSInterface() = delete;
	~EditorVCSInterface() = delete;
};


