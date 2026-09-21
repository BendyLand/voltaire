/**************************************************************************/
/*  export_template_manager.cpp                                           */
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

#include "core/config/engine.h"
#include "core/error/error_list.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/marshalls.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/progress_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "export_template_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/texture.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

<<<<<<< HEAD
void ExportTemplateManager::_request_mirrors()
{
	mirrors_list->clear();
	mirrors_empty = true;
	_update_install_button();

	// Downloadable export templates are only available for stable and official alpha/beta/RC builds
	// (which always have a number following their status, e.g. "alpha1").
	// Therefore, don't display download-related features when using a development version
	// (whose builds aren't numbered).
	if (!strcmp(VLTR_VERSION_STATUS, "dev") || !strcmp(VLTR_VERSION_STATUS, "beta") ||
		!strcmp(VLTR_VERSION_STATUS, "rc")) {
		_set_empty_mirror_list();
		mirrors_list->set_tooltip_text(
			TTRC("Official export templates aren't available for development builds."));
#ifdef REAL_T_IS_DOUBLE
	}
	else if (true) {
		_set_empty_mirror_list();
		mirrors_list->set_tooltip_text(
			TTRC("Official export templates aren't available for double-precision builds."));
#endif
	}
	else if (!_is_online()) {
		mirrors_list->set_tooltip_text(TTRC("Template downloading is disabled in offline mode."));
	}
	else {
		mirrors_list->set_tooltip_text(String());
	}

	if (mirrors_list->get_tooltip_text().is_empty()) {
		const String mirrors_metadata_url =
			vformat("https://godotengine.org/mirrorlist/%s.json", VLTR_VERSION_FULL_CONFIG);
		mirrors_requester->request(mirrors_metadata_url);
	}
}

void ExportTemplateManager::_set_empty_mirror_list()
{
	mirrors_list->add_item(TTRC("No mirrors"));
	mirrors_list->set_disabled(true);
	open_mirror->set_disabled(true);
	mirrors_empty = true;
	_update_install_button();
}

=======
>>>>>>> fix/remove-object
bool ExportTemplateManager::_is_online() const { return !offline_container->is_visible(); }

void ExportTemplateManager::_open_mirror()
{
	OS::get_singleton()->shell_open(_get_current_mirror_url());
}

void ExportTemplateManager::_delete_confirmed()
{
	_delete_file(item_to_delete);

	const String selected_version = version_list->get_item_text(version_list->get_current());
	if (selected_version != VLTR_VERSION_FULL_CONFIG) {
		// Deleting all installed templates removes the version from list.
		_update_version_list();
	}
	_update_template_tree();
	item_to_delete = nullptr;
}

void ExportTemplateManager::_delete_file(const TreeItem* p_item)
{
	if (_item_is_file(p_item)) {
		const String selected_version = version_list->get_item_text(version_list->get_current());
		const String full_path =
			_get_template_folder_path(selected_version).path_join(p_item->get_text(0));

		if (FileAccess::exists(full_path)) {
			OS::get_singleton()->move_to_trash(full_path);
		}
		file_metadata.erase(p_item->get_text(0));
	}
	else {
		for (TreeItem* child = p_item->get_first_child(); child; child = child->get_next()) {
			_delete_file(child);
		}
	}
}

