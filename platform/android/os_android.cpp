/**************************************************************************/
/*  os_android.cpp                                                        */
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

#include "servers/display/display_server.h"


#ifndef TOOLS_ENABLED
#include "servers/display/display_server.h"
#endif

#include "core/os/os.h"
#include "core/io/net_socket.h"
#define WANT_SCENE_TREE_H
#include "servers/display/display_server.h"

#include <dlfcn.h>
#include <sys/system_properties.h>

const char *OS_Android::ANDROID_EXEC_PATH = "apk";

String _remove_symlink(const String &dir) {
	// Workaround for Android 6.0+ using a symlink.
	// Save the current directory.
	char current_dir_name[2048];
	getcwd(current_dir_name, 2048);
	// Change directory to the external data directory.
	chdir(dir.utf8().get_data());
	// Get the actual directory without the potential symlink.
	char dir_name_without_symlink[2048];
	getcwd(dir_name_without_symlink, 2048);
	// Convert back to a String.
	String dir_without_symlink(dir_name_without_symlink);
	// Restore original current directory.
	chdir(current_dir_name);
	return dir_without_symlink;
}


void OS_Android::alert(const String &p_alert, const String &p_title) {
	ERR_FAIL_NULL(data->godot_java);
	data->godot_java->alert(p_alert, p_title);
}

void OS_Android::initialize_core() {
	OS_Unix::initialize_core();

#ifdef TOOLS_ENABLED
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
#else // TOOLS_ENABLED
	FileAccess::make_default<FileAccessAndroid>(FileAccess::ACCESS_RESOURCES);
#if defined(OVERRIDE_PATH_ENABLED)
	if (data->use_apk_expansion) {
		FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	}
#endif // defined(OVERRIDE_PATH_ENABLED)
#endif // TOOLS_ENABLED
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessFilesystemJAndroid>(FileAccess::ACCESS_FILESYSTEM);

#ifdef TOOLS_ENABLED
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
#else // TOOLS_ENABLED
	DirAccess::make_default<DirAccessJAndroid>(DirAccess::ACCESS_RESOURCES);
#if defined(OVERRIDE_PATH_ENABLED)
	if (data->use_apk_expansion) {
		DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	}
#endif // defined(OVERRIDE_PATH_ENABLED)
#endif // TOOLS_ENABLED
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessJAndroid>(DirAccess::ACCESS_FILESYSTEM);

	NetSocketAndroid::make_default();
}

void OS_Android::initialize() {
	initialize_core();
}

void OS_Android::initialize_joypads() {
	Input::set_fallback_mapping(data->godot_java->get_input_fallback_mapping());

	// This queries/updates the currently connected devices/joypads.
	data->godot_java->init_input_devices();
}

void OS_Android::delete_main_loop() {
	MainLoop::clear();
}

void OS_Android::finalize() {
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void OS_Android::finalize_core() {
	OS_Unix::finalize_core();
}

GodotJavaWrapper *OS_Android::get_godot_java() {
	return data ? data->godot_java : nullptr;
}

GodotIOJavaWrapper *OS_Android::get_godot_io_java() {
	return data ? data->godot_io_java : nullptr;
}

bool OS_Android::request_permission(const String &p_name) {
	return data->godot_java->request_permission(p_name);
}

bool OS_Android::request_permissions() {
	return data->godot_java->request_permissions();
}

Vector<String> OS_Android::get_granted_permissions() {
	return data->godot_java->get_granted_permissions();
}

bool OS_Android::copy_dynamic_library(const String &p_library_path, const String &p_target_dir, String *r_copy_path) {
	if (!FileAccess::exists(p_library_path)) {
		return false;
	}

	Ref<DirAccess> da_ref = DirAccess::create_for_path(p_library_path);
	if (da_ref.is_null()) {
		return false;
	}

	String copy_path = p_target_dir.path_join(p_library_path.get_file());
	bool copy_exists = FileAccess::exists(copy_path);
	if (copy_exists) {
		print_verbose("Deleting existing library copy " + copy_path);
		if (da_ref->remove(copy_path) != OK) {
			print_verbose("Unable to delete " + copy_path);
		}
	}

	print_verbose("Copying " + p_library_path + " to " + p_target_dir);
	Error create_dir_result = da_ref->make_dir_recursive(p_target_dir);
	if (create_dir_result == OK || create_dir_result == ERR_ALREADY_EXISTS) {
		copy_exists = da_ref->copy(p_library_path, copy_path) == OK;
	}

	if (copy_exists && r_copy_path != nullptr) {
		*r_copy_path = copy_path;
	}

	return copy_exists;
}

Error OS_Android::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	String path = p_path;
	bool so_file_exists = true;
	if (!FileAccess::exists(path)) {
		path = p_path.get_file();
		so_file_exists = false;
	}

	p_library_handle = dlopen(path.utf8().get_data(), RTLD_NOW);
	if (!p_library_handle && so_file_exists) {
		// The library (and its dependencies) may be on the sdcard and thus inaccessible.
		// Try to copy to the internal directory for access.
		const String dynamic_library_path = get_dynamic_libraries_path();

		if (p_data != nullptr && p_data->library_dependencies != nullptr && !p_data->library_dependencies->is_empty()) {
			// Copy the library dependencies
			print_verbose("Copying library dependencies..");
			for (const String &library_dependency_path : *p_data->library_dependencies) {
				String internal_library_dependency_path;
				if (!copy_dynamic_library(library_dependency_path, dynamic_library_path.path_join(library_dependency_path.get_base_dir()), &internal_library_dependency_path)) {
					ERR_PRINT(vformat("Unable to copy library dependency %s", library_dependency_path));
				} else {
					void *lib_dependency_handle = dlopen(internal_library_dependency_path.utf8().get_data(), RTLD_NOW);
					if (!lib_dependency_handle) {
						ERR_PRINT(vformat("Can't open dynamic library dependency: %s. Error: %s.", internal_library_dependency_path, dlerror()));
					}
				}
			}
		}

		String internal_path;
		print_verbose("Copying library " + p_path);
		const bool internal_so_file_exists = copy_dynamic_library(p_path, dynamic_library_path.path_join(p_path.get_base_dir()), &internal_path);

		if (internal_so_file_exists) {
			print_verbose("Opening library " + internal_path);
			p_library_handle = dlopen(internal_path.utf8().get_data(), RTLD_NOW);
			if (p_library_handle) {
				path = internal_path;
			}
		}
	}

	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

String OS_Android::get_name() {
	return "Android";
}

String OS_Android::get_system_property(const char *key) {
	String value;
	char value_str[PROP_VALUE_MAX];
	if (__system_property_get(key, value_str)) {
		value = String(value_str);
	}
	return value;
}

String OS_Android::get_distribution_name() {
	if (!get_system_property("ro.havoc.version").is_empty()) {
		return "Havoc OS";
	} else if (!get_system_property("org.pex.version").is_empty()) { // Putting before "Pixel Experience", because it's derivating from it.
		return "Pixel Extended";
	} else if (!get_system_property("org.pixelexperience.version").is_empty()) {
		return "Pixel Experience";
	} else if (!get_system_property("ro.potato.version").is_empty()) {
		return "POSP";
	} else if (!get_system_property("ro.xtended.version").is_empty()) {
		return "Project-Xtended";
	} else if (!get_system_property("org.evolution.version").is_empty()) {
		return "Evolution X";
	} else if (!get_system_property("ro.corvus.version").is_empty()) {
		return "Corvus-Q";
	} else if (!get_system_property("ro.pa.version").is_empty()) {
		return "Paranoid Android";
	} else if (!get_system_property("ro.crdroid.version").is_empty()) {
		return "crDroid Android";
	} else if (!get_system_property("ro.syberia.version").is_empty()) {
		return "Syberia Project";
	} else if (!get_system_property("ro.arrow.version").is_empty()) {
		return "ArrowOS";
	} else if (!get_system_property("ro.lineage.version").is_empty()) { // Putting LineageOS last, just in case any derivative writes to "ro.lineage.version".
		return "LineageOS";
	}

	if (!get_system_property("ro.modversion").is_empty()) { // Handles other Android custom ROMs.
		return vformat("%s %s", get_name(), "Custom ROM");
	}

	// Handles stock Android.
	return get_name();
}

String OS_Android::get_version() {
	const Vector<const char *> roms = { "ro.havoc.version", "org.pex.version", "org.pixelexperience.version",
		"ro.potato.version", "ro.xtended.version", "org.evolution.version", "ro.corvus.version", "ro.pa.version",
		"ro.crdroid.version", "ro.syberia.version", "ro.arrow.version", "ro.lineage.version" };
	for (int i = 0; i < roms.size(); i++) {
		String rom_version = get_system_property(roms[i]);
		if (!rom_version.is_empty()) {
			return rom_version;
		}
	}

	String mod_version = get_system_property("ro.modversion"); // Handles other Android custom ROMs.
	if (!mod_version.is_empty()) {
		return mod_version;
	}

	// Handles stock Android.
	String sdk_version = get_system_property("ro.build.version.sdk");
	String build = get_system_property("ro.build.version.incremental");
	if (!sdk_version.is_empty()) {
		if (!build.is_empty()) {
			return vformat("%s.%s", sdk_version, build);
		}
		return sdk_version;
	}

	return "";
}

String OS_Android::get_version_alias() {
	String release = get_system_property("ro.build.version.release_or_codename");
	String sdk_version = get_system_property("ro.build.version.sdk");
	String build = get_system_property("ro.build.version.incremental");

	return vformat("%s (SDK %s build %s)", release, sdk_version, build);
}

void OS_Android::main_loop_begin() {
	if (MainLoop::is_active()) {
		MainLoop::initialize();
	}
}

bool OS_Android::main_loop_iterate(bool *r_should_swap_buffers) {
	GodotProfileFrameMark;
	GodotProfileZone("OS_Android::main_loop_iterate");
	if (!MainLoop::is_active()) {
		return false;
	}
	DisplayServer::driver_as<DisplayServerAndroid>()->reset_swap_buffers_flag();
	DisplayServer::driver_as<DisplayServerAndroid>()->process_events();
	uint64_t current_frames_drawn = Engine::get_frames_drawn();
	bool exit = Main::iteration();

	if (r_should_swap_buffers) {
		*r_should_swap_buffers = !OS::is_in_low_processor_usage_mode() ||
				DisplayServer::driver_as<DisplayServerAndroid>()->should_swap_buffers() ||
				Renderer::has_changed() ||
				current_frames_drawn != Engine::get_frames_drawn();
	}

	return exit;
}

void OS_Android::main_loop_end() {
	if (MainLoop::is_active()) {
		if (SceneTree::is_initialized()) {
			SceneTree::quit();
		}
		MainLoop::finalize();
	}
}

#ifdef TOOLS_ENABLED
void OS_Android::_on_main_screen_changed(const String &p_screen_name) {
	if (OS_Android::is_initialized() && OS_Android::get_godot_java() != nullptr) {
		OS_Android::get_godot_java()->on_editor_workspace_selected(p_screen_name);
	}
}{
		OS_Android::get_godot_java()->on_editor_workspace_selected(p_screen_name);
	}{
		OS_Android::get_godot_java()->on_editor_workspace_selected(p_screen_name);
	}
}

void OS_Android::_on_distraction_free_mode_changed(bool p_enable) {
	if (OS_Android::is_initialized() && OS_Android::get_godot_java() != nullptr) {
		OS_Android::get_godot_java()->on_distraction_free_mode_changed(p_enable);
	}
}{
		OS_Android::get_godot_java()->on_distraction_free_mode_changed(p_enable);
	}{
		OS_Android::get_godot_java()->on_distraction_free_mode_changed(p_enable);
	}
}
#endif

void OS_Android::main_loop_focusout() {
	DisplayServer::driver_as<DisplayServerAndroid>()->send_window_event(DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT);
	}

	// Only pause when we are not in PiP mode.
	data->audio_driver_android.set_pause(!DisplayServer::driver_as<DisplayServerAndroid>()->is_in_pip_mode());
}

void OS_Android::main_loop_focusin() {
	DisplayServer::driver_as<DisplayServerAndroid>()->send_window_event(DisplayServerEnums::WINDOW_EVENT_FOCUS_IN);
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN);
	}
	data->audio_driver_android.set_pause(false);
}

Error OS_Android::shell_open(const String &p_uri) {
	return data->godot_io_java->open_uri(p_uri);
}

String OS_Android::get_resource_dir() {
#ifdef TOOLS_ENABLED
	return OS_Unix::get_resource_dir();
#else
	if (data->remote_fs_dir.is_empty()) {
		return "/"; // Android has its own filesystem for resources inside the APK
	} else {
		return data->remote_fs_dir;
	}
#endif
}

String OS_Android::get_locale() {
	String locale = data->godot_io_java->get_locale();
	if (!locale.is_empty()) {
		return locale;
	}

	return OS_Unix::get_locale();
}

String OS_Android::get_model_name() {
	String model = data->godot_io_java->get_model();
	if (!model.is_empty()) {
		return model;
	}

	return OS_Unix::get_model_name();
}

String OS_Android::get_processor_name() {
	return get_system_property("ro.soc.model");
}

String OS_Android::get_data_path() {
	return OS::get_user_data_dir();
}

void OS_Android::_load_system_font_config() {
	data->font_aliases.clear();
	data->fonts.clear();
	data->font_names.clear();

	Ref<XMLParser> parser;
	parser.instantiate();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	String root = String(getenv("ANDROID_ROOT")).path_join("fonts");

	Error err = parser->open(String(getenv("ANDROID_ROOT")).path_join("/etc/fonts.xml"));
	if (err == OK) {
		bool in_font_node = false;
		String fb, fn;
		FontInfo fi;

		while (parser->read() == OK) {
			if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
				in_font_node = false;
				if (parser->get_node_name() == "familyset") {
					int ver = parser->has_attribute("version") ? parser->get_named_attribute_value("version").to_int() : 0;
					if (ver < 21) {
						ERR_PRINT(vformat("Unsupported font config version %s", ver));
						break;
					}
				} else if (parser->get_node_name() == "alias") {
					String name = parser->has_attribute("name") ? parser->get_named_attribute_value("name").strip_edges() : String();
					String to = parser->has_attribute("to") ? parser->get_named_attribute_value("to").strip_edges() : String();
					if (!name.is_empty() && !to.is_empty()) {
						data->font_aliases[name] = to;
					}
				} else if (parser->get_node_name() == "family") {
					fn = parser->has_attribute("name") ? parser->get_named_attribute_value("name").strip_edges() : String();
					String lang_code = parser->has_attribute("lang") ? parser->get_named_attribute_value("lang").strip_edges() : String();
					Vector<String> lang_codes = lang_code.split(",");
					for (int i = 0; i < lang_codes.size(); i++) {
						Vector<String> lang_code_elements = lang_codes[i].split("-");
						if (lang_code_elements.size() >= 1 && lang_code_elements[0] != "und") {
							// Add missing script codes.
							if (lang_code_elements[0] == "ko") {
								fi.script.insert("Hani");
								fi.script.insert("Hang");
							}
							if (lang_code_elements[0] == "ja") {
								fi.script.insert("Hani");
								fi.script.insert("Kana");
								fi.script.insert("Hira");
							}
							if (!lang_code_elements[0].is_empty()) {
								fi.lang.insert(lang_code_elements[0]);
							}
						}
						if (lang_code_elements.size() >= 2) {
							// Add common codes for variants and remove variants not supported by HarfBuzz/ICU.
							if (lang_code_elements[1] == "Aran") {
								fi.script.insert("Arab");
							}
							if (lang_code_elements[1] == "Cyrs") {
								fi.script.insert("Cyrl");
							}
							if (lang_code_elements[1] == "Hanb") {
								fi.script.insert("Hani");
								fi.script.insert("Bopo");
							}
							if (lang_code_elements[1] == "Hans" || lang_code_elements[1] == "Hant") {
								fi.script.insert("Hani");
							}
							if (lang_code_elements[1] == "Syrj" || lang_code_elements[1] == "Syre" || lang_code_elements[1] == "Syrn") {
								fi.script.insert("Syrc");
							}
							if (!lang_code_elements[1].is_empty() && lang_code_elements[1] != "Zsym" && lang_code_elements[1] != "Zsye" && lang_code_elements[1] != "Zmth") {
								fi.script.insert(lang_code_elements[1]);
							}
						}
					}
				} else if (parser->get_node_name() == "font") {
					in_font_node = true;
					fb = parser->has_attribute("fallbackFor") ? parser->get_named_attribute_value("fallbackFor").strip_edges() : String();
					fi.weight = parser->has_attribute("weight") ? parser->get_named_attribute_value("weight").to_int() : 400;
					fi.italic = parser->has_attribute("style") && parser->get_named_attribute_value("style").strip_edges() == "italic";
				}
			}
			if (parser->get_node_type() == XMLParser::NODE_TEXT) {
				if (in_font_node) {
					fi.filename = parser->get_node_data().strip_edges();
					fi.font_name = fn;
					if (da->file_exists(root.path_join(fi.filename))) {
						if (!fb.is_empty() && fn.is_empty()) {
							fi.font_name = fb;
							fi.priority = 2;
						}
						if (fi.font_name.is_empty()) {
							fi.font_name = "sans-serif";
							fi.priority = 5;
						}
						if (fi.font_name.ends_with("-condensed")) {
							fi.stretch = 75;
							fi.font_name = fi.font_name.trim_suffix("-condensed");
						}
						data->fonts.push_back(fi);
						data->font_names.insert(fi.font_name);
					}
				}
			}
			if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
				in_font_node = false;
				if (parser->get_node_name() == "font") {
					fb = String();
					fi.font_name = String();
					fi.priority = 0;
					fi.weight = 400;
					fi.stretch = 100;
					fi.italic = false;
				} else if (parser->get_node_name() == "family") {
					fi = FontInfo();
					fn = String();
				}
			}
		}
		parser->close();
	} else {
		ERR_PRINT("Unable to load font config");
	}

	data->font_config_loaded = true;
}

Vector<String> OS_Android::get_system_fonts() {
	if (!data->font_config_loaded) {
		_load_system_font_config();
	}
	Vector<String> ret;
	for (const String &E : data->font_names) {
		ret.push_back(E);
	}
	return ret;
}

Vector<String> OS_Android::get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale, const String &p_script, int p_weight, int p_stretch, bool p_italic) {
	if (!data->font_config_loaded) {
		_load_system_font_config();
	}
	String font_name = p_font_name.to_lower();
	if (data->font_aliases.has(font_name)) {
		font_name = data->font_aliases[font_name];
	}
	String root = String(getenv("ANDROID_ROOT")).path_join("fonts");
	String lang_prefix = p_locale.get_slicec('_', 0);
	Vector<String> ret;
	int best_score = 0;
	for (const List<FontInfo>::Element *E = data->fonts.front(); E; E = E->next()) {
		int score = 0;
		if (!E->get().script.is_empty() && !p_script.is_empty() && !E->get().script.has(p_script)) {
			continue;
		}
		float sim = E->get().font_name.similarity(font_name);
		if (sim > 0.0) {
			score += (60 * sim + 5 - E->get().priority);
		}
		if (E->get().lang.has(p_locale)) {
			score += 120;
		} else if (E->get().lang.has(lang_prefix)) {
			score += 115;
		}
		if (E->get().script.has(p_script)) {
			score += 240;
		}
		score += (20 - Math::abs(E->get().weight - p_weight) / 50);
		score += (20 - Math::abs(E->get().stretch - p_stretch) / 10);
		if (E->get().italic == p_italic) {
			score += 30;
		}
		if (score > best_score) {
			best_score = score;
			if (!ret.has(root.path_join(E->get().filename))) {
				ret.insert(0, root.path_join(E->get().filename));
			}
		} else if (score == best_score || E->get().script.is_empty()) {
			if (!ret.has(root.path_join(E->get().filename))) {
				ret.push_back(root.path_join(E->get().filename));
			}
		}
		if (score >= 490) {
			break; // Perfect match.
		}
	}

	return ret;
}

String OS_Android::get_system_font_path(const String &p_font_name, int p_weight, int p_stretch, bool p_italic) {
	if (!data->font_config_loaded) {
		_load_system_font_config();
	}
	String font_name = p_font_name.to_lower();
	if (data->font_aliases.has(font_name)) {
		font_name = data->font_aliases[font_name];
	}
	String root = String(getenv("ANDROID_ROOT")).path_join("fonts");

	int best_score = 0;
	const List<FontInfo>::Element *best_match = nullptr;

	for (const List<FontInfo>::Element *E = data->fonts.front(); E; E = E->next()) {
		int score = 0;
		if (E->get().font_name == font_name) {
			score += (65 - E->get().priority);
		}
		score += (20 - Math::abs(E->get().weight - p_weight) / 50);
		score += (20 - Math::abs(E->get().stretch - p_stretch) / 10);
		if (E->get().italic == p_italic) {
			score += 30;
		}
		if (score >= 60 && score > best_score) {
			best_score = score;
			best_match = E;
		}
		if (score >= 140) {
			break; // Perfect match.
		}
	}
	if (best_match) {
		return root.path_join(best_match->get().filename);
	}
	return String();
}

String OS_Android::get_executable_path() {
	// Since unix process creation is restricted on Android, we bypass
	// OS_Unix::get_executable_path() so we can return ANDROID_EXEC_PATH.
	// Detection of ANDROID_EXEC_PATH allows to handle process creation in an Android compliant
	// manner.
	return OS::get_executable_path();
}

String OS_Android::get_user_data_dir(const String &p_user_dir) {
	if (!data->data_dir_cache.is_empty()) {
		return data->data_dir_cache;
	}

	String data_dir = data->godot_io_java->get_user_data_dir(p_user_dir);
	if (!data_dir.is_empty()) {
		data->data_dir_cache = _remove_symlink(data_dir);
		return data->data_dir_cache;
	}
	return ".";
}

String OS_Android::get_dynamic_libraries_path() {
	return get_cache_path().path_join("dynamic_libraries");
}

String OS_Android::get_cache_path() {
	if (!data->cache_dir_cache.is_empty()) {
		return data->cache_dir_cache;
	}

	String cache_dir = data->godot_io_java->get_cache_dir();
	if (!cache_dir.is_empty()) {
		data->cache_dir_cache = _remove_symlink(cache_dir);
		return data->cache_dir_cache;
	}
	return ".";
}

String OS_Android::get_temp_path() {
	if (!data->temp_dir_cache.is_empty()) {
		return data->temp_dir_cache;
	}

	String temp_dir = data->godot_io_java->get_temp_dir();
	if (!temp_dir.is_empty()) {
		data->temp_dir_cache = _remove_symlink(temp_dir);
		return data->temp_dir_cache;
	}
	return ".";
}

String OS_Android::get_unique_id() {
	String unique_id = data->godot_io_java->get_unique_id();
	if (!unique_id.is_empty()) {
		return unique_id;
	}

	return OS::get_unique_id();
}

String OS_Android::get_system_dir(OS::SystemDir p_dir, bool p_shared_storage) {
	return data->godot_io_java->get_system_dir(p_dir, p_shared_storage);
}

Error OS_Android::move_to_trash(const String &p_path) {
	Ref<DirAccess> da_ref = DirAccess::create_for_path(p_path);
	if (da_ref.is_null()) {
		return FAILED;
	}

	// Check if it's a directory
	if (da_ref->dir_exists(p_path)) {
		Error err = da_ref->change_dir(p_path);
		if (err) {
			return err;
		}
		// This is directory, let's erase its contents
		err = da_ref->erase_contents_recursive();
		if (err) {
			return err;
		}
		// Remove the top directory
		return da_ref->remove(p_path);
	} else if (da_ref->file_exists(p_path)) {
		// This is a file, let's remove it.
		return da_ref->remove(p_path);
	} else {
		return FAILED;
	}
}

void OS_Android::set_display_size(const Size2i &p_size) {
	data->display_size = p_size;
}

Size2i OS_Android::get_display_size() {
	return data->display_size;
}

void OS_Android::set_opengl_extensions(const char *p_gl_extensions) {
#if defined(GLES3_ENABLED)
	ERR_FAIL_NULL(p_gl_extensions);
	data->gl_extensions = p_gl_extensions;
#endif
}

void OS_Android::set_native_window(ANativeWindow *p_native_window) {
#if defined(VULKAN_ENABLED)
	data->native_window = p_native_window;
#endif
}

ANativeWindow *OS_Android::get_native_window() {
#if defined(VULKAN_ENABLED)
	return data->native_window;
#else
	return nullptr;
#endif
}

void OS_Android::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	data->godot_java->vibrate(p_duration_ms, p_amplitude);
}

String OS_Android::get_config_path() {
	return OS::get_user_data_dir().path_join("config");
}

void OS_Android::benchmark_begin_measure(const String &p_context, const String &p_what) {
#ifdef TOOLS_ENABLED
	data->godot_java->begin_benchmark_measure(p_context, p_what);
#endif
}

void OS_Android::benchmark_end_measure(const String &p_context, const String &p_what) {
#ifdef TOOLS_ENABLED
	data->godot_java->end_benchmark_measure(p_context, p_what);
#endif
}

void OS_Android::benchmark_dump() {
#ifdef TOOLS_ENABLED
	if (!OS::is_use_benchmark_set()) {
		return;
	}
	data->godot_java->dump_benchmark(OS::get_benchmark_file());
#endif
}

#ifdef TOOLS_ENABLED
Error OS_Android::sign_apk(const String &p_input_path, const String &p_output_path, const String &p_keystore_path, const String &p_keystore_user, const String &p_keystore_password) {
	return data->godot_java->sign_apk(p_input_path, p_output_path, p_keystore_path, p_keystore_user, p_keystore_password);
}

Error OS_Android::verify_apk(const String &p_apk_path) {
	return data->godot_java->verify_apk(p_apk_path);
}
#endif

bool OS_Android::_check_internal_feature_support(const String &p_feature) {
	if (p_feature == "macos" || p_feature == "web_ios" || p_feature == "web_macos" || p_feature == "windows") {
		return false;
	}

	if (p_feature == "system_fonts") {
		return true;
	}
	if (p_feature == "mobile") {
		return true;
	}
#if defined(__aarch64__)
	if (p_feature == "arm64-v8a" || p_feature == "arm64") {
		return true;
	}
#elif defined(__ARM_ARCH_7A__)
	if (p_feature == "armeabi-v7a" || p_feature == "armeabi" || p_feature == "arm32") {
		return true;
	}
#elif defined(__arm__)
	if (p_feature == "armeabi" || p_feature == "arm") {
		return true;
	}
#endif

	if (data->godot_java->check_internal_feature_support(p_feature)) {
		return true;
	}

	return false;
}

void OS_Android::setup(GodotJavaWrapper *p_godot_java, GodotIOJavaWrapper *p_godot_io_java, bool p_use_apk_expansion) {
	if (!data) {
		data = memnew(Data);
	}
	data->display_size.width = DEFAULT_WINDOW_WIDTH;
	data->display_size.height = DEFAULT_WINDOW_HEIGHT;

	data->use_apk_expansion = p_use_apk_expansion;


#if defined(GLES3_ENABLED)
	data->gl_extensions = nullptr;
#endif

#if defined(VULKAN_ENABLED)
	data->native_window = nullptr;
#endif

	data->godot_java = p_godot_java;
	data->godot_io_java = p_godot_io_java;

	Vector<Logger *> loggers;
	loggers.push_back(memnew(Logger(Logger::BACKEND_ANDROID)));
	OS::_set_logger(memnew(CompositeLogger(loggers)));

	AudioDriverManager::add_driver(&data->audio_driver_android);

	DisplayServerAndroid::register_android_driver();
}

Error OS_Android::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	if (p_path == ANDROID_EXEC_PATH) {
		return create_instance(p_arguments);
	} else {
		return OS_Unix::execute(p_path, p_arguments, r_pipe, r_exitcode, read_stderr, p_pipe_mutex, p_open_console);
	}
}

Error OS_Android::create_process(const String &p_path, const List<String> &p_arguments, OS::ProcessID *r_child_id, bool p_open_console) {
	if (p_path == ANDROID_EXEC_PATH) {
		return create_instance(p_arguments, r_child_id);
	} else {
		return OS_Unix::create_process(p_path, p_arguments, r_child_id, p_open_console);
	}
}

Error OS_Android::create_instance(const List<String> &p_arguments, OS::ProcessID *r_child_id) {
	int instance_id = data->godot_java->create_new_godot_instance(p_arguments);
	if (instance_id == -1) {
		return FAILED;
	}
	if (r_child_id) {
		*r_child_id = instance_id;
	}
	return OK;
}

Error OS_Android::kill(const OS::ProcessID &p_pid) {
	if (data->godot_java->force_quit(nullptr, p_pid)) {
		return OK;
	}
	return OS_Unix::kill(p_pid);
}

String OS_Android::get_system_ca_certificates() {
	return data->godot_java->get_ca_certificates();
}

Error OS_Android::setup_remote_filesystem(const String &p_server_host, int p_port, const String &p_password, String &r_project_path) {
	r_project_path = OS::get_user_data_dir();
	Error err = OS_Unix::setup_remote_filesystem(p_server_host, p_port, p_password, r_project_path);
	if (err == OK) {
		data->remote_fs_dir = r_project_path;
		FileAccess::make_default<FileAccessFilesystemJAndroid>(FileAccess::ACCESS_RESOURCES);
	}
	return err;
}

void OS_Android::load_platform_gdextensions() {}

/* -------------------------------------------------------------------------- */
/*                        OS Static Platform Dispatch                         */
/* -------------------------------------------------------------------------- */

void OS::initialize_core() {
	OS_Android::initialize_core();
	_initialized = true;
}

void OS::initialize() {
	OS_Android::initialize();
	_initialized = true;
}

void OS::finalize() {
	_initialized = false;
	OS_Android::finalize();
	if (_logger) {
		memdelete(_logger);
		_logger = nullptr;
	}
}

void OS::finalize_core() {
	OS_Android::finalize_core();
}

void OS::initialize_joypads() {
	OS_Android::initialize_joypads();
}

uint64_t OS::get_ticks_usec() {
	return OS_Unix::get_ticks_usec();
}

void OS::delay_usec(uint32_t p_usec) {
	OS_Unix::delay_usec(p_usec);
}

String OS::get_environment(const String &p_var) {
	return OS_Unix::get_environment(p_var);
}

bool OS::has_environment(const String &p_var) {
	return OS_Unix::has_environment(p_var);
}

void OS::set_environment(const String &p_var, const String &p_value) {
	OS_Unix::set_environment(p_var, p_value);
}

void OS::unset_environment(const String &p_var) {
	OS_Unix::unset_environment(p_var);
}

void OS::load_shell_environment() {
}

OS::DateTime OS::get_datetime(bool p_utc) {
	return OS_Unix::get_datetime(p_utc);
}

OS::TimeZoneInfo OS::get_time_zone_info() {
	return OS_Unix::get_time_zone_info();
}

Error OS::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool p_read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return OS_Android::execute(p_path, p_arguments, r_pipe, r_exitcode, p_read_stderr, p_pipe_mutex, p_open_console);
}

Error OS::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	return OS_Android::create_process(p_path, p_arguments, r_child_id, p_open_console);
}

Error OS::kill(const ProcessID &p_pid) {
	return OS_Android::kill(p_pid);
}

bool OS::is_process_running(const ProcessID &p_pid) {
	return OS_Unix::is_process_running(p_pid);
}

Error OS::get_entropy(uint8_t *r_buffer, int p_bytes) {
	return OS_Unix::get_entropy(r_buffer, p_bytes);
}

String OS::get_system_ca_certificates() {
	return OS_Android::get_system_ca_certificates();
}

String OS::get_name() {
	return OS_Android::get_name();
}

List<String> OS::get_cmdline_platform_args() {
	return List<String>();
}

