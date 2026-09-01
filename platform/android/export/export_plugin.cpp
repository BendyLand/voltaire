/**************************************************************************/
/*  export_plugin.cpp                                                     */
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
#include "core/io/json.h"
#include "core/io/marshalls.h"
#include "core/math/random_pcg.h"
#include "core/os/os.h"
#include "core/os/shared_object.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_plugin.h"
#include "editor/export/export_template_manager.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "export_plugin.h"
#include "logo_svg.gen.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "modules/svg/image_loader_svg.h"
#include "run_icon_svg.gen.h"
#include "scene/resources/image_texture.h"

#ifdef MODULE_MONO_ENABLED
#include "modules/mono/utils/path_utils.h"
#endif

#ifdef ANDROID_ENABLED
#include "../os_android.h"
#include "android_editor_gradle_runner.h"
#endif

#ifndef ANDROID_ENABLED
#include "editor/editor_log.h"
#include "editor/editor_string_names.h"
#endif

static const char* ANDROID_PERMS[] = {"ACCESS_CHECKIN_PROPERTIES", "ACCESS_COARSE_LOCATION",
	"ACCESS_FINE_LOCATION", "ACCESS_LOCATION_EXTRA_COMMANDS", "ACCESS_MEDIA_LOCATION",
	"ACCESS_MOCK_LOCATION", "ACCESS_NETWORK_STATE", "ACCESS_SURFACE_FLINGER", "ACCESS_WIFI_STATE",
	"ACCOUNT_MANAGER", "ADD_VOICEMAIL", "AUTHENTICATE_ACCOUNTS", "BATTERY_STATS",
	"BIND_ACCESSIBILITY_SERVICE", "BIND_APPWIDGET", "BIND_DEVICE_ADMIN", "BIND_INPUT_METHOD",
	"BIND_NFC_SERVICE", "BIND_NOTIFICATION_LISTENER_SERVICE", "BIND_PRINT_SERVICE",
	"BIND_REMOTEVIEWS", "BIND_TEXT_SERVICE", "BIND_VPN_SERVICE", "BIND_WALLPAPER", "BLUETOOTH",
	"BLUETOOTH_ADMIN", "BLUETOOTH_PRIVILEGED", "BRICK", "BROADCAST_PACKAGE_REMOVED",
	"BROADCAST_SMS", "BROADCAST_STICKY", "BROADCAST_WAP_PUSH", "CALL_PHONE", "CALL_PRIVILEGED",
	"CAMERA", "CAPTURE_AUDIO_OUTPUT", "CAPTURE_SECURE_VIDEO_OUTPUT", "CAPTURE_VIDEO_OUTPUT",
	"CHANGE_COMPONENT_ENABLED_STATE", "CHANGE_CONFIGURATION", "CHANGE_NETWORK_STATE",
	"CHANGE_WIFI_MULTICAST_STATE", "CHANGE_WIFI_STATE", "CLEAR_APP_CACHE", "CLEAR_APP_USER_DATA",
	"CONTROL_LOCATION_UPDATES", "DELETE_CACHE_FILES", "DELETE_PACKAGES", "DEVICE_POWER",
	"DIAGNOSTIC", "DISABLE_KEYGUARD", "DUMP", "EXPAND_STATUS_BAR", "FACTORY_TEST", "FLASHLIGHT",
	"FORCE_BACK", "GET_ACCOUNTS", "GET_PACKAGE_SIZE", "GET_TASKS", "GET_TOP_ACTIVITY_INFO",
	"GLOBAL_SEARCH", "HARDWARE_TEST", "INJECT_EVENTS", "INSTALL_LOCATION_PROVIDER",
	"INSTALL_PACKAGES", "INSTALL_SHORTCUT", "INTERNAL_SYSTEM_WINDOW", "INTERNET",
	"KILL_BACKGROUND_PROCESSES", "LOCATION_HARDWARE", "MANAGE_ACCOUNTS", "MANAGE_APP_TOKENS",
	"MANAGE_DOCUMENTS", "MANAGE_EXTERNAL_STORAGE", "MANAGE_MEDIA", "MASTER_CLEAR",
	"MEDIA_CONTENT_CONTROL", "MODIFY_AUDIO_SETTINGS", "MODIFY_PHONE_STATE",
	"MOUNT_FORMAT_FILESYSTEMS", "MOUNT_UNMOUNT_FILESYSTEMS", "NFC", "PERSISTENT_ACTIVITY",
	"POST_NOTIFICATIONS", "PROCESS_OUTGOING_CALLS", "READ_CALENDAR", "READ_CALL_LOG",
	"READ_CONTACTS", "READ_EXTERNAL_STORAGE", "READ_FRAME_BUFFER", "READ_HISTORY_BOOKMARKS",
	"READ_INPUT_STATE", "READ_LOGS", "READ_MEDIA_AUDIO", "READ_MEDIA_IMAGES", "READ_MEDIA_VIDEO",
	"READ_MEDIA_VISUAL_USER_SELECTED", "READ_PHONE_STATE", "READ_PROFILE", "READ_SMS",
	"READ_SOCIAL_STREAM", "READ_SYNC_SETTINGS", "READ_SYNC_STATS", "READ_USER_DICTIONARY", "REBOOT",
	"RECEIVE_BOOT_COMPLETED", "RECEIVE_MMS", "RECEIVE_SMS", "RECEIVE_WAP_PUSH", "RECORD_AUDIO",
	"REORDER_TASKS", "RESTART_PACKAGES", "SEND_RESPOND_VIA_MESSAGE", "SEND_SMS",
	"SET_ACTIVITY_WATCHER", "SET_ALARM", "SET_ALWAYS_FINISH", "SET_ANIMATION_SCALE",
	"SET_DEBUG_APP", "SET_ORIENTATION", "SET_POINTER_SPEED", "SET_PREFERRED_APPLICATIONS",
	"SET_PROCESS_LIMIT", "SET_TIME", "SET_TIME_ZONE", "SET_WALLPAPER", "SET_WALLPAPER_HINTS",
	"SIGNAL_PERSISTENT_PROCESSES", "STATUS_BAR", "SUBSCRIBED_FEEDS_READ", "SUBSCRIBED_FEEDS_WRITE",
	"SYSTEM_ALERT_WINDOW", "TRANSMIT_IR", "UNINSTALL_SHORTCUT", "UPDATE_DEVICE_STATS",
	"USE_CREDENTIALS", "USE_SIP", "VIBRATE", "WAKE_LOCK", "WRITE_APN_SETTINGS", "WRITE_CALENDAR",
	"WRITE_CALL_LOG", "WRITE_CONTACTS", "WRITE_EXTERNAL_STORAGE", "WRITE_GSERVICES",
	"WRITE_HISTORY_BOOKMARKS", "WRITE_PROFILE", "WRITE_SECURE_SETTINGS", "WRITE_SETTINGS",
	"WRITE_SMS", "WRITE_SOCIAL_STREAM", "WRITE_SYNC_SETTINGS", "WRITE_USER_DICTIONARY", nullptr};