void ExportTemplateManager::_initialize_template_data()
{
	// Base templates.
	{
		TemplateInfo info;
		info.name = "Windows x86_32";
		info.description = TTRC("32-bit build for Microsoft Windows, including console wrapper.");
		info.file_list = {"windows_debug_x86_32.exe", "windows_debug_x86_32_console.exe",
			"windows_release_x86_32.exe", "windows_release_x86_32_console.exe"};
		template_data[TemplateID::WINDOWS_X86_32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Windows x86_64";
		info.description = TTRC("64-bit build for Microsoft Windows, including console wrapper.");
		info.file_list = {"windows_debug_x86_64.exe", "windows_debug_x86_64_console.exe",
			"windows_release_x86_64.exe", "windows_release_x86_64_console.exe"};
		template_data[TemplateID::WINDOWS_X86_64] = info;
	}
	{
		TemplateInfo info;
		info.name = "Windows arm64";
		info.description = TTRC(
			"64-bit build for Microsoft Windows on ARM architecture, including console wrapper.");
		info.file_list = {"windows_debug_arm64.exe", "windows_debug_arm64_console.exe",
			"windows_release_arm64.exe", "windows_release_arm64_console.exe"};
		template_data[TemplateID::WINDOWS_ARM64] = info;
	}

	{
		TemplateInfo info;
		info.name = "Linux x86_32";
		info.description = TTRC("32-bit build for Linux systems.");
		info.file_list = {"linux_debug.x86_32", "linux_release.x86_32"};
		template_data[TemplateID::LINUX_X86_32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux x86_64";
		info.description = TTRC("64-bit build for Linux systems.");
		info.file_list = {"linux_debug.x86_64", "linux_release.x86_64"};
		template_data[TemplateID::LINUX_X86_64] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux arm32";
		info.description = TTRC("32-bit build for Linux systems on ARM architecture.");
		info.file_list = {"linux_debug.arm32", "linux_release.arm32"};
		template_data[TemplateID::LINUX_ARM32] = info;
	}
	{
		TemplateInfo info;
		info.name = "Linux arm64";
		info.description = TTRC("64-bit build for Linux systems on ARM architecture.");
		info.file_list = {"linux_debug.arm64", "linux_release.arm64"};
		template_data[TemplateID::LINUX_ARM64] = info;
	}

	{
		TemplateInfo info;
		info.name = "macOS";
		info.description = TTRC("Universal build for macOS.");
		info.file_list = {"macos.zip"};
		template_data[TemplateID::MACOS] = info;
	}

	{
		TemplateInfo info;
		info.name = "Web";
		info.description =
			TTRC("Regular web build with threading support. Threads improve performance, but "
				 "require \"cross-origin isolated\" website to run.");
		info.file_list = {"web_debug.zip", "web_release.zip"};
		template_data[TemplateID::WEB] = info;
	}
	{
		TemplateInfo info;
		info.name = TTR("Web with Extensions");
		info.description = TTRC("Web build with support for GDExtensions. Only useful if you use "
								"GDExtensions, otherwise it only increases build size.");
		info.file_list = {"web_dlink_debug.zip", "web_dlink_release.zip"};
		template_data[TemplateID::WEB_EXTENSIONS] = info;
	}
	{
		TemplateInfo info;
		info.name = TTR("Web Single-Threaded");
		info.description = TTRC("Web build without threading support.");
		info.file_list = {"web_nothreads_debug.zip", "web_nothreads_release.zip"};
		template_data[TemplateID::WEB_NOTHREADS] = info;
	}
	{
		TemplateInfo info;
		info.name = TTR("Web with Extensions Single-Threaded");
		info.description = TTRC("Web build with GDExtension support and no threading support.");
		info.file_list = {"web_dlink_nothreads_debug.zip", "web_dlink_nothreads_release.zip"};
		template_data[TemplateID::WEB_EXTENSIONS_NOTHREADS] = info;
	}

	{
		TemplateInfo info;
		info.name = "Android";
		info.description = TTRC("Android APK template and source for Gradle builds.");
		info.file_list = {"android_debug.apk", "android_release.apk", "android_source.zip"};
		template_data[TemplateID::ANDROID] = info;
	}

	{
		TemplateInfo info;
		info.name = "iOS";
		info.description = TTRC("Build for Apple's iOS.");
		info.file_list = {"ios.zip"};
		template_data[TemplateID::IOS] = info;
	}

	{
		TemplateInfo info;
		info.name = TTR("ICU Data");
		info.description =
			TTRC("Line breaking dictionaries for TextServer, used by certain languages.");
		info.file_list = {"icudt_godot.dat"};
		template_data[TemplateID::ICU_DATA] = info;
	}

	// Platforms.
	{
		PlatformInfo info;
		info.name = "Windows";
		info.icon = _get_platform_icon("Windows Desktop");
		info.templates = {
			TemplateID::WINDOWS_X86_32, TemplateID::WINDOWS_X86_64, TemplateID::WINDOWS_ARM64};
		info.group = TTR("Desktop", "Platform Group");
		platform_map[PlatformID::WINDOWS] = info;
	}
	{
		PlatformInfo info;
		info.name = "Linux";
		info.icon = _get_platform_icon("Linux");
		info.templates = {TemplateID::LINUX_X86_32, TemplateID::LINUX_X86_64,
			TemplateID::LINUX_ARM32, TemplateID::LINUX_ARM64};
		info.group = TTR("Desktop", "Platform Group");
		platform_map[PlatformID::LINUX] = info;
	}
	{
		PlatformInfo info;
		info.name = "macOS";
		info.icon = _get_platform_icon("macOS");
		info.templates = {TemplateID::MACOS};
		info.group = TTR("Desktop", "Platform Group");
		platform_map[PlatformID::MACOS] = info;
	}
	{
		PlatformInfo info;
		info.name = "Android";
		info.icon = _get_platform_icon("Android");
		info.templates = {TemplateID::ANDROID};
		info.group = TTR("Mobile", "Platform Group");
		platform_map[PlatformID::ANDROID] = info;
	}
	{
		PlatformInfo info;
		info.name = "iOS";
		info.icon = _get_platform_icon("iOS");
		info.templates = {TemplateID::IOS};
		info.group = TTR("Mobile", "Platform Group");
		platform_map[PlatformID::IOS] = info;
	}
	{
		PlatformInfo info;
		info.name = "Web";
		info.icon = _get_platform_icon("Web");
		info.templates = {TemplateID::WEB, TemplateID::WEB_EXTENSIONS, TemplateID::WEB_NOTHREADS,
			TemplateID::WEB_EXTENSIONS_NOTHREADS};
		info.group = TTR("Web", "Platform Group");
		platform_map[PlatformID::WEB] = info;
	}
	{
		PlatformInfo info;
		info.name = TTR("Common");
		info.templates = {TemplateID::ICU_DATA};
		platform_map[PlatformID::COMMON] = info;
	}

	// Template directory status.
	DirAccess::make_dir_recursive_absolute(_get_template_folder_path(VLTR_VERSION_FULL_CONFIG));
	_update_version_list();
}

void ExportTemplateManager::_update_template_tree()
{
	downloading_items.clear();

	const String selected_version = version_list->get_item_text(version_list->get_current());
	Ref<DirAccess> template_directory =
		DirAccess::open(_get_template_folder_path(selected_version));
	ERR_FAIL_COND(template_directory.is_null());

	bool is_current_version = (selected_version == VLTR_VERSION_FULL_CONFIG);
	HashMap<TemplateID, LocalVector<String>> installed_template_files;

	for (const KeyValue<PlatformID, PlatformInfo>& KV : platform_map) {
		for (TemplateID id : KV.value.templates) {
			for (const String& file : template_data[id].file_list) {
				if (template_directory->file_exists(file)) {
					installed_template_files[id].push_back(file);
				}
			}
		}
	}

	_fill_template_tree(available_templates_tree, installed_template_files, is_current_version);
	_fill_template_tree(installed_templates_tree, installed_template_files, is_current_version);
}

<<<<<<< HEAD
void ExportTemplateManager::_update_install_button()
{
	if (is_downloading()) {
		install_button->set_text(TTRC("Downloading templates..."));
		install_button->set_disabled(true);
		install_button->set_tooltip_text(String());
		return;
	}

	download_all_enabled = true;
	for (TreeItem* item = available_templates_tree->get_root(); item;
		 item = item->get_next_in_tree()) {
		if (item->is_checked(0)) {
			download_all_enabled = false;
			break;
		}
	}
	if (download_all_enabled) {
		install_button->set_text(TTRC("Install All Templates"));
	}
	else {
		install_button->set_text(TTRC("Install Selected Templates"));
	}

	install_button->set_disabled(!_can_download_templates());
	if (install_button->is_disabled()) {
		if (!_is_online()) {
			install_button->set_tooltip_text(TTRC("Download not available in offline mode."));
		}
		else if (mirrors_empty) {
			install_button->set_tooltip_text(TTRC("No mirrors available for download."));
		}
		else {
			install_button->set_tooltip_text(
				TTRC("Downloads are only available for the current Godot version."));
		}
	}
	else {
		install_button->set_tooltip_text(String());
	}
}

=======
>>>>>>> fix/remove-object
bool ExportTemplateManager::_can_download_templates()
{
	const String selected_version = version_list->get_item_text(version_list->get_current());
	return !mirrors_empty && _is_online() && selected_version == VLTR_VERSION_FULL_CONFIG;
}

void ExportTemplateManager::_update_folding_cache(TreeItem* p_item)
{
	folding_cache[_get_item_path(p_item)] = p_item->is_collapsed();
	if (p_item->get_cell_mode(0) == TreeItem::CELL_MODE_CHECK) {
		if (p_item->is_indeterminate(0)) {
			checked_cache[_get_item_path(p_item)] = 1;
		}
		else {
			checked_cache[_get_item_path(p_item)] = p_item->is_checked(0) ? 2 : 0;
		}
	}
	for (TreeItem* child = p_item->get_first_child(); child; child = child->get_next()) {
		_update_folding_cache(child);
	}
}

String ExportTemplateManager::_get_template_folder_path(const String& p_version) const
{
	return EditorPaths::get_singleton()->get_export_templates_dir().path_join(p_version);
}

Ref<Texture2D> ExportTemplateManager::_get_platform_icon(const String& p_platform_name)
{
	for (int i = 0; i < EditorExport::get_singleton()->get_export_platform_count(); i++) {
		Ref<EditorExportPlatform> platform = EditorExport::get_singleton()->get_export_platform(i);
		if (platform->get_name() == p_platform_name) {
			return platform->get_logo();
		}
	}
	return Ref<Texture2D>();
}

void ExportTemplateManager::_version_selected()
{
	file_metadata.clear();
	_update_template_tree();
	_update_install_button();
}

void ExportTemplateManager::_tree_button_clicked(
	TreeItem* p_item, int p_column, int p_id, MouseButton p_button)
{
	switch ((ButtonID)p_id) {
	case ButtonID::FAIL: {
		FileMetadata* meta = _get_file_metadata(p_item);
		EditorNode::get_singleton()->show_warning(meta->fail_reason + ".", TTR("Download Failed"));
	} break;

	case ButtonID::NONE: {
	} break;
	}
}

void ExportTemplateManager::_tree_item_edited()
{
	TreeItem* edited = available_templates_tree->get_edited();
	ERR_FAIL_NULL(edited);

	edited->propagate_check(0, false);
	_update_install_button();
}

void ExportTemplateManager::_open_template_directory()
{
	const String selected_version = version_list->get_item_text(version_list->get_current());
	OS::get_singleton()->shell_show_in_file_manager(
		_get_template_folder_path(selected_version), true);
}

void ExportTemplateManager::_queue_download_tree_item(TreeItem* p_item)
{
	if (_item_is_file(p_item)) {
		bool valid;
		bool is_installed_tree = p_item->get_tree() == installed_templates_tree;
		if (is_installed_tree) {
			FileMetadata* meta = _get_file_metadata(p_item);
			valid = meta->is_missing;
		}
		else {
			valid = download_all_enabled || p_item->is_checked(0);
		}

		if (valid) {
			queued_files.insert(p_item->get_text(0));
			if (!is_installed_tree) {
				queued_templates.insert(p_item->get_parent()->get_text(0));
			}
		}
	}
	else {
		for (TreeItem* child = p_item->get_first_child(); child; child = child->get_next()) {
			_queue_download_tree_item(child);
		}
	}
}

<<<<<<< HEAD
void ExportTemplateManager::_process_download_queue()
{
	queue_update_pending = false;

	int downloader_index = 0;
	bool is_finished = true;
	for (TreeItem* item : downloading_items) {
		FileMetadata* meta = _get_file_metadata(item);

		is_finished = is_finished && _status_is_finished(meta->download_status);
		if (meta->download_status != DownloadStatus::PENDING) {
			continue;
		}

		TemplateDownloader* downloader = _get_available_downloader(&downloader_index);
		if (!downloader) {
			break;
		}
		downloader_index++;

		Error err = downloader->download_template(item->get_text(0), _get_current_mirror_url());
		if (err == OK) {
			meta->download_status = DownloadStatus::IN_PROGRESS;
			meta->downloader = downloader;
		}
		else {
			_item_download_failed(
				item, vformat(TTR("Download request failed: %s."), TTR(error_names[err])));
		}
	}

	if (is_finished) {
		// Exit "downloading mode".
		queued_templates.clear();
		downloading_items.clear();
		set_process_internal(false);
		_update_install_button();
		EditorNode::get_bottom_panel()->get_progress_indicator()->hide();

		for (int i = 0; i < version_list->get_item_count(); i++) {
			version_list->set_item_disabled(i, false);
		}
	}
	else {
		set_process_internal(true);
	}
}

=======
>>>>>>> fix/remove-object
TemplateDownloader* ExportTemplateManager::_get_available_downloader(int* r_from_index)
{
	int counter = -1;
	for (TemplateDownloader* downloader : downloaders) {
		counter++;
		if (counter < *r_from_index) {
			continue;
		}
		if (!downloader->is_downloading()) {
			*r_from_index = counter;
			return downloader;
		}
	}
	return nullptr;
}

void ExportTemplateManager::_download_request_completed(const String& p_filename)
{
	bool found = false;
	bool template_finished = false;

	queued_files.erase(p_filename);
	for (TreeItem* item : downloading_items) {
		if (item->get_text(0) != p_filename) {
			continue;
		}
		item->clear_buttons();

		FileMetadata* meta = _get_file_metadata(p_filename);
		meta->downloader = nullptr;
		meta->download_status = DownloadStatus::COMPLETED;
		meta->is_missing = false;

		found = true;
		template_finished = _is_template_download_finished(item->get_parent());
		if (template_finished) {
			queued_templates.erase(item->get_parent()->get_text(0));
		}
		break;
	}
	if (!found) {
		ERR_FAIL_COND(!found);
	}
	_queue_process_download_queue();

	if (template_finished) {
		_update_template_tree();
	}
}

bool ExportTemplateManager::_is_template_download_finished(TreeItem* p_template)
{
	for (TreeItem* child = p_template->get_first_child(); child; child = child->get_next()) {
		if (!downloading_items.has(child)) {
			continue;
		}
		FileMetadata* meta = _get_file_metadata(child);
		if (!_status_is_finished(meta->download_status)) {
			return false;
		}
	}
	return true;
}

void ExportTemplateManager::_apply_item_folding(TreeItem* p_item, bool p_default)
{
	if (folding_cache.is_empty()) {
		if (p_default) {
			p_item->set_collapsed(true);
		}
	}
	else {
		bool* cached = folding_cache.getptr(_get_item_path(p_item));
		if (cached) {
			p_item->set_collapsed(*cached);
		}
		else if (p_default) {
			p_item->set_collapsed(true);
		}
	}
}

<<<<<<< HEAD
void ExportTemplateManager::_cancel_item_download(TreeItem* p_item)
{
	_item_download_failed(p_item, TTR("Canceled by the user"));
	queued_files.erase(p_item->get_text(0));

	FileMetadata* meta = _get_file_metadata(p_item);
	if (meta->downloader) {
		meta->downloader->cancel_download();
		meta->downloader = nullptr;
	}
}

void ExportTemplateManager::_item_download_failed(TreeItem* p_item, const String& p_reason)
{
	FileMetadata* meta = _get_file_metadata(p_item);
	meta->fail_reason = p_reason;
	meta->download_status = DownloadStatus::FAILED;

	p_item->clear_buttons();
	_add_fail_reason_button(p_item);
}

void ExportTemplateManager::_add_fail_reason_button(TreeItem* p_item, const String& p_filename)
{
	FileMetadata* meta =
		_get_file_metadata(p_filename.is_empty() ? p_item->get_text(0) : p_filename);
	p_item->add_button(0, theme_cache.failure_icon, (int)ButtonID::FAIL);
	p_item->set_button_tooltip_text(
		0, -1, vformat(TTR("Download failed.\nReason: %s."), meta->fail_reason));
}

=======
>>>>>>> fix/remove-object
ExportTemplateManager::FileMetadata* ExportTemplateManager::_get_file_metadata(
	const String& p_text) const
{
	FileMetadata* meta = file_metadata.getptr(p_text);
	if (likely(meta)) {
		return meta;
	}
	HashMap<String, FileMetadata>::Iterator it = file_metadata.insert(p_text, FileMetadata());
	return &it->value;
}

ExportTemplateManager::FileMetadata* ExportTemplateManager::_get_file_metadata(
	const TreeItem* p_item) const
{
	return _get_file_metadata(p_item->get_text(0));
}

float ExportTemplateManager::_get_download_progress(const TreeItem* p_item) const
{
	FileMetadata* meta = _get_file_metadata(p_item);
	switch (meta->download_status) {
	case DownloadStatus::NONE:
	case DownloadStatus::PENDING: {
		return 0.0;
	}

	case DownloadStatus::IN_PROGRESS: {
		if (!meta->downloader) {
			return 0.0;
		}
		return meta->downloader->get_download_progress();
	}

	case DownloadStatus::COMPLETED: {
		return 1.0;
	}

	case DownloadStatus::FAILED: {
		return meta->progress_cache;
	}
	}
	return 0.0;
}

void ExportTemplateManager::_draw_item_progress(TreeItem* p_item, const Rect2& p_rect)
{
	Tree* owning_tree = p_item->get_tree();
	RID ci = owning_tree->get_custom_drawing_canvas_item();
	RS::get_singleton()->canvas_item_add_rect(ci, p_rect, Color(0, 0, 0, 0.5));

	if (!_item_is_file(p_item)) {
		float progress = 0.0;
		int item_count = 0;

		bool has_fail = false;
		for (TreeItem* child = p_item->get_first_child(); child; child = child->get_next()) {
			if (!downloading_items.has(child)) {
				continue;
			}
			item_count++;
			progress += _get_download_progress(child);

			FileMetadata* meta = _get_file_metadata(child);
			has_fail = has_fail || meta->download_status == DownloadStatus::FAILED;
		}
		progress /= item_count;
		RS::get_singleton()->canvas_item_add_rect(ci,
			Rect2(p_rect.position, Vector2(p_rect.size.x * progress, p_rect.size.y)),
			has_fail ? theme_cache.download_failed_color : theme_cache.download_progress_color);
		return;
	}

	FileMetadata* meta = _get_file_metadata(p_item);
	switch (meta->download_status) {
	case DownloadStatus::NONE: {
	} break;

	case DownloadStatus::PENDING: {
		uint64_t frame = Engine::get_singleton()->get_frames_drawn();
		const Ref<Texture2D> progress_texture = theme_cache.progress_icons[frame / 4 % 8];
		const Rect2 rect = Rect2(
			Vector2(p_rect.get_end().x - progress_texture->get_width(),
				p_rect.position.y + p_rect.size.y * 0.5 - progress_texture->get_height() * 0.5),
			progress_texture->get_size());
		RS::get_singleton()->canvas_item_add_texture_rect(ci, rect, progress_texture->get_rid());
	} break;

	case DownloadStatus::IN_PROGRESS: {
		float progress = _get_download_progress(p_item);
		meta->progress_cache = progress;
		RS::get_singleton()->canvas_item_add_rect(ci,
			Rect2(p_rect.position, Vector2(p_rect.size.x * progress, p_rect.size.y)),
			theme_cache.download_progress_color);
	} break;

	case DownloadStatus::COMPLETED: {
		RS::get_singleton()->canvas_item_add_rect(ci, p_rect, theme_cache.download_progress_color);
	} break;

	case DownloadStatus::FAILED: {
		RS::get_singleton()->canvas_item_add_rect(ci,
			Rect2(p_rect.position,
				Vector2(p_rect.size.x * _get_download_progress(p_item), p_rect.size.y)),
			theme_cache.download_failed_color);
	} break;
	}
}

bool ExportTemplateManager::is_android_template_installed(const Ref<EditorExportPreset>& p_preset)
{
	return DirAccess::exists(get_android_build_directory(p_preset));
}

bool ExportTemplateManager::can_install_android_template(const Ref<EditorExportPreset>& p_preset)
{
	return FileAccess::exists(get_android_source_zip(p_preset));
}

bool ExportTemplateManager::is_downloading() const { return !queued_files.is_empty(); }

<<<<<<< HEAD
void ExportTemplateManager::stop_download()
{
	for (TreeItem* item : downloading_items) {
		FileMetadata* meta = _get_file_metadata(item);
		if (meta && !_status_is_finished(meta->download_status)) {
			_cancel_item_download(item);
		}
	}
}

=======
>>>>>>> fix/remove-object
int TemplateDownloader::_find_sequence_backwards(
	const PackedByteArray& p_source, const PackedByteArray& p_target) const
{
	const int64_t source_size = p_source.size();
	const int64_t target_size = p_target.size();

	if (target_size == 0) {
		return -1;
	}
	if (target_size > source_size) {
		return -1;
	}
	const uint8_t* src_ptr = p_source.ptr();
	const uint8_t* tgt_ptr = p_target.ptr();

	for (int64_t i = source_size - target_size; i >= 0; i--) {
		if (memcmp(&src_ptr[i], tgt_ptr, target_size) == 0) {
			return (int)i;
		}
	}
	return -1;
}

String TemplateDownloader::_get_download_error(int p_result, int p_response_code) const
{
	switch (p_result) {
	case HTTPRequest::RESULT_CANT_RESOLVE:
		return TTR("Can't resolve the requested address");
	case HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED:
	case HTTPRequest::RESULT_CONNECTION_ERROR:
	case HTTPRequest::RESULT_CHUNKED_BODY_SIZE_MISMATCH:
	case HTTPRequest::RESULT_TLS_HANDSHAKE_ERROR:
	case HTTPRequest::RESULT_CANT_CONNECT:
		return TTR("Can't connect to the mirror");
	case HTTPRequest::RESULT_NO_RESPONSE:
		return TTR("No response from the mirror");
	case HTTPRequest::RESULT_TIMEOUT:
		return TTR("Request timed out");
	case HTTPRequest::RESULT_REQUEST_FAILED:
		return TTR("Request failed");
	case HTTPRequest::RESULT_REDIRECT_LIMIT_REACHED:
		return TTR("Request ended up in a redirect loop");
	}

	switch (p_response_code) {
	case HTTPClient::RESPONSE_FORBIDDEN:
		return TTR("Forbidden");
	case HTTPClient::RESPONSE_NOT_FOUND:
		return TTR("Not found");
	default: // Handle only common errors.
		return vformat(TTR("Response code: %d"), p_response_code);
	}
}

void TemplateDownloader::_request_completed(int p_result, int p_response_code,
	const PackedStringArray& p_headers, const PackedByteArray& p_body)
{
	switch (current_step) {
	case Step::WAITING: {
		_download_failed(
			String()); // Not really possible to happen, so just fail with empty message.
		ERR_FAIL_MSG("Request completed on wrong step.");
	} break;
	}
}

bool TemplateDownloader::_is_retryable_result(int p_result, int p_response_code) const
{
	switch (p_result) {
	case HTTPRequest::RESULT_CONNECTION_ERROR:
	case HTTPRequest::RESULT_CHUNKED_BODY_SIZE_MISMATCH:
	case HTTPRequest::RESULT_TLS_HANDSHAKE_ERROR:
	case HTTPRequest::RESULT_CANT_CONNECT:
	case HTTPRequest::RESULT_NO_RESPONSE:
	case HTTPRequest::RESULT_TIMEOUT:
		return true;
	}

	return p_result == HTTPRequest::RESULT_SUCCESS &&
		   (p_response_code == HTTPClient::RESPONSE_REQUEST_TIMEOUT ||
			   p_response_code == HTTPClient::RESPONSE_TOO_MANY_REQUESTS ||
			   p_response_code == HTTPClient::RESPONSE_INTERNAL_SERVER_ERROR ||
			   p_response_code == HTTPClient::RESPONSE_BAD_GATEWAY ||
			   p_response_code == HTTPClient::RESPONSE_SERVICE_UNAVAILABLE ||
			   p_response_code == HTTPClient::RESPONSE_GATEWAY_TIMEOUT);
}

int64_t TemplateDownloader::_get_fragment_download_size() const
{
	if (fragment_end_byte < fragment_start_byte) {
		return 0;
	}
	return fragment_end_byte - fragment_start_byte + 1;
}

int64_t TemplateDownloader::_get_partial_download_size() const
{
	if (partial_download_path.is_empty() || !FileAccess::exists(partial_download_path)) {
		return 0;
	}

	Ref<FileAccess> f = FileAccess::open(partial_download_path, FileAccess::READ);
	if (f.is_null()) {
		return 0;
	}
	return MIN((int64_t)f->get_length(), _get_fragment_download_size());
}

void TemplateDownloader::_clear_partial_download()
{
	if (!partial_download_path.is_empty() && FileAccess::exists(partial_download_path)) {
		DirAccess::remove_absolute(partial_download_path);
	}
}

<<<<<<< HEAD
Error TemplateDownloader::_request_file_fragment()
{
	const int64_t fragment_size = _get_fragment_download_size();
	if (fragment_size <= 0) {
		return ERR_INVALID_DATA;
	}

	int64_t partial_size = _get_partial_download_size();
	if (partial_size >= fragment_size) {
		_download_completed();
		return OK;
	}

	request_start_partial_size = partial_size;
	const int64_t request_start_byte = fragment_start_byte + partial_size;
	const String data_range = vformat("Range: bytes=%d-%d", request_start_byte, fragment_end_byte);

	set_download_file(partial_download_path);
	set_keep_partial_download(true);
	set_append_to_download_file(partial_size > 0);
	return request(url, PackedStringArray{data_range}, HTTPClient::METHOD_GET);
}

bool TemplateDownloader::_retry_file_fragment(const String& p_reason)
{
	if (retry_count >= MAX_DOWNLOAD_RETRIES) {
		_download_failed(
			vformat(TTR("%s. Download failed after %d retries."), p_reason, MAX_DOWNLOAD_RETRIES));
		return false;
	}

	retry_count++;
	Error err = _request_file_fragment();
	if (err != OK) {
		_download_failed(vformat(TTR("Download request failed: %s."), TTR(error_names[err])));
		return false;
	}
	return true;
}

void TemplateDownloader::_bind_methods() {}

Error TemplateDownloader::download_template(const String& p_file_name, const String& p_source)
{
	url = p_source;
	filename = p_file_name;
	partial_download_path = EditorPaths::get_singleton()->get_temp_dir().path_join(
		(filename + "-" + url).md5_text() + "-" + filename.validate_filename() + ".part");
	_clear_partial_download();

	set_download_file(String());
	set_keep_partial_download(false);
	set_append_to_download_file(false);
	request_start_partial_size = 0;
	retry_count = 0;
	range_restart_attempted = false;
	current_step = Step::QUERYING;
	return request(p_source, PackedStringArray(), HTTPClient::METHOD_HEAD);
}

=======
>>>>>>> fix/remove-object
void TemplateDownloader::cancel_download()
{
	cancel_request();
	_clear_partial_download();

	current_step = Step::WAITING;
	filename = String();
	url = String();
	partial_download_path = String();
	file_size = 0;
	file_info = FileInfo();
	fragment_start_byte = 0;
	fragment_end_byte = 0;
	request_start_partial_size = 0;
	retry_count = 0;
	range_restart_attempted = false;
}

float TemplateDownloader::get_download_progress() const
{
	if (current_step == Step::DOWNLOADING) {
		const int64_t fragment_size = _get_fragment_download_size();
		if (fragment_size <= 0) {
			return 0.0f;
		}
		const int64_t downloaded_size =
			MIN(request_start_partial_size + get_downloaded_bytes(), fragment_size);
		return (float)downloaded_size / (float)fragment_size;
	}
	return 0.0f;
}

String ExportTemplateManager::get_android_build_directory(const Ref<EditorExportPreset>& p_preset)
{
	return String();
}

String ExportTemplateManager::_get_item_path(TreeItem*) const {}

bool ExportTemplateManager::_item_is_file(TreeItem const*) const {}

void ExportTemplateManager::_update_version_list() {}

void ExportTemplateManager::_update_install_button() {}

void ExportTemplateManager::_queue_process_download_queue() {}

void ExportTemplateManager::_fill_template_tree(Tree* p_tree,
	const HashMap<TemplateID, LocalVector<String>>& p_installed_template_files,
	bool p_is_current_version)
{
}

String ExportTemplateManager::_get_current_mirror_url() const {}

String ExportTemplateManager::get_android_source_zip(Ref<EditorExportPreset> const&) {}

void TemplateDownloader::_download_failed(const String& p_reason) {}