bool OS::_check_internal_feature_support(const String &p_feature) {
	return OS_Android::_check_internal_feature_support(p_feature);
}

void OS::disable_crash_handler() {
}

void OS::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	OS_Android::vibrate_handheld(p_duration_ms, p_amplitude);
}

Error OS::move_to_trash(const String &p_path) {
	return OS_Android::move_to_trash(p_path);
}

#ifdef TOOLS_ENABLED
bool OS::_test_create_rendering_device(const String &p_display_driver) {
	return false;
}

bool OS::_test_create_rendering_device_and_gl(const String &p_display_driver) {
	return false;
}
#endif


// --- Merged from platform/android/file_access_android.cpp ---
#include "servers/display/display_server.h"


#include "core/string/ustring.h"

#include <android/asset_manager_jni.h>

AAssetManager *FileAccessAndroid::asset_manager = nullptr;
jobject FileAccessAndroid::j_asset_manager = nullptr;

String FileAccessAndroid::get_path() const {
	return path_src;
}

String FileAccessAndroid::get_path_absolute() const {
	return absolute_path;
}

Error FileAccessAndroid::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	String path = fix_path(p_path).simplify_path();
	absolute_path = path;
	if (path.begins_with("/")) {
		path = path.substr(1);
	} else if (path.begins_with("res://")) {
		path = path.substr(6);
	}

	ERR_FAIL_COND_V(p_mode_flags & FileAccess::WRITE, ERR_UNAVAILABLE); //can't write on android..
	asset = AAssetManager_open(asset_manager, path.utf8().get_data(), AASSET_MODE_STREAMING);
	if (!asset) {
		return ERR_CANT_OPEN;
	}
	len = AAsset_getLength(asset);
	pos = 0;
	eof = false;

	return OK;
}

void FileAccessAndroid::_close() {
	if (!asset) {
		return;
	}
	AAsset_close(asset);
	asset = nullptr;
}

bool FileAccessAndroid::is_open() const {
	return asset != nullptr;
}

void FileAccessAndroid::seek(uint64_t p_position) {
	ERR_FAIL_NULL(asset);

	AAsset_seek(asset, p_position, SEEK_SET);
	pos = p_position;
	if (pos > len) {
		pos = len;
		eof = true;
	} else {
		eof = false;
	}
}

void FileAccessAndroid::seek_end(int64_t p_position) {
	ERR_FAIL_NULL(asset);
	AAsset_seek(asset, p_position, SEEK_END);
	pos = len + p_position;
}

uint64_t FileAccessAndroid::get_position() const {
	return pos;
}

uint64_t FileAccessAndroid::get_length() const {
	return len;
}

bool FileAccessAndroid::eof_reached() const {
	return eof;
}

uint64_t FileAccessAndroid::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	int r = AAsset_read(asset, p_dst, p_length);

	if (pos + p_length > len) {
		eof = true;
	}

	if (r >= 0) {
		pos += r;
		if (pos > len) {
			pos = len;
		}
	}

	return r;
}

int64_t FileAccessAndroid::_get_size(const String &p_file) {
	return AAsset_getLength64(asset);
}

Error FileAccessAndroid::get_error() const {
	return eof ? ERR_FILE_EOF : OK; // not sure what else it may happen
}

void FileAccessAndroid::flush() {
	ERR_FAIL();
}

bool FileAccessAndroid::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_V(false);
}

bool FileAccessAndroid::file_exists(const String &p_path) {
	String path = fix_path(p_path).simplify_path();
	if (path.begins_with("/")) {
		path = path.substr(1);
	} else if (path.begins_with("res://")) {
		path = path.substr(6);
	}

	AAsset *at = AAssetManager_open(asset_manager, path.utf8().get_data(), AASSET_MODE_STREAMING);

	if (!at) {
		return false;
	}

	AAsset_close(at);
	return true;
}

void FileAccessAndroid::close() {
	_close();
}

FileAccessAndroid::~FileAccessAndroid() {
	_close();
}

void FileAccessAndroid::setup(jobject p_asset_manager) {
	JNIEnv *env = get_jni_env();
	j_asset_manager = env->NewGlobalRef(p_asset_manager);
	asset_manager = AAssetManager_fromJava(env, j_asset_manager);
}

void FileAccessAndroid::terminate() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(j_asset_manager);
}


// --- Merged from platform/android/file_access_filesystem_jandroid.cpp ---
#include "servers/display/display_server.h"


#include "core/string/ustring.h"

#include <unistd.h>

jobject FileAccessFilesystemJAndroid::file_access_handler = nullptr;
jclass FileAccessFilesystemJAndroid::cls;

jmethodID FileAccessFilesystemJAndroid::_file_open = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_get_size = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_seek = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_seek_end = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_read = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_tell = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_eof = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_set_eof = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_close = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_write = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_flush = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_exists = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_last_modified = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_last_accessed = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_resize = nullptr;
jmethodID FileAccessFilesystemJAndroid::_file_size = nullptr;

String FileAccessFilesystemJAndroid::get_path() const {
	return path_src;
}

String FileAccessFilesystemJAndroid::get_path_absolute() const {
	return absolute_path;
}

Error FileAccessFilesystemJAndroid::open_internal(const String &p_path, int p_mode_flags) {
	if (is_open()) {
		_close();
	}

	if (_file_open) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		String path = fix_path(p_path).simplify_path();
		jstring js = env->NewStringUTF(path.utf8().get_data());
		int res = env->CallIntMethod(file_access_handler, _file_open, js, p_mode_flags);
		env->DeleteLocalRef(js);

		if (res < 0) {
			// Errors are passed back as their negative value to differentiate from the positive file id.
			return static_cast<Error>(-res);
		}

		id = res;
		path_src = p_path;
		absolute_path = path;
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

void FileAccessFilesystemJAndroid::_close() {
	if (!is_open()) {
		return;
	}

	if (_file_close) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(file_access_handler, _file_close, id);
	}
	id = 0;
}

bool FileAccessFilesystemJAndroid::is_open() const {
	return id != 0;
}

void FileAccessFilesystemJAndroid::seek(uint64_t p_position) {
	if (_file_seek) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		ERR_FAIL_COND_MSG(!is_open(), "File must be opened before use.");
		env->CallVoidMethod(file_access_handler, _file_seek, id, p_position);
	}
}

void FileAccessFilesystemJAndroid::seek_end(int64_t p_position) {
	if (_file_seek_end) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		ERR_FAIL_COND_MSG(!is_open(), "File must be opened before use.");
		env->CallVoidMethod(file_access_handler, _file_seek_end, id, p_position);
	}
}

uint64_t FileAccessFilesystemJAndroid::get_position() const {
	if (_file_tell) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		ERR_FAIL_COND_V_MSG(!is_open(), 0, "File must be opened before use.");
		return env->CallLongMethod(file_access_handler, _file_tell, id);
	} else {
		return 0;
	}
}

uint64_t FileAccessFilesystemJAndroid::get_length() const {
	if (_file_get_size) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		ERR_FAIL_COND_V_MSG(!is_open(), 0, "File must be opened before use.");
		return env->CallLongMethod(file_access_handler, _file_get_size, id);
	} else {
		return 0;
	}
}

bool FileAccessFilesystemJAndroid::eof_reached() const {
	if (_file_eof) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		ERR_FAIL_COND_V_MSG(!is_open(), false, "File must be opened before use.");
		return env->CallBooleanMethod(file_access_handler, _file_eof, id);
	} else {
		return false;
	}
}

void FileAccessFilesystemJAndroid::_set_eof(bool eof) {
	if (_file_set_eof) {
		ERR_FAIL_COND_MSG(!is_open(), "File must be opened before use.");

		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(file_access_handler, _file_set_eof, id, eof);
	}
}

String FileAccessFilesystemJAndroid::get_line() const {
	ERR_FAIL_COND_V_MSG(!is_open(), String(), "File must be opened before use.");

	const size_t buffer_size_limit = 2048;
	const uint64_t file_size = get_length();
	const uint64_t start_position = get_position();

	String result;
	LocalVector<uint8_t> line_buffer;
	size_t current_buffer_size = 0;
	uint64_t line_buffer_position = 0;

	while (true) {
		size_t line_buffer_size = MIN(buffer_size_limit, file_size - get_position());
		if (line_buffer_size <= 0) {
			const_cast<FileAccessFilesystemJAndroid *>(this)->_set_eof(true);
			break;
		}

		current_buffer_size += line_buffer_size;
		line_buffer.resize(current_buffer_size);

		uint64_t bytes_read = get_buffer(&line_buffer[line_buffer_position], current_buffer_size - line_buffer_position);
		if (bytes_read <= 0) {
			break;
		}

		for (; bytes_read > 0; line_buffer_position++, bytes_read--) {
			uint8_t elem = line_buffer[line_buffer_position];
			if (elem == '\r' || elem == '\n' || elem == '\0') {
				// Found the end of the line
				const bool is_crlf = elem == '\r' && line_buffer_position + 1 < current_buffer_size && line_buffer[line_buffer_position + 1] == '\n';
				const_cast<FileAccessFilesystemJAndroid *>(this)->seek(start_position + line_buffer_position + (is_crlf ? 2 : 1));
				if (result.append_utf8((const char *)line_buffer.ptr(), line_buffer_position)) {
					return String();
				}
				return result;
			}
		}
	}

	if (result.append_utf8((const char *)line_buffer.ptr(), line_buffer_position)) {
		return String();
	}
	return result;
}

uint64_t FileAccessFilesystemJAndroid::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	if (_file_read) {
		ERR_FAIL_COND_V_MSG(!is_open(), 0, "File must be opened before use.");
		if (p_length == 0) {
			return 0;
		}

		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);

		jobject j_buffer = env->NewDirectByteBuffer(p_dst, p_length);
		int length = env->CallIntMethod(file_access_handler, _file_read, id, j_buffer);
		env->DeleteLocalRef(j_buffer);
		return length;
	} else {
		return 0;
	}
}

bool FileAccessFilesystemJAndroid::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	if (_file_write) {
		ERR_FAIL_COND_V_MSG(!is_open(), false, "File must be opened before use.");
		ERR_FAIL_COND_V(!p_src && p_length > 0, false);
		if (p_length == 0) {
			return true;
		}

		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		jobject j_buffer = env->NewDirectByteBuffer((void *)p_src, p_length);
		bool ok = env->CallBooleanMethod(file_access_handler, _file_write, id, j_buffer);
		env->DeleteLocalRef(j_buffer);
		return ok;
	} else {
		return false;
	}
}

Error FileAccessFilesystemJAndroid::get_error() const {
	if (eof_reached()) {
		return ERR_FILE_EOF;
	}
	return OK;
}

Error FileAccessFilesystemJAndroid::resize(int64_t p_length) {
	if (_file_resize) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, FAILED);
		ERR_FAIL_COND_V_MSG(!is_open(), FAILED, "File must be opened before use.");
		int res = env->CallIntMethod(file_access_handler, _file_resize, id, p_length);
		return static_cast<Error>(res);
	} else {
		return ERR_UNAVAILABLE;
	}
}

void FileAccessFilesystemJAndroid::flush() {
	if (_file_flush) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		ERR_FAIL_COND_MSG(!is_open(), "File must be opened before use.");
		env->CallVoidMethod(file_access_handler, _file_flush, id);
	}
}

bool FileAccessFilesystemJAndroid::file_exists(const String &p_path) {
	if (_file_exists) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		String path = fix_path(p_path).simplify_path();
		jstring js = env->NewStringUTF(path.utf8().get_data());
		bool result = env->CallBooleanMethod(file_access_handler, _file_exists, js);
		env->DeleteLocalRef(js);
		return result;
	} else {
		return false;
	}
}

uint64_t FileAccessFilesystemJAndroid::_get_modified_time(const String &p_file) {
	if (_file_last_modified) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);

		String path = fix_path(p_file).simplify_path();
		jstring js = env->NewStringUTF(path.utf8().get_data());
		uint64_t result = env->CallLongMethod(file_access_handler, _file_last_modified, js);
		env->DeleteLocalRef(js);
		return result;
	} else {
		return 0;
	}
}

uint64_t FileAccessFilesystemJAndroid::_get_access_time(const String &p_file) {
	if (_file_last_accessed) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);

		String path = fix_path(p_file).simplify_path();
		jstring js = env->NewStringUTF(path.utf8().get_data());
		uint64_t result = env->CallLongMethod(file_access_handler, _file_last_accessed, js);
		env->DeleteLocalRef(js);
		return result;
	} else {
		return 0;
	}
}

int64_t FileAccessFilesystemJAndroid::_get_size(const String &p_file) {
	if (_file_size) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, -1);

		String path = fix_path(p_file).simplify_path();
		jstring js = env->NewStringUTF(path.utf8().get_data());
		int64_t result = env->CallLongMethod(file_access_handler, _file_size, js);
		env->DeleteLocalRef(js);
		return result;
	} else {
		return -1;
	}
}

void FileAccessFilesystemJAndroid::setup(jobject p_file_access_handler) {
	JNIEnv *env = get_jni_env();
	file_access_handler = env->NewGlobalRef(p_file_access_handler);

	jclass c = env->GetObjectClass(file_access_handler);
	cls = (jclass)env->NewGlobalRef(c);

	_file_open = env->GetMethodID(cls, "fileOpen", "(Ljava/lang/String;I)I");
	_file_get_size = env->GetMethodID(cls, "fileGetSize", "(I)J");
	_file_tell = env->GetMethodID(cls, "fileGetPosition", "(I)J");
	_file_eof = env->GetMethodID(cls, "isFileEof", "(I)Z");
	_file_set_eof = env->GetMethodID(cls, "setFileEof", "(IZ)V");
	_file_seek = env->GetMethodID(cls, "fileSeek", "(IJ)V");
	_file_seek_end = env->GetMethodID(cls, "fileSeekFromEnd", "(IJ)V");
	_file_read = env->GetMethodID(cls, "fileRead", "(ILjava/nio/ByteBuffer;)I");
	_file_close = env->GetMethodID(cls, "fileClose", "(I)V");
	_file_write = env->GetMethodID(cls, "fileWrite", "(ILjava/nio/ByteBuffer;)Z");
	_file_flush = env->GetMethodID(cls, "fileFlush", "(I)V");
	_file_exists = env->GetMethodID(cls, "fileExists", "(Ljava/lang/String;)Z");
	_file_last_modified = env->GetMethodID(cls, "fileLastModified", "(Ljava/lang/String;)J");
	_file_last_accessed = env->GetMethodID(cls, "fileLastAccessed", "(Ljava/lang/String;)J");
	_file_resize = env->GetMethodID(cls, "fileResize", "(IJ)I");
	_file_size = env->GetMethodID(cls, "fileSize", "(Ljava/lang/String;)J");
}

void FileAccessFilesystemJAndroid::terminate() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(cls);
	env->DeleteGlobalRef(file_access_handler);
}

void FileAccessFilesystemJAndroid::close() {
	if (is_open()) {
		_close();
	}
}

FileAccessFilesystemJAndroid::FileAccessFilesystemJAndroid() {
	id = 0;
}

FileAccessFilesystemJAndroid::~FileAccessFilesystemJAndroid() {
	if (is_open()) {
		_close();
	}
}


// --- Merged from platform/android/audio_driver_opensl.cpp ---
#include "servers/display/display_server.h"

#include "core/os/os.h"

#define MAX_NUMBER_INTERFACES 3
#define MAX_NUMBER_OUTPUT_DEVICES 6

/* Structure for passing information to callback function */

void AudioDriverOpenSL::_buffer_callback(
		SLAndroidSimpleBufferQueueItf queueItf) {
	bool mix = true;

	if (pause) {
		mix = false;
	} else {
		mix = mutex.try_lock();
	}

	if (mix) {
		audio_server_process(buffer_size, mixdown_buffer);
	} else {
		int32_t *src_buff = mixdown_buffer;
		for (unsigned int i = 0; i < buffer_size * 2; i++) {
			src_buff[i] = 0;
		}
	}

	if (mix) {
		mutex.unlock();
	}

	const int32_t *src_buff = mixdown_buffer;

	int16_t *ptr = (int16_t *)buffers[last_free];
	last_free = (last_free + 1) % BUFFER_COUNT;

	for (unsigned int i = 0; i < buffer_size * 2; i++) {
		ptr[i] = src_buff[i] >> 16;
	}

	(*queueItf)->Enqueue(queueItf, ptr, 4 * buffer_size);
}

void AudioDriverOpenSL::_buffer_callbacks(
		SLAndroidSimpleBufferQueueItf queueItf,
		void *pContext) {
	AudioDriverOpenSL *ad = static_cast<AudioDriverOpenSL *>(pContext);

	ad->_buffer_callback(queueItf);
}

Error AudioDriverOpenSL::init() {
	SLresult res;
	SLEngineOption EngineOption[] = {
		{ (SLuint32)SL_ENGINEOPTION_THREADSAFE, (SLuint32)SL_BOOLEAN_TRUE }
	};
	res = slCreateEngine(&sl, 1, EngineOption, 0, nullptr, nullptr);
	ERR_FAIL_COND_V_MSG(res != SL_RESULT_SUCCESS, ERR_INVALID_PARAMETER, "Could not initialize OpenSL.");

	res = (*sl)->Realize(sl, SL_BOOLEAN_FALSE);
	ERR_FAIL_COND_V_MSG(res != SL_RESULT_SUCCESS, ERR_INVALID_PARAMETER, "Could not realize OpenSL.");

	return OK;
}

void AudioDriverOpenSL::start() {
	active = false;

	SLresult res;

	buffer_size = 1024;

	for (int i = 0; i < BUFFER_COUNT; i++) {
		buffers[i] = memnew_arr(int16_t, buffer_size * 2);
		memset(buffers[i], 0, buffer_size * 4);
	}

	mixdown_buffer = memnew_arr(int32_t, buffer_size * 2);

	/* Callback context for the buffer queue callback function */

	/* Get the SL Engine Interface which is implicit */
	res = (*sl)->GetInterface(sl, SL_IID_ENGINE, (void *)&EngineItf);

	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);

	{
		const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
		const SLboolean req[1] = { SL_BOOLEAN_FALSE };
		res = (*EngineItf)->CreateOutputMix(EngineItf, &OutputMix, 0, ids, req);
	}

	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
	// Realizing the Output Mix object in synchronous mode.
	res = (*OutputMix)->Realize(OutputMix, SL_BOOLEAN_FALSE);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);

	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, BUFFER_COUNT };
	/* Setup the format of the content in the buffer queue */
	pcm.formatType = SL_DATAFORMAT_PCM;
	pcm.numChannels = 2;
	pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
	pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

	audioSource.pFormat = (void *)&pcm;
	audioSource.pLocator = (void *)&loc_bufq;

	/* Setup the data sink structure */
	locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	locator_outputmix.outputMix = OutputMix;
	audioSink.pLocator = (void *)&locator_outputmix;
	audioSink.pFormat = nullptr;

	/* Create the music player */
	{
		const SLInterfaceID ids[2] = { SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND };
		const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

		res = (*EngineItf)->CreateAudioPlayer(EngineItf, &player, &audioSource, &audioSink, 1, ids, req);
		ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
	}
	/* Realizing the player in synchronous mode. */
	res = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
	/* Get seek and play interfaces */
	res = (*player)->GetInterface(player, SL_IID_PLAY, (void *)&playItf);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
	res = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE,
			(void *)&bufferQueueItf);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
	/* Setup to receive buffer queue event callbacks */
	res = (*bufferQueueItf)->RegisterCallback(bufferQueueItf, _buffer_callbacks, this);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);

	last_free = 0;

	//fill up buffers
	for (int i = 0; i < BUFFER_COUNT; i++) {
		/* Enqueue a few buffers to get the ball rolling */
		res = (*bufferQueueItf)->Enqueue(bufferQueueItf, buffers[i], 4 * buffer_size); /* Size given in */
	}

	res = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);

	active = true;
}

void AudioDriverOpenSL::_record_buffer_callback(SLAndroidSimpleBufferQueueItf queueItf) {
	for (int i = 0; i < rec_buffer.size(); i++) {
		int32_t sample = rec_buffer[i] << 16;
		input_buffer_write(sample);
		input_buffer_write(sample); // call twice to convert to Stereo
	}

	SLresult res = (*recordBufferQueueItf)->Enqueue(recordBufferQueueItf, rec_buffer.ptrw(), rec_buffer.size() * sizeof(int16_t));
	ERR_FAIL_COND(res != SL_RESULT_SUCCESS);
}

void AudioDriverOpenSL::_record_buffer_callbacks(SLAndroidSimpleBufferQueueItf queueItf, void *pContext) {
	AudioDriverOpenSL *ad = static_cast<AudioDriverOpenSL *>(pContext);

	ad->_record_buffer_callback(queueItf);
}

Error AudioDriverOpenSL::init_input_device() {
	SLDataLocator_IODevice loc_dev = {
		SL_DATALOCATOR_IODEVICE,
		SL_IODEVICE_AUDIOINPUT,
		SL_DEFAULTDEVICEID_AUDIOINPUT,
		nullptr
	};
	SLDataSource recSource = { &loc_dev, nullptr };

	SLDataLocator_AndroidSimpleBufferQueue loc_bq = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
		2
	};
	SLDataFormat_PCM format_pcm = {
		SL_DATAFORMAT_PCM,
		1,
		SL_SAMPLINGRATE_44_1,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_CENTER,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSink recSnk = { &loc_bq, &format_pcm };

	const SLInterfaceID ids[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
	const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	SLresult res = (*EngineItf)->CreateAudioRecorder(EngineItf, &recorder, &recSource, &recSnk, 2, ids, req);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	res = (*recorder)->Realize(recorder, SL_BOOLEAN_FALSE);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	res = (*recorder)->GetInterface(recorder, SL_IID_RECORD, (void *)&recordItf);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	res = (*recorder)->GetInterface(recorder, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, (void *)&recordBufferQueueItf);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	res = (*recordBufferQueueItf)->RegisterCallback(recordBufferQueueItf, _record_buffer_callbacks, this);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	SLuint32 state;
	res = (*recordItf)->GetRecordState(recordItf, &state);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	if (state != SL_RECORDSTATE_STOPPED) {
		res = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
		ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

		res = (*recordBufferQueueItf)->Clear(recordBufferQueueItf);
		ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);
	}

	const int rec_buffer_frames = 2048;
	rec_buffer.resize(rec_buffer_frames);
	input_buffer_init(rec_buffer_frames);

	res = (*recordBufferQueueItf)->Enqueue(recordBufferQueueItf, rec_buffer.ptrw(), rec_buffer.size() * sizeof(int16_t));
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	res = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_RECORDING);
	ERR_FAIL_COND_V(res != SL_RESULT_SUCCESS, ERR_CANT_OPEN);

	return OK;
}

Error AudioDriverOpenSL::input_start() {
	if (recordItf || recordBufferQueueItf) {
		return ERR_ALREADY_IN_USE;
	}

	if (OS::request_permission("RECORD_AUDIO")) {
		Error err = init_input_device();
		if (err != OK) {
			input_stop();
		}
		return err;
	}

	WARN_PRINT("Unable to start audio capture - No RECORD_AUDIO permission");
	return ERR_UNAUTHORIZED;
}

Error AudioDriverOpenSL::input_stop() {
	if (recordItf) {
		(*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
		recordItf = nullptr;
	}

	if (recordBufferQueueItf) {
		(*recordBufferQueueItf)->Clear(recordBufferQueueItf);
		recordBufferQueueItf = nullptr;
	}
	if (recorder) {
		(*recorder)->Destroy(recorder);
		recorder = nullptr;
	}

	return OK;
}

int AudioDriverOpenSL::get_mix_rate() const {
	return 44100; // hardcoded for Android, as selected by SL_SAMPLINGRATE_44_1
}

AudioDriver::SpeakerMode AudioDriverOpenSL::get_speaker_mode() const {
	return SPEAKER_MODE_STEREO;
}

void AudioDriverOpenSL::lock() {
	if (active) {
		mutex.lock();
	}
}

void AudioDriverOpenSL::unlock() {
	if (active) {
		mutex.unlock();
	}
}

void AudioDriverOpenSL::finish() {
	if (recordItf) {
		(*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
		recordItf = nullptr;
	}
	if (recorder) {
		(*recorder)->Destroy(recorder);
		recorder = nullptr;
	}
	if (playItf) {
		(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
		playItf = nullptr;
	}
	if (player) {
		(*player)->Destroy(player);
		player = nullptr;
	}
	if (OutputMix) {
		(*OutputMix)->Destroy(OutputMix);
		OutputMix = nullptr;
	}
	if (sl) {
		(*sl)->Destroy(sl);
		sl = nullptr;
	}
}

void AudioDriverOpenSL::set_pause(bool p_pause) {
	pause = p_pause;

	if (active && playItf) {
		if (pause) {
			(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PAUSED);
		} else {
			(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
		}
	}
}

AudioDriverOpenSL::AudioDriverOpenSL() {
}


// --- Merged from platform/android/dir_access_jandroid.cpp ---
#include "servers/display/display_server.h"


#include "core/string/ustring.h"

jobject DirAccessJAndroid::dir_access_handler = nullptr;
jclass DirAccessJAndroid::cls = nullptr;
jmethodID DirAccessJAndroid::_dir_open = nullptr;
jmethodID DirAccessJAndroid::_dir_next = nullptr;
jmethodID DirAccessJAndroid::_dir_close = nullptr;
jmethodID DirAccessJAndroid::_dir_is_dir = nullptr;
jmethodID DirAccessJAndroid::_dir_exists = nullptr;
jmethodID DirAccessJAndroid::_file_exists = nullptr;
jmethodID DirAccessJAndroid::_get_drive_count = nullptr;
jmethodID DirAccessJAndroid::_get_drive = nullptr;
jmethodID DirAccessJAndroid::_make_dir = nullptr;
jmethodID DirAccessJAndroid::_get_space_left = nullptr;
jmethodID DirAccessJAndroid::_rename = nullptr;
jmethodID DirAccessJAndroid::_remove = nullptr;
jmethodID DirAccessJAndroid::_current_is_hidden = nullptr;

Error DirAccessJAndroid::list_dir_begin() {
	list_dir_end();
	int res = dir_open(current_dir);
	if (res <= 0) {
		return ERR_CANT_OPEN;
	}

	id = res;

	return OK;
}

String DirAccessJAndroid::get_next() {
	ERR_FAIL_COND_V(id == 0, "");
	if (_dir_next) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, "");
		jstring str = (jstring)env->CallObjectMethod(dir_access_handler, _dir_next, id);
		if (!str) {
			return "";
		}

		String ret = jstring_to_string((jstring)str, env);
		env->DeleteLocalRef((jobject)str);
		return ret;
	} else {
		return "";
	}
}

bool DirAccessJAndroid::current_is_dir() const {
	if (_dir_is_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(dir_access_handler, _dir_is_dir, id);
	} else {
		return false;
	}
}

bool DirAccessJAndroid::current_is_hidden() const {
	if (_current_is_hidden) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(dir_access_handler, _current_is_hidden, id);
	}
	return false;
}

void DirAccessJAndroid::list_dir_end() {
	if (id == 0) {
		return;
	}

	dir_close(id);
	id = 0;
}

int DirAccessJAndroid::get_drive_count() {
	if (_get_drive_count) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallIntMethod(dir_access_handler, _get_drive_count, get_access_type());
	} else {
		return 0;
	}
}

String DirAccessJAndroid::get_drive(int p_drive) {
	if (_get_drive) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, "");
		jstring j_drive = (jstring)env->CallObjectMethod(dir_access_handler, _get_drive, get_access_type(), p_drive);
		if (!j_drive) {
			return "";
		}

		String drive = jstring_to_string(j_drive, env);
		env->DeleteLocalRef(j_drive);
		return drive;
	} else {
		return "";
	}
}

String DirAccessJAndroid::_get_root_string() const {
	if (get_access_type() == ACCESS_FILESYSTEM) {
		return "/";
	}
	return DirAccessUnix::_get_root_string();
}

String DirAccessJAndroid::get_current_dir(bool p_include_drive) const {
	String base = _get_root_path();
	String bd = current_dir;
	if (!base.is_empty()) {
		bd = current_dir.replace_first(base, "");
	}

	String root_string = _get_root_string();
	if (bd.begins_with(root_string)) {
		return bd;
	} else if (bd.begins_with("/")) {
		return root_string + bd.substr(1);
	} else {
		return root_string + bd;
	}
}

Error DirAccessJAndroid::change_dir(String p_dir) {
	String new_dir = get_absolute_path(p_dir);
	if (new_dir == current_dir) {
		return OK;
	}

	if (!dir_exists(new_dir)) {
		return ERR_INVALID_PARAMETER;
	}

	current_dir = new_dir;
	return OK;
}

String DirAccessJAndroid::get_absolute_path(String p_path) {
	if (current_dir != "" && p_path == current_dir) {
		return current_dir;
	}

	if (p_path.is_relative_path()) {
		p_path = get_current_dir().path_join(p_path);
	}

	p_path = fix_path(p_path);
	p_path = p_path.simplify_path();
	return p_path;
}

bool DirAccessJAndroid::file_exists(String p_file) {
	if (_file_exists) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		String path = get_absolute_path(p_file);
		jstring j_path = env->NewStringUTF(path.utf8().get_data());
		bool result = env->CallBooleanMethod(dir_access_handler, _file_exists, get_access_type(), j_path);
		env->DeleteLocalRef(j_path);
		return result;
	} else {
		return false;
	}
}

bool DirAccessJAndroid::dir_exists(String p_dir) {
	if (_dir_exists) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		String path = get_absolute_path(p_dir);
		jstring j_path = env->NewStringUTF(path.utf8().get_data());
		bool result = env->CallBooleanMethod(dir_access_handler, _dir_exists, get_access_type(), j_path);
		env->DeleteLocalRef(j_path);
		return result;
	} else {
		return false;
	}
}

Error DirAccessJAndroid::make_dir(String p_dir) {
	// Check if the directory exists already
	if (dir_exists(p_dir)) {
		return ERR_ALREADY_EXISTS;
	}

	if (_make_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		String path = get_absolute_path(p_dir);
		jstring j_dir = env->NewStringUTF(path.utf8().get_data());
		bool result = env->CallBooleanMethod(dir_access_handler, _make_dir, get_access_type(), j_dir);
		env->DeleteLocalRef(j_dir);
		if (result) {
			return OK;
		} else {
			return FAILED;
		}
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error DirAccessJAndroid::make_dir_recursive(const String &p_dir) {
	Error err = make_dir(p_dir);
	if (err != OK && err != ERR_ALREADY_EXISTS) {
		ERR_FAIL_V_MSG(err, "Could not create directory: " + p_dir);
	}
	return OK;
}

Error DirAccessJAndroid::rename(String p_from, String p_to) {
	if (_rename) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		String from_path = get_absolute_path(p_from);
		jstring j_from = env->NewStringUTF(from_path.utf8().get_data());

		String to_path = get_absolute_path(p_to);
		jstring j_to = env->NewStringUTF(to_path.utf8().get_data());

		bool result = env->CallBooleanMethod(dir_access_handler, _rename, get_access_type(), j_from, j_to);
		env->DeleteLocalRef(j_from);
		env->DeleteLocalRef(j_to);
		if (result) {
			return OK;
		} else {
			return FAILED;
		}
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error DirAccessJAndroid::remove(String p_name) {
	if (_remove) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		String path = get_absolute_path(p_name);
		jstring j_name = env->NewStringUTF(path.utf8().get_data());
		bool result = env->CallBooleanMethod(dir_access_handler, _remove, get_access_type(), j_name);
		env->DeleteLocalRef(j_name);
		if (result) {
			return OK;
		} else {
			return FAILED;
		}
	} else {
		return ERR_UNCONFIGURED;
	}
}

uint64_t DirAccessJAndroid::get_space_left() {
	if (_get_space_left) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallLongMethod(dir_access_handler, _get_space_left, get_access_type());
	} else {
		return 0;
	}
}

void DirAccessJAndroid::setup(jobject p_dir_access_handler) {
	JNIEnv *env = get_jni_env();
	dir_access_handler = env->NewGlobalRef(p_dir_access_handler);

	jclass c = env->GetObjectClass(dir_access_handler);
	cls = (jclass)env->NewGlobalRef(c);

	_dir_open = env->GetMethodID(cls, "dirOpen", "(ILjava/lang/String;)I");
	_dir_next = env->GetMethodID(cls, "dirNext", "(I)Ljava/lang/String;");
	_dir_close = env->GetMethodID(cls, "dirClose", "(I)V");
	_dir_is_dir = env->GetMethodID(cls, "dirIsDir", "(I)Z");
	_dir_exists = env->GetMethodID(cls, "dirExists", "(ILjava/lang/String;)Z");
	_file_exists = env->GetMethodID(cls, "fileExists", "(ILjava/lang/String;)Z");
	_get_drive_count = env->GetMethodID(cls, "getDriveCount", "(I)I");
	_get_drive = env->GetMethodID(cls, "getDrive", "(II)Ljava/lang/String;");
	_make_dir = env->GetMethodID(cls, "makeDir", "(ILjava/lang/String;)Z");
	_get_space_left = env->GetMethodID(cls, "getSpaceLeft", "(I)J");
	_rename = env->GetMethodID(cls, "rename", "(ILjava/lang/String;Ljava/lang/String;)Z");
	_remove = env->GetMethodID(cls, "remove", "(ILjava/lang/String;)Z");
	_current_is_hidden = env->GetMethodID(cls, "isCurrentHidden", "(I)Z");
}

void DirAccessJAndroid::terminate() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(cls);
	env->DeleteGlobalRef(dir_access_handler);
}

