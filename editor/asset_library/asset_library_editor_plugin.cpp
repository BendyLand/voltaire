/**************************************************************************/
/*  asset_library_editor_plugin.cpp                                       */
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

#include "asset_library_editor_plugin.h"
#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/json.h"
#include "core/io/stream_peer_tls.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/project_manager/project_manager.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

void EditorAssetLibraryItem::set_image(int p_type, int p_index, const Ref<Texture2D>& p_image)
{
	ERR_FAIL_COND(p_type != EditorAssetLibrary::IMAGE_QUEUE_THUMBNAIL);
	ERR_FAIL_COND(p_index != 0);

	icon->set_texture(p_image);
}

void EditorAssetLibraryItem::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		icon->set_texture(get_editor_theme_icon(SNAME("AssetThumbLoading")));
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		_calculate_misc_links_size();
	} break;
	}
}

void EditorAssetLibraryItem::_author_clicked()
{
	OS::get_singleton()->shell_open(
		"https://store.godotengine.org/publisher/" + author_id.uri_encode() + "/");
}

void EditorAssetLibraryItem::_license_clicked()
{
	ERR_FAIL_COND(!license_url.begins_with("http"));
	OS::get_singleton()->shell_open(license_url);
}

Control* EditorAssetLibraryZoomMode::remove_previews()
{
	ERR_FAIL_NULL_V(previews, nullptr);

	remove_child(previews);
	return previews;
}

void EditorAssetLibraryZoomMode::input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouse> m = p_event;
	if (m.is_valid()) {
		return;
	}

	if (p_event->is_action_pressed(SNAME("ui_cancel"))) {
		hide();
	}

	// Block inputs from going elsewhere.
	get_tree()->get_root()->set_input_as_handled();
}

void EditorAssetLibraryItemDescription::_store_pressed()
{
	OS::get_singleton()->shell_open(store_url);
}

void EditorAssetLibraryItemDescription::_source_pressed()
{
	OS::get_singleton()->shell_open(source_url);
}

void EditorAssetLibraryItemDescription::_zoom_toggled(bool p_pressed)
{
	if (p_pressed) {
		root->remove_child(previews_vbox);
		zoom_mode = memnew(EditorAssetLibraryZoomMode(previews_vbox));
		get_tree()->get_root()->add_child(zoom_mode);
		hide();
	}
	else {
		root->add_child(zoom_mode->remove_previews());
		zoom_mode->queue_free();
		zoom_mode = nullptr;

		show();
	}
}

void EditorAssetLibraryItemDownload::_close()
{
	// Clean up downloaded file.
	DirAccess::remove_file_or_error(download->get_download_file());
	queue_free();
}

bool EditorAssetLibraryItemDownload::can_install() const { return install_button->is_visible(); }

void EditorAssetLibrary::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		add_theme_style_override(
			SceneStringName(panel), get_theme_stylebox(SNAME("bg"), SNAME("AssetLib")).ptr());
		error_label->move_to_front();
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
#ifndef ANDROID_ENABLED
			// Focus the search box automatically when switching to the Templates tab (in the
			// Project Manager) or switching to the AssetLib tab (in the editor). The Project
			// Manager's project filter box is automatically focused in the project manager code.
			filter->grab_focus();
#endif

			if (initial_loading) {
				_repository_changed(0); // Update when shown for the first time.
			}
		}
	} break;

	case NOTIFICATION_RESIZED: {
		_update_asset_items_columns();
	} break;

	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (EditorSettings::get_singleton()->check_changed_settings_in_group(
				"asset_store/use_threads") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"network/http_proxy")) {
		}

		if (EditorSettings::get_singleton()->check_changed_settings_in_group(
				"asset_store/available_urls")) {
			_update_repository_options();

			if (!loading_blocked && is_visible()) {
				_request_current_config();
			}
		}
	} break;
	}
}

const char* EditorAssetLibrary::sort_key[SORT_MAX] = {
	"relevance",
	"updated_desc",
	"updated_asc",
	"reviews_desc",
	"reviews_asc",
	"created_desc",
	"created_asc",
};

const char* EditorAssetLibrary::sort_text[SORT_MAX] = {
	TTRC("Relevance"),
	TTRC("Updated (Newest First)"),
	TTRC("Updated (Oldest First)"),
	TTRC("Reviews (Highest Score First)"),
	TTRC("Reviews (Lowest Score First)"),
	TTRC("Created (Newest First)"),
	TTRC("Created (Oldest First)"),
};

