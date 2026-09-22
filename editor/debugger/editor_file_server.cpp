/**************************************************************************/
/*  editor_file_server.cpp                                                */
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
#include "editor/export/editor_export_platform.h"
#include "editor/settings/editor_settings.h"
#include "editor_file_server.h"

#define FILESYSTEM_PROTOCOL_VERSION 1
#define PASSWORD_LENGTH 32
#define MAX_FILE_BUFFER_SIZE                                                                       \
	100 * 1024 * 1024 // 100mb max file buffer size (description of files to update, compressed).

static void _add_file(String f, const uint64_t& p_modified_time,
	HashMap<String, uint64_t>& files_to_send, HashMap<String, uint64_t>& cached_files)
{
	f = f.replace_first("res://", ""); // remove res://
	const uint64_t* cached_mt = cached_files.getptr(f);
	if (cached_mt && *cached_mt == p_modified_time) {
		// File is good, skip it.
		cached_files.erase(
			f); // Erase to mark this file as existing. Remaining files not added to files_to_send
				// will be considered erased here, so they need to be erased in the client too.
		return;
	}
	files_to_send.insert(f, p_modified_time);
}

void EditorFileServer::_scan_files_changed(EditorFileSystemDirectory* efd,
	const Vector<String>& p_tags, HashMap<String, uint64_t>& files_to_send,
	HashMap<String, uint64_t>& cached_files)
{
	for (int i = 0; i < efd->get_file_count(); i++) {
		String f = efd->get_file_path(i);
		if (FileAccess::exists(f + ".import")) {
			// is imported, determine what to do
			// Todo the modified times of remapped files should most likely be kept in
			// EditorFileSystem to speed this up in the future.
			Ref<ConfigFile> cf;
			cf.instantiate();
			Error err = cf->load(f + ".import");

			ERR_CONTINUE(err != OK);
			{
				uint64_t mt = FileAccess::get_modified_time(f + ".import");
				_add_file(f + ".import", mt, files_to_send, cached_files);
			}

			if (!cf->has_section("remap")) {
				continue;
			}
		}
		else {
			uint64_t mt = efd->get_file_modified_time(i);
			_add_file(f, mt, files_to_send, cached_files);
		}
	}

	for (int i = 0; i < efd->get_subdir_count(); i++) {
		_scan_files_changed(efd->get_subdir(i), p_tags, files_to_send, cached_files);
	}
}

static void _add_custom_file(const String& f, HashMap<String, uint64_t>& files_to_send,
	HashMap<String, uint64_t>& cached_files)
{
	if (!FileAccess::exists(f)) {
		return;
	}
	_add_file(f, FileAccess::get_modified_time(f), files_to_send, cached_files);
}

void EditorFileServer::start()
{
	if (active) {
		stop();
	}
	Error err = server->listen(port);
	ERR_FAIL_COND_MSG(err != OK, "EditorFileServer: Unable to listen on port " + itos(port));
	active = true;
}

bool EditorFileServer::is_active() const { return active; }

void EditorFileServer::stop()
{
	if (active) {
		server->stop();
		active = false;
	}
}

EditorFileServer::EditorFileServer() { server.instantiate(); }

EditorFileServer::~EditorFileServer() { stop(); }


