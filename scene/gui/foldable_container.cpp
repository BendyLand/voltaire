/**************************************************************************/
/*  foldable_container.cpp                                                */
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

#include "foldable_container.h"
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"

Size2 FoldableContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Container::get_inner_combined_maximum_size();

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	return ms;
}

bool FoldableContainer::is_folded() const { return folded; }

Ref<FoldableGroup> FoldableContainer::get_foldable_group() const { return foldable_group; }

String FoldableContainer::get_title() const { return title; }

HorizontalAlignment FoldableContainer::get_title_alignment() const { return title_alignment; }

String FoldableContainer::get_language() const { return language; }

Control::TextDirection FoldableContainer::get_title_text_direction() const
{
	return title_text_direction;
}

TextServer::OverrunBehavior FoldableContainer::get_title_text_overrun_behavior() const
{
	return overrun_behavior;
}

FoldableContainer::TitlePosition FoldableContainer::get_title_position() const
{
	return title_position;
}

void FoldableContainer::add_title_bar_control(Control* p_control)
{
	ERR_FAIL_NULL(p_control);
	if (p_control->get_parent()) {
		p_control->get_parent()->remove_child(p_control);
		ERR_FAIL_COND_MSG(
			p_control->get_parent() != nullptr, "Failed to remove control from parent.");
	}
	add_child(p_control, false, INTERNAL_MODE_FRONT);
	title_controls.push_back(p_control);
}

void FoldableContainer::remove_title_bar_control(Control* p_control)
{
	ERR_FAIL_NULL(p_control);

	int64_t index = title_controls.find(p_control);
	ERR_FAIL_COND_MSG(index == -1, "Can't remove control from title bar.");

	title_controls.remove_at(index);
	remove_child(p_control);
}

bool FoldableContainer::has_point(const Point2& p_point) const
{
	if (folded) {
		return _get_title_rect().has_point(p_point);
	}
	return Control::has_point(p_point);
}

void FoldableContainer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		RID ci = get_canvas_item();
		Size2 size = get_size();
		int h_separation = _get_h_separation();

		Ref<StyleBox> title_style = _get_title_style();
		Ref<Texture2D> icon = _get_title_icon();

		real_t title_controls_width = _get_title_controls_width();
		if (title_controls_width > 0) {
			title_controls_width += h_separation;
		}

		const Rect2 title_rect = _get_title_rect();
		_draw_flippable_stylebox(title_style, title_rect);

		Size2 title_ms = title_style->get_minimum_size();
		int title_text_width = size.width - title_ms.width;

		int title_style_ofs = (title_position == POSITION_TOP)
								  ? title_style->get_margin(SIDE_TOP)
								  : title_style->get_margin(SIDE_BOTTOM);
		Point2 title_text_pos(title_style->get_margin(SIDE_LEFT), title_style_ofs);
		title_text_pos.y += MAX(
			(title_minimum_size.height - title_ms.height - text_buf->get_size().height) * 0.5, 0);

		title_text_width -= icon->get_width() + h_separation + title_controls_width;
		Point2 icon_pos(
			0, MAX((title_minimum_size.height - title_ms.height - icon->get_height()) * 0.5, 0) +
				   title_style_ofs);

		bool rtl = is_layout_rtl();
		if (rtl) {
			icon_pos.x = size.width - title_style->get_margin(SIDE_RIGHT) - icon->get_width();
			title_text_pos.x += title_controls_width;
		}
		else {
			icon_pos.x = title_style->get_margin(SIDE_LEFT);
			title_text_pos.x += icon->get_width() + h_separation;
		}
		icon->draw(ci, title_rect.position + icon_pos);

		Color font_color =
			folded ? theme_cache.title_collapsed_font_color : theme_cache.title_font_color;
		if (is_hovering) {
			font_color = theme_cache.title_hovered_font_color;
		}
		text_buf->set_width(title_text_width);

		if (title_text_width > 0) {
			if (theme_cache.title_font_outline_size > 0 &&
				theme_cache.title_font_outline_color.a > 0) {
				text_buf->draw_outline(ci, title_rect.position + title_text_pos,
					theme_cache.title_font_outline_size, theme_cache.title_font_outline_color);
			}
			text_buf->draw(ci, title_rect.position + title_text_pos, font_color);
		}

		if (!folded) {
			Rect2 panel_rect(
				Point2(0, (title_position == POSITION_TOP) ? title_minimum_size.height : 0),
				Size2(size.width, size.height - title_minimum_size.height));
			_draw_flippable_stylebox(theme_cache.panel_style, panel_rect);
		}

		if (has_focus(true)) {
			Rect2 focus_rect = folded ? title_rect : Rect2(Point2(), size);
			_draw_flippable_stylebox(theme_cache.focus_style, focus_rect);
		}
	} break;
	}
}

