/**************************************************************************/
/*  project_settings.cpp                                                  */
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

#include "core/input/input_map.h"
#include "core/io/compression.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_pack.h"
#include "core/io/marshalls.h"
#include "core/io/resource_uid.h"
#include "core/os/os.h"
#include "core/templates/rb_set.h"
#include "core/version.h"
#include "project_settings.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#endif									 // TOOLS_ENABLED

ProjectSettings* ProjectSettings::get_singleton() { return singleton; }

String ProjectSettings::get_project_data_dir_name() const { return project_data_dir_name; }

String ProjectSettings::get_project_data_path() const
{
	return "res://" + get_project_data_dir_name();
}

String ProjectSettings::get_resource_path() const { return resource_path; }

// This returns paths like "res://.godot/imported".
String ProjectSettings::get_imported_files_path() const
{
	return get_project_data_path().path_join("imported");
}

#ifdef TOOLS_ENABLED
// Returns the features that a project must have when opened with this build of Godot.
// This is used by the project manager to provide the initial_settings for config/features.
const Vector<String> ProjectSettings::get_required_features()
{
	Vector<String> features;
	features.append(VLTR_VERSION_BRANCH);
#ifdef REAL_T_IS_DOUBLE
	features.append("Double Precision");
#endif
	return features;
}

// Returns the features supported by this build of Godot. Includes all required features.
const Vector<String> ProjectSettings::_get_supported_features()
{
	Vector<String> features = get_required_features();

#ifdef LIBGODOT_ENABLED
	features.append("LibGodot");
#endif

#ifdef MODULE_MONO_ENABLED
	features.append("C#");
#endif
	// Allow pinning to a specific patch number or build type by marking
	// them as supported. They're only used if the user adds them manually.
	features.append(VLTR_VERSION_BRANCH "." _MKSTR(VLTR_VERSION_PATCH));
	features.append(VLTR_VERSION_FULL_CONFIG);
	features.append(VLTR_VERSION_FULL_BUILD);

#ifdef RD_ENABLED
	features.append("Forward Plus");
	features.append("Mobile");
#endif

#ifdef GLES3_ENABLED
	features.append("GL Compatibility");
#endif
	return features;
}

// Returns the features that this project needs but this build of Godot lacks.
const Vector<String> ProjectSettings::get_unsupported_features(
	const Vector<String>& p_project_features)
{
	Vector<String> unsupported_features;
	Vector<String> supported_features = singleton->_get_supported_features();
	for (int i = 0; i < p_project_features.size(); i++) {
		if (!supported_features.has(p_project_features[i])) {
			// Temporary compatibility code to ease upgrade to 4.0 beta 2+.
			if (p_project_features[i].begins_with("Vulkan")) {
				continue;
			}
			unsupported_features.append(p_project_features[i]);
		}
	}
	unsupported_features.sort();
	return unsupported_features;
}

// Returns the features that both this project has and this build of Godot has, ensuring required
// features exist.
const Vector<String> ProjectSettings::_trim_to_supported_features(
	const Vector<String>& p_project_features)
{
	// Remove unsupported features if present.
	Vector<String> features = Vector<String>(p_project_features);
	Vector<String> supported_features = _get_supported_features();
	for (int i = p_project_features.size() - 1; i > -1; i--) {
		if (!supported_features.has(p_project_features[i])) {
			features.remove_at(i);
		}
	}
	// Add required features if not present.
	Vector<String> required_features = get_required_features();
	for (int i = 0; i < required_features.size(); i++) {
		if (!features.has(required_features[i])) {
			features.append(required_features[i]);
		}
	}
	features.sort();
	return features;
}
#endif // TOOLS_ENABLED

