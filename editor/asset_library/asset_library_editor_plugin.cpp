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

	case NOTIFICATION_THEME_CHANGED: {
		author->add_theme_color_override(
			SceneStringName(font_color), get_theme_color(SNAME("faded_text"), SNAME("AssetLib")));
		license->add_theme_color_override(
			SceneStringName(font_color), get_theme_color(SNAME("faded_text"), SNAME("AssetLib")));
		verified->set_texture(get_editor_theme_icon(SNAME("Verified")));
		rating_icon->set_texture(get_editor_theme_icon(SNAME("ThumbsUp")));

		_calculate_misc_links_size();
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

EditorAssetLibraryItem::EditorAssetLibraryItem(bool p_clickable)
{
	is_clickable = p_clickable;
	if (p_clickable) {
		button = memnew(Button);
		button->set_theme_type_variation(SceneStringName(FlatButton));
		add_child(button);
	}

	margin = memnew(MarginContainer);
	int margin_size = 5 * EDSCALE;
	margin->add_theme_constant_override(SNAME("margin_left"), margin_size);
	margin->add_theme_constant_override(SNAME("margin_right"), margin_size);
	margin->add_theme_constant_override(SNAME("margin_top"), margin_size);
	margin->add_theme_constant_override(SNAME("margin_bottom"), margin_size);
	margin->set_mouse_filter(MOUSE_FILTER_IGNORE);
	margin->set_clip_contents(true);
	add_child(margin);

	HBoxContainer* hb = memnew(HBoxContainer);
	// Add some spacing to visually separate the icon from the asset details.
	hb->add_theme_constant_override("separation", 15 * EDSCALE);
	hb->set_mouse_filter(MOUSE_FILTER_IGNORE);
	margin->add_child(hb);

	icon = memnew(TextureRect);
	icon->set_accessibility_name(TTRC("Thumbnail"));
	icon->set_custom_minimum_size(EditorAssetLibrary::THUMBNAIL_SIZE * EDSCALE);
	icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	icon->set_mouse_filter(MOUSE_FILTER_IGNORE);
	hb->add_child(icon);

	text_margin = memnew(MarginContainer);
	text_margin->add_theme_constant_override(SNAME("margin_left"), margin_size);
	text_margin->add_theme_constant_override(SNAME("margin_right"), margin_size);
	text_margin->add_theme_constant_override(SNAME("margin_top"), margin_size);
	text_margin->add_theme_constant_override(SNAME("margin_bottom"), margin_size);
	text_margin->set_h_size_flags(SIZE_EXPAND_FILL);
	text_margin->set_mouse_filter(MOUSE_FILTER_IGNORE);
	text_margin->set_clip_contents(true);
	hb->add_child(text_margin);

	VBoxContainer* vb = memnew(VBoxContainer);
	vb->set_mouse_filter(MOUSE_FILTER_IGNORE);
	vb->set_h_size_flags(SIZE_EXPAND_FILL);
	text_margin->add_child(vb);

	Ref<StyleBoxEmpty> label_margin;
	label_margin.instantiate();
	label_margin->set_content_margin_all(0);

	title = memnew(Label);
	title->set_accessibility_name(TTRC("Title"));
	title->set_auto_translate_mode(AutoTranslateMode::AUTO_TRANSLATE_MODE_DISABLED);
	title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	title->set_mouse_filter(MOUSE_FILTER_IGNORE);
	title->set_focus_mode(FOCUS_ACCESSIBILITY);
	vb->add_child(title);

	author_license_hbox = memnew(HBoxContainer);
	author_license_hbox->add_theme_constant_override("separation", 5 * EDSCALE);
	author_license_hbox->set_mouse_filter(MOUSE_FILTER_IGNORE);
	vb->add_child(author_license_hbox);

	author = memnew(LinkButton);
	author->set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);
	author->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	author->set_tooltip_text(TTRC("Author"));
	author->set_accessibility_name(TTRC("Author"));
	author->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	author_license_hbox->add_child(author);

	verified = memnew(TextureRect);
	verified->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	verified->set_tooltip_text(TTRC("Verified Author"));
	author_license_hbox->add_child(verified);

	separator = memnew(HSeparator);
	separator->set_mouse_filter(MOUSE_FILTER_IGNORE);
	author_license_hbox->add_child(separator);

	license = memnew(LinkButton);
	license->set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);
	license->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	license->set_tooltip_text(TTRC("License"));
	license->set_accessibility_name(TTRC("License"));
	license->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	author_license_hbox->add_child(license);

	// Ensure the entire asset card can be clicked.
	Control* spacer = vb->add_spacer();
	spacer->set_mouse_filter(MOUSE_FILTER_IGNORE);

	HBoxContainer* rating_hbox = memnew(HBoxContainer);
	rating_hbox->set_mouse_filter(MOUSE_FILTER_IGNORE);
	vb->add_child(rating_hbox);

	rating_icon = memnew(TextureRect);
	rating_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	rating_icon->set_mouse_filter(MOUSE_FILTER_IGNORE);
	rating_hbox->add_child(rating_icon);

	rating_count = memnew(Label);
	rating_count->set_theme_type_variation("LabelNoMargin");
	rating_count->set_accessibility_name(TTRC("Review Score"));
	rating_hbox->add_child(rating_count);

	set_accessibility_name(TTRC("Open Asset Details"));
	set_custom_minimum_size(Size2(250, 80) * EDSCALE);
	set_h_size_flags(SIZE_EXPAND_FILL);
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