DirAccessJAndroid::DirAccessJAndroid() {
}

DirAccessJAndroid::~DirAccessJAndroid() {
	list_dir_end();
}

int DirAccessJAndroid::dir_open(String p_path) {
	if (_dir_open) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);

		String path = get_absolute_path(p_path);
		jstring js = env->NewStringUTF(path.utf8().get_data());
		int dirId = env->CallIntMethod(dir_access_handler, _dir_open, get_access_type(), js);
		env->DeleteLocalRef(js);
		return dirId;
	} else {
		return 0;
	}
}

void DirAccessJAndroid::dir_close(int p_id) {
	if (_dir_close) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(dir_access_handler, _dir_close, p_id);
	}
}


// --- Merged from platform/android/tts_android.cpp ---
#include "servers/display/display_server.h"


#include "core/os/os.h"
#include "servers/display/display_server.h"

bool TTS_Android::initialized = false;
jobject TTS_Android::tts = nullptr;
jclass TTS_Android::cls = nullptr;

Thread TTS_Android::init_thread;
SafeFlag TTS_Android::quit_request;
SafeFlag TTS_Android::init_done;

jmethodID TTS_Android::_init = nullptr;
jmethodID TTS_Android::_is_speaking = nullptr;
jmethodID TTS_Android::_is_paused = nullptr;
jmethodID TTS_Android::_get_state = nullptr;
jmethodID TTS_Android::_get_voices = nullptr;
jmethodID TTS_Android::_speak = nullptr;
jmethodID TTS_Android::_pause_speaking = nullptr;
jmethodID TTS_Android::_resume_speaking = nullptr;
jmethodID TTS_Android::_stop_speaking = nullptr;

HashMap<int64_t, Char16String> TTS_Android::ids;

void TTS_Android::_thread_function(void *self) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->CallVoidMethod(tts, _init);

	uint64_t sleep = 200;
	while (env->CallIntMethod(tts, _get_state) == INIT_STATE_UNKNOWN && !quit_request.is_set()) {
		OS::delay_usec(1000 * sleep);
	}
	init_done.set();
}

void TTS_Android::initialize_tts(bool p_wait) {
	if (!_init || !_get_state || !tts) {
		return;
	}
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	if (!init_thread.is_started() && !init_done.is_set()) {
		init_thread.start(TTS_Android::_thread_function, nullptr);
	}

	if (env->CallIntMethod(tts, _get_state) == INIT_STATE_SUCCESS) {
		initialized = true;
		return;
	}

	// If it's not initialized at launch wait for 1 second for TTS init.
	if (p_wait) {
		uint64_t sleep = 200;
		uint64_t wait = 1000000;
		uint64_t time = OS::get_ticks_usec();
		while (OS::get_ticks_usec() - time < wait) {
			OS::delay_usec(1000 * sleep);
			if (init_done.is_set()) {
				break;
			}
		}
	}

	if (env->CallIntMethod(tts, _get_state) == INIT_STATE_SUCCESS) {
		initialized = true;
	}
}

void TTS_Android::setup(jobject p_tts) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	tts = env->NewGlobalRef(p_tts);
	quit_request.clear();
	init_done.clear();

	jclass c = env->GetObjectClass(tts);
	cls = (jclass)env->NewGlobalRef(c);

	_init = env->GetMethodID(cls, "init", "()V");
	_is_speaking = env->GetMethodID(cls, "isSpeaking", "()Z");
	_is_paused = env->GetMethodID(cls, "isPaused", "()Z");
	_get_state = env->GetMethodID(cls, "getState", "()I");
	_get_voices = env->GetMethodID(cls, "getVoices", "()[Ljava/lang/String;");
	_speak = env->GetMethodID(cls, "speak", "(Ljava/lang/String;Ljava/lang/String;IFFJZ)V");
	_pause_speaking = env->GetMethodID(cls, "pauseSpeaking", "()V");
	_resume_speaking = env->GetMethodID(cls, "resumeSpeaking", "()V");
	_stop_speaking = env->GetMethodID(cls, "stopSpeaking", "()V");

	bool tts_enabled = GLOBAL_GET("audio/general/text_to_speech");
	if (tts_enabled) {
		initialize_tts(false);
	}
}

void TTS_Android::terminate() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	if (init_thread.is_started()) {
		quit_request.set();
		init_thread.wait_to_finish();
	}

	if (cls) {
		env->DeleteGlobalRef(cls);
	}
	if (tts) {
		env->DeleteGlobalRef(tts);
	}
}

void TTS_Android::_java_utterance_callback(int p_event, int64_t p_id, int p_pos) {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_MSG(!initialized || tts == nullptr, "Text to Speech not initialized.");
	if (ids.has(p_id)) {
		int pos = 0;
		if ((DisplayServerEnums::TTSUtteranceEvent)p_event == DisplayServerEnums::TTS_UTTERANCE_BOUNDARY) {
			// Convert position from UTF-16 to UTF-32.
			const Char16String &string = ids[p_id];
			for (int i = 0; i < MIN(p_pos, string.length()); i++) {
				char16_t c = string[i];
				if ((c & 0xfffffc00) == 0xd800) {
					i++;
				}
				pos++;
			}
		} else if ((DisplayServerEnums::TTSUtteranceEvent)p_event != DisplayServerEnums::TTS_UTTERANCE_STARTED) {
			ids.erase(p_id);
		}
		DisplayServer::tts_post_utterance_event((DisplayServerEnums::TTSUtteranceEvent)p_event, p_id, pos);
	}
}

bool TTS_Android::is_speaking() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_V_MSG(!initialized || tts == nullptr, false, "Text to Speech not initialized.");
	if (_is_speaking) {
		JNIEnv *env = get_jni_env();

		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(tts, _is_speaking);
	} else {
		return false;
	}
}

bool TTS_Android::is_paused() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_V_MSG(!initialized || tts == nullptr, false, "Text to Speech not initialized.");
	if (_is_paused) {
		JNIEnv *env = get_jni_env();

		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(tts, _is_paused);
	} else {
		return false;
	}
}

Array TTS_Android::get_voices() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_V_MSG(!initialized || tts == nullptr, Array(), "Text to Speech not initialized.");
	Array list;
	if (_get_voices) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, list);

		jobject voices_object = env->CallObjectMethod(tts, _get_voices);
		jobjectArray *arr = reinterpret_cast<jobjectArray *>(&voices_object);

		jsize len = env->GetArrayLength(*arr);
		for (int i = 0; i < len; i++) {
			jstring jStr = (jstring)env->GetObjectArrayElement(*arr, i);
			String str = jstring_to_string(jStr, env);
			Vector<String> tokens = str.split(";", true, 2);
			if (tokens.size() == 2) {
				Dictionary voice_d;
				voice_d["name"] = tokens[1];
				voice_d["id"] = tokens[1];
				voice_d["language"] = tokens[0];
				list.push_back(voice_d);
			}
			env->DeleteLocalRef(jStr);
		}
	}
	return list;
}

void TTS_Android::speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_MSG(!initialized || tts == nullptr, "Text to Speech not initialized.");
	if (p_interrupt) {
		stop();
	}

	if (p_text.is_empty()) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_utterance_id);
		return;
	}

	ids[p_utterance_id] = p_text.utf16();

	if (_speak) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring jStrT = env->NewStringUTF(p_text.utf8().get_data());
		jstring jStrV = env->NewStringUTF(p_voice.utf8().get_data());
		env->CallVoidMethod(tts, _speak, jStrT, jStrV, CLAMP(p_volume, 0, 100), CLAMP(p_pitch, 0.f, 2.f), CLAMP(p_rate, 0.1f, 10.f), p_utterance_id, p_interrupt);
		env->DeleteLocalRef(jStrT);
		env->DeleteLocalRef(jStrV);
	}
}

void TTS_Android::pause() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_MSG(!initialized || tts == nullptr, "Text to Speech not initialized.");
	if (_pause_speaking) {
		JNIEnv *env = get_jni_env();

		ERR_FAIL_NULL(env);
		env->CallVoidMethod(tts, _pause_speaking);
	}
}

void TTS_Android::resume() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_MSG(!initialized || tts == nullptr, "Text to Speech not initialized.");
	if (_resume_speaking) {
		JNIEnv *env = get_jni_env();

		ERR_FAIL_NULL(env);
		env->CallVoidMethod(tts, _resume_speaking);
	}
}

void TTS_Android::stop() {
	if (unlikely(!initialized)) {
		initialize_tts();
	}
	ERR_FAIL_COND_MSG(!initialized || tts == nullptr, "Text to Speech not initialized.");
	for (const KeyValue<int64_t, Char16String> &E : ids) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, E.key);
	}
	ids.clear();

	if (_stop_speaking) {
		JNIEnv *env = get_jni_env();

		ERR_FAIL_NULL(env);
		env->CallVoidMethod(tts, _stop_speaking);
	}
}


// --- Merged from platform/android/thread_jandroid.cpp ---
#include "servers/display/display_server.h"

#include "core/os/mutex.h"

#include <android/log.h>

static JavaVM *java_vm = nullptr;
static thread_local JNIEnv *env = nullptr;

// The logic here need to improve, init_thread/term_tread are designed to work with Thread::callback
// Calling init_thread from setup_android_thread and get_jni_env to setup an env we're keeping and not detaching
// could cause issues on app termination.
//
// We should be making sure that any thread started calls a nice cleanup function when it's done,
// especially now that we use many more threads.

static void init_thread() {
	if (env) {
		// thread never detached! just keep using...
		return;
	}

	java_vm->AttachCurrentThread(&env, nullptr);
}

static void term_thread() {
	java_vm->DetachCurrentThread();

	// this is no longer valid, must called init_thread to re-establish
	env = nullptr;
}

void init_thread_jandroid(JavaVM *p_jvm, JNIEnv *p_env) {
	java_vm = p_jvm;
	env = p_env;
	Thread::_set_platform_functions({ .init = init_thread, .term = &term_thread });
}

void setup_android_thread() {
	if (!env) {
		// !BAS! see remarks above
		init_thread();
	}
}

JNIEnv *get_jni_env() {
	if (!env) {
		// !BAS! see remarks above
		init_thread();
	}

	return env;
}


// --- Merged from platform/android/net_socket_android.cpp ---
#include "servers/display/display_server.h"


jobject NetSocketAndroid::net_utils = nullptr;
jclass NetSocketAndroid::cls = nullptr;
jmethodID NetSocketAndroid::_multicast_lock_acquire = nullptr;
jmethodID NetSocketAndroid::_multicast_lock_release = nullptr;

void NetSocketAndroid::setup(jobject p_net_utils) {
	JNIEnv *env = get_jni_env();

	net_utils = env->NewGlobalRef(p_net_utils);

	jclass c = env->GetObjectClass(net_utils);
	cls = (jclass)env->NewGlobalRef(c);

	_multicast_lock_acquire = env->GetMethodID(cls, "multicastLockAcquire", "()V");
	_multicast_lock_release = env->GetMethodID(cls, "multicastLockRelease", "()V");
}

void NetSocketAndroid::terminate() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(cls);
	env->DeleteGlobalRef(net_utils);
}

void NetSocketAndroid::multicast_lock_acquire() {
	if (_multicast_lock_acquire) {
		JNIEnv *env = get_jni_env();
		env->CallVoidMethod(net_utils, _multicast_lock_acquire);
	}
}

void NetSocketAndroid::multicast_lock_release() {
	if (_multicast_lock_release) {
		JNIEnv *env = get_jni_env();
		env->CallVoidMethod(net_utils, _multicast_lock_release);
	}
}

Ref<NetSocket> NetSocketAndroid::_create_func() {
	return memnew(NetSocketAndroid);
}

void NetSocketAndroid::make_default() {
	_create = _create_func;
}

NetSocketAndroid::~NetSocketAndroid() {
	close();
}

void NetSocketAndroid::close() {
	NetSocketUnix::close();
	if (wants_broadcast) {
		multicast_lock_release();
	}
	if (multicast_groups) {
		multicast_lock_release();
	}
	wants_broadcast = false;
	multicast_groups = 0;
}

Error NetSocketAndroid::set_broadcasting_enabled(bool p_enabled) {
	Error err = NetSocketUnix::set_broadcasting_enabled(p_enabled);
	if (err != OK) {
		return err;
	}

	if (p_enabled != wants_broadcast) {
		if (p_enabled) {
			multicast_lock_acquire();
		} else {
			multicast_lock_release();
		}

		wants_broadcast = p_enabled;
	}

	return OK;
}

Error NetSocketAndroid::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	Error err = NetSocketUnix::join_multicast_group(p_multi_address, p_if_name);
	if (err != OK) {
		return err;
	}

	if (!multicast_groups) {
		multicast_lock_acquire();
	}
	multicast_groups++;

	return OK;
}

Error NetSocketAndroid::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	Error err = NetSocketUnix::leave_multicast_group(p_multi_address, p_if_name);
	if (err != OK) {
		return err;
	}

	ERR_FAIL_COND_V(multicast_groups == 0, ERR_BUG);

	multicast_groups--;
	if (!multicast_groups) {
		multicast_lock_release();
	}

	return OK;
}

// === Merged from platform/android/java_class_wrapper.cpp ===
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

#include "servers/display/display_server.h"


