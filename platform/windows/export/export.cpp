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
// --- Inlined from platform/windows/export/export_plugin.h ---
#include "core/os/process_id.h"
#include "editor/export/editor_export_platform.h"

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
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"

#include "logo_svg.gen.h"
#include "modules/svg/image_loader_svg.h"
#include "run_icon_svg.gen.h"
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
#include "editor/export/editor_export.h"


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


