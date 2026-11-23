/**************************************************************************/
/*  export.cpp                                                            */
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

#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"
#include "editor/editor_ui.h"
// --- Inlined from platform/android/export/export_plugin.h ---
#ifndef DISABLE_DEPRECATED
// --- Folded from platform/android/export/godot_plugin_config.h ---
#ifndef DISABLE_DEPRECATED

#include "core/io/config_file.h"
#include "core/string/ustring.h"

/*
 The `config` section and fields are required and defined as follow:
- **name**: name of the plugin.
- **binary_type**: can be either `local` or `remote`. The type affects the **binary** field.
- **binary**:
  - if **binary_type** is `local`, then this should be the filename of the plugin `aar` file in the `res://android/plugins` directory (e.g: `MyPlugin.aar`).
  - if **binary_type** is `remote`, then this should be a declaration for a remote gradle binary (e.g: "org.godot.example:my-plugin:0.0.0").

The `dependencies` section and fields are optional and defined as follow:
- **local**: contains a list of local `.aar` binary files the plugin depends on. The local binary dependencies must also be located in the `res://android/plugins` directory.
- **remote**: contains a list of remote binary gradle dependencies for the plugin.
- **custom_maven_repos**: contains a list of urls specifying custom maven repos required for the plugin's dependencies.

 See https://github.com/godotengine/godot/issues/38157#issuecomment-618773871
 */
struct PluginConfigAndroid {
	inline static const char *PLUGIN_CONFIG_EXT = ".gdap";

	inline static const char *CONFIG_SECTION = "config";
	inline static const char *CONFIG_NAME_KEY = "name";
	inline static const char *CONFIG_BINARY_TYPE_KEY = "binary_type";
	inline static const char *CONFIG_BINARY_KEY = "binary";

	inline static const char *DEPENDENCIES_SECTION = "dependencies";
	inline static const char *DEPENDENCIES_LOCAL_KEY = "local";
	inline static const char *DEPENDENCIES_REMOTE_KEY = "remote";
	inline static const char *DEPENDENCIES_CUSTOM_MAVEN_REPOS_KEY = "custom_maven_repos";

	inline static const char *BINARY_TYPE_LOCAL = "local";
	inline static const char *BINARY_TYPE_REMOTE = "remote";

	// Set to true when the config file is properly loaded.
	bool valid_config = false;
	// Unix timestamp of last change to this plugin.
	uint64_t last_updated = 0;

	// Required config section
	String name;
	String binary_type;
	String binary;

	// Optional dependencies section
	Vector<String> local_dependencies;
	Vector<String> remote_dependencies;
	Vector<String> custom_maven_repos;

	static String resolve_local_dependency_path(String plugin_config_dir, String dependency_path);

	static PluginConfigAndroid resolve_prebuilt_plugin(PluginConfigAndroid prebuilt_plugin, String plugin_config_dir);

	static Vector<PluginConfigAndroid> get_prebuilt_plugins(String plugins_base_dir);

	static bool is_plugin_config_valid(PluginConfigAndroid plugin_config);

	static uint64_t get_plugin_modification_time(const PluginConfigAndroid &plugin_config, const String &config_path);

	static PluginConfigAndroid load_plugin_config(Ref<ConfigFile> config_file, const String &path);

	static void get_plugins_binaries(String binary_type, Vector<PluginConfigAndroid> plugins_configs, Vector<String> &r_result);

	static void get_plugins_custom_maven_repos(Vector<PluginConfigAndroid> plugins_configs, Vector<String> &r_result);

	static void get_plugins_names(Vector<PluginConfigAndroid> plugins_configs, Vector<String> &r_result);
};

#endif // DISABLE_DEPRECATED

// --- End platform/android/export/godot_plugin_config.h ---
#endif // DISABLE_DEPRECATED

#include "core/io/image.h"
#include "core/io/file_access.h"
#include "editor/editor_ui.h"
// --- Folded from platform/android/export/gradle_export_util.h ---
#include "core/crypto/crypto_core.h"
#include "editor/editor_ui.h"
#include "servers/display/display_server_enums.h"

const String VLTR_PROJECT_NAME_XML_STRING = R"(<?xml version="1.0" encoding="utf-8"?>
<!--WARNING: THIS FILE WILL BE OVERWRITTEN AT BUILD TIME-->
<resources>
	<string name="vltr_project_name_string">%s</string>
</resources>
)";

// Application category.
// See https://developer.android.com/guide/topics/manifest/application-element#appCategory for standards
static const int APP_CATEGORY_ACCESSIBILITY = 0;
static const int APP_CATEGORY_AUDIO = 1;
static const int APP_CATEGORY_GAME = 2;
static const int APP_CATEGORY_IMAGE = 3;
static const int APP_CATEGORY_MAPS = 4;
static const int APP_CATEGORY_NEWS = 5;
static const int APP_CATEGORY_PRODUCTIVITY = 6;
static const int APP_CATEGORY_SOCIAL = 7;
static const int APP_CATEGORY_VIDEO = 8;
static const int APP_CATEGORY_UNDEFINED = 9;

// Supported XR modes.
// This should match the entries in 'platform/android/java/lib/src/main/java/org/godotengine/godot/xr/XRMode.java'
static const int XR_MODE_REGULAR = 0;
static const int XR_MODE_OPENXR = 1;

struct CustomExportData {
	EditorExportPlatform::PackData pd;
	String assets_directory;
	String libs_directory;
	bool debug;
	Vector<String> libs;
};

struct MetadataInfo {
	String name;
	String value;
};

int _get_android_orientation_value(DisplayServerEnums::ScreenOrientation screen_orientation);

String _get_android_orientation_label(DisplayServerEnums::ScreenOrientation screen_orientation);

int _get_app_category_value(int category_index);

String _get_app_category_label(int category_index);

Error _store_temp_file(const String &p_simplified_path, const Vector<uint8_t> &p_data, const Vector<String> &p_enc_in_filters, const Vector<String> &p_enc_ex_filters, const Vector<uint8_t> &p_key, uint64_t p_seed, bool p_delta, Vector<uint8_t> &r_enc_data, EditorExportPlatform::SavedData &r_sd);

// Utility method used to create a directory.
Error create_directory(const String &p_dir);

// Writes p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_file_at_path(const String &p_path, const Vector<uint8_t> &p_data);

// Writes string p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_string_at_path(const String &p_path, const String &p_data);

// Implementation of EditorExportSaveFunction.
// This method will only be called as an input to export_project_files.
// It is used by the export_project_files method to save all the asset files into the gradle project.
// It's functionality mirrors that of the method save_apk_file.
// This method will be called ONLY when gradle build is enabled.
Error rename_and_store_file_in_gradle_project(const Ref<EditorExportPreset> &p_preset, void *p_userdata, const String &p_path, const Vector<uint8_t> &p_data, int p_file, int p_total, const Vector<String> &p_enc_in_filters, const Vector<String> &p_enc_ex_filters, const Vector<uint8_t> &p_key, uint64_t p_seed, bool p_delta);

String bool_to_string(bool v);

String _get_gles_tag();

String _get_screen_sizes_tag(const Ref<EditorExportPreset> &p_preset);

String _get_activity_tag(const Ref<EditorExportPlatform> &p_export_platform, const Ref<EditorExportPreset> &p_preset, bool p_debug);

String _get_application_tag(const Ref<EditorExportPlatform> &p_export_platform, const Ref<EditorExportPreset> &p_preset, bool p_has_read_write_storage_permission, bool p_debug, const Vector<MetadataInfo> &p_metadata);

// --- End platform/android/export/gradle_export_util.h ---

class ImageTexture;

// Optional environment variables for defining confidential information. If any
// of these is set, they will override the values set in the credentials file.
const String ENV_ANDROID_KEYSTORE_DEBUG_PATH = "GODOT_ANDROID_KEYSTORE_DEBUG_PATH";
const String ENV_ANDROID_KEYSTORE_DEBUG_USER = "GODOT_ANDROID_KEYSTORE_DEBUG_USER";
const String ENV_ANDROID_KEYSTORE_DEBUG_PASS = "GODOT_ANDROID_KEYSTORE_DEBUG_PASSWORD";
const String ENV_ANDROID_KEYSTORE_RELEASE_PATH = "GODOT_ANDROID_KEYSTORE_RELEASE_PATH";
const String ENV_ANDROID_KEYSTORE_RELEASE_USER = "GODOT_ANDROID_KEYSTORE_RELEASE_USER";
const String ENV_ANDROID_KEYSTORE_RELEASE_PASS = "GODOT_ANDROID_KEYSTORE_RELEASE_PASSWORD";

const String DEFAULT_ANDROID_KEYSTORE_DEBUG_USER = "androiddebugkey";
const String DEFAULT_ANDROID_KEYSTORE_DEBUG_PASSWORD = "android";

struct LauncherIcon
{
	const char* export_path;
	int dimensions = 0;
};

class AndroidEditorGradleRunner;

class EditorExportPlatformAndroid : public EditorExportPlatform
{
	Ref<ImageTexture> logo;
	Ref<ImageTexture> run_icon;

	struct Device
	{
		String id;
		String name;
		String description;
		int api_level = 0;
		String architecture;
	};

	struct APKExportData
	{
		EditorExportPlatform::PackData pd;
		zipFile apk;
		EditorProgress* ep = nullptr;
	};

	struct FeatureInfo
	{
		String name;
		bool required;
		String version;
	};

#ifndef DISABLE_DEPRECATED
	mutable Vector<PluginConfigAndroid> android_plugins;
	mutable SafeFlag android_plugins_changed;
	Mutex android_plugins_lock;
#endif // DISABLE_DEPRECATED
	String last_plugin_names;
	uint64_t last_gradle_build_time = 0;
	String last_gradle_build_dir;

#ifndef ANDROID_ENABLED
	bool use_scrcpy = false;
	Vector<Device> devices;
	SafeFlag devices_changed;
	Mutex device_lock;

	Thread check_for_changes_thread;
	SafeFlag quit_request;
	SafeFlag has_runnable_preset;

	static void _check_for_changes_poll_thread(void* ud);
	void _start_check_for_changes_poll_thread();
	void _stop_check_for_changes_poll_thread();
	void _update_preset_status();
#else  // ANDROID_ENABLED
	AndroidEditorGradleRunner* android_editor_gradle_runner = nullptr;
#endif // ANDROID_ENABLED

	String get_project_name(const Ref<EditorExportPreset>& p_preset, const String& p_name) const;

	String get_package_name(const Ref<EditorExportPreset>& p_preset, const String& p_package) const;

	String get_valid_basename(const Ref<EditorExportPreset>& p_preset) const;

	String get_assets_directory(const Ref<EditorExportPreset>& p_preset, int p_export_format) const;

	bool is_package_name_valid(const Ref<EditorExportPreset>& p_preset, const String& p_package,
		String* r_error = nullptr) const;
	bool is_project_name_valid(const Ref<EditorExportPreset>& p_preset) const;

	static bool _should_compress_asset(const String& p_path, const Vector<uint8_t>& p_data);

	static zip_fileinfo get_zip_fileinfo();

	struct ABI
	{
		String abi;
		String arch;

		bool operator==(const ABI& p_a) const { return p_a.abi == abi; }

		ABI(const String& p_abi, const String& p_arch)
		{
			abi = p_abi;
			arch = p_arch;
		}

		ABI() {}
	};

	static Vector<ABI> get_abis();

#ifndef DISABLE_DEPRECATED
	/// List the gdap files in the directory specified by the p_path parameter.
	static Vector<String> list_gdap_files(const String& p_path);

	static Vector<PluginConfigAndroid> get_plugins();

	static Vector<PluginConfigAndroid> get_enabled_plugins(
		const Ref<EditorExportPreset>& p_presets);
#endif // DISABLE_DEPRECATED

	static Error store_in_apk(APKExportData* ed, const String& p_path,
		const Vector<uint8_t>& p_data, int compression_method = Z_DEFLATED);

	static Error save_apk_so(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	static Error save_apk_file(const Ref<EditorExportPreset>& p_preset, void* p_userdata,
		const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
		const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta);

	static Error ignore_apk_file(const Ref<EditorExportPreset>& p_preset, void* p_userdata,
		const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
		const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta);

	static Error copy_gradle_so(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	bool _has_read_write_storage_permission(const Vector<String>& p_permissions);

	bool _has_manage_external_storage_permission(const Vector<String>& p_permissions);

	void _get_manifest_info(const Ref<EditorExportPreset>& p_preset, bool p_give_internet,
		Vector<String>& r_permissions, Vector<FeatureInfo>& r_features,
		Vector<MetadataInfo>& r_metadata);

	void _write_tmp_manifest(
		const Ref<EditorExportPreset>& p_preset, bool p_give_internet, bool p_debug);

	bool _is_transparency_allowed(const Ref<EditorExportPreset>& p_preset) const;

	void _fix_themes_xml(const Ref<EditorExportPreset>& p_preset);

	void _fix_manifest(
		const Ref<EditorExportPreset>& p_preset, Vector<uint8_t>& p_manifest, bool p_give_internet);

	static String _get_keystore_path(const Ref<EditorExportPreset>& p_preset, bool p_debug);

	static String _parse_string(const uint8_t* p_bytes, bool p_utf8);

	void _fix_resources(const Ref<EditorExportPreset>& p_preset, Vector<uint8_t>& r_manifest);

	void _process_launcher_icons(const String& p_file_name, const Ref<Image>& p_source_image,
		int dimension, Vector<uint8_t>& p_data);

	void load_icon_refs(const Ref<EditorExportPreset>& p_preset, Ref<Image>& icon,
		Ref<Image>& foreground, Ref<Image>& background, Ref<Image>& monochrome,
		Ref<Image>& splash_icon, Ref<Image>& splash_branding_image);

	void _copy_icons_to_gradle_project(const Ref<EditorExportPreset>& p_preset,
		const Ref<Image>& p_main_image, const Ref<Image>& p_foreground,
		const Ref<Image>& p_background, const Ref<Image>& p_monochrome,
		const Ref<Image>& p_splash_icon, const Ref<Image>& p_splash_branding_image);

	static void _create_editor_debug_keystore_if_needed();

	static Vector<ABI> get_enabled_abis(const Ref<EditorExportPreset>& p_preset);

	bool _uses_vulkan(const Ref<EditorExportPreset>& p_preset) const;

	Error _generate_sparse_pck_metadata(
		const Ref<EditorExportPreset>& p_preset, PackData& p_pack_data, Vector<uint8_t>& r_data);

protected:
	void _notification(int p_what);

public:
	typedef Error (*EditorExportSaveFunction)(void* p_userdata, const String& p_path,
		const Vector<uint8_t>& p_data, int p_file, int p_total,
		const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed);

	virtual String get_name() const override;

	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const override;

	virtual void get_export_options(List<ExportOption>* r_options) const override;

	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;

	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const override;

	virtual String get_os_name() const override;

	virtual Ref<Texture2D> get_logo() const override;

	virtual bool should_update_export_options() override;

#ifndef ANDROID_ENABLED
	virtual bool poll_export() override;

	virtual int get_options_count() const override;

	virtual bool is_option_runnable(int p_index) const override { return p_index != 0; }

	virtual String get_options_tooltip() const override;

	virtual String get_option_label(int p_index) const override;

	virtual String get_option_tooltip(int p_index) const override;

	virtual String get_device_architecture(int p_index) const override;

	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags) override;
#endif // ANDROID_ENABLED

	virtual Ref<Texture2D> get_run_icon() const override;

	static String get_adb_path();

	static String get_apksigner_path(int p_target_sdk = -1, bool p_check_executes = false);

	static String get_java_path();

	static String get_keytool_path();

	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;
	static bool has_valid_username_and_password(
		const Ref<EditorExportPreset>& p_preset, String& r_error);

	virtual List<String> get_binary_extensions(
		const Ref<EditorExportPreset>& p_preset) const override;

	String _get_deprecated_plugins_names(const Ref<EditorExportPreset>& p_preset) const;

	String _get_plugins_names(const Ref<EditorExportPreset>& p_preset) const;

	String _resolve_export_plugin_android_library_path(const String& p_android_library_path) const;

	bool _is_clean_build_required(const Ref<EditorExportPreset>& p_preset);

	void get_command_line_flags(const Ref<EditorExportPreset>& p_preset, const String& p_path,
		uint32_t& r_command_line_flags);

	Error sign_apk(const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& export_path,
		EditorProgress& ep);

	void _clear_assets_directory(const Ref<EditorExportPreset>& p_preset);

	void _remove_copied_libs(String p_gdextension_libs_path);

	static String join_list(const List<String>& p_parts, const String& p_separator);
	static String join_abis(const Vector<ABI>& p_parts, const String& p_separator, bool p_use_arch);

	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;

	Error export_project_helper(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, int export_format, bool should_sign, uint32_t p_flags);

	virtual void get_platform_features(List<String>* r_features) const override;

	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features) override;

	virtual void initialize() override;

	~EditorExportPlatformAndroid();
};


// --- End platform/android/export/export_plugin.h ---

String get_default_android_sdk_path();

void register_android_exporter_types() {}

inline String get_default_android_sdk_path()
{
#ifdef WINDOWS_ENABLED
	return OS::get_environment("LOCALAPPDATA").path_join("Android/Sdk");
#elif LINUXBSD_ENABLED
	return OS::get_environment("HOME").path_join("Android/Sdk");
#elif MACOS_ENABLED
	return OS::get_environment("HOME").path_join("Library/Android/sdk");
#else
	return String();
#endif
}



// ============================================================================
// Merged from platform/android/export/android_editor_gradle_runner.cpp
// ============================================================================