bool JavaClass::_call_method(JavaObject *p_instance, const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error, Variant &ret) {
	HashMap<String, List<MethodInfo>>::Iterator M = methods.find(p_method);
	if (!M) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return false;
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, false);

	env->PushLocalFrame(p_argcount);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return false;
	}

	MethodInfo *method = nullptr;
	for (MethodInfo &E : M->value) {
		if (!p_instance && !E._static && !E._constructor) {
			r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
			continue;
		}

		int pc = E.param_types.size();
		if (p_argcount < pc) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.expected = pc;
			continue;
		}
		if (p_argcount > pc) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.expected = pc;
			continue;
		}
		uint32_t *ptypes = E.param_types.ptrw();
		bool valid = true;

		for (int i = 0; i < pc; i++) {
			Variant::Type arg_expected = Variant::NIL;
			switch (ptypes[i]) {
				case ARG_TYPE_VOID: {
					//bug?
				} break;
				case ARG_TYPE_BOOLEAN: {
					if (p_args[i]->get_type() != Variant::BOOL) {
						arg_expected = Variant::BOOL;
					}
				} break;
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BYTE:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_CHAR:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_SHORT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_INT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_LONG:
				case ARG_TYPE_BYTE:
				case ARG_TYPE_CHAR:
				case ARG_TYPE_SHORT:
				case ARG_TYPE_INT:
				case ARG_TYPE_LONG: {
					if (!p_args[i]->is_num()) {
						arg_expected = Variant::INT;
					}
				} break;
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_FLOAT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_DOUBLE:
				case ARG_TYPE_FLOAT:
				case ARG_TYPE_DOUBLE: {
					if (!p_args[i]->is_num()) {
						arg_expected = Variant::FLOAT;
					}
				} break;
				case ARG_TYPE_STRING:
				case ARG_TYPE_CHARSEQUENCE: {
					if (!p_args[i]->is_string()) {
						arg_expected = Variant::STRING;
					}
				} break;
				case ARG_TYPE_CALLABLE: {
					if (p_args[i]->get_type() != Variant::CALLABLE) {
						arg_expected = Variant::CALLABLE;
					}
				} break;
				case ARG_TYPE_CLASS: {
					String cn = E.param_sigs[i].string();
					if (cn.begins_with("L") && cn.ends_with(";")) {
						cn = cn.substr(1, cn.length() - 2);
					}
					if (cn == "org/godotengine/godot/Dictionary") {
						if (p_args[i]->get_type() != Variant::DICTIONARY) {
							arg_expected = Variant::DICTIONARY;
						}
					} else if (p_args[i]->get_type() != Variant::OBJECT && p_args[i]->get_type() != Variant::NIL) {
						arg_expected = Variant::OBJECT;
					} else {
						Ref<RefCounted> ref = *p_args[i];
						if (ref.is_valid()) {
							if (Object::cast_to<JavaObject>(ref.ptr())) {
								Ref<JavaObject> jo = ref;
								jclass c = jni_find_class(env, cn.utf8().get_data());
								if (!c || !env->IsInstanceOf(jo->instance, c)) {
									arg_expected = Variant::OBJECT;
								} else {
									//ok
								}
							} else {
								arg_expected = Variant::OBJECT;
							}
						}
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::BOOL) {
							arg_expected = Variant::ARRAY;
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_BYTE:
				case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
					if (p_args[i]->get_type() != Variant::PACKED_BYTE_ARRAY) {
						arg_expected = Variant::PACKED_BYTE_ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_SHORT:
				case ARG_ARRAY_BIT | ARG_TYPE_INT: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::INT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_INT32_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::INT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_INT64_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::FLOAT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_FLOAT32_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::FLOAT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_FLOAT64_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_STRING:
				case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && (arr.get_typed_builtin() != Variant::STRING && arr.get_typed_builtin() != Variant::STRING_NAME)) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_STRING_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::CALLABLE) {
							arg_expected = Variant::ARRAY;
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::OBJECT) {
							arg_expected = Variant::ARRAY;
						} else {
							String cn = E.param_sigs[i].string();
							if (cn.begins_with("[L") && cn.ends_with(";")) {
								cn = cn.substr(2, cn.length() - 3);
							}
							jclass c = jni_find_class(env, cn.utf8().get_data());
							if (c) {
								for (int j = 0; j < arr.size(); j++) {
									Ref<JavaObject> jo = arr[j];
									if (jo.is_valid()) {
										if (!env->IsInstanceOf(jo->instance, c)) {
											arg_expected = Variant::ARRAY;
											break;
										}
									} else {
										arg_expected = Variant::ARRAY;
										break;
									}
								}
							} else {
								arg_expected = Variant::ARRAY;
							}
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				default: {
					if (p_args[i]->get_type() != Variant::ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
			}

			if (arg_expected != Variant::NIL) {
				r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
				r_error.argument = i;
				r_error.expected = arg_expected;
				valid = false;
				break;
			}
		}
		if (!valid) {
			continue;
		}

		method = &E;
		break;
	}

	if (!method) {
		if (r_error.error == Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL) {
			ERR_PRINT(vformat(R"(Cannot call static function "%s" on Java class "%s" directly. Make an instance instead.)", p_method, java_class_name));
		}
		return true; //no version convinces
	}

	r_error.error = Callable::CallError::CALL_OK;

	jvalue *argv = nullptr;

	if (method->param_types.size()) {
		argv = (jvalue *)alloca(sizeof(jvalue) * method->param_types.size());
	}

	for (int i = 0; i < method->param_types.size(); i++) {
		switch (method->param_types[i]) {
			case ARG_TYPE_VOID: {
				//can't happen
				argv[i].l = nullptr; //I hope this works
			} break;

			case ARG_TYPE_BOOLEAN: {
				argv[i].z = *p_args[i];
			} break;
			case ARG_TYPE_BYTE: {
				argv[i].b = *p_args[i];
			} break;
			case ARG_TYPE_CHAR: {
				argv[i].c = *p_args[i];
			} break;
			case ARG_TYPE_SHORT: {
				argv[i].s = *p_args[i];
			} break;
			case ARG_TYPE_INT: {
				argv[i].i = *p_args[i];
			} break;
			case ARG_TYPE_LONG: {
				argv[i].j = (int64_t)*p_args[i];
			} break;
			case ARG_TYPE_FLOAT: {
				argv[i].f = *p_args[i];
			} break;
			case ARG_TYPE_DOUBLE: {
				argv[i].d = *p_args[i];
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BOOLEAN: {
				jclass bclass = jni_find_class(env, "java/lang/Boolean");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(Z)V");
				jvalue val;
				val.z = (bool)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BYTE: {
				jclass bclass = jni_find_class(env, "java/lang/Byte");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(B)V");
				jvalue val;
				val.b = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_CHAR: {
				jclass bclass = jni_find_class(env, "java/lang/Character");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(C)V");
				jvalue val;
				val.c = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_SHORT: {
				jclass bclass = jni_find_class(env, "java/lang/Short");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(S)V");
				jvalue val;
				val.s = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_INT: {
				jclass bclass = jni_find_class(env, "java/lang/Integer");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(I)V");
				jvalue val;
				val.i = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_LONG: {
				jclass bclass = jni_find_class(env, "java/lang/Long");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(J)V");
				jvalue val;
				val.j = (int64_t)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_FLOAT: {
				jclass bclass = jni_find_class(env, "java/lang/Float");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(F)V");
				jvalue val;
				val.f = (float)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_DOUBLE: {
				jclass bclass = jni_find_class(env, "java/lang/Double");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(D)V");
				jvalue val;
				val.d = (double)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_TYPE_STRING:
			case ARG_TYPE_CHARSEQUENCE: {
				String s = *p_args[i];
				jstring jStr = env->NewStringUTF(s.utf8().get_data());
				argv[i].l = jStr;
			} break;
			case ARG_TYPE_CALLABLE: {
				jobject jcallable = callable_to_jcallable(env, *p_args[i]);
				argv[i].l = jcallable;
			} break;
			case ARG_TYPE_CLASS: {
				if (p_args[i]->get_type() == Variant::DICTIONARY) {
					argv[i].l = _variant_to_jobject(env, Variant::DICTIONARY, p_args[i]);
				} else {
					Ref<JavaObject> jo = *p_args[i];
					if (jo.is_valid()) {
						argv[i].l = jo->instance;
					} else {
						argv[i].l = nullptr; //I hope this works
					}
				}
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
				Array arr = *p_args[i];
				jbooleanArray a = env->NewBooleanArray(arr.size());
				for (int j = 0; j < arr.size(); j++) {
					jboolean val = arr[j];
					env->SetBooleanArrayRegion(a, j, 1, &val);
				}
				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
				jbyteArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewByteArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jbyte val = arr[j];
						env->SetByteArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_BYTE_ARRAY) {
					PackedByteArray arr = *p_args[i];
					a = env->NewByteArray(arr.size());
					env->SetByteArrayRegion(a, 0, arr.size(), (const jbyte *)arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
				jcharArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewCharArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jchar val = arr[j];
						env->SetCharArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_BYTE_ARRAY) {
					PackedByteArray arr = *p_args[i];
					// The data is expected to be UTF-16 encoded, so the length is half the size of the byte array.
					int size = arr.size() / 2;
					a = env->NewCharArray(size);
					env->SetCharArrayRegion(a, 0, size, (const jchar *)arr.ptr());
				}

				argv[i].l = a;

			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
				jshortArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewShortArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jshort val = arr[j];
						env->SetShortArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT32_ARRAY) {
					PackedInt32Array arr = *p_args[i];
					a = env->NewShortArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jshort val = arr[j];
						env->SetShortArrayRegion(a, j, 1, &val);
					}
				}

				argv[i].l = a;

			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_INT: {
				jintArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewIntArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jint val = arr[j];
						env->SetIntArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT32_ARRAY) {
					PackedInt32Array arr = *p_args[i];
					a = env->NewIntArray(arr.size());
					env->SetIntArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
				jlongArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewLongArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jlong val = (int64_t)arr[j];
						env->SetLongArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT64_ARRAY) {
					PackedInt64Array arr = *p_args[i];
					a = env->NewLongArray(arr.size());
					env->SetLongArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
				jfloatArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewFloatArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jfloat val = arr[j];
						env->SetFloatArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_FLOAT32_ARRAY) {
					PackedFloat32Array arr = *p_args[i];
					a = env->NewFloatArray(arr.size());
					env->SetFloatArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
				jdoubleArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewDoubleArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jdouble val = arr[j];
						env->SetDoubleArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_FLOAT64_ARRAY) {
					PackedFloat64Array arr = *p_args[i];
					a = env->NewDoubleArray(arr.size());
					env->SetDoubleArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_STRING:
			case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
				jobjectArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewObjectArray(arr.size(), jni_find_class(env, "java/lang/String"), nullptr);
					for (int j = 0; j < arr.size(); j++) {
						String s = arr[j];
						jstring jStr = env->NewStringUTF(s.utf8().get_data());
						env->SetObjectArrayElement(a, j, jStr);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_STRING_ARRAY) {
					PackedStringArray arr = *p_args[i];
					a = env->NewObjectArray(arr.size(), jni_find_class(env, "java/lang/String"), nullptr);
					for (int j = 0; j < arr.size(); j++) {
						String s = arr[j];
						jstring jStr = env->NewStringUTF(s.utf8().get_data());
						env->SetObjectArrayElement(a, j, jStr);
					}
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
				Array arr = *p_args[i];
				jobjectArray jarr = env->NewObjectArray(arr.size(), jni_find_class(env, "org/godotengine/godot/variant/Callable"), nullptr);
				for (int j = 0; j < arr.size(); j++) {
					Variant callable = arr[j];
					jobject jcallable = callable_to_jcallable(env, callable);
					env->SetObjectArrayElement(jarr, j, jcallable);
				}

				argv[i].l = jarr;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
				String cn = method->param_sigs[i].string();
				if (cn.begins_with("[L") && cn.ends_with(";")) {
					cn = cn.substr(2, cn.length() - 3);
				}
				jclass c = jni_find_class(env, cn.utf8().get_data());
				if (c) {
					Array arr = *p_args[i];
					jobjectArray jarr = env->NewObjectArray(arr.size(), c, nullptr);
					for (int j = 0; j < arr.size(); j++) {
						Ref<JavaObject> jo = arr[j];
						env->SetObjectArrayElement(jarr, j, jo->instance);
					}

					argv[i].l = jarr;
				}
			} break;
		}
	}

	r_error.error = Callable::CallError::CALL_OK;
	bool success = true;

	switch (method->return_type) {
		case ARG_TYPE_VOID: {
			if (method->_static) {
				env->CallStaticVoidMethodA(_class, method->method, argv);
			} else {
				env->CallVoidMethodA(p_instance->instance, method->method, argv);
			}
			ret = Variant();

		} break;
		case ARG_TYPE_BOOLEAN: {
			if (method->_static) {
				ret = env->CallStaticBooleanMethodA(_class, method->method, argv);
			} else {
				ret = env->CallBooleanMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_BYTE: {
			if (method->_static) {
				ret = env->CallStaticByteMethodA(_class, method->method, argv);
			} else {
				ret = env->CallByteMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_CHAR: {
			if (method->_static) {
				ret = env->CallStaticCharMethodA(_class, method->method, argv);
			} else {
				ret = env->CallCharMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_SHORT: {
			if (method->_static) {
				ret = env->CallStaticShortMethodA(_class, method->method, argv);
			} else {
				ret = env->CallShortMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_INT: {
			if (method->_static) {
				ret = env->CallStaticIntMethodA(_class, method->method, argv);
			} else {
				ret = env->CallIntMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_LONG: {
			if (method->_static) {
				ret = (int64_t)env->CallStaticLongMethodA(_class, method->method, argv);
			} else {
				ret = (int64_t)env->CallLongMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_FLOAT: {
			if (method->_static) {
				ret = env->CallStaticFloatMethodA(_class, method->method, argv);
			} else {
				ret = env->CallFloatMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_DOUBLE: {
			if (method->_static) {
				ret = env->CallStaticDoubleMethodA(_class, method->method, argv);
			} else {
				ret = env->CallDoubleMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		default: {
			jobject obj;
			if (method->_constructor) {
				obj = env->NewObjectA(_class, method->method, argv);
			} else if (method->_static) {
				obj = env->CallStaticObjectMethodA(_class, method->method, argv);
			} else {
				obj = env->CallObjectMethodA(p_instance->instance, method->method, argv);
			}

			if (!obj) {
				ret = Variant();
			} else {
				if (!_convert_object_to_variant(env, obj, ret, method->return_type)) {
					ret = Variant();
					r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
					success = false;
				}
			}

		} break;
	}

	jobject exception = env->ExceptionOccurred();
	if (exception) {
		env->ExceptionClear();

		jclass java_class = env->GetObjectClass(exception);
		Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);

		JavaClassWrapper::data->exception.instantiate(java_class_wrapped, exception);
	} else {
		JavaClassWrapper::data->exception.unref();
	}

	env->PopLocalFrame(nullptr);

	return success;
}

bool JavaClass::_get(const String &p_name, Variant &r_ret) const {
	if (constant_map.has(p_name)) {
		r_ret = constant_map[p_name];
		return true;
	}

	return false;
}

Variant JavaClass::callp(const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	// Godot methods take precedence.
	Variant ret = RefCounted::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	String method = (p_method == java_constructor_name) ? "<init>" : p_method;
	bool found = _call_method(nullptr, method, p_args, p_argcount, r_error, ret);
	if (found) {
		return ret;
	}

	return Variant();
}

String JavaClass::get_java_class_name() const {
	return java_class_name;
}

TypedArray<Dictionary> JavaClass::get_java_method_list() const {
	TypedArray<Dictionary> method_list;

	for (const KeyValue<String, List<MethodInfo>> &item : methods) {
		for (const MethodInfo &mi : item.value) {
			Dictionary method;

			method["name"] = mi._constructor ? java_constructor_name : String(item.key);
			method["id"] = (uint64_t)mi.method;
			method["default_args"] = Array();
			method["flags"] = METHOD_FLAGS_DEFAULT & (mi._static || mi._constructor ? METHOD_FLAG_STATIC : METHOD_FLAG_NORMAL);

			{
				Array a;

				for (uint32_t argtype : mi.param_types) {
					Dictionary d;

					Variant::Type t = Variant::NIL;
					float likelihood = 0.0;
					_convert_to_variant_type(argtype, t, likelihood);
					d["type"] = t;
					if (t == Variant::OBJECT) {
						d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
						d["hint_string"] = "JavaObject";
					} else {
						d["hint"] = 0;
						d["hint_string"] = "";
					}

					a.push_back(d);
				}

				method["args"] = a;
			}

			{
				Dictionary d;

				if (mi._constructor) {
					d["type"] = Variant::OBJECT;
					d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
					d["hint_string"] = "JavaObject";
				} else {
					Variant::Type t = Variant::NIL;
					float likelihood = 0.0;
					_convert_to_variant_type(mi.return_type, t, likelihood);
					d["type"] = t;
					if (t == Variant::OBJECT) {
						d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
						d["hint_string"] = "JavaObject";
					} else {
						d["hint"] = 0;
						d["hint_string"] = "";
					}
				}

				method["return_type"] = d;
			}

			method_list.push_back(method);
		}
	}

	return method_list;
}

Ref<JavaClass> JavaClass::get_java_parent_class() const {
	ERR_FAIL_NULL_V(_class, Ref<JavaClass>());

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jclass superclass = (jclass)env->CallObjectMethod(_class, JavaClassWrapper::data->Class_getSuperclass);
	if (!superclass) {
		return Ref<JavaClass>();
	}

	Ref<JavaClass> ret = JavaClassWrapper::wrap_jclass(superclass);
	env->DeleteLocalRef(superclass);
	return ret;
}

String JavaClass::_to_string() {
	return "<JavaClass:" + java_class_name + ">";
}

bool JavaClass::has_java_method(const String &p_method) const {
	String method = (p_method == java_constructor_name) ? "<init>" : p_method;
	return methods.has(method);
}

JavaClass::JavaClass() {
}

JavaClass::~JavaClass() {
	if (_class) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->DeleteGlobalRef(_class);
	}
}

/////////////////////

Variant JavaObject::callp(const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	// Godot methods take precedence.
	Variant ret = RefCounted::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	if (instance) {
		Ref<JavaClass> c = base_class;
		while (c.is_valid()) {
			bool found = c->_call_method(this, p_method, p_args, p_argcount, r_error, ret);
			if (found) {
				return ret;
			}
			c = c->get_java_parent_class();
		}
	}

	return Variant();
}

Ref<JavaClass> JavaObject::get_java_class() const {
	return base_class;
}

String JavaObject::_to_string() {
	if (base_class.is_valid() && instance) {
		return "<JavaObject:" + base_class->java_class_name + " \"" + (String)call("toString") + "\">";
	}
	return RefCounted::_to_string();
}

bool JavaObject::has_java_method(const String &p_method) const {
	if (instance) {
		Ref<JavaClass> c = base_class;
		while (c.is_valid()) {
			if (c->has_java_method(p_method)) {
				return true;
			}
			c = c->get_java_parent_class();
		}
	}
	return false;
}

JavaObject::JavaObject() {
}

JavaObject::JavaObject(const Ref<JavaClass> &p_base, jobject p_instance) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	base_class = p_base;
	instance = env->NewGlobalRef(p_instance);
}

JavaObject::~JavaObject() {
	if (instance) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->DeleteGlobalRef(instance);
	}
}

////////////////////

bool JavaClassWrapper::_get_type_sig(JNIEnv *env, jobject obj, uint32_t &sig, String &strsig) {
	jstring name2 = (jstring)env->CallObjectMethod(obj, data->Class_getName);
	String str_type = jstring_to_string(name2, env);
	env->DeleteLocalRef(name2);
	uint32_t t = 0;

	if (str_type.begins_with("[")) {
		t = JavaClass::ARG_ARRAY_BIT;
		strsig = "[";
		str_type = str_type.substr(1);
		if (str_type.begins_with("[")) {
			print_line("Nested arrays not supported for type: " + str_type);
			return false;
		}
		if (str_type.begins_with("L")) {
			str_type = str_type.substr(1, str_type.length() - 2); //ok it's a class
		}
	}

	if (str_type == "void" || str_type == "V") {
		t |= JavaClass::ARG_TYPE_VOID;
		strsig += "V";
	} else if (str_type == "boolean" || str_type == "Z") {
		t |= JavaClass::ARG_TYPE_BOOLEAN;
		strsig += "Z";
	} else if (str_type == "byte" || str_type == "B") {
		t |= JavaClass::ARG_TYPE_BYTE;
		strsig += "B";
	} else if (str_type == "char" || str_type == "C") {
		t |= JavaClass::ARG_TYPE_CHAR;
		strsig += "C";
	} else if (str_type == "short" || str_type == "S") {
		t |= JavaClass::ARG_TYPE_SHORT;
		strsig += "S";
	} else if (str_type == "int" || str_type == "I") {
		t |= JavaClass::ARG_TYPE_INT;
		strsig += "I";
	} else if (str_type == "long" || str_type == "J") {
		t |= JavaClass::ARG_TYPE_LONG;
		strsig += "J";
	} else if (str_type == "float" || str_type == "F") {
		t |= JavaClass::ARG_TYPE_FLOAT;
		strsig += "F";
	} else if (str_type == "double" || str_type == "D") {
		t |= JavaClass::ARG_TYPE_DOUBLE;
		strsig += "D";
	} else if (str_type == "java.lang.String") {
		t |= JavaClass::ARG_TYPE_STRING;
		strsig += "Ljava/lang/String;";
	} else if (str_type == "java.lang.CharSequence") {
		t |= JavaClass::ARG_TYPE_CHARSEQUENCE;
		strsig += "Ljava/lang/CharSequence;";
	} else if (str_type == "org.godotengine.godot.variant.Callable") {
		t |= JavaClass::ARG_TYPE_CALLABLE;
		strsig += "Lorg/godotengine/godot/variant/Callable;";
	} else if (str_type == "java.lang.Boolean") {
		t |= JavaClass::ARG_TYPE_BOOLEAN | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Boolean;";
	} else if (str_type == "java.lang.Byte") {
		t |= JavaClass::ARG_TYPE_BYTE | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Byte;";
	} else if (str_type == "java.lang.Character") {
		t |= JavaClass::ARG_TYPE_CHAR | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Character;";
	} else if (str_type == "java.lang.Short") {
		t |= JavaClass::ARG_TYPE_SHORT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Short;";
	} else if (str_type == "java.lang.Integer") {
		t |= JavaClass::ARG_TYPE_INT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Integer;";
	} else if (str_type == "java.lang.Long") {
		t |= JavaClass::ARG_TYPE_LONG | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Long;";
	} else if (str_type == "java.lang.Float") {
		t |= JavaClass::ARG_TYPE_FLOAT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Float;";
	} else if (str_type == "java.lang.Double") {
		t |= JavaClass::ARG_TYPE_DOUBLE | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Double;";
	} else {
		//a class likely
		strsig += "L" + str_type.replace_char('.', '/') + ";";
		t |= JavaClass::ARG_TYPE_CLASS;
	}

	sig = t;

	return true;
}

bool JavaClass::_convert_object_to_variant(JNIEnv *env, jobject obj, Variant &var, uint32_t p_sig) {
	if (!obj) {
		var = Variant(); //seems null is just null...
		return true;
	}

	switch (p_sig) {
		case ARG_TYPE_VOID: {
			return Variant();
		} break;
		case ARG_TYPE_BOOLEAN | ARG_NUMBER_CLASS_BIT: {
			var = env->CallBooleanMethod(obj, JavaClassWrapper::data->Boolean_booleanValue);
			return true;
		} break;
		case ARG_TYPE_BYTE | ARG_NUMBER_CLASS_BIT: {
			var = env->CallByteMethod(obj, JavaClassWrapper::data->Byte_byteValue);
			return true;

		} break;
		case ARG_TYPE_CHAR | ARG_NUMBER_CLASS_BIT: {
			var = env->CallCharMethod(obj, JavaClassWrapper::data->Character_characterValue);
			return true;

		} break;
		case ARG_TYPE_SHORT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallShortMethod(obj, JavaClassWrapper::data->Short_shortValue);
			return true;

		} break;
		case ARG_TYPE_INT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallIntMethod(obj, JavaClassWrapper::data->Integer_integerValue);
			return true;

		} break;
		case ARG_TYPE_LONG | ARG_NUMBER_CLASS_BIT: {
			var = (int64_t)env->CallLongMethod(obj, JavaClassWrapper::data->Long_longValue);
			return true;

		} break;
		case ARG_TYPE_FLOAT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallFloatMethod(obj, JavaClassWrapper::data->Float_floatValue);
			return true;

		} break;
		case ARG_TYPE_DOUBLE | ARG_NUMBER_CLASS_BIT: {
			var = env->CallDoubleMethod(obj, JavaClassWrapper::data->Double_doubleValue);
			return true;
		} break;
		case ARG_TYPE_STRING: {
			var = jstring_to_string((jstring)obj, env);
			return true;
		} break;
		case ARG_TYPE_CHARSEQUENCE: {
			var = charsequence_to_string(env, obj);
			return true;
		} break;
		case ARG_TYPE_CALLABLE: {
			var = jcallable_to_callable(env, obj);
			return true;
		} break;
		case ARG_TYPE_CLASS: {
			jclass java_class = env->GetObjectClass(obj);
			Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);
			env->DeleteLocalRef(java_class);

			if (java_class_wrapped.is_valid()) {
				String cn = java_class_wrapped->get_java_class_name();
				if (cn == "org.godotengine.godot.Dictionary") {
					var = _jobject_to_variant(env, obj);
				} else {
					Ref<JavaObject> ret = Ref<JavaObject>(memnew(JavaObject(java_class_wrapped, obj)));
					var = ret;
				}
				return true;
			}

			return false;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_VOID: {
			var = Array(); // ?
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
			Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			for (int i = 0; i < count; i++) {
				jboolean val;
				env->GetBooleanArrayRegion((jbooleanArray)arr, i, 1, &val);
				ret[i] = (bool)val;
			}

			var = ret;
			return true;

		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetByteArrayRegion((jbyteArray)arr, 0, count, (int8_t *)ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			// Char arrays are UTF-16 encoded, so it's double the length.
			ret.resize(count * 2);
			env->GetCharArrayRegion((jcharArray)arr, 0, count, (jchar *)ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jshort val;
				env->GetShortArrayRegion((jshortArray)arr, i, 1, &val);
				ptr[i] = val;
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_INT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetIntArrayRegion((jintArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
			PackedInt64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetLongArrayRegion((jlongArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
			PackedFloat32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetFloatArrayRegion((jfloatArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
			PackedFloat64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetDoubleArrayRegion((jdoubleArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
			Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					bool val = env->CallBooleanMethod(o, JavaClassWrapper::data->Boolean_booleanValue);
					ret[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;

		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			uint8_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallByteMethod(o, JavaClassWrapper::data->Byte_byteValue);
					ptr[i] = (uint8_t)val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			// Char arrays are UTF-16 encoded, so it's double the length.
			ret.resize(count * 2);

			jchar *ptr = (jchar *)ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					count = i;
					break;
				} else {
					int val = env->CallCharMethod(o, JavaClassWrapper::data->Character_characterValue);
					ptr[i] = (jchar)val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallShortMethod(o, JavaClassWrapper::data->Short_shortValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_INT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallIntMethod(o, JavaClassWrapper::data->Integer_integerValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_LONG: {
			PackedInt64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int64_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int64_t val = env->CallLongMethod(o, JavaClassWrapper::data->Long_longValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
			PackedFloat32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			float *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0.0;
				} else {
					float val = env->CallFloatMethod(o, JavaClassWrapper::data->Float_floatValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
			PackedFloat64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			double *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0.0;
				} else {
					double val = env->CallDoubleMethod(o, JavaClassWrapper::data->Double_doubleValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;

		case ARG_ARRAY_BIT | ARG_TYPE_STRING: {
			PackedStringArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			String *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					String val = jstring_to_string((jstring)o, env);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
			PackedStringArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			String *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					String val = charsequence_to_string(env, o);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
			Array ret;
			jobjectArray jarr = (jobjectArray)obj;
			int count = env->GetArrayLength(jarr);
			ret.resize(count);
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(jarr, i);
				if (o) {
					Callable callable = jcallable_to_callable(env, o);
					ret[i] = callable;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
			Array ret;
			jobjectArray jarr = (jobjectArray)obj;
			int count = env->GetArrayLength(jarr);
			ret.resize(count);
			for (int i = 0; i < count; i++) {
				jobject obj = env->GetObjectArrayElement(jarr, i);
				if (obj) {
					jclass java_class = env->GetObjectClass(obj);
					Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);
					env->DeleteLocalRef(java_class);

					if (java_class_wrapped.is_valid()) {
						String cn = java_class_wrapped->get_java_class_name();
						if (cn == "org.godotengine.godot.Dictionary") {
							ret[i] = _jobject_to_variant(env, obj);
						} else {
							Ref<JavaObject> java_obj_wrapped = Ref<JavaObject>(memnew(JavaObject(java_class_wrapped, obj)));
							ret[i] = java_obj_wrapped;
						}
					}
				}
				env->DeleteLocalRef(obj);
			}

			var = ret;
			return true;
		} break;
	}

	return false;
}

bool JavaClassWrapper::_wrap_class_components(JNIEnv *p_env, const Ref<JavaClass> &p_java_class, jclass p_class, bool p_allow_non_public_methods_access) {
	ERR_FAIL_NULL_V(p_class, false);

	jobjectArray constructors = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getConstructors);
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V(constructors, false);

	jobjectArray methods = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getDeclaredMethods);
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V(methods, false);

	int constructor_count = p_env->GetArrayLength(constructors);
	int method_count = p_env->GetArrayLength(methods);

	int methods_and_constructors_count = method_count + constructor_count;
	for (int i = 0; i < methods_and_constructors_count; i++) {
		bool is_constructor = i < constructor_count;
		jobject obj = is_constructor
				? p_env->GetObjectArrayElement(constructors, i)
				: p_env->GetObjectArrayElement(methods, i - constructor_count);
		ERR_CONTINUE(!obj);

		String str_method;
		if (is_constructor) {
			str_method = "<init>";
		} else {
			jstring name = (jstring)p_env->CallObjectMethod(obj, data->Method_getName);
			str_method = jstring_to_string(name, p_env);
			p_env->DeleteLocalRef(name);
		}

		jint mods = p_env->CallIntMethod(obj, is_constructor ? data->Constructor_getModifiers : data->Method_getModifiers);
		bool is_public = (mods & 0x0001) != 0; // java.lang.reflect.Modifier.PUBLIC

		if (!is_public && (is_constructor || !p_allow_non_public_methods_access)) {
			p_env->DeleteLocalRef(obj);
			continue; //not public bye
		}

		jobjectArray param_types = (jobjectArray)p_env->CallObjectMethod(obj, is_constructor ? data->Constructor_getParameterTypes : data->Method_getParameterTypes);
		int count = p_env->GetArrayLength(param_types);

		if (!p_java_class->methods.has(str_method)) {
			p_java_class->methods[str_method] = List<JavaClass::MethodInfo>();
		}

		JavaClass::MethodInfo mi;
		mi._public = is_public;
		mi._static = (mods & 0x8) != 0;
		mi._constructor = is_constructor;
		bool valid = true;
		String signature = "(";

		for (int j = 0; j < count; j++) {
			jobject obj2 = p_env->GetObjectArrayElement(param_types, j);
			String strsig;
			uint32_t sig = 0;
			if (!_get_type_sig(p_env, obj2, sig, strsig)) {
				valid = false;
				p_env->DeleteLocalRef(obj2);
				break;
			}
			signature += strsig;
			mi.param_types.push_back(sig);
			mi.param_sigs.push_back(strsig);
			p_env->DeleteLocalRef(obj2);
		}

		if (!valid) {
			print_line("Method can't be bound (unsupported arguments): " + p_java_class->java_class_name + "::" + str_method);
			p_env->DeleteLocalRef(obj);
			p_env->DeleteLocalRef(param_types);
			continue;
		}

		signature += ")";

		if (is_constructor) {
			signature += "V";
			mi.return_type = JavaClass::ARG_TYPE_CLASS;
		} else {
			jobject return_type = (jobject)p_env->CallObjectMethod(obj, data->Method_getReturnType);

			String strsig;
			uint32_t sig = 0;
			if (!_get_type_sig(p_env, return_type, sig, strsig)) {
				print_line("Method can't be bound (unsupported return type): " + p_java_class->java_class_name + "::" + str_method);
				p_env->DeleteLocalRef(obj);
				p_env->DeleteLocalRef(param_types);
				p_env->DeleteLocalRef(return_type);
				continue;
			}

			signature += strsig;
			mi.return_type = sig;

			p_env->DeleteLocalRef(return_type);
		}

		bool discard = false;

		for (List<JavaClass::MethodInfo>::Element *E = p_java_class->methods[str_method].front(); E; E = E->next()) {
			float new_likeliness = 0;
			float existing_likeliness = 0;

			if (E->get().param_types.size() != mi.param_types.size()) {
				continue;
			}
			bool this_valid = true;
			for (int j = 0; j < E->get().param_types.size(); j++) {
				Variant::Type _new;
				float new_l;
				Variant::Type existing;
				float existing_l;
				JavaClass::_convert_to_variant_type(E->get().param_types[j], existing, existing_l);
				JavaClass::_convert_to_variant_type(mi.param_types[j], _new, new_l);
				if (_new != existing) {
					this_valid = false;
					break;
				} else if ((_new == Variant::OBJECT || _new == Variant::ARRAY) && E->get().param_sigs[j] != mi.param_sigs[j]) {
					this_valid = false;
					break;
				}
				new_likeliness += new_l;
				existing_likeliness += existing_l;
			}

			if (!this_valid) {
				continue;
			}

			if (new_likeliness > existing_likeliness) {
				p_java_class->methods[str_method].erase(E);
				break;
			} else {
				discard = true;
			}
		}

		if (!discard) {
			if (mi._static) {
				mi.method = p_env->GetStaticMethodID(p_class, str_method.utf8().get_data(), signature.utf8().get_data());
			} else {
				mi.method = p_env->GetMethodID(p_class, str_method.utf8().get_data(), signature.utf8().get_data());
			}

			if (p_env->ExceptionCheck()) {
				// Exceptions may be thrown when trying to access hidden methods; write the exception to the logs and continue.
				p_env->ExceptionDescribe();
				p_env->ExceptionClear();
				continue;
			}

			if (mi.method) {
				p_java_class->methods[str_method].push_back(mi);
			}
		}

		p_env->DeleteLocalRef(obj);
		p_env->DeleteLocalRef(param_types);
	}

	p_env->DeleteLocalRef(constructors);
	p_env->DeleteLocalRef(methods);

	jobjectArray fields = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getFields);

	int count = p_env->GetArrayLength(fields);

	for (int i = 0; i < count; i++) {
		jobject obj = p_env->GetObjectArrayElement(fields, i);
		ERR_CONTINUE(!obj);

		jstring name = (jstring)p_env->CallObjectMethod(obj, data->Field_getName);
		String str_field = jstring_to_string(name, p_env);
		p_env->DeleteLocalRef(name);
		int mods = p_env->CallIntMethod(obj, data->Field_getModifiers);
		if ((mods & 0x8) && (mods & 0x1)) { //static public!

			jobject objc = p_env->CallObjectMethod(obj, data->Field_get, nullptr);
			if (objc) {
				uint32_t sig;
				String strsig;
				jclass cl = p_env->GetObjectClass(objc);
				if (JavaClassWrapper::_get_type_sig(p_env, cl, sig, strsig)) {
					if ((sig & JavaClass::ARG_TYPE_MASK) <= JavaClass::ARG_TYPE_STRING) {
						Variant value;
						if (JavaClass::_convert_object_to_variant(p_env, objc, value, sig)) {
							p_java_class->constant_map[str_field] = value;
						}
					}
				}

				p_env->DeleteLocalRef(cl);
			}

			p_env->DeleteLocalRef(objc);
		}
		p_env->DeleteLocalRef(obj);
	}

	p_env->DeleteLocalRef(fields);
	return true;
}{
					if ((sig & JavaClass::ARG_TYPE_MASK) <= JavaClass::ARG_TYPE_STRING) {
						Variant value;
						if (JavaClass::_convert_object_to_variant(p_env, objc, value, sig)) {
							p_java_class->constant_map[str_field] = value;
						}
					}
				}

				p_env->DeleteLocalRef(cl);
			}

			p_env->DeleteLocalRef(objc);
		}
		p_env->DeleteLocalRef(obj);
	}

	p_env->DeleteLocalRef(fields);
	return true;
}

Ref<JavaClass> JavaClassWrapper::_wrap(const String &p_class, bool p_allow_non_public_methods_access) {
	String class_name_dots = p_class.replace_char('/', '.');
	if (data->class_cache.has(class_name_dots)) {
		return data->class_cache[class_name_dots];
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jclass bclass = jni_find_class(env, class_name_dots.replace_char('.', '/').utf8().get_data());
	ERR_FAIL_NULL_V_MSG(bclass, Ref<JavaClass>(), vformat("Java class '%s' not found.", p_class));

	Ref<JavaClass> java_class;
	java_class.instantiate();
	java_class->java_class_name = class_name_dots;
	Vector<String> class_name_parts = class_name_dots.split(".");
	java_class->java_constructor_name = class_name_parts[class_name_parts.size() - 1];
	java_class->_class = (jclass)env->NewGlobalRef(bclass);
	java_class->is_interface = env->CallBooleanMethod(bclass, data->Class_isInterface);

	bool class_components_configured;
	if (_is_proxy_class(env, bclass)) {
		// Proxy class components must be setup using the interfaces they implement.
		jobjectArray interfaces = (jobjectArray)env->CallObjectMethod(bclass, data->Class_getInterfaces);
		if (env->ExceptionCheck()) {
			env->ExceptionDescribe();
			env->ExceptionClear();
		}
		int interfaces_count = interfaces == nullptr ? 0 : env->GetArrayLength(interfaces);
		for (int i = 0; i < interfaces_count; i++) {
			jclass interface = (jclass)env->GetObjectArrayElement(interfaces, i);
			ERR_CONTINUE(!interface);

			jstring j_interface_name = (jstring)env->CallObjectMethod(interface, data->Class_getName);
			String interface_name = jstring_to_string(j_interface_name, env);
			env->DeleteLocalRef(j_interface_name);
			if (!_wrap_class_components(env, java_class, interface, p_allow_non_public_methods_access)) {
				ERR_PRINT(vformat("Unable to set up components for proxy class interface %s.", interface_name));
				continue;
			}

			class_components_configured = true;
		}
	} else {
		class_components_configured = _wrap_class_components(env, java_class, bclass, p_allow_non_public_methods_access);
	}
	env->DeleteLocalRef(bclass);

	if (!class_components_configured) {
		java_class.unref();
		return Ref<JavaClass>();
	}

	// Cache the initialized JavaClass instance.
	data->class_cache[class_name_dots] = java_class;

	return java_class;
}

Ref<JavaObject> JavaClassWrapper::create_sam_callback(const String &p_sam_interface, const Callable &p_callable) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->android_runtime_class, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->ARP_create_proxy_from_godot_callable, Ref<JavaObject>());

	Ref<JavaClass> interface_class = wrap(p_sam_interface);
	if (interface_class.is_null()) {
		ERR_PRINT(vformat("Invalid java class: %s.", p_sam_interface));
		return Ref<JavaObject>();
	}
	if (!interface_class->is_interface) {
		ERR_PRINT(vformat("Java class %s must be an interface.", p_sam_interface));
		return Ref<JavaObject>();
	}

	if (interface_class->methods.size() != 1) {
		ERR_PRINT(vformat("%s must be a Single Abstract Method (SAM) interface.", p_sam_interface));
		return Ref<JavaObject>();
	}

	jobject j_callable = callable_to_jcallable(env, p_callable);
	jstring j_interface = env->NewStringUTF(p_sam_interface.utf8().get_data());

	jobject proxy = env->CallStaticObjectMethod(data->android_runtime_class, data->ARP_create_proxy_from_godot_callable, j_interface, j_callable);
	Ref<JavaObject> result = _jobject_to_variant(env, proxy);

	env->DeleteLocalRef(j_callable);
	env->DeleteLocalRef(j_interface);
	env->DeleteLocalRef(proxy);

	return result;
}

Ref<JavaObject> JavaClassWrapper::create_proxy(const Object *p_object, const PackedStringArray &p_interfaces) {
	ERR_FAIL_NULL_V(p_object, Ref<JavaObject>());
	ERR_FAIL_COND_V(p_interfaces.is_empty(), Ref<JavaObject>());

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->android_runtime_class, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->ARP_create_proxy_from_godot_callable, Ref<JavaObject>());

	for (const String &interface_name : p_interfaces) {
		Ref<JavaClass> interface_class = wrap(interface_name);
		if (interface_class.is_null()) {
			ERR_PRINT(vformat("Invalid java class: %s.", interface_name));
			return Ref<JavaObject>();
		}

		if (!interface_class->is_interface) {
			ERR_PRINT(vformat("Java class %s must be an interface.", interface_name));
			return Ref<JavaObject>();
		}
	}

	jlong object_id = p_object->get_instance_id();
	jobjectArray j_interfaces = env->NewObjectArray(p_interfaces.size(), jni_find_class(env, "java/lang/String"), nullptr);
	for (int i = 0; i < p_interfaces.size(); i++) {
		jstring j_interface = env->NewStringUTF(p_interfaces[i].utf8().get_data());
		env->SetObjectArrayElement(j_interfaces, i, j_interface);
		env->DeleteLocalRef(j_interface);
	}

	jobject proxy = env->CallStaticObjectMethod(data->android_runtime_class, data->ARP_create_proxy_from_godot_object_id, object_id, j_interfaces);
	Ref<JavaObject> result = _jobject_to_variant(env, proxy);

	env->DeleteLocalRef(j_interfaces);
	env->DeleteLocalRef(proxy);

	return result;
}

Ref<JavaClass> JavaClassWrapper::wrap_jclass(jclass p_class, bool p_allow_non_public_methods_access) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jstring class_name = (jstring)env->CallObjectMethod(p_class, data->Class_getName);
	String class_name_string = jstring_to_string(class_name, env);
	env->DeleteLocalRef(class_name);

	return _wrap(class_name_string, p_allow_non_public_methods_access);
}

bool JavaClassWrapper::_is_proxy_class(JNIEnv *env, jclass p_class) {
	ERR_FAIL_NULL_V(data->proxy_class, false);
	ERR_FAIL_NULL_V(data->Proxy_isProxyClass, false);
	ERR_FAIL_NULL_V(env, false);

	return env->CallStaticBooleanMethod(data->proxy_class, data->Proxy_isProxyClass, p_class);
}

void JavaClassWrapper::finalize() {
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void JavaClassWrapper::initialize() {
	if (data) {
		return;
	}
	data = memnew(Data);

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	data->proxy_class = jni_find_class(env, "java/lang/reflect/Proxy");
	if (data->proxy_class) {
		data->proxy_class = (jclass)env->NewGlobalRef(data->proxy_class);
		data->Proxy_isProxyClass = env->GetStaticMethodID(data->proxy_class, "isProxyClass", "(Ljava/lang/Class;)Z");
	}

	data->android_runtime_class = jni_find_class(env, "org/godotengine/godot/plugin/AndroidRuntimePlugin");
	if (data->android_runtime_class) {
		data->android_runtime_class = (jclass)env->NewGlobalRef(data->android_runtime_class);
		data->ARP_create_proxy_from_godot_callable = env->GetStaticMethodID(data->android_runtime_class, "createProxyFromGodotCallable", "(Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;)Ljava/lang/Object;");
		data->ARP_create_proxy_from_godot_object_id = env->GetStaticMethodID(data->android_runtime_class, "createProxyFromGodotObjectID", "(J[Ljava/lang/String;)Ljava/lang/Object;");
	}

	jclass bclass = jni_find_class(env, "java/lang/Class");
	data->Class_getConstructors = env->GetMethodID(bclass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	data->Class_getDeclaredMethods = env->GetMethodID(bclass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	data->Class_getFields = env->GetMethodID(bclass, "getFields", "()[Ljava/lang/reflect/Field;");
	data->Class_getInterfaces = env->GetMethodID(bclass, "getInterfaces", "()[Ljava/lang/Class;");
	data->Class_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Class_getSuperclass = env->GetMethodID(bclass, "getSuperclass", "()Ljava/lang/Class;");
	data->Class_isInterface = env->GetMethodID(bclass, "isInterface", "()Z");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Constructor");
	data->Constructor_getParameterTypes = env->GetMethodID(bclass, "getParameterTypes", "()[Ljava/lang/Class;");
	data->Constructor_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Method");
	data->Method_getParameterTypes = env->GetMethodID(bclass, "getParameterTypes", "()[Ljava/lang/Class;");
	data->Method_getReturnType = env->GetMethodID(bclass, "getReturnType", "()Ljava/lang/Class;");
	data->Method_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Method_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Field");
	data->Field_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Field_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	data->Field_get = env->GetMethodID(bclass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Boolean");
	data->Boolean_booleanValue = env->GetMethodID(bclass, "booleanValue", "()Z");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Byte");
	data->Byte_byteValue = env->GetMethodID(bclass, "byteValue", "()B");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Character");
	data->Character_characterValue = env->GetMethodID(bclass, "charValue", "()C");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Short");
	data->Short_shortValue = env->GetMethodID(bclass, "shortValue", "()S");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Integer");
	data->Integer_integerValue = env->GetMethodID(bclass, "intValue", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Long");
	data->Long_longValue = env->GetMethodID(bclass, "longValue", "()J");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Float");
	data->Float_floatValue = env->GetMethodID(bclass, "floatValue", "()F");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Double");
	data->Double_doubleValue = env->GetMethodID(bclass, "doubleValue", "()D");
	env->DeleteLocalRef(bclass);
}


// --- Merged from platform/android/java_godot_lib_jni.cpp ---
// --- Inlined from platform/android/java_godot_lib_jni.h ---
#include <android/log.h>
#include <jni.h>

// These functions can be called from within JAVA and are the means by which our JAVA implementation calls back into our C++ code.
// See java/src/org/godotengine/godot/GodotLib.java for the JAVA side of this (yes that's why we have the long names)
extern "C" {
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_initialize(JNIEnv *env, jclass clazz, jobject p_godot_native_bridge, jobject p_asset_manager, jobject p_godot_io, jobject p_net_utils, jobject p_directory_access_handler, jobject p_file_access_handler, jboolean p_use_apk_expansion);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ondestroy(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_setup(JNIEnv *env, jclass clazz, jobjectArray p_cmdline, jobject p_godot_tts);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_resize(JNIEnv *env, jclass clazz, jobject p_surface, jint p_width, jint p_height);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_newcontext(JNIEnv *env, jclass clazz, jobject p_surface);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_step(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ttsCallback(JNIEnv *env, jclass clazz, jint event, jlong id, jint pos);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_back(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchMouseEvent(JNIEnv *env, jclass clazz, jint p_event_type, jint p_button_mask, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y, jboolean p_double_click, jboolean p_source_mouse_relative, jfloat p_pressure, jfloat p_tilt_x, jfloat p_tilt_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchTouchEvent(JNIEnv *env, jclass clazz, jint ev, jint pointer, jint pointer_count, jfloatArray positions, jboolean p_double_tap);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnify(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_factor);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_pan(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_key(JNIEnv *env, jclass clazz, jint p_physical_keycode, jint p_unicode, jint p_key_label, jboolean p_pressed, jboolean p_echo);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joybutton(JNIEnv *env, jclass clazz, jint p_device, jint p_button, jboolean p_pressed);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyaxis(JNIEnv *env, jclass clazz, jint p_device, jint p_axis, jfloat p_value);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyhat(JNIEnv *env, jclass clazz, jint p_device, jint p_hat_x, jint p_hat_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyconnectionchanged(JNIEnv *env, jclass clazz, jint p_device, jboolean p_connected, jstring p_name);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_accelerometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gravity(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnetometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gyroscope(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusin(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusout(JNIEnv *env, jclass clazz);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getGlobal(JNIEnv *env, jclass clazz, jstring path);
JNIEXPORT jobjectArray JNICALL Java_org_godotengine_godot_GodotLib_getRendererInfo(JNIEnv *env, jclass clazz, jboolean p_vulkan_requirements_met);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getEditorSetting(JNIEnv *env, jclass clazz, jstring p_setting_key);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorSetting(JNIEnv *env, jclass clazz, jstring p_key, jobject p_data);
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_GodotLib_getEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_default_value);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_data);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setVirtualKeyboardHeight(JNIEnv *env, jclass clazz, jint p_height);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_requestPermissionResult(JNIEnv *env, jclass clazz, jstring p_permission, jboolean p_result);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onNightModeChanged(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_hardwareKeyboardConnected(JNIEnv *env, jclass clazz, jboolean p_connected);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_filePickerCallback(JNIEnv *env, jclass clazz, jboolean p_ok, jobjectArray p_selected_paths);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererResumed(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererPaused(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onOrientationChange(JNIEnv *env, jclass clazz, jint p_orientation);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_shouldDispatchInputToRenderThread(JNIEnv *env, jclass clazz);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getProjectResourceDir(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isEditorHint(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isProjectManagerHint(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_hasFeature(JNIEnv *env, jclass clazz, jstring p_feature);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onPictureInPictureModeChanged(JNIEnv *env, jclass clazz, jboolean p_is_in_picture_in_picture_mode);
}

// --- End platform/android/java_godot_lib_jni.h ---

#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/os/mutex.h"
#include "core/os/os.h"
#include "servers/display/display_server.h"

#ifndef XR_DISABLED
#include "servers/display/display_server.h"
#endif // XR_DISABLED

#ifdef TOOLS_ENABLED
#include "servers/display/display_server.h"
#endif

#include <android/asset_manager_jni.h>
#include <android/input.h>
#include <android/native_window_jni.h>
#include <unistd.h>

static AndroidInputHandler *input_handler = nullptr;
static GodotJavaWrapper *godot_java = nullptr;
static GodotIOJavaWrapper *godot_io_java = nullptr;

enum StartupStep {
	STEP_TERMINATED = -1,
	STEP_SETUP,
	STEP_SHOW_LOGO,
	STEP_STARTED
};

static SafeNumeric<int> step; // Shared between UI and render threads

static Vector3 accelerometer;
static Vector3 gravity;
static Vector3 magnetometer;
static Vector3 gyroscope;

static void _terminate(JNIEnv *env, bool p_restart = false) {
	if (step.get() == STEP_TERMINATED) {
		return;
	}

	step.set(STEP_TERMINATED); // Ensure no further steps are attempted and no further events are sent

	// lets cleanup
	// Unregister android plugins
	unregister_plugins_singletons();

	JavaClassWrapper::finalize();
	if (input_handler) {
		delete input_handler;
	}
	// Whether restarting is handled by 'Main::cleanup()'
	bool restart_on_cleanup = false;
	if (OS_Android::is_initialized()) {
		restart_on_cleanup = OS::is_restart_on_exit_set();
		OS_Android::main_loop_end();
		Main::cleanup();
		OS::finalize();
		OS::finalize_core();
	}
	if (godot_io_java) {
		delete godot_io_java;
	}

	TTS_Android::terminate();
	FileAccessAndroid::terminate();
	DirAccessJAndroid::terminate();
	FileAccessFilesystemJAndroid::terminate();
	NetSocketAndroid::terminate();

	cleanup_android_class_loader();
	godot_cleanup_profiler();

	if (godot_java) {
		godot_java->on_godot_terminating(env);
		if (!restart_on_cleanup) {
			if (p_restart) {
				godot_java->restart(env);
			} else {
				godot_java->force_quit(env);
			}
		}
		delete godot_java;
	}
}

static String rendering_source_to_string(OS::RenderingSource p_source) {
	switch (p_source) {
		case OS::RENDERING_SOURCE_DEFAULT:
			return "Default";
		case OS::RENDERING_SOURCE_PROJECT_SETTING:
			return "ProjectSettings";
		case OS::RENDERING_SOURCE_COMMANDLINE:
			return "Commandline";
		case OS::RENDERING_SOURCE_FALLBACK:
			return "Fallback";
		default:
			return "Unknown";
	}
}

extern "C" {

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setVirtualKeyboardHeight(JNIEnv *env, jclass clazz, jint p_height) {
	if (godot_io_java) {
		godot_io_java->set_vk_height(p_height);
	}
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_initialize(JNIEnv *env, jclass clazz, jobject p_godot_native_bridge, jobject p_asset_manager, jobject p_godot_io, jobject p_net_utils, jobject p_directory_access_handler, jobject p_file_access_handler, jboolean p_use_apk_expansion) {
	godot_init_profiler();

	JavaVM *jvm;
	env->GetJavaVM(&jvm);

	init_thread_jandroid(jvm, env);
	setup_android_class_loader();

	// create our wrapper classes
	godot_java = new GodotJavaWrapper(env, p_godot_native_bridge);
	godot_io_java = new GodotIOJavaWrapper(env, p_godot_io);

	FileAccessAndroid::setup(p_asset_manager);
	DirAccessJAndroid::setup(p_directory_access_handler);
	FileAccessFilesystemJAndroid::setup(p_file_access_handler);
	NetSocketAndroid::setup(p_net_utils);

	OS_Android::setup(godot_java, godot_io_java, p_use_apk_expansion);
	OS::initialize_core();
	OS::initialize();

	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ondestroy(JNIEnv *env, jclass clazz) {
	_terminate(env, false);
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_setup(JNIEnv *env, jclass clazz, jobjectArray p_cmdline, jobject p_godot_tts) {
	setup_android_thread();

	const char **cmdline = nullptr;
	jstring *j_cmdline = nullptr;
	int cmdlen = 0;
	if (p_cmdline) {
		cmdlen = env->GetArrayLength(p_cmdline);
		if (cmdlen) {
			cmdline = (const char **)memalloc((cmdlen + 1) * sizeof(const char *));
			ERR_FAIL_NULL_V_MSG(cmdline, false, "Out of memory.");
			cmdline[cmdlen] = nullptr;
			j_cmdline = (jstring *)memalloc(cmdlen * sizeof(jstring));
			ERR_FAIL_NULL_V_MSG(j_cmdline, false, "Out of memory.");

			for (int i = 0; i < cmdlen; i++) {
				jstring string = (jstring)env->GetObjectArrayElement(p_cmdline, i);
				const char *rawString = env->GetStringUTFChars(string, nullptr);

				cmdline[i] = rawString;
				j_cmdline[i] = string;
			}
		}
	}

	Error err = Main::setup(OS_Android::ANDROID_EXEC_PATH, cmdlen, (char **)cmdline, false);
	if (cmdline) {
		if (j_cmdline) {
			for (int i = 0; i < cmdlen; ++i) {
				env->ReleaseStringUTFChars(j_cmdline[i], cmdline[i]);
			}
			memfree(j_cmdline);
		}
		memfree(cmdline);
	}

	// Note: --help and --version return ERR_HELP, but this should be translated to 0 if exit codes are propagated.
	if (err != OK) {
		return false;
	}

	Thread::release_main_thread(); // setup2 will be called from another thread.
	set_current_thread_safe_for_nodes(false);

	TTS_Android::setup(p_godot_tts);

	JavaClassWrapper::initialize();
	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_resize(JNIEnv *env, jclass clazz, jobject p_surface, jint p_width, jint p_height) {
	if (OS_Android::is_initialized()) {
		OS_Android::set_display_size(Size2i(p_width, p_height));

		// No need to reset the surface during startup
		if (step.get() > STEP_SETUP) {
			if (p_surface) {
				ANativeWindow *native_window = ANativeWindow_fromSurface(env, p_surface);
				OS_Android::set_native_window(native_window);
			}
			DisplayServer::driver_as<DisplayServerAndroid>()->reset_window();
			DisplayServer::driver_as<DisplayServerAndroid>()->notify_surface_changed(p_width, p_height);
		}
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_newcontext(JNIEnv *env, jclass clazz, jobject p_surface) {
	if (OS_Android::is_initialized()) {
		if (step.get() == STEP_SETUP) {
			// During startup
			if (p_surface) {
				ANativeWindow *native_window = ANativeWindow_fromSurface(env, p_surface);
				OS_Android::set_native_window(native_window);
			}
		} else {
			// Rendering context recreated because it was lost; restart app to let it reload everything
			_terminate(env, true);
		}
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_back(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
		dsa->send_window_event(DisplayServerEnums::WINDOW_EVENT_GO_BACK_REQUEST);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ttsCallback(JNIEnv *env, jclass clazz, jint event, jlong id, jint pos) {
	TTS_Android::_java_utterance_callback(event, id, pos);
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_step(JNIEnv *env, jclass clazz) {
	if (step.get() == STEP_TERMINATED) {
		return true;
	}

	if (step.get() == STEP_SETUP) {
		// Since Godot is initialized on the UI thread, main_thread_id was set to that thread's id,
		// but for Godot purposes, the main thread is the one running the game loop
		Main::setup2(false); // The logo is shown in the next frame otherwise we run into rendering issues
		input_handler = new AndroidInputHandler();
		step.increment();
		return true;
	}

	if (step.get() == STEP_SHOW_LOGO) {
		bool xr_enabled = false;
#ifndef XR_DISABLED
		// Unlike PCVR, there's no additional 2D screen onto which to render the boot logo,
		// so we skip this step if xr is enabled.
		if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
			xr_enabled = GLOBAL_GET_CACHED(bool, "xr/shaders/enabled");
		} else {
			xr_enabled = XRServer::get_xr_mode() == XRServer::XRMODE_ON;
		}
#endif // XR_DISABLED
		if (!xr_enabled) {
			Main::setup_boot_logo();
		}

		step.increment();
		return true;
	}

	if (step.get() == STEP_STARTED) {
		if (Main::start() != EXIT_SUCCESS) {
			return true; // should exit instead and print the error
		}

		godot_java->on_godot_setup_completed(env);
		OS_Android::main_loop_begin();
		godot_java->on_godot_main_loop_started(env);
		step.increment();
	}

	DisplayServer::driver_as<DisplayServerAndroid>()->process_accelerometer(accelerometer);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_gravity(gravity);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_magnetometer(magnetometer);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_gyroscope(gyroscope);

	bool should_swap_buffers = false;
	if (OS_Android::main_loop_iterate(&should_swap_buffers)) {
		_terminate(env, false);
	}

	return should_swap_buffers;
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchMouseEvent(JNIEnv *env, jclass clazz, jint p_event_type, jint p_button_mask, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y, jboolean p_double_click, jboolean p_source_mouse_relative, jfloat p_pressure, jfloat p_tilt_x, jfloat p_tilt_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	input_handler->process_mouse_event(p_event_type, p_button_mask, Point2(p_x, p_y), Vector2(p_delta_x, p_delta_y), p_double_click, p_source_mouse_relative, p_pressure, Vector2(p_tilt_x, p_tilt_y));
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchTouchEvent(JNIEnv *env, jclass clazz, jint ev, jint pointer, jint pointer_count, jfloatArray position, jboolean p_double_tap) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	Vector<AndroidInputHandler::TouchPos> points;
	for (int i = 0; i < pointer_count; i++) {
		jfloat p[6];
		env->GetFloatArrayRegion(position, i * 6, 6, p);
		AndroidInputHandler::TouchPos tp;
		tp.id = (int)p[0];
		tp.pos = Point2(p[1], p[2]);
		tp.pressure = p[3];
		tp.tilt = Vector2(p[4], p[5]);
		tp.double_tap = p_double_tap;
		points.push_back(tp);
	}

	input_handler->process_touch_event(ev, pointer, points);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnify(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_factor) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_magnify(Point2(p_x, p_y), p_factor);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_pan(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_pan(Point2(p_x, p_y), Vector2(p_delta_x, p_delta_y));
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joybutton(JNIEnv *env, jclass clazz, jint p_device, jint p_button, jboolean p_pressed) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_BUTTON;
	jevent.index = p_button;
	jevent.pressed = p_pressed;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyaxis(JNIEnv *env, jclass clazz, jint p_device, jint p_axis, jfloat p_value) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_AXIS;
	jevent.index = p_axis;
	jevent.value = p_value;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyhat(JNIEnv *env, jclass clazz, jint p_device, jint p_hat_x, jint p_hat_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_HAT;
	uint32_t hat = HatMask::CENTER;
	if (p_hat_x != 0) {
		if (p_hat_x < 0) {
			hat.set_flag(HatMask::LEFT);
		} else {
			hat.set_flag(HatMask::RIGHT);
		}
	}
	if (p_hat_y != 0) {
		if (p_hat_y < 0) {
			hat.set_flag(HatMask::UP);
		} else {
			hat.set_flag(HatMask::DOWN);
		}
	}
	jevent.hat = hat;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyconnectionchanged(JNIEnv *env, jclass clazz, jint p_device, jboolean p_connected, jstring p_name) {
	if (OS_Android::is_initialized()) {
		String name = jstring_to_string(p_name, env);
		(void)name;
	}
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_key(JNIEnv *env, jclass clazz, jint p_physical_keycode, jint p_unicode, jint p_key_label, jboolean p_pressed, jboolean p_echo) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_key_event(p_physical_keycode, p_unicode, p_key_label, p_pressed, p_echo);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_accelerometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	accelerometer = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gravity(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	gravity = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnetometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	magnetometer = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gyroscope(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	gyroscope = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusin(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	OS_Android::main_loop_focusin();
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusout(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	OS_Android::main_loop_focusout();
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getGlobal(JNIEnv *env, jclass clazz, jstring path) {
	String js = jstring_to_string(path, env);

	Variant setting_with_override = GLOBAL_GET(js);
	String setting_value = (setting_with_override.get_type() == Variant::NIL) ? "" : setting_with_override;
	return env->NewStringUTF(setting_value.utf8().get_data());
}

JNIEXPORT jobjectArray JNICALL Java_org_godotengine_godot_GodotLib_getRendererInfo(JNIEnv *env, jclass clazz, jboolean p_vulkan_requirements_met) {
	String rendering_driver_original = Renderer::get_current_rendering_driver_name();
	String rendering_driver_chosen = rendering_driver_original;
	String rendering_method = Renderer::get_current_rendering_method();

#ifdef VULKAN_ENABLED
	if (rendering_driver_original == "vulkan" && !DisplayServerAndroid::check_vulkan_global_context(p_vulkan_requirements_met)) {
		// The Android display server only gets created after this step, so a static check must be used to check for Vulkan
		// availability instead. If the check fails, it'll fall back to OpenGL3 accordingly if the relevant project setting
		// is enabled. The Vulkan context created by this check will be reused by the DisplayServer afterwards.
		rendering_driver_chosen = Renderer::get_current_rendering_driver_name();
		rendering_method = Renderer::get_current_rendering_method();
	}
#ifndef XR_DISABLED
	// When running in XR mode, vulkan initialization must be done by the XR module, so we ensure that the vulkan
	// global context is reset.
	// Note: This is temporary workaround to address https://github.com/godotengine/godot/issues/115924
	// A proper fix involves updating the Android init flow so that DisplayServerAndroid can update the Android surface
	// type (vulkan or opengl) after it's initialized.
	bool xr_enabled = false;
	if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
		xr_enabled = GLOBAL_GET_CACHED(bool, "xr/shaders/enabled");
	} else {
		xr_enabled = XRServer::get_xr_mode() == XRServer::XRMODE_ON;
	}
	if (xr_enabled) {
		DisplayServerAndroid::free_vulkan_global_context();
	}
#endif // XR_DISABLED
#endif

	String rendering_driver_source = rendering_source_to_string(OS::get_current_rendering_driver_name_source());
	String rendering_method_source = rendering_source_to_string(OS::get_current_rendering_method_source());

	jobjectArray result = env->NewObjectArray(5, jni_find_class(env, "java/lang/String"), nullptr);
	env->SetObjectArrayElement(result, 0, env->NewStringUTF(rendering_driver_chosen.utf8().get_data()));
	env->SetObjectArrayElement(result, 1, env->NewStringUTF(rendering_driver_original.utf8().get_data()));
	env->SetObjectArrayElement(result, 2, env->NewStringUTF(rendering_method.utf8().get_data()));
	env->SetObjectArrayElement(result, 3, env->NewStringUTF(rendering_driver_source.utf8().get_data()));
	env->SetObjectArrayElement(result, 4, env->NewStringUTF(rendering_method_source.utf8().get_data()));

	return result;
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getEditorSetting(JNIEnv *env, jclass clazz, jstring p_setting_key) {
	String editor_setting_value = "";
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String godot_setting_key = jstring_to_string(p_setting_key, env);
		Variant editor_setting = EDITOR_GET(godot_setting_key);
		editor_setting_value = (editor_setting.get_type() == Variant::NIL) ? "" : editor_setting;
	}
#else
	WARN_PRINT("Access to the Editor Settings in only available on Editor builds");
#endif

	return env->NewStringUTF(editor_setting_value.utf8().get_data());
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorSetting(JNIEnv *env, jclass clazz, jstring p_key, jobject p_data) {
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_data);
		(void)key;
		(void)data;
	}
#else
	WARN_PRINT("Access to the Editor Settings in only available on Editor builds");
#endif
}

JNIEXPORT jobject JNICALL Java_org_godotengine_godot_GodotLib_getEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_default_value) {
	jvalue result;

#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String section = jstring_to_string(p_section, env);
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_default_value);
		(void)section;
		(void)key;
		result.l = _variant_to_jobject(env, data.get_type(), &data);
	}
#else
	WARN_PRINT("Access to the Editor Settings Project Metadata is only available on Editor builds");
#endif

	return result.l;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_data) {
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String section = jstring_to_string(p_section, env);
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_data);
		(void)section;
		(void)key;
		(void)data;
	}
#else
	WARN_PRINT("Access to the Editor Settings Project Metadata is only available on Editor builds");
#endif
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onNightModeChanged(JNIEnv *env, jclass clazz) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_system_theme_changed();
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_hardwareKeyboardConnected(JNIEnv *env, jclass clazz, jboolean p_connected) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_hardware_keyboard_connection_changed(p_connected);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_filePickerCallback(JNIEnv *env, jclass clazz, jboolean p_ok, jobjectArray p_selected_paths) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		Vector<String> selected_paths;

		jint length = env->GetArrayLength(p_selected_paths);
		for (jint i = 0; i < length; ++i) {
			jstring java_string = (jstring)env->GetObjectArrayElement(p_selected_paths, i);
			String path = jstring_to_string(java_string, env);
			selected_paths.push_back(path);
			env->DeleteLocalRef(java_string);
		}
		ds->emit_file_picker_callback(p_ok, selected_paths);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_requestPermissionResult(JNIEnv *env, jclass clazz, jstring p_permission, jboolean p_result) {
	String permission = jstring_to_string(p_permission, env);
	if (permission == "android.permission.RECORD_AUDIO" && p_result) {
		AudioDriverManager::get_active_driver()->input_start();
	}

}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererResumed(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	// We force redraw to ensure we render at least once when resuming the app.
	Main::force_redraw();
	if (CameraServer::is_initialized()) {
		CameraServer::handle_application_resume();
	}
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_RESUMED);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererPaused(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (CameraServer::is_initialized()) {
		CameraServer::handle_application_pause();
	}

	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PAUSED);
	}

	if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
		dsa->notify_application_paused();
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onOrientationChange(JNIEnv *env, jclass clazz, jint p_orientation) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (CameraServer::is_initialized()) {
		CameraServer::handle_display_rotation_change(p_orientation);
	}
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_shouldDispatchInputToRenderThread(JNIEnv *env, jclass clazz) {
	if (Input::is_initialized()) {
		return !Input::is_agile_input_event_flushing();
	}
	return false;
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getProjectResourceDir(JNIEnv *env, jclass clazz) {
	const String resource_dir = OS::get_resource_dir();
	return env->NewStringUTF(resource_dir.utf8().get_data());
}
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isEditorHint(JNIEnv *env, jclass clazz) {
	return Engine::is_editor_hint();
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isProjectManagerHint(JNIEnv *env, jclass clazz) {
	return Engine::is_project_manager_hint();
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_hasFeature(JNIEnv *env, jclass clazz, jstring p_feature) {
	if (OS::is_initialized()) {
		String feature = jstring_to_string(p_feature, env);
		return OS::has_feature(feature);
	}
	return false;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onPictureInPictureModeChanged(JNIEnv *env, jclass clazz, jboolean p_is_in_picture_in_picture_mode) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (MainLoop::is_active()) {
		if (p_is_in_picture_in_picture_mode) {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_ENTERED);
		} else {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_EXITED);
		}
	}
}
}


// --- Merged from platform/android/java_godot_wrapper.cpp ---
#include "servers/display/display_server.h"


// JNIEnv is only valid within the thread it belongs to, in a multi threading environment
// we can't cache it.
// For Godot we call most access methods from our thread and we thus get a valid JNIEnv
// from get_jni_env(). For one or two we expect to pass the environment

// TODO we could probably create a base class for this...

GodotJavaWrapper::GodotJavaWrapper(JNIEnv *p_env, jobject p_godot_native_bridge) {
	godot_native_bridge = p_env->NewGlobalRef(p_godot_native_bridge);

	// get info about our Godot class so we can get pointers and stuff...
	godot_native_bridge_class = jni_find_class(p_env, "org/godotengine/godot/nativeapi/GodotNativeBridge");
	if (godot_native_bridge_class) {
		godot_native_bridge_class = (jclass)p_env->NewGlobalRef(godot_native_bridge_class);
	} else {
		// this is a pretty serious fail.. bail... pointers will stay 0
		return;
	}

	// get some Godot method pointers...
	_restart = p_env->GetMethodID(godot_native_bridge_class, "restart", "()V");
	_finish = p_env->GetMethodID(godot_native_bridge_class, "forceQuit", "(I)Z");
	_set_keep_screen_on = p_env->GetMethodID(godot_native_bridge_class, "setKeepScreenOn", "(Z)V");
	_alert = p_env->GetMethodID(godot_native_bridge_class, "alert", "(Ljava/lang/String;Ljava/lang/String;)V");
	_is_dark_mode_supported = p_env->GetMethodID(godot_native_bridge_class, "isDarkModeSupported", "()Z");
	_is_dark_mode = p_env->GetMethodID(godot_native_bridge_class, "isDarkMode", "()Z");
	_get_accent_color = p_env->GetMethodID(godot_native_bridge_class, "getAccentColor", "()I");
	_get_base_color = p_env->GetMethodID(godot_native_bridge_class, "getBaseColor", "()I");
	_get_clipboard = p_env->GetMethodID(godot_native_bridge_class, "getClipboard", "()Ljava/lang/String;");
	_set_clipboard = p_env->GetMethodID(godot_native_bridge_class, "setClipboard", "(Ljava/lang/String;)V");
	_has_clipboard = p_env->GetMethodID(godot_native_bridge_class, "hasClipboard", "()Z");
	_show_dialog = p_env->GetMethodID(godot_native_bridge_class, "showDialog", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
	_show_input_dialog = p_env->GetMethodID(godot_native_bridge_class, "showInputDialog", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	_show_file_picker = p_env->GetMethodID(godot_native_bridge_class, "showFilePicker", "(Ljava/lang/String;Ljava/lang/String;I[Ljava/lang/String;)V");
	_request_permission = p_env->GetMethodID(godot_native_bridge_class, "requestPermission", "(Ljava/lang/String;)Z");
	_request_permissions = p_env->GetMethodID(godot_native_bridge_class, "requestPermissions", "()Z");
	_get_granted_permissions = p_env->GetMethodID(godot_native_bridge_class, "getGrantedPermissions", "()[Ljava/lang/String;");
	_get_ca_certificates = p_env->GetMethodID(godot_native_bridge_class, "getCACertificates", "()Ljava/lang/String;");
	_init_input_devices = p_env->GetMethodID(godot_native_bridge_class, "initInputDevices", "()V");
	_vibrate = p_env->GetMethodID(godot_native_bridge_class, "vibrate", "(II)V");
	_get_input_fallback_mapping = p_env->GetMethodID(godot_native_bridge_class, "getInputFallbackMapping", "()Ljava/lang/String;");
	_on_godot_setup_completed = p_env->GetMethodID(godot_native_bridge_class, "onGodotSetupCompleted", "()V");
	_on_godot_main_loop_started = p_env->GetMethodID(godot_native_bridge_class, "onGodotMainLoopStarted", "()V");
	_on_godot_terminating = p_env->GetMethodID(godot_native_bridge_class, "onGodotTerminating", "()V");
	_create_new_godot_instance = p_env->GetMethodID(godot_native_bridge_class, "createNewGodotInstance", "([Ljava/lang/String;)I");
	_get_render_view = p_env->GetMethodID(godot_native_bridge_class, "getRenderView", "()Lorg/godotengine/godot/GodotRenderView;");
	_begin_benchmark_measure = p_env->GetMethodID(godot_native_bridge_class, "nativeBeginBenchmarkMeasure", "(Ljava/lang/String;Ljava/lang/String;)V");
	_end_benchmark_measure = p_env->GetMethodID(godot_native_bridge_class, "nativeEndBenchmarkMeasure", "(Ljava/lang/String;Ljava/lang/String;)V");
	_dump_benchmark = p_env->GetMethodID(godot_native_bridge_class, "nativeDumpBenchmark", "(Ljava/lang/String;)V");
	_get_gdextension_list_config_file = p_env->GetMethodID(godot_native_bridge_class, "getGDExtensionConfigFiles", "()[Ljava/lang/String;");
	_check_internal_feature_support = p_env->GetMethodID(godot_native_bridge_class, "checkInternalFeatureSupport", "(Ljava/lang/String;)Z");
	_sign_apk = p_env->GetMethodID(godot_native_bridge_class, "nativeSignApk", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
	_verify_apk = p_env->GetMethodID(godot_native_bridge_class, "nativeVerifyApk", "(Ljava/lang/String;)I");
	_enable_immersive_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeEnableImmersiveMode", "(Z)V");
	_is_in_immersive_mode = p_env->GetMethodID(godot_native_bridge_class, "isInImmersiveMode", "()Z");
	_set_window_color = p_env->GetMethodID(godot_native_bridge_class, "setWindowColor", "(Ljava/lang/String;)V");
	_on_editor_workspace_selected = p_env->GetMethodID(godot_native_bridge_class, "nativeOnEditorWorkspaceSelected", "(Ljava/lang/String;)V");
	_on_distraction_free_mode_changed = p_env->GetMethodID(godot_native_bridge_class, "nativeOnDistractionFreeModeChanged", "(Z)V");
	_get_activity = p_env->GetMethodID(godot_native_bridge_class, "getActivity", "()Landroid/app/Activity;");
	_build_env_connect = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvConnect", "(Lorg/godotengine/godot/variant/Callable;)Z");
	_build_env_disconnect = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvDisconnect", "()V");
	_build_env_execute = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvExecute", "(Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;Lorg/godotengine/godot/variant/Callable;)I");
	_build_env_cancel = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvCancel", "(I)V");
	_build_env_clean_project = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvCleanProject", "(Ljava/lang/String;Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;)V");

	// PiP mode method ids.
	_is_pip_mode_supported = p_env->GetMethodID(godot_native_bridge_class, "nativeIsPiPModeSupported", "()Z");
	_is_in_pip_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeIsInPiPMode", "()Z");
	_enter_pip_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeEnterPiPMode", "()V");
	_set_pip_mode_aspect_ratio = p_env->GetMethodID(godot_native_bridge_class, "nativeSetPiPModeAspectRatio", "(II)V");
	_set_auto_enter_pip_mode_on_background = p_env->GetMethodID(godot_native_bridge_class, "nativeSetAutoEnterPiPModeOnBackground", "(Z)V");
}

GodotJavaWrapper::~GodotJavaWrapper() {
	if (godot_view) {
		delete godot_view;
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);
	env->DeleteGlobalRef(godot_native_bridge);
	env->DeleteGlobalRef(godot_native_bridge_class);
}

jobject GodotJavaWrapper::get_activity() {
	if (_get_activity) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, nullptr);
		jobject activity = env->CallObjectMethod(godot_native_bridge, _get_activity);
		return activity;
	}
	return nullptr;
}

GodotJavaViewWrapper *GodotJavaWrapper::get_godot_view() {
	if (godot_view != nullptr) {
		return godot_view;
	}
	if (_get_render_view) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, nullptr);
		jobject godot_render_view = env->CallObjectMethod(godot_native_bridge, _get_render_view);
		if (!env->IsSameObject(godot_render_view, nullptr)) {
			godot_view = new GodotJavaViewWrapper(godot_render_view);
		}
	}
	return godot_view;
}

void GodotJavaWrapper::on_godot_setup_completed(JNIEnv *p_env) {
	if (_on_godot_setup_completed) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_setup_completed);
	}
}

void GodotJavaWrapper::on_godot_main_loop_started(JNIEnv *p_env) {
	if (_on_godot_main_loop_started) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_main_loop_started);
	}
}

void GodotJavaWrapper::on_godot_terminating(JNIEnv *p_env) {
	if (_on_godot_terminating) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_terminating);
	}
}

void GodotJavaWrapper::restart(JNIEnv *p_env) {
	if (_restart) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _restart);
	}
}

bool GodotJavaWrapper::force_quit(JNIEnv *p_env, int p_instance_id) {
	if (_finish) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL_V(p_env, false);
		return p_env->CallBooleanMethod(godot_native_bridge, _finish, p_instance_id);
	}
	return false;
}

void GodotJavaWrapper::set_keep_screen_on(bool p_enabled) {
	if (_set_keep_screen_on) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_keep_screen_on, p_enabled);
	}
}

void GodotJavaWrapper::alert(const String &p_message, const String &p_title) {
	if (_alert) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStrMessage = env->NewStringUTF(p_message.utf8().get_data());
		jstring jStrTitle = env->NewStringUTF(p_title.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _alert, jStrMessage, jStrTitle);
		env->DeleteLocalRef(jStrMessage);
		env->DeleteLocalRef(jStrTitle);
	}
}

bool GodotJavaWrapper::is_dark_mode_supported() {
	if (_is_dark_mode_supported) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_dark_mode_supported);
	} else {
		return false;
	}
}

bool GodotJavaWrapper::is_dark_mode() {
	if (_is_dark_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_dark_mode);
	} else {
		return false;
	}
}

// Convert ARGB to RGBA.
static Color _argb_to_rgba(int p_color) {
	int alpha = (p_color >> 24) & 0xFF;
	int red = (p_color >> 16) & 0xFF;
	int green = (p_color >> 8) & 0xFF;
	int blue = p_color & 0xFF;
	return Color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
}

Color GodotJavaWrapper::get_accent_color() {
	if (_get_accent_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, Color(0, 0, 0, 0));
		int accent_color = env->CallIntMethod(godot_native_bridge, _get_accent_color);
		return _argb_to_rgba(accent_color);
	} else {
		return Color(0, 0, 0, 0);
	}
}

Color GodotJavaWrapper::get_base_color() {
	if (_get_base_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, Color(0, 0, 0, 0));
		int base_color = env->CallIntMethod(godot_native_bridge, _get_base_color);
		return _argb_to_rgba(base_color);
	} else {
		return Color(0, 0, 0, 0);
	}
}

bool GodotJavaWrapper::has_get_clipboard() {
	return _get_clipboard != nullptr;
}

String GodotJavaWrapper::get_clipboard() {
	String clipboard;
	if (_get_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_native_bridge, _get_clipboard);
		clipboard = jstring_to_string(s, env);
		env->DeleteLocalRef(s);
	}
	return clipboard;
}

String GodotJavaWrapper::get_input_fallback_mapping() {
	String input_fallback_mapping;
	if (_get_input_fallback_mapping) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring fallback_mapping = (jstring)env->CallObjectMethod(godot_native_bridge, _get_input_fallback_mapping);
		input_fallback_mapping = jstring_to_string(fallback_mapping, env);
		env->DeleteLocalRef(fallback_mapping);
	}
	return input_fallback_mapping;
}

bool GodotJavaWrapper::has_set_clipboard() {
	return _set_clipboard != nullptr;
}

void GodotJavaWrapper::set_clipboard(const String &p_text) {
	if (_set_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStr = env->NewStringUTF(p_text.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _set_clipboard, jStr);
		env->DeleteLocalRef(jStr);
	}
}

bool GodotJavaWrapper::has_has_clipboard() {
	return _has_clipboard != nullptr;
}

bool GodotJavaWrapper::has_clipboard() {
	if (_has_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _has_clipboard);
	} else {
		return false;
	}
}

Error GodotJavaWrapper::show_dialog(const String &p_title, const String &p_description, const Vector<String> &p_buttons) {
	if (_show_input_dialog) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring j_title = env->NewStringUTF(p_title.utf8().get_data());
		jstring j_description = env->NewStringUTF(p_description.utf8().get_data());
		jobjectArray j_buttons = env->NewObjectArray(p_buttons.size(), jni_find_class(env, "java/lang/String"), nullptr);
		for (int i = 0; i < p_buttons.size(); ++i) {
			jstring j_button = env->NewStringUTF(p_buttons[i].utf8().get_data());
			env->SetObjectArrayElement(j_buttons, i, j_button);
			env->DeleteLocalRef(j_button);
		}
		env->CallVoidMethod(godot_native_bridge, _show_dialog, j_title, j_description, j_buttons);
		env->DeleteLocalRef(j_title);
		env->DeleteLocalRef(j_description);
		env->DeleteLocalRef(j_buttons);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::show_input_dialog(const String &p_title, const String &p_message, const String &p_existing_text) {
	if (_show_input_dialog) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring jStrTitle = env->NewStringUTF(p_title.utf8().get_data());
		jstring jStrMessage = env->NewStringUTF(p_message.utf8().get_data());
		jstring jStrExistingText = env->NewStringUTF(p_existing_text.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _show_input_dialog, jStrTitle, jStrMessage, jStrExistingText);
		env->DeleteLocalRef(jStrTitle);
		env->DeleteLocalRef(jStrMessage);
		env->DeleteLocalRef(jStrExistingText);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::show_file_picker(const String &p_current_directory, const String &p_filename, int p_mode, const Vector<String> &p_filters) {
	if (_show_file_picker) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring j_current_directory = env->NewStringUTF(p_current_directory.utf8().get_data());
		jstring j_filename = env->NewStringUTF(p_filename.utf8().get_data());
		jint j_mode = p_mode;
		Vector<String> filters;
		for (const String &E : p_filters) {
			filters.append_array(E.get_slicec(';', 0).split(",")); // Add extensions.
			filters.append_array(E.get_slicec(';', 2).split(",")); // Add MIME types.
		}
		jobjectArray j_filters = env->NewObjectArray(filters.size(), jni_find_class(env, "java/lang/String"), nullptr);
		for (int i = 0; i < filters.size(); ++i) {
			jstring j_filter = env->NewStringUTF(filters[i].utf8().get_data());
			env->SetObjectArrayElement(j_filters, i, j_filter);
			env->DeleteLocalRef(j_filter);
		}
		env->CallVoidMethod(godot_native_bridge, _show_file_picker, j_current_directory, j_filename, j_mode, j_filters);
		env->DeleteLocalRef(j_current_directory);
		env->DeleteLocalRef(j_filename);
		env->DeleteLocalRef(j_filters);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

bool GodotJavaWrapper::request_permission(const String &p_name) {
	if (_request_permission) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		jstring jStrName = env->NewStringUTF(p_name.utf8().get_data());
		bool result = env->CallBooleanMethod(godot_native_bridge, _request_permission, jStrName);
		env->DeleteLocalRef(jStrName);
		return result;
	} else {
		return false;
	}
}

bool GodotJavaWrapper::request_permissions() {
	if (_request_permissions) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _request_permissions);
	} else {
		return false;
	}
}

Vector<String> GodotJavaWrapper::get_granted_permissions() const {
	Vector<String> permissions_list;
	if (_get_granted_permissions) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, permissions_list);
		jobject permissions_object = env->CallObjectMethod(godot_native_bridge, _get_granted_permissions);
		jobjectArray *arr = reinterpret_cast<jobjectArray *>(&permissions_object);

		jsize len = env->GetArrayLength(*arr);
		for (int i = 0; i < len; i++) {
			jstring jstr = (jstring)env->GetObjectArrayElement(*arr, i);
			String str = jstring_to_string(jstr, env);
			permissions_list.push_back(str);
			env->DeleteLocalRef(jstr);
		}
	}
	return permissions_list;
}

Vector<String> GodotJavaWrapper::get_gdextension_list_config_file() const {
	Vector<String> config_file_list;
	if (_get_gdextension_list_config_file) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, config_file_list);
		jobject config_file_list_object = env->CallObjectMethod(godot_native_bridge, _get_gdextension_list_config_file);
		jobjectArray *arr = reinterpret_cast<jobjectArray *>(&config_file_list_object);

		jsize len = env->GetArrayLength(*arr);
		for (int i = 0; i < len; i++) {
			jstring j_config_file = (jstring)env->GetObjectArrayElement(*arr, i);
			String config_file = jstring_to_string(j_config_file, env);
			config_file_list.push_back(config_file);
			env->DeleteLocalRef(j_config_file);
		}
	}
	return config_file_list;
}

String GodotJavaWrapper::get_ca_certificates() const {
	String ca_certificates;
	if (_get_ca_certificates) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_native_bridge, _get_ca_certificates);
		ca_certificates = jstring_to_string(s, env);
		env->DeleteLocalRef(s);
	}
	return ca_certificates;
}

void GodotJavaWrapper::init_input_devices() {
	if (_init_input_devices) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _init_input_devices);
	}
}

void GodotJavaWrapper::vibrate(int p_duration_ms, float p_amplitude) {
	if (_vibrate) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		int j_amplitude = -1.0;

		if (p_amplitude != -1.0) {
			j_amplitude = CLAMP(int(p_amplitude * 255), 1, 255);
		}

		env->CallVoidMethod(godot_native_bridge, _vibrate, p_duration_ms, j_amplitude);
	}
}

int GodotJavaWrapper::create_new_godot_instance(const List<String> &args) {
	if (_create_new_godot_instance) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		jobjectArray jargs = env->NewObjectArray(args.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));
		int i = 0;
		for (List<String>::ConstIterator itr = args.begin(); itr != args.end(); ++itr, ++i) {
			jstring j_arg = env->NewStringUTF(itr->utf8().get_data());
			env->SetObjectArrayElement(jargs, i, j_arg);
			env->DeleteLocalRef(j_arg);
		}
		return env->CallIntMethod(godot_native_bridge, _create_new_godot_instance, jargs);
	} else {
		return 0;
	}
}

void GodotJavaWrapper::begin_benchmark_measure(const String &p_context, const String &p_label) {
	if (_begin_benchmark_measure) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_context = env->NewStringUTF(p_context.utf8().get_data());
		jstring j_label = env->NewStringUTF(p_label.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _begin_benchmark_measure, j_context, j_label);
		env->DeleteLocalRef(j_context);
		env->DeleteLocalRef(j_label);
	}
}

void GodotJavaWrapper::end_benchmark_measure(const String &p_context, const String &p_label) {
	if (_end_benchmark_measure) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_context = env->NewStringUTF(p_context.utf8().get_data());
		jstring j_label = env->NewStringUTF(p_label.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _end_benchmark_measure, j_context, j_label);
		env->DeleteLocalRef(j_context);
		env->DeleteLocalRef(j_label);
	}
}

void GodotJavaWrapper::dump_benchmark(const String &benchmark_file) {
	if (_dump_benchmark) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_benchmark_file = env->NewStringUTF(benchmark_file.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _dump_benchmark, j_benchmark_file);
		env->DeleteLocalRef(j_benchmark_file);
	}
}

bool GodotJavaWrapper::check_internal_feature_support(const String &p_feature) const {
	if (_check_internal_feature_support) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		jstring j_feature = env->NewStringUTF(p_feature.utf8().get_data());
		bool result = env->CallBooleanMethod(godot_native_bridge, _check_internal_feature_support, j_feature);
		env->DeleteLocalRef(j_feature);
		return result;
	} else {
		return false;
	}
}

Error GodotJavaWrapper::sign_apk(const String &p_input_path, const String &p_output_path, const String &p_keystore_path, const String &p_keystore_user, const String &p_keystore_password) {
	if (_sign_apk) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		jstring j_input_path = env->NewStringUTF(p_input_path.utf8().get_data());
		jstring j_output_path = env->NewStringUTF(p_output_path.utf8().get_data());
		jstring j_keystore_path = env->NewStringUTF(p_keystore_path.utf8().get_data());
		jstring j_keystore_user = env->NewStringUTF(p_keystore_user.utf8().get_data());
		jstring j_keystore_password = env->NewStringUTF(p_keystore_password.utf8().get_data());

		int result = env->CallIntMethod(godot_native_bridge, _sign_apk, j_input_path, j_output_path, j_keystore_path, j_keystore_user, j_keystore_password);

		env->DeleteLocalRef(j_input_path);
		env->DeleteLocalRef(j_output_path);
		env->DeleteLocalRef(j_keystore_path);
		env->DeleteLocalRef(j_keystore_user);
		env->DeleteLocalRef(j_keystore_password);

		return static_cast<Error>(result);
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::verify_apk(const String &p_apk_path) {
	if (_verify_apk) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		jstring j_apk_path = env->NewStringUTF(p_apk_path.utf8().get_data());
		int result = env->CallIntMethod(godot_native_bridge, _verify_apk, j_apk_path);
		env->DeleteLocalRef(j_apk_path);
		return static_cast<Error>(result);
	} else {
		return ERR_UNCONFIGURED;
	}
}

void GodotJavaWrapper::enable_immersive_mode(bool p_enabled) {
	if (_enable_immersive_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _enable_immersive_mode, p_enabled);
	}
}

bool GodotJavaWrapper::is_in_immersive_mode() {
	if (_is_in_immersive_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_in_immersive_mode);
	} else {
		return false;
	}
}

void GodotJavaWrapper::set_window_color(const Color &p_color) {
	if (_set_window_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		String color = "#" + p_color.to_html(false);
		jstring jStrColor = env->NewStringUTF(color.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _set_window_color, jStrColor);
	}
}

void GodotJavaWrapper::on_editor_workspace_selected(const String &p_workspace) {
	if (_on_editor_workspace_selected) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring j_workspace = env->NewStringUTF(p_workspace.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _on_editor_workspace_selected, j_workspace);
	}
}

void GodotJavaWrapper::on_distraction_free_mode_changed(bool p_enabled) {
	if (_on_distraction_free_mode_changed) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(godot_native_bridge, _on_distraction_free_mode_changed, p_enabled);
	}
}

bool GodotJavaWrapper::build_env_connect(const Callable &p_callback) {
	if (_build_env_connect) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		jobject j_callback = callable_to_jcallable(env, p_callback);
		jboolean result = env->CallBooleanMethod(godot_native_bridge, _build_env_connect, j_callback);
		env->DeleteLocalRef(j_callback);

		return result;
	}

	return false;
}

void GodotJavaWrapper::build_env_disconnect() {
	if (_build_env_disconnect) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(godot_native_bridge, _build_env_disconnect);
	}
}

int GodotJavaWrapper::build_env_execute(const String &p_build_tool, const List<String> &p_arguments, const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_output_callback, const Callable &p_result_callback) {
	if (_build_env_execute) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, -1);

		jstring j_build_tool = env->NewStringUTF(p_build_tool.utf8().get_data());
		jobjectArray j_args = env->NewObjectArray(p_arguments.size(), env->FindClass("java/lang/String"), nullptr);
		for (int i = 0; i < p_arguments.size(); i++) {
			jstring j_arg = env->NewStringUTF(p_arguments.get(i).utf8().get_data());
			env->SetObjectArrayElement(j_args, i, j_arg);
			env->DeleteLocalRef(j_arg);
		}
		jstring j_project_path = env->NewStringUTF(p_project_path.utf8().get_data());
		jstring j_gradle_build_directory = env->NewStringUTF(p_gradle_build_directory.utf8().get_data());
		jobject j_output_callback = callable_to_jcallable(env, p_output_callback);
		jobject j_result_callback = callable_to_jcallable(env, p_result_callback);

		jint result = env->CallIntMethod(godot_native_bridge, _build_env_execute, j_build_tool, j_args, j_project_path, j_gradle_build_directory, j_output_callback, j_result_callback);

		env->DeleteLocalRef(j_build_tool);
		env->DeleteLocalRef(j_args);
		env->DeleteLocalRef(j_project_path);
		env->DeleteLocalRef(j_gradle_build_directory);
		env->DeleteLocalRef(j_output_callback);
		env->DeleteLocalRef(j_result_callback);

		return result;
	}

	return -1;
}

void GodotJavaWrapper::build_env_cancel(int p_job_id) {
	if (_build_env_cancel) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _build_env_cancel, p_job_id);
	}
}

void GodotJavaWrapper::build_env_clean_project(const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_callback) {
	if (_build_env_clean_project) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring j_project_path = env->NewStringUTF(p_project_path.utf8().get_data());
		jstring j_gradle_build_directory = env->NewStringUTF(p_gradle_build_directory.utf8().get_data());
		jobject j_callback = callable_to_jcallable(env, p_callback);

		env->CallVoidMethod(godot_native_bridge, _build_env_clean_project, j_project_path, j_gradle_build_directory, j_callback);

		env->DeleteLocalRef(j_project_path);
		env->DeleteLocalRef(j_gradle_build_directory);
		env->DeleteLocalRef(j_callback);
	}
}

bool GodotJavaWrapper::is_pip_mode_supported() {
	if (_is_pip_mode_supported) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_pip_mode_supported);
	} else {
		return false;
	}
}

bool GodotJavaWrapper::is_in_pip_mode() {
	if (_is_in_pip_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_in_pip_mode);
	} else {
		return false;
	}
}

void GodotJavaWrapper::enter_pip_mode() {
	if (_enter_pip_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _enter_pip_mode);
	}
}

void GodotJavaWrapper::set_pip_mode_aspect_ratio(int p_numerator, int p_denominator) {
	if (_set_pip_mode_aspect_ratio) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_pip_mode_aspect_ratio, p_numerator, p_denominator);
	}
}

void GodotJavaWrapper::set_auto_enter_pip_mode_on_background(bool p_auto_enter_on_background) {
	if (_set_auto_enter_pip_mode_on_background) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_auto_enter_pip_mode_on_background, p_auto_enter_on_background);
	}
}


// --- Merged from platform/android/java_godot_view_wrapper.cpp ---
#include "servers/display/display_server.h"


#include "core/string/ustring.h"

GodotJavaViewWrapper::GodotJavaViewWrapper(jobject godot_view) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	_godot_view = env->NewGlobalRef(godot_view);

	_cls = (jclass)env->NewGlobalRef(env->GetObjectClass(godot_view));

	_configure_pointer_icon = env->GetMethodID(_cls, "configurePointerIcon", "(ILjava/lang/String;FF)V");
	_set_pointer_icon = env->GetMethodID(_cls, "setPointerIcon", "(I)V");

	int android_device_api_level = android_get_device_api_level();
	if (android_device_api_level >= __ANDROID_API_O__) {
		_request_pointer_capture = env->GetMethodID(_cls, "requestPointerCapture", "()V");
		_release_pointer_capture = env->GetMethodID(_cls, "releasePointerCapture", "()V");
	}

	_can_capture_pointer = env->GetMethodID(_cls, "canCapturePointer", "()Z");
}

bool GodotJavaViewWrapper::can_update_pointer_icon() const {
	return _configure_pointer_icon != nullptr && _set_pointer_icon != nullptr;
}

bool GodotJavaViewWrapper::can_capture_pointer() const {
	// We can capture the pointer if the other jni capture method ids are initialized,
	// and GodotView#canCapturePointer() returns true.
	if (_request_pointer_capture != nullptr && _release_pointer_capture != nullptr && _can_capture_pointer != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		return env->CallBooleanMethod(_godot_view, _can_capture_pointer);
	}

	return false;
}

void GodotJavaViewWrapper::request_pointer_capture() {
	if (_request_pointer_capture != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _request_pointer_capture);
	}
}

void GodotJavaViewWrapper::release_pointer_capture() {
	if (_release_pointer_capture != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _release_pointer_capture);
	}
}

