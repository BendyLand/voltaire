/**************************************************************************/
/*  editor_properties.cpp                                                 */
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
#include "core/input/input_map.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/editor_variant_type_selectors.h"
#include "editor/inspector/editor_properties_array_dict.h"
#include "editor/inspector/editor_properties_vector.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/property_selector.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor_properties.h"
#include "modules/modules_enabled.gen.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/3d/fog_volume.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/mesh.h"
#include "scene/resources/sky.h"
#include "servers/display/display_server.h"

#ifdef MODULE_VISUAL_SHADER_ENABLED
#endif // MODULE_VISUAL_SHADER_ENABLED

void EditorPropertyNil::update_property() {}

EditorPropertyNil::EditorPropertyNil()
{
	Label* prop_label = memnew(Label);
	prop_label->set_text("<null>");
	add_child(prop_label);
}

void EditorPropertyText::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_update_theme();
	} break;
	}
}

void EditorPropertyText::_set_read_only(bool p_read_only) { text->set_editable(!p_read_only); }

void EditorPropertyText::_update_theme()
{
	Ref<Font> font;
	int font_size;

	if (monospaced) {
		font = get_theme_font(SNAME("source"), EditorStringName(EditorFonts));
		font_size = get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts));
	}
	else {
		font = get_theme_font(SceneStringName(font), SNAME("LineEdit"));
		font_size = get_theme_font_size(SceneStringName(font_size), SNAME("LineEdit"));
	}

	text->add_theme_font_override(SceneStringName(font), font.ptr());
	text->add_theme_font_size_override(SceneStringName(font_size), font_size);
}

void EditorPropertyText::_text_submitted(const String& p_string)
{
	if (updating) {
		return;
	}

	if (text->has_focus()) {
		_text_changed(p_string);
	}
}

void EditorPropertyText::set_secret(bool p_enabled) { text->set_secret(p_enabled); }

void EditorPropertyText::set_placeholder(const String& p_string)
{
	text->set_placeholder(p_string);
}

void EditorPropertyText::set_monospaced(bool p_monospaced)
{
	if (p_monospaced == monospaced) {
		return;
	}
	monospaced = p_monospaced;
	_update_theme();
}

void EditorPropertyMultilineText::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_update_theme();
	} break;
	}
}

void EditorPropertyMultilineText::EditorPropertyMultilineText::set_monospaced(bool p_monospaced)
{
	if (p_monospaced == monospaced) {
		return;
	}
	monospaced = p_monospaced;
	_update_theme();
}

bool EditorPropertyMultilineText::EditorPropertyMultilineText::get_monospaced()
{
	return monospaced;
}

void EditorPropertyMultilineText::EditorPropertyMultilineText::set_wrap_lines(bool p_wrap_lines)
{
	if (p_wrap_lines == wrap_lines) {
		return;
	}
	wrap_lines = p_wrap_lines;
	_update_theme();
}

bool EditorPropertyMultilineText::EditorPropertyMultilineText::get_wrap_lines()
{
	return wrap_lines;
}

void EditorPropertyLocale::setup(const String& p_hint_text) {}

void EditorPropertyLocale::_locale_focus_exited() { _locale_selected(locale->get_text()); }

void EditorPropertyPath::setup(
	const Vector<String>& p_extensions, bool p_folder, bool p_global, bool p_enable_uid)
{
	extensions = p_extensions;
	folder = p_folder;
	global = p_global;
	enable_uid = p_enable_uid;
}

void EditorPropertyPath::set_save_mode() { save_mode = true; }

void EditorPropertyPath::_path_focus_exited() { _path_selected(path->get_text()); }

void EditorPropertyPath::_toggle_uid_display()
{
	display_uid = !display_uid;
	_update_uid_icon();
	update_property();
}

void EditorPropertyClassName::setup(const String& p_base_type, const String& p_selected_type)
{
	base_type = p_base_type;
	dialog->set_base_type(base_type);
	selected_type = p_selected_type;
	property->set_text(selected_type);
}

void EditorPropertyEnum::set_option_button_clip(bool p_enable) { options->set_clip_text(p_enable); }

OptionButton* EditorPropertyEnum::get_option_button() { return options; }

EditorPropertyFlags::EditorPropertyFlags()
{
	vbox = memnew(VBoxContainer);
	vbox->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	add_child(vbox);
}

Size2 EditorPropertyLayersGrid::get_grid_size() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	return Vector2(0, font->get_height(font_size) * 3);
}

void EditorPropertyLayersGrid::set_read_only(bool p_read_only) { read_only = p_read_only; }

Size2 EditorPropertyLayersGrid::get_minimum_size() const
{
	Size2 min_size = get_grid_size();

	// Add extra rows when expanded.
	if (expanded) {
		const int bsize = (min_size.height * 80 / 100) / 2;
		for (int i = 0; i < expansion_rows; ++i) {
			min_size.y += 2 * (bsize + 1) + 3;
		}
	}

	return min_size;
}

