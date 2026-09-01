/**************************************************************************/
/*  editor_file_system.cpp                                                */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/os.h"
#include "editor/doc/editor_help.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_paths.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor_file_system.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

EditorFileSystem* EditorFileSystem::singleton = nullptr;
int EditorFileSystem::nb_files_total = 0;
EditorFileSystem::ScannedDirectory* EditorFileSystem::first_scan_root_dir = nullptr;

// the name is the version, to keep compatibility with different versions of Godot
#define CACHE_FILE_NAME "filesystem_cache10"

int EditorFileSystemDirectory::find_file_index(const String& p_file) const
{
	for (int i = 0; i < files.size(); i++) {
		if (files[i]->file == p_file) {
			return i;
		}
	}
	return -1;
}

int EditorFileSystemDirectory::find_dir_index(const String& p_dir) const
{
	for (int i = 0; i < subdirs.size(); i++) {
		if (subdirs[i]->name == p_dir) {
			return i;
		}
	}

	return -1;
}

void EditorFileSystemDirectory::force_update()
{
	// We set modified_time to 0 to force `EditorFileSystem::_scan_fs_changes` to search changes in
	// the directory
	modified_time = 0;
}

int EditorFileSystemDirectory::get_subdir_count() const { return subdirs.size(); }

EditorFileSystemDirectory* EditorFileSystemDirectory::get_subdir(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, subdirs.size(), nullptr);
	return subdirs[p_idx];
}

int EditorFileSystemDirectory::get_file_count() const { return files.size(); }

String EditorFileSystemDirectory::get_file(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");

	return files[p_idx]->file;
}

String EditorFileSystemDirectory::get_path() const
{
	int parents = 0;
	const EditorFileSystemDirectory* efd = this;
	// Determine the level of nesting.
	while (efd->parent) {
		parents++;
		efd = efd->parent;
	}

	if (parents == 0) {
		return "res://";
	}

	// Using PackedStringArray, because the path is built in reverse order.
	PackedStringArray path_bits;
	// Allocate an array based on nesting. It will store path bits.
	path_bits.resize(parents + 2); // Last String is empty, so paths end with /.
	String* path_write = path_bits.ptrw();
	path_write[0] = "res:/";

	efd = this;
	for (int i = parents; i > 0; i--) {
		path_write[i] = efd->name;
		efd = efd->parent;
	}
	return String("/").join(path_bits);
}

String EditorFileSystemDirectory::get_file_path(int p_idx) const
{
	return get_path().path_join(get_file(p_idx));
}

ResourceUID::ID EditorFileSystemDirectory::get_file_uid(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), ResourceUID::INVALID_ID);
	return files[p_idx]->uid;
}

Vector<String> EditorFileSystemDirectory::get_file_deps(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), Vector<String>());
	Vector<String> deps;

	for (int i = 0; i < files[p_idx]->deps.size(); i++) {
		String dep = files[p_idx]->deps[i];
		int sep_idx = dep.find("::"); // may contain type information, unwanted
		if (sep_idx != -1) {
			dep = dep.substr(0, sep_idx);
		}
		ResourceUID::ID uid = ResourceUID::get_singleton()->text_to_id(dep);
		if (uid != ResourceUID::INVALID_ID) {
			// return proper dependency resource from uid
			if (ResourceUID::get_singleton()->has_id(uid)) {
				dep = ResourceUID::get_singleton()->get_id_path(uid);
			}
			else {
				continue;
			}
		}
		deps.push_back(dep);
	}
	return deps;
}

bool EditorFileSystemDirectory::get_file_import_is_valid(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), false);
	return files[p_idx]->import_valid;
}

uint64_t EditorFileSystemDirectory::get_file_modified_time(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), 0);
	return files[p_idx]->modified_time;
}

uint64_t EditorFileSystemDirectory::get_file_import_modified_time(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), 0);
	return files[p_idx]->import_modified_time;
}

String EditorFileSystemDirectory::get_file_script_class_name(int p_idx) const
{
	return files[p_idx]->class_info.name;
}

String EditorFileSystemDirectory::get_file_script_class_extends(int p_idx) const
{
	return files[p_idx]->class_info.extends;
}

String EditorFileSystemDirectory::get_file_script_class_icon_path(int p_idx) const
{
	return files[p_idx]->class_info.icon_path;
}

String EditorFileSystemDirectory::get_file_icon_path(int p_idx) const
{
	return files[p_idx]->class_info.icon_path;
}

StringName EditorFileSystemDirectory::get_file_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");
	return files[p_idx]->type;
}

StringName EditorFileSystemDirectory::get_file_resource_script_class(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");
	return files[p_idx]->resource_script_class;
}

String EditorFileSystemDirectory::get_name() { return name; }

EditorFileSystemDirectory* EditorFileSystemDirectory::get_parent() { return parent; }

void EditorFileSystemDirectory::_bind_methods() {}

EditorFileSystemDirectory::EditorFileSystemDirectory()
{
	modified_time = 0;
	parent = nullptr;
}

EditorFileSystemDirectory::~EditorFileSystemDirectory()
{
	for (EditorFileSystemDirectory::FileInfo* fi : files) {
		memdelete(fi);
	}

	for (EditorFileSystemDirectory* dir : subdirs) {
		memdelete(dir);
	}
}

EditorFileSystem::ScannedDirectory::~ScannedDirectory()
{
	for (ScannedDirectory* dir : subdirs) {
		memdelete(dir);
	}
}

void EditorFileSystem::_load_first_scan_root_dir()
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	first_scan_root_dir = memnew(ScannedDirectory);
	first_scan_root_dir->full_path = "res://";

	nb_files_total = _scan_new_dir(first_scan_root_dir, d);
}

