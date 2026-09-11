/**************************************************************************/
/*  box_container.cpp                                                     */
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

#include "box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/margin_container.h"
#include "scene/theme/theme_db.h"

struct _MinSizeCache
{
	int min_size = 0;
	int desired_size = 0;
	int max_size = -1;
	real_t stretch_ratio = 0;
	bool will_stretch = false;
	int final_size = 0;
};

Size2 BoxContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	/* Calculate MINIMUM SIZE */

	Size2i minimum;

	bool first = true;

	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}

		Size2i size = p_use_desired_sizes ? c->get_bound_desired_size().ceil()
										  : c->get_bound_minimum_size().ceil();

		if (vertical) { /* VERTICAL */

			if (size.width > minimum.width) {
				minimum.width = size.width;
			}

			minimum.height += size.height + (first ? 0 : theme_cache.separation);

		}
		else { /* HORIZONTAL */

			if (size.height > minimum.height) {
				minimum.height = size.height;
			}

			minimum.width += size.width + (first ? 0 : theme_cache.separation);
		}

		first = false;
	}

	return minimum;
}

Size2 BoxContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 BoxContainer::get_desired_size() const { return _get_minimum_size(true); }

void BoxContainer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		update_minimum_size();
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED:
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		queue_sort();
	} break;
	}
}

void BoxContainer::set_alignment(AlignmentMode p_alignment)
{
	if (alignment == p_alignment) {
		return;
	}
	alignment = p_alignment;
	_resort();
}

BoxContainer::AlignmentMode BoxContainer::get_alignment() const { return alignment; }

bool BoxContainer::is_vertical() const { return vertical; }

void BoxContainer::set_reverse_sort(bool p_reverse_sort)
{
	if (reverse_sort == p_reverse_sort) {
		return;
	}
	reverse_sort = p_reverse_sort;
	queue_sort();
}

bool BoxContainer::is_reverse_sort() const { return reverse_sort; }

Control* BoxContainer::add_spacer(bool p_begin)
{
	Control* c = memnew(Control);
	c->set_mouse_filter(MOUSE_FILTER_PASS); // allow spacer to pass mouse events

	if (vertical) {
		c->set_v_size_flags(SIZE_EXPAND_FILL);
	}
	else {
		c->set_h_size_flags(SIZE_EXPAND_FILL);
	}

	add_child(c);
	if (p_begin) {
		move_child(c, 0);
	}

	return c;
}

Vector<int> BoxContainer::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (!vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> BoxContainer::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

BoxContainer::BoxContainer(bool p_vertical) { vertical = p_vertical; }

void BoxContainer::_bind_methods() {}

MarginContainer* VBoxContainer::add_margin_child(
	const String& p_label, Control* p_control, bool p_expand)
{
	Label* l = memnew(Label);
	l->set_theme_type_variation("HeaderSmall");
	l->set_text(p_label);
	add_child(l);
	MarginContainer* mc = memnew(MarginContainer);
	mc->add_child(p_control, true);
	add_child(mc);
	if (p_expand) {
		mc->set_v_size_flags(SIZE_EXPAND_FILL);
	}
	p_control->set_accessibility_name(p_label);

	return mc;
}


