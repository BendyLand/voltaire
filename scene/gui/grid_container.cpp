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

<<<<<<< HEAD
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

=======
>>>>>>> fix/remove-object
int GridContainer::get_columns() const { return columns; }

int GridContainer::get_h_separation() const { return theme_cache.h_separation; }

Size2 GridContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GridContainer::get_desired_size() const { return _get_minimum_size(true); }



Size2 GridContainer::_get_minimum_size(bool) const {}

void GridContainer::set_columns(int) {}