#ifdef ANDROID_ENABLED
// --- Inlined from platform/android/export/android_editor_gradle_runner.h ---
#ifdef ANDROID_ENABLED


class Dialog;
class RichTextLabel;

class AndroidEditorGradleRunner
{
	RichTextLabel* output_label = nullptr;
	Dialog* output_dialog = nullptr;

	enum State
	{
		STATE_IDLE,
		STATE_BUILDING,
		STATE_CLEANING,
	};

	State state = STATE_IDLE;

	String project_path;
	String build_path;
	String output_path;
	String export_format;
	List<String> gradle_build_args;
	List<String> gradle_copy_args;
	int64_t job_id;

	void _android_gradle_build_connect();
	void _android_gradle_build_disconnect();
	void _android_gradle_build_output(int p_type, const String& p_line);
	void _android_gradle_build_build();
	void _android_gradle_build_build_callback(int p_exit_code);
	void _android_gradle_build_copy();
	void _android_gradle_build_copy_callback(int p_exit_code);
	void _android_gradle_build_clean_project(bool p_was_successful);
	void _android_gradle_build_clean_project_callback();

	void _android_gradle_build_failed(const String& p_msg = String());
	void _android_gradle_build_cancel();

public:
	mem_unique_ptr<Object> obj;
	void run_gradle(const String& p_project_path, const String& p_build_path,
		const String& p_output_path, const String& p_export_format,
		const List<String>& p_gradle_build_args, const List<String>& p_gradle_copy_args);
};

#endif // ANDROID_ENABLED



// --- End platform/android/export/android_editor_gradle_runner.h ---

#include "platform/android/os_android.h"
#include "../os_android.h"

#include "editor/settings/editor_settings.h"
#include "scene/gui/dialogs.h"

void AndroidEditorGradleRunner::run_gradle(const String &p_project_path, const String &p_build_path, const String &p_output_path, const String &p_export_format, const List<String> &p_gradle_build_args, const List<String> &p_gradle_copy_args) {
	project_path = p_project_path;
	build_path = p_build_path;
	output_path = p_output_path;
	export_format = p_export_format;
	gradle_build_args = p_gradle_build_args;
	gradle_copy_args = p_gradle_copy_args;

	if (output_dialog == nullptr) {
		output_label = memnew(RichTextLabel);
		output_label->set_selection_enabled(true);
		output_label->set_context_menu_enabled(true);
		output_label->set_scroll_follow(true);

		output_dialog = memnew(Dialog(true));
		output_dialog->set_unparent_when_invisible(true);
		output_dialog->set_title(TTR("Building Android Project (gradle)"));
		output_dialog->add_child(output_label);

		output_dialog->connect("canceled", callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_cancel));
	}

	output_label->clear();
	output_dialog->get_ok_button()->set_disabled(true);
	output_dialog->popup_centered_ratio();

	state = STATE_BUILDING;
	_android_gradle_build_connect();
}

