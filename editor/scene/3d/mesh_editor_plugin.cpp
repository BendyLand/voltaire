/**************************************************************************/
/*  mesh_editor_plugin.cpp                                                */
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
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "mesh_editor_plugin.h"
#include "scene/gui/button.h"
#include "scene/main/viewport.h"

void MeshEditor::gui_input(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && (mm->get_button_mask() & 1) != 0) {
		rot_x -= mm->get_relative().y * 0.01;
		rot_y -= mm->get_relative().x * 0.01;

		rot_x = CLAMP(rot_x, -Math::PI / 2, Math::PI / 2);
		_update_rotation();
	}
}

void MeshEditor::_update_theme_item_cache()
{
	SubViewportContainer::_update_theme_item_cache();

	theme_cache.light_1_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight1"));
	theme_cache.light_2_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight2"));
}

void MeshEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		light_1_switch->set_button_icon(theme_cache.light_1_icon);
		light_2_switch->set_button_icon(theme_cache.light_2_icon);
	} break;
	}
}

void MeshEditor::_update_rotation()
{
	Transform3D t;
	t.basis.rotate(Vector3(0, 1, 0), -rot_y);
	t.basis.rotate(Vector3(1, 0, 0), -rot_x);
	rotation->set_transform(t);
}

void MeshEditor::edit(Ref<Mesh> p_mesh)
{
	mesh = p_mesh;
	mesh_instance->set_mesh(mesh);

	rot_x = Math::deg_to_rad(-15.0);
	rot_y = Math::deg_to_rad(30.0);
	_update_rotation();

	AABB aabb = mesh->get_aabb();
	Vector3 ofs = aabb.get_center();
	float m = aabb.get_longest_axis_size();
	if (m != 0) {
		m = 1.0 / m;
		m *= 0.5;
		Transform3D xform;
		xform.basis.scale(Vector3(m, m, m));
		xform.origin = -xform.basis.xform(ofs); //-ofs*m;
		// xform.origin.z -= aabb.get_longest_axis_size() * 2;
		mesh_instance->set_transform(xform);
	}
}

void MeshEditor::_on_light_1_switch_pressed() { light1->set_visible(light_1_switch->is_pressed()); }

void MeshEditor::_on_light_2_switch_pressed() { light2->set_visible(light_2_switch->is_pressed()); }


