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

void EditorAssetLibraryItem::configure(const String& p_title, const String& p_asset_id,
	const String& p_author, const String& p_author_id, bool p_verified,
	const String& p_license_type, const String& p_license_url, int p_rating)
{
	title_text = p_title;
	title->set_text(title_text);
	title->set_tooltip_text(title_text);
	asset_id = p_asset_id;
	author->set_text(p_author);
	author_id = p_author_id;
	verified->set_visible(p_verified);
	license->set_text(p_license_type);
	license_url = p_license_url;
	rating_count->set_text(itos(p_rating));

	if (author_id.is_empty()) {
		author->set_disabled(true);
		author->set_mouse_filter(MOUSE_FILTER_IGNORE);
	}

	_calculate_misc_links_size();
}

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

void EditorAssetLibraryItem::_bind_methods() {}

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
	verified->hide();
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

//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

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

void EditorAssetLibraryItemDescription::configure(const String& p_title, const String& p_asset_id,
	const String& p_author, const String& p_author_id, bool p_verified,
	const String& p_license_type, const String& p_license_url, int p_rating,
	const String& p_description, const HashMap<String, String>& p_tags, const String& p_store_url,
	const String& p_source_url)
{
	asset_id = p_asset_id;
	title = p_title;
	item->configure(p_title, p_asset_id, p_author, p_author_id, p_verified, p_license_type,
		p_license_url, p_rating);

	releases.clear();

	version->show();
	version->set_text(TTRC("Loading..."));
	version_list->hide();
	version_list->clear();

	store_url = p_store_url;

	source_url = p_source_url;
	source->set_visible(!p_source_url.is_empty());

	description->clear();
	description->append_text(p_description);

	if (!p_tags.is_empty()) {
		description->append_text("\n[b]" + TTR("Tags:") + "[/b]");
		for (const KeyValue<String, String>& KV : p_tags) {
			description->add_text(" ");
			description->add_text("#" + KV.key);
			description->pop();
		}
	}

	changelog->set_text(TTRC("Loading..."));

	set_title(p_title);
	if (install_mode == MODE_DOWNLOAD) {
		get_ok_button()->set_disabled(true);
	}
}

void EditorAssetLibraryItemDescription::set_install_mode(InstallMode p_mode)
{
	if (p_mode == install_mode) {
		return;
	}

	switch (p_mode) {
	case MODE_DOWNLOAD: {
		set_ok_button_text(TTRC("Download"));
		get_ok_button()->set_disabled(releases.is_empty());
		version_list->set_disabled(releases.is_empty());
	} break;

	case MODE_DOWNLOADING: {
		set_ok_button_text(TTRC("Downloading..."));
		get_ok_button()->set_disabled(true);
		version_list->set_disabled(true);
	} break;

	case MODE_INSTALL: {
		set_ok_button_text(TTRC("Install..."));
		get_ok_button()->set_disabled(false);
		version_list->set_disabled(true);
	} break;
	}

	install_mode = p_mode;
}

void EditorAssetLibraryItemDescription::add_release(
	const String& p_url, const String& p_version, const String& p_changes, const String& p_sha256)
{
	Release release;
	release.url = p_url;
	release.version = p_version;
	release.sha256 = p_sha256;

	if (releases.is_empty()) {
		version->set_text(p_version);
		if (install_mode == MODE_DOWNLOAD) {
			get_ok_button()->set_disabled(false);
		}

		changelog->clear();
		changelog->append_text(
			p_changes.is_empty() ? TTRC("No changelog provided for this version.") : p_changes);

	}
	else if (releases.size() == 1) {
		version->hide();
		version_list->set_text(releases[0].version);
		if (install_mode == MODE_DOWNLOAD) {
			version_list->set_disabled(false);
		}
		version_list->show();
	}

	version_list->add_item(p_version, releases.size());

	releases.append(release);
}

