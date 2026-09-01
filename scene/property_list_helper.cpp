/**************************************************************************/
/*  property_list_helper.cpp                                              */
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

#include "property_list_helper.h"

HashMap<StringName, Vector<PropertyListHelper*>> PropertyListHelper::base_helpers; // static

void PropertyListHelper::clear_base_helpers()
{ // static
	for (KeyValue<StringName, Vector<PropertyListHelper*>>& E : base_helpers) {
		for (PropertyListHelper* helper : E.value) {
			helper->clear();
		}
	}
	base_helpers.clear();
}

void PropertyListHelper::register_base_helper(
	const StringName& p_class_name, PropertyListHelper* p_helper)
{ // static
	base_helpers[p_class_name].push_back(p_helper);
}

Vector<PropertyListHelper*> PropertyListHelper::get_helpers_for_class(
	const StringName& p_class_name)
{
	if (base_helpers.has(p_class_name)) {
		return base_helpers[p_class_name];
	}
	else {
		return Vector<PropertyListHelper*>();
	}
}

void PropertyListHelper::set_prefix(const String& p_prefix) { prefix = p_prefix; }

#ifdef TOOLS_ENABLED

#endif

bool PropertyListHelper::property_can_revert(const String& p_property) const
{
	return is_property_valid(p_property);
}


