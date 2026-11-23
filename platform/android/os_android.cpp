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

#include "os_android.h"

#include "platform/android/os_android.h"

#ifndef TOOLS_ENABLED
#include "platform/android/os_android.h"
#endif

#include "core/config/engine.h"
#include "core/extension/gdextension_manager.h"
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "drivers/unix/os_unix.h"
#include "main/main.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

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

class AndroidLogger : public Logger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err) {
		__android_log_vprint(p_err ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "voltaire", p_format, p_list);
	}

	virtual ~AndroidLogger() {}
};

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
	loggers.push_back(memnew(AndroidLogger));
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
#include "platform/android/os_android.h"


#include "core/string/print_string.h"

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
#include "platform/android/os_android.h"


#include "core/templates/local_vector.h"

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
#include "platform/android/os_android.h"

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
#include "platform/android/os_android.h"


#include "core/string/print_string.h"

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
#include "platform/android/os_android.h"


#include "core/config/project_settings.h"
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
#include "platform/android/os_android.h"

#include "core/os/thread.h"

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
#include "platform/android/os_android.h"


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
