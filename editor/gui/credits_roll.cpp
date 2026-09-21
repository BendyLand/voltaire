/**************************************************************************/
/*  credits_roll.cpp                                                      */
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

#include "core/authors.gen.h"
#include "core/donors.gen.h"
#include "core/input/input.h"
#include "core/license.gen.h"
#include "core/string/string_builder.h"
#include "credits_roll.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/project_manager/project_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

Label* CreditsRoll::_create_label(const String& p_with_text, LabelSize p_size)
{
	Label* label = memnew(Label);
	label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	label->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	label->set_text(p_with_text);

	switch (p_size) {
	case LabelSize::NORMAL: {
		label->add_theme_font_size_override(SceneStringName(font_size), font_size_normal);
		label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	} break;

	case LabelSize::HEADER: {
		label->add_theme_font_size_override(SceneStringName(font_size), font_size_header);
		label->add_theme_font_override(SceneStringName(font), bold_font.ptr());
	} break;

	case LabelSize::BIG_HEADER: {
		label->add_theme_font_size_override(SceneStringName(font_size), font_size_big_header);
		label->add_theme_font_override(SceneStringName(font), bold_font.ptr());
	} break;
	}
	content->add_child(label);
	return label;
}

void CreditsRoll::_create_nothing(int p_size)
{
	if (p_size == -1) {
		p_size = 30 * EDSCALE;
	}
	Control* c = memnew(Control);
	c->set_custom_minimum_size(Vector2(0, p_size));
	content->add_child(c);
}

String CreditsRoll::_build_string(const char* const* p_from) const
{
	StringBuilder sb;

	while (*p_from) {
		sb.append(String::utf8(*p_from));
		sb.append("\n");
		p_from++;
	}
	return sb.as_string();
}

void CreditsRoll::_visibility_changed()
{
	if (!is_visible()) {
		mouse_enabled = false;
		set_process_internal(false);
		set_process_input(false);
	}
}

void CreditsRoll::input(const Ref<InputEvent>& p_event)
{
	// Block inputs from going elsewhere while the credits roll.
	get_tree()->get_root()->set_input_as_handled();
}

CreditsRoll::CreditsRoll()
{
	ColorRect* background = memnew(ColorRect);
	background->set_color(Color(0, 0, 0, 1));
	background->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	add_child(background);

	content = memnew(VBoxContainer);
	content->set_grow_direction_preset(Control::PRESET_VCENTER_WIDE);
	add_child(content);
}


