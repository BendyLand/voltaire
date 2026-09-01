/**************************************************************************/
/*  theme_editor_preview.cpp                                              */
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
#include "core/io/resource_loader.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"
#include "scene/resources/packed_scene.h"
#include "scene/theme/theme_db.h"
#include "theme_editor_preview.h"

void ScalableContainer::_notification(int p_what)
{
	if (EDSCALE == 1 || p_what != NOTIFICATION_SORT_CHILDREN) {
		return;
	}

	Size2 size = get_size() / EDSCALE;
	size.width -= get_margin_size(SIDE_LEFT) + get_margin_size(SIDE_RIGHT);
	size.height -= get_margin_size(SIDE_TOP) + get_margin_size(SIDE_BOTTOM);

	for (Node* child : iterate_children()) {
		Control* control = as_sortable_control(child);
		if (control) {
			fit_child_in_rect(control, Rect2(control->get_position(), size));
		}
	}
}

Size2 ScalableContainer::get_minimum_size() const
{
	return MarginContainer::get_minimum_size() * EDSCALE;
}

ScalableContainer::ScalableContainer()
{
	set_offset_transform_enabled(true);
	set_offset_transform_pivot_ratio(Point2());
	set_offset_transform_visual_only(false);
	set_offset_transform_scale(Size2(EDSCALE, EDSCALE));
}

void ThemeEditorPreview::set_preview_theme(const Ref<Theme>& p_theme)
{
	preview_content->set_theme(p_theme);
}

void ThemeEditorPreview::add_preview_overlay(Control* p_overlay)
{
	preview_overlay->add_child(p_overlay);
	p_overlay->hide();
}

void ThemeEditorPreview::_picker_button_cbk()
{
	picker_overlay->set_visible(picker_button->is_pressed());
	if (picker_button->is_pressed()) {
		_reset_picker_overlay();
	}
}

void ThemeEditorPreview::_reset_picker_overlay()
{
	hovered_control = nullptr;
	picker_overlay->queue_redraw();
}

void ThemeEditorPreview::_notification(int p_what)
{
	switch (p_what) {
	// Due to NOTIFICATION_READY being called only once, and theme contexts being destroyed on node
	// removal, this is the notification needed, as it can be triggered indefinitely.
	case NOTIFICATION_POST_ENTER_TREE: {
		Vector<Ref<Theme>> preview_themes;
		preview_themes.push_back(ThemeDB::get_singleton()->get_default_theme());
		ThemeDB::get_singleton()->create_theme_context(preview_root, preview_themes);
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		picker_button->set_button_icon(get_editor_theme_icon(SNAME("ColorPick")));

		theme_cache.preview_picker_overlay =
			get_theme_stylebox(SNAME("preview_picker_overlay"), SNAME("ThemeEditor"));
		theme_cache.preview_picker_overlay_color =
			get_theme_color(SNAME("preview_picker_overlay_color"), SNAME("ThemeEditor"));
		theme_cache.preview_picker_label =
			get_theme_stylebox(SNAME("preview_picker_label"), SNAME("ThemeEditor"));
		theme_cache.preview_picker_font =
			get_theme_font(SNAME("status_source"), EditorStringName(EditorFonts));
		theme_cache.font_size = get_theme_default_font_size();
	} break;
	}
}

void DefaultThemeEditorPreview::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		test_color_picker_button->set_custom_minimum_size(
			Size2(0,
				get_theme_constant(SNAME("inspector_property_height"), EditorStringName(Editor))) /
			EDSCALE);
	} break;
	}
}