void AndroidEditorGradleRunner::_android_gradle_build_connect() {
	_android_gradle_build_output(0, TTR("> Connecting to Gradle Build Environment..."));

	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	if (!godot_java->build_env_connect(callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_build))) {
		_android_gradle_build_failed(TTR("Unable to connect to Gradle Build Environment service"));
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_disconnect() {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	godot_java->build_env_disconnect();
}

void AndroidEditorGradleRunner::_android_gradle_build_output(int p_type, const String &p_line) {
	if (p_type == 0) {
		print_line(p_line);
		output_label->append_text("[color=green]" + p_line + "[/color]\n");
	} else if (p_type == 1) {
		print_line(p_line);
		output_label->add_text(p_line + "\n");
	} else {
		print_error(p_line);
		output_label->append_text("[color=red]" + p_line + "[/color]\n");
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_build() {
	_android_gradle_build_output(0, TTR("> Starting Gradle build..."));

	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	job_id = godot_java->build_env_execute(
			"gradle",
			gradle_build_args,
			project_path,
			build_path,
			callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_output),
			callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_build_callback));
	if (job_id < 0) {
		_android_gradle_build_failed(TTR("Failed to execute Gradle command"));
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_build_callback(int p_exit_code) {
	job_id = -1;
	if (p_exit_code != 0) {
		_android_gradle_build_failed();
		return;
	}

	_android_gradle_build_copy();
}

void AndroidEditorGradleRunner::_android_gradle_build_copy() {
	_android_gradle_build_output(0, TTR("> Copying Gradle artifacts..."));

	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	job_id = godot_java->build_env_execute(
			"gradle",
			gradle_copy_args,
			project_path,
			build_path,
			callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_output),
			callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_copy_callback));
	if (job_id < 0) {
		_android_gradle_build_failed(TTR("Failed to execute Gradle command"));
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_copy_callback(int p_exit_code) {
	job_id = -1;
	if (p_exit_code != 0) {
		_android_gradle_build_failed();
	} else {
		_android_gradle_build_clean_project(true);
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_clean_project(bool p_was_successful) {
	if (state != STATE_CLEANING) {
		state = STATE_CLEANING;

		if (p_was_successful) {
			output_dialog->hide();

			bool prompt_apk_install = EDITOR_GET("export/android/install_exported_apk");
			if (prompt_apk_install && export_format == "apk") {
				OS_Android::shell_open(output_path);
			}
		} else {
			output_dialog->get_ok_button()->set_disabled(false);
		}

		GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
		godot_java->build_env_clean_project(
				project_path,
				build_path,
				callable_mp(this, &AndroidEditorGradleRunner::_android_gradle_build_clean_project_callback));
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_clean_project_callback() {
	// Ensure we haven't switched back to STATE_BUILDING in the meantime.
	if (state == STATE_CLEANING) {
		_android_gradle_build_disconnect();
		state = STATE_IDLE;
	}
}

void AndroidEditorGradleRunner::_android_gradle_build_failed(const String &p_msg) {
	job_id = -1;

	if (p_msg != "") {
		_android_gradle_build_output(1, p_msg);
	}

	_android_gradle_build_clean_project(false);
}

void AndroidEditorGradleRunner::_android_gradle_build_cancel() {
	if (job_id > 0) {
		GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
		godot_java->build_env_cancel(job_id);
		_android_gradle_build_clean_project(false);
	}
}

#endif // ANDROID_ENABLED

// ============================================================================
// Merged from platform/android/export/export_plugin.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/config_file.h"
#include "core/io/marshalls.h"
#include "core/math/random_pcg.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "core/io/resource_loader.h"
#include "platform/android/export/export_icons_svg.gen.h"
#include "scene/resources/image_texture.h"

#ifdef MODULE_MONO_ENABLED
#include "modules/mono/utils/path_utils.h"
#endif

#ifdef ANDROID_ENABLED
#include "../os_android.h"

#endif

#ifndef ANDROID_ENABLED
#include "editor/editor_ui.h"
#endif

static String _get_android_build_directory(const Ref<EditorExportPreset>& p_preset)
{
	(void)p_preset;
	return String();
}

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
		EditorExport::get_runnable_preset_for_platform(this).is_valid();
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
	String gradle_build_directory = _get_android_build_directory(p_preset);
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
	OS::DateTime dt = OS::get_datetime();

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
		ProjectSettings::get_resource_path().path_join("android/plugins");

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
		EditorExport::get_export_plugins();
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
	String manifest_path = _get_android_build_directory(p_preset).path_join(
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

	Vector<uint8_t> buffer = working_image->save_webp_to_buffer();
	p_data.resize(buffer.size());
	memcpy(p_data.ptrw(), buffer.ptr(), p_data.size());
}

void EditorExportPlatformAndroid::_copy_icons_to_gradle_project(
	const Ref<EditorExportPreset>& p_preset, const Ref<Image>& p_main_image,
	const Ref<Image>& p_foreground, const Ref<Image>& p_background, const Ref<Image>& p_monochrome,
	const Ref<Image>& p_splash_icon, const Ref<Image>& p_splash_branding_image)
{
	String gradle_build_dir = _get_android_build_directory(p_preset);

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
	Error error = OS::execute(keytool_path, args, &output, nullptr, true);
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
		ProjectSettings::globalize_path("res://" + assembly_name + ".csproj");

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
		Error err = OS::execute("dotnet", args, &pipe, &exitcode, true);
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
	String gradle_build_directory = _get_android_build_directory(p_preset);

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
				ProjectSettings::globalize_path(p_android_library_path);
		}
		else {
			const String export_plugin_absolute_path =
				String("res://addons/").path_join(p_android_library_path);
			absolute_path =
				ProjectSettings::globalize_path(export_plugin_absolute_path);
		}
	}
	return absolute_path;
}

bool EditorExportPlatformAndroid::_is_clean_build_required(const Ref<EditorExportPreset>& p_preset)
{
	bool first_build = last_gradle_build_time == 0;
	bool have_plugins_changed = false;
	String gradle_build_dir = _get_android_build_directory(p_preset);
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

	last_gradle_build_time = OS::get_unix_time();
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

Vector<PluginConfigAndroid> EditorExportPlatformAndroid::get_enabled_plugins(
	const Ref<EditorExportPreset>& p_presets)
{
	return Vector<PluginConfigAndroid>();
}

void EditorExportPlatformAndroid::_check_for_changes_poll_thread(void* ud) {}

String EditorExportPlatformAndroid::get_valid_basename(
	const Ref<EditorExportPreset>& p_preset) const
{
	return String();
}

String EditorExportPlatformAndroid::_get_plugins_names(
	const Ref<EditorExportPreset>& p_preset) const
{
	return String();
}

void EditorExportPlatformAndroid::_get_manifest_info(const Ref<EditorExportPreset>& p_preset,
	bool p_give_internet, Vector<String>& r_permissions, Vector<FeatureInfo>& r_features,
	Vector<MetadataInfo>& r_metadata)
{
}

String _get_screen_sizes_tag(const Ref<EditorExportPreset>& p_preset) { return String(); }

String _get_application_tag(const Ref<EditorExportPlatform>& p_platform,
	const Ref<EditorExportPreset>& p_preset, bool p_is_in_front, bool p_is_engine,
	const Vector<MetadataInfo>& p_metadata)
{
	return String();
}

void register_android_exporter() {}

PluginConfigAndroid PluginConfigAndroid::load_plugin_config(
	Ref<ConfigFile> p_config_file, const String& p_path)
{
	return PluginConfigAndroid();
}

Error EditorExportPlatformAndroid::run(
	const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags)
{
	return OK;
}

void EditorExportPlatformAndroid::initialize() {}

bool EditorExportPlatformAndroid::has_valid_project_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error) const
{
	return false;
}

bool EditorExportPlatformAndroid::has_valid_export_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error, bool& r_missing_templates,
	bool p_debug) const
{
	return false;
}

void EditorExportPlatformAndroid::get_preset_features(
	const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const
{
}

String EditorExportPlatformAndroid::get_export_option_warning(
	const EditorExportPreset* p_preset, const String& p_name) const
{
	return String();
}

bool EditorExportPlatformAndroid::get_export_option_visibility(
	const EditorExportPreset* p_preset, const String& p_option) const
{
	return false;
}

void EditorExportPlatformAndroid::get_export_options(List<ExportOption>* r_options) const {}

List<String> EditorExportPlatformAndroid::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	return List<String>();
}

Error EditorExportPlatformAndroid::export_project(const Ref<EditorExportPreset>& p_preset,
	bool p_debug, const String& p_path, uint32_t p_flags, bool p_notify)
{
	return OK;
}



// ============================================================================
// Merged from platform/android/export/godot_plugin_config.cpp
// ============================================================================


#ifndef DISABLE_DEPRECATED

#include "core/config/project_settings.h"

/*
 * Set of prebuilt plugins.
 * Currently unused, this is just for future reference:
 */
// static const PluginConfigAndroid MY_PREBUILT_PLUGIN = {
//    /*.valid_config =*/true,
//    /*.last_updated =*/0,
//    /*.name =*/"GodotPayment",
//    /*.binary_type =*/"local",
//    /*.binary =*/"res://android/build/libs/plugins/GodotPayment.release.aar",
//    /*.local_dependencies =*/{},
//     /*.remote_dependencies =*/String("com.android.billingclient:billing:2.2.1").split("|"),
//     /*.custom_maven_repos =*/{}
// };

String PluginConfigAndroid::resolve_local_dependency_path(
	String plugin_config_dir, String dependency_path)
{
	String absolute_path;
	if (!dependency_path.is_empty()) {
		if (dependency_path.is_absolute_path()) {
			absolute_path = ProjectSettings::globalize_path(dependency_path);
		}
		else {
			absolute_path = plugin_config_dir.path_join(dependency_path);
		}
	}

	return absolute_path;
}

PluginConfigAndroid PluginConfigAndroid::resolve_prebuilt_plugin(
	PluginConfigAndroid prebuilt_plugin, String plugin_config_dir)
{
	PluginConfigAndroid resolved = prebuilt_plugin;
	resolved.binary = resolved.binary_type == PluginConfigAndroid::BINARY_TYPE_LOCAL
						  ? resolve_local_dependency_path(plugin_config_dir, prebuilt_plugin.binary)
						  : prebuilt_plugin.binary;
	if (!prebuilt_plugin.local_dependencies.is_empty()) {
		resolved.local_dependencies.clear();
		for (int i = 0; i < prebuilt_plugin.local_dependencies.size(); i++) {
			resolved.local_dependencies.push_back(resolve_local_dependency_path(
				plugin_config_dir, prebuilt_plugin.local_dependencies[i]));
		}
	}
	return resolved;
}

Vector<PluginConfigAndroid> PluginConfigAndroid::get_prebuilt_plugins(String plugins_base_dir)
{
	Vector<PluginConfigAndroid> prebuilt_plugins;
	return prebuilt_plugins;
}

bool PluginConfigAndroid::is_plugin_config_valid(PluginConfigAndroid plugin_config)
{
	bool valid_name = !plugin_config.name.is_empty();
	bool valid_binary_type = plugin_config.binary_type == PluginConfigAndroid::BINARY_TYPE_LOCAL ||
							 plugin_config.binary_type == PluginConfigAndroid::BINARY_TYPE_REMOTE;

	bool valid_binary = false;
	if (valid_binary_type) {
		valid_binary = !plugin_config.binary.is_empty() &&
					   (plugin_config.binary_type == PluginConfigAndroid::BINARY_TYPE_REMOTE ||
						   FileAccess::exists(plugin_config.binary));
	}

	bool valid_local_dependencies = true;
	if (!plugin_config.local_dependencies.is_empty()) {
		for (int i = 0; i < plugin_config.local_dependencies.size(); i++) {
			if (!FileAccess::exists(plugin_config.local_dependencies[i])) {
				valid_local_dependencies = false;
				break;
			}
		}
	}
	return valid_name && valid_binary && valid_binary_type && valid_local_dependencies;
}

uint64_t PluginConfigAndroid::get_plugin_modification_time(
	const PluginConfigAndroid& plugin_config, const String& config_path)
{
	uint64_t last_updated = FileAccess::get_modified_time(config_path);
	last_updated = MAX(last_updated, FileAccess::get_modified_time(plugin_config.binary));

	for (int i = 0; i < plugin_config.local_dependencies.size(); i++) {
		String binary = plugin_config.local_dependencies.get(i);
		last_updated = MAX(last_updated, FileAccess::get_modified_time(binary));
	}

	return last_updated;
}

void PluginConfigAndroid::get_plugins_binaries(
	String binary_type, Vector<PluginConfigAndroid> plugins_configs, Vector<String>& r_result)
{
	if (!plugins_configs.is_empty()) {
		for (int i = 0; i < plugins_configs.size(); i++) {
			PluginConfigAndroid config = plugins_configs[i];
			if (!config.valid_config) {
				continue;
			}

			if (config.binary_type == binary_type) {
				r_result.push_back(config.binary);
			}

			if (binary_type == PluginConfigAndroid::BINARY_TYPE_LOCAL) {
				r_result.append_array(config.local_dependencies);
			}

			if (binary_type == PluginConfigAndroid::BINARY_TYPE_REMOTE) {
				r_result.append_array(config.remote_dependencies);
			}
		}
	}
}

void PluginConfigAndroid::get_plugins_custom_maven_repos(
	Vector<PluginConfigAndroid> plugins_configs, Vector<String>& r_result)
{
	if (!plugins_configs.is_empty()) {
		for (int i = 0; i < plugins_configs.size(); i++) {
			PluginConfigAndroid config = plugins_configs[i];
			if (!config.valid_config) {
				continue;
			}

			r_result.append_array(config.custom_maven_repos);
		}
	}
}

void PluginConfigAndroid::get_plugins_names(
	Vector<PluginConfigAndroid> plugins_configs, Vector<String>& r_result)
{
	if (!plugins_configs.is_empty()) {
		for (int i = 0; i < plugins_configs.size(); i++) {
			PluginConfigAndroid config = plugins_configs[i];
			if (!config.valid_config) {
				continue;
			}

			r_result.push_back(config.name);
		}
	}
}

#endif // DISABLE_DEPRECATED



// ============================================================================
// Merged from platform/android/export/gradle_export_util.cpp
// ============================================================================

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/editor_ui.h"

#include "modules/regex/regex.h"

int _get_android_orientation_value(DisplayServerEnums::ScreenOrientation screen_orientation)
{
	switch (screen_orientation) {
	case DisplayServerEnums::SCREEN_PORTRAIT:
		return 1;
	case DisplayServerEnums::SCREEN_REVERSE_LANDSCAPE:
		return 8;
	case DisplayServerEnums::SCREEN_REVERSE_PORTRAIT:
		return 9;
	case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
		return 11;
	case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
		return 12;
	case DisplayServerEnums::SCREEN_SENSOR:
		return 13;
	case DisplayServerEnums::SCREEN_LANDSCAPE:
	default:
		return 0;
	}
}

String _get_android_orientation_label(DisplayServerEnums::ScreenOrientation screen_orientation)
{
	switch (screen_orientation) {
	case DisplayServerEnums::SCREEN_PORTRAIT:
		return "portrait";
	case DisplayServerEnums::SCREEN_REVERSE_LANDSCAPE:
		return "reverseLandscape";
	case DisplayServerEnums::SCREEN_REVERSE_PORTRAIT:
		return "reversePortrait";
	case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
		return "userLandscape";
	case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
		return "userPortrait";
	case DisplayServerEnums::SCREEN_SENSOR:
		return "fullUser";
	case DisplayServerEnums::SCREEN_LANDSCAPE:
	default:
		return "landscape";
	}
}

int _get_app_category_value(int category_index)
{
	switch (category_index) {
	case APP_CATEGORY_ACCESSIBILITY:
		return 8;
	case APP_CATEGORY_AUDIO:
		return 1;
	case APP_CATEGORY_IMAGE:
		return 3;
	case APP_CATEGORY_MAPS:
		return 6;
	case APP_CATEGORY_NEWS:
		return 5;
	case APP_CATEGORY_PRODUCTIVITY:
		return 7;
	case APP_CATEGORY_SOCIAL:
		return 4;
	case APP_CATEGORY_UNDEFINED:
		return -1;
	case APP_CATEGORY_VIDEO:
		return 2;
	case APP_CATEGORY_GAME:
	default:
		return 0;
	}
}

String _get_app_category_label(int category_index)
{
	switch (category_index) {
	case APP_CATEGORY_ACCESSIBILITY:
		return "accessibility";
	case APP_CATEGORY_AUDIO:
		return "audio";
	case APP_CATEGORY_IMAGE:
		return "image";
	case APP_CATEGORY_MAPS:
		return "maps";
	case APP_CATEGORY_NEWS:
		return "news";
	case APP_CATEGORY_PRODUCTIVITY:
		return "productivity";
	case APP_CATEGORY_SOCIAL:
		return "social";
	case APP_CATEGORY_VIDEO:
		return "video";
	case APP_CATEGORY_GAME:
	default:
		return "game";
	}
}

// Utility method used to create a directory.
Error create_directory(const String& p_dir)
{
	if (!DirAccess::exists(p_dir)) {
		Ref<DirAccess> filesystem_da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		ERR_FAIL_COND_V_MSG(
			filesystem_da.is_null(), ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
		Error err = filesystem_da->make_dir_recursive(p_dir);
		ERR_FAIL_COND_V_MSG(err, ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
	}
	return OK;
}

// Writes p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_file_at_path(const String& p_path, const Vector<uint8_t>& p_data)
{
	String dir = p_path.get_base_dir();
	Error err = create_directory(dir);
	if (err != OK) {
		return err;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_path + "'.");
	fa->store_buffer(p_data.ptr(), p_data.size());
	return OK;
}

// Writes string p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_string_at_path(const String& p_path, const String& p_data)
{
	String dir = p_path.get_base_dir();
	Error err = create_directory(dir);
	if (err != OK) {
		if (OS::is_stdout_verbose()) {
			print_error("Unable to write data into " + p_path);
		}
		return err;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_path + "'.");
	fa->store_string(p_data);
	return OK;
}

// Implementation of EditorExportSaveFunction.
// This method will only be called as an input to export_project_files.
// It is used by the export_project_files method to save all the asset files into the gradle
// project. It's functionality mirrors that of the method save_apk_file. This method will be called
// ONLY when gradle build is enabled.
Error rename_and_store_file_in_gradle_project(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	CustomExportData* export_data = static_cast<CustomExportData*>(p_userdata);

	const String simplified_path = EditorExportPlatform::simplify_path(p_path);

	Vector<uint8_t> enc_data;
	EditorExportPlatform::SavedData sd;
	Error err = _store_temp_file(simplified_path, p_data, p_enc_in_filters, p_enc_ex_filters, p_key,
		p_seed, p_delta, enc_data, sd);
	if (err != OK) {
		return err;
	}

	String dst_path;
	if (export_data->pd.salt.length() == 32) {
		dst_path = export_data->assets_directory + String("/") +
				   (simplified_path + export_data->pd.salt).sha256_text();
	}
	else {
		dst_path =
			export_data->assets_directory + String("/") + simplified_path.trim_prefix("res://");
	}
	print_verbose("Saving project files from " + simplified_path + " into " + dst_path);
	err = store_file_at_path(dst_path, enc_data);

	export_data->pd.file_ofs.push_back(sd);
	return err;
}

String _android_xml_escape(const String& p_string)
{
	// Android XML requires strings to be both valid XML (`xml_escape()`) but also
	// to escape characters which are valid XML but have special meaning in Android XML.
	// https://developer.android.com/guide/topics/resources/string-resource.html#FormattingAndStyling
	// Note: Didn't handle U+XXXX unicode chars, could be done if needed.
	return p_string.replace("@", "\\@")
		.replace("?", "\\?")
		.replace("'", "\\'")
		.replace("\"", "\\\"")
		.replace("\n", "\\n")
		.replace("\t", "\\t")
		.xml_escape(false);
}

String bool_to_string(bool v) { return v ? "true" : "false"; }

String _get_gles_tag()
{
	return "    <uses-feature android:glEsVersion=\"0x00030000\" android:required=\"true\" />\n";
}

Error _store_temp_file(const String& p_simplified_path, const Vector<uint8_t>& p_data,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta, Vector<uint8_t>& r_enc_data,
	EditorExportPlatform::SavedData& r_sd)
{
	Error err = OK;
	Ref<FileAccess> ftmp =
		FileAccess::create_temp(FileAccess::WRITE_READ, "export", "tmp", false, &err);
	if (err != OK) {
		return err;
	}
	r_sd.path_utf8 = p_simplified_path.trim_prefix("res://").utf8();
	r_sd.ofs = 0;
	r_sd.size = p_data.size();
	r_sd.delta = p_delta;
	err = EditorExportPlatform::_encrypt_and_store_data(ftmp, p_simplified_path, p_data,
		p_enc_in_filters, p_enc_ex_filters, p_key, p_seed, r_sd.encrypted);
	if (err != OK) {
		return err;
	}

	r_enc_data.resize(ftmp->get_length());
	ftmp->seek(0);
	ftmp->get_buffer(r_enc_data.ptrw(), r_enc_data.size());
	ftmp.unref();

	// Store MD5 of original file.
	{
		unsigned char hash[16];
		CryptoCore::md5(p_data.ptr(), p_data.size(), hash);
		r_sd.md5.resize(16);
		for (int i = 0; i < 16; i++) {
			r_sd.md5.write[i] = hash[i];
		}
	}
	return OK;
}


// --- Merged from platform/ios/export/export.cpp ---
#include "editor/editor_ui.h"
// --- Inlined from platform/ios/export/export_plugin.h ---
#include "editor/editor_ui.h"

class EditorExportPlatformIOS : public EditorExportPlatformAppleEmbedded
{
	static Vector<String> device_types;

	virtual String get_platform_name() const override { return "ios"; }

	virtual String get_sdk_name() const override { return "iphoneos"; }

	virtual const Vector<String> get_device_types() const override { return device_types; }

	virtual String get_minimum_deployment_target() const override { return "15.0"; }

	virtual Vector<IconInfo> get_icon_infos() const override;

	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;

	virtual Error _export_loading_screen_file(
		const Ref<EditorExportPreset>& p_preset, const String& p_dest_dir) override;
	virtual Error _export_icons(
		const Ref<EditorExportPreset>& p_preset, const String& p_iconset_dir) override;

	virtual String _process_config_file_line(const Ref<EditorExportPreset>& p_preset,
		const String& p_line, const AppleEmbeddedConfigData& p_config, bool p_debug,
		const CodeSigningDetails& p_code_signing) override;

public:
	virtual String get_name() const override { return "iOS"; }

	virtual String get_os_name() const override { return "iOS"; }

	virtual void get_platform_features(List<String>* r_features) const override
	{
		EditorExportPlatformAppleEmbedded::get_platform_features(r_features);
		r_features->push_back("ios");
	}

	virtual void initialize() override;
	~EditorExportPlatformIOS();
};



// --- End platform/ios/export/export_plugin.h ---

#ifdef MACOS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

void register_ios_exporter_types() {}

void register_ios_exporter()
{
	Ref<EditorExportPlatformIOS> platform;
	platform.instantiate();

	EditorExport::add_export_platform(platform);
}



// ============================================================================
// Merged from platform/ios/export/export_plugin.cpp
// ============================================================================

#include "core/config/engine.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "main/splash.gen.h"
#include "platform/android/export/export_icons_svg.gen.h"

Vector<String> EditorExportPlatformIOS::device_types({"iPhone", "iPad"});

void EditorExportPlatformIOS::initialize()
{
	if (Engine::is_editor_hint()) {
		EditorExportPlatformAppleEmbedded::_initialize(_ios_logo_svg, _ios_run_icon_svg);
#ifdef MACOS_ENABLED
		_start_remote_device_poller_thread();
#endif
	}
}

EditorExportPlatformIOS::~EditorExportPlatformIOS()
{
#ifdef MACOS_ENABLED
	_stop_remote_device_poller_thread();
#endif
}

Vector<EditorExportPlatformAppleEmbedded::IconInfo> EditorExportPlatformIOS::get_icon_infos() const
{
	return {
		// Settings on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/settings_58x58"), "universal", "Icon-58", "58", "2x", "29x29", false},
		{PNAME("icons/settings_87x87"), "universal", "Icon-87", "87", "3x", "29x29", false},

		// Notifications on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/notification_40x40"), "universal", "Icon-40", "40", "2x", "20x20", false},
		{PNAME("icons/notification_60x60"), "universal", "Icon-60", "60", "3x", "20x20", false},
		{PNAME("icons/notification_76x76"), "universal", "Icon-76", "76", "2x", "38x38", false},
		{PNAME("icons/notification_114x114"), "universal", "Icon-114", "114", "3x", "38x38", false},

		// Spotlight on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/spotlight_80x80"), "universal", "Icon-80", "80", "2x", "40x40", false},
		{PNAME("icons/spotlight_120x120"), "universal", "Icon-120", "120", "3x", "40x40", false},

		// Home Screen on iPhone
		{PNAME("icons/iphone_120x120"), "universal", "Icon-120-1", "120", "2x", "60x60", false},
		{PNAME("icons/iphone_180x180"), "universal", "Icon-180", "180", "3x", "60x60", false},

		// Home Screen on iPad Pro
		{PNAME("icons/ipad_167x167"), "universal", "Icon-167", "167", "2x", "83.5x83.5", false},

		// Home Screen on iPad, iPad mini
		{PNAME("icons/ipad_152x152"), "universal", "Icon-152", "152", "2x", "76x76", false},

		{PNAME("icons/ios_128x128"), "universal", "Icon-128", "128", "2x", "64x64", false},
		{PNAME("icons/ios_192x192"), "universal", "Icon-192", "192", "3x", "64x64", false},

		{PNAME("icons/ios_136x136"), "universal", "Icon-136", "136", "2x", "68x68", false},

		// App Store
		{PNAME("icons/app_store_1024x1024"), "universal", "Icon-1024", "1024", "1x", "1024x1024",
			true},
	};
}

void EditorExportPlatformIOS::get_export_options(
	List<EditorExportPlatform::ExportOption>* r_options) const
{
}

bool EditorExportPlatformIOS::has_valid_export_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error, bool& r_missing_templates,
	bool p_debug) const
{
	return false;
}

String EditorExportPlatformIOS::_process_config_file_line(const Ref<EditorExportPreset>& p_preset,
	const String& p_line,
	const EditorExportPlatformAppleEmbedded::AppleEmbeddedConfigData& p_config_data, bool p_is_main,
	const EditorExportPlatformAppleEmbedded::CodeSigningDetails& p_signing_details)
{
	return String();
}

Error EditorExportPlatformIOS::_export_loading_screen_file(
	const Ref<EditorExportPreset>& p_preset, const String& p_dest_dir)
{
	return OK;
}

Error EditorExportPlatformIOS::_export_icons(
	const Ref<EditorExportPreset>& p_preset, const String& p_dest_dir)
{
	return OK;
}


// --- Merged from platform/linuxbsd/export/export.cpp ---
#include "editor/editor_ui.h"
// --- Inlined from platform/linuxbsd/export/export_plugin.h ---
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "scene/resources/image_texture.h"

class EditorExportPlatformLinuxBSD : public EditorExportPlatformPC
{
	HashMap<String, String> extensions;

	struct SSHCleanupCommand
	{
		String host;
		String port;
		Vector<String> ssh_args;
		String cmd_args;
		bool wait = false;

		SSHCleanupCommand() {}

		SSHCleanupCommand(const String& p_host, const String& p_port,
			const Vector<String>& p_ssh_arg, const String& p_cmd_args, bool p_wait = false)
		{
			host = p_host;
			port = p_port;
			ssh_args = p_ssh_arg;
			cmd_args = p_cmd_args;
			wait = p_wait;
		}
	};

	Ref<ImageTexture> run_icon;
	Ref<ImageTexture> stop_icon;

	Vector<SSHCleanupCommand> cleanup_commands;
	ProcessID ssh_pid = 0;
	int menu_options = 0;

	bool is_elf(const String& p_path) const;
	bool is_shebang(const String& p_path) const;

	Error _export_debug_script(const Ref<EditorExportPreset>& p_preset, const String& p_app_name,
		const String& p_pkg_name, const String& p_path);
	String _get_exe_arch(const String& p_path) const;

public:
	virtual String get_template_file_name(
		const String& p_target, const String& p_arch) const override;
	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual List<String> get_binary_extensions(
		const Ref<EditorExportPreset>& p_preset) const override;
	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;
	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;
	virtual Error fixup_embedded_pck(
		const String& p_path, int64_t p_embedded_start, int64_t p_embedded_size) override;
	virtual bool is_executable(const String& p_path) const override;

	virtual void get_platform_features(List<String>* r_features) const override;

	virtual Ref<Texture2D> get_run_icon() const override;
	virtual bool poll_export() override;
	virtual int get_options_count() const override;
	virtual String get_option_label(int p_index) const override;
	virtual String get_option_tooltip(int p_index) const override;
	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags) override;
	virtual void cleanup() override;
};



// --- End platform/linuxbsd/export/export_plugin.h ---

void register_linuxbsd_exporter_types() {}

void register_linuxbsd_exporter()
{
	Ref<EditorExportPlatformLinuxBSD> platform;
	platform.instantiate();
	platform->set_name("Linux");
	platform->set_os_name("Linux");
	platform->set_chmod_flags(0755);

	EditorExport::add_export_platform(platform);
}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/export/export_plugin.cpp
// ============================================================================

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "core/io/resource_loader.h"
#include "platform/android/export/export_icons_svg.gen.h"

Error EditorExportPlatformLinuxBSD::_export_debug_script(const Ref<EditorExportPreset>& p_preset,
	const String& p_app_name, const String& p_pkg_name, const String& p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Debug Script Export"),
			vformat(TTR("Could not open file \"%s\"."), p_path));
		return ERR_CANT_CREATE;
	}

	f->store_line("#!/bin/sh");
	f->store_line("printf '\\033c\\033]0;%s\\a' " + p_app_name);
	f->store_line("base_path=\"$(dirname \"$(realpath \"$0\")\")\"");
	f->store_line("\"$base_path/" + p_pkg_name + "\" \"$@\"");

	return OK;
}

String EditorExportPlatformLinuxBSD::get_template_file_name(
	const String& p_target, const String& p_arch) const
{
	return "linux_" + p_target + "." + p_arch;
}

bool EditorExportPlatformLinuxBSD::is_elf(const String& p_path) const
{
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("Can't open file: \"%s\".", p_path));
	uint32_t magic = fb->get_32();
	return (magic == 0x464c457f);
}

bool EditorExportPlatformLinuxBSD::is_shebang(const String& p_path) const
{
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("Can't open file: \"%s\".", p_path));
	uint16_t magic = fb->get_16();
	return (magic == 0x2123);
}

bool EditorExportPlatformLinuxBSD::is_executable(const String& p_path) const
{
	return is_elf(p_path) || is_shebang(p_path);
}

void EditorExportPlatformLinuxBSD::get_platform_features(List<String>* r_features) const
{
	EditorExportPlatformPC::get_platform_features(r_features);
	r_features->push_back("linux");
	r_features->push_back("linuxbsd");
}

String EditorExportPlatformLinuxBSD::_get_exe_arch(const String& p_path) const
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return "invalid";
	}

	// Read and check ELF magic number.
	{
		uint32_t magic = f->get_32();
		if (magic != 0x464c457f) { // 0x7F + "ELF"
			return "invalid";
		}
	}

	// Process header.
	int64_t header_pos = f->get_position();
	f->seek(header_pos + 14);
	uint16_t machine = f->get_16();
	f->close();

	switch (machine) {
	case 0x0003:
		return "x86_32";
	case 0x003e:
		return "x86_64";
	case 0x0015:
		return "ppc64";
	case 0x0028:
		return "arm32";
	case 0x00b7:
		return "arm64";
	case 0x00f3:
		return "rv64";
	case 0x0102:
		return "loongarch64";
	default:
		return "unknown";
	}
}

