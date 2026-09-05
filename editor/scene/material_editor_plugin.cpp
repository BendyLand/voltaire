/**************************************************************************/
/*  material_editor_plugin.cpp                                            */
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
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "material_editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/blit_material.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/particle_process_material.h"
#include "scene/resources/sky.h"
#include "servers/rendering/rendering_server.h"

// 3D.
#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"

void MaterialEditor::gui_input(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && ((mm->get_button_mask() & 1) != 0)) {
		rot.x -= mm->get_relative().y * 0.01;
		rot.y -= mm->get_relative().x * 0.01;
		if (quad_instance->is_visible()) {
			// Clamp rotation so the quad is always visible.
			const real_t limit = Math::deg_to_rad(80.0);
			rot = rot.clampf(-limit, limit);
		}
		else {
			rot.x = CLAMP(rot.x, -Math::PI / 2, Math::PI / 2);
		}
		_update_rotation();
		_store_rotation_metadata();
	}
}

void MaterialEditor::set_autohide_buttons(bool p_autohide)
{
	autohide_buttons = p_autohide;
	if (autohide_buttons) {
		layout_3d->hide();
	}
	else {
		layout_3d->show();
	}
}

void MaterialEditor::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.light_1_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight1"));
	theme_cache.light_2_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight2"));

	theme_cache.sphere_icon = get_editor_theme_icon(SNAME("MaterialPreviewSphere"));
	theme_cache.box_icon = get_editor_theme_icon(SNAME("MaterialPreviewCube"));
	theme_cache.quad_icon = get_editor_theme_icon(SNAME("MaterialPreviewQuad"));

	theme_cache.checkerboard = get_editor_theme_icon(SNAME("Checkerboard"));
}

void MaterialEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		light_1_switch->set_button_icon(theme_cache.light_1_icon);
		light_2_switch->set_button_icon(theme_cache.light_2_icon);

		sphere_switch->set_button_icon(theme_cache.sphere_icon);
		box_switch->set_button_icon(theme_cache.box_icon);
		quad_switch->set_button_icon(theme_cache.quad_icon);

		error_label->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
	} break;

	case NOTIFICATION_DRAW: {
		if (!is_unsupported_shader_mode) {
			Size2 size = get_size();
			draw_rect(Rect2(Point2(), size),
				Color(0, 0, 0,
					1)); // Since checkerboard texture is transluscent, draw opaque black behind it.
			draw_texture_rect(theme_cache.checkerboard.ptr(), Rect2(Point2(), size), true);
		}
	} break;

	case NOTIFICATION_MOUSE_ENTER: {
		if (autohide_buttons) {
			Shader::Mode mode =
				material.is_valid() ? material->get_shader_mode() : Shader::MODE_MAX;
			if (mode == Shader::MODE_SPATIAL) {
				layout_3d->show();
			}
		}
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		if (autohide_buttons) {
			layout_3d->hide();
		}
	} break;
	}
}

void MaterialEditor::_set_rotation(real_t p_x_degrees, real_t p_y_degrees)
{
	rot.x = Math::deg_to_rad(p_x_degrees);
	rot.y = Math::deg_to_rad(p_y_degrees);
	_update_rotation();
}

void MaterialEditor::_update_rotation()
{
	Transform3D t;
	t.basis.rotate(Vector3(0, 1, 0), -rot.y);
	t.basis.rotate(Vector3(1, 0, 0), -rot.x);
	rotation->set_transform(t);
}

void MaterialEditor::edit(Ref<Material> p_material, const Ref<Environment>& p_env)
{
	material = p_material;
	camera->set_environment(p_env);

	is_unsupported_shader_mode = false;
	if (material.is_valid()) {
		Shader::Mode mode = p_material->get_shader_mode();
		switch (mode) {
		case Shader::MODE_CANVAS_ITEM:
			layout_error->hide();
			layout_3d->hide();
			layout_2d->show();
			rect_instance->set_material(material);
			vc->hide();
			break;
		case Shader::MODE_SPATIAL:
			layout_error->hide();
			layout_2d->hide();
			if (!autohide_buttons) {
				layout_3d->show();
			}
			sphere_instance->set_material_override(material);
			box_instance->set_material_override(material);
			quad_instance->set_material_override(material);
			vc->show();
			break;
		default:
			layout_error->show();
			layout_2d->hide();
			layout_3d->hide();
			is_unsupported_shader_mode = true;
			vc->hide();
			break;
		}
	}
	else {
		hide();
	}
}

void MaterialEditor::_on_light_1_switch_pressed()
{
	light1->set_visible(light_1_switch->is_pressed());
}

void MaterialEditor::_on_light_2_switch_pressed()
{
	light2->set_visible(light_2_switch->is_pressed());
}

///////////////////////

MaterialEditorPlugin::MaterialEditorPlugin()
{
	Ref<EditorInspectorPluginMaterial> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}

String ParticleProcessMaterialConversionPlugin::converts_to() const { return "ShaderMaterial"; }

bool ParticleProcessMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	Ref<ParticleProcessMaterial> mat = p_resource;
	return mat.is_valid();
}

Ref<Resource> ParticleProcessMaterialConversionPlugin::convert(
	const Ref<Resource>& p_resource) const
{
	return MaterialEditor::make_shader_material(p_resource);
}

String CanvasItemMaterialConversionPlugin::converts_to() const { return "ShaderMaterial"; }

bool CanvasItemMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	Ref<CanvasItemMaterial> mat = p_resource;
	return mat.is_valid();
}

String BlitMaterialConversionPlugin::converts_to() const { return "ShaderMaterial"; }

bool BlitMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	Ref<BlitMaterial> mat = p_resource;
	return mat.is_valid();
}


