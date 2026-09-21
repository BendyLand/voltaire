/**************************************************************************/
/*  shader_globals_override.cpp                                           */
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

#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"
#include "shader_globals_override.h"

StringName* ShaderGlobalsOverride::_remap(const StringName& p_name) const
{
	StringName* r = param_remaps.getptr(p_name);
	if (!r) {
		// not cached, do caching
		String p = p_name;
		if (p.begins_with("params/")) {
			String q = p.replace_first("params/", "");
			param_remaps[p] = q;
			r = param_remaps.getptr(p);
		}
	}

	return r;
}

PackedStringArray ShaderGlobalsOverride::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (!active) {
		warnings.push_back(RTR("ShaderGlobalsOverride is not active because another node of the "
							   "same type is in the scene."));
	}

	return warnings;
}