void GodotJavaViewWrapper::configure_pointer_icon(int pointer_type, const String &image_path, const Vector2 &p_hotspot) {
	if (_configure_pointer_icon != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring jImagePath = env->NewStringUTF(image_path.utf8().get_data());
		env->CallVoidMethod(_godot_view, _configure_pointer_icon, pointer_type, jImagePath, p_hotspot.x, p_hotspot.y);
		env->DeleteLocalRef(jImagePath);
	}
}

void GodotJavaViewWrapper::set_pointer_icon(int pointer_type) {
	if (_set_pointer_icon != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _set_pointer_icon, pointer_type);
	}
}

GodotJavaViewWrapper::~GodotJavaViewWrapper() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(_godot_view);
	env->DeleteGlobalRef(_cls);
}


// --- Merged from platform/android/java_godot_io_wrapper.cpp ---
#include "servers/display/display_server.h"


#include "core/io/ip.h"

// JNIEnv is only valid within the thread it belongs to, in a multi threading environment
// we can't cache it.
// For GodotIO we call all access methods from our thread and we thus get a valid JNIEnv
// from get_jni_env().

GodotIOJavaWrapper::GodotIOJavaWrapper(JNIEnv *p_env, jobject p_godot_io_instance) {
	godot_io_instance = p_env->NewGlobalRef(p_godot_io_instance);
	if (godot_io_instance) {
		cls = p_env->GetObjectClass(godot_io_instance);
		if (cls) {
			cls = (jclass)p_env->NewGlobalRef(cls);
		} else {
			// this is a pretty serious fail.. bail... pointers will stay 0
			return;
		}

		_open_URI = p_env->GetMethodID(cls, "openURI", "(Ljava/lang/String;)I");
		_get_cache_dir = p_env->GetMethodID(cls, "getCacheDir", "()Ljava/lang/String;");
		_get_temp_dir = p_env->GetMethodID(cls, "getTempDir", "()Ljava/lang/String;");
		_get_data_dir = p_env->GetMethodID(cls, "getDataDir", "()Ljava/lang/String;");
		_get_display_cutouts = p_env->GetMethodID(cls, "getDisplayCutouts", "()[I"),
		_get_display_safe_area = p_env->GetMethodID(cls, "getDisplaySafeArea", "()[I"),
		_get_locale = p_env->GetMethodID(cls, "getLocale", "()Ljava/lang/String;");
		_get_model = p_env->GetMethodID(cls, "getModel", "()Ljava/lang/String;");
		_get_screen_DPI = p_env->GetMethodID(cls, "getScreenDPI", "()I");
		_get_scaled_density = p_env->GetMethodID(cls, "getScaledDensity", "()F");
		_get_screen_refresh_rate = p_env->GetMethodID(cls, "getScreenRefreshRate", "(D)D");
		_get_unique_id = p_env->GetMethodID(cls, "getUniqueID", "()Ljava/lang/String;");
		_show_keyboard = p_env->GetMethodID(cls, "showKeyboard", "(Ljava/lang/String;IIII)V");
		_hide_keyboard = p_env->GetMethodID(cls, "hideKeyboard", "()V");
		_has_hardware_keyboard = p_env->GetMethodID(cls, "hasHardwareKeyboard", "()Z");
		_set_screen_orientation = p_env->GetMethodID(cls, "setScreenOrientation", "(I)V");
		_get_screen_orientation = p_env->GetMethodID(cls, "getScreenOrientation", "()I");
		_get_system_dir = p_env->GetMethodID(cls, "getSystemDir", "(IZ)Ljava/lang/String;");
		_get_display_rotation = p_env->GetMethodID(cls, "getDisplayRotation", "()I");
	}
}