String ProjectSettings::localize_path(const String& p_path) const
{
	String path = p_path.simplify_path();

	if (resource_path.is_empty() || (path.is_absolute_path() && !path.begins_with(resource_path))) {
		return path;
	}

	// Check if we have a special path (like res://) or a protocol identifier.
	int p = path.find("://");
	bool found = false;
	if (p > 0) {
		found = true;
		for (int i = 0; i < p; i++) {
			if (!is_ascii_alphanumeric_char(path[i])) {
				found = false;
				break;
			}
		}
	}
	if (found) {
		return path;
	}

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	if (dir->change_dir(path) == OK) {
		String cwd = dir->get_current_dir();
		cwd = cwd.replace_char('\\', '/');

		// Ensure that we end with a '/'.
		// This is important to ensure that we do not wrongly localize the resource path
		// in an absolute path that just happens to contain this string but points to a
		// different folder (e.g. "/my/project" as resource_path would be contained in
		// "/my/project_data", even though the latter is not part of res://.
		// `path_join("")` is an easy way to ensure we have a trailing '/'.
		const String res_path = resource_path.path_join("");

		// DirAccess::get_current_dir() is not guaranteed to return a path that with a trailing '/',
		// so we must make sure we have it as well in order to compare with 'res_path'.
		cwd = cwd.path_join("");

		if (!cwd.begins_with(res_path)) {
			return path;
		}

		return cwd.replace_first(res_path, "res://");
	}
	else {
		int sep = path.rfind_char('/');
		if (sep == -1) {
			return "res://" + path;
		}

		String parent = path.substr(0, sep);

		String plocal = localize_path(parent);
		if (plocal.is_empty()) {
			return "";
		}
		// Only strip the starting '/' from 'path' if its parent ('plocal') ends with '/'
		if (plocal[plocal.length() - 1] == '/') {
			sep += 1;
		}
		return plocal + path.substr(sep);
	}
}

void ProjectSettings::add_hidden_prefix(const String& p_prefix)
{
	hidden_prefixes.push_back(p_prefix);
}

String ProjectSettings::globalize_path(const String& p_path) const
{
	if (p_path.begins_with("res://")) {
		if (!resource_path.is_empty()) {
			return p_path.replace("res:/", resource_path);
		}
		return p_path.replace("res://", "");
	}
	else if (p_path.begins_with("uid://")) {
		const String path = ResourceUID::uid_to_path(p_path);
		if (!resource_path.is_empty()) {
			return path.replace("res:/", resource_path);
		}
		return path.replace("res://", "");
	}
	else if (p_path.begins_with("user://")) {
		String data_dir = OS::get_singleton()->get_user_data_dir();
		if (!data_dir.is_empty()) {
			return p_path.replace("user:/", data_dir);
		}
		return p_path.replace("user://", "");
	}

	return p_path;
}

struct _VCSort
{
	String name;
	int order = 0;
	uint32_t flags = 0;

	bool operator<(const _VCSort& p_vcs) const
	{
		return order == p_vcs.order ? name < p_vcs.name : order < p_vcs.order;
	}
};


void ProjectSettings::_queue_changed(const StringName& p_name)
{
	changed_settings.insert(p_name);
}

void ProjectSettings::_emit_changed()
{
	if (!is_changed) {
		return;
	}
	is_changed = false;

	// Clear the changed settings after emitting the signal
	changed_settings.clear();
}

bool ProjectSettings::load_resource_pack(const String& p_pack, bool p_replace_files, int p_offset)
{
	return ProjectSettings::_load_resource_pack(p_pack, p_replace_files, p_offset, false);
}