void EditorFileSystem::scan_for_uid()
{
	// Load file structure into memory.
	_load_first_scan_root_dir();

	// Load extensions for which an .import should exists.
	List<String> extensionsl;
	HashSet<String> import_extensions;
	ResourceFormatImporter::get_singleton()->get_recognized_extensions(&extensionsl);
	for (const String& E : extensionsl) {
		import_extensions.insert(E);
	}

	// Scan the file system to load uid.
	_scan_for_uid_directory(first_scan_root_dir, import_extensions);

	// It's done, resetting the callback method to prevent a second scan.
	ResourceUID::scan_for_uid_on_startup = nullptr;
}

void EditorFileSystem::_scan_for_uid_directory(
	const ScannedDirectory* p_scan_dir, const HashSet<String>& p_import_extensions)
{
	for (ScannedDirectory* scan_sub_dir : p_scan_dir->subdirs) {
		_scan_for_uid_directory(scan_sub_dir, p_import_extensions);
	}

	for (const String& scan_file : p_scan_dir->files) {
		const String ext = scan_file.get_extension().to_lower();

		if (ext == "uid" || ext == "import") {
			continue;
		}

		const String path = p_scan_dir->full_path.path_join(scan_file);
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		if (p_import_extensions.has(ext)) {
			if (FileAccess::exists(path + ".import")) {
				uid = ResourceFormatImporter::get_singleton()->get_resource_uid(path);
			}
		}
		else {
			uid = ResourceLoader::get_resource_uid(path);
		}

		if (uid != ResourceUID::INVALID_ID) {
			if (!ResourceUID::get_singleton()->has_id(uid)) {
				ResourceUID::get_singleton()->add_id(uid, path);
			}
		}
	}
}

void EditorFileSystem::_first_scan_filesystem()
{
	EditorProgress ep = EditorProgress("first_scan_filesystem", TTR("Project initialization"), 5);
	HashSet<String> existing_class_names;
	HashSet<String> extensions;

	if (!first_scan_root_dir) {
		ep.step(TTR("Scanning file structure..."), 0, true);
		_load_first_scan_root_dir();
	}

	// Preloading GDExtensions file extensions to prevent looping on all the resource loaders
	// for each files in _first_scan_process_scripts.
	List<String> gdextension_extensions;
	ResourceLoader::get_recognized_extensions_for_type("GDExtension", &gdextension_extensions);

	// This loads the global class names from the scripts and ensures that even if the
	// global_script_class_cache.cfg was missing or invalid, the global class names are valid in
	// ScriptServer. At the same time, to prevent looping multiple times in all files, it looks for
	// extensions.
	ep.step(TTR("Loading global class names..."), 1, true);
	_first_scan_process_scripts(
		first_scan_root_dir, gdextension_extensions, existing_class_names, extensions);

	// Removing invalid global class to prevent having invalid paths in ScriptServer.
	bool save_scripts = _remove_invalid_global_class_names(existing_class_names);

	// If a global class is found or removed, we sync global_script_class_cache.cfg with the
	// ScriptServer
	if (!existing_class_names.is_empty() || save_scripts) {
		EditorNode::get_editor_data().script_class_save_global_classes();
	}

	// Processing extensions to add new extensions or remove invalid ones.
	// Important to do it in the first scan so custom types, new class names, custom importers,
	// etc... from extensions are ready to go before plugins, autoloads and resources
	// validation/importation. At this point, a restart of the editor should not be needed so we
	// don't use the return value.
	ep.step(TTR("Verifying GDExtensions..."), 2, true);

	// Now that all the global class names should be loaded, create autoloads and plugins.
	// This is done after loading the global class names because autoloads and plugins can use
	// global class names.
	ep.step(TTR("Creating autoload scripts..."), 3, true);
	ProjectSettingsEditor::get_singleton()->init_autoloads();

	ep.step(TTR("Initializing plugins..."), 4, true);
	EditorNode::get_singleton()->init_plugins();

	ep.step(TTR("Starting file scan..."), 5, true);
}

