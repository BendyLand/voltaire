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

#include "editor/export/editor_export.h"
#include "editor/settings/editor_settings.h"
#include "export.h"
// --- Inlined from platform/macos/export/export_plugin.h ---
#include <sys/stat.h>
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/process_id.h"
#include "editor/export/editor_export_platform.h"
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
#include "core/io/plist.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "drivers/png/png_driver_common.h"
#include "editor/editor_string_names.h"
#include "editor/export/apple_export_utils.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"

#include "logo_svg.gen.h"
#include "modules/svg/image_loader_svg.h"
#include "run_icon_svg.gen.h"
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