Error EditorExportPlatformLinuxBSD::fixup_embedded_pck(
	const String& p_path, int64_t p_embedded_start, int64_t p_embedded_size)
{
	// Patch the header of the "pck" section in the ELF file so that it corresponds to the embedded
	// data.

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ_WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("PCK Embedding"),
			vformat(TTR("Failed to open executable file \"%s\"."), p_path));
		return ERR_CANT_OPEN;
	}

	// Read and check ELF magic number.
	{
		uint32_t magic = f->get_32();
		if (magic != 0x464c457f) { // 0x7F + "ELF"
			add_message(EXPORT_MESSAGE_ERROR, TTR("PCK Embedding"),
				TTR("Executable file header corrupted."));
			return ERR_FILE_CORRUPT;
		}
	}

	// Read program architecture bits from class field.

	int bits = f->get_8() * 32;

	if (bits == 32 && p_embedded_size >= 0x100000000) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("PCK Embedding"),
			TTR("32-bit executables cannot have embedded data >= 4 GiB."));
	}

	// Get info about the section header table.

	int64_t section_table_pos;
	int64_t section_header_size;
	if (bits == 32) {
		section_header_size = 40;
		f->seek(0x20);
		section_table_pos = f->get_32();
		f->seek(0x30);
	}
	else { // 64
		section_header_size = 64;
		f->seek(0x28);
		section_table_pos = f->get_64();
		f->seek(0x3c);
	}
	int num_sections = f->get_16();
	int string_section_idx = f->get_16();

	// Load the strings table.
	uint8_t* strings;
	{
		// Jump to the strings section header.
		f->seek(section_table_pos + string_section_idx * section_header_size);

		// Read strings data size and offset.
		int64_t string_data_pos;
		int64_t string_data_size;
		if (bits == 32) {
			f->seek(f->get_position() + 0x10);
			string_data_pos = f->get_32();
			string_data_size = f->get_32();
		}
		else { // 64
			f->seek(f->get_position() + 0x18);
			string_data_pos = f->get_64();
			string_data_size = f->get_64();
		}

		// Read strings data.
		f->seek(string_data_pos);
		strings = (uint8_t*)memalloc(string_data_size);
		if (!strings) {
			return ERR_OUT_OF_MEMORY;
		}
		f->get_buffer(strings, string_data_size);
	}

	// Search for the "pck" section.

	bool found = false;
	for (int i = 0; i < num_sections; ++i) {
		int64_t section_header_pos = section_table_pos + i * section_header_size;
		f->seek(section_header_pos);

		uint32_t name_offset = f->get_32();
		if (strcmp((char*)strings + name_offset, "pck") == 0) {
			// "pck" section found, let's patch!

			if (bits == 32) {
				f->seek(section_header_pos + 0x10);
				f->store_32(p_embedded_start);
				f->store_32(p_embedded_size);
			}
			else { // 64
				f->seek(section_header_pos + 0x18);
				f->store_64(p_embedded_start);
				f->store_64(p_embedded_size);
			}

			found = true;
			break;
		}
	}

	memfree(strings);

	if (!found) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("PCK Embedding"),
			TTR("Executable \"pck\" section not found."));
		return ERR_FILE_CORRUPT;
	}
	return OK;
}

Ref<Texture2D> EditorExportPlatformLinuxBSD::get_run_icon() const { return run_icon; }

int EditorExportPlatformLinuxBSD::get_options_count() const { return menu_options; }

String EditorExportPlatformLinuxBSD::get_option_label(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall") : TTR("Run on remote Linux/BSD system");
}

String EditorExportPlatformLinuxBSD::get_option_tooltip(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall running project from the remote system")
					 : TTR("Run exported project on remote Linux/BSD system");
}

void EditorExportPlatformLinuxBSD::cleanup()
{
	if (ssh_pid != 0 && OS::is_process_running(ssh_pid)) {
		__print_line("Terminating connection...");
		OS::kill(ssh_pid);
		OS::delay_usec(1000);
	}

	if (!cleanup_commands.is_empty()) {
		__print_line("Stopping and deleting previous version...");
		for (const SSHCleanupCommand& cmd : cleanup_commands) {
			if (cmd.wait) {
				ssh_run_on_remote(cmd.host, cmd.port, cmd.ssh_args, cmd.cmd_args);
			}
			else {
				ssh_run_on_remote_no_wait(cmd.host, cmd.port, cmd.ssh_args, cmd.cmd_args);
			}
		}
	}
	ssh_pid = 0;
	cleanup_commands.clear();
}

Error EditorExportPlatformLinuxBSD::run(
	const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags)
{
	return OK;
}

bool EditorExportPlatformLinuxBSD::poll_export() { return true; }

bool EditorExportPlatformLinuxBSD::has_valid_export_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error, bool& r_missing_templates,
	bool p_debug) const
{
	return false;
}

bool EditorExportPlatformLinuxBSD::get_export_option_visibility(
	const EditorExportPreset* p_preset, const String& p_option) const
{
	return false;
}

void EditorExportPlatformLinuxBSD::get_export_options(List<ExportOption>* r_options) const {}

List<String> EditorExportPlatformLinuxBSD::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	return List<String>();
}

Error EditorExportPlatformLinuxBSD::export_project(const Ref<EditorExportPreset>& p_preset,
	bool p_debug, const String& p_path, uint32_t p_flags, bool p_notify)
{
	return OK;
}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif


// --- Merged from platform/macos/export/export.cpp ---
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"
#include "editor/editor_ui.h"
// --- Inlined from platform/macos/export/export_plugin.h ---
#include <sys/stat.h>
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "scene/resources/image_texture.h"

// Optional environment variables for defining confidential information. If any
// of these is set, they will override the values set in the credentials file.
const String ENV_MAC_CODESIGN_CERT_FILE = "GODOT_MACOS_CODESIGN_CERTIFICATE_FILE";
const String ENV_MAC_CODESIGN_CERT_PASS = "GODOT_MACOS_CODESIGN_CERTIFICATE_PASSWORD";
const String ENV_MAC_CODESIGN_PROFILE = "GODOT_MACOS_CODESIGN_PROVISIONING_PROFILE";
const String ENV_MAC_NOTARIZATION_UUID = "GODOT_MACOS_NOTARIZATION_API_UUID";
const String ENV_MAC_NOTARIZATION_KEY = "GODOT_MACOS_NOTARIZATION_API_KEY";
const String ENV_MAC_NOTARIZATION_KEY_ID = "GODOT_MACOS_NOTARIZATION_API_KEY_ID";
const String ENV_MAC_NOTARIZATION_APPLE_ID = "GODOT_MACOS_NOTARIZATION_APPLE_ID_NAME";
const String ENV_MAC_NOTARIZATION_APPLE_PASS = "GODOT_MACOS_NOTARIZATION_APPLE_ID_PASSWORD";

class EditorExportPlatformMacOS : public EditorExportPlatform
{
	int version_code = 0;

	Ref<ImageTexture> logo;

	struct SSHCleanupCommand
	{
		String host;
		String port;
		Vector<String> ssh_args;
		String cmd_args;
		bool wait = false;

		SSHCleanupCommand() {}

		SSHCleanupCommand(const String& p_host, const String& p_port,
			const Vector<String>& p_ssh_arg, const String& p_cmd_args, bool p_wait = false)
		{
			host = p_host;
			port = p_port;
			ssh_args = p_ssh_arg;
			cmd_args = p_cmd_args;
			wait = p_wait;
		}
	};

	Ref<ImageTexture> run_icon;
	Ref<ImageTexture> stop_icon;

	Vector<SSHCleanupCommand> cleanup_commands;
	ProcessID ssh_pid = 0;
	int menu_options = 0;

	void _fix_privacy_manifest(const Ref<EditorExportPreset>& p_preset, Vector<uint8_t>& plist);
	void _fix_plist(const Ref<EditorExportPreset>& p_preset, Vector<uint8_t>& plist,
		const String& p_binary, bool p_lg_icon_exported, const String& p_lg_icon);
	void _make_icon(
		const Ref<EditorExportPreset>& p_preset, const Ref<Image>& p_icon, Vector<uint8_t>& p_data);

	Error _export_liquid_glass_icon(const Ref<EditorExportPreset>& p_preset,
		const String& p_app_path, const String& p_icon_path);
	Error _notarize(const Ref<EditorExportPreset>& p_preset, const String& p_path);
	void _code_sign(const Ref<EditorExportPreset>& p_preset, const String& p_path,
		const String& p_ent_path, bool p_warn = true, bool p_set_id = false);
	void _code_sign_directory(const Ref<EditorExportPreset>& p_preset, const String& p_path,
		const String& p_ent_path, const String& p_helper_ent_path,
		bool p_should_error_on_non_code = true);
	Error _copy_and_sign_files(Ref<DirAccess>& dir_access, const String& p_src_path,
		const String& p_in_app_path, bool p_sign_enabled, const Ref<EditorExportPreset>& p_preset,
		const String& p_ent_path, const String& p_helper_ent_path,
		bool p_should_error_on_non_code_sign, bool p_sandbox);
	Error _export_macos_plugins_for(Ref<EditorExportPlugin> p_editor_export_plugin,
		const String& p_app_path_name, Ref<DirAccess>& dir_access, bool p_sign_enabled,
		const Ref<EditorExportPreset>& p_preset, const String& p_ent_path,
		const String& p_helper_ent_path, bool p_sandbox);
	Error _create_dmg(
		const String& p_dmg_path, const String& p_pkg_name, const String& p_app_path_name);
	Error _create_pkg(const Ref<EditorExportPreset>& p_preset, const String& p_pkg_path,
		const String& p_app_path_name);
	Error _export_debug_script(const Ref<EditorExportPreset>& p_preset, const String& p_app_name,
		const String& p_pkg_name, const String& p_path);

	bool use_codesign() const { return true; }

	bool is_package_name_valid(const String& p_package, String* r_error = nullptr) const
	{
		String pname = p_package;

		if (pname.length() == 0) {
			if (r_error) {
				*r_error = TTR("Identifier is missing.");
			}
			return false;
		}

		for (int i = 0; i < pname.length(); i++) {
			char32_t c = pname[i];
			if (!(is_ascii_alphanumeric_char(c) || c == '-' || c == '.')) {
				if (r_error) {
					*r_error = vformat(
						TTR("The character '%s' is not allowed in Identifier."), String::chr(c));
				}
				return false;
			}
		}

		return true;
	}

	bool is_shebang(const String& p_path) const;

protected:
	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const override;
	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;
	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const override;

public:
	virtual String get_name() const override { return "macOS"; }

	virtual String get_os_name() const override { return "macOS"; }

	virtual Ref<Texture2D> get_logo() const override { return logo; }

	virtual bool is_executable(const String& p_path) const override;
	virtual List<String> get_binary_extensions(
		const Ref<EditorExportPreset>& p_preset) const override;
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;

	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;

	virtual void get_platform_features(List<String>* r_features) const override
	{
		r_features->push_back("pc");
		r_features->push_back("s3tc");
		r_features->push_back("macos");
	}

	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features) override
	{
	}

	virtual Ref<Texture2D> get_run_icon() const override;
	virtual bool poll_export() override;
	virtual int get_options_count() const override;
	virtual String get_option_label(int p_index) const override;
	virtual String get_option_tooltip(int p_index) const override;
	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags) override;
	virtual void cleanup() override;
};



// --- End platform/macos/export/export_plugin.h ---

void register_macos_exporter_types() {}
void register_macos_exporter() {}



// ============================================================================
// Merged from platform/macos/export/export_plugin.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/marshalls.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "core/io/resource_loader.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "core/io/resource_loader.h"
#include "platform/android/export/export_icons_svg.gen.h"
#include "scene/resources/image_texture.h"

struct DataCollectionInfo
{
	String prop_name;
	String type_name;
};

static const DataCollectionInfo data_collect_type_info[] = {
	{"name", "NSPrivacyCollectedDataTypeName"},
	{"email_address", "NSPrivacyCollectedDataTypeEmailAddress"},
	{"phone_number", "NSPrivacyCollectedDataTypePhoneNumber"},
	{"physical_address", "NSPrivacyCollectedDataTypePhysicalAddress"},
	{"other_contact_info", "NSPrivacyCollectedDataTypeOtherUserContactInfo"},
	{"health", "NSPrivacyCollectedDataTypeHealth"},
	{"fitness", "NSPrivacyCollectedDataTypeFitness"},
	{"payment_info", "NSPrivacyCollectedDataTypePaymentInfo"},
	{"credit_info", "NSPrivacyCollectedDataTypeCreditInfo"},
	{"other_financial_info", "NSPrivacyCollectedDataTypeOtherFinancialInfo"},
	{"precise_location", "NSPrivacyCollectedDataTypePreciseLocation"},
	{"coarse_location", "NSPrivacyCollectedDataTypeCoarseLocation"},
	{"sensitive_info", "NSPrivacyCollectedDataTypeSensitiveInfo"},
	{"contacts", "NSPrivacyCollectedDataTypeContacts"},
	{"emails_or_text_messages", "NSPrivacyCollectedDataTypeEmailsOrTextMessages"},
	{"photos_or_videos", "NSPrivacyCollectedDataTypePhotosorVideos"},
	{"audio_data", "NSPrivacyCollectedDataTypeAudioData"},
	{"gameplay_content", "NSPrivacyCollectedDataTypeGameplayContent"},
	{"customer_support", "NSPrivacyCollectedDataTypeCustomerSupport"},
	{"other_user_content", "NSPrivacyCollectedDataTypeOtherUserContent"},
	{"browsing_history", "NSPrivacyCollectedDataTypeBrowsingHistory"},
	{"search_history", "NSPrivacyCollectedDataTypeSearchHistory"},
	{"user_id", "NSPrivacyCollectedDataTypeUserID"},
	{"device_id", "NSPrivacyCollectedDataTypeDeviceID"},
	{"purchase_history", "NSPrivacyCollectedDataTypePurchaseHistory"},
	{"product_interaction", "NSPrivacyCollectedDataTypeProductInteraction"},
	{"advertising_data", "NSPrivacyCollectedDataTypeAdvertisingData"},
	{"other_usage_data", "NSPrivacyCollectedDataTypeOtherUsageData"},
	{"crash_data", "NSPrivacyCollectedDataTypeCrashData"},
	{"performance_data", "NSPrivacyCollectedDataTypePerformanceData"},
	{"other_diagnostic_data", "NSPrivacyCollectedDataTypeOtherDiagnosticData"},
	{"environment_scanning", "NSPrivacyCollectedDataTypeEnvironmentScanning"},
	{"hands", "NSPrivacyCollectedDataTypeHands"},
	{"head", "NSPrivacyCollectedDataTypeHead"},
	{"other_data_types", "NSPrivacyCollectedDataTypeOtherDataTypes"},
};

static const DataCollectionInfo data_collect_purpose_info[] = {
	{"Analytics", "NSPrivacyCollectedDataTypePurposeAnalytics"},
	{"App Functionality", "NSPrivacyCollectedDataTypePurposeAppFunctionality"},
	{"Developer Advertising", "NSPrivacyCollectedDataTypePurposeDeveloperAdvertising"},
	{"Third-party Advertising", "NSPrivacyCollectedDataTypePurposeThirdPartyAdvertising"},
	{"Product Personalization", "NSPrivacyCollectedDataTypePurposeProductPersonalization"},
	{"Other", "NSPrivacyCollectedDataTypePurposeOther"},
};

void _rgba8_to_packbits_encode(
	int p_ch, int p_size, Vector<uint8_t>& p_source, Vector<uint8_t>& p_dest)
{
	int src_len = p_size * p_size;

	Vector<uint8_t> result;

	int i = 0;
	const uint8_t* src = p_source.ptr();
	while (i < src_len) {
		Vector<uint8_t> seq;

		uint8_t count = 0;
		while (count <= 0x7f && i < src_len) {
			if (i + 2 < src_len && src[i * 4 + p_ch] == src[(i + 1) * 4 + p_ch] &&
				src[i] == src[(i + 2) * 4 + p_ch]) {
				break;
			}
			seq.push_back(src[i * 4 + p_ch]);
			i++;
			count++;
		}
		if (!seq.is_empty()) {
			result.push_back(count - 1);
			result.append_array(seq);
		}
		if (i >= src_len) {
			break;
		}

		uint8_t rep = src[i * 4 + p_ch];
		count = 0;
		while (count <= 0x7f && i < src_len && src[i * 4 + p_ch] == rep) {
			i++;
			count++;
		}
		if (count >= 3) {
			result.push_back(0x80 + count - 3);
			result.push_back(rep);
		}
		else {
			result.push_back(count - 1);
			for (int j = 0; j < count; j++) {
				result.push_back(rep);
			}
		}
	}

	int ofs = p_dest.size();
	p_dest.resize(p_dest.size() + result.size());
	memcpy(&p_dest.write[ofs], result.ptr(), result.size());
}

/**
 * If we're running the macOS version of the Godot editor we'll:
 * - export our application bundle to a temporary folder
 * - attempt to code sign it
 * - and then wrap it up in a DMG
 */

Error EditorExportPlatformMacOS::_export_macos_plugins_for(
	Ref<EditorExportPlugin> p_editor_export_plugin, const String& p_app_path_name,
	Ref<DirAccess>& dir_access, bool p_sign_enabled, const Ref<EditorExportPreset>& p_preset,
	const String& p_ent_path, const String& p_helper_ent_path, bool p_sandbox)
{
	Error error{OK};
	const Vector<String>& macos_plugins{p_editor_export_plugin->get_macos_plugin_files()};
	for (int i = 0; i < macos_plugins.size(); ++i) {
		String src_path{ProjectSettings::globalize_path(macos_plugins[i])};
		String path_in_app{p_app_path_name + "/Contents/PlugIns/" + src_path.get_file()};
		error = _copy_and_sign_files(dir_access, src_path, path_in_app, p_sign_enabled, p_preset,
			p_ent_path, p_helper_ent_path, false, p_sandbox);
		if (error != OK) {
			break;
		}
	}
	return error;
}

