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

String SceneThemeEditorPreview::get_preview_scene_path() const
{
	if (loaded_scene.is_null()) {
		return "";
	}

	return loaded_scene->get_path();
}