DefaultThemeEditorPreview::DefaultThemeEditorPreview()
{
	set_oversampling_with_scale(OVERSAMPLING_WITH_SCALE_ENABLED);

	Panel* main_panel = memnew(Panel);
	preview_content->add_child(main_panel);

	MarginContainer* main_mc = memnew(MarginContainer);
	main_mc->add_theme_constant_override("margin_right", 4);
	main_mc->add_theme_constant_override("margin_top", 4);
	main_mc->add_theme_constant_override("margin_left", 4);
	main_mc->add_theme_constant_override("margin_bottom", 4);
	preview_content->add_child(main_mc);

	HBoxContainer* main_hb = memnew(HBoxContainer);
	main_mc->add_child(main_hb);
	main_hb->add_theme_constant_override("separation", 20);

	VBoxContainer* first_vb = memnew(VBoxContainer);
	main_hb->add_child(first_vb);
	first_vb->set_h_size_flags(SIZE_EXPAND_FILL);
	first_vb->add_theme_constant_override("separation", 10);

	first_vb->add_child(memnew(Label("Label")));

	first_vb->add_child(memnew(Button("Button")));
	Button* bt = memnew(Button);
	bt->set_text(TTR("Toggle Button"));
	bt->set_toggle_mode(true);
	bt->set_pressed(true);
	first_vb->add_child(bt);
	bt = memnew(Button);
	bt->set_text(TTR("Disabled Button"));
	bt->set_disabled(true);
	first_vb->add_child(bt);
	Button* tb = memnew(Button);
	tb->set_flat(true);
	tb->set_text("Flat Button");
	first_vb->add_child(tb);

	CheckButton* cb = memnew(CheckButton);
	cb->set_text("CheckButton");
	first_vb->add_child(cb);
	CheckBox* cbx = memnew(CheckBox);
	cbx->set_text("CheckBox");
	first_vb->add_child(cbx);

	MenuButton* test_menu_button = memnew(MenuButton);
	test_menu_button->set_text("MenuButton");
	test_menu_button->get_popup()->add_item(TTR("Item"));
	test_menu_button->get_popup()->add_item(TTR("Disabled Item"));
	test_menu_button->get_popup()->set_item_disabled(1, true);
	test_menu_button->get_popup()->add_separator();
	test_menu_button->get_popup()->add_check_item(TTR("Check Item"));
	test_menu_button->get_popup()->add_check_item(TTR("Checked Item"));
	test_menu_button->get_popup()->set_item_checked(4, true);
	test_menu_button->get_popup()->add_separator();
	test_menu_button->get_popup()->add_radio_check_item(TTR("Radio Item"));
	test_menu_button->get_popup()->add_radio_check_item(TTR("Checked Radio Item"));
	test_menu_button->get_popup()->set_item_checked(7, true);
	test_menu_button->get_popup()->add_separator(TTR("Named Separator"));

	PopupMenu* test_submenu = memnew(PopupMenu);
	test_menu_button->get_popup()->add_submenu_node_item(TTR("Submenu"), test_submenu);
	test_submenu->add_item(TTR("Subitem 1"));
	test_submenu->add_item(TTR("Subitem 2"));
	first_vb->add_child(test_menu_button);

	OptionButton* test_option_button = memnew(OptionButton);
	test_option_button->add_item("OptionButton");
	test_option_button->add_separator();
	test_option_button->add_item(TTR("Has"));
	test_option_button->add_item(TTR("Many"));
	test_option_button->add_item(TTR("Options"));
	first_vb->add_child(test_option_button);
	test_color_picker_button = memnew(ColorPickerButton);
	first_vb->add_child(test_color_picker_button);

	VBoxContainer* second_vb = memnew(VBoxContainer);
	second_vb->set_h_size_flags(SIZE_EXPAND_FILL);
	main_hb->add_child(second_vb);
	second_vb->add_theme_constant_override("separation", 10);
	LineEdit* le = memnew(LineEdit);
	le->set_text("LineEdit");
	second_vb->add_child(le);
	le = memnew(LineEdit);
	le->set_text(TTR("Disabled LineEdit"));
	le->set_editable(false);
	second_vb->add_child(le);
	TextEdit* te = memnew(TextEdit);
	te->set_text("TextEdit");
	te->set_custom_minimum_size(Size2(0, 100));
	second_vb->add_child(te);
	second_vb->add_child(memnew(SpinBox));

	HBoxContainer* vhb = memnew(HBoxContainer);
	second_vb->add_child(vhb);
	vhb->set_custom_minimum_size(Size2(0, 100));
	vhb->add_child(memnew(VSlider));
	VScrollBar* vsb = memnew(VScrollBar);
	vsb->set_page(25);
	vhb->add_child(vsb);
	vhb->add_child(memnew(VSeparator));
	VBoxContainer* hvb = memnew(VBoxContainer);
	vhb->add_child(hvb);
	hvb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	hvb->set_h_size_flags(SIZE_EXPAND_FILL);
	hvb->add_child(memnew(HSlider));
	HScrollBar* hsb = memnew(HScrollBar);
	hsb->set_page(25);
	hvb->add_child(hsb);
	HSlider* hs = memnew(HSlider);
	hs->set_editable(false);
	hvb->add_child(hs);
	hvb->add_child(memnew(HSeparator));
	ProgressBar* pb = memnew(ProgressBar);
	pb->set_value(50);
	hvb->add_child(pb);

	VBoxContainer* third_vb = memnew(VBoxContainer);
	third_vb->set_h_size_flags(SIZE_EXPAND_FILL);
	third_vb->add_theme_constant_override("separation", 10);
	main_hb->add_child(third_vb);

	TabContainer* tc = memnew(TabContainer);
	third_vb->add_child(tc);
	tc->set_custom_minimum_size(Size2(0, 135));
	Control* tcc = memnew(Control);
	tcc->set_name(TTR("Tab 1"));
	tc->add_child(tcc);
	tcc = memnew(Control);
	tcc->set_name(TTR("Tab 2"));
	tc->add_child(tcc);
	tcc = memnew(Control);
	tcc->set_name(TTR("Tab 3"));
	tc->add_child(tcc);
	tc->set_tab_disabled(2, true);

	Tree* test_tree = memnew(Tree);
	third_vb->add_child(test_tree);
	test_tree->set_custom_minimum_size(Size2(0, 175));

	TreeItem* item = test_tree->create_item();
	item->set_text(0, "Tree");
	item = test_tree->create_item(test_tree->get_root());
	item->set_text(0, "Item");
	item = test_tree->create_item(test_tree->get_root());
	item->set_editable(0, true);
	item->set_text(0, TTR("Editable Item"));
	TreeItem* sub_tree = test_tree->create_item(test_tree->get_root());
	sub_tree->set_text(0, TTR("Subtree"));
	item = test_tree->create_item(sub_tree);
	item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	item->set_editable(0, true);
	item->set_text(0, "Check Item");
	item = test_tree->create_item(sub_tree);
	item->set_cell_mode(0, TreeItem::CELL_MODE_RANGE);
	item->set_editable(0, true);
	item->set_range_config(0, 0, 20, 0.1);
	item->set_range(0, 2);
	item = test_tree->create_item(sub_tree);
	item->set_cell_mode(0, TreeItem::CELL_MODE_RANGE);
	item->set_editable(0, true);
	item->set_text(0, TTR("Has,Many,Options"));
	item->set_range(0, 2);
}

void SceneThemeEditorPreview::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		reload_scene_button->set_button_icon(get_editor_theme_icon(SNAME("Reload")));
	} break;
	}
}

String SceneThemeEditorPreview::get_preview_scene_path() const
{
	if (loaded_scene.is_null()) {
		return "";
	}

	return loaded_scene->get_path();
}