void EditorFileSystem::_scan_filesystem()
{
	// On the first scan, the first_scan_root_dir is created in _first_scan_filesystem.
	ERR_FAIL_COND(!scanning || new_filesystem || (first_scan && !first_scan_root_dir));

	// read .fscache
	String cpath;

	sources_changed.clear();
	file_cache.clear();

	String fscache =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join(CACHE_FILE_NAME);
	{
		Ref<FileAccess> f = FileAccess::open(fscache, FileAccess::READ);

		bool first = true;
		if (f.is_valid()) {
			// read the disk cache
			while (!f->eof_reached()) {
				String l = f->get_line().strip_edges();
				if (first) {
					if (first_scan) {
						// only use this on first scan, afterwards it gets ignored
						// this is so on first reimport we synchronize versions, then
						// we don't care until editor restart. This is for usability mainly so
						// your workflow is not killed after changing a setting by forceful
						// reimporting everything there is.
						filesystem_settings_version_for_import = l.strip_edges();
						if (filesystem_settings_version_for_import !=
							ResourceFormatImporter::get_singleton()->get_import_settings_hash()) {
							revalidate_import_files = true;
						}
					}
					first = false;
					continue;
				}
				if (l.is_empty()) {
					continue;
				}

				if (l.begins_with("::")) {
					Vector<String> split = l.split("::");
					ERR_CONTINUE(split.size() != 3);
					const String& name = split[1];

					cpath = name;

				}
				else {
					// The last section (deps) may contain the same splitter, so limit the maxsplit
					// to 8 to get the complete deps.
					Vector<String> split = l.split("::", true, 8);
					ERR_CONTINUE(split.size() < 9);
					const String name = cpath.path_join(split[0]);

					FileCache fc;
					fc.type = split[1].get_slicec('/', 0);
					fc.resource_script_class = split[1].get_slicec('/', 1);
					fc.uid = split[2].to_int();
					fc.modification_time = split[3].to_int();
					fc.import_modification_time = split[4].to_int();
					fc.import_valid = split[5].to_int() != 0;
					fc.import_group_file = split[6].strip_edges();
					{
						const Vector<String>& slices = split[7].split("<>");
						ERR_CONTINUE(slices.size() < 7);
						fc.class_info.name = slices[0];
						fc.class_info.extends = slices[1];
						fc.class_info.icon_path = slices[2];
						fc.class_info.is_abstract = slices[3].to_int();
						fc.class_info.is_tool = slices[4].to_int();
						fc.import_md5 = slices[5];
						fc.import_dest_paths = slices[6].split("<*>");
					}
					fc.deps = split[8].strip_edges().split("<>", false);

					file_cache[name] = fc;
				}
			}
		}
	}

	const String update_cache =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join("filesystem_update4");
	if (first_scan && FileAccess::exists(update_cache)) {
		{
			Ref<FileAccess> f2 = FileAccess::open(update_cache, FileAccess::READ);
			String l = f2->get_line().strip_edges();
			while (!l.is_empty()) {
				dep_update_list.insert(l);
				file_cache.erase(l); // Erase cache for this, so it gets updated.
				l = f2->get_line().strip_edges();
			}
		}

		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		d->remove(update_cache); // Bye bye update cache.
	}

	EditorProgressBG scan_progress("efs", "ScanFS", 1000);
	ScanProgress sp;
	sp.hi = nb_files_total;
	sp.progress = &scan_progress;

	new_filesystem = memnew(EditorFileSystemDirectory);
	new_filesystem->parent = nullptr;

	ScannedDirectory* sd;
	HashSet<String>* processed_files = nullptr;
	// On the first scan, the first_scan_root_dir is created in _first_scan_filesystem.
	if (first_scan) {
		sd = first_scan_root_dir;
		// Will be updated on scan.
		ResourceUID::get_singleton()->clear();
		ResourceUID::scan_for_uid_on_startup = nullptr;
		processed_files = memnew(HashSet<String>());
	}
	else {
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		sd = memnew(ScannedDirectory);
		sd->full_path = "res://";
		nb_files_total = _scan_new_dir(sd, d);
	}

	_process_file_system(sd, new_filesystem, sp, processed_files);

	if (first_scan) {
		_process_removed_files(*processed_files);
	}
	dep_update_list.clear();
	file_cache.clear(); // clear caches, no longer needed

	if (first_scan) {
		memdelete(first_scan_root_dir);
		first_scan_root_dir = nullptr;
		memdelete(processed_files);
	}
	else {
		// on the first scan this is done from the main thread after re-importing
		_save_filesystem_cache();
	}

	scanning = false;
}