real_t FoldableContainer::_get_title_controls_width() const
{
	real_t width = 0.0;
	int visible_controls = 0;
	for (const Control* control : title_controls) {
		if (control->is_visible()) {
			width += control->get_bound_minimum_size().x;
			visible_controls++;
		}
	}
	if (visible_controls > 1) {
		width += _get_h_separation() * (visible_controls - 1);
	}
	return width;
}

Ref<StyleBox> FoldableContainer::_get_title_style() const
{
	if (is_hovering) {
		return folded ? theme_cache.title_collapsed_hover_style : theme_cache.title_hover_style;
	}
	return folded ? theme_cache.title_collapsed_style : theme_cache.title_style;
}

Ref<Texture2D> FoldableContainer::_get_title_icon() const
{
	if (!folded) {
		return (title_position == POSITION_TOP) ? theme_cache.expanded_arrow
												: theme_cache.expanded_arrow_mirrored;
	}
	else if (is_layout_rtl()) {
		return theme_cache.folded_arrow_mirrored;
	}
	return theme_cache.folded_arrow;
}

Rect2 FoldableContainer::_get_title_rect() const
{
	return Rect2(0,
		(title_position == POSITION_TOP) ? 0 : (get_size().height - title_minimum_size.height),
		get_size().width, title_minimum_size.height);
}

void FoldableContainer::_update_title_min_size() const
{
	Ref<StyleBox> title_style =
		folded ? theme_cache.title_collapsed_style : theme_cache.title_style;
	Ref<Texture2D> icon = _get_title_icon();
	Size2 title_ms = title_style->get_minimum_size();
	int h_separation = _get_h_separation();

	title_minimum_size = title_ms;
	title_minimum_size.width += icon->get_width();

	if (!title.is_empty()) {
		title_minimum_size.width += h_separation;
		Size2 text_size = text_buf->get_size();
		title_minimum_size.height += MAX(text_size.height, icon->get_height());
		if (overrun_behavior == TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING) {
			title_minimum_size.width += text_size.width;
		}
	}
	else {
		title_minimum_size.height += icon->get_height();
	}

	if (!title_controls.is_empty()) {
		real_t controls_height = 0;
		int visible_controls = 0;

		for (const Control* control : title_controls) {
			if (!control->is_visible()) {
				continue;
			}
			Vector2 size = control->get_bound_minimum_size();
			title_minimum_size.width += size.width;
			controls_height = MAX(controls_height, size.height);
			visible_controls++;
		}
		if (visible_controls > 0) {
			title_minimum_size.width += h_separation * visible_controls;
		}
		title_minimum_size.height =
			MAX(title_minimum_size.height, title_ms.height + controls_height);
	}
}

HorizontalAlignment FoldableContainer::_get_actual_alignment() const
{
	if (is_layout_rtl()) {
		if (title_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			return HORIZONTAL_ALIGNMENT_LEFT;
		}
		else if (title_alignment == HORIZONTAL_ALIGNMENT_LEFT) {
			return HORIZONTAL_ALIGNMENT_RIGHT;
		}
	}
	return title_alignment;
}

void FoldableContainer::_update_group()
{
	foldable_group->updating_group = true;
	for (FoldableContainer* container : foldable_group->containers) {
		if (container != this) {
			container->set_folded(true);
		}
	}
	foldable_group->updating_group = false;
}

void FoldableContainer::_draw_flippable_stylebox(
	const Ref<StyleBox> p_stylebox, const Rect2& p_rect)
{
	if (title_position == POSITION_BOTTOM) {
		Rect2 rect(-p_rect.position, p_rect.size);
		draw_set_transform(
			Point2(0.0, p_stylebox->get_draw_rect(rect).size.height), 0.0, Size2(1.0, -1.0));
		p_stylebox->draw(get_canvas_item(), rect);
		draw_set_transform_matrix(Transform2D());
	}
	else {
		p_stylebox->draw(get_canvas_item(), p_rect);
	}
}

FoldableContainer::FoldableContainer(const String& p_text)
{
	text_buf.instantiate();
	set_title(p_text);
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
}

FoldableContainer::~FoldableContainer()
{
	if (foldable_group.is_valid()) {
		foldable_group->containers.erase(this);
	}
}

FoldableContainer* FoldableGroup::get_expanded_container() const
{
	for (FoldableContainer* container : containers) {
		if (!container->is_folded()) {
			return container;
		}
	}

	return nullptr;
}

void FoldableGroup::set_allow_folding_all(bool p_enabled)
{
	allow_folding_all = p_enabled;
	if (!allow_folding_all && !get_expanded_container() && containers.size() > 0) {
		updating_group = true;
		(*containers.begin())->set_folded(false);
		updating_group = false;
	}
}

bool FoldableGroup::is_allow_folding_all() const { return allow_folding_all; }

void FoldableGroup::get_containers(List<FoldableContainer*>* r_containers) const
{
	for (FoldableContainer* container : containers) {
		r_containers->push_back(container);
	}
}

FoldableGroup::FoldableGroup() { set_local_to_scene(true); }


