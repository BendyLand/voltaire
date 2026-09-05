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

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "export_plugin.h"
#include "logo_svg.gen.h"
#include "modules/svg/image_loader_svg.h"
#include "run_icon_svg.gen.h"

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

Ref<Texture2D> EditorExportPlatformLinuxBSD::get_option_icon(int p_index) const
{
	if (p_index == 1) {
		return stop_icon;
	}
	else {
		return EditorExportPlatform::get_option_icon(p_index);
	}
}

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
	if (ssh_pid != 0 && OS::get_singleton()->is_process_running(ssh_pid)) {
		__print_line("Terminating connection...");
		OS::get_singleton()->kill(ssh_pid);
		OS::get_singleton()->delay_usec(1000);
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

void EditorExportPlatformLinuxBSD::initialize()
{
	if (EditorNode::get_singleton()) {
		Ref<Image> img = memnew(Image);
		const bool upsample = !Math::is_equal_approx(Math::round(EDSCALE), EDSCALE);

		ImageLoaderSVG::create_image_from_string(img, _linuxbsd_logo_svg, EDSCALE, upsample, false);
		set_logo(ImageTexture::create_from_image(img));

		ImageLoaderSVG::create_image_from_string(
			img, _linuxbsd_run_icon_svg, EDSCALE, upsample, false);
		run_icon = ImageTexture::create_from_image(img);

		Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
		if (theme.is_valid()) {
			stop_icon = theme->get_icon(SNAME("Stop"), EditorStringName(EditorIcons));
		}
		else {
			stop_icon.instantiate();
		}
	}
}


