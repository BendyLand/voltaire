/**************************************************************************/
/*  flow_container.cpp                                                    */
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

#include "flow_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/theme/theme_db.h"

struct _LineData
{
	int child_count = 0;
	int min_line_height = 0;
	int min_line_length = 0;
	int stretch_avail = 0;
	float stretch_ratio_total = 0;
	bool is_filled = false;
};

Size2 FlowContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	Size2i minimum;

	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}

		Size2i size =
			p_use_desired_sizes ? c->get_bound_desired_size() : c->get_bound_minimum_size();

		if (vertical) { /* VERTICAL */
			minimum.height = MAX(minimum.height, size.height);
			minimum.width = cached_size;

		}
		else { /* HORIZONTAL */
			minimum.width = MAX(minimum.width, size.width);
			minimum.height = cached_size;
		}
	}

	return minimum;
}

Size2 FlowContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 FlowContainer::get_desired_size() const { return _get_minimum_size(true); }

Vector<int> FlowContainer::get_allowed_size_flags_horizontal() const
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

Vector<int> FlowContainer::get_allowed_size_flags_vertical() const
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

void FlowContainer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
		update_minimum_size();
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

int FlowContainer::get_line_count() const { return cached_line_count; }

int FlowContainer::get_line_max_child_count() const { return cached_line_max_child_count; }

void FlowContainer::set_alignment(AlignmentMode p_alignment)
{
	if (alignment == p_alignment) {
		return;
	}
	alignment = p_alignment;
	_resort();
}

FlowContainer::AlignmentMode FlowContainer::get_alignment() const { return alignment; }

void FlowContainer::set_last_wrap_alignment(LastWrapAlignmentMode p_last_wrap_alignment)
{
	if (last_wrap_alignment == p_last_wrap_alignment) {
		return;
	}
	last_wrap_alignment = p_last_wrap_alignment;
	_resort();
}

FlowContainer::LastWrapAlignmentMode FlowContainer::get_last_wrap_alignment() const
{
	return last_wrap_alignment;
}

bool FlowContainer::is_vertical() const { return vertical; }

void FlowContainer::set_reverse_fill(bool p_reverse_fill)
{
	if (reverse_fill == p_reverse_fill) {
		return;
	}
	reverse_fill = p_reverse_fill;
	_resort();
}

bool FlowContainer::is_reverse_fill() const { return reverse_fill; }

FlowContainer::FlowContainer(bool p_vertical) { vertical = p_vertical; }


