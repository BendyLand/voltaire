/**************************************************************************/
/*  class_db.cpp                                                          */
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

#include "class_db.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "core/templates/sort_array.h"
#include "core/version.h"

#define ERR_FAIL_NO_CLASS(m_type, m_class)                                                         \
	ERR_FAIL_NULL_MSG(m_type, vformat("Cannot get class \"%s\".", m_class))

#ifdef DEBUG_ENABLED

MethodDefinition D_METHODP(const char* p_name, const char* const** p_args, uint32_t p_argcount)
{
	MethodDefinition md;
	md.name = StringName(p_name);
	md.args.resize(p_argcount);
	for (uint32_t i = 0; i < p_argcount; i++) {
		md.args.write[i] = StringName(*p_args[i]);
	}
	return md;
}

#endif // DEBUG_ENABLED

#ifdef TOOLS_ENABLED

class PlaceholderExtensionInstance
{
	StringName class_name;
	HashMap<StringName, Variant> properties;

	// Checks if a property is from a runtime class, and not a non-runtime base class.
	bool is_runtime_property(const StringName& p_property_name) { return false; }

public:
	PlaceholderExtensionInstance(const StringName& p_class_name) { class_name = p_class_name; }

	void set(const StringName& p_name, const Variant& p_value, bool& r_valid)
	{
		r_valid = is_runtime_property(p_name);
		if (r_valid) {
			properties[p_name] = p_value;
		}
	}

	Variant get(const StringName& p_name, bool& r_valid)
	{
		const Variant* value = properties.getptr(p_name);
		Variant ret;

		if (value) {
			ret = *value;
			r_valid = true;
		}
		return ret;
	}
};
#endif

static MethodInfo info_from_bind(MethodBind* p_method)
{
	MethodInfo minfo;
	minfo.name = p_method->get_name();
	minfo.id = p_method->get_method_id();

	for (int i = 0; i < p_method->get_argument_count(); i++) {
		minfo.arguments.push_back(p_method->get_argument_info(i));
	}

	minfo.return_val = p_method->get_return_info();
	minfo.flags = p_method->get_hint_flags();

	for (int i = 0; i < p_method->get_argument_count(); i++) {
		if (p_method->has_default_argument(i)) {
			minfo.default_arguments.push_back(p_method->get_default_argument(i));
		}
	}

	return minfo;
}

#undef ERR_FAIL_NO_CLASS


