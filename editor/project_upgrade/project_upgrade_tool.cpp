/**************************************************************************/
/*  project_upgrade_tool.cpp                                              */
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

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/templates/mem_unique_ptr.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "project_upgrade_tool.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/dialogs.h"

void ProjectUpgradeTool::_add_files(EditorFileSystemDirectory* p_dir,
	Vector<String>& r_reimport_paths, Vector<String>& r_resave_scenes,
	Vector<String>& r_resave_resources)
{
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		const String path = p_dir->get_file_path(i);
		const String ext = path.get_extension();
		if (ext == "tscn" || ext == "scn") {
			r_resave_scenes.append(path);
		}
		else if (ext == "tres" || ext == "res") {
			r_resave_resources.append(path);
		}
		else if (da->file_exists(path + ".import")) {
			r_reimport_paths.append(path);
		}
	}

	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_add_files(p_dir->get_subdir(i), r_reimport_paths, r_resave_scenes, r_resave_resources);
	}
}