GodotIOJavaWrapper::~GodotIOJavaWrapper() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(cls);
	env->DeleteGlobalRef(godot_io_instance);
}

jobject GodotIOJavaWrapper::get_instance() {
	return godot_io_instance;
}

Error GodotIOJavaWrapper::open_uri(const String &p_uri) {
	if (_open_URI) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNAVAILABLE);
		jstring jStr = env->NewStringUTF(p_uri.utf8().get_data());
		Error result = env->CallIntMethod(godot_io_instance, _open_URI, jStr) ? ERR_CANT_OPEN : OK;
		env->DeleteLocalRef(jStr);
		return result;
	} else {
		return ERR_UNAVAILABLE;
	}
}

String GodotIOJavaWrapper::get_cache_dir() {
	if (_get_cache_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_cache_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_temp_dir() {
	if (_get_temp_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_temp_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_user_data_dir(const String &p_user_dir) {
	if (_get_data_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_data_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_locale() {
	if (_get_locale) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_locale);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_model() {
	if (_get_model) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_model);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

int GodotIOJavaWrapper::get_screen_dpi() {
	if (_get_screen_DPI) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 160);
		return env->CallIntMethod(godot_io_instance, _get_screen_DPI);
	} else {
		return 160;
	}
}

float GodotIOJavaWrapper::get_scaled_density() {
	if (_get_scaled_density) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 1.0f);
		return env->CallFloatMethod(godot_io_instance, _get_scaled_density);
	} else {
		return 1.0f;
	}
}

float GodotIOJavaWrapper::get_screen_refresh_rate(float fallback) {
	if (_get_screen_refresh_rate) {
		JNIEnv *env = get_jni_env();
		if (env == nullptr) {
			ERR_PRINT("An error occurred while trying to get screen refresh rate.");
			return fallback;
		}
		return (float)env->CallDoubleMethod(godot_io_instance, _get_screen_refresh_rate, (double)fallback);
	}
	ERR_PRINT("An error occurred while trying to get the screen refresh rate.");
	return fallback;
}

TypedArray<Rect2> GodotIOJavaWrapper::get_display_cutouts() {
	TypedArray<Rect2> result;
	ERR_FAIL_NULL_V(_get_display_cutouts, result);
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, result);
	jintArray returnArray = (jintArray)env->CallObjectMethod(godot_io_instance, _get_display_cutouts);
	jint arrayLength = env->GetArrayLength(returnArray);
	jint *arrayBody = env->GetIntArrayElements(returnArray, JNI_FALSE);
	int cutouts = arrayLength / 4;
	for (int i = 0; i < cutouts; i++) {
		int x = arrayBody[i * 4];
		int y = arrayBody[i * 4 + 1];
		int width = arrayBody[i * 4 + 2];
		int height = arrayBody[i * 4 + 3];
		Rect2 cutout(x, y, width, height);
		result.append(cutout);
	}
	env->ReleaseIntArrayElements(returnArray, arrayBody, 0);
	return result;
}

Rect2i GodotIOJavaWrapper::get_display_safe_area() {
	Rect2i result;
	ERR_FAIL_NULL_V(_get_display_safe_area, result);
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, result);
	jintArray returnArray = (jintArray)env->CallObjectMethod(godot_io_instance, _get_display_safe_area);
	ERR_FAIL_COND_V(env->GetArrayLength(returnArray) != 4, result);
	jint *arrayBody = env->GetIntArrayElements(returnArray, JNI_FALSE);
	result = Rect2i(arrayBody[0], arrayBody[1], arrayBody[2], arrayBody[3]);
	env->ReleaseIntArrayElements(returnArray, arrayBody, 0);
	return result;
}

String GodotIOJavaWrapper::get_unique_id() {
	if (_get_unique_id) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_unique_id);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

bool GodotIOJavaWrapper::has_vk() {
	return (_show_keyboard != nullptr) && (_hide_keyboard != nullptr);
}

bool GodotIOJavaWrapper::has_hardware_keyboard() {
	if (_has_hardware_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_io_instance, _has_hardware_keyboard);
	} else {
		return false;
	}
}

void GodotIOJavaWrapper::show_vk(const String &p_existing, int p_type, int p_max_input_length, int p_cursor_start, int p_cursor_end) {
	if (_show_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStr = env->NewStringUTF(p_existing.utf8().get_data());
		env->CallVoidMethod(godot_io_instance, _show_keyboard, jStr, p_type, p_max_input_length, p_cursor_start, p_cursor_end);
		env->DeleteLocalRef(jStr);
	}
}

void GodotIOJavaWrapper::hide_vk() {
	if (_hide_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_io_instance, _hide_keyboard);
	}
}

void GodotIOJavaWrapper::set_screen_orientation(int p_orient) {
	if (_set_screen_orientation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_io_instance, _set_screen_orientation, p_orient);
	}
}

int GodotIOJavaWrapper::get_screen_orientation() {
	if (_get_screen_orientation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallIntMethod(godot_io_instance, _get_screen_orientation);
	} else {
		return 0;
	}
}

String GodotIOJavaWrapper::get_system_dir(int p_dir, bool p_shared_storage) {
	if (_get_system_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String("."));
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_system_dir, p_dir, p_shared_storage);
		return jstring_to_string(s, env);
	} else {
		return String(".");
	}
}

int GodotIOJavaWrapper::get_display_rotation() {
	if (_get_display_rotation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallIntMethod(godot_io_instance, _get_display_rotation);
	} else {
		return 0;
	}
}

// SafeNumeric because it can be changed from non-main thread and we need to
// ensure the change is immediately visible to other threads.
static SafeNumeric<int> virtual_keyboard_height;

int GodotIOJavaWrapper::get_vk_height() {
	return virtual_keyboard_height.get();
}

void GodotIOJavaWrapper::set_vk_height(int p_height) {
	virtual_keyboard_height.set(p_height);
}


// --- Merged from platform/android/jni_utils.cpp ---
#include "servers/display/display_server.h"


// Maximum recursion depth allowed when converting variants to jobjects or vice-versa.
#define RECURSION_DEPTH_LIMIT 64

static jobject android_class_loader = nullptr;
static jmethodID load_class_method = nullptr;

jobject callable_to_jcallable(JNIEnv *p_env, const Variant &p_callable) {
	ERR_FAIL_NULL_V(p_env, nullptr);
	if (p_callable.get_type() != Variant::CALLABLE) {
		return nullptr;
	}

	Variant *callable_jcopy = memnew(Variant(p_callable));

	jclass bclass = jni_find_class(p_env, "org/godotengine/godot/variant/Callable");
	jmethodID ctor = p_env->GetMethodID(bclass, "<init>", "(J)V");
	jobject jcallable = p_env->NewObject(bclass, ctor, reinterpret_cast<int64_t>(callable_jcopy));
	p_env->DeleteLocalRef(bclass);

	return jcallable;
}

Callable jcallable_to_callable(JNIEnv *p_env, jobject p_jcallable_obj) {
	ERR_FAIL_NULL_V(p_env, Callable());

	const Variant *callable_variant = nullptr;
	if (p_jcallable_obj) {
		jclass callable_class = jni_find_class(p_env, "org/godotengine/godot/variant/Callable");
		if (callable_class && p_env->IsInstanceOf(p_jcallable_obj, callable_class)) {
			jmethodID get_native_pointer = p_env->GetMethodID(callable_class, "getNativePointer", "()J");
			jlong native_callable = p_env->CallLongMethod(p_jcallable_obj, get_native_pointer);

			callable_variant = reinterpret_cast<const Variant *>(native_callable);
		}

		p_env->DeleteLocalRef(callable_class);
	}

	ERR_FAIL_NULL_V(callable_variant, Callable());
	return *callable_variant;
}

String charsequence_to_string(JNIEnv *p_env, jobject p_charsequence) {
	ERR_FAIL_NULL_V(p_env, String());

	String result;
	if (p_charsequence) {
		jclass bclass = jni_find_class(p_env, "java/lang/CharSequence");
		if (bclass && p_env->IsInstanceOf(p_charsequence, bclass)) {
			jmethodID to_string = p_env->GetMethodID(bclass, "toString", "()Ljava/lang/String;");
			jstring obj_string = (jstring)p_env->CallObjectMethod(p_charsequence, to_string);

			result = jstring_to_string(obj_string, p_env);
			p_env->DeleteLocalRef(obj_string);
		}

		p_env->DeleteLocalRef(bclass);
	}
	return result;
}