void EditorFileSystem::_save_filesystem_cache()
{
	group_file_cache.clear();

	String fscache =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join(CACHE_FILE_NAME);

	Ref<FileAccess> f = FileAccess::open(fscache, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(
		f.is_null(), "Cannot create file '" + fscache + "'. Check user write permissions.");

	f->store_line(filesystem_settings_version_for_import);
	_save_filesystem_cache(filesystem, f);
}

void EditorFileSystem::_thread_func(void* _userdata)
{
	EditorFileSystem* sd = (EditorFileSystem*)_userdata;
	sd->_scan_filesystem();
}

bool EditorFileSystem::_is_test_for_reimport_needed(const String& p_path,
	uint64_t p_last_modification_time, uint64_t p_modification_time,
	uint64_t p_last_import_modification_time, uint64_t p_import_modification_time,
	const Vector<String>& p_import_dest_paths)
{
	// The idea here is to trust the cache. If the last modification times in the cache correspond
	// to the last modification times of the files on disk, it means the files have not changed
	// since the last import, and the files in .godot/imported (p_import_dest_paths) should all be
	// valid.
	if (p_last_modification_time != p_modification_time) {
		return true;
	}
	if (p_last_import_modification_time != p_import_modification_time) {
		return true;
	}
	if (reimport_on_missing_imported_files) {
		for (const String& path : p_import_dest_paths) {
			if (!FileAccess::exists(path)) {
				return true;
			}
		}
	}
	return false;
}

bool EditorFileSystem::_scan_import_support(const Vector<String>& reimports)
{
	if (import_support_queries.is_empty()) {
		return false;
	}
	HashMap<String, int> import_support_test;
	Vector<bool> import_support_tested;
	import_support_tested.resize(import_support_queries.size());
	for (int i = 0; i < import_support_queries.size(); i++) {
		import_support_tested.write[i] = false;
	}

	if (import_support_test.is_empty()) {
		return false; // well nothing to do
	}

	for (int i = 0; i < reimports.size(); i++) {
		HashMap<String, int>::Iterator E =
			import_support_test.find(reimports[i].get_extension().to_lower());
		if (E) {
			import_support_tested.write[E->value] = true;
		}
	}

	return false;
}

void EditorFileSystem::ScanProgress::increment()
{
	current++;
	float ratio = current / MAX(hi, 1.0f);
	if (progress) {
		progress->step(ratio * 1000.0f);
	}
	EditorFileSystem::singleton->scan_total = ratio;
}

int EditorFileSystem::_scan_new_dir(ScannedDirectory* p_dir, Ref<DirAccess>& da)
{
	List<String> dirs;
	List<String> files;

	String cd = da->get_current_dir();

	da->list_dir_begin();
	while (true) {
		String f = da->get_next();
		if (f.is_empty()) {
			break;
		}

		if (da->current_is_hidden()) {
			continue;
		}

		if (da->current_is_dir()) {
			if (f.begins_with(".")) { // Ignore special and . / ..
				continue;
			}

			if (_should_skip_directory(cd.path_join(f))) {
				continue;
			}

			dirs.push_back(f);

		}
		else {
			files.push_back(f);
		}
	}

	da->list_dir_end();

	dirs.sort_custom<FileNoCaseComparator>();
	files.sort_custom<FileNoCaseComparator>();

	int nb_files_total_scan = 0;

	for (const String& dir : dirs) {
		if (da->change_dir(dir) == OK) {
			String d = da->get_current_dir();

			if (d == cd || !d.begins_with(cd)) {
				da->change_dir(cd); // avoid recursion
			}
			else {
				ScannedDirectory* sd = memnew(ScannedDirectory);
				sd->name = dir;
				sd->full_path = p_dir->full_path.path_join(sd->name);

				nb_files_total_scan += _scan_new_dir(sd, da);

				p_dir->subdirs.push_back(sd);

				da->change_dir("..");
			}
		}
		else {
			ERR_PRINT("Cannot go into subdir '" + dir + "'.");
		}
	}

	p_dir->files = files;
	nb_files_total_scan += files.size();

	return nb_files_total_scan;
}

void EditorFileSystem::_process_removed_files(const HashSet<String>& p_processed_files) {}

void EditorFileSystem::_scan_fs_changes(
	EditorFileSystemDirectory* p_dir, ScanProgress& p_progress, bool p_recursive)
{
	uint64_t current_mtime = FileAccess::get_modified_time(p_dir->get_path());

	bool updated_dir = false;
	String cd = p_dir->get_path();
	int diff_nb_files = 0;

	if (current_mtime != p_dir->modified_time || using_fat32_or_exfat) {
		updated_dir = true;
		p_dir->modified_time = current_mtime;
		// ooooops, dir changed, see what's going on

		// first mark everything as verified

		for (int i = 0; i < p_dir->files.size(); i++) {
			p_dir->files[i]->verified = false;
		}

		for (int i = 0; i < p_dir->subdirs.size(); i++) {
			p_dir->get_subdir(i)->verified = false;
		}

		diff_nb_files -= p_dir->files.size();

		// then scan files and directories and check what's different

		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);

		Error ret = da->change_dir(cd);
		ERR_FAIL_COND_MSG(ret != OK, "Cannot change to '" + cd + "' folder.");

		da->list_dir_begin();
		while (true) {
			String f = da->get_next();
			if (f.is_empty()) {
				break;
			}

			if (da->current_is_hidden()) {
				continue;
			}

			if (da->current_is_dir()) {
				if (f.begins_with(".")) { // Ignore special and . / ..
					continue;
				}

				int idx = p_dir->find_dir_index(f);
				if (idx == -1) {
					String dir_path = cd.path_join(f);
					if (_should_skip_directory(dir_path)) {
						continue;
					}

					ScannedDirectory sd;
					sd.name = f;
					sd.full_path = dir_path;

					EditorFileSystemDirectory* efd = memnew(EditorFileSystemDirectory);
					efd->parent = p_dir;
					efd->name = f;

					Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
					d->change_dir(dir_path);
					int nb_files_dir = _scan_new_dir(&sd, d);
					p_progress.hi += nb_files_dir;
					diff_nb_files += nb_files_dir;
					_process_file_system(&sd, efd, p_progress, nullptr);

					ItemAction ia;
					ia.action = ItemAction::ACTION_DIR_ADD;
					ia.dir = p_dir;
					ia.file = f;
					ia.new_dir = efd;
					scan_actions.push_back(ia);
				}
				else {
					p_dir->subdirs[idx]->verified = true;
				}

			}
			else {
				String ext = f.get_extension().to_lower();
				if (!valid_extensions.has(ext)) {
					continue; // invalid
				}

				int idx = p_dir->find_file_index(f);

				if (idx == -1) {
					// never seen this file, add actition to add it
					EditorFileSystemDirectory::FileInfo* fi =
						memnew(EditorFileSystemDirectory::FileInfo);
					fi->file = f;

					String path = cd.path_join(fi->file);
					fi->modified_time = FileAccess::get_modified_time(path);
					fi->import_modified_time = 0;
					fi->import_md5 = "";
					fi->import_dest_paths = Vector<String>();
					fi->type = ResourceLoader::get_resource_type(path);
					fi->resource_script_class = ResourceLoader::get_resource_script_class(path);
					if (fi->type == "" && textfile_extensions.has(ext)) {
						fi->type = "TextFile";
					}
					if (fi->type == "" && other_file_extensions.has(ext)) {
						fi->type = "OtherFile";
					}
					fi->class_info = _get_global_script_class(fi->type, path);
					fi->import_valid = (fi->type == "TextFile" || fi->type == "OtherFile")
										   ? true
										   : ResourceLoader::is_import_valid(path);
					fi->import_group_file = ResourceLoader::get_import_group_file(path);

					{
						ItemAction ia;
						ia.action = ItemAction::ACTION_FILE_ADD;
						ia.dir = p_dir;
						ia.file = f;
						ia.new_file = fi;
						scan_actions.push_back(ia);
					}

					if (_can_import_file(f)) {
						// if it can be imported, and it was added, it needs to be reimported
						ItemAction ia;
						ia.action = ItemAction::ACTION_FILE_TEST_REIMPORT;
						ia.dir = p_dir;
						ia.file = f;
						scan_actions.push_back(ia);
					}
					diff_nb_files++;
				}
				else {
					p_dir->files[idx]->verified = true;
				}
			}
		}

		da->list_dir_end();
	}

	for (int i = 0; i < p_dir->files.size(); i++) {
		if (updated_dir && !p_dir->files[i]->verified) {
			// this file was removed, add action to remove it
			ItemAction ia;
			ia.action = ItemAction::ACTION_FILE_REMOVE;
			ia.dir = p_dir;
			ia.file = p_dir->files[i]->file;
			scan_actions.push_back(ia);
			diff_nb_files--;
			continue;
		}

		String path = cd.path_join(p_dir->files[i]->file);

		if (_can_import_file(p_dir->files[i]->file)) {
			// Check here if file must be imported or not.
			// Same logic as in _process_file_system, the last modifications dates
			// needs to be trusted to prevent reading all the .import files and the md5
			// each time the user switch back to Godot.
			uint64_t mt = FileAccess::get_modified_time(path);
			uint64_t import_mt = FileAccess::get_modified_time(path + ".import");
			if (_is_test_for_reimport_needed(path, p_dir->files[i]->modified_time, mt,
					p_dir->files[i]->import_modified_time, import_mt,
					p_dir->files[i]->import_dest_paths)) {
				ItemAction ia;
				ia.action = ItemAction::ACTION_FILE_TEST_REIMPORT;
				ia.dir = p_dir;
				ia.file = p_dir->files[i]->file;
				scan_actions.push_back(ia);
			}
		}
		else {
			uint64_t mt = FileAccess::get_modified_time(path);

			if (mt != p_dir->files[i]->modified_time) {
				p_dir->files[i]->modified_time = mt; // save new time, but test for reload

				ItemAction ia;
				ia.action = ItemAction::ACTION_FILE_RELOAD;
				ia.dir = p_dir;
				ia.file = p_dir->files[i]->file;
				scan_actions.push_back(ia);
			}
		}

		p_progress.increment();
	}

	for (int i = 0; i < p_dir->subdirs.size(); i++) {
		if ((updated_dir && !p_dir->subdirs[i]->verified) ||
			_should_skip_directory(p_dir->subdirs[i]->get_path())) {
			// Add all the files of the folder to be sure _update_scan_actions process the removed
			// files for global class names.
			diff_nb_files += _insert_actions_delete_files_directory(p_dir->subdirs[i]);

			// this directory was removed or ignored, add action to remove it
			ItemAction ia;
			ia.action = ItemAction::ACTION_DIR_REMOVE;
			ia.dir = p_dir->subdirs[i];
			scan_actions.push_back(ia);
			continue;
		}
		if (p_recursive) {
			_scan_fs_changes(p_dir->get_subdir(i), p_progress);
		}
	}

	nb_files_total = MAX(nb_files_total + diff_nb_files, 0);
}

