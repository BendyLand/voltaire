/**************************************************************************/
/*  editor_vcs_interface.h                                                */
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

#include "core/string/ustring.h"
#include "core/templates/mem_unique_ptr.h"

class EditorVCSInterface
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

protected:
	static EditorVCSInterface* singleton;

	static void _bind_methods();

public:
	static EditorVCSInterface* get_singleton();
	static void set_singleton(EditorVCSInterface* p_singleton);

	enum class VCSMetadata
	{
		NONE,
		GIT,
	};
	static void create_vcs_metadata_files(VCSMetadata p_vcs_metadata_type, String& p_dir);

	// Proxies to the editor for use
	bool initialize(const String& p_project_path);
	void set_credentials(const String& p_username, const String& p_password,
		const String& p_ssh_public_key_path, const String& p_ssh_private_key_path,
		const String& p_ssh_passphrase);
	void stage_file(const String& p_file_path);
	void unstage_file(const String& p_file_path);
	void discard_file(const String& p_file_path);
	void commit(const String& p_msg, bool p_amend);
	bool allow_amends();
	bool shut_down();
	String get_vcs_name();
	void create_branch(const String& p_branch_name);
	void remove_branch(const String& p_branch_name);
	void create_remote(const String& p_remote_name, const String& p_remote_url);
	void remove_remote(const String& p_remote_name);
	String get_current_branch_name();
	bool checkout_branch(const String& p_branch_name);
	void pull(const String& p_remote);
	void push(const String& p_remote, bool p_force);
	void fetch(const String& p_remote);
	void popup_error(const String& p_msg);
};