void EditorAssetLibraryItemDescription::add_preview(
	int p_id, bool p_video, const String& p_url, const String& p_thumbnail)
{
	if (preview_images.is_empty()) {
		desc_vbox->set_h_size_flags(0);
		previews_vbox->show();
	}

	Preview new_preview;
	new_preview.id = p_id;
	new_preview.video_link = p_url;
	new_preview.is_video = p_video;
	new_preview.button = memnew(Button);
	new_preview.button->set_button_icon(previews->get_editor_theme_icon(SNAME("ThumbnailWait")));
	new_preview.button->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	new_preview.button->set_expand_icon(true);
	new_preview.button->set_toggle_mode(!p_video);
	new_preview.button->set_theme_type_variation(SNAME("ThumbnailButton"));
	new_preview.button->set_custom_minimum_size(Size2(preview_hb->get_size().height, 0));
	preview_hb->add_child(new_preview.button);

	if (!p_video) {
		new_preview.button->set_button_group(preview_group);
		// Enable the preview arrows if more than one screenshot is available.
		if (previous_preview->is_disabled()) {
			List<BaseButton*> buttons;
			preview_group->get_buttons(&buttons);
			if (buttons.size() > 1) {
				previous_preview->set_disabled(false);
				next_preview->set_disabled(false);
			}
		}

		zoom_button->set_disabled(false);
	}

	preview_images.push_back(new_preview);
}

EditorAssetLibraryItemDescription::EditorAssetLibraryItemDescription()
{
	root = memnew(HBoxContainer);
	root->add_theme_constant_override("separation", 15 * EDSCALE);
	add_child(root);

	desc_vbox = memnew(VBoxContainer);
	desc_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	desc_vbox->set_custom_minimum_size(Size2(440, 440) * EDSCALE);
	root->add_child(desc_vbox);

	item = memnew(EditorAssetLibraryItem);
	desc_vbox->add_child(item);

	HBoxContainer* contents = memnew(HBoxContainer);
	desc_vbox->add_child(contents);

	version_label = memnew(Label(TTRC("Version:")));
	contents->add_child(version_label);

	version = memnew(Label);
	version->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	version->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	version->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
	version->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	version->set_theme_type_variation("LabelNoMargin");
	contents->add_child(version);

	version_list = memnew(OptionButton);
	version_list->set_fit_to_longest_item(false);
	version_list->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	version_list->set_tooltip_text(TTRC("Download other versions."));
	version_list->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
	version_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	version_list->hide(); // Will be shown if multiple versions are available.
	contents->add_child(version_list);

	store = memnew(Button);
	store->set_text(TTRC("Store Page"));
	store->set_tooltip_text(
		TTRC("Open the web browser to show the asset in the online store page."));
	store->set_theme_type_variation(SceneStringName(FlatButton));
	contents->add_child(store);

	source = memnew(Button);
	source->set_text(TTRC("View Source"));
	source->set_tooltip_text(TTRC("Open the web browser to show a page with the source files."));
	source->set_theme_type_variation(SceneStringName(FlatButton));
	source->hide(); // Will be shown if the source link is available.
	contents->add_child(source);

	tabs = memnew(TabContainer);
	tabs->set_theme_type_variation("TabContainerInner");
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	desc_vbox->add_child(tabs);

	description = memnew(RichTextLabel);
	description->set_selection_enabled(true);
	description->set_context_menu_enabled(true);
	description->set_name(TTRC("Description"));
	description->add_theme_constant_override(
		SceneStringName(line_separation), Math::round(5 * EDSCALE));
	tabs->add_child(description);

	changelog = memnew(RichTextLabel);
	changelog->set_selection_enabled(true);
	changelog->set_context_menu_enabled(true);
	changelog->set_name(TTRC("Changelog"));
	changelog->add_theme_constant_override(
		SceneStringName(line_separation), Math::round(5 * EDSCALE));
	tabs->add_child(changelog);

	previews_vbox = memnew(VBoxContainer);
	previews_vbox->hide(); // Will be shown if we add any previews later.
	previews_vbox->add_theme_constant_override("separation", 15 * EDSCALE);
	previews_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	previews_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(previews_vbox);

	HBoxContainer* previews_hbox = memnew(HBoxContainer);
	previews_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	previews_vbox->add_child(previews_hbox);

	previous_preview = memnew(Button);
	previous_preview->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	previous_preview->set_disabled(true);
	previous_preview->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	previews_hbox->add_child(previous_preview);

	preview = memnew(TextureRect);
	preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	preview->set_custom_minimum_size(Size2(640, 345) * EDSCALE);
	preview->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	previews_hbox->add_child(preview);

	MarginContainer* mc = memnew(MarginContainer);
	previews_hbox->add_child(mc);

	next_preview = memnew(Button);
	next_preview->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	next_preview->set_disabled(true);
	next_preview->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	mc->add_child(next_preview);

	zoom_button = memnew(Button);
	zoom_button->set_toggle_mode(true);
	zoom_button->set_disabled(true);
	zoom_button->set_tooltip_text(TTRC("Toggle full view of preview images."));
	zoom_button->set_v_size_flags(Control::SIZE_SHRINK_END);
	mc->add_child(zoom_button);

	previews_bg = memnew(PanelContainer);
	previews_vbox->add_child(previews_bg);

	previews = memnew(ScrollContainer);
	previews->set_follow_focus(true);
	previews->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	previews_bg->add_child(previews);
	preview_hb = memnew(HBoxContainer);
	preview_hb->set_custom_minimum_size(Size2(620, 90) * EDSCALE);
	preview_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	previews->add_child(preview_hb);

	preview_group.instantiate();

	set_ok_button_text(TTRC("Download"));
	set_cancel_button_text(TTRC("Close"));
}

