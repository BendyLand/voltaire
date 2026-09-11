/**************************************************************************/
/*  gpu_particles_collision_sdf_editor_plugin.cpp                         */
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
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "gpu_particles_collision_sdf_editor_plugin.h"
#include "scene/3d/gpu_particles_collision_3d.h"
#include "scene/main/scene_tree.h"

void GPUParticlesCollisionSDF3DEditorPlugin::_bake()
{
	if (col_sdf) {
		if (col_sdf->get_texture().is_null() ||
			!col_sdf->get_texture()->get_path().is_resource_file()) {
			String path = get_tree()->get_edited_scene_root()->get_scene_file_path();
			if (path.is_empty()) {
				path = "res://" + col_sdf->get_name() + "_data.exr";
			}
			else {
				path = path.get_basename() + "." + col_sdf->get_name() + "_data.exr";
			}
			probe_file->set_current_path(path);
			probe_file->popup_file_dialog();
			return;
		}

		_sdf_save_path_and_bake(col_sdf->get_texture()->get_path());
	}
}

EditorProgress* GPUParticlesCollisionSDF3DEditorPlugin::tmp_progress = nullptr;

void GPUParticlesCollisionSDF3DEditorPlugin::bake_func_begin(int p_steps)
{
	ERR_FAIL_COND(tmp_progress != nullptr);

	tmp_progress = memnew(EditorProgress("bake_sdf", TTR("Bake SDF"), p_steps));
}

void GPUParticlesCollisionSDF3DEditorPlugin::bake_func_step(int p_step, const String& p_description)
{
	ERR_FAIL_NULL(tmp_progress);
	tmp_progress->step(p_description, p_step, false);
}

void GPUParticlesCollisionSDF3DEditorPlugin::bake_func_end()
{
	ERR_FAIL_NULL(tmp_progress);
	memdelete(tmp_progress);
	tmp_progress = nullptr;
}