Error EditorExportPlatformMacOS::_create_dmg(
	const String& p_dmg_path, const String& p_pkg_name, const String& p_app_path_name)
{
	List<String> args;

	if (FileAccess::exists(p_dmg_path)) {
		OS::move_to_trash(p_dmg_path);
	}

	args.push_back("create");
	args.push_back(p_dmg_path);
	args.push_back("-volname");
	args.push_back(p_pkg_name);
	args.push_back("-fs");
	args.push_back("HFS+");
	args.push_back("-srcfolder");
	args.push_back(p_app_path_name);

	String str;
	Error err = OS::execute("hdiutil", args, &str, nullptr, true);
	if (err != OK) {
		add_message(
			EXPORT_MESSAGE_ERROR, TTR("DMG Creation"), TTR("Could not start hdiutil executable."));
		return err;
	}

	print_verbose("hdiutil returned: " + str);
	if (str.contains("create failed")) {
		if (str.contains("File exists")) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("DMG Creation"),
				TTR("`hdiutil create` failed - file exists."));
		}
		else {
			add_message(EXPORT_MESSAGE_ERROR, TTR("DMG Creation"), TTR("`hdiutil create` failed."));
		}
		return FAILED;
	}

	return OK;
}

bool EditorExportPlatformMacOS::is_shebang(const String& p_path) const
{
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("Can't open file: \"%s\".", p_path));
	uint16_t magic = fb->get_16();
	return (magic == 0x2123);
}

bool EditorExportPlatformMacOS::is_executable(const String& p_path) const
{
	return MachO::is_macho(p_path) || LipO::is_lipo(p_path) || is_shebang(p_path);
}

Error EditorExportPlatformMacOS::_export_debug_script(const Ref<EditorExportPreset>& p_preset,
	const String& p_app_name, const String& p_pkg_name, const String& p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Debug Script Export"),
			vformat(TTR("Could not open file \"%s\"."), p_path));
		return ERR_CANT_CREATE;
	}

	f->store_line("#!/bin/sh");
	f->store_line("printf '\\033c\\033]0;%s\\a' " + p_app_name);
	f->store_line("");
	f->store_line("function app_realpath() {");
	f->store_line("    SOURCE=$1");
	f->store_line("    while [ -h \"$SOURCE\" ]; do");
	f->store_line("        DIR=$(dirname \"$SOURCE\")");
	f->store_line("        SOURCE=$(readlink \"$SOURCE\")");
	f->store_line("        [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE");
	f->store_line("    done");
	f->store_line("    echo \"$( cd -P \"$( dirname \"$SOURCE\" )\" >/dev/null 2>&1 && pwd )\"");
	f->store_line("}");
	f->store_line("");
	f->store_line("BASE_PATH=\"$(app_realpath \"${BASH_SOURCE[0]}\")\"");
	f->store_line("\"$BASE_PATH/" + p_pkg_name + "\" \"$@\"");
	f->store_line("");

	return OK;
}

Ref<Texture2D> EditorExportPlatformMacOS::get_run_icon() const { return run_icon; }

int EditorExportPlatformMacOS::get_options_count() const { return menu_options; }

String EditorExportPlatformMacOS::get_option_label(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall") : TTR("Run on remote macOS system");
}

String EditorExportPlatformMacOS::get_option_tooltip(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall running project from the remote system")
					 : TTR("Run exported project on remote macOS system");
}

Error EditorExportPlatformMacOS::_copy_and_sign_files(Ref<DirAccess>& dir_access,
	const String& p_src_path, const String& p_in_app_path, bool p_sign_enabled,
	const Ref<EditorExportPreset>& p_preset, const String& p_ent_path,
	const String& p_helper_ent_path, bool p_should_error_on_non_code_sign, bool p_sandbox)
{
	return OK;
}


// --- Merged from platform/visionos/export/export.cpp ---
#include "editor/editor_ui.h"
// --- Inlined from platform/visionos/export/export_plugin.h ---
#include "editor/editor_ui.h"

class EditorExportPlatformVisionOS : public EditorExportPlatformAppleEmbedded
{
	static Vector<String> device_types;

	virtual String get_platform_name() const override { return "visionos"; }

	virtual String get_sdk_name() const override { return "xros"; }

	virtual const Vector<String> get_device_types() const override { return device_types; }

	virtual String get_minimum_deployment_target() const override { return "26.0"; }

	virtual Vector<EditorExportPlatformAppleEmbedded::IconInfo> get_icon_infos() const override;

	virtual void get_export_options(List<ExportOption>* r_options) const override;

	virtual String _process_config_file_line(const Ref<EditorExportPreset>& p_preset,
		const String& p_line, const AppleEmbeddedConfigData& p_config, bool p_debug,
		const CodeSigningDetails& p_code_signing) override;

public:
	virtual String get_name() const override { return "visionOS"; }

	virtual String get_os_name() const override { return "visionOS"; }

	virtual void get_platform_features(List<String>* r_features) const override
	{
		EditorExportPlatformAppleEmbedded::get_platform_features(r_features);
		r_features->push_back("visionos");
	}

	virtual void initialize() override;
	~EditorExportPlatformVisionOS();
};



// --- End platform/visionos/export/export_plugin.h ---

void register_visionos_exporter_types() {}

void register_visionos_exporter()
{
	Ref<EditorExportPlatformVisionOS> platform;
	platform.instantiate();

	EditorExport::add_export_platform(platform);
}



// ============================================================================
// Merged from platform/visionos/export/export_plugin.cpp
// ============================================================================

#include "core/config/engine.h"

#include "platform/android/export/export_icons_svg.gen.h"

Vector<String> EditorExportPlatformVisionOS::device_types({"realityDevice"});

void EditorExportPlatformVisionOS::initialize()
{
	if (Engine::is_editor_hint()) {
		EditorExportPlatformAppleEmbedded::_initialize(_visionos_logo_svg, _visionos_run_icon_svg);
#ifdef MACOS_ENABLED
		_start_remote_device_poller_thread();
#endif
	}
}

EditorExportPlatformVisionOS::~EditorExportPlatformVisionOS()
{
#ifdef MACOS_ENABLED
	_stop_remote_device_poller_thread();
#endif
}

Vector<EditorExportPlatformAppleEmbedded::IconInfo>
EditorExportPlatformVisionOS::get_icon_infos() const
{
	return Vector<EditorExportPlatformAppleEmbedded::IconInfo>();
}

String EditorExportPlatformVisionOS::_process_config_file_line(
	const Ref<EditorExportPreset>& p_preset, const String& p_line,
	const AppleEmbeddedConfigData& p_config, bool p_debug, const CodeSigningDetails& p_code_signing)
{
	// Do visionOS specific processing first, and call super implementation if there are no matches

	String strnew;

	// Supported Destinations
	if (p_line.contains("$targeted_device_family")) {
		strnew += p_line.replace("$targeted_device_family", "7") + "\n";

		// MoltenVK Framework not used on visionOS
	}
	else if (p_line.contains("$moltenvk_buildfile")) {
		strnew += p_line.replace("$moltenvk_buildfile", "") + "\n";
	}
	else if (p_line.contains("$moltenvk_fileref")) {
		strnew += p_line.replace("$moltenvk_fileref", "") + "\n";
	}
	else if (p_line.contains("$moltenvk_buildphase")) {
		strnew += p_line.replace("$moltenvk_buildphase", "") + "\n";
	}
	else if (p_line.contains("$moltenvk_buildgrp")) {
		strnew += p_line.replace("$moltenvk_buildgrp", "") + "\n";

		// Launch Storyboard
	}
	else if (p_line.contains("$plist_launch_screen_name")) {
		strnew += p_line.replace("$plist_launch_screen_name", "") + "\n";
	}
	else if (p_line.contains("$pbx_launch_screen_file_reference")) {
		strnew += p_line.replace("$pbx_launch_screen_file_reference", "") + "\n";
	}
	else if (p_line.contains("$pbx_launch_screen_copy_files")) {
		strnew += p_line.replace("$pbx_launch_screen_copy_files", "") + "\n";
	}
	else if (p_line.contains("$pbx_launch_screen_build_phase")) {
		strnew += p_line.replace("$pbx_launch_screen_build_phase", "") + "\n";
	}
	else if (p_line.contains("$pbx_launch_screen_build_reference")) {
		strnew += p_line.replace("$pbx_launch_screen_build_reference", "") + "\n";
	}
	else if (p_line.contains("$valid_archs")) {
		strnew += p_line.replace("$valid_archs", "arm64") + "\n";

		// Apple Embedded common
	}
	else {
		strnew += EditorExportPlatformAppleEmbedded::_process_config_file_line(
			p_preset, p_line, p_config, p_debug, p_code_signing);
	}
	return strnew;
}

void EditorExportPlatformVisionOS::get_export_options(
	List<EditorExportPlatform::ExportOption>* r_options) const
{
}


// --- Merged from platform/web/export/export.cpp ---
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"
#include "editor/editor_ui.h"
// --- Inlined from platform/web/export/export_plugin.h ---
#include "editor/editor_ui.h"
// --- Folded from platform/web/export/editor_http_server.h ---
#include "core/io/stream_peer.h"

class EditorHTTPServer : public RefCounted
{
private:
	Ref<SocketServer> server;
	HashMap<String, String> mimes;
	Ref<StreamPeerSocket> tcp;
	Ref<StreamPeerTLS> tls;
	Ref<StreamPeer> peer;
	Ref<CryptoKey> key;
	Ref<X509Certificate> cert;
	bool use_tls = false;
	uint64_t time = 0;
	uint8_t req_buf[4096];
	int req_pos = 0;

	SafeFlag server_quit;
	Mutex server_lock;
	Thread server_thread;

	void _clear_client();
	void _set_internal_certs(Ref<Crypto> p_crypto);
	void _send_response();
	void _poll();

	static void _server_thread_poll(void* data);

public:
	EditorHTTPServer();
	~EditorHTTPServer();

	void stop();
	Error listen(
		int p_port, IPAddress p_address, bool p_use_tls, String p_tls_key, String p_tls_cert);
	bool is_listening() const;
};



// --- End platform/web/export/editor_http_server.h ---
#include "main/splash.gen.h"

class ImageTexture;

class EditorExportPlatformWeb : public EditorExportPlatform
{
	enum RemoteDebugState
	{
		REMOTE_DEBUG_STATE_UNAVAILABLE,
		REMOTE_DEBUG_STATE_AVAILABLE,
		REMOTE_DEBUG_STATE_SERVING,
	};

	Ref<ImageTexture> logo;
	Ref<ImageTexture> run_icon;
	Ref<ImageTexture> stop_icon;
	Ref<ImageTexture> restart_icon;
	RemoteDebugState remote_debug_state = REMOTE_DEBUG_STATE_UNAVAILABLE;

	Ref<EditorHTTPServer> server;

	String _get_template_name(bool p_extension, bool p_thread_support, bool p_debug) const
	{
		String name = "web";
		if (p_extension) {
			name += "_dlink";
		}
		if (!p_thread_support) {
			name += "_nothreads";
		}
		if (p_debug) {
			name += "_debug.zip";
		}
		else {
			name += "_release.zip";
		}
		return name;
	}

	Error _extract_template(
		const String& p_template, const String& p_dir, const String& p_name, bool pwa);
	void _replace_strings(const HashMap<String, String>& p_replaces, Vector<uint8_t>& r_template);
	Error _build_pwa(const Ref<EditorExportPreset>& p_preset, const String p_path,
		const Vector<SharedObject>& p_shared_objects);
	Error _write_or_error(const uint8_t* p_content, int p_len, String p_path);

	Error _export_project(const Ref<EditorExportPreset>& p_preset, int p_debug_flags);
	Error _launch_browser(const String& p_bind_host, uint16_t p_bind_port, bool p_use_tls);
	Error _start_server(const String& p_bind_host, uint16_t p_bind_port, bool p_use_tls);
	Error _stop_server();

public:
	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const override;

	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;

	virtual String get_name() const override;
	virtual String get_os_name() const override;
	virtual Ref<Texture2D> get_logo() const override;

	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;
	virtual List<String> get_binary_extensions(
		const Ref<EditorExportPreset>& p_preset) const override;
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;

	virtual bool poll_export() override;
	virtual int get_options_count() const override;
	virtual String get_option_label(int p_index) const override;
	virtual String get_option_tooltip(int p_index) const override;
	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_option, uint32_t p_debug_flags) override;
	virtual Ref<Texture2D> get_run_icon() const override;

	virtual void get_platform_features(List<String>* r_features) const override
	{
		r_features->push_back("web");
		r_features->push_back(get_os_name().to_lower());
	}

	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features) override
	{
	}

	String get_debug_protocol() const override { return "ws://"; }

	~EditorExportPlatformWeb() = default;
};

// --- End platform/web/export/export_plugin.h ---

void register_web_exporter_types() {}





// ============================================================================
// Merged from platform/web/export/editor_http_server.cpp
// ============================================================================


#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/settings/editor_settings.h"

void EditorHTTPServer::_server_thread_poll(void *data) {
	EditorHTTPServer *web_server = static_cast<EditorHTTPServer *>(data);
	while (!web_server->server_quit.is_set()) {
		OS::delay_usec(6900);
		{
			MutexLock lock(web_server->server_lock);
			web_server->_poll();
		}
	}
}

void EditorHTTPServer::_clear_client() {
	peer = Ref<StreamPeer>();
	tls = Ref<StreamPeerTLS>();
	tcp = Ref<StreamPeerSocket>();
	memset(req_buf, 0, sizeof(req_buf));
	time = 0;
	req_pos = 0;
}

void EditorHTTPServer::_set_internal_certs(Ref<Crypto> p_crypto) {
	const String cache_path = EditorPaths::get_cache_dir();
	const String key_path = cache_path.path_join("html5_server.key");
	const String crt_path = cache_path.path_join("html5_server.crt");
	bool regen = !FileAccess::exists(key_path) || !FileAccess::exists(crt_path);
	if (!regen) {
		key = Ref<CryptoKey>(CryptoKey::create());
		cert = Ref<X509Certificate>(X509Certificate::create());
		if (key->load(key_path) != OK || cert->load(crt_path) != OK) {
			regen = true;
		}
	}
	if (regen) {
		key = p_crypto->generate_rsa(2048);
		key->save(key_path);
		cert = p_crypto->generate_self_signed_certificate(key, "CN=godot-debug.local,O=A Game Dev,C=XXA", "20140101000000", "20340101000000");
		cert->save(crt_path);
	}
}

void EditorHTTPServer::_send_response() {
	Vector<String> psa = String((char *)req_buf).split("\r\n");
	int len = psa.size();
	ERR_FAIL_COND_MSG(len < 4, "Not enough response headers, got: " + itos(len) + ", expected >= 4.");

	Vector<String> req = psa[0].split(" ", false);
	ERR_FAIL_COND_MSG(req.size() < 2, "Invalid protocol or status code.");

	// Wrong protocol
	ERR_FAIL_COND_MSG(req[0] != "GET" || req[2] != "HTTP/1.1", "Invalid method or HTTP version.");

	const int query_index = req[1].find_char('?');
	const String path = (query_index == -1) ? req[1] : req[1].substr(0, query_index);

	const String req_file = path.get_file();
	const String req_ext = path.get_extension();
	const String cache_path = EditorPaths::get_temp_dir().path_join("web");
	const String filepath = cache_path.path_join(req_file);

	if (!mimes.has(req_ext) || !FileAccess::exists(filepath)) {
		String s = "HTTP/1.1 404 Not Found\r\n";
		s += "Connection: Close\r\n";
		s += "\r\n";
		CharString cs = s.utf8();
		peer->put_data((const uint8_t *)cs.get_data(), cs.size() - 1);
		return;
	}
	const String ctype = mimes[req_ext];

	Ref<FileAccess> f = FileAccess::open(filepath, FileAccess::READ);
	ERR_FAIL_COND(f.is_null());
	String s = "HTTP/1.1 200 OK\r\n";
	s += "Connection: Close\r\n";
	s += "Content-Type: " + ctype + "\r\n";
	s += "Access-Control-Allow-Origin: *\r\n";
	s += "Cross-Origin-Opener-Policy: same-origin\r\n";
	s += "Cross-Origin-Embedder-Policy: require-corp\r\n";
	s += "Cache-Control: no-store, max-age=0\r\n";
	s += "\r\n";
	CharString cs = s.utf8();
	Error err = peer->put_data((const uint8_t *)cs.get_data(), cs.size() - 1);
	if (err != OK) {
		ERR_FAIL();
	}

	while (true) {
		uint8_t bytes[4096];
		uint64_t read = f->get_buffer(bytes, 4096);
		if (read == 0) {
			break;
		}
		err = peer->put_data(bytes, read);
		if (err != OK) {
			ERR_FAIL();
		}
	}
}

void EditorHTTPServer::_poll() {
	if (!server->is_listening()) {
		return;
	}
	if (tcp.is_null()) {
		if (!server->is_connection_available()) {
			return;
		}
		tcp = server->take_connection();
		peer = tcp;
		time = OS::get_ticks_usec();
	}
	if (OS::get_ticks_usec() - time > 1000000) {
		_clear_client();
		return;
	}
	if (tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		return;
	}

	if (use_tls) {
		if (tls.is_null()) {
			tls = Ref<StreamPeerTLS>(StreamPeerTLS::create());
			peer = tls;
			if (tls->accept_stream(tcp, TLSOptions::server(key, cert)) != OK) {
				_clear_client();
				return;
			}
		}
		tls->poll();
		if (tls->get_status() == StreamPeerTLS::STATUS_HANDSHAKING) {
			// Still handshaking, keep waiting.
			return;
		}
		if (tls->get_status() != StreamPeerTLS::STATUS_CONNECTED) {
			_clear_client();
			return;
		}
	}

	while (true) {
		char *r = (char *)req_buf;
		int l = req_pos - 1;
		if (l > 3 && r[l] == '\n' && r[l - 1] == '\r' && r[l - 2] == '\n' && r[l - 3] == '\r') {
			_send_response();
			_clear_client();
			return;
		}

		int read = 0;
		ERR_FAIL_COND(req_pos >= 4096);
		Error err = peer->get_partial_data(&req_buf[req_pos], 1, read);
		if (err != OK) {
			// Got an error
			_clear_client();
			return;
		} else if (read != 1) {
			// Busy, wait next poll
			return;
		}
		req_pos += read;
	}
}