jobject _variant_to_jobject(JNIEnv *env, Variant::Type p_type, const Variant *p_arg, int p_depth) {
	jobject ret = nullptr;

	if (p_depth > RECURSION_DEPTH_LIMIT) {
		ERR_PRINT("Variant is too deep! Bailing.");
		return ret;
	}

	env->PushLocalFrame(2);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return ret;
	}

	switch (p_type) {
		case Variant::BOOL: {
			jclass bclass = jni_find_class(env, "java/lang/Boolean");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(Z)V");
			jvalue val;
			val.z = (bool)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::INT: {
			jclass bclass = jni_find_class(env, "java/lang/Long");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(J)V");
			jvalue val;
			val.j = (jlong)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::FLOAT: {
			jclass bclass = jni_find_class(env, "java/lang/Double");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(D)V");
			jvalue val;
			val.d = (double)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::STRING: {
			String s = *p_arg;
			jstring jStr = env->NewStringUTF(s.utf8().get_data());
			ret = jStr;
		} break;
		case Variant::PACKED_STRING_ARRAY: {
			Vector<String> sarray = *p_arg;
			jobjectArray arr = env->NewObjectArray(sarray.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));

			for (int j = 0; j < sarray.size(); j++) {
				jstring str = env->NewStringUTF(sarray[j].utf8().get_data());
				env->SetObjectArrayElement(arr, j, str);
				env->DeleteLocalRef(str);
			}
			ret = arr;
		} break;

		case Variant::CALLABLE: {
			jobject jcallable = callable_to_jcallable(env, *p_arg);
			ret = jcallable;
		} break;

		case Variant::DICTIONARY: {
			Dictionary dict = *p_arg;
			jclass dclass = jni_find_class(env, "org/godotengine/godot/Dictionary");
			jmethodID ctor = env->GetMethodID(dclass, "<init>", "()V");
			jobject jdict = env->NewObject(dclass, ctor);

			Array keys = dict.keys();

			jobjectArray jkeys = env->NewObjectArray(keys.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));
			for (int j = 0; j < keys.size(); j++) {
				jstring str = env->NewStringUTF(String(keys[j]).utf8().get_data());
				env->SetObjectArrayElement(jkeys, j, str);
				env->DeleteLocalRef(str);
			}

			jmethodID set_keys = env->GetMethodID(dclass, "set_keys", "([Ljava/lang/String;)V");
			jvalue val;
			val.l = jkeys;
			env->CallVoidMethodA(jdict, set_keys, &val);
			env->DeleteLocalRef(jkeys);

			jobjectArray jvalues = env->NewObjectArray(keys.size(), jni_find_class(env, "java/lang/Object"), nullptr);

			for (int j = 0; j < keys.size(); j++) {
				Variant var = dict[keys[j]];
				jobject jvar = _variant_to_jobject(env, var.get_type(), &var, p_depth + 1);
				env->SetObjectArrayElement(jvalues, j, jvar);
				if (jvar) {
					env->DeleteLocalRef(jvar);
				}
			}

			jmethodID set_values = env->GetMethodID(dclass, "set_values", "([Ljava/lang/Object;)V");
			val.l = jvalues;
			env->CallVoidMethodA(jdict, set_values, &val);
			env->DeleteLocalRef(jvalues);
			env->DeleteLocalRef(dclass);

			ret = jdict;
		} break;

		case Variant::ARRAY: {
			Array array = *p_arg;
			jobjectArray arr = env->NewObjectArray(array.size(), jni_find_class(env, "java/lang/Object"), nullptr);

			for (int j = 0; j < array.size(); j++) {
				Variant var = array[j];
				jobject jvar = _variant_to_jobject(env, var.get_type(), &var, p_depth + 1);
				env->SetObjectArrayElement(arr, j, jvar);
				if (jvar) {
					env->DeleteLocalRef(jvar);
				}
			}
			ret = arr;
		} break;

		case Variant::PACKED_INT32_ARRAY: {
			Vector<int> array = *p_arg;
			jintArray arr = env->NewIntArray(array.size());
			const int *r = array.ptr();
			env->SetIntArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_INT64_ARRAY: {
			Vector<int64_t> array = *p_arg;
			jlongArray arr = env->NewLongArray(array.size());
			const int64_t *r = array.ptr();
			env->SetLongArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_BYTE_ARRAY: {
			Vector<uint8_t> array = *p_arg;
			jbyteArray arr = env->NewByteArray(array.size());
			const uint8_t *r = array.ptr();
			env->SetByteArrayRegion(arr, 0, array.size(), reinterpret_cast<const signed char *>(r));
			ret = arr;
		} break;
		case Variant::PACKED_FLOAT32_ARRAY: {
			Vector<float> array = *p_arg;
			jfloatArray arr = env->NewFloatArray(array.size());
			const float *r = array.ptr();
			env->SetFloatArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_FLOAT64_ARRAY: {
			Vector<double> array = *p_arg;
			jdoubleArray arr = env->NewDoubleArray(array.size());
			const double *r = array.ptr();
			env->SetDoubleArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::OBJECT: {
			Ref<JavaObject> generic_object = *p_arg;
			if (generic_object.is_valid()) {
				jobject obj = env->NewLocalRef(generic_object->get_instance());
				ret = obj;
			}
		} break;

		// Add default to prevent compiler warning about not handling all types.
		default:
			break;
	}

	return env->PopLocalFrame(ret);
}

String _get_class_name(JNIEnv *env, jclass cls, bool *array) {
	jclass cclass = jni_find_class(env, "java/lang/Class");
	jmethodID getName = env->GetMethodID(cclass, "getName", "()Ljava/lang/String;");
	jstring clsName = (jstring)env->CallObjectMethod(cls, getName);

	if (array) {
		jmethodID isArray = env->GetMethodID(cclass, "isArray", "()Z");
		jboolean isarr = env->CallBooleanMethod(cls, isArray);
		(*array) = isarr != 0;
	}
	String name = jstring_to_string(clsName, env);
	env->DeleteLocalRef(clsName);
	env->DeleteLocalRef(cclass);

	return name;
}

Variant _jobject_to_variant(JNIEnv *env, jobject obj, int p_depth) {
	ERR_FAIL_COND_V_MSG(p_depth > RECURSION_DEPTH_LIMIT, Variant(), "Variant is too deep! Bailing.");

	if (obj == nullptr) {
		return Variant();
	}

	jclass c = env->GetObjectClass(obj);
	bool array;
	String name = _get_class_name(env, c, &array);

	if (name == "java.lang.String") {
		return jstring_to_string((jstring)obj, env);
	}

	if (name == "java.lang.CharSequence") {
		return charsequence_to_string(env, obj);
	}

	if (name == "[Ljava.lang.String;") {
		jobjectArray arr = (jobjectArray)obj;
		int stringCount = env->GetArrayLength(arr);
		Vector<String> sarr;

		for (int i = 0; i < stringCount; i++) {
			jstring string = (jstring)env->GetObjectArrayElement(arr, i);
			sarr.push_back(jstring_to_string(string, env));
			env->DeleteLocalRef(string);
		}

		return sarr;
	}

	if (name == "[Ljava.lang.CharSequence;") {
		jobjectArray arr = (jobjectArray)obj;
		int stringCount = env->GetArrayLength(arr);
		Vector<String> sarr;

		for (int i = 0; i < stringCount; i++) {
			jobject charsequence = env->GetObjectArrayElement(arr, i);
			sarr.push_back(charsequence_to_string(env, charsequence));
			env->DeleteLocalRef(charsequence);
		}

		return sarr;
	}

	if (name == "java.lang.Boolean") {
		jmethodID boolValue = env->GetMethodID(c, "booleanValue", "()Z");
		bool ret = env->CallBooleanMethod(obj, boolValue);
		return ret;
	}

	if (name == "java.lang.Integer" || name == "java.lang.Long") {
		jclass nclass = jni_find_class(env, "java/lang/Number");
		jmethodID longValue = env->GetMethodID(nclass, "longValue", "()J");
		jlong ret = env->CallLongMethod(obj, longValue);
		env->DeleteLocalRef(nclass);
		return ret;
	}

	if (name == "[I") {
		jintArray arr = (jintArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<int> sarr;
		sarr.resize(fCount);

		int *w = sarr.ptrw();
		env->GetIntArrayRegion(arr, 0, fCount, w);
		return sarr;
	}

	if (name == "[J") {
		jlongArray arr = (jlongArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<int64_t> sarr;
		sarr.resize(fCount);

		int64_t *w = sarr.ptrw();
		env->GetLongArrayRegion(arr, 0, fCount, w);
		return sarr;
	}

	if (name == "[B") {
		jbyteArray arr = (jbyteArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<uint8_t> sarr;
		sarr.resize(fCount);

		uint8_t *w = sarr.ptrw();
		env->GetByteArrayRegion(arr, 0, fCount, reinterpret_cast<signed char *>(w));
		return sarr;
	}

	if (name == "java.lang.Float" || name == "java.lang.Double") {
		jclass nclass = jni_find_class(env, "java/lang/Number");
		jmethodID doubleValue = env->GetMethodID(nclass, "doubleValue", "()D");
		double ret = env->CallDoubleMethod(obj, doubleValue);
		env->DeleteLocalRef(nclass);
		return ret;
	}

	if (name == "[D") {
		jdoubleArray arr = (jdoubleArray)obj;
		int fCount = env->GetArrayLength(arr);
		PackedFloat64Array packed_array;
		packed_array.resize(fCount);

		double *w = packed_array.ptrw();

		for (int i = 0; i < fCount; i++) {
			double n;
			env->GetDoubleArrayRegion(arr, i, 1, &n);
			w[i] = n;
		}
		return packed_array;
	}

	if (name == "[F") {
		jfloatArray arr = (jfloatArray)obj;
		int fCount = env->GetArrayLength(arr);
		PackedFloat32Array packed_array;
		packed_array.resize(fCount);

		float *w = packed_array.ptrw();

		for (int i = 0; i < fCount; i++) {
			float n;
			env->GetFloatArrayRegion(arr, i, 1, &n);
			w[i] = n;
		}
		return packed_array;
	}

	if (name == "[Ljava.lang.Object;") {
		jobjectArray arr = (jobjectArray)obj;
		int objCount = env->GetArrayLength(arr);
		Array varr;

		for (int i = 0; i < objCount; i++) {
			jobject jobj = env->GetObjectArrayElement(arr, i);
			Variant v = _jobject_to_variant(env, jobj, p_depth + 1);
			varr.push_back(v);
			env->DeleteLocalRef(jobj);
		}

		return varr;
	}

	if (name == "org.godotengine.godot.Dictionary") {
		Dictionary ret;
		jclass oclass = c;

		jmethodID entrySet = env->GetMethodID(oclass, "entrySet", "()Ljava/util/Set;");
		jobject entrySetObj = env->CallObjectMethod(obj, entrySet);

		jclass setClass = env->GetObjectClass(entrySetObj);
		jmethodID iterator = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
		jobject iteratorObj = env->CallObjectMethod(entrySetObj, iterator);

		jclass iteratorClass = env->GetObjectClass(iteratorObj);
		jmethodID hasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
		jmethodID next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

		jclass entryClass = jni_find_class(env, "java/util/Map$Entry");
		jmethodID getKey = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
		jmethodID getValue = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

		while (env->CallBooleanMethod(iteratorObj, hasNext)) {
			jobject entry = env->CallObjectMethod(iteratorObj, next);

			jobject jkey = env->CallObjectMethod(entry, getKey);
			jobject jvalue = env->CallObjectMethod(entry, getValue);

			Variant key = _jobject_to_variant(env, jkey, p_depth + 1);
			Variant value = _jobject_to_variant(env, jvalue, p_depth + 1);
			ret[key] = value;

			env->DeleteLocalRef(entry);
			env->DeleteLocalRef(jkey);
			env->DeleteLocalRef(jvalue);
		}

		env->DeleteLocalRef(entrySetObj);
		env->DeleteLocalRef(iteratorObj);
		env->DeleteLocalRef(setClass);
		env->DeleteLocalRef(iteratorClass);
		env->DeleteLocalRef(entryClass);

		return ret;
	}

	if (name == "org.godotengine.godot.variant.Callable") {
		return jcallable_to_callable(env, obj);
	}

	Ref<JavaObject> generic_object(memnew(JavaObject(JavaClassWrapper::wrap(name), obj)));

	env->DeleteLocalRef(c);

	return generic_object;
}

Variant::Type get_jni_type(const String &p_type) {
	static struct {
		const char *name;
		Variant::Type type;
	} _type_to_vtype[] = {
		{ "void", Variant::NIL },
		{ "boolean", Variant::BOOL },
		{ "int", Variant::INT },
		{ "long", Variant::INT },
		{ "float", Variant::FLOAT },
		{ "double", Variant::FLOAT },
		{ "java.lang.String", Variant::STRING },
		{ "java.lang.CharSequence", Variant::STRING },
		{ "[I", Variant::PACKED_INT32_ARRAY },
		{ "[J", Variant::PACKED_INT64_ARRAY },
		{ "[B", Variant::PACKED_BYTE_ARRAY },
		{ "[F", Variant::PACKED_FLOAT32_ARRAY },
		{ "[D", Variant::PACKED_FLOAT64_ARRAY },
		{ "[Ljava.lang.String;", Variant::PACKED_STRING_ARRAY },
		{ "[Ljava.lang.CharSequence;", Variant::PACKED_STRING_ARRAY },
		{ "org.godotengine.godot.Dictionary", Variant::DICTIONARY },
		{ "org.godotengine.godot.variant.Callable", Variant::CALLABLE },
		{ nullptr, Variant::NIL }
	};

	int idx = 0;

	while (_type_to_vtype[idx].name) {
		if (p_type == _type_to_vtype[idx].name) {
			return _type_to_vtype[idx].type;
		}

		idx++;
	}

	return Variant::OBJECT;
}

void setup_android_class_loader() {
	// Find a known class defined in the Godot package and obtain its ClassLoader.
	// This ClassLoader will be used by jni_find_class() to locate classes at runtime
	// in a thread-safe manner, avoiding issues with FindClass in non-main threads.

	if (android_class_loader) {
		cleanup_android_class_loader();
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	jclass known_class = env->FindClass("org/godotengine/godot/Godot");
	ERR_FAIL_NULL(known_class);

	jclass class_class = env->FindClass("java/lang/Class");
	ERR_FAIL_NULL(class_class);

	jmethodID get_class_loader_method = env->GetMethodID(class_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
	ERR_FAIL_NULL(get_class_loader_method);

	jobject class_loader = env->CallObjectMethod(known_class, get_class_loader_method);
	ERR_FAIL_NULL(class_loader);

	// NOTE: Make global ref so it can be used later.
	android_class_loader = env->NewGlobalRef(class_loader);
	ERR_FAIL_NULL(android_class_loader);

	jclass class_loader_class = env->FindClass("java/lang/ClassLoader");
	ERR_FAIL_NULL(class_loader_class);

	load_class_method = env->GetMethodID(class_loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	if (!load_class_method) {
		env->DeleteGlobalRef(android_class_loader);
		android_class_loader = nullptr;
		ERR_FAIL_MSG("Failed to find method ID for ClassLoader::loadClass.");
	}

	env->DeleteLocalRef(class_loader_class);
	env->DeleteLocalRef(class_loader);
	env->DeleteLocalRef(class_class);
	env->DeleteLocalRef(known_class);
}

void cleanup_android_class_loader() {
	if (android_class_loader != nullptr) {
		JNIEnv *env = get_jni_env();
		if (env) {
			env->DeleteGlobalRef(android_class_loader);
		} else {
			ERR_PRINT("Failed to release Android ClassLoader - JNIEnv is not available.");
		}
		android_class_loader = nullptr;
		load_class_method = nullptr;
	}
}

jclass jni_find_class(JNIEnv *p_env, const char *p_class_name) {
	ERR_FAIL_NULL_V(p_env, nullptr);
	ERR_FAIL_NULL_V(p_class_name, nullptr);

	jobject class_object = nullptr;
	if (!android_class_loader || !load_class_method) {
		ERR_PRINT("Android ClassLoader is not initialized. Falling back to FindClass.");
		class_object = p_env->FindClass(p_class_name);
	} else {
		jstring java_class_name = p_env->NewStringUTF(p_class_name);
		class_object = p_env->CallObjectMethod(
				android_class_loader,
				load_class_method,
				java_class_name);
		p_env->DeleteLocalRef(java_class_name);
	}
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V_MSG(class_object, nullptr, vformat("Failed to find Java class: \"%s\".", p_class_name));
	return static_cast<jclass>(class_object);
}


// --- Merged from platform/android/plugin/godot_plugin_jni.cpp ---
#include "servers/display/display_server.h"


#include "core/os/os.h"
#include "core/typedefs.h"

static HashMap<String, JNISingleton *> jni_singletons;

void unregister_plugins_singletons() {
	for (const KeyValue<String, JNISingleton *> &E : jni_singletons) {
		Engine::remove_singleton(E.key);

		memdelete(E.value);
	}
	jni_singletons.clear();
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSingleton(JNIEnv *env, jclass clazz, jstring name, jobject obj) {
	String singname = jstring_to_string(name, env);

	ERR_FAIL_COND_V(jni_singletons.has(singname), false);

	jclass java_class = env->GetObjectClass(obj);
	Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class, true);
	env->DeleteLocalRef(java_class);

	Ref<JavaObject> plugin_object = memnew(JavaObject(java_class_wrapped, obj));
	JNISingleton *plugin_singleton = memnew(JNISingleton(plugin_object));
	jni_singletons[singname] = plugin_singleton;

	Engine::add_singleton(Engine::Singleton(singname, plugin_singleton));
	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterMethod(JNIEnv *env, jclass clazz, jstring sname, jstring name, jstring ret, jobjectArray args) {
	String singname = jstring_to_string(sname, env);

	ERR_FAIL_COND(!jni_singletons.has(singname));

	JNISingleton *s = jni_singletons.get(singname);

	String mname = jstring_to_string(name, env);
	String retval = jstring_to_string(ret, env);
	Vector<Variant::Type> types;

	int stringCount = env->GetArrayLength(args);

	for (int i = 0; i < stringCount; i++) {
		jstring string = (jstring)env->GetObjectArrayElement(args, i);
		const String rawString = jstring_to_string(string, env);
		types.push_back(get_jni_type(rawString));
	}

	s->add_method(mname, types, get_jni_type(retval));
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_param_types) {
	String singleton_name = jstring_to_string(j_plugin_name, env);

	ERR_FAIL_COND(!jni_singletons.has(singleton_name));

	JNISingleton *singleton = jni_singletons.get(singleton_name);

	String signal_name = jstring_to_string(j_signal_name, env);
	Vector<Variant::Type> types;

	int stringCount = env->GetArrayLength(j_signal_param_types);

	for (int i = 0; i < stringCount; i++) {
		jstring j_signal_param_type = (jstring)env->GetObjectArrayElement(j_signal_param_types, i);
		const String signal_param_type = jstring_to_string(j_signal_param_type, env);
		types.push_back(get_jni_type(signal_param_type));
	}

	singleton->add_signal(signal_name, types);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeEmitSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_params) {
	String singleton_name = jstring_to_string(j_plugin_name, env);

	ERR_FAIL_COND(!jni_singletons.has(singleton_name));

	JNISingleton *singleton = jni_singletons.get(singleton_name);

	String signal_name = jstring_to_string(j_signal_name, env);

	int count = env->GetArrayLength(j_signal_params);

	Variant *variant_params = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **args = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject j_param = env->GetObjectArrayElement(j_signal_params, i);
		memnew_placement(&variant_params[i], Variant(_jobject_to_variant(env, j_param)));
		args[i] = &variant_params[i];
		env->DeleteLocalRef(j_param);
	}

	singleton->emit_signalp(String(signal_name), args, count);

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		variant_params[i].~Variant();
	}
}
}


// --- Merged from platform/android/variant/callable_jni.cpp ---
// --- Inlined from platform/android/variant/callable_jni.h ---
#include <jni.h>

extern "C" {
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCall(JNIEnv *p_env, jclass p_clazz, jlong p_native_callable, jobjectArray p_parameters);
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObject(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters);
JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObjectDeferred(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters);
JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_releaseNativePointer(JNIEnv *p_env, jclass clazz, jlong p_native_pointer);
}

// --- End platform/android/variant/callable_jni.h ---

#include "servers/display/display_server.h"

#include "core/typedefs.h"

static Callable _generate_callable(JNIEnv *p_env, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_object_id));
	ERR_FAIL_NULL_V(obj, Callable());

	String str_method = jstring_to_string(p_method_name, p_env);

	int count = p_env->GetArrayLength(p_parameters);

	Variant *args = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **argptrs = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject jobj = p_env->GetObjectArrayElement(p_parameters, i);
		ERR_FAIL_NULL_V(jobj, Callable());
		memnew_placement(&args[i], Variant(_jobject_to_variant(p_env, jobj)));
		argptrs[i] = &args[i];
		p_env->DeleteLocalRef(jobj);
	}

	Callable ret = Callable(obj, str_method).bindp(argptrs, count);

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		args[i].~Variant();
	}

	return ret;
}

extern "C" {
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCall(JNIEnv *p_env, jclass p_clazz, jlong p_native_callable, jobjectArray p_parameters) {
	const Variant *callable_variant = reinterpret_cast<const Variant *>(p_native_callable);
	ERR_FAIL_NULL_V(callable_variant, nullptr);
	if (callable_variant->get_type() != Variant::CALLABLE) {
		return nullptr;
	}

	int count = p_env->GetArrayLength(p_parameters);

	Variant *args = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **argptrs = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject jobj = p_env->GetObjectArrayElement(p_parameters, i);
		ERR_FAIL_NULL_V(jobj, nullptr);
		memnew_placement(&args[i], Variant(_jobject_to_variant(p_env, jobj)));
		argptrs[i] = &args[i];
		p_env->DeleteLocalRef(jobj);
	}

	Callable callable = *callable_variant;
	jobject ret = nullptr;
	if (callable.is_valid()) {
		Callable::CallError err;
		Variant result;
		callable.callp(argptrs, count, result, err);
		ret = _variant_to_jobject(p_env, result.get_type(), &result);
	}

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		args[i].~Variant();
	}

	return ret;
}

JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObject(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Callable callable = _generate_callable(p_env, p_object_id, p_method_name, p_parameters);
	if (callable.is_valid()) {
		Variant result = callable.call();
		return _variant_to_jobject(p_env, result.get_type(), &result);
	} else {
		return nullptr;
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObjectDeferred(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Callable callable = _generate_callable(p_env, p_object_id, p_method_name, p_parameters);
	if (callable.is_valid()) {
		callable.call_deferred();
	}
}

JNIEXPORT void JNICALL
Java_org_godotengine_godot_variant_Callable_releaseNativePointer(JNIEnv *p_env, jclass clazz, jlong p_native_pointer) {
	Variant *variant = reinterpret_cast<Variant *>(p_native_pointer);
	ERR_FAIL_NULL(variant);
	memdelete(variant);
}
}

// === Merged from platform/android/display_server_android.cpp ===
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

#include "servers/display/display_server.h"


#include "core/os/os.h"
#include "servers/display/display_server.h"

#if defined(RD_ENABLED)
#include "servers/display/display_server.h"

#if defined(VULKAN_ENABLED)
#include "servers/display/display_server.h"
#endif
#endif

#ifdef GLES3_ENABLED
#include "servers/display/display_server.h"

#include <EGL/egl.h>
#endif

#if defined(RD_ENABLED)
static RenderingContextDriver *rendering_context_global = nullptr;
static bool rendering_context_global_checked = false;
#endif

bool DisplayServerAndroid::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		} break;
#endif
		case DisplayServerEnums::FEATURE_PIP_MODE: {
			GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
			ERR_FAIL_NULL_V(godot_java, false);
			return godot_java->is_pip_mode_supported();
		} break;

		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		//case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		//case DisplayServerEnums::FEATURE_HIDPI:
		//case DisplayServerEnums::FEATURE_ICON:
		//case DisplayServerEnums::FEATURE_IME:
		case DisplayServerEnums::FEATURE_MOUSE:
		//case DisplayServerEnums::FEATURE_MOUSE_WARP:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		//case DisplayServerEnums::FEATURE_NATIVE_ICON:
		case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		case DisplayServerEnums::FEATURE_ORIENTATION:
		case DisplayServerEnums::FEATURE_TOUCHSCREEN:
		case DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
			return true;
		default:
			return false;
	}
}

String DisplayServerAndroid::get_name() const {
	return "Android";
}

bool DisplayServerAndroid::tts_is_speaking() const {
	return TTS_Android::is_speaking();
}

bool DisplayServerAndroid::tts_is_paused() const {
	return TTS_Android::is_paused();
}

TypedArray<Dictionary> DisplayServerAndroid::tts_get_voices() const {
	return TTS_Android::get_voices();
}

void DisplayServerAndroid::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	TTS_Android::speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServerAndroid::tts_pause() {
	TTS_Android::pause();
}

void DisplayServerAndroid::tts_resume() {
	TTS_Android::resume();
}

void DisplayServerAndroid::tts_stop() {
	TTS_Android::stop();
}

bool DisplayServerAndroid::is_dark_mode_supported() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	return godot_java->is_dark_mode_supported();
}

bool DisplayServerAndroid::is_dark_mode() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	return godot_java->is_dark_mode();
}

void DisplayServerAndroid::set_system_theme_change_callback(const Callable &p_callable) {
	system_theme_changed = p_callable;
}

void DisplayServerAndroid::emit_system_theme_changed() {
	if (system_theme_changed.is_valid()) {
		system_theme_changed.call_deferred();
	}
}

void DisplayServerAndroid::set_hardware_keyboard_connection_change_callback(const Callable &p_callable) {
	hardware_keyboard_connection_changed = p_callable;
}

void DisplayServerAndroid::emit_hardware_keyboard_connection_changed(bool p_connected) {
	if (hardware_keyboard_connection_changed.is_valid()) {
		hardware_keyboard_connection_changed.call_deferred(p_connected);
	}
}

void DisplayServerAndroid::clipboard_set(const String &p_text) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);

	if (godot_java->has_set_clipboard()) {
		godot_java->set_clipboard(p_text);
	} else {
		DisplayServerDriver::clipboard_set(p_text);
	}
}

String DisplayServerAndroid::clipboard_get() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, String());

	if (godot_java->has_get_clipboard()) {
		return godot_java->get_clipboard();
	} else {
		return DisplayServerDriver::clipboard_get();
	}
}

bool DisplayServerAndroid::clipboard_has() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	if (godot_java->has_has_clipboard()) {
		return godot_java->has_clipboard();
	} else {
		return DisplayServerDriver::clipboard_has();
	}
}

Error DisplayServerAndroid::dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	dialog_callback = p_callback;
	return godot_java->show_dialog(p_title, p_description, p_buttons);
}

void DisplayServerAndroid::emit_dialog_callback(int p_button_index) {
	if (dialog_callback.is_valid()) {
		dialog_callback.call_deferred(p_button_index);
	}
}

Error DisplayServerAndroid::dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	input_dialog_callback = p_callback;
	return godot_java->show_input_dialog(p_title, p_description, p_partial);
}

void DisplayServerAndroid::emit_input_dialog_callback(String p_text) {
	if (input_dialog_callback.is_valid()) {
		input_dialog_callback.call_deferred(p_text);
	}
}

Error DisplayServerAndroid::file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	ERR_FAIL_COND_V(p_window_id != DisplayServerEnums::MAIN_WINDOW_ID, ERR_UNAVAILABLE);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	file_picker_callback = p_callback;
	return godot_java->show_file_picker(p_current_directory, p_filename, p_mode, p_filters);
}

void DisplayServerAndroid::emit_file_picker_callback(bool p_ok, const Vector<String> &p_selected_paths) {
	if (file_picker_callback.is_valid()) {
		file_picker_callback.call_deferred(p_ok, p_selected_paths, 0);
	}
}

Color DisplayServerAndroid::get_accent_color() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, Color(0, 0, 0, 0));
	return godot_java->get_accent_color();
}

Color DisplayServerAndroid::get_base_color() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, Color(0, 0, 0, 0));
	return godot_java->get_base_color();
}

TypedArray<Rect2> DisplayServerAndroid::get_display_cutouts(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, TypedArray<Rect2>());

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, Array());
	return godot_io_java->get_display_cutouts();
}

Rect2i DisplayServerAndroid::get_display_safe_area(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, Rect2i());
	return godot_io_java->get_display_safe_area();
}

void DisplayServerAndroid::screen_set_keep_on(bool p_enable) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);

	godot_java->set_keep_screen_on(p_enable);
	keep_screen_on = p_enable;
}

bool DisplayServerAndroid::screen_is_kept_on() const {
	return keep_screen_on;
}

void DisplayServerAndroid::screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen) {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX(p_screen, screen_count);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	godot_io_java->set_screen_orientation(p_orientation);
}

DisplayServerEnums::ScreenOrientation DisplayServerAndroid::screen_get_orientation(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, DisplayServerEnums::SCREEN_LANDSCAPE);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, DisplayServerEnums::SCREEN_LANDSCAPE);

	const int orientation = godot_io_java->get_screen_orientation();
	ERR_FAIL_INDEX_V_MSG(orientation, 7, DisplayServerEnums::SCREEN_LANDSCAPE, "Unrecognized screen orientation");
	return (DisplayServerEnums::ScreenOrientation)orientation;
}

int DisplayServerAndroid::get_display_rotation() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	return godot_io_java->get_display_rotation();
}

int DisplayServerAndroid::get_screen_count() const {
	return 1;
}

int DisplayServerAndroid::get_primary_screen() const {
	return 0;
}

Point2i DisplayServerAndroid::screen_get_position(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return Point2i(0, 0);
}

Size2i DisplayServerAndroid::screen_get_size(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	return OS_Android::get_display_size();
}

Rect2i DisplayServerAndroid::screen_get_usable_rect(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	Size2i display_size = OS_Android::get_display_size();
	return Rect2i(0, 0, display_size.width, display_size.height);
}

int DisplayServerAndroid::screen_get_dpi(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 160);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 160);

	return godot_io_java->get_screen_dpi();
}

float DisplayServerAndroid::screen_get_scale(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 1.0f);

	float screen_scale = godot_io_java->get_scaled_density();

	// Update the scale to avoid cropping.
	Size2i screen_size = screen_get_size(p_screen);
	if (screen_size != Size2i()) {
		float width_scale = screen_size.width / (float)OS_Android::DEFAULT_WINDOW_WIDTH;
		float height_scale = screen_size.height / (float)OS_Android::DEFAULT_WINDOW_HEIGHT;
		screen_scale = MIN(screen_scale, MIN(width_scale, height_scale));
	}

	return screen_scale;
}

float DisplayServerAndroid::screen_get_refresh_rate(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	if (!godot_io_java) {
		ERR_PRINT("An error occurred while trying to get the screen refresh rate.");
		return SCREEN_REFRESH_RATE_FALLBACK;
	}

	return godot_io_java->get_screen_refresh_rate(SCREEN_REFRESH_RATE_FALLBACK);
}

bool DisplayServerAndroid::is_touchscreen_available() const {
	return true;
}

void DisplayServerAndroid::virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	if (godot_io_java->has_vk()) {
		godot_io_java->show_vk(p_existing_text, (int)p_type, p_max_length, p_cursor_start, p_cursor_end);
	} else {
		ERR_PRINT("Virtual keyboard not available");
	}
}

void DisplayServerAndroid::virtual_keyboard_hide() {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	if (godot_io_java->has_vk()) {
		godot_io_java->hide_vk();
	} else {
		ERR_PRINT("Virtual keyboard not available");
	}
}

int DisplayServerAndroid::virtual_keyboard_get_height() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	return godot_io_java->get_vk_height();
}

bool DisplayServerAndroid::has_hardware_keyboard() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, false);

	return godot_io_java->has_hardware_keyboard();
}

void DisplayServerAndroid::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	window_event_callback = p_callable;
}

void DisplayServerAndroid::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	input_event_callback = p_callable;
}

void DisplayServerAndroid::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	input_text_callback = p_callable;
}

void DisplayServerAndroid::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	rect_changed_callback = p_callable;
}

void DisplayServerAndroid::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

template <typename... Args>
void DisplayServerAndroid::_window_callback(const Callable &p_callable, bool p_deferred, const Args &...p_rest_args) const {
	if (p_callable.is_valid()) {
		if (p_deferred) {
			p_callable.call_deferred(p_rest_args...);
		} else {
			p_callable.call(p_rest_args...);
		}
	}
}

void DisplayServerAndroid::send_window_event(DisplayServerEnums::WindowEvent p_event, bool p_deferred) const {
	_window_callback(window_event_callback, p_deferred, int(p_event));
}

void DisplayServerAndroid::send_input_event(const Ref<InputEvent> &p_event) const {
	_window_callback(input_event_callback, false, p_event);
}

void DisplayServerAndroid::send_input_text(const String &p_text) const {
	_window_callback(input_text_callback, false, p_text, false);
}

void DisplayServerAndroid::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	if (DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>()) {
		ds->send_input_event(p_event);
	}
}

Vector<DisplayServerEnums::WindowID> DisplayServerAndroid::get_window_list() const {
	Vector<DisplayServerEnums::WindowID> ret;
	ret.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
	return ret;
}

DisplayServerEnums::WindowID DisplayServerAndroid::get_window_at_screen_position(const Point2i &p_position) const {
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

int64_t DisplayServerAndroid::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, 0);
	switch (p_handle_type) {
		case DisplayServerEnums::WINDOW_HANDLE: {
			return reinterpret_cast<int64_t>(OS_Android::get_godot_java()->get_activity());
		}
		case DisplayServerEnums::WINDOW_VIEW: {
			return 0; // Not supported.
		}
#ifdef GLES3_ENABLED
		case DisplayServerEnums::DISPLAY_HANDLE: {
			if (rendering_driver == "opengl3") {
				return reinterpret_cast<int64_t>(eglGetCurrentDisplay());
			}
			return 0;
		}
		case DisplayServerEnums::OPENGL_CONTEXT: {
			if (rendering_driver == "opengl3") {
				return reinterpret_cast<int64_t>(eglGetCurrentContext());
			}
			return 0;
		}
		case DisplayServerEnums::EGL_DISPLAY: {
			// @todo Find a way to get this from the Java side.
			return 0;
		}
		case DisplayServerEnums::EGL_CONFIG: {
			// @todo Find a way to get this from the Java side.
			return 0;
		}
#endif
		default: {
			return 0;
		}
	}
}

void DisplayServerAndroid::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	window_attached_instance_id = p_instance;
}

ObjectID DisplayServerAndroid::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, ObjectID());
	return window_attached_instance_id;
}

void DisplayServerAndroid::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

int DisplayServerAndroid::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, DisplayServerEnums::INVALID_SCREEN);
	return 0;
}

void DisplayServerAndroid::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Point2i DisplayServerAndroid::window_get_position(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

Point2i DisplayServerAndroid::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

void DisplayServerAndroid::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAndroid::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAndroid::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_size(DisplayServerEnums::WindowID p_window) const {
	return OS_Android::get_display_size();
}

Size2i DisplayServerAndroid::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return OS_Android::get_display_size();
}

void DisplayServerAndroid::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	OS_Android::get_godot_java()->enable_immersive_mode(p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN);
}