void EditorFileSystem::_delete_internal_files(const String& p_file)
{
	if (FileAccess::exists(p_file + ".import")) {
		List<String> paths;
		ResourceFormatImporter::get_singleton()->get_internal_resource_path_list(p_file, &paths);
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		for (const String& E : paths) {
			da->remove(E);
		}
		da->remove(p_file + ".import");
	}
	if (FileAccess::exists(p_file + ".uid")) {
		DirAccess::remove_absolute(p_file + ".uid");
	}
}

int EditorFileSystem::_insert_actions_delete_files_directory(EditorFileSystemDirectory* p_dir)
{
	int nb_files = 0;
	for (EditorFileSystemDirectory::FileInfo* fi : p_dir->files) {
		ItemAction ia;
		ia.action = ItemAction::ACTION_FILE_REMOVE;
		ia.dir = p_dir;
		ia.file = fi->file;
		scan_actions.push_back(ia);
		nb_files++;
	}

	for (EditorFileSystemDirectory* sub_dir : p_dir->subdirs) {
		nb_files += _insert_actions_delete_files_directory(sub_dir);
	}

	return nb_files;
}

void EditorFileSystem::_thread_func_sources(void* _userdata)
{
	EditorFileSystem* efs = (EditorFileSystem*)_userdata;
	if (efs->filesystem) {
		EditorProgressBG pr("sources", TTR("ScanSources"), 1000);
		ScanProgress sp;
		sp.progress = &pr;
		sp.hi = efs->nb_files_total;
		efs->_scan_fs_changes(efs->filesystem, sp);
	}
	efs->scanning_changes_done.set();
}

String EditorFileSystem::_get_file_by_class_name(EditorFileSystemDirectory* p_dir,
	const String& p_class_name, EditorFileSystemDirectory::FileInfo*& r_file_info)
{
	for (EditorFileSystemDirectory::FileInfo* fi : p_dir->files) {
		if (fi->class_info.name == p_class_name) {
			r_file_info = fi;
			return p_dir->get_path().path_join(fi->file);
		}
	}

	for (EditorFileSystemDirectory* sub_dir : p_dir->subdirs) {
		String file = _get_file_by_class_name(sub_dir, p_class_name, r_file_info);
		if (!file.is_empty()) {
			return file;
		}
	}
	r_file_info = nullptr;
	return "";
}

bool EditorFileSystem::is_scanning() const { return scanning || scanning_changes || first_scan; }

float EditorFileSystem::get_scanning_progress() const { return scan_total; }

EditorFileSystemDirectory* EditorFileSystem::get_filesystem() { return filesystem; }

