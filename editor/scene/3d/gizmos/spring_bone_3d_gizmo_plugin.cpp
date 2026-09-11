/**************************************************************************/
/*  spring_bone_3d_gizmo_plugin.cpp                                       */
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

#include "editor/settings/editor_settings.h"
#include "scene/3d/spring_bone_collision_3d.h"
#include "scene/3d/spring_bone_collision_capsule_3d.h"
#include "scene/3d/spring_bone_collision_plane_3d.h"
#include "scene/3d/spring_bone_collision_sphere_3d.h"
#include "scene/3d/spring_bone_simulator_3d.h"
#include "scene/resources/surface_tool.h"
#include "spring_bone_3d_gizmo_plugin.h"

// SpringBoneSimulator3D

SpringBoneSimulator3DGizmoPlugin::SelectionMaterials
	SpringBoneSimulator3DGizmoPlugin::selection_materials;

SpringBoneSimulator3DGizmoPlugin::SpringBoneSimulator3DGizmoPlugin()
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

SpringBoneSimulator3DGizmoPlugin::~SpringBoneSimulator3DGizmoPlugin()
{
	selection_materials.unselected_mat.unref();
	selection_materials.selected_mat.unref();
}

String SpringBoneSimulator3DGizmoPlugin::get_gizmo_name() const { return "SpringBoneSimulator3D"; }

void SpringBoneSimulator3DGizmoPlugin::draw_sphere(Ref<SurfaceTool>& p_surface_tool,
	const Basis& p_basis, const Vector3& p_center, float p_radius, const Color& p_color)
{
	static constexpr int STEP = 16;
	static constexpr float SPPI = Math::TAU / (float)STEP;

	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::UP * p_radius))
							   .rotated(p_basis.xform(Vector3::RIGHT), SPPI * ((i - 1) % STEP))));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::UP * p_radius))
							   .rotated(p_basis.xform(Vector3::RIGHT), SPPI * (i % STEP))));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::RIGHT * p_radius))
							   .rotated(p_basis.xform(Vector3::FORWARD), SPPI * ((i - 1) % STEP))));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::RIGHT * p_radius))
							   .rotated(p_basis.xform(Vector3::FORWARD), SPPI * (i % STEP))));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::FORWARD * p_radius))
							   .rotated(p_basis.xform(Vector3::UP), SPPI * ((i - 1) % STEP))));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			p_center + ((p_basis.xform(Vector3::FORWARD * p_radius))
							   .rotated(p_basis.xform(Vector3::UP), SPPI * (i % STEP))));
	}
}

void SpringBoneSimulator3DGizmoPlugin::draw_line(Ref<SurfaceTool>& p_surface_tool,
	const Vector3& p_begin_pos, const Vector3& p_end_pos, const Color& p_color)
{
	p_surface_tool->set_color(p_color);
	p_surface_tool->add_vertex(p_begin_pos);
	p_surface_tool->set_color(p_color);
	p_surface_tool->add_vertex(p_end_pos);
}

// SpringBoneCollision3D

SpringBoneCollision3DGizmoPlugin::SelectionMaterials
	SpringBoneCollision3DGizmoPlugin::selection_materials;

SpringBoneCollision3DGizmoPlugin::SpringBoneCollision3DGizmoPlugin()
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

SpringBoneCollision3DGizmoPlugin::~SpringBoneCollision3DGizmoPlugin()
{
	selection_materials.unselected_mat.unref();
	selection_materials.selected_mat.unref();
}

String SpringBoneCollision3DGizmoPlugin::get_gizmo_name() const { return "SpringBoneCollision3D"; }

void SpringBoneCollision3DGizmoPlugin::draw_sphere(
	Ref<SurfaceTool>& p_surface_tool, float p_radius, const Color& p_color)
{
	static constexpr int STEP = 16;
	static constexpr float SPPI = Math::TAU / (float)STEP;

	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::UP * p_radius).rotated(Vector3::RIGHT, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::UP * p_radius).rotated(Vector3::RIGHT, SPPI * (i % STEP)));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::RIGHT * p_radius).rotated(Vector3::FORWARD, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::RIGHT * p_radius).rotated(Vector3::FORWARD, SPPI * (i % STEP)));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * (i % STEP)));
	}
}

