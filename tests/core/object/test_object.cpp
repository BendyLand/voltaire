/**************************************************************************/
/*  test_object.cpp                                                       */
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

#include "core/templates/mem_unique_ptr.h"
#include "tests/test_macros.h"

TEST_FORCE_LINK(test_object)

#include "tests/signal_watcher.h"

namespace TestObject
{

class _TestDerivedObject
{
	int property_value;

protected:
	static void _bind_methods() {}

public:
	mem_unique_ptr<Object> obj;

	void set_property(int value) { property_value = value; }

	int get_property() const { return property_value; }
};

class _MockScriptInstance : public ScriptInstance
{
	StringName property_name = "NO_NAME";
	Variant property_value;

public:
	bool set(const StringName& p_name, const Variant& p_value) override
	{
		property_name = p_name;
		property_value = p_value;
		return true;
	}

	bool get(const StringName& p_name, Variant& r_ret) const override
	{
		if (property_name == p_name) {
			r_ret = property_value;
			return true;
		}
		return false;
	}

	void get_property_list(List<PropertyInfo>* p_properties) const override {}

	Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid) const override
	{
		return Variant::PACKED_FLOAT32_ARRAY;
	}

	virtual void validate_property(PropertyInfo& p_property) const override {}

	bool property_can_revert(const StringName& p_name) const override { return false; }

	bool property_get_revert(const StringName& p_name, Variant& r_ret) const override
	{
		return false;
	}

	void get_method_list(List<MethodInfo>* p_list) const override {}

	bool has_method(const StringName& p_method) const override { return false; }

	int get_method_argument_count(
		const StringName& p_method, bool* r_is_valid = nullptr) const override
	{
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return 0;
	}

	Variant callp(const StringName& p_method, const Variant** p_args, int p_argcount,
		Callable::CallError& r_error) override
	{
		return Variant();
	}

	void notification(int p_notification, bool p_reversed = false) override {}

	Ref<Script> get_script() const override { return Ref<Script>(); }

	const Variant get_rpc_config() const override { return Variant(); }

	ScriptLanguage* get_language() override { return nullptr; }
};

} // namespace TestObject


