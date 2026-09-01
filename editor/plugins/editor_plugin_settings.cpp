/**************************************************************************/
/*  editor_plugin_settings.cpp                                            */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "editor_plugin_settings.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

void EditorPluginSettings::_create_clicked()
{
	plugin_config_dialog->config("");
	plugin_config_dialog->popup_centered();
}

Vector<String> EditorPluginSettings::_get_plugins(const String& p_dir)
{
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	Error err = da->change_dir(p_dir);
	if (err != OK) {
		return Vector<String>();
	}

	Vector<String> plugins;
	da->list_dir_begin();
	for (String path = da->get_next(); !path.is_empty(); path = da->get_next()) {
		if (path[0] == '.' || !da->current_is_dir()) {
			continue;
		}

		const String full_path = p_dir.path_join(path);
		const String plugin_config = full_path.path_join("plugin.cfg");
		if (FileAccess::exists(plugin_config)) {
			plugins.push_back(plugin_config);
		}
		else {
			plugins.append_array(_get_plugins(full_path));
		}
	}

	da->list_dir_end();
	return plugins;
}