static const char* MISMATCHED_VERSIONS_MESSAGE =
	"Android build version mismatch:\n| Template installed: %s\n| Requested version: %s\nPlease "
	"reinstall Android build template from 'Project' menu.";

static const char* GDEXTENSION_LIBS_PATH = "libs/gdextensionlibs.json";

// This template string must be in sync with the content of
// 'platform/android/java/lib/src/main/java/res/mipmap-anydpi-v26/icon.xml'.
static const String ICON_XML_TEMPLATE =
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	"<adaptive-icon xmlns:android=\"http://schemas.android.com/apk/res/android\">\n"
	"    <background android:drawable=\"@mipmap/icon_background\"/>\n"
	"    <foreground android:drawable=\"@mipmap/icon_foreground\"/>\n"
	"%s" // Placeholder for the optional monochrome tag.
	"</adaptive-icon>";

static const String ICON_XML_PATH = "res/mipmap-anydpi-v26/icon.xml";
static const String THEMED_ICON_XML_PATH = "res/mipmap-anydpi-v26/themed_icon.xml";

static const String ANDROID_SPLASH_ICON_PATH = "res/drawable/splash_icon.webp";
static const String ANDROID_SPLASH_BRANDING_IMAGE_PATH = "res/drawable/splash_branding_image.webp";

static const char* DISABLE_GODOT_SPLASH_OPTION = PNAME("splash_screen/disable_godot_boot_splash");
static const char* ANDROID_SPLASH_ICON_OPTION = PNAME("splash_screen/icon");
static const char* ANDROID_SPLASH_BACKGROUND_COLOR_OPTION = PNAME("splash_screen/background_color");
static const char* ANDROID_SPLASH_BRANDING_IMAGE_OPTION = PNAME("splash_screen/branding_image");

static const int ICON_DENSITIES_COUNT = 6;
static const char* LAUNCHER_ICON_OPTION = PNAME("launcher_icons/main_192x192");
static const char* LAUNCHER_ADAPTIVE_ICON_FOREGROUND_OPTION =
	PNAME("launcher_icons/adaptive_foreground_432x432");
static const char* LAUNCHER_ADAPTIVE_ICON_BACKGROUND_OPTION =
	PNAME("launcher_icons/adaptive_background_432x432");
static const char* LAUNCHER_ADAPTIVE_ICON_MONOCHROME_OPTION =
	PNAME("launcher_icons/adaptive_monochrome_432x432");

static const LauncherIcon LAUNCHER_ICONS[ICON_DENSITIES_COUNT] = {
	{"res/mipmap-xxxhdpi-v4/icon.webp", 192}, {"res/mipmap-xxhdpi-v4/icon.webp", 144},
	{"res/mipmap-xhdpi-v4/icon.webp", 96}, {"res/mipmap-hdpi-v4/icon.webp", 72},
	{"res/mipmap-mdpi-v4/icon.webp", 48}, {"res/mipmap/icon.webp", 192}};

static const LauncherIcon LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[ICON_DENSITIES_COUNT] = {
	{"res/mipmap-xxxhdpi-v4/icon_foreground.webp", 432},
	{"res/mipmap-xxhdpi-v4/icon_foreground.webp", 324},
	{"res/mipmap-xhdpi-v4/icon_foreground.webp", 216},
	{"res/mipmap-hdpi-v4/icon_foreground.webp", 162},
	{"res/mipmap-mdpi-v4/icon_foreground.webp", 108}, {"res/mipmap/icon_foreground.webp", 432}};

static const LauncherIcon LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[ICON_DENSITIES_COUNT] = {
	{"res/mipmap-xxxhdpi-v4/icon_background.webp", 432},
	{"res/mipmap-xxhdpi-v4/icon_background.webp", 324},
	{"res/mipmap-xhdpi-v4/icon_background.webp", 216},
	{"res/mipmap-hdpi-v4/icon_background.webp", 162},
	{"res/mipmap-mdpi-v4/icon_background.webp", 108}, {"res/mipmap/icon_background.webp", 432}};

static const LauncherIcon LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[ICON_DENSITIES_COUNT] = {
	{"res/mipmap-xxxhdpi-v4/icon_monochrome.webp", 432},
	{"res/mipmap-xxhdpi-v4/icon_monochrome.webp", 324},
	{"res/mipmap-xhdpi-v4/icon_monochrome.webp", 216},
	{"res/mipmap-hdpi-v4/icon_monochrome.webp", 162},
	{"res/mipmap-mdpi-v4/icon_monochrome.webp", 108}, {"res/mipmap/icon_monochrome.webp", 432}};

static const int EXPORT_FORMAT_APK = 0;
static const int EXPORT_FORMAT_AAB = 1;

static const char* APK_ASSETS_DIRECTORY = "src/main/assets";
static const char* AAB_ASSETS_DIRECTORY = "assetPackInstallTime/src/main/assets";

static const int DEFAULT_MIN_SDK_VERSION =
	24; // Should match the value in 'platform/android/java/app/config.gradle#minSdk'
static const int VULKAN_MIN_SDK_VERSION =
	29; // Minimum recommended sdk version for Vulkan 1.1 support. See
		// https://developer.android.com/games/develop/vulkan/native-engine-support#recommendations