///////////////////////////////////////////////////////////////////////////////////

void EditorAssetLibraryItemDownload::configure(const String& p_title, const String& p_asset_id,
	const String& p_version, const Ref<Texture2D>& p_preview, const String& p_download_url,
	const String& p_sha256)
{
	title->set_text(p_title);
	version->set_text(p_version);
	icon->set_texture(p_preview);
	asset_id = p_asset_id;
	if (p_preview.is_null()) {
		icon->set_texture(get_editor_theme_icon(SNAME("FileBrokenBigThumb")));
	}
	host = p_download_url;
	sha256 = p_sha256;
	_make_request();
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

	case NOTIFICATION_PROCESS: {
		progress->show();

		if (download->get_downloaded_bytes() > 0) {
			progress->set_max(download->get_body_size());
			progress->set_value(download->get_downloaded_bytes());
		}

		int cstatus = download->get_http_client_status();

		if (cstatus == HTTPClient::STATUS_BODY) {
			if (download->get_body_size() > 0) {
				progress->set_indeterminate(false);
				status->set_text(vformat(TTR("Downloading (%s / %s)..."),
					String::humanize_size(download->get_downloaded_bytes()),
					String::humanize_size(download->get_body_size())));
			}
			else {
				progress->set_indeterminate(true);
				status->set_text(vformat(TTR("Downloading...") + " (%s)",
					String::humanize_size(download->get_downloaded_bytes())));
			}
		}

		if (cstatus != prev_status) {
			switch (cstatus) {
			case HTTPClient::STATUS_RESOLVING: {
				status->set_text(TTRC("Resolving..."));
				progress->set_max(1);
				progress->set_value(0);
			} break;
			case HTTPClient::STATUS_CONNECTING: {
				status->set_text(TTRC("Connecting..."));
				progress->set_max(1);
				progress->set_value(0);
			} break;
			case HTTPClient::STATUS_REQUESTING: {
				status->set_text(TTRC("Requesting..."));
				progress->set_max(1);
				progress->set_value(0);
			} break;
			default: {
			}
			}
			prev_status = cstatus;
		}
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

void EditorAssetLibraryItemDownload::_make_request()
{
	// Hide the Retry button if we've just pressed it.
	retry_button->hide();

	download->cancel_request();
	download->set_download_file(
		EditorPaths::get_singleton()->get_cache_dir().path_join("tmp_asset_" + asset_id) + ".zip");

	Error err = download->request(host);
	if (err != OK) {
		status->set_text(TTRC("Error making request"));
	}
	else {
		progress->set_indeterminate(true);
		set_process(true);
	}
}

void EditorAssetLibraryItemDownload::_bind_methods() {}

EditorAssetLibraryItemDownload::EditorAssetLibraryItemDownload()
{
	panel = memnew(PanelContainer);
	add_child(panel);

	HBoxContainer* hb = memnew(HBoxContainer);
	panel->add_child(hb);
	icon = memnew(TextureRect);
	icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	icon->set_v_size_flags(0);
	hb->add_child(icon);

	VBoxContainer* vb = memnew(VBoxContainer);
	hb->add_child(vb);
	vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vb->add_theme_constant_override("separation", 0);

	HBoxContainer* title_hb = memnew(HBoxContainer);
	vb->add_child(title_hb);
	title = memnew(Label);
	title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	title->set_theme_type_variation("LabelNoMarginVertical");
	title->set_focus_mode(FOCUS_ACCESSIBILITY);
	title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_hb->add_child(title);

	dismiss_button = memnew(TextureButton);
	dismiss_button->set_accessibility_name(TTRC("Close"));
	title_hb->add_child(dismiss_button);

	version = memnew(Label);
	version->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	version->set_theme_type_variation("LabelNoMarginVertical");
	vb->add_child(version);

	spacer = memnew(Control);
	vb->add_child(spacer);

	status = memnew(Label(TTRC("Idle")));
	vb->add_child(status);

	progress_hbox = memnew(HBoxContainer);
	vb->add_child(progress_hbox);

	progress = memnew(ProgressBar);
	progress->set_editor_preview_indeterminate(true);
	progress->hide();
	progress->set_h_size_flags(SIZE_EXPAND_FILL);
	progress_hbox->add_child(progress);

	retry_button = memnew(Button);
	retry_button->set_text(TTRC("Retry"));
	retry_button->hide(); // Only show the Retry button in case of a failure.
	retry_button->set_h_size_flags(SIZE_EXPAND | SIZE_SHRINK_END);
	progress_hbox->add_child(retry_button);

	install_button = memnew(Button);
	install_button->set_text(TTRC("Install..."));
	install_button->hide();
	install_button->set_h_size_flags(SIZE_EXPAND | SIZE_SHRINK_END);
	progress_hbox->add_child(install_button);

	set_custom_minimum_size(Size2(400 * EDSCALE, 0));

	download = memnew(HTTPRequest);
	panel->add_child(download);

	download_error = memnew(AcceptDialog);
	download_error->set_title(TTRC("Download Error"));
	panel->add_child(download_error);

	asset_installer = memnew(EditorAssetInstaller);
	panel->add_child(asset_installer);

	prev_status = -1;

	external_install = false;
}

////////////////////////////////////////////////////////////////////////////////
void EditorAssetLibrary::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		add_theme_style_override(
			SceneStringName(panel), get_theme_stylebox(SNAME("bg"), SNAME("AssetLib")).ptr());
		error_label->move_to_front();
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (!initial_loading) {
			_search();
		}
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

	case NOTIFICATION_PROCESS: {
		// Check for finished image updates.
		List<int> to_delete;
		for (KeyValue<int, ImageQueue>& E : image_queue) {
			if (!E.value.update_finished) {
				continue;
			}

			E.value.thread->wait_to_finish();
			E.value.request->queue_free();
			to_delete.push_back(E.key);
			_update_image_queue();
		}

		while (to_delete.size()) {
			image_queue[to_delete.front()->get()].request->queue_free();
			image_queue.erase(to_delete.front()->get());
			to_delete.pop_front();
		}

		const bool no_downloads = downloads_hb->get_child_count() == 0;
		if (no_downloads == downloads_scroll->is_visible()) {
			downloads_scroll->set_visible(!no_downloads);

			if (Engine::get_singleton()->is_project_manager_hint()) {
				library_mc->set_theme_type_variation(
					no_downloads ? "NoBorderAssetLibProjectManager"
								 : "NoBorderAssetLibProjectManagerHorizontal");
			}
			else {
				library_mc->set_theme_type_variation(
					no_downloads ? "NoBorderAssetLib" : "NoBorderAssetLibHorizontal");
			}
			library_scroll->set_scroll_hint_mode(
				no_downloads ? ScrollContainer::SCROLL_HINT_MODE_TOP_AND_LEFT
							 : ScrollContainer::SCROLL_HINT_MODE_ALL);
		}

		if (image_queue.is_empty()) {
			set_process(false);
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

void EditorAssetLibrary::_tag_clicked(const String& p_tag)
{
	description->hide();
	filter->set_text(p_tag);
	_search();
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

void EditorAssetLibrary::_select_asset(const String& p_id)
{
	_api_request("assets/" + p_id, REQUESTING_ASSET);
}

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

void EditorAssetLibrary::_repository_changed(int p_repository_id)
{
	_set_library_message(TTRC("Loading..."));

	if (asset_items) {
		memdelete(asset_items);
		asset_items = nullptr;
	}

	if (asset_top_page) {
		memdelete(asset_top_page);
		asset_top_page = nullptr;
	}

	if (asset_bottom_page) {
		memdelete(asset_bottom_page);
		asset_bottom_page = nullptr;
	}

	filter->set_editable(false);
	sort->set_disabled(true);
	categories->set_disabled(true);

	_api_request("", REQUESTING_CHECK);
}

void EditorAssetLibrary::_licenses_id_pressed(int p_id)
{
	licenses->get_popup()->set_item_checked(p_id, !licenses->get_popup()->is_item_checked(p_id));
}

void EditorAssetLibrary::_licenses_popup_hide()
{
	licenses_all_toggled = true;

	bool research = false;
	PopupMenu* pm = licenses->get_popup();
	for (unsigned int i = 0; i < licenses_toggled.size(); i++) {
		bool toggled = pm->is_item_checked(i);
		if (toggled != licenses_toggled[i]) {
			licenses_toggled[i] = toggled;
			research = true;
		}

		if (!toggled) {
			licenses_all_toggled = false;
		}
	}

	if (research) {
		_search();
	}
}

void EditorAssetLibrary::_search(int p_page)
{
	ERR_FAIL_COND(p_page <= 0);

	String search = filter->get_text().to_lower();
	String args = "?query=" + search.uri_encode();

	args += "&require_release=true";
	args += "&type=" + String(templates_only ? "1" : "0");
	args += "&sort=" + String(sort_key[sort->get_selected()]);

	args += "&compatibility=" + itos(VLTR_VERSION_MAJOR) + "." + itos(VLTR_VERSION_MINOR);
	if (VLTR_VERSION_PATCH > 0) {
		args += "." + itos(VLTR_VERSION_PATCH);
	}

	current_page = p_page;
	if (p_page > 1) {
		args += "&page=" + itos(p_page);
	}

	_api_request("search/query/" + args, REQUESTING_SEARCH);
}

void EditorAssetLibrary::_request_current_config()
{
	_repository_changed(repository->get_selected());
}

HBoxContainer* EditorAssetLibrary::_make_pages(
	int p_page, int p_page_count, int p_page_len, int p_total_items, int p_current_items)
{
	HBoxContainer* hbc = memnew(HBoxContainer);

	if (p_page_count < 1) {
		return hbc;
	}

	// 🎜 Do the Mario! Eat your arms, and then again... 🎜
	int from = p_page - (5 / EDSCALE);
	if (from < 1) {
		from = 1;
	}
	int to = from + (10 / EDSCALE);
	if (to > p_page_count) {
		to = p_page_count;
	}

	hbc->add_spacer();
	hbc->add_theme_constant_override("separation", 5 * EDSCALE);

	Button* first = memnew(Button);
	first->set_button_icon(get_editor_theme_icon(SNAME("BackStart")));
	first->set_tooltip_text(TTR("First", "Pagination"));
	first->set_theme_type_variation("PanelBackgroundButton");
	first->set_disabled(true);
	first->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbc->add_child(first);

	Button* prev = memnew(Button);
	prev->set_button_icon(get_editor_theme_icon(SNAME("Back")));
	prev->set_tooltip_text(TTR("Previous", "Pagination"));
	prev->set_theme_type_variation("PanelBackgroundButton");
	prev->set_disabled(true);
	prev->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbc->add_child(prev);

	hbc->add_child(memnew(VSeparator));

	for (int i = from; i <= to; i++) {
		Button* current = memnew(Button);
		// Add padding to make page number buttons easier to click.
		current->set_text(vformat(" %d ", i));
		current->set_theme_type_variation("PanelBackgroundButton");
		if (i == p_page) {
			current->set_disabled(true);
			current->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		}
		hbc->add_child(current);
	}

	hbc->add_child(memnew(VSeparator));

	Button* next = memnew(Button);
	next->set_button_icon(get_editor_theme_icon(SNAME("Forward")));
	next->set_tooltip_text(TTR("Next", "Pagination"));
	next->set_theme_type_variation("PanelBackgroundButton");
	next->set_disabled(true);
	next->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbc->add_child(next);

	Button* last = memnew(Button);
	last->set_button_icon(get_editor_theme_icon(SNAME("ForwardEnd")));
	last->set_tooltip_text(TTR("Last", "Pagination"));
	last->set_theme_type_variation("PanelBackgroundButton");
	last->set_disabled(true);
	last->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbc->add_child(last);

	hbc->add_spacer();

	return hbc;
}

void EditorAssetLibrary::_update_button_icon(Button* p_button, const StringName& p_icon)
{
	p_button->set_button_icon(get_editor_theme_icon(p_icon));
}

void EditorAssetLibrary::_api_request(
	const String& p_request, RequestType p_request_type, bool p_is_parallel)
{
	if (!p_is_parallel) {
		error_hb->hide();
	}

	if (loading_blocked) {
		return;
	}

	HTTPRequest* requester = nullptr;
	if (p_is_parallel) {
		requester = memnew(HTTPRequest);
		add_child(requester);
	}
	else {
		requester = request;
		// Make it clear that it's busy.
		library_scroll->set_modulate(Color(1, 1, 1, 0.5));
	}

	requester->request(host + "/" + p_request);
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

void EditorAssetLibrary::_update_downloads_section()
{
	const bool has_downloads = downloads_hb->get_child_count() > 0;
	downloads_scroll->set_visible(has_downloads);
	library_mc->set_theme_type_variation(
		has_downloads
			? "NoBorderHorizontal"
			: (Engine::get_singleton()->is_project_manager_hint() ? "NoBorderAssetLibProjectManager"
																  : "NoBorderAssetLib"));
	library_scroll->set_scroll_hint_mode(has_downloads
											 ? ScrollContainer::SCROLL_HINT_MODE_ALL
											 : ScrollContainer::SCROLL_HINT_MODE_TOP_AND_LEFT);
}

void EditorAssetLibrary::_set_library_message(const String& p_message)
{
	library_message->set_text(p_message);

	library_message_button->hide();

	library_message_box->show();

	// Remove pagination, as an error message is being shown and there are no assets to list.
	// Pagination is recreated when the next search is performed.
	if (asset_top_page) {
		memdelete(asset_top_page);
		asset_top_page = nullptr;
	}
	if (asset_bottom_page) {
		memdelete(asset_bottom_page);
		asset_bottom_page = nullptr;
	}
}

///////

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

AssetLibraryEditorPlugin::AssetLibraryEditorPlugin()
{
	addon_library = memnew(EditorAssetLibrary);
	addon_library->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	EditorNode::get_singleton()->get_editor_main_screen()->get_control()->add_child(addon_library);
	addon_library->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	addon_library->hide();
}