void EditorHTTPServer::stop() {
	server_quit.set();
	if (server_thread.is_started()) {
		server_thread.wait_to_finish();
	}
	if (server.is_valid()) {
		server->stop();
	}
	_clear_client();
}

Error EditorHTTPServer::listen(int p_port, IPAddress p_address, bool p_use_tls, String p_tls_key, String p_tls_cert) {
	MutexLock lock(server_lock);
	if (server->is_listening()) {
		return ERR_ALREADY_IN_USE;
	}
	use_tls = p_use_tls;
	if (use_tls) {
		Ref<Crypto> crypto = Crypto::create();
		if (crypto.is_null()) {
			return ERR_UNAVAILABLE;
		}
		if (!p_tls_key.is_empty() && !p_tls_cert.is_empty()) {
			key = Ref<CryptoKey>(CryptoKey::create());
			Error err = key->load(p_tls_key);
			ERR_FAIL_COND_V(err != OK, err);
			cert = Ref<X509Certificate>(X509Certificate::create());
			err = cert->load(p_tls_cert);
			ERR_FAIL_COND_V(err != OK, err);
		} else {
			_set_internal_certs(crypto);
		}
	}
	Error err = server->listen(p_port, p_address);
	if (err == OK) {
		server_quit.clear();
		server_thread.start(_server_thread_poll, this);
	}
	return err;
}

bool EditorHTTPServer::is_listening() const {
	MutexLock lock(server_lock);
	return server->is_listening();
}

EditorHTTPServer::EditorHTTPServer() {
	mimes["html"] = "text/html";
	mimes["js"] = "application/javascript";
	mimes["json"] = "application/json";
	mimes["pck"] = "application/octet-stream";
	mimes["png"] = "image/png";
	mimes["svg"] = "image/svg";
	mimes["wasm"] = "application/wasm";
	server.instantiate();
	stop();
}

EditorHTTPServer::~EditorHTTPServer() {
	stop();
}

// ============================================================================
// Merged from platform/web/export/export_plugin.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "core/io/resource_loader.h"
#include "platform/android/export/export_icons_svg.gen.h"
#include "scene/resources/image_texture.h"

Error EditorExportPlatformWeb::_extract_template(
	const String& p_template, const String& p_dir, const String& p_name, bool pwa)
{
	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);
	unzFile pkg = unzOpen2(p_template.utf8().get_data(), &io);

	if (!pkg) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
			vformat(TTR("Could not open template for export: \"%s\"."), p_template));
		return ERR_FILE_NOT_FOUND;
	}

	if (unzGoToFirstFile(pkg) != UNZ_OK) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
			vformat(TTR("Invalid export template: \"%s\"."), p_template));
		unzClose(pkg);
		return ERR_FILE_CORRUPT;
	}

	do {
		// get filename
		unz_file_info info;
		char fname[16384];
		unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);

		String file = String::utf8(fname);

		// Skip folders.
		if (file.ends_with("/")) {
			continue;
		}

		// Skip service worker and offline page if not exporting pwa.
		if (!pwa && (file == "godot.service.worker.js" || file == "godot.offline.html")) {
			continue;
		}
		Vector<uint8_t> data;
		data.resize(info.uncompressed_size);

		// read
		unzOpenCurrentFile(pkg);
		unzReadCurrentFile(pkg, data.ptrw(), data.size());
		unzCloseCurrentFile(pkg);

		// write
		String dst = p_dir.path_join(file.replace("voltaire", p_name));
		Ref<FileAccess> f = FileAccess::open(dst, FileAccess::WRITE);
		if (f.is_null()) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
				vformat(TTR("Could not write file: \"%s\"."), dst));
			unzClose(pkg);
			return ERR_FILE_CANT_WRITE;
		}
		f->store_buffer(data.ptr(), data.size());

	} while (unzGoToNextFile(pkg) == UNZ_OK);
	unzClose(pkg);
	return OK;
}

Error EditorExportPlatformWeb::_write_or_error(const uint8_t* p_content, int p_size, String p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Export"),
			vformat(TTR("Could not write file: \"%s\"."), p_path));
		return ERR_FILE_CANT_WRITE;
	}
	f->store_buffer(p_content, p_size);
	return OK;
}

void EditorExportPlatformWeb::_replace_strings(
	const HashMap<String, String>& p_replaces, Vector<uint8_t>& r_template)
{
	String str_template =
		String::utf8(reinterpret_cast<const char*>(r_template.ptr()), r_template.size());
	String out;
	Vector<String> lines = str_template.split("\n");
	for (int i = 0; i < lines.size(); i++) {
		String current_line = lines[i];
		for (const KeyValue<String, String>& E : p_replaces) {
			current_line = current_line.replace(E.key, E.value);
		}
		out += current_line + "\n";
	}
	CharString cs = out.utf8();
	r_template.resize(cs.length());
	for (int i = 0; i < cs.length(); i++) {
		r_template.write[i] = cs[i];
	}
}

String EditorExportPlatformWeb::get_name() const { return "Web"; }

String EditorExportPlatformWeb::get_os_name() const { return "Web"; }

Ref<Texture2D> EditorExportPlatformWeb::get_logo() const { return logo; }

List<String> EditorExportPlatformWeb::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	List<String> list;
	list.push_back("html");
	return list;
}

bool EditorExportPlatformWeb::poll_export()
{
	Ref<EditorExportPreset> preset =
		EditorExport::get_runnable_preset_for_platform(this);

	RemoteDebugState prev_remote_debug_state = remote_debug_state;
	remote_debug_state = REMOTE_DEBUG_STATE_UNAVAILABLE;

	if (preset.is_valid()) {
		const bool debug = true;
		// Throwaway variables to pass to validation functions.
		String err;
		bool missing_templates;

		bool valid = has_valid_export_configuration(preset, err, missing_templates, debug) &&
					 has_valid_project_configuration(preset, err);

		if (valid) {
			if (server->is_listening()) {
				remote_debug_state = REMOTE_DEBUG_STATE_SERVING;
			}
			else {
				remote_debug_state = REMOTE_DEBUG_STATE_AVAILABLE;
			}
		}
	}

	if (remote_debug_state != REMOTE_DEBUG_STATE_SERVING && server->is_listening()) {
		server->stop();
	}

	return remote_debug_state != prev_remote_debug_state;
}

int EditorExportPlatformWeb::get_options_count() const
{
	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE: {
		return 0;
	} break;

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		return 2;
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		return 3;
	} break;
	}

	return 0;
}

String EditorExportPlatformWeb::get_option_label(int p_index) const
{
	String run_in_browser = TTR("Run in Browser");
	String start_http_server = TTR("Start HTTP Server");
	String reexport_project = TTR("Re-export Project");
	String stop_http_server = TTR("Stop HTTP Server");

	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE:
		return "";

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return start_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return reexport_project;
		case 2:
			return stop_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;
	}

	return "";
}

String EditorExportPlatformWeb::get_option_tooltip(int p_index) const
{
	String run_in_browser = TTR("Run exported HTML in the system's default browser.");
	String start_http_server = TTR("Start the HTTP server.");
	String reexport_project = TTR("Export project again to account for updates.");
	String stop_http_server = TTR("Stop the HTTP server.");

	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE:
		return "";

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return start_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return reexport_project;
		case 2:
			return stop_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;
	}

	return "";
}

Error EditorExportPlatformWeb::_export_project(
	const Ref<EditorExportPreset>& p_preset, int p_debug_flags)
{
	const String dest = EditorPaths::get_temp_dir().path_join("web");
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (!da->dir_exists(dest)) {
		Error err = da->make_dir_recursive(dest);
		if (err != OK) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Run"),
				vformat(TTR("Could not create HTTP server directory: %s."), dest));
			return err;
		}
	}

	const String basepath = dest.path_join("tmp_js_export");
	Error err = export_project(p_preset, true, basepath + ".html", p_debug_flags);
	if (err != OK) {
		// Export generates several files, clean them up on failure.
		DirAccess::remove_file_or_error(basepath + ".html");
		DirAccess::remove_file_or_error(basepath + ".offline.html");
		DirAccess::remove_file_or_error(basepath + ".js");
		DirAccess::remove_file_or_error(basepath + ".audio.worklet.js");
		DirAccess::remove_file_or_error(basepath + ".audio.position.worklet.js");
		DirAccess::remove_file_or_error(basepath + ".service.worker.js");
		DirAccess::remove_file_or_error(basepath + ".pck");
		DirAccess::remove_file_or_error(basepath + ".png");
		DirAccess::remove_file_or_error(basepath + ".side.wasm");
		DirAccess::remove_file_or_error(basepath + ".wasm");
		DirAccess::remove_file_or_error(basepath + ".icon.png");
		DirAccess::remove_file_or_error(basepath + ".apple-touch-icon.png");
	}
	return err;
}

Error EditorExportPlatformWeb::_launch_browser(
	const String& p_bind_host, const uint16_t p_bind_port, const bool p_use_tls)
{
	OS::shell_open(String((p_use_tls ? "https://" : "http://") + p_bind_host +
										   ":" + itos(p_bind_port) + "/tmp_js_export.html"));
	// FIXME: Find out how to clean up export files after running the successfully
	// exported game. Might not be trivial.
	return OK;
}

Error EditorExportPlatformWeb::_stop_server()
{
	server->stop();
	return OK;
}

Ref<Texture2D> EditorExportPlatformWeb::get_run_icon() const { return run_icon; }

void register_web_exporter() {}


// --- Merged from platform/windows/export/export.cpp ---
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"
#include "editor/editor_ui.h"
// --- Inlined from platform/windows/export/export_plugin.h ---
#include "core/os/os.h"
#include "editor/editor_ui.h"

// Optional environment variables for defining confidential information. If any
// of these is set, they will override the values set in the credentials file.
const String ENV_WIN_CODESIGN_ID_TYPE = "GODOT_WINDOWS_CODESIGN_IDENTITY_TYPE";
const String ENV_WIN_CODESIGN_ID = "GODOT_WINDOWS_CODESIGN_IDENTITY";
const String ENV_WIN_CODESIGN_PASS = "GODOT_WINDOWS_CODESIGN_PASSWORD";

class EditorExportPlatformWindows : public EditorExportPlatformPC
{
	struct SSHCleanupCommand
	{
		String host;
		String port;
		Vector<String> ssh_args;
		String cmd_args;
		bool wait = false;

		SSHCleanupCommand() {}

		SSHCleanupCommand(const String& p_host, const String& p_port,
			const Vector<String>& p_ssh_arg, const String& p_cmd_args, bool p_wait = false)
		{
			host = p_host;
			port = p_port;
			ssh_args = p_ssh_arg;
			cmd_args = p_cmd_args;
			wait = p_wait;
		}
	};

	Ref<ImageTexture> run_icon;
	Ref<ImageTexture> stop_icon;

	Vector<SSHCleanupCommand> cleanup_commands;
	ProcessID ssh_pid = 0;
	int menu_options = 0;

	Error _process_icon(const Ref<EditorExportPreset>& p_preset, const String& p_src_path,
		const String& p_dst_path);
	Error _add_data(
		const Ref<EditorExportPreset>& p_preset, const String& p_path, bool p_console_icon);
	Error _code_sign(const Ref<EditorExportPreset>& p_preset, const String& p_path);

	String _get_exe_arch(const String& p_path) const;

public:
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;
	virtual Error modify_template(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags) override;
	virtual Error sign_shared_object(
		const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path) override;
	virtual List<String> get_binary_extensions(
		const Ref<EditorExportPreset>& p_preset) const override;
	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;
	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;
	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const override;

	virtual String get_template_file_name(
		const String& p_target, const String& p_arch) const override;
	virtual Error fixup_embedded_pck(
		const String& p_path, int64_t p_embedded_start, int64_t p_embedded_size) override;

	virtual void get_platform_features(List<String>* r_features) const override;

	virtual Ref<Texture2D> get_run_icon() const override;
	virtual bool poll_export() override;
	virtual int get_options_count() const override;
	virtual String get_option_label(int p_index) const override;
	virtual String get_option_tooltip(int p_index) const override;
	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags) override;
	virtual void cleanup() override;
};



// --- End platform/windows/export/export_plugin.h ---

void register_windows_exporter_types() {}





// ============================================================================
// Merged from platform/windows/export/export_plugin.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/settings/editor_settings.h"

#include "platform/android/export/export_icons_svg.gen.h"
#include "core/io/resource_loader.h"
#include "platform/android/export/export_icons_svg.gen.h"
#include "scene/resources/image_texture.h"
// --- Inlined from platform/windows/export/template_modifier.h ---
#include "core/io/file_access.h"

class EditorExportPreset;

class TemplateModifier {
	const uint32_t PE_PAGE_SIZE = 4096;
	const uint32_t BLOCK_SIZE = 512;
	const uint32_t COFF_HEADER_SIZE = 24;
	const uint32_t POINTER_TO_PE_HEADER_OFFSET = 0x3c;
	// all offsets below are calculated from POINTER_TO_PE_HEADER_OFFSET value, at 0 is magic string PE (0x50450000)
	const uint32_t MAGIC_NUMBER_OFFSET = 24;
	const uint32_t SIZE_OF_INITIALIZED_DATA_OFFSET = 32;
	const uint32_t SIZE_OF_IMAGE_OFFSET = 80;

	struct ByteStream {
		void save(uint8_t p_value, Vector<uint8_t> &r_bytes) const;
		void save(uint16_t p_value, Vector<uint8_t> &r_bytes) const;
		void save(uint32_t p_value, Vector<uint8_t> &r_bytes) const;
		void save(const String &p_value, Vector<uint8_t> &r_bytes) const;
		void save(uint32_t p_value, Vector<uint8_t> &r_bytes, uint32_t p_count) const;
		Vector<uint8_t> save() const;
	};

	struct ResourceDirectoryTable : ByteStream {
		static const uint16_t SIZE = 16;

		uint16_t name_entry_count = 0;
		uint16_t id_entry_count = 0;

		Vector<uint8_t> save() const;
	};

	struct ResourceDirectoryEntry : ByteStream {
		static const uint16_t SIZE = 8;
		static const uint32_t ICON = 0x03;
		static const uint32_t GROUP_ICON = 0x0e;
		static const uint32_t MANIFEST = 0x18;
		static const uint32_t VERSION = 0x10;
		static const uint32_t ENGLISH = 0x0409;
		static const uint32_t HIGH_BIT = 0x80000000;

		uint32_t id = 0;
		uint32_t data_offset = 0;
		bool name = false;
		bool subdirectory = false;

		Vector<uint8_t> save() const;
	};

	struct FixedFileInfo : ByteStream {
		uint32_t signature = 0xfeef04bd;
		uint32_t struct_version = 0x10000;
		uint32_t file_version_ms = 0;
		uint32_t file_version_ls = 0;
		uint32_t product_version_ms = 0;
		uint32_t product_version_ls = 0;
		uint32_t file_flags_mask = 0;
		uint32_t file_flags = 0;
		uint32_t file_os = 0x00000004;
		uint32_t file_type = 0x00000001;
		uint32_t file_subtype = 0;
		uint32_t file_date_ms = 0;
		uint32_t file_date_ls = 0;

		Vector<uint8_t> save() const;
		void set_file_version(const String &p_file_version);
		void set_product_version(const String &p_product_version);
	};

	struct Structure : ByteStream {
		uint16_t length = 0;
		uint16_t value_length = 0;
		uint16_t type = 0;
		String key;

		Vector<uint8_t> save() const;
		Vector<uint8_t> &add_length(Vector<uint8_t> &r_bytes) const;
	};

	struct StringStructure : Structure {
		String value;

		Vector<uint8_t> save() const;
		StringStructure();
		StringStructure(const String &p_key, const String &p_value);
	};

	struct StringTable : Structure {
		Vector<StringStructure> strings;

		Vector<uint8_t> save() const;
		void put(const String &p_key, const String &p_value);
		StringTable();
	};

	struct StringFileInfo : Structure {
		StringTable string_table;

		Vector<uint8_t> save() const;
		StringFileInfo();
	};

	struct Var : Structure {
		const uint32_t value = 0x04b00409;

		Vector<uint8_t> save() const;
		Var();
	};

	struct VarFileInfo : Structure {
		Var var;

		Vector<uint8_t> save() const;
		VarFileInfo();
	};

	struct VersionInfo : Structure {
		FixedFileInfo value;
		StringFileInfo string_file_info;
		VarFileInfo var_file_info;

		Vector<uint8_t> save() const;
		VersionInfo();
	};

	struct ManifestInfo : Structure {
		String manifest;

		Vector<uint8_t> save() const;
		ManifestInfo() {}
	};

	struct IconEntry : ByteStream {
		static const uint32_t SIZE = 16;

		uint8_t width = 0;
		uint8_t height = 0;
		uint8_t colors = 0;
		uint8_t reserved = 0;
		uint16_t planes = 0;
		uint16_t bits_per_pixel = 32;
		uint32_t image_size = 0;
		uint32_t image_offset = 0;
		Vector<uint8_t> data;

