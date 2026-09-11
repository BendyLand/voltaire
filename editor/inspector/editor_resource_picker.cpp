/**************************************************************************/
/*  editor_resource_picker.cpp                                            */
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

#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_resource_picker.h"
#include "scene/gui/button.h"
#include "scene/gui/texture_rect.h"
#include "scene/property_utils.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/rendering_server.h"

void EditorResourcePicker::_update_menu()
{
	if (edit_menu && edit_menu->is_visible()) {
		edit_button->set_pressed(false);
		edit_menu->hide();
		return;
	}

	_update_menu_items();

	Rect2 gt = edit_button->get_screen_rect();
	edit_menu->reset_size();
	int ms = edit_menu->get_contents_minimum_size().width;
	Vector2 popup_pos = gt.get_end() - Vector2(ms, 0);
	edit_menu->set_position(popup_pos);
	edit_menu->popup();
}

void EditorResourcePicker::_button_draw()
{
	if (dropping) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		assign_button->draw_rect(Rect2(Point2(), assign_button->get_size()), color, false);
	}
}

void EditorResourcePicker::_button_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::RIGHT) {
		// Only attempt to update and show the menu if we have
		// a valid resource or the Picker is editable, as
		// there will otherwise be nothing to display.
		if (edited_resource.is_valid() || is_editable()) {
			if (edit_menu && edit_menu->is_visible()) {
				edit_button->set_pressed(false);
				edit_menu->hide();
				return;
			}

			_update_menu_items();

			Vector2 pos = get_screen_position() + mb->get_position();
			edit_menu->reset_size();
			edit_menu->set_position(pos);
			edit_menu->popup();
		}
	}
}

void EditorResourcePicker::_on_unique_button_pressed()
{
	if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
		_edit_menu_cbk(OBJ_MENU_MAKE_UNIQUE);
	}
	else if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::RIGHT) &&
			   _is_uniqueness_enabled(true)) {
		_edit_menu_cbk(OBJ_MENU_MAKE_UNIQUE_RECURSIVE);
	}
}

static bool _should_hide_type(const StringName& p_type)
{
	if (p_type == SNAME("MissingResource")) {
		return true;
	}

	return false;
}

bool EditorResourcePicker::_is_type_valid(
	const String& p_type_name, const HashSet<StringName>& p_allowed_types) const
{
	for (const StringName& E : p_allowed_types) {
		String at = E;
		if (p_type_name == at ||
			EditorNode::get_editor_data().script_class_is_parent(p_type_name, at)) {
			return true;
		}
	}

	return false;
}

void EditorResourcePicker::set_assign_button_min_size(const Size2i& p_size)
{
	assign_button_min_size = p_size;
	assign_button->set_custom_minimum_size(assign_button_min_size);
}

String EditorResourcePicker::get_base_type() const { return base_type; }

Vector<String> EditorResourcePicker::get_allowed_types() const
{
	_ensure_allowed_types();
	HashSet<StringName> allowed_types(allowed_types_without_convert);

	Vector<String> types;
	types.resize(allowed_types.size());

	int i = 0;
	String* w = types.ptrw();
	for (const StringName& E : allowed_types) {
		w[i] = E;
		i++;
	}

	return types;
}

void EditorResourcePicker::set_edited_resource_no_check(Ref<Resource> p_resource)
{
	edited_resource = p_resource;
	_update_resource();
}

Ref<Resource> EditorResourcePicker::get_edited_resource() { return edited_resource; }

void EditorResourcePicker::set_toggle_mode(bool p_enable)
{
	assign_button->set_toggle_mode(p_enable);
}

bool EditorResourcePicker::is_toggle_mode() const { return assign_button->is_toggle_mode(); }

void EditorResourcePicker::set_toggle_pressed(bool p_pressed)
{
	if (!is_toggle_mode()) {
		return;
	}

	assign_button->set_pressed(p_pressed);
}

bool EditorResourcePicker::is_toggle_pressed() const { return assign_button->is_pressed(); }

void EditorResourcePicker::set_editable(bool p_editable)
{
	editable = p_editable;
	assign_button->set_disabled(!editable && edited_resource.is_null());
	quick_load_button->set_visible(editable && edited_resource.is_null());
	edit_button->set_visible(editable);
}