void EditorAssetLibrary::_image_update(void* p_image_queue)
{
	ImageQueue* iq = static_cast<ImageQueue*>(p_image_queue);
	PackedByteArray image_data = iq->data;

	if (iq->use_cache) {
		String cache_filename_base = EditorPaths::get_singleton()->get_cache_dir().path_join(
			"assetimage_" + iq->image_url.md5_text());

		Ref<FileAccess> file = FileAccess::open(cache_filename_base + ".data", FileAccess::READ);
		if (file.is_valid()) {
			PackedByteArray cached_data;
			int len = file->get_32();
			cached_data.resize(len);

			uint8_t* w = cached_data.ptrw();
			file->get_buffer(w, len);

			image_data = cached_data;
		}
	}

	int len = image_data.size();
	const uint8_t* r = image_data.ptr();
	Ref<Image> image = memnew(Image);

	uint8_t png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
	uint8_t jpg_signature[3] = {255, 216, 255};
	uint8_t webp_signature[4] = {82, 73, 70, 70};
	uint8_t bmp_signature[2] = {66, 77};

	if (r) {
		Ref<Image> parsed_image;

		if ((memcmp(&r[0], &png_signature[0], 8) == 0) && Image::_png_mem_loader_func) {
			parsed_image = Image::_png_mem_loader_func(r, len);
		}
		else if ((memcmp(&r[0], &jpg_signature[0], 3) == 0) && Image::_jpg_mem_loader_func) {
			parsed_image = Image::_jpg_mem_loader_func(r, len);
		}
		else if ((memcmp(&r[0], &webp_signature[0], 4) == 0) && Image::_webp_mem_loader_func) {
			parsed_image = Image::_webp_mem_loader_func(r, len);
		}
		else if ((memcmp(&r[0], &bmp_signature[0], 2) == 0) && Image::_bmp_mem_loader_func) {
			parsed_image = Image::_bmp_mem_loader_func(r, len);
		}

		if (parsed_image.is_null()) {
			if (is_print_verbose_enabled()) {
				ERR_PRINT(vformat("Asset Store: Invalid image downloaded from '%s' for asset # %d",
					iq->image_url, iq->asset_id));
			}
		}
		else {
			image->copy_internals_from(parsed_image);
		}
	}

	if (!image->is_empty()) {
		Size2 max_size;
		switch (iq->image_type) {
		case IMAGE_QUEUE_THUMBNAIL:
		case IMAGE_QUEUE_VIDEO_THUMBNAIL: {
			max_size = THUMBNAIL_SIZE;
		} break;

		case IMAGE_QUEUE_SCREENSHOT: {
			max_size.y = image->get_height();
		} break;
		}

		float scale_ratio = max_size.y / image->get_height();
		if (max_size.x > 0) {
			scale_ratio = MIN(scale_ratio, max_size.x / image->get_width());
		}
		if (scale_ratio < 1) {
			image->resize(image->get_width() * scale_ratio * EDSCALE,
				image->get_height() * scale_ratio * EDSCALE, Image::INTERPOLATE_LANCZOS);
		}

		iq->texture = ImageTexture::create_from_image(image);
	}

	iq->update_finished = true;
}

void EditorAssetLibrary::_licenses_id_pressed(int p_id)
{}

void EditorAssetLibrary::_request_current_config()
{
	_repository_changed(repository->get_selected());
}

void EditorAssetLibrary::_asset_open() { asset_open->popup_file_dialog(); }

void EditorAssetLibrary::_manage_plugins()
{
	ProjectSettingsEditor::get_singleton()->popup_project_settings(true);
	ProjectSettingsEditor::get_singleton()->set_plugins_page();
}

void EditorAssetLibrary::_update_asset_items_columns()
{
	if (!asset_items) {
		return;
	}

	int new_columns = get_size().x / (450.0 * EDSCALE);
	new_columns = MAX(1, new_columns);

	if (new_columns != asset_items->get_columns()) {
		asset_items->set_columns(new_columns);
	}
}

bool AssetLibraryEditorPlugin::is_available()
{
#ifdef WEB_ENABLED
	// Asset Store can't work on Web editor for now as most assets are sourced
	// directly from GitHub which does not set CORS.
	return false;
#else
	return StreamPeerTLS::is_available() && !Engine::get_singleton()->is_recovery_mode_hint();
#endif
}