bool ProjectSettings::_load_resource_pack(
	const String& p_pack, bool p_replace_files, int p_offset, bool p_main_pack)
{
	if (PackedData::get_singleton()->is_disabled()) {
		return false;
	}

	if (p_pack == "res://") {
		// Loading the resource directory as a pack source is reserved for internal use only.
		return false;
	}

	if (!p_main_pack && !using_datapack && !OS::get_singleton()->get_resource_dir().is_empty()) {
		// Add the project's resource file system to PackedData so directory access keeps working
		// when the game is running without a main pack, like in the editor or on Android.
		PackedData::get_singleton()->add_pack_source(memnew(PackedSourceDirectory));
		PackedData::get_singleton()->add_pack("res://", false, 0);
		DirAccess::make_default<DirAccessPack>(DirAccess::ACCESS_RESOURCES);
		using_datapack = true;
	}

	bool ok = PackedData::get_singleton()->add_pack(p_pack, p_replace_files, p_offset) == OK;
	if (!ok) {
		return false;
	}

	if (project_loaded) {
		// This pack may have declared new global classes (make sure they are picked up).
		refresh_global_class_list();

		// This pack may have defined new UIDs, make sure they are cached.
		ResourceUID::get_singleton()->load_from_cache(false);
	}

	// If the data pack was found, all directory access will be from here.
	if (!using_datapack) {
		DirAccess::make_default<DirAccessPack>(DirAccess::ACCESS_RESOURCES);
		using_datapack = true;
	}

	return true;
}

/*
 * This method is responsible for loading a project.godot file and/or data file
 * using the following merit order:
 *  - If using NetworkClient, try to lookup project file or fail.
 *  - If --main-pack was passed by the user (`p_main_pack`), load it or fail.
 *  - Search for project PCKs automatically. For each step we try loading a potential
 *    PCK, and if it doesn't work, we proceed to the next step. If any step succeeds,
 *    we try loading the project settings, and abort if it fails. Steps:
 *    o Bundled PCK in the executable.
 *    o [macOS only] PCK with same basename as the binary in the .app resource dir.
 *    o PCK with same basename as the binary in the binary's directory. We handle both
 *      changing the extension to '.pck' (e.g. 'win_game.exe' -> 'win_game.pck') and
 *      appending '.pck' to the binary name (e.g. 'linux_game' -> 'linux_game.pck').
 *    o PCK with the same basename as the binary in the current working directory.
 *      Same as above for the two possible PCK file names.
 *  - On Android, look for 'assets.sparsepck' and try loading it, if it doesn't work,
 *    proceed to the next step.
 *  - On relevant platforms (Android/iOS), lookup project file in OS resource path.
 *    If found, load it or fail.
 *  - Lookup project file in passed `p_path` (--path passed by the user), i.e. we
 *    are running from source code.
 *    If not found and `p_upwards` is true, look for project files in parent folders
 *    up to the system root (used to run a game from command line while in a subfolder).
 *    If a project file is found, load it or fail.
 *    If nothing was found, error out.
 */
