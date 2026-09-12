/**************************************************************************/
/*  animation_tree_editor_plugin.cpp                                      */
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

#include "animation_tree_editor_plugin.h"
#include "core/string/string_buffer.h"
#include "editor/animation/animation_blend_space_1d_editor.h"
#include "editor/animation/animation_blend_space_2d_editor.h"
#include "editor/animation/animation_blend_tree_editor_plugin.h"
#include "editor/animation/animation_state_machine_editor.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/gui/button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"

void AnimationTreeEditor::_path_button_pressed(int p_path)
{
	edited_path.clear();
	for (int i = 0; i <= p_path; i++) {
		edited_path.push_back(button_path[i]);
	}
}

void AnimationTreeEditor::_animation_list_changed()
{
	AnimationNodeBlendTreeEditor* bte = AnimationNodeBlendTreeEditor::get_singleton();
	if (bte) {
		bte->update_graph();
	}
}

void AnimationTreeEditor::_update_path()
{
	while (path_hb->get_child_count() > 1) {
		memdelete(path_hb->get_child(1));
	}

	Ref<ButtonGroup> group;
	group.instantiate();

	Button* b = memnew(Button);
	b->set_text(TTR("Root"));
	b->set_toggle_mode(true);
	b->set_button_group(group);
	b->set_pressed(true);
	b->set_focus_mode(FOCUS_ACCESSIBILITY);
	path_hb->add_child(b);
	for (int i = 0; i < button_path.size(); i++) {
		// bread crumbs.
		TextureRect* texture_rect = memnew(TextureRect);
		texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		texture_rect->set_custom_minimum_size(Size2(16, 16) * EDSCALE);
		texture_rect->set_texture(get_editor_theme_icon(SNAME("GuiTreeArrowRight")));
		path_hb->add_child(texture_rect);

		b = memnew(Button);
		b->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		b->set_text(button_path[i]);
		b->set_toggle_mode(true);
		b->set_button_group(group);
		path_hb->add_child(b);
		b->set_pressed(true);
		b->set_focus_mode(FOCUS_ACCESSIBILITY);
	}
}



Vector<String> AnimationTreeEditor::get_edited_path() const { return button_path; }

AnimationTreeEditor* AnimationTreeEditor::singleton = nullptr;

void AnimationTreeEditor::remove_plugin(AnimationTreeNodeEditorPlugin* p_editor)
{
	ERR_FAIL_COND(p_editor->get_parent() != editor_base);
	editor_base->remove_child(p_editor);
	editors.erase(p_editor);
}

String AnimationTreeEditor::get_base_path()
{
	String path = Animation::PARAMETERS_BASE_PATH;
	for (int i = 0; i < edited_path.size(); i++) {
		path += edited_path[i] + "/";
	}
	return path;
}

bool AnimationTreeEditor::can_edit(const Ref<AnimationNode>& p_node) const
{
	for (int i = 0; i < editors.size(); i++) {
		if (editors[i]->can_edit(p_node)) {
			return true;
		}
	}
	return false;
}

LocalVector<StringName> AnimationTreeEditor::get_animation_list()
{
	// This can be called off the main thread due to resource preview generation. Quit early in that
	// case.
	if (!singleton->tree || !Thread::is_main_thread() || !singleton->is_visible()) {
		// When tree is empty, singleton not in the main thread.
		return LocalVector<StringName>();
	}

	AnimationTree* tree = singleton->tree;
	if (!tree) {
		return LocalVector<StringName>();
	}

	return tree->get_sorted_animation_list();
}

AnimationTreeEditorPlugin::AnimationTreeEditorPlugin()
{
	anim_tree_editor = memnew(AnimationTreeEditor);
	anim_tree_editor->set_custom_minimum_size(Size2(0, 300) * EDSCALE);
	EditorDockManager::get_singleton()->add_dock(anim_tree_editor);
	anim_tree_editor->close();
}


