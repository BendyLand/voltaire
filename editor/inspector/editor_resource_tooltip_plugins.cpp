/**************************************************************************/
/*  editor_resource_tooltip_plugins.cpp                                   */
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
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/themes/editor_scale.h"
#include "editor_resource_tooltip_plugins.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"

VBoxContainer* EditorResourceTooltipPlugin::make_default_tooltip(const String& p_resource_path)
{
	VBoxContainer* vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", -4 * EDSCALE);
	{
		Label* label = memnew(Label(p_resource_path.get_file()));
		vb->add_child(label);
	}

	ResourceUID::ID id = EditorFileSystem::get_singleton()->get_file_uid(p_resource_path);
	if (id != ResourceUID::INVALID_ID) {
		Label* label = memnew(Label(ResourceUID::get_singleton()->id_to_text(id)));
		vb->add_child(label);
	}

	{
		Ref<FileAccess> f = FileAccess::open(p_resource_path, FileAccess::READ);
		if (f.is_valid()) {
			Label* label =
				memnew(Label(vformat(TTR("Size: %s"), String::humanize_size(f->get_length()))));
			vb->add_child(label);
		}
		else {
			Label* label = memnew(Label(TTR("Invalid file or broken link.")));
			label->add_theme_color_override(SceneStringName(font_color),
				EditorNode::get_singleton()->get_gui_base()->get_theme_color(
					SNAME("error_color"), EditorStringName(Editor)));
			vb->add_child(label);
			return vb;
		}
	}

	if (ResourceLoader::exists(p_resource_path)) {
		String type = ResourceLoader::get_resource_type(p_resource_path);
		Label* label = memnew(Label(vformat(TTR("Type: %s"), type)));
		vb->add_child(label);
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (da->is_link(p_resource_path)) {
		Label* link = memnew(Label(vformat(TTR("Link to: %s"), da->read_link(p_resource_path))));
		vb->add_child(link);
	}
	return vb;
}

// EditorTextureTooltipPlugin

bool EditorTextureTooltipPlugin::handles(const String& p_resource_type) const { return true; }

// EditorAudioStreamTooltipPlugin

bool EditorAudioStreamTooltipPlugin::handles(const String& p_resource_type) const { return true; }

bool EditorResourceTooltipPlugin::handles(const String& p_type) const { return false; }