static const int DEFAULT_TARGET_SDK_VERSION =
	36; // Should match the value in 'platform/android/java/app/config.gradle#targetSdk'

#ifndef ANDROID_ENABLED
void EditorExportPlatformAndroid::_update_preset_status()
{
	bool has_runnable =
		EditorExport::get_singleton()->get_runnable_preset_for_platform(this).is_valid();
	if (has_runnable) {
		has_runnable_preset.set();
		_start_check_for_changes_poll_thread();
	}
	else {
		has_runnable_preset.clear();
		_stop_check_for_changes_poll_thread();
	}
	devices_changed.set();
}

void EditorExportPlatformAndroid::_start_check_for_changes_poll_thread()
{
	quit_request.clear();
	if (!check_for_changes_thread.is_started()) {
		check_for_changes_thread.start(_check_for_changes_poll_thread, this);
	}
}

void EditorExportPlatformAndroid::_stop_check_for_changes_poll_thread()
{
	quit_request.set();
	if (check_for_changes_thread.is_started()) {
		check_for_changes_thread.wait_to_finish();
	}
}
#endif

String EditorExportPlatformAndroid::get_package_name(
	const Ref<EditorExportPreset>& p_preset, const String& p_package) const
{
	String pname = p_package;
	String name = get_valid_basename(p_preset);
	pname = pname.replace("$genname", name);
	return pname;
}

String EditorExportPlatformAndroid::get_assets_directory(
	const Ref<EditorExportPreset>& p_preset, int p_export_format) const
{
	String gradle_build_directory = ExportTemplateManager::get_android_build_directory(p_preset);
	return gradle_build_directory.path_join(
		p_export_format == EXPORT_FORMAT_AAB ? AAB_ASSETS_DIRECTORY : APK_ASSETS_DIRECTORY);
}

bool EditorExportPlatformAndroid::is_package_name_valid(
	const Ref<EditorExportPreset>& p_preset, const String& p_package, String* r_error) const
{
	String pname = get_package_name(p_preset, p_package);

	if (pname.length() == 0) {
		if (r_error) {
			*r_error = TTR("Package name is missing.");
		}
		return false;
	}

	int segments = 0;
	bool first = true;
	for (int i = 0; i < pname.length(); i++) {
		char32_t c = pname[i];
		if (first && c == '.') {
			if (r_error) {
				*r_error = TTR("Package segments must be of non-zero length.");
			}
			return false;
		}
		if (c == '.') {
			segments++;
			first = true;
			continue;
		}
		if (!is_ascii_identifier_char(c)) {
			if (r_error) {
				*r_error = vformat(
					TTR("The character '%s' is not allowed in Android application package names."),
					String::chr(c));
			}
			return false;
		}
		if (first && is_digit(c)) {
			if (r_error) {
				*r_error = TTR("A digit cannot be the first character in a package segment.");
			}
			return false;
		}
		if (first && is_underscore(c)) {
			if (r_error) {
				*r_error = vformat(
					TTR("The character '%s' cannot be the first character in a package segment."),
					String::chr(c));
			}
			return false;
		}
		first = false;
	}

	if (segments == 0) {
		if (r_error) {
			*r_error = TTR("The package must have at least one '.' separator.");
		}
		return false;
	}

	if (first) {
		if (r_error) {
			*r_error = TTR("Package segments must be of non-zero length.");
		}
		return false;
	}

	return true;
}

bool EditorExportPlatformAndroid::_should_compress_asset(
	const String& p_path, const Vector<uint8_t>& p_data)
{
	/*
	 *  By not compressing files with little or no benefit in doing so,
	 *  a performance gain is expected at runtime. Moreover, if the APK is
	 *  zip-aligned, assets stored as they are can be efficiently read by
	 *  Android by memory-mapping them.
	 */

	// -- Unconditional uncompress to mimic AAPT plus some other

	static const char* unconditional_compress_ext[] = {// From
		// https://github.com/android/platform_frameworks_base/blob/master/tools/aapt/Package.cpp
		// These formats are already compressed, or don't compress well:
		".jpg", ".jpeg", ".png", ".gif", ".wav", ".mp2", ".mp3", ".ogg", ".aac", ".mpg", ".mpeg",
		".mid", ".midi", ".smf", ".jet", ".rtttl", ".imy", ".xmf", ".mp4", ".m4a", ".m4v", ".3gp",
		".3gpp", ".3g2", ".3gpp2", ".amr", ".awb", ".wma", ".wmv",
		// Godot-specific:
		".webp", // Same reasoning as .png
		".cfb",	 // Don't let small config files slow-down startup
		".scn",	 // Binary scenes are usually already compressed
		".ctex", // Streamable textures are usually already compressed
		".pck",	 // Pack.
		// Trailer for easier processing
		nullptr};

	for (const char** ext = unconditional_compress_ext; *ext; ++ext) {
		if (p_path.to_lower().ends_with(String(*ext))) {
			return false;
		}
	}

	// -- Compressed resource?

	if (p_data.size() >= 4 && p_data[0] == 'R' && p_data[1] == 'S' && p_data[2] == 'C' &&
		p_data[3] == 'C') {
		// Already compressed
		return false;
	}

	// --- TODO: Decide on texture resources according to their image compression setting

	return true;
}

zip_fileinfo EditorExportPlatformAndroid::get_zip_fileinfo()
{
	OS::DateTime dt = OS::get_singleton()->get_datetime();

	zip_fileinfo zipfi;
	zipfi.tmz_date.tm_year = dt.year;
	zipfi.tmz_date.tm_mon = dt.month - 1; // tm_mon is zero indexed
	zipfi.tmz_date.tm_mday = dt.day;
	zipfi.tmz_date.tm_hour = dt.hour;
	zipfi.tmz_date.tm_min = dt.minute;
	zipfi.tmz_date.tm_sec = dt.second;
	zipfi.dosDate = 0;
	zipfi.external_fa = 0;
	zipfi.internal_fa = 0;

	return zipfi;
}