		Vector<uint8_t> save() const;
		void load(Ref<FileAccess> p_file);
	};

	struct GroupIcon : ByteStream {
		static constexpr uint8_t SIZES[6] = { 16, 32, 48, 64, 128, 0 };

		uint16_t reserved = 0;
		uint16_t type = 1;
		uint16_t image_count = 0;
		Vector<IconEntry> icon_entries;
		Vector<Vector<uint8_t>> images;

		Vector<uint8_t> save() const;
		void load(Ref<FileAccess> p_icon_file);
		void fill_with_godot_blue();
	};

	struct SectionEntry : ByteStream {
		static const uint32_t SIZE = 40;

		String name;
		uint32_t virtual_size = 0;
		uint32_t virtual_address = 0;
		uint32_t size_of_raw_data = 0;
		uint32_t pointer_to_raw_data = 0;
		uint32_t pointer_to_relocations = 0;
		uint32_t pointer_to_line_numbers = 0;
		uint16_t number_of_relocations = 0;
		uint16_t number_of_line_numbers = 0;
		uint32_t characteristics = 0;

		Vector<uint8_t> save() const;
		void load(Ref<FileAccess> p_file);
	};

	struct ResourceDataEntry : ByteStream {
		static const uint16_t SIZE = 16;

		uint32_t rva = 0;
		uint32_t size = 0;

		Vector<uint8_t> save() const;
	};

	uint32_t _snap(uint32_t p_value, uint32_t p_size) const;
	uint32_t _get_pe_header_offset(Ref<FileAccess> p_executable) const;
	Vector<SectionEntry> _get_section_entries(Ref<FileAccess> p_executable) const;
	GroupIcon _create_group_icon(const String &p_icon_path) const;
	ManifestInfo _create_manifest_info() const;
	VersionInfo _create_version_info(const HashMap<String, String> &p_strings) const;
	Vector<uint8_t> _create_resources(uint32_t p_virtual_address, const GroupIcon &p_group_icon, const VersionInfo &p_version_info, const ManifestInfo &p_manifest_info) const;
	Error _truncate(const String &p_executable_path, uint32_t p_size) const;
	HashMap<String, String> _get_strings(const Ref<EditorExportPreset> &p_preset) const;
	Error _modify_template(const Ref<EditorExportPreset> &p_preset, const String &p_template_path, const String &p_icon_path) const;

public:
	static Error modify(const Ref<EditorExportPreset> &p_preset, const String &p_template_path, const String &p_icon_path);
};

// --- End platform/windows/export/template_modifier.h ---

#ifdef WINDOWS_ENABLED
#include "shlobj.h"

// Converts long path to Windows UNC format.
static String fix_path(const String& p_path)
{
	String path = p_path;
	if (p_path.is_relative_path()) {
		Char16String current_dir_name;
		size_t str_len = GetCurrentDirectoryW(0, nullptr);
		current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
		path = String::utf16((const char16_t*)current_dir_name.get_data())
				   .trim_prefix(R"(\\?\)")
				   .replace_char('\\', '/')
				   .path_join(path);
	}
	path = path.simplify_path();
	path = path.replace_char('/', '\\');
	if (path.size() >= MAX_PATH && !path.is_network_share_path() && !path.begins_with(R"(\\?\)")) {
		path = R"(\\?\)" + path;
	}
	return path;
}

#endif

String EditorExportPlatformWindows::get_template_file_name(
	const String& p_target, const String& p_arch) const
{
	return "windows_" + p_target + "_" + p_arch + ".exe";
}

List<String> EditorExportPlatformWindows::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	List<String> list;
	list.push_back("exe");
	list.push_back("zip");
	return list;
}

void EditorExportPlatformWindows::get_platform_features(List<String>* r_features) const
{
	EditorExportPlatformPC::get_platform_features(r_features);
	r_features->push_back("windows");
}

Ref<Texture2D> EditorExportPlatformWindows::get_run_icon() const { return run_icon; }

int EditorExportPlatformWindows::get_options_count() const { return menu_options; }

String EditorExportPlatformWindows::get_option_label(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall") : TTR("Run on remote Windows system");
}

String EditorExportPlatformWindows::get_option_tooltip(int p_index) const
{
	return (p_index) ? TTR("Stop and uninstall running project from the remote system")
					 : TTR("Run exported project on remote Windows system");
}

void EditorExportPlatformWindows::cleanup()
{
	if (ssh_pid != 0 && OS::is_process_running(ssh_pid)) {
		__print_line("Terminating connection...");
		OS::kill(ssh_pid);
		OS::delay_usec(1000);
	}

	if (!cleanup_commands.is_empty()) {
		__print_line("Stopping and deleting previous version...");
		for (const SSHCleanupCommand& cmd : cleanup_commands) {
			if (cmd.wait) {
				ssh_run_on_remote(cmd.host, cmd.port, cmd.ssh_args, cmd.cmd_args);
			}
			else {
				ssh_run_on_remote_no_wait(cmd.host, cmd.port, cmd.ssh_args, cmd.cmd_args);
			}
		}
	}
	ssh_pid = 0;
	cleanup_commands.clear();
}

void register_windows_exporter() {}



// ============================================================================
// Merged from platform/windows/export/template_modifier.cpp
// ============================================================================

#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "editor/editor_ui.h"


void TemplateModifier::ByteStream::save(uint8_t p_value, Vector<uint8_t>& r_bytes) const
{
	save(p_value, r_bytes, 1);
}

void TemplateModifier::ByteStream::save(uint16_t p_value, Vector<uint8_t>& r_bytes) const
{
	save(p_value, r_bytes, 2);
}

void TemplateModifier::ByteStream::save(uint32_t p_value, Vector<uint8_t>& r_bytes) const
{
	save(p_value, r_bytes, 4);
}

void TemplateModifier::ByteStream::save(const String& p_value, Vector<uint8_t>& r_bytes) const
{
	r_bytes.append_array(p_value.to_utf16_buffer());
	save((uint16_t)0, r_bytes);
}

void TemplateModifier::ByteStream::save(
	uint32_t p_value, Vector<uint8_t>& r_bytes, uint32_t p_count) const
{
	for (uint32_t i = 0; i < p_count; i++) {
		r_bytes.append((uint8_t)(p_value & 0xff));
		p_value >>= 8;
	}
}

Vector<uint8_t> TemplateModifier::ByteStream::save() const { return Vector<uint8_t>(); }

Vector<uint8_t> TemplateModifier::Structure::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(length, bytes);
	ByteStream::save(value_length, bytes);
	ByteStream::save(type, bytes);
	ByteStream::save(key, bytes);
	while (bytes.size() % 4) {
		bytes.append(0);
	}
	return bytes;
}

Vector<uint8_t>& TemplateModifier::Structure::add_length(Vector<uint8_t>& r_bytes) const
{
	r_bytes.write[0] = r_bytes.size() & 0xff;
	r_bytes.write[1] = r_bytes.size() >> 8 & 0xff;
	return r_bytes;
}

Vector<uint8_t> TemplateModifier::ResourceDirectoryTable::save() const
{
	Vector<uint8_t> bytes;
	bytes.resize_initialized(12);
	ByteStream::save(name_entry_count, bytes);
	ByteStream::save(id_entry_count, bytes);
	return bytes;
}

Vector<uint8_t> TemplateModifier::ResourceDirectoryEntry::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(id | (name ? HIGH_BIT : 0), bytes);
	ByteStream::save(data_offset | (subdirectory ? HIGH_BIT : 0), bytes);
	return bytes;
}

Vector<uint8_t> TemplateModifier::FixedFileInfo::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(signature, bytes);
	ByteStream::save(struct_version, bytes);
	ByteStream::save(file_version_ms, bytes);
	ByteStream::save(file_version_ls, bytes);
	ByteStream::save(product_version_ms, bytes);
	ByteStream::save(product_version_ls, bytes);
	ByteStream::save(file_flags_mask, bytes);
	ByteStream::save(file_flags, bytes);
	ByteStream::save(file_os, bytes);
	ByteStream::save(file_type, bytes);
	ByteStream::save(file_subtype, bytes);
	ByteStream::save(file_date_ms, bytes);
	ByteStream::save(file_date_ls, bytes);
	return bytes;
}

void TemplateModifier::FixedFileInfo::set_file_version(const String& p_file_version)
{
	Vector<String> parts = p_file_version.split(".");
	while (parts.size() < 4) {
		parts.append("0");
	}
	file_version_ms = parts[0].to_int() << 16 | (parts[1].to_int() & 0xffff);
	file_version_ls = parts[2].to_int() << 16 | (parts[3].to_int() & 0xffff);
}

void TemplateModifier::FixedFileInfo::set_product_version(const String& p_product_version)
{
	Vector<String> parts = p_product_version.split(".");
	while (parts.size() < 4) {
		parts.append("0");
	}
	product_version_ms = parts[0].to_int() << 16 | (parts[1].to_int() & 0xffff);
	product_version_ls = parts[2].to_int() << 16 | (parts[3].to_int() & 0xffff);
}

Vector<uint8_t> TemplateModifier::StringStructure::save() const
{
	Vector<uint8_t> bytes = Structure::save();
	ByteStream::save(value, bytes);
	return add_length(bytes);
}

TemplateModifier::StringStructure::StringStructure() { type = 1; }

TemplateModifier::StringStructure::StringStructure(const String& p_key, const String& p_value)
{
	type = 1;
	value_length = p_value.length() + 1;
	key = p_key;
	value = p_value;
}

Vector<uint8_t> TemplateModifier::StringTable::save() const
{
	Vector<uint8_t> bytes = Structure::save();
	for (const StringStructure& string : strings) {
		bytes.append_array(string.save());
		while (bytes.size() % 4) {
			bytes.append(0);
		}
	}
	return add_length(bytes);
}

void TemplateModifier::StringTable::put(const String& p_key, const String& p_value)
{
	strings.append(StringStructure(p_key, p_value));
}

TemplateModifier::StringTable::StringTable()
{
	key = "040904b0";
	type = 1;
}

TemplateModifier::StringFileInfo::StringFileInfo()
{
	key = "StringFileInfo";
	value_length = 0;
	type = 1;
}

Vector<uint8_t> TemplateModifier::StringFileInfo::save() const
{
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(string_table.save());
	return add_length(bytes);
}

Vector<uint8_t> TemplateModifier::Var::save() const
{
	Vector<uint8_t> bytes = Structure::save();
	ByteStream::save(value, bytes);
	return add_length(bytes);
}

TemplateModifier::Var::Var()
{
	value_length = 4;
	key = "Translation";
}

Vector<uint8_t> TemplateModifier::VarFileInfo::save() const
{
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(var.save());
	return add_length(bytes);
}

TemplateModifier::VarFileInfo::VarFileInfo()
{
	type = 1;
	key = "VarFileInfo";
}

Vector<uint8_t> TemplateModifier::VersionInfo::save() const
{
	Vector<uint8_t> fixed_file_info = value.save();
	Vector<uint8_t> bytes = Structure::save();
	bytes.append_array(fixed_file_info);
	bytes.append_array(string_file_info.save());
	while (bytes.size() % 4) {
		bytes.append(0);
	}
	bytes.append_array(var_file_info.save());
	return add_length(bytes);
}

TemplateModifier::VersionInfo::VersionInfo()
{
	key = "VS_VERSION_INFO";
	value_length = 52;
}

Vector<uint8_t> TemplateModifier::ManifestInfo::save() const
{
	Vector<uint8_t> bytes = manifest.to_utf8_buffer();
	return bytes;
}

Vector<uint8_t> TemplateModifier::IconEntry::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(width, bytes);
	ByteStream::save(height, bytes);
	ByteStream::save(colors, bytes);
	ByteStream::save((uint8_t)0, bytes);
	ByteStream::save(planes, bytes);
	ByteStream::save(bits_per_pixel, bytes);
	ByteStream::save(image_size, bytes);
	ByteStream::save((uint16_t)image_offset, bytes);
	return bytes;
}

void TemplateModifier::IconEntry::load(Ref<FileAccess> p_file)
{
	width = p_file->get_8();		   // Width in pixels.
	height = p_file->get_8();		   // Height in pixels.
	colors = p_file->get_8();		   // Number of colors in the palette (0 - no palette).
	p_file->get_8();				   // Reserved.
	planes = p_file->get_16();		   // Number of color planes.
	bits_per_pixel = p_file->get_16(); // Bits per pixel.
	image_size = p_file->get_32();	   // Image data size in bytes.
	image_offset = p_file->get_32();   // Image data offset.
}

Vector<uint8_t> TemplateModifier::GroupIcon::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(reserved, bytes);
	ByteStream::save(type, bytes);
	ByteStream::save(image_count, bytes);
	for (const IconEntry& icon_entry : icon_entries) {
		bytes.append_array(icon_entry.save());
	}
	return bytes;
}

void TemplateModifier::GroupIcon::load(Ref<FileAccess> p_icon_file)
{
	if (p_icon_file->get_32() != 0x10000) { // Wrong reserved bytes
		ERR_FAIL_MSG("Wrong icon file type.");
	}

	image_count = p_icon_file->get_16();
	for (uint16_t i = 0; i < image_count; i++) {
		IconEntry icon_entry;
		icon_entry.load(p_icon_file);
		icon_entries.append(icon_entry);
	}

	int id = 1;
	for (IconEntry& icon_entry : icon_entries) {
		Vector<uint8_t> image;
		image.resize(icon_entry.image_size);
		p_icon_file->seek(icon_entry.image_offset);
		p_icon_file->get_buffer(image.ptrw(), image.size());
		icon_entry.image_offset = id++;
		images.append(image);
	}
}

void TemplateModifier::GroupIcon::fill_with_godot_blue()
{
	uint32_t id = 1;
	for (uint8_t size : SIZES) {
		Ref<Image> image =
			Image::create_empty(size ? size : 256, size ? size : 256, false, Image::FORMAT_RGB8);
		image->fill(Color::hex(0x478cbfff));
		Vector<uint8_t> data = image->save_png_to_buffer();
		IconEntry icon_entry;
		icon_entry.width = size;
		icon_entry.height = size;
		icon_entry.bits_per_pixel = 24;
		icon_entry.image_size = data.size();
		icon_entry.image_offset = id++;
		icon_entries.append(icon_entry);
		images.append(data);
	}
}

Vector<uint8_t> TemplateModifier::SectionEntry::save() const
{
	Vector<uint8_t> bytes;
	bytes.append_array(name.to_utf8_buffer());
	while (bytes.size() < 8) {
		bytes.append(0);
	}
	ByteStream::save(virtual_size, bytes);
	ByteStream::save(virtual_address, bytes);
	ByteStream::save(size_of_raw_data, bytes);
	ByteStream::save(pointer_to_raw_data, bytes);
	ByteStream::save(pointer_to_relocations, bytes);
	ByteStream::save(pointer_to_line_numbers, bytes);
	ByteStream::save(number_of_relocations, bytes);
	ByteStream::save(number_of_line_numbers, bytes);
	ByteStream::save(characteristics, bytes);
	return bytes;
}

void TemplateModifier::SectionEntry::load(Ref<FileAccess> p_file)
{
	uint8_t section_name[8];
	p_file->get_buffer(section_name, 8);
	name = String::utf8((char*)section_name, 8);
	virtual_size = p_file->get_32();
	virtual_address = p_file->get_32();
	size_of_raw_data = p_file->get_32();
	pointer_to_raw_data = p_file->get_32();
	pointer_to_relocations = p_file->get_32();
	pointer_to_line_numbers = p_file->get_32();
	number_of_relocations = p_file->get_16();
	number_of_line_numbers = p_file->get_16();
	characteristics = p_file->get_32();
}

Vector<uint8_t> TemplateModifier::ResourceDataEntry::save() const
{
	Vector<uint8_t> bytes;
	ByteStream::save(rva, bytes);
	ByteStream::save(size, bytes);
	ByteStream::save(0, bytes, 8);
	return bytes;
}

uint32_t TemplateModifier::_get_pe_header_offset(Ref<FileAccess> p_executable) const
{
	p_executable->seek(POINTER_TO_PE_HEADER_OFFSET);
	uint32_t pe_header_offset = p_executable->get_32();

	p_executable->seek(pe_header_offset);
	uint32_t magic = p_executable->get_32();

	return magic == 0x00004550 ? pe_header_offset : 0;
}

uint32_t TemplateModifier::_snap(uint32_t p_value, uint32_t p_size) const
{
	return p_value + (p_value % p_size ? p_size - (p_value % p_size) : 0);
}

