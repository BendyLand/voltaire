/**************************************************************************/
/*  packed_scene_translation_parser_plugin.cpp                            */
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

#include "core/io/resource_loader.h"
#include "packed_scene_translation_parser_plugin.h"
#include "scene/resources/packed_scene.h"

void PackedSceneEditorTranslationParserPlugin::get_recognized_extensions(
	List<String>* r_extensions) const
{
	ResourceLoader::get_recognized_extensions_for_type("PackedScene", r_extensions);
}

bool PackedSceneEditorTranslationParserPlugin::match_property(
	const String& p_property_name, const String& p_node_type)
{
	for (const String& lookup_property : lookup_properties) {
		if (p_property_name.match(lookup_property)) {
			return true;
		}
	}
	return false;
}

PackedSceneEditorTranslationParserPlugin::PackedSceneEditorTranslationParserPlugin()
{
	// Scene Node's properties containing strings that will be fetched for translation.
	lookup_properties.insert("text");
	lookup_properties.insert("*_text");
	lookup_properties.insert("popup/*/text");
	lookup_properties.insert("title");
	lookup_properties.insert("filters");
	lookup_properties.insert("script");
	lookup_properties.insert("item_*/text");
	lookup_properties.insert("accessibility_name");
	lookup_properties.insert("accessibility_description");

	// Exception list (to prevent false positives).
	exception_list.insert("LineEdit", {"text"});
	exception_list.insert("TextEdit", {"text"});
	exception_list.insert("CodeEdit", {"text"});
	exception_list.insert("Control", {"tooltip_text"});
}


