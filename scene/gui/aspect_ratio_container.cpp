/**************************************************************************/
/*  aspect_ratio_container.cpp                                            */
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

#include "aspect_ratio_container.h"
#include "scene/gui/texture_rect.h"

Size2 AspectRatioContainer::get_minimum_size() const
{
	Size2 ms;
	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}
		Size2 minsize = c->get_bound_minimum_size();
		ms = ms.max(minsize);
	}
	return ms;
}

void AspectRatioContainer::set_ratio(float p_ratio)
{
	if (ratio == p_ratio) {
		return;
	}
	ratio = p_ratio;
	queue_sort();
}

void AspectRatioContainer::set_stretch_mode(StretchMode p_mode)
{
	if (stretch_mode == p_mode) {
		return;
	}
	stretch_mode = p_mode;
	queue_sort();
}

void AspectRatioContainer::set_alignment_horizontal(AlignmentMode p_alignment_horizontal)
{
	if (alignment_horizontal == p_alignment_horizontal) {
		return;
	}
	alignment_horizontal = p_alignment_horizontal;
	queue_sort();
}

void AspectRatioContainer::set_alignment_vertical(AlignmentMode p_alignment_vertical)
{
	if (alignment_vertical == p_alignment_vertical) {
		return;
	}
	alignment_vertical = p_alignment_vertical;
	queue_sort();
}

Vector<int> AspectRatioContainer::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> AspectRatioContainer::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}


