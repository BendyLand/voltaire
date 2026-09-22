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

	String f = ProjectSettings::localize_path(p_file);
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

	String f = ProjectSettings::localize_path(p_path);

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

void EditorFileSystem::_queue_update_script_class(
	const String& p_path, const ScriptClassInfoUpdate& p_script_update)
{
	MutexLock update_script_lock(update_script_mutex);

	update_script_paths.insert(p_path, p_script_update);
	update_script_paths_documentation.insert(p_path);
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

bool EditorFileSystem::_should_skip_directory(const String& p_path)
{
	String project_data_path = ProjectSettings::get_project_data_path();
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

ResourceUID::ID EditorFileSystem::_resource_saver_get_resource_id_for_path(
	const String& p_path, bool p_generate)
{
	if (!p_path.is_resource_file() ||
		p_path.begins_with(ProjectSettings::get_project_data_path())) {
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


bool EditorFileSystem::_scan_extensions() { return true; }