Error ProjectSettings::_setup(
	const String& p_path, const String& p_main_pack, bool p_upwards, bool p_ignore_override)
{
	if (!OS::get_singleton()->get_resource_dir().is_empty()) {
		// OS will call ProjectSettings->get_resource_path which will be empty if not overridden!
		// If the OS would rather use a specific location, then it will not be empty.
		resource_path = OS::get_singleton()->get_resource_dir().replace_char('\\', '/');
		if (!resource_path.is_empty() && resource_path[resource_path.length() - 1] == '/') {
			resource_path = resource_path.substr(0, resource_path.length() - 1); // Chop end.
		}
	}

	// Attempt with a user-defined main pack first

	if (!p_main_pack.is_empty()) {
		bool ok = _load_resource_pack(p_main_pack, false, 0, true);
		Error err = _load_settings_text_or_binary("res://project.godot", "res://project.binary");
		return err;
	}

	String exec_path = OS::get_singleton()->get_executable_path();

	if (!exec_path.is_empty()) {
		// We do several tests sequentially until one succeeds to find a PCK,
		// and if so, we attempt loading it at the end.

		// Attempt with PCK bundled into executable.
		bool found = _load_resource_pack(exec_path, false, 0, true);

		// Attempt with exec_name.pck.
		// (This is the usual case when distributing a Godot game.)
		String exec_dir = exec_path.get_base_dir();
		String exec_filename = exec_path.get_file();
		String exec_basename = exec_filename.get_basename();

		// Based on the OS, it can be the exec path + '.pck' (Linux w/o extension, macOS in .app
		// bundle) or the exec path's basename + '.pck' (Windows). We need to test both
		// possibilities as extensions for Linux binaries are optional (so both 'mygame.bin' and
		// 'mygame' should be able to find 'mygame.pck').

#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
		if (!found) {
			// Attempt to load PCK from macOS .app bundle resources.
			found = _load_resource_pack(OS::get_singleton()->get_bundle_resource_dir().path_join(
											exec_basename + ".pck"),
						false, 0, true) ||
					_load_resource_pack(OS::get_singleton()->get_bundle_resource_dir().path_join(
											exec_filename + ".pck"),
						false, 0, true);
		}
#endif

		if (!found) {
			// Try to load data pack at the location of the executable.
			// As mentioned above, we have two potential names to attempt.
			found =
				_load_resource_pack(exec_dir.path_join(exec_basename + ".pck"), false, 0, true) ||
				_load_resource_pack(exec_dir.path_join(exec_filename + ".pck"), false, 0, true);
		}

		if (!found) {
			// If we couldn't find them next to the executable, we attempt
			// the current working directory. Same story, two tests.
			found = _load_resource_pack(exec_basename + ".pck", false, 0, true) ||
					_load_resource_pack(exec_filename + ".pck", false, 0, true);
		}

		// If we opened our package, try and load our project.
		if (found) {
			Error err = _load_settings_text_or_binary("res://project.godot", "res://project.binary");
			return err;
		}
	}

#ifdef ANDROID_ENABLED
	// Attempt to load sparse PCK assets.
	_load_resource_pack("res://assets.sparsepck", false, 0, true);
#endif

	// Try to use the filesystem for files, according to OS.
	// (Only Android -when reading from PCK-.)

	if (!OS::get_singleton()->get_resource_dir().is_empty()) {
		Error err = _load_settings_text_or_binary("res://project.godot", "res://project.binary");
		return err;
	}

#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	// Attempt to load project file from macOS .app bundle resources.
	resource_path = OS::get_singleton()->get_bundle_resource_dir();
	if (!resource_path.is_empty()) {
		if (resource_path[resource_path.length() - 1] == '/') {
			resource_path = resource_path.substr(0, resource_path.length() - 1); // Chop end.
		}
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		ERR_FAIL_COND_V_MSG(d.is_null(), ERR_CANT_CREATE,
			vformat("Cannot create DirAccess for path '%s'.", resource_path));
		d->change_dir(resource_path);

		Error err;

		err = _load_settings_text_or_binary(
			resource_path.path_join("project.godot"), resource_path.path_join("project.binary"));
		if (err == OK && !p_ignore_override) {
			// Optional, we don't mind if it fails.
#ifdef OVERRIDE_ENABLED
			bool disable_override =
				GLOBAL_GET("application/config/disable_project_settings_override");
			if (!disable_override) {
				_load_settings_text(resource_path.path_join("override.cfg"));
			}
#endif // OVERRIDE_ENABLED
			return err;
		}
	}
#endif // MACOS_ENABLED

	// Nothing was found, try to find a project file in provided path (`p_path`)
	// or, if requested (`p_upwards`) in parent directories.

	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	d->change_dir(p_path);

	String current_dir = d->get_current_dir();
	bool found = false;
	Error err;

	while (true) {
		// Set the resource path early so things can be resolved when loading.
		resource_path = current_dir;
		resource_path =
			resource_path.replace_char('\\', '/'); // Windows path to Unix path just in case.
		err = _load_settings_text_or_binary(
			current_dir.path_join("project.godot"), current_dir.path_join("project.binary"));
		if (err == OK) {
			found = true;
			break;
		}

#if defined(OVERRIDE_PATH_ENABLED)
		if (p_upwards) {
			// Try to load settings ascending through parent directories
			d->change_dir("..");
			if (d->get_current_dir() == current_dir) {
				break; // not doing anything useful
			}
			current_dir = d->get_current_dir();
		}
		else {
#else
		{
#endif
			break;
		}
	}

	if (!found) {
		return err;
	}

	if (resource_path.length() && resource_path[resource_path.length() - 1] == '/') {
		resource_path = resource_path.substr(0, resource_path.length() - 1); // Chop end.
	}

	return OK;
}

Error ProjectSettings::setup(
	const String& p_path, const String& p_main_pack, bool p_upwards, bool p_ignore_override)
{
	Error err = _setup(p_path, p_main_pack, p_upwards, p_ignore_override);

	// Updating the default value after the project settings have loaded.
	project_data_dir_name = PROJECT_DATA_DIR_NAME_SUFFIX;

	// Using GLOBAL_GET on every block for compressing can be slow, so assigning here.
	load_scene_groups_cache();

	project_loaded = err == OK;
	return err;
}

Error ProjectSettings::_load_settings_binary(const String& p_path)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (err != OK) {
		return err;
	}

	uint8_t hdr[4];
	f->get_buffer(hdr, 4);
	ERR_FAIL_COND_V_MSG((hdr[0] != 'E' || hdr[1] != 'C' || hdr[2] != 'F' || hdr[3] != 'G'),
		ERR_FILE_CORRUPT, "Corrupted header in binary project.binary (not ECFG).");

	uint32_t count = f->get_32();

	for (uint32_t i = 0; i < count; i++) {
		uint32_t slen = f->get_32();
		CharString cs;
		cs.resize_uninitialized(slen + 1);
		cs[slen] = 0;
		f->get_buffer((uint8_t*)cs.ptr(), slen);
		String key = String::utf8(cs.ptr(), slen);

		uint32_t vlen = f->get_32();
		Vector<uint8_t> d;
		d.resize(vlen);
		f->get_buffer(d.ptrw(), vlen);
	}

	return OK;
}

