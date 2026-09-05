/**************************************************************************/
/*  chain_ik_3d_gizmo_plugin.cpp                                          */
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

#include "chain_ik_3d_gizmo_plugin.h"
#include "editor/settings/editor_settings.h"
#include "scene/3d/iterate_ik_3d.h"
#include "scene/resources/surface_tool.h"

ChainIK3DGizmoPlugin::SelectionMaterials ChainIK3DGizmoPlugin::selection_materials;

ChainIK3DGizmoPlugin::ChainIK3DGizmoPlugin()
{
	selection_materials.unselected_mat.instantiate();
	selection_materials.unselected_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_materials.unselected_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	selection_materials.unselected_mat->set_flag(
		StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	selection_materials.unselected_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	selection_materials.unselected_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	selection_materials.selected_mat.instantiate();
	Ref<Shader> sh;
	sh.instantiate();
	sh->set_code(R"(
// Skeleton 3D gizmo bones shader.

shader_type spatial;
render_mode unshaded, shadows_disabled;

void vertex() {
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb,vec3(0.04045)));
	}
	VERTEX = VERTEX;
	POSITION = PROJECTION_MATRIX * VIEW_MATRIX * MODEL_MATRIX * vec4(VERTEX.xyz, 1.0);
	POSITION.z = mix(POSITION.z, POSITION.w, 0.998);
}

void fragment() {
	ALBEDO = COLOR.rgb;
	ALPHA = COLOR.a;
}
)");
	selection_materials.selected_mat->set_shader(sh);
}

ChainIK3DGizmoPlugin::~ChainIK3DGizmoPlugin()
{
	selection_materials.unselected_mat.unref();
	selection_materials.selected_mat.unref();
}

String ChainIK3DGizmoPlugin::get_gizmo_name() const { return "ChainIK3D"; }

void ChainIK3DGizmoPlugin::draw_line(Ref<SurfaceTool>& p_surface_tool, const Vector3& p_begin_pos,
	const Vector3& p_end_pos, const Color& p_color)
{
	p_surface_tool->set_color(p_color);
	p_surface_tool->add_vertex(p_begin_pos);
	p_surface_tool->set_color(p_color);
	p_surface_tool->add_vertex(p_end_pos);
}