Vector<EditorExportPlatformAndroid::ABI> EditorExportPlatformAndroid::get_abis()
{
	// Should have the same order and size as get_archs.
	Vector<ABI> abis;
	abis.push_back(ABI("armeabi-v7a", "arm32"));
	abis.push_back(ABI("arm64-v8a", "arm64"));
	abis.push_back(ABI("x86", "x86_32"));
	abis.push_back(ABI("x86_64", "x86_64"));
	return abis;
}

#ifndef DISABLE_DEPRECATED
/// List the gdap files in the directory specified by the p_path parameter.
Vector<String> EditorExportPlatformAndroid::list_gdap_files(const String& p_path)
{
	Vector<String> dir_files;
	Ref<DirAccess> da = DirAccess::open(p_path);
	if (da.is_valid()) {
		da->list_dir_begin();
		while (true) {
			String file = da->get_next();
			if (file.is_empty()) {
				break;
			}

			if (da->current_is_dir() || da->current_is_hidden()) {
				continue;
			}

			if (file.ends_with(PluginConfigAndroid::PLUGIN_CONFIG_EXT)) {
				dir_files.push_back(file);
			}
		}
		da->list_dir_end();
	}

	return dir_files;
}

Vector<PluginConfigAndroid> EditorExportPlatformAndroid::get_plugins()
{
	Vector<PluginConfigAndroid> loaded_plugins;

	String plugins_dir =
		ProjectSettings::get_singleton()->get_resource_path().path_join("android/plugins");

	// Add the prebuilt plugins
	loaded_plugins.append_array(PluginConfigAndroid::get_prebuilt_plugins(plugins_dir));

	if (DirAccess::exists(plugins_dir)) {
		Vector<String> plugins_filenames = list_gdap_files(plugins_dir);

		if (!plugins_filenames.is_empty()) {
			Ref<ConfigFile> config_file;
			config_file.instantiate();
			for (int i = 0; i < plugins_filenames.size(); i++) {
				PluginConfigAndroid config = PluginConfigAndroid::load_plugin_config(
					config_file, plugins_dir.path_join(plugins_filenames[i]));
				if (config.valid_config) {
					loaded_plugins.push_back(config);
				}
				else {
					print_error("Invalid plugin config file " + plugins_filenames[i]);
				}
			}
		}
	}

	return loaded_plugins;
}

#endif // DISABLE_DEPRECATED

Error EditorExportPlatformAndroid::store_in_apk(
	APKExportData* ed, const String& p_path, const Vector<uint8_t>& p_data, int compression_method)
{
	zip_fileinfo zipfi = get_zip_fileinfo();
	zipOpenNewFileInZip(ed->apk, p_path.utf8().get_data(), &zipfi, nullptr, 0, nullptr, 0, nullptr,
		compression_method, Z_DEFAULT_COMPRESSION);

	zipWriteInFileInZip(ed->apk, p_data.ptr(), p_data.size());
	zipCloseFileInZip(ed->apk);

	return OK;
}

Error EditorExportPlatformAndroid::save_apk_so(
	const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so)
{
	if (!p_so.path.get_file().begins_with("lib")) {
		String err = "Android .so file names must start with \"lib\", but got: " + p_so.path;
		ERR_PRINT(err);
		return FAILED;
	}
	APKExportData* ed = static_cast<APKExportData*>(p_userdata);
	Vector<ABI> abis = get_abis();
	bool exported = false;
	for (int i = 0; i < p_so.tags.size(); ++i) {
		// shared objects can be fat (compatible with multiple ABIs)
		int abi_index = -1;
		for (int j = 0; j < abis.size(); ++j) {
			if (abis[j].abi == p_so.tags[i] || abis[j].arch == p_so.tags[i]) {
				abi_index = j;
				break;
			}
		}
		if (abi_index != -1) {
			exported = true;
			String abi = abis[abi_index].abi;
			String dst_path = String("lib").path_join(abi).path_join(p_so.path.get_file());
			Vector<uint8_t> array = FileAccess::get_file_as_bytes(p_so.path);
			Error store_err = store_in_apk(ed, dst_path, array, Z_NO_COMPRESSION);
			ERR_FAIL_COND_V_MSG(
				store_err, store_err, "Cannot store in apk file '" + dst_path + "'.");
		}
	}
	if (!exported) {
		ERR_PRINT("Cannot determine architecture for library \"" + p_so.path +
				  "\". One of the supported architectures must be used as a tag: " +
				  join_abis(abis, " ", true));
		return FAILED;
	}
	return OK;
}

Error EditorExportPlatformAndroid::save_apk_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	APKExportData* ed = static_cast<APKExportData*>(p_userdata);

	const String simplified_path = simplify_path(p_path);

	Vector<uint8_t> enc_data;
	EditorExportPlatform::SavedData sd;
	Error err = _store_temp_file(simplified_path, p_data, p_enc_in_filters, p_enc_ex_filters, p_key,
		p_seed, p_delta, enc_data, sd);
	if (err != OK) {
		return err;
	}

	String dst_path;
	if (ed->pd.salt.length() == 32) {
		dst_path = String("assets/") + (simplified_path + ed->pd.salt).sha256_text();
	}
	else {
		dst_path = String("assets/") + simplified_path.trim_prefix("res://");
	}
	print_verbose("Saving project files from " + simplified_path + " into " + dst_path);
	store_in_apk(
		ed, dst_path, enc_data, _should_compress_asset(simplified_path, enc_data) ? Z_DEFLATED : 0);

	ed->pd.file_ofs.push_back(sd);

	return OK;
}

Error EditorExportPlatformAndroid::ignore_apk_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	return OK;
}