EditorAssetLibraryZoomMode::EditorAssetLibraryZoomMode(Control* p_previews)
{
	ERR_FAIL_NULL(p_previews);
	ERR_FAIL_COND(p_previews->get_parent());

	Ref<Theme> theme;
	if (EditorNode::get_singleton()) {
		theme = EditorNode::get_singleton()->get_editor_theme();
	}
	else if (ProjectManager::get_singleton()) {
		theme = ProjectManager::get_singleton()->get_theme();
	}
	else {
		return;
	}

	ColorRect* dim = memnew(ColorRect);
	dim->set_color(theme->get_color(SNAME("base_color"), EditorStringName(Editor)));
	dim->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(dim);

	previews = p_previews;
	add_child(previews);
	p_previews->set_anchors_and_offsets_preset(
		Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 40 * EDSCALE);

	set_process_input(true);
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

void EditorAssetLibraryItemDownload::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("AssetLib")).ptr());
		version->add_theme_color_override(
			SceneStringName(font_color), get_theme_color(SNAME("faded_text"), SNAME("AssetLib")));
		dismiss_button->set_texture_normal(get_theme_icon(SNAME("dismiss"), SNAME("AssetLib")));
		spacer->set_custom_minimum_size(Size2(0, 8 * EDSCALE));

		Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Button"));
		int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Button"));
	} break;
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

	case NOTIFICATION_THEME_CHANGED: {
		error_tr->set_texture(get_editor_theme_icon(SNAME("Error")));
		filter->set_right_icon(get_editor_theme_icon(SNAME("Search")));
		library_scroll->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
		downloads_scroll->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("downloads"), SNAME("AssetLib")).ptr());
		error_label->add_theme_color_override(
			"color", get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
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

void EditorAssetLibrary::_image_request_completed(int p_status, int p_code,
	const PackedStringArray& headers, const PackedByteArray& p_data, int p_queue_id)
{
	ERR_FAIL_COND(!image_queue.has(p_queue_id));

	if (p_status == HTTPRequest::RESULT_SUCCESS && p_code < HTTPClient::RESPONSE_BAD_REQUEST) {
		if (p_code != HTTPClient::RESPONSE_NOT_MODIFIED) {
			for (int i = 0; i < headers.size(); i++) {
				if (headers[i].findn("ETag:") == 0) { // Save etag
					String cache_filename_base =
						EditorPaths::get_singleton()->get_cache_dir().path_join(
							"assetimage_" + image_queue[p_queue_id].image_url.md5_text());
					String new_etag =
						headers[i].substr(headers[i].find_char(':') + 1).strip_edges();
					Ref<FileAccess> file =
						FileAccess::open(cache_filename_base + ".etag", FileAccess::WRITE);
					if (file.is_valid()) {
						file->store_line(new_etag);
					}

					int len = p_data.size();
					const uint8_t* r = p_data.ptr();
					file = FileAccess::open(cache_filename_base + ".data", FileAccess::WRITE);
					if (file.is_valid()) {
						file->store_32(len);
						file->store_buffer(r, len);
					}

					break;
				}
			}
		}

		image_queue[p_queue_id].data = const_cast<PackedByteArray&>(p_data);
		image_queue[p_queue_id].use_cache = p_code == HTTPClient::RESPONSE_NOT_MODIFIED;
		set_process(true);
		image_queue[p_queue_id].thread->start(_image_update, &image_queue[p_queue_id]);
	}
	else {
		if (is_print_verbose_enabled()) {
			WARN_PRINT(vformat("Asset Store: Error getting image from '%s' for asset # %d.",
				image_queue[p_queue_id].image_url, image_queue[p_queue_id].asset_id));
		}

		image_queue[p_queue_id].request->queue_free();
		image_queue.erase(p_queue_id);
		_update_image_queue();
	}
}

void EditorAssetLibrary::_update_image_queue()
{
	const int max_images = 6;
	int current_images = 0;

	List<int> to_delete;
	for (KeyValue<int, ImageQueue>& E : image_queue) {
		if (!E.value.active && current_images < max_images) {
			String cache_filename_base = EditorPaths::get_singleton()->get_cache_dir().path_join(
				"assetimage_" + E.value.image_url.md5_text());
			Vector<String> headers;

			if (FileAccess::exists(cache_filename_base + ".etag") &&
				FileAccess::exists(cache_filename_base + ".data")) {
				Ref<FileAccess> file =
					FileAccess::open(cache_filename_base + ".etag", FileAccess::READ);
				if (file.is_valid()) {
					headers.push_back("If-None-Match: " + file->get_line());
				}
			}

			Error err = E.value.request->request(E.value.image_url, headers);
			if (err != OK) {
				to_delete.push_back(E.key);
			}
			else {
				E.value.active = true;
			}
		}

		current_images++;
	}

	while (to_delete.size()) {
		image_queue[to_delete.front()->get()].request->queue_free();
		image_queue.erase(to_delete.front()->get());
		to_delete.pop_front();
	}
}

void EditorAssetLibrary::_licenses_id_pressed(int p_id)
{
	licenses->get_popup()->set_item_checked(p_id, !licenses->get_popup()->is_item_checked(p_id));
}

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

const Ref<Texture2D> AssetLibraryEditorPlugin::get_plugin_icon() const
{
	return EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("AssetStore"), EditorStringName(EditorIcons));
}