bool EditorResourcePicker::is_editable() const { return editable; }

// EditorScriptPicker

bool EditorScriptPicker::handle_menu_selected(int p_which)
{
	switch (p_which) {
	case OBJ_MENU_NEW_SCRIPT: {
		if (script_owner) {
			SceneTreeDock::get_singleton()->open_script_dialog(script_owner, false);
		}
		return true;
	}

	case OBJ_MENU_EXTEND_SCRIPT: {
		if (script_owner) {
			SceneTreeDock::get_singleton()->open_script_dialog(script_owner, true);
		}
		return true;
	}
	}

	return false;
}

void EditorScriptPicker::set_script_owner(Node* p_owner) { script_owner = p_owner; }

Node* EditorScriptPicker::get_script_owner() const { return script_owner; }

// EditorShaderPicker

bool EditorShaderPicker::handle_menu_selected(int p_which)
{
	Ref<ShaderMaterial> ed_material = Ref<ShaderMaterial>(get_edited_material());

	switch (p_which) {
	case OBJ_MENU_NEW_SHADER: {
		if (ed_material.is_valid()) {
			SceneTreeDock::get_singleton()->open_shader_dialog(ed_material, preferred_mode);
			return true;
		}
	} break;
	default:
		break;
	}
	return false;
}

void EditorShaderPicker::set_edited_material(ShaderMaterial* p_material)
{
	edited_material = p_material;
}

ShaderMaterial* EditorShaderPicker::get_edited_material() const { return edited_material; }

void EditorShaderPicker::set_preferred_mode(int p_mode) { preferred_mode = p_mode; }

//////////////

void EditorAudioStreamPicker::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY:
	case NOTIFICATION_THEME_CHANGED: {
		_update_resource();
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		Ref<AudioStream> audio_stream = get_edited_resource();
		if (audio_stream.is_valid()) {
			if (audio_stream->get_length() > 0) {
				Ref<AudioStreamPreview> preview =
					AudioStreamPreviewGenerator::get_singleton()->generate_preview(audio_stream);
				if (preview.is_valid()) {
					if (preview->get_version() != last_preview_version) {
						stream_preview_rect->queue_redraw();
						last_preview_version = preview->get_version();
					}
				}
			}

			uint64_t tagged_frame = audio_stream->get_tagged_frame();
			uint64_t diff_frames = AudioServer::get_singleton()->get_mixed_frames() - tagged_frame;
			uint64_t diff_msec = diff_frames * 1000 / AudioServer::get_singleton()->get_mix_rate();

			if (diff_msec < 300) {
				uint32_t count = audio_stream->get_tagged_frame_count();

				bool differ = false;

				if (count != tagged_frame_offset_count) {
					differ = true;
				}
				float offsets[MAX_TAGGED_FRAMES];

				for (uint32_t i = 0; i < MIN(count, uint32_t(MAX_TAGGED_FRAMES)); i++) {
					offsets[i] = audio_stream->get_tagged_frame_offset(i);
					if (offsets[i] != tagged_frame_offsets[i]) {
						differ = true;
					}
				}

				if (differ) {
					tagged_frame_offset_count = count;
					for (uint32_t i = 0; i < count; i++) {
						tagged_frame_offsets[i] = offsets[i];
					}
				}

				stream_preview_rect->queue_redraw();
			}
			else {
				if (tagged_frame_offset_count != 0) {
					stream_preview_rect->queue_redraw();
				}
				tagged_frame_offset_count = 0;
			}
		}
	} break;
	}
}

void EditorAudioStreamPicker::_update_resource()
{
	EditorResourcePicker::_update_resource();

	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	Ref<AudioStream> audio_stream = get_edited_resource();
	if (audio_stream.is_valid() && audio_stream->get_length() > 0.0) {
		set_assign_button_min_size(Size2(1, font->get_height(font_size) * 3));
	}
	else {
		set_assign_button_min_size(Size2(1, font->get_height(font_size) * 1.5));
	}

	stream_preview_rect->queue_redraw();
}

bool EditorResourcePicker::handle_menu_selected(int p_idx) { return true; }


