/**************************************************************************/
/*  editor_folding.cpp                                                    */
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

#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "editor/file_system/editor_paths.h"
#include "editor/inspector/editor_inspector.h"
#include "editor_folding.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/animation.h"

bool EditorFolding::has_folding_data(const String& p_path)
{
	String file = p_path.get_file() + "-folding-" + p_path.md5_text() + ".cfg";
	file = EditorPaths::get_singleton()->get_project_settings_dir().path_join(file);
	return FileAccess::exists(file);
}

void EditorFolding::unfold_scene(Node* p_scene)
{
	HashSet<Ref<Resource>> resources;
	_do_node_unfolds(p_scene, p_scene, resources);
}

Vector<String> EditorFolding::_get_animation_folds(const Animation* p_animation)
{
	Vector<String> folded_groups;
	folded_groups.resize(p_animation->editor_get_folded_groups().size());
	if (folded_groups.size()) {
		String* w = folded_groups.ptrw();
		int idx = 0;
		for (const StringName& group_name : p_animation->editor_get_folded_groups()) {
			w[idx++] = group_name;
		}
	}

	return folded_groups;
}

void EditorFolding::_set_animation_folds(Animation* p_animation, const Vector<String>& p_folds)
{
	p_animation->editor_clear_folded_groups();
	for (const String& group_name : p_folds) {
		p_animation->editor_add_folded_group(group_name);
	}
}


