/**************************************************************************/
/*  particles_3d_editor_plugin.cpp                                        */
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

#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/scene_tree_editor.h"
#include "particles_3d_editor_plugin.h"
#include "scene/3d/cpu_particles_3d.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/particle_process_material.h"

void Particles3DEditorPlugin::_menu_callback(int p_idx)
{
	switch (p_idx) {
	case MENU_OPTION_GENERATE_AABB: {
		if (need_show_lifetime_dialog(generate_seconds)) {
			generate_aabb->popup_centered();
		}
		else {
			_generate_aabb();
		}
	} break;

	case MENU_OPTION_CREATE_EMISSION_VOLUME_FROM_NODE: {
		if (_can_generate_points()) {
			emission_tree_dialog->popup_scenetree_dialog();
		}
	} break;

	default: {
		ParticlesEditorPlugin::_menu_callback(p_idx);
	}
	}
}

void Particles3DEditorPlugin::_add_menu_options(PopupMenu* p_menu)
{
	p_menu->add_item(TTR("Generate AABB"), MENU_OPTION_GENERATE_AABB);
	p_menu->add_item(
		TTR("Create Emission Points From Node"), MENU_OPTION_CREATE_EMISSION_VOLUME_FROM_NODE);
}

GPUParticles3DEditorPlugin::GPUParticles3DEditorPlugin()
{
	handled_type = TTRC("GPUParticles3D");
	conversion_option_name = TTR("Convert to CPUParticles3D");
}

CPUParticles3DEditorPlugin::CPUParticles3DEditorPlugin()
{
	handled_type = TTRC("CPUParticles3D");
	conversion_option_name = TTR("Convert to GPUParticles3D");
}