void EditorFileSystem::_save_filesystem_cache(
	EditorFileSystemDirectory* p_dir, Ref<FileAccess> p_file)
{
	if (!p_dir) {
		return; // none
	}
	p_file->store_line("::" + p_dir->get_path() + "::" + String::num_int64(p_dir->modified_time));

	for (int i = 0; i < p_dir->files.size(); i++) {
		const EditorFileSystemDirectory::FileInfo* file_info = p_dir->files[i];
		if (!file_info->import_group_file.is_empty()) {
			group_file_cache.insert(file_info->import_group_file);
		}

		String type = file_info->type;
		if (file_info->resource_script_class) {
			type += "/" + String(file_info->resource_script_class);
		}

		PackedStringArray cache_string;
		cache_string.append(file_info->file);
		cache_string.append(type);
		cache_string.append(itos(file_info->uid));
		cache_string.append(itos(file_info->modified_time));
		cache_string.append(itos(file_info->import_modified_time));
		cache_string.append(itos(file_info->import_valid));
		cache_string.append(file_info->import_group_file);
		cache_string.append(String("<>").join({file_info->class_info.name,
			file_info->class_info.extends, file_info->class_info.icon_path,
			itos(file_info->class_info.is_abstract), itos(file_info->class_info.is_tool),
			file_info->import_md5, String("<*>").join(file_info->import_dest_paths)}));
		cache_string.append(String("<>").join(file_info->deps));

		p_file->store_line(String("::").join(cache_string));
	}

	for (int i = 0; i < p_dir->subdirs.size(); i++) {
		_save_filesystem_cache(p_dir->subdirs[i], p_file);
	}
}

bool EditorFileSystem::_find_file(
	const String& p_file, EditorFileSystemDirectory** r_d, int& r_file_pos) const
{
	// todo make faster

	if (!filesystem || scanning) {
		return false;
	}

	String f = ProjectSettings::get_singleton()->localize_path(p_file);
	if (!f.begins_with("res://")) {
		return false;
	}
	f = f.substr(6);
	f = f.replace_char('\\', '/');

	Vector<String> path = f.split("/");

	if (path.is_empty()) {
		return false;
	}
	const String file = path[path.size() - 1];
	const String file_lower = file.to_lower();
	path.resize(path.size() - 1);

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	EditorFileSystemDirectory* fs = filesystem;

	for (const String& path_bit : path) {
		if (path_bit.begins_with(".")) {
			return false;
		}
		const String path_bit_lower = path_bit.to_lower();

		int idx = -1;
		for (int j = 0; j < fs->get_subdir_count(); j++) {
			if (is_case_sensitive) {
				if (fs->get_subdir(j)->get_name() == path_bit) {
					idx = j;
					break;
				}
			}
			else {
				if (fs->get_subdir(j)->get_name().to_lower() == path_bit_lower) {
					idx = j;
					break;
				}
			}
		}

		if (idx == -1) {
			// Only create a missing directory in memory when it exists on disk.
			if (!dir->dir_exists(fs->get_path().path_join(path_bit))) {
				return false;
			}
			EditorFileSystemDirectory* efsd = memnew(EditorFileSystemDirectory);

			efsd->name = path_bit;
			efsd->parent = fs;

			int idx2 = 0;
			for (int j = 0; j < fs->get_subdir_count(); j++) {
				if (efsd->name.filenocasecmp_to(fs->get_subdir(j)->get_name()) < 0) {
					break;
				}
				idx2++;
			}

			if (idx2 == fs->get_subdir_count()) {
				fs->subdirs.push_back(efsd);
			}
			else {
				fs->subdirs.insert(idx2, efsd);
			}
			fs = efsd;
		}
		else {
			fs = fs->get_subdir(idx);
		}
	}

	int cpos = -1;
	for (int i = 0; i < fs->files.size(); i++) {
		if (is_case_sensitive) {
			if (fs->files[i]->file == file) {
				cpos = i;
				break;
			}
		}
		else {
			if (fs->files[i]->file.to_lower() == file_lower) {
				cpos = i;
				break;
			}
		}
	}

	r_file_pos = cpos;
	*r_d = fs;

	return cpos != -1;
}

String EditorFileSystem::get_file_type(const String& p_file) const
{
	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;

	if (!_find_file(p_file, &fs, cpos)) {
		return "";
	}

	return fs->files[cpos]->type;
}

EditorFileSystemDirectory* EditorFileSystem::find_file(const String& p_file, int* r_index) const
{
	if (!filesystem || scanning) {
		return nullptr;
	}

	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;
	if (!_find_file(p_file, &fs, cpos)) {
		return nullptr;
	}

	if (r_index) {
		*r_index = cpos;
	}

	return fs;
}

ResourceUID::ID EditorFileSystem::get_file_uid(const String& p_path) const
{
	int file_idx;
	EditorFileSystemDirectory* directory = find_file(p_path, &file_idx);

	if (!directory) {
		return ResourceUID::INVALID_ID;
	}
	return directory->files[file_idx]->uid;
}

EditorFileSystemDirectory* EditorFileSystem::get_filesystem_path(const String& p_path)
{
	if (!filesystem || scanning) {
		return nullptr;
	}

	String f = ProjectSettings::get_singleton()->localize_path(p_path);

	if (!f.begins_with("res://")) {
		return nullptr;
	}

	f = f.substr(6);
	f = f.replace_char('\\', '/');
	if (f.is_empty()) {
		return filesystem;
	}

	if (f.ends_with("/")) {
		f = f.substr(0, f.length() - 1);
	}

	Vector<String> path = f.split("/");

	if (path.is_empty()) {
		return nullptr;
	}

	EditorFileSystemDirectory* fs = filesystem;

	for (int i = 0; i < path.size(); i++) {
		int idx = -1;
		for (int j = 0; j < fs->get_subdir_count(); j++) {
			if (fs->get_subdir(j)->get_name() == path[i]) {
				idx = j;
				break;
			}
		}

		if (idx == -1) {
			return nullptr;
		}
		else {
			fs = fs->get_subdir(idx);
		}
	}

	return fs;
}