Error EditorExportPlatformAndroid::copy_gradle_so(
	const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so)
{
	ERR_FAIL_COND_V_MSG(!p_so.path.get_file().begins_with("lib"), FAILED,
		"Android .so file names must start with \"lib\", but got: " + p_so.path);
	Vector<ABI> abis = get_abis();
	CustomExportData* export_data = static_cast<CustomExportData*>(p_userdata);
	bool exported = false;
	for (int i = 0; i < p_so.tags.size(); ++i) {
		int abi_index = -1;
		for (int j = 0; j < abis.size(); ++j) {
			if (abis[j].abi == p_so.tags[i] || abis[j].arch == p_so.tags[i]) {
				abi_index = j;
				break;
			}
		}
		if (abi_index != -1) {
			exported = true;
			String type = export_data->debug ? "debug" : "release";
			String abi = abis[abi_index].abi;
			String filename = p_so.path.get_file();
			String dst_path =
				export_data->libs_directory.path_join(type).path_join(abi).path_join(filename);
			Vector<uint8_t> data = FileAccess::get_file_as_bytes(p_so.path);
			print_verbose("Copying .so file from " + p_so.path + " to " + dst_path);
			Error err = store_file_at_path(dst_path, data);
			ERR_FAIL_COND_V_MSG(
				err, err, "Failed to copy .so file from " + p_so.path + " to " + dst_path);
			export_data->libs.push_back(dst_path);
		}
	}
	ERR_FAIL_COND_V_MSG(!exported, FAILED,
		"Cannot determine architecture for library \"" + p_so.path +
			"\". One of the supported architectures must be used as a tag:" +
			join_abis(abis, " ", true));
	return OK;
}

bool EditorExportPlatformAndroid::_has_read_write_storage_permission(
	const Vector<String>& p_permissions)
{
	return p_permissions.has("android.permission.READ_EXTERNAL_STORAGE") ||
		   p_permissions.has("android.permission.WRITE_EXTERNAL_STORAGE");
}

bool EditorExportPlatformAndroid::_has_manage_external_storage_permission(
	const Vector<String>& p_permissions)
{
	return p_permissions.has("android.permission.MANAGE_EXTERNAL_STORAGE");
}

void EditorExportPlatformAndroid::_write_tmp_manifest(
	const Ref<EditorExportPreset>& p_preset, bool p_give_internet, bool p_debug)
{
	print_verbose("Building temporary manifest...");
	String manifest_text =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n"
		"    xmlns:tools=\"http://schemas.android.com/tools\">\n";

	manifest_text += _get_screen_sizes_tag(p_preset);
	manifest_text += _get_gles_tag();

	Vector<String> perms;
	Vector<FeatureInfo> features;
	Vector<MetadataInfo> manifest_metadata;
	_get_manifest_info(p_preset, p_give_internet, perms, features, manifest_metadata);
	for (int i = 0; i < perms.size(); i++) {
		String permission = perms.get(i);
		if (permission == "android.permission.WRITE_EXTERNAL_STORAGE" ||
			(permission == "android.permission.READ_EXTERNAL_STORAGE" &&
				_has_manage_external_storage_permission(perms))) {
			manifest_text += vformat(
				"    <uses-permission android:name=\"%s\" android:maxSdkVersion=\"29\" />\n",
				permission);
		}
		else {
			manifest_text += vformat("    <uses-permission android:name=\"%s\" />\n", permission);
		}
	}

	for (int i = 0; i < features.size(); i++) {
		manifest_text += vformat("    <uses-feature tools:node=\"replace\" android:name=\"%s\" "
								 "android:required=\"%s\" android:version=\"%s\" />\n",
			features[i].name, features[i].required, features[i].version);
	}

	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_singleton()->get_export_plugins();
	for (int i = 0; i < export_plugins.size(); i++) {
		if (export_plugins[i]->supports_platform(Ref<EditorExportPlatform>(this))) {
			const String contents = export_plugins[i]->get_android_manifest_element_contents(
				Ref<EditorExportPlatform>(this), p_debug);
			if (!contents.is_empty()) {
				const String export_plugin_name = export_plugins[i]->get_name();
				manifest_text +=
					"<!-- Start of manifest element contents from " + export_plugin_name + " -->\n";
				manifest_text += contents;
				manifest_text += "\n";
				manifest_text +=
					"<!-- End of manifest element contents from " + export_plugin_name + " -->\n";
			}
		}
	}

	manifest_text += _get_application_tag(Ref<EditorExportPlatform>(this), p_preset,
		_has_read_write_storage_permission(perms), p_debug, manifest_metadata);
	manifest_text += "</manifest>\n";
	String manifest_path = ExportTemplateManager::get_android_build_directory(p_preset).path_join(
		vformat("src/%s/AndroidManifest.xml", (p_debug ? "debug" : "release")));

	print_verbose("Storing manifest into " + manifest_path + ": " + "\n" + manifest_text);
	store_string_at_path(manifest_path, manifest_text);
}

String EditorExportPlatformAndroid::_parse_string(const uint8_t* p_bytes, bool p_utf8)
{
	uint32_t offset = 0;
	uint32_t len = 0;

	if (p_utf8) {
		uint8_t byte = p_bytes[offset];
		if (byte & 0x80) {
			offset += 2;
		}
		else {
			offset += 1;
		}
		byte = p_bytes[offset];
		offset++;
		if (byte & 0x80) {
			len = byte & 0x7F;
			len = (len << 8) + p_bytes[offset];
			offset++;
		}
		else {
			len = byte;
		}
	}
	else {
		len = decode_uint16(&p_bytes[offset]);
		offset += 2;
		if (len & 0x8000) {
			len &= 0x7FFF;
			len = (len << 16) + decode_uint16(&p_bytes[offset]);
			offset += 2;
		}
	}

	if (p_utf8) {
		Vector<uint8_t> str8;
		str8.resize(len + 1);
		for (uint32_t i = 0; i < len; i++) {
			str8.write[i] = p_bytes[offset + i];
		}
		str8.write[len] = 0;
		return String::utf8((const char*)str8.ptr(), len);
	}
	else {
		String str;
		for (uint32_t i = 0; i < len; i++) {
			char32_t c = decode_uint16(&p_bytes[offset + i * 2]);
			if (c == 0) {
				break;
			}
			str += String::chr(c);
		}
		return str;
	}
}