Error ProjectSettings::_load_settings_text(const String& p_path)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (f.is_null()) {
		// FIXME: Above 'err' error code is ERR_FILE_CANT_OPEN if the file is missing
		// This needs to be streamlined if we want decent error reporting
		return ERR_FILE_NOT_FOUND;
	}

	String assign;
	int lines = 0;
	String error_text;
	String section;
	int config_version = 0;

	while (true) {
		if (err == ERR_FILE_EOF) {
			// If we're loading a project.godot from source code, we can operate some
			// ProjectSettings conversions if need be.
			_convert_to_last_version(config_version);
			last_save_time =
				FileAccess::get_modified_time(get_resource_path().path_join("project.godot"));
			return OK;
		}
	}
}

Error ProjectSettings::_load_settings_text_or_binary(
	const String& p_text_path, const String& p_bin_path)
{
	// Attempt first to load the binary project.godot file.
	Error err = _load_settings_binary(p_bin_path);
	if (err == OK) {
		return OK;
	}

	// Fallback to text-based project.godot file if binary was not found.
	err = _load_settings_text(p_text_path);
	if (err == OK) {
		return OK;
	}
	return err;
}

Error ProjectSettings::load_custom(const String& p_path)
{
	if (p_path.ends_with(".binary")) {
		return _load_settings_binary(p_path);
	}
	return _load_settings_text(p_path);
}

Error ProjectSettings::save()
{
	Error error = save_custom(get_resource_path().path_join("project.godot"));
	if (error == OK) {
		last_save_time =
			FileAccess::get_modified_time(get_resource_path().path_join("project.godot"));
	}
	return error;
}

Error ProjectSettings::_save_settings_binary(const String& p_file,
	const RBMap<String, List<String>>& p_props, const CustomMap& p_custom,
	const String& p_custom_features)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

	uint8_t hdr[4] = {'E', 'C', 'F', 'G'};
	file->store_buffer(hdr, 4);

	int count = 0;

	for (const KeyValue<String, List<String>>& E : p_props) {
		count += E.value.size();
	}

	if (!p_custom_features.is_empty()) {
		// Store how many properties are saved, add one for custom features, which must always go
		// first.
		file->store_32(uint32_t(count + 1));

		int len;
		Vector<uint8_t> buff;
		buff.resize(len);
		file->store_32(uint32_t(len));
		file->store_buffer(buff.ptr(), buff.size());

	}
	else {
		// Store how many properties are saved.
		file->store_32(uint32_t(count));
	}

	for (const KeyValue<String, List<String>>& E : p_props) {
		for (const String& key : E.value) {
			String k = key;
			if (!E.key.is_empty()) {
				k = E.key + "/" + k;
			}
			file->store_pascal_string(k);
			int len;
			Vector<uint8_t> buff;
			buff.resize(len);
			file->store_32(uint32_t(len));
			file->store_buffer(buff.ptr(), buff.size());
		}
	}

	return OK;
}