void EditorFileSystem::_save_late_updated_files()
{
	// files that already existed, and were modified, need re-scanning for dependencies upon project
	// restart. This is done via saving this special file
	String fscache =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join("filesystem_update4");
	Ref<FileAccess> f = FileAccess::open(fscache, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(
		f.is_null(), "Cannot create file '" + fscache + "'. Check user write permissions.");
	for (const String& E : late_update_files) {
		f->store_line(E);
	}
}

Vector<String> EditorFileSystem::_get_dependencies(const String& p_path)
{
	// Avoid error spam on first opening of a not yet imported project by treating the following
	// situation as a benign one, not letting the file open error happen: the resource is of an
	// importable type but it has not been imported yet.
	if (ResourceFormatImporter::get_singleton()->recognize_path(p_path)) {
		const String& internal_path =
			ResourceFormatImporter::get_singleton()->get_internal_resource_path(p_path);
		if (!internal_path.is_empty() &&
			!FileAccess::exists(
				internal_path)) { // If path is empty (error), keep the code flow to the error.
			return Vector<String>();
		}
	}

	List<String> deps;
	ResourceLoader::get_dependencies(p_path, &deps);

	Vector<String> ret;
	for (const String& E : deps) {
		ret.push_back(E);
	}

	return ret;
}

void EditorFileSystem::_update_file_icon_path(EditorFileSystemDirectory::FileInfo* file_info)
{
	String icon_path;
	if (file_info->resource_script_class != StringName()) {
		icon_path = EditorNode::get_editor_data().script_class_get_icon_path(
			file_info->resource_script_class);
	}
	else if (file_info->class_info.icon_path.is_empty() && !file_info->deps.is_empty()) {
		const String& script_dep = file_info->deps[0]; // Assuming the first dependency is a script.
		const String& script_path =
			script_dep.contains("::") ? script_dep.get_slice("::", 2) : script_dep;
		if (!script_path.is_empty()) {
			String* cached = file_icon_cache.getptr(script_path);
			if (cached) {
				icon_path = *cached;
			}
			else {
				file_icon_cache.insert(script_path, icon_path);
			}
		}
	}
	if (icon_path.is_empty() && !file_info->type.is_empty()) {
		Ref<Texture2D> icon = EditorNode::get_singleton()->get_class_icon(file_info->type);
		if (icon.is_valid()) {
			icon_path = icon->get_path();
		}
	}

	file_info->class_info.icon_path = icon_path;
}

void EditorFileSystem::_update_files_icon_path(EditorFileSystemDirectory* edp)
{
	if (!edp) {
		edp = filesystem;
		file_icon_cache.clear();
	}
	for (EditorFileSystemDirectory* sub_dir : edp->subdirs) {
		_update_files_icon_path(sub_dir);
	}
	for (EditorFileSystemDirectory::FileInfo* fi : edp->files) {
		_update_file_icon_path(fi);
	}
}

void EditorFileSystem::_process_update_pending()
{
	_update_script_classes();
	// Parse documentation second, as it requires the class names to be loaded
	// because _update_script_documentation loads the scripts completely.
	if (!EditorNode::is_cmdline_mode()) {
		_update_script_documentation();
		_update_pending_scene_groups();
	}
}

void EditorFileSystem::_queue_update_script_class(
	const String& p_path, const ScriptClassInfoUpdate& p_script_update)
{
	MutexLock update_script_lock(update_script_mutex);

	update_script_paths.insert(p_path, p_script_update);
	update_script_paths_documentation.insert(p_path);
}

void EditorFileSystem::_update_pending_scene_groups()
{
	if (!FileAccess::exists(ProjectSettings::get_singleton()->get_scene_groups_cache_path())) {
		_get_all_scenes(get_filesystem(), update_scene_paths);
		_update_scene_groups();
	}
	else if (!update_scene_paths.is_empty()) {
		_update_scene_groups();
	}
}

void EditorFileSystem::_queue_update_scene_groups(const String& p_path)
{
	MutexLock update_scene_lock(update_scene_mutex);
	update_scene_paths.insert(p_path);
}

void EditorFileSystem::_get_all_scenes(EditorFileSystemDirectory* p_dir, HashSet<String>& r_list)
{
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		if (p_dir->get_file_type(i) == SNAME("PackedScene")) {
			r_list.insert(p_dir->get_file_path(i));
		}
	}

	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_get_all_scenes(p_dir->get_subdir(i), r_list);
	}
}

void EditorFileSystem::update_file(const String& p_file)
{
	ERR_FAIL_COND(p_file.is_empty());
	update_files({p_file});
}

HashSet<String> EditorFileSystem::get_valid_extensions() const
{
	return HashSet<String>(valid_extensions);
}

void EditorFileSystem::_find_group_files(EditorFileSystemDirectory* efd,
	HashMap<String, Vector<String>>& group_files, HashSet<String>& groups_to_reimport)
{
	int fc = efd->files.size();
	const EditorFileSystemDirectory::FileInfo* const* files = efd->files.ptr();
	for (int i = 0; i < fc; i++) {
		if (groups_to_reimport.has(files[i]->import_group_file)) {
			if (!group_files.has(files[i]->import_group_file)) {
				group_files[files[i]->import_group_file] = Vector<String>();
			}
			group_files[files[i]->import_group_file].push_back(efd->get_file_path(i));
		}
	}

	for (int i = 0; i < efd->get_subdir_count(); i++) {
		_find_group_files(efd->get_subdir(i), group_files, groups_to_reimport);
	}
}

