/**************************************************************************/
/*  resource_preloader.cpp                                                */
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

#include "core/templates/rb_set.h"
#include "resource_preloader.h"

void ResourcePreloader::rename_resource(const StringName& p_from_name, const StringName& p_to_name)
{
	ERR_FAIL_COND(!resources.has(p_from_name));

	Ref<Resource> res = resources[p_from_name];

	resources.erase(p_from_name);
	add_resource(p_to_name, res);
}

bool ResourcePreloader::has_resource(const StringName& p_name) const
{
	return resources.has(p_name);
}

Ref<Resource> ResourcePreloader::get_resource(const StringName& p_name) const
{
	ERR_FAIL_COND_V(!resources.has(p_name), Ref<Resource>());
	return resources[p_name];
}

Vector<String> ResourcePreloader::_get_resource_list() const
{
	Vector<String> res;
	res.resize(resources.size());
	int i = 0;
	for (const KeyValue<StringName, Ref<Resource>>& E : resources) {
		res.set(i, E.key);
		i++;
	}

	return res;
}

void ResourcePreloader::get_resource_list(List<StringName>* p_list)
{
	for (const KeyValue<StringName, Ref<Resource>>& E : resources) {
		p_list->push_back(E.key);
	}
}

void ResourcePreloader::_bind_methods() {}

ResourcePreloader::ResourcePreloader() {}


