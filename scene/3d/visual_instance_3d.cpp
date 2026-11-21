/**************************************************************************/
/*  visual_instance_3d.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "visual_instance_3d.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/math/triangle_mesh.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

void RenderInstance3D::update_visibility(Node3D* p_owner)
{
	if (!p_owner || !p_owner->is_inside_tree()) {
		return;
	}

	bool already_visible = p_owner->_is_vi_visible();
	bool visible = p_owner->is_visible_in_tree();
	p_owner->_set_vi_visible(visible);

	if (visible && !already_visible) {
		if (!p_owner->_is_using_identity_transform()) {
			Transform3D gt = p_owner->get_global_transform();
			Renderer::instance_set_transform(instance, gt);
		}
	}

	Renderer::instance_set_visible(instance, visible);
}

void RenderInstance3D::set_instance_use_identity_transform(Node3D* p_owner, bool p_enable)
{
	if (!p_owner) {
		return;
	}
	p_owner->_set_use_identity_transform(p_enable);

	if (p_owner->is_inside_tree()) {
		if (p_enable) {
			Renderer::instance_set_transform(instance, Transform3D());
		}
		else {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	}
}

void RenderInstance3D::fti_update_servers_xform(Node3D* p_owner)
{
	if (p_owner && !p_owner->_is_using_identity_transform()) {
		Renderer::instance_set_transform(
			instance, p_owner->_get_cached_global_transform_interpolated());
	}
}

void RenderInstance3D::notification(Node3D* p_owner, int p_what)
{
	if (!p_owner) {
		return;
	}

	switch (p_what) {
	case Node3D::NOTIFICATION_ENTER_WORLD: {
		ERR_FAIL_COND(p_owner->get_world_3d().is_null());
		if (!instance.is_valid() && Renderer::data) {
			instance = Renderer::instance_create();
			if (base.is_valid()) {
				Renderer::instance_set_base(instance, base);
			}
			Renderer::instance_set_layer_mask(instance, layers);
		}
		Renderer::instance_set_scenario(instance, p_owner->get_world_3d()->get_scenario());
		update_visibility(p_owner);
	} break;

	case Node3D::NOTIFICATION_TRANSFORM_CHANGED: {
		if (p_owner->_is_vi_visible() &&
			!(p_owner->is_inside_tree() && SceneTree::is_physics_interpolation_enabled()) &&
			!p_owner->_is_using_identity_transform()) {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	} break;

	case Node::NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		if (p_owner->_is_vi_visible() && p_owner->is_inside_tree()) {
			Renderer::instance_teleport(instance);
		}
	} break;

	case Node3D::NOTIFICATION_EXIT_WORLD: {
		Renderer::instance_set_scenario(instance, RID());
		Renderer::instance_attach_skeleton(instance, RID());
		p_owner->_set_vi_visible(false);
	} break;

	case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
		update_visibility(p_owner);
	} break;
	}
}

RID RenderInstance3D::get_instance() const { return instance; }

void RenderInstance3D::set_layer_mask(uint32_t p_mask)
{
	layers = p_mask;
	if (instance.is_valid()) {
		Renderer::instance_set_layer_mask(instance, p_mask);
	}
}

uint32_t RenderInstance3D::get_layer_mask() const { return layers; }

void RenderInstance3D::set_layer_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_layer_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_layer_mask(mask);
}

bool RenderInstance3D::get_layer_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return layers & (1 << (p_layer_number - 1));
}

void RenderInstance3D::set_sorting_offset(float p_offset)
{
	sorting_offset = p_offset;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

float RenderInstance3D::get_sorting_offset() const { return sorting_offset; }

void RenderInstance3D::set_sorting_use_aabb_center(bool p_enabled)
{
	sorting_use_aabb_center = p_enabled;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

bool RenderInstance3D::is_sorting_use_aabb_center() const { return sorting_use_aabb_center; }

void RenderInstance3D::set_base(const RID& p_base)
{
	base = p_base;
	if (!instance.is_valid() && base.is_valid() && Renderer::data) {
		instance = Renderer::instance_create();
		Renderer::instance_set_layer_mask(instance, layers);
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_base(instance, p_base);
	}
}

RID RenderInstance3D::get_base() const { return base; }

void RenderInstance3D::set_material_override(const Ref<Material>& p_material)
{
	material_override = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_override(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_override() const { return material_override; }

void RenderInstance3D::set_material_overlay(const Ref<Material>& p_material)
{
	material_overlay = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_overlay(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_overlay() const { return material_overlay; }

float RenderInstance3D::get_transparency() const { return transparency; }

float RenderInstance3D::get_visibility_range_begin() const { return visibility_range_begin; }

float RenderInstance3D::get_visibility_range_end() const { return visibility_range_end; }

float RenderInstance3D::get_visibility_range_begin_margin() const
{
	return visibility_range_begin_margin;
}

float RenderInstance3D::get_visibility_range_end_margin() const
{
	return visibility_range_end_margin;
}

RenderInstance3D::VisibilityRangeFadeMode RenderInstance3D::get_visibility_range_fade_mode() const
{
	return visibility_range_fade_mode;
}

const String* RenderInstance3D::_instance_uniform_get_remap(const String& p_name) const
{
	String* r = instance_shader_parameter_property_remap.getptr(p_name);
	if (!r) {
		String s = p_name;
#ifndef DISABLE_DEPRECATED
		if (s.begins_with("shader_uniforms/")) {
			s = s.replace("shader_uniforms/", "instance_shader_parameters/");
		}
#endif // DISABLE_DEPRECATED
		if (s.begins_with("instance_shader_parameters/")) {
			String pname = String(s);
			String name = s.replace("instance_shader_parameters/", "");
			instance_shader_parameter_property_remap[pname] = name;
			return instance_shader_parameter_property_remap.getptr(pname);
		}
		return nullptr;
	}

	return r;
}

void RenderInstance3D::set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting)
{
	shadow_casting_setting = p_shadow_casting_setting;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_cast_shadows_setting(
			instance, (RSE::ShadowCastingSetting)p_shadow_casting_setting);
	}
}

RenderInstance3D::ShadowCastingSetting RenderInstance3D::get_cast_shadows_setting() const
{
	return shadow_casting_setting;
}

void RenderInstance3D::set_extra_cull_margin(float p_margin)
{
	ERR_FAIL_COND(p_margin < 0);
	extra_cull_margin = p_margin;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_extra_visibility_margin(instance, extra_cull_margin);
	}
}

float RenderInstance3D::get_extra_cull_margin() const { return extra_cull_margin; }

void RenderInstance3D::set_lod_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.0);
	lod_bias = p_bias;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_lod_bias(instance, lod_bias);
	}
}

float RenderInstance3D::get_lod_bias() const { return lod_bias; }

AABB RenderInstance3D::get_custom_aabb() const { return custom_aabb; }

void RenderInstance3D::set_lightmap_texel_scale(float p_scale) { lightmap_texel_scale = p_scale; }

float RenderInstance3D::get_lightmap_texel_scale() const { return lightmap_texel_scale; }

void RenderInstance3D::set_gi_mode(GIMode p_mode)
{
	gi_mode = p_mode;
	if (!instance.is_valid() || !Renderer::data) {
		return;
	}

	switch (p_mode) {
	case GI_MODE_DISABLED: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_STATIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, true);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_DYNAMIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, true);
	} break;
	}
}

RenderInstance3D::GIMode RenderInstance3D::get_gi_mode() const { return gi_mode; }

void RenderInstance3D::set_ignore_occlusion_culling(bool p_enabled)
{
	ignore_occlusion_culling = p_enabled;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_flag(
			instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, ignore_occlusion_culling);
	}
}

bool RenderInstance3D::is_ignoring_occlusion_culling() const { return ignore_occlusion_culling; }

void RenderInstance3D::append_geometry_warnings(PackedStringArray& r_warnings) const
{
	if (!Math::is_zero_approx(visibility_range_end) &&
		visibility_range_end <= visibility_range_begin) {
		r_warnings.push_back(
			RTR("The visibility range's End distance is set to a non-zero value, but is lower than "
				"the Begin distance.\nTo resolve this, set the End distance to 0 or to a value "
				"greater than the Begin distance."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_begin) &&
		Math::is_zero_approx(visibility_range_begin_margin)) {
		r_warnings.push_back(RTR("Configured to fade in smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range Begin Margin above 0."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_end) &&
		Math::is_zero_approx(visibility_range_end_margin)) {
		r_warnings.push_back(RTR("Configured to fade out smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range End Margin above 0."));
	}

	if (!Math::is_zero_approx(transparency) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(
			RTR("Instance transparency is only available when using the Forward+ renderer."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(RTR("Visibility range transparency fade is only available when using "
								 "the Forward+ renderer."));
	}
}

RenderInstance3D::~RenderInstance3D()
{
	if (material_overlay.is_valid()) {
		set_material_overlay(Ref<Material>());
	}
	if (material_override.is_valid()) {
		set_material_override(Ref<Material>());
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::free_rid(instance);
	}
}

// ==========================================================================
// Merged from scene/3d/mesh_instance_3d.cpp
// ==========================================================================

#include "mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/resources/3d/shape_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

#include <cfloat> // FLT_EPSILON

#ifndef NAVIGATION_3D_DISABLED
RID MeshInstance3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

Ref<Mesh> MeshInstance3D::get_mesh() const { return mesh; }

int MeshInstance3D::get_blend_shape_count() const
{
	if (mesh.is_null()) {
		return 0;
	}
	return mesh->get_blend_shape_count();
}

int MeshInstance3D::find_blend_shape_by_name(const String& p_name)
{
	if (mesh.is_null()) {
		return -1;
	}
	for (int i = 0; i < mesh->get_blend_shape_count(); i++) {
		if (mesh->get_blend_shape_name(i) == p_name) {
			return i;
		}
	}
	return -1;
}

float MeshInstance3D::get_blend_shape_value(int p_blend_shape) const
{
	ERR_FAIL_COND_V(mesh.is_null(), 0.0);
	ERR_FAIL_INDEX_V(p_blend_shape, (int)blend_shape_tracks.size(), 0);
	return blend_shape_tracks[p_blend_shape];
}

void MeshInstance3D::set_blend_shape_value(int p_blend_shape, float p_value)
{
	ERR_FAIL_COND(mesh.is_null());
	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_tracks.size());
	blend_shape_tracks[p_blend_shape] = p_value;
	Renderer::instance_set_blend_shape_weight(
		get_instance(), p_blend_shape, p_value);
}

void MeshInstance3D::set_skin(const Ref<Skin>& p_skin)
{
	skin_internal = p_skin;
	skin = p_skin;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

Ref<Skin> MeshInstance3D::get_skin() const { return skin; }

Ref<SkinReference> MeshInstance3D::get_skin_reference() const { return skin_ref; }

void MeshInstance3D::set_skeleton_path(const NodePath& p_skeleton)
{
	skeleton_path = p_skeleton;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

NodePath MeshInstance3D::get_skeleton_path() { return skeleton_path; }

AABB MeshInstance3D::get_aabb() const
{
	if (mesh.is_valid()) {
		return mesh->get_aabb();
	}

	return AABB();
}

#ifndef PHYSICS_3D_DISABLED
Node* MeshInstance3D::create_trimesh_collision_node()
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<Shape3D> shape = mesh->create_trimesh_shape();
	if (shape.is_null()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}

Node* MeshInstance3D::create_convex_collision_node(bool p_clean, bool p_simplify)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<Shape3D> shape = mesh->create_convex_shape(p_clean, p_simplify);
	if (shape.is_null()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}

Node* MeshInstance3D::create_multiple_convex_collisions_node(
	const Ref<MeshConvexDecompositionSettings>& p_settings)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<MeshConvexDecompositionSettings> settings;
	if (p_settings.is_valid()) {
		settings = p_settings;
	}
	else {
		settings.instantiate();
	}

	Vector<Ref<Shape3D>> shapes = mesh->convex_decompose(settings);
	if (!shapes.size()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	for (int i = 0; i < shapes.size(); i++) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(shapes[i]);
		static_body->add_child(cshape, true);
	}
	return static_body;
}

#endif // PHYSICS_3D_DISABLED

int MeshInstance3D::get_surface_override_material_count() const
{
	return surface_override_materials.size();
}

void MeshInstance3D::set_surface_override_material(int p_surface, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_surface, surface_override_materials.size());

	surface_override_materials.write[p_surface] = p_material;

	if (surface_override_materials[p_surface].is_valid()) {
		Renderer::instance_set_surface_override_material(
			get_instance(), p_surface, surface_override_materials[p_surface]->get_rid());
	}
	else {
		Renderer::instance_set_surface_override_material(
			get_instance(), p_surface, RID());
	}
}

Ref<Material> MeshInstance3D::get_surface_override_material(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surface_override_materials.size(), Ref<Material>());

	return surface_override_materials[p_surface];
}

Ref<Material> MeshInstance3D::get_active_material(int p_surface) const
{
	Ref<Material> mat_override = get_material_override();
	if (mat_override.is_valid()) {
		return mat_override;
	}

	Ref<Mesh> m = get_mesh();
	if (m.is_null() || m->get_surface_count() == 0) {
		return Ref<Material>();
	}

	Ref<Material> surface_material = get_surface_override_material(p_surface);
	if (surface_material.is_valid()) {
		return surface_material;
	}

	return m->surface_get_material(p_surface);
}

void MeshInstance3D::create_debug_tangents()
{
	MeshInstance3D* mi = create_debug_tangents_node();
	if (!mi) {
		return;
	}

	add_child(mi, true);
	if (is_inside_tree() && this == SceneTree::get_edited_scene_root()) {
		mi->set_owner(this);
	}
	else {
		mi->set_owner(get_owner());
	}
}

bool MeshInstance3D::_property_can_revert(const String& p_name) const
{
	HashMap<String, int>::ConstIterator E = blend_shape_properties.find(p_name);
	if (E) {
		return true;
	}
	return false;
}

Ref<TriangleMesh> MeshInstance3D::generate_triangle_mesh() const
{
	if (mesh.is_valid()) {
		return mesh->generate_triangle_mesh();
	}
	return Ref<TriangleMesh>();
}

PackedStringArray MeshInstance3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);
	if (mesh.is_null()) {
		warnings.push_back(RTR("MeshInstance3D requires a Mesh to render anything. Please add a "
							   "mesh resource for it!"));
	}
	return warnings;
}

void MeshInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void MeshInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void MeshInstance3D::_resolve_skeleton_path() {}

void MeshInstance3D::set_mesh(Ref<Mesh> const& p_mesh)
{
	mesh = p_mesh;
	render_instance.set_base(mesh.is_valid() ? mesh->get_rid() : RID());
}

MeshInstance3D::MeshInstance3D() {}

MeshInstance3D* MeshInstance3D::create_debug_tangents_node()
{
	return nullptr;
}

// ==========================================================================
// Merged from scene/3d/multimesh_instance_3d.cpp
// ==========================================================================

#include "multimesh_instance_3d.h"

#ifndef NAVIGATION_3D_DISABLED
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"

RID MultiMeshInstance3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

void MultiMeshInstance3D::_refresh_interpolated()
{
	if (is_inside_tree() && multimesh.is_valid()) {
		bool interpolated = is_physics_interpolated_and_enabled();
		multimesh->set_physics_interpolated(interpolated);
	}
}

void MultiMeshInstance3D::_physics_interpolated_changed()
{
	Node3D::_physics_interpolated_changed();
	_refresh_interpolated();
}

void MultiMeshInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void MultiMeshInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_refresh_interpolated();
	}
}

void MultiMeshInstance3D::set_multimesh(const Ref<MultiMesh>& p_multimesh)
{
	multimesh = p_multimesh;
	if (multimesh.is_valid()) {
		set_base(multimesh->get_rid());
		_refresh_interpolated();
	}
	else {
		set_base(RID());
	}
}

Ref<MultiMesh> MultiMeshInstance3D::get_multimesh() const { return multimesh; }

AABB MultiMeshInstance3D::get_aabb() const
{
	if (multimesh.is_null()) {
		return AABB();
	}
	else {
		return multimesh->get_aabb();
	}
}

// ==========================================================================
// Merged from scene/3d/importer_mesh_instance_3d.cpp
// ==========================================================================

#include "importer_mesh_instance_3d.h"
#include "scene/resources/3d/importer_mesh.h"

void ImporterMeshInstance3D::set_mesh(const Ref<ImporterMesh>& p_mesh) { mesh = p_mesh; }

Ref<ImporterMesh> ImporterMeshInstance3D::get_mesh() const { return mesh; }

void ImporterMeshInstance3D::set_skin(const Ref<Skin>& p_skin) { skin = p_skin; }

Ref<Skin> ImporterMeshInstance3D::get_skin() const { return skin; }

void ImporterMeshInstance3D::set_surface_material(int p_idx, const Ref<Material>& p_material)
{
	ERR_FAIL_COND(p_idx < 0);
	if (p_idx >= surface_materials.size()) {
		surface_materials.resize(p_idx + 1);
	}

	surface_materials.write[p_idx] = p_material;
}

Ref<Material> ImporterMeshInstance3D::get_surface_material(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < 0, Ref<Material>());
	if (p_idx >= surface_materials.size()) {
		return Ref<Material>();
	}
	return surface_materials[p_idx];
}

void ImporterMeshInstance3D::set_skeleton_path(const NodePath& p_path) { skeleton_path = p_path; }

NodePath ImporterMeshInstance3D::get_skeleton_path() const { return skeleton_path; }

uint32_t ImporterMeshInstance3D::get_layer_mask() const { return layer_mask; }

void ImporterMeshInstance3D::set_layer_mask(const uint32_t p_layer_mask)
{
	layer_mask = p_layer_mask;
}

void ImporterMeshInstance3D::set_cast_shadows_setting(
	RenderInstance3D::ShadowCastingSetting p_shadow_casting_setting)
{
	shadow_casting_setting = p_shadow_casting_setting;
}

RenderInstance3D::ShadowCastingSetting ImporterMeshInstance3D::get_cast_shadows_setting() const
{
	return shadow_casting_setting;
}

float ImporterMeshInstance3D::get_visibility_range_begin() const { return visibility_range_begin; }

float ImporterMeshInstance3D::get_visibility_range_end() const { return visibility_range_end; }

float ImporterMeshInstance3D::get_visibility_range_begin_margin() const
{
	return visibility_range_begin_margin;
}

float ImporterMeshInstance3D::get_visibility_range_end_margin() const
{
	return visibility_range_end_margin;
}

RenderInstance3D::VisibilityRangeFadeMode
ImporterMeshInstance3D::get_visibility_range_fade_mode() const
{
	return visibility_range_fade_mode;
}

// ==========================================================================
// Merged from scene/3d/sprite_3d.cpp
// ==========================================================================

#include "sprite_3d.h"

#include "core/config/engine.h"
#include "core/math/triangle_mesh.h"
#include "scene/resources/atlas_texture.h"
#include "servers/rendering/renderer.h"

Color Sprite3D::_get_color_accum()
{
	if (!color_dirty) {
		return color_accum;
	}

	if (parent_sprite) {
		color_accum = parent_sprite->_get_color_accum();
	}
	else {
		color_accum = Color(1, 1, 1, 1);
	}

	color_accum.r *= modulate.r;
	color_accum.g *= modulate.g;
	color_accum.b *= modulate.b;
	color_accum.a *= modulate.a;
	color_dirty = false;
	return color_accum;
}

void Sprite3D::_queue_redraw()
{
	redraw_needed = true;
}

void Sprite3D::_propagate_color_changed()
{
	if (color_dirty) {
		return;
	}

	color_dirty = true;
	_queue_redraw();

	for (Sprite3D*& E : children) {
		E->_propagate_color_changed();
	}
}

void Sprite3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Sprite3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_im_update();
	} break;

	case NOTIFICATION_UNPARENTED: {
		if (parent_sprite) {
			parent_sprite->children.erase(pI);
			pI = nullptr;
			parent_sprite = nullptr;

			_propagate_color_changed();
		}
	} break;
	}
}

void Sprite3D::set_centered(bool p_center)
{
	if (centered == p_center) {
		return;
	}
	centered = p_center;
	_queue_redraw();
}

bool Sprite3D::is_centered() const { return centered; }

void Sprite3D::set_offset(const Point2& p_offset)
{
	if (offset == p_offset) {
		return;
	}
	offset = p_offset;
	_queue_redraw();
}

Point2 Sprite3D::get_offset() const { return offset; }

void Sprite3D::set_flip_h(bool p_flip)
{
	if (hflip == p_flip) {
		return;
	}
	hflip = p_flip;
	_queue_redraw();
}

bool Sprite3D::is_flipped_h() const { return hflip; }

void Sprite3D::set_flip_v(bool p_flip)
{
	if (vflip == p_flip) {
		return;
	}
	vflip = p_flip;
	_queue_redraw();
}

bool Sprite3D::is_flipped_v() const { return vflip; }

void Sprite3D::set_modulate(const Color& p_color)
{
	if (modulate == p_color) {
		return;
	}
	modulate = p_color;
	_propagate_color_changed();
	_queue_redraw();
}

Color Sprite3D::get_modulate() const { return modulate; }

void Sprite3D::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (render_priority == p_priority) {
		return;
	}
	render_priority = p_priority;
	_queue_redraw();
}

int Sprite3D::get_render_priority() const { return render_priority; }

void Sprite3D::set_pixel_size(real_t p_amount)
{
	if (pixel_size == p_amount) {
		return;
	}
	pixel_size = p_amount;
	_queue_redraw();
}

real_t Sprite3D::get_pixel_size() const { return pixel_size; }

void Sprite3D::set_axis(Vector3::Axis p_axis)
{
	ERR_FAIL_INDEX(p_axis, 3);
	if (axis == p_axis) {
		return;
	}
	axis = p_axis;
	_queue_redraw();
}

Vector3::Axis Sprite3D::get_axis() const { return axis; }

void Sprite3D::_im_update()
{
	if (!redraw_needed) {
		return;
	}
	_draw();
	redraw_needed = false;
	pending_update = false;
}

AABB Sprite3D::get_aabb() const { return aabb; }

Ref<TriangleMesh> Sprite3D::generate_triangle_mesh() const
{
	if (triangle_mesh.is_valid()) {
		return triangle_mesh;
	}

	Vector<Vector3> faces;
	faces.resize(6);
	Vector3* facesw = faces.ptrw();

	Rect2 final_rect = get_item_rect();
	if (final_rect.size.x == 0 || final_rect.size.y == 0) {
		return Ref<TriangleMesh>();
	}

	real_t px_size = get_pixel_size();
	Vector2 vertices[4] = {
		(final_rect.position + Vector2(0, final_rect.size.y)) * px_size,
		(final_rect.position + final_rect.size) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, 0)) * px_size,
		final_rect.position * px_size,
	};

	int x_axis = ((axis + 1) % 3);
	int y_axis = ((axis + 2) % 3);

	if (axis != Vector3::AXIS_Z) {
		SWAP(x_axis, y_axis);
		for (int i = 0; i < 4; i++) {
			if (axis == Vector3::AXIS_Y) {
				vertices[i].y = -vertices[i].y;
			}
			else if (axis == Vector3::AXIS_X) {
				vertices[i].x = -vertices[i].x;
			}
		}
	}

	static const int indices[6] = {0, 1, 2, 0, 2, 3};
	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vector3 vtx;
		vtx[x_axis] = vertices[i][0];
		vtx[y_axis] = vertices[i][1];
		facesw[j] = vtx;
	}

	triangle_mesh.instantiate();
	triangle_mesh->create(faces);
	return triangle_mesh;
}

void Sprite3D::set_draw_flag(DrawFlags p_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	if (flags[p_flag] == p_enable) {
		return;
	}
	flags[p_flag] = p_enable;
	_queue_redraw();
}

bool Sprite3D::get_draw_flag(DrawFlags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void Sprite3D::set_alpha_cut_mode(AlphaCutMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, ALPHA_CUT_MAX);
	if (alpha_cut == p_mode) {
		return;
	}
	alpha_cut = p_mode;
	_queue_redraw();
}

Sprite3D::AlphaCutMode Sprite3D::get_alpha_cut_mode() const { return alpha_cut; }

void Sprite3D::set_alpha_hash_scale(float p_hash_scale)
{
	if (alpha_hash_scale == p_hash_scale) {
		return;
	}
	alpha_hash_scale = p_hash_scale;
	_queue_redraw();
}

float Sprite3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

void Sprite3D::set_alpha_scissor_threshold(float p_threshold)
{
	if (alpha_scissor_threshold == p_threshold) {
		return;
	}
	alpha_scissor_threshold = p_threshold;
	_queue_redraw();
}

float Sprite3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void Sprite3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa)
{
	if (alpha_antialiasing_mode == p_alpha_aa) {
		return;
	}
	alpha_antialiasing_mode = p_alpha_aa;
	_queue_redraw();
}

StandardMaterial3D::AlphaAntiAliasing Sprite3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void Sprite3D::set_alpha_antialiasing_edge(float p_edge)
{
	if (alpha_antialiasing_edge == p_edge) {
		return;
	}
	alpha_antialiasing_edge = p_edge;
	_queue_redraw();
}

float Sprite3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

void Sprite3D::set_billboard_mode(StandardMaterial3D::BillboardMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, 3);
	if (billboard_mode == p_mode) {
		return;
	}
	billboard_mode = p_mode;
	_queue_redraw();
}

StandardMaterial3D::BillboardMode Sprite3D::get_billboard_mode() const { return billboard_mode; }

void Sprite3D::set_texture_filter(StandardMaterial3D::TextureFilter p_filter)
{
	if (texture_filter == p_filter) {
		return;
	}
	texture_filter = p_filter;
	_queue_redraw();
}

StandardMaterial3D::TextureFilter Sprite3D::get_texture_filter() const { return texture_filter; }

void Sprite3D::_draw()
{
	if (get_base() != mesh) {
		set_base(mesh);
	}

	if (frames.is_valid()) {
		if (!frames->has_animation(animation)) {
			return;
		}
		Ref<Texture2D> frame_tex = frames->get_frame_texture(animation, frame);
		if (frame_tex.is_null()) {
			set_base(RID());
			return;
		}
		Size2 tsize = frame_tex->get_size();
		if (tsize.x == 0 || tsize.y == 0) {
			return;
		}
		Point2 ofs = offset;
		if (centered) {
			ofs -= tsize / 2;
		}
		draw_texture_rect(frame_tex, Rect2(ofs, tsize), Rect2(Point2(), tsize));
		return;
	}

	if (texture.is_null()) {
		set_base(RID());
		return;
	}
	Vector2 tsize = texture->get_size();
	if (tsize.x == 0 || tsize.y == 0) {
		return;
	}

	Rect2 base_rect =
		region ? region_rect : Rect2(0, 0, texture->get_width(), texture->get_height());
	Size2 frame_size = base_rect.size / Size2(hframes, vframes);
	Point2 frame_offset = Point2(frame % hframes, frame / hframes) * frame_size;

	Point2 dst_offset = offset;
	if (centered) {
		dst_offset -= frame_size / 2.0f;
	}

	draw_texture_rect(texture, Rect2(dst_offset, frame_size),
		Rect2(base_rect.position + frame_offset, frame_size));
}

void Sprite3D::set_texture(const Ref<Texture2D>& p_texture)
{
	if (texture == p_texture) {
		return;
	}
	texture = p_texture;
	triangle_mesh.unref();
	_queue_redraw();
}

Ref<Texture2D> Sprite3D::get_texture() const { return texture; }

void Sprite3D::set_region_enabled(bool p_region)
{
	if (region == p_region) {
		return;
	}
	region = p_region;
	_queue_redraw();
}

bool Sprite3D::is_region_enabled() const { return region; }

void Sprite3D::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}
	region_rect = p_region_rect;
	if (region) {
		_queue_redraw();
	}
}

Rect2 Sprite3D::get_region_rect() const { return region_rect; }

void Sprite3D::set_frame(int p_frame)
{
	if (frames.is_valid()) {
		if (!frames->has_animation(animation)) {
			return;
		}
		int limit = frames->get_frame_count(animation);
		p_frame = CLAMP(p_frame, 0, MAX(0, limit - 1));
	}
	else {
		ERR_FAIL_INDEX(p_frame, vframes * hframes);
	}
	if (frame == p_frame) {
		return;
	}
	frame = p_frame;
	_calc_frame_speed_scale();
	_queue_redraw();
}

int Sprite3D::get_frame() const { return frame; }

void Sprite3D::set_frame_coords(const Vector2i& p_coord)
{
	ERR_FAIL_INDEX(p_coord.x, hframes);
	ERR_FAIL_INDEX(p_coord.y, vframes);
	set_frame(p_coord.y * hframes + p_coord.x);
}

Vector2i Sprite3D::get_frame_coords() const { return Vector2i(frame % hframes, frame / hframes); }

void Sprite3D::set_vframes(int p_amount)
{
	ERR_FAIL_COND(p_amount < 1);
	vframes = p_amount;
	_queue_redraw();
}

int Sprite3D::get_vframes() const { return vframes; }

void Sprite3D::set_hframes(int p_amount)
{
	ERR_FAIL_COND(p_amount < 1);
	hframes = p_amount;
	_queue_redraw();
}

int Sprite3D::get_hframes() const { return hframes; }

void Sprite3D::set_sprite_frames(const Ref<SpriteFrames>& p_frames)
{
	frames = p_frames;
	triangle_mesh.unref();
	_queue_redraw();
}

Ref<SpriteFrames> Sprite3D::get_sprite_frames() const { return frames; }

void Sprite3D::play(const String& p_name, float p_custom_scale, bool p_from_end)
{
	if (!p_name.is_empty()) {
		animation = p_name;
	}
	custom_speed_scale = p_custom_scale;
	playing = true;
	if (p_from_end && frames.is_valid() && frames->has_animation(animation)) {
		frame = MAX(0, frames->get_frame_count(animation) - 1);
	}
	_calc_frame_speed_scale();
	_queue_redraw();
}

void Sprite3D::play_backwards(const String& p_name) { play(p_name, -1.0f, true); }

void Sprite3D::pause() { _stop_internal(false); }

void Sprite3D::stop() { _stop_internal(true); }

bool Sprite3D::is_playing() const { return playing; }

void Sprite3D::set_animation(const String& p_name)
{
	if (animation == p_name) {
		return;
	}
	animation = p_name;
	frame = 0;
	_calc_frame_speed_scale();
	_queue_redraw();
}

String Sprite3D::get_animation() const { return animation; }

void Sprite3D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}
	autoplay = p_name;
}

String Sprite3D::get_autoplay() const { return autoplay; }

void Sprite3D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t Sprite3D::get_frame_progress() const { return frame_progress; }

void Sprite3D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float Sprite3D::get_speed_scale() const { return speed_scale; }

float Sprite3D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

double Sprite3D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void Sprite3D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

void Sprite3D::_stop_internal(bool p_reset)
{
	playing = false;
	if (p_reset) {
		frame = 0;
		frame_progress = 0.0;
		_queue_redraw();
	}
}

Rect2 Sprite3D::get_item_rect() const
{
	if (frames.is_valid()) {
		if (!frames->has_animation(animation) || frame < 0 ||
			frame >= frames->get_frame_count(animation)) {
			return Rect2(0, 0, 1, 1);
		}
		Ref<Texture2D> t = !animation.is_empty() ? frames->get_frame_texture(animation, frame)
												 : Ref<Texture2D>();
		if (t.is_null()) {
			return Rect2(0, 0, 1, 1);
		}
		Size2 s = t->get_size();
		Point2 ofs = offset;
		if (centered) {
			ofs -= s / 2;
		}
		if (s == Size2(0, 0)) {
			s = Size2(1, 1);
		}
		return Rect2(ofs, s);
	}

	if (texture.is_null()) {
		return Rect2(0, 0, 1, 1);
	}

	Size2 s = region ? region_rect.size : (texture->get_size() / Point2(hframes, vframes));
	Point2 ofs = offset;
	if (centered) {
		ofs -= s / 2;
	}
	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}
	return Rect2(ofs, s);
}

PackedStringArray Sprite3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);
	return warnings;
}

#ifdef TOOLS_ENABLED
void Sprite3D::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && frames.is_valid()) {
		if (pf == "play" || pf == "play_backwards" || pf == "set_animation" ||
			pf == "set_autoplay") {
			List<String> al;
			frames->get_animation_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node3D::get_argument_options(p_function, p_idx, r_options);
}
#endif

void Sprite3D::draw_texture_rect(Ref<Texture2D>, Rect2, Rect2) {}

Sprite3D::Sprite3D() {}

Sprite3D::~Sprite3D()
{
	if (!Renderer::data) {
		return;
	}
	if (mesh.is_valid()) {
		Renderer::free_rid(mesh);
	}
	if (material.is_valid()) {
		Renderer::free_rid(material);
	}
}

// ==========================================================================
// Merged from scene/3d/label_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/triangle_mesh.h"
#include "label_3d.h"
#include "scene/main/window.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/renderer.h"

AABB Label3D::get_aabb() const { return aabb; }

Ref<TriangleMesh> Label3D::generate_triangle_mesh() const
{
	if (triangle_mesh.is_valid()) {
		return triangle_mesh;
	}

	Ref<Font> font = _get_font_or_default();
	if (font.is_null()) {
		return Ref<TriangleMesh>();
	}

	Vector<Vector3> faces;
	faces.resize(6);
	Vector3* facesw = faces.ptrw();

	float total_h = 0.0;
	float max_line_w = 0.0;
	for (int i = 0; i < lines_rid.size(); i++) {
		total_h += TS->shaped_text_get_size(lines_rid[i]).y + line_spacing;
		max_line_w = MAX(max_line_w, TS->shaped_text_get_width(lines_rid[i]));
	}

	float vbegin = 0;
	switch (vertical_alignment) {
	case VERTICAL_ALIGNMENT_FILL:
	case VERTICAL_ALIGNMENT_TOP: {
		// Nothing.
	} break;
	case VERTICAL_ALIGNMENT_CENTER: {
		vbegin = (total_h - line_spacing) / 2.0;
	} break;
	case VERTICAL_ALIGNMENT_BOTTOM: {
		vbegin = (total_h - line_spacing);
	} break;
	}

	Vector2 offset = Vector2(0, vbegin);
	switch (horizontal_alignment) {
	case HORIZONTAL_ALIGNMENT_LEFT:
		break;
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_CENTER: {
		offset.x = -max_line_w / 2.0;
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		offset.x = -max_line_w;
	} break;
	}

	Rect2 final_rect = Rect2(offset + lbl_offset, Size2(max_line_w, total_h));

	if (final_rect.size.x == 0 || final_rect.size.y == 0) {
		return Ref<TriangleMesh>();
	}

	real_t px_size = get_pixel_size();

	Vector2 vertices[4] = {

		(final_rect.position + Vector2(0, -final_rect.size.y)) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, -final_rect.size.y)) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, 0)) * px_size,
		final_rect.position * px_size,

	};

	static const int indices[6] = {0, 1, 2, 0, 2, 3};

	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vector3 vtx;
		vtx[0] = vertices[i][0];
		vtx[1] = vertices[i][1];
		facesw[j] = vtx;
	}

	triangle_mesh.instantiate();
	triangle_mesh->create(faces);

	return triangle_mesh;
}

String Label3D::get_text() const { return text; }

void Label3D::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (horizontal_alignment != p_alignment) {
		if (horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL ||
			p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			dirty_lines = true; // Reshape lines.
		}
		horizontal_alignment = p_alignment;
		_queue_update();
	}
}

HorizontalAlignment Label3D::get_horizontal_alignment() const { return horizontal_alignment; }

void Label3D::set_vertical_alignment(VerticalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (vertical_alignment != p_alignment) {
		vertical_alignment = p_alignment;
		_queue_update();
	}
}

VerticalAlignment Label3D::get_vertical_alignment() const { return vertical_alignment; }

void Label3D::set_text_direction(TextServer::Direction p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		dirty_text = true;
		_queue_update();
	}
}

TextServer::Direction Label3D::get_text_direction() const { return text_direction; }

void Label3D::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		dirty_text = true;
		_queue_update();
	}
}

String Label3D::get_language() const { return language; }

void Label3D::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		st_parser = p_parser;
		dirty_text = true;
		_queue_update();
	}
}

TextServer::StructuredTextParser Label3D::get_structured_text_bidi_override() const
{
	return st_parser;
}

void Label3D::set_uppercase(bool p_uppercase)
{
	if (uppercase != p_uppercase) {
		uppercase = p_uppercase;
		dirty_text = true;
		_queue_update();
	}
}

bool Label3D::is_uppercase() const { return uppercase; }

void Label3D::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (render_priority != p_priority) {
		render_priority = p_priority;
		_queue_update();
	}
}

int Label3D::get_render_priority() const { return render_priority; }

void Label3D::set_outline_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (outline_render_priority != p_priority) {
		outline_render_priority = p_priority;
		_queue_update();
	}
}

int Label3D::get_outline_render_priority() const { return outline_render_priority; }

void Label3D::_font_changed()
{
	dirty_font = true;
	_queue_update();
}

Ref<Font> Label3D::get_font() const { return font_override; }

void Label3D::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = p_size;
		dirty_font = true;
		_queue_update();
	}
}

int Label3D::get_font_size() const { return font_size; }

void Label3D::set_outline_size(int p_size)
{
	if (outline_size != p_size) {
		outline_size = p_size;
		_queue_update();
	}
}

int Label3D::get_outline_size() const { return outline_size; }

void Label3D::set_modulate(const Color& p_color)
{
	if (modulate != p_color) {
		modulate = p_color;
		_queue_update();
	}
}

Color Label3D::get_modulate() const { return modulate; }

void Label3D::set_outline_modulate(const Color& p_color)
{
	if (outline_modulate != p_color) {
		outline_modulate = p_color;
		_queue_update();
	}
}

Color Label3D::get_outline_modulate() const { return outline_modulate; }

void Label3D::set_autowrap_mode(TextServer::AutowrapMode p_mode)
{
	if (autowrap_mode != p_mode) {
		autowrap_mode = p_mode;
		dirty_lines = true;
		_queue_update();
	}
}

TextServer::AutowrapMode Label3D::get_autowrap_mode() const { return autowrap_mode; }

void Label3D::set_autowrap_trim_flags(uint32_t p_flags)
{
	if (autowrap_flags_trim != (p_flags & TextServer::BREAK_TRIM_MASK)) {
		autowrap_flags_trim = (p_flags & TextServer::BREAK_TRIM_MASK);
		dirty_lines = true;
		_queue_update();
	}
}

uint32_t Label3D::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

void Label3D::set_justification_flags(uint32_t p_flags)
{
	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		dirty_lines = true;
		_queue_update();
	}
}

uint32_t Label3D::get_justification_flags() const { return jst_flags; }

void Label3D::set_width(float p_width)
{
	if (width != p_width) {
		width = p_width;
		dirty_lines = true;
		_queue_update();
	}
}

float Label3D::get_width() const { return width; }

void Label3D::set_pixel_size(real_t p_amount)
{
	if (pixel_size != p_amount) {
		pixel_size = p_amount;
		_queue_update();
	}
}

real_t Label3D::get_pixel_size() const { return pixel_size; }

void Label3D::set_offset(const Point2& p_offset)
{
	if (lbl_offset != p_offset) {
		lbl_offset = p_offset;
		_queue_update();
	}
}

Point2 Label3D::get_offset() const { return lbl_offset; }

void Label3D::set_line_spacing(float p_line_spacing)
{
	if (line_spacing != p_line_spacing) {
		line_spacing = p_line_spacing;
		_queue_update();
	}
}

float Label3D::get_line_spacing() const { return line_spacing; }

void Label3D::set_draw_flag(DrawFlags p_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	if (flags[p_flag] != p_enable) {
		flags[p_flag] = p_enable;
		_queue_update();
	}
}

bool Label3D::get_draw_flag(DrawFlags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void Label3D::set_billboard_mode(StandardMaterial3D::BillboardMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, 3);
	if (billboard_mode != p_mode) {
		billboard_mode = p_mode;
		_queue_update();
	}
}

StandardMaterial3D::BillboardMode Label3D::get_billboard_mode() const { return billboard_mode; }

void Label3D::set_texture_filter(StandardMaterial3D::TextureFilter p_filter)
{
	if (texture_filter != p_filter) {
		texture_filter = p_filter;
		_queue_update();
	}
}

StandardMaterial3D::TextureFilter Label3D::get_texture_filter() const { return texture_filter; }

Label3D::AlphaCutMode Label3D::get_alpha_cut_mode() const { return alpha_cut; }

void Label3D::set_alpha_hash_scale(float p_hash_scale)
{
	if (alpha_hash_scale != p_hash_scale) {
		alpha_hash_scale = p_hash_scale;
		_queue_update();
	}
}

float Label3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

void Label3D::set_alpha_scissor_threshold(float p_threshold)
{
	if (alpha_scissor_threshold != p_threshold) {
		alpha_scissor_threshold = p_threshold;
		_queue_update();
	}
}

float Label3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void Label3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa)
{
	if (alpha_antialiasing_mode != p_alpha_aa) {
		alpha_antialiasing_mode = p_alpha_aa;
		_queue_update();
	}
}

StandardMaterial3D::AlphaAntiAliasing Label3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void Label3D::set_alpha_antialiasing_edge(float p_edge)
{
	if (alpha_antialiasing_edge != p_edge) {
		alpha_antialiasing_edge = p_edge;
		_queue_update();
	}
}

float Label3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

Label3D::Label3D()
{
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = (i == FLAG_DOUBLE_SIDED);
	}

	text_rid = TS->create_shaped_text();

	mesh = Renderer::mesh_create();

	// Disable shadow casting by default to improve performance and avoid unintended visual
	// artifacts.
	render_instance.set_cast_shadows_setting(RenderInstance3D::SHADOW_CASTING_SETTING_OFF);

	// Label3D can't contribute to GI in any way, so disable it to improve performance.
	render_instance.set_gi_mode(RenderInstance3D::GI_MODE_DISABLED);

	set_base(mesh);
}

void Label3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void Label3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

Label3D::~Label3D()
{
	for (int i = 0; i < lines_rid.size(); i++) {
		TS->free_rid(lines_rid[i]);
	}
	lines_rid.clear();

	TS->free_rid(text_rid);

	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(mesh);
	for (KeyValue<SurfaceKey, SurfaceData> E : surfaces) {
		Renderer::free_rid(E.value.material);
	}
	surfaces.clear();
}

void Label3D::_queue_update() {}


Ref<Font> Label3D::_get_font_or_default() const { return Ref<Font>(); }

// ==========================================================================
// Merged from scene/3d/decal.cpp
// ==========================================================================

#include "core/os/os.h"
#include "decal.h"
#include "servers/rendering/renderer.h"

Vector3 Decal::get_size() const { return size; }

Ref<Texture2D> Decal::get_texture(DecalTexture p_type) const
{
	ERR_FAIL_INDEX_V(p_type, TEXTURE_MAX, Ref<Texture2D>());
	return textures[p_type];
}

void Decal::set_emission_energy(real_t p_energy)
{
	emission_energy = p_energy;
	Renderer::decal_set_emission_energy(decal, emission_energy);
}

real_t Decal::get_emission_energy() const { return emission_energy; }

void Decal::set_albedo_mix(real_t p_mix)
{
	albedo_mix = p_mix;
	Renderer::decal_set_albedo_mix(decal, albedo_mix);
}

real_t Decal::get_albedo_mix() const { return albedo_mix; }

void Decal::set_upper_fade(real_t p_fade)
{
	upper_fade = MAX(p_fade, 0.0);
	Renderer::decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_upper_fade() const { return upper_fade; }

void Decal::set_lower_fade(real_t p_fade)
{
	lower_fade = MAX(p_fade, 0.0);
	Renderer::decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_lower_fade() const { return lower_fade; }

void Decal::set_normal_fade(real_t p_fade)
{
	normal_fade = p_fade;
	Renderer::decal_set_normal_fade(decal, normal_fade);
}

real_t Decal::get_normal_fade() const { return normal_fade; }

void Decal::set_modulate(Color p_modulate)
{
	modulate = p_modulate;
	Renderer::decal_set_modulate(decal, p_modulate);
}

Color Decal::get_modulate() const { return modulate; }

bool Decal::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Decal::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	Renderer::decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_begin() const { return distance_fade_begin; }

void Decal::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	Renderer::decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_length() const { return distance_fade_length; }

uint32_t Decal::get_cull_mask() const { return cull_mask; }

AABB Decal::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

PackedStringArray Decal::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (OS::get_current_rendering_method() == "gl_compatibility" ||
		OS::get_current_rendering_method() == "dummy") {
		warnings.push_back(
			RTR("Decals are only available when using the Forward+ or Mobile renderer."));
		return warnings;
	}

	if (textures[TEXTURE_ALBEDO].is_null() && textures[TEXTURE_NORMAL].is_null() &&
		textures[TEXTURE_ORM].is_null() && textures[TEXTURE_EMISSION].is_null()) {
		warnings.push_back(RTR("The decal has no textures loaded into any of its texture "
							   "properties, and will therefore not be visible."));
	}

	if ((textures[TEXTURE_NORMAL].is_valid() || textures[TEXTURE_ORM].is_valid()) &&
		textures[TEXTURE_ALBEDO].is_null()) {
		warnings.push_back(
			RTR("The decal has a Normal and/or ORM texture, but no Albedo texture is set.\nAn "
				"Albedo texture with an alpha channel is required to blend the normal/ORM maps "
				"onto the underlying surface.\nIf you don't want the Albedo texture to be visible, "
				"set Albedo Mix to 0."));
	}

	if (cull_mask == 0) {
		warnings.push_back(RTR("The decal's Cull Mask has no bits enabled, which means the decal "
							   "will not paint objects on any layer.\nTo resolve this, enable at "
							   "least one bit in the Cull Mask property."));
	}

	return warnings;
}

void Decal::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void Decal::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

Decal::Decal()
{
	decal = Renderer::decal_create();
	set_base(decal);
}

Decal::~Decal()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(decal);
}

// ==========================================================================
// Merged from scene/3d/fog_volume.cpp
// ==========================================================================

#include "core/os/os.h"
#include "fog_volume.h"
#include "scene/main/viewport.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

Vector3 FogVolume::get_size() const { return size; }

RSE::FogVolumeShape FogVolume::get_shape() const { return shape; }

Ref<Material> FogVolume::get_material() const { return material; }

AABB FogVolume::get_aabb() const
{
	if (shape != RSE::FOG_VOLUME_SHAPE_WORLD) {
		return AABB(-size / 2, size);
	}
	return AABB();
}

PackedStringArray FogVolume::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	Ref<Environment> environment = get_viewport()->find_world_3d()->get_environment();

	if (OS::get_current_rendering_method() != "forward_plus") {
		warnings.push_back(RTR("Fog Volumes are only visible when using the Forward+ renderer."));
		return warnings;
	}

	if (environment.is_valid() && !environment->is_volumetric_fog_enabled()) {
		warnings.push_back(RTR("Fog Volumes need volumetric fog to be enabled in the scene's "
							   "Environment in order to be visible."));
	}

	return warnings;
}

void FogVolume::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void FogVolume::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

FogVolume::FogVolume()
{
	volume = Renderer::fog_volume_create();
	Renderer::fog_volume_set_shape(volume, RSE::FOG_VOLUME_SHAPE_BOX);
	set_base(volume);
}

FogVolume::~FogVolume()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(volume);
}

// ==========================================================================
// Merged from scene/3d/occluder_instance_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/marshalls.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_2d.h"
#include "occluder_instance_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/surface_tool.h"
#include "servers/rendering/renderer.h"

RID Occluder3D::get_rid() const { return occluder; }

void Occluder3D::_update()
{
	_update_arrays(vertices, indices);

	aabb = AABB();

	const Vector3* ptr = vertices.ptr();
	for (int i = 0; i < vertices.size(); i++) {
		aabb.expand_to(ptr[i]);
	}

	debug_lines.clear();
	debug_mesh.unref();

	Renderer::occluder_set_mesh(occluder, vertices, indices);
	emit_changed();
}

PackedVector3Array Occluder3D::get_vertices() const { return vertices; }

PackedInt32Array Occluder3D::get_indices() const { return indices; }

Vector<Vector3> Occluder3D::get_debug_lines() const
{
	if (!debug_lines.is_empty()) {
		return debug_lines;
	}

	if (indices.size() % 3 != 0) {
		return Vector<Vector3>();
	}

	const Vector3* vertices_ptr = vertices.ptr();
	debug_lines.resize(indices.size() / 3 * 6);
	Vector3* line_ptr = debug_lines.ptrw();
	int line_i = 0;
	for (int i = 0; i < indices.size() / 3; i++) {
		for (int j = 0; j < 3; j++) {
			int a = indices[i * 3 + j];
			int b = indices[i * 3 + (j + 1) % 3];
			ERR_FAIL_INDEX_V_MSG(
				a, vertices.size(), Vector<Vector3>(), "Occluder indices are out of range.");
			ERR_FAIL_INDEX_V_MSG(
				b, vertices.size(), Vector<Vector3>(), "Occluder indices are out of range.");
			line_ptr[line_i++] = vertices_ptr[a];
			line_ptr[line_i++] = vertices_ptr[b];
		}
	}
	return debug_lines;
}

AABB Occluder3D::get_aabb() const { return aabb; }

void Occluder3D::set_occluder_type(OccluderType p_type)
{
	if (occluder_type == p_type) {
		return;
	}
	occluder_type = p_type;
	_update();
}

Occluder3D::OccluderType Occluder3D::get_occluder_type() const { return occluder_type; }

void Occluder3D::set_arrays(PackedVector3Array p_vertices, PackedInt32Array p_indices)
{
	array_vertices = p_vertices;
	array_indices = p_indices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_vertices(PackedVector3Array p_vertices)
{
	array_vertices = p_vertices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_indices(PackedInt32Array p_indices)
{
	array_indices = p_indices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_quad_size(const Size2& p_size)
{
	if (quad_size == p_size) {
		return;
	}

	quad_size = p_size.maxf(0);
	if (occluder_type == OCCLUDER_QUAD) {
		_update();
	}
}

Size2 Occluder3D::get_quad_size() const { return quad_size; }

void Occluder3D::set_box_size(const Vector3& p_size)
{
	if (box_size == p_size) {
		return;
	}

	box_size = p_size.maxf(0);
	if (occluder_type == OCCLUDER_BOX) {
		_update();
	}
}

Vector3 Occluder3D::get_box_size() const { return box_size; }

void Occluder3D::set_radius(float p_radius)
{
	if (radius == p_radius) {
		return;
	}

	radius = MAX(p_radius, 0.0f);
	if (occluder_type == OCCLUDER_SPHERE) {
		_update();
	}
}

float Occluder3D::get_radius() const { return radius; }

void Occluder3D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	if (occluder_type == OCCLUDER_POLYGON) {
		_update();
	}
}

Vector<Vector2> Occluder3D::get_polygon() const { return polygon; }

bool Occluder3D::_has_editable_3d_polygon_no_depth() const { return false; }

void Occluder3D::_update_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices)
{
	switch (occluder_type) {
	case OCCLUDER_ARRAY: {
		r_vertices = array_vertices;
		r_indices = array_indices;
	} break;
	case OCCLUDER_QUAD: {
		Size2 _size = Size2(quad_size.x / 2.0f, quad_size.y / 2.0f);

		r_vertices = {
			Vector3(-_size.x, -_size.y, 0),
			Vector3(-_size.x, _size.y, 0),
			Vector3(_size.x, _size.y, 0),
			Vector3(_size.x, -_size.y, 0),
		};

		r_indices = {0, 1, 2, 0, 2, 3};
	} break;
	case OCCLUDER_BOX: {
		Vector3 _size = Vector3(box_size.x / 2.0f, box_size.y / 2.0f, box_size.z / 2.0f);

		r_vertices = {
			// front
			Vector3(-_size.x, -_size.y, _size.z),
			Vector3(_size.x, -_size.y, _size.z),
			Vector3(_size.x, _size.y, _size.z),
			Vector3(-_size.x, _size.y, _size.z),
			// back
			Vector3(-_size.x, -_size.y, -_size.z),
			Vector3(_size.x, -_size.y, -_size.z),
			Vector3(_size.x, _size.y, -_size.z),
			Vector3(-_size.x, _size.y, -_size.z),
		};

		r_indices = {// front
			0, 1, 2, 2, 3, 0,
			// right
			1, 5, 6, 6, 2, 1,
			// back
			7, 6, 5, 5, 4, 7,
			// left
			4, 0, 3, 3, 7, 4,
			// bottom
			4, 5, 1, 1, 0, 4,
			// top
			3, 2, 6, 6, 7, 3};
	} break;
	case OCCLUDER_SPHERE: {
		r_vertices.resize((RINGS + 2) * (RADIAL_SEGMENTS + 1));
		int vertex_i = 0;
		Vector3* vertex_ptr = r_vertices.ptrw();

		r_indices.resize((RINGS + 1) * RADIAL_SEGMENTS * 6);
		int idx_i = 0;
		int* idx_ptr = r_indices.ptrw();

		int current_row = 0;
		int previous_row = 0;
		int point = 0;
		for (int j = 0; j <= (RINGS + 1); j++) {
			float v = j / float(RINGS + 1);
			float w = Math::sin(Math::PI * v);
			float y = Math::cos(Math::PI * v);
			for (int i = 0; i <= RADIAL_SEGMENTS; i++) {
				float u = i / float(RADIAL_SEGMENTS);

				float x = Math::cos(u * Math::TAU);
				float z = Math::sin(u * Math::TAU);
				vertex_ptr[vertex_i++] = Vector3(x * w, y, z * w) * radius;

				if (i > 0 && j > 0) {
					idx_ptr[idx_i++] = previous_row + i - 1;
					idx_ptr[idx_i++] = previous_row + i;
					idx_ptr[idx_i++] = current_row + i - 1;

					idx_ptr[idx_i++] = previous_row + i;
					idx_ptr[idx_i++] = current_row + i;
					idx_ptr[idx_i++] = current_row + i - 1;
				}

				point++;
			}

			previous_row = current_row;
			current_row = point;
		}
	} break;
	case OCCLUDER_POLYGON: {
		if (polygon.size() < 3) {
			r_vertices.clear();
			r_indices.clear();
			return;
		}

		Vector<Point2> occluder_polygon = polygon;
		if (Triangulate::get_area(occluder_polygon) > 0) {
			occluder_polygon.reverse();
		}

		Vector<int> occluder_indices = Geometry2D::triangulate_polygon(occluder_polygon);

		if (occluder_indices.size() < 3) {
			r_vertices.clear();
			r_indices.clear();
			ERR_FAIL_MSG("Failed to triangulate Occluder3D. Make sure the polygon doesn't have "
						 "any intersecting edges.");
		}

		r_vertices.resize(occluder_polygon.size());
		Vector3* vertex_ptr = r_vertices.ptrw();
		const Vector2* polygon_ptr = occluder_polygon.ptr();
		for (int i = 0; i < occluder_polygon.size(); i++) {
			vertex_ptr[i] = Vector3(polygon_ptr[i].x, polygon_ptr[i].y, 0.0);
		}

		r_indices.resize(occluder_indices.size());
		memcpy(r_indices.ptrw(), occluder_indices.ptr(), occluder_indices.size() * sizeof(int));
	} break;
	}
}

Occluder3D::Occluder3D(OccluderType p_type) : occluder_type(p_type)
{
	occluder = Renderer::occluder_create();
	if (occluder_type != OCCLUDER_ARRAY && occluder_type != OCCLUDER_POLYGON) {
		_update();
	}
}

Occluder3D::~Occluder3D()
{
	if (occluder.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(occluder);
	}
}

/////////////////////////////////////////////////

AABB OccluderInstance3D::get_aabb() const
{
	if (occluder.is_valid()) {
		return occluder->get_aabb();
	}
	return AABB();
}

Ref<Occluder3D> OccluderInstance3D::get_occluder() const { return occluder; }

uint32_t OccluderInstance3D::get_bake_mask() const { return bake_mask; }

void OccluderInstance3D::set_bake_simplification_distance(float p_dist)
{
	bake_simplification_dist = MAX(p_dist, 0.0f);
}

float OccluderInstance3D::get_bake_simplification_distance() const
{
	return bake_simplification_dist;
}

bool OccluderInstance3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return bake_mask & (1 << (p_layer_number - 1));
}

OccluderInstance3D::BakeError OccluderInstance3D::bake_scene(
	Node* p_from_node, String p_occluder_path)
{
	if (p_occluder_path.is_empty()) {
		if (get_occluder().is_null()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}
		p_occluder_path = get_occluder()->get_path();
		if (!p_occluder_path.is_resource_file()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}
	}

	PackedVector3Array vertices;
	PackedInt32Array indices;

	_bake_node(p_from_node, vertices, indices);

	if (vertices.is_empty() || indices.is_empty()) {
		return BAKE_ERROR_NO_MESHES;
	}

	Ref<Occluder3D> occ;
	if (get_occluder().is_valid()) {
		if (get_occluder()->get_occluder_type() == Occluder3D::OCCLUDER_ARRAY) {
			occ = get_occluder();
		}
		set_occluder(Ref<Occluder3D>()); // clear
	}

	if (occ.is_null()) {
		occ = memnew(Occluder3D(Occluder3D::OCCLUDER_ARRAY));
	}

	occ->set_arrays(vertices, indices);

	Error err = ResourceSaver::save(occ.ptr(), p_occluder_path);

	if (err != OK) {
		return BAKE_ERROR_CANT_SAVE;
	}

	occ->set_path(p_occluder_path);
	set_occluder(occ);

	return BAKE_ERROR_OK;
}

PackedStringArray OccluderInstance3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!bool(GLOBAL_GET_CACHED(bool, "rendering/occlusion_culling/use_occlusion_culling"))) {
		warnings.push_back(RTR(
			"Occlusion culling is disabled in the Project Settings, which means occlusion culling "
			"won't be performed in the root viewport.\nTo resolve this, open the Project Settings "
			"and enable Rendering > Occlusion Culling > Use Occlusion Culling."));
	}

	if (bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any occluder meshes for this OccluderInstance3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	if (occluder.is_null()) {
		warnings.push_back(
			RTR("No occluder mesh is defined in the Occluder property, so no occlusion culling "
				"will be performed using this OccluderInstance3D.\nTo resolve this, set the "
				"Occluder property to one of the primitive occluder types or bake the scene meshes "
				"by selecting the OccluderInstance3D and pressing the Bake Occluders button at the "
				"top of the 3D editor viewport."));
	}
	else {
		if (occluder->get_occluder_type() == Occluder3D::OCCLUDER_ARRAY &&
			occluder->get_indices().size() < 3) {
			// Setting a new array Occluder3D from the inspector will create an empty occluder,
			// so warn the user about this.
			warnings.push_back(
				RTR("The occluder mesh has less than 3 vertices, so no occlusion culling will be "
					"performed using this OccluderInstance3D.\nTo generate a proper occluder mesh, "
					"select the OccluderInstance3D then use the Bake Occluders button at the top "
					"of the 3D editor viewport."));
		}
		if (occluder->get_occluder_type() == Occluder3D::OCCLUDER_POLYGON &&
			occluder->get_polygon().size() < 3) {
			warnings.push_back(RTR(
				"The polygon occluder has less than 3 vertices, so no occlusion culling will be "
				"performed using this OccluderInstance3D.\nVertices can be added in the inspector "
				"or using the polygon editing tools at the top of the 3D editor viewport."));
		}
	}

	return warnings;
}

bool OccluderInstance3D::_is_editable_3d_polygon() const
{
	return occluder.is_valid() && occluder->get_occluder_type() == Occluder3D::OCCLUDER_POLYGON;
}

Ref<Resource> OccluderInstance3D::_get_editable_3d_polygon_resource() const { return occluder; }


void OccluderInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void OccluderInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

OccluderInstance3D::OccluderInstance3D() {}

OccluderInstance3D::~OccluderInstance3D() {}



void OccluderInstance3D::set_occluder(Ref<Occluder3D> const&) {}

void OccluderInstance3D::_bake_node(Node*, Vector<Vector3>&, Vector<int>&) {}

// ==========================================================================
// Merged from scene/3d/visible_on_screen_notifier_3d.cpp
// ==========================================================================

#include "visible_on_screen_notifier_3d.h"

#include "core/config/engine.h"
#include "servers/rendering/renderer.h"

void VisibleOnScreenNotifier3D::set_aabb(const AABB& p_aabb) { aabb = p_aabb; }

AABB VisibleOnScreenNotifier3D::get_aabb() const { return aabb; }

bool VisibleOnScreenNotifier3D::is_on_screen() const { return on_screen; }

void VisibleOnScreenNotifier3D::set_enabler_enabled(bool p_enabled)
{
	enabler_enabled = p_enabled;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier3D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier3D::set_enable_node_path(const NodePath& p_path)
{
	enable_node_path = p_path;
}

void VisibleOnScreenNotifier3D::_visibility_enter()
{
	on_screen = true;
	if (enabler_enabled) {
		_update_enable_mode(true);
	}
}

void VisibleOnScreenNotifier3D::_visibility_exit()
{
	on_screen = false;
	if (enabler_enabled) {
		_update_enable_mode(false);
	}
}

void VisibleOnScreenNotifier3D::_update_enable_mode(bool p_enable)
{
	if (Engine::is_editor_hint()) {
		return;
	}
	Node* node = get_node_or_null(enable_node_path);
	if (!node) {
		return;
	}
	if (p_enable) {
		switch (enable_mode) {
		case ENABLE_MODE_INHERIT:
			node->set_process_mode(PROCESS_MODE_INHERIT);
			break;
		case ENABLE_MODE_ALWAYS:
			node->set_process_mode(PROCESS_MODE_ALWAYS);
			break;
		case ENABLE_MODE_WHEN_PAUSED:
			node->set_process_mode(PROCESS_MODE_WHEN_PAUSED);
			break;
		}
	}
	else {
		node->set_process_mode(PROCESS_MODE_DISABLED);
	}
}

void VisibleOnScreenNotifier3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void VisibleOnScreenNotifier3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
	case NOTIFICATION_EXIT_TREE: {
		on_screen = false;
	} break;
	}
}

VisibleOnScreenNotifier3D::VisibleOnScreenNotifier3D(bool p_enabler) : enabler_enabled(p_enabler) {}

VisibleOnScreenNotifier3D::~VisibleOnScreenNotifier3D()
{
	RID base_old = get_base();
	set_base(RID());
	if (base_old.is_valid() && Renderer::data) {
		Renderer::free_rid(base_old);
	}
}