void EditorPropertyLayers::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		button->set_texture_normal(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		button->set_texture_pressed(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		button->set_texture_disabled(get_editor_theme_icon(SNAME("GuiTabMenu")));
	} break;
	}
}

void EditorPropertyLayers::_refresh_names() { setup(layer_type); }

void EditorPropertyInteger::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyInteger::set_deferred_drag_mode_enabled(bool p_enabled)
{
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	spin->set_deferred_drag_mode_enabled(p_enabled);
}

void EditorPropertyObjectID::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		edit->add_theme_constant_override("icon_max_width",
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));
	} break;
	}
}

void EditorPropertyObjectID::setup(const String& p_base_type) { base_type = p_base_type; }

void EditorPropertyFloat::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyFloat::set_deferred_drag_mode_enabled(bool p_enabled)
{
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	spin->set_deferred_drag_mode_enabled(p_enabled);
}

void EditorPropertyEasing::_set_read_only(bool p_read_only) { spin->set_read_only(p_read_only); }

void EditorPropertyEasing::setup(bool p_positive_only, bool p_flip)
{
	flip = p_flip;
	positive_only = p_positive_only;
}

void EditorPropertyRect2::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyRect2::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 2]);
		}
	} break;
	}
}

void EditorPropertyRect2i::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyRect2i::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 2]);
		}
	} break;
	}
}

void EditorPropertyPlane::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyPlane::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 4; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i]);
		}
	} break;
	}
}

void EditorPropertyQuaternion::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 4; i++) {
		spin[i]->set_read_only(p_read_only);
	}
	for (int i = 0; i < 3; i++) {
		euler[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyQuaternion::_custom_value_changed(double val)
{
	edit_euler.x = euler[0]->get_value();
	edit_euler.y = euler[1]->get_value();
	edit_euler.z = euler[2]->get_value();

	Vector3 v;
	v.x = Math::deg_to_rad(edit_euler.x);
	v.y = Math::deg_to_rad(edit_euler.y);
	v.z = Math::deg_to_rad(edit_euler.z);

	Quaternion temp_q = Quaternion::from_euler(v);
	spin[0]->set_value_no_signal(temp_q.x);
	spin[1]->set_value_no_signal(temp_q.y);
	spin[2]->set_value_no_signal(temp_q.z);
	spin[3]->set_value_no_signal(temp_q.w);
	_value_changed(-1, "");
}

bool EditorPropertyQuaternion::is_grabbing_euler()
{
	bool is_grabbing = false;
	for (int i = 0; i < 3; i++) {
		is_grabbing |= euler[i]->is_grabbing();
	}
	return is_grabbing;
}

void EditorPropertyAABB::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyAABB::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 3]);
		}
	} break;
	}
}

void EditorPropertyTransform2D::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 6; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyTransform2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 6; i++) {
			// For Transform2D, use the 4th color (cyan) for the origin vector.
			if (i % 3 == 2) {
				spin[i]->add_theme_color_override("label_color", colors[3]);
			}
			else {
				spin[i]->add_theme_color_override("label_color", colors[i % 3]);
			}
		}
	} break;
	}
}

void EditorPropertyBasis::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 9; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyBasis::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 9; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 3]);
		}
	} break;
	}
}

void EditorPropertyTransform3D::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 12; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyTransform3D::update_using_transform(Transform3D p_transform)
{
	spin[0]->set_value_no_signal(p_transform.basis[0][0]);
	spin[1]->set_value_no_signal(p_transform.basis[0][1]);
	spin[2]->set_value_no_signal(p_transform.basis[0][2]);
	spin[3]->set_value_no_signal(p_transform.origin[0]);
	spin[4]->set_value_no_signal(p_transform.basis[1][0]);
	spin[5]->set_value_no_signal(p_transform.basis[1][1]);
	spin[6]->set_value_no_signal(p_transform.basis[1][2]);
	spin[7]->set_value_no_signal(p_transform.origin[1]);
	spin[8]->set_value_no_signal(p_transform.basis[2][0]);
	spin[9]->set_value_no_signal(p_transform.basis[2][1]);
	spin[10]->set_value_no_signal(p_transform.basis[2][2]);
	spin[11]->set_value_no_signal(p_transform.origin[2]);
}

void EditorPropertyTransform3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 12; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 4]);
		}
	} break;
	}
}

void EditorPropertyProjection::_set_read_only(bool p_read_only)
{
	for (int i = 0; i < 12; i++) {
		spin[i]->set_read_only(p_read_only);
	}
}