Vector<uint8_t> TemplateModifier::_create_resources(uint32_t p_virtual_address,
	const GroupIcon& p_group_icon, const VersionInfo& p_version_info,
	const ManifestInfo& p_manifest_info) const
{
	// 0x04, 0x00 as string length ICON in UTF16 and padding to 32 bits
	const uint8_t ICON_DIRECTORY_STRING[] = {
		0x04, 0x00, 0x49, 0x00, 0x43, 0x00, 0x4f, 0x00, 0x4e, 0x00, 0x00, 0x00};
	const uint16_t RT_ENTRY_COUNT = 4;
	const uint32_t icon_count = p_group_icon.images.size();

	ResourceDirectoryTable root_directory_table;
	root_directory_table.id_entry_count = RT_ENTRY_COUNT;

	Vector<uint8_t> resources = root_directory_table.save();

	ResourceDirectoryEntry rt_icon_entry;
	rt_icon_entry.id = ResourceDirectoryEntry::ICON;
	rt_icon_entry.data_offset =
		ResourceDirectoryTable::SIZE + RT_ENTRY_COUNT * ResourceDirectoryEntry::SIZE;
	rt_icon_entry.subdirectory = true;
	resources.append_array(rt_icon_entry.save());

	ResourceDirectoryEntry rt_group_icon_entry;
	rt_group_icon_entry.id = ResourceDirectoryEntry::GROUP_ICON;
	rt_group_icon_entry.data_offset =
		(2 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count) * ResourceDirectoryEntry::SIZE;
	rt_group_icon_entry.subdirectory = true;
	resources.append_array(rt_group_icon_entry.save());

	ResourceDirectoryEntry rt_version_entry;
	rt_version_entry.id = ResourceDirectoryEntry::VERSION;
	rt_version_entry.data_offset =
		(4 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 2) * ResourceDirectoryEntry::SIZE;
	rt_version_entry.subdirectory = true;
	resources.append_array(rt_version_entry.save());

	ResourceDirectoryEntry rt_manifest_entry;
	rt_manifest_entry.id = ResourceDirectoryEntry::MANIFEST;
	rt_manifest_entry.data_offset =
		(6 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 4) * ResourceDirectoryEntry::SIZE;
	rt_manifest_entry.subdirectory = true;
	resources.append_array(rt_manifest_entry.save());

	ResourceDirectoryTable icon_table;
	icon_table.id_entry_count = icon_count;
	resources.append_array(icon_table.save());

	for (uint32_t i = 0; i < icon_count; i++) {
		ResourceDirectoryEntry icon_entry;
		icon_entry.id = i + 1;
		icon_entry.data_offset = (2 + i) * ResourceDirectoryTable::SIZE +
								 (RT_ENTRY_COUNT + icon_count + i) * ResourceDirectoryEntry::SIZE;
		icon_entry.subdirectory = true;
		resources.append_array(icon_entry.save());
	}

	for (uint32_t i = 0; i < icon_count; i++) {
		ResourceDirectoryTable language_icon_table;
		language_icon_table.id_entry_count = 1;
		resources.append_array(language_icon_table.save());

		ResourceDirectoryEntry language_icon_entry;
		language_icon_entry.id = ResourceDirectoryEntry::ENGLISH;
		language_icon_entry.data_offset =
			(8 + icon_count) * ResourceDirectoryTable::SIZE +
			(RT_ENTRY_COUNT + icon_count * 2 + 6) * ResourceDirectoryEntry::SIZE +
			sizeof(ICON_DIRECTORY_STRING) + i * ResourceDataEntry::SIZE;
		resources.append_array(language_icon_entry.save());
	}

	ResourceDirectoryTable group_icon_name_table;
	group_icon_name_table.name_entry_count = 1;
	resources.append_array(group_icon_name_table.save());

	ResourceDirectoryEntry group_icon_name_entry;
	group_icon_name_entry.id = (6 + icon_count) * ResourceDirectoryTable::SIZE +
							   (RT_ENTRY_COUNT + icon_count * 2 + 4) * ResourceDirectoryEntry::SIZE;
	group_icon_name_entry.data_offset =
		(3 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 1) * ResourceDirectoryEntry::SIZE;
	group_icon_name_entry.name = true;
	group_icon_name_entry.subdirectory = true;
	resources.append_array(group_icon_name_entry.save());

	ResourceDirectoryTable group_icon_language_table;
	group_icon_language_table.id_entry_count = 1;
	resources.append_array(group_icon_language_table.save());

	ResourceDirectoryEntry group_icon_language_entry;
	group_icon_language_entry.id = ResourceDirectoryEntry::ENGLISH;
	group_icon_language_entry.data_offset =
		(8 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 6) * ResourceDirectoryEntry::SIZE +
		sizeof(ICON_DIRECTORY_STRING) + icon_count * ResourceDataEntry::SIZE;
	resources.append_array(group_icon_language_entry.save());

	ResourceDirectoryTable version_table;
	version_table.id_entry_count = 1;
	resources.append_array(version_table.save());

	ResourceDirectoryEntry version_entry;
	version_entry.id = 1;
	version_entry.data_offset =
		(5 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 3) * ResourceDirectoryEntry::SIZE;
	version_entry.subdirectory = true;
	resources.append_array(version_entry.save());

	ResourceDirectoryTable version_language_table;
	version_language_table.id_entry_count = 1;
	resources.append_array(version_language_table.save());

	ResourceDirectoryEntry version_language_entry;
	version_language_entry.id = ResourceDirectoryEntry::ENGLISH;
	version_language_entry.data_offset =
		(8 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 6) * ResourceDirectoryEntry::SIZE +
		sizeof(ICON_DIRECTORY_STRING) + (icon_count + 1) * ResourceDataEntry::SIZE;
	resources.append_array(version_language_entry.save());

	ResourceDirectoryTable manifest_table;
	manifest_table.id_entry_count = 1;
	resources.append_array(manifest_table.save());

	ResourceDirectoryEntry manifest_entry;
	manifest_entry.id = 1;
	manifest_entry.data_offset =
		(7 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 5) * ResourceDirectoryEntry::SIZE;
	manifest_entry.subdirectory = true;
	resources.append_array(manifest_entry.save());

	ResourceDirectoryTable manifest_language_table;
	manifest_language_table.id_entry_count = 1;
	resources.append_array(manifest_language_table.save());

	ResourceDirectoryEntry manifest_language_entry;
	manifest_language_entry.id = ResourceDirectoryEntry::ENGLISH;
	manifest_language_entry.data_offset =
		(8 + icon_count) * ResourceDirectoryTable::SIZE +
		(RT_ENTRY_COUNT + 2 * icon_count + 6) * ResourceDirectoryEntry::SIZE +
		sizeof(ICON_DIRECTORY_STRING) + (icon_count + 2) * ResourceDataEntry::SIZE;
	resources.append_array(manifest_language_entry.save());

	Vector<uint8_t> icon_directory_string;
	icon_directory_string.resize(sizeof(ICON_DIRECTORY_STRING));
	memcpy(icon_directory_string.ptrw(), ICON_DIRECTORY_STRING, sizeof(ICON_DIRECTORY_STRING));
	resources.append_array(icon_directory_string);

	Vector<Vector<uint8_t>> data_entries;
	for (const Vector<uint8_t>& image : p_group_icon.images) {
		data_entries.append(image);
	}
	data_entries.append(p_group_icon.save());
	data_entries.append(p_version_info.save());
	data_entries.append(p_manifest_info.save());

	uint32_t offset = resources.size() + data_entries.size() * ResourceDataEntry::SIZE;

	for (const Vector<uint8_t>& data_entry : data_entries) {
		ResourceDataEntry resource_data_entry;
		resource_data_entry.rva = p_virtual_address + offset;
		resource_data_entry.size = data_entry.size();
		resources.append_array(resource_data_entry.save());
		offset += resource_data_entry.size;
		while (offset % 4) {
			offset += 1;
		}
	}

	for (const Vector<uint8_t>& data_entry : data_entries) {
		resources.append_array(data_entry);
		while (resources.size() % 4) {
			resources.append(0);
		}
	}

	return resources;
}

TemplateModifier::VersionInfo TemplateModifier::_create_version_info(
	const HashMap<String, String>& p_strings) const
{
	StringTable string_table;
	for (const KeyValue<String, String>& E : p_strings) {
		string_table.put(E.key, E.value);
	}

	StringFileInfo string_file_info;
	string_file_info.string_table = string_table;

	FixedFileInfo fixed_file_info;
	if (p_strings.has("FileVersion")) {
		fixed_file_info.set_file_version(p_strings["FileVersion"]);
	}
	if (p_strings.has("ProductVersion")) {
		fixed_file_info.set_product_version(p_strings["ProductVersion"]);
	}

	VersionInfo version_info;
	version_info.value = fixed_file_info;
	version_info.string_file_info = string_file_info;

	return version_info;
}

TemplateModifier::ManifestInfo TemplateModifier::_create_manifest_info() const
{
	ManifestInfo manifest_info;
	manifest_info.manifest = R"MANIFEST(<?xml version="1.0" encoding="utf-8"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<application xmlns="urn:schemas-microsoft-com:asm.v3">
		<windowsSettings xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
			<ws2:longPathAware>true</ws2:longPathAware>
		</windowsSettings>
	</application>
	<dependency>
		<dependentAssembly>
			<assemblyIdentity type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'/>
		</dependentAssembly>
	</dependency>
</assembly>)MANIFEST";
	return manifest_info;
}

TemplateModifier::GroupIcon TemplateModifier::_create_group_icon(const String& p_icon_path) const
{
	GroupIcon group_icon;

	Ref<FileAccess> icon_file = FileAccess::open(p_icon_path, FileAccess::READ);
	if (icon_file.is_null()) {
		group_icon.fill_with_godot_blue();
		return group_icon;
	}

	group_icon.load(icon_file);

	return group_icon;
}

Error TemplateModifier::_truncate(const String& p_path, uint32_t p_size) const
{
	Error error;

	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &error);
	ERR_FAIL_COND_V(error != OK, ERR_CANT_OPEN);

	String truncated_path = p_path + ".truncated";
	Ref<FileAccess> truncated = FileAccess::open(truncated_path, FileAccess::WRITE, &error);
	ERR_FAIL_COND_V(error != OK, ERR_CANT_CREATE);

	truncated->store_buffer(file->get_buffer(p_size));

	file->close();
	truncated->close();

	DirAccess::remove_absolute(p_path);
	DirAccess::rename_absolute(truncated_path, p_path);

	return error;
}

Error TemplateModifier::_modify_template(const Ref<EditorExportPreset>& p_preset,
	const String& p_template_path, const String& p_icon_path) const
{
	Error error;
	Ref<FileAccess> template_file =
		FileAccess::open(p_template_path, FileAccess::READ_WRITE, &error);
	ERR_FAIL_COND_V(error != OK, ERR_CANT_OPEN);

	Vector<SectionEntry> section_entries = _get_section_entries(template_file);
	ERR_FAIL_COND_V(section_entries.size() < 2, ERR_CANT_OPEN);

	// Find resource (".rsrc") and relocation (".reloc") sections, usually last two, but ".debug_*"
	// sections (referenced as "/[n]"), symbol table, and string table can follow.
	int resource_index = section_entries.size() - 2;
	int relocations_index = section_entries.size() - 1;
	for (int i = 0; i < section_entries.size(); i++) {
		if (section_entries[i].name == ".rsrc") {
			resource_index = i;
		}
		else if (section_entries[i].name == ".reloc") {
			relocations_index = i;
		}
	}

	ERR_FAIL_COND_V(section_entries[resource_index].name != ".rsrc", ERR_CANT_OPEN);
	ERR_FAIL_COND_V(section_entries[relocations_index].name != ".reloc", ERR_CANT_OPEN);

	uint64_t original_template_size = template_file->get_length();

	GroupIcon group_icon = _create_group_icon(p_icon_path);

	VersionInfo version_info = _create_version_info(_get_strings(p_preset));
	ManifestInfo manifest_info = _create_manifest_info();

	SectionEntry& resources_section_entry = section_entries.write[resource_index];
	uint32_t old_resources_size_of_raw_data = resources_section_entry.size_of_raw_data;
	Vector<uint8_t> resources = _create_resources(
		resources_section_entry.virtual_address, group_icon, version_info, manifest_info);
	resources_section_entry.virtual_size = resources.size();
	resources.resize_initialized(_snap(resources.size(), BLOCK_SIZE));
	resources_section_entry.size_of_raw_data = resources.size();

	int32_t raw_size_delta =
		resources_section_entry.size_of_raw_data - old_resources_size_of_raw_data;
	uint32_t old_last_section_virtual_address =
		section_entries.get(section_entries.size() - 1).virtual_address;

	// Some data (e.g. DWARF debug symbols) can be placed after the last section.
	uint32_t old_footer_offset =
		section_entries.get(section_entries.size() - 1).pointer_to_raw_data +
		section_entries.get(section_entries.size() - 1).size_of_raw_data;

	// Copy and update sections after ".rsrc".
	Vector<Vector<uint8_t>> moved_section_data;
	uint32_t prev_virtual_address = resources_section_entry.virtual_address;
	uint32_t prev_virtual_size = resources_section_entry.virtual_size;
	for (int i = resource_index + 1; i < section_entries.size(); i++) {
		SectionEntry& section_entry = section_entries.write[i];
		template_file->seek(section_entry.pointer_to_raw_data);
		Vector<uint8_t> data = template_file->get_buffer(section_entry.size_of_raw_data);
		moved_section_data.push_back(data);
		section_entry.pointer_to_raw_data += raw_size_delta;
		section_entry.virtual_address =
			prev_virtual_address + _snap(prev_virtual_size, PE_PAGE_SIZE);
		prev_virtual_address = section_entry.virtual_address;
		prev_virtual_size = section_entry.virtual_size;
	}

	// Copy COFF symbol table and string table after the last section.
	uint32_t footer_size = template_file->get_length() - old_footer_offset;
	template_file->seek(old_footer_offset);
	Vector<uint8_t> footer;
	if (footer_size > 0) {
		footer = template_file->get_buffer(footer_size);
	}

	uint32_t pe_header_offset = _get_pe_header_offset(template_file);

	// Update symbol table pointer.
	template_file->seek(pe_header_offset + 12);
	uint32_t symbols_offset = template_file->get_32();
	if (symbols_offset > resources_section_entry.pointer_to_raw_data) {
		template_file->seek(pe_header_offset + 12);
		template_file->store_32(symbols_offset + raw_size_delta);
	}

	template_file->seek(pe_header_offset + MAGIC_NUMBER_OFFSET);
	uint16_t magic_number = template_file->get_16();
	ERR_FAIL_COND_V_MSG(magic_number != 0x10b && magic_number != 0x20b, ERR_CANT_OPEN,
		vformat("Magic number has wrong value: %04x", magic_number));
	bool pe32plus = magic_number == 0x20b;

	// Update image size.
	template_file->seek(pe_header_offset + SIZE_OF_INITIALIZED_DATA_OFFSET);
	uint32_t size_of_initialized_data = template_file->get_32();
	size_of_initialized_data +=
		resources_section_entry.size_of_raw_data - old_resources_size_of_raw_data;
	template_file->seek(pe_header_offset + SIZE_OF_INITIALIZED_DATA_OFFSET);
	template_file->store_32(size_of_initialized_data);

	template_file->seek(pe_header_offset + SIZE_OF_IMAGE_OFFSET);
	uint32_t size_of_image = template_file->get_32();
	size_of_image += section_entries.get(section_entries.size() - 1).virtual_address -
					 old_last_section_virtual_address;
	template_file->seek(pe_header_offset + SIZE_OF_IMAGE_OFFSET);
	template_file->store_32(size_of_image);

	uint32_t optional_header_offset = pe_header_offset + COFF_HEADER_SIZE;

	// Update resource section size.
	template_file->seek(optional_header_offset + (pe32plus ? 132 : 116));
	template_file->store_32(resources_section_entry.virtual_size);

	// Update relocation section size and pointer.
	template_file->seek(optional_header_offset + (pe32plus ? 152 : 136));
	template_file->store_32(section_entries[relocations_index].virtual_address);
	template_file->store_32(section_entries[relocations_index].virtual_size);

	template_file->seek(
		optional_header_offset + (pe32plus ? 240 : 224) + SectionEntry::SIZE * resource_index);
	template_file->store_buffer(resources_section_entry.save());
	for (int i = resource_index + 1; i < section_entries.size(); i++) {
		template_file->seek(
			optional_header_offset + (pe32plus ? 240 : 224) + SectionEntry::SIZE * i);
		template_file->store_buffer(section_entries[i].save());
	}

	// Write new resource section.
	template_file->seek(resources_section_entry.pointer_to_raw_data);
	template_file->store_buffer(resources);
	// Write the rest of sections.
	for (const Vector<uint8_t>& data : moved_section_data) {
		template_file->store_buffer(data);
	}
	// Write footer data.
	if (footer_size > 0) {
		template_file->store_buffer(footer);
	}

	if (template_file->get_position() < original_template_size) {
		template_file->close();
		_truncate(p_template_path,
			section_entries.get(section_entries.size() - 1).pointer_to_raw_data +
				section_entries.get(section_entries.size() - 1).size_of_raw_data + footer_size);
	}

	return OK;
}

Vector<TemplateModifier::SectionEntry> TemplateModifier::_get_section_entries(
	Ref<FileAccess> p_executable) const
{
	Vector<SectionEntry> section_entries;

	uint32_t pe_header_offset = _get_pe_header_offset(p_executable);
	if (pe_header_offset == 0) {
		return section_entries;
	}

	p_executable->seek(pe_header_offset + 6);
	int num_sections = p_executable->get_16();
	p_executable->seek(pe_header_offset + 20
);
	uint16_t size_of_optional_header = p_executable->get_16();
	p_executable->seek(pe_header_offset + COFF_HEADER_SIZE + size_of_optional_header);

	for (int i = 0; i < num_sections; ++i) {
		SectionEntry section_entry;
		section_entry.load(p_executable);
		section_entries.append(section_entry);
	}

	return section_entries;
}

Error TemplateModifier::modify(const Ref<EditorExportPreset>& p_preset,
	const String& p_template_path, const String& p_icon_path)
{
	TemplateModifier template_modifier;
	return template_modifier._modify_template(p_preset, p_template_path, p_icon_path);
}

HashMap<String, String> TemplateModifier::_get_strings(Ref<EditorExportPreset> const&) const
{
	return HashMap<String, String>();
}

#ifdef TOOLS_ENABLED
#include "editor/export/register_exporters.gen.cpp"
#include "editor/translations/doc_translations.gen.cpp"
#include "editor/translations/extractable_translations.gen.cpp"
#endif