DisplayServerEnums::WindowMode DisplayServerAndroid::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	if (OS_Android::get_godot_java()->is_in_immersive_mode()) {
		return DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
	} else {
		return DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
	}
}

bool DisplayServerAndroid::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerAndroid::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

bool DisplayServerAndroid::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, false);
	switch (p_flag) {
		case DisplayServerEnums::WindowFlags::WINDOW_FLAG_TRANSPARENT:
			return is_window_transparency_available();

		default:
			return false;
	}
}

void DisplayServerAndroid::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

bool DisplayServerAndroid::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerAndroid::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerAndroid::can_any_window_draw() const {
	return true;
}

void DisplayServerAndroid::window_set_color(const Color &p_color) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_window_color(p_color);
}

void DisplayServerAndroid::process_events() {
	Input::flush_buffered_events();
}

Vector<String> DisplayServerAndroid::get_rendering_drivers_func() {
	Vector<String> drivers;

#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
#endif
#ifdef VULKAN_ENABLED
	drivers.push_back("vulkan");
#endif

	return drivers;
}

DisplayServerDriver *DisplayServerAndroid::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	DisplayServerDriver *ds = memnew(DisplayServerAndroid(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
	if (r_error != OK) {
		if (p_rendering_driver == "vulkan") {
			OS::alert(
					"Your device seems not to support the required Vulkan version.\n\n"
					"Please try exporting your game using the 'gl_compatibility' renderer.",
					"Unable to initialize Vulkan video driver");
		} else {
			OS::alert(
					"Your device seems not to support the required OpenGL ES 3.0 version.",
					"Unable to initialize OpenGL video driver");
		}
	}
	return ds;
}

void DisplayServerAndroid::register_android_driver() {
	DisplayServer::register_create_function("android", create_func, get_rendering_drivers_func);
}

#ifdef VULKAN_ENABLED
bool DisplayServerAndroid::check_vulkan_global_context(bool p_vulkan_requirements_met) {
	if (!rendering_context_global_checked) {
		bool fallback_to_opengl3 = GLOBAL_GET("rendering/rendering_device/fallback_to_opengl3");
		Error err = ERR_CANT_CREATE;
		if (p_vulkan_requirements_met) {
			rendering_context_global = memnew(RenderingContextDriverVulkanAndroid);
			err = rendering_context_global->initialize();
		}

		if (err != OK) {
			if (rendering_context_global != nullptr) {
				memdelete(rendering_context_global);
				rendering_context_global = nullptr;
			}

#if defined(GLES3_ENABLED)
			if (fallback_to_opengl3) {
				WARN_PRINT("Your device does not seem to support Vulkan, switching to OpenGL 3.");
				OS::set_current_rendering_driver_name("opengl3", OS::RENDERING_SOURCE_FALLBACK);
				OS::set_current_rendering_method("gl_compatibility", OS::RENDERING_SOURCE_FALLBACK);
			} else
#endif
			{
				ERR_PRINT("Failed to initialize Vulkan context.");
			}
		}

		rendering_context_global_checked = true;
	}

	return rendering_context_global != nullptr;
}

void DisplayServerAndroid::free_vulkan_global_context() {
	if (rendering_context_global != nullptr) {
		memdelete(rendering_context_global);
		rendering_context_global = nullptr;
		rendering_context_global_checked = false;
	}
}
#endif

void DisplayServerAndroid::reset_window() {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		if (rendering_device) {
			rendering_device->screen_free(DisplayServerEnums::MAIN_WINDOW_ID);
		}

		DisplayServerEnums::VSyncMode last_vsync_mode = rendering_context_global->window_get_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID);
		rendering_context_global->window_destroy(DisplayServerEnums::MAIN_WINDOW_ID);

		union {
#ifdef VULKAN_ENABLED
			RenderingContextDriverVulkanAndroid::WindowPlatformData vulkan;
#endif
		} wpd;
#ifdef VULKAN_ENABLED
		if (rendering_driver == "vulkan") {
			ANativeWindow *native_window = OS_Android::get_native_window();
			ERR_FAIL_NULL(native_window);
			wpd.vulkan.window = native_window;
		}
#endif

		if (rendering_context_global->window_create(DisplayServerEnums::MAIN_WINDOW_ID, &wpd) != OK) {
			ERR_PRINT(vformat("Failed to reset %s window.", rendering_driver));
			return;
		}

		Size2i display_size = OS_Android::get_display_size();
		rendering_context_global->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, display_size.width, display_size.height);
		rendering_context_global->window_set_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID, last_vsync_mode);

		if (rendering_device) {
			rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);
		}
	}
#endif
}

void DisplayServerAndroid::notify_surface_changed(int p_width, int p_height) {
	if (rect_changed_callback.is_valid()) {
		rect_changed_callback.call(Rect2i(0, 0, p_width, p_height));
	}
}

void DisplayServerAndroid::notify_application_paused() {
#if defined(RD_ENABLED)
	if (rendering_device) {
		rendering_device->update_pipeline_cache();
	}
#endif // defined(RD_ENABLED)
}

DisplayServerAndroid::DisplayServerAndroid(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	rendering_driver = p_rendering_driver;

	keep_screen_on = GLOBAL_GET("display/window/energy_saving/keep_screen_on");

	NativeMenu::initialize();

#ifdef VULKAN_ENABLED
	if (rendering_driver == "vulkan") {
		check_vulkan_global_context(true);
		if (rendering_context_global == nullptr) {
			ERR_PRINT("Can't initialize display server with Vulkan driver because no Vulkan context is available.");
			r_error = ERR_UNAVAILABLE;
			return;
		}

		ANativeWindow *native_window = OS_Android::get_native_window();
		ERR_FAIL_NULL(native_window);

		RenderingContextDriverVulkanAndroid::WindowPlatformData wpd;
		wpd.window = native_window;

		if (rendering_context_global->window_create(DisplayServerEnums::MAIN_WINDOW_ID, &wpd) != OK) {
			ERR_PRINT(vformat("Failed to create %s window.", rendering_driver));
			r_error = ERR_UNAVAILABLE;
			return;
		}

		Size2i display_size = OS_Android::get_display_size();
		rendering_context_global->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, display_size.width, display_size.height);
		rendering_context_global->window_set_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID, p_vsync_mode);

		rendering_device = memnew(RenderingDevice);
		if (rendering_device->initialize(rendering_context_global, DisplayServerEnums::MAIN_WINDOW_ID) != OK) {
			rendering_device = nullptr;
			r_error = ERR_UNAVAILABLE;
			return;
		}

		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
	}
#endif

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3") {
		RasterizerGLES3::make_current(false);
	}
#endif

	Input::set_event_dispatch_function(_dispatch_input_events);

	r_error = OK;
}

DisplayServerAndroid::~DisplayServerAndroid() {
	NativeMenu::finalize();

#if defined(RD_ENABLED)
	memdelete(rendering_device);

	free_vulkan_global_context();
#endif
}

void DisplayServerAndroid::process_accelerometer(const Vector3 &p_accelerometer) {
	Input::set_accelerometer(p_accelerometer);
}

void DisplayServerAndroid::process_gravity(const Vector3 &p_gravity) {
	Input::set_gravity(p_gravity);
}

void DisplayServerAndroid::process_magnetometer(const Vector3 &p_magnetometer) {
	Input::set_magnetometer(p_magnetometer);
}

void DisplayServerAndroid::process_gyroscope(const Vector3 &p_gyroscope) {
	Input::set_gyroscope(p_gyroscope);
}

void DisplayServerAndroid::_mouse_update_mode() {
	DisplayServerEnums::MouseMode wanted_mouse_mode = mouse_mode_override_enabled
			? mouse_mode_override
			: mouse_mode_base;

	if (!OS_Android::get_godot_java()->get_godot_view()->can_update_pointer_icon() || !OS_Android::get_godot_java()->get_godot_view()->can_capture_pointer()) {
		return;
	}
	if (mouse_mode == wanted_mouse_mode) {
		return;
	}

	if (wanted_mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_HIDDEN) {
		OS_Android::get_godot_java()->get_godot_view()->set_pointer_icon(CURSOR_TYPE_NULL);
	} else {
		cursor_set_shape(cursor_shape);
	}

	if (wanted_mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_CAPTURED) {
		OS_Android::get_godot_java()->get_godot_view()->request_pointer_capture();
	} else {
		OS_Android::get_godot_java()->get_godot_view()->release_pointer_capture();
	}

	mouse_mode = wanted_mouse_mode;
}

void DisplayServerAndroid::mouse_set_mode(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_base) {
		return;
	}
	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerAndroid::mouse_get_mode() const {
	return mouse_mode;
}

void DisplayServerAndroid::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_override) {
		return;
	}
	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerAndroid::mouse_get_mode_override() const {
	return mouse_mode_override;
}

void DisplayServerAndroid::mouse_set_mode_override_enabled(bool p_override_enabled) {
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerAndroid::mouse_is_mode_override_enabled() const {
	return mouse_mode_override_enabled;
}

Point2i DisplayServerAndroid::mouse_get_position() const {
	return Input::get_mouse_position();
}

uint32_t DisplayServerAndroid::mouse_get_button_state() const {
	return Input::get_mouse_button_mask();
}

void DisplayServerAndroid::_cursor_set_shape_helper(DisplayServerEnums::CursorShape p_shape, bool force) {
	if (!OS_Android::get_godot_java()->get_godot_view()->can_update_pointer_icon()) {
		return;
	}
	if (cursor_shape == p_shape && !force) {
		return;
	}

	cursor_shape = p_shape;

	if (mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_VISIBLE || mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_CONFINED) {
		OS_Android::get_godot_java()->get_godot_view()->set_pointer_icon(android_cursors[cursor_shape]);
	}
}

void DisplayServerAndroid::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	_cursor_set_shape_helper(p_shape);
}

DisplayServerEnums::CursorShape DisplayServerAndroid::cursor_get_shape() const {
	return cursor_shape;
}

void DisplayServerAndroid::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	String cursor_path = p_cursor.is_valid() ? p_cursor->get_path() : "";
	if (!cursor_path.is_empty()) {
		cursor_path = ProjectSettings::globalize_path(cursor_path);
	}
	OS_Android::get_godot_java()->get_godot_view()->configure_pointer_icon(android_cursors[cursor_shape], cursor_path, p_hotspot);
	_cursor_set_shape_helper(p_shape, true);
}

void DisplayServerAndroid::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		rendering_context_global->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerAndroid::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		return rendering_context_global->window_get_vsync_mode(p_window);
	}
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

void DisplayServerAndroid::reset_swap_buffers_flag() {
	swap_buffers_flag = false;
}

bool DisplayServerAndroid::should_swap_buffers() const {
	return swap_buffers_flag;
}

void DisplayServerAndroid::swap_buffers() {
	swap_buffers_flag = true;
}

void DisplayServerAndroid::set_native_icon(const String &p_filename) {
	// NOT SUPPORTED
}

void DisplayServerAndroid::set_icon(const Ref<Image> &p_icon) {
	// NOT SUPPORTED
}

bool DisplayServerAndroid::is_window_transparency_available() const {
	return GLOBAL_GET_CACHED(bool, "display/window/per_pixel_transparency/allowed");
}

bool DisplayServerAndroid::is_in_pip_mode(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, false);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);
	return godot_java->is_in_pip_mode();
}

void DisplayServerAndroid::pip_mode_enter(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->enter_pip_mode();
}

void DisplayServerAndroid::pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_pip_mode_aspect_ratio(p_numerator, p_denominator);
}

void DisplayServerAndroid::pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_auto_enter_pip_mode_on_background(p_auto_enter_on_background);
}


// --- Merged from platform/android/android_input_handler.cpp ---
#include "servers/display/display_server.h"


#include "core/os/os.h"

void AndroidInputHandler::process_joy_event(AndroidInputHandler::JoypadEvent p_event) {
	switch (p_event.type) {
		case JOY_EVENT_BUTTON:
			Input::joy_button(p_event.device, (JoyButton)p_event.index, p_event.pressed);
			break;
		case JOY_EVENT_AXIS:
			Input::joy_axis(p_event.device, (JoyAxis)p_event.index, p_event.value);
			break;
		case JOY_EVENT_HAT:
			Input::joy_hat(p_event.device, p_event.hat);
			break;
		default:
			return;
	}
}

void AndroidInputHandler::_set_key_modifier_state(Ref<InputEventWithModifiers> ev, Key p_keycode) {
	if (p_keycode != Key::SHIFT) {
		ev->set_shift_pressed(shift_mem);
	}
	if (p_keycode != Key::ALT) {
		ev->set_alt_pressed(alt_mem);
	}
	if (p_keycode != Key::META) {
		ev->set_meta_pressed(meta_mem);
	}
	if (p_keycode != Key::CTRL) {
		ev->set_ctrl_pressed(control_mem);
	}
}

void AndroidInputHandler::process_key_event(int p_physical_keycode, int p_unicode, int p_key_label, bool p_pressed, bool p_echo) {
	static char32_t prev_wc = 0;
	char32_t unicode = p_unicode;
	if ((p_unicode & 0xfffffc00) == 0xd800) {
		if (prev_wc != 0) {
			ERR_PRINT("invalid utf16 surrogate input");
		}
		prev_wc = unicode;
		return; // Skip surrogate.
	} else if ((unicode & 0xfffffc00) == 0xdc00) {
		if (prev_wc == 0) {
			ERR_PRINT("invalid utf16 surrogate input");
			return; // Skip invalid surrogate.
		}
		unicode = (prev_wc << 10UL) + unicode - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
		prev_wc = 0;
	} else {
		prev_wc = 0;
	}

	Ref<InputEventKey> ev;
	ev.instantiate();

	Key physical_keycode = godot_code_from_android_code(p_physical_keycode);
	Key keycode;
	if (unicode == '\b') { // 0x08
		keycode = Key::BACKSPACE;
	} else if (unicode == '\t') { // 0x09
		keycode = Key::TAB;
	} else if (unicode == '\n') { // 0x0A
		keycode = Key::ENTER;
	} else if (unicode == 0x1B) {
		keycode = Key::ESCAPE;
	} else if (unicode == 0x1F) {
		keycode = Key::KEY_DELETE;
	} else {
		keycode = fix_keycode(unicode, physical_keycode);
	}

	switch (physical_keycode) {
		case Key::SHIFT: {
			shift_mem = p_pressed;
		} break;
		case Key::ALT: {
			alt_mem = p_pressed;
		} break;
		case Key::CTRL: {
			control_mem = p_pressed;
		} break;
		case Key::META: {
			meta_mem = p_pressed;
		} break;
		default:
			break;
	}

	ev->set_keycode(keycode);
	ev->set_physical_keycode(physical_keycode);
	ev->set_key_label(fix_key_label(p_key_label, keycode));
	ev->set_unicode(fix_unicode(unicode));
	ev->set_location(godot_location_from_android_code(p_physical_keycode));
	ev->set_pressed(p_pressed);
	ev->set_echo(p_echo);

	_set_key_modifier_state(ev, keycode);

	if (p_physical_keycode == AKEYCODE_BACK && p_pressed) {
		if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
			dsa->send_window_event(DisplayServerEnums::WINDOW_EVENT_GO_BACK_REQUEST, true);
		}
	}

	Input::parse_input_event(ev);
}

void AndroidInputHandler::_cancel_all_touch() {
	_parse_all_touch(false, true);
	touch.clear();
}

void AndroidInputHandler::_parse_all_touch(bool p_pressed, bool p_canceled) {
	if (touch.size()) {
		//end all if exist
		for (int i = 0; i < touch.size(); i++) {
			Ref<InputEventScreenTouch> ev;
			ev.instantiate();
			ev->set_index(touch[i].id);
			ev->set_pressed(p_pressed);
			ev->set_canceled(p_canceled);
			ev->set_position(touch[i].pos);
			ev->set_double_tap(touch[i].double_tap);
			Input::parse_input_event(ev);
		}
	}
}

void AndroidInputHandler::_release_all_touch() {
	_parse_all_touch(false, false);
	touch.clear();
}

void AndroidInputHandler::process_touch_event(int p_event, int p_pointer, const Vector<TouchPos> &p_points) {
	switch (p_event) {
		case AMOTION_EVENT_ACTION_DOWN: { //gesture begin
			// Release any remaining touches or mouse event
			_release_mouse_event_info();
			_release_all_touch();

			touch.resize(p_points.size());
			for (int i = 0; i < p_points.size(); i++) {
				touch.write[i].id = p_points[i].id;
				touch.write[i].pos = p_points[i].pos;
				touch.write[i].pressure = p_points[i].pressure;
				touch.write[i].tilt = p_points[i].tilt;
				touch.write[i].double_tap = p_points[i].double_tap;
			}

			//send touch
			_parse_all_touch(true, false);

		} break;
		case AMOTION_EVENT_ACTION_MOVE: { //motion
			if (touch.size() != p_points.size()) {
				return;
			}

			for (int i = 0; i < touch.size(); i++) {
				int idx = -1;
				for (int j = 0; j < p_points.size(); j++) {
					if (touch[i].id == p_points[j].id) {
						idx = j;
						break;
					}
				}

				ERR_CONTINUE(idx == -1);

				if (touch[i].pos == p_points[idx].pos) {
					continue; // Don't move unnecessarily.
				}

				Ref<InputEventScreenDrag> ev;
				ev.instantiate();
				ev->set_index(touch[i].id);
				ev->set_position(p_points[idx].pos);
				ev->set_relative(p_points[idx].pos - touch[i].pos);
				ev->set_relative_screen_position(ev->get_relative());
				ev->set_pressure(p_points[idx].pressure);
				ev->set_tilt(p_points[idx].tilt);
				Input::parse_input_event(ev);
				touch.write[i].pos = p_points[idx].pos;
			}

		} break;
		case AMOTION_EVENT_ACTION_CANCEL: {
			_cancel_all_touch();
		} break;
		case AMOTION_EVENT_ACTION_UP: { //release
			_release_all_touch();
		} break;
		case AMOTION_EVENT_ACTION_POINTER_DOWN: { // add touch
			for (int i = 0; i < p_points.size(); i++) {
				if (p_points[i].id == p_pointer) {
					TouchPos tp = p_points[i];
					touch.push_back(tp);

					Ref<InputEventScreenTouch> ev;
					ev.instantiate();

					ev->set_index(tp.id);
					ev->set_pressed(true);
					ev->set_position(tp.pos);
					Input::parse_input_event(ev);

					break;
				}
			}
		} break;
		case AMOTION_EVENT_ACTION_POINTER_UP: { // remove touch
			for (int i = 0; i < touch.size(); i++) {
				if (touch[i].id == p_pointer) {
					Ref<InputEventScreenTouch> ev;
					ev.instantiate();
					ev->set_index(touch[i].id);
					ev->set_pressed(false);
					ev->set_position(touch[i].pos);
					Input::parse_input_event(ev);
					touch.remove_at(i);

					break;
				}
			}
		} break;
	}
}

void AndroidInputHandler::_cancel_mouse_event_info(bool p_source_mouse_relative) {
	buttons_state = uint32_t();
	_parse_mouse_event_info(uint32_t(), false, true, false, p_source_mouse_relative);
	mouse_event_info.valid = false;
}

void AndroidInputHandler::_parse_mouse_event_info(uint32_t event_buttons_mask, bool p_pressed, bool p_canceled, bool p_double_click, bool p_source_mouse_relative) {
	if (!mouse_event_info.valid) {
		return;
	}

	Ref<InputEventMouseButton> ev;
	ev.instantiate();
	_set_key_modifier_state(ev, Key::NONE);
	if (p_source_mouse_relative) {
		ev->set_position(hover_prev_pos);
		ev->set_global_position(hover_prev_pos);
	} else {
		ev->set_position(mouse_event_info.pos);
		ev->set_global_position(mouse_event_info.pos);
		hover_prev_pos = mouse_event_info.pos;
	}
	ev->set_pressed(p_pressed);
	ev->set_canceled(p_canceled);
	uint32_t changed_button_mask = buttons_state.get_different(event_buttons_mask);

	buttons_state = event_buttons_mask;

	ev->set_button_index(_button_index_from_mask(changed_button_mask));
	ev->set_button_mask(event_buttons_mask);
	ev->set_double_click(p_double_click);
	Input::parse_input_event(ev);
}

void AndroidInputHandler::_release_mouse_event_info(bool p_source_mouse_relative) {
	_parse_mouse_event_info(uint32_t(), false, false, false, p_source_mouse_relative);
	mouse_event_info.valid = false;
}

void AndroidInputHandler::process_mouse_event(int p_event_action, int p_event_android_buttons_mask, Point2 p_event_pos, Vector2 p_delta, bool p_double_click, bool p_source_mouse_relative, float p_pressure, Vector2 p_tilt) {
	uint32_t event_buttons_mask = _android_button_mask_to_godot_button_mask(p_event_android_buttons_mask);
	switch (p_event_action) {
		case AMOTION_EVENT_ACTION_HOVER_MOVE: // hover move
		case AMOTION_EVENT_ACTION_HOVER_ENTER: // hover enter
		case AMOTION_EVENT_ACTION_HOVER_EXIT: { // hover exit
			// https://developer.android.com/reference/android/view/MotionEvent.html#ACTION_HOVER_ENTER
			Ref<InputEventMouseMotion> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			ev->set_position(p_event_pos);
			ev->set_global_position(p_event_pos);
			ev->set_relative(p_event_pos - hover_prev_pos);
			ev->set_relative_screen_position(ev->get_relative());
			Input::parse_input_event(ev);
			hover_prev_pos = p_event_pos;
		} break;

		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_BUTTON_PRESS: {
			// Release any remaining touches or mouse event
			_release_mouse_event_info();
			_release_all_touch();

			mouse_event_info.valid = true;
			mouse_event_info.pos = p_event_pos;
			_parse_mouse_event_info(event_buttons_mask, true, false, p_double_click, p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_CANCEL: {
			_cancel_mouse_event_info(p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_BUTTON_RELEASE: {
			_release_mouse_event_info(p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_MOVE: {
			if (!p_source_mouse_relative && !mouse_event_info.valid) {
				return;
			}

			Ref<InputEventMouseMotion> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			if (p_source_mouse_relative) {
				ev->set_position(hover_prev_pos);
				ev->set_global_position(hover_prev_pos);
				ev->set_relative(p_event_pos);
				ev->set_relative_screen_position(p_event_pos);
			} else {
				ev->set_position(p_event_pos);
				ev->set_global_position(p_event_pos);
				ev->set_relative(p_event_pos - hover_prev_pos);
				ev->set_relative_screen_position(ev->get_relative());
				mouse_event_info.pos = p_event_pos;
				hover_prev_pos = p_event_pos;
			}
			ev->set_button_mask(event_buttons_mask);
			ev->set_pressure(p_pressure);
			ev->set_tilt(p_tilt);
			Input::parse_input_event(ev);
		} break;

		case AMOTION_EVENT_ACTION_SCROLL: {
			Ref<InputEventMouseButton> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			if (p_source_mouse_relative) {
				ev->set_position(hover_prev_pos);
				ev->set_global_position(hover_prev_pos);
			} else {
				ev->set_position(p_event_pos);
				ev->set_global_position(p_event_pos);
			}
			ev->set_pressed(true);
			buttons_state = event_buttons_mask;
			if (p_delta.y > 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_UP, p_delta.y);
			} else if (p_delta.y < 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_DOWN, -p_delta.y);
			}

			if (p_delta.x > 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_RIGHT, p_delta.x);
			} else if (p_delta.x < 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_LEFT, -p_delta.x);
			}
		} break;
	}
}

void AndroidInputHandler::_wheel_button_click(uint32_t &ev, MouseButton wheel_button, float factor) {
	Ref<InputEventMouseButton> evd = ev->duplicate();
	_set_key_modifier_state(evd, Key::NONE);
	evd->set_button_index(wheel_button);
	evd->set_button_mask(event_buttons_mask.get_different(mouse_button_to_mask(wheel_button)));
	evd->set_factor(factor);
	Input::parse_input_event(evd);
	Ref<InputEventMouseButton> evdd = evd->duplicate();
	evdd->set_pressed(false);
	evdd->set_button_mask(event_buttons_mask);
	Input::parse_input_event(evdd);
}

void AndroidInputHandler::process_magnify(Point2 p_pos, float p_factor) {
	Ref<InputEventMagnifyGesture> magnify_event;
	magnify_event.instantiate();
	_set_key_modifier_state(magnify_event, Key::NONE);
	magnify_event->set_position(p_pos);
	magnify_event->set_factor(p_factor);
	Input::parse_input_event(magnify_event);
}

void AndroidInputHandler::process_pan(Point2 p_pos, Vector2 p_delta) {
	Ref<InputEventPanGesture> pan_event;
	pan_event.instantiate();
	_set_key_modifier_state(pan_event, Key::NONE);
	pan_event->set_position(p_pos);
	pan_event->set_delta(p_delta);
	Input::parse_input_event(pan_event);
}

MouseButton AndroidInputHandler::_button_index_from_mask(uint32_t button_mask) {
	switch (button_mask) {
		case MouseButtonMask::LEFT:
			return MouseButton::LEFT;
		case MouseButtonMask::RIGHT:
			return MouseButton::RIGHT;
		case MouseButtonMask::MIDDLE:
			return MouseButton::MIDDLE;
		case MouseButtonMask::MB_XBUTTON1:
			return MouseButton::MB_XBUTTON1;
		case MouseButtonMask::MB_XBUTTON2:
			return MouseButton::MB_XBUTTON2;
		default:
			return MouseButton::NONE;
	}
}

uint32_t AndroidInputHandler::_android_button_mask_to_godot_button_mask(int android_button_mask) {
	uint32_t godot_button_mask = MouseButtonMask::NONE;
	if (android_button_mask & AMOTION_EVENT_BUTTON_PRIMARY) {
		godot_button_mask.set_flag(MouseButtonMask::LEFT);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_SECONDARY) {
		godot_button_mask.set_flag(MouseButtonMask::RIGHT);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_TERTIARY) {
		godot_button_mask.set_flag(MouseButtonMask::MIDDLE);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_BACK) {
		godot_button_mask.set_flag(MouseButtonMask::MB_XBUTTON1);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_FORWARD) {
		godot_button_mask.set_flag(MouseButtonMask::MB_XBUTTON2);
	}

	return godot_button_mask;
}


// --- Merged from platform/android/android_keys_utils.cpp ---
#include "servers/display/display_server.h"

Key godot_code_from_android_code(unsigned int p_code) {
	for (int i = 0; android_godot_code_pairs[i].android_code != AKEYCODE_MAX; i++) {
		if (android_godot_code_pairs[i].android_code == p_code) {
			return android_godot_code_pairs[i].godot_code;
		}
	}
	return Key::UNKNOWN;
}

KeyLocation godot_location_from_android_code(unsigned int p_code) {
	for (int i = 0; android_godot_location_pairs[i].android_code != AKEYCODE_MAX; i++) {
		if (android_godot_location_pairs[i].android_code == p_code) {
			return android_godot_location_pairs[i].godot_code;
		}
	}
	return KeyLocation::UNSPECIFIED;
}


// --- Merged from platform/android/rendering_context_driver_vulkan_android.cpp ---
#include "servers/display/display_server.h"

#ifdef VULKAN_ENABLED

#include <drivers/vulkan/godot_vulkan.h>

const char *RenderingContextDriverVulkanAndroid::_get_platform_surface_extension() const {
	return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkanAndroid::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkAndroidSurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	create_info.window = wpd->window;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateAndroidSurfaceKHR(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

bool RenderingContextDriverVulkanAndroid::_use_validation_layers() const {
	TightLocalVector<const char *> layer_names;
	Error err = _find_validation_layers(layer_names);

	// On Android, we use validation layers automatically if they were explicitly linked with the app.
	return (err == OK) && !layer_names.is_empty();
}

#endif // VULKAN_ENABLED


// --- Merged from platform/android/dialog_utils_jni.cpp ---
// --- Inlined from platform/android/dialog_utils_jni.h ---
#include <jni.h>

extern "C" {
JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_dialogCallback(JNIEnv *env, jclass clazz, jint p_button_index);
JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_inputDialogCallback(JNIEnv *env, jclass clazz, jstring p_text);
}

// --- End platform/android/dialog_utils_jni.h ---

#include "servers/display/display_server.h"

extern "C" {

JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_dialogCallback(JNIEnv *env, jclass clazz, jint p_button_index) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_dialog_callback(p_button_index);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_inputDialogCallback(JNIEnv *env, jclass clazz, jstring p_text) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		String text = jstring_to_string(p_text, env);
		ds->emit_input_dialog_callback(text);
	}
}
}

// === Merged from modules/openxr/extensions/platform/openxr_android_extension.cpp ===
#include "core/version_generated.gen.h"
#if defined(MODULE_OPENXR_ENABLED)
/*  openxr_android_extension.cpp                                          */
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

#define WANT_OPENXR_PLATFORM_INC
#include "servers/display/display_server.h"

#include "servers/display/display_server.h" // IWYU pragma: keep. `XR_FAILED` macro.
#include "servers/display/display_server.h"

#include "core/os/os.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

void OpenXRAndroidExtension::initialize() {
	if (data) {
		return;
	}
	data = memnew(Data);

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->GetJavaVM(&data->vm);
	data->activity_object = env->NewGlobalRef(OS_Android::get_godot_java()->get_activity());
}

HashMap<String, bool *> OpenXRAndroidExtension::get_requested_extensions(XrVersion p_version) {
	HashMap<String, bool *> request_extensions;

	// XR_KHR_LOADER_INIT_EXTENSION_NAME is a dependency of XR_KHR_LOADER_INIT_ANDROID_EXTENSION_NAME
	request_extensions[XR_KHR_LOADER_INIT_EXTENSION_NAME] = &data->loader_init_extension_available;
	request_extensions[XR_KHR_LOADER_INIT_ANDROID_EXTENSION_NAME] = &data->loader_init_android_extension_available;
	request_extensions[XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME] = &data->create_instance_extension_available;

	return request_extensions;
}

void OpenXRAndroidExtension::on_before_instance_created() {
	if (XR_FAILED(EXT_TRY_INIT_XR_FUNC(xrInitializeLoaderKHR))) {
		// XR_KHR_loader_init not supported on this platform
		return;
	}
	data->loader_init_android_extension_available = true;

	XrLoaderInitInfoAndroidKHR loader_init_info_android = {
		.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
		.next = nullptr,
		.applicationVM = data->vm,
		.applicationContext = data->activity_object
	};
	XrResult result = xrInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR *)&loader_init_info_android);
	ERR_FAIL_COND_MSG(XR_FAILED(result), "Failed to call xrInitializeLoaderKHR");
}

// We're keeping the Android create info struct here to avoid including openxr_platform.h in a header, which would break other extensions.
// This is reasonably safe as the struct is only used during initialization and the extension is a singleton.
static XrInstanceCreateInfoAndroidKHR instance_create_info;

void *OpenXRAndroidExtension::set_instance_create_info_and_get_next_pointer(XrVersion p_xr_version, void *p_next_pointer) {
	if (!data->create_instance_extension_available) {
		if (!data->loader_init_android_extension_available) {
			WARN_PRINT("No Android extensions available, couldn't pass JVM and Activity to OpenXR");
		}
		return nullptr;
	}

	instance_create_info = {
		.type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
		.next = p_next_pointer,
		.applicationVM = data->vm,
		.applicationActivity = data->activity_object
	};
	return &instance_create_info;
}

void OpenXRAndroidExtension::finalize() {
	if (!data) {
		return;
	}
	JNIEnv *env = get_jni_env();
	if (env) {
		env->DeleteGlobalRef(data->activity_object);
	}
	memdelete(data);
	data = nullptr;
}
#endif // defined(MODULE_OPENXR_ENABLED)
