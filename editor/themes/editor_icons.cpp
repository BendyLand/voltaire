/**************************************************************************/
/*  editor_icons.cpp                                                      */
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

#include "editor/editor_string_names.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_icons.gen.h"
#include "editor/themes/editor_scale.h"
#include "editor_icons.h"
#include "modules/svg/image_loader_svg.h"
#include "scene/resources/dpi_texture.h"

void editor_configure_icons(bool p_dark_icon_and_font)
{
	if (p_dark_icon_and_font) {
		ImageLoaderSVG::set_forced_color_map(HashMap<Color, Color>());
	}
	else {
		ImageLoaderSVG::set_forced_color_map(EditorColorMap::get_color_conversion_map());
	}
}

float get_gizmo_handle_scale(const String& p_gizmo_handle_name, float p_gizmo_handle_scale)
{
	if (p_gizmo_handle_scale > 1.0f) {
		// The names of the icons that require additional scaling.
		static HashSet<StringName> gizmo_to_scale;
		if (gizmo_to_scale.is_empty()) {
			gizmo_to_scale.insert("EditorHandle");
			gizmo_to_scale.insert("EditorHandleAdd");
			gizmo_to_scale.insert("EditorHandleDisabled");
			gizmo_to_scale.insert("EditorCurveHandle");
			gizmo_to_scale.insert("EditorPathSharpHandle");
			gizmo_to_scale.insert("EditorPathSmoothHandle");
			gizmo_to_scale.insert("EditorControlAnchor");
		}

		if (gizmo_to_scale.has(p_gizmo_handle_name)) {
			return EDSCALE * p_gizmo_handle_scale;
		}
	}

	return EDSCALE;
}

void editor_copy_icons(const Ref<Theme>& p_theme, const Ref<Theme>& p_old_theme)
{
	for (int i = 0; i < editor_icons_count; i++) {
		p_theme->set_icon(editor_icons_names[i], EditorStringName(EditorIcons),
			p_old_theme->get_icon(editor_icons_names[i], EditorStringName(EditorIcons)));
	}
}

// Returns the SVG code for the default project icon.
String get_default_project_icon()
{
	// FIXME: This icon can probably be predefined in editor_icons.gen.h so we don't have to look
	// up.
	for (int i = 0; i < editor_icons_count; i++) {
		if (strcmp(editor_icons_names[i], "DefaultProjectIcon") == 0) {
			return String(editor_icons_sources[i]);
		}
	}
	return String();
}