void EditorExportPlatformAndroid::_process_launcher_icons(const String& p_file_name,
	const Ref<Image>& p_source_image, int dimension, Vector<uint8_t>& p_data)
{
	Ref<Image> working_image = p_source_image;

	if (p_source_image->get_width() != dimension || p_source_image->get_height() != dimension) {
		working_image = p_source_image->duplicate();
		working_image->resize(dimension, dimension, Image::Interpolation::INTERPOLATE_LANCZOS);
	}

	Vector<uint8_t> buffer = working_image->save_webp_to_buffer();
	p_data.resize(buffer.size());
	memcpy(p_data.ptrw(), buffer.ptr(), p_data.size());
}

void EditorExportPlatformAndroid::_copy_icons_to_gradle_project(
	const Ref<EditorExportPreset>& p_preset, const Ref<Image>& p_main_image,
	const Ref<Image>& p_foreground, const Ref<Image>& p_background, const Ref<Image>& p_monochrome,
	const Ref<Image>& p_splash_icon, const Ref<Image>& p_splash_branding_image)
{
	String gradle_build_dir = ExportTemplateManager::get_android_build_directory(p_preset);

	// Copy splash screen icon to the drawable directory.
	// This is only for png/webp/svg file; XML file is handled in _fix_themes_xml().
	if (p_splash_icon.is_valid() && !p_splash_icon->is_empty()) {
		print_verbose("Copying splash screen icon into " + ANDROID_SPLASH_ICON_PATH);
		Vector<uint8_t> buffer = p_splash_icon->save_webp_to_buffer();
		store_file_at_path(gradle_build_dir.path_join(ANDROID_SPLASH_ICON_PATH), buffer);
	}

	if (p_splash_branding_image.is_valid() && !p_splash_branding_image->is_empty()) {
		print_verbose(
			"Copying splash screen branding image into " + ANDROID_SPLASH_BRANDING_IMAGE_PATH);
		Vector<uint8_t> buffer = p_splash_branding_image->save_webp_to_buffer();
		store_file_at_path(gradle_build_dir.path_join(ANDROID_SPLASH_BRANDING_IMAGE_PATH), buffer);
	}

	String monochrome_tag = "";

	// Prepare images to be resized for the icons. If some image ends up being uninitialized,
	// the default image from the export template will be used.

	for (int i = 0; i < ICON_DENSITIES_COUNT; ++i) {
		if (p_main_image.is_valid() && !p_main_image->is_empty()) {
			print_verbose("Processing launcher icon for dimension " +
						  itos(LAUNCHER_ICONS[i].dimensions) + " into " +
						  LAUNCHER_ICONS[i].export_path);
			Vector<uint8_t> data;
			_process_launcher_icons(
				LAUNCHER_ICONS[i].export_path, p_main_image, LAUNCHER_ICONS[i].dimensions, data);
			store_file_at_path(gradle_build_dir.path_join(LAUNCHER_ICONS[i].export_path), data);
		}

		if (p_foreground.is_valid() && !p_foreground->is_empty()) {
			print_verbose("Processing launcher adaptive icon p_foreground for dimension " +
						  itos(LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[i].dimensions) + " into " +
						  LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[i].export_path);
			Vector<uint8_t> data;
			_process_launcher_icons(LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[i].export_path, p_foreground,
				LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[i].dimensions, data);
			store_file_at_path(
				gradle_build_dir.path_join(LAUNCHER_ADAPTIVE_ICON_FOREGROUNDS[i].export_path),
				data);
		}

		if (p_background.is_valid() && !p_background->is_empty()) {
			print_verbose("Processing launcher adaptive icon p_background for dimension " +
						  itos(LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[i].dimensions) + " into " +
						  LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[i].export_path);
			Vector<uint8_t> data;
			_process_launcher_icons(LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[i].export_path, p_background,
				LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[i].dimensions, data);
			store_file_at_path(
				gradle_build_dir.path_join(LAUNCHER_ADAPTIVE_ICON_BACKGROUNDS[i].export_path),
				data);
		}

		if (p_monochrome.is_valid() && !p_monochrome->is_empty()) {
			print_verbose("Processing launcher adaptive icon p_monochrome for dimension " +
						  itos(LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[i].dimensions) + " into " +
						  LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[i].export_path);
			Vector<uint8_t> data;
			_process_launcher_icons(LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[i].export_path, p_monochrome,
				LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[i].dimensions, data);
			store_file_at_path(
				gradle_build_dir.path_join(LAUNCHER_ADAPTIVE_ICON_MONOCHROMES[i].export_path),
				data);
			monochrome_tag = "    <monochrome android:drawable=\"@mipmap/icon_monochrome\"/>\n";
		}
	}

	// Finalize the icon.xml by formatting the template with the optional monochrome tag.
	store_string_at_path(
		gradle_build_dir.path_join(ICON_XML_PATH), vformat(ICON_XML_TEMPLATE, monochrome_tag));
}

String EditorExportPlatformAndroid::get_name() const { return "Android"; }

String EditorExportPlatformAndroid::get_os_name() const { return "Android"; }

Ref<Texture2D> EditorExportPlatformAndroid::get_logo() const { return logo; }

bool EditorExportPlatformAndroid::should_update_export_options()
{
#ifndef DISABLE_DEPRECATED
	if (android_plugins_changed.is_set()) {
		// don't clear unless we're reporting true, to avoid race
		android_plugins_changed.clear();
		return true;
	}
#endif // DISABLE_DEPRECATED
	return false;
}

#ifndef ANDROID_ENABLED
bool EditorExportPlatformAndroid::poll_export()
{
	bool dc = devices_changed.is_set();
	if (dc) {
		// don't clear unless we're reporting true, to avoid race
		devices_changed.clear();
	}
	return dc;
}

int EditorExportPlatformAndroid::get_options_count() const
{
	MutexLock lock(device_lock);
	return devices.size() + 1;
}

Ref<Texture2D> EditorExportPlatformAndroid::get_option_icon(int p_index) const
{
	if (p_index == 0) {
		Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
		ERR_FAIL_COND_V(theme.is_null(), Ref<ImageTexture>());
		return theme->get_icon(use_scrcpy ? SNAME("GuiChecked") : SNAME("GuiUnchecked"),
			EditorStringName(EditorIcons));
	}
	return EditorExportPlatform::get_option_icon(p_index - 1);
}

String EditorExportPlatformAndroid::get_options_tooltip() const
{
	return TTR("Select device from the list");
}