void EditorPropertyProjection::update_using_transform(Projection p_transform)
{
	spin[0]->set_value_no_signal(p_transform.columns[0][0]);
	spin[1]->set_value_no_signal(p_transform.columns[0][1]);
	spin[2]->set_value_no_signal(p_transform.columns[0][2]);
	spin[3]->set_value_no_signal(p_transform.columns[0][3]);
	spin[4]->set_value_no_signal(p_transform.columns[1][0]);
	spin[5]->set_value_no_signal(p_transform.columns[1][1]);
	spin[6]->set_value_no_signal(p_transform.columns[1][2]);
	spin[7]->set_value_no_signal(p_transform.columns[1][3]);
	spin[8]->set_value_no_signal(p_transform.columns[2][0]);
	spin[9]->set_value_no_signal(p_transform.columns[2][1]);
	spin[10]->set_value_no_signal(p_transform.columns[2][2]);
	spin[11]->set_value_no_signal(p_transform.columns[2][3]);
	spin[12]->set_value_no_signal(p_transform.columns[3][0]);
	spin[13]->set_value_no_signal(p_transform.columns[3][1]);
	spin[14]->set_value_no_signal(p_transform.columns[3][2]);
	spin[15]->set_value_no_signal(p_transform.columns[3][3]);
}

void EditorPropertyProjection::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color* colors = _get_property_colors();
		for (int i = 0; i < 16; i++) {
			spin[i]->add_theme_color_override("label_color", colors[i % 4]);
		}
	} break;
	}
}

void EditorPropertyColor::set_live_changes_enabled(bool p_enabled)
{
	live_changes_enabled = p_enabled;
}

void EditorPropertyNodePath::setup(
	const Vector<StringName>& p_valid_types, bool p_use_path_from_scene_root, bool p_editing_node)
{
	valid_types = p_valid_types;
	editing_node = p_editing_node;
	use_path_from_scene_root = p_use_path_from_scene_root;
}

EditorPropertyRID::EditorPropertyRID()
{
	label = memnew(Label);
	add_child(label);
}

void EditorPropertyResource::_set_read_only(bool p_read_only)
{
	resource_picker->set_editable(!p_read_only);
}

void EditorPropertyResource::_resource_selected(const Ref<Resource>& p_resource, bool p_inspect)
{
	_select_resource(p_resource, p_inspect, false);
}

void EditorPropertyResource::_resource_expand_requested(
	const Ref<Resource>& p_resource, bool p_inspect)
{
	_select_resource(p_resource, p_inspect, true);
}

bool EditorPropertyResource::_should_stop_editing() const
{
	return !resource_picker->is_toggle_pressed();
}

void EditorPropertyResource::collapse_all_folding()
{
	if (sub_inspector) {
		sub_inspector->collapse_all_folding();
	}
}

void EditorPropertyResource::expand_all_folding()
{
	if (sub_inspector) {
		sub_inspector->expand_all_folding();
	}
}

void EditorPropertyResource::expand_revertable()
{
	if (sub_inspector) {
		sub_inspector->expand_revertable();
	}
}

void EditorPropertyResource::set_use_sub_inspector(bool p_enable) { use_sub_inspector = p_enable; }

void EditorPropertyResource::set_use_filter(bool p_use)
{
	use_filter = p_use;
	if (sub_inspector) {
		update_property();
	}
}

void EditorPropertyResource::set_keying(bool p_keying)
{
	EditorProperty::set_keying(p_keying);
	if (sub_inspector) {
		sub_inspector->set_keying(p_keying);
	}
}

void EditorPropertyResource::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_EXIT_TREE: {
		const EditorInspector* ei = get_parent_inspector();
		const EditorInspector* main_ei = InspectorDock::get_inspector_singleton();
		if (ei && main_ei && ei != main_ei && !main_ei->is_ancestor_of(ei)) {
			fold_resource();
		}
	} break;
	}
}



void EditorPropertyMultilineText::_update_theme() {}

void EditorProperty::set_label(String const&) {}

void EditorPropertyResource::_select_resource(Ref<Resource> const&, bool, bool) {}

void EditorProperty::_update_property_bg() {}

void EditorProperty::update_editor_property_status() {}

void EditorPropertyText::_text_changed(String const&) {}

void EditorProperty::set_name_fixed_size(float) {}

void EditorProperty::set_keying(bool) {}

void EditorPropertyResource::fold_resource() {}

void EditorPropertyQuaternion::_value_changed(double, String const&) {}

void EditorPropertyPath::_update_uid_icon() {}

void EditorPropertyPath::_path_selected(String const&) {}

void EditorPropertyLocale::_locale_selected(String const&) {}

void EditorPropertyLayers::setup(EditorPropertyLayers::LayerType) {}

EditorInspector* EditorProperty::get_parent_inspector() const {}

void EditorPropertyEnum::setup(Vector<String> const&) {}

void EditorProperty::deselect() {}

void EditorPropertyRID::update_property() {}

void EditorPropertyFlags::_set_read_only(bool p_read_only) {}


