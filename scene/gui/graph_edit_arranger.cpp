/**************************************************************************/
/*  graph_edit_arranger.cpp                                               */
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

#include <cfloat> // FLT_MIN, FLT_MAX
#include "graph_edit_arranger.h"
#include "scene/gui/graph_edit.h"

int GraphEditArranger::_set_operations(
	SET_OPERATIONS p_operation, HashSet<StringName>& r_u, const HashSet<StringName>& r_v)
{
	switch (p_operation) {
	case GraphEditArranger::IS_EQUAL: {
		for (const StringName& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return r_u.size() == r_v.size();
	} break;
	case GraphEditArranger::IS_SUBSET: {
		if (r_u.size() == r_v.size() && !r_u.size()) {
			return 1;
		}
		for (const StringName& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return 1;
	} break;
	case GraphEditArranger::DIFFERENCE: {
		Vector<StringName> common;
		for (const StringName& E : r_u) {
			if (r_v.has(E)) {
				common.append(E);
			}
		}
		for (const StringName& E : common) {
			r_u.erase(E);
		}
		return r_u.size();
	} break;
	case GraphEditArranger::UNION: {
		for (const StringName& E : r_v) {
			if (!r_u.has(E)) {
				r_u.insert(E);
			}
		}
		return r_u.size();
	} break;
	default:
		break;
	}
	return -1;
}

HashMap<int, Vector<StringName>> GraphEditArranger::_layering(
	const HashSet<StringName>& r_selected_nodes,
	const HashMap<StringName, HashSet<StringName>>& r_upper_neighbours)
{
	HashMap<int, Vector<StringName>> l;

	HashSet<StringName> p(r_selected_nodes);
	HashSet<StringName> q(r_selected_nodes);
	HashSet<StringName> u;
	HashSet<StringName> z;
	int current_layer = 0;
	bool selected = false;

	while (!_set_operations(GraphEditArranger::IS_EQUAL, q, u)) {
		_set_operations(GraphEditArranger::DIFFERENCE, p, u);
		for (const StringName& E : p) {
			HashSet<StringName> n(r_upper_neighbours[E]);
			if (_set_operations(GraphEditArranger::IS_SUBSET, n, z)) {
				Vector<StringName> t;
				t.push_back(E);
				if (!l.has(current_layer)) {
					l.insert(current_layer, Vector<StringName>{});
				}
				selected = true;
				t.append_array(l[current_layer]);
				l.insert(current_layer, t);
				u.insert(E);
			}
		}
		if (!selected) {
			current_layer++;
			uint32_t previous_size_z = z.size();
			_set_operations(GraphEditArranger::UNION, z, u);
			if (z.size() == previous_size_z) {
				WARN_PRINT(
					"Graph contains cycle(s). The cycle(s) will not be rearranged accurately.");
				Vector<StringName> t;
				if (l.has(0)) {
					t.append_array(l[0]);
				}
				for (const StringName& E : p) {
					t.push_back(E);
				}
				l.insert(0, t);
				break;
			}
		}
		selected = false;
	}

	return l;
}