String EditorExportPlatformAndroid::get_option_label(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, devices.size() + 1, "");
	if (p_index == 0) {
		return TTR("Mirror Android devices");
	}
	MutexLock lock(device_lock);
	return devices[p_index - 1].name;
}

String EditorExportPlatformAndroid::get_option_tooltip(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, devices.size() + 1, "");
	if (p_index == 0) {
		return TTR("If enabled, \"scrcpy\" is used to start the project and automatically stream "
				   "device display (or virtual display) content.");
	}
	MutexLock lock(device_lock);
	String s = devices[p_index - 1].description;
	if (devices.size() == 1) {
		// Tooltip will be:
		// Name
		// Description
		s = devices[p_index - 1].name + "\n\n" + s;
	}
	return s;
}

String EditorExportPlatformAndroid::get_device_architecture(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, devices.size() + 1, "");
	if (p_index == 0) {
		return String();
	}
	MutexLock lock(device_lock);
	return devices[p_index - 1].architecture;
}

#endif // ANDROID_ENABLED

Ref<Texture2D> EditorExportPlatformAndroid::get_run_icon() const { return run_icon; }

static bool has_valid_keystore_credentials(String& r_error_str, const String& p_keystore,
	const String& p_username, const String& p_password, const String& p_type)
{
	String output;
	List<String> args;
	args.push_back("-list");
	args.push_back("-keystore");
	args.push_back(p_keystore);
	args.push_back("-storepass");
	args.push_back(p_password);
	args.push_back("-alias");
	args.push_back(p_username);
	String keytool_path = EditorExportPlatformAndroid::get_keytool_path();
	Error error = OS::get_singleton()->execute(keytool_path, args, &output, nullptr, true);
	String keytool_error = "keytool error:";
	bool valid = output.substr(0, keytool_error.length()) != keytool_error;

	if (error != OK) {
		r_error_str =
			TTR("Error: There was a problem validating the keystore username and password");
		return false;
	}
	if (!valid) {
		r_error_str = TTR(
			p_type + " Username and/or Password is invalid for the given " + p_type + " Keystore");
		return false;
	}
	r_error_str = "";
	return true;
}

#ifdef MODULE_MONO_ENABLED
static uint64_t _last_validate_tfm_time = 0;
static String _last_validate_tfm = "";

bool _validate_dotnet_tfm(const String& required_tfm, String& r_error)
{
	String assembly_name = Path::get_csharp_project_name();
	String project_path =
		ProjectSettings::get_singleton()->globalize_path("res://" + assembly_name + ".csproj");

	if (!FileAccess::exists(project_path)) {
		return true;
	}

	uint64_t modified_time = FileAccess::get_modified_time(project_path);
	String tfm;

	if (modified_time == _last_validate_tfm_time) {
		tfm = _last_validate_tfm;
	}
	else {
		String pipe;
		List<String> args;
		args.push_back("build");
		args.push_back(project_path);
		args.push_back("/p:GodotTargetPlatform=android");
		args.push_back("--getProperty:TargetFramework");

		int exitcode;
		Error err = OS::get_singleton()->execute("dotnet", args, &pipe, &exitcode, true);
		if (err != OK || exitcode != 0) {
			if (err != OK) {
				WARN_PRINT("Failed to execute dotnet command. Error " + String(error_names[err]));
			}
			else if (exitcode != 0) {
				print_line(pipe);
				WARN_PRINT("dotnet command exited with code " + itos(exitcode) +
						   ". See output above for more details.");
			}
			r_error +=
				vformat(TTR("Unable to determine the C# project's TFM, it may be incompatible. The "
							"export template only supports '%s'. Make sure the project targets "
							"'%s' or consider using gradle builds instead."),
					required_tfm, required_tfm) +
				"\n";
			return true;
		}
		else {
			tfm = pipe.strip_edges();
			_last_validate_tfm_time = modified_time;
			_last_validate_tfm = tfm;
		}
	}

	if (tfm != required_tfm) {
		r_error += vformat(TTR("C# project targets '%s' but the export template only supports "
							   "'%s'. Consider using gradle builds instead."),
					   tfm, required_tfm) +
				   "\n";
		return false;
	}

	return true;
}
#endif

void EditorExportPlatformAndroid::_clear_assets_directory(const Ref<EditorExportPreset>& p_preset)
{
	Ref<DirAccess> da_res = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	String gradle_build_directory = ExportTemplateManager::get_android_build_directory(p_preset);

	// Clear the APK assets directory
	String apk_assets_directory = gradle_build_directory.path_join(APK_ASSETS_DIRECTORY);
	if (da_res->dir_exists(apk_assets_directory)) {
		print_verbose("Clearing APK assets directory...");
		Ref<DirAccess> da_assets = DirAccess::open(apk_assets_directory);
		ERR_FAIL_COND(da_assets.is_null());

		da_assets->erase_contents_recursive();
		da_res->remove(apk_assets_directory);
	}

	// Clear the AAB assets directory
	String aab_assets_directory = gradle_build_directory.path_join(AAB_ASSETS_DIRECTORY);
	if (da_res->dir_exists(aab_assets_directory)) {
		print_verbose("Clearing AAB assets directory...");
		Ref<DirAccess> da_assets = DirAccess::open(aab_assets_directory);
		ERR_FAIL_COND(da_assets.is_null());

		da_assets->erase_contents_recursive();
		da_res->remove(aab_assets_directory);
	}
}

String EditorExportPlatformAndroid::join_list(
	const List<String>& p_parts, const String& p_separator)
{
	String ret;
	for (List<String>::ConstIterator itr = p_parts.begin(); itr != p_parts.end(); ++itr) {
		if (itr != p_parts.begin()) {
			ret += p_separator;
		}
		ret += *itr;
	}
	return ret;
}

String EditorExportPlatformAndroid::join_abis(
	const Vector<EditorExportPlatformAndroid::ABI>& p_parts, const String& p_separator,
	bool p_use_arch)
{
	String ret;
	for (int i = 0; i < p_parts.size(); ++i) {
		if (i > 0) {
			ret += p_separator;
		}
		ret += (p_use_arch) ? p_parts[i].arch : p_parts[i].abi;
	}
	return ret;
}