Error ProjectSettings::_save_settings_text(const String& p_file,
	const RBMap<String, List<String>>& p_props, const CustomMap& p_custom,
	const String& p_custom_features)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

	file->store_line("; Engine configuration file.");
	file->store_line("; It's best edited using the editor UI and not directly,");
	file->store_line("; since the parameters that go here are not all obvious.");
	file->store_line(";");
	file->store_line("; Format:");
	file->store_line(";   [section] ; section goes between []");
	file->store_line(";   param=value ; assign values to parameters");
	file->store_line("");

	file->store_string("config_version=" + itos(CONFIG_VERSION) + "\n");
	if (!p_custom_features.is_empty()) {
		file->store_string("custom_features=\"" + p_custom_features + "\"\n");
	}
	file->store_string("\n");

	for (const KeyValue<String, List<String>>& E : p_props) {
		if (E.key != p_props.begin()->key) {
			file->store_string("\n");
		}

		if (!E.key.is_empty()) {
			file->store_string("[" + E.key + "]\n\n");
		}
		for (const String& F : E.value) {
			String key = F;
			if (!E.key.is_empty()) {
				key = E.key + "/" + key;
			}
			String vstr;
			file->store_string(F.property_name_encode() + "=" + vstr + "\n");
		}
	}

	return OK;
}

Error ProjectSettings::_save_custom_bnd(const String& p_file)
{ // add other params as dictionary and array?
	return save_custom(p_file);
}

#ifdef TOOLS_ENABLED
bool _csproj_exists(const String& p_root_dir)
{
	Ref<DirAccess> dir = DirAccess::open(p_root_dir);
	ERR_FAIL_COND_V(dir.is_null(), false);

	dir->list_dir_begin();
	String file_name = dir->_get_next();
	while (file_name != "") {
		if (!dir->current_is_dir() && file_name.get_extension() == "csproj") {
			return true;
		}
		file_name = dir->_get_next();
	}

	return false;
}
#endif // TOOLS_ENABLED

Error ProjectSettings::save_custom(const String& p_path, const CustomMap& p_custom,
	const Vector<String>& p_custom_features, bool p_merge_with_current)
{
	ERR_FAIL_COND_V_MSG(
		p_path.is_empty(), ERR_INVALID_PARAMETER, "Project settings save path cannot be empty.");


	RBSet<_VCSort> vclist;
	RBMap<String, List<String>> save_props;

	for (const _VCSort& E : vclist) {
		String category = E.name;
		String name = E.name;

		int div = category.find_char('/');

		if (div < 0) {
			category = "";
		}
		else {
			category = category.substr(0, div);
			name = name.substr(div + 1);
		}
		save_props[category].push_back(name);
	}

	String save_features;

	for (int i = 0; i < p_custom_features.size(); i++) {
		if (i > 0) {
			save_features += ",";
		}

		String f = p_custom_features[i].strip_edges().remove_char('\"');
		save_features += f;
	}

	if (p_path.ends_with(".godot") || p_path.ends_with("override.cfg")) {
		return _save_settings_text(p_path, save_props, p_custom, save_features);
	}
	else if (p_path.ends_with(".binary")) {
		return _save_settings_binary(p_path, save_props, p_custom, save_features);
	}
}

bool ProjectSettings::is_using_datapack() const { return using_datapack; }

bool ProjectSettings::is_project_loaded() const { return project_loaded; }

