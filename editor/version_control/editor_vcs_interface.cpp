/**************************************************************************/
/*  editor_vcs_interface.cpp                                              */
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

#include "editor/editor_node.h"
#include "editor_vcs_interface.h"

EditorVCSInterface* EditorVCSInterface::singleton = nullptr;

void EditorVCSInterface::popup_error(const String& p_msg)
{
	// TRANSLATORS: %s refers to the name of a version control system (e.g. "Git").
	EditorNode::get_singleton()->show_warning(
		p_msg.strip_edges(), vformat(TTR("%s Error"), get_vcs_name()));
}

EditorVCSInterface* EditorVCSInterface::get_singleton() { return singleton; }

void EditorVCSInterface::set_singleton(EditorVCSInterface* p_singleton) { singleton = p_singleton; }

void EditorVCSInterface::create_vcs_metadata_files(VCSMetadata p_vcs_metadata_type, String& p_dir)
{
	if (p_vcs_metadata_type == VCSMetadata::GIT) {
		Ref<FileAccess> f = FileAccess::open(p_dir.path_join(".gitignore"), FileAccess::WRITE);
		if (f.is_null()) {
			ERR_FAIL_MSG("Couldn't create .gitignore in project path.");
		}
		else {
			f->store_line("# Godot 4+ specific ignores");
			f->store_line(".godot/");
			f->store_line("/android/");
		}
		f = FileAccess::open(p_dir.path_join(".gitattributes"), FileAccess::WRITE);
		if (f.is_null()) {
			ERR_FAIL_MSG("Couldn't create .gitattributes in project path.");
		}
		else {
			f->store_line("# Normalize EOL for all files that Git considers text files.");
			f->store_line("* text=auto eol=lf");
		}
	}
}

bool EditorVCSInterface::shut_down() { return true; }

void EditorVCSInterface::push(const String& p_remote, bool p_force) {}

void EditorVCSInterface::stage_file(const String& p_file_path) {}

void EditorVCSInterface::unstage_file(const String& p_file_path) {}

void EditorVCSInterface::set_credentials(const String& p_username, const String& p_password,
	const String& p_ssh_public_key, const String& p_ssh_private_key, const String& p_ssh_passphrase)
{
}

void EditorVCSInterface::discard_file(const String& p_file_path) {}

void EditorVCSInterface::create_remote(const String& p_remote_name, const String& p_remote_url) {}

String EditorVCSInterface::get_current_branch_name() { return String(); }

void EditorVCSInterface::create_branch(const String& p_branch_name) {}

bool EditorVCSInterface::checkout_branch(const String& p_branch_name) { return true; }

void EditorVCSInterface::fetch(const String& p_remote) {}

void EditorVCSInterface::remove_remote(const String& p_remote_name) {}

void EditorVCSInterface::remove_branch(const String& p_branch_name) {}

bool EditorVCSInterface::initialize(const String& p_project_path) { return false; }

bool EditorVCSInterface::allow_amends() { return false; }

String EditorVCSInterface::get_vcs_name() { return String(); }

void EditorVCSInterface::pull(const String& p_remote) {}