String EditorExportPlatformAndroid::_get_deprecated_plugins_names(
	const Ref<EditorExportPreset>& p_preset) const
{
	Vector<String> names;

#ifndef DISABLE_DEPRECATED
	PluginConfigAndroid::get_plugins_names(get_enabled_plugins(p_preset), names);
#endif // DISABLE_DEPRECATED

	String plugins_names = String("|").join(names);
	return plugins_names;
}

String EditorExportPlatformAndroid::_resolve_export_plugin_android_library_path(
	const String& p_android_library_path) const
{
	String absolute_path;
	if (!p_android_library_path.is_empty()) {
		if (p_android_library_path.is_absolute_path()) {
			absolute_path =
				ProjectSettings::get_singleton()->globalize_path(p_android_library_path);
		}
		else {
			const String export_plugin_absolute_path =
				String("res://addons/").path_join(p_android_library_path);
			absolute_path =
				ProjectSettings::get_singleton()->globalize_path(export_plugin_absolute_path);
		}
	}
	return absolute_path;
}

bool EditorExportPlatformAndroid::_is_clean_build_required(const Ref<EditorExportPreset>& p_preset)
{
	bool first_build = last_gradle_build_time == 0;
	bool have_plugins_changed = false;
	String gradle_build_dir = ExportTemplateManager::get_android_build_directory(p_preset);
	bool has_build_dir_changed = last_gradle_build_dir != gradle_build_dir;

	String plugin_names = _get_plugins_names(p_preset);

	if (!first_build) {
		have_plugins_changed = plugin_names != last_plugin_names;
#ifndef DISABLE_DEPRECATED
		if (!have_plugins_changed) {
			Vector<PluginConfigAndroid> enabled_plugins = get_enabled_plugins(p_preset);
			for (int i = 0; i < enabled_plugins.size(); i++) {
				if (enabled_plugins.get(i).last_updated > last_gradle_build_time) {
					have_plugins_changed = true;
					break;
				}
			}
		}
#endif // DISABLE_DEPRECATED
	}

	last_gradle_build_time = OS::get_singleton()->get_unix_time();
	last_gradle_build_dir = gradle_build_dir;
	last_plugin_names = plugin_names;

	return have_plugins_changed || has_build_dir_changed || first_build;
}

Error EditorExportPlatformAndroid::_generate_sparse_pck_metadata(
	const Ref<EditorExportPreset>& p_preset, PackData& p_pack_data, Vector<uint8_t>& r_data)
{
	Error err;
	Ref<FileAccess> ftmp =
		FileAccess::create_temp(FileAccess::WRITE_READ, "export_index", "tmp", false, &err);
	if (err != OK) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("Could not create temporary file!"));
		return err;
	}
	int64_t pck_start_pos = ftmp->get_position();
	uint64_t file_base_ofs = 0;
	uint64_t dir_base_ofs = 0;
	EditorExportPlatform::_store_header(ftmp,
		p_preset->get_enc_pck() && p_preset->get_enc_directory(), true, file_base_ofs, dir_base_ofs,
		p_pack_data.salt);

	// Write directory.
	uint64_t dir_offset = ftmp->get_position();
	ftmp->seek(dir_base_ofs);
	ftmp->store_64(dir_offset - pck_start_pos);
	ftmp->seek(dir_offset);

	Vector<uint8_t> key;
	if (p_preset->get_enc_pck() && p_preset->get_enc_directory()) {
		String script_key = _get_script_encryption_key(p_preset);
		key.resize(32);
		if (script_key.length() == 64) {
			for (int i = 0; i < 32; i++) {
				int v = 0;
				if (i * 2 < script_key.length()) {
					char32_t ct = script_key[i * 2];
					if (is_digit(ct)) {
						ct = ct - '0';
					}
					else if (ct >= 'a' && ct <= 'f') {
						ct = 10 + ct - 'a';
					}
					v |= ct << 4;
				}

				if (i * 2 + 1 < script_key.length()) {
					char32_t ct = script_key[i * 2 + 1];
					if (is_digit(ct)) {
						ct = ct - '0';
					}
					else if (ct >= 'a' && ct <= 'f') {
						ct = 10 + ct - 'a';
					}
					v |= ct;
				}
				key.write[i] = v;
			}
		}
	}

	if (!EditorExportPlatform::_encrypt_and_store_directory(
			ftmp, p_pack_data, key, p_preset->get_seed(), 0)) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("Can't create encrypted file."));
		return ERR_CANT_CREATE;
	}

	r_data.resize(ftmp->get_length());
	ftmp->seek(0);
	ftmp->get_buffer(r_data.ptrw(), r_data.size());
	ftmp.unref();

	return OK;
}

#ifdef ANDROID_ENABLED
// Copies the given keystore to temp file.
// Returns the new path on success, or an empty String on failure.
static String _copy_keystore_to_temp(
	const String& p_keystore_path, const String& p_build_path, const String& p_name)
{
	Error err;
	PackedByteArray keystore_data = FileAccess::get_file_as_bytes(p_keystore_path, &err);
	if (err != OK) {
		return String();
	}

	String temp_dir = p_build_path + "/.android";
	String temp_filename = temp_dir + "/" + p_name;

	DirAccess::make_dir_recursive_absolute(temp_dir);
	Ref<FileAccess> temp_file = FileAccess::open(temp_filename, FileAccess::WRITE);
	if (!temp_file.is_valid()) {
		return String();
	}

	temp_file->store_buffer(keystore_data);
	return temp_filename;
}
#endif

void EditorExportPlatformAndroid::get_platform_features(List<String>* r_features) const
{
	r_features->push_back("mobile");
	r_features->push_back("android");
}

void EditorExportPlatformAndroid::resolve_platform_feature_priorities(
	const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features)
{
}

EditorExportPlatformAndroid::~EditorExportPlatformAndroid()
{
#ifndef ANDROID_ENABLED
	_stop_check_for_changes_poll_thread();
#else
	memdelete(android_editor_gradle_runner);
#endif
}


