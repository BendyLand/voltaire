/**************************************************************************/
/*  grid_container.cpp                                                    */
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

#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "grid_container.h"
#include "scene/theme/theme_db.h"

void GridContainer::_notification(int p_what)
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

void GridContainer::set_columns(int p_columns)
{
	ERR_FAIL_COND(p_columns < 1);

	if (columns == p_columns) {
		return;
	}

	columns = p_columns;
	queue_sort();
	update_minimum_size();
}

int GridContainer::get_columns() const { return columns; }

int GridContainer::get_h_separation() const { return theme_cache.h_separation; }

void GridContainer::_bind_methods() {}

Size2 GridContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	RBMap<int, int> col_minw;
	RBMap<int, int> row_minh;

	int max_row = 0;
	int max_col = 0;

	int valid_controls_index = 0;
	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}
		int row = valid_controls_index / columns;
		int col = valid_controls_index % columns;
		valid_controls_index++;

		Size2i ms = p_use_desired_sizes ? c->get_bound_desired_size() : c->get_bound_minimum_size();
		if (col_minw.has(col)) {
			col_minw[col] = MAX(col_minw[col], ms.width);
		}
		else {
			col_minw[col] = ms.width;
		}

		if (row_minh.has(row)) {
			row_minh[row] = MAX(row_minh[row], ms.height);
		}
		else {
			row_minh[row] = ms.height;
		}
		max_col = MAX(col, max_col);
		max_row = MAX(row, max_row);
	}

	Size2 ms;

	for (const KeyValue<int, int>& E : col_minw) {
		ms.width += E.value;
	}

	for (const KeyValue<int, int>& E : row_minh) {
		ms.height += E.value;
	}

	ms.height += theme_cache.v_separation * max_row;
	ms.width += theme_cache.h_separation * max_col;

	return ms;
}

Size2 GridContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GridContainer::get_desired_size() const { return _get_minimum_size(true); }