void SpringBoneCollision3DGizmoPlugin::draw_capsule(
	Ref<SurfaceTool>& p_surface_tool, float p_radius, float p_height, const Color& p_color)
{
	static constexpr int STEP = 16;
	static constexpr int HALF_STEP = 8;
	static constexpr float SPPI = (float)Math::TAU / STEP;
	static constexpr float HALF_PI = (float)Math::PI * 0.5f;

	Vector3 top = Vector3::UP * (p_height * 0.5 - p_radius);
	Vector3 bottom = -top;

	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(i - 1 < HALF_STEP ? top : bottom) +
			(Vector3::UP * p_radius).rotated(Vector3::RIGHT, -HALF_PI + SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(i - 1 < HALF_STEP ? top : bottom) +
			(Vector3::UP * p_radius).rotated(Vector3::RIGHT, -HALF_PI + SPPI * (i % STEP)));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(i - 1 < HALF_STEP ? top : bottom) +
			(Vector3::RIGHT * p_radius).rotated(Vector3::FORWARD, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			(i - 1 < HALF_STEP ? top : bottom) +
			(Vector3::RIGHT * p_radius).rotated(Vector3::FORWARD, SPPI * (i % STEP)));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			top + (Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			top + (Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * (i % STEP)));
	}
	for (int i = 1; i <= STEP; i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			bottom + (Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * ((i - 1) % STEP)));
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(
			bottom + (Vector3::FORWARD * p_radius).rotated(Vector3::UP, SPPI * (i % STEP)));
	}
	LocalVector<Vector3> directions;
	directions.resize(4);
	directions[0] = Vector3::RIGHT;
	directions[1] = Vector3::LEFT;
	directions[2] = Vector3::FORWARD;
	directions[3] = Vector3::BACK;
	for (int i = 0; i < 4; i++) {
		Vector3 dir = directions[i] * p_radius;
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(top + dir);
		p_surface_tool->add_vertex(bottom + dir);
	}
}

void SpringBoneCollision3DGizmoPlugin::draw_plane(
	Ref<SurfaceTool>& p_surface_tool, const Color& p_color)
{
	static constexpr float HALF_PI = (float)Math::PI * 0.5f;
	static constexpr float ARROW_LENGTH = 0.3f;
	static constexpr float ARROW_HALF_WIDTH = 0.05f;
	static constexpr float ARROW_TOP_HALF_WIDTH = 0.1f;
	static constexpr float ARROW_TOP = 0.5f;
	static constexpr float RECT_SIZE = 1.0f;
	static constexpr int RECT_STEP_COUNT = 9;
	static constexpr float RECT_HALF_SIZE = RECT_SIZE * 0.5f;
	static constexpr float RECT_STEP = RECT_SIZE / RECT_STEP_COUNT;

	p_surface_tool->set_color(p_color);

	// Draw arrow of the normal.
	LocalVector<Vector3> arrow;
	arrow.resize(7);
	arrow[0] = Vector3(0, ARROW_TOP, 0);
	arrow[1] = Vector3(-ARROW_TOP_HALF_WIDTH, ARROW_LENGTH, 0);
	arrow[2] = Vector3(-ARROW_HALF_WIDTH, ARROW_LENGTH, 0);
	arrow[3] = Vector3(-ARROW_HALF_WIDTH, 0, 0);
	arrow[4] = Vector3(ARROW_HALF_WIDTH, 0, 0);
	arrow[5] = Vector3(ARROW_HALF_WIDTH, ARROW_LENGTH, 0);
	arrow[6] = Vector3(ARROW_TOP_HALF_WIDTH, ARROW_LENGTH, 0);
	for (int i = 0; i < 2; i++) {
		Basis ma(Vector3::UP, HALF_PI * i);
		for (uint32_t j = 0; j < arrow.size(); j++) {
			Vector3 v1 = arrow[j];
			Vector3 v2 = arrow[(j + 1) % arrow.size()];
			p_surface_tool->add_vertex(ma.xform(v1));
			p_surface_tool->add_vertex(ma.xform(v2));
		}
	}

	// Draw dashed line of the rect.
	for (int i = 0; i < 4; i++) {
		Basis ma(Vector3::UP, HALF_PI * i);
		for (int j = 0; j < RECT_STEP_COUNT; j++) {
			if (j % 2 == 1) {
				continue;
			}
			Vector3 v1 = Vector3(RECT_HALF_SIZE, 0, RECT_HALF_SIZE - RECT_STEP * j);
			Vector3 v2 = Vector3(RECT_HALF_SIZE, 0, RECT_HALF_SIZE - RECT_STEP * (j + 1));
			p_surface_tool->add_vertex(ma.xform(v1));
			p_surface_tool->add_vertex(ma.xform(v2));
		}
	}
}