bool EditorFileSystem::_copy_directory(
	const String& p_from, const String& p_to, HashMap<String, String>* p_files)
{
	Ref<DirAccess> old_dir = DirAccess::open(p_from);
	ERR_FAIL_COND_V(old_dir.is_null(), false);

	Error err = make_dir_recursive(p_to);
	if (err != OK && err != ERR_ALREADY_EXISTS) {
		return false;
	}

	bool success = true;
	old_dir->set_include_navigational(false);
	old_dir->list_dir_begin();

	for (String F = old_dir->_get_next(); !F.is_empty(); F = old_dir->_get_next()) {
		if (old_dir->current_is_dir()) {
			success = _copy_directory(p_from.path_join(F), p_to.path_join(F), p_files) && success;
		}
		else if (F.get_extension() != "import" && F.get_extension() != "uid") {
			(*p_files)[p_from.path_join(F)] = p_to.path_join(F);
		}
	}
	return success;
}

Error EditorFileSystem::_resource_import(const String& p_path)
{
	Vector<String> files;
	files.push_back(p_path);

	singleton->update_file(p_path);
	singleton->reimport_files(files);

	return OK;
}

bool EditorFileSystem::_should_skip_directory(const String& p_path)
{
	String project_data_path = ProjectSettings::get_singleton()->get_project_data_path();
	if (p_path == project_data_path || p_path.begins_with(project_data_path + "/")) {
		return true;
	}

	if (FileAccess::exists(p_path.path_join("project.godot"))) {
		// Skip if another project inside this.
		if (EditorFileSystem::get_singleton() == nullptr ||
			EditorFileSystem::get_singleton()->first_scan) {
			WARN_PRINT_ONCE(vformat(
				"Detected another project.godot at %s. The folder will be ignored.", p_path));
		}
		return true;
	}

	if (FileAccess::exists(p_path.path_join(".gdignore"))) {
		// Skip if a `.gdignore` file is inside this.
		return true;
	}

	return false;
}

bool EditorFileSystem::is_group_file(const String& p_path) const
{
	return group_file_cache.has(p_path);
}

void EditorFileSystem::move_group_file(const String& p_path, const String& p_new_path)
{
	if (get_filesystem()) {
		_move_group_files(get_filesystem(), p_path, p_new_path);
		if (group_file_cache.has(p_path)) {
			group_file_cache.erase(p_path);
			group_file_cache.insert(p_new_path);
		}
	}
}

Error EditorFileSystem::copy_file(const String& p_from, const String& p_to)
{
	Error err = _copy_file(p_from, p_to);
	if (err != OK) {
		return err;
	}

	EditorFileSystemDirectory* parent = get_filesystem_path(p_to.get_base_dir());
	ERR_FAIL_NULL_V(parent, ERR_FILE_NOT_FOUND);

	ScanProgress sp;
	_scan_fs_changes(parent, sp, false);

	_queue_refresh_filesystem();
	return OK;
}

ResourceUID::ID EditorFileSystem::_resource_saver_get_resource_id_for_path(
	const String& p_path, bool p_generate)
{
	if (!p_path.is_resource_file() ||
		p_path.begins_with(ProjectSettings::get_singleton()->get_project_data_path())) {
		// Saved externally (configuration file) or internal file, do not assign an ID.
		return ResourceUID::INVALID_ID;
	}

	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;

	if (!singleton->_find_file(p_path, &fs, cpos)) {
		// Fallback to ResourceLoader if filesystem cache fails (can happen during scanning etc.).
		ResourceUID::ID fallback = ResourceLoader::get_resource_uid(p_path);
		if (fallback != ResourceUID::INVALID_ID) {
			return fallback;
		}

		if (p_generate) {
			return ResourceUID::get_singleton()->create_id_for_path(
				p_path); // Just create a new one, we will be notified of save anyway and fetch the
						 // right UID at that time, to keep things simple.
		}
		else {
			return ResourceUID::INVALID_ID;
		}
	}
	else if (fs->files[cpos]->uid != ResourceUID::INVALID_ID) {
		return fs->files[cpos]->uid;
	}
	else if (p_generate) {
		return ResourceUID::get_singleton()->create_id_for_path(
			p_path); // Just create a new one, we will be notified of save anyway and fetch the
					 // right UID at that time, to keep things simple.
	}
	else {
		return ResourceUID::INVALID_ID;
	}
}

bool EditorFileSystem::_can_import_file(const String& p_file)
{
	for (const String& F : import_extensions) {
		if (p_file.right(F.length()).nocasecmp_to(F) == 0) {
			return true;
		}
	}

	return false;
}

void EditorFileSystem::add_import_format_support_query(
	Ref<EditorFileSystemImportFormatSupportQuery> p_query)
{
	ERR_FAIL_COND(import_support_queries.has(p_query));
	import_support_queries.push_back(p_query);
}

void EditorFileSystem::remove_import_format_support_query(
	Ref<EditorFileSystemImportFormatSupportQuery> p_query)
{
	import_support_queries.erase(p_query);
}

EditorFileSystem::~EditorFileSystem()
{
	memdelete(filesystem);
	filesystem = nullptr;
	ResourceSaver::set_get_resource_id_for_path(nullptr);
}

void EditorFileSystemImportFormatSupportQuery::_bind_methods() {}

bool EditorFileSystem::_scan_extensions() { return true; }