Vector<String> ProjectSettings::get_changed_settings() const
{
	Vector<String> arr;
	for (const StringName& setting : changed_settings) {
		arr.push_back(setting);
	}
	return arr;
}

bool ProjectSettings::check_changed_settings_in_group(const String& p_setting_prefix) const
{
	for (const StringName& setting : changed_settings) {
		if (String(setting).begins_with(p_setting_prefix)) {
			return true;
		}
	}
	return false;
}

String ProjectSettings::get_global_class_list_path() const
{
	return get_project_data_path().path_join("global_script_class_cache.cfg");
}

bool ProjectSettings::has_custom_feature(const String& p_feature) const
{
	return custom_features.has(p_feature);
}

const HashMap<StringName, ProjectSettings::AutoloadInfo>& ProjectSettings::get_autoload_list() const
{
	return autoloads;
}

void ProjectSettings::add_autoload(const AutoloadInfo& p_autoload, bool p_front_insert)
{
	ERR_FAIL_COND_MSG(p_autoload.name == StringName(), "Trying to add autoload with no name.");
	if (p_front_insert) {
		if (autoloads.has(p_autoload.name)) {
			autoloads.erase(p_autoload.name);
		}
		autoloads.insert(p_autoload.name, p_autoload, true);
	}
	else {
		autoloads[p_autoload.name] = p_autoload;
	}
}

void ProjectSettings::remove_autoload(const StringName& p_autoload)
{
	ERR_FAIL_COND_MSG(!autoloads.has(p_autoload), "Trying to remove non-existent autoload.");
	autoloads.erase(p_autoload);
}

bool ProjectSettings::has_autoload(const StringName& p_autoload) const
{
	return autoloads.has(p_autoload);
}

ProjectSettings::AutoloadInfo ProjectSettings::get_autoload(const StringName& p_name) const
{
	ERR_FAIL_COND_V_MSG(
		!autoloads.has(p_name), AutoloadInfo(), "Trying to get non-existent autoload.");
	return autoloads[p_name];
}

void ProjectSettings::fix_autoload_paths()
{
	for (KeyValue<StringName, AutoloadInfo>& kv : autoloads) {
		kv.value.path = ResourceUID::ensure_path(kv.value.path);
	}
}

const HashMap<StringName, String>& ProjectSettings::get_global_groups_list() const
{
	return global_groups;
}

void ProjectSettings::add_global_group(const StringName& p_name, const String& p_description)
{
	ERR_FAIL_COND_MSG(p_name == StringName(), "Trying to add global group with no name.");
	global_groups[p_name] = p_description;
}

void ProjectSettings::remove_global_group(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(!global_groups.has(p_name), "Trying to remove non-existent global group.");
	global_groups.erase(p_name);
}

bool ProjectSettings::has_global_group(const StringName& p_name) const
{
	return global_groups.has(p_name);
}

void ProjectSettings::remove_scene_groups_cache(const StringName& p_path)
{
	scene_groups_cache.erase(p_path);
}

void ProjectSettings::add_scene_groups_cache(
	const StringName& p_path, const HashSet<StringName>& p_cache)
{
	scene_groups_cache[p_path] = p_cache;
}

void ProjectSettings::save_scene_groups_cache()
{
	Ref<ConfigFile> cf;
	cf.instantiate();
	for (const KeyValue<StringName, HashSet<StringName>>& E : scene_groups_cache) {
		if (E.value.is_empty()) {
			continue;
		}
	}
	cf->save(get_scene_groups_cache_path());
}

String ProjectSettings::get_scene_groups_cache_path() const
{
	return get_project_data_path().path_join("scene_groups_cache.cfg");
}

const HashMap<StringName, HashSet<StringName>>& ProjectSettings::get_scene_groups_cache() const
{
	return scene_groups_cache;
}

bool ProjectSettings::has_editor_setting_override(const String& p_setting) const
{
	return has_setting(EDITOR_SETTING_OVERRIDE_PREFIX + p_setting);
}

